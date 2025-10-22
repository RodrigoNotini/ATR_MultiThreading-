#include "globals.hpp"
#include <iostream>
    namespace atr {
    // Definição real das variáveis (uma única vez no projeto)
    HANDLE g_semItems_L1 = nullptr;
    HANDLE g_semSpaces_L1 = nullptr;
	HANDLE mutexL1 = nullptr;
    int g_head_L1 = 0, g_tail_L1 = 0, g_count_L1 = 0, g_cap_L1 = 200;
    std::string* g_buf_L1 = nullptr;

    HANDLE g_semItems_L2 = nullptr;
    HANDLE g_semSpaces_L2 = nullptr;
	HANDLE mutexL2 = nullptr;
    int g_head_L2 = 0, g_tail_L2 = 0, g_count_L2 = 0, g_cap_L2 = 100;
    std::string* g_buf_L2 = nullptr;

    HANDLE g_evtRunMedicao = nullptr;
    HANDLE g_evtRunCLP = nullptr;
    HANDLE g_evtRunCaptura = nullptr;
    HANDLE g_evtRunExibicao = nullptr;
	HANDLE g_evtRunAnalise = nullptr;
	HANDLE g_evtClearExibicao = nullptr;
    HANDLE g_evtQuitAll = nullptr;


    void init_globals() {
        g_buf_L1 = new std::string[g_cap_L1];
        g_buf_L2 = new std::string[g_cap_L2];

        g_semItems_L1 = CreateSemaphore(nullptr, 0, g_cap_L1, L"Local\\ATR.SEM.ITEMS.L1");
        g_semSpaces_L1 = CreateSemaphore(nullptr, g_cap_L1, g_cap_L1, L"Local\\ATR.SEM.SPACES.L1");
		mutexL1 = CreateMutex(nullptr, FALSE, L"Local\\ATR.MTX.L1");

        g_semItems_L2 = CreateSemaphore(nullptr, 0, g_cap_L2, L"Local\\ATR.SEM.ITEMS.L2");
        g_semSpaces_L2 = CreateSemaphore(nullptr, g_cap_L2, g_cap_L2, L"Local\\ATR.SEM.SPACES.L2");
		mutexL2 = CreateMutex(nullptr, FALSE, L"Local\\ATR.MTX.L2");

        //Eventos que começam  sinalizados com manual-reset
        g_evtRunMedicao = CreateEvent(nullptr, TRUE, TRUE, L"Local\\ATR.EVT.RUN.MED");
        g_evtRunCLP = CreateEvent(nullptr, TRUE, TRUE, L"Local\\ATR.EVT.RUN.CLP");
        g_evtRunCaptura = CreateEvent(nullptr, TRUE, TRUE, L"Local\\ATR.EVT.RUN.CAPTURA");
        g_evtRunExibicao = CreateEvent(nullptr, TRUE, TRUE, L"Local\\ATR.EVT.RUN.EXIBICAO");
        g_evtRunAnalise = CreateEvent(nullptr, TRUE, TRUE, L"Local\\ATR.EVT.RUN.ANALISE");
        g_evtClearExibicao = CreateEvent(nullptr, FALSE, FALSE, L"Local\\ATR.EVT.RUN.CLEAR_EXIBICAO");//Não sinalizado reset automatico
        g_evtQuitAll = CreateEvent(nullptr, TRUE, FALSE, L"Local\\ATR.EVT.QUITALL");//Nao sinalizado reset manual

        std::cout << "[globals] Inicializado" << std::endl;
    }

    void cleanup_globals() {
        CloseHandle(mutexL1);
		CloseHandle(mutexL2);
        CloseHandle(g_semItems_L1);
        CloseHandle(g_semSpaces_L1);
        CloseHandle(g_semItems_L2);
        CloseHandle(g_semSpaces_L2);
        CloseHandle(g_evtRunMedicao);
        CloseHandle(g_evtRunCLP);
        CloseHandle(g_evtRunExibicao);
        CloseHandle(g_evtRunCaptura);
        CloseHandle(g_evtRunAnalise);
        CloseHandle(g_evtClearExibicao);
        CloseHandle(g_evtQuitAll);
        delete[] g_buf_L1;
        delete[] g_buf_L2;
        std::cout << "[globals] Limpo" << std::endl;
    }

    void open_child_kernels(){
		//Função para abrir os kernels nos processos filhos de launcher
        g_buf_L1 = new std::string[g_cap_L1];
        g_buf_L2 = new std::string[g_cap_L2];

        g_semItems_L1 = OpenSemaphore(SEMAPHORE_ALL_ACCESS, FALSE, L"Local\\ATR.SEM.ITEMS.L1");
        g_semSpaces_L1 = OpenSemaphore(SEMAPHORE_ALL_ACCESS, FALSE, L"Local\\ATR.SEM.SPACES.L1");
        g_semItems_L2 = OpenSemaphore(SEMAPHORE_ALL_ACCESS, FALSE, L"Local\\ATR.SEM.ITEMS.L2");
        g_semSpaces_L2 = OpenSemaphore(SEMAPHORE_ALL_ACCESS, FALSE, L"Local\\ATR.SEM.SPACES.L2");
        g_evtRunMedicao = OpenEvent(EVENT_ALL_ACCESS, FALSE, L"Local\\ATR.EVT.RUN.MED");
        g_evtRunCLP = OpenEvent(EVENT_ALL_ACCESS, FALSE, L"Local\\ATR.EVT.RUN.CLP");
        g_evtRunCaptura = OpenEvent(EVENT_ALL_ACCESS, FALSE, L"Local\\ATR.EVT.RUN.CAPTURA");
        g_evtRunExibicao = OpenEvent(EVENT_ALL_ACCESS, FALSE, L"Local\\ATR.EVT.RUN.EXIBICAO");
        g_evtRunAnalise = OpenEvent(EVENT_ALL_ACCESS, FALSE, L"Local\\ATR.EVT.RUN.ANALISE");
        g_evtClearExibicao = OpenEvent(EVENT_ALL_ACCESS, FALSE, L"Local\\ATR.EVT.RUN.CLEAR_EXIBICAO");
        g_evtQuitAll = OpenEvent(EVENT_ALL_ACCESS, FALSE, L"Local\\ATR.EVT.QUITALL");
        mutexL1=OpenMutex(MUTEX_ALL_ACCESS, FALSE, L"Local\\ATR.MTX.L1");
        mutexL2=OpenMutex(MUTEX_ALL_ACCESS, FALSE, L"Local\\ATR.MTX.L2");
    }

    void close_child_kernels() {
		//Função para fechar os kernels abertos nos processos filhos
        CloseHandle(mutexL1);
        CloseHandle(mutexL2);
        CloseHandle(g_semItems_L1);
        CloseHandle(g_semSpaces_L1);
        CloseHandle(g_semItems_L2);
        CloseHandle(g_semSpaces_L2);
        CloseHandle(g_evtRunMedicao);
        CloseHandle(g_evtRunCLP);
        CloseHandle(g_evtRunExibicao);
        CloseHandle(g_evtRunCaptura);
        CloseHandle(g_evtRunAnalise);
        CloseHandle(g_evtClearExibicao);
        CloseHandle(g_evtQuitAll);
        delete[] g_buf_L1;
        delete[] g_buf_L2;
        std::cout << "[globals childs] Limpo" << std::endl;
    }


    } // namespace atr


  