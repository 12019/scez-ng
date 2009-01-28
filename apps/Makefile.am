# $Id: Makefile.am 1093 2002-03-22 12:42:30Z zwiebeltu $

if WITH_DES
CRYPTPROGS = cyflexman gendivkey
else
CRYPTPROGS =
endif

INCLUDES = -I$(top_srcdir)/scez -I$(top_srcdir)/sio
needed_libs = @JIBAPILIB@ ../scez/libscez.la ../sio/libsio.la

bin_PROGRAMS = \
	bcupload \
	cafeman \
	cardcheck \
	cfupload \
	crdetect \
	cttest \
	filescan \
	flexpasswd \
	getcert \
	scanclains \
	scdir \
	scsh \
	scwait \
	season \
	simdump \
	simman \
	$(CRYPTPROGS)


bcupload_LDADD = $(needed_libs)
bcupload_SOURCES = bcupload.c
cafeman_LDADD = $(needed_libs)
cafeman_SOURCES = cafeman.c
cardcheck_LDADD = $(needed_libs)
cardcheck_SOURCES = cardcheck.c
cfupload_LDADD = $(needed_libs)
cfupload_SOURCES = cfupload.c
crdetect_LDADD = $(needed_libs)
crdetect_SOURCES = crdetect.c
cttest_LDADD = $(needed_libs)
cttest_SOURCES = cttest.c
cyflexman_LDADD = $(needed_libs)
cyflexman_SOURCES = cyflexman.c
filescan_LDADD = $(needed_libs)
filescan_SOURCES = filescan.c
flexpasswd_LDADD = $(needed_libs)
flexpasswd_SOURCES = flexpasswd.c
gendivkey_LDADD = $(needed_libs)
gendivkey_SOURCES = gendivkey.c
getcert_LDADD = $(needed_libs)
getcert_SOURCES = getcert.c
scanclains_LDADD = $(needed_libs)
scanclains_SOURCES = scanclains.c
scdir_LDADD = $(needed_libs)
scdir_SOURCES = scdir.c
scsh_LDADD = $(needed_libs)
scsh_SOURCES = scsh.c
scwait_LDADD = $(needed_libs)
scwait_SOURCES = scwait.c
season_LDADD = $(needed_libs)
season_SOURCES = season.c
simdump_LDADD = $(needed_libs)
simdump_SOURCES = simdump.c
simman_LDADD = $(needed_libs)
simman_SOURCES = simman.c simparse.c

EXTRA_DIST = \
	simman.h

