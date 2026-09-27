#include "common.hpp"
#include "err.hpp"
#include "parser.hpp"

// Used to set timeout of hanging on a socket.
int set_socket_timeout(int socket_fd, uint16_t timeout) {
    struct timeval tv;
    tv.tv_sec = timeout; tv.tv_usec = 0;
    return setsockopt(socket_fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
}

// Santas little helpers.
bool is_bad_status(char status) {
    return (status < 0) || (status > 4);
}

bool is_bad_size(size_t actual, size_t expected) {
    return actual != (expected + ERR_MSG_SIZE);
}

// Function that holds all the logic of handling server response.
// Prints to stdout or stderr (!) Returns exit code for the client program.
int handle_server_response(int socket_fd, uint16_t timeout) {
    std::vector<char> buffer(BUFFER_SIZE, '\0');
    ssize_t recv_res = recv(socket_fd, buffer.data(), buffer.size(), 0);
    size_t u_recv = static_cast<size_t>(recv_res);

    if (recv_res < 0) { // Handling recv errors
        if (errno == EWOULDBLOCK || errno == EAGAIN || errno == ECONNREFUSED) {
            std::cout << "Timeout of " << timeout << " seconds expired while "
                << "waiting for response!\n";
        } else {
            error("Recv: "); 
            return EXIT_FAILURE;
        }
    } else if (u_recv == ERR_MSG_SIZE) { // Decoding possible error message.
        if (buffer[STATUS_BIT] != (char) -1) {
            std::cout << "Received invalid message from the server!\n";
            return EXIT_SUCCESS;
        }

        std::cout << "Server couldn't interpret " 
            << static_cast<int>(buffer[ERR_BIT]) << ". byte of your message.\n";
        std::cout << "Below up to first twelve bytes of your message"
            << " are presented (byte decoded).\n";
        for (size_t i = 0; i < STATUS_BIT; ++i) {
            std::cout << static_cast<int>(buffer[i]) << ' ';
        }
    } 
    // Decoding possible message with game info from the server.
    else if (u_recv > ERR_MSG_SIZE && u_recv < MAX_REPLY_SIZE) {
        size_t expected_row_size = (((uint8_t) buffer[MAX_PAWN_BIT]) / 8) + 1;
        size_t u32_size = sizeof(uint32_t);
        GameState gst; uint32_t gid;
        gst.pawns.row.resize(expected_row_size);

        if (is_bad_status(buffer[STATUS_BIT]) ||
                            is_bad_size(u_recv, expected_row_size)) {
            std::cout << "Received invalid message from the server!\n";
            return EXIT_SUCCESS;
        }
        // We safely extract data from buffor and switch endianess.
        std::memcpy(&gid, &buffer[GID_BIT_SERVER], u32_size);
        std::memcpy(&gst.playerA_id, &buffer[PLA_ID_BIT], u32_size);
        std::memcpy(&gst.playerB_id, &buffer[PLB_ID_BIT], u32_size);
        std::memcpy(&gst.status, &buffer[STATUS_BIT], sizeof(char));
        gst.pawns.max_pawn = static_cast<uint8_t>(buffer[MAX_PAWN_BIT]);
        std::memcpy(gst.pawns.row.data(), &buffer[PWN_BIT], expected_row_size);

        gid = ntohl(gid);
        gst.playerA_id = ntohl(gst.playerA_id);
        gst.playerB_id = ntohl(gst.playerB_id);

        if (gst.playerA_id == 0 || (gst.playerB_id == 0 && gst.status != 0)) {
            std::cout << "Received invalid message from the server!\n";
            return EXIT_SUCCESS;
        }
        // Pretty print game played by the player into stdout.
        std::cout << prettify(gst, gid);
    } else {
        std::cout << "Received invalid message from the server!\n";
    }

    return EXIT_SUCCESS;
}

int main(int argc, char * argv[]) {
    // Parse client args.
    auto maybe_params = Parser::parse_client_args(argc, argv);
    if (std::holds_alternative<err_msg_t>(maybe_params)) {
        std::cerr << std::get<err_msg_t>(maybe_params) << '\n';
        return EXIT_FAILURE;
    }
    auto args = std::get<ClientParams>(maybe_params);

    // Resolve address, build a connection.
    struct sockaddr_in server_address = 
        get_server_address(args.host.c_str(), args.port);
    socklen_t len_serv_addr = sizeof(server_address);

    int socket_fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (socket_fd < 0)
        syserr("Socket:");

    // Putting timeout on a socket, equal to one provided by user.
    if (set_socket_timeout(socket_fd, args.timeout) < 0) {
        close(socket_fd);
        syserr("Setsockopt:");
    }

    if (connect(socket_fd, (struct sockaddr *) &server_address, len_serv_addr) < 0) {
        close(socket_fd);
        syserr("Connect:");
    }

    // Send request to the server and wait for response.
    ssize_t sent = send(socket_fd, args.request.data(), args.request.size(), 0);
    if ((sent < 0) || (static_cast<size_t>(sent) != args.request.size())) {
        close(socket_fd);
        fatal("Failed to send the message to the server!");
    }
    
    return handle_server_response(socket_fd, args.timeout);
}