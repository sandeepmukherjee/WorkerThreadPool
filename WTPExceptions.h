
#ifndef WTP_EXCEPTIONS_H
#define WTP_EXCEPTIONS_H

#include <string>

namespace WTP {

/**
 * @brief WTP Internal error.
 *
 * This exception is thrown when WTP encounters an internal error or a 
 * an underlying error. The former is generally a WTP bug. The latter can
 * be caused by instability in the system.
 * In either case once this exception is thrown, the WTP is in an undefined
 * state. The recommended action is to assert and terminate the program
 * immediately.
 */
class InternalError {
public:
    /**
     * Retrieves the message associated wit this error.
     */
    std::string getMessage() const {return msg;}
    /**
     * Default constructor.
     * @param[in] errmsg String representing this error.
     * @param[in] file Filename where this error occured typically __FILE__.
     * @param[in] line Line number where error occured, typically __LINE__.
     */
    InternalError(const std::string& errmsg, const char *file, int line);
private:
    std::string file;
    int line;
    std::string msg;
};

/**
 * @brief WTP Caller error.
 *
 * This exception is thrown when WTP functions are invoked in an incorrect
 * manner. This is almost always a bug in the software using WTP, and not
 * in WTP itself.
 * Once this exception is thrown, the WTP is in an undefined state.
 * The recommended action is to assert to terminate the program
 * immediately.
 */
class CallerError {
public:
    /**
     * Retrieves the message associated wit this error.
     */
    std::string getMessage() const {return msg;}
    /**
     * Default constructor.
     * @param[in] errmsg String representing this error.
     */
    CallerError(const std::string& errmsg);
private:
    std::string msg;
};

}

#endif
