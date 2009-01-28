# $Id: Makefile.am 1617 2005-11-03 17:41:39Z laforge $

INCLUDES = @JIBAPIINC@
#needed_libs = -L$(top_srcdir)/sio/.libs -L$(top_srcdir)/sio -lsio
needed_libs = ../sio/libsio.la readers/libscez_readers.la cards/libscez_cards.la

lib_LTLIBRARIES = libscez.la

libscez_la_SOURCES = \
	scctapi.c \
	scgeneral.c \
	scpts.c \
	screader.c \
	scsmartcard.c \
	sct0.c \
	sct1.c

libscez_la_LIBADD = $(needed_libs) #@LTLIBOBJS@

libscez_la_LDFLAGS = -version-info 1:0:0

EXTRA_DIST = \
	scctapi.h \
	scgeneral.h \
	scinternal.h \
	scmacros.h \
	scpts.h \
	screader.h \
	scsmartcard.h \
	sct0.h \
	sct1.h 
