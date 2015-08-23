
#include "Freelist.h"
#include "WTPExceptions.h"
#include "Logger.h"
#include "WorkerThreadPool.h"
#include <string>
#include <iostream>
#include <cstdlib>
#include <sstream>
#include <assert.h>

using namespace std;
WorkerThreadPool *wtp = NULL;
unsigned int logq = 0;
Freelist loglist;

static void log(const std::string& str)
{
    if (logq == 0 or wtp == NULL)
        throw string("Attempt to use logging but WTP not initialized");
    Logger *logger = dynamic_cast<Logger *>(loglist.getItem());
    if (logger == NULL)
        logger = new Logger();
    logger->log(str);
    wtp->addWorkItem(logger, logq);
}


class TestItem : public WorkItem {
public:
    TestItem(int serial) : _serial(serial) {}
 void run() {
        _tm = int((double(rand())/RAND_MAX) * 5);
        buf << "Starting " << toString() << ", sleeptime=" << _tm;
        string str = buf.str();
        log(str);
        buf.clear(); buf.str("");
        sleep(_tm);
        buf << "Done " << _serial;
        log(buf.str());
        // buf.clear(); buf.str("");
    }
    void reset() {
        buf.clear(); buf.str("");
    }

private:
    int _serial;
    ostringstream buf;
    std::string _name;
    int _tm;
};

int main()
{
try {
    Logger::init();
    int i;
    Freelist freelist;
    wtp = new WorkerThreadPool();
    unsigned int sq = wtp->addQueue();
    logq = wtp->addQueue();

    TestItem *ti = new TestItem(999);
    
    freelist.addItem(ti);
    for (i=0; i< 20; i++) {
        ti = new TestItem(i);
        freelist.addItem(ti);
    }

    for (i=0; i< 20; i++) {
        Logger *logitem = new Logger();
        loglist.addItem(logitem);
    }

    wtp->startThreads();

    ti = dynamic_cast<TestItem *>(freelist.getItem());
    wtp->addWorkItem(ti, sq);
    for (i=0; i< 20; i++) {
        WorkItem *wi = freelist.getItem();
        if (wi == NULL)
            throw string("Freelist empty");
        wtp->addWorkItem(wi);
        sleep(1); // Gives a chance for concurrent queue to empty out.
        ostringstream buf;
        uint32_t proc = wtp->getTotalProcessing();
        uint32_t queued = wtp->getTotalQueued();
        uint32_t total = wtp->getTotalItems();
        buf << "Total queued = " << queued <<
        ", total processing = " << proc <<
        ", total Items = " << total;
        log(buf.str());
        assert(total == proc+queued);
    }

    cout << "Waiting for empty\n";
    wtp->waitEmpty();
    cout << "WTP empty. Waiting for shutdown.\n";
    wtp->shutDown();
    delete wtp;

} catch (CallerError& exc) {
    cout << "Illegal use of WTP library: " << exc.getMessage() << endl;
}    

}
