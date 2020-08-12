#ifndef LOGGER_HPP__
#define LOGGER_HPP__

#include <memory>
#include <unordered_map>
#include <vector>

#include <spdlog/logger.h>
#include <spdlog/sinks/base_sink.h>

struct Logger final
{
    static Logger& Instance();

private:
    Logger();

public:

    template <typename T = spdlog::sinks::sink, typename... Args>
    std::shared_ptr<T> RegisterSink(Args&&... args) {
        return std::dynamic_pointer_cast<T>(_sinks.emplace_back(std::make_shared<T>(std::forward<Args&&>(args)...)));
    }

    template <typename T = spdlog::logger>
    std::shared_ptr<T> RegisterLogger(const std::string& loggerName, spdlog::sinks_init_list sinks) {
        auto [itr, success] = _loggers.emplace(std::piecewise_construct,
            std::forward_as_tuple(loggerName),
            std::forward_as_tuple(std::make_shared<T>(loggerName, sinks)));

        if (!success)
            return nullptr;

        return std::dynamic_pointer_cast<T>(itr->second);
    }

    template <typename T = spdlog::logger>
    std::shared_ptr<T> GetLogger(std::string const& loggerName) {
        auto itr = _loggers.find(loggerName);
        if (itr == _loggers.end())
            return nullptr;

        return std::dynamic_pointer_cast<T>(itr->second);
    }

private:
    std::vector<spdlog::sink_ptr> _sinks;
    std::unordered_map<std::string, std::shared_ptr<spdlog::logger>> _loggers;
};

#endif // LOGGER_HPP__
