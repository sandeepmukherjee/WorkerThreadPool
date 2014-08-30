
/*
 * This is a collection of tests to ensure that WTP generates errors at 
 * appropriate times.
 */

#include "Freelist.h"
#include "Logger.h"
#include "WorkerThreadPool.h"
#include "WTPExceptions.h"
#include <string>
#include <iostream>
#include <cstdlib>

using namespace std;
WorkerThreadPool *wtp = NULL;
uint64_t starttime;

/**
 * Sleeps for a specified number of seconds.
 */
class Sleeper : public WorkItem {
public:
    /**
     * @param sleeptime Sleep duration (seconds).
     */
    Sleeper(int sleeptime) : _sleeptime(sleeptime) {}
	void run() {
        uint64_t delta = time(NULL) - starttime;
        cout << delta << ": starting sleeper test" << 
                ", sleeptime=" << _sleeptime << endl;
        sleep(_sleeptime);
        delta = time(NULL) - starttime;
        cout << delta << ": done." << endl;
    }
    void reset() {
        _sleeptime = -1;    
    }
private:
    int _sleeptime;
};

/**
 * Add same WI twice.
 */
void test1()
{
    int sleeptime = 5;
    starttime = time(NULL);
    wtp = new WorkerThreadPool();
    unsigned int sq = wtp->addQueue();
    wtp->startThreads();

	Sleeper *wi = new Sleeper(sleeptime);
	wtp->addWorkItem(wi, sq);

	// Try adding a WI that's already in queue.
    try {
        wtp->addWorkItem(wi, sq);
        throw string("Test failure. Was expecting a CallerError. Did not see it");
    } catch (CallerError& ex) {
        cout << "WTP generated caller error (expected): " <<
                ex.getMessage() << std::endl;
    }

    cout << "Waiting for empty\n";
    wtp->waitEmpty();
    cout << "WTP empty. Waiting for shutdown.\n";
    wtp->shutDown();
    delete wtp;


}

class SharedFlag : public SharedObject {
public:
    bool getStatus() { return _status;}
    void setStatus(bool status) { _status = status;}
    SharedFlag() : _status(false) {}
private:
    bool _status;
};

/**
 * Does something illegal.
 */
class Rogue : public WorkItem {
public:
    Rogue(WorkerThreadPool *wtp, SharedFlag& sf) : _wtp(wtp), _sf(sf) {}
	void run() {
        try {
            _wtp->waitEmpty();
            _sf.setStatus(false);
        } catch (CallerError& err) {
            cout << "WTP threw CallerError(expected): " << 
                    err.getMessage() << endl;
                    _sf.setStatus(true);
        }
    }
    void reset() {
        _wtp = NULL;
    }
private:
    WorkerThreadPool *_wtp;
    SharedFlag& _sf;
};

void test2()
{
    WorkerThreadPool *wtp = new WorkerThreadPool();
    wtp->startThreads();
    SharedFlag so;
    Rogue *wi = new Rogue(wtp, so);
	wtp->addWorkItem(wi);
    cout << "Waiting for empty\n";
    wtp->waitEmpty();
    cout << "WTP empty. Waiting for shutdown.\n";
    wtp->shutDown();
    delete wtp;

    bool status = so.getStatus();
    if (!status)
        throw string("Expected exception was not thrown.");


}

/**
 * Test for illegal queue check.
 */
void test3()
{
    WorkerThreadPool *wtp = new WorkerThreadPool();
    wtp->startThreads();
    Sleeper *wi = new Sleeper(1); // Any WI will work. It will never run.
    try {
        wtp->addWorkItem(wi, 1);
    } catch (CallerError& ex) {
        cout << "WTP generated caller error (expected): " <<
                ex.getMessage() << std::endl;
    }
}

int main()
{
try {
    test1();
    test2();
    test3();
} catch (CallerError& exc) {
    cout << "Unexpected test error (this may be a bug in test program): " << exc.getMessage() << endl;
} catch (InternalError& exc) {
    cout << "InternalError (This may be a WTP bug): " << exc.getMessage() << endl;
}    

}
