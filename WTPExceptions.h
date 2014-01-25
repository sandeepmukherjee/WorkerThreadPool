
#ifndef WTP_EXCEPTIONS_H
#define WTP_EXCEPTIONS_H

#include <string>

namespace WTP {

class InternalError {
public:
    std::string getMessage() const {return msg;}
    InternalError(const std::string& errmsg, const char *file, int line);
private:
    std::string file;
    int line;
    std::string msg;
};

class CallerError {
public:
    std::string getMessage() const {return msg;}
    CallerError(const std::string& errmsg) : msg(errmsg) {}
private:
    std::string msg;
    int line;
};

}

#endif
