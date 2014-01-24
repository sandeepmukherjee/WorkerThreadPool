
#include "WorkerThreadPool.h"
#include <string>
#include <iostream>
#include <sstream>
#include <cstdlib>
#include "Freelist.h"
#include "Logger.h"
#include "WTPExceptions.h"

using namespace std;

WorkerThreadPool *wtp = NULL;
unsigned int logq = 0;
Freelist loglist;

static void logInit()
{
	int i;
	for (i=0; i< 20; i++) {
		Logger *logitem = new Logger();
		loglist.addItem(logitem);
	}
	logq = wtp->addQueue();
	Logger::init();

}

static inline void log(const string& str)
{
	if (logq == 0 or wtp == NULL)
		throw string("Attempt to use logging but WTP not initialized");
	Logger *logger = dynamic_cast<Logger *>(loglist.getItem());
	if (logger == NULL)
		logger = new Logger();
	logger->log(str);
	wtp->addWorkItem(logger, logq);
}

static inline void log(ostringstream& buf)
{
	log(buf.str());
	buf.clear();
	buf.str("");
}

static inline unsigned int randnum(int seconds) {
	return int((double(rand())/RAND_MAX) * seconds);
}

class SubItem : public WorkItem {
public:
	SubItem(string name) : myname(name) {}
	void run() {
		unsigned int tm = randnum(7);
		buf << "Subitem " << this << " starting. Sleeping for " << tm <<
		       " seconds";
		log(buf);
		sleep(tm);
		buf << "Subitem " << this << " has woken.";
		log(buf);
	}
	void reset() {
		buf.clear(); buf.str("");
	}
private:
	string myname;
	ostringstream buf;
};

class TestItem : public WorkItem {
public:
	TestItem(int serial) : _serial(serial) {}
 void run() {
		buf << "TestItem " << this << " starting..";
		log(buf);
		SubItem *si = new SubItem("First subitem");
		SubItem *si2 = new SubItem("Second subitem");
		addSubItem(si);	
		addSubItem(si2);	
		unsigned int tm = randnum(7);
		buf << "TestItem " << this << " sleeping for " << tm << " seconds..";
		log(buf);
		sleep(tm);
		buf << "TestItem " << this << " has woken.";
		log(buf);
	}

	void subitemsComplete() {
		buf << "TestItem " << this << " done.";
		log(buf);
	}
private:
	int _serial;
	ostringstream buf;
};

int main()
{
try {
	int i;
	Freelist freelist;
	wtp = new WorkerThreadPool();
	unsigned int sq = wtp->addQueue();
	cout << "Added serial queue " << sq << endl;

	logInit();

	for (i=0; i< 20; i++) {
		TestItem *ti = new TestItem(i);
		freelist.addItem(ti);
	}

	wtp->startThreads();

	ostringstream buf;
	for (i=0; i< 8; i++) {
		WorkItem *wi = freelist.getItem();
		if (wi == NULL)
			throw string("Freelist empty");
		buf << "Adding TestItem " << wi << " to queue";
		log(buf);
		wtp->addWorkItem(wi, sq);
		wtp->addWorkItem(wi, sq);
	}
	log("All WI added. Waiting for empty..");

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
