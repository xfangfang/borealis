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

#ifdef PS5
#include <unistd.h>
#endif

#include <borealis/core/event.hpp>
#include <cstdio>
#include <mutex>
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

#define BRLS_LOG_ERROR(format, ...) brls::Logger::error("{}:{} " format, __FILE__, __LINE__, ##__VA_ARGS__)
#define BRLS_LOG_WARNING(format, ...) brls::Logger::warning("{}:{} " format, __FILE__, __LINE__, ##__VA_ARGS__)
#define BRLS_LOG_INFO(format, ...) brls::Logger::info("{}:{} " format, __FILE__, __LINE__, ##__VA_ARGS__)
#define BRLS_LOG_DEBUG(format, ...) brls::Logger::debug("{}:{} " format, __FILE__, __LINE__, ##__VA_ARGS__)
#define BRLS_LOG_VERBOSE(format, ...) brls::Logger::verbose("{}:{} " format, __FILE__, __LINE__, ##__VA_ARGS__)

class Logger
{
  public:
    using TimePoint = std::chrono::system_clock::time_point;

    static void setLogLevel(LogLevel logLevel);

    static LogLevel getLogLevel();

    static void setLogOutput(std::FILE *logOut);

    /**
     * If sets to true, each log operation will lock a mutex, making the Logger thread-safe.
     */
    static void setThreadSafeLogging(bool threadSafeLogging);

    /**
     * Bounds the size of the file the log is written to.
     *
     * When the output is a regular file and it grows past this many bytes, only
     * warnings and errors are written from then on, so a title that runs for
     * hours on a console - where debug logging is on and nothing ever rotates
     * the file - cannot fill the storage. 0 keeps the log unbounded.
     */
    static void setLogFileLimit(std::size_t limit) { Logger::logFileLimit = limit; }

    /**
     * The handle the console's own log collects, or -1 to turn the copy off.
     *
     * A console title has no reader for its standard streams, so the platform
     * moves them into a file. Warnings and errors are written to this handle as
     * well, which is what a log server on the network shows while the title
     * runs, and the only trace that survives a title that stops without
     * reaching its file.
     */
    static void setConsoleLogHandle(int handle) { Logger::consoleLog = handle; }

    template <typename... Args>
    inline static void log(LogLevel level, std::string prefix, std::string color, fmt::format_string<Args...> format, Args&&... args)
    {
        if (Logger::logLevel < level)
            return;

        if (Logger::logFileLimit != 0 && level < LogLevel::LOG_WARNING) {
            if (Logger::logFull)
                return;
            // Measuring the file costs a call, so it is only done every now and
            // then rather than for every line.
            if ((++Logger::logWrites & 0xFF) == 0) {
                long size = Logger::logOut == nullptr ? -1 : std::ftell(Logger::logOut);
                if (size > 0 && (std::size_t)size >= Logger::logFileLimit) {
                    Logger::logFull = true;
                    Logger::log(LogLevel::LOG_WARNING, "WARNING", BRLS_WARNING_COLOR,
                        "log file reached {} bytes, only warnings and errors follow", (std::size_t)size);
                }
            }
        }

        TimePoint now = std::chrono::system_clock::now();
        uint64_t ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            now.time_since_epoch()).count() % 1000;
#ifdef PS4
        OrbisDateTime lt{};
        if (sceRtcGetCurrentClockLocalTime)
            sceRtcGetCurrentClockLocalTime(&lt);
#else
        std::time_t tt = std::chrono::system_clock::to_time_t(now);
        std::tm time_tm = *std::localtime(&tt);
#endif
        std::string log = fmt::format(format, std::forward<Args>(args)...);

        std::unique_lock<std::mutex> lock;
        if (Logger::threadSafeLogging)
            lock = std::unique_lock { logMtx };

        try
        {
#ifdef IOS
            fmt::print(logOut, "{:%H:%M:%S}.{:03d} {} {}\n", time_tm, (int)ms, color, log);
#elif defined(ANDROID)
            __android_log_print(6 - (int)level, "borealis", "%02d:%02d:%02d.%03d %s\n", time_tm.tm_hour, time_tm.tm_min, time_tm.tm_sec, (int)ms, log.c_str());
#elif defined(__PSV__)
            sceClibPrintf("%02d:%02d:%02d.%03d\033%s[%s]\033[0m %s\n", time_tm.tm_hour, time_tm.tm_min, time_tm.tm_sec, (int)ms, color.c_str(), prefix.c_str(), log.c_str());
#elif defined(PS4)
            sceKernelDebugOutText(0, fmt::format("{:02d}:{:02d}:{:02d}.{:03d}\033{}[{}]\033[0m {}\n", lt.hour, lt.minute, lt.second, (int)ms, color, prefix, log).c_str());
#else
            fmt::print(logOut, "{:%H:%M:%S}.{:03d}\033{}[{}]\033[0m {}\n", time_tm, (int)ms, color, prefix, log);
#endif

#ifdef PS5
            // Warnings and errors also go to the console's own log, without
            // the colours: it is read by a log server, not by a terminal.
            if (Logger::consoleLog >= 0 && level <= LogLevel::LOG_WARNING) {
                const std::string line = fmt::format("{:02d}:{:02d}:{:02d}.{:03d} [{}] {}\n",
                    time_tm.tm_hour, time_tm.tm_min, time_tm.tm_sec, (int)ms, prefix, log);
                const ssize_t ignored = ::write(Logger::consoleLog, line.data(), line.size());
                (void)ignored;
            }
#endif

            logEvent.fire(now, level, log);
        }
        catch (const std::exception& e)
        {
            // will be printed after the first fmt::print (so after the log tag)
            printf("! Invalid log format string: \"%s\": %s\n", fmt::basic_string_view<char>(format).data(), e.what());
        }

#ifdef __MINGW32__
        fflush(logOut);
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

    static Event<TimePoint, LogLevel, std::string>* getLogEvent()
    {
        return &logEvent;
    }

  private:
    inline static std::mutex logMtx;
    inline static bool threadSafeLogging = true;
    inline static Event<TimePoint, LogLevel, std::string> logEvent;
    inline static std::FILE *logOut = stdout;
    inline static LogLevel logLevel = LogLevel::LOG_INFO;
    inline static std::size_t logFileLimit = 0;
    inline static std::size_t logWrites = 0;
    inline static bool logFull = false;
    inline static int consoleLog = -1;
};

} // namespace brls
