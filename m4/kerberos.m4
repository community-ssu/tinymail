AC_DEFUN([AC_TNY_KERBEROS_CHECK],
[

AC_ARG_WITH(krb5, [  --with-krb5=DIR      Location of Kerberos 5 install dir], with_krb5="$withval", with_krb5="no")
AC_ARG_WITH(krb5-libs, [  --with-krb5-libs=DIR Location of Kerberos 5 libraries], with_krb5_libs="$withval", with_krb5_libs="$with_krb5/lib")
AC_ARG_WITH(krb5-includes, [  --with-krb5-includes=DIR Location of Kerberos 5 headers], with_krb5_includes="$withval", with_krb5_includes="")
AC_ARG_WITH(krb4, [  --with-krb4=DIR      Location of Kerberos 4 install dir], with_krb4="$withval", with_krb4="no")
AC_ARG_WITH(krb4-libs, [  --with-krb4-libs=DIR Location of Kerberos 4 libraries], with_krb4_libs="$withval", with_krb4_libs="$with_krb4/lib")
AC_ARG_WITH(krb4-includes, [  --with-krb4-includes=DIR Location of Kerberos 4 headers], with_krb4_includes="$withval", with_krb4_includes="")

msg_krb5="no"
if test "x${with_krb5}" != "xno"; then
	LDFLAGS_save="$LDFLAGS"
	
	mitlibs="-lkrb5 -lk5crypto -lcom_err -lgssapi_krb5"
	heimlibs="-lkrb5 -lcrypto -lasn1 -lcom_err -lroken -lgssapi"
	AC_CACHE_CHECK([for Kerberos 5], ac_cv_lib_kerberos5,
	[
		LDFLAGS="$LDFLAGS -L$with_krb5_libs $mitlibs"
		AC_TRY_LINK_FUNC(krb5_init_context, ac_cv_lib_kerberos5="$mitlibs",
		[
			LDFLAGS="$LDFLAGS_save -L$with_krb5_libs $heimlibs"
			AC_TRY_LINK_FUNC(krb5_init_context, ac_cv_lib_kerberos5="$heimlibs", ac_cv_lib_kerberos5="no")
		])
		LDFLAGS="$LDFLAGS_save"
	])
	if test "$ac_cv_lib_kerberos5" != "no"; then
		AC_DEFINE(HAVE_KRB5,1,[Define if you have Krb5])
		if test "$ac_cv_lib_kerberos5" = "$mitlibs"; then
			AC_DEFINE(HAVE_MIT_KRB5,1,[Define if you have MIT Krb5])
			if test -z "$with_krb5_includes"; then
				KRB5_CFLAGS="-I$with_krb5/include"
			else
				KRB5_CFLAGS="-I$with_krb5_includes"
			fi
			msg_krb5="yes (MIT)"
		else
			AC_DEFINE(HAVE_HEIMDAL_KRB5,1,[Define if you have Heimdal])
			if test -z "$with_krb5_includes"; then
				KRB5_CFLAGS="-I$with_krb5/include/heimdal"
			else
				KRB5_CFLAGS="-I$with_krb5_includes"
			fi
			msg_krb5="yes (Heimdal)"
		fi
		KRB5_LDFLAGS="-L$with_krb5_libs $ac_cv_lib_kerberos5"
	fi
else
	AC_MSG_CHECKING(for Kerberos 5)
	AC_MSG_RESULT($with_krb5)
fi
AM_CONDITIONAL(ENABLE_KRB5, test x$with_krb5 != xno)

AC_CHECK_HEADER([et/com_err.h],[AC_DEFINE([HAVE_ET_COM_ERR_H], 1, [Have et/comm_err.h])])
AC_CHECK_HEADER([com_err.h],[AC_DEFINE([HAVE_COM_ERR_H], 1, [Have comm_err.h])])

msg_krb4="no"
if test "x${with_krb4}" != "xno"; then
	LDFLAGS_save="$LDFLAGS"
	AC_CACHE_CHECK(for Kerberos 4, ac_cv_lib_kerberos4,
	[
		ac_cv_lib_kerberos4="no"

		mitcompatlibs="-lkrb4 -ldes425 -lkrb5 -lk5crypto -lcom_err"
		# Look for MIT krb5 compat krb4
		LDFLAGS="$LDFLAGS -L$with_krb4_libs $mitcompatlibs"
		AC_TRY_LINK_FUNC(krb_mk_req, ac_cv_lib_kerberos4="$mitcompatlibs")
		
		if test "$ac_cv_lib_kerberos4" = "no"; then
			# Look for KTH krb4
			LDFLAGS="$LDFLAGS_save -L$with_krb4_libs -lkrb -lcrypto -lcom_err -lroken"
			AC_TRY_LINK_FUNC(krb_mk_req, ac_cv_lib_kerberos4="-lkrb -lcrypto -lcom_err -lroken")
		fi
		if test "$ac_cv_lib_kerberos4" = "no"; then
			# Look for old MIT krb4
			LDFLAGS="$LDFLAGS_save -L$with_krb4_libs -lkrb"
			AC_TRY_LINK_FUNC(krb_mk_req, ac_cv_lib_kerberos4="-lkrb",
			[
				LDFLAGS="$LDFLAGS -ldes"
				AC_TRY_LINK_FUNC(krb_mk_req, ac_cv_lib_kerberos4="-lkrb -ldes")
			])
		fi
	])
	LDFLAGS="$LDFLAGS_save"
	if test "$ac_cv_lib_kerberos4" != "no"; then
		AC_DEFINE(HAVE_KRB4,1,[Define if you have Krb4])
		msg_krb4="yes"

		if test -z "$with_krb4_includes"; then
			if test -f "$with_krb4/include/krb.h" -o -f "$with_krb4/include/port-sockets.h"; then
				KRB4_CFLAGS="-I$with_krb4/include"
			fi
			if test -d "$with_krb4/include/kerberosIV"; then
				KRB4_CFLAGS="$KRB4_CFLAGS -I$with_krb4/include/kerberosIV"
			fi
		else
			KRB4_CFLAGS="-I$with_krb4_includes"
		fi
		KRB4_LDFLAGS="-L$with_krb4_libs $ac_cv_lib_kerberos4"
		
		CFLAGS_save="$CFLAGS"
		CFLAGS="$CFLAGS $KRB4_CFLAGS"
		AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[#include "krb.h"
		int krb_sendauth;
		]], [[return 0]])],[AC_DEFINE(NEED_KRB_SENDAUTH_PROTO,1,[Need krb_sendauth proto])],[])
		CFLAGS="$CFLAGS_save"
	fi
else
	AC_MSG_CHECKING(for Kerberos 4)
	AC_MSG_RESULT(${with_krb4})
fi

AC_SUBST(KRB5_CFLAGS)
AC_SUBST(KRB5_LDFLAGS)
AC_SUBST(KRB4_CFLAGS)
AC_SUBST(KRB4_LDFLAGS)

])
