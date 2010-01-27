rm -rf tinymail
svn co https://svn.tinymail.org/svn/tinymail
cd tinymail/trunk
PKG_CONFIG_PATH=/opt/asyncworker/lib/pkgconfig/ ./autogen.sh --enable-gtk-doc --enable-tests --enable-asyncworker
make

