
#include "utils.hpp"
#include "globals.hpp"
#include <iostream>
#include "shared_layout.hpp"
//Captura 44 -> Exibicao
//Captura 11 -> Lista 2
static bool pull_L1_blocking(const char* consuidor_tag, HANDLE evtrun) {

}
DWORD WINAPI thr_msg_capture(LPVOID) {
	std::cout << "Thread captura iniciada com sucesso!" << std::endl;

}
int main() {
    atr::open_child_kernels();
	HANDLE h1 = CreateThread(nullptr, 0, thr_msg_capture, nullptr, 0, nullptr);

}
