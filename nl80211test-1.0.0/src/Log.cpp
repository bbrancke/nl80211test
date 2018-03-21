// Log.cpp - placeholder

#include "Log.h"

Log::Log() { }
Log::Log(const char *name) { cout << "LOG NAME: " << name << endl; }
void Log::SetLogName(const char *name) { }

void Log::LogErr(const char *at, const char *msg)
{
	cout << "ERROR at: " << at << ": " << msg << endl;
}

void Log::LogErr(const char *at, const string& msg)
{
	LogErr(at, msg.c_str());
}

void Log::LogErr(const char *at, const stringstream& msg)
{
	LogErr(at, msg.str());
}


void Log::LogInfo(const char *msg)
{
	cout << "INFO: " << msg << endl;
}

void Log::LogInfo(const string& msg)
{
	LogInfo(msg.c_str());
}

void Log::LogInfo(const stringstream& msg)
{
	LogInfo(msg.str());
}

