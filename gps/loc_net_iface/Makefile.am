AM_CFLAGS = \
     -I./ \
     -I./le \
     $(LOCPLA_CFLAGS) \
     $(LOCHAL_CFLAGS) \
     $(GPSUTILS_CFLAGS) \
     $(QMIF_CFLAGS) \
     $(QMI_CFLAGS) \
     $(DATA_CFLAGS) \
     $(DSUTILS_CFLAGS) \
     -I$(WORKSPACE)/gps/framework/native/lcp/data-items/inc

ACLOCAL_AMFLAGS = -I m4

AM_CPPFLAGS = -std=c++11

libloc_net_iface_la_SOURCES = \
     LocNetIfaceBase.cpp \
     LocNetIfaceAgps.cpp \
     le/LocNetIface.cpp

if USE_GLIB
libloc_net_iface_la_CFLAGS = -DUSE_GLIB $(AM_CFLAGS) @GLIB_CFLAGS@
libloc_net_iface_la_LDFLAGS = -lstdc++ -Wl,-z,defs -lpthread @GLIB_LIBS@ -shared -version-info 1:0:0
libloc_net_iface_la_LDFLAGS += -Wl,--export-dynamic
libloc_net_iface_la_CPPFLAGS = -DUSE_GLIB $(AM_CFLAGS) $(AM_CPPFLAGS) @GLIB_CFLAGS@
else
libloc_net_iface_la_CFLAGS = $(AM_CFLAGS)
libloc_net_iface_la_LDFLAGS = -lstdc++ -Wl,-z,defs -lpthread -shared -version-info 1:0:0
libloc_net_iface_la_LDFLAGS += -Wl,--export-dynamic
libloc_net_iface_la_CPPFLAGS = $(AM_CFLAGS) $(AM_CPPFLAGS)
endif

libloc_net_iface_la_LIBADD  = -lstdc++ $(QMIF_LIBS) -lqmiservices
libloc_net_iface_la_LIBADD += $(GPSUTILS_LIBS) $(LOCPLA_LIBS) $(LOCHAL_LIBS)
libloc_net_iface_la_LIBADD += -lqcmap_client -ldsi_netctrl -ldataitems

library_include_HEADERS = \
    LocNetIfaceBase.h \
    LocNetIfaceAgps.h \
    le/LocNetIface.h

#Create and Install libraries
lib_LTLIBRARIES = libloc_net_iface.la

library_includedir = $(pkgincludedir)
pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = loc-net-iface.pc
EXTRA_DIST = $(pkgconfig_DATA)
