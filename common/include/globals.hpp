#pragma once
#include <windows.h>
#include <string>
namespace atr {
	// Lista 1 (mensagens de io_entrada)
	extern HANDLE g_semItems_L1;   // conta itens disponíveis
	extern HANDLE g_semSpaces_L1;  // conta espaços livres
	extern HANDLE mutexL1; // define seção crítica de acesso ao buffer, head,tail,count e cap da lista 1
	extern int g_head_L1, g_tail_L1, g_count_L1, g_cap_L1; // controle do buffer circular
	extern std::string* g_buf_L1; // buffer circular 

	// Lista 2 (mensagens tipo 11)
	extern HANDLE g_semItems_L2; // conta itens disponíveis na lista 2
	extern HANDLE g_semSpaces_L2; // conta espaços livres na lista 2
	extern HANDLE mutexL2; // define seção crítica de acesso ao buffer, head,tail,count e cap da lista 2
	extern int g_head_L2, g_tail_L2, g_count_L2, g_cap_L2;  // controle do buffer circular 2
	extern std::string* g_buf_L2;// buffer circular 2

	// Eventos de controle de threads utilizados pelo teclado.
	extern HANDLE g_evtRunMedicao;
	extern HANDLE g_evtRunCLP;
	extern HANDLE g_evtRunCaptura;
	extern HANDLE g_evtRunExibicao;
	extern HANDLE g_evtRunAnalise;
	extern HANDLE g_evtClearExibicao;
	extern HANDLE g_evtQuitAll;
	

	// Função de inicialização e limpeza
	void init_globals();
	void cleanup_globals();
	void open_child_kernels();
	void close_child_kernels();

} // Namespace ATR