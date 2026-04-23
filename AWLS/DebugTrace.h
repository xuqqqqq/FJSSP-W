#ifndef AWLS_DEBUG_TRACE_H
#define AWLS_DEBUG_TRACE_H

#include <cstdlib>
#include <fstream>
#include <mutex>
#include <sstream>
#include <string>

namespace awls_trace
{
inline bool enabled()
{
    static const bool on = []() {
        const char* value = std::getenv("AWLS_TRACE");
        return value != nullptr && value[0] != '\0' && value[0] != '0';
    }();
    return on;
}

inline std::ofstream& stream()
{
    static std::ofstream out([]() {
        const char* path = std::getenv("AWLS_TRACE_FILE");
        return path != nullptr && path[0] != '\0' ? std::string(path) : std::string("awls_trace.log");
    }(), std::ios::out | std::ios::trunc);
    return out;
}

inline std::mutex& trace_mutex()
{
    static std::mutex mtx;
    return mtx;
}

template <typename... Args>
inline void log(Args&&... args)
{
    if (!enabled())
    {
        return;
    }

    std::ostringstream oss;
    (oss << ... << args);
    std::lock_guard<std::mutex> lock(trace_mutex());
    stream() << oss.str() << '\n';
    stream().flush();
}
}

#endif
