NULL =

AM_CPPFLAGS = \
	-I. -I$(top_srcdir)				\
	$(TINYMAIL_CFLAGS) 				\
	-I$(top_srcdir)/libtinymail			\
	-I$(top_srcdir)/libtinymailui			\
	-I$(top_srcdir)/libtinymailui-gtk		\
	-I$(top_srcdir)/libtinymail-camel		\
	-I$(top_srcdir)/libtinymail-gnome-desktop	\
	$(NULL)

BUILT_SOURCES = tinymail-vala-test.vala.stamp

noinst_PROGRAMS = \
	tinymail-vala-test \
	$(NULL)

tinymail_vala_test_VALASOURCES = \
	tinymail-vala-test.vala \
	$(NULL)

tinymail_vala_test_SOURCES = \
	tinymail-vala-test.vala.stamp \
	$(tinymail_vala_test_VALASOURCES:.vala=.c) \
	$(tinymail_vala_test_VALASOURCES:.vala=.h) \
	$(NULL)

tinymail-vala-test.vala.stamp: $(tinymail_vala_test_VALASOURCES)
	$(VALAC) -C --vapidir $(top_srcdir)/bindings/vala --pkg libtinymail-gnome-desktop-1.0 --basedir $(top_srcdir) $^
	touch $@

tinymail_vala_test_LDADD = \
	$(TINYMAIL_LIBS)							\
	$(LIBTINYMAIL_GNOME_DESKTOP_LIBS)					\
	$(top_builddir)/libtinymail/libtinymail-$(API_VERSION).la		\
	$(top_builddir)/libtinymailui/libtinymailui-$(API_VERSION).la		\
	$(top_builddir)/libtinymailui-gtk/libtinymailui-gtk-$(API_VERSION).la	\
	$(top_builddir)/libtinymail-camel/libtinymail-camel-$(API_VERSION).la	\
	@PLATFORM_LIBS@								\
	$(NULL)

EXTRA_DIST = $(tinymail_vala_test_VALASOURCES) tinymail-vala-test.vala.stamp
