
#include "Logger.h"
#include <iostream>

using namespace std;

void Logger::run()
{
	cout << (logtime-starttime) << ": " << message << endl;
}

void Logger::log(const std::string& msg)
{
	message = msg;
	logtime = time(NULL);
}

void Logger::reset()
{
	message = string("");
	logtime = 0;
}	

uint64_t Logger::starttime(0);
void Logger::init()
{
	starttime = time(NULL);
}
