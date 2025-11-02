#include "globals.hpp"
#include <iostream>

namespace atr {

    // ===== Variáveis globais =====
    HANDLE g_semItems_L1 = nullptr;
    HANDLE g_semSpaces_L1 = nullptr;
    HANDLE mutexL1 = nullptr;

    HANDLE g_semItems_L2 = nullptr;
    HANDLE g_semSpaces_L2 = nullptr;
    HANDLE mutexL2 = nullptr;

    HANDLE g_evtRunMedicao = nullptr;
    HANDLE g_evtRunCLP = nullptr;
    HANDLE g_evtRunCaptura = nullptr;
    HANDLE g_evtRunExibicao = nullptr;
    HANDLE g_evtRunAnalise = nullptr;
    HANDLE g_evtClearExibicao = nullptr;
    HANDLE g_evtQuitAll = nullptr;

    HANDLE      g_hMapB1 = nullptr;
    SharedRing* g_B1 = nullptr;

    // ===== Launcher =====
    void init_globals() {
        const size_t total = ring_total_size(B1_CAP, MSG_SZ);

        g_hMapB1 = CreateFileMappingW(
            INVALID_HANDLE_VALUE, nullptr, PAGE_READWRITE,
            DWORD((uint64_t)total >> 32), DWORD(total & 0xFFFFFFFF),
            ATR_B1_NAME);

        if (!g_hMapB1) {
            std::cerr << "[globals] ERRO CreateFileMappingW(B1): " << GetLastError() << "\n";
            ExitProcess(1);
        }

        const DWORD lastErr = GetLastError();
        const bool created_new = (lastErr != ERROR_ALREADY_EXISTS);

        g_B1 = reinterpret_cast<SharedRing*>(
            MapViewOfFile(g_hMapB1, FILE_MAP_ALL_ACCESS, 0, 0, 0)
            );
        if (!g_B1) {
            std::cerr << "[globals] ERRO MapViewOfFile(B1): " << GetLastError() << "\n";
            CloseHandle(g_hMapB1); g_hMapB1 = nullptr;
            ExitProcess(1);
        }
		// Se não existia antes, inicializa o layout
        if (created_new) {
            ZeroMemory(g_B1, total);
            g_B1->hdr.capacity = B1_CAP;
            g_B1->hdr.msg_size = MSG_SZ;
            g_B1->hdr.head = 0;
            g_B1->hdr.tail = 0;
        }

        // Sync L1
        g_semItems_L1 = CreateSemaphore(nullptr, 0, B1_CAP, L"Local\\ATR.SEM.ITEMS.L1");
        g_semSpaces_L1 = CreateSemaphore(nullptr, B1_CAP, B1_CAP, L"Local\\ATR.SEM.SPACES.L1");
        mutexL1 = CreateMutex(nullptr, FALSE, L"Local\\ATR.MTX.L1");

        if (!g_semItems_L1 || !g_semSpaces_L1 || !mutexL1) {
            std::cerr << "[globals] ERRO criando sync L1: " << GetLastError() << "\n";
            ExitProcess(1);
        }

        // Sync L2
        g_semItems_L2 = CreateSemaphore(nullptr, 0, B2_CAP, L"Local\\ATR.SEM.ITEMS.L2");
        g_semSpaces_L2 = CreateSemaphore(nullptr, B2_CAP, B2_CAP, L"Local\\ATR.SEM.SPACES.L2");
        mutexL2 = CreateMutex(nullptr, FALSE, L"Local\\ATR.MTX.L2");

        if (!g_semItems_L2 || !g_semSpaces_L2 || !mutexL2) {
            std::cerr << "[globals] ERRO criando sync L2: " << GetLastError() << "\n";
            ExitProcess(1);
        }

        // Eventos
        g_evtRunMedicao = CreateEvent(nullptr, TRUE, TRUE, L"Local\\ATR.EVT.RUN.MED");
        g_evtRunCLP = CreateEvent(nullptr, TRUE, TRUE, L"Local\\ATR.EVT.RUN.CLP");
        g_evtRunCaptura = CreateEvent(nullptr, TRUE, TRUE, L"Local\\ATR.EVT.RUN.CAPTURA");
        g_evtRunExibicao = CreateEvent(nullptr, TRUE, TRUE, L"Local\\ATR.EVT.RUN.EXIBICAO");
        g_evtRunAnalise = CreateEvent(nullptr, TRUE, TRUE, L"Local\\ATR.EVT.RUN.ANALISE");
        g_evtClearExibicao = CreateEvent(nullptr, FALSE, FALSE, L"Local\\ATR.EVT.RUN.CLEAR_EXIBICAO");
        g_evtQuitAll = CreateEvent(nullptr, TRUE, FALSE, L"Local\\ATR.EVT.QUITALL");

        if (!g_evtRunMedicao || !g_evtRunCLP || !g_evtRunCaptura || !g_evtRunExibicao ||
            !g_evtRunAnalise || !g_evtClearExibicao || !g_evtQuitAll) {
            std::cerr << "[globals] ERRO criando eventos: " << GetLastError() << "\n";
            ExitProcess(1);
        }

        std::cout << "[globals] Inicializado\n";
    }

    void cleanup_globals() {
        // Fechar sync/eventos
        if (mutexL1) { CloseHandle(mutexL1);         mutexL1 = nullptr; }
        if (mutexL2) { CloseHandle(mutexL2);         mutexL2 = nullptr; }
        if (g_semItems_L1) { CloseHandle(g_semItems_L1);   g_semItems_L1 = nullptr; }
        if (g_semSpaces_L1) { CloseHandle(g_semSpaces_L1);  g_semSpaces_L1 = nullptr; }
        if (g_semItems_L2) { CloseHandle(g_semItems_L2);   g_semItems_L2 = nullptr; }
        if (g_semSpaces_L2) { CloseHandle(g_semSpaces_L2);  g_semSpaces_L2 = nullptr; }
        if (g_evtRunMedicao) { CloseHandle(g_evtRunMedicao); g_evtRunMedicao = nullptr; }
        if (g_evtRunCLP) { CloseHandle(g_evtRunCLP);     g_evtRunCLP = nullptr; }
        if (g_evtRunExibicao) { CloseHandle(g_evtRunExibicao); g_evtRunExibicao = nullptr; }
        if (g_evtRunCaptura) { CloseHandle(g_evtRunCaptura); g_evtRunCaptura = nullptr; }
        if (g_evtRunAnalise) { CloseHandle(g_evtRunAnalise); g_evtRunAnalise = nullptr; }
        if (g_evtClearExibicao) { CloseHandle(g_evtClearExibicao); g_evtClearExibicao = nullptr; }
        if (g_evtQuitAll) { CloseHandle(g_evtQuitAll);    g_evtQuitAll = nullptr; }

        // Desmapear e fechar mapping (ordem!)
        if (g_B1) { UnmapViewOfFile(g_B1); g_B1 = nullptr; }
        if (g_hMapB1) { CloseHandle(g_hMapB1); g_hMapB1 = nullptr; }

        std::cout << "[globals] Limpo\n";
    }

    // ===== Filhos =====
    void open_child_kernels() {
        // Abrir mapping e mapear a view
        g_hMapB1 = OpenFileMappingW(FILE_MAP_ALL_ACCESS, FALSE, ATR_B1_NAME);
        if (!g_hMapB1) {
            std::cerr << "[globals childs] ERRO OpenFileMapping(B1): " << GetLastError() << "\n";
            ExitProcess(1);
        }

        g_B1 = reinterpret_cast<SharedRing*>(
            MapViewOfFile(g_hMapB1, FILE_MAP_ALL_ACCESS, 0, 0, 0)
            );
        if (!g_B1) {
            std::cerr << "[globals childs] ERRO MapViewOfFile(B1): " << GetLastError() << "\n";
            CloseHandle(g_hMapB1); g_hMapB1 = nullptr;
            ExitProcess(1);
        }

        // (opcional) validar layout
        if (g_B1->hdr.capacity != B1_CAP || g_B1->hdr.msg_size != MSG_SZ) {
            std::cerr << "[globals childs] ERRO: SharedRing B1 parâmetros inesperados!\n";
            ExitProcess(1);
        }

        // Abrir sync e eventos
        g_semItems_L1 = OpenSemaphore(SEMAPHORE_ALL_ACCESS, FALSE, L"Local\\ATR.SEM.ITEMS.L1");
        g_semSpaces_L1 = OpenSemaphore(SEMAPHORE_ALL_ACCESS, FALSE, L"Local\\ATR.SEM.SPACES.L1");
        g_semItems_L2 = OpenSemaphore(SEMAPHORE_ALL_ACCESS, FALSE, L"Local\\ATR.SEM.ITEMS.L2");
        g_semSpaces_L2 = OpenSemaphore(SEMAPHORE_ALL_ACCESS, FALSE, L"Local\\ATR.SEM.SPACES.L2");
        mutexL1 = OpenMutex(MUTEX_ALL_ACCESS, FALSE, L"Local\\ATR.MTX.L1");
        mutexL2 = OpenMutex(MUTEX_ALL_ACCESS, FALSE, L"Local\\ATR.MTX.L2");

        g_evtRunMedicao = OpenEvent(EVENT_ALL_ACCESS, FALSE, L"Local\\ATR.EVT.RUN.MED");
        g_evtRunCLP = OpenEvent(EVENT_ALL_ACCESS, FALSE, L"Local\\ATR.EVT.RUN.CLP");
        g_evtRunCaptura = OpenEvent(EVENT_ALL_ACCESS, FALSE, L"Local\\ATR.EVT.RUN.CAPTURA");
        g_evtRunExibicao = OpenEvent(EVENT_ALL_ACCESS, FALSE, L"Local\\ATR.EVT.RUN.EXIBICAO");
        g_evtRunAnalise = OpenEvent(EVENT_ALL_ACCESS, FALSE, L"Local\\ATR.EVT.RUN.ANALISE");
        g_evtClearExibicao = OpenEvent(EVENT_ALL_ACCESS, FALSE, L"Local\\ATR.EVT.RUN.CLEAR_EXIBICAO");
        g_evtQuitAll = OpenEvent(EVENT_ALL_ACCESS, FALSE, L"Local\\ATR.EVT.QUITALL");

        if (!g_semItems_L1 || !g_semSpaces_L1 || !g_semItems_L2 || !g_semSpaces_L2 ||
            !mutexL1 || !mutexL2 || !g_evtRunMedicao || !g_evtRunCLP || !g_evtRunCaptura ||
            !g_evtRunExibicao || !g_evtRunAnalise || !g_evtClearExibicao || !g_evtQuitAll) {
            std::cerr << "[globals childs] ERRO abrindo objetos kernel: " << GetLastError() << "\n";
            ExitProcess(1);
        }
    }

    void close_child_kernels() {
        // Fechar sync/eventos
        if (mutexL1) { CloseHandle(mutexL1);         mutexL1 = nullptr; }
        if (mutexL2) { CloseHandle(mutexL2);         mutexL2 = nullptr; }
        if (g_semItems_L1) { CloseHandle(g_semItems_L1);   g_semItems_L1 = nullptr; }
        if (g_semSpaces_L1) { CloseHandle(g_semSpaces_L1);  g_semSpaces_L1 = nullptr; }
        if (g_semItems_L2) { CloseHandle(g_semItems_L2);   g_semItems_L2 = nullptr; }
        if (g_semSpaces_L2) { CloseHandle(g_semSpaces_L2);  g_semSpaces_L2 = nullptr; }
        if (g_evtRunMedicao) { CloseHandle(g_evtRunMedicao); g_evtRunMedicao = nullptr; }
        if (g_evtRunCLP) { CloseHandle(g_evtRunCLP);     g_evtRunCLP = nullptr; }
        if (g_evtRunExibicao) { CloseHandle(g_evtRunExibicao); g_evtRunExibicao = nullptr; }
        if (g_evtRunCaptura) { CloseHandle(g_evtRunCaptura); g_evtRunCaptura = nullptr; }
        if (g_evtRunAnalise) { CloseHandle(g_evtRunAnalise); g_evtRunAnalise = nullptr; }
        if (g_evtClearExibicao) { CloseHandle(g_evtClearExibicao); g_evtClearExibicao = nullptr; }
        if (g_evtQuitAll) { CloseHandle(g_evtQuitAll);    g_evtQuitAll = nullptr; }

        // Desmapear e fechar mapping (ordem!)
        if (g_B1) { UnmapViewOfFile(g_B1); g_B1 = nullptr; }
        if (g_hMapB1) { CloseHandle(g_hMapB1); g_hMapB1 = nullptr; }

        std::cout << "[globals childs] Limpo\n";
    }

} // namespace atr
