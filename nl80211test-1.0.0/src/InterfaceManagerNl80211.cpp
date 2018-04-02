// InterfaceManagerNl80211.cpp

#include "InterfaceManagerNl80211.h"

// Global static pointer used to ensure a single instance of the class:
InterfaceManagerNl80211* InterfaceManagerNl80211::m_pInstance = NULL; 

InterfaceManagerNl80211::InterfaceManagerNl80211() : Nl80211InterfaceAdmin("InterfaceManagerNl80211")
{
	// Random seed (for random MAC addresses):
	srand(time(nullptr));
}

InterfaceManagerNl80211* InterfaceManagerNl80211::GetInstance()
{
	if (m_pInstance == nullptr)
	{
		m_pInstance = new InterfaceManagerNl80211;
	}
	return m_pInstance;
}

// At startup, main():
//   Instantiates this (Singleton) for all to use.
//   Calls my Init().
//   (does some other startup stuff...)
//   Calls my CreateInterfaces()
//   At this point, main() can start hostapd on the newly created "ap0" interface.
// Init() gets the list of current Wifi interfaces. We must have exactly TWO "phy" ids,
//   one is the built-in TI chip we use for the AP and the STA (for alert emails).
//   The other phy Id is the Realtek USB that we use for Survey mode.
// Init() returns false if:
//     it can't get Interface List from nl80211 OR
//     number of 'Phy's is not two.
//     [At startup, it is possible to have multiple interfaces defined
//     but they map to a physical device, e.g. Phy #0 (built-in TI chip)
//     can have Virtual Interface (VIF) "ap0" for hostapd and VIF "sta0"
//     for email alerts (using wpa_supplicant to connect); "ap0" and "sta0"
//     BOTH "map" to Phy #0.]
//
// Init() gets all current Wi-Fi interfaces into a list for us.
// Init() also calls SetWirelessPowerSaveOff([name]) for every Wi-Fi
// interface it finds.
// IMPORTANT:
// We MUST set power save mode off
// on NEWLY ADDED interfaces AS WELL [in CreateInterfaces()]!
bool InterfaceManagerNl80211::Init()
{
	if (!GetInterfaceList())
	{
		LogErr(AT, "InterfaceManagerNl80211::Init() can't get Interface List");
		return false;
	}
	// Create a vector<(uint32_t)PhyId> from m_interfaces.
	// This should have a count of two when done, or return false (ERROR, # of physical devices NOT two).
	vector<uint32_t>phys;
	for (OneInterface* i : m_interfaces)
	{
		uint32_t phyId = i->phy;
		auto it = find(phys.begin(), phys.end(), phyId);
		if (it == phys.end())
		{
			// This phyId is NOT in the phys list yet, add it...
			phys.push_back(phyId);
		}
	}
	// Normally we have phy 0 is the (built-in) TI, phy 1 is Realtek.
	// If one re-sets itself or does weird things it can get a new Phy ID.
	if (phys.size() != 2)
	{
		stringstream ss;
		ss << "InterfaceManagerNl80211::Init(): Found " << phys.size() << " physical devices, expect TWO.";
		LogErr(AT, ss);
		return false;
	}
	// Set Power Management to OFF for all Wi-Fi interfaces.
	// (See dire warnings all around for what happens if we don't.)
	for (OneInterface* i : m_interfaces)
	{
		if (!m_ifIoctls.SetWirelessPowerSaveOff(i->name))
		{
			string s("InterfaceManagerNl80211.Init(): SetWirelessPowerSaveOff(");
			s += i->name;
			s += ") failed, continuing anyway.";
			LogErr(AT, s);
		}
	}
	// Fills m_tiInterfaces and m_notTiInterfaces (vectors)
	// These lists will be invalid once we add / change Interfaces...
	if (!CategorizeInterfaceList())
	{
		// FATAL, no TI chipset detected.
		LogErr(AT, "Init(): FATAL: No TI phy found, reboot required.");
		return false;
	}
	// Each Device should have only ONE Virtual Interface (VIF) at startup:
	// FOR NOW, require reboot if > 1 of either...
	if (m_tiInterfaces.size() != 1)
	{
		LogErr(AT, "Number of TI VIFs is not one, reboot required.");
		return false;
	}
	if (m_notTiInterfaces.size() != 1)
	{
		LogErr(AT, "Number of USB radio VIFs is not one, reboot required.");
		return false;
	}
	// This will be the Hostapd ap's interface name:
	OneInterface *oneIface = m_tiInterfaces[0];
	strncpy(m_apName, oneIface->name, SHX_IFNAMESIZE);
	// This will be the monitor/survey interface name:
	oneIface = m_notTiInterfaces[0];
	strncpy(m_monName, oneIface->name, SHX_IFNAMESIZE);
	LogInfo("InterfaceManager::Init() Complete, success. Results:");
	stringstream s;
	s << "AP interface name: [" << m_apName << "], Monitor interface name: [" << m_monName << "]";
	LogInfo(s);
	
	return true;
}

// Normally, m_interfaces has just two entries:
//   "wlan0" interface info and "wlan1" interface info, and
//   this will return a single interface: "wlan0" most of the time
//   (but might be *"wlan1"*).
// Fills m_tiInterfaces and m_notTiInterfaces (type: vector<OneInterface *>)
bool InterfaceManagerNl80211::CategorizeInterfaceList()
{
	bool found = false;
	for (OneInterface* i : m_interfaces)
	{
		// TI chip's MAC addres all start with these 3 bytes (the "OUI"):
		//     TiChipsetOui[3] = { 0xD0, 0xB5, 0xC2 };  // D0-B5-C2
		// We keep the OUI when randomizing the new interface's MACs.
		if (memcmp(m_TiChipsetOui, i->mac, 3) == 0)
		{
			found = true;
			m_tiInterfaces.push_back(i);
		}
		else
		{
			m_notTiInterfaces.push_back(i);
		}
	}
	return found;
}

bool InterfaceManagerNl80211::GetInterfaceByPhyAndName(uint32_t phyId,
	const char *name, OneInterface **iface)
{
	for (OneInterface* i : m_interfaces)
	{
		if (i->phy == phyId &&
			(strcmp(i->name, name) == 0))
		{
			*iface = i;
			return true;
		}	
		
	}
	return false;
}

// CreateInterfaces():
// At start-up: typically we have two entries in m_interfaces.
// Entries (type: OneInterface *) in m_interfaces are:
// OneInterface members: phy (uint32_t), name[IFNAMSIZE] (char), mac[6] (uint8_t)
//    The device:		Name:	phy	mac
//  Built-in TI chip	wlan0	0	D0:B5:C2:CB:90:CA
//  Realtek USB radio	wlan1	1	EC:F0:0E:67:79:0A
// If here, then Init() guaranteed we have two physical devices
// I have seen name and phy reversed (Realtek came up BEFORE the TI chip)
// and the name and phy can differ if something weird/bad happened before
// ShadowX started (or was re-started).
//
// When this method completes, we will have:
//	TI chip			apX		0	For hostapd --------+-- TWO Virtual
//					staY	0	For wpa_supplicant -+-- Interfaces
//													+-- on TI chip
//	Realtek USB		monZ	1	For survey / pcap
// Get Interface names from me -- MONITOR only!:
// FOR NOW, X, Y and Z are all 0 (ap0, sta0, mon0)
//   so we don't have to update hostapd.conf, wpa_supp.conf, etc.
//   I *think* this will be OK. May change if can't recreate
//     an interface name that prev. existed and had an error.
//
// IMPORTANT:
// 1:
// main() has a Terminator class and should ensure that hostapd (and
// wpa_supplicant*) are not running; main() should start hostapd
// AFTER this method creates interfaces. (*-wpa_supplicant is run
// only when sending alert emails, and is started and stopped by
// the email manager).
// 2:
// Original attempt did:
//   Action:									STATUS:
//   1. Delete all interfaces on the TI phy.	OK
//   2. Create new ap0 and sta0 on the TI phy.	OK
//   3. Assign random MAC addr to ap0 & sta0.	OK
//   4. Bring UP interfaces ap0 and sta0		ERROR!
// WTF?
// Step (1) causes the firmware to be completely unloaded when the last
//   interface on the TI phy is shut down.
// Step (4) errors "wlcore: Firmware load failed after three retries"
//   (something like that).
// If Bluetooth Audio is enabled: If pins BT_AUD(_IN?)_ENABLE*
//   *and* WIFI_ENABLE* are both enabled (step 4) then:
//   The damn wl-12xx chip goes into an unrecoverable "Test Mode"!!
//   -- Device NEVER recovers Wi-Fi until power cycle!
// Even worse: reboot does NOT work!
// The only way to get out of this is to HARD reset:
//   MUST cycle power on the device to get
//   Wi-Fi back! And: power has to be OFF for about one minute!
// I have seen a patch in the DTS file for BT_AUD_ENABLE* pins but only
//   for BeagleBone Black which has wl-18xx (ShadowX has wl-12xx chip)
// Conclusion: Do *NOT* bring down all interfaces on the TI phy,
//   it will never come back to life!
// *-Pin names are from memory and are probably not exactly right.
// There was a mistake in the DeviceTree file from Jumpnow
//    fixedregulator@2 { bt_enable .. }
//       was MISSING this line: "enable-active-high;"
//       (default is enable when LOW)
// so that may now be resolved, firmware now reloads when ifconfig down [unloads], then up reloads SUCCESSFULLY.
//
// *** EVEN MORE IMPORTANT ***:
// We MUST set power save mode off
// on NEWLY ADDED interfaces AS WELL before doing anything with them!
// Else the TI wilink8 driver gets really hosed, kernel panic
// when we try to bring interface up, somewhere here:
// -- The crash relates to:
//    wl1271_ps_elp_wakeup [wlcore]) from [<bf4456e8>] (wl1271_op_add_interface ...
//  wakeup ---------+----+-- after a (few ms) sleep.
// Adding a new interface, MUST also call SetWirelessPowerSaveOff(["wlan1"])
// BEFORE attempting to bring the interface UP.
// (equiv cmd line: iwconfig [wlan0] power off
// and then "iwconfig [wlan0]" will show "Power Management:off"

/***
NOTE ON POWER MANAGEMENT:
iwconfig wlan0  == the underlying phy (the TI chipset) we create sta0 from.
   Power Management:off

Then added sta0, crashes on BringInterfaceUP()

-- The crash relates to wakeup (sleeps for a few ms, or longer if not UP):
   wl1271_ps_elp_wakeup [wlcore]) from [<bf4456e8>] (wl1271_op_add_interface ...

BUT: iwconfig sta0 shows:
   Power Management:on

Adding a new interface does not "inherit" anything (it appears) from the
original VIF. 

SUCCESSFUL:
If I then do "iwconfig sta0 power off"
then I can bring sta0 up

root@duovero:~# ifconfig sta0 up
root@duovero:~#

now status is:
sta0      Link encap:Ethernet  HWaddr D0:B5:C2:CD:93:11
          UP BROADCAST MULTICAST  MTU:1500  Metric:1
 ...
iwconfig sta0 shows Power Management:off

tldr: Conclusion: Set Power Save Mode OFF before bringing the interface UP.
****/
bool InterfaceManagerNl80211::CreateInterfaces()
{
	OneInterface *oneIface;
	bool found;
	uint32_t phyId;
// For debug, show InterfaceList:
LogInterfaceList("CreateInterfaces Entry");

	// Get list of Interfaces now using the TI chip:
	// Need the phy ID for the TI chip, look for OUI (first 3 bytes)
	// of "D0-B5-C2"
	// If ShadowX is restarted, we longer have "wlan0" but should have
	// "sta0" and "ap0" whose MAC addresses begin with D0-B5-C2.
	
	oneIface = m_tiInterfaces[0];
	phyId = oneIface->phy;

	// Frank thinks that ALL the TI chips will have the SAME MAC address
	// across all ShadowX devices, so randomize the AP and STA interface
	// MAC addresses:
	uint8_t mac[6];
	memcpy(mac, m_TiChipsetOui, 3);
	mac[3] = rand() % 256;
	mac[4] = rand() % 256;
	mac[5] = rand() % 256;

	// Create and bring up 'sta0' interface first on the TI-phy.
	// 'sta0' will send alert emails for us.
	// Do we already have VIF "sta0"?
	if (GetInterfaceByPhyAndName(phyId, m_staName, &oneIface))
	{
		LogInfo("CreateInterfaces(): sta0 already exists, not creating.");
		// Exists, bring it down:
		if (!m_ifIoctls.BringInterfaceDown("sta0"))
		{
			LogErr(AT, "CreateInterfaces(): Can't bring Interface sta0 down.");
			return false;
		}
	}
	else
	{
		if (!CreateStationInterface(m_staName, phyId))
		{
			LogErr(AT, "CreateInterfaces(): Can't create Interface sta0!");
			return false;
		}
		// A NEW interface starts out DOWN.
	}
	// Set Power Save OFF on all newly created interfaces.
	// When here, the sta0 interface is created but not UP

	// ALL NEWLY CREATED INTERFACED:
	// Set Power Save Mode OFF before bringing the interface UP.

	// This particular software version of the TI wl-18xx crashes badly when
	// entering power save mode (power save is fractions of a second). When
	// power returns it has thrown away something critical (can't remember
	// at the moment) but the effect is that it brings the interface down,
	// unrecoverable error.
	// This is the programatic equiv of "iwconfig sta0 power off" --
	//   disable Power Save Mode.
	if (!m_ifIoctls.SetWirelessPowerSaveOff(m_staName))
	{
		LogErr(AT, "SetWirelessPowerSaveOff(sta0) failed, continuing anyway.");
	}
	else
	{
		LogInfo("Power Save Off on sta0.");
	}

	if (!m_ifIoctls.SetMacAddress(m_staName, mac, false))
	{
		LogErr(AT, "CreateInterfaces(): Can't set Interface sta0's MAC address, continuing...");
	}
	// Bring UP the interface:
	if (!m_ifIoctls.BringInterfaceUp(m_staName))
	{
		LogErr(AT, "CreateInterfaces(): Can't bring Interface sta0 up.");
		return false;
	}
	LogInfo("sta0 is created and UP.");

	// Do the same thing for"ap0", this is for ShadowX's access point.
	// Creating as AP is OK, but when BringIfaceUp("ap0") FAILS
	//    "Interface name is not unique" what ever that means...
	// m_apName is now "ap0"...
	if (GetInterfaceByPhyAndName(phyId, m_apName, &oneIface))
	{
		LogInfo("CreateInterfaces(): (ap) already exists, not creating.");
		if (!m_ifIoctls.BringInterfaceDown(m_apName))
		{
			LogErr(AT, "CreateInterfaces(): Can't bring Interface sta0 down.");
			return false;
		}
	}
	else
	{
		if (!CreateApInterface(m_apName, phyId))
		{
			LogErr(AT, "CreateInterfaces(): Can't create AP Interface ap0!");
			// NL80211 connection is closed and cleaned up at this point.
			return false;
		}
	}

	if (!m_ifIoctls.SetWirelessPowerSaveOff(m_apName))
	{
		LogErr(AT, "SetWirelessPowerSaveOff(ap) failed, continuing anyway.");
	}
	else
	{
		LogInfo("Power Save Off on (ap).");
	}

	// MAC addresses *must* be unique or else we get:
	//		# ifconfig sta0 up
	//		ifconfig: SIOCSIFFLAGS: Name not unique on network
	// so bump the final octet up one (or back to zero):
	mac[5]++;
	if (!m_ifIoctls.SetMacAddress(m_apName, mac, false))
	{
		LogErr(AT, "CreateInterfaces(): Can't set Interface (ap)'s MAC address, continuing...");
	}
	// Bring UP the ap0 interface:
	if (!m_ifIoctls.BringInterfaceUp(m_apName))
	{
		LogErr(AT, "CreateInterfaces(): Can't bring ap Interface up.");
		return false;
	}

	// Need to re-read InterfaceList here, it is no longer valid...
	if (!GetInterfaceList())
	{
		LogErr(AT, "CreateInterfaces(): Can't re-read Interface List (1).");
		return false;
	}
LogInterfaceList("CreateInterfaces: II");
	// We should have ap0, sta0, wlan[0] on this PHY, and ap0 and sta0 are UP.
	// The firmware should NOT unload when we remove interface 'wlan[0]'
	// Somehow I remember (earlier) that it DOES indeed unload right here.
	// Let us see if that is also fixed by the changes made...
	//
	// Delete all interfaces associated with this phy Id
	// EXCEPT sta0 and ap0:
	//  Delete..() and Create...() are in Nl80211InterfaceAdmin class
	//  and Open() / Close() the NL80211 connection for us automatically.
	for (OneInterface* i : m_interfaces)
	{
		if (i->phy == phyId &&
			(strcmp(i->name, m_apName) != 0) &&
			(strcmp(i->name, m_staName) != 0))
		{
			m_ifIoctls.BringInterfaceDown(i->name);
			if (!DeleteInterface(i->name))
			{
				string s("CreateInterfaces(): Can't delete iface [");
				s += i->name;
				s += "], continuing...";
				LogErr(AT, s);
			}
		}
	}

	// --------------------------------------------------------
	// Set up mon0 interface on the USB radio:
	uint32_t tiPhy = phyId;
	phyId = 1;
	found = false;
	// Get the OTHER Phy ID:
	for (OneInterface* i : m_interfaces)
	{
		if (i->phy != tiPhy)
		{
			found = true;
			phyId = i->phy;
			break;
		}
	}
	if (!found)
	{
		LogErr(AT, "CreateInterfaces(): Can't find USB Phy ID, assuming phyId = 1");
		// Continue anyway...
	}

	// Does mon0 already exist?
	if (GetInterfaceByPhyAndName(phyId, "mon0", &oneIface))
	{
		LogInfo("CreateInterfaces(): mon0 already exists, leaving alone.");
		if (!m_ifIoctls.BringInterfaceDown("mon0"))
		{
			LogErr(AT, "CreateInterfaces(): Can't bring Interface mon0 down.");
			return false;
		}
	}
	else
	{
		if (!CreateMonitorInterface("mon0", phyId))
		{
			LogErr(AT, "CreateInterfaces(): Can't create mon0 interface.");
			return false;
		}
	}

	if (!m_ifIoctls.SetWirelessPowerSaveOff("mon0"))
	{
		LogErr(AT, "SetWirelessPowerSaveOff(mon0) failed, continuing anyway.");
	}
	else
	{
		LogInfo("Power Save Off on mon0.");
	}
	if (!m_ifIoctls.BringInterfaceUp("mon0"))
	{
		LogErr(AT, "CreateInterfaces(): Can't bring mon0 up.");
		return false;
	}

	// Delete interfaces that are NOT mon0:
	for (OneInterface* i : m_interfaces)
	{
		if (i->phy == phyId &&
			(strcmp(i->name, "mon0") != 0))
		{
			m_ifIoctls.BringInterfaceDown(i->name);
			if (!DeleteInterface(i->name))
			{
				string s("CreateInterfaces(): Can't delete iface [");
				s += i->name;
				s += "], continuing...";
				LogErr(AT, s);
			}
		}
	}

	// Finally re-read the new interface list:
	if (!GetInterfaceList())
	{
		LogErr(AT, "CreateInterfaces(): Can't re-read Interface List (2).");
		return false;
	}
LogInterfaceList("CreateInterfaces: COMPLETE");
	return true;
}

const char *InterfaceManagerNl80211::GetMonitorInterfaceName()
{
	// Called by Survey's ChannelChange->ChannelSetter class.
	// This is the iface name of the USB radio. The Realtek
	// driver does not support Virtual Interfaces, it just
	// doesn't work. So this will be wlan1 or wlan0, whichever
	// it came up as.
	return m_monName;
}

const char *InterfaceManagerNl80211::GetApInterfaceName()
{
	return m_apName;
}

const char *InterfaceManagerNl80211::GetStaInterfaceName()
{
	return m_staName;
}

