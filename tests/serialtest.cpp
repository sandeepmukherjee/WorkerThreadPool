
#include "Freelist.h"
#include "Logger.h"
#include "WorkerThreadPool.h"
#include "WTPExceptions.h"
#include <string>
#include <iostream>
#include <cstdlib>
#include <unistd.h>

using namespace std;
WorkerThreadPool *wtp = NULL;
uint64_t starttime;


class TestItem : public WorkItem {
public:
    TestItem(int serial) : _serial(serial) {}
 void run() {
        uint64_t delta = time(NULL) - starttime;
        int tm = int((double(rand())/RAND_MAX) * 5) + 1;
        cout << delta << ": starting serial=" << _serial << ", sleeptime=" << tm << endl;
        sleep(tm);
        delta = time(NULL) - starttime;
        cout << delta << ": done. serial=" << _serial << endl;
    }
    void reset() {
        _serial = -1;    
    }
private:
    int _serial;
};

int main()
{
try {
    int i;
    starttime = time(NULL);
    wtp = new WorkerThreadPool();
    unsigned int sq = wtp->addQueue();

    wtp->startThreads();

    for (i=0; i< 20; i++) {
        WorkItem *wi = new TestItem(i);
        wtp->addWorkItem(wi, sq);
    }

    cout << "Waiting for empty\n";
    wtp->waitEmpty();
    cout << "WTP empty. Waiting for shutdown.\n";
    wtp->shutDown();
    delete wtp;

} catch (CallerError& exc) {
    cout << "Illegal WTP use: " << exc.getMessage() << endl;
} catch (InternalError& exc) {
    cout << "InternalError: " << exc.getMessage() << endl;
}    

}
