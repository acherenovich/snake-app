#pragma once

#include <iostream>
#include <memory>
#include <string>
#include <mutex>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <chrono>
#include <format>

namespace Utils {

    class Log : public std::enable_shared_from_this<Log> {
    public:
        using Shared = std::shared_ptr<Log>;

        enum class Level {
            Message,
            Debug,
            Warning,
            Error,
            Trace,
        };

        Log() = default;

        explicit Log(const std::string& prefix_) : prefix(prefix_) {}

        ~Log() {
            if (logFile.is_open()) {
                logFile.close();
            }
        }

        void SetPrefix(const std::string& prefix_) {
            std::lock_guard<std::mutex> lock(mutex);
            prefix = prefix_;
            UpdateChildrenPrefix();
        }

        void SetLogPath(const std::string& path) {
            std::lock_guard<std::mutex> lock(mutex);
            if (!logFilePath.empty() && logFile.is_open()) {
                logFile.close();
            }
            logFile.open(path, std::ios::app);
            if (!logFile.is_open()) {
                throw std::runtime_error(std::format("Failed to open log file: {}", path));
            }
            logFilePath = path;
            UpdateChildrenLogPath();
        }

        Shared CreateChild(const std::string& childPrefix) {
            std::lock_guard<std::mutex> lock(mutex);
            auto childLogger = std::make_shared<Log>();
            childLogger->prefix = std::format("{} {}", prefix, childPrefix);
            childLogger->logFilePath = logFilePath; // Share the same log file path
            if (!logFilePath.empty()) {
                childLogger->SetLogPath(logFilePath); // Ensure child logger opens the same file
            }
            childLogger->parent = shared_from_this();
            children.push_back(childLogger);
            return childLogger;
        }

        template<typename... Args>
        void Msg(std::basic_format_string<char, Args...> format, Args&&... args) {
            Perform(Level::Message, std::format(format, std::forward<Args>(args)...));
        }

        template<typename... Args>
        void Msg(const std::string& format, Args&&... args) {
            Perform(Level::Message, std::vformat(format, std::make_format_args(args...)));
        }

        void Msg(const std::string& message) {
            Perform(Level::Message, message);
        }

        // Warning
        template<typename... Args>
        void Warning(std::basic_format_string<char, Args...> format, Args&&... args) {
            Perform(Level::Warning, std::format(format, std::forward<Args>(args)...));
        }

        template<typename... Args>
        void Warning(const std::string& format, Args&&... args) {
            Perform(Level::Warning, std::vformat(format, std::make_format_args(args...)));
        }

        void Warning(const std::string& message) {
            Perform(Level::Warning, message);
        }

        // Debug
        template<typename... Args>
        void Debug(std::basic_format_string<char, Args...> format, Args&&... args) {
            Perform(Level::Debug, std::format(format, std::forward<Args>(args)...));
        }

        template<typename... Args>
        void Debug(const std::string& format, Args&&... args) {
            Perform(Level::Debug, std::vformat(format, std::make_format_args(args...)));
        }

        void Debug(const std::string& message) {
            Perform(Level::Debug, message);
        }

        template<typename... Args>
        void Error(std::basic_format_string<char, Args...> format, Args&&... args) {
            Perform(Level::Error, std::format(format, std::forward<Args>(args)...));
        }

        // Перегрузка для строк формата (например, std::string)
        template<typename... Args>
        void Error(const std::string& format, Args&&... args) {
            Perform(Level::Error, std::vformat(format, std::make_format_args(args...)));
        }

        void Error(const std::string& message) {
            Perform(Level::Error, message);
        }

    private:
        void Perform(Level level, const std::string& message) {
            std::lock_guard<std::mutex> lock(mutex);

            // Формируем полный текст сообщения с префиксом
            std::string output = std::format("\033[1;35m{} {}[{}] •\033[0;37m {} {} \033[0m",
                                             FormattedDate(),
                                             LevelToColor(level),
                                             LevelToString(level),
                                             prefix,
                                             message
            );

            // Вывод в консоль
            std::cout << output << std::endl;

            // Запись в файл, если файл лога открыт
            if (logFile.is_open()) {
                logFile << StripColors(output) << std::endl;
            }
        }

        std::string FormattedDate() const {
            auto now = std::chrono::system_clock::now();
            auto itt = std::chrono::system_clock::to_time_t(now);
            std::tm tm{};
#if defined(_MSC_VER) || defined(__MINGW32__)
            localtime_s(&tm, &itt);
#else
            localtime_r(&itt, &tm);
#endif
            std::ostringstream oss;
            oss << std::put_time(&tm, "%Y-%m-%d %H:%M:%S");
            return oss.str();
        }

        std::string LevelToString(Level level) const {
            switch (level) {
                case Level::Message: return "MESSAGE";
                case Level::Debug: return "DEBUG";
                case Level::Warning: return "WARNING";
                case Level::Error: return "ERROR";
                case Level::Trace: return "TRACE";
                default: return "UNKNOWN";
            }
        }

        std::string LevelToColor(Level level) const {
            switch (level) {
                case Level::Message: return "\033[1;32m"; // Green
                case Level::Debug: return "\033[1;36m";   // Cyan
                case Level::Warning: return "\033[1;33m"; // Yellow
                case Level::Error: return "\033[1;31m";   // Red
                case Level::Trace: return "\033[1;35m";   // Magenta
                default: return "\033[0m";                // Reset
            }
        }

        std::string StripColors(const std::string& input) const {
            std::string result = input;
            result.erase(std::remove(result.begin(), result.end(), '\033'), result.end());
            return result;
        }

        void UpdateChildrenPrefix() {
            for (const auto& child : children) {
                child->SetPrefix(std::format("{} {}", prefix, child->prefix));
            }
        }

        void UpdateChildrenLogPath() {
            for (const auto& child : children) {
                child->SetLogPath(logFilePath);
            }
        }

        std::string prefix;
        std::string logFilePath; // Store the file path for re-opening in children
        std::weak_ptr<Log> parent;
        std::vector<Shared> children;
        mutable std::mutex mutex;
        std::ofstream logFile;
    };

    inline std::shared_ptr<Log> Logger(const std::string& prefix) {
        static auto logger = std::make_shared<Log>(prefix);
        return logger;
    }
}