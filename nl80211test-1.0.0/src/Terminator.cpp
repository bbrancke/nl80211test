// Terminator.cpp

#include "Terminator.h"

// Call KillInstancesOf() with SIGTERM, wait 5-10 seconds and call a 2nd time with SIGKILL
// Send (-1) for signal to get count of running appName's (won't send kill() to them).
// appName: app to terminate all instances of.
// Ignore PID: ShadowX main()'s PID, this will ignore and not kill it.
bool Terminator::KillInstancesOf(int signal, const char *appName, pid_t ignorePid, int& numKilled)
{
	using namespace std;

	stringstream pidToIgnore;
	DIR *dir;
	struct dirent *entry;
	int rv;
	numKilled = 0;

	pidToIgnore << ignorePid;	

	dir = opendir("/proc");
	if (dir == NULL)
	{
		int myErr = errno;
		string s("opendir() failed: ");
		s += strerror(myErr);
		LogErr(AT, s);
		return false;
	}
	
	while (NULL != (entry = readdir(dir)))
	{
		// d_name is a number that is the pid.
		if (!isdigit(*entry->d_name))
		{
			continue;
		}
		if (pidToIgnore.str() == entry->d_name)
		{
			// Don't mess around with the parent...
			continue;
		}
		// ex: "cat /proc/1316/status":
		// First Line is "Name:" - find ones that match appName
		// Name: metalayer-crawl
		// State: S (sleeping)
		// SleepAVG:   84%
		// ... We only care about Name: in here, PID is folder in /proc/ (1316).
		string fileName("/proc/");
		fileName += entry->d_name;
		fileName += "/status";
		ifstream f(fileName.c_str());
		string exeName;
		string line;
		bool found = false;
		if (f.is_open())
		{
			while (!found && getline(f, line))
			{
				if (line.substr(0, 5) == "Name:")
				{
					found = true;
					exeName = line.substr(6);
				}
			}
			f.close();
		}
		if (!found)
		{
			continue;
		}
		if (exeName != appName)
		{
			continue;
		}

		if (signal >= 0)
		{
			pid_t pid = (pid_t)atoi(entry->d_name);

			rv = kill(pid, signal);
			if (rv != 0)
			{
				int myErr = errno;
				string s("kill() failed: ");
				s += strerror(myErr);
				LogErr(AT, s);
			}
		}
		numKilled++;
		// Continue through process folder for other
		// instances of appName...
	}  // while (entry != NULL)

	closedir(dir);

	return true;
}

bool Terminator::Terminate(const char *appName, pid_t ignorePid)
{
	int numKilled;

	if (!KillInstancesOf(SIGTERM, appName, ignorePid, numKilled))
	{
		return false;
	}
	if (numKilled == 0)
	{
		string s("Terminate: No processes named ");
		s += appName;
		s += " were running.";
		LogInfo(s);
		return true;
	}
	stringstream stat1;
	stat1 << "Terminator sent SIGTERM to " << numKilled <<
		" instance(s) of " << appName;
	LogInfo(stat1);
	stringstream level;
	for (int i = 0; i < 10; i++)
	{
		this_thread::sleep_for(milliseconds(500));
		// Send (-1) as signal to just get # of processes running...
		if (!KillInstancesOf(-1, appName, ignorePid, numKilled))
		{
			return false;
		}
		if (numKilled == 0)
		{
			// All have stopped.
			level << "Level 1 (" << i << " iters DONE. 1 iter = 500ms)";
			LogInfo(level);
			return true;
		}
	}

	// After 5 seconds, send the force-kill SIGKILL
	if (!KillInstancesOf(SIGKILL, appName, ignorePid, numKilled))
	{
		return false;
	}
	if (numKilled == 0)
	{
		// They all finished and exited...
		level << "Level 2 (SIGKILL, but they were already dead)";
		LogInfo(level);
		return true;
	}
	stringstream stat2;
	stat2 << numKilled << " instance(s) of " << appName <<
		" were still running, sent SIGKILL...";
	LogInfo(stat2);
	for (int i = 0; i < 8; i++)
	{
		this_thread::sleep_for(milliseconds(500));
		// Send (-1) as signal to just get # of processes running...
		if (!KillInstancesOf(-1, appName, ignorePid, numKilled))
		{
			return false;
		}
		if (numKilled == 0)
		{
			LogInfo("All instances have now closed.");
			level << "Level 3 (SIGKILL: all stopped after " << i << " iters (1 iter = 500ms).";
			LogInfo(level);
			return true;
		}
	}

	if (!KillInstancesOf(SIGKILL, appName, ignorePid, numKilled))
	{
		return false;
	}
	if (numKilled == 0)
	{
		LogInfo("All instances have now closed after second KILL");
		return true;
	}
	
	stringstream stat3;
	stat3 << numKilled << " instances of " << appName <<
			" are STILL running. I give up.";
	LogErr(AT, stat3);
	return false;
}

