ACLOCAL_AMFLAGS = ${ACLOCAL_FLAGS} -I m4
AM_CXXFLAGS = -std=c++11 -g
# MY_LIBS   =-lm -lrt -ldl -lpcap -lcrypto -L $(TINYXML) -ltiny -lbluetooth 
# AM_LDFLAGS = -lprotobuf -ldl -lpcap -lssl -lcrypto -lrt -lbluetooth -lgps -lpthread
AM_LDFLAGS = -lnl-genl-3 -lnl-3
PKGLIBS=nl-3 \
    nl-genl-3
AUTOMAKE_OPTIONS = foreign
bin_PROGRAMS = nl80211test
# Not to BRAD: STOP USING CPPFLAGS...
# xxx_CPPFLAGS is *C* *P*re *P*rocessor flags (i.e. .c files)
# it is NOT for C-PlusPlus files!
#nl80211test_CPPFLAGS = -std=c++11
nl80211test_LDADD = -lnl-3 -lnl-genl-3
nl80211test_SOURCES = \
	main.cpp \
	ChannelSetterNl80211.cpp \
	InterfaceManagerNl80211.cpp \
	Nl80211InterfaceAdmin.cpp \
	Log.cpp \
	Nl80211Base.cpp \
	IfIoctls.cpp \
	HostapdManager.cpp \
	Terminator.cpp




