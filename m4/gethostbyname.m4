AC_DEFUN([AC_TNY_GETHOSTBYNAME_CHECK],
[

AC_CHECK_FUNCS(gethostbyname_r,[
AC_CACHE_CHECK([if gethostbyname_r wants five arguments], ac_cv_gethostbyname_r_five_args,
[
	AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[
		#include "confdefs.h"
		#include <sys/types.h>
		#include <sys/socket.h>
		#include <netinet/in.h>
		#include <netdb.h>

		#define BUFSIZE (sizeof(struct hostent)+10)
	]], [[
		struct hostent hent;
		char buffer[BUFSIZE];
		int bufsize=BUFSIZE;
		int h_errno;

		(void)gethostbyname_r ("www.ximian.com", &hent, buffer, bufsize, &h_errno);
	]])],[
		ac_cv_gethostbyname_r_five_args=yes
	],[
		ac_cv_gethostbyname_r_five_args=no
	])
])])
	
if test "x$ac_cv_gethostbyname_r_five_args" = "xyes" ; then
	AC_DEFINE(GETHOSTBYNAME_R_FIVE_ARGS, 1, [Solaris-style gethostbyname_r])
fi

])
