// consumidor.cpp
#include "utils.hpp"
#include "globals.hpp"
#include "shared_layout.hpp"
#include <iostream>
#include <cstring>   // strnlen/memcpy (MSVC tem strnlen)

static bool pull_L1_blocking(const char* consumidor_tag, HANDLE evtRun) {
    for (;;) {
        // 0) Se estiver PAUSADO, espera RUN ou QUIT
        if (WaitForSingleObject(evtRun, 0) != WAIT_OBJECT_0) {
            HANDLE hs_run[2] = { atr::g_evtQuitAll, evtRun };
            DWORD wr = WaitForMultipleObjects(2, hs_run, FALSE, INFINITE);
            if (wr == WAIT_OBJECT_0) return false; // QUIT
            // RUN sinalizado -> continua
        }

        HANDLE hs[2] = { atr::g_evtQuitAll, atr::g_semItems_L1 };

        //Tenta non-blocking pegar item, tempo de espera 0 não fica bloqueado
        DWORD now = WaitForSingleObject(atr::g_semItems_L1, 0);
        if (now == WAIT_TIMEOUT) {
			// Normalmente lista está vazia aqui, pois a captura não é periódica enquanto a produção é.
            // atr::log_warn(consumidor_tag, "Lista L1 VAZIA — bloqueando até chegar item.");
            // Espera por ITEM ou QUIT
            DWORD r = WaitForMultipleObjects(2, hs, FALSE, INFINITE);
            if (r == WAIT_OBJECT_0) return false;         // QUIT
            if (r != WAIT_OBJECT_0 + 1) {
                atr::log_error(consumidor_tag, "Falha ao aguardar item/quit (WFMO).");
                return false;
            }
            // r == WAIT_OBJECT_0+1 -> acabamos de CONSUMIR um item do semáforo
        }
        else if (now != WAIT_OBJECT_0) {
            atr::log_error(consumidor_tag, "Falha ao aguardar item (WFSO 0ms).");
            return false;
        }
        // Chegamos aqui COM UM ITEM garantido (semáforo já decrementado)

        //Se PAUSOU exatamente agora, devolve o item e espera RUN/QUIT
        if (WaitForSingleObject(evtRun, 0) != WAIT_OBJECT_0) {
            ReleaseSemaphore(atr::g_semItems_L1, 1, nullptr);  // devolve o item
            HANDLE hs_run[2] = { atr::g_evtQuitAll, evtRun };
            DWORD wr2 = WaitForMultipleObjects(2, hs_run, FALSE, INFINITE);
            if (wr2 == WAIT_OBJECT_0) return false;           // QUIT durante pausa
            continue;                                          // RUN -> tenta de novo
        }

        //Entra na região crítica para ler e avançar o tail
        DWORD mw = WaitForSingleObject(atr::mutexL1, INFINITE);
        if (mw != WAIT_OBJECT_0) {
            atr::log_error(consumidor_tag, "Falha ao adquirir mutexL1.");
            // devolve o item ao semáforo para não perder
            ReleaseSemaphore(atr::g_semItems_L1, 1, nullptr);
            return false;
        }

        const LONG t = atr::g_B1->hdr.tail;
        BYTE* src = atr::slot_ptr(atr::g_B1, t);
        const size_t cap = size_t(atr::g_B1->hdr.msg_size);

        // Como o produtor zera o slot e copia no máx. cap-1, podemos confiar no '\0'.
        const char* csrc = reinterpret_cast<const char*>(src);
        size_t n = 0;
        n = ::strnlen(csrc, cap - 1);
        std::string msg(csrc, n);
        const LONG idx_fisico = t % atr::g_B1->hdr.capacity;
        // avança o tail lógico
        atr::g_B1->hdr.tail = t + 1;
        ReleaseMutex(atr::mutexL1);

        // 4) Libera uma VAGA no semáforo de espaços
        ReleaseSemaphore(atr::g_semSpaces_L1, 1, nullptr);

        // Testando se a mensagem foi consumida corretamente.
        std::cout << "[consumidor " << consumidor_tag << "] "
            << "L1 pull idx=" << t
            << " (fis=" << idx_fisico << "), bytes=" << n
            << ", msg=\"" << msg << "\"\n";

        
        if (msg.rfind("44/", 0) == 0) {
            // Captura 44 -> Exibição (envio por meio de Pipe na etapa 2)
            atr::log_info(consumidor_tag, "roteado: MSG44 para exibicao");
            // TODO: enviar para exibicao
        }
        else if (msg.rfind("11/", 0) == 0) {
            atr::log_info(consumidor_tag, "roteado: MSG11 para L2");
            // TODO: push em L2 (semáforos/mutex de L2)
        }
        else {
            atr::log_warn(consumidor_tag, "Formato desconhecido (nem 11/ nem 44/)");
        }

        // Continua consumindo até QUIT
    }
}

DWORD WINAPI thr_msg_capture(LPVOID) {
    std::cout << "Thread captura iniciada com sucesso!" << std::endl;

    for (;;) {
        // Sai imediatamente se QUIT já estiver sinalizado
        if (WaitForSingleObject(atr::g_evtQuitAll, 0) == WAIT_OBJECT_0) break;

        // Se estiver pausado, espera RUN ou QUIT
        if (WaitForSingleObject(atr::g_evtRunCaptura, 0) != WAIT_OBJECT_0) {
            HANDLE hs[2] = { atr::g_evtQuitAll, atr::g_evtRunCaptura };
            DWORD r = WaitForMultipleObjects(2, hs, FALSE, INFINITE);
            if (r == WAIT_OBJECT_0) break; // QUIT
        }

        // Tenta consumir 1 item (a função já respeita PAUSE/QUIT internamente)
        if (!pull_L1_blocking("captura", atr::g_evtRunCaptura)) break;
    }

    atr::log_info("captura", "Encerrando thread de captura.");
    return 0;
}
int main() {
	atr::open_child_kernels();
	HANDLE hCapture = CreateThread(nullptr, 0, thr_msg_capture, nullptr, 0, nullptr);
    DWORD n = 0;
    if (hCapture) n++;
    if (n > 0) {
        WaitForSingleObject(hCapture, INFINITE);
    }
    if (hCapture) CloseHandle(hCapture);
    atr::close_child_kernels();
    return 0;
}