
#include "SharedObject.h"
#include "WTPExceptions.h"
#include <string>

using namespace WTP;

WTP::SharedObject::SharedObject()
{
    int rc = pthread_mutex_init(&_mutex, NULL);
    if (rc != 0)
        throw InternalError("Failed to initialize mutex", __FILE__, __LINE__);
}
    
void WTP::SharedObject::lock()
{
    int rc = pthread_mutex_lock(&_mutex);
    if (rc != 0)
        throw InternalError("Failed to lock mutex", __FILE__, __LINE__);
}

void WTP::SharedObject::unlock()
{
    int rc = pthread_mutex_unlock(&_mutex);
    if (rc != 0)
        throw InternalError("Failed to unlock mutex", __FILE__, __LINE__);
}

