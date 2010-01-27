SUBDIRS = .

pkg = libtinymailui

EXTRA_DIST = 
SNK = tny-sharp.snk
SYMBOLS =
INCLUDE_API = ../tny/libtinymail-api.xml
METADATA = libtinymailui.metadata
references = ../tny/libtinymail-sharp.dll
glue_includes =

sources = 				

customs = 				


%-$(API_VERSION).pc: %.pc
	cp $< $@

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = libtinymailui-sharp-$(API_VERSION).pc
add_dist = $(pkgconfig_DATA:-$(API_VERSION).pc=.pc.in)
DISTCLEANFILES = $(pkgconfig_DATA)

include $(top_srcdir)/bindings/dotnet/Makefile.include

