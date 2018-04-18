// IfIoctls.cpp
// Handle things like Set Mac Address, Bring Interface Up / Down
// that aren't done in Nl80211.

#include "IfIoctls.h"

IfIoctls::IfIoctls() : Log("IfIoctls") { }

IfIoctls::~IfIoctls()
{
	Close();
}

bool IfIoctls::Open()
{
	Close();
	m_fd = socket(AF_INET, SOCK_DGRAM, 0);
	if (m_fd < 0)
	{
		int myErr = errno;
		string s("Open(): Can't get socket: ");
		s += strerror(myErr);
		LogErr(AT, s);
		return false;
	}
	return true;
}

bool IfIoctls::Close()
{
	if (m_fd > 0)
	{
		close(m_fd);
		m_fd = -1;
	}
	return true;
}

bool IfIoctls::GetFlags(const char *interfaceName, int& flags)
{
	struct ifreq ifr;
	if (!Open())
	{
		return false;
	}

	memset(&ifr, 0, sizeof(struct ifreq));
	strncpy(ifr.ifr_name, interfaceName, SHX_IFNAMESIZE);
	if (ioctl(m_fd, SIOCGIFFLAGS, &ifr) < 0)
	{
		int myErr = errno;
		Close();
		string s("Get interface flags failed: ");
		s += strerror(myErr);
		LogErr(AT, s);
		return false;
	}

	flags = ifr.ifr_flags;
	Close();
	return true;
}

bool IfIoctls::SetFlags(const char *interfaceName, int flags)
{
	struct ifreq ifr;
	if (!Open())
	{
		return false;
	}

	memset(&ifr, 0, sizeof(struct ifreq));
	strncpy(ifr.ifr_name, interfaceName, SHX_IFNAMESIZE);
	ifr.ifr_flags = (short)flags;
	if (ioctl(m_fd, SIOCSIFFLAGS, &ifr) < 0)
	{
		int myErr = errno;
		Close();
		string s("Set interface flags(");
		s += interfaceName;
		s += ") failed: ";
		s += strerror(myErr);
		LogErr(AT, s);
		return false;
	}

	Close();
	return true;
}

bool IfIoctls::BringInterfaceUp(const char *interfaceName)
{
	int flags;
	if (!GetFlags(interfaceName, flags))
	{
		// Failure reason already logged.
		return false;
	}
	flags |= IFF_UP;
	return SetFlags(interfaceName, flags);
}

bool IfIoctls::BringInterfaceDown(const char *interfaceName)
{
	int flags;
	if (!GetFlags(interfaceName, flags))
	{
		// Failure reason already logged.
		return false;
	}
	flags &= ~IFF_UP;
	return SetFlags(interfaceName, flags);
}

bool IfIoctls::GetInterfaceFlags(const char *interfaceName, int& rawFlags, bool& isUp, bool& isRunning)
{
	if (!GetFlags(interfaceName, rawFlags))
	{
		rawFlags = 0;
		isUp = false;
		isRunning = false;
		return false;
	}
	isUp = (rawFlags & IFF_UP) != 0;
	isRunning = (rawFlags & IFF_RUNNING) != 0;
	return true;
}

bool IfIoctls::SetIpAddressAndNetmask(const char *ifaceName, const char *ipAddress, const char *netmask)
{
	struct ifreq ifr;
	struct sockaddr_in sa;

	if (!Open())
	{
		return false;
	}

	memset(&ifr, 0, sizeof(struct ifreq));
	strncpy(ifr.ifr_name, ifaceName, SHX_IFNAMESIZE);
	memset(&sa, 0, sizeof(struct sockaddr_in));
	sa.sin_family = AF_INET;
	inet_aton(ipAddress, &sa.sin_addr);
	memcpy(&(ifr.ifr_addr), &sa, sizeof(sa));
	if (ioctl(m_fd, SIOCSIFADDR, &ifr) < 0)
	{
		int myErr = errno;
		Close();
		string s("SetIpAddressAndNetmask: Can't set addr (SIOCSIFADDR): ");
		s += strerror(myErr);
		LogErr(AT, s);
		return false;
	}

	inet_aton(netmask, &sa.sin_addr);
	memcpy(&(ifr.ifr_addr), &sa, sizeof(sa));
	if (ioctl(m_fd, SIOCSIFNETMASK, &ifr) < 0)
	{
		int myErr = errno;
		Close();
		string s("SetIpAddressAndNetmask: Can't set netmask (SIOCSIFNETMASK): ");
		s += strerror(myErr);
		LogErr(AT, s);
		return false;
	}
	Close();
	return true;
}

bool IfIoctls::SetMacAddress(const char *ifaceName, const uint8_t *mac, bool isMonitorMode)
{
	struct ifreq ifr;
	if (!Open())
	{
		return false;
	}

	memset(&ifr, 0, sizeof(struct ifreq));
	memcpy(&ifr.ifr_hwaddr.sa_data, mac, 6);
	strncpy(ifr.ifr_name, ifaceName, SHX_IFNAMESIZE);
	// For a normal 80211 interface (STA / AP)
	// sa_family is ONE (Ethernet 10/100Mbps), this is ARPHRD_ETHER (== 1)
	// (see /usr/include/net/if_arp.h)
	// For interface in MONITOR mode (with Radiotap headers),
	// address family is ARPHRD_IEEE80211_RADIOTAP (== 803)
	ifr.ifr_hwaddr.sa_family =
		isMonitorMode
			?
			ARPHRD_IEEE80211_RADIOTAP
			:
			ARPHRD_ETHER;

	if (ioctl(m_fd, SIOCSIFHWADDR, &ifr) < 0)
	{
		int myErr = errno;
		Close();
		string s("SetMacAddress: Can't set SIOCSIHWADDR: ");
		s += strerror(myErr);
		LogErr(AT, s);
		return false;
	}

	Close();
	return true;
}

// The TI chip crashes badly coming out of power save mode sometimes,
// causes loss of interface (and the AP!). So turn off power save mode:
bool IfIoctls::SetWirelessPowerSaveOff(const char *ifaceName)
{
	// NO: struct iwreq wrq;
	// In here we want to use the ShadowX version of iwreq,
	//   derived from linux/wireless.h:
	shx_iwreq wrq;

	if (!Open())
	{
		return false;
	}
	memset(&wrq, 0, sizeof(shx_iwreq));
	wrq.u.power.disabled = 1;
	strncpy(wrq.ifr_name, ifaceName, sizeof(wrq.ifr_name));
	// 0 = Success, -1 = error (errno is set)
	if (ioctl(m_fd, SHX_SIOCSIWPOWER, &wrq) < 0)
	{
		int myErr = errno;
		Close();
		string s("SetWirelessPowerSaveOff: Can't set SIOCSIWPOWER: ");
		s += strerror(myErr);
		LogErr(AT, s);
		return false;
	}
	return true;
}

bool IfIoctls::GetFrequency(const char *ifaceName,
  int32_t& Mantissa, int16_t& Exponent)
{
  shx_iwreq wrq;

	if (!Open())
	{
		return false;
	}
	memset(&wrq, 0, sizeof(shx_iwreq));
  strncpy(wrq.ifr_name, ifaceName, sizeof(wrq.ifr_name));
  if (ioctl(m_fd, SHX_SIOCGIWFREQ, &wrq) < 0)
  {
    int myErr = errno;
    Close();
    string s("GetFrequency: Can't SIOCGIWFREQ: ");
    s += strerror(myErr);
    LogErr(AT, s);
    return false;
  }
  Mantissa = wrq.u.freq.m;
  Exponent = wrq.u.freq.e;
  return true;
}

