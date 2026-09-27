#ifndef LIB_HPP
#define LIB_HPP

#include <arpa/inet.h>
#include <cinttypes>
#include <cstring>
#include <iostream>
#include <limits>
#include <map>
#include <netdb.h>
#include <netinet/in.h>
#include <optional>
#include <string>
#include <variant>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <vector>

// Types of messages.
enum class MsgType : uint8_t {
    MSG_JOIN = 0,
    MSG_MOVE_1 = 1,
    MSG_MOVE_2 = 2,
    MSG_KEEP_ALIVE = 3,
    MSG_GIVE_UP = 4
};

// Sizes of server to client messages.
static constexpr size_t BUFFER_SIZE = 1024;
static constexpr size_t MAX_REPLY_SIZE = 46;
static constexpr size_t ERR_MSG_SIZE = 14;

// Sizes of client to server messages.
static constexpr size_t MAX_REQUEST_MSG_SIZE = 10;
static constexpr size_t LITTLE_MSG_SIZE = 5;
static constexpr size_t MEDIUM_MSG_SIZE = 9;

// Offsets of bits and their meanings.

// Client to server.
static constexpr size_t MSG_BIT = 0;
static constexpr size_t PLID_BIT = 1;
static constexpr size_t GID_BIT = 5;
static constexpr size_t PAWN_BIT = 9;
static constexpr uint8_t MSG_BIT_S = 0;
static constexpr uint8_t PLID_BIT_S = 1;
static constexpr uint8_t GID_BIT_S = 5;
static constexpr uint8_t PAWN_BIT_S = 9;

// Server to client.
static constexpr size_t GID_BIT_SERVER = 0;
static constexpr size_t PLA_ID_BIT = 4;
static constexpr size_t PLB_ID_BIT = 8;
static constexpr size_t STATUS_BIT = 12;
static constexpr size_t MAX_PAWN_BIT = 13;
static constexpr size_t ERR_BIT = 13;
static constexpr size_t PWN_BIT = 14;

// Simple struct representing pawn row.
struct pawn_row_t {
    std::vector<char> row;
    uint8_t max_pawn;
};

// Functions to work with pawn row.

// Sets [pos] pin to value [value], does nothing if pos is out of bounds.
void set_bit(pawn_row_t & v, size_t pos, bool value);
// Gets value of [pos] pin. Returns false if pos is out of bounds.
bool get_bit(const pawn_row_t & v, size_t pos);
// Returns true if and only if all pawns in pawn row are removed.
bool empty(const pawn_row_t & v);

// Struct holding state of the game.
struct GameState {
    uint32_t playerA_id{}, playerB_id{};
    uint8_t status{};
    pawn_row_t pawns{};

    GameState() = default;
    GameState(uint32_t pid, const pawn_row_t & pr):
      playerA_id(pid), pawns(pr) {}
};

// Structs holding entry parameters for client and server.
struct ClientParams {
    std::string host{};
    std::vector<char> request{};
    uint16_t port{};
    uint8_t timeout{};
};

struct ServerParams {
    pawn_row_t row{};
    std::string host{};
    uint16_t port{};
    uint8_t timeout{};
};

// Struct representing client request decoded from datagram.
struct ClientRequest {
    uint32_t game_id, player_id;
    MsgType msg_type;
    uint8_t pawn;
};

// Helper types
using err_msg_t = std::string; // Holds info about invalid parametres.
using good_msg_t = std::string; // Holds valid server to cl message.
// Holds info about byte that couldn't be interpreted.
using error_bit_t = uint8_t;

// Helper for client to print GameState on the screen.
std::string prettify(const GameState & gst, uint32_t gid);

#endif // LIB_HPP 