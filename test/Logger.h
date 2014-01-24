
#include "WorkItem.h"
#include "time.h"
#include <string>

using namespace WTP;

class Logger : public WorkItem
{
public:
	void run();
	void reset();
	void log(const std::string& msg);
	static void init();
private:
	uint64_t logtime;
	std::string message;
	static uint64_t starttime;
};
