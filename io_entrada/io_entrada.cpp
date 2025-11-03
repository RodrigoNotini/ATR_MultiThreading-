#define NOMINMAX
#include "messages.hpp"
#include "utils.hpp"
#include "globals.hpp"
#include "shared_layout.hpp"
#include <windows.h>
#include <thread>
#include <random>
#include <sstream>
#include <iostream>
#include <algorithm>

// Empilha em L1 respeitando PAUSE/RUN e QUIT; retorna false se não empilhar (ex.: QUIT)
static bool push_L1_blocking(const std::string& s, const char* produtor_tag, HANDLE evtRun) {
    for (;;) {
        // Pausado: aguarda RUN ou QUIT
        if (WaitForSingleObject(evtRun, 0) != WAIT_OBJECT_0) {
            HANDLE hs_run[2] = { atr::g_evtQuitAll, evtRun };
            DWORD wr = WaitForMultipleObjects(2, hs_run, FALSE, INFINITE);
            if (wr == WAIT_OBJECT_0) return false; // QUIT
        }

        HANDLE hs[2] = { atr::g_evtQuitAll, atr::g_semSpaces_L1 };

        // Tenta pegar vaga sem bloquear
        DWORD now = WaitForSingleObject(atr::g_semSpaces_L1, 0);
        if (now == WAIT_TIMEOUT) {
            atr::log_warn(produtor_tag, "Lista L1 CHEIA — produtor vai bloquear até abrir vaga.");

            // Espera por vaga ou QUIT
            DWORD r = WaitForMultipleObjects(2, hs, FALSE, INFINITE);
            if (r == WAIT_OBJECT_0) return false; // QUIT
            if (r != WAIT_OBJECT_0 + 1) {
                atr::log_error(produtor_tag, "Falha ao aguardar vaga/quit (WFMO).");
                return false;
            }
        }
        else if (now != WAIT_OBJECT_0) {
            atr::log_error(produtor_tag, "Falha ao tentar vaga em L1 (WFSO 0ms).");
            return false;
        }
        // A partir daqui há vaga garantida (semáforo já decrementado)

        // Se pausou exatamente agora, devolve vaga e aguarda RUN/QUIT
        if (WaitForSingleObject(evtRun, 0) != WAIT_OBJECT_0) {
            ReleaseSemaphore(atr::g_semSpaces_L1, 1, nullptr);
            HANDLE hs_run[2] = { atr::g_evtQuitAll, evtRun };
            DWORD wr2 = WaitForMultipleObjects(2, hs_run, FALSE, INFINITE);
            if (wr2 == WAIT_OBJECT_0) return false; // QUIT
            continue; // RUN → tenta de novo
        }

        // Entra na região crítica para escrever no buffer
        WaitForSingleObject(atr::mutexL1, INFINITE);

        const LONG h = atr::g_B1->hdr.head;               // índice lógico atual
        BYTE* dst = atr::slot_ptr(atr::g_B1, h);          // ponteiro para slot físico
        const size_t cap = size_t(atr::g_B1->hdr.msg_size);

        // Zera o slot e copia até cap-1, garantindo '\0'
        memset(dst, 0, cap);
        const size_t n = std::min(s.size(), cap ? cap - 1 : 0);
        memcpy(dst, s.data(), n);

        const LONG idx_fisico = h % atr::g_B1->hdr.capacity;
        const char* pushed = reinterpret_cast<const char*>(dst);

        // Log do push (índices e bytes)
        std::cout << "[produtor " << produtor_tag << "] "
            << "L1 push idx=" << h
            << " (fis=" << idx_fisico << "), bytes=" << n
            << ", msg=\"" << std::string(pushed, n) << "\"\n";

        atr::g_B1->hdr.head = h + 1;                      // avança head lógico
        ReleaseMutex(atr::mutexL1);                       // sai da RC

        ReleaseSemaphore(atr::g_semItems_L1, 1, nullptr); // sinaliza item disponível
        return true;
    }
}

// Espera um período (ms) — Fase A: simples Sleep; não trata QUIT antecipado
static VOID wait_period_or_quit(DWORD period_ms) {
    Sleep(period_ms);
}

// Produtor de mensagens 11 (período aleatório 1–5s)
DWORD WINAPI thr_msg11(LPVOID) {
    std::cout << "Thread msg 11 iniciada com sucesso!" << std::endl;

    std::mt19937 rng(GetTickCount64());
    std::uniform_int_distribution<int> dist_ms(1000, 5000);

    while (true) {
        // Encerra se QUIT
        if (WaitForSingleObject(atr::g_evtQuitAll, 0) == WAIT_OBJECT_0) break;

        // Pausado: aguarda RUN ou QUIT
        if (WaitForSingleObject(atr::g_evtRunMedicao, 0) != WAIT_OBJECT_0) {
            HANDLE hs[2] = { atr::g_evtQuitAll, atr::g_evtRunMedicao };
            DWORD r = WaitForMultipleObjects(2, hs, FALSE, INFINITE);
            if (r == WAIT_OBJECT_0) break; // QUIT
        }

        // Produz e empilha
        auto m11 = atr::make_random_msg11(/*seed*/123, /*n*/2);
        const std::string msg_str = m11.serialize_ascii();
        if (!push_L1_blocking(msg_str, "prod_11", atr::g_evtRunMedicao)) break;

        // Espera próximo período
        DWORD T_ms = static_cast<DWORD>(dist_ms(rng));
        wait_period_or_quit(T_ms);
    }

    atr::log_info("prod_11", "Encerrando thread de mensagens 11.");
    return 0;
}

// Produtor de mensagens 44 (período fixo 500 ms)
DWORD WINAPI thr_msg44(LPVOID) {
    std::cout << "Thread msg 44 iniciada com sucesso!" << std::endl;
    const DWORD T_ms = 500;

    while (true) {
        // Encerra se QUIT
        if (WaitForSingleObject(atr::g_evtQuitAll, 0) == WAIT_OBJECT_0) break;

        // Pausado: aguarda RUN ou QUIT
        if (WaitForSingleObject(atr::g_evtRunCLP, 0) != WAIT_OBJECT_0) {
            HANDLE hs[2] = { atr::g_evtQuitAll, atr::g_evtRunCLP };
            DWORD r = WaitForMultipleObjects(2, hs, FALSE, INFINITE);
            if (r == WAIT_OBJECT_0) break; // QUIT
        }

        // Produz e empilha
        auto m44 = atr::make_random_msg44(/*seed*/456, /*n*/5);
        const std::string msg_str = m44.serialize_ascii();
        if (!push_L1_blocking(msg_str, "prod_44", atr::g_evtRunCLP)) break;

        // Espera próximo período
        wait_period_or_quit(T_ms);
    }

    atr::log_info("prod_44", "Encerrando thread de mensagens 44.");
    return 0;
}

int main() {
    atr::open_child_kernels(); // abre/cria objetos kernel compartilhados

    HANDLE h11 = CreateThread(nullptr, 0, thr_msg11, nullptr, 0, nullptr);
    HANDLE h44 = CreateThread(nullptr, 0, thr_msg44, nullptr, 0, nullptr);

    HANDLE hs[2];
    DWORD n = 0;
    if (h11) hs[n++] = h11;
    if (h44) hs[n++] = h44;

    // Aguarda as threads (se existirem)
    if (n > 0) {
        WaitForMultipleObjects(n, hs, TRUE, INFINITE);
    }

    // Limpeza de handles
    if (h11) CloseHandle(h11);
    if (h44) CloseHandle(h44);

    atr::close_child_kernels(); // fecha objetos kernel
    return 0;
}
