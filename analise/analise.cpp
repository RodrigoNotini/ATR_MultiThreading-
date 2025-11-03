#include "utils.hpp"
#include "globals.hpp"
#include "shared_layout.hpp"
#include <windows.h>
#include <iostream>

// Thread responsável por realizar a análise granulométrica
DWORD WINAPI thr_analise_granulometria(LPVOID) {
    int n = 0; // contador de análises realizadas
    for (;;) {
        HANDLE hs[] = { atr::g_evtQuitAll, atr::g_evtRunAnalise };
        DWORD wr = WaitForMultipleObjects(2, hs, FALSE, INFINITE);

        if (wr == WAIT_OBJECT_0) return 0;           // Evento de saída global (QUIT)
        // wr == WAIT_OBJECT_0 + 1 → RUN foi sinalizado (iniciar ou continuar execução)

        // Loop enquanto o evento RUN estiver sinalizado
        while (WaitForSingleObject(atr::g_evtRunAnalise, 0) == WAIT_OBJECT_0) {
            if (WaitForSingleObject(atr::g_evtQuitAll, 0) == WAIT_OBJECT_0) return 0; // Verifica se é para encerrar
            ++n; // incrementa o número da análise
            std::cout << "Analise de dado\n"
                << "Numero " << n << "\n";
            Sleep(200); // pequena pausa para evitar busy-loop
        }
        // RUN foi resetado (PAUSE) → volta ao topo e espera novo RUN ou QUIT
    }
}

int main() {
    atr::open_child_kernels(); // abre os objetos kernel compartilhados
    HANDLE h = CreateThread(nullptr, 0, thr_analise_granulometria, nullptr, 0, nullptr); // cria thread de análise
    if (h) WaitForSingleObject(h, INFINITE); // espera término da thread
    atr::close_child_kernels(); // fecha os objetos kernel
    if (h) CloseHandle(h); // libera o handle da thread
    return 0;
}
