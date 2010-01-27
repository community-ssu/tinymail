AC_DEFUN([AC_TNY_BRKSPOOL_CHECK],
[

AC_MSG_CHECKING(for SunOS broken spool format)
if test "x$host_os" = "xsunos" ; then
   with_broken_spool="yes"
fi

AC_ARG_WITH(broken-spool, 
[  --with-broken-spool=[yes/no] Using SunOS/Solaris sendmail which has a broken spool format],,with_broken_spool=${with_broken_spool:=no})

if test "x$with_broken_spool" = "xyes"; then
  AC_DEFINE(HAVE_BROKEN_SPOOL,1,[Define if mail delivered to the system mail directory is in broken Content-Length format])
fi

AC_MSG_RESULT($with_broken_spool)

])
