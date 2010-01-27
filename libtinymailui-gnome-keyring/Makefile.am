INCLUDES = -I. -I$(top_srcdir) \
	$(TINYMAIL_CFLAGS) $(EXTRA_CFLAGS) \
	$(LIBTINYMAILUI_GTK_CFLAGS) \
	-I$(top_srcdir)/libtinymail \
	-I$(top_srcdir)/libtinymailui \
	-I$(top_srcdir)/libtinymailui-gtk 

if BUILD_GNOME
INCLUDES += -DGNOME
endif

if BUILD_MOZEMBED
INCLUDES += -DMOZEMBED
endif

noinst_LTLIBRARIES = libtinymailui-gnome-keyring.la

libtinymailui_gnome_keyring_la_SOURCES = \
	tny-gnome-keyring-password-getter.c \
	tny-gnome-keyring-password-getter.h
