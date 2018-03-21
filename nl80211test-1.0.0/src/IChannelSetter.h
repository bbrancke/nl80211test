// IChannelSetter.h
// Base class for ChannelSetterNl80211 and
// (FUTURE) ChannelSetterWext
// Use this instead of the raw IOCTLs (Wext)
// or Nl80211 Channel Set functions.
// This does not replace ChannelChanger class, it is
// to be owned / used by the ChannelChanger class.

#ifndef ICHANNELSETTER_H_
#define ICHANNELSETTER_H_

#include <string>

#include <stdint.h>

class IChannelSetter
{
public:
	IChannelSetter() {}
	virtual bool OpenConnection() = 0;
	virtual bool SetChannel(int32_t channel) = 0;
	virtual bool CloseConnection() = 0;
	virtual ~IChannelSetter() { }
};

#endif  // ICHANNELSETTER_H_

