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
// Update for NanoPi-NeoPlus2:
//   The built-in Broadcom interface does NOT support
//      Virtual Interfaces [VIF]s.
//   The built-in interface appears to ALWAYS be wlan0.
//   This still gets the Interface List from nl80211
//      for HostapdManager to start the Access Point.
//      We create a VIF to send emails via an outside AP
//      it is usually auto-named wlx000e8e719b18; this
//      saves that iface name for EmailSender to use
//      when configured to send emails (egress) over AP.
//      Ethernet port is always named eth0.
//   After CreateInterfaces() is called, main() can
//     use HostapdManager to start hostapd.
// Init() gets the list of current Wifi interfaces. We must have exactly TWO "phy" ids,
//   one is the built-in chip we use for the AP.
//   The other phy Id is the Ralink USB that we use for Survey mode.
//   and the STA (for alert emails).
// NOTE: Built in chipset does not support VIFs; neither does the new
//   (80211ac) REALTEK USB chip that is proposes as a replacement for Ralink.
//   Will have to see if there is an updated driver OR shutdown
//   either the AP or Survey interface and reconfigure that one when
//   it comes time to send alert emails via an Acess Point.
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
LogInterfaceList("Init() interfaces found");
	
	// Create a vector<(uint32_t)PhyId> from m_interfaces.
	// This should have a count of two when done,
	// (means we have two physical devices)
	// or return false (ERROR, # of physical devices NOT two).
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
		if (strictPhyCountCheck)
		{
			LogErr(AT, ss);
			return false;
		}
		// (else... Is OK, for developing. Note it and try to continue
		LogInfo(ss);
	}
	
	// Set Power Management to OFF for all Wi-Fi interfaces.
	// (See dire warnings all around for what happens if we don't.)
	// If an Interface is already UP, then this fails.
	// LATER: Changes to kernel setup (Power Mgmt disabled)
	//   make this not as important.
	for (OneInterface* i : m_interfaces)
	{
		// main() has killed any apps (wpa_supplicant, hostapd, etc.)
		// Bring all the wireless interfaces DOWN. hostapd brings
		// its interface up automatically in AP mode.
		if (!m_ifIoctls.BringInterfaceDown(i->name))
		{
			string s("InterfaceManagerNl80211.Init(): BringIface DOWN(");
			s += i->name;
			s += ") failed, continuing anyway.";
			LogErr(AT, s);
		}
		if (!m_ifIoctls.SetWirelessPowerSaveOff(i->name))
		{
			string s("InterfaceManagerNl80211.Init(): SetWirelessPowerSaveOff(");
			s += i->name;
			s += ") failed, continuing anyway.";
			LogErr(AT, s);
		}
	}
	// Fills m_builtinInterfaces and m_externalInterfaces (vectors)
	// These lists will be invalid once we add / change Interfaces...
	if (!CategorizeInterfaceList())
	{
		// FATAL, no built-in chipset detected.
		LogErr(AT, "Init(): FATAL: No built-in phy found, reboot required.");
		// LogInterfaceList("Interfaces Detected");  Already showed at top...
		return false;
	}
	// Each Device should have only ONE Virtual Interface (VIF) at startup:
	// FOR NOW, require reboot if > 1 of either...
	// This guarantees we can use m_xxxInterfaces[0] below for monName and apName
	bool retVal = true;
	OneInterface *oneIface;
	if (m_builtinInterfaces.size() == 1)
	{
		// This will be the Hostapd ap's interface name:
		oneIface = m_builtinInterfaces[0];
		strncpy(m_apName, oneIface->name, SHX_IFNAMESIZE);
	}
	else
	{
		LogErr(AT, "Number of Built-in VIFs is not one, reboot required.");
		strcpy(m_apName, "UNK");
		retVal = false;
	}

	if (m_externalInterfaces.size() == 1)
	{
		// This will be the monitor/survey interface name:
		oneIface = m_externalInterfaces[0];
		strncpy(m_monName, oneIface->name, SHX_IFNAMESIZE);
	}
	else
	{
		LogErr(AT, "Number of USB radio VIFs is not one, reboot required.");
		strcpy(m_monName, "UNK");
		retVal = false;
	}

	// CreateInterfaces() [below] creates the Virtual Interface [VIF]
	// that we will use for sending alert emails when configured
	// to use an external Access Point.
	LogInfo("InterfaceManager::Init() Complete. Results:");

	stringstream s;
	s << "AP interface name: [" << m_apName << "], Monitor interface name: [" << m_monName << "]";
	LogInfo(s);
	
	return retVal;
}

// Normally, m_interfaces has just two entries:
//   "wlan0" interface info and "wlan1" interface info, and
//   this will return a single interface: "wlan0" most of the time
//   (but might be *"wlan1"*).
// Fills m_builtinInterfaces and m_externalInterfaces (type: vector<OneInterface *>)
bool InterfaceManagerNl80211::CategorizeInterfaceList()
{
	bool found = false;
	for (OneInterface* i : m_interfaces)
	{
		// TI chip's MAC addres all start with these 3 bytes (the "OUI"):
		//     TiChipsetOui[3] = { 0xD0, 0xB5, 0xC2 };  // D0-B5-C2
		// (new: now using generic "m_builtinWifiChipOui, is ac-83-f3)
		// We keep the OUI when randomizing the new interface's MACs.
		if (memcmp(m_builtinWifiChipOui, i->mac, 3) == 0)
		{
			found = true;
			m_builtinInterfaces.push_back(i);
		}
		else
		{
			m_externalInterfaces.push_back(i);
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
//  Built-in chip	  wlan0	0	  [TI: D0:B5:C2:CB:90:CA, Bcom(NeoPi): ac:83:f3:47:42:a8]
//  Realtek USB radio	wlan1	1	EC:F0:0E:67:79:0A  -- or: -- 
//  Ralink USB radio wlan1 1  00:0e:8e:71:9b:1f
// If here, then Init() guaranteed we have two physical devices
// I have seen name and phy reversed (Realtek came up BEFORE the TI chip)
// and the name and phy can differ if something weird/bad happened before
// ShadowX started (or was re-started).
//
// We need:
//   The built-in Wi-Fi chipset for the ShadowX Access Point, and
//   the USB radio to be in monitor mode for PCAP captures and
//   a "Station" mode VIF for sending alert emails.
// NanoPi-Neo Plus2:
//  Built-in Wi-Fi will not handle second VIF
//  so we add it to the USB radio (RALINK).
//  Note that (current) Realtek 80211ac driver is great
//  for monitoring but doesn't support a second VIF!
//  There we have to get creative, such as closing down
//  the AP, switching to STA mode, send emails and restart the AP.
//  Or maybe by then the REALTEK driver
//    will be updated to properly handle VIFs.

// HostApdManager, WpaSuppManager, EmailSender, Survey/DF monitor mode:
// ALL MUST GET Interface names from me.
//
// IMPORTANT:
// 1:
// main() has a Terminator class and should ensure that hostapd (and
// wpa_supplicant*) are not running; main() should start hostapd
// AFTER this method creates interfaces. [*-wpa_supplicant is run
// only when sending alert emails, and is started and stopped by
// the email manager, that should use WpaSupplicantManager, which
// in turn calls my 'GetWpaSupplicantIfaceName()'].

// LATER: I don't think #2 happens on the new NEO PLUS2 platform
//   but keeping this in case we see ever see this again.
//   *** I think it is still a good idea to turn power save mode OFF
//   *** on all interfaces, and always keep at least one in the list.
// *** NeoPlus2: We use the assigned power-up names and just create
//     ONE new STA interface on the USB radio's PHY ID.
// You can skip reading #2 if you don't have weird firmware load errors.
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
	// 4/18/2018. REMOVED RANDOMIZE MAC ADDRS, only create one new interface
	// on the USB (Ralink) radio; use that new name (our choice for name is
	// OVER-RIDDEN by the driver; we have to find the name once it is added).
	// Look at the commit for 4/18/2018 to see how it used to create VIF
	//   on the TI interface and randomized MAC addrs.
	//
	//
	// NanoPi-Neo Plus2: create STA VIF on the ralink radio
	// Init() has already conveniently broken all wireless interfaces
	// in m_interfaces list into m_builtinInterfaces and m_externalInterfaces.
	if (m_externalInterfaces.size() < 1)
	{
		strcpy(m_wpaName, "UNK");
		LogErr(AT, "CreateInterfaces(): No USB radio detected, can't create wpa iface");
		return false;
	}
	oneIface = m_externalInterfaces[0];
	// Create new wpa supplicant interface on USB radio's phy:
	phyId = oneIface->phy;
	// Make a copy of the current Interface List:
	// The driver ignores our proposed name for a new Virtual Interface
	//   so we have to deduce what it assigned by re-reading interface list.
	vector<string> origIfaces;
	for (OneInterface *i : m_interfaces)
	{
		origIfaces.push_back(i->name);
	}
	if (!CreateStationInterface("wpa0", phyId))
	{
		LogErr(AT, "Couldn't create wpa_supplicant interface");
		return false;
	}
	
	// Re-get the interface list:
	// It might take a fraction of a second or so for the new interface
	// to notify everbody that it exists, so try a few times to see it.
	found = false;
	int i = 0;
	do
	{
		if (!GetInterfaceList())
		{
			LogErr(AT, "CreateInterfaces(): Can't re-read Interface List (1).");
			return false;
		}
		if (m_interfaces.size() != origIfaces.size())
		{
			found = true;
		}
		else
		{
			i++;
			if (i > 20)
			{
				// Five seconds and the new interface still doesn't exist!
				LogErr(AT, "New STA interface has not appeared after 5 seconds.");
				return false;
			}
			// Wait a little bit and re-try...
			this_thread::sleep_for(milliseconds(250));
		}
	} while (!found);
	LogInterfaceList("CreateInterfaces Part II");
	// Find the new interface in m_interfaces list:
	for (OneInterface *i : m_interfaces)
	{
		string name = i->name;
		auto it = find(origIfaces.begin(), origIfaces.end(), name);
		if (it == origIfaces.end())
		{
			// This interface is NEW...
			// It is usually a weird name like "wlx000e8e719b18"
			strncpy(m_wpaName, i->name, 16);
		}
	}
	string info("wpa_supplicant should use interface [");
	info += m_wpaName;
	info += "]";
	LogInfo(info);
	if (!m_ifIoctls.SetWirelessPowerSaveOff((const char *)m_wpaName))
	{
		LogErr(AT, "SetWirelessPowerSaveOff(wpa iface) failed, continuing anyway.");
	}
	else
	{
		LogInfo("Power Save Off on wpa iface.");
	}
	// Now we have:
	//  - The wpa_supplicant interface set up (it is still down)
	//     Its name is set
	//     [WpaSupplicantManager will call my GetWpaSupplicantInterfaceName()]
	//  - The hostapd interface name is set for HostApdManager
	//  - The monitor interface name is set.
	// Put the monitor interface into monitor mode, and we're done:
	// bool SetInterfaceMode(const char *interfaceName, InterfaceType itype);
	// itype: InterfaceType::Station, ::Ap, ::Monitor
	if (!SetInterfaceMode((const char *)m_monName, InterfaceType::Monitor))
	{
		LogErr(AT, "Can't set mon interface to MONITOR mode");
		return false;
	}
	// We're not setting AP's MAC address or anything else FOR NOW.
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

const char *InterfaceManagerNl80211::GetWpaSupplicantInterfaceName()
{
	return m_wpaName;
}

