
#include "messages.hpp"
#include "utils.hpp"
#include "globals.hpp"
#include <windows.h>
#include <thread>
#include <random>
#include <sstream>
#include <iostream>

// Empilha em L1 com aviso se estiver cheia
// Empilha em L1; retorna false se quit foi sinalizado (não empilhou)
// Empilha em L1; retorna false se QUIT foi sinalizado (não empilhou)
// Agora respeita on/off (evtRun) durante toda a execução.
static bool push_L1_blocking(const std::string& s, const char* produtor_tag, HANDLE evtRun) {
    for (;;) {
        // Se estiver PAUSADO, espera RUN ou QUIT
        if (WaitForSingleObject(evtRun, 0) != WAIT_OBJECT_0) {
            HANDLE hs_run[2] = { atr::g_evtQuitAll, evtRun };
            DWORD wr = WaitForMultipleObjects(2, hs_run, FALSE, INFINITE);
            if (wr == WAIT_OBJECT_0) return false;        // QUIT
            // wr == WAIT_OBJECT_0+1 -> RUN ficou sinalizado, continua
        }

        HANDLE hs[2] = { atr::g_evtQuitAll, atr::g_semSpaces_L1 };

        // Tenta non-blocking primeiro 
        DWORD now = WaitForSingleObject(atr::g_semSpaces_L1, 0);
        if (now == WAIT_TIMEOUT) {
            atr::log_warn(produtor_tag, "Lista L1 CHEIA — produtor vai bloquear até abrir vaga.");

            // Espera por VAGA OU QUIT
            DWORD r = WaitForMultipleObjects(2, hs, FALSE, INFINITE);
            if (r == WAIT_OBJECT_0) return false;                   // QUIT
            if (r != WAIT_OBJECT_0 + 1) {
                atr::log_error(produtor_tag, "Falha ao aguardar vaga/quit (WFMO).");
                return false;
            }
            // r == WAIT_OBJECT_0+1 -> acabamos de consumir uma vaga do semáforo
        }
        else if (now != WAIT_OBJECT_0) {
            atr::log_error(produtor_tag, "Falha ao tentar vaga em L1 (WFSO 0ms).");
            return false;
        }
        // Chegamos aqui com uma vaga garantida (o semáforo já foi decrementado)

        // Se PAUSOU exatamente agora, devolve a vaga e aguarda RUN/QUIT antes de tentar de novo
        if (WaitForSingleObject(evtRun, 0) != WAIT_OBJECT_0) {
            ReleaseSemaphore(atr::g_semSpaces_L1, 1, nullptr);      // devolve o espaço
            HANDLE hs_run[2] = { atr::g_evtQuitAll, evtRun };
            DWORD wr2 = WaitForMultipleObjects(2, hs_run, FALSE, INFINITE);
            if (wr2 == WAIT_OBJECT_0) return false;                 // QUIT durante a pausa
            // RUN sinalizado: volta ao topo do loop para tentar novamente
            continue;
        }

        // Espera pela mutex para conseguir acesso ao buffer
		WaitForSingleObject(atr::mutexL1, INFINITE);
        atr::g_buf_L1[atr::g_tail_L1] = s;
        std::cout << "Elemento adicionado na L1: " << atr::g_buf_L1[atr::g_tail_L1] << std::endl;
        atr::g_tail_L1 = (atr::g_tail_L1 + 1) % atr::g_cap_L1;
        ++atr::g_count_L1;
		ReleaseMutex(atr::mutexL1);

        // Um item a mais disponível
        ReleaseSemaphore(atr::g_semItems_L1, 1, nullptr);
        return true;
    }
}

// Implementar função com sleep na fase 1
// Espera um período em ms, mas sai cedo se quit_all for sinalizado
static bool wait_period_or_quit(DWORD period_ms) {
    // Espera com timeout; retorna false se recebeu quit
    DWORD r = WaitForSingleObject(atr::g_evtQuitAll, period_ms);
    return (r == WAIT_TIMEOUT);  // true => tempo passou (continua), false => quit
}

DWORD WINAPI thr_msg11(LPVOID) {
    // período aleatório: 1 a 5 s
    std::cout << "Thread msg 11 iniciada com sucesso!" << std::endl;

    std::mt19937 rng(GetTickCount64());
    std::uniform_int_distribution<int> dist_ms(1000, 5000);

    while (true) {
        // 1) Sai se QUIT já estiver sinalizado
        if (WaitForSingleObject(atr::g_evtQuitAll, 0) == WAIT_OBJECT_0) break;

        // 2) Se RUN (g_evtRunMedicao) não estiver sinalizado,programa está em pausa e espera RUN OU QUIT
        if (WaitForSingleObject(atr::g_evtRunMedicao, 0) != WAIT_OBJECT_0) {
            HANDLE hs[2] = { atr::g_evtQuitAll, atr::g_evtRunMedicao };
            DWORD r = WaitForMultipleObjects(2, hs, FALSE, INFINITE);
            if (r == WAIT_OBJECT_0) break;               // QUIT
            // r == WAIT_OBJECT_0 + 1 -> RUN sinalizado, continua
        }

        // 3) Produz e tenta empilhar (push_L1_blocking já respeita QUIT)
        auto m11 = atr::make_random_msg11(/*seed*/123, /*n*/2);
        if (!push_L1_blocking(m11.serialize_ascii(), "prod_11",atr::g_evtRunMedicao)) break;

        // 4) Espera o período, saindo cedo se QUIT for sinalizado
        DWORD T_ms = static_cast<DWORD>(dist_ms(rng));
        if (!wait_period_or_quit(T_ms)) break;  // recebeu quit
    }

    atr::log_info("prod_11", "Encerrando thread de mensagens 11.");
    return 0;
}


DWORD WINAPI thr_msg44(LPVOID) {
    std::cout << "Thread msg 44 iniciada com sucesso!" << std::endl;
    const DWORD T_ms = 500;

    while (true) {
        // 1) Sai imediatamente se QUIT já estiver sinalizado
        if (WaitForSingleObject(atr::g_evtQuitAll, 0) == WAIT_OBJECT_0) break;

        // 2) Se RUN (g_evtRunCLP) NÃO estiver sinalizado (pausado) espera RUN ou QUIT
        if (WaitForSingleObject(atr::g_evtRunCLP, 0) != WAIT_OBJECT_0) {
            HANDLE hs[2] = { atr::g_evtQuitAll, atr::g_evtRunCLP };
            DWORD r = WaitForMultipleObjects(2, hs, FALSE, INFINITE);
            if (r == WAIT_OBJECT_0) break; // QUIT
            // r == WAIT_OBJECT_0 + 1 -> RUN ficou sinalizado, continua
        }

        // 3) Produz e tenta empilhar (push_L1_blocking já trata QUIT)
        auto m44 = atr::make_random_msg44(/*seed*/456, /*n*/5);
        if (!push_L1_blocking(m44.serialize_ascii(), "prod_44",atr::g_evtRunCLP)) break;

        // 4) Espera o período, saindo cedo se QUIT for sinalizado
        if (!wait_period_or_quit(T_ms)) break;
    }

    atr::log_info("prod_44", "Encerrando thread de mensagens 44.");
    return 0;
}


int main() {
    atr::open_child_kernels();

    HANDLE h11 = CreateThread(nullptr, 0, thr_msg11, nullptr, 0, nullptr);
    HANDLE h44 = CreateThread(nullptr, 0, thr_msg44, nullptr, 0, nullptr);

    HANDLE hs[2] = { h11, h44 };
    // Espera pela sinalização dos 2 objetos definidos no vetor de handles durante tempo infinito
    WaitForMultipleObjects(2, hs, TRUE, INFINITE);
	CloseHandle(h11);
	CloseHandle(h44);
    atr::close_child_kernels();
    return 0;
}
