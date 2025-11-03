
#include "utils.hpp"
#include "globals.hpp"
#include "shared_layout.hpp"
#include <iostream>
#include <cstring>   

// Consome 1 item de L1 respeitando RUN/PAUSE e QUIT
static bool pull_L1_blocking(const char* consumidor_tag, HANDLE evtRun) {
    for (;;) {
        // 0) Se PAUSADO: espera RUN ou QUIT
        if (WaitForSingleObject(evtRun, 0) != WAIT_OBJECT_0) {
            HANDLE hs_run[2] = { atr::g_evtQuitAll, evtRun };
            DWORD wr = WaitForMultipleObjects(2, hs_run, FALSE, INFINITE);
            if (wr == WAIT_OBJECT_0) return false; // QUIT
            // RUN → prossegue
        }

        HANDLE hs[2] = { atr::g_evtQuitAll, atr::g_semItems_L1 };

        // 1) Tenta pegar item sem bloquear
        DWORD now = WaitForSingleObject(atr::g_semItems_L1, 0);
        if (now == WAIT_TIMEOUT) {
            // Lista vazia: espera ITEM ou QUIT
            DWORD r = WaitForMultipleObjects(2, hs, FALSE, INFINITE);
            if (r == WAIT_OBJECT_0) return false;         // QUIT
            if (r != WAIT_OBJECT_0 + 1) {
                atr::log_error(consumidor_tag, "Falha ao aguardar item/quit (WFMO).");
                return false;
            }
            // r == WAIT_OBJECT_0+1 → item consumido do semáforo
        }
        else if (now != WAIT_OBJECT_0) {
            atr::log_error(consumidor_tag, "Falha ao aguardar item (WFSO 0ms).");
            return false;
        }
        // Chegou aqui COM 1 ITEM garantido

        // 2) Se PAUSOU exatamente agora, devolve item e espera RUN/QUIT
        if (WaitForSingleObject(evtRun, 0) != WAIT_OBJECT_0) {
            ReleaseSemaphore(atr::g_semItems_L1, 1, nullptr);  // devolve item
            HANDLE hs_run[2] = { atr::g_evtQuitAll, evtRun };
            DWORD wr2 = WaitForMultipleObjects(2, hs_run, FALSE, INFINITE);
            if (wr2 == WAIT_OBJECT_0) return false;           // QUIT durante pausa
            continue;                                          // RUN → tenta de novo
        }

        // 3) Entra na região crítica: leitura e avanço do tail
        DWORD mw = WaitForSingleObject(atr::mutexL1, INFINITE);
        if (mw != WAIT_OBJECT_0) {
            atr::log_error(consumidor_tag, "Falha ao adquirir mutexL1.");
            ReleaseSemaphore(atr::g_semItems_L1, 1, nullptr);  // devolve item
            return false;
        }

        const LONG t = atr::g_B1->hdr.tail;
        BYTE* src = atr::slot_ptr(atr::g_B1, t);
        const size_t cap = size_t(atr::g_B1->hdr.msg_size);

        // Leitura segura (produtor garante '\0' até cap-1)
        const char* csrc = reinterpret_cast<const char*>(src);
        size_t n = 0;
        n = ::strnlen(csrc, cap - 1);
        std::string msg(csrc, n);
        const LONG idx_fisico = t % atr::g_B1->hdr.capacity;

        // Avança tail lógico e sai da RC
        atr::g_B1->hdr.tail = t + 1;
        ReleaseMutex(atr::mutexL1);

        // 4) Libera 1 vaga no semáforo de espaços
        ReleaseSemaphore(atr::g_semSpaces_L1, 1, nullptr);

        // Log simples do consumo
        std::cout << "[consumidor " << consumidor_tag << "] "
            << "L1 pull idx=" << t
            << " (fis=" << idx_fisico << "), bytes=" << n
            << ", msg=\"" << msg << "\"\n";

        // 5) Roteamento básico por prefixo
        if (msg.rfind("44/", 0) == 0) {
            // MSG44 → Exibição (Pipe na etapa 2)
            atr::log_info(consumidor_tag, "roteado: MSG44 para exibicao");
            // TODO: enviar para exibicao
        }
        else if (msg.rfind("11/", 0) == 0) {
            // MSG11 → L2 (fila de análise)
            atr::log_info(consumidor_tag, "roteado: MSG11 para L2");
            // TODO: push em L2 (semáforos/mutex de L2)
        }
        else {
            atr::log_warn(consumidor_tag, "Formato desconhecido (nem 11/ nem 44/)");
        }

        // Continua consumindo até QUIT
    }
}

// Thread que consome mensagens de L1 (captura)
DWORD WINAPI thr_msg_capture(LPVOID) {
    std::cout << "Thread captura iniciada com sucesso!" << std::endl;

    for (;;) {
        // Sai se QUIT
        if (WaitForSingleObject(atr::g_evtQuitAll, 0) == WAIT_OBJECT_0) break;

        // Pausa: espera RUN ou QUIT
        if (WaitForSingleObject(atr::g_evtRunCaptura, 0) != WAIT_OBJECT_0) {
            HANDLE hs[2] = { atr::g_evtQuitAll, atr::g_evtRunCaptura };
            DWORD r = WaitForMultipleObjects(2, hs, FALSE, INFINITE);
            if (r == WAIT_OBJECT_0) break; // QUIT
        }

        // Consome 1 item (função já trata PAUSE/QUIT)
        if (!pull_L1_blocking("captura", atr::g_evtRunCaptura)) break;
    }

    atr::log_info("captura", "Encerrando thread de captura.");
    return 0;
}

int main() {
    atr::open_child_kernels(); // abre/cria objetos kernel compartilhados

    HANDLE hCapture = CreateThread(nullptr, 0, thr_msg_capture, nullptr, 0, nullptr);
    DWORD n = 0;
    if (hCapture) n++;
    if (n > 0) {
        WaitForSingleObject(hCapture, INFINITE); // aguarda fim da thread
    }
    if (hCapture) CloseHandle(hCapture); // fecha handle
    atr::close_child_kernels(); // fecha objetos kernel
    return 0;
}
