#ifndef SERVER_LIB_HPP
#define SERVER_LIB_HPP

#include "err.hpp"
#include "lib.hpp"
#include "parser.hpp"

#include <cstring>
#include <memory>
#include <openssl/ssl.h>
#include <openssl/err.h>

// Result of sending message to the server via Connection class.
enum class WriteInfo : uint8_t {
    SUCCESS,
    PARTIAL,
    WBLOCK_RD,
    WBLOCK_WR,
    SERVER_CLOSED,
    SYSTEM_ERROR
};

// Structs that represents parsed http message.
struct HTTPHeader {
    uint16_t response_code;
    std::string protocol;
    std::string explained_code;
    q_params_t headers;

    // Converts parsed response back to original HTTP.
    std::string to_str() const {
        std::string res = std::format(
            "{} {} {}\r\n",
            protocol, response_code, explained_code
        );

        for (const auto& [key, value] : headers) {
            res.append(key).append(": ").append(value).append("\r\n");
        }
        res.append("\r\n");
        return res;
    }
};

// Struct representing HTTPBody.
struct HTTPBody {
    std::string body;
};

// Struct informing about other events in read stream from connection class.
enum class ReadInfo : uint8_t {
    SYSTEM_ERROR,     // Socket error or other system-level issue.
    END_OF_STREAM,    // No complete message yet.
    SERVER_CLOSED,    // Server closed the connection (read() returns 0).
    INVALID_RESPONSE, // Received data cannot be parsed as valid HTTP response.
};
using MaybeResponse = std::variant<HTTPHeader, HTTPBody, ReadInfo>;

// Class responsible for exchanging data with the server and
// maintaining the state of that connection.
class IConnection {
private:
    std::string bracketted(const std::string & name) {
        return std::string("[").append(name).append("]");
    }
protected:
    int sockfd;
    std::string name;
    std::string send_buffer;;
    std::string filepath;
    q_params_t options{};

    // Variables used to maintain the state of messages.
    enum class DecodeState {
        WAITING_HEADER,
        WAITING_BODY
    };
    DecodeState current_state = DecodeState::WAITING_HEADER;
    std::string buffer;
    bool connection_closed = false;
    bool socket_error = false;
    ssize_t expected_length = -1;

    // Helper function that prints every option in the format "key: value\r\n".
    void add_options(std::string& m, const q_params_t& o) {
        for (const auto& [key, value] : o) {
            m.append(key).append(": ").append(value).append("\r\n");
        }
    }

    // Helper function that wraps message with correct header,
    // using header options saved in this object.
    std::string prepare_send(const std::string & message) {
        std::string data{"GET "};
        data.append(filepath).append(" HTTP/1.1\r\nHost: ").append(name);
        data.append("\r\n");
        add_options(data, options);
        data.append("\r\n").append(message);
        return data;
    }

    // Parses str expecting HTTP headers, adds them to response and returns expected
    // length of the body. Returns:
    //  (-2) - parse failed, (-1) - no content length, (>=0) - content length.
    ssize_t parse_options(const std::string& str, HTTPHeader & response) {
        using namespace parser_detail;
        ssize_t to_return = -1;
        auto [first_line, remaining] = split_by_delimiter(str, "\r\n");

        // Parse first line.
        auto [protocol, rest] = split_by_delimiter(first_line, " ");
        auto [code, explained_code] = split_by_delimiter(rest, " ");
        response.protocol = protocol;
        response.explained_code = explained_code;

        auto maybe_code = read_unumber<uint16_t>(code.c_str());
        if (maybe_code.has_value() == false) return -2;
        response.response_code = *maybe_code;

        // Parse next lines as options.
        std::string current_opt = remaining;
        while (current_opt.empty() == false) {
            auto [line, rest] = split_by_delimiter(current_opt, "\r\n");
            auto [key, value] = split_by_delimiter(line, ":");
            trim(key); trim(value);

            if (key == "Content-Length") {
                auto maybe_len = read_unumber<uint32_t>(value.c_str());
                if (maybe_len.has_value() == false) return -2;
                to_return = *maybe_len;
            }

            response.headers.emplace_back(key, value);
            current_opt = rest;
        }
        return to_return;
    }

public:
    IConnection(int fd, const ParsedUrl & url)
        : sockfd(fd),
            name(url.is_bracketed ? bracketted(url.host) : url.host),
            filepath(url.filepath) {}
    virtual ~IConnection() {
        close_con();
    }

    // Sends message to the server, returns WriteInfo = operation result.
    virtual WriteInfo send_message(const std::string& bdy = {}) = 0;

    // Receives message from the server, that can be read with read_next().
    virtual void receive_message() = 0;

    // Closes descryptor for socket connection.
    void close_con() {
        Logger& logger = Logger::get_instance();
        if (sockfd >= 0) {
            if (close(sockfd) < 0) {
                logger << LogLevel::WARNINGS << format_error(
                "Closing connection to %s failed ", name.c_str()) << '\n';
            }
            sockfd = -1;
        }
    }

    // Getters and setters for connection parameters.
    int fd() const { return sockfd;}
    void clear_options() { options.clear();}
    void add_option(const std::string& key, const std::string& value) {
        options.emplace_back(key, value);
    }

    // Designed to work like stream of parsed HTTP messages. Returns next:
    // full header / part of body / error info of that stream.
    MaybeResponse read_next() {
        Logger& logger = Logger::get_instance();
        if (socket_error) {
            logger << LogLevel::ERRORS << 
                "Cannot read from connection object due to previous error.\n";
            return ReadInfo::SYSTEM_ERROR;
        }
        
        if (current_state == DecodeState::WAITING_HEADER) {
            size_t end_pos = buffer.find("\r\n\r\n");
            if (end_pos != std::string::npos) {
                std::string header = buffer.substr(0, end_pos);
                buffer.erase(0, end_pos + 4);

                HTTPHeader res;
                expected_length = parse_options(header, res);
                if (expected_length == -2) {
                    return ReadInfo::INVALID_RESPONSE;
                }
                current_state = DecodeState::WAITING_BODY;
                return res;
            } else {
                if (connection_closed) {
                    return ReadInfo::SERVER_CLOSED;
                }
                return ReadInfo::END_OF_STREAM;
            }
        } else { // WAITING_BODY
            if (expected_length != -1) { // We count expected message length.
                if (connection_closed) {
                    if (buffer.empty() == false) {
                        HTTPBody body; body.body = std::move(buffer);
                        buffer.clear();
                        return body;
                    }
                    return ReadInfo::SERVER_CLOSED;
                }
                if (buffer.empty()) return ReadInfo::END_OF_STREAM;

                size_t len = buffer.size();
                HTTPBody res;
                if (len >= static_cast<size_t>(expected_length)) {
                    res = HTTPBody{buffer.substr(0, expected_length)};
                    buffer.erase(0, expected_length);
                    expected_length = -1;
                    current_state = DecodeState::WAITING_HEADER;
                } else {
                    expected_length -= len;
                    res = HTTPBody{buffer};
                    buffer.clear();
                }
                return res;
            } 
            else { // We read until the server closes the connection.
                if (connection_closed && buffer.empty()) {
                    return ReadInfo::SERVER_CLOSED;
                } else if (!buffer.empty()){
                    HTTPBody body{buffer};
                    buffer.clear();
                    return body;
                } else {
                    return ReadInfo::END_OF_STREAM;
                }
            }
        }
    }
};

// Function that switches socket between block and nonblock mode.
// Returns true if successful.
bool switch_socket_mode(int sockfd, bool blocking_mode);

// Singleton for holding SSL_CTX object.
class SSL_FACTORY {
    private:
        SSL_FACTORY() {
            ctx = SSL_CTX_new(TLS_method());
            if (ctx == nullptr) {
                Logger::get_instance() << LogLevel::ERRORS << 
                                "Failed to create SSL_CTX\n";
            }
        }

        ~SSL_FACTORY() {
            if (ctx != nullptr) {
                SSL_CTX_free(ctx);
                ctx = nullptr;
            }
        }
        SSL_CTX* ctx;
    public:
        static SSL_FACTORY & get_instance() {
            static SSL_FACTORY instance;
            return instance;
        }

        SSL * create_ssl() {
            if (ctx == nullptr) { return nullptr;}
            return SSL_new(ctx);
        }

        SSL_FACTORY(const SSL_FACTORY&) = delete;
        SSL_FACTORY& operator=(const SSL_FACTORY&) = delete;
};

// Helper classes for sending/receiving HTTP/HTTPS.
namespace server_lib_detail {
    class HttpConnection : public IConnection {
    public:
        HttpConnection(int fd, const ParsedUrl & url)
            : IConnection(fd, url) {}

        virtual WriteInfo send_message(const std::string& message) override {
            if (socket_error) return WriteInfo::SYSTEM_ERROR;
            if (send_buffer.empty()) {
                send_buffer = prepare_send(message);
            }
            ssize_t bytes_sent = write(sockfd, send_buffer.c_str(), 
                                                send_buffer.size());
            if (bytes_sent < 0) {
                if (errno == EWOULDBLOCK || errno == EAGAIN) {
                    Logger::get_instance() << LogLevel::DEBUG << 
                            "Write to pipe would block!\n";
                    return WriteInfo::WBLOCK_WR;
                } else {
                    return WriteInfo::SYSTEM_ERROR;
                }
            } else if (bytes_sent == 0) {
                return WriteInfo::SERVER_CLOSED;
            } else if (static_cast<size_t>(bytes_sent) < send_buffer.size()) {
                send_buffer.erase(0, bytes_sent);
                return WriteInfo::PARTIAL;
            }
            Logger::get_instance() << LogLevel::CRITICAL << send_buffer;
            send_buffer.clear();
            return WriteInfo::SUCCESS;
        }

        virtual void receive_message() override {
            auto & logger = Logger::get_instance();
            constexpr int BUF_SIZE = 16384;
            char buf[BUF_SIZE] = {0};
            ssize_t bytes_read = read(sockfd, buf, BUF_SIZE);

            if (bytes_read == 0) { // Connection closed by the server.
                logger << LogLevel::DEBUG << 
                            "Server closed the connection!\n";
                connection_closed = true;
            } 
            else if (bytes_read < 0) { // Error handling
                if (errno == EWOULDBLOCK || errno == EAGAIN) {
                    logger << LogLevel::DEBUG << 
                            "Read from pipe would block!\n";
                } else {
                    logger << LogLevel::ERRORS
                        << format_error("System error on read: ");
                    socket_error = true;
                }
            } 
            else { // Proper message
                buffer.append(buf, bytes_read);
            }
        }
    };

    class HttpsConnection : public IConnection {
    private:
        SSL * ssl;
    public:
        // In HTTPS constructor needs to establish SSL connection. Failure 
        // leaves object in damaged state, where all operations fail. 
        HttpsConnection(int fd, const ParsedUrl & url)
            : IConnection(fd, url) {
            // Get an SSL object.
            ssl = SSL_FACTORY::get_instance().create_ssl();
            if (ssl == nullptr) {
                Logger::get_instance() << LogLevel::ERRORS << 
                                "Failed to create SSL object\n";
                socket_error = true; return;
            }
            // Connent it with our socket and set SNI.
            if (SSL_set_fd(ssl, fd) == 0) {
                Logger::get_instance() << LogLevel::ERRORS << 
                                "Failed to set SSL file descriptor\n";
                socket_error = true; return;
            }
            if (SSL_set_tlsext_host_name(ssl, url.host.c_str()) == 0) {
                Logger::get_instance() << LogLevel::ERRORS << 
                    "Failed to set SNI for SSL connection - reason: \n" <<
                    ERR_error_string(ERR_get_error(), nullptr) << '\n';
                socket_error = true; return;
            }

            // Establish SSL connection, for that we need to temporarily switch
            // back socket to blocking mode, because SSL_connect blocks.
            if (switch_socket_mode(fd, true) == false) { 
                socket_error = true; return; 
            }
            if (SSL_connect(ssl) <= 0) {
                Logger::get_instance() << LogLevel::ERRORS <<
                    "Failed to establish SSL connection - reason: " <<
                    ERR_error_string(ERR_get_error(), nullptr) << '\n';
                socket_error = true; return;
            }
            if (switch_socket_mode(fd, false) == false) { 
                socket_error = true; return; 
            }
        }

        // Free memory used by SSL object and close connection.
        ~HttpsConnection() {
            if (ssl != nullptr) {
                SSL_shutdown(ssl);
                SSL_free(ssl);
                ssl = nullptr;
            }
        }

        virtual WriteInfo send_message(const std::string& message) override {
            if (socket_error) return WriteInfo::SYSTEM_ERROR;
            if (send_buffer.empty()) {
                send_buffer = prepare_send(message);
            }
            ssize_t bytes_sent = SSL_write(ssl, send_buffer.c_str(), send_buffer.size());
            if (bytes_sent <= 0) {
                int err = SSL_get_error(ssl, bytes_sent);
                if (err == SSL_ERROR_WANT_READ) {
                    return WriteInfo::WBLOCK_RD;
                } else if (err == SSL_ERROR_WANT_WRITE) {
                    return WriteInfo::WBLOCK_WR;
                } else if (bytes_sent == 0 && err == SSL_ERROR_ZERO_RETURN) {
                    return WriteInfo::SERVER_CLOSED;
                }
                return WriteInfo::SYSTEM_ERROR;
            } else if (static_cast<size_t>(bytes_sent) < send_buffer.size()) {
                send_buffer.erase(0, bytes_sent);
                return WriteInfo::PARTIAL;
            }
            Logger::get_instance() << LogLevel::CRITICAL << send_buffer;
            send_buffer.clear();
            return WriteInfo::SUCCESS;
        }

        virtual void receive_message() override {
            auto & logger = Logger::get_instance();
            constexpr int BUF_SIZE = 16384;
            char buf[BUF_SIZE] = {0};
            ssize_t bytes_read = SSL_read(ssl, buf, BUF_SIZE);
            if (bytes_read <= 0) {
                int err = SSL_get_error(ssl, bytes_read);
                if (err == SSL_ERROR_WANT_READ) {
                    logger << LogLevel::DEBUG << "SSL_read needs to read!\n";
                } else if (err == SSL_ERROR_WANT_WRITE) {
                    logger << LogLevel::DEBUG << "SSL_read needs to write!\n";
                } else if (err == SSL_ERROR_ZERO_RETURN) {
                    logger << LogLevel::DEBUG
                            << "Server closed the SSL connection!\n";
                    connection_closed = true;
                } else {
                    logger << LogLevel::ERRORS << "Error with SSL connection: "
                        << ERR_error_string(ERR_get_error(), nullptr) << '\n';
                    socket_error = true;
                }
            } else { // Proper message
                buffer.append(buf, bytes_read);
            }
        }
    };
}

using conn_error_t = std::string;
using Connection = std::unique_ptr<IConnection>;
using Resultconn = std::variant
        <Connection, conn_error_t>;

// Function to establish connection with a server:
//   - Resolves the server name and port.
//   - Creates a socket and sets timeout for receiving data.
//   - Tries to connect to any of resolved addresses.
//   - Returns either a connection object or an error message.
Resultconn establish_conn(const ParsedUrl & url, int domain);

#endif // SERVER_LIB_HPP 