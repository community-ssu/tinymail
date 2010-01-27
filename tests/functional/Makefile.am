INCLUDES = -I. -I$(top_srcdir) -I$(top_srcdir)/tests/shared \
	$(TINYMAIL_CFLAGS) \
	-I$(top_srcdir)/libtinymail \
	-I$(top_srcdir)/libtinymailui \
	-I$(top_srcdir)/libtinymailui-gtk \
	-I$(top_srcdir)/libtinymail-camel \
	-I$(top_srcdir)/libtinymail-camel/camel-lite \
	-DASYNC_HEADERS

if BUILD_GNOME
INCLUDES += -DGNOME
endif

if BUILD_MOZEMBED
INCLUDES += -DMOZEMBED
endif

bin_PROGRAMS = folder-lister folder-lister-async msg-transfer msg-sender anything folder-transfer account-refresh folder-remove

anything_SOURCES = anything.c
anything_LDADD = \
	$(TINYMAIL_LIBS) $(LIBTINYMAIL_GNOME_DESKTOP_LIBS) \
	$(top_builddir)/libtinymail/libtinymail-$(API_VERSION).la \
	$(top_builddir)/libtinymailui/libtinymailui-$(API_VERSION).la \
	$(top_builddir)/libtinymailui-gtk/libtinymailui-gtk-$(API_VERSION).la \
	$(top_builddir)/libtinymail-camel/libtinymail-camel-$(API_VERSION).la \
	$(top_builddir)/tests/shared/libtestsshared.la

msg_sender_SOURCES = msg-sender.c
msg_sender_LDADD = \
	$(TINYMAIL_LIBS) $(LIBTINYMAIL_GNOME_DESKTOP_LIBS) \
	$(top_builddir)/libtinymail/libtinymail-$(API_VERSION).la \
	$(top_builddir)/libtinymailui/libtinymailui-$(API_VERSION).la \
	$(top_builddir)/libtinymailui-gtk/libtinymailui-gtk-$(API_VERSION).la \
	$(top_builddir)/libtinymail-camel/libtinymail-camel-$(API_VERSION).la \
	$(top_builddir)/tests/shared/libtestsshared.la

folder_lister_SOURCES = folder-lister.c 
folder_lister_LDADD = \
	$(TINYMAIL_LIBS) $(LIBTINYMAIL_GNOME_DESKTOP_LIBS) \
	$(top_builddir)/libtinymail/libtinymail-$(API_VERSION).la \
	$(top_builddir)/libtinymailui/libtinymailui-$(API_VERSION).la \
	$(top_builddir)/libtinymailui-gtk/libtinymailui-gtk-$(API_VERSION).la \
	$(top_builddir)/libtinymail-camel/libtinymail-camel-$(API_VERSION).la \
	$(top_builddir)/tests/shared/libtestsshared.la

folder_lister_async_SOURCES = folder-lister-async.c
folder_lister_async_LDADD = \
	$(TINYMAIL_LIBS) $(LIBTINYMAIL_GNOME_DESKTOP_LIBS) \
	$(top_builddir)/libtinymail/libtinymail-$(API_VERSION).la \
	$(top_builddir)/libtinymailui/libtinymailui-$(API_VERSION).la \
	$(top_builddir)/libtinymailui-gtk/libtinymailui-gtk-$(API_VERSION).la \
	$(top_builddir)/libtinymail-camel/libtinymail-camel-$(API_VERSION).la \
	$(top_builddir)/tests/shared/libtestsshared.la

msg_transfer_SOURCES = msg-transfer.c
msg_transfer_LDADD = \
	$(TINYMAIL_LIBS) $(LIBTINYMAIL_GNOME_DESKTOP_LIBS) \
	$(top_builddir)/libtinymail/libtinymail-$(API_VERSION).la \
	$(top_builddir)/libtinymailui/libtinymailui-$(API_VERSION).la \
	$(top_builddir)/libtinymailui-gtk/libtinymailui-gtk-$(API_VERSION).la \
	$(top_builddir)/libtinymail-camel/libtinymail-camel-$(API_VERSION).la \
	$(top_builddir)/tests/shared/libtestsshared.la

folder_transfer_SOURCES = folder-transfer.c
folder_transfer_LDADD = \
	$(TINYMAIL_LIBS) $(LIBTINYMAIL_GNOME_DESKTOP_LIBS) \
	$(top_builddir)/libtinymail/libtinymail-$(API_VERSION).la \
	$(top_builddir)/libtinymailui/libtinymailui-$(API_VERSION).la \
	$(top_builddir)/libtinymailui-gtk/libtinymailui-gtk-$(API_VERSION).la \
	$(top_builddir)/libtinymail-camel/libtinymail-camel-$(API_VERSION).la \
	$(top_builddir)/tests/shared/libtestsshared.la

account_refresh_SOURCES = account-refresh.c
account_refresh_LDADD = \
	$(TINYMAIL_LIBS) $(LIBTINYMAIL_GNOME_DESKTOP_LIBS) \
	$(top_builddir)/libtinymail/libtinymail-$(API_VERSION).la \
	$(top_builddir)/libtinymailui/libtinymailui-$(API_VERSION).la \
	$(top_builddir)/libtinymailui-gtk/libtinymailui-gtk-$(API_VERSION).la \
	$(top_builddir)/libtinymail-camel/libtinymail-camel-$(API_VERSION).la \
	$(top_builddir)/tests/shared/libtestsshared.la

folder_remove_SOURCES = folder-remove.c
folder_remove_LDADD = \
	$(TINYMAIL_LIBS) $(LIBTINYMAIL_GNOME_DESKTOP_LIBS) \
	$(top_builddir)/libtinymail/libtinymail-$(API_VERSION).la \
	$(top_builddir)/libtinymailui/libtinymailui-$(API_VERSION).la \
	$(top_builddir)/libtinymailui-gtk/libtinymailui-gtk-$(API_VERSION).la \
	$(top_builddir)/libtinymail-camel/libtinymail-camel-$(API_VERSION).la \
	$(top_builddir)/tests/shared/libtestsshared.la
