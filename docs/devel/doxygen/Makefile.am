all: libtinymail
	$(DOXYGEN) libtinymail
	$(DOXYGEN) libtinymailui
	$(DOXYGEN) libtinymailui-gtk
	$(DOXYGEN) libtinymail-camel

clean:
	rm -rf out/libtinymail libtinymail
	rm -rf out/libtinymailui libtinymailui
	rm -rf out/libtinymailui-gtk libtinymailui-gtk
	rm -rf out/libtinymail-camel libtinymail-camel

EXTRA_DIST=Makefile.in

distclean: clean
	rm -rf Makefile
