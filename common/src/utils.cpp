#include "utils.hpp"
#include <iomanip>
#include <sstream>
#include <iostream>
#include <random>
#include <thread>

namespace atr {

    // Singleton do gerador de números aleatórios
    Rng& Rng::instance() {
        static Rng rng;
        return rng;
    }

    // Construtor: semente via std::random_device e mt19937_64
    Rng::Rng() {
        std::random_device rd;
        gen_ = std::mt19937_64(rd());
    }

    // Inteiro uniforme em [lo, hi] com proteção por mutex
    int Rng::uniform_int(int lo, int hi) {
        std::scoped_lock lk(mtx_);
        std::uniform_int_distribution<int> dist(lo, hi);
        return dist(gen_);
    }

    // Real uniforme em [lo, hi] com proteção por mutex
    double Rng::uniform_real(double lo, double hi) {
        std::scoped_lock lk(mtx_);
        std::uniform_real_distribution<double> dist(lo, hi);
        return dist(gen_);
    }

    // Hora local no formato HH:MM:SS
    std::string now_hhmmss() {
        using namespace std::chrono;
        auto now = floor<seconds>(system_clock::now());
        std::time_t tt = system_clock::to_time_t(now);
        std::tm tm{};
#ifdef _WIN32
        localtime_s(&tm, &tt);     // versão segura no Windows
#else
        localtime_r(&tt, &tm);     // versão segura POSIX
#endif
        std::ostringstream oss;
        oss << std::setfill('0')
            << std::setw(2) << tm.tm_hour << ":"
            << std::setw(2) << tm.tm_min << ":"
            << std::setw(2) << tm.tm_sec;
        return oss.str();
    }

    // Hora local no formato HH:MM:SS:ms (milissegundos)
    std::string now_hhmmss_ms() {
        using namespace std::chrono;
        auto now = system_clock::now();
        auto s = time_point_cast<seconds>(now);                 // parte inteira em segundos
        auto ms = duration_cast<milliseconds>(now - s).count();  // milissegundos restantes
        std::time_t tt = system_clock::to_time_t(s);
        std::tm tm{};
#ifdef _WIN32
        localtime_s(&tm, &tt);
#else
        localtime_r(&tt, &tm);
#endif
        std::ostringstream oss;
        oss << std::setfill('0')
            << std::setw(2) << tm.tm_hour << ":"
            << std::setw(2) << tm.tm_min << ":"
            << std::setw(2) << tm.tm_sec << ":"
            << std::setw(3) << ms;
        return oss.str();
    }

    // Preenche à esquerda até 'width' com o caractere 'fill'
    std::string pad_left(std::string_view s, size_t width, char fill) {
        if (s.size() >= width) return std::string(s);
        return std::string(width - s.size(), fill) + std::string(s);
    }

    // Formata em ponto fixo com largura mínima 'width' e 'precision' casas decimais
    std::string format_fixed(double value, int width, int precision) {
        std::ostringstream oss;
        oss << std::fixed << std::setprecision(precision) << value;
        auto str = oss.str();
        if (str.size() < static_cast<size_t>(width)) {
            str = std::string(static_cast<size_t>(width) - str.size(), '0') + str; // preenche com zeros à esquerda
        }
        return str;
    }

    // Logs simples (WARN/INFO/ERROR) no console
    void log_warn(std::string_view tag, std::string_view msg) {
        std::cout << "[WARN][" << tag << "] " << msg << std::endl;
    }

    void log_info(std::string_view tag, std::string_view msg) {
        std::cout << "[INFO][" << tag << "] " << msg << std::endl;
    }

    void log_error(std::string_view tag, std::string_view msg) {
        std::cerr << "[ERR ][" << tag << "] " << msg << std::endl;
    }

    // Calcula o ponteiro para o slot do índice lógico 'idx' no ring buffer compartilhado
    BYTE* slot_ptr(atr::SharedRing* r, LONG idx) {
        const LONG cap = r->hdr.capacity;
        const LONG pos = idx % cap; // se cap for potência de 2, poderia usar & (cap-1)
        return const_cast<BYTE*>(&r->data[0]) + size_t(pos) * r->hdr.msg_size;
    }

} // namespace atr
