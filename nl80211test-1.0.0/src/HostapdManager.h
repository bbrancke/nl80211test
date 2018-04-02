// HostapdManager.h
// Start Hostapd, get Interface name from InterfaceManager

#ifndef HOSTAPDMANAGER_H_
#define HOSTAPDMANAGER_H_

#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <vector>
#include <algorithm>

#include <cstring>
#include <cstdlib>
#include <ctime>

#include <unistd.h>
#include <stdint.h>
#include <sys/stat.h>
#include <errno.h>

#include "Log.h"
#include "IfIoctls.h"
#include "InterfaceManagerNl80211.h"

using namespace std;

class HostapdManager : public Log
{
public:
	HostapdManager() : Log("HostapdManager") { }
	bool StartHostapd();
	bool UpdateHostapdConf(const char *interfaceName);
	bool WriteNewHostapdConf(const char *interfaceName);
	bool WriteNewDhcpdConf(const char *interfaceName);
	bool UpdateDhcpConf(const char *interfaceName);
private:
	const char *m_hostapdConfName = "/etc/hostapd.conf";
	const char *m_hostapdConfTempName = "/etc/hostapd.temp";
	const char *m_apIpAddress = "192.168.40.1";
	const char *m_apNetmask = "255.255.255.0";
	// For DHCP: Start, end IP addresses to hand out, lease time, etc.
	// If you change the AP IP address (above), you should set
	//   these values so they match up.
	const char *m_dhcpConfName = "/etc/udhcpd.conf";
	const char *m_dhcpConfTempName = "/etc/udhcpd.temp";
	const char *m_dhcpStart = "192.168.40.20";
	const char *m_dhcpEnd = "192.168.40.240";
	// (use defaults for everything else).
};

#endif  // HOSTAPDMANAGER_H_
