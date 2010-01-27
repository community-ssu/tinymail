INCLUDES = -I. -I$(top_srcdir) $(EXTRA_CFLAGS) \
	$(LIBTINYMAILUI_WEBKIT_CFLAGS) \
	-I$(top_srcdir)/libtinymail \
	-I$(top_srcdir)/libtinymailui \
	-I$(top_srcdir)/libtinymailui-gtk \
	-I$(top_srcdir)/libtinymail-camel

if BUILD_GNOME
INCLUDES +=  -I$(top_srcdir)/libtinymail-gnomevfs -DGNOME
endif

lib_LTLIBRARIES = libtinymailui-webkit-1.0.la

libtinymailui_webkit_1_0_headers = \
	tny-webkit-msg-view.h \
	tny-webkit-html-mime-part-view.h \
	tny-webkit-stream.h

libtinymailui_webkit_1_0_la_SOURCES = \
	$(libtinymailui_webkit_1_0_headers) \
	tny-webkit-html-mime-part-view.c \
	tny-webkit-msg-view.c \
	tny-webkit-stream.c

libtinymailui_webkit_1_0_la_LIBADD = 	\
	$(LIBTINYMAILUI_WEBKIT_LIBS)	\
	$(top_builddir)/libtinymail/libtinymail-$(API_VERSION).la   \
	$(top_builddir)/libtinymailui/libtinymailui-$(API_VERSION).la \
	$(top_builddir)/libtinymailui-gtk/libtinymailui-gtk-$(API_VERSION).la

libtinymailui_webkit_1_0_la_LDFLAGS = -export-dynamic -version-info $(LT_VERSION_INFO) $(LIBTOOL_EXPORT_OPTIONS)

if BUILD_GNOME
libtinymailui_webkit_1_0_la_LIBADD +=  $(top_builddir)/libtinymail-gnomevfs/libtinymail-gnomevfs-$(API_VERSION).la
endif

libtinymailuiwebkitincludedir = $(includedir)/libtinymailui-webkit-$(API_VERSION)
libtinymailuiwebkitinclude_HEADERS = $(libtinymailui_webkit_1_0_headers)

libtinymailui-webkit-$(API_VERSION).pc: libtinymailui-webkit.pc
	cp $< $@

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = libtinymailui-webkit-$(API_VERSION).pc
EXTRA_DIST = $(pkgconfig_DATA:-$(API_VERSION).pc=.pc.in)

DISTCLEANFILES = $(pkgconfig_DATA)
