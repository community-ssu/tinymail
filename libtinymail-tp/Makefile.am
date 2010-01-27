SUBDIRS = .

INCLUDES = $(LIBTINYMAIL_TP_CFLAGS) $(EXTRA_CFLAGS) \
	-I. -I$(top_srcdir) \
	-I$(top_srcdir)/libtinymail

lib_LTLIBRARIES = libtinymail-tp-1.0.la 

libtinymail_tp_1_0_headers =	\
	tny-tp-mail-notifier.h 

libtinymail_tp_1_0_la_SOURCES = \
	$(libtinymail_tp_1_0_headers) \
	tny-tp-mail-notifier.c 

libtinymail_tp_1_0_la_LIBADD = \
	$(LIBTINYMAIL_TP_LIBS) \
	$(top_builddir)/libtinymail/libtinymail-$(API_VERSION).la 

libtinymail_tp_1_0_la_LDFLAGS = -export-dynamic -version-info \
	$(LT_VERSION_INFO) $(LIBTOOL_EXPORT_OPTIONS)
libtinymail_tp_1_0includedir = $(includedir)/libtinymail-tp-$(API_VERSION)
libtinymail_tp_1_0include_HEADERS = $(libtinymail_tp_1_0_headers)

libtinymail-tp-$(API_VERSION).pc: libtinymail-tp.pc
	cp $< $@

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = libtinymail-tp-$(API_VERSION).pc
EXTRA_DIST = $(pkgconfig_DATA:-$(API_VERSION).pc=.pc.in)

DISTCLEANFILES = $(pkgconfig_DATA)
