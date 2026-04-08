#pragma once
// Logger — simple file-based logger.
// Call Logger::init() once at startup to create a timestamped log file in
// logs/ relative to the executable.  Then use Logger::info/warn/error from
// any thread.

#include <filesystem>
#include <fstream>
#include <mutex>
#include <string>
#include <string_view>

namespace sag
{

    class Logger
    {
    public:
        Logger() = delete;

        // Creates logs/ directory and opens a new timestamped log file.
        // Must be called before any logging; safe to call multiple times (no-op after first).
        static void init(const std::filesystem::path &exe_dir);

        static void info(std::string_view message);
        static void warn(std::string_view message);
        static void error(std::string_view message);

    private:
        enum class Level
        {
            Info,
            Warn,
            Error
        };
        static void write(Level level, std::string_view message);

        static std::ofstream file_;
        static std::mutex mutex_;
        static bool initialized_;
    };

} // namespace sag
