// Nl80211Base.cpp

// Base class sets up connection to nl80211 sub-system.

#include "Nl80211Base.h"

Nl80211Base::Nl80211Base(const char* name) : Log(name)
{ }

int Nl80211Base::list_interface_handler(struct nl_msg *msg, void *arg)
{
	nl80211CallbackInfo* info;
	Nl80211Base* instance;
	struct genlmsghdr *gnlh;
	struct nlattr *tb_msg[NL80211_ATTR_MAX + 1];

	int len;
	uint32_t phyId;
	uint32_t interfaceType;
  uint32_t freq;
	const char *interfaceName;
	const uint8_t *macAddress;

	info = (nl80211CallbackInfo *)arg;
	instance = info->m_pInstance;  // arg->m_pInstance == [this *]

	gnlh = (genlmsghdr *)nlmsg_data(nlmsg_hdr(msg));
	// Parse 'msg' into the 'tb_msg' array:
	nla_parse(tb_msg, NL80211_ATTR_MAX, genlmsg_attrdata(gnlh, 0), genlmsg_attrlen(gnlh, 0), NULL);
	// Need these values to fill a new OneInterface:
	// One (or more) of these are not present in one interface on NeoPLUS
	// device. Missing was Interface name, the built-in interface has TWO
	// interfaces, ae:83:f3:47:42:a8 and ac:83:f3:47:42:a8 (*ae* vs *ac*)
	// Only the "ac" one was active; the ae one did not have Interface Name.
	// IGNORE if no Interface name, we can't do anything with it
	// (ifconfig [name] UP??) - there's no name!
	if (!tb_msg[NL80211_ATTR_IFNAME])
	{
		return NL_SKIP;
	}
	interfaceName = nla_get_string(tb_msg[NL80211_ATTR_IFNAME]);

	if (tb_msg[NL80211_ATTR_WIPHY])
	{
		phyId = nla_get_u32(tb_msg[NL80211_ATTR_WIPHY]);
	}
	else
	{
		instance->LogInfo("Interface missing attribute: PHY ID");
		phyId = 0;
	}
	if (tb_msg[NL80211_ATTR_MAC])
	{
		len = nla_len(tb_msg[NL80211_ATTR_MAC]);
		macAddress = (uint8_t *)nla_data(tb_msg[NL80211_ATTR_MAC]);
	}
	else
	{
		instance->LogInfo("Interface missing attribute: MAC address");
		len = 6;
		macAddress = (uint8_t *)"\x00\x00\x00\x00\x00\x00";
	}
	if (tb_msg[NL80211_ATTR_IFTYPE])
	{
		interfaceType = nla_get_u32(tb_msg[NL80211_ATTR_IFTYPE]);
	}
	else
	{
		instance->LogInfo("Interface missing attribute: Interface type");
		interfaceType = 0;
	}
  if (tb_msg[NL80211_ATTR_WIPHY_FREQ])
  {
     freq = nla_get_u32(tb_msg[NL80211_ATTR_WIPHY_FREQ]);
  }
  else
  {
    instance->LogInfo("Interface FREQ attr missing");
    freq = 0;
  }
	instance->AddInterfaceToList(phyId, interfaceName, len, macAddress, interfaceType, freq);

/******
	if (tb_msg[NL80211_ATTR_IFNAME]
		&& tb_msg[NL80211_ATTR_WIPHY]
		&& tb_msg[NL80211_ATTR_MAC]
		&& tb_msg[NL80211_ATTR_IFTYPE])
	{
		phyId = nla_get_u32(tb_msg[NL80211_ATTR_WIPHY]);
		interfaceType = nla_get_u32(tb_msg[NL80211_ATTR_IFTYPE]);
		interfaceName = nla_get_string(tb_msg[NL80211_ATTR_IFNAME]);
		len = nla_len(tb_msg[NL80211_ATTR_MAC]);
		macAddress = (uint8_t *)nla_data(tb_msg[NL80211_ATTR_MAC]);
		instance->AddInterfaceToList(phyId, interfaceName, len, macAddress, interfaceType);
	}
	else
	{
		instance->LogInfo("Nl80211: GET_INTERFACEs callback; MISSING iface param!");
	}
*****/
	return NL_SKIP;
}

int Nl80211Base::finish_handler(struct nl_msg *msg, void *arg)
{
	// (static)
	nl80211CallbackInfo* info = (nl80211CallbackInfo *)arg;
	info->status = 0;
	Nl80211Base* instance = info->m_pInstance;
	instance->LogInfo("finish_handler() called");
	// Caller checks:
	//   while (info.status > 0)
	//   {
	//       nl_recvmsgs(wifi.nls, cb);
	//   }
	return NL_SKIP;
}

int Nl80211Base::error_handler(struct sockaddr_nl *nla, struct nlmsgerr *err, void *arg)
{
	// (static)
	// An error occurred, set arg->errcode to (0 - err->error [a NEGATIVE int])
	nl80211CallbackInfo* info = (nl80211CallbackInfo *)arg;
	Nl80211Base* instance = info->m_pInstance;
	instance->LogInfo("error_handler() called");
	info->status = 0;
	info->errcode = (0 - err->error);  // convert to positive errno (for sterror(), etc)

	return NL_STOP;
}

int Nl80211Base::ack_handler(struct nl_msg *msg, void *arg)
{
	// (static). Complete.
	nl80211CallbackInfo* info = (nl80211CallbackInfo *)arg;
	Nl80211Base* instance = info->m_pInstance;
	instance->LogInfo("ack_handler() called");
	info->status = 0;

	return NL_STOP;
}

void Nl80211Base::ClearInterfaceList()
{
	for (OneInterface *pI : m_interfaces)
	{
		delete pI;
	}
	m_interfaces.clear();
	LogInfo("Nl80211: ClearInterfaceList()");
}

void Nl80211Base::AddInterfaceToList(uint32_t phyId, const char *interfaceName,
		int macLength, const uint8_t *macAddress, uint32_t interfaceType, uint32_t frequency)
{
	OneInterface *pI = new OneInterface(phyId, interfaceName, macAddress, 
    macLength, interfaceType, frequency);
	m_interfaces.push_back(pI);
}

bool Nl80211Base::FreeMessage()
{
	if (m_msg != nullptr)
	{
		nlmsg_free(m_msg);
		m_msg = nullptr;
	}
	return true;
}

bool Nl80211Base::Close()
{
	if (m_sock != nullptr)
	{
		nl_socket_free(m_sock);
		m_sock = nullptr;
	}
	if (m_cb != nullptr)
	{
		nl_cb_put(m_cb);
		m_cb = nullptr;
	}
	return FreeMessage();
}

bool Nl80211Base::Open()
{
	m_sock = nl_socket_alloc();
	if (m_sock == nullptr)
	{
		LogErr(AT, "Can't alloc netlink socket.");
		return false;
	}
	nl_socket_set_buffer_size(m_sock, 8192, 8192);
	if (genl_connect(m_sock))
	{
		LogErr(AT, "Can't connect to generic netlink.");
		Close();
		return false;
	}
// /usr/include/linux\nl80211.h:30:
// #define NL80211_GENL_NAME "nl80211"
	m_nl80211Id = genl_ctrl_resolve(m_sock, NL80211_GENL_NAME);
	if (m_nl80211Id < 0)
	{
		LogErr(AT, "nl80211: Not found.");
		Close();
		return false;
	}

	return true;
}

bool Nl80211Base::GetInterfaceIndex(const char* ifaceName, uint32_t& deviceId)
{
	try
	{
		deviceId = if_nametoindex(ifaceName);
	}
	catch(...)
	{
		deviceId = 0;
	}
	if (deviceId == 0)
	{
		string s("Can't get Device Id for: [");
		s += ifaceName;
		s += "]";
		LogErr(AT, s);
		return false;
	}
	// else OK...
	return true;
}

bool Nl80211Base::SetupCallback()
{
	m_cb = nl_cb_alloc(NL_CB_DEFAULT);
	if (m_cb == nullptr)
	{
		LogErr(AT, "Can't allocate netlink callback.");
		return false;
	}

	m_cbInfo.m_pInstance = this;
	m_cbInfo.status = 1;
	m_cbInfo.errcode = 0;

	nl_cb_set(m_cb, NL_CB_VALID, NL_CB_CUSTOM, list_interface_handler, &m_cbInfo);
	return true;
}

// flags: 0 or NLM_F_DUMP if repeating responses expected.
// cmd:  NL80211_CMD_GET_INTERFACE - get List of interfaces
//       NL80211_CMD_SET_WIPHY - set frequency
bool Nl80211Base::SetupMessage(int flags, uint8_t cmd)
{
	m_msg = nlmsg_alloc();
	if (m_msg == nullptr)
	{
		LogErr(AT, "Can't allocate NL message");
		return false;
	}

	// Add Generic Netlink headers to Netlink message
	// void *genlmsg_put(struct nl_msg *msg, uint32_t port, uint32_t seq,
	//     int family, int hdrlen, int flags, uint8_t cmd, uint8_t version)
	//
	// Here is msg setup for SET FREQ:
	// genlmsg_put(msg, 0, 0, genl_family_get_id(state.nl80211), 0,
	//       0, NL80211_CMD_SET_WIPHY, 0);
	
 	// Get list of interfaces:
	//genlmsg_put(m_msg, 0, 0, m_nl80211Id, 0,
	//	NLM_F_DUMP, NL80211_CMD_GET_INTERFACE, 0);
	// Note: params 2 and 3 (0, 0) are
	// NL_AUTO_PID and NL_AUTO_SEQ, which are both defined as zero
	// For use with nl_send_auto(...) [ My: SendAndFreeMessage() ]

	genlmsg_put(m_msg, 0, 0, m_nl80211Id, 0, flags, cmd, 0);
	
	return true;
}

// Add (unsigned) 32-bit parameter to the message.
// Used, e.g., "NLA_PUT_U32(msg, NL80211_ATTR_IFINDEX, devid);" ==>
//    "AddMessageParameterU32(NL80211_ATTR_IFINDEX, deviceId);"
bool Nl80211Base::AddMessageParameterU32(enum nl80211_attrs parameterName, uint32_t value)
{
	NLA_PUT_U32(m_msg, parameterName, value);
	return true;
nla_put_failure:
	LogErr(AT, "Can't Add Parameter");
	return false;
}

bool Nl80211Base::AddMessageParameterString(enum nl80211_attrs parameterName, const char *value)
{
	NLA_PUT_STRING(m_msg, parameterName, value);
	return true;
nla_put_failure:
	LogErr(AT, "Can't Add Parameter");
	return false;
}

bool Nl80211Base::SendWithRepeatingResponses()
{
	// "nl_send_auto_complete: DEPRECATED, please use nl_send_auto()"
//	nl_send_auto_complete(m_sock, m_msg);
	nl_send_auto(m_sock, m_msg);
	// m_cbInfo.status = 1;
	// m_cb (an nl_cb *) can hold > 1 callback, calls finish_handler()
	//   when FINISHed, and list_interface_handler() foreach interface
	nl_cb_set(m_cb, NL_CB_FINISH, NL_CB_CUSTOM, finish_handler, &m_cbInfo);
	nl_cb_err(m_cb, NL_CB_CUSTOM, error_handler, &m_cbInfo);
	nl_cb_set(m_cb, NL_CB_ACK, NL_CB_CUSTOM, ack_handler, &m_cbInfo);
	// finish_handler() method sets m_cbInfo->status to zero when complete.
	// [So does error_handler() and ack_handler()}
	while (m_cbInfo.status > 0)
	{
		nl_recvmsgs(m_sock, m_cb);
	}
	if (m_cbInfo.errcode != 0)
	{
		// error_handler() was called!
		string s("Nl80211:SendWithRepeatingResp() ERROR: ");
		s += strerror(m_cbInfo.errcode);
		LogErr(AT, s);
		return false;
	}
	// else OK:
	return true;
}

// SetChannel() and Create[/Delete]VirtualInterface() will
//   call this method to Send m_msg to nl80211,
//   checks for error returned from nl80211.
bool Nl80211Base::SendAndFreeMessage(bool waitForAck)
{
	int rv;
	// "nl_send_auto_complete: DEPRECATED, please use nl_send_auto()"
	// int nl_send_auto(struct nl_sock *sk, struct nl_msg *msg)
	// SAME parameters, returns # of bytes sent OR negative error code
	//   nl_send_auto_complete(m_sock, m_msg);  DEPRECATED...
	rv = nl_send_auto(m_sock, m_msg);
	if (rv < 0)
	{
		rv = 0 - rv;  // ==> positive error #.
		stringstream s;
		s << "Nl80211Base::SendAndFreeMessage: send_auto FAILED: ";
		s << strerror(rv);
		LogErr(AT, s);
		FreeMessage();
		return false;
	}
// aircrack-ng does NOT wait for ACK between channel change.
// but rv is always > 0, even though the call FAILS.
// It is sufficient to check wait_for_ack's ret val, so...
// Added 'waitForAck' param..
	if (waitForAck)
	{
		// Wait for ACK from nl80211:
		rv = nl_wait_for_ack(m_sock);  // 0: Succcess, <0: Error (w/errno)
		if (rv < 0)
		{
			rv = 0 - rv;  // ==> positive error #.
			// e.g. Unknown Interface name (EINVAL); multitude of possibilities...
			stringstream s;
			s << "Nl80211Base::SendAndFreeMessage: wait_for_ack returned ERROR: ";
			s << strerror(rv);
			LogErr(AT, s);
			FreeMessage();
			return false;
		}
	}

	return FreeMessage();
}

