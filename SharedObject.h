
#ifndef WTP_SHARED_OBJECT_H
#define WTP_SHARED_OBJECT_H

#include <pthread.h>

namespace WTP {

/**
 * @brief A class for managing consistency groups.
 *
 * A SharedObject represents a consistency group - a collection of objects
 * that must all be protected by one lock.
 * This class is meant to be subclassed.
 */
class SharedObject {
public:
    /** Default constructor */
    SharedObject();
    
    /** Lock all data structures */
    void lock();

    /** Unlock all data structures */
    void unlock();

    virtual ~SharedObject() {}

private:
    pthread_mutex_t _mutex; /**< The mutex that locks everything in this obj */
    SharedObject operator=(SharedObject& rhs);
    SharedObject(SharedObject& rhs);

};

}

#endif
