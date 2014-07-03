
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

int main()
{
try {
    test1();
} catch (CallerError& exc) {
    cout << "Unexpected test error (this may be a bug in test program): " << exc.getMessage() << endl;
} catch (InternalError& exc) {
    cout << "InternalError (This may be a WTP bug): " << exc.getMessage() << endl;
}    

}
