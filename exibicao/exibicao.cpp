#include "utils.hpp"
#include "globals.hpp"
#include "shared_layout.hpp"
#include <windows.h>
#include <iostream>

DWORD WINAPI thr_exibicao(LPVOID) {
    for (;;) {
        // Sai se QUIT
        if (WaitForSingleObject(atr::g_evtQuitAll, 0) == WAIT_OBJECT_0) return 0;

        // Bloqueia até RUN ou QUIT
        HANDLE hs[] = { atr::g_evtQuitAll, atr::g_evtRunExibicao };
        DWORD wr = WaitForMultipleObjects(2, hs, FALSE, INFINITE);
        if (wr == WAIT_OBJECT_0) return 0;          // QUIT
        // wr == WAIT_OBJECT_0+1 -> RUN está sinalizado (manual-reset)

        // FAZ O TRABALHO ENQUANTO RUN CONTINUAR SINALIZADO
        if (WaitForSingleObject(atr::g_evtRunExibicao, 0) == WAIT_OBJECT_0) {
            std::cout << "Exibindo dados na tela...\n";
            Sleep(200); // evita loop 100% CPU
        }
    }
}

DWORD WINAPI thr_analise_granulometria(LPVOID) {
    for (;;) {
        if (WaitForSingleObject(atr::g_evtQuitAll, 0) == WAIT_OBJECT_0) return 0;

        HANDLE hs[] = { atr::g_evtQuitAll, atr::g_evtRunAnalise };
        DWORD wr = WaitForMultipleObjects(2, hs, FALSE, INFINITE);
        if (wr == WAIT_OBJECT_0) return 0;

        if (WaitForSingleObject(atr::g_evtRunAnalise, 0) == WAIT_OBJECT_0) {
            std::cout << "Analise de dados\n";
            Sleep(200);
        }
    }
}

int main() {
    atr::open_child_kernels();  // cria/abre eventos: g_evtQuitAll (manual-reset), g_evtRunMedicao (manual-reset)
    // opcional: começar em RUN
    SetEvent(atr::g_evtRunMedicao);

    HANDLE h1 = CreateThread(nullptr, 0, thr_exibicao, nullptr, 0, nullptr);
    HANDLE h2 = CreateThread(nullptr, 0, thr_analise_granulometria, nullptr, 0, nullptr);

    // >>> mantenha o processo vivo até QUIT
    WaitForSingleObject(atr::g_evtQuitAll, INFINITE);

    // depois de QUIT, espere threads terminarem e limpe
    HANDLE hs[] = { h1, h2 };
    WaitForMultipleObjects(2, hs, TRUE, INFINITE);
    if (h1) CloseHandle(h1);
    if (h2) CloseHandle(h2);

    atr::close_child_kernels();
    return 0;
}
