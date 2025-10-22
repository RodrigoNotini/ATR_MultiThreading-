
#pragma once
#include <string>
#include <cstdint>

namespace atr {

// Event names (used later in Part B/F)
inline constexpr const char* EVT_BLOCK_MED   = "Global\\EVT_BLOCK_MED";
inline constexpr const char* EVT_BLOCK_PROC  = "Global\\EVT_BLOCK_PROC";
inline constexpr const char* EVT_BLOCK_CAPT  = "Global\\EVT_BLOCK_CAPT";
inline constexpr const char* EVT_BLOCK_EXIB  = "Global\\EVT_BLOCK_EXIB";
inline constexpr const char* EVT_BLOCK_ANAL  = "Global\\EVT_BLOCK_ANAL";
inline constexpr const char* EVT_QUIT_ALL    = "Global\\EVT_QUIT_ALL";

// Fixed-size message formats

// Tipo 11 (granulometria)
struct Msg11 {
    uint16_t tipo = 11;
    uint16_t nseq = 0;      // 0..9999 (wrap)
    uint8_t  id   = 1;      // 1..2
    double   gr_med = 0.0;  // 0.00..100.00
    double   gr_max = 0.0;
    double   gr_min = 0.0;
    double   sigma  = 0.0;  // 0.00..100.00

    // Serialize into the fixed ASCII layout:
    // "11/NSEQ/TIMESTAMP/ID/GR_MED/GR_MAX/GR_MIN/SIGMA"
    std::string serialize_ascii() const;
};

// Tipo 44 (processo)
struct Msg44 {
    uint16_t tipo = 44;
    uint16_t nseq = 0;    // 0..9999 (wrap)
    uint8_t  id   = 1;    // 1..6
    double   vel  = 0.0;  // 0..1000 cm/s
    double   incl = 0.0;  // 0..45 graus
    double   pot  = 0.0;  // 0..2 kWh
    double   vz_ent = 0.0;// 0..1000 kg/h
    double   vz_sai = 0.0;// 0..1000 kg/h

    // Serialize into:
    // "44/NSEQ/ID/TIMESTAMP/VEL/INCL/POT/VZ_ENT/VZ_SAIDA"
    std::string serialize_ascii() const;
};

// Helpers to generate random valid messages according to spec
Msg11 make_random_msg11(uint16_t nseq, uint8_t id);
Msg44 make_random_msg44(uint16_t nseq, uint8_t id);

} // namespace atr
