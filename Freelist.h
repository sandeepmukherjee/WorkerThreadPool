#ifndef WTP_FREELIST_H
#define WTP_FREELIST_H

#include "WorkItem.h"
#include "SharedObject.h"

#include <list>

namespace WTP {

/**
 * @brief A container for storing preallocated WorkItems.
 *
 * A Freelist allows the caller to store pre-allocated WorkItem instances.
 * This can be used when repeated object instantiation is slow or leads to
 * memory fragmentation.
 * Caution: If you use WorkItems from a freelist, the freelist must be
 * destroyed last - after the WorkerThreadPool has been shut down.
 */
class Freelist : public SharedObject
{
public:
    /** Default constructor */
    Freelist() {}
    /**
     * Adds a WorkItem to the freelist.
     * This calls the WorkItem.reset() method.
     * @param[in] wi Derived instance of WorkItem to be added to freelist.
     */
    void addItem(WorkItem *wi);
    /**
     * Fetches a WorkItem from the freelist.
     * Note, do not make any assumptions on ordering - items can be returned
     * in a different order than added.
     * @return The WorkItem or NULL if the list is empty.
     */
    WorkItem * getItem();
    /**
     * Virtual destructor. This will free up all WorkItems in the list.
     */
    virtual ~Freelist() { drain(); }

private:
    std::list<WorkItem *> _wilist; // The actual list
    Freelist& operator=(Freelist& rhs); // Disallow assignment
    Freelist(Freelist& rhs); // Disallow copying

    /** Free up all WorkItem instances in the list */
    void drain();
};

}

#endif
