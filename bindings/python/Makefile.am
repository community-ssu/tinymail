INCLUDES = $(PYTHON_INCLUDES) $(LIBTINYMAIL_CFLAGS) $(PYGTK_CFLAGS) \
	$(LIBTINYMAILUI_GTK_CFLAGS) $(TINYMAIL_PYTHON_CFLAGS) \
	-I$(top_srcdir)/libtinymail -I$(top_srcdir)/libtinymailui \
	-I$(top_srcdir)/libtinymailui-gtk -I$(top_srcdir)/libtinymail-camel \
	@PLATFORM_CFLAGS@ -I$(top_srcdir)/libtinymail-camel/camel-lite

common_ldflags = -module -avoid-version

pthdir = $(pythondir)
pth_DATA = tinymail.pth

tinymaildir = $(pythondir)/tinymail-$(API_VERSION)/tinymail
ptinymaildir = $(pythondir)/tinymail-$(API_VERSION)/tinymail

tinymail_LTLIBRARIES = _tinymail.la camel.la ui.la 
if BUILD_UIGTK
tinymail_LTLIBRARIES += uigtk.la 
endif

ptinymail_PYTHON = __init__.py

_tinymail_la_LDFLAGS = $(common_ldflags) -export-symbols-regex init_tinymail
_tinymail_la_LIBADD = $(LIBTINYMAIL_LIBS) $(TINYMAIL_PYTHON_LIBS) \
	$(top_builddir)/libtinymail/libtinymail-$(API_VERSION).la 
_tinymail_la_SOURCES = tinymailmodule.c pytny-status.c
nodist__tinymail_la_SOURCES = tinymail.c

ui_la_LDFLAGS = $(common_ldflags) -export-symbols-regex initui
ui_la_LIBADD = $(LIBTINYMAILUI_LIBS) $(TINYMAIL_PYTHON_LIBS) \
	$(LIBTINYMAIL_LIBS) \
	$(top_builddir)/libtinymail/libtinymail-$(API_VERSION).la \
	$(top_builddir)/libtinymailui/libtinymailui-$(API_VERSION).la
ui_la_SOURCES = tinymailuimodule.c
nodist_ui_la_SOURCES = tinymailui.c
 
uigtk_la_LDFLAGS = $(common_ldflags) -export-symbols-regex inituigtk
uigtk_la_LIBADD = $(LIBTINYMAILUI_GTK_LIBS) $(LIBTINYMAILUI_LIBS) \
	$(LIBTINYMAIL_LIBS) $(TINYMAIL_PYTHON_LIBS) \
	$(top_builddir)/libtinymail/libtinymail-$(API_VERSION).la \
	$(top_builddir)/libtinymailui/libtinymailui-$(API_VERSION).la \
	$(top_builddir)/libtinymailui-gtk/libtinymailui-gtk-$(API_VERSION).la
uigtk_la_SOURCES = tinymailui-gtkmodule.c
nodist_uigtk_la_SOURCES = tinymailui-gtk.c

camel_la_LDFLAGS = $(common_ldflags) -export-symbols-regex initcamel
camel_la_LIBADD = $(LIBTINYMAILUI_LIBS) $(LIBTINYMAIL_LIBS) \
        $(TINYMAIL_PYTHON_LIBS) \
        $(top_builddir)/libtinymail/libtinymail-$(API_VERSION).la \
        @PLATFORM_LIBS@ \
        $(top_builddir)/libtinymail-camel/libtinymail-camel-$(API_VERSION).la
camel_la_SOURCES = tinymail-camelmodule.c pytny-session-camel.c
nodist_camel_la_SOURCES = tinymail-camel.c

CREATEDEFS = $(PYTHON) $(top_srcdir)/bindings/python/createdefs.py

CLEANFILES = tinymail.c tinymailui.c tinymailui-gtk.c \
	tinymail.defs \
	tinymailui.defs tinymailui-gtk.defs \
	tinymail-camel.defs tinymail-camel.c

TINYMAIL_DEFS = $(top_srcdir)/bindings/python/tinymail-base.defs \
	$(top_srcdir)/bindings/python/tinymail-extra.defs

tinymail.defs: $(TINYMAIL_DEFS)
	$(CREATEDEFS) $@ $(TINYMAIL_DEFS)

tinymail.c: tinymail.defs tinymail.override
	$(PYGTK_CODEGEN) --prefix pytinymail \
	--override $(top_srcdir)/bindings/python/tinymail.override \
	tinymail.defs > $@

TINYMAILUI_DEFS = $(top_srcdir)/bindings/python/tinymailui-base.defs

tinymailui.defs: $(TINYMAILUI_DEFS)
	$(CREATEDEFS) $@ $(TINYMAILUI_DEFS)

tinymailui.c: tinymailui.defs tinymailui.override
	$(PYGTK_CODEGEN) --prefix pyui \
	--register tinymail.defs \
	--override $(top_srcdir)/bindings/python/tinymailui.override \
	tinymailui.defs > $@

TINYMAILUI_GTK_DEFS = $(top_srcdir)/bindings/python/tinymailui-gtk-base.defs

tinymailui-gtk.defs: $(TINYMAILUI_GTK_DEFS)
	$(CREATEDEFS) $@ $(TINYMAILUI_GTK_DEFS)

tinymailui-gtk.c: tinymail.defs $(PYGTK_DEFSDIR)/gdk-types.defs $(PYGTK_DEFSDIR)/gtk-types.defs tinymailui-gtk.defs tinymailui-gtk.override
	$(PYGTK_CODEGEN) --prefix pyuigtk --register $(PYGTK_DEFSDIR)/gdk-types.defs \
	--register $(PYGTK_DEFSDIR)/gtk-types.defs --register tinymail.defs \
	--register tinymailui.defs --override $(top_srcdir)/bindings/python/tinymailui-gtk.override \
	tinymailui-gtk.defs > $@

TINYMAIL_CAMEL_DEFS = $(top_srcdir)/bindings/python/tinymail-camel-base.defs \
	$(top_srcdir)/bindings/python/tinymail-camel-extra.defs

tinymail-camel.defs: $(TINYMAIL_CAMEL_DEFS)
	$(CREATEDEFS) $@ $(TINYMAIL_CAMEL_DEFS)

tinymail-camel.c: tinymail-camel.defs tinymail-camel.override
	$(PYGTK_CODEGEN) --prefix pycamel \
	--register tinymail.defs \
	--override $(top_srcdir)/bindings/python/tinymail-camel.override \
	tinymail-camel.defs > $@

EXTRA_DIST = filter.py tinymail.override \
	tinymailui.override tinymailui-gtk.override \
	tinymail-camel.override \
	tinymail-camel-base.defs tinymail-camel-extra.defs \
	tinymail-base.defs tinymail-extra.defs \
	tinymailui-gtk-base.defs \
	tinymailui-base.defs createdefs.py pytny-status.h \
	pytny-session-camel.h

#
#	pytny-status.c pytny-session-camel.c tinymailmodule.c \
#	tinymail-camelmodule.c \
#	tinymailui-gtkmodule.c tinymailuimodule.c
#
#
DIST_SUBDIRS=gendefs

