all:tinymail-base.defs tinymailui-base.defs tinymailui-gtk-base.defs tinymail-camel-base.defs

CREATEDEFS = $(PYTHON) $(top_srcdir)/bindings/python/createdefs.py

CLEANFILES = tinymail-base.defs \
	     tinymailui-base.defs \
	     tinymailui-gtk-base.defs \
	     tinymail-camel-base.defs 

# To update:
# ls libtinymail/*h | grep -v \\-priv | sed s/^/\\t\$\(top_srcdir\)\\//g | sed s/$/\\\ \\\\/g
# ls libtinymail/*h | grep -v \\-priv | sed s/^/\\$\(top_srcdir\)\\//g | sed s/$/\\\ \\\\/g
#
tinymail_h_files = \
        $(top_srcdir)/libtinymail/tny-account.h \
        $(top_srcdir)/libtinymail/tny-account-store.h \
        $(top_srcdir)/libtinymail/tny-combined-account.h \
        $(top_srcdir)/libtinymail/tny-connection-policy.h \
        $(top_srcdir)/libtinymail/tny-device.h \
        $(top_srcdir)/libtinymail/tny-enums.h \
        $(top_srcdir)/libtinymail/tny-error.h \
        $(top_srcdir)/libtinymail/tny-folder-change.h \
        $(top_srcdir)/libtinymail/tny-folder.h \
        $(top_srcdir)/libtinymail/tny-folder-monitor.h \
        $(top_srcdir)/libtinymail/tny-folder-observer.h \
        $(top_srcdir)/libtinymail/tny-folder-stats.h \
        $(top_srcdir)/libtinymail/tny-folder-store-change.h \
        $(top_srcdir)/libtinymail/tny-folder-store.h \
        $(top_srcdir)/libtinymail/tny-folder-store-observer.h \
        $(top_srcdir)/libtinymail/tny-folder-store-query.h \
        $(top_srcdir)/libtinymail/tny-fs-stream.h \
        $(top_srcdir)/libtinymail/tny.h \
        $(top_srcdir)/libtinymail/tny-header.h \
        $(top_srcdir)/libtinymail/tny-iterator.h \
        $(top_srcdir)/libtinymail/tny-list.h \
        $(top_srcdir)/libtinymail/tny-lockable.h \
        $(top_srcdir)/libtinymail/tny-merge-folder.h \
        $(top_srcdir)/libtinymail/tny-mime-part.h \
        $(top_srcdir)/libtinymail/tny-msg.h \
        $(top_srcdir)/libtinymail/tny-msg-receive-strategy.h \
        $(top_srcdir)/libtinymail/tny-msg-remove-strategy.h \
        $(top_srcdir)/libtinymail/tny-noop-lockable.h \
        $(top_srcdir)/libtinymail/tny-pair.h \
        $(top_srcdir)/libtinymail/tny-password-getter.h \
        $(top_srcdir)/libtinymail/tny-seekable.h \
        $(top_srcdir)/libtinymail/tny-send-queue.h \
        $(top_srcdir)/libtinymail/tny-shared.h \
        $(top_srcdir)/libtinymail/tny-signals-marshal.h \
        $(top_srcdir)/libtinymail/tny-simple-list.h \
        $(top_srcdir)/libtinymail/tny-status.h \
        $(top_srcdir)/libtinymail/tny-store-account.h \
        $(top_srcdir)/libtinymail/tny-stream.h \
        $(top_srcdir)/libtinymail/tny-transport-account.h 

tinymail-base.defs: $(tinymail_h_files) $(top_srcdir)/bindings/python/gendefs/defsgen.py
	$(PYTHON) defsgen.py -m tny -f $(top_srcdir)/bindings/python/tinymail-extra.defs -l $(top_srcdir)/libtinymail/.libs/libtinymail-1.0.so $(tinymail_h_files) > $@

tinymailui_h_files = \
        $(top_srcdir)/libtinymailui/tny-account-store-view.h \
        $(top_srcdir)/libtinymailui/tny-header-view.h \
        $(top_srcdir)/libtinymailui/tny-mime-part-saver.h \
        $(top_srcdir)/libtinymailui/tny-mime-part-save-strategy.h \
        $(top_srcdir)/libtinymailui/tny-mime-part-view.h \
        $(top_srcdir)/libtinymailui/tny-msg-view.h \
        $(top_srcdir)/libtinymailui/tny-msg-window.h \
        $(top_srcdir)/libtinymailui/tny-platform-factory.h \
        $(top_srcdir)/libtinymailui/tny-summary-view.h \
        $(top_srcdir)/libtinymailui/tnyui.h 

tinymailui-base.defs: $(tinymailui_h_files) $(top_srcdir)/bindings/python/gendefs/defsgen.py
	$(PYTHON) $(top_srcdir)/bindings/python/gendefs/defsgen.py -m tny -l $(top_srcdir)/libtinymailui/.libs/libtinymailui-1.0.so $(tinymailui_h_files) > $@

tinymailui_gtk_h_files = \
        $(top_srcdir)/libtinymailui-gtk/tny-gtk-account-list-model.h \
        $(top_srcdir)/libtinymailui-gtk/tny-gtk-attach-list-model.h \
        $(top_srcdir)/libtinymailui-gtk/tny-gtk-attachment-mime-part-view.h \
        $(top_srcdir)/libtinymailui-gtk/tny-gtk-enums.h \
        $(top_srcdir)/libtinymailui-gtk/tny-gtk-expander-mime-part-view.h \
        $(top_srcdir)/libtinymailui-gtk/tny-gtk-folder-store-tree-model.h \
        $(top_srcdir)/libtinymailui-gtk/tny-gtk-header-list-model.h \
        $(top_srcdir)/libtinymailui-gtk/tny-gtk-header-view.h \
        $(top_srcdir)/libtinymailui-gtk/tny-gtk-image-mime-part-view.h \
        $(top_srcdir)/libtinymailui-gtk/tny-gtk-lockable.h \
        $(top_srcdir)/libtinymailui-gtk/tny-gtk-mime-part-save-strategy.h \
        $(top_srcdir)/libtinymailui-gtk/tny-gtk-msg-view.h \
        $(top_srcdir)/libtinymailui-gtk/tny-gtk-msg-window.h \
        $(top_srcdir)/libtinymailui-gtk/tny-gtk-password-dialog.h \
        $(top_srcdir)/libtinymailui-gtk/tny-gtk-pixbuf-stream.h \
        $(top_srcdir)/libtinymailui-gtk/tny-gtk-text-buffer-stream.h \
        $(top_srcdir)/libtinymailui-gtk/tny-gtk-text-mime-part-view.h 

tinymailui-gtk-base.defs: $(tinymailui_gtk_h_files) $(top_srcdir)/bindings/python/gendefs/defsgen.py
	$(PYTHON) defsgen.py -m tny --novirtuals -l $(top_srcdir)/libtinymailui-gtk/.libs/libtinymailui-gtk-1.0.so $(tinymailui_gtk_h_files) > $@

tinymail_camel_h_files = \
        $(top_srcdir)/libtinymail-camel/tny-camel-account.h \
        $(top_srcdir)/libtinymail-camel/tny-camel-bs-mime-part.h \
        $(top_srcdir)/libtinymail-camel/tny-camel-bs-msg.h \
        $(top_srcdir)/libtinymail-camel/tny-camel-bs-msg-receive-strategy.h \
        $(top_srcdir)/libtinymail-camel/tny-camel-default-connection-policy.h \
        $(top_srcdir)/libtinymail-camel/tny-camel-folder.h \
        $(top_srcdir)/libtinymail-camel/tny-camel-full-msg-receive-strategy.h \
        $(top_srcdir)/libtinymail-camel/tny-camel-header.h \
        $(top_srcdir)/libtinymail-camel/tny-camel-imap-folder.h \
        $(top_srcdir)/libtinymail-camel/tny-camel-imap-store-account.h \
        $(top_srcdir)/libtinymail-camel/tny-camel-mem-stream.h \
        $(top_srcdir)/libtinymail-camel/tny-camel-mime-part.h \
        $(top_srcdir)/libtinymail-camel/tny-camel-msg.h \
        $(top_srcdir)/libtinymail-camel/tny-camel-msg-remove-strategy.h \
        $(top_srcdir)/libtinymail-camel/tny-camel-nntp-folder.h \
        $(top_srcdir)/libtinymail-camel/tny-camel-nntp-store-account.h \
        $(top_srcdir)/libtinymail-camel/tny-camel-partial-msg-receive-strategy.h \
        $(top_srcdir)/libtinymail-camel/tny-camel-pop-folder.h \
        $(top_srcdir)/libtinymail-camel/tny-camel-pop-remote-msg-remove-strategy.h \
        $(top_srcdir)/libtinymail-camel/tny-camel-pop-store-account.h \
        $(top_srcdir)/libtinymail-camel/tny-camel-recover-connection-policy.h \
        $(top_srcdir)/libtinymail-camel/tny-camel-send-queue.h \
        $(top_srcdir)/libtinymail-camel/tny-camel-shared.h \
        $(top_srcdir)/libtinymail-camel/tny-camel-store-account.h \
        $(top_srcdir)/libtinymail-camel/tny-camel-stream.h \
        $(top_srcdir)/libtinymail-camel/tny-camel-transport-account.h \
        $(top_srcdir)/libtinymail-camel/tny-session-camel.h \
        $(top_srcdir)/libtinymail-camel/tny-stream-camel.h 

tinymail-camel-base.defs : $(tinymail_camel_h_files) $(top_srcdir)/bindings/python/gendefs/defsgen.py
	$(PYTHON) defsgen.py -m tny -f $(top_srcdir)/bindings/python/tinymail-camel-extra.defs --novirtuals -l $(top_srcdir)/libtinymail-camel/.libs/libtinymail-camel-1.0.so $(tinymail_camel_h_files) > $@
