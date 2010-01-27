AC_DEFUN([AC_TNY_WIN32_CHECK],
[

AC_MSG_CHECKING([for Win32])
case "$host" in
*-mingw*)
    os_win32=yes
    NO_UNDEFINED='-no-undefined'
    SOCKET_LIBS='-lws2_32 -ldnsapi'
    DL_LIB=''
    SOFTOKN3_LIB=''
    LIBEXECDIR_IN_SERVER_FILE='../../../libexec'
    ;;
*)  os_win32=no
    NO_UNDEFINED=''
    SOCKET_LIBS=''
    DL_LIB='-ldl'
    SOFTOKN3_LIB='-lsoftokn3'
    LIBEXECDIR_IN_SERVER_FILE="$libexecdir"
    ;;
esac
AC_MSG_RESULT([$os_win32])
AM_CONDITIONAL(OS_WIN32, [test $os_win32 = yes])
AC_SUBST(NO_UNDEFINED)
AC_SUBST(SOCKET_LIBS)
AC_SUBST(LIBEXECDIR_IN_SERVER_FILE)

])
