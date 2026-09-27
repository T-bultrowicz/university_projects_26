#include "parser.hpp"
#include "server_lib.hpp"

#include <signal.h>

static constexpr int FDS_SIZE = 2;

// Returns string in lower case.
std::string to_lower(const std::string & str) {
    std::string res(str);
    for (char& c : res) {
        c = std::tolower(c);
    }
    return res;
}

// Saves cookies from header into connection object.
void save_cookies(const HTTPHeader & header, Connection & connection) {
    for (const auto& [key, value] : header.headers) {
        std::string key_lower = to_lower(key);
        if (key_lower == "set-cookie") {
            std::string cookie = 
                parser_detail::split_by_delimiter(value, ";").first;
            parser_detail::trim(cookie);
            connection->add_option("Cookie", cookie);
        }
    }
}

// Set's up connection object and configures it according to our options.
// Exits with error in case of failure.
Connection new_connection(const ParsedUrl & url, int domain, bool mlpxng) {
    Resultconn conn = establish_conn(url, domain);
    if (std::holds_alternative<conn_error_t>(conn)) {
        Logger::get_instance() << LogLevel::ERRORS 
                        << std::get<conn_error_t>(conn);
        exit(1);
    }
    Connection res = std::move(std::get<Connection>(conn));
    res->add_option("Connection", "Keep-Alive");
    if (mlpxng)
        res->add_option("Icy-MetaData", "1");
    return res;
}

// Gets info about where to redirect from parsed header.
ParsedUrl get_redirect_info(const HTTPHeader & header) {
    Parser::ResultUrl res_url;
    for (const auto& [key, value] : header.headers) {
        std::string key_lower = to_lower(key);
        if (key_lower == "location") {
            res_url = Parser::parse_url(value);
            break;
        }
    }
    if (std::holds_alternative<Parser::entry_err_t>(res_url)) {
        Logger::get_instance() << LogLevel::ERRORS 
                        << std::get<Parser::entry_err_t>(res_url);
        exit(1);
    }
    return std::get<ParsedUrl>(res_url);
}

// Handles info received from a connection read stream.
void handle_read_info(const ReadInfo & info, bool & keep_working, int & exit_code) {
    Logger& logger = Logger::get_instance();
    switch (info) {
        case ReadInfo::SYSTEM_ERROR:
            logger << LogLevel::ERRORS << "Error while reading from server!\n";
            keep_working = false;
            exit_code = 1;
            break;
        case ReadInfo::INVALID_RESPONSE:
            logger << LogLevel::WARNINGS << "Invalid response from the server!\n";
            keep_working = false;
            exit_code = 1;
            break;
        case ReadInfo::SERVER_CLOSED:
            logger << LogLevel::CRITICAL << "Server closed the connection!\n";
            keep_working = false;
            break;
        default:
            break;
    }
}

// Saves metaint special byte for extracting metadata.
uint64_t save_metaint(const HTTPHeader & header) {
    for (const auto& [key, value] : header.headers) {
        std::string key_lower = to_lower(key);
        if (key_lower == "icy-metaint") {
            auto maybe_metaint = 
                    parser_detail::read_unumber<uint32_t>(value.c_str());
            if (maybe_metaint.has_value()) {
                return *maybe_metaint;
            } else {
                Logger::get_instance() << LogLevel::WARNINGS << 
                    "Invalid Icy-MetaInt value, ignoring it.\n";
            }
        }
    }
    return std::string::npos;
}

// For multiplexing.
enum class ReadState : uint8_t {
    AUDIO,
    META_DATA,
    SPECIAL_BIT
};

// For state machine inside the main loop.
enum class NetState { 
    SENDING, 
    READING, 
    WANT_READ_DURING_SEND, 
    WANT_WRITE_DURING_SEND 
};

// Overly complicated due to unblocking SSL_read && SSL_write nature.
bool should_read(NetState state, int revents) {
    return (state == NetState::READING && (revents & POLLIN));
}

bool should_write(NetState state, int revents) {
    if ((state == NetState::SENDING || state == NetState::WANT_WRITE_DURING_SEND)
        && (revents & POLLOUT)) {
        return true;
    }

    if (state == NetState::WANT_READ_DURING_SEND && (revents & POLLIN)) {
        return true;
    }
    return false;
}

// Filters metadata and prints audio + metadata accordingly.
// Changes ReadState and size_t to values correct after parsing str.
void filter_metadata
(std::string_view str, ReadState & rs, size_t & until, size_t metaint) {
    constexpr size_t MULTIPLIER = 16;
    size_t pos = 0, n = str.size();
    while (pos < n) {
        size_t left = n - pos;
        if (rs == ReadState::AUDIO) {
            if (until > left) {
                until -= left;
                std::cout << str.substr(pos);
                pos = n;
            } else {
                std::cout << str.substr(pos, until);
                rs = ReadState::SPECIAL_BIT;
                pos += until;
            }
        } else if (rs == ReadState::SPECIAL_BIT) {
            until = static_cast<uint8_t>(str[pos]) * MULTIPLIER;
            ++pos;
            rs = ReadState::META_DATA;
        } else {
            std::string_view to_send;
            if (until > left) {
                to_send = str.substr(pos);
                pos = n;
                until -= left;
            } else {
                to_send = str.substr(pos, until);
                rs = ReadState::AUDIO;
                pos += until;
                until = metaint;
            }
            std::clog << to_send;
            if (to_send.empty() == false) std::clog << '\n';
        }
    }
}

int main(int argc, char* const argv[]) {
    // Process input.
    Parser::ResultCmdLine result = Parser::parse_cmdline_args(argc, argv);
    if (std::holds_alternative<Parser::entry_err_t>(result)) {
        std::clog << std::get<Parser::entry_err_t>(result) << std::endl;
        return 1;
    }
    ProgramEntry entry = std::get<ProgramEntry>(result);
    bool mlpxng = entry.multiplexing;

    // Get logger insance.
    Logger & logger = Logger::get_instance();
    logger.set_verbosity(entry.verbosity);

    // Parse URL given.
    Parser::ResultUrl url_result = Parser::parse_url(entry.server_url);
    if (std::holds_alternative<Parser::entry_err_t>(url_result)) {
        logger << LogLevel::EXTRA_QUIET << std::get<Parser::entry_err_t>(url_result);
        return 1;
    }
    ParsedUrl parsed_url = std::get<ParsedUrl>(url_result);
    logger << LogLevel::DEBUG << "Done Parsing!\n" <<
        std::format("Args ==============\n{}\nURL ==============\n{}\n", 
            entry.to_str(), parsed_url.to_str());

    // Block SIGINT to check it with sigpending().
    sigset_t set; sigemptyset(&set); sigaddset(&set, SIGINT);
    if (sigprocmask(SIG_BLOCK, &set, nullptr) < 0) {
        logger << LogLevel::WARNINGS << format_error("Failed to block SIGINT, ")
        << "\nIt may cause issues with Ctrl+C handling!\n";
    }

    // Ignore SIGPIPE to prevent crashes when writing to closed socket.
    struct sigaction sa{};
    sa.sa_handler = SIG_IGN;
    if (sigaction(SIGPIPE, &sa, nullptr) < 0) {
        logger << LogLevel::WARNINGS << format_error("Failed to ignore SIGPIPE, ")
        << "\nIt may cause crashes when writing to closed socket!\n";
    }

    // Establish communication with a server. Prepare stdin.
    Connection connection = new_connection(parsed_url, entry.domain, mlpxng);
    if (switch_socket_mode(STDIN_FILENO, false) == false) {
        logger << LogLevel::WARNINGS << "Failed to set non-blocking mode for"
        << " stdin - it may cause block in future!\n";
    }

    // Prepare communication using poll.
    struct pollfd fds[FDS_SIZE]; // 0 - stdin, 1 - server
    fds[0].fd = STDIN_FILENO, fds[1].fd = connection->fd();
    fds[0].events = fds[1].events = POLLIN; 
    fds[0].revents = fds[1].revents = 0;
    std::string buf; // User input.

    // Used for meta-data filtering iff -m enabled.
    size_t until_change = std::string::npos, interval; 
    ReadState mode = ReadState::AUDIO;
    // Variables that handle logic of work flow.
    NetState net_mode = NetState::SENDING;
    bool keep_working = true; int exit_code = 0;
    while (keep_working) {
        // Set POLLOUT flag only when sending message to the server is required.
        if (net_mode == NetState::WANT_WRITE_DURING_SEND 
                || net_mode == NetState::SENDING) {
            fds[1].events = POLLOUT;
        } else {
            fds[1].events = POLLIN;
        }

        int poll_res = poll(fds, FDS_SIZE, entry.timeout_ms);
        // Handle errors and timeouts.
        if (poll_res < 0) {
            logger << LogLevel::ERRORS << format_error("Error while polling:") << '\n';
            exit_code = 1; break;
        } else if (poll_res == 0) {
            logger << LogLevel::CRITICAL << "data receiving timeout\n";
            connection = new_connection(parsed_url, entry.domain, mlpxng);
            fds[1].fd = connection->fd();
            net_mode = NetState::SENDING;
            mode = ReadState::AUDIO; until_change = std::string::npos;
            continue;
        }

        // Handle sending message to the server.
        if (should_write(net_mode, fds[1].revents)) {
            WriteInfo info = connection->send_message();
            switch(info) {
                case WriteInfo::SUCCESS:
                    net_mode = NetState::READING;
                    break;
                case WriteInfo::PARTIAL:
                    logger << LogLevel::WARNINGS << 
                    "Only partial message sent to the server!\n";
                    net_mode = NetState::SENDING;
                    break;
                case WriteInfo::WBLOCK_WR:
                    net_mode = NetState::WANT_WRITE_DURING_SEND;
                    break;
                case WriteInfo::WBLOCK_RD:
                    net_mode = NetState::WANT_READ_DURING_SEND;
                    break;
                case WriteInfo::SERVER_CLOSED:
                    logger << LogLevel::CRITICAL << 
                                "Server closed the connection!\n";
                    keep_working = false;
                    break;
                case WriteInfo::SYSTEM_ERROR:
                    logger << LogLevel::ERRORS <<
                    "Error while sending message to the server!\n";
                    keep_working = false; exit_code = 1;
                    break;
                default:
                    break;
            }
        }

        // Handle receiving message from the server and user input.
        if (fds[0].revents & POLLIN) {
            char input_chunk[16] = {0};
            ssize_t read_bytes = read(STDIN_FILENO, input_chunk, 
                                                sizeof(input_chunk));

            if (read_bytes < 0) {
                if (errno != EAGAIN && errno != EWOULDBLOCK) {
                    logger << LogLevel::WARNINGS << 
                    format_error("Error reading stdin");
                }
            } else if (read_bytes == 0) {
                fds[0].fd = -1;
            } else {
                buf.append(input_chunk, read_bytes);
                size_t newline_pos;
                while ((newline_pos = buf.find('\n')) != std::string::npos) {
                    std::string line = buf.substr(0, newline_pos);
                    buf.erase(0, newline_pos + 1);
                    parser_detail::trim(line);

                    if (line == "quit") {
                        logger << LogLevel::CRITICAL << "Quitting...\n";
                        keep_working = false;
                        break; 
                    } else if (!line.empty()) {
                        logger << LogLevel::WARNINGS << "Unknown command: " 
                        << line << "\n" << "Supported commands: [quit]\n";
                    }
                }
            }
        }

        if (should_read(net_mode, fds[1].revents)) {
            connection->receive_message();

            bool keep_reading = true;
            while (keep_reading) {
                auto message = connection->read_next();
                if (std::holds_alternative<ReadInfo>(message)) {
                    handle_read_info(std::get<ReadInfo>(message), 
                                                keep_working, exit_code);
                    keep_reading = false;
                } else if (std::holds_alternative<HTTPHeader>(message)) {
                    HTTPHeader header = std::get<HTTPHeader>(message);
                    logger << LogLevel::CRITICAL << '\n' << header.to_str() << '\n';
                    if (header.response_code >= 400) {
                        logger << LogLevel::ERRORS << "Server responded" <<
                        " with an error code: " << header.response_code << '\n';
                        keep_working = false; exit_code = 1;
                    } else if (header.response_code >= 300) {
                        ParsedUrl url = get_redirect_info(header);

                        connection = new_connection(url, entry.domain, mlpxng);
                        fds[1].fd = connection->fd();
                        net_mode = NetState::SENDING;
                        mode = ReadState::AUDIO; until_change = std::string::npos;
                        keep_reading = false;
                    }
                    save_cookies(header, connection);
                    if (mlpxng) {
                        until_change = interval = save_metaint(header);
                        logger << LogLevel::DEBUG << "MetaInt = " << interval << '\n';
                    }
                } else {
                    std::string body = std::get<HTTPBody>(message).body;
                    if (mlpxng && until_change != std::string::npos) {
                        filter_metadata(body, mode, until_change, interval);
                    } else {
                        std::cout << body;
                    }

                    if (std::cout.fail()) {
                        Logger::get_instance() << LogLevel::ERRORS 
                        << "Error writing to stdout. Closing.\n";
                        keep_working = false;
                        exit_code = 1;
                    }
                }
            }
        }

        // Check if SIGINT arrived
        sigset_t pending;
        if (sigpending(&pending) < 0) {
            logger << LogLevel::WARNINGS 
            << format_error("Failed to check pending signals! ");
        }
        if (sigismember(&pending, SIGINT)) {
            logger << LogLevel::WARNINGS << "SIGINT received, quitting...\n";
            keep_working = false;
        }
    }
    // Ensure all output buffers are flushed before exiting
    std::cout.flush();
    std::clog.flush();
    std::cerr.flush();

    return exit_code;
}