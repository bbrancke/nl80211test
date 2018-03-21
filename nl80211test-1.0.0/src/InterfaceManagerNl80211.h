// InterfaceManagerNl80211.h
// InterfaceManager using NL80211 subsystem.

#ifndef INTERFACEMANAGERNL80211_H_
#define INTERFACEMANAGERNL80211_H_

#include <iostream>
#include <string>
#include <sstream>
#include <vector>
#include <algorithm>

#include <cstring>
#include <cstdlib>
#include <ctime>

#include <stdint.h>

#include "Log.h"
#include "OneInterface.h"
#include "Nl80211InterfaceAdmin.h"
#include "IfIoctls.h"

// This is no longer based upon Interface Manager Interface.
// The Interface class was mostly empty, and the whole idea
// of ShadowX parts (main(), etc) being "isolated" from the
// actual implentation of wifi manipulaton started
// to make less and less sense and more and more difficult to implement... sigh..
//  #include "IInterfaceManager.h"  no longer

using namespace std;

class InterfaceManagerNl80211 : public Nl80211InterfaceAdmin
{
public:
	static InterfaceManagerNl80211* GetInstance();
	// This is a singleton; not copiable and not assignable:
	InterfaceManagerNl80211(InterfaceManagerNl80211 const&) = delete;
	InterfaceManagerNl80211& operator=(InterfaceManagerNl80211 const&) = delete;
	// Init() and CreateInterfaces() are called by main() at startup, in order:
	bool Init();
	bool CreateInterfaces();
	// Next main() can start hostapd and begin surveys.
	//
	// Survey-ChannelChanger->ChannelSet class calls this
	// (currently I ALWAYS return "mon0"):
	const char *GetMonitorInterfaceName();
private:
	InterfaceManagerNl80211();  // Private so that ctor can't be called
	static InterfaceManagerNl80211* m_pInstance;
	const uint8_t m_TiChipsetOui[3] = { 0xD0, 0xB5, 0xC2 };
	// It appears "ap0" is RESERVED, you can create a new ap0 but
	// BringUp() says "Interface name not unique"
	// No that is not it. sta0 comes up fine as a STA; wlan0 is a STA
	// Let us try ap0 as a STA and see if hostapd can bring ap0 up as an AP...
	// LATER: Can't have two interfaces with the same IP address,
	//        that's what causes "not unique" error. D'oh...
	//   TODO: Shouldn't we have members for STATION VIF name and MON0 VIF name too?
	// Was in the process of deleting this, but it does remove A LOT of "ap0"s from the code...
	const char *m_apName = "ap0";
	IfIoctls m_ifIoctls;
	bool GetTiInterfaceList(vector<OneInterface *>& tiInterfaces);
	bool GetInterfaceByPhyAndName(uint32_t phyId, const char *name,
		OneInterface **iface);
};


#endif  // INTERFACEMANAGERNL80211_H_

