// IfIoctls.h
// Handle things like Set Mac Address, Bring Interface Up / Down
// that don't appear to be done by Nl80211.

#ifndef IFIOCTLS_H_
#define IFIOCTLS_H_

#include <iostream>
#include <string>
#include <sstream>

#include  <cstring>

#include <stdint.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <net/if_arp.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <arpa/inet.h>
// #include <linux/wireless.h>  NO: Compile errors "redef of 'struct ifconf'
//   -- use local version named: "ShxWireless.h" instead (below)
#include <errno.h>

// ShadowX's version of linux/wireless.h:
#include "ShxWireless.h"
#include "Log.h"

using namespace std;

class IfIoctls : public Log
{
public:
	IfIoctls();
	~IfIoctls();
	bool BringInterfaceUp(const char *interfaceName);
	bool BringInterfaceDown(const char *interfaceName);
	bool GetInterfaceFlags(const char *interfaceName, int& rawFlags, bool& isUp, bool& isRunning);
	bool SetIpAddressAndNetmask(const char *ifaceName, const char *ipAddress, const char *netmask);
	bool SetMacAddress(const char *ifaceName, const uint8_t *mac, bool isMonitorMode);
	bool SetWirelessPowerSaveOff(const char *ifaceName);
private:
	bool GetFlags(const char *interfaceName, int& flags);
	bool SetFlags(const char *interfaceName, int flags);
	bool Open();
	bool Close();
	int m_fd = -1;
};

#endif  // IFIOCTLS_H_

