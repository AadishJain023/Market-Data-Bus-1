#pragma once
#include <cstdint>
#include <array>
#include <algorithm>

namespace md {

//log2 histogram bucket covers [2^i, 2^(i + 1)) ns
template <size_t MAX_B = 48>
struct Log2Histogram {
    std::array<uint64_t, MAX_B> b {};
    uint64_t n = 0;
    uint64_t min_v = (uint64_t)-1;
    uint64_t max_v = 0;
    long double sum = 0.0L;
    //the below function finds out floor(log2(x));
    //builtin_clzll(x) count leading zeros 
    //floor(log2(x)) is same as the index of the set bit

    //x is latency/duration in nanoseconds
    static inline size_t bucket_of(uint64_t x) {
        // we handle 0 separately
        if(x == 0) return 0;
        #if defined(__GNUG__) || defined(__clang__)
            size_t lg = 63u - (size_t)__builtin_clzll(x);
        #else 
            size_t lg = 0;
            while ((1ULL << (lg + 1)) <= x && (lg + 1) < 63) ++lg;
        #endif
            return (lg >= MAX_B) ? (MAX_B - 1) : lg;
    }

    void record(uint64_t x) {
        ++n;
        min_v = std::min(min_v, x);
        max_v = std::max(max_v, x);
        sum += (long double) x;
        b[bucket_of(x)]++;
    }

    //p [0, 1] percentile
    uint64_t percentile(double p) const {
        //n is total number of recorded samples
        if(n == 0)return 0;
        if(p <= 0.0)return min_v;
        if(p >= 1.0) return max_v;
        const uint64_t target = (uint64_t)((long double)n * (long double)p);
        uint64_t c = 0;
        for(size_t i = 0; i < MAX_B; ++i) {
            c += b[i];
            if(c >= target) {
                return (i >= 63) ? max_v : (1ULL << i);
            }
        }
        return max_v;
    }

    uint64_t avg() const {
        if(n == 0)return 0;
        return (uint64_t)(sum / (long double)n);
    }
};

struct PerfSnapshot{
    uint64_t events = 0;
    uint64_t duration_ns = 0;
    uint64_t eps = 0;


    uint64_t lat_min = 0;
    uint64_t lat_avg = 0;
    uint64_t lat_p50 = 0;
    uint64_t lat_p95 = 0;
    uint64_t lat_p99 = 0;
    uint64_t lat_max = 0;
};

}
