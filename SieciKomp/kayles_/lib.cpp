#include "lib.hpp"

void set_bit(pawn_row_t & v, size_t pos, bool value) {
    size_t cell = pos / 8, offset = 7 - (pos % 8);
    if (cell >= v.row.size())
        return;
    
    if (value)
        v.row[cell] |= (1 << offset);
    else
        v.row[cell] &= ~(1 << offset);
}

bool get_bit(const pawn_row_t & v, size_t pos) {
    size_t cell = pos / 8, offset = 7 - (pos % 8);
    if (cell >= v.row.size()) return false;

    return (v.row[cell] & (1 << offset)) != 0; 
}

bool empty(const pawn_row_t & v) {
    for (size_t i = 0; i <= v.max_pawn; ++i) {
        if (get_bit(v, i))
            return false;
    }
    return true;
}

std::string prettify(uint8_t status) {
    switch(status) {
        case 0:
            return "WAITING FOR OPPONENT";
        case 1:
            return "TURN_A";
        case 2:
            return "TURN_B";
        case 3:
            return "WIN_A";
        case 4:
            return "WIN_B";
        default:
            return "UNKNOWN";
    }
    return "UNKNOWN";
}

std::string prettify(const pawn_row_t & row) {
    std::string res{};
    for (size_t i = 0; i <= row.max_pawn; ++i) {
        if (get_bit(row, i) == true) {
            res.push_back('I');
        } else {
            res.push_back('.');
        }
    }
    return res;
}

std::string prettify(const GameState & gst, uint32_t gid) {
    std::string res{"====== KAYLES GAME ======\n"};
    res.append("GAME ID: ").append(std::to_string(gid));
    res.append(" PLAYER A: ").append(std::to_string(gst.playerA_id));
    res.append(" PLAYER B: ").append(std::to_string(gst.playerB_id));
    res.append("\nSTATUS: ");
    res.append(prettify(gst.status)).append(" MAX PAWN: ");
    res.append(std::to_string(gst.pawns.max_pawn)).append("\nBOARD: ");
    res.append(prettify(gst.pawns));
    return res;
}