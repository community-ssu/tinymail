INCLUDES = -I. -I$(top_srcdir) $(EXTRA_CFLAGS) \
	$(LIBTINYMAILUI_GTKHTML_CFLAGS) \
	-I$(top_srcdir)/libtinymail \
	-I$(top_srcdir)/libtinymailui \
	-I$(top_srcdir)/libtinymailui-gtk \
	-I$(top_srcdir)/libtinymail-camel

if BUILD_GNOME
INCLUDES +=  -I$(top_srcdir)/libtinymail-gnomevfs -DGNOME
endif

lib_LTLIBRARIES = libtinymailui-gtkhtml-1.0.la

libtinymailui_gtkhtml_1_0_headers = \
	tny-gtk-html-msg-view.h \
	tny-gtk-html-mime-part-view.h \
	tny-gtk-html-stream.h

libtinymailui_gtkhtml_1_0_la_SOURCES = \
	$(libtinymailui_gtkhtml_1_0_headers) \
	tny-gtk-html-mime-part-view.c \
	tny-gtk-html-msg-view.c \
	tny-gtk-html-stream.c

libtinymailui_gtkhtml_1_0_la_LIBADD = 	\
	$(LIBTINYMAILUI_GTKHTML_LIBS)	\
	$(top_builddir)/libtinymail/libtinymail-$(API_VERSION).la   \
	$(top_builddir)/libtinymailui/libtinymailui-$(API_VERSION).la \
	$(top_builddir)/libtinymailui-gtk/libtinymailui-gtk-$(API_VERSION).la

libtinymailui_gtkhtml_1_0_la_LDFLAGS = -export-dynamic -version-info $(LT_VERSION_INFO) $(LIBTOOL_EXPORT_OPTIONS)

if BUILD_GNOME
libtinymailui_gtkhtml_1_0_la_LIBADD +=  $(top_builddir)/libtinymail-gnomevfs/libtinymail-gnomevfs-$(API_VERSION).la
endif

libtinymailuigtkhtmlincludedir = $(includedir)/libtinymailui-gtkhtml-$(API_VERSION)
libtinymailuigtkhtmlinclude_HEADERS = $(libtinymailui_gtkhtml_1_0_headers)

libtinymailui-gtkhtml-$(API_VERSION).pc: libtinymailui-gtkhtml.pc
	cp $< $@

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = libtinymailui-gtkhtml-$(API_VERSION).pc
EXTRA_DIST = $(pkgconfig_DATA:-$(API_VERSION).pc=.pc.in)

DISTCLEANFILES = $(pkgconfig_DATA)
