/*
    Copyright 2019 natinusala
    Copyright 2019 p-sam

    Licensed under the Apache License, Version 2.0 (the "License");
    you may not use this file except in compliance with the License.
    You may obtain a copy of the License at

        http://www.apache.org/licenses/LICENSE-2.0

    Unless required by applicable law or agreed to in writing, software
    distributed under the License is distributed on an "AS IS" BASIS,
    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
    See the License for the specific language governing permissions and
    limitations under the License.
*/

#pragma once

#ifdef ANDROID
#include <android/log.h>
#endif

#ifdef __PSV__
#include <psp2/kernel/clib.h>
#endif

#ifdef PS4
#include <orbis/libkernel.h>
#include <borealis/platforms/ps4/ps4_sysmodule.hpp>
#endif

#include <fmt/core.h>
#include <fmt/chrono.h>

#include <borealis/core/event.hpp>
#include <string>

namespace brls
{

enum class LogLevel
{
    LOG_ERROR = 0,
    LOG_WARNING,
    LOG_INFO,
    LOG_DEBUG,
    LOG_VERBOSE
};

#ifdef IOS
#define BRLS_ERROR_COLOR "🔴"
#define BRLS_WARNING_COLOR "🟠"
#define BRLS_INFO_COLOR "🔵"
#define BRLS_DEBUG_COLOR "🟢"
#define BRLS_VERBOSE_COLOR "⚪️"
#else
#define BRLS_ERROR_COLOR "[0;31m"
#define BRLS_WARNING_COLOR "[0;33m"
#define BRLS_INFO_COLOR "[0;34m"
#define BRLS_DEBUG_COLOR "[0;32m"
#define BRLS_VERBOSE_COLOR "[0;37m"
#endif

static inline std::string print(const unsigned char* str)
{
    if (str)
    {
        return std::string { (const char*)str };
    }
    return "";
}

class Logger
{
  public:
    static void setLogLevel(LogLevel logLevel);

    template <typename... Args>
    inline static void log(LogLevel level, std::string prefix, std::string color, fmt::format_string<Args...> format, Args&&... args)
    {
        if (Logger::logLevel < level)
            return;

        auto now    = std::chrono::system_clock::now();
        uint64_t ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            now.time_since_epoch()).count() % 1000;
#ifdef PS4
        OrbisDateTime lt{};
        if (sceRtcGetCurrentClockLocalTime)
            sceRtcGetCurrentClockLocalTime(&lt);
#else
        std::tm time_tm = fmt::localtime(std::chrono::system_clock::to_time_t(now));
#endif
        std::string log = fmt::format(format, std::forward<Args>(args)...);

        try
        {
#ifdef IOS
            fmt::print("{:%H:%M:%S}.{:03d} {} {}\n", time_tm, (int)ms, color, log);
#elif defined(ANDROID)
            __android_log_print(6 - (int)level, "borealis", "%s\n", log.c_str());
#elif defined(__PSV__)
            sceClibPrintf("%02d:%02d:%02d.%03d\033%s[%s]\033[0m %s\n", time_tm.tm_hour, time_tm.tm_min, time_tm.tm_sec, (int)ms, color.c_str(), prefix.c_str(), log.c_str());
#elif defined(PS4)
            sceKernelDebugOutText(0, fmt::format("{:02d}:{:02d}:{:02d}.{:03d}\033{}[{}]\033[0m {}\n", lt.hour, lt.minute, lt.second, (int)ms, color, prefix, log).c_str());
#else
            fmt::print("{:%H:%M:%S}.{:03d}\033{}[{}]\033[0m {}\n", time_tm, (int)ms, color, prefix, log);
#endif

            logEvent.fire(level, log);
        }
        catch (const std::exception& e)
        {
            // will be printed after the first fmt::print (so after the log tag)
            printf("! Invalid log format string: \"%s\": %s\n", fmt::basic_string_view<char>(format).data(), e.what());
        }

#ifdef __MINGW32__
        fflush(0);
#endif
    }

    template <typename... Args>
    inline static void error(fmt::format_string<Args...> format, Args&&... args)
    {
        Logger::log(LogLevel::LOG_ERROR, "ERROR", BRLS_ERROR_COLOR, format, std::forward<Args>(args)...);
    }

    template <typename... Args>
    inline static void warning(fmt::format_string<Args...> format, Args&&... args)
    {
        Logger::log(LogLevel::LOG_WARNING, "WARNING", BRLS_WARNING_COLOR, format, std::forward<Args>(args)...);
    }

    template <typename... Args>
    inline static void info(fmt::format_string<Args...> format, Args&&... args)
    {
        Logger::log(LogLevel::LOG_INFO, "INFO", BRLS_INFO_COLOR, format, std::forward<Args>(args)...);
    }

    template <typename... Args>
    inline static void debug(fmt::format_string<Args...> format, Args&&... args)
    {
        Logger::log(LogLevel::LOG_DEBUG, "DEBUG", BRLS_DEBUG_COLOR, format, std::forward<Args>(args)...);
    }

    template <typename... Args>
    inline static void verbose(fmt::format_string<Args...> format, Args&&... args)
    {
        Logger::log(LogLevel::LOG_VERBOSE, "VERBOSE", BRLS_VERBOSE_COLOR, format, std::forward<Args>(args)...);
    }

    static Event<LogLevel, std::string>* getLogEvent()
    {
        return &logEvent;
    }

  private:
    inline static LogLevel logLevel = LogLevel::LOG_INFO;
    inline static Event<LogLevel, std::string> logEvent;
};

} // namespace brls
