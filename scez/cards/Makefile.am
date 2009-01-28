# $Id: Makefile.am 1108 2002-07-04 10:37:49Z zwiebeltu $

INCLUDES = @JIBAPIINC@
#needed_libs = -L$(top_srcdir)/sio/.libs -L$(top_srcdir)/sio -lsio

lib_LTLIBRARIES = libscez_cards.la

libscez_cards_la_SOURCES = \
	scbasiccard.c \
	sccarteb.c \
	sccryptoflex.c \
	sccyberflex.c \
	sceasycheck.c \
	scgeldkarte.c \
	scgpk4000.c \
	scgsmsim.c \
	scmfc.c \
	scmultiflex.c \
	scopenplatform.c \
	scpayflex.c \
	scproton.c \
	scquick.c \
	scsmartcafe.c \
	scstarcos.c \
	sctcos.c

libscez_cards_la_LDFLAGS = -version-info 1:0:0

EXTRA_DIST = \
	scbasiccard.h \
	sccarteb.h \
	sccryptoflex.h \
	sccyberflex.h \
	sceasycheck.h \
	scgeldkarte.h \
	scgpk4000.h \
	scgsmsim.h \
	scmfc.h \
	scmultiflex.h \
	scopenplatform.h \
	scpayflex.h \
	scproton.h \
	scquick.h \
	scsmartcafe.h \
	scstarcos.h \
	sctcos.h

