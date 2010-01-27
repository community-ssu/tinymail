INCLUDES = $(LIBTINYMAIL_CFLAGS) -I. -I$(top_srcdir) \
	-I$(top_srcdir)/libtinymail $(EXTRA_CFLAGS)

lib_LTLIBRARIES = libtinymail-acap-1.0.la

libtinymail_acap_1_0_headers = \
	tny-acap-account-store.h

libtinymail_acap_1_0_la_SOURCES = \
	$(libtinymail_acap_1_0_headers) \
	tny-acap-account-store.c

libtinymail_acap_1_0_la_LIBADD = $(LIBTINYMAIL_LIBS)
libtinymail_acap_1_0_la_LDFLAGS = -export-dynamic -version-info $(LT_VERSION_INFO) $(LIBTOOL_EXPORT_OPTIONS)
libtinymail_acap_1_0_includedir = $(includedir)/libtinymail-$(API_VERSION)
libtinymail_acap_1_0_include_HEADERS = $(libtinymail_acap_1_0_headers)

libtinymail-acap-$(API_VERSION).pc: libtinymail-acap.pc
	cp $< $@

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = libtinymail-acap-$(API_VERSION).pc
EXTRA_DIST = $(pkgconfig_DATA:-$(API_VERSION).pc=.pc.in)

DISTCLEANFILES = $(pkgconfig_DATA)
