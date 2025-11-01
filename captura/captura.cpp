
#include "utils.hpp"
#include <iostream>
#include "shared_layout.hpp"

int main() {
    atr::log_info("captura", "Skeleton pronto. Em fases futuras: consumir buffer1 e demux 11/44.");
	atr::log_error("captura", "Erro simulado para teste de log_error.");
    std::cout << "OK captura.\n";
    return 0;
}
