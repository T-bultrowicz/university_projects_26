#include "parser.hpp"

namespace parser_detail {
    str_pair split_by_delimiter(const std::string& str, std::string delimiter) {
        size_t len = delimiter.length();
        size_t pos = str.find(delimiter);
        if (pos == std::string::npos) {
            return {str, ""};
        }
        return {str.substr(0, pos), str.substr(pos + len)};
    }

    void trim(std::string & str) {
        size_t first = str.find_first_not_of(" \n\r\t");
        if (first == std::string::npos) {
            str.clear();
            return;
        }
        size_t last = str.find_last_not_of(" \n\r\t");
        str.erase(last + 1);
        str.erase(0, first);
    }
}