# $Id: Makefile.am 1108 2002-07-04 10:37:49Z zwiebeltu $

INCLUDES = @JIBAPIINC@
#needed_libs = -L$(top_srcdir)/sio/.libs -L$(top_srcdir)/sio -lsio

lib_LTLIBRARIES = libscez_readers.la

# (scbluedot disabled due to compile errors in it)
libscez_readers_la_SOURCES = \
	scacr20.c \
	scdumbmouse.c \
	scgcr400.c \
	scgpr400.c \
	scintertex.c \
	screflex60.c \
	sctowitoko.c

libscez_readers_la_LDFLAGS = -version-info 1:0:0

EXTRA_DIST = \
	scacr20.h \
	scdumbmouse.h \
	scgcr400.h \
	scgpr400.h \
	scintertex.h \
	screflex60.h \
	sctowitoko.h
