SUBDIRS = .

pkg = libtinymailui-gtk

EXTRA_DIST = 
SNK = tny-sharp.snk
SYMBOLS =
INCLUDE_API = ../tny/libtinymail-api.xml ../ui/libtinymailui-api.xml
INCLUDE_EXTRA = $(GTK_SHARP_CFLAGS)
METADATA = libtinymailui-gtk.metadata
references = ../tny/libtinymail-sharp.dll ../ui/libtinymailui-sharp.dll
glue_includes =

sources =	

customs = AccountListModel.custom AttachListModel.custom FolderStoreTreeModel.custom HeaderListModel.custom


%-$(API_VERSION).pc: %.pc
	cp $< $@

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = libtinymailui-gtk-sharp-$(API_VERSION).pc
add_dist = $(pkgconfig_DATA:-$(API_VERSION).pc=.pc.in)
DISTCLEANFILES = $(pkgconfig_DATA)

include $(top_srcdir)/bindings/dotnet/Makefile.include

