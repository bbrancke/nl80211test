// Nl80211InterfaceAdmin.cpp
// Includes convenience "translator" funcs between InterfaceManagerNl80211
// class and Nl80211Base class.

#include "Nl80211InterfaceAdmin.h"

Nl80211InterfaceAdmin::Nl80211InterfaceAdmin(const char *name) : Nl80211Base(name) { }

bool Nl80211InterfaceAdmin::GetInterfaceList()
{
	LogInfo("Get InterfaceList() entry.");

	ClearInterfaceList();
	if (!Open())
	{
		Close();
		LogErr(AT, "Nl80211 open failed.");
		return false;
	}
	if (!SetupCallback())
	{
		Close();
		LogErr(AT, "Nl80211 SetupCallback failed.");
		return false;
	}
	// flags: 0 or NLM_F_DUMP if repeating responses expected.
	// cmd:  NL80211_CMD_GET_INTERFACE - get List of interfaces
	//       NL80211_CMD_SET_WIPHY - set frequency
	if (!SetupMessage(NLM_F_DUMP, NL80211_CMD_GET_INTERFACE))
	{
		Close();
		LogErr(AT, "Nl80211 SetupMessage failed.");
		return false;
	}
	// Call this when expecting multiple responses [e.g., GetInterfaceList()]:
	if (!SendWithRepeatingResponses())
	{
		Close();
		LogErr(AT, "Nl80211 SendWithRepeatingResponses failed.");
		return false;
	}
	// All done, close everything:
	Close();
	LogInfo("GetInterfaceList() complete, success");
	return true;
}

/*********
enum nl80211_iftype {
	NL80211_IFTYPE_UNSPECIFIED,
	NL80211_IFTYPE_ADHOC,
	NL80211_IFTYPE_STATION,
	NL80211_IFTYPE_AP,
	NL80211_IFTYPE_AP_VLAN,
	NL80211_IFTYPE_WDS,
	NL80211_IFTYPE_MONITOR,
	NL80211_IFTYPE_MESH_POINT,

	// Last:
	__NL80211_IFTYPE_AFTER_LAST,
	NL80211_IFTYPE_MAX = __NL80211_IFTYPE_AFTER_LAST - 1
};
**********/
void Nl80211InterfaceAdmin::IfTypeToString(uint32_t iftype, string& strType)
{
	enum nl80211_iftype type = (enum nl80211_iftype)iftype;
	stringstream s;
	switch (type)
	{
		case nl80211_iftype::NL80211_IFTYPE_UNSPECIFIED:
			strType = "Unspecified ";
			break;
		case nl80211_iftype::NL80211_IFTYPE_ADHOC:
			strType = "Ad Hoc      ";
			break;
		case nl80211_iftype::NL80211_IFTYPE_STATION:
			strType = "Station     ";
			break;
		case nl80211_iftype::NL80211_IFTYPE_AP:
			strType = "Access Point";
			break;
		case nl80211_iftype::NL80211_IFTYPE_AP_VLAN:
			strType = "AP VLAN     ";
			break;
		case nl80211_iftype::NL80211_IFTYPE_WDS:
			strType = "WDS         ";
			break;
		case nl80211_iftype::NL80211_IFTYPE_MONITOR:
			strType = "Monitor     ";
			break;
		case nl80211_iftype::NL80211_IFTYPE_MESH_POINT:
			strType = "Mesh Point  ";
			break;
		default:
			s << "Unknown (" << iftype << ")";
			strType = s.str();
			break;
	}
}

void Nl80211InterfaceAdmin::LogInterfaceList(const char *caller)
{
	int j;
	IfIoctls ifIoctls;
	stringstream s;
	s << "Nl80211Base: " << caller << ":";
	LogInfo(s);
	stringstream s2;
	s2 << "Interface List has " << m_interfaces.size() << " elements:";
	LogInfo(s2);

	LogInfo("#\tName:\tPhy\tType        \tMAC");
	j = 0;
	for (OneInterface *i : m_interfaces)
	{
		stringstream info;
		string strIftype;
		char buf[32];
		uint8_t *p = i->mac;
		sprintf(buf, "%02x:%02x:%02x:%02x:%02x:%02x",
			p[0], p[1], p[2], p[3], p[4], p[5]);
		IfTypeToString(i->iftype, strIftype);
		
		info << j << "\t" << i->name << "\t" << i->phy
			<< "\t" << strIftype << "\t" << buf << "\t";
		int flags;
		bool isUp;
		bool isRunning;
		if (ifIoctls.GetInterfaceFlags(i->name, flags, isUp, isRunning))
		{
			if (isUp || isRunning)
			{
				if (isUp)
				{
					info << "UP ";
				}
				if (isRunning)
				{
					info << "RUNNING";
				}
			}
			else
			{
				info << "(down)";
			}
			
		}
		LogInfo(info);
		j++;
	}
	//       List # x   wlan0    0   xx:xx:xx:xx:xx:xx
	LogInfo("============ End of Interface List ============");
}

// NL80211_CMD_SET_INTERFACE: Set type of a virtual interface.
// Requires:
//    NL80211_ATTR_IFINDEX and
//    NL80211_ATTR_IFTYPE.
bool Nl80211InterfaceAdmin::SetInterfaceMode(const char *interfaceName, InterfaceType itype)
{
	unsigned int ifIndex;
	enum nl80211_iftype type;
	
	ifIndex = if_nametoindex(interfaceName);
	if (ifIndex == 0)
	{
		string s("SetInterfaceType(): Can't get if index for interface: [");
		s += interfaceName;
		s += "]";
		LogErr(AT, s);
		return false;
	}
	switch (itype)
	{
		case InterfaceType::Station:
			type = NL80211_IFTYPE_STATION;
			break;
		case InterfaceType::Ap:
			type = NL80211_IFTYPE_AP;
			break;
		case InterfaceType::Monitor:
			type = NL80211_IFTYPE_MONITOR;
			break;
		default:
			LogErr(AT, "SetInterfaceType(): Unknown Iface Type, aborting...");
			return false;
	}

	if (!Open())
	{
		LogErr(AT, "SetInterfaceType(): Can't connect to NL80211.");
		return false;
	}

	if (!SetupMessage(0, NL80211_CMD_SET_INTERFACE))
	{
		Close();
		LogErr(AT, "SetInterfaceType(): SetupMessage failed.");
		return false;
	}
	// NL80211_ATTR_IFINDEX, NL80211_ATTR_IFTYPE.
	if (!AddMessageParameterU32(NL80211_ATTR_IFINDEX, ifIndex)
		|| !AddMessageParameterU32(NL80211_ATTR_IFTYPE, type))
	{
		Close();
		// Detailed error already logged...
		LogErr(AT, "_SetInterfaceType(): AddParam() failed.");
		return false;
	}

	if (!SendAndFreeMessage())
	{
		Close();
		// Detailed error already logged...
		LogErr(AT, "SetInterfaceType(): Send...() failed.");
		return false;
	}

	Close();
	LogInfo("SetInterfaceType() complete, success");
}

// _createInterface(): private:
bool Nl80211InterfaceAdmin::_createInterface(const char *newInterfaceName, 
	uint32_t phyId, enum nl80211_iftype type)
{
	// "NL80211_CMD_NEW_INTERFACE: ... sent from userspace to request
	// creation of a new virtual interface, requires attributes:
	// NL80211_ATTR_WIPHY, NL80211_ATTR_IFTYPE and NL80211_ATTR_IFNAME."
	// ATTR_WIPHY is also known as 'phyId' in Nl80211Base
	// or OneInterface->phy (type: uint32_t). Its the physical Device Id.
	// Usually Phy #0 is the built-in TI chip, Phy #1 is USB radio.
	if (!Open())
	{
		LogErr(AT, "_createInterface(): Can't connect to NL80211.");
		return false;
	}

	if (!SetupMessage(0, NL80211_CMD_NEW_INTERFACE))
	{
		Close();
		LogErr(AT, "_createInterface(): SetupMessage failed.");
		return false;
	}

	if (!AddMessageParameterU32(NL80211_ATTR_WIPHY, phyId)
		|| !AddMessageParameterString(NL80211_ATTR_IFNAME, newInterfaceName)
		|| !AddMessageParameterU32(NL80211_ATTR_IFTYPE, type))
	{
		Close();
		// Detailed error already logged...
		LogErr(AT, "_createInterface(): AddParam() failed.");
		return false;
	}

	if (!SendAndFreeMessage())
	{
		Close();
		// Detailed error already logged...
		LogErr(AT, "_createInterface(): Send...() failed.");
		return false;
	}

	Close();
	string cs("_createInterface('");
	cs += newInterfaceName;
	cs += "') complete, success.";
	LogInfo(cs);

	return true;
}

// enum nl80211_iftype (of interest to us) are:
//   NL80211_IFTYPE_STATION,
//   NL80211_IFTYPE_AP,
//   NL80211_IFTYPE_MONITOR
// See: linux/nl80211.h

// Create [AP / STA / MON] Interface(): public
// example newInterfaceName: "ap0" - is 'interface' in hostapd.conf "interface=ap0"
bool Nl80211InterfaceAdmin::CreateApInterface(const char *newInterfaceName,
	uint32_t phyId)
{
	// '__ap' in "iw dev interface add xyz0 type __ap"
	// maps to type enum nl80211_iftype::NL80211_IFTYPE_AP
	return _createInterface(newInterfaceName, phyId, NL80211_IFTYPE_AP);
}

// example newInterfaceName: "sta0" - for wpa_supplicant (Alert e-mails on built-in TI chip)
bool Nl80211InterfaceAdmin::CreateStationInterface(const char *newInterfaceName,
	uint32_t phyId)
{
	return _createInterface(newInterfaceName, phyId, NL80211_IFTYPE_STATION);
}

// example newInterfaceName: "mon0" for Realtek USB radio (survey)
bool Nl80211InterfaceAdmin::CreateMonitorInterface(const char *newInterfaceName,
	uint32_t phyId)
{
	return _createInterface(newInterfaceName, phyId, NL80211_IFTYPE_MONITOR);
}

bool Nl80211InterfaceAdmin::DeleteInterface(const char *interfaceName)
{
	unsigned int ifIndex;
	
	ifIndex = if_nametoindex(interfaceName);
	if (ifIndex == 0)
	{
		string s("DeleteInterface(): Can't get if index for interface: [");
		s += interfaceName;
		s += "]";
		LogErr(AT, s);
		return false;
	}

	if (!Open())
	{
		LogErr(AT, "DeleteInterface(): Can't connect to NL80211.");
		return false;
	}

	if (!SetupMessage(0, NL80211_CMD_DEL_INTERFACE))
	{
		Close();
		LogErr(AT, "DeleteInterface(): SetupMessage failed.");
		return false;
	}
// NLA_PUT_U32(msg, NL80211_ATTR_IFINDEX, ifidx);
	if (!AddMessageParameterU32(NL80211_ATTR_IFINDEX, ifIndex))
	{
		Close();
		// Detailed error already logged...
		LogErr(AT, "DeleteInterface(): AddParam() failed.");
		return false;
	}

	if (!SendAndFreeMessage())
	{
		Close();
		// Detailed error already logged...
		LogErr(AT, "DeleteInterface(): Send...() failed.");
		return false;
	}

	Close();
	LogInfo("DeleteInterface() complete, success");

	return true;
}

