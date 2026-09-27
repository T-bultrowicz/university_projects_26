#ifndef LIB_HPP
#define LIB_HPP

#include <arpa/inet.h>
#include <cinttypes>
#include <fcntl.h>
#include <format>
#include <iostream>
#include <limits>
#include <netdb.h>
#include <netinet/in.h>
#include <optional>
#include <poll.h>
#include <string>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <variant>
#include <vector>

// Options passed in http requests.
using str_pair = std::pair<std::string, std::string>;
using q_params_t = std::vector<str_pair>;

// Class representing level of verbosity.
enum class LogLevel : uint8_t {
    EXTRA_QUIET,
    CRITICAL,
    ERRORS,
    WARNINGS,
    DEBUG
};

// Singleton logger class that filters messages with importance lower 
// than the set verbosity level.
class Logger {
    private:
        Logger() = default;
        ~Logger() = default;

        LogLevel verbosity{};
        LogLevel curr_importance{};
    public:
        static Logger& get_instance() {
            static Logger instance;
            return instance;
        }

        void set_verbosity(LogLevel level) {
            verbosity = level;
        }

        Logger(const Logger&) = delete;
        Logger& operator=(const Logger&) = delete;

        template <typename T>
        friend Logger& operator<<(Logger& logger, const T& message);
        friend Logger& operator<<(Logger& logger, LogLevel level);
};

// Set importance of messages to be logged.
inline Logger& operator<<(Logger& logger, LogLevel level) {
    logger.curr_importance = level;
    return logger;
}

template <typename T>
Logger& operator<<(Logger& logger, const T& message) {
    if (logger.curr_importance <= logger.verbosity) {
        std::clog << message;
    }
    return logger;
}

#endif // LIB_HPP 