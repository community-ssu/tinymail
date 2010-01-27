SUBDIRS = .

pkg = libtinymail-platform

EXTRA_DIST = 
SNK = tny-sharp.snk
SYMBOLS =
INCLUDE_API = ../tny/libtinymail-api.xml ../ui/libtinymailui-api.xml ../camel/libtinymail-camel-api.xml
METADATA = libtinymail-platform.metadata
references = ../tny/libtinymail-sharp.dll ../ui/libtinymailui-sharp.dll ../camel/libtinymail-camel-sharp.dll
glue_includes =

sources = Init.cs

customs =


%-$(API_VERSION).pc: %.pc
	cp $< $@

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = libtinymail-platform-sharp-$(API_VERSION).pc
add_dist = $(pkgconfig_DATA:-$(API_VERSION).pc=.pc.in)
DISTCLEANFILES = $(pkgconfig_DATA)

include $(top_srcdir)/bindings/dotnet/Makefile.include

