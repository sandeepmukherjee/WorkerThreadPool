
#include "WTPExceptions.h"
#include <sstream>

WTP::InternalError::InternalError(const std::string& message, const char *file, int line)
{
    std::ostringstream buf;
    buf << "WTP INTERR:" << std::string(file) << ":" << line << ":" << message;
    msg = buf.str();
}
