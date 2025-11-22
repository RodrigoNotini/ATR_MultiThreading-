// exibicao.cpp
#include "utils.hpp"
#include "globals.hpp"
#include "shared_layout.hpp"
#include <windows.h>
#include <iostream>
#include <string>

DWORD WINAPI thr_exibicao(LPVOID) {

    std::cout << "[exibicao] Thread iniciada.\n";

    char buffer[512];
    DWORD bytesRead = 0;
    DWORD bytesAvail = 0;

    for (;;) {

        // Se QUIT for sinalizado, sai
        if (WaitForSingleObject(atr::g_evtQuitAll, 0) == WAIT_OBJECT_0)
            return 0;

        // Espera RUN (com possibilidade de QUIT)
        HANDLE hs[] = { atr::g_evtQuitAll, atr::g_evtRunExibicao };
        DWORD wr = WaitForMultipleObjects(2, hs, FALSE, INFINITE);

        if (wr == WAIT_OBJECT_0) return 0;  // QUIT

        // RUN está ativo → laço de trabalho
        while (WaitForSingleObject(atr::g_evtRunExibicao, 0) == WAIT_OBJECT_0) {

            // --- TENTA LER DO PIPE SEM BLOQUEAR ---
            BOOL ok = PeekNamedPipe(
                atr::hNamedPipe,
                nullptr,
                0,
                nullptr,
                &bytesAvail,
                nullptr
            );

            if (!ok) {
                std::cout << "[exibicao] PeekNamedPipe ERRO: " << GetLastError() << "\n";
                break;
            }

            if (bytesAvail > 0) {
                // Agora que sabemos que tem dados → usamos ReadFile
                BOOL ok2 = ReadFile(
                    atr::hNamedPipe,
                    buffer,
                    sizeof(buffer),
                    &bytesRead,
                    nullptr
                );

                if (ok2 && bytesRead > 0) {
                    std::string msg(buffer, bytesRead);
                    std::cout << "[exibicao] MENSAGEM RECEBIDA:\n" << msg << "\n";
                }
            }

            // std::cout << "[exibicao] Atualizando tela...\n"; debbugando 

            Sleep(100);

            if (WaitForSingleObject(atr::g_evtQuitAll, 0) == WAIT_OBJECT_0)
                return 0;
        }
    }
}



int main() {
    atr::open_child_kernels();  // abre/cria objetos kernel compartilhados (eventos globais)
	atr::init_namedpipe();   // inicializa named pipe para comunicação com captura
    HANDLE h1 = CreateThread(nullptr, 0, thr_exibicao, nullptr, 0, nullptr); // cria thread de exibição
    if (h1) WaitForSingleObject(h1, INFINITE); // espera término da thread
    if (h1) CloseHandle(h1); // libera handle da thread
    atr::close_child_kernels(); // fecha objetos kernel
	atr::close_namedpipe(); // fecha named pipe
    return 0;
}
