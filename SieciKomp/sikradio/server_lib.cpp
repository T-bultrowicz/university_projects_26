#include "server_lib.hpp"

namespace {
    // Prints IP address from sockaddr in a human-readable format.
    std::string get_ip_str(const struct sockaddr* sa) {
        char addr[INET6_ADDRSTRLEN];
        void* ptr;

        if (sa->sa_family == AF_INET) {
            ptr = &((struct sockaddr_in*)sa)->sin_addr;
        } else {
            ptr = &((struct sockaddr_in6*)sa)->sin6_addr;
        }

        inet_ntop(sa->sa_family, ptr, addr, sizeof(addr));
        return std::string(addr);
    }

    // Returns connection type corresponding to scheme in url.
    Connection create_connection(int fd, const ParsedUrl & url) {
        using namespace server_lib_detail;
        if (url.scheme_http) {
            return std::make_unique<HttpConnection>(fd, url);
        } else {
            return std::make_unique<HttpsConnection>(fd, url);
        }
    }
}

Resultconn establish_conn(const ParsedUrl & url, int domain) {
    Logger& logger = Logger::get_instance();
    logger << LogLevel::CRITICAL << "Resolving name: ";
    if (url.is_bracketed) {
        logger << '[' << url.host << ']' << '\n';
    } else {
        logger << url.host << '\n';
    }

    const char * host = url.host.c_str();
    std::string port_str = std::to_string(url.port);
    struct addrinfo hints{}, *res;
    hints.ai_family = domain;        // IPv4, IPv6 or both
    hints.ai_socktype = SOCK_STREAM; // TCP
    hints.ai_protocol = IPPROTO_TCP; // TCP

    if (int err = getaddrinfo(host, port_str.c_str(), &hints, &res); err != 0) {
        return std::string("getaddrinfo error: ") + gai_strerror(err);
    }

    // Try to connect with resolved addresses.
    for (auto * p = res; p != nullptr; p = p->ai_next) {
        int sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
        if (sockfd < 0) {
            logger << LogLevel::WARNINGS << 
                std::format("Creating socket for {} failed ",
                        p->ai_family == AF_INET ? "IPv4" : "IPv6") << '\n';
            continue;
        }

        logger << LogLevel::CRITICAL << 
            std::format("Connecting to server: {}:{}\n",
            get_ip_str(p->ai_addr), url.port);
        
        if (connect(sockfd, p->ai_addr, p->ai_addrlen) == 0) {
            if (fcntl(sockfd, F_SETFL, O_NONBLOCK) < 0) {
                logger << LogLevel::WARNINGS << 
                    std::format("Setting socket to non-blocking for {} failed ",
                            p->ai_family == AF_INET ? "IPv4" : "IPv6") << '\n';
                close(sockfd);
                continue;
            }
            std::string canonname(p->ai_canonname ? p->ai_canonname : host);
            freeaddrinfo(res);
            return create_connection(sockfd, url);
        } else {
            logger << LogLevel::CRITICAL << format_error(
                std::format("Connecting to address {} failed ",
                        get_ip_str(p->ai_addr)).c_str()) << '\n';
            close(sockfd);
        }
    }

    freeaddrinfo(res);
    return "Failed to connect to any resolved address... ";
}

bool switch_socket_mode(int sockfd, bool blocking_mode) {
    int flag = blocking_mode ? 0 : O_NONBLOCK;
    std::string mode = blocking_mode ? "blocking" : "non-blocking";

    if (fcntl(sockfd, F_SETFL, flag) < 0) {
        Logger::get_instance() << LogLevel::WARNINGS << 
            std::format("Setting socket to {} for {} failed ",
                    mode, sockfd) << '\n';
        return false;
    }
    return true;
}