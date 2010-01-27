## Process this file with automake to produce Makefile.in


SUBDIRS = pop3 smtp imap local  

if ENABLE_NNTP
SUBDIRS += nntp 
endif

if ENABLE_IMAPP
SUBDIRS += imapp 
endif


if OS_WIN32
else
SUBDIRS += sendmail 
endif


