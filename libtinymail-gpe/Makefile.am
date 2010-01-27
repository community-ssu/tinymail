INCLUDES = -I. -I$(top_srcdir) $(EXTRA_CFLAGS) \
	$(LIBTINYMAIL_GPE_CFLAGS) \
	$(LIBTINYMAIL_CAMEL_CFLAGS) \
	-I$(top_srcdir)/libtinymail \
	-I$(top_srcdir)/libtinymailui \
	-I$(top_srcdir)/libtinymailui-gtk \
	-I$(top_srcdir)/libtinymail-camel \
	-I$(top_srcdir)/libtinymail-camel/camel-lite \
	-I$(top_srcdir)/libtinymail-gnomevfs 


lib_LTLIBRARIES = libtinymail-gpe-1.0.la

libtinymail_gpe_1_0_headers = \
	tny-gpe-account-store.h \
	tny-gpe-device.h \
	tny-gpe-platform-factory.h

libtinymail_gpe_1_0_la_SOURCES = \
	$(libtinymail_gpe_1_0_headers) \
	tny-gpe-account-store.c \
	tny-gpe-device-priv.h \
	tny-gpe-device.c \
	tny-gpe-platform-factory.c

libtinymail_gpe_1_0_la_LIBADD = \
	$(LIBTINYMAIL_GPE_LIBS)	\
	$(top_builddir)/libtinymail/libtinymail-$(API_VERSION).la \
	$(top_builddir)/libtinymailui/libtinymailui-$(API_VERSION).la \
	$(top_builddir)/libtinymailui-gtk/libtinymailui-gtk-$(API_VERSION).la \
	$(top_builddir)/libtinymail-camel/libtinymail-camel-$(API_VERSION).la

libtinymail_gpe_1_0_la_LDFLAGS = -export-dynamic \
	-version-info $(LT_VERSION_INFO) $(LIBTOOL_EXPORT_OPTIONS)

libtinymailgpeincludedir = $(includedir)/libtinymail-gpe-$(API_VERSION)
libtinymailgpeinclude_HEADERS = $(libtinymail_gpe_1_0_headers)

libtinymail-gpe-$(API_VERSION).pc: libtinymail-gpe.pc
	cp $< $@

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = libtinymail-gpe-$(API_VERSION).pc
EXTRA_DIST = $(pkgconfig_DATA:-$(API_VERSION).pc=.pc.in)

DISTCLEANFILES = $(pkgconfig_DATA)

