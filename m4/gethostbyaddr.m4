AC_DEFUN([AC_TNY_GETHOSTBYADDR_CHECK],
[

AC_CHECK_FUNCS(gethostbyaddr_r,[
AC_CACHE_CHECK([if gethostbyaddr_r wants seven arguments], ac_cv_gethostbyaddr_r_seven_args,
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

		(void)gethostbyaddr_r ("www.ximian.com", 14, AF_INET, &hent, buffer, bufsize, &h_errno);
	]])],[
		ac_cv_gethostbyaddr_r_seven_args=yes
	],[
		ac_cv_gethostbyaddr_r_seven_args=no
	])
])])
	
if test "x$ac_cv_gethostbyaddr_r_seven_args" = "xyes" ; then
	AC_DEFINE(GETHOSTBYADDR_R_SEVEN_ARGS, 1, [Solaris-style gethostbyaddr_r])
fi

])
