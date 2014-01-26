
#ifndef WTP_WORKER_THREAD_POOL_H
#define WTP_WORKER_THREAD_POOL_H

#include "WorkItem.h"
#include "SharedObject.h"
#include <pthread.h>
#include <queue>

namespace WTP {

/**
 * @brief For internal use only. DO NOT USE.
 *
 * This is an internal use class.
 * Do not instantiate or invoke any method, not even public ones.
 * Represents a queue of WorkItem objects to be executed.
 */
class WTPqueue : public std::queue<WorkItem *> {
public:

    /**
     * States of a WTP queue. For state table see:
     * https://docs.google.com/spreadsheet/ccc?key=0AsHaZBDKpPwbdGFOLTZ4QVpuLXY1d3VSbnhPY0hjUkE&usp=sharing
     */
    enum State {
        IDLE, /**< No thread is processing a WorkItem from this queue. */
        PROCESSING, /**< A thread is processing a WorkItem from this queue. */
        PROCESSING_SI, /**< A thread is processing a WorkItem from this queue and the WorkItem has spawned one or more SubItems */
        PAUSED /**< A WorkItem from this queue has completed, but it is waiting for all SubItems to complete. No thread is blocked on this WorkItem. */
    };

    /**
     * Events that can be sent to a WTP queue.
     */
    enum QueueEvent {
        WI_START, /**< A WorkItem has started executing */
        SI_START, /**< A SubItem has started executing */
        WI_END, /** A WorkItem has returned from its Run method */
        SI_END, /** A SubItem has returned from its Run method */
        ALL_DONE /** The WorkItem and all SubItems have completed */
    };

    /** Default constructor */
    WTPqueue() :  _waitingWI(NULL), _state(IDLE),_qid(0) {}

    /**
     * Remove all waiting WorkItem objects and free them up or return them
     * to their freelists if they are from a freelist.
     * Neither the Run(), nor subItemsComplete() get called.
     */
    void drain();
    /**
     * If a WorkItem spawns one or more subitems, and completes before the
     * subitems complete, the queue maintains a pointer to the parent WorkItem
     * till all subitems complete.
     * This function retrieves the parent WorkItem
     * @return Parent WorkItem
     */

    WorkItem *getWaitingWI() { return _waitingWI; }
    /**
     * Stores the WorkItem in the queue.
     * @param wi WorkItem to be stored.
     */
    void setWaitingWI(WorkItem *wi) { _waitingWI = wi;}
    /**
     * Deliver an event to the WTPqueue. This may change its state.
     * @param evt Event to be delivered.
     */
    void deliverEvent(enum QueueEvent evt);
    /**
     * Get current state of the queue.
     */
    enum State getState() { return _state; }
    /**
     * Sets the Queue ID (QID) of this queue. This must only be done once.
     * @param qid QID to set
     */
    void setQID(unsigned int qid) { _qid = qid; }
private:
    /**< Pointer to the parent WorkItem that has spawned SubItem(s) and are
     * waiting for them to complete.
     */
    WorkItem *_waitingWI;
    State _state;
    unsigned int _qid; /**< Queue ID of this queue */
};

/**
 * @brief A class that represents a pool of threads that can
 * execute arbitray WorkItems.
 *
 * Callers add WorkItem objects to queues which are maintained internally
 * by WorkerThreadPool.
 * When a worker thread is free it looks in its list of queues,
 * selects one, removes the next WorkItem from that queue,
 * and invokes its Run() method. Every WorkerThreadPool comes with exactly
 * one "concurrent" queue.
 * WorkItem objects added to this queue may execute concurrently.
 * Callers can create additional queues, but all additional queues
 * are "sequential", meaning the next WorkItem does not get executed till
 * the current one has completed.
 * WorkItems can create SubItems, which execute concurrently. The parent
 * WorkItem waits till all subitems complete.
 * To use you need to:
 * - Instantiate a WorkerThreadPool object.
 * - [optional] Instantiate a Freelist and populate with pre-allocated
 *   WorkItem objects.
 * - Start all threads using the startThreads() function.
 * - [optional] Add one or more sequential queues.
 * - Instantiate a WorkItem and add to a queue. Repeat as needed.
 * - When done, call waitEmpty().
 * - Call shutDown() to shut down the threads.
 * - Deallocate WorkerThreadPool object.
 * - If using freelists, deallocate them.
 * @see WorkItem, Freelist
 */
class WorkerThreadPool
{
public:
    /**
     * Create an instance of a WorkerThreadPool.
     * @param[in] nthreads The initial number of threads in the threadpool.
     */
    WorkerThreadPool(int nthreads = 5);
    /**
     * Before destroying an instance of a WorkerThreadPool, callers should
     * shut it down.
     */
    ~WorkerThreadPool() { delete[] _threadpool; }
    /**
     * Start all worker threads. This must be invoked after instantiating this
     * class, or else none of the WorkItems will execute.
     */
    void startThreads();
    /**
     * Add a work item into one of the queues. The WorkItem stays in the queue
     * till a worker thread dequeues it and starts executing it.
     * @param[in] item WorkItem to be queues. IMPORTANT: Do not access item
     *            after this call outside of the WTP. The WTP will deallocate
     *            the item after it is complete, or return it to its freelist
     *              if it came from one.
     * @param[in] qid Queue ID of queue to add to. The qid is obtained by
     *            calling the addQueue() function. If omitted, the default
     *            (concurrent) queue is used.
     */
    void addWorkItem(WorkItem *item, unsigned int qid = 0);
    /**
     * Creates a new sequential queue.
     * @return The Queue ID (QID) of the new queue created.
     */
    unsigned int addQueue();
    /**
     * Shut down the WorkerThreadPool object. Executing work items are allowed
     * to proceed to completion. All queued work items are freed or returned
     * back to their respective freelists.
     * Because this function waits for all running WorkItems to complete, it
     * can block for a significant amount of time.
     */
    void shutDown();
    /**
     * @brief Waits for all WorkItems (queued as well as executing) to complete.
     *
     * Do not call from within a WorkItem.
     * Caution: This function may block for a long time.
     */
    void waitEmpty();

    /** For internal use unly. DO NOT CALL */
    void _addSubItem(WorkItem *item, WorkItem *parent);
    /** For internal use unly. DO NOT CALL */
    WTPqueue& getQueue(unsigned int qid);
    /** For internal use unly. DO NOT CALL */
    bool isEmpty(); /**< Returns true if WTP is empty, false otherwise */
    void _threadStart(); /**< For internal use only. */
private:
    int _nthreads; /**< Number of threads in the threadpool */
    unsigned int _nqueued; /**< Number of WorkItem queued and waiting to be executed */
    unsigned int _nprocessing; /**< # of WorkItems currently being processed by threads */
    unsigned int _totalItems; /**< Total  number of WorkItems in the pool */
    bool _shuttingDown; /**< true when the threadpool is shutting down */
    uint64_t _gen; /**< A monotonically increasing number, used for debugging */
    pthread_t *_threadpool; /**< List of POSIX threads */
    pthread_mutex_t _mutex;
    pthread_cond_t _cond; /**< Used to signal waiting threads that there are more WorkItems waiting */
    pthread_cond_t _empty_cond; /**< Used to signal a caller that the WTP is empty */
    std::vector<WTPqueue> queuelist;
    /**
     * Retrieves the next queue from which a WorkItem can be retrieved to 
     * run.
     * @return WTPQueue object.
     */
    WTPqueue* _getNextQueue(unsigned int& qid);
    bool _threadIsInternal(); /**< Returns true if the calling thread is a WTP worker thread */
    void completeItem(WorkItem *wi); /**< Complete post-execution steps of a WTP */
    uint64_t createGen() { return ++_gen; } /**< Returns a new generation number */
    void incrTotalItems(); /**< Increments the total WorkItem count */
    void decrTotalItems();/**< Decrements the total WorkItem count */

    void lock();
    void unlock();
    
};

}

#endif
