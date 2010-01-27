AC_DEFUN([AC_TNY_SYSMAIL_CHECK],
[

AC_PATH_PROG(SENDMAIL, sendmail, /usr/sbin/sendmail, /usr/sbin:/usr/lib)
AC_DEFINE_UNQUOTED(SENDMAIL_PATH, "$SENDMAIL", [Path to a sendmail binary, or equivalent])

AC_MSG_CHECKING(system mail directory)
if test -d /var/mail -a '!' -h /var/mail ; then
	system_mail_dir=/var/mail
else
	system_mail_dir=/var/spool/mail
fi
AC_DEFINE_UNQUOTED(SYSTEM_MAIL_DIR, "$system_mail_dir", [Directory local mail is delivered to])

case `ls -ld $system_mail_dir 2>&1 | awk '{print $1;}'` in
d??????rw?)
	CAMEL_LOCK_HELPER_USER=""
	CAMEL_LOCK_HELPER_GROUP=""
	system_mail_perm="world writable"
	;;

d???rw????)
	CAMEL_LOCK_HELPER_USER=""
	CAMEL_LOCK_HELPER_GROUP=`ls -ld $system_mail_dir 2>&1 | awk '{print $4;}'`
	system_mail_perm="writable by group $CAMEL_LOCK_HELPER_GROUP"
	;;

drw???????)
	CAMEL_LOCK_HELPER_USER=`ls -ld $system_mail_dir 2>&1 | awk '{print $3;}'`
	CAMEL_LOCK_HELPER_GROUP=""
	system_mail_perm="writable by user $CAMEL_LOCK_HELPER_USER"
	;;

*)
	CAMEL_LOCK_HELPER_USER=""
	CAMEL_LOCK_HELPER_GROUP=""
	system_mail_perm="???"
	;;
esac

AC_MSG_RESULT([$system_mail_dir, $system_mail_perm])
AC_SUBST(CAMEL_LOCK_HELPER_USER)
AC_SUBST(CAMEL_LOCK_HELPER_GROUP)

])
