SUBDIRS = .
EXTRA_DIST = 

pkg = libtinymail

SNK = tny-sharp.snk
SYMBOLS =
INCLUDE_API =
METADATA = libtinymail.metadata
references =
glue_includes =

sources = TnyException.cs

customs = IteratorAdapter.custom ListAdapter.custom SimpleList.custom

%-$(API_VERSION).pc: %.pc
	cp $< $@

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = libtinymail-sharp-$(API_VERSION).pc
add_dist = $(pkgconfig_DATA:-$(API_VERSION).pc=.pc.in)
DISTCLEANFILES = $(pkgconfig_DATA)

include $(top_srcdir)/bindings/dotnet/Makefile.include

