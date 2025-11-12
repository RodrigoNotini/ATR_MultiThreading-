#include "utils.hpp"
#include "globals.hpp"
#include "shared_layout.hpp"
#include <windows.h>
#include <iostream>
// Mensagens do tipo 44
// Thread responsável por exibir os dados na tela
DWORD WINAPI thr_exibicao(LPVOID) {
    int n = 0; // contador de exibições
    for (;;) {
        // Sai se o evento global de saída (QUIT) estiver sinalizado
        if (WaitForSingleObject(atr::g_evtQuitAll, 0) == WAIT_OBJECT_0) return 0;

        // Aguarda até que RUN ou QUIT sejam sinalizados
        HANDLE hs[] = { atr::g_evtQuitAll, atr::g_evtRunExibicao };
        DWORD wr = WaitForMultipleObjects(2, hs, FALSE, INFINITE);
        if (wr == WAIT_OBJECT_0) return 0;          // QUIT
        // wr == WAIT_OBJECT_0 + 1 → RUN foi sinalizado (manual-reset)

        // Executa enquanto RUN permanecer sinalizado
        if (WaitForSingleObject(atr::g_evtRunExibicao, 0) == WAIT_OBJECT_0) {
            ++n; // incrementa contador
            std::cout << "Exibindo dados na tela...\n"
                << "Numero " << n << "\n";
            Sleep(200); // pequena pausa para evitar uso excessivo da CPU
        }
    }
}

int main() {
    atr::open_child_kernels();  // abre/cria objetos kernel compartilhados (eventos globais)
    HANDLE h1 = CreateThread(nullptr, 0, thr_exibicao, nullptr, 0, nullptr); // cria thread de exibição
    if (h1) WaitForSingleObject(h1, INFINITE); // espera término da thread
    if (h1) CloseHandle(h1); // libera handle da thread
    atr::close_child_kernels(); // fecha objetos kernel
    return 0;
}
