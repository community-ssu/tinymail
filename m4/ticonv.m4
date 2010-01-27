AC_DEFUN([AC_TNY_ICONV_CHECK],
[

have_iconv=no
AC_ARG_WITH(libiconv, [  --with-libiconv         Prefix where libiconv is installed])
case $withval in
/*)
    ICONV_CFLAGS="-I$withval/include"
    ICONV_LIBS="-L$withval/lib"
    ;;
esac

save_CFLAGS="$CFLAGS"
save_LIBS="$LIBS"
CFLAGS="$CFLAGS $ICONV_CFLAGS"
LIBS="$LIBS $ICONV_LIBS -liconv"
AC_CACHE_CHECK(for iconv in -liconv, ac_cv_libiconv, AC_LINK_IFELSE([AC_LANG_PROGRAM([[
#include <iconv.h>
]], [[
	iconv_t cd;
	cd = iconv_open ("UTF-8", "ISO-8859-1");
]])],[ac_cv_libiconv=yes],[ac_cv_libiconv=no]))
if test $ac_cv_libiconv = yes; then
	ICONV_LIBS="$ICONV_LIBS -liconv"
	have_iconv=yes
else
	CFLAGS="$save_CFLAGS"
	LIBS="$save_LIBS"
	AC_CHECK_FUNC(iconv, have_iconv=yes, have_iconv=no)
fi

if test $have_iconv = yes; then
	if test $ac_cv_libiconv = no; then
		AC_CHECK_FUNCS(gnu_get_libc_version)
	fi
	AC_CACHE_CHECK([if iconv() handles UTF-8], ac_cv_libiconv_utf8, AC_RUN_IFELSE([AC_LANG_SOURCE([[
#include <iconv.h>
#include <stdlib.h>
#include <string.h>
#ifdef HAVE_GNU_GET_LIBC_VERSION
#include <gnu/libc-version.h>
#endif

int main (int argc, char **argv)
{
	const char *from = "Some Text \xA4";
	const char *utf8 = "Some Text \xC2\xA4";
	char *transbuf = (char *) malloc (20), *trans = transbuf;
	iconv_t cd;
	size_t from_len = strlen (from), utf8_len = 20;
	size_t utf8_real_len = strlen (utf8);

#ifdef HAVE_GNU_GET_LIBC_VERSION
	/* glibc 2.1.2's iconv is broken in hard to test ways. */
	if (!strcmp (gnu_get_libc_version (), "2.1.2"))
		return (1);
#endif

	cd = iconv_open ("UTF-8", "ISO-8859-1");
	if (cd == (iconv_t) -1)
		return (1);
	if (iconv (cd, &from, &from_len, &trans, &utf8_len) == -1 || from_len != 0)
		return (1);
	if (memcmp (utf8, transbuf, utf8_real_len) != 0)
		return (1);

	return (0);
}]])],[ac_cv_libiconv_utf8=yes],[ac_cv_libiconv_utf8=no; have_iconv=no],[ac_cv_libiconv_utf8=no; have_iconv=no]))
fi

if test "$have_iconv" = no; then
	AC_MSG_ERROR([You need to install a working iconv implementation, such as ftp://ftp.gnu.org/pub/gnu/libiconv])
fi
AC_SUBST(ICONV_CFLAGS)
AC_SUBST(ICONV_LIBS)

CFLAGS="$CFLAGS -I$srcdir"

AC_ARG_WITH([iconv-detect-h],
	AC_HELP_STRING([--with-iconv-detect-h],
	[Use a hand generated iconv-detect.h rather than running iconv-detect (in libtinymail-camel/camel-lite)]),
	[cp $withval iconv-detect.h],
	AC_MSG_CHECKING(preferred charset formats for system iconv)
	AC_RUN_IFELSE([AC_LANG_SOURCE([[
#define CONFIGURE_IN
#include "iconv-detect.c"
	]])],[
		AC_MSG_RESULT(found)
	],[
		AC_MSG_RESULT(not found)
	],[])

	CFLAGS="$save_CFLAGS"
	LIBS="$save_LIBS"
	)
])
