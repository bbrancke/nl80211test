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
#include <ctime>
#include <thread>

using namespace std;
using namespace chrono;

#include <sys/types.h>
#include <unistd.h>

#include "Nl80211Base.h"
#include "Log.h"
#include "IfIoctls.h"
#include "InterfaceManagerNl80211.h"
#include "Terminator.h"
#include "HostapdManager.h"
#include "ChannelSetterNl80211.h"
#include "TextColor.h"

void wait(const char *msg)
{
	cout << msg << ", wait 15..." << endl;
	this_thread::sleep_for(seconds(15));
}

void ShowResult(const char *who, bool rv)
{
	cout << endl << endl;
	cout << "main(): " << who << ":" << endl;
	if (rv)
	{
		_GREEN("******** Success. ********");   
	}
	else
	{
		_RED("****************** FAILED! ******************");
	}
	cout << endl << endl;
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
	ShowResult("Add Interface", rv);
	cout << "Note:" << endl;
	cout << "  For a NEW INTERFACE on the TI chip:" << endl;
	cout << "    1. Create it (just did)" << endl;
	cout << "    2. SET POWER SAVE OFF [VERY IMPORTANT!]" << endl;
	cout << "    3. Then Bring the Interface UP" << endl;
}

void BringIfaceUpOrDown(IfIoctls *ifIoctls, bool up)
{
	string in("");
	bool rv;
	cout << "=====================" << endl;
	cout << "Bring Interface ";
	if (up)
	{
		cout << "UP";
	}
	else
	{
		cout << "DOWN";
	}
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
	string s("Interface [");
	s += in;
	s += "]: ";
	if (up)
	{
		s += "UP";
	}
	else
	{
		s += "DOWN";
	}
	ShowResult(s.c_str(), rv);
}

void SetPowerSaveOff(IfIoctls *ifIoctls)
{
	string iface("");
	bool rv;

	cout << "===================" << endl;
	cout << "Set Power Save Off:" << endl << "Interface Name:" << endl << "? ";
	
	getline(cin, iface);
	rv = ifIoctls->SetWirelessPowerSaveOff(iface.c_str());
	ShowResult("Set Power Save OFF", rv);
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
	cout << "Calling nl80211 function. This can take up to forty seconds, please wait..." << endl;
	auto startTime = system_clock::now();
	rv = im->SetInterfaceMode(iface.c_str(), type);
	auto doneTime = system_clock::now();
	auto dur = doneTime - startTime;
	milliseconds ms = duration_cast<milliseconds>(dur);
	int mstot = ms.count();
	int seconds = mstot / 1000;
	mstot %= 1000;
	cout << "COMPLETED in " << seconds << "." << mstot << " seconds." << endl;
	ShowResult("Set Interface Mode", rv);
}


bool SetupInterfaces(IfIoctls *ifIoctls, InterfaceManagerNl80211 *im)
{
	string in("");
	bool quit = false;
	cout << endl <<
		"+=======================================================+" << endl;
	cout <<
		"|==================== Interface Setup ==================|" << endl;
	cout <<
		"+=======================================================+" << endl;
	bool shutdown = false;
	bool reloadInterfaces = false;
	do
	{
		if (reloadInterfaces)
		{
			im->GetInterfaceList();
			reloadInterfaces = false;
		}
		cout << "INFO: ================= Interfaces: =================" << endl;
		im->LogInterfaceList("Interfaces found:");

		cout << endl << endl << "Options:" << endl <<
			"1. List Interfaces" << endl <<
			"2. Add Interface" << endl <<
			"3. Set Iface Mode" << endl <<
			"4. Iface UP" << endl <<
			"5. Iface DOWN" << endl <<
			"6. Set Power Save OFF" << endl <<
			"7. Exit Interface Setup" << endl <<
			"8. Quit" << endl;
			"? ";
		getline(cin, in);
		switch (in[0])
		{
			case '1':  // List Interfaces
			case 'l':
				im->GetInterfaceList();
				im->LogInterfaceList("Interfaces found:");
				break;
			case '2':  // Add Interface
				AddAnInterface(im);
				reloadInterfaces = true;
				break;
			case '3':  // Set Iface Mode
				SetAnInterfacesMode(im);
				reloadInterfaces = true;
				break;
			case '4':  // Iface UP
				BringIfaceUpOrDown(ifIoctls, true);
				reloadInterfaces = true;
				break;
			case '5':  // Iface DOWN
				BringIfaceUpOrDown(ifIoctls, false);
				reloadInterfaces = true;
				break;
			case '6':  // Set Power Save OFF.
				SetPowerSaveOff(ifIoctls);
				break;
			case '7':
			case 'x':
				cout << "Exiting..." << endl;
				quit = true;
				break;
			case '8':
			case 'q':
				quit = true;
				shutdown = true;
				break;
		}
	} while (!quit);
	return shutdown;
}


bool RunTerminator()
{
	Terminator term;
	string in("");
	bool quit = false;
	cout << endl <<
		"+=======================================================+" << endl;
	cout <<
		"|====================== Terminator =====================|" << endl;
	cout <<
		"+=======================================================+" << endl;
	bool shutdown = false;
	bool rv;
	do
	{
		cout << endl << endl << "Options:" << endl <<
			"1. Kill hostapd(s)" << endl <<
			"2. Kill wpa_supplicant(s)" << endl <<
			"3. Kill process(es) by name" << endl <<
			"4. Exit Terminator" << endl <<
			"5. Quit" << endl;
			"? ";
		getline(cin, in);
		switch (in[0])
		{
			case '1':
			case 'h':
				rv = term.Terminate("hostapd", getpid());
				ShowResult("Killall hostapd: Terminate returned", rv);
				break;
			case '2':
			case 'w':
				rv = term.Terminate("wpa_supplicant", getpid());
				ShowResult("Killall wpa_supplicant: Terminate returned", rv);
				break;
			case '3':
			case 'k':
				cout << "Process Name? ";
				getline(cin, in);
				rv = term.Terminate(in.c_str(), getpid());
				ShowResult("Terminate returned", rv);
				break;
			case '4':
				quit = true;
				break;
			case '5':
			case 'q':
				quit = true;
				shutdown = true;
				break;
		}
	} while (!quit);
	return shutdown;
}

int ChannelChangeTest()
{
	int chan;
	ChannelSetterNl80211 cs;
	bool rv;
	
	string in("");

	cout << "Channel Change Test." <<
		"Hit Enter when ready to start..." << endl <<
		"? ";
	
	getline(cin, in);

	//rv = cs.OpenConnection2();
  rv = cs.OpenConnection(); 
	ShowResult("ChannelSetter Open()", rv);
	if (!rv)
	{
		return false;
	}
	
	for (chan = 1; chan <= 13; chan++)
	{
		cout << endl << "Setting to chan: " << chan << "..." << endl;
		//auto startTime = system_clock::now();
		time_point<system_clock>startTime = system_clock::now();
		// rv = cs.SetChannel2(chan);
    rv = cs.SetChannel(chan);
		if (!rv)
		{
			cout << "SetChannel() failed... Channel Change Test aborted." << endl << endl;
			return false;
		}
		auto doneTime = system_clock::now();
		auto dur = doneTime - startTime;
		milliseconds ms = duration_cast<milliseconds>(dur);
		int mstot = ms.count();
		int seconds = mstot / 1000;
		mstot %= 1000;
		time_t startTtp = system_clock::to_time_t(startTime);
		time_t doneTtp = system_clock::to_time_t(doneTime);
		cout << 
			"Channel set complete:" << endl <<
		 	"    Started: " << ctime(&startTtp) <<
		 	"  Completed: " << ctime(&doneTtp) <<
		 	"   Duration: " << seconds << "." << mstot << " seconds, sleep(4)..." << endl;
		this_thread::sleep_for(milliseconds(4000));
	}
	// cs.CloseConnection2();
  cs.CloseConnection();
	cout << "Channel Change Test complete..." << endl << endl;
	return false;
}

int main(int argc, char* argv[])
{
	Log l;
	bool rv;
	IfIoctls ifIoctls;
	HostapdManager apMgr;
	//   IInterfaceManager im;  <== A ptr in real use,
	//   this is a Singleton class; main instantiates
	_YELLOW("main(): **MUST** run this program as root (sudo)!");
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
	// MUCH LATER: I added a clause something like usb.powersave=off (?) to
	//   the Linux start-up command line [it is in kernel's menuconfig]
	//   and this doesn't seem to be a problem anymore, new interfaces
	//   show up in iwconfig as "Power Management:off"
	
	rv = im->Init();
	if (!rv)
	{
		cout << "main(): Init() failed!" << endl;
		return 0;
	}
	
	string in("");
	bool quit = false;
	// Three distinct parts at startup.
	// 1. Terminate instances of shadowx, hostapd, wpa_supplicant.
	// 2. Set up the interfaces.
	// 3. start hostapd (HostapdManager)
	do
	{
		cout << endl << endl <<
			"=========== Main Startup Options ============" << endl <<
			"1. Terminator" << endl <<
			"2. Setup Interfaces" << endl <<
			"3. Start Hostapd" << endl <<
			"4. Run Channel Change Test" << endl <<
			"5. Quit" << endl <<
			"? ";
		getline(cin, in);
		switch (in[0])
		{
			case '1':  // Terminator
			case 't':
				quit = RunTerminator();
				break;
			case '2':  // Setup Interfaces
				quit = SetupInterfaces(&ifIoctls, im);
				break;
			case '3':  // Start hostapd
				apMgr.StartHostapd();
				break;
			case '4':  // Channel Change Test
			case 'c':
				 quit = ChannelChangeTest();
				 break;
			case '5':
			case 'q':
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
