// ChannelSetterNl80211.h
// Set Channel using NL80211 subsystem.
// Replaces (deprecated) Wireless Extensions (WEXT) IOCTLs.

#ifndef CHANNELSETTERNL80211_H_
#define CHANNELSETTERNL80211_H_

#include <iostream>
#include <string>
#include <sstream>

#include <stdint.h>

#include "Log.h"
#include "Nl80211Base.h"
#include "InterfaceManagerNl80211.h"
#include "IChannelSetter.h"

using namespace std;

struct nl80211_state {
    struct nl_sock *nl_sock;
    struct nl_cache *nl_cache;
    struct genl_family *nl80211;
};

class ChannelSetterNl80211 : public Nl80211Base
{
public:
	ChannelSetterNl80211();
	bool OpenConnection();
	bool SetChannel(uint32_t channel);

	// SetChannel2() is how aircrack sets channel:
	bool OpenConnection2();
	bool SetChannel2(int channel);
	bool CloseConnection2();

	bool CloseConnection();
	~ChannelSetterNl80211();
private:
	uint32_t ChannelToFrequency(uint32_t channel);
	uint32_t m_interfaceIndex;
	
	
	struct nl80211_state m_state;
};

#endif  // CHANNELSETTERNL80211_H_

