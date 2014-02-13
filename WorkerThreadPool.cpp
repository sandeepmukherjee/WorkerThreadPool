
/* vim: set shiftwidth=4 tabstop=4: */

#include "WorkerThreadPool.h"
#include "WTPExceptions.h"

#include <pthread.h>
#include <string>
#include <iostream>

using namespace std;
using namespace WTP;

static void *thread_start(void *arg)
{
    WorkerThreadPool *wtp = (WorkerThreadPool *)arg;
    wtp->_threadStart();

    return NULL;
}

WTP::WorkerThreadPool::WorkerThreadPool(int nthreads) :
                   _nthreads(nthreads), _nqueued(0), _nprocessing(0),
                    _totalItems(0), _shuttingDown(false), _gen(0)
{
    int rc = pthread_mutex_init(&_mutex, NULL);
    if (rc != 0)
        throw InternalError("Failed to initialize mutex", __FILE__, __LINE__);
    rc = pthread_cond_init(&_cond, NULL);
    if (rc != 0)
        throw InternalError("Failed to initialize conditional variable",
                            __FILE__, __LINE__);
    rc = pthread_cond_init(&_empty_cond, NULL);
    if (rc != 0)
        throw InternalError("Failed to initialize conditional variable",
                            __FILE__, __LINE__);

    WTPqueue defqueue;
    queuelist.push_back(defqueue);
    _threadpool = new pthread_t[nthreads];
}


void WTP::WorkerThreadPool::startThreads()
{
    int i;
    for (i=0; i<_nthreads; i++) {
        pthread_t thread;
        int rc = pthread_create(&thread, NULL, thread_start, (void *)this);
        if (rc) {
            throw InternalError("Thread creation failed", __FILE__, __LINE__);
        }
        _threadpool[i] = thread;
    }
}

bool WTP::WorkerThreadPool::isEmpty()
{
    lock();
    int ret = _totalItems;
    unlock();
    return (ret == 0);

}

void WTP::WorkerThreadPool::shutDown()
{
    lock();
    _shuttingDown = true;
    pthread_cond_broadcast(&_cond);
    unlock();
    // Wait for all threads to die.
    int i;
    for (i=0; i< _nthreads; i++) {
        pthread_join(_threadpool[i], NULL);
    }

    // No more worker threads running. We don't need the lock now.
    // Drain and free up queues.

    while(!queuelist.empty()) {
        WTPqueue& q = queuelist.back();
        q.drain();
        queuelist.pop_back();
    }

}

void WTP::WorkerThreadPool::_threadStart()
{
    uint32_t total_items = 0;
    while (1) {
        lock();  /////////////// Begin critical region /////////////////

        unsigned int qid = 0;
        WTPqueue* pqueue = NULL;
        while ((pqueue = _getNextQueue(qid)) == NULL && !_shuttingDown) {
            pthread_cond_wait(&_cond, &_mutex);
        }

        if(_shuttingDown == true) {
            unlock();
            return;
        }

        WorkItem *wi = pqueue->front(); pqueue->pop();
        _nqueued--; _nprocessing++;
        wi->incrRefcount();
        pqueue->deliverEvent(WTPqueue::WI_START);

        unlock(); /////////////// End critical region /////////////////

        if (wi->getState() != WorkItem::QUEUED)
            throw CallerError("Workitem not in queued state"); // Caller added an illegal WorkItem
        wi->setState(WorkItem::RUNNING);
        wi->setThreadPool(this);
        wi->setQID(qid);

        wi->run(); // Need to catch Exceptions. TBD.

        wi->setThreadPool(NULL);
        wi->setState(WorkItem::IDLE);

        lock(); /////////////// Begin critical region /////////////////
        _nprocessing--;

        pqueue->deliverEvent(WTPqueue::WI_END);
        int refcount = wi->decrRefcount();
        if (refcount == 0) {
            decrTotalItems();
            pqueue->deliverEvent(WTPqueue::ALL_DONE);
        }
        
        WorkItem *parent = NULL;
        if (wi->isSubItem()) {
            parent = wi->getParent();
            int prefcount = parent->decrRefcount();
            unsigned int pqid = parent->getQID();
            WTPqueue& parentQueue = getQueue(pqid);
            parentQueue.deliverEvent(WTPqueue::SI_END);
            if (prefcount == 0) {
                decrTotalItems();
                parentQueue.deliverEvent(WTPqueue::ALL_DONE);
            }
            else
                parent = NULL;
        }

        if (refcount > 0)
            wi = NULL; // Don't deallocate a WI with subitems.

        total_items = _totalItems;
        pthread_cond_signal(&_cond);
        unlock(); /////////////// End critical region /////////////////

        completeItem(wi); // Note, wi may be NULL. This is OK.
        completeItem(parent);

        if (total_items == 0) {
            lock();
            pthread_cond_signal(&_empty_cond);
            unlock();
        }
    }
}

void WTP::WorkerThreadPool::completeItem(WorkItem *wi)
{
    if (wi == NULL)
        return;
    wi->subItemsComplete(); // This is a caller-defined function

    // cout << "Deallocating workitem " << wi << endl;
    if (wi->fromFreelist()) {
        wi->reset();
        wi->setState(WorkItem::FREELIST);
        wi->returnToFreelist();
    } else {
        wi->setState(WorkItem::INVALID);
        delete wi;
    }

}

void WTP::WorkerThreadPool::waitEmpty()
{
    lock();
    if (_threadIsInternal()) {
        unlock();
        throw CallerError("waitEmpty() called from within WorkItem.");
    }
    if (_nqueued == 0 and _nprocessing == 0) {
        unlock();
        return;
    }
    pthread_cond_wait(&_empty_cond, &_mutex);
    unlock();
}

WTPqueue& WTP::WorkerThreadPool::getQueue(unsigned int qid)
{
    unsigned int size = queuelist.size();
    if (qid >= size)
        throw CallerError("Illegal queue ID specified");
    return queuelist.at(qid);
}

WTPqueue* WTP::WorkerThreadPool::_getNextQueue(unsigned int& qid)
{
    int size = queuelist.size();
    int i;
    for (i=0; i<size; i++) {
        WTPqueue& q = queuelist.at(i);
        if(!q.empty() ) {
            if (i == 0) {
                qid = i;
                return &q;
            } else {
                if (q.getState() == WTPqueue::IDLE) {
                    qid = i;
                    return &q;
                }
            }
        }
    }

    return NULL;
}

/*
 * Pause the donor queue. Subitems can only be added to parallel queue.
 */
void WTP::WorkerThreadPool::_addSubItem(WorkItem *item, WorkItem *parent)
{
    unsigned int donorQid = parent->getQID();
    if (donorQid == 0)
        throw CallerError("Attempt to create subitems from parallel queue");
    lock();
    WTPqueue& donorQ = queuelist.at(donorQid);
    donorQ.setWaitingWI(parent);
    donorQ.deliverEvent(WTPqueue::SI_START);
    unlock();
    addWorkItem(item, 0); // Add to parallel queue.
}

void WTP::WorkerThreadPool::addWorkItem(WorkItem *item, unsigned int qid)
{
    if (item->getState() != WorkItem::IDLE)
        throw CallerError("Attempt to add non-idle WorkItem to threadpool");
    lock();
    // cout << "Added workitem " << item << endl;
    unsigned int size = queuelist.size();
    if (qid >= size) {
        unlock();
        throw CallerError("Attempt to add WorkItem to illegal queue ID");
    }
    WTPqueue& pqueue = queuelist.at(qid);
    item->setState(WorkItem::QUEUED);
    item->setGen(createGen());
    pqueue.push(item);
    _nqueued++;
    incrTotalItems();

    pthread_cond_signal(&_cond);
    unlock();
}

void WTP::WorkerThreadPool::incrTotalItems()
{
    _totalItems++;
    // cout << "Total Items=" << _totalItems << endl;
}

void WTP::WorkerThreadPool::decrTotalItems()
{
    _totalItems--;
    // cout << "Total Items=" << _totalItems << endl;
}

unsigned int WTP::WorkerThreadPool::addQueue()
{
    WTPqueue newqueue;
    lock();
    int qid = queuelist.size();
    newqueue.setQID(qid);
    queuelist.push_back(newqueue);
    unlock();
    return qid;
}

bool WTP::WorkerThreadPool::_threadIsInternal()
{
    pthread_t mine = pthread_self();
    int i;
    for (i=0; i<_nthreads; i++) {
        if (_threadpool[i] == mine)
            return true;
    }
    return false;
}

void WorkerThreadPool::lock()
{
   int rc = pthread_mutex_lock(&_mutex);
   if (rc != 0)
       throw InternalError("Failed to lock mutex", __FILE__, __LINE__);
}

void WorkerThreadPool::unlock()
{
       int rc = pthread_mutex_unlock(&_mutex);
       if (rc != 0)
		   throw InternalError("Failed to unlock mutex", __FILE__, __LINE__);
}

uint32_t WorkerThreadPool::getTotalItems()
{
    uint32_t ret = 0;
    lock();
    ret = _totalItems;
    unlock();
    return ret;
}

uint32_t WorkerThreadPool::getTotalProcessing()
{
    uint32_t ret = 0;
    lock();
    ret = _nprocessing;
    unlock();
    return ret;
}

uint32_t WorkerThreadPool::getTotalQueued()
{
    uint32_t ret = 0;
    lock();
    ret = _nqueued;
    unlock();
    return ret;
}

void WTPqueue::drain()
{
    while(!this->empty()) {
        WorkItem *wi = this->front(); this->pop();
        if (wi->fromFreelist()) {
            wi->reset();
            wi->setState(WorkItem::FREELIST);
            wi->returnToFreelist();
        } else {
            delete wi;
        }
    }
}

void WTPqueue::deliverEvent(enum QueueEvent evt)
{
    if (_qid == 0)
        return;  // No states for the concurrent queue.
    bool illegal = false;
    // cout << "Event " << evt << " caused queue " << _qid << " to transition from " << _state << " to ";
    switch (evt) {
    case WI_START:
        if (_state == IDLE)
            _state = PROCESSING;
        else
            illegal = true;
        break;
    case SI_START:
        if (_state == PROCESSING or _state == PROCESSING_SI)
            _state = PROCESSING_SI;
        else
            illegal = true;
        break;
    case WI_END:
        if (_state == PROCESSING)
            _state = IDLE;
        else if (_state == PROCESSING_SI)
            _state = PAUSED;
        else
            illegal = true;

        break;
    case SI_END:
        if (_state == IDLE or _state == PROCESSING) 
            illegal = true;
        break;
    case ALL_DONE:
        _state = IDLE;
        break;
    default:
        throw InternalError("Unhandled event to WTPqueue", __FILE__, __LINE__);
    }
    // cout << _state << endl;
    if (illegal)
        throw InternalError("Illegal state transition to WTPqueue", __FILE__, __LINE__);

}
