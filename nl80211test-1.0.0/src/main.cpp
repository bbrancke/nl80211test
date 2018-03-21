// main.cpp - test code.
// To build:
// NO NO NO g++ -g -Wall -std=c++11 -I /usr/include/libnl3/ main.cpp InterfaceManagerNl80211.cpp Nl80211InterfaceAdmin.cpp ChannelSetterNl80211.cpp IfIoctls.cpp Nl80211Base.cpp Log.cpp -lnl-genl-3 -lnl-3 -o test
//
// Note: We do not have /usr/include/libnl3/ folder on ShadowX device.
//   I have added local copies of the netlink/*.h headers since
//   ShadowX does not have these headers and so can't build this
//   on the device; perhaps we need libnl3-dev /genl3-*dev* to the
//   main shadowx-console-image recipe??)
// Now use:
//  g++ -g -Wall -std=c++11 main.cpp InterfaceManagerNl80211.cpp Nl80211InterfaceAdmin.cpp ChannelSetterNl80211.cpp IfIoctls.cpp Nl80211Base.cpp Log.cpp -lnl-genl-3 -lnl-3 -o test
//
// "netlink/netlink.h: No such file or directory":
// Had to install:
//		sudo apt-get install libnl-3-dev
// and:
//		sudo apt-get install libnl-genl-3-dev
// and:  libnl-route-3-dev and libnfnetlink-dev
// and: sudo apt-get install libnl-dev libnl1
// WARNING: One of the without the '3' resets everything BACK!
// FINALLY, it is compiling...
// sudo apt-get install libnl-genl-3-dev
//
/****
Base class opens nl80211 socket, derived classes needed to implement.
Need InterfaceManagerNl80211 (SINGLETON, created by main(),
    somebody to detect platform "Y" or not (by OID?) (which
    main() should instantiate (only ONE for now, *no* WEXT/ioctl version.
---    by OID? Supports Virtual iface? LO band and HI band? )NOT IMPLEMENTED)
- setup at start, return iface for email, AP, surveys
-  MAC addresses to ignore for survey
- Init() get list of physical devices, ensures that we have TWO (not one,
   three is right out...)
-- CreateInterfaces(): Create virtual interfaces (VIFs)
    "sta0" and "ap0" for the TI (built-in) chip and
    "mon0" MONITOR VIF for the USB radio,
    del the 'base' interfaces (wlan0 and wlan1).

main() should use its Terminator class to kill any hostapd
   and wpa_supplicant's running BEFORE calling CreateInterfaces()
   and then start hostapd (on ap0) AFTER CreateInterfaces().
   Alert emails can then be sent using wpa_supplicant (on sta0).
InterfaceManagerNl80211 is a singleton and has methods that
   return the Interface Name that Survey's "Pcap Open" and
   "Channel Changer->ChannelSetter" *and* EmailManager should
   call in case the VIF names change.

ChannelChanger using nl80211 sted IOCTLs
Radiotap parser class for ShadowX *and* Cardinal.

NL80211:
https://github.com/yurovsky/utils/blob/master/nl-genl/list-80211.c
http://yurovsky.github.io/2015/01/06/linux-netlink-list-wifi-interfaces/
https://github.com/bmegli/wifi-scan
https://wireless.wiki.kernel.org/en/developers/documentation/nl80211
http://elixir.free-electrons.com/linux/latest/source/include/uapi/linux/nl80211.h

****/
#include <iostream>
#include <string>
#include <sstream>
#include <chrono>
#include <thread>

using namespace std;
using namespace chrono;

#include "Nl80211Base.h"
#include "Log.h"
#include "IfIoctls.h"
#include "InterfaceManagerNl80211.h"

void wait(const char *msg)
{
	cout << msg << ", wait 15..." << endl;
	this_thread::sleep_for(seconds(15));
}

void AddAnInterface(InterfaceManagerNl80211 *im)
{
	string in("");
	bool rv;
	int type;
	cout << "==============" << endl;
	cout << "Add Interface:" << endl << "Type:" << endl << "1: Station" << endl << "2. AP" << endl << "? ";
	getline(cin, in);
	switch (in[0])
	{
		case '1':
			type = 1;
			break;
		case '2':
			type = 2;
			break;
		default:
			cout << "Unknown value..." << endl;
			return;
	}
	
	cout << "  Add to phy #? ";
	getline(cin, in);
	int phyId = atoi(in.c_str());
	cout << "  New interface name? ";
	getline(cin, in);
	switch (type)
	{
		case 1:
			rv = im->CreateStationInterface(in.c_str(), phyId);
			break;
		case 2:
			rv = im->CreateApInterface(in.c_str(), phyId);
			break;
	}
	cout << rv ? "Success" : "FAILED";
	cout << endl;
}

void BringIfaceUpOrDown(IfIoctls *ifIoctls, bool up)
{
	string in("");
	bool rv;
	cout << "=====================" << endl;
	cout << "Bring Interface ";
	cout << up ? "UP" : "DOWN";
	cout << "." << endl;
	cout << "  Interface name? ";
	getline(cin, in);
	if (up)
	{
		rv = ifIoctls->BringInterfaceUp(in.c_str());
	}
	else
	{
		rv = ifIoctls->BringInterfaceDown(in.c_str());
	}
	cout << rv ? "Success" : "FAILED";
	cout << endl;
}

void SetAnInterfacesMode(InterfaceManagerNl80211 *im)
{
	// bool SetInterfaceMode(const char *interfaceName, InterfaceType itype);
	// itype: InterfaceType::Station, ::Ap, ::Monitor
	InterfaceType type;
	string in("");
	string iface("");
	bool rv;

	cout << "===================" << endl;
	cout << "Set Interface Mode:" << endl << "Type:" << endl <<
		"1: Station" << endl << "2. AP" << endl << "3. Monitor" << endl << "? ";
	getline(cin, in);
	switch (in[0])
	{
		case '1':
			type = InterfaceType::Station;
			break;
		case '2':
			type = InterfaceType::Ap;
			break;
		case '3':
			type = InterfaceType::Monitor;
			break;
		default:
			cout << "Unknown value..." << endl;
			return;
	}
	cout << "Interface name? " << endl;
	getline(cin, iface);
	rv = im->SetInterfaceMode(iface.c_str(), type);
	cout << rv ? "SUCCESS" : "FAILED!";
	cout << endl;
}

int main(int argc, char* argv[])
{
	Log l;
	bool rv;
	IfIoctls ifIoctls;
/* This is now done by Nl80211IfaceMgr's Init():	
// TEMP test...

if (!ifIoctls.SetWirelessPowerSaveOff("wlan0"))
{
	cout << "main(): Set Power Save Off wlan0 failed, continuing..." << endl;
}
else
{
	cout << "main(): Set Power Save Off wlan0 success!" << endl;
}

if (!ifIoctls.SetWirelessPowerSaveOff("wlan1"))
{
	cout << "main(): Set Power Save Off wlan1 failed, continuing..." << endl;
}
else
{
	cout << "main(): Set Power Save Off wlan1 success!" << endl;
}
*/
	// Here we need to choose InterfaceManagerNl80211 or InterfaceManagerWext
	// (the latter is not implemented, so no real choice...)
	//   IInterfaceManager im;  <== A ptr in real use,
	//   this is a Singleton class; main instantiates
	// Need a GetInstance() function here.
	//InterfaceManagerNl80211 im("InterfaceManagerNl80211");
	//im.GetInterfaceList();
	InterfaceManagerNl80211 *im = InterfaceManagerNl80211::GetInstance();
	cout << "main(): calling Init()..." << endl;
	// Init() gets all current Wi-Fi interfaces into a list for us.
	// Init() also calls SetWirelessPowerSaveOff([name]) for every Wi-Fi
	// interface it finds.
	// IMPORTANT:
	// We MUST set power save mode off
	// on NEWLY ADDED interfaces AS WELL
	// else the TI wilink8 driver gets really hosed, kernel panic
	// when we try to bring interface up, somewhere here:
	// -- The crash relates to:
	//    wl1271_ps_elp_wakeup [wlcore]) from [<bf4456e8>] (wl1271_op_add_interface ...
	// Adding a new interface, MUST also call SetWirelessPowerSaveOff(["wlan1"])
	// BEFORE attempting to bring the interface UP.
	// (equiv cmd line: iwconfig [wlan0] power off
	// and then "iwconfig [wlan0]" will show "Power Management:off"
	
	rv = im->Init();
	if (!rv)
	{
		cout << "main(): Init() failed!" << endl;
		return 0;
	}
	// ifIoctls.BringInterfaceUp(const char *interfaceName) and BringInterfaceDown(const char *interfaceName) [bool]
	string in = "";
	bool quit = false;

	im->LogInterfaceList("Interfaces found:");
	do
	{
		// cout << endl << endl << "Options:" << endl << "1. List Interfaces" << endl <<
		//		"2. Add Interface" << endl << "3. Set Iface Mode" << endl <<
		//		"4. Set Power Save OFF" << endl << "5. Quit" << endl << "? ";

		cout << endl << endl << "Options:" << endl <<
			"1. List Interfaces" << endl <<
			"2. Add Interface" << endl <<
			"3. Set Iface Mode" << endl <<
			"4. Iface UP" << endl <<
			"5. Iface DOWN" << endl <<
			"6. Set Power Save OFF" << endl <<
			"7. Quit" << endl << "? ";
		getline(cin, in);
		switch (in[0])
		{
			case '1':  // List Interfaces
				im->GetInterfaceList();
				im->LogInterfaceList("Interfaces found:");
				break;
			case '2':  // Add Interface
				AddAnInterface(im);
				break;
			case '3':  // Set Iface Mode
				SetAnInterfacesMode(im);
				break;
			case '4':  // Iface UP
				BringIfaceUpOrDown(&ifIoctls, true);
				break;
			case '5':  // Iface DOWN
				BringIfaceUpOrDown(&ifIoctls, false);
				break;
			case '6':  // Set Power Save OFF.
				cout << "Set Power Save OFF: not done yet..." << endl;
				break;
			case '7':
				cout << "Quitting..." << endl;
				quit = true;
				break;
		}
	} while (!quit);
	cout << "Goodbye..." << endl << endl;
}
/***	
	wait("main(): Init() OK");
	cout << "main(): calling CreateInterfaces()..." << endl;
	rv = im->CreateInterfaces();
	if (!rv)
	{
		cout << "main(): CreateInterfaces() failed!" << endl;
		return 0;
	}
	cout << "main(): CreateInterfaces() success, complete!" << endl << endl;
}
****/
