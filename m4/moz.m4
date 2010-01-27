AC_DEFUN([AC_TNY_MOZ_CHECK],
[

AC_MSG_CHECKING(Mozilla nspr pkg-config module)
AC_ARG_WITH([mozilla-nspr-pc],
  AS_HELP_STRING([--with-mozilla-nspr-pc=NAME], [name of nspr pkg-config module]),
      [mozilla_nspr=$with_mozilla_nspr_pc],
      [mozilla_nspr="no"
       for pc in nspr mozilla-nspr firefox-nspr xulrunner-nspr microb-engine-nspr; do
           if $PKG_CONFIG --exists $pc; then
               mozilla_nspr=$pc
               break
           fi
       done])
AC_MSG_RESULT($mozilla_nspr)

AC_MSG_CHECKING(Mozilla nss pkg-config module)
AC_ARG_WITH([mozilla-nss-pc],
  AS_HELP_STRING([--with-mozilla-nss-pc=NAME], [name of nss pkg-config module]),
      [mozilla_nss=$with_mozilla_nss_pc],
      [mozilla_nss="no"
       for pc in nss mozilla-nss firefox-nss xulrunner-nss microb-engine-nss; do
           if $PKG_CONFIG --exists $pc; then
               mozilla_nss=$pc
               break
           fi
       done])
AC_MSG_RESULT($mozilla_nss)

AC_MSG_CHECKING(Mozilla xpcom pkg-config module)
AC_ARG_WITH([mozilla-xpcom-pc],
  AS_HELP_STRING([--with-mozilla-xpcom-pc=NAME], [name of xpcom pkg-config module]),
      [mozilla_xpcom=$with_mozilla_xpcom_pc],
      [mozilla_xpcom="no"
       for pc in libxul-embedding xpcom mozilla-xpcom firefox-xpcom xulrunner-xpcom microb-engine-xpcom; do
           if $PKG_CONFIG --exists $pc; then
               mozilla_xpcom=$pc
               break
           fi
       done])
AC_MSG_RESULT($mozilla_xpcom)

AC_MSG_CHECKING(Mozilla home)
mozilla_home="no"
if test x$mozilla_xpcom != xno; then
    mozilla_home="`$PKG_CONFIG --variable=libdir $mozilla_xpcom`"
fi
AC_MSG_RESULT($mozilla_home)

AC_MSG_CHECKING(Mozilla gtkmozembed pkg-config module)
AC_ARG_WITH([mozilla-gtkmozembed-pc],
  AS_HELP_STRING([--with-mozilla-gtkmozembed-pc=NAME], [name of gtkmozembed pkg-config module]),
      [mozilla_gtkmozembed=$with_mozilla_gtkmozembed_pc],
      [mozilla_gtkmozembed="no"
       for pc in libxul-embedding gtkmozembed mozilla-gtkmozembed firefox-gtkmozembed xulrunner-gtkmozembed microb-engine-gtkembedmoz gtkembedmoz; do
           if $PKG_CONFIG --exists $pc; then
               mozilla_gtkmozembed=$pc
               break
           fi
       done])
AC_MSG_RESULT($mozilla_gtkmozembed)

#Detect Mozilla XPCom version
if test x$mozilla_xpcom != xno; then

   AC_MSG_CHECKING(Mozilla xpcom engine version)
   #The only real way to detect the MOZILLA engine version is using the version in mozilla-config.h
   #of the engine we use.
   mozilla_xpcom_includedir="`$PKG_CONFIG --variable=includedir $mozilla_xpcom`"
   mozilla_xpcom_includetype="unstable"

   # append /stable or /unstable for mozilla >= 1.9
   if test -d $mozilla_xpcom_includedir/$mozilla_xpcom_includetype; then
       mozilla_xpcom_includedir="$mozilla_xpcom_includedir/$mozilla_xpcom_includetype"
   fi
   if test -f $mozilla_xpcom_includedir/mozilla-config.h
   then
    mozilla_version=`cat $mozilla_xpcom_includedir/mozilla-config.h | grep MOZILLA_VERSION_U | sed "s/.*_VERSION_U\ //"|tr ".abpre+" " "`
    mozilla_major_version=`echo $mozilla_version | awk ' { print $[1]; } '`
    mozilla_minor_version=`echo $mozilla_version | awk ' { print $[2]; } '`
   else
     mozilla_version=1.8
     mozilla_major_version=1
     mozilla_minor_version=8
   fi

   if test "$mozilla_major_version" != "1"; then
     AC_DEFINE([HAVE_MOZILLA_1_9],[1],[Define if we have mozilla api 1.9])
     AC_DEFINE([HAVE_MOZILLA_1_8],[1],[Define if we have mozilla api 1.8])
   fi

   if test "$mozilla_major_version" = "1" -a "$mozilla_minor_version" -ge "8"; then
     AC_DEFINE([HAVE_MOZILLA_1_8],[1],[Define if we have mozilla api 1.8])
   fi
      
   if test "$mozilla_major_version" = "1" -a "$mozilla_minor_version" -ge "9"; then
     AC_DEFINE([HAVE_MOZILLA_1_9],[1],[Define if we have mozilla api 1.9])
   fi

   AC_MSG_RESULT($mozilla_version)

fi


AM_CONDITIONAL([HAVE_MOZILLA_1_8],[test "$mozilla_major_version" = "1" -a "$mozilla_minor_version" -ge "8"])
AM_CONDITIONAL([HAVE_MOZILLA_1_9],[test "$mozilla_major_version" = "1" -a "$mozilla_minor_version" -ge "9"])
AC_DEFINE([MOZEMBED_MOZILLA_VERSION],"$mozilla_version",[Detected mozilla engine version for mozembed])

])

dnl vim:syntax=m4
