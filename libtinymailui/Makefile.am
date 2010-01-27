INCLUDES = $(EXTRA_CFLAGS) \
	$(LIBTINYMAILUI_CFLAGS) \
	-I$(top_srcdir)/libtinymail

lib_LTLIBRARIES = libtinymailui-1.0.la

libtinymailui_1_0_headers = \
	tnyui.h \
	tny-summary-view.h \
	tny-msg-view.h \
	tny-mime-part-save-strategy.h \
	tny-msg-window.h \
	tny-platform-factory.h \
	tny-header-view.h \
	tny-account-store-view.h \
	tny-mime-part-view.h \
	tny-mime-part-saver.h

libtinymailui_1_0_la_SOURCES = \
	$(libtinymailui_1_0_headers) \
	tny-summary-view.c \
	tny-mime-part-save-strategy.c \
	tny-msg-view.c \
	tny-msg-window.c \
	tny-platform-factory.c \
	tny-header-view.c \
	tny-account-store-view.c \
	tny-mime-part-view.c \
	tny-mime-part-saver.c 

libtinymailui_1_0_la_LIBADD = \
	$(LIBTINYMAILUI_LIBS) \
	$(top_builddir)/libtinymail/libtinymail-$(API_VERSION).la

libtinymailui_1_0_la_LDFLAGS = -export-dynamic -version-info $(LT_VERSION_INFO) $(LIBTOOL_EXPORT_OPTIONS)

libtinymailuiincludedir = $(includedir)/libtinymailui-$(API_VERSION)
libtinymailuiinclude_HEADERS = $(libtinymailui_1_0_headers)

libtinymailui-$(API_VERSION).pc: libtinymailui.pc
	cp $< $@

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = libtinymailui-$(API_VERSION).pc
EXTRA_DIST = $(pkgconfig_DATA:-$(API_VERSION).pc=.pc.in)

DISTCLEANFILES = $(pkgconfig_DATA)
