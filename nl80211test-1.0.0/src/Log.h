// Log.h - placeholder

#ifndef LOG_H_
#define LOG_H_

#include <iostream>
#include <string>
#include <sstream>

#define AT "at..."

using namespace std;

class Log
{
public:
	Log();
	Log(const char *name);
	void SetLogName(const char *name);
	void LogErr(const char *at, const char *msg);
	void LogErr(const char *at, const string& msg);
	void LogErr(const char *at, const stringstream& msg);
	void LogInfo(const char *msg);
	void LogInfo(const string& msg);
	void LogInfo(const stringstream& msg);
};

#endif  // LOG_H_

