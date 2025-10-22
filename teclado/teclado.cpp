#include "utils.hpp"
#include "globals.hpp"
#include "messages.hpp"

#include <windows.h>
#include <conio.h>
#include <thread>
#include <iostream>



// alterna RUN/PAUSE de um evento manual-reset
static void toggle(HANDLE h, const char* tag) {
	// Se está sinalizado, reseta (pausa); se não, sinaliza (resume)
    if (WaitForSingleObject(h, 0) == WAIT_OBJECT_0) {
        // Objeto evento sinalizado
        ResetEvent(h);
        atr::log_info("teclado", std::string("pause ") + tag);
    }
    else {
		// Caso ele não esteja sinalizado, sinaliza
        SetEvent(h);
        atr::log_info("teclado", std::string("resume ") + tag);
    }
}

// thread que lê teclas e sinaliza eventos
DWORD WINAPI keyboard_reading(LPVOID) {
    std::cout <<"Thread de leitura de teclado iniciada. Pressione:\n"
              << "  'm' - alterna medição (Msg11)\n"
              << "  'p' - alterna CLP (Msg44)\n"
              << "  'r' - alterna captura\n"
              << "  'e' - alterna exibição\n"
              << "  'c' - envia clear para exibição\n"
		<< "  ESC - encerra tudo\n";
        
    for (;;) {
        int ch = _getch();            // leitura não-bufferizada
        if (ch == 27) {//ESC
            // Manda evento de finalização de todas as threads
            atr::log_info("teclado", "quit_all");
            SetEvent(atr::g_evtQuitAll);
            break;
        }
        switch (ch) {
        case 'm': toggle(atr::g_evtRunMedicao, "11");
            atr::log_info("Medição", "on-off"); break; // Msg11
        case 'p': toggle(atr::g_evtRunCLP, "44"); 
            atr::log_info("CLP", "on-off"); break;// Msg44
        case 'r': toggle(atr::g_evtRunCaptura, "Captura");
            atr::log_info("captura", "on-off"); break;
        case 'e': toggle(atr::g_evtRunExibicao, "Exibicao");
            atr::log_info("exibicao", "on-off"); break;
        case 'a': toggle(atr::g_evtRunAnalise, "Exibicao");
            atr::log_info("granulometria", "on-off"); break;
        case 'c': /* opção: enviar 'clear' p/ exibicao na Parte B */
            SetEvent(atr::g_evtClearExibicao);
            atr::log_info("exibicao", "clear"); break;
        default:  std::cout << "Caso geral" << (char)ch << "\n"; break;
        }
    }
    return 0;
}

int main() {
    atr::init_globals();

    HANDLE hKey = CreateThread(nullptr, 0, keyboard_reading, nullptr, 0, nullptr);
    // Verifica erro na ciração da thread
    if (hKey == NULL) {
        DWORD err = GetLastError();
        atr::log_error("teclado", std::string("CreateThread falhou: ") + std::to_string(err));
        atr::cleanup_globals();
        return 1;
    }
    // Espera a thread de leitura de teclado terminar
    WaitForSingleObject(hKey, INFINITE);

    if (!CloseHandle(hKey)) {
        atr::log_error("teclado", "CloseHandle falhou");
    }

    atr::cleanup_globals();
    return 0;
}
