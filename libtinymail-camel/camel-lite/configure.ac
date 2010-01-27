AC_INIT([camel-lite],[1.0],[http://tinymail.org/])
AC_CONFIG_AUX_DIR(.)
AC_CONFIG_SRCDIR(camel/camel-object.c)
AM_INIT_AUTOMAKE(AC_PACKAGE_NAME, AC_PACKAGE_VERSION)

m4_pattern_allow([^AC_TNY_])

AC_CONFIG_MACRO_DIR([../../m4])
AC_SUBST(ACLOCAL_AMFLAGS, "$ACLOCAL_FLAGS -I ../../m4")

AC_CONFIG_HEADERS(config.h)

BASE_VERSION=1.0
API_VERSION=1.2
AC_SUBST(BASE_VERSION)
AC_SUBST(API_VERSION)
AC_DEFINE_UNQUOTED(BASE_VERSION, "$BASE_VERSION", [Base version (Major.Minor)])
AC_DEFINE_UNQUOTED(API_VERSION, "$API_VERSION", [Api version (Major.Minor)])

# Libtool versioning
LIBEDATASERVER_CURRENT=8
LIBEDATASERVER_REVISION=0
LIBEDATASERVER_AGE=1
LIBCAMEL_CURRENT=9
LIBCAMEL_REVISION=0
LIBCAMEL_AGE=1

AC_SUBST(LIBEDATASERVER_CURRENT)
AC_SUBST(LIBEDATASERVER_REVISION)
AC_SUBST(LIBEDATASERVER_AGE)
AC_SUBST(LIBCAMEL_CURRENT)
AC_SUBST(LIBCAMEL_REVISION)
AC_SUBST(LIBCAMEL_AGE)

AC_PATH_PROG(PKG_CONFIG, pkg-config, no)

AC_PROG_LIBTOOL
AC_ISC_POSIX
AC_C_INLINE
AC_HEADER_STDC([])
AC_PROG_INSTALL
AC_PROG_LN_S
AC_PROG_MAKE_SET
AC_SYS_LARGEFILE

GETTEXT_PACKAGE=camel-lite
AC_SUBST(GETTEXT_PACKAGE)
AC_DEFINE_UNQUOTED(GETTEXT_PACKAGE, "$GETTEXT_PACKAGE", [Package name for gettext])

AC_TNY_WIN32_CHECK

AC_CHECK_HEADERS(pthread.h semaphore.h sys/wait.h)
AC_CHECK_FUNCS(fsync strptime strtok_r)
AC_CHECK_HEADERS(alloca.h)

AC_CHECK_FUNCS(regexec,,[AC_CHECK_LIB(regex,regexec,
	      [REGEX_LIBS=-lregex
	       AC_DEFINE(HAVE_REGEXEC,1,[Define to 1 if you have the regexec function.])],
	      [AC_MSG_ERROR([No regex library found])])])
AC_SUBST(REGEX_LIBS)

AC_TNY_ICONV_CHECK
AC_TNY_STRFTIME_CHECK

msg_ssl="no"
msg_smime="no"
SSL_LIBS=""

mozilla_nspr_eds=""
mozilla_nss_eds=""

AC_TNY_MOZ_CHECK

dnl ### Debugging ###
AC_ARG_ENABLE(debug,
AC_HELP_STRING([--enable-debug],
  [Enable debugging (yes, no)]),
[case "${enableval}" in
  yes) debug=true ;;
  no)  debug=false ;;
esac],[debug=$debug])


dnl ### Select the SSL implementation ##
AC_ARG_WITH(ssl,
AC_HELP_STRING([--with-ssl],
  [Set which SSL library to use (nss, openssl, none)]),
  [case "$with_ssl" in

    nss)
	if test x"$mozilla_nspr" != "xno" -a x"$mozilla_nss" != "xno"; then
		mozilla_nss_eds=$mozilla_nss
		mozilla_nspr_eds=$mozilla_nspr
		msg_ssl="yes (Mozilla NSS)"
		msg_smime="yes (Mozilla NSS)"
		has_nss=yes
		AC_DEFINE(ENABLE_SMIME,1,[Define if SMIME should be enabled])
	        AC_DEFINE(HAVE_NSS,1,[Define if you have NSS])
        	AC_DEFINE(HAVE_SSL,1,[Define if you have a supported SSL library])
	        AC_DEFINE(HAVE_OPENSSL,0,[Define if you want to use OpenSSL as SSL library])
	else
		AC_MSG_ERROR([Mozilla NSS not found and must be installed])
 	fi
    ;;

    openssl) 
	    AC_CHECK_LIB(crypto, EVP_md5, [
                SSL_LIBS="-lcrypto -lssl"
                msg_ssl="yes (OpenSSL)"
		has_nss=no
                AC_DEFINE(HAVE_SSL,1,[Define if you have a supported SSL library])
                AC_DEFINE(HAVE_OPENSSL,1,[Define if you want to use OpenSSL as SSL library])
        	],,)
    ;;

    none) ;;

    *) AC_MSG_ERROR(bad value ${with_ssl} for --with-ssl) ;;

   esac], [
	if test x"$mozilla_nspr" != "xno" -a x"$mozilla_nss" != "xno"; then
		mozilla_nspr_eds=$mozilla_nspr
		mozilla_nss_eds=$mozilla_nss
                msg_ssl="yes (Mozilla NSS)"
                msg_smime="yes (Mozilla NSS)"
		has_nss=yes
                AC_DEFINE(ENABLE_SMIME,1,[Define if SMIME should be enabled])
                AC_DEFINE(HAVE_NSS,1,[Define if you have NSS])
                AC_DEFINE(HAVE_SSL,1,[Define if you have a supported SSL library])
                AC_DEFINE(HAVE_OPENSSL,0,[Define if you want to use OpenSSL as SSL library])
        fi
   ])

want_cst=no
AC_ARG_ENABLE(cst,
[  --enable-cst=[no/yes]      Build with libcst support, incomplete and not supported yet],want_cst=$enableval,,)

if test "x$want_cst" = "xyes"; then
   if test x$has_nss = xyes; then
        AC_MSG_CHECKING(Support for libCST)
     	PKG_CHECK_MODULES(LIBCST, cst, have_cst=yes, have_cst=no)
     	AC_MSG_RESULT($have_cst)
   else
	LIBCST_CFLAGS=
     	LIBCST_LIBS=
     	have_cst=no
   fi
else
   have_cst=no
fi

AM_CONDITIONAL([HAVE_CST],[test x"$have_cst" == "xyes"])
if test x"$have_cst" == "xyes"; then
	AC_DEFINE(ENABLE_CST, [], [Enable libcst])
fi
AC_SUBST(LIBCST_CFLAGS)
AC_SUBST(LIBCST_LIBS)

AC_SUBST(msg_smime)
AC_SUBST(msg_ssl)
AC_SUBST(SSL_LIBS)
AC_MSG_CHECKING(Support for SSL)
AC_MSG_RESULT($msg_ssl $SSL_LIBS)
AC_MSG_CHECKING(Support for SMIME)
AC_MSG_RESULT($msg_smime)

AM_CONDITIONAL(ENABLE_SMIME, test "x$msg_smime" != "xno")
AM_CONDITIONAL(ENABLE_OPENSSL, [test "x$msg_ssl" == "xyes (OpenSSL)"])

AC_TNY_SYSMAIL_CHECK
AC_TNY_TIMEZONE_CHECK

AC_CHECK_FUNCS(mkstemp mkdtemp isblank)

AC_TNY_GETHOSTBYNAME_CHECK
AC_TNY_GETHOSTBYADDR_CHECK

dnl **************************************************
dnl stat(v)fs location/type
dnl **************************************************

AC_CHECK_HEADERS(sys/statvfs.h)
AC_CHECK_FUNCS(statvfs)
AC_CHECK_HEADERS(sys/param.h)
AC_CHECK_HEADERS(sys/mount.h)
AC_CHECK_FUNCS(statfs)

AC_TNY_IPV6_CHECK

dnl **************************************************
dnl NNTP support.
dnl **************************************************
AC_ARG_ENABLE(nntp, 
[  --enable-nntp=[no/yes]      Build Usenet news (NNTP) backend],,enable_nntp=yes)
if test "x$enable_nntp" = "xyes"; then
	AC_DEFINE(ENABLE_NNTP,1,[Build NNTP backend])
	msg_nntp=yes
else
	msg_nntp=no
fi
AM_CONDITIONAL(ENABLE_NNTP, test x$enable_nntp = xyes)

dnl **************************************************
dnl New IMAP code support.
dnl **************************************************
AC_ARG_ENABLE(imapp, 
[  --enable-imapp=[no/yes]      Attempt to compile alternative, incomplete, very unsupported IMAPv4r1 code],,enable_imapp=no)
if test "x$enable_imapp" = "xyes"; then
	AC_DEFINE(ENABLE_IMAPP,1,[Really don't try this at home])
	msg_imapp=yes
else
	msg_imapp=no
fi
AM_CONDITIONAL(ENABLE_IMAPP, test x$enable_imapp = xyes)

dnl **************************************************
dnl IMAP IDLE support.
dnl **************************************************
AC_ARG_ENABLE(idle, 
[  --disable-idle      Disable IMAP IDLE support],
[ case "$enableval" in
  no)
    AC_DEFINE(DISABLE_IDLE,1,[do not use IMAP IDLE]);;
  *)
    ;;
  esac ], )


AC_TNY_FILELOCK_CHECK
AC_TNY_BRKSPOOL_CHECK
AC_TNY_KERBEROS_CHECK

GLIB_REQUIRED="2.4.0"

AC_SUBST(GLIB_REQUIRED)

dnl We use AC_SUBST_FILE because AC_SUBST won't deal with newlines
EVO_MARSHAL_RULE=$srcdir/marshal.mk
AC_SUBST_FILE(EVO_MARSHAL_RULE)

AC_DEFINE(ENABLE_THREADS,1,[Required])


CFLAGS="$CFLAGS $WARN_CFLAGS"

PKG_CHECK_MODULES(E_DATA_SERVER, libxml-2.0 glib-2.0 gobject-2.0 $mozilla_nspr_eds $mozilla_nss_eds)
AC_SUBST(E_DATA_SERVER_CFLAGS)
AC_SUBST(E_DATA_SERVER_LIBS)


zlib_found=false
dnl deflateInit is a #define, use deflateEnd instead
AC_CHECK_LIB(z, deflateEnd,
    [ AC_CHECK_HEADER(zlib.h, zlib_found=true) ])                                                                                                                                      
if test "x$zlib_found" = "xfalse"; then
    AC_MSG_ERROR([*** zlib is required])
fi

PKG_CHECK_MODULES(CAMEL, gthread-2.0 gmodule-2.0 gobject-2.0 $mozilla_nss_eds $mozilla_nspr_eds)
CAMEL_CFLAGS="$CAMEL_CFLAGS $LIBCST_CFLAGS $THREADS_CFLAGS $KRB4_CFLAGS $KRB5_CFLAGS"
CAMEL_LIBS="$CAMEL_LIBS -lz $LIBCST_LIBS $THREADS_LIBS $KRB4_LDFLAGS $KRB5_LDFLAGS"

if test x$debug == xtrue; then
	CAMEL_CFLAGS="$CAMEL_CFLAGS -ggdb -O0 -DDEBUG"
fi

AC_SUBST(CAMEL_CFLAGS)
AC_SUBST(CAMEL_LIBS)

camel_lite_providerdir='${libdir}'/camel-lite-$API_VERSION/camel-providers
camel_providerdir=$camel_lite_providerdir
AC_SUBST(camel_lite_providerdir)
AC_SUBST(camel_providerdir)

AC_OUTPUT([
Makefile
bs/Makefile
camel/Makefile
camel/providers/Makefile
camel/providers/imap/Makefile
camel/providers/imapp/Makefile
camel/providers/local/Makefile
camel/providers/nntp/Makefile
camel/providers/pop3/Makefile
camel/providers/sendmail/Makefile
camel/providers/smtp/Makefile
camel/camel-lite.pc
camel/camel-lite-provider.pc
libedataserver/Makefile
summary/Makefile
])

