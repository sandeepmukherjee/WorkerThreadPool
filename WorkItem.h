
#ifndef WTP_WORKITEM_H
#define WTP_WORKITEM_H

#include <stdlib.h>
#include <assert.h>
#include <stdint.h>
#include <string>


namespace WTP {

class Freelist;
class WorkerThreadPool;

/**
 * @brief Base class for user-defined WorkItems.
 *
 * This is an abstract class that users must subclass.
 * Users need to define, at minimum, the Run() function in the derived class.
 * Dereived instances are added to a WTP queue using the
 * WorkerThreadPool::addWorkItem() function.
 * When a worker thread dequeues the object, it invokes the object's Run()
 * method.
 */
class WorkItem {
public:
    /**
     * Valid states of a WorkItem
     */
    enum  State {
        INVALID, /**< The WorkItem has been freed. For debugging purposes */
        QUEUED, /**< In a WTP queue, and waiting for a worker thread */
        RUNNING, /**< Currently executing */
        IDLE, /**< The default state of a newly allocated WorkItem */
        FREELIST, /**< In a freelist */
    };
    WorkItem () : _state (IDLE), _freelist(NULL),_subitem(false),
             _threadPool(NULL),_refcount(0),_parent(NULL), _gen(0) {}
    /**
     * @brief Implementors must override this function. 
     *
     * Users need to implement this function in a subclass.
     * All processing logic must be in this function (or invoked from this
     * function)
     * IMPORTANT: Do not throw any exception from this function.
     * The implementation should not block for very long periods of time.
     */
    virtual void run() = 0;
    
    /**
     * Callers need to define this function if using freelists.
     * This function should clear out all state information, free up any
     * allocated data, and make this WorkItem ready for reuse.
     * The derived function should call the base class implementation in
     * addition to resetting all data.
     * The implementation should not block for very long periods of time.
     */
    virtual void reset() {_gen=0; _parent=NULL;}

    /**
     * @brief User-defined callback, gets invoked when sub-items complete.
     *
     * This function gets invoked by the threadpool when all submitems
     * complete. The function is guaranteed to be invoked after all submitems
     * complete, but may or may not be invoked before the queue starts moving
      * again.
     * This means that implementations must not rely on the queue to be 
     * paused when this function is called.
     */
    virtual void subitemsComplete() {}
    /**
     * @brief returns a string representation of this object.
     *
     * Caution: The first invocation to this function may be slow.
     */
    virtual std::string toString()  {return std::string("TBD");}
    virtual ~WorkItem() {}

    /** For internal use only. DO NOT CALL */
    void setState (enum State state) {_state = state;}
    /** For internal use only. DO NOT CALL */
    enum State getState () {return _state;}
    void returnToFreelist(); /**< For internal use only. DO NOT CALL */
    /**
     * For internal use only. DO NOT CALL.
     * Returns true if the WorkItem came from a freelist, false otherwise
     */
    bool fromFreelist() { return (_freelist != NULL); }
    /** For internal use only. DO NOT CALL */
    bool isSubItem() { return _subitem; }
    /** For internal use only. DO NOT CALL */
    void setThreadPool(WorkerThreadPool *wtp) {_threadPool = wtp;}
    /** For internal use only. DO NOT CALL */
    unsigned int getQID() {return _qid;}
    /** For internal use only. DO NOT CALL */
    void setQID(unsigned int qid) {_qid = qid;}
    /** For internal use only. DO NOT CALL */
    int incrRefcount() { return ++_refcount;}
    /** For internal use only. DO NOT CALL */
    int decrRefcount() { --_refcount; assert(_refcount >=0); return _refcount;}
    /** For internal use only. DO NOT CALL */
    WorkItem *getParent() { return _parent; }
    /** For internal use only. DO NOT CALL */
    void setGen(uint64_t gen) { _gen = gen; }
    /** For internal use only. DO NOT CALL */
    uint64_t getGen() { return _gen; }
protected:
    /**
     * Adds a SubItem to the concurrent queue.
     * Subitems can only be added from a WorkItem executing in a
     * sequential queue.
     * Subitems cannnot create more subitems.
     * The sequential queue is paused till all subitems and
     * parent WorkItem complete.
     * The function subitemsComplete() gets invoked by the threadpool when all
     * submitems complete.
     * The function is guaranteed to be invoked after all submitems
     */
    void addSubItem(WorkItem *wi);
private:
    enum State _state; //**< The state of a WorkItem at any given time.
    Freelist *_freelist;
    bool _subitem;
    WorkerThreadPool *_threadPool;
    unsigned int _qid; 
    /**
     * The _refcount indicates how many subitems are outstanding.
     * When a WorkItem starts executing the refcount is 1.
     * Whenever a SubItem is launched, the refcount gets incremented.
     * Whenever a SubItem completes, the refcount gets decremented.
     */
    int _refcount;
    /** 
     * Valid only in subitems.
     * It's a pointer to the WorkItem that spawned this object.
     */
    WorkItem *_parent;
    uint64_t _gen; // Used primarily for debugging. Valid only when in WTP.

    WorkItem& operator=(const WorkItem& rhs);
    WorkItem(const WorkItem& rhs);
};

}

#endif
