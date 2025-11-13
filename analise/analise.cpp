#include "utils.hpp"
#include "globals.hpp"
#include "shared_layout.hpp"
#include <windows.h>
#include <iostream>
// Mensagens do tipo 11, L2 associada precisa apenas da visão do buffer 2
// Thread responsável por realizar a análise granulométrica
static bool pull_L2_blocking(const char* consumidor_tag, HANDLE evtRun) {
    for (;;) {
        // 0) Se PAUSADO: espera RUN ou QUIT
        if (WaitForSingleObject(evtRun, 0) != WAIT_OBJECT_0) {
            HANDLE hs_run[2] = { atr::g_evtQuitAll, evtRun };
            DWORD wr = WaitForMultipleObjects(2, hs_run, FALSE, INFINITE);
            if (wr == WAIT_OBJECT_0) return false; // QUIT
            // RUN → prossegue
        }
        HANDLE hs[2] = { atr::g_evtQuitAll, atr::g_semItems_L2 };

        // 1) Tenta pegar item sem bloquear
        DWORD now = WaitForSingleObject(atr::g_semItems_L2, 0);
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
            ReleaseSemaphore(atr::g_semItems_L2, 1, nullptr);  // devolve item
            HANDLE hs_run[2] = { atr::g_evtQuitAll, evtRun };
            DWORD wr2 = WaitForMultipleObjects(2, hs_run, FALSE, INFINITE);
            if (wr2 == WAIT_OBJECT_0) return false;           // QUIT durante pausa
            continue;                                          // RUN → tenta de novo
        }

        // 3) Entra na região crítica: leitura e avanço do tail
        DWORD mw = WaitForSingleObject(atr::mutexL1, INFINITE);
        if (mw != WAIT_OBJECT_0) {
            atr::log_error(consumidor_tag, "Falha ao adquirir mutexL1.");
            ReleaseSemaphore(atr::g_semItems_L2, 1, nullptr);  // devolve item
            return false;
        }

        const LONG t = atr::g_B2->hdr.tail;
        BYTE* src = atr::slot_ptr(atr::g_B2, t);
        const size_t cap = size_t(atr::g_B2->hdr.msg_size);

        // Leitura segura (produtor garante '\0' até cap-1)
        const char* csrc = reinterpret_cast<const char*>(src);
        size_t n = 0;
        n = ::strnlen(csrc, cap - 1);
        std::string msg(csrc, n);
        const LONG idx_fisico = t % atr::g_B2->hdr.capacity;

        // Avança tail lógico e sai da RC
        atr::g_B2->hdr.tail = t + 1;
        ReleaseMutex(atr::mutexL1);

        // 4) Libera 1 vaga no semáforo de espaços
        ReleaseSemaphore(atr::g_semSpaces_L2, 1, nullptr);

        // Log simples do consumo
        std::cout << "[consumidor " << consumidor_tag << "] "
            << "L2 pull idx=" << t
            << " (fis=" << idx_fisico << "), bytes=" << n
            << ", msg=\"" << msg << "\"\n";
    }
}
DWORD WINAPI thr_analise_granulometria(LPVOID) {
    std::cout << "Thread analise iniciada com sucesso!" << std::endl;

    for (;;) {
        // Sai se QUIT
        if (WaitForSingleObject(atr::g_evtQuitAll, 0) == WAIT_OBJECT_0) break;

        // Pausa: espera RUN ou QUIT
        if (WaitForSingleObject(atr::g_evtRunAnalise, 0) != WAIT_OBJECT_0) {
            HANDLE hs[2] = { atr::g_evtQuitAll, atr::g_evtRunAnalise };
            DWORD r = WaitForMultipleObjects(2, hs, FALSE, INFINITE);
            if (r == WAIT_OBJECT_0) break; // QUIT
        }

        // Consome 1 item (função já trata PAUSE/QUIT)
        if (!pull_L2_blocking("analise", atr::g_evtRunAnalise)) break;
    }

    atr::log_info("analise", "Encerrando thread de captura.");
    return 0;
}

int main() {
    atr::open_child_kernels(); // abre os objetos kernel compartilhados
    HANDLE h = CreateThread(nullptr, 0, thr_analise_granulometria, nullptr, 0, nullptr); // cria thread de análise
    if (h) WaitForSingleObject(h, INFINITE); // espera término da thread
    atr::close_child_kernels(); // fecha os objetos kernel
    if (h) CloseHandle(h); // libera o handle da thread
    return 0;
}
