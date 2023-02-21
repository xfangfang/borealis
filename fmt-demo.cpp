#include <fmt/core.h>

enum class LogLevel
{
    LOG_ERROR = 0,
    LOG_WARNING,
    LOG_INFO,
    LOG_DEBUG,
    LOG_VERBOSE
};

template <typename... T>
void log(const LogLevel level, const std::string prefix, const std::string color, fmt::format_string<T...> format, T&&... args) {
    fmt::print("{}", prefix);
    fmt::print(format, std::forward<T>(args)...);
}


template <typename... T>
static void error(fmt::format_string<T...> format, T&&... args)
{
    log(LogLevel::LOG_ERROR, "ERROR", "[0;31m", format, args...);
}

int main() {
    error("error: performSyncTasks: {}", 1);
}

