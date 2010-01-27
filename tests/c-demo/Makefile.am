INCLUDES = -I. -I$(top_srcdir)				\
	$(TINYMAIL_CFLAGS) 				\
	-I$(top_srcdir)/libtinymail			\
	-I$(top_srcdir)/libtinymailui			\
	-I$(top_srcdir)/libtinymailui-gtk		\
	-I$(top_srcdir)/libtinymail-camel		\
	-I$(top_srcdir)/$(PLATFORMDIR)			\
	-DASYNC_HEADERS					\
	-DPLATFORM=$(PLATFORM)

if BUILD_GNOME
INCLUDES += -DGNOME
endif

if BUILD_MOZEMBED
INCLUDES += -DMOZEMBED -I$(top_srcdir)/libtinymailui-mozembed $(LIBTINYMAILUI_MOZEMBED_CFLAGS)
endif

bin_PROGRAMS = tny-demoui

tny_demoui_SOURCES = 				\
	tny-main.c				\
	tny-demoui-summary-view.c		\
	tny-demoui-summary-view.h

tny_demoui_LDADD = 							        \
	$(TINYMAIL_LIBS)						        \
	$(LIBTINYMAIL_GNOME_DESKTOP_LIBS)					\
	$(top_builddir)/libtinymail/libtinymail-$(API_VERSION).la 	        \
	$(top_builddir)/libtinymailui/libtinymailui-$(API_VERSION).la	        \
	$(top_builddir)/libtinymailui-gtk/libtinymailui-gtk-$(API_VERSION).la   \
	$(top_builddir)/libtinymail-camel/libtinymail-camel-$(API_VERSION).la	\
	$(top_builddir)/$(PLATFORMDIR)/$(PLATFORMDIR)-$(API_VERSION).la


if BUILD_MOZEMBED
tny_demoui_LDADD +=  $(top_builddir)/libtinymailui-mozembed/libtinymailui-mozembed-$(API_VERSION).la \
	$(LIBTINYMAILUI_MOZEMBED_LIBS)
endif

