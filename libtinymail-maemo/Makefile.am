INCLUDES = -I. -I$(top_srcdir) $(EXTRA_CFLAGS) \
	$(LIBTINYMAIL_MAEMO_CFLAGS) \
	$(LIBTINYMAIL_CAMEL_CFLAGS) \
	$(LIBTINYMAIL_MAEMO_CONIC_CFLAGS) \
	-I$(top_srcdir)/libtinymail \
	-I$(top_srcdir)/libtinymailui \
	-I$(top_srcdir)/libtinymailui-gtk \
	-I$(top_srcdir)/libtinymail-camel \
	-I$(top_srcdir)/libtinymail-camel/camel-lite \
	-I$(top_srcdir)/libtinymail-gnomevfs \
	-DDBUS_API_SUBJECT_TO_CHANGE

lib_LTLIBRARIES = libtinymail-maemo-1.0.la

libtinymail_maemo_1_0_headers = \
	tny-maemo-account-store.h \
	tny-maemo-device.h \
	tny-maemo-device-priv.h \
	tny-maemo-platform-factory.h \
	tny-maemo-conic-device.h 	

libtinymail_maemo_1_0_la_SOURCES =  \
	$(libtinymail_maemo_1_0_headers) \
	tny-maemo-account-store.c \
	tny-maemo-device.c \
	tny-maemo-platform-factory.c


#
# if we have libconic and build for ARM: use tny-maemo-conic-device.c (the real one)
# if we have libconic and build for x86 (scratchbox): use tny-maemo-conic-dummy-device.c
# if we don't have libconic: use tny-maemo-noconic-device.c
#
if MAEMO_CONIC_HAVE_CONIC
if MAEMO_CONIC_DEVICE_DUMMY
libtinymail_maemo_1_0_la_SOURCES += tny-maemo-conic-dummy-device.c
else
libtinymail_maemo_1_0_la_SOURCES += tny-maemo-conic-device.c
endif
else
libtinymail_maemo_1_0_la_SOURCES += tny-maemo-noconic-device.c
endif


libtinymail_maemo_1_0_la_LIBADD = \
	$(LIBTINYMAIL_MAEMO_LIBS) \
	$(LIBTINYMAIL_MAEMO_CONIC_LIBS) \
	$(top_builddir)/libtinymail/libtinymail-$(API_VERSION).la \
	$(top_builddir)/libtinymailui/libtinymailui-$(API_VERSION).la \
	$(top_builddir)/libtinymailui-gtk/libtinymailui-gtk-$(API_VERSION).la \
	$(top_builddir)/libtinymail-camel/libtinymail-camel-$(API_VERSION).la

libtinymail_maemo_1_0_la_LDFLAGS = -export-dynamic \
	-version-info $(LT_VERSION_INFO) $(LIBTOOL_EXPORT_OPTIONS)

libtinymailmaemoincludedir = $(includedir)/libtinymail-maemo-$(API_VERSION)
libtinymailmaemoinclude_HEADERS = $(libtinymail_maemo_1_0_headers)

libtinymail-maemo-$(API_VERSION).pc: libtinymail-maemo.pc
	cp $< $@

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = libtinymail-maemo-$(API_VERSION).pc
EXTRA_DIST = $(pkgconfig_DATA:-$(API_VERSION).pc=.pc.in)

DISTCLEANFILES = $(pkgconfig_DATA)
