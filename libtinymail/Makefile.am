INCLUDES = $(LIBTINYMAIL_CFLAGS) -I. -I$(top_srcdir) $(EXTRA_CFLAGS)

lib_LTLIBRARIES = libtinymail-1.0.la

tny-signals-marshal.h: tny-signals-marshal.list
	glib-genmarshal --header --prefix=$(subst -,_,tny)_marshal $< > tny-signals-marshal.h

tny-signals-marshal.c: tny-signals-marshal.list tny-signals-marshal.h
	glib-genmarshal --body --prefix=$(subst -,_,tny)_marshal $< > tny-signals-marshal.c
	sed -i "1i#include \"tny-signals-marshal.h\"" tny-signals-marshal.c

CLEANFILES=tny-signals-marshal.c tny-signals-marshal.h

private_headers = tny-common-priv.h observer-mixin-priv.h

libtinymail_1_0_headers = \
	tny-signals-marshal.h \
	tny-shared.h tny.h \
	tny-error.h \
	tny-status.h \
	tny-list.h \
	tny-iterator.h \
	tny-folder.h \
	tny-mime-part.h \
	tny-header.h \
	tny-pair.h \
	tny-folder-stats.h \
	tny-folder-change.h \
	tny-folder-store-change.h \
	tny-folder-observer.h \
	tny-folder-store-observer.h \
	tny-folder-monitor.h \
	tny-msg.h \
	tny-device.h \
	tny-lockable.h \
	tny-noop-lockable.h \
	tny-account.h \
	tny-store-account.h \
	tny-transport-account.h \
	tny-account-store.h \
	tny-stream.h \
	tny-seekable.h \
	tny-fs-stream.h \
	tny-simple-list.h \
	tny-folder-store.h \
	tny-folder-store-query.h \
	tny-msg-remove-strategy.h \
	tny-msg-receive-strategy.h \
	tny-send-queue.h \
	tny-password-getter.h \
	tny-merge-folder.h \
	tny-stream-cache.h \
	tny-fs-stream-cache.h \
	tny-cached-file.h \
	tny-cached-file-stream.h \
	tny-combined-account.h \
	tny-connection-policy.h \
	tny-merge-folder-store.h

libtinymail_1_0_la_SOURCES = \
	$(libtinymail_1_0_headers) \
	tny-expunged-header.c \
	tny-signals-marshal.c \
	tny-error.c \
	tny-status.c \
	tny-list.c \
	tny-iterator.c \
	tny-msg.c \
	tny-header.c \
	tny-pair.c \
	tny-folder-stats.c \
	tny-folder-change.c \
	tny-folder-store-change.c \
	tny-folder-observer.c \
	tny-folder-store-observer.c \
	tny-folder-monitor.c \
	tny-mime-part.c \
	tny-folder.c \
	tny-device.c \
	tny-lockable.c \
	tny-noop-lockable.c \
	tny-account.c \
	tny-store-account.c \
	tny-transport-account.c \
	tny-account-store.c \
	tny-stream.c \
	tny-seekable.c \
	tny-fs-stream.c \
	tny-simple-list-priv.h \
	tny-simple-list.c \
	tny-simple-list-iterator-priv.h \
	tny-simple-list-iterator.c \
	tny-folder-store.c \
	tny-folder-store-query.c \
	tny-msg-remove-strategy.c \
	tny-msg-receive-strategy.c \
	tny-send-queue.c \
	tny-password-getter.c \
	tny-stream-cache.c \
	tny-fs-stream-cache.c \
	tny-cached-file.c \
	tny-cached-file-stream.c \
	tny-merge-folder.c \
	tny-combined-account.c \
	tny-idle-stopper.c \
	tny-progress-info.c \
	tny-connection-policy.c \
	tny-merge-folder-store.c \
	observer-mixin.c \
	$(private_headers)

libtinymail_1_0_la_LIBADD = $(LIBTINYMAIL_LIBS)
libtinymail_1_0_la_LDFLAGS = -export-dynamic -version-info $(LT_VERSION_INFO) $(LIBTOOL_EXPORT_OPTIONS)
libtinymail_1_0_includedir = $(includedir)/libtinymail-$(API_VERSION)
libtinymail_1_0_include_HEADERS = $(libtinymail_1_0_headers)

libtinymail-$(API_VERSION).pc: libtinymail.pc
	cp $< $@

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = libtinymail-$(API_VERSION).pc
EXTRA_DIST = $(pkgconfig_DATA:-$(API_VERSION).pc=.pc.in) tny-signals-marshal.list

DISTCLEANFILES = $(pkgconfig_DATA)
