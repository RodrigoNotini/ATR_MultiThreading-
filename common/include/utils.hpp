
#pragma once
#include <string>
#include <string_view>
#include <chrono>
#include <random>
#include <mutex>
#include <wtypes.h>
#include "shared_layout.hpp"

namespace atr {

// Thread-safe RNG (Mersenne Twister) with a global instance
class Rng {
public:
    static Rng& instance();
    // Uniform integer in [lo, hi]
    int uniform_int(int lo, int hi);
    // Uniform real in [lo, hi]
    double uniform_real(double lo, double hi);
private:
    Rng();
    std::mt19937_64 gen_;
    std::mutex mtx_;
};


// Time helpers
std::string now_hhmmss();      // HH:MM:SS
std::string now_hhmmss_ms();   // HH:MM:SS:ms

// String helpers
std::string pad_left(std::string_view s, size_t width, char fill='0');
std::string format_fixed(double value, int width, int precision);

// Logging (simple thread-safe prints)
void log_warn(std::string_view tag, std::string_view msg);
void log_info(std::string_view tag, std::string_view msg);
void log_error(std::string_view tag, std::string_view msg);
BYTE* slot_ptr(SharedRing* r, LONG idx);

} // namespace atr

