INCLUDES = -I. -I$(top_srcdir) $(EXTRA_CFLAGS) \
	$(LIBTINYMAIL_OLPC_CFLAGS) \
	$(LIBTINYMAIL_CAMEL_CFLAGS) \
	-I$(top_srcdir)/libtinymail \
	-I$(top_srcdir)/libtinymailui \
	-I$(top_srcdir)/libtinymailui-gtk \
	-I$(top_srcdir)/libtinymail-camel \
	-I$(top_srcdir)/libtinymail-camel/camel-lite \
	-I$(top_srcdir)/libtinymail-gnomevfs 


lib_LTLIBRARIES = libtinymail-olpc-1.0.la

libtinymail_olpc_1_0_headers = \
	tny-olpc-account-store.h \
	tny-olpc-device.h \
	tny-olpc-platform-factory.h

libtinymail_olpc_1_0_la_SOURCES = \
	$(libtinymail_olpc_1_0_headers) \
	tny-olpc-account-store.c \
	tny-olpc-device-priv.h \
	tny-olpc-device.c \
	tny-olpc-platform-factory.c

libtinymail_olpc_1_0_la_LIBADD = \
	$(LIBTINYMAIL_OLPC_LIBS)\
	$(top_builddir)/libtinymail/libtinymail-$(API_VERSION).la \
	$(top_builddir)/libtinymailui/libtinymailui-$(API_VERSION).la \
	$(top_builddir)/libtinymailui-gtk/libtinymailui-gtk-$(API_VERSION).la \
	$(top_builddir)/libtinymail-camel/libtinymail-camel-$(API_VERSION).la

libtinymail_olpc_1_0_la_LDFLAGS = -export-dynamic \
	-version-info $(LT_VERSION_INFO) $(LIBTOOL_EXPORT_OPTIONS)

libtinymailolpcincludedir = $(includedir)/libtinymail-olpc-$(API_VERSION)
libtinymailolpcinclude_HEADERS = $(libtinymail_olpc_1_0_headers)

libtinymail-olpc-$(API_VERSION).pc: libtinymail-olpc.pc
	cp $< $@

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = libtinymail-olpc-$(API_VERSION).pc
EXTRA_DIST = $(pkgconfig_DATA:-$(API_VERSION).pc=.pc.in)

DISTCLEANFILES = $(pkgconfig_DATA)
