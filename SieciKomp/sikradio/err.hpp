#ifndef MIM_ERR_H
#define MIM_ERR_H

#include <string>

// My implementation based on the one from the course.
// Returns formatted error, containing explained errno.
std::string format_error(const char* fmt, ...);

#endif // MIM_ERR_H

