#pragma once
#include <windows.h>
#include <string>
#include "shared_layout.hpp"
namespace atr {
	//Definindo nova Lista 1
	extern SharedRing* g_B1; // Ponteiro para a estrutura de dados compartilhada da Lista 1
	extern HANDLE g_hMapB1; // Handle para arquivo mapeado (shared memory) da Lista 1
	//Definindo nova Lista 2
	extern SharedRing* g_B2; // Ponteiro para a estrutura de dados compartilhada da Lista 2
	extern HANDLE g_hMapB2; // Handle para arquivo mapeado (shared memory) da Lista 2

	// Lista 1 (mensagens de io_entrada)
	extern HANDLE g_semItems_L1;   // conta itens disponíveis
	extern HANDLE g_semSpaces_L1;  // conta espaços livres
	extern HANDLE mutexL1; // define seção crítica de acesso ao buffer, head,tail,count e cap da lista 1

	// Lista 2 (mensagens tipo 11)
	extern HANDLE g_semItems_L2; // conta itens disponíveis na lista 2
	extern HANDLE g_semSpaces_L2; // conta espaços livres na lista 2
	extern HANDLE mutexL2; // define seção crítica de acesso ao buffer, head,tail,count e cap da lista 

	// Eventos de controle de threads utilizados pelo teclado.
	extern HANDLE g_evtRunMedicao;
	extern HANDLE g_evtRunCLP;
	extern HANDLE g_evtRunCaptura;
	extern HANDLE g_evtRunExibicao;
	extern HANDLE g_evtRunAnalise;
	extern HANDLE g_evtClearExibicao;
	extern HANDLE g_evtQuitAll;

	extern HANDLE hMailSlot;
	

	// Função de inicialização e limpeza
	void init_globals();
	void init_mailslot();
	void write_mailslot_clear();
	void cleanup_globals();
	void open_child_kernels();
	void close_child_kernels();

} // Namespace ATR