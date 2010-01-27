INCLUDES = -I. -I$(top_srcdir) $(EXTRA_CFLAGS) \
	$(LIBTINYMAIL_GNOME_DESKTOP_CFLAGS) \
	$(LIBTINYMAIL_CAMEL_CFLAGS) \
	-I$(top_srcdir)/libtinymail \
	-I$(top_srcdir)/libtinymailui \
	-I$(top_srcdir)/libtinymailui-gtk \
	-I$(top_srcdir)/libtinymail-camel \
	-I$(top_srcdir)/libtinymail-camel/camel-lite \
	-I$(top_srcdir)/libtinymail-gnomevfs 

if BUILD_MOZEMBED
INCLUDES += -I$(top_srcdir)/libtinymailui-mozembed -DMOZEMBED
endif

if BUILD_GTKHTML
INCLUDES += -I$(top_srcdir)/libtinymailui-gtkhtml -DGTKHTML
endif

if BUILD_GNOME
INCLUDES += -I$(top_srcdir)/libtinymailui-gnome-keyring -DGNOME
endif

lib_LTLIBRARIES = libtinymail-gnome-desktop-1.0.la

libtinymail_gnome_desktop_1_0_headers = \
	tny-gnome-account-store.h \
	tny-gnome-device.h \
	tny-gnome-platform-factory.h

libtinymail_gnome_desktop_1_0_la_SOURCES = \
	$(libtinymail_gnome_desktop_1_0_headers) \
	tny-gnome-account-store.c \
	tny-gnome-device-priv.h \
	tny-gnome-device.c \
	tny-gnome-platform-factory.c

libtinymail_gnome_desktop_1_0_la_LIBADD = \
	$(LIBTINYMAIL_GNOME_DESKTOP_LIBS) \
	$(top_builddir)/libtinymail/libtinymail-$(API_VERSION).la \
	$(top_builddir)/libtinymailui/libtinymailui-$(API_VERSION).la \
	$(top_builddir)/libtinymailui-gtk/libtinymailui-gtk-$(API_VERSION).la \
	$(top_builddir)/libtinymail-camel/libtinymail-camel-$(API_VERSION).la

libtinymail_gnome_desktop_1_0_la_LDFLAGS = -export-dynamic \
	-version-info $(LT_VERSION_INFO) $(LIBTOOL_EXPORT_OPTIONS)

if BUILD_GNOME
libtinymail_gnome_desktop_1_0_la_LIBADD += $(top_builddir)/libtinymailui-gnome-keyring/libtinymailui-gnome-keyring.la
endif

if BUILD_MOZEMBED
libtinymail_gnome_desktop_1_0_la_LIBADD += $(top_builddir)/libtinymailui-mozembed/libtinymailui-mozembed-$(API_VERSION).la
endif

if BUILD_GTKHTML
libtinymail_gnome_desktop_1_0_la_LIBADD += $(top_builddir)/libtinymailui-gtkhtml/libtinymailui-gtkhtml-$(API_VERSION).la
endif

libtinymailgnomedesktopincludedir = $(includedir)/libtinymail-gnome-desktop-$(API_VERSION)
libtinymailgnomedesktopinclude_HEADERS = $(libtinymail_gnome_desktop_1_0_headers)

libtinymail-gnome-desktop-$(API_VERSION).pc: libtinymail-gnome-desktop.pc
	cp $< $@

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = libtinymail-gnome-desktop-$(API_VERSION).pc
EXTRA_DIST = $(pkgconfig_DATA:-$(API_VERSION).pc=.pc.in)

DISTCLEANFILES = $(pkgconfig_DATA)
