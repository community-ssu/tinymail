INCLUDES = $(LIBTINYMAIL_GNOMEVFS_CFLAGS) \
	-I. -I$(top_srcdir) $(EXTRA_CFLAGS) \
	-I$(top_srcdir)/libtinymail

lib_LTLIBRARIES = libtinymail-gnomevfs-1.0.la 

libtinymail_gnomevfs_1_0_headers =		\
	tny-vfs-stream.h

libtinymail_gnomevfs_1_0_la_SOURCES = 		\
	$(libtinymail_gnomevfs_1_0_headers)	\
	tny-vfs-stream.c

libtinymail_gnomevfs_1_0_la_LIBADD =		\
	$(LIBTINYMAIL_GNOMEVFS_LIBS)		\
	$(top_builddir)/libtinymail/libtinymail-$(API_VERSION).la

libtinymail_gnomevfs_1_0_la_LDFLAGS = -export-dynamic \
	-version-info $(LT_VERSION_INFO) $(LIBTOOL_EXPORT_OPTIONS)

libtinymail_gnomevfs_1_0includedir = $(includedir)/libtinymail-gnomevfs-$(API_VERSION)
libtinymail_gnomevfs_1_0include_HEADERS = $(libtinymail_gnomevfs_1_0_headers)

libtinymail-gnomevfs-$(API_VERSION).pc: libtinymail-gnomevfs.pc
	cp $< $@

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = libtinymail-gnomevfs-$(API_VERSION).pc
EXTRA_DIST = $(pkgconfig_DATA:-$(API_VERSION).pc=.pc.in)

DISTCLEANFILES = $(pkgconfig_DATA)
