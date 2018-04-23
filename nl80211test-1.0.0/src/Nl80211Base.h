// Nl80211Base.h
// Base class sets up connection to nl80211 sub-system.

#ifndef NL80211BASE_H_
#define NL80211BASE_H_

#include <iostream>
#include <string>
#include <sstream>
#include <cstring>  // std::strerror()
#include <vector>
#include <cstdio>

#include "netlink/socket.h"
#include "netlink/netlink.h"
#include "netlink/genl/genl.h"
#include "netlink/genl/family.h"
#include "netlink/genl/ctrl.h"

#include "net/if.h"  // if_nametoindex (can __THROW); note: conflicts with linux/if.h

#include <linux/nl80211.h>

#include <stdint.h>

#include "OneInterface.h"
#include "Log.h"

using namespace std;

// fwd def:
class Nl80211Base;

// this is the "arg *" we're going to
// pass back to our nl80211 callback
typedef struct
{
	Nl80211Base* m_pInstance;
	// Set status to zero when complete:
	int status;
	// This is set by Error handler (if error occurred):
	int errcode;
} nl80211CallbackInfo;

class Nl80211Base : protected Log
{
public:
	Nl80211Base(const char* name);
	// The NL80211_CMD_GET_INTERFACE command has these callback functions:
	//    In C++, define "C-style" callback funcs as "static";
	//       then we don't need "extern C { ... }" around them...
	//       See: C-style callbacks in C++ at:
	// http://tanks4code.blogspot.com/2008/07/c-style-callbacks-in-c-code.html
	// On Complete:
	static int finish_handler(struct nl_msg *msg, void *arg);
	// An error occurred, set arg->errcode to (0 - err->error [a NEGATIVE int]);
	static int error_handler(struct sockaddr_nl *nla, struct nlmsgerr *err, void *arg);
	// On Command Complete:
	//   Here, we only see "finish"ed handler called. ack_handler isn't called
	//   (I think finish_handler()s ret val is sufficient)
	static int ack_handler(struct nl_msg *msg, void *arg);

	// 'arg' is the last parameter to the xxx call:
	// int nl_cb_set(struct nl_cb *cb, enum nl_cb_type type,
	//      enum nl_cb_kind kind, nl_recvmsg_msg_cb_t func,
	//      void *arg <== this guy);
	// Example:
	//     nl_cb_set(cb, NL_CB_VALID, NL_CB_CUSTOM, list_interface_handler, NULL);
	static int list_interface_handler(struct nl_msg *msg, void *arg);
	
	bool Close();
	bool FreeMessage();
	bool Open();
	bool GetInterfaceIndex(const char* ifaceName, uint32_t& deviceId);
	bool SetupCallback();
	// flags: 0 or NLM_F_DUMP if repeating responses expected.
	// cmd:  NL80211_CMD_GET_INTERFACE - get List of interfaces
	//       NL80211_CMD_SET_WIPHY - set frequency
	bool SetupMessage(int flags, uint8_t cmd);
	bool AddMessageParameterU32(enum nl80211_attrs parameterName, uint32_t value);
	bool AddMessageParameterString(enum nl80211_attrs parameterName, const char *value);
	// Call this when expecting multiple responses [e.g., GetInterfaceList()]:
	bool SendWithRepeatingResponses();
	// Send with no mult [e.g., SetChannel()]
	bool SendAndFreeMessage(bool waitForAck);
	void ClearInterfaceList();
	void AddInterfaceToList(uint32_t phyId, const char *interfaceName,
		int macLength, const uint8_t *macAddress,
		uint32_t interfaceType, uint32_t frequency);
protected:
	Nl80211Base() { }
	vector<OneInterface *> m_interfaces;
private:
	struct nl_sock *m_sock = nullptr;
	struct nl_msg *m_msg = nullptr;
	struct nl_cb *m_cb = nullptr;
	int32_t m_nl80211Id;
	nl80211CallbackInfo m_cbInfo;
};

#endif  // NL80211BASE_H_

