#ifndef PARSER_HPP
#define PARSER_HPP

#include "lib.hpp"

// This namespace hides helper functions and constants used in the parsing process.
namespace parser_detail {
    // Timeout constants - in milliseconds.
    constexpr uint32_t DEFAULT_TIMEOUT = 5'000;
    constexpr uint32_t MIN_TIMEOUT = 100;
    constexpr uint32_t MAX_TIMEOUT = 100'000;

    // Safe read of unsigned numbers smaller than 64 bits.
    template <typename T>
    std::optional<T> read_unumber (char const * str) {
        char *endptr;
        errno = 0;
        unsigned long p = strtoul(str, &endptr, 10);
        if (errno != 0 || *endptr != 0 || p > std::numeric_limits<T>::max()) {
            return std::nullopt;
        }
        return static_cast<T>(p);
    }

    // Splits string into two parts by the first occurrence of a delimiter.
    str_pair split_by_delimiter(const std::string& str, std::string delimiter);

    // Trims leading and trailing whitespace characters from the string.
    void trim(std::string & str);
}

// Struct that represents parsed and verified command line arguments.
struct ProgramEntry {
    std::string server_url{};
    uint32_t    timeout_ms     = parser_detail::DEFAULT_TIMEOUT;
    LogLevel    verbosity      = LogLevel::ERRORS;
    int         domain         = AF_UNSPEC;
    bool        multiplexing{};

    std::string to_str() const {
        return std::format(
            "U: {}\nT: {}\nV: {}\nIPv: {}\nM: {}",
            server_url, timeout_ms, static_cast<int>(verbosity), domain, multiplexing
        );
    }
};

// Structs that represents parsed URL components after verification.
struct ParsedUrl {
    bool scheme_http;
    bool is_bracketed;
    std::string host;
    uint16_t port;
    std::string filepath;

    std::string to_str() const {
        std::string res = std::format(
            "Scheme: {}\nHost: {}\nPort: {}\nFilepath: {}\n",
            scheme_http ? "http" : "https", host, port, filepath
        );
        return res;
    }
};

// Class responsible for parsing and veryfing various parameters
// related to the program's execution, such as command line arguments and URL.
class Parser {
public:
    // Type alias for error messages due to incorrect format.
    using entry_err_t = std::string;
    // Result of parsing command line arguments.
    using ResultCmdLine = std::variant<ProgramEntry, entry_err_t>;
    // Result of parsing URL.
    using ResultUrl = std::variant<ParsedUrl, entry_err_t>;

    // Parses and verifies command line arguments.
    static ResultCmdLine parse_cmdline_args(int argc, char* const argv[]) {
        using namespace parser_detail;
        ResultCmdLine result = get_input(argc, argv);

        if (std::holds_alternative<entry_err_t>(result)) {
            return result;
        }

        ProgramEntry entry = std::get<ProgramEntry>(result);
        if (entry.timeout_ms < MIN_TIMEOUT || entry.timeout_ms > MAX_TIMEOUT) {
            return "Error: Timeout value must bebetween 100 and 100'000 (ms)";
        }
        uint8_t max_verb = static_cast<uint8_t>(LogLevel::DEBUG);
        if (static_cast<uint8_t>(entry.verbosity) > max_verb) {
            return "Error: Verbosity level must be between 0 and 4.";
        }
        if (entry.server_url.empty()) {
            return "Error: Server URL cannot be empty.";
        }

        return entry;
    }

    // Parses and verifies URL, checks its format and extracts components.
    // Igoners anchor and requires URL to be in format: 
    // URL = scheme://host[:port][/filepath][?query_params][#anchor]
    static ResultUrl parse_url(const std::string& str_url) {
        using namespace parser_detail;
        std::string url(str_url);
        trim(url);
        ParsedUrl res_url{};
        res_url.filepath = "/";

        // Parse scheme.
        auto [scheme, rest] = split_by_delimiter(url, "://");
        if (scheme != "http" && scheme != "https") {
            return "Error: URL scheme must be 'http' or 'https'.";
        }
        res_url.scheme_http = (scheme == "http");

        auto [host_port, next] = split_by_delimiter(rest, "/");
        rest = std::move(next);
        // Get host and port.
        std::string host, port_str;
        if (host_port.empty() == false && host_port[0] == '[') {
            // IPv6 address, host is between brackets.
            auto [parsed_host, parsed_port] = split_by_delimiter(host_port, "]");
            host = parsed_host.substr(1); // Remove opening bracket.
            auto [_, real_port] = split_by_delimiter(parsed_port, ":");
            port_str = std::move(real_port);
            res_url.is_bracketed = true;
        } else {
            auto [parsed_host, parsed_port] = split_by_delimiter(host_port, ":");
            host = std::move(parsed_host);
            port_str = std::move(parsed_port);
            res_url.is_bracketed = false;
        }

        if (host.empty()) {
            return "Error: Host cannot be empty.";
        }
        res_url.host = host;
        auto maybe_port = parse_port(port_str, res_url.scheme_http);
        if (maybe_port.has_value() == false) {
            return "Error: Invalid port number.";
        }
        res_url.port = *maybe_port;

        // Parse rest of the URL, from filepath onwards.
        auto [filepath, query_anchor_str] = split_by_delimiter(rest, "#");
        res_url.filepath.append(filepath);
        return res_url;
    }
private:
    // Private constructor prevents instantiation of the class.
    Parser() = default;

    // Basic parse of command line arguments, check only their format. 
    static ResultCmdLine get_input(int argc, char* const argv[]) {
        using namespace parser_detail;
        bool url_provided = false;
        ProgramEntry entry;
        const char optstring[] = ":qm46t:v:u:";
        int opt; entry_err_t err;

        while ((opt = getopt(argc, argv, optstring)) != -1) {
            if (opt == 'v') {
                auto maybe_v = read_unumber<uint8_t>(optarg);
                if (maybe_v.has_value() == false) {
                    return "Error: Incorrect verbosity parameter.";
                }
                entry.verbosity = static_cast<LogLevel>(*maybe_v);
            } else if (opt == 't') {
                auto maybe_v = read_unumber<uint32_t>(optarg);
                if (maybe_v.has_value() == false) {
                    return "Error: Incorrect timeout parameter.";
                }
                entry.timeout_ms = *maybe_v;
            } else switch(opt) {
                case 'u':
                    entry.server_url = optarg;
                    url_provided = true;
                    break;
                case '4':
                    entry.domain = (entry.domain == AF_INET6
                        ? AF_UNSPEC
                        : AF_INET
                    );
                    break;
                case '6':
                    entry.domain = (entry.domain == AF_INET
                        ? AF_UNSPEC
                        : AF_INET6
                    );
                    break;
                case 'm':
                    entry.multiplexing = true;
                    break;
                case 'q':
                    entry.verbosity = LogLevel::EXTRA_QUIET;
                    break;
                case ':':
                    err = std::format("Error: Option -{} requires an argument",
                                                                (char)optopt);
                    return err;
                case '?': 
                    err = std::format("Error: Unknown option - {}", 
                                                                (char)optopt);
                    return err;
            }
        }

        if (url_provided == false) {
            return "Error: Server URL is required.";
        }
        return entry;
    }

    // Get port value or default.
    static std::optional<uint16_t> parse_port(
        const std::string& port_str, bool scheme_http) {
        using namespace parser_detail;
        if (port_str.empty()) {
            return scheme_http ? 80 : 443;
        }
        return read_unumber<uint16_t>(port_str.c_str());
    }
};

#endif // PARSER_HPP