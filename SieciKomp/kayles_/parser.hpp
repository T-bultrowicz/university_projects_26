#ifndef PARSER_HPP
#define PARSER_HPP

#include "lib.hpp"

namespace parser_detail {
    // Values of parametres as described in the task description.
    static constexpr size_t MAX_PAWN_ROW_IDX = 255;
    static constexpr uint8_t MAX_MSG_TYPE = 4;
    static constexpr size_t MAX_ARGS = 4;
    static constexpr size_t LITTLE_ARGS = 2;
    static constexpr size_t MED_ARGS = 3;

    // Helper to split message argument.
    using str_pair = std::pair<std::string, std::string>;
    str_pair split_by_delimiter(const std::string& str, std::string delimiter) {
        size_t len = delimiter.length();
        size_t pos = str.find(delimiter);
        if (pos == std::string::npos) {
            return {str, ""};
        }
        return {str.substr(0, pos), str.substr(pos + len)};
    }

    // Helper that safely reads unsigned values up to 32 bits.
    template <typename T>
    std::optional<T> read_unumber (char const * str) {
        char *endptr;
        errno = 0;
        unsigned long p = strtoul(str, &endptr, 10);
        if (errno != 0 || *endptr != 0 || p > std::numeric_limits<T>::max())
            return std::nullopt;
        if (str[0] == '-' || str[0] == '+')
            return std::nullopt;

        return static_cast<T>(p);
    }

    size_t calc_size(MsgType msg_type) {
        switch (msg_type) {
            case MsgType::MSG_JOIN:
                return LITTLE_MSG_SIZE;
            case MsgType::MSG_MOVE_1: case MsgType::MSG_MOVE_2:
                return MAX_REQUEST_MSG_SIZE;
            case MsgType::MSG_KEEP_ALIVE: case MsgType::MSG_GIVE_UP:
                return MEDIUM_MSG_SIZE;
            default:
                return 0;
        }
    }

    // Helper that converts message into UDP datagram.
    std::optional<std::vector<char>> parse_message(char * str) {
        std::string message{str};
        std::vector<uint32_t> args;

        // Gather all args given. Check if user gave falid numbers.
        while (message.empty() == false) {
            auto [num_str, rest] = split_by_delimiter(message, "/");
            auto num_opt = read_unumber<uint32_t>(num_str.c_str());
            if (num_opt.has_value() == false)
                return std::nullopt;
            
            args.push_back(*num_opt);
            message = std::move(rest);
        }
        // Basic checks.
        if (args.size() == 0 || args.size() > MAX_ARGS || args[0] > MAX_MSG_TYPE)
            return std::nullopt;

        // Check if amount of arguments is correct.
        MsgType msg_type = static_cast<MsgType>(args[0]);
        switch (msg_type) {
            case MsgType::MSG_JOIN:
                if (args.size() != LITTLE_ARGS)
                    return std::nullopt;
                break;
            case MsgType::MSG_MOVE_1: case MsgType::MSG_MOVE_2:
                if (args.size() != MAX_ARGS)
                    return std::nullopt;
                if (args[3] > UINT8_MAX) // Overflow check
                    return std::nullopt;
                break;
            case MsgType::MSG_KEEP_ALIVE: case MsgType::MSG_GIVE_UP:
                if (args.size() != MED_ARGS)
                    return std::nullopt;
                break;
            default:
                return std::nullopt;
        }

        std::vector<char> buff(calc_size(msg_type), 0);
        buff[MSG_BIT] = static_cast<char>(args[0]);
        // Construct UDP datagram. Fall-trough cases inside switch are used(!),
        // as longer message types contain all data of shorter ones.
        switch (msg_type) {
            case MsgType::MSG_MOVE_1: case MsgType::MSG_MOVE_2:
                buff[PAWN_BIT] = static_cast<char>(args[3]);
            case MsgType::MSG_KEEP_ALIVE: case MsgType::MSG_GIVE_UP:
                args[2] = htonl(args[2]);
                std::memcpy(&buff[GID_BIT], &args[2], sizeof(uint32_t));
            default:
                if (args[1] == 0) // check if player id is non-zero
                    return std::nullopt;
                
                args[1] = htonl(args[1]);
                std::memcpy(&buff[PLID_BIT], &args[1], sizeof(uint32_t));
                break;
        }
        return buff;
    }

    // Helper that converts pawn row string into pawn_row_t struct.
    std::optional<pawn_row_t> parse_pawn_row(char * str) {
        if (str == nullptr || str[0] == '\0')
            return std::nullopt;

        // Build pawn_row.
        pawn_row_t pawn_row;    
        size_t i = 0;
        for (; str[i] != '\0'; ++i) { 
            if (str[i] != '0' && str[i] != '1')
                return std::nullopt;

            if (i % 8 == 0)
                pawn_row.row.push_back(0);
            set_bit(pawn_row, i, (str[i] == '1'));
        }
        // Check length and that side characters are equal 1.
        if (str[0] == '0' || str[i - 1] == '0' || i > MAX_PAWN_ROW_IDX + 1)
            return std::nullopt;

        pawn_row.max_pawn = i - 1;
        return pawn_row;
    }
}

class Parser {
    private:
        static constexpr uint8_t MAX_TIMEOUT = 99;
    public:

        // Returns either parsed client parametres or error message on what
        // was typed in wrong by the user.
        static std::variant<ClientParams, err_msg_t> parse_client_args
                                                (int argc, char * argv[]) {
            using namespace parser_detail;
            int opt;
            ClientParams params{};
            bool flags[4]{};

            // Use getopt to handle options in linux style.
            while ((opt = getopt(argc, argv, ":a:p:m:t:")) != -1) {
                switch (opt) {
                    case 'a': {
                        flags[0] = true;
                        params.host = optarg;
                        break;
                    }
                    case 'p': {
                        flags[1] = true;
                        auto port_opt = read_unumber<uint16_t>(optarg);
                        if (port_opt.has_value() == false)
                            return std::string{"Invalid port: "} + optarg;

                        params.port = *port_opt;
                        if (params.port == 0)
                            return std::string{"Port must be non-zero!"};
                        break;
                    }
                    case 'm': {
                        flags[2] = true;
                        auto message_opt = parse_message(optarg);
                        if (message_opt.has_value() == false)
                            return std::string{"Invalid message: "} + optarg;
                        
                        params.request = *message_opt;
                        break;
                    }
                    case 't': {
                        auto timeout_opt = read_unumber<uint16_t>(optarg);
                        if (timeout_opt.has_value() == false)
                            return std::string{"Invalid timeout: "} + optarg;

                        params.timeout = *timeout_opt;
                        if (params.timeout == 0 || params.timeout > MAX_TIMEOUT)
                            return std::string{"Timeout must be between 1 and "
                            + std::to_string(static_cast<int>(MAX_TIMEOUT)) + "!\n "};
                        
                        flags[3] = true;
                        break;
                    }
                    case '?': {
                        std::string result{"Invalid option: "};
                        return result.append(std::string{static_cast<char>(optopt)});
                    }
                    default: {
                        std::string result{"Lacking argument for: "};
                        return result.append(std::string{static_cast<char>(optopt)});
                    }
                }
            }

            std::string names[4]{"-a: address", "-p: port", "-m: message", "-t: timeout"};
            for (int i = 0; i < 4; ++i) { // Assure all parametres were provided.
                if (flags[i] == false)
                    return std::string{"Missing required argument: "} + names[i];
            }
            return params;
        }

        // Returns either parsed server parametres or error message on what
        // was typed in wrong by the user.
        static std::variant<ServerParams, err_msg_t> parse_server_args
                                                (int argc, char * argv[]) {
            using namespace parser_detail;
            int opt;
            ServerParams params{};
            bool flags[4]{};

            // Use getopt to handle options in linux style.
            while ((opt = getopt(argc, argv, ":a:p:r:t:")) != -1) {
                switch(opt) {
                    case 'a': {
                        flags[0] = true;
                        params.host = optarg;
                        break;
                    }
                    case 'p': {
                        flags[1] = true;
                        auto port_opt = read_unumber<uint16_t>(optarg);
                        if (port_opt.has_value() == false)
                            return std::string{"Invalid port: "} + optarg;

                        params.port = *port_opt;
                        break;
                    }
                    case 'r': {
                        flags[2] = true;
                        auto pawn_row_opt = parse_pawn_row(optarg);
                        if (pawn_row_opt.has_value() == false)
                            return std::string{"Invalid pawn row: "} + optarg;

                        params.row = *pawn_row_opt;
                        break;
                    }
                    case 't': {
                        flags[3] = true;
                        auto timeout_opt = read_unumber<uint32_t>(optarg);
                        if (timeout_opt.has_value() == false)
                            return std::string{"Invalid timeout: "} + optarg;

                        params.timeout = *timeout_opt;
                        if (params.timeout == 0 || params.timeout > MAX_TIMEOUT)
                            return std::string{"Timeout must be between 1 and "
                            + std::to_string(static_cast<int>(MAX_TIMEOUT)) + "!\n "};
                        
                        break;
                    }
                    case '?': {
                        std::string result{"Invalid option: "};
                        return result.append(std::string{static_cast<char>(optopt)});
                    }
                    default: {
                        std::string result{"Lacking argument for: "};
                        return result.append(std::string{static_cast<char>(optopt)});
                    }
                }
            }

            std::string names[4]{"-a: address", "-p: port", "-r: pawn row", "-t: timeout"};
            for (int i = 0; i < 4; ++i) { // Assure all parametres were provided.
                if (flags[i] == false)
                    return std::string{"Missing required argument: "} + names[i];
            }
            return params;
        };

        // Returns either decoded from datagram client request or bit that
        // couldn't be interpreted by the server.
        // Performs only (!) basic checks, without looking into records.
        static std::variant<ClientRequest, error_bit_t> parse_cl_request
            (const std::vector<char> & buff, size_t recv) {
            if (recv == 0)
                return MSG_BIT_S;

            ClientRequest res; bool check_size = true;
            res.msg_type = static_cast<MsgType>(buff[MSG_BIT]);

            switch(res.msg_type) {
                case MsgType::MSG_MOVE_1: case MsgType::MSG_MOVE_2:
                    if (recv != MAX_REQUEST_MSG_SIZE)
                        return static_cast<error_bit_t>
                            (std::min(MAX_REQUEST_MSG_SIZE, recv));
                    
                    check_size = false;
                    res.pawn = static_cast<uint8_t>(buff[PAWN_BIT]);

                case MsgType::MSG_KEEP_ALIVE: case MsgType::MSG_GIVE_UP:
                    if (check_size && recv != MEDIUM_MSG_SIZE)
                        return static_cast<error_bit_t>
                            (std::min(MEDIUM_MSG_SIZE, recv));

                    check_size = false;
                    std::memcpy(&res.game_id, &buff[GID_BIT], sizeof(uint32_t));
                    res.game_id = ntohl(res.game_id);

                case MsgType::MSG_JOIN:
                    if (check_size && recv != LITTLE_MSG_SIZE)
                        return static_cast<error_bit_t>
                            (std::min(LITTLE_MSG_SIZE, recv));     
                    
                    check_size = false;
                    std::memcpy(&res.player_id, &buff[PLID_BIT], sizeof(uint32_t));
                    res.player_id = ntohl(res.player_id);
                    if (res.player_id == 0)
                        return PLID_BIT_S;
                    break;
                default:
                    return MSG_BIT_S;
            }

            return res;
        }

};

#endif // PARSER_HPP