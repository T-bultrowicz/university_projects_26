#include "common.hpp"
#include "err.hpp"
#include "parser.hpp"

#include <atomic>
#include <chrono>
#include <csignal>

// Simple Ctrl + C handler.
static std::atomic_bool server_running = true;

void signal_handler(int signum) {
    if (signum == SIGINT) {
        server_running = false;
    }
}

// Class that holds all data server needs -> 
//  game states and timestamps needed for proper servicing of users.
// Follows singleton principle.
class GameServer {
    private:
        using gid_t = uint32_t;
        using stamp_t = std::chrono::time_point<
            std::chrono::system_clock, std::chrono::milliseconds>;
        static constexpr uint8_t WAITING_FOR_OPPONENT = 0;
        static constexpr uint8_t TURN_A = 1;
        static constexpr uint8_t TURN_B = 2;
        static constexpr uint8_t WIN_A = 3;
        static constexpr uint8_t WIN_B = 4;
        
        // Structure that holds game progress info 
        // and timestamps to track timeouts.
        struct record_t {
            GameState gst;
            stamp_t game_stamp, A_stamp, B_stamp;

            record_t(uint32_t pid, const pawn_row_t & pr):
              gst(GameState(pid, pr)),
              game_stamp(stamp_now()),
              A_stamp(stamp_now()),
              B_stamp(stamp_now() - std::chrono::seconds(1)) {}
        };
        
        std::map<gid_t, record_t> records{};
        std::optional<std::pair<gid_t, record_t>> waiting_game{};
        gid_t id_generator{};
        const pawn_row_t template_row{};
        const std::chrono::seconds timeout{};

        // Simple function that returns stamp of current moment.
        static stamp_t stamp_now() {
            using namespace std::chrono;
            return time_point_cast<milliseconds>(system_clock::now());
        }

        // Refreshes single record, returns true if it should be deleted.
        bool refresh_single(record_t & record) {
            using namespace std::chrono;
            stamp_t now = stamp_now();

            if (record.gst.status == TURN_A || record.gst.status == TURN_B) {
                if (now - record.A_stamp > timeout) {
                    if (record.A_stamp > record.B_stamp) {
                        record.gst.status = WIN_A;
                        record.game_stamp = record.B_stamp + timeout;
                    } else {
                        record.gst.status = WIN_B;
                        record.game_stamp = record.A_stamp + timeout;
                    }
                } else if (now - record.B_stamp > timeout) {
                    record.gst.status = WIN_A;
                    record.game_stamp = record.B_stamp + timeout;
                }
            }

            return (now - record.game_stamp) > timeout;
        }

        // Refreshes every record held on the server. Deletes outdated.
        void refresh_records() {
            if (waiting_game.has_value()) {
                auto & record = waiting_game->second;
                if (stamp_now() - record.A_stamp > timeout) {
                    waiting_game.reset();
                }
            }

            for (auto it = records.begin(); it != records.end();) {
                if (refresh_single(it->second))
                    it = records.erase(it);
                else
                    it = std::next(it);
            }
        }

        // Prepares UDP datagram to be sent to the client, based on game state.
        good_msg_t prepare_response(gid_t gid, const record_t & record) {
            const pawn_row_t & pr = record.gst.pawns;
            size_t actual_size = ERR_MSG_SIZE + pr.row.size();
            good_msg_t reply(actual_size, 0);

            uint32_t net_gid = htonl(gid);
            uint32_t net_A_id = htonl(record.gst.playerA_id);
            uint32_t net_B_id = htonl(record.gst.playerB_id);
            
            std::memcpy(&reply[GID_BIT_SERVER], &net_gid, sizeof(uint32_t));
            std::memcpy(&reply[PLA_ID_BIT], &net_A_id, sizeof(uint32_t));
            std::memcpy(&reply[PLB_ID_BIT], &net_B_id, sizeof(uint32_t));
            reply[STATUS_BIT] = static_cast<char>(record.gst.status);
            reply[MAX_PAWN_BIT] = static_cast<char>(pr.max_pawn);
            std::memcpy(&reply[PWN_BIT], pr.row.data(), pr.row.size());
            return reply;
        }

        // Changes board accorind to move if its legal. Assumes every
        // check on whether move can be done was already made before
        // calling the function.
        void change_board_state(GameState & gst, 
                const ClientRequest & reqst, bool is_A) {
            // Single pin move.
            if (reqst.msg_type == MsgType::MSG_MOVE_1 && 
                get_bit(gst.pawns, reqst.pawn) == true) { 

                set_bit(gst.pawns, reqst.pawn, false);
                if (empty(gst.pawns)) {
                    gst.status = is_A ? WIN_A : WIN_B;
                } else {
                    gst.status = is_A ? TURN_B : TURN_A;
                }
            } 
            // Double pin move.
            else if (reqst.msg_type == MsgType::MSG_MOVE_2 &&
                        get_bit(gst.pawns, reqst.pawn) == true &&
                        get_bit(gst.pawns, reqst.pawn + 1) == true) {

                set_bit(gst.pawns, reqst.pawn, false);
                set_bit(gst.pawns, reqst.pawn + 1, false);
                if (empty(gst.pawns)) {
                    gst.status = is_A ? WIN_A : WIN_B;
                } else {
                    gst.status = is_A ? TURN_B : TURN_A;
                }
            } 
            // Give up move.
            else if (reqst.msg_type == MsgType::MSG_GIVE_UP) {
                gst.status = is_A ? WIN_B : WIN_A;
            }
        }

    public:
        GameServer(const ServerParams & args):
            template_row(args.row), 
            timeout(std::chrono::seconds(args.timeout)) {}
        
        // Handles request and returns either valid UDP datagram to be sent to
        // client or bit that couldn't be interpreted by the server.
        std::variant<good_msg_t, error_bit_t> handle_request
            (const ClientRequest & reqst) {
            refresh_records(); // Handle timeouts.
            good_msg_t reply;

            // Handle waiting game cases for everything EXCEPT MSG_JOIN.
            // (Because it is simpler to handle it explicitly).
            if (waiting_game.has_value() && reqst.msg_type != MsgType::MSG_JOIN) {
                auto & [gid, record] = *waiting_game;
                if (reqst.game_id == gid && 
                                reqst.player_id == record.gst.playerA_id) {
                    // Only inside that if we can send response.
                    if (reqst.msg_type == MsgType::MSG_KEEP_ALIVE) {
                        // Only inside that if we can update timestamps.
                        record.A_stamp = stamp_now();
                        record.game_stamp = stamp_now();
                    }
                    return prepare_response(gid, record);
                }
            }

            if (reqst.msg_type == MsgType::MSG_JOIN) { // Handle MSG_JOIN
                if (waiting_game.has_value()) { // Start the game.
                    record_t & r = waiting_game->second;
                    r.gst.playerB_id = reqst.player_id;
                    r.B_stamp = stamp_now();
                    r.game_stamp = stamp_now();
                    r.gst.status = TURN_B;

                    reply = prepare_response(waiting_game->first, r);
                    records.insert(*waiting_game);
                    waiting_game.reset();
                } else { // Prepare waiting game.
                    waiting_game = {id_generator++, 
                        record_t(reqst.player_id, template_row)};

                    reply = prepare_response(
                        waiting_game->first, waiting_game->second);
                }
            } else { // Handle other moves on all games except the waiting one.
                auto it = records.find(reqst.game_id);
                if (it == records.end())
                    return GID_BIT_S;

                auto & [gid, record] = *it;
                // Keep in mind that player can play as both sides.
                if (record.gst.playerA_id == reqst.player_id) {
                    record.A_stamp = stamp_now();
                    record.game_stamp = stamp_now();
                    // Make sure to update both timestamps if pids are the same.
                    if (record.gst.playerB_id == reqst.player_id) {
                        record.B_stamp = stamp_now();
                    }

                    if (record.gst.status == TURN_A) {
                        change_board_state(record.gst, reqst, true);
                        // Return here, so if both pids are the same,
                        // server doesn't make same move twice as plA and plB.
                        return prepare_response(gid, record);
                    }
                }
                if (record.gst.playerB_id == reqst.player_id) {
                    record.B_stamp = stamp_now();
                    record.game_stamp = stamp_now();

                    if (record.gst.status == TURN_B)
                        change_board_state(record.gst, reqst, false);
                } 
                // Player absent in game he requested.
                else if (record.gst.playerA_id != reqst.player_id
                        && record.gst.playerB_id != reqst.player_id) {
                    return PLID_BIT_S;
                }

                reply = prepare_response(gid, record); // Send reply.
            }
            return reply;
        };
};

int main(int argc, char * argv[]) {
    // Parse server params.
    auto maybe_params = Parser::parse_server_args(argc, argv);
    if (std::holds_alternative<err_msg_t>(maybe_params)) {
        std::cerr << std::get<err_msg_t>(maybe_params) << '\n';
        return EXIT_FAILURE;
    }
    auto args = std::get<ServerParams>(maybe_params);
    GameServer server(args); // Create GameServer object.

    // Create proper UDP socket to listen for clients.
    int socket_fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (socket_fd < 0)
        syserr("Socket:");

    struct sockaddr_in server_address = 
        get_server_address(args.host.c_str(), args.port);
    if (bind(socket_fd, (struct sockaddr *) &server_address, 
                        (socklen_t) sizeof server_address) < 0) {
        syserr("Bind:");
    }

    // Install handling of SIGINT.
    install_signal_handler(SIGINT, signal_handler, 0);

    std::vector<char> buffer(BUFFER_SIZE, 0);
    struct sockaddr_in client_adress;
    socklen_t client_len = sizeof(client_adress);
    bool send_correct_msg;

    // Main server loop.
    while (server_running) {
        memset(buffer.data(), 0, buffer.size());
        send_correct_msg = true;

        // Receive a client
        ssize_t received = recvfrom(socket_fd, buffer.data(), buffer.size(), 0,
                        (struct sockaddr *) &client_adress, &client_len);
        if (received < 0) { // Handle errors - EINTR means SIGINT pressed.
            if (errno != EINTR) {
                close(socket_fd); syserr("Recvfrom:");
            }
        } else {
            size_t u_recv = static_cast<size_t>(received);
            // Sketches of replies to client.
            std::string msg_to_send, err_msg(ERR_MSG_SIZE, 0);
            std::memcpy(err_msg.data(), buffer.data(), std::min(u_recv, 12UL));
            err_msg[STATUS_BIT] = -1;

            // Check correctness of request and process it.
            auto maybe_request = Parser::parse_cl_request(buffer, u_recv);
            if (std::holds_alternative<error_bit_t>(maybe_request)) {
                err_msg[ERR_BIT] = std::get<error_bit_t>(maybe_request);
                send_correct_msg = false;
            } else {
                auto request = std::get<ClientRequest>(maybe_request);
                auto maybe_reply = server.handle_request(request);

                if (std::holds_alternative<error_bit_t>(maybe_reply)) {
                    err_msg[ERR_BIT] = std::get<error_bit_t>(maybe_reply);
                    send_correct_msg = false;
                } else {
                    msg_to_send = std::move(std::get<good_msg_t>(maybe_reply));
                }
            }
            if (send_correct_msg == false)
                msg_to_send = std::move(err_msg);
            
            // Send response to the client, handle errors.
            ssize_t bytes_sent = sendto(
                socket_fd, msg_to_send.data(), msg_to_send.size(), 0,
                (struct sockaddr *) &client_adress, client_len);
            if (bytes_sent < 0) {
                if (errno != EINTR) {
                    close(socket_fd); syserr("Sendto:");
                }
            } else if (static_cast<size_t>(bytes_sent) != msg_to_send.size()) {
                std::cerr << "Failed to send full message to client!\n";
            }
        }
    }

    // Close socket and return.
    std::cout << "SIGINT received, shutting down server.\n";
    close(socket_fd);
    return EXIT_SUCCESS;
}