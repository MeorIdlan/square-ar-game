// Logger implementation

#include "logger.h"

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#endif

#include <chrono>
#include <ctime>
#include <format>
#include <iomanip>
#include <sstream>

namespace sag
{

    // Static member definitions
    std::ofstream Logger::file_;
    std::mutex Logger::mutex_;
    bool Logger::initialized_ = false;

    // Returns a timestamp string formatted as "YYYY-MM-DD HH:MM:SS.mmm"
    static std::string current_timestamp()
    {
        using namespace std::chrono;
        auto now = system_clock::now();
        auto ms = duration_cast<milliseconds>(now.time_since_epoch()) % 1000;
        auto time = system_clock::to_time_t(now);

        std::tm tm_buf{};
#ifdef _WIN32
        localtime_s(&tm_buf, &time);
#else
        localtime_r(&time, &tm_buf);
#endif

        return std::format("{:04d}-{:02d}-{:02d} {:02d}:{:02d}:{:02d}.{:03d}",
                           tm_buf.tm_year + 1900,
                           tm_buf.tm_mon + 1,
                           tm_buf.tm_mday,
                           tm_buf.tm_hour,
                           tm_buf.tm_min,
                           tm_buf.tm_sec,
                           static_cast<int>(ms.count()));
    }

    // Returns a filename-safe timestamp like "2026-04-09_14-30-00"
    static std::string run_timestamp()
    {
        using namespace std::chrono;
        auto now = system_clock::now();
        auto time = system_clock::to_time_t(now);

        std::tm tm_buf{};
#ifdef _WIN32
        localtime_s(&tm_buf, &time);
#else
        localtime_r(&time, &tm_buf);
#endif

        return std::format("{:04d}-{:02d}-{:02d}_{:02d}-{:02d}-{:02d}",
                           tm_buf.tm_year + 1900,
                           tm_buf.tm_mon + 1,
                           tm_buf.tm_mday,
                           tm_buf.tm_hour,
                           tm_buf.tm_min,
                           tm_buf.tm_sec);
    }

    void Logger::init(const std::filesystem::path &exe_dir)
    {
        std::filesystem::path log_dir = exe_dir / "logs";
        std::filesystem::create_directories(log_dir);

        std::filesystem::path log_path = log_dir / (run_timestamp() + ".log");

        std::lock_guard lock(mutex_);
        if (initialized_)
            return;

        file_.open(log_path, std::ios::out | std::ios::trunc);
        initialized_ = true;

        // Write the first line directly (mutex already held)
        std::string first = std::format("[{}] [INFO ] Log started — {}\n",
                                        current_timestamp(), log_path.string());
        if (file_.is_open())
        {
            file_ << first;
            file_.flush();
        }
#ifdef _WIN32
        OutputDebugStringA(first.c_str());
#endif
    }

    void Logger::info(std::string_view message) { write(Level::Info, message); }
    void Logger::warn(std::string_view message) { write(Level::Warn, message); }
    void Logger::error(std::string_view message) { write(Level::Error, message); }

    void Logger::write(Level level, std::string_view message)
    {
        const char *tag = (level == Level::Info) ? "INFO " : (level == Level::Warn) ? "WARN "
                                                                                    : "ERROR";
        std::string line = std::format("[{}] [{}] {}\n", current_timestamp(), tag, message);

        std::lock_guard lock(mutex_);
        if (file_.is_open())
        {
            file_ << line;
            file_.flush();
        }

        // Also emit to the debugger output on Windows (visible in VS Output window)
#ifdef _WIN32
        OutputDebugStringA(line.c_str());
#endif
    }

} // namespace sag
