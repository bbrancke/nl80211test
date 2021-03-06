// Terminator.h

#ifndef TERMINATOR_H_
#define TERMINATOR_H_

#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <map>
#include <sstream>
#include <thread>
#include <chrono>
#include <cstdlib>
#include <cstring>

#include <ctype.h>
#include <sys/types.h>
#include <dirent.h>
#include <signal.h>
#include <errno.h>

#include "Log.h"

using namespace std;
using namespace chrono;

class Terminator : public Log
{
public:
	Terminator() : Log("Terminator") { }
	bool Terminate(const char *appName, pid_t ignorePid);
private:
	bool KillInstancesOf(int signal, const char *appName, pid_t ignorePid, int& numKilled);
};

#endif  // TERMINATOR_H_

