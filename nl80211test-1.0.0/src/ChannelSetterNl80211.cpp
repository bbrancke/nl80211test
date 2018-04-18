// ChannelSetterNl80211.cpp
// Implements Channel Set using Nl80211,
// replaces (now deprecated) Wireless Extensions (WEXT) IOCTLs.

#include "ChannelSetterNl80211.h"

ChannelSetterNl80211::ChannelSetterNl80211() : Nl80211Base("ChannelSetterNl80211")
{ }

bool ChannelSetterNl80211::OpenConnection()
{
	InterfaceManagerNl80211 *im;
	const char *interfaceName;
	if (!Open())
	{
		LogErr(AT, "Can't connect to NL80211.");
		return false;
	}
	im = InterfaceManagerNl80211::GetInstance();
	// Get Survey Interface Name from InterfaceManager (const char *):
	// Currently this ALWAYS "mon0" but this may change if re-creating
	// a troubled iface name does not succeed.
	interfaceName = im->GetMonitorInterfaceName();
cout << "Channel Setter using interface: " << interfaceName << endl;
	im->GetInterfaceIndex(interfaceName, m_interfaceIndex);
	return true;
}

uint32_t ChannelSetterNl80211::ChannelToFrequency(uint32_t channel)
{
	if (channel < 14)
	{
		return 2407 + channel * 5;
	}
	if (channel == 14)
	{
		return 2484;
	}
	return (channel + 1000) * 5;
}

bool ChannelSetterNl80211::SetChannel(uint32_t channel)
{
	uint32_t freq = ChannelToFrequency(channel);
	uint32_t htval = NL80211_CHAN_NO_HT;
/***
Shouldn't this use (newer):
516: * @NL80211_CMD_SET_CHANNEL: Set the channel (using %NL80211_ATTR_WIPHY_FREQ
 *	and the attributes determining channel width) the given interface
 *	(identifed by %NL80211_ATTR_IFINDEX) shall operate on.
160: * @NL80211_CMD_SET_WIPHY: set wiphy parameters, needs %NL80211_ATTR_WIPHY or
 *	%NL80211_ATTR_IFINDEX; can be used to set %NL80211_ATTR_WIPHY_NAME,
 *	%NL80211_ATTR_WIPHY_TXQ_PARAMS, %NL80211_ATTR_WIPHY_FREQ (and the
 *	attributes determining the channel width; this is used for setting
 *	monitor mode channel),  %NL80211_ATTR_WIPHY_RETRY_SHORT,
 *	%NL80211_ATTR_WIPHY_RETRY_LONG, %NL80211_ATTR_WIPHY_FRAG_THRESHOLD,
 *	and/or %NL80211_ATTR_WIPHY_RTS_THRESHOLD.
 *	However, for setting the channel, see %NL80211_CMD_SET_CHANNEL  <=== THIS
 *	instead, the support here is for backward compatibility only.
See /usr/include/linux/nl80211.h
****/
	if (!SetupMessage(0, NL80211_CMD_SET_WIPHY))
	{
		return false;
	}

	if (!AddMessageParameterU32(NL80211_ATTR_IFINDEX, m_interfaceIndex)
		||
		!AddMessageParameterU32(NL80211_ATTR_WIPHY_FREQ, freq)
		||
		!AddMessageParameterU32(NL80211_ATTR_WIPHY_CHANNEL_TYPE, htval))
	{
		LogErr(AT, "SetChannel() aborted, AddParam() failed.");
		return false;
	}

	return SendAndFreeMessage(false);
}

// This is how aircrack sets channel:
bool ChannelSetterNl80211::OpenConnection2()
{
    int err;
    struct nl80211_state *state = &m_state;  //(hack...)

    state->nl_sock = nl_socket_alloc();

    if (!state->nl_sock) {
        fprintf(stderr, "Failed to allocate netlink socket.\n");
//        return -ENOMEM;
        return false;
    }

    if (genl_connect(state->nl_sock)) {
        fprintf(stderr, "Failed to connect to generic netlink.\n");
        err = -ENOLINK;
        goto out_handle_destroy;
    }

    if (genl_ctrl_alloc_cache(state->nl_sock, &state->nl_cache)) {
        fprintf(stderr, "Failed to allocate generic netlink cache.\n");
        err = -ENOMEM;
        goto out_handle_destroy;
    }

    state->nl80211 = genl_ctrl_search_by_name(state->nl_cache, "nl80211");
    if (!state->nl80211) {
        fprintf(stderr, "nl80211 not found.\n");
        err = -ENOENT;
        goto out_cache_free;
    }

//    return 0;
    return true;

 out_cache_free:
    nl_cache_free(state->nl_cache);
 out_handle_destroy:
    nl_socket_free(state->nl_sock);
//    return err;
    cout << "Open2 FAILED" << endl;
    return false;
}


bool ChannelSetterNl80211::CloseConnection2()
{
	struct nl80211_state *state = &m_state;  //(hack...)
    genl_family_put(state->nl80211);
    nl_cache_free(state->nl_cache);
    nl_socket_free(state->nl_sock);
	return true;
}

bool ChannelSetterNl80211::SetChannel2(int channel)
{
    unsigned int devid;
    struct nl_msg *msg;
    unsigned int freq;
    int err;
    unsigned int htval = NL80211_CHAN_NO_HT;

/* libnl stuff */

    devid=if_nametoindex("wlan1");   //  HACK !!! wi->wi_interface);
//    freq=ieee80211_channel_to_frequency(channel);
    freq = (unsigned int) ChannelToFrequency((uint32_t) channel);
    msg=nlmsg_alloc();
    if (!msg) {
        fprintf(stderr, "failed to allocate netlink message\n");
//        return 2;
        return false;
    }
// 	genlmsg_put(m_msg, 0, 0, m_nl80211Id, 0, flags, cmd, 0);
    genlmsg_put(msg, 0, 0, genl_family_get_id(m_state.nl80211), 0,
            0, NL80211_CMD_SET_WIPHY, 0);

    NLA_PUT_U32(msg, NL80211_ATTR_IFINDEX, devid);
    NLA_PUT_U32(msg, NL80211_ATTR_WIPHY_FREQ, freq);
    NLA_PUT_U32(msg, NL80211_ATTR_WIPHY_CHANNEL_TYPE, htval);

    nl_send_auto_complete(m_state.nl_sock,msg);
    nlmsg_free(msg);

//    dev->channel = channel;

//    return( 0 );
    return true;

nla_put_failure:
    printf("PUT FAILURE!\n");
    return false;
//    return -ENOBUFS;
}


bool ChannelSetterNl80211::CloseConnection()
{
cout << "TODO: Close nl80211 Connection!!" << endl;
	return true;
}

ChannelSetterNl80211::~ChannelSetterNl80211()
{
}

