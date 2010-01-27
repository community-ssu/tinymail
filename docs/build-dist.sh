
# apt-get install check

svn co https://svn.tinymail.org/svn/tinymail

cd tinymail/trunk
./autogen.sh --enable-gtk-doc --enable-tests --enable-unit-tests
make
make distcheck

