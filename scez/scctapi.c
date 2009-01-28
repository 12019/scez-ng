/****************************************************************************
*																			*
*					SCEZ chipcard library - CT-API routines					*
*						Copyright Matthias Bruestle 1999					*
*					For licensing, see the file COPYRIGHT					*
*																			*
****************************************************************************/

/* $Id: scctapi.c 1103 2002-06-21 14:41:42Z zwiebeltu $ */

#include <scez/scinternal.h>
#include <scez/scctapi.h>

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#ifdef WINDOWS
#include <windows.h>
#endif

/* Can load a shared library. */
#if defined(HAVE_LIBDL) || defined(__WIN16__) || defined(__WIN32__)

#if defined(HAVE_DLFCN_H)
#include <dlfcn.h>
#endif /* HAVE_DLFCN_H */

/****************************************************************************
*																			*
*								Static Variables							*
*																			*
****************************************************************************/

/* good enough for unique CT handle? */
static WORD scCtapiLastCtHandle=0;

#if defined(HAVE_LIBDL)
#define CTAPI_CALL
#elif defined(__WIN32__)
#define CTAPI_CALL __stdcall
#elif defined(__WIN16__)
#define CTAPI_CALL FAR PASCAL _export
#endif /* HAVE_LIBDL */

/* Counts how many SC_CARD_INFOs use the CT-API driver.
 * scCtapiShutdown unloads library only when it reaches again 0.
 * Or would it be better to let the library stay loaded?
 * What happens in case of errors?
 */
static int scCtapiICounter = 0;

#if defined(HAVE_LIBDL)

static void *scCtapiLibHandle = NULL;

#elif defined(WINDOWS)

#define NULL_HINSTANCE	(HINSTANCE) NULL
static HINSTANCE scCtapiLibHandle = NULL_HINSTANCE;

#endif /* HAVE_LIBDL */

/* Function prototypes */
unsigned char CTAPI_CALL CT_init( WORD ctn, WORD pn );
unsigned char CTAPI_CALL CT_data( WORD ctn, BYTE *dad, BYTE *sad, WORD lenc,
	BYTE *command, WORD *lenr, BYTE *response );
unsigned char CTAPI_CALL CT_close( WORD ctn );

/* Global function pointers. */
typedef unsigned char (CTAPI_CALL *CT_INIT)( WORD ctn, WORD pn );
typedef unsigned char (CTAPI_CALL *CT_DATA)( WORD ctn, BYTE *dad, BYTE *sad,
	WORD lenc, BYTE *command, WORD *lenr, BYTE *response );
typedef unsigned char (CTAPI_CALL *CT_CLOSE)( WORD ctn );
static CT_INIT pCT_init = NULL;
static CT_DATA pCT_data = NULL;
static CT_CLOSE pCT_close = NULL;
#define CT_init pCT_init
#define CT_data pCT_data
#define CT_close pCT_close


/****************************************************************************
*																			*
*							Init/Shutdown Routines							*
*																			*
****************************************************************************/

/* Initializes reader and library */

int scCtapiInit( SC_READER_INFO *ri, const char *param )
{
	BYTE command[]={0x20,0x11,0x00,0x00};
	BYTE response[256+2]; /* Not asked for data, but some send data. */
	BYTE sad=2, dad=1;
	WORD pn, lenc=4, lenr=2;
	unsigned char ret;
	int tmp;
#ifdef __WIN16__
	UINT errorMode;
#endif /* __WIN16__ */

#ifdef HAVE_LIBDL
	if( scCtapiLibHandle==NULL ) {
		scCtapiLibHandle = dlopen( SC_CTAPI_LIB, RTLD_LAZY );
		if( !scCtapiLibHandle ) return( SC_EXIT_LIB_ERROR );

		pCT_init = (CT_INIT) dlsym( scCtapiLibHandle, "CT_init" );
		if( dlerror()!=NULL ) return( SC_EXIT_LIB_ERROR );
		pCT_data = (CT_DATA) dlsym( scCtapiLibHandle, "CT_data" );
		if( dlerror()!=NULL ) return( SC_EXIT_LIB_ERROR );
		pCT_close = (CT_CLOSE) dlsym( scCtapiLibHandle, "CT_close" );
		if( dlerror()!=NULL ) return( SC_EXIT_LIB_ERROR );
	}
#elif defined(WINDOWS)
	if( scCtapiLibHandle==NULL_HINSTANCE ) {
#ifdef __WIN16__
		errorMode = SetErrorMode( SEM_NOOPENFILEERRORBOX );
		scCtapiLibHandle = LoadLibrary( SC_CTAPI_LIB );
		SetErrorMode( errorMode );
		if( scCtapiLibHandle == NULL_HINSTANCE ) return( SC_EXIT_LIB_ERROR );
#else
		if( ( scCtapiLibHandle = LoadLibrary( SC_CTAPI_LIB ) ) ==
			NULL_HINSTANCE ) return( SC_EXIT_LIB_ERROR );
#endif /* __WIN16__ */

		pCT_init = (CT_INIT) GetProcAddress( scCtapiLibHandle, "CT_init" );
		pCT_data = (CT_DATA) GetProcAddress( scCtapiLibHandle, "CT_data" );
		pCT_close = (CT_CLOSE) GetProcAddress( scCtapiLibHandle, "CT_close" );

		/* Make sure we got valid pointers for every card function */
		if( pCT_init == NULL || pCT_data == NULL || pCT_close == NULL ) {
			FreeLibrary( scCtapiLibHandle );
			scCtapiLibHandle = NULL_HINSTANCE;
			return( SC_EXIT_LIB_ERROR );
		}
	}
#endif /* HAVE_LIBDL */

	scCtapiICounter++;

	if( (ri->slot<1) && (ri->slot>14) ) return( SC_EXIT_NO_SLOT );

	if( sscanf( param, "%2d", &tmp )!=1 ) return( SC_EXIT_BAD_PARAM );
	if( (tmp<0) && (tmp>15) ) return( SC_EXIT_BAD_PARAM );
	pn=tmp & 0xFFFF;

	ri->ctn=scCtapiLastCtHandle++;

	/* Init */
	ret = (*CT_init)( ri->ctn, pn );
	if( ret!=SC_CTAPI_EXIT_OK ) return( SC_EXIT_UNKNOWN_ERROR );

	/* Reset CT */
	ret = (*CT_data)( ri->ctn, &dad, &sad, lenc, command, &lenr, response );
	if( (ret!=SC_CTAPI_EXIT_OK) || (lenr<2) || (response[lenr-2]!=0x90) )
		return( SC_EXIT_UNKNOWN_ERROR );

	ri->pinpad=FALSE;
	ri->display=FALSE;

	ri->scShutdown=scCtapiShutdown;
	ri->scGetCap=NULL;
	ri->scActivate=scCtapiActivate;
	ri->scDeactivate=scCtapiDeactivate;
	ri->scWriteBuffer=NULL;
	ri->scReadBuffer=NULL;
	ri->scWriteChar=NULL;
	ri->scReadChar=NULL;
	ri->scSetSpeed=NULL;
	ri->scCardStatus=scCtapiCardStatus;
	ri->scResetCard=scCtapiResetCard;
	ri->scPTS=NULL;
	ri->scT0=scCtapiT0;
	ri->scT1=scCtapiT1;
	ri->scSendAPDU=scCtapiSendAPDU;
	ri->scVerifyPIN=NULL;
	ri->scWaitReq=NULL;

	return( SC_EXIT_OK );
}

int scCtapiShutdown( SC_READER_INFO *ri )
{
	unsigned char ret;

	/* Hmm..... */
	ret = (*CT_close)( ri->ctn );
	if( ret!=SC_CTAPI_EXIT_OK ) return( SC_EXIT_UNKNOWN_ERROR );

	scCtapiICounter--;

#if defined(HAVE_LIBDL)
	if( (scCtapiLibHandle!=NULL) && (scCtapiICounter==0) ) {
		dlclose( scCtapiLibHandle );
		scCtapiLibHandle = NULL;
	}
#elif defined(WINDOWS)
	if( (scCtapiLibHandle!=NULL_HINSTANCE) && (scCtapiICounter==0) ) {
		FreeLibrary( scCtapiLibHandle );
		scCtapiLibHandle = NULL_HINSTANCE;
	}
#endif /* HAVE_LIBDL */

	return( SC_EXIT_OK );
}

int scCtapiDetect( SC_READER_DETECT_INFO *rdi )
{
	char *libname[]= {
#if defined(HAVE_LIBDL)
#ifdef __FreeBSD__
		"/usr/lib/libct_b1.so",
		"/usr/lib/libct_kaan.so",
		"/usr/lib/libctapi.so",
		"/usr/lib/libctapi-t2i.so",
		"/usr/lib/libctapi-mct09.so",
		"/usr/lib/libctapi-towitoko.so",
		"/usr/lib/libix_ix36.so",
		"/usr/lib/liblit_a210.so",
		"/usr/lib/libslb_rf60.so",
		"/usr/lib/libtodos_ag.so",
#else /* !__FreeBSD__ */
		"libct_b1.so",
		"libct_kaan.so",
		"libctapi.so",
		"libctapi-t2i.so",
		"libctapi-mct09.so",
		"libctapi-towitoko.so",
		"libix_ix36.so",
		"liblit_a210.so",
		"libslb_rf60.so",
		"libtodos_ag.so",
#endif /* __FreeBSD__ */
#elif defined(WINDOWS)
		"ct.dll",
		"ct32.dll",
		"ctapi.dll",
		"ctapi32.dll",
		"ctapiw32.dll",
		"cttwkw32.dll",
#endif /* HAVE_LIBDL */
		NULL
	};

#if defined(HAVE_LIBDL)
	static void *LibHandle = NULL;
#elif defined(WINDOWS)
#define NULL_HINSTANCE	(HINSTANCE) NULL
	static HINSTANCE LibHandle = NULL_HINSTANCE;
#endif /* HAVE_LIBDL */

	BYTE resetct[]={0x20,0x11,0x00,0x00};
	BYTE getstatus[]={0x20,0x13,0x00,0x80,0x00};
	BYTE response[40];
	BYTE sad=2, dad=1;
	WORD ctn=0, pn, lenc=4, lenr=2;
	unsigned char ret;
	int i=0, tmp;

	rdi->prob=0;
	strncpy( rdi->name, "CTAPI", SC_READER_NAME_MAXLEN );
	rdi->major=SC_READER_CTAPI;
	rdi->minor=0;
	rdi->slots=1;
	rdi->pinpad=FALSE;
	rdi->display=FALSE;

	if( strlen(rdi->param)>=SC_READER_PARAM_MAXLEN )
		return( SC_EXIT_BAD_PARAM );

	if( sscanf( rdi->param, "%2d", &tmp )!=1 ) return( SC_EXIT_BAD_PARAM );
	if( (tmp<0) && (tmp>15) ) return( SC_EXIT_BAD_PARAM );
	pn=tmp & 0xFFFF;

	while( libname[i]!=NULL ) {
		ctn++;

#ifdef HAVE_LIBDL
		if( LibHandle!=NULL ) {
			dlclose( LibHandle );
			LibHandle=NULL;
		}

		LibHandle = dlopen( libname[i], RTLD_LAZY );
		if( !LibHandle ) { i++; continue; }

		pCT_init = (CT_INIT) dlsym( LibHandle, "CT_init" );
		if( dlerror()!=NULL ) { i++; dlclose( LibHandle ); continue; }
		pCT_data = (CT_DATA) dlsym( LibHandle, "CT_data" );
		if( dlerror()!=NULL ) { i++; dlclose( LibHandle ); continue; }
		pCT_close = (CT_CLOSE) dlsym( LibHandle, "CT_close" );
		if( dlerror()!=NULL ) { i++; dlclose( LibHandle ); continue; }

		/* Init */
		ret = (*CT_init)( ctn, pn );
		if( ret!=SC_CTAPI_EXIT_OK ) { i++; dlclose( LibHandle ); continue; }

		/* Reset CT */
		dad=1; sad=2; lenc=4; lenr=2;
		ret = (*CT_data)( ctn, &dad, &sad, lenc, resetct, &lenr, response );
		if( (ret!=SC_CTAPI_EXIT_OK) || (lenr!=2) || (response[0]!=0x90) )
		{ i++; (*CT_close)( ctn ); dlclose( LibHandle ); continue; }

		/* Get Status ICC */
		dad=1; sad=2; lenc=5; lenr=sizeof(response);
		ret = (*CT_data)( ctn, &dad, &sad, lenc, getstatus, &lenr, response );
		if( (ret!=SC_CTAPI_EXIT_OK) || (lenr<2) || (response[lenr-2]!=0x90) )
		{ i++; (*CT_close)( ctn ); dlclose( LibHandle ); continue; }

		ret = (*CT_close)( ctn );
		if( ret!=SC_CTAPI_EXIT_OK ) { i++; dlclose( LibHandle ); continue; }

		if( LibHandle!=NULL ) {
			dlclose( LibHandle );
			LibHandle=NULL;
		}
#elif defined(WINDOWS)
		if( LibHandle!=NULL_HINSTANCE ) {
			FreeLibrary( LibHandle );
			LibHandle=(HINSTANCE)NULL;
		}

		if( ( LibHandle = LoadLibrary( libname[i] ) ) == NULL_HINSTANCE )
		{ i++; continue; }

		pCT_init = (CT_INIT) GetProcAddress( LibHandle, "CT_init" );
		pCT_data = (CT_DATA) GetProcAddress( LibHandle, "CT_data" );
		pCT_close = (CT_CLOSE) GetProcAddress( LibHandle, "CT_close" );

		/* Make sure we got valid pointers for every card function */
		if( pCT_init == NULL || pCT_data == NULL || pCT_close == NULL )
		{ i++; FreeLibrary( LibHandle ); continue; }

		/* Init */
		ret = (*CT_init)( ctn, pn );
		if( ret!=SC_CTAPI_EXIT_OK ) { i++; FreeLibrary( LibHandle ); continue; }

		/* Reset CT */
		dad=1; sad=2; lenr=2;
		ret = (*CT_data)( ctn, &dad, &sad, lenc, resetct, &lenr, response );
		if( (ret!=SC_CTAPI_EXIT_OK) || (lenr!=2) || (response[0]!=0x90) )
		{ i++; (*CT_close)( ctn ); FreeLibrary( LibHandle ); continue; }

		/* Get Status ICC */
		dad=1; sad=2; lenc=5; lenr=sizeof(response);
		ret = (*CT_data)( ctn, &dad, &sad, lenc, getstatus, &lenr, response );
		if( (ret!=SC_CTAPI_EXIT_OK) || (lenr<2) || (response[lenr-2]!=0x90) )
		{ i++; (*CT_close)( ctn ); FreeLibrary( LibHandle ); continue; }

		ret = (*CT_close)( ctn );
		if( ret!=SC_CTAPI_EXIT_OK ) { i++; FreeLibrary( LibHandle ); continue; }

		if( LibHandle!=NULL_HINSTANCE ) {
			FreeLibrary( LibHandle );
			LibHandle=(HINSTANCE)NULL;
		}
#endif /* HAVE_LIBDL */

		rdi->minor=i; /* Some information about the library name. */
		rdi->prob=150;

		return( SC_EXIT_OK );
	}

#ifdef HAVE_LIBDL
	if( LibHandle!=NULL ) dlclose( LibHandle );
#elif defined(WINDOWS)
	if( LibHandle!=NULL_HINSTANCE ) FreeLibrary( LibHandle );
#endif /* HAVE_LIBDL */

	return( SC_EXIT_OK );
}

/****************************************************************************
*																			*
*						CT-BCS - common commands							*
*																			*
****************************************************************************/

/* RESET CT */
/* atr/atrlen can be NULL, when not ATR is requested. */

int scCtapiBcsResetCT( SC_READER_INFO *ri, BYTE *sw, BYTE unit,
	BYTE qualifier, BYTE *atr, int *atrlen )
{
	BYTE command[]={0x20,0x11,0x00,0x00,0x00};
	BYTE response[SC_CTAPI_MAX_BUFFER_SIZE+2];
	BYTE sad=2, dad=1;
	WORD lenc=4, lenr=SC_CTAPI_MAX_BUFFER_SIZE+2;
	unsigned char ret;

	if( unit==SC_CTAPI_UNIT_CT ) command[2]=unit;
	else if( unit==SC_CTAPI_UNIT_ICC ) command[2]=ri->slot & 0xFF;
	else return( SC_EXIT_BAD_PARAM );

	if( qualifier>0x02 ) return( SC_EXIT_BAD_PARAM );
	command[3]=qualifier;
	if( qualifier!=0x00 ) lenc++;

	ret = (*CT_data)( ri->ctn, &dad, &sad, lenc, command, &lenr, response );
	if( (ret!=SC_CTAPI_EXIT_OK) || (lenr<2) )
		return( SC_EXIT_UNKNOWN_ERROR );

	if( (atr!=NULL) && (atrlen!=NULL) &&
		(qualifier!=0x00) && ((lenr-2)<=*atrlen) ) {
		memcpy( atr, response, lenr-2 );
		*atrlen=lenr-2;
	} else if( atrlen!=NULL ) {
		if( (lenr-2)>*atrlen ) *atrlen=-1;
		else *atrlen=0;
	}

	sw[0]=response[lenr-2];
	sw[1]=response[lenr-1];

	return( SC_EXIT_OK );
}

/* REQUEST ICC */

/* atr/atrlen can be NULL, when not ATR is requested.
 * atrlen: Specifies maximum ATR length and returns ATR length.
 *         If atrlen is to small for ATR it returns -1.
 */

int scCtapiBcsRequestICC( SC_READER_INFO *ri, BYTE *sw, BYTE qualifier,
	char *message, BYTE timeout, BYTE *atr, int *atrlen )
{
	BYTE command[SC_CTAPI_MAX_BUFFER_SIZE+6];
	BYTE response[SC_CTAPI_MAX_BUFFER_SIZE+2];
	BYTE sad=2, dad=1;
	WORD lenc=4, lenr=SC_CTAPI_MAX_BUFFER_SIZE+2;
	unsigned char ret;
	int datasize=0;
	int	tmpsize;

	memset( command, 0, sizeof(command) );

	command[0]=0x20;
	command[1]=0x12;
	command[2]=ri->slot & 0xFF;

	if( (qualifier&0xF)>0x02 ) return( SC_EXIT_BAD_PARAM );
	if( ((qualifier&0xF0)!=0x00) && ((qualifier&0xF0)!=0xF0) )
		return( SC_EXIT_BAD_PARAM );
	command[3]=qualifier;

	if( (message!=NULL) && ((qualifier&0xF0)==0x00) ) {
		command[5]=0x50;
		tmpsize=strlen( message );
		if( tmpsize>0xFD ) return( SC_EXIT_BAD_PARAM );
		command[6]=tmpsize;
		memcpy( command+7, message, tmpsize );
		datasize+=tmpsize+2;
		lenc+=tmpsize+2+1;
	}

	if( timeout!=0xFF ) {
		command[5+datasize]=0x80;
		command[5+datasize+1]=0x01;
		command[5+datasize+2]=timeout;
		datasize+=3;
		lenc+=3;
		if( message==NULL ) lenc++;
	}

	if( datasize>0xFF ) return( SC_EXIT_BAD_PARAM );
	if( datasize>0 ) command[4]=datasize;

	if( (qualifier&0x0F)>0x00 ) {
		command[lenc]=0x00;
		lenc++;
	}

	ret = (*CT_data)( ri->ctn, &dad, &sad, lenc, command, &lenr, response );
	if( (ret!=SC_CTAPI_EXIT_OK) || (lenr<2) )
		return( SC_EXIT_UNKNOWN_ERROR );

	if( (atr!=NULL) && (atrlen!=NULL) &&
		(qualifier!=0x00) && ((lenr-2)<=*atrlen) ) {
		memcpy( atr, response, lenr-2 );
		*atrlen=lenr-2;
	} else if( atrlen!=NULL ) {
		if( (lenr-2)>*atrlen ) *atrlen=-1;
		else *atrlen=0;
	}

	sw[0]=response[lenr-2];
	sw[1]=response[lenr-1];

	return( SC_EXIT_OK );
}

/* GET STATUS */

/* len: Specifies maximum status length and returns status length.
 *      If len is to small for status it returns -1.
 */

int scCtapiBcsGetStatus( SC_READER_INFO *ri, BYTE *sw, BYTE unit,
	BYTE *status, int *len )
{
	BYTE command[]={0x20,0x13,0x00,0x00,0x00};
	BYTE response[SC_CTAPI_MAX_BUFFER_SIZE+2];
	BYTE sad=2, dad=1;
	WORD lenc=4, lenr=SC_CTAPI_MAX_BUFFER_SIZE+2;
	unsigned char ret;

	if( unit==SC_CTAPI_UNIT_CT ) command[3]=0x46;
	else if( unit==SC_CTAPI_UNIT_ICC ) command[3]=0x80;
	else return( SC_EXIT_BAD_PARAM );

	ret = (*CT_data)( ri->ctn, &dad, &sad, lenc, command, &lenr, response );
	if( (ret!=SC_CTAPI_EXIT_OK) || (lenr<2) )
		return( SC_EXIT_UNKNOWN_ERROR );

	if( (status!=NULL) && (len!=NULL) && ((lenr-2)<=*len) ) {
		memcpy( status, response, lenr-2 );
		*len=lenr-2;
	} else if( len!=NULL ) {
		if( (lenr-2)>*len ) *len=-1;
		else *len=0;
	}

	sw[0]=response[lenr-2];
	sw[1]=response[lenr-1];

	return( SC_EXIT_OK );
}

/* EJECT ICC */

int scCtapiBcsEjectICC( SC_READER_INFO *ri, BYTE *sw, BYTE qualifier,
	char *message, BYTE timeout )
{
	BYTE command[SC_CTAPI_MAX_BUFFER_SIZE+6];
	BYTE response[2];
	BYTE sad=2, dad=1;
	WORD lenc=4, lenr=2;
	unsigned char ret;
	int datasize=0;
	int	tmpsize;

	memset( command, 0, sizeof(command) );

	command[0]=0x20;
	command[1]=0x15;
	command[2]=ri->slot & 0xFF;

	if( ((qualifier&0xF0)!=0x00) && ((qualifier&0xF0)!=0xF0) )
		return( SC_EXIT_BAD_PARAM );
	command[3]=qualifier;

	if( (message!=NULL) && ((qualifier&0xF0)==0x00) ) {
		command[5]=0x50;
		tmpsize=strlen( message );
		if( tmpsize>0xFD ) return( SC_EXIT_BAD_PARAM );
		command[6]=tmpsize;
		memcpy( command+7, message, tmpsize );
		datasize+=tmpsize+2;
		lenc+=tmpsize+2+1;
	}

	if( timeout!=0xFF ) {
		command[5+datasize]=0x80;
		command[5+datasize+1]=0x01;
		command[5+datasize+2]=timeout;
		datasize+=3;
		lenc+=3;
		if( message==NULL ) lenc++;
	}

	if( datasize>0xFF ) return( SC_EXIT_BAD_PARAM );
	if( datasize>0 ) command[4]=datasize;

	if( (qualifier&0x0F)>0x00 ) {
		command[lenc]=0x00;
		lenc++;
	}

	ret = (*CT_data)( ri->ctn, &dad, &sad, lenc, command, &lenr, response );
	if( (ret!=SC_CTAPI_EXIT_OK) || (lenr!=2) )
		return( SC_EXIT_UNKNOWN_ERROR );

	sw[0]=response[0];
	sw[1]=response[1];

	return( SC_EXIT_OK );
}

/****************************************************************************
*																			*
*						CT-BCS - additional commands						*
*																			*
****************************************************************************/

/* INPUT */

int scCtapiBcsInput( SC_READER_INFO *ri, BYTE *sw, BYTE qualifier,
	char *message, BYTE timeout, BYTE *input, int *inputlen )
{
	BYTE command[SC_CTAPI_MAX_BUFFER_SIZE+6];
	BYTE response[SC_CTAPI_MAX_BUFFER_SIZE+2];
	BYTE sad=2, dad=1;
	WORD lenc=4, lenr=SC_CTAPI_MAX_BUFFER_SIZE+2;
	unsigned char ret;
	int datasize=0;
	int	tmpsize;

	if( (input==NULL) || (inputlen==NULL) ) return( SC_EXIT_BAD_PARAM );

	memset( command, 0, sizeof(command) );

	command[0]=0x20;
	command[1]=0x16;
	command[2]=0x50; /* Unit: Keyboard */

	if( qualifier>0x02 ) return( SC_EXIT_BAD_PARAM );
	command[3]=qualifier;

	/* Text */
	if( message!=NULL ) {
		command[5]=0x50;
		tmpsize=strlen( message );
		if( tmpsize>0xFD ) return( SC_EXIT_BAD_PARAM );
		command[6]=tmpsize;
		memcpy( command+7, message, tmpsize );
		datasize+=tmpsize+2;
		lenc+=tmpsize+2+1;
	}

	/* Timeout */
	if( timeout!=0xFF ) {
		command[5+datasize]=0x80;
		command[5+datasize+1]=0x01;
		command[5+datasize+2]=timeout;
		datasize+=3;
		lenc+=3;
		if( message==NULL ) lenc++;
	}

	if( datasize>0xFF ) return( SC_EXIT_BAD_PARAM );
	if( datasize>0 ) command[4]=datasize;

	/* Le */
	command[lenc]=0x00;
	lenc++;

	ret = (*CT_data)( ri->ctn, &dad, &sad, lenc, command, &lenr, response );
	if( (ret!=SC_CTAPI_EXIT_OK) || (lenr<2) )
		return( SC_EXIT_UNKNOWN_ERROR );

	if( (lenr-2)<=*inputlen ) {
		memcpy( input, response, lenr-2 );
		*inputlen=lenr-2;
	} else return( SC_EXIT_UNKNOWN_ERROR );

	sw[0]=response[lenr-2];
	sw[1]=response[lenr-1];

	return( SC_EXIT_OK );
}

/* OUTPUT */

int scCtapiBcsOutput( SC_READER_INFO *ri, BYTE *sw, char *message )
{
	BYTE command[SC_CTAPI_MAX_BUFFER_SIZE+6];
	BYTE response[2];
	BYTE sad=2, dad=1;
	WORD lenc=5, lenr=2;
	unsigned char ret;
	int datasize=0;

	if( (message==NULL) || (strlen(message)>0xFD) ) return( SC_EXIT_BAD_PARAM );

	memset( command, 0, sizeof(command) );

	command[0]=0x20;
	command[1]=0x17;
	command[2]=0x40; /* Unit: Display */
	command[3]=0x00; /* Default qualifier */

	/* Text */
	command[5]=0x50;
	command[6]=strlen( message ) & 0xFF;
	memcpy( command+7, message, strlen( message ) );
	datasize+=(strlen( message ) & 0xFF) + 2;
	lenc+=datasize;

	ret = (*CT_data)( ri->ctn, &dad, &sad, lenc, command, &lenr, response );
	if( (ret!=SC_CTAPI_EXIT_OK) || (lenr!=2) )
		return( SC_EXIT_UNKNOWN_ERROR );

	sw[0]=response[0];
	sw[1]=response[1];

	return( SC_EXIT_OK );
}

/* PERFORM VERIFICATION */

int scCtapiBcsPerformVer( SC_READER_INFO *ri, BYTE *sw, BYTE *data,
	BYTE datalen, char *message, BYTE timeout )
{
	BYTE command[SC_CTAPI_MAX_BUFFER_SIZE+6];
	BYTE response[2];
	BYTE sad=2, dad=1;
	WORD lenc=5, lenr=2;
	unsigned char ret;

	if( (data==NULL) || (datalen==0x00) || (datalen>=0xFD) )
		return( SC_EXIT_BAD_PARAM );

	memset( command, 0, sizeof(command) );

	command[0]=0x20;
	command[1]=0x18;
	command[2]=ri->slot & 0xFF;
	command[3]=0x00; /* Default qualifier */

	/* Text */
	if( message!=NULL ) {
		if( strlen( message )>=0xFD ) return( SC_EXIT_BAD_PARAM );
		command[lenc++]=0x50;
		command[lenc++]=strlen( message );
		memcpy( command+lenc, message, strlen( message ) );
		lenc+=strlen( message );
	}

	/* Command to perform */
	command[lenc++]=0x52;
	command[lenc++]=datalen;
	memcpy( command+lenc, data, datalen );
	lenc+=datalen;

	/* Timeout */
	if( timeout!=0xFF ) {
		command[lenc++]=0x80;
		command[lenc++]=0x01;
		command[lenc++]=timeout;
	}

	ret = (*CT_data)( ri->ctn, &dad, &sad, lenc, command, &lenr, response );
	if( (ret!=SC_CTAPI_EXIT_OK) || (lenr!=2) )
		return( SC_EXIT_UNKNOWN_ERROR );

	sw[0]=response[0];
	sw[1]=response[1];

	return( SC_EXIT_OK );
}

/* MODIFY VERIFICATION DATA */

int scCtapiBcsModVerData( SC_READER_INFO *ri, BYTE *sw, BYTE *data,
	BYTE datalen, char *message, BYTE timeout )
{
	BYTE command[SC_CTAPI_MAX_BUFFER_SIZE+6];
	BYTE response[2];
	BYTE sad=2, dad=1;
	WORD lenc=5, lenr=2;
	unsigned char ret;

	if( (data==NULL) || (datalen==0x00) || (datalen>=0xFD) )
		return( SC_EXIT_BAD_PARAM );

	memset( command, 0, sizeof(command) );

	command[0]=0x20;
	command[1]=0x19;
	command[2]=ri->slot & 0xFF;
	command[3]=0x00; /* Default qualifier */

	/* Text */
	if( message!=NULL ) {
		if( strlen( message )>=0xFD ) return( SC_EXIT_BAD_PARAM );
		command[lenc++]=0x50;
		command[lenc++]=strlen( message );
		memcpy( command+lenc, message, strlen( message ) );
		lenc+=strlen( message );
	}

	/* Command to perform */
	command[lenc++]=0x52;
	command[lenc++]=datalen;
	memcpy( command+lenc, data, datalen );
	lenc+=datalen;

	/* Timeout */
	if( timeout!=0xFF ) {
		command[lenc++]=0x80;
		command[lenc++]=0x01;
		command[lenc++]=timeout;
	}

	ret = (*CT_data)( ri->ctn, &dad, &sad, lenc, command, &lenr, response );
	if( (ret!=SC_CTAPI_EXIT_OK) || (lenr!=2) )
		return( SC_EXIT_UNKNOWN_ERROR );

	sw[0]=response[0];
	sw[1]=response[1];

	return( SC_EXIT_OK );
}

/****************************************************************************
*																			*
*							Low Level Functions								*
*																			*
****************************************************************************/

/* Activate Card */

int scCtapiActivate( SC_READER_INFO *ri )
{
	BYTE command[]={0x20,0x12,0x00,0x00};
	BYTE response[2];
	BYTE sad=2, dad=1;
	WORD lenc=4, lenr=2;
	unsigned char ret;

	command[2]=ri->slot & 0xFF;

	/* Request ICC */
	ret = (*CT_data)( ri->ctn, &dad, &sad, lenc, command, &lenr, response );
	if( (ret!=SC_CTAPI_EXIT_OK) || (lenr!=2) || (response[0]!=0x90) )
		return( SC_EXIT_UNKNOWN_ERROR );

	return( SC_EXIT_OK );
}

/* Deactivate Card */

int scCtapiDeactivate( SC_READER_INFO *ri )
{
	BYTE command[]={0x20,0x15,0x00,0x06};
	BYTE response[2];
	BYTE sad=2, dad=1;
	WORD lenc=4, lenr=2;
	unsigned char ret;

	command[2]=ri->slot & 0xFF;

	/* Eject ICC */
	ret = (*CT_data)( ri->ctn, &dad, &sad, lenc, command, &lenr, response );
	if( (ret!=SC_CTAPI_EXIT_OK) || (lenr!=2) || (response[0]!=0x90) )
		return( SC_EXIT_UNKNOWN_ERROR );

	return( SC_EXIT_OK );
}

/* Get card status */

int scCtapiCardStatus( SC_READER_INFO *ri )
{
	BYTE command[]={0x20,0x13,0x00,0x80,0x00};
	BYTE response[18];
	BYTE sad=2, dad=1;
	WORD lenc=5, lenr=18;
	unsigned char ret;

	/* Get Status */
	ret = (*CT_data)( ri->ctn, &dad, &sad, lenc, command, &lenr, response );
	if( (ret!=SC_CTAPI_EXIT_OK) || (lenr<2) || (response[lenr-2]!=0x90) )
		return( SC_EXIT_UNKNOWN_ERROR );

	if( response[0]==0x80 ) ri->status=response[(ri->slot & 0xFF)+1];
	else ri->status=response[(ri->slot & 0xFF)-1];

	ri->status &= SC_CTAPI_CARD;

	return( SC_EXIT_OK );
}

/****************************************************************************
*																			*
*							SmartCard Functions								*
*																			*
****************************************************************************/

/* Reset Card */

int scCtapiResetCard( SC_READER_INFO *ri, SC_CARD_INFO *ci )
{
	BYTE command[]={0x20,0x11,0x00,0x01,0x00};
	BYTE response[34];
	BYTE sad=2, dad=1;
	WORD lenc=5, lenr=34;
	unsigned char ret;

	command[2]=ri->slot;

	scGeneralCleanCI( ci );

	/* Reset ICC */
	ret = (*CT_data)( ri->ctn, &dad, &sad, lenc, command, &lenr, response );
	if( (ret!=SC_CTAPI_EXIT_OK) || (lenr<2) || (response[lenr-2]!=0x90) )
		return( SC_EXIT_UNKNOWN_ERROR );

	memcpy( ci->atr, response, lenr-2 );
	ci->atrlen=lenr-2;

	return( SC_EXIT_OK );
}

/* Transmit APDU with protocol T=0 */

int scCtapiT0( SC_READER_INFO *ri, SC_CARD_INFO *ci, SC_APDU *apdu )
{
	BYTE sad=2, dad=0, dest;
	WORD lenr=2, le;
	unsigned char ret;

	if( (ret=scReaderCheckAPDU( apdu, TRUE ))!=SC_EXIT_OK ) return( ret );

	if( (ri->slot>1) && (ri->slot<15) ) dad=ri->slot & 0xFF;
	if( dad==1 ) dad=0;
	dest=dad;

	switch( apdu->cse ) {
		case SC_APDU_CASE_1:
			if( apdu->cmdlen==5 ) apdu->cmdlen--;
		case SC_APDU_CASE_3_SHORT:
			break;
		case SC_APDU_CASE_2_SHORT:
		case SC_APDU_CASE_4_SHORT:
				le=apdu->cmd[apdu->cmdlen-1];
				if( le==0x0000 ) le=256;
				lenr+=le;
			break;
		default:
			return( SC_EXIT_BAD_PARAM );
	}

	/* Send command */
	ret = (*CT_data)( ri->ctn, &dad, &sad, (WORD)(apdu->cmdlen & 0xFFFF),
		apdu->cmd, &lenr, apdu->rsp );
	if( (ret!=SC_CTAPI_EXIT_OK) || (lenr<2) || (sad!=dest) )
		return( SC_EXIT_UNKNOWN_ERROR );

	apdu->rsplen=lenr;

	return( SC_EXIT_OK );
}

/* Transmit APDU with protocol T=1 */

int scCtapiT1( SC_READER_INFO *ri, SC_CARD_INFO *ci, SC_APDU *apdu )
{
	BYTE sad=2, dad=0, dest;
	WORD lenr=2, le;
	unsigned char ret;

#ifndef NO_APDU_CHECK
	if( (ret=scReaderCheckAPDU( apdu, FALSE ))!=SC_EXIT_OK ) return( ret );
#endif /* NO_APDU_CHECK */

	if( (ri->slot>1) && (ri->slot<15) ) dad=ri->slot & 0xFF;
	dest=dad;

#if 0
	if( (apdu->cse==SC_APDU_CASE_2_SHORT) ||
		(apdu->cse==SC_APDU_CASE_4_SHORT) ) {
		le=apdu->cmd[apdu->cmdlen-1];
		if( le==0x0000 ) le=256;
		lenr+=le;
	}
#endif
	lenr=SC_GENERAL_SHORT_DATA_SIZE+2;

	/* Send command */
	ret = (*CT_data)( ri->ctn, &dad, &sad, (WORD)(apdu->cmdlen & 0xFFFF),
		apdu->cmd, &lenr, apdu->rsp );
	if( (ret!=SC_CTAPI_EXIT_OK) || (lenr<2) || (sad!=dest) )
		return( SC_EXIT_UNKNOWN_ERROR );

	apdu->rsplen=lenr;

	return( SC_EXIT_OK );
}

/* Transmit APDU */

/* Protocol T=0:
 *  - For case 4 instructions: The le-bytes is striped before calling
 *    scCtapiT0.
 */

int scCtapiSendAPDU( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	SC_APDU *apdu )
{
	int ret, origcse;

#ifndef NO_APDU_CHECK
	if( (ret=scReaderCheckAPDU( apdu, TRUE ))!=SC_EXIT_OK ) return( ret );
#endif /* NO_APDU_CHECK */

#ifdef READER_DEBUG
    printf(" [CMD: %d /",apdu->cse);
    for(ret=0;ret<apdu->cmdlen;ret++) printf(" %.2X",apdu->cmd[ret]);
    printf("]");
#endif

	if( ci->protocol==SC_PROTOCOL_T0 ) {
		origcse=apdu->cse;

		/* Strip le-Byte */
		if( apdu->cse==SC_APDU_CASE_4_SHORT ) {
			apdu->cmdlen--;
			apdu->cse=SC_APDU_CASE_3_SHORT;
		}

		if( (ret=scCtapiT0( ri, ci, apdu ))!=SC_EXIT_OK )
			return( ret );

		/* There should be a response */
		if( origcse==SC_APDU_CASE_4_SHORT ) {
			int s;
			int	n;
			int le=apdu->cmd[apdu->cmdlen];

			if( le==0x00 ) le=256;

			apdu->cmdlen++;

			if( apdu->rsplen<2 ) return( SC_EXIT_BAD_SW );

			memcpy( ci->sw, apdu->rsp+apdu->rsplen-2, 2 );

			if( (ret=scSmartcardSimpleProcessSW( ci, &s, &n ))!=SC_EXIT_OK )
				return( ret );

			if( (s==SC_SW_DATA_AVAIL) && (n>0) && (apdu->rsplen==2) ) {
				SC_APDU grapdu;

				/* Setup Get Response */
				grapdu.cse=SC_APDU_CASE_2_SHORT;
				if( (grapdu.cmd=malloc(5))==NULL )
					return( SC_EXIT_MALLOC_ERROR );
				grapdu.cmdlen=5;
				if( (grapdu.rsp=malloc(SC_GENERAL_SHORT_DATA_SIZE+2))==NULL )
					return( SC_EXIT_MALLOC_ERROR );
				grapdu.rsplen=0;
				memcpy( grapdu.cmd , ci->t0.getrsp, 5 );
				grapdu.cmd[4] = (BYTE) n;

				/* Execute Get Response */
				if( (ret=scCtapiT0( ri, ci, &grapdu ))!=SC_EXIT_OK )
					return( ret );

				/* Copy response to user-apdu */
				/* Don't copy more than requested */
				if( grapdu.rsplen>(le+2) ) {
					memcpy( apdu->rsp, grapdu.rsp, le );
					memcpy( apdu->rsp+le,
						grapdu.rsp+grapdu.rsplen-2, 2 );
					apdu->rsplen=le+2;
				} else {
					memcpy( apdu->rsp, grapdu.rsp, grapdu.rsplen );
					apdu->rsplen=grapdu.rsplen;
				}
			}
		}


#ifdef READER_DEBUG
		printf(" [RSP: ");
		for(ret=0;ret<apdu->rsplen;ret++) printf(" %.2X",apdu->rsp[ret]);
		printf("]");
#endif
		return( SC_EXIT_OK );
	} else if( ci->protocol==SC_PROTOCOL_T1 ) {
		return( scCtapiT1( ri, ci, apdu ) );
	}

	return( SC_EXIT_NOT_IMPLEMENTED );
}

#else /* Cannot load a shared library. */

int scCtapiInit( SC_READER_INFO *ri, const char *param )
{ return( SC_EXIT_NOT_IMPLEMENTED ); }

int scCtapiShutdown( SC_READER_INFO *ri )
{ return( SC_EXIT_NOT_IMPLEMENTED ); }

int scCtapiDetect( SC_READER_DETECT_INFO *rdi )
{ return( SC_EXIT_NOT_SUPPORTED ); }

int scCtapiBcsResetCT( SC_READER_INFO *ri, BYTE *sw, BYTE unit,
	BYTE qualifier, BYTE *atr, int *atrlen )
{ return( SC_EXIT_NOT_IMPLEMENTED ); }

int scCtapiBcsRequestICC( SC_READER_INFO *ri, BYTE *sw, BYTE qualifier,
	char *message, BYTE timeout, BYTE *atr, int *atrlen )
{ return( SC_EXIT_NOT_IMPLEMENTED ); }

int scCtapiBcsGetStatus( SC_READER_INFO *ri, BYTE *sw, BYTE unit,
	BYTE *status, int *len )
{ return( SC_EXIT_NOT_IMPLEMENTED ); }

int scCtapiBcsEjectICC( SC_READER_INFO *ri, BYTE *sw, BYTE qualifier,
	char *message, BYTE timeout )
{ return( SC_EXIT_NOT_IMPLEMENTED ); }

int scCtapiBcsInput( SC_READER_INFO *ri, BYTE *sw, BYTE qualifier,
	char *message, BYTE timeout, BYTE *input, int *inputlen )
{ return( SC_EXIT_NOT_IMPLEMENTED ); }

int scCtapiBcsOutput( SC_READER_INFO *ri, BYTE *sw, char *message )
{ return( SC_EXIT_NOT_IMPLEMENTED ); }

int scCtapiBcsPerformVer( SC_READER_INFO *ri, BYTE *sw, BYTE *data,
	BYTE datalen, char *message, BYTE timeout )
{ return( SC_EXIT_NOT_IMPLEMENTED ); }

int scCtapiBcsModVerData( SC_READER_INFO *ri, BYTE *sw, BYTE *data,
	BYTE datalen, char *message, BYTE timeout )
{ return( SC_EXIT_NOT_IMPLEMENTED ); }

int scCtapiActivate( SC_READER_INFO *ri )
{ return( SC_EXIT_NOT_IMPLEMENTED ); }

int scCtapiDeactivate( SC_READER_INFO *ri )
{ return( SC_EXIT_NOT_IMPLEMENTED ); }

int scCtapiCardStatus( SC_READER_INFO *ri )
{ return( SC_EXIT_NOT_IMPLEMENTED ); }

int scCtapiResetCard( SC_READER_INFO *ri, SC_CARD_INFO *ci )
{ return( SC_EXIT_NOT_IMPLEMENTED ); }

int scCtapiT0( SC_READER_INFO *ri, SC_CARD_INFO *ci, SC_APDU *apdu )
{ return( SC_EXIT_NOT_IMPLEMENTED ); }

int scCtapiT1( SC_READER_INFO *ri, SC_CARD_INFO *ci, SC_APDU *apdu )
{ return( SC_EXIT_NOT_IMPLEMENTED ); }

int scCtapiSendAPDU( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	SC_APDU *apdu )
{ return( SC_EXIT_NOT_IMPLEMENTED ); }

#endif /* HAVE_LIBDL||__WIN16__||__WIN32__ */

