AC_DEFUN([AC_TNY_IPV6_CHECK],
[

AC_CACHE_CHECK([if system supports getaddrinfo and getnameinfo], have_addrinfo,
[
	AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[
		#include "confdefs.h"
		#include <sys/types.h>
		#include <sys/socket.h>
		#include <netinet/in.h>
		#include <netdb.h>
		#include <stddef.h>

	]], [[
		struct addrinfo hints, *res;
		struct sockaddr_in6 sin6;
		int af = AF_INET6;
		char host[NI_MAXHOST];
		char serv[NI_MAXSERV];

		getaddrinfo ("www.ximian.com", NULL, &hints, &res);
		freeaddrinfo (res);
		getnameinfo((struct sockaddr *)&sin6, sizeof(sin6), host, sizeof(host), serv, sizeof(serv), 0);
	]])],[
		have_addrinfo=yes
	],[
		have_addrinfo=no
	])
])

if test x"$have_addrinfo" = "xno" ; then
   AC_DEFINE(NEED_ADDRINFO,1,[Enable getaddrinfo emulation])
   if test x"$enable_ipv6" = "xyes" ; then
      AC_MSG_ERROR([system doesn't support necessary interfaces for ipv6 support])
   fi
   msg_ipv6=no
else
   AC_ARG_ENABLE(ipv6, [  --enable-ipv6=[no/yes]      Enable support for resolving IPv6 addresses.],,enable_ipv6=yes)
   if test x"$enable_ipv6" = "xyes"; then
        msg_ipv6=yes
	AC_DEFINE(ENABLE_IPv6,1,[Enable IPv6 support])
	AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[
		#include "confdefs.h"
		#include <sys/types.h>
		#include <sys/socket.h>
		#include <netinet/in.h>
		#include <netdb.h>

	]], [[
		struct addrinfo hints;
		
		hints.ai_flags = AI_ADDRCONFIG;
	]])],[
		AC_DEFINE(HAVE_AI_ADDRCONFIG,1,[Define if the system defines the AI_ADDRCONFIG flag for getaddrinfo])
	],[])
   else
	msg_ipv6=no
   fi
fi

AM_CONDITIONAL(ENABLE_IPv6, test "x$enable_ipv6" = "xyes")

])
