# libdes 4.01 Makefile for Microsoft Visual C++ 5.0.

ALL: des.lib

CLEAN:
	-@erase cbc_cksm.obj
	-@erase cbc_enc.obj
	-@erase ecb_enc.obj
	-@erase pcbc_enc.obj
	-@erase qud_cksm.obj
	-@erase rand_key.obj
	-@erase set_key.obj
	-@erase str2key.obj
	-@erase enc_read.obj
	-@erase enc_writ.obj
	-@erase fcrypt.obj
	-@erase cfb_enc.obj
	-@erase ecb3_enc.obj
	-@erase ofb_enc.obj
	-@erase cbc3_enc.obj
	-@erase read_pwd.obj
	-@erase cfb64enc.obj
	-@erase ofb64enc.obj
	-@erase ede_enc.obj
	-@erase cfb64ede.obj
	-@erase ofb64ede.obj
	-@erase supp.obj
	-@erase des.lib

CPP=cl.exe
CPP_FLAGS=/nologo /Ox /W3 /DMSDOS /c # /DWIN32 /D_WINDOWS /DWINDOWS

.c.obj::
   $(CPP) @<<
   $(CPP_FLAGS) $< 
<<

LIB32=link.exe -lib
LIB32_FLAGS=/nologo /out:des.lib
LIB32_OBJS= \
	cbc_cksm.obj \
	cbc_enc.obj \
	cbc3_enc.obj \
	cfb_enc.obj \
	cfb64ede.obj \
	cfb64enc.obj \
	des_enc.obj \
	ecb_enc.obj \
	ecb3_enc.obj \
	ede_enc.obj \
	enc_read.obj \
	enc_writ.obj \
	fcrypt.obj \
	ncbc_enc.obj \
	ofb_enc.obj \
	ofb64ede.obj \
	ofb64enc.obj \
	pcbc_enc.obj \
	qud_cksm.obj \
	rand_key.obj \
	read_pwd.obj \
	set_key.obj \
	str2key.obj \
	supp.obj \
	xcbc_enc.obj

des.lib: $(LIB32_OBJS)
    $(LIB32) @<<
  $(LIB32_FLAGS) $(LIB32_OBJS)
<<




