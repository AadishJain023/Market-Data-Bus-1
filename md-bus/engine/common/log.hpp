#pragma once
#include <fmt/core.h>
#include <fmt/format.h>
#include <string_view> 
#include <chrono>
#include <thread>
#include <utility>
#include <mutex>
#include <functional> 

namespace md {
enum class LogLevel {
    Debug,
    Info,
    Warn,
    Error
};

inline std::mutex& log_mutex() {
    static std::mutex m;
    return m;
}

inline const char* to_string(LogLevel lvl) {
    switch(lvl){
        case LogLevel::Debug: return "DEBUG";
        case LogLevel::Info: return "INFO";
        case LogLevel::Warn: return "WARN";
        case LogLevel::Error: return "ERROR";
    }
    return "UNK"; // unknown
}

inline std::atomic<LogLevel>& global_log_level() {
    static std::atomic<LogLevel> lvl{LogLevel::Debug};
    return lvl;
}

inline void set_log_level(LogLevel lvl) {
    global_log_level().store(lvl, std::memory_order_relaxed);
}

inline LogLevel get_log_level() {
    return global_log_level().load(std::memory_order_relaxed);
}

//hash thread id into a stable printable integer
inline unsigned long long tid_u64() {
    auto tid = std::this_thread::get_id();
    return static_cast<unsigned long long>(std::hash<std::thread::id>{}(tid));
}

inline long long now_ms_since_epoch() {
    using namespace std::chrono;
    auto now = system_clock::now();
    return duration_cast<milliseconds>(now.time_since_epoch()).count();
}

//the goal is to print a log message with log level and a format string
//And any number of extra arguments(of anytype)
//Args is a parameter pack can be int, int or string, int ,int etc (therefore an 
//array)

//&& is forwarding reference (and not an rvalue reference) 
// what it does is 	For lvalues → parameter is actually an lvalue reference.
//For rvalues → parameter is an rvalue reference.

//forward 	•	If the original argument was an lvalue, std::forward returns an lvalue → it will be copied, not moved.

//•	If the original argument was an rvalue / temporary, std::forward returns an rvalue → it can be moved instead of copied.

//i.e && gives you ability to avoid copies and forward Lets you avoid extra copies

template <typename... Args> //... is to tell Args is pack of types
inline void log(LogLevel lvl, std::string_view fmt_str, Args&&... args){
    if(static_cast<int>(lvl) < static_cast<int>(get_log_level())){
        return;
    }

    const auto ms = now_ms_since_epoch();
    const auto tid = tid_u64();

    fmt::memory_buffer buf;
    fmt::format_to(std::back_inserter(buf),
                    "[{}] t = {}ms tid = {} ",
                   to_string(lvl),
                   ms,
                   tid);

    fmt::format_to(std::back_inserter(buf),
                    fmt_str,
                    std::forward<Args>(args)...);
    
    buf.push_back('\n');

    {
        std::lock_guard<std::mutex> lk(log_mutex());
        fmt::print("{}", fmt::to_string(buf));
    }
}

// Convenience wrappers 
//calling log function that we defines above
template<typename... Args>
inline void log_debug(std::string_view fmt_str, Args&&... args){
    log(LogLevel::Debug, fmt_str, std::forward<Args>(args)...);
}

template<typename... Args>
inline void log_info(std::string_view fmt_str, Args&&... args){
    log(LogLevel::Info, fmt_str, std::forward<Args>(args)...);
}

template<typename... Args>
inline void log_warn(std::string_view fmt_str, Args&&... args){
    log(LogLevel::Warn, fmt_str, std::forward<Args>(args)...);
}

template<typename... Args>
inline void log_error(std::string_view fmt_str, Args&&... args){
    log(LogLevel::Error, fmt_str, std::forward<Args>(args)...);
}

}