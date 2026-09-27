#include <cerrno>
#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "err.hpp"

std::string format_error(const char* fmt, ...) {
    int org_errno = errno;
    char buffer[1024];
    
    va_list fmt_args;
    va_start(fmt_args, fmt);
    int writ = vsnprintf(buffer, sizeof(buffer), fmt, fmt_args);
    va_end(fmt_args);

    std::string result = (writ < 0) ? "Formatting error" : std::string(buffer);
    if (org_errno != 0) {
        result += " (" + std::to_string(org_errno) + "; " + 
                            std::strerror(org_errno) + ")";
    }

    return result;
}
