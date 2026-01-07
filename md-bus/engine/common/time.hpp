#pragma once
#include <cstdint>
#include <chrono>

namespace md {

inline uint64_t now_ns() {
    using namespace std::chrono;
    return (uint64_t)duration_cast<nanoseconds>(steady_clock::now().
    time_since_epoch()).count();
}

inline uint64_t now_ms() {
    return now_ns() / 1'000'000ULL;
}

}
