#ifndef AWLS_DEBUG_TRACE_H
#define AWLS_DEBUG_TRACE_H

#include <cstdlib>
#include <fstream>
#include <mutex>
#include <sstream>
#include <string>

namespace awls_trace
{
inline std::string get_env(const char* name)
{
#ifdef _WIN32
    char* buffer = nullptr;
    size_t length = 0;
    if (_dupenv_s(&buffer, &length, name) != 0 || buffer == nullptr)
    {
        return {};
    }
    std::string value(buffer);
    free(buffer);
    return value;
#else
    const char* value = std::getenv(name);
    return value != nullptr ? std::string(value) : std::string();
#endif
}

inline bool enabled()
{
    static const bool on = []() {
        const std::string value = get_env("AWLS_TRACE");
        return !value.empty() && value[0] != '0';
    }();
    return on;
}

inline std::ofstream& stream()
{
    static std::ofstream out([]() {
        const std::string path = get_env("AWLS_TRACE_FILE");
        return !path.empty() ? path : std::string("awls_trace.log");
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
