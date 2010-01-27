INCLUDES = -I. -I$(top_srcdir) $(EXTRA_CFLAGS) \
	$(LIBTINYMAILUI_GTK_CFLAGS)	\
	-I$(top_srcdir)/libtinymail	\
	-I$(top_srcdir)/libtinymailui 

if BUILD_GNOME
INCLUDES += -I$(top_srcdir)/libtinymail-gnomevfs -DGNOME
endif

lib_LTLIBRARIES = libtinymailui-gtk-1.0.la

libtinymailui_gtk_1_0_headers = \
	tny-gtk-mime-part-save-strategy.h \
	tny-gtk-msg-view.h \
	tny-gtk-lockable.h \
	tny-gtk-msg-window.h \
	tny-gtk-text-mime-part-view.h \
	tny-gtk-pixbuf-stream.h \
	tny-gtk-image-mime-part-view.h \
	tny-gtk-attachment-mime-part-view.h\
	tny-gtk-header-list-model.h \
	tny-gtk-account-list-model.h \
	tny-gtk-folder-list-store.h \
	tny-gtk-folder-store-tree-model.h \
	tny-gtk-attach-list-model.h \
	tny-gtk-text-buffer-stream.h \
	tny-gtk-header-view.h \
	tny-gtk-password-dialog.h \
	tny-gtk-expander-mime-part-view.h

libtinymailui_gtk_1_0_la_SOURCES = \
	$(libtinymailui_gtk_1_0_headers) \
	tny-gtk-mime-part-save-strategy.c \
	tny-gtk-msg-view.c \
	tny-gtk-lockable.c \
	tny-gtk-text-mime-part-view.c \
	tny-gtk-pixbuf-stream.c \
	tny-gtk-image-mime-part-view.c \
	tny-gtk-attachment-mime-part-view.c \
	tny-gtk-msg-window.c \
	tny-gtk-folder-list-store.c \
	tny-gtk-folder-list-store-iterator-priv.h \
	tny-gtk-folder-list-store-iterator.c \
	tny-gtk-folder-store-tree-model.c \
	tny-gtk-folder-store-tree-model-iterator-priv.h \
	tny-gtk-folder-store-tree-model-iterator.c \
	tny-gtk-header-list-iterator-priv.h \
	tny-gtk-header-list-model.c \
	tny-gtk-header-list-iterator.c \
	tny-gtk-attach-list-model-iterator-priv.h \
	tny-gtk-attach-list-model-iterator.c \
	tny-gtk-attach-list-model-priv.h \
	tny-gtk-attach-list-model.c \
	tny-gtk-account-list-model.c \
	tny-gtk-account-list-model-iterator.c \
	tny-gtk-account-list-model-iterator-priv.h \
	tny-gtk-text-buffer-stream.c \
	tny-gtk-header-view.c \
	tny-gtk-password-dialog.c \
	tny-gtk-expander-mime-part-view.c

libtinymailui_gtk_1_0_la_LIBADD = \
	$(LIBTINYMAILUI_GTK_LIBS) \
	$(top_builddir)/libtinymail/libtinymail-$(API_VERSION).la \
	$(top_builddir)/libtinymailui/libtinymailui-$(API_VERSION).la

libtinymailui_gtk_1_0_la_LDFLAGS = -export-dynamic \
	-version-info $(LT_VERSION_INFO) $(LIBTOOL_EXPORT_OPTIONS)

if BUILD_GNOME
libtinymailui_gtk_1_0_la_LIBADD += $(top_builddir)/libtinymail-gnomevfs/libtinymail-gnomevfs-$(API_VERSION).la
endif

libtinymailuigtkincludedir = $(includedir)/libtinymailui-gtk-$(API_VERSION)
libtinymailuigtkinclude_HEADERS = $(libtinymailui_gtk_1_0_headers)

libtinymailui-gtk-$(API_VERSION).pc: libtinymailui-gtk.pc
	cp $< $@

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = libtinymailui-gtk-$(API_VERSION).pc
EXTRA_DIST = $(pkgconfig_DATA:-$(API_VERSION).pc=.pc.in)

DISTCLEANFILES = $(pkgconfig_DATA)

