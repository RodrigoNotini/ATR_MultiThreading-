#define NOMINMAX
#include "messages.hpp"
#include "utils.hpp"
#include <sstream>

namespace atr {

    // Converte Msg11 para string ASCII no formato especificado
    std::string Msg11::serialize_ascii() const {
        std::ostringstream oss;
        // Formato: "11/NSEQ/TIMESTAMP/ID/GR_MED/GR_MAX/GR_MIN/SIGMA"
        std::string nseq_str = atr::pad_left(std::to_string(nseq % 10000), 4, '0'); // NSEQ com 4 dígitos
        std::string ts = atr::now_hhmmss();                                   // HH:MM:SS
        std::string id_str = atr::pad_left(std::to_string(id), 2, '0');           // ID com 2 dígitos
        auto f = [](double v) { return atr::format_fixed(v, 6, 2); };                // largura 6, 2 casas
        oss << "11/" << nseq_str << "/" << ts << "/"
            << id_str << "/"
            << f(gr_med) << "/" << f(gr_max) << "/"
            << f(gr_min) << "/" << f(sigma);
        return oss.str();
    }

    // Converte Msg44 para string ASCII no formato especificado
    std::string Msg44::serialize_ascii() const {
        std::ostringstream oss;
        // Formato: "44/NSEQ/ID/TIMESTAMP/VEL/INCL/POT/VZ_ENT/VZ_SAIDA"
        std::string nseq_str = atr::pad_left(std::to_string(nseq % 10000), 4, '0'); // NSEQ com 4 dígitos
        std::string id_str = atr::pad_left(std::to_string(id), 2, '0');           // ID com 2 dígitos
        std::string ts = atr::now_hhmmss_ms();                                // HH:MM:SS.mmm
        auto f6_1 = [](double v) { return atr::format_fixed(v, 6, 1); };             // largura 6, 1 casa
        auto f4_1 = [](double v) { return atr::format_fixed(v, 4, 1); };             // largura 4, 1 casa
        auto f5_3 = [](double v) { return atr::format_fixed(v, 5, 3); };             // largura 5, 3 casas
        std::string vel_s = f6_1(vel);
        std::string incl_s = f4_1(incl);
        std::string pot_s = f5_3(pot);
        std::string vz_e_s = f6_1(vz_ent);
        std::string vz_s_s = f6_1(vz_sai);

        oss << "44/" << nseq_str << "/" << id_str << "/"
            << ts << "/"
            << vel_s << "/" << incl_s << "/" << pot_s
            << "/" << vz_e_s << "/" << vz_s_s;
        return oss.str();
    }

    // Gera Msg11 pseudoaleatória com limites razoáveis
    Msg11 make_random_msg11(uint16_t nseq, uint8_t id) {
        Msg11 m;
        m.nseq = nseq;
        m.id = id;
        auto& rng = Rng::instance();                               // gerador global
        m.gr_med = rng.uniform_real(0.0, 100.0);                   // média
        m.gr_max = std::max(m.gr_med, rng.uniform_real(0.0, 100.0));
        m.gr_min = std::min(m.gr_med, rng.uniform_real(0.0, 100.0));
        double diff = m.gr_max - m.gr_min;
        m.sigma = rng.uniform_real(0.0, std::max(0.01, diff / 3.0)); // desvio padrão
        return m;
    }

    // Gera Msg44 pseudoaleatória dentro de faixas típicas
    Msg44 make_random_msg44(uint16_t nseq, uint8_t id) {
        Msg44 m;
        m.nseq = nseq;
        m.id = id;
        auto& rng = Rng::instance();             // gerador global
        m.vel = rng.uniform_real(0.0, 1000.0);
        m.incl = rng.uniform_real(0.0, 45.0);
        m.pot = rng.uniform_real(0.0, 2.0);
        m.vz_ent = rng.uniform_real(0.0, 1000.0);
        m.vz_sai = rng.uniform_real(0.0, 1000.0);
        return m;
    }

} // namespace atr
