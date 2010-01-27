AC_DEFUN([AC_TNY_PYTHON_BINDING_CHECK],
[
  AM_PATH_PYTHON(2.3)
  AC_PATH_PROG(PYTHON, python, no)
  if test x$PYTHON = xno; then
       AC_MSG_ERROR(Please install python)
  fi
  PYGTK_CODEGENDIR="`$PKG_CONFIG --variable=codegendir pygtk-2.0`"

  changequote(<<, >>)dnl
  PY_VER=`$PYTHON -c 'import distutils.sysconfig; print distutils.sysconfig.get_config_vars("VERSION")[0];'`
  PY_LIB=`$PYTHON -c 'import distutils.sysconfig; print distutils.sysconfig.get_python_lib(standard_lib=1);'`
  PY_INC=`$PYTHON -c 'import distutils.sysconfig; print distutils.sysconfig.get_config_vars("INCLUDEPY")[0];'`
  PY_PREFIX=`$PYTHON -c 'import sys; print sys.prefix'`
  PY_EXEC_PREFIX=`$PYTHON -c 'import sys; print sys.exec_prefix'`
  changequote([, ])dnl
  if test -f $PY_INC/Python.h; then
    PYTHON_LIBS="-L$PY_LIB/config -lpython$PY_VER -lpthread -lutil"
    PYTHON_CFLAGS="-I$PY_INC"
  else
    AC_MSG_ERROR([Can't find Python.h])
  fi

  AC_PATH_PROG(PYGTK_CODEGEN, pygtk-codegen-2.0, no)
  if test "x$PYGTK_CODEGEN" = xno; then
	AC_MSG_ERROR(could not find pygtk-codegen-2.0 script)
  fi
  AC_MSG_CHECKING(for pygtk defs)
  PYGTK_DEFSDIR=`$PKG_CONFIG --variable=defsdir pygtk-2.0`
  
  AC_SUBST(PYGTK_DEFSDIR)
  AC_MSG_RESULT($PYGTK_DEFSDIR)

  PKG_CHECK_MODULES(PYGTK, pygtk-2.0 >= 2.8)
  PKG_CHECK_MODULES(TINYMAIL_PYTHON, pygobject-2.0 pygtk-2.0)
  TINYMAIL_PYTHON_CFLAGS="$TINYMAIL_PYTHON_CFLAGS $PYTHON_CFLAGS"
  TINYMAIL_PYTHON_LIBS="$TINYMAIL_PYTHON_LIBS $PYTHON_LIBS"
])
