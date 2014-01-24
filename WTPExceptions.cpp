
#include "WTPExceptions.h"
#include <sstream>

WTP::InternalError::InternalError(const std::string& message, const char *file, int line)
{
	std::ostringstream buf;
	buf << std::string(file) << ":" << line;
	msg = buf.str();
}
