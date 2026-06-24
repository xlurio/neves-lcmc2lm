#ifndef MCC2LM_LOGGER_HPP

#define MCC2LM_LOGGER_HPP

#include <format>
#include <iostream>
#include <mutex>
#include <string>
#include <utility>
#include <mcc2lm/constants.hpp>
#include <mcc2lm/exceptions.hpp>

#ifndef MCC2LM_LOG_LEVEL
#define MCC2LM_LOG_LEVEL mcc2lm::LogLevel::INFO
#endif

namespace mcc2lm
{
    class Logger
    {
        std::string logger_name;

        static std::string level_to_string(LogLevel level)
        {
            switch (level)
            {
            case LogLevel::DEBUG:
                return "DEBUG";
            case LogLevel::INFO:
                return "INFO";
            case LogLevel::WARNING:
                return "WARNING";
            case LogLevel::ERROR:
                return "ERROR";
            case LogLevel::CRITICAL:
                return "CRITICAL";
            }

            throw AssertionError("Unknown level");
        }

        template <typename... T>
        void Log(LogLevel level, std::string format_str, T &&...args);

    public:
        Logger(std::string name = "mcc2lm") : logger_name(std::move(name)) {}

        template <typename... T>
        void Debug(std::string format_str, T &&...args);

        template <typename... T>
        void Info(std::string format_str, T &&...args);

        template <typename... T>
        void Warning(std::string format_str, T &&...args);

        template <typename... T>
        void Error(std::string format_str, T &&...args);

        template <typename... T>
        void Critical(std::string format_str, T &&...args);
    };

    template <typename... T>
    void Logger::Log(LogLevel level, std::string format_str, T &&...args)
    {
        if (level < MCC2LM_LOG_LEVEL)
        {
            return;
        }

        const std::string rendered = std::vformat(format_str, std::make_format_args(args...));
        const std::string line = "[" + level_to_string(level) + "] " + logger_name + ": " + rendered;

        static std::mutex sink_mutex;
        std::lock_guard<std::mutex> lock(sink_mutex);

        std::ostream &sink = (level >= LogLevel::ERROR) ? std::cerr : std::clog;
        sink << line << '\n';
    }

    template <typename... T>
    void Logger::Debug(std::string format_str, T &&...args)
    {
        Log(LogLevel::DEBUG, std::move(format_str), std::forward<T>(args)...);
    }

    template <typename... T>
    void Logger::Info(std::string format_str, T &&...args)
    {
        Log(LogLevel::INFO, std::move(format_str), std::forward<T>(args)...);
    }

    template <typename... T>
    void Logger::Warning(std::string format_str, T &&...args)
    {
        Log(LogLevel::WARNING, std::move(format_str), std::forward<T>(args)...);
    }

    template <typename... T>
    void Logger::Error(std::string format_str, T &&...args)
    {
        Log(LogLevel::ERROR, std::move(format_str), std::forward<T>(args)...);
    }

    template <typename... T>
    void Logger::Critical(std::string format_str, T &&...args)
    {
        Log(LogLevel::CRITICAL, std::move(format_str), std::forward<T>(args)...);
    }
}

#endif
