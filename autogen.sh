#!/bin/sh
# Run this to generate all the initial makefiles, etc.

export ACLOCAL_FLAGS="-I `pwd`/m4 $ACLOCAL_FLAGS"

srcdir=`dirname $0`
test -z "$srcdir" && srcdir=.
REQUIRED_AUTOMAKE_VERSION=1.7
PKG_NAME=tinymail

(test -f $srcdir/configure.ac \
  && test -f $srcdir/libtinymail/tny-folder.c) || {
    echo -n "**Error**: Directory "\`$srcdir\'" does not look like the"
    echo " top-level $PKG_NAME directory"
    exit 1
}


which gnome-autogen.sh || {
    echo "You need to install gnome-common from the GNOME CVS"
    exit 1
}
USE_GNOME2_MACROS=1 . gnome-autogen.sh

### Dirty lil hack #
patch -fp0 < gtk-doc.make.distcheck.patch

# TODO: Put this in a make target (stupid gtk-doc.make doesn't make this possible afaik)

./gentypes.sh $srcdir

exit 0

