AC_DEFUN([AC_TNY_TIMEZONE_CHECK],
[

AC_CACHE_CHECK(for tm_gmtoff in struct tm, ac_cv_struct_tm_gmtoff,
	AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[
		#include <time.h>
		]], [[
		struct tm tm;
		tm.tm_gmtoff = 1;
		]])],[ac_cv_struct_tm_gmtoff=yes],[ac_cv_struct_tm_gmtoff=no]))
if test $ac_cv_struct_tm_gmtoff = yes; then
	AC_DEFINE(HAVE_TM_GMTOFF, 1, [Define if struct tm has a tm_gmtoff member])
else
	AC_CACHE_CHECK(for timezone variable, ac_cv_var_timezone,
		AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[
			#include <time.h>
		]], [[
			timezone = 1;
		]])],[ac_cv_var_timezone=yes],[ac_cv_var_timezone=no]))
	if test $ac_cv_var_timezone = yes; then
		AC_DEFINE(HAVE_TIMEZONE, 1, [Define if libc defines a timezone variable])
		AC_CACHE_CHECK(for altzone variable, ac_cv_var_altzone,
			AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[
				#include <time.h>
			]], [[
				altzone = 1;
			]])],[ac_cv_var_altzone=yes],[ac_cv_var_altzone=no]))
		if test $ac_cv_var_altzone = yes; then
			AC_DEFINE(HAVE_ALTZONE, 1, [Define if libc defines an altzone variable])
		fi
	else
		AC_MSG_ERROR([unable to find a way to determine timezone])
	fi
fi

])
