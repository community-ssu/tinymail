AC_DEFUN([AC_TNY_FILELOCK_CHECK],
[

AC_ARG_ENABLE(dot-locking, 
[  --enable-dot-locking=[yes/no] Enable support for locking mail files with dot locking],,enable_dot_locking=yes)
if test $os_win32 != yes -a "x$enable_dot_locking" = "xyes"; then
  AC_DEFINE(USE_DOT,1,[Define to use dot locking for mbox files])
  msg_dot=yes
else
  msg_dot=no	
fi

AC_ARG_ENABLE(file-locking, 
[  --enable-file-locking=[fcntl/flock/no] Enable support for locking mail files with file locking],,enable_file_locking=fcntl)
if test $os_win32 != yes -a "x$enable_file_locking" = "xfcntl"; then
  AC_DEFINE(USE_FCNTL,1,[Define to use fcntl locking for mbox files])
  msg_file=fcntl
else
  if test $os_win32 != yes -a "x$enable_file_locking" = "xflock"; then
    AC_DEFINE(USE_FLOCK,1,[Define to use flock locking for mbox files])
    msg_file=flock
  else
    msg_file=no	
  fi
fi

])
