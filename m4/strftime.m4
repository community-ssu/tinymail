AC_DEFUN([AC_TNY_STRFTIME_CHECK],
[


dnl Check to see if strftime supports the use of %l and %k

AC_MSG_CHECKING(for %l and %k support in strftime)
AC_RUN_IFELSE([AC_LANG_SOURCE([[
#include <string.h>
#include <time.h>

int main(int argc, char **argv)
{
	char buf[10];
	time_t rawtime;
	struct tm *timeinfo;

	time(&rawtime);
	timeinfo=localtime(&rawtime);
	buf[0] = '\0';
	strftime(buf, 10, "%lx%k", timeinfo);

	if (buf[0] == '\0' || buf[0] == 'x' || strstr(buf, "l") || strstr(buf, "k"))
		exit(1);
	else
		exit(0);
}]])],[
AC_DEFINE(HAVE_LKSTRFTIME, 1, [strftime supports use of l and k])
ac_cv_lkstrftime=yes
],[ac_cv_lkstrftime=no],[ac_cv_lkstrftime=no])
AC_MSG_RESULT($ac_cv_lkstrftime)


])
