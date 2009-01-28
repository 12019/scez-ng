/****************************************************************************
*																			*
*						SCEZ chipcard library - CT-API Test					*
*						Copyright Matthias Bruestle 2000					*
*					For licensing, see the file COPYRIGHT					*
*																			*
****************************************************************************/

/* $Id: cttest.c 874 2000-09-01 15:24:13Z zwiebeltu $ */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#define VERBOSE

#if defined(__WIN16__) || defined(__WIN32__)
#ifndef WINDOWS
#define WINDOWS
#endif  /* WINDOWS */
#else
#define HAS_DLOPEN
#endif

#if defined(HAS_DLOPEN)
#include <dlfcn.h>
#elif defined(__WIN32__)
#include <windows.h>
#endif /* HAS_DLOPEN */

#ifdef __BORLANDC__
#include <windows.h>
#ifndef LONG
#define LONG    DWORD
#endif
#else /* Not __BORLANDC__ */
#ifndef BYTE
#define BYTE    unsigned char
#endif
#ifndef UBYTE
#define UBYTE   signed char
#endif
#ifndef WORD
#define WORD    unsigned short
#endif
#ifndef LONG
#define LONG    unsigned long
#endif
#endif /* __BORLANDC__ */

#define BOOLEAN int

#ifndef FALSE
#define FALSE   0
#endif
#ifndef TRUE
#define TRUE    !0
#endif

#define CTAPI_OK          0
#define CTAPI_ERR_INVALID -1
#define CTAPI_ERR_CT      -8
#define CTAPI_ERR_TRANS   -10
#define CTAPI_ERR_MEMORY  -11
#define CTAPI_ERR_HOST    -127
#define CTAPI_ERR_HTSI    -128

#define CTAPI_BCS_VERSION_UNKNOWN	0
#define CTAPI_BCS_VERSION_0_9		1
#define CTAPI_BCS_VERSION_1_0		2

typedef struct ctapi_info {
	int		bcsver;		/* CT-BCS version */
	BYTE	atr[32];
	int		atrlen;
} CTAPI_INFO;

#if defined(HAS_DLOPEN)
#define CTAPI_CALL
#elif defined(__WIN32__)
#define CTAPI_CALL __stdcall
#endif /* HAS_DLOPEN */

/* Function prototypes */
signed char CTAPI_CALL CT_init( WORD ctn, WORD pn );
signed char CTAPI_CALL CT_data( WORD ctn, BYTE *dad, BYTE *sad, WORD lenc,
	BYTE *command, WORD *lenr, BYTE *response );
signed char CTAPI_CALL CT_close( WORD ctn );

/* Global function pointers. */
typedef signed char (CTAPI_CALL *CT_INIT)( WORD ctn, WORD pn );
typedef signed char (CTAPI_CALL *CT_DATA)( WORD ctn, BYTE *dad, BYTE *sad,
	WORD lenc, BYTE *command, WORD *lenr, BYTE *response );
typedef signed char (CTAPI_CALL *CT_CLOSE)( WORD ctn );
static CT_INIT pCT_init = NULL;
static CT_DATA pCT_data = NULL;
static CT_CLOSE pCT_close = NULL;
#define CT_init pCT_init
#define CT_data pCT_data
#define CT_close pCT_close

#if defined(HAS_DLOPEN)
	static void *LibHandle = NULL;
#elif defined(WINDOWS)
#define NULL_HINSTANCE	(HINSTANCE) NULL
	static HINSTANCE LibHandle = NULL_HINSTANCE;
#endif /* HAS_DLOPEN */

#if defined(HAS_DLOPEN)
void unloadlib( void *handle )
{
	dlclose( handle );
}
#elif defined(WINDOWS)
void unloadlib( HINSTANCE handle )
{
	FreeLibrary( handle );
}
#endif /* HAS_DLOPEN */

/* Eject ICC */

BOOLEAN eject( WORD ctn )
{
	BYTE cmd[]={0x20,0x15,0x01,0x00};
	BYTE rsp[260];
	BYTE sad=2, dad=1;
	WORD lenc=4, lenr=16;
	UBYTE ret;

	ret = (*CT_data)( ctn, &dad, &sad, lenc, cmd, &lenr, rsp );

	if( (ret!=CTAPI_OK) || (lenr!=2) ) {
		return(FALSE);
	}

	return(TRUE);
}

BOOLEAN checkret( UBYTE ret_is, UBYTE ret_should )
{
	if( ret_is!=ret_should ) { printf(" ret=%d",ret_is); return(FALSE); }

	return(TRUE);
}

BOOLEAN checkad( BYTE sad_is, BYTE sad_should, BYTE dad_is, BYTE dad_should )
{
	BOOLEAN ret=TRUE;

	if( sad_is!=sad_should ) { printf(" sad=%d",sad_is); ret=FALSE; }
	if( dad_is!=dad_should ) { printf(" dad=%d",dad_is); ret=FALSE; }

	return(ret);
}

BOOLEAN checklenr( WORD lenr_is, WORD lenr_should )
{
	if( lenr_is!=lenr_should ) { printf(" lenr=%d",lenr_is); return(FALSE); }

	return(TRUE);
}

BOOLEAN checksw( BYTE sw1_is, BYTE sw1_should, BYTE sw2_is, BYTE sw2_should )
{
	BOOLEAN ret=TRUE;

	if( sw1_is!=sw1_should ) { printf(" sw1=%.2X",sw1_is); ret=FALSE; }
	if( sw2_is!=sw2_should ) { printf(" sw2=%.2X",sw2_is); ret=FALSE; }

	return(ret);
} 

BOOLEAN checkswext( BYTE sw1_is, BYTE sw1_should, BYTE sw2_is, BYTE sw2_should,
	BYTE sw1, BYTE sw2 )
{
	BOOLEAN ret=TRUE;

	if( sw1_is!=sw1_should ) { printf(" sw1=%.2X",sw1); ret=FALSE; }
	if( sw2_is!=sw2_should ) { printf(" sw2=%.2X",sw2); ret=FALSE; }

	return(ret);
} 

void printrsp( BYTE *rsp, WORD lenr )
{
	int i;

	if( lenr>0 ) {
		printf(" rsp=");
		if(lenr>(255+2)) lenr=255+2;
		for(i=0;i<lenr;i++) printf("%.2X",rsp[i]);
	}
}

/* Test 1: Reset CT */

BOOLEAN test1( WORD ctn, CTAPI_INFO *ci )
{
	BYTE cmd[]={0x20,0x11,0x00,0x00};
	BYTE rsp[260];
	BYTE sad=2, dad=1;
	WORD lenc=4, lenr=2;
	UBYTE ret;

	printf("Test 1:");

	memset( rsp, 0, sizeof(rsp) );

	ret = (*CT_data)( ctn, &dad, &sad, lenc, cmd, &lenr, rsp );

	if( (ret!=CTAPI_OK) || (lenr!=2) || (rsp[lenr-2]!=0x90) ||
		(rsp[lenr-1]!=0x00) ) {
		printf(" Error.\n");
#ifdef VERBOSE
		printf("  ");
		checkret(ret,CTAPI_OK);
		checkad(sad,1,dad,2);
		checklenr(lenr,2);
		if(lenr>=2) checksw(rsp[lenr-2],0x90,rsp[lenr-1],0x00);
		printrsp(rsp,lenr);
		printf("\n");
#endif
		return(FALSE);
	}

	printf(" Ok\n");

	return(TRUE);
}

/* Test 2: Request ICC */

BOOLEAN test2( WORD ctn, CTAPI_INFO *ci )
{
	BYTE cmd[]={0x20,0x12,0x01,0x00};
	BYTE rsp[260];
	BYTE sad=2, dad=1;
	WORD lenc=4, lenr=2;
	UBYTE ret;

	printf("Test 2:");

	memset( rsp, 0, sizeof(rsp) );

	ret = (*CT_data)( ctn, &dad, &sad, lenc, cmd, &lenr, rsp );

	if( (ret!=CTAPI_OK) || (lenr!=2) || (rsp[lenr-2]!=0x90) ||
		(rsp[lenr-1]!=0x01) ) {
		printf(" Error.\n");
#ifdef VERBOSE
		printf("  ");
		checkret(ret,CTAPI_OK);
		checkad(sad,1,dad,2);
		checklenr(lenr,2);
		if(lenr>=2) checksw(rsp[lenr-2],0x90,rsp[lenr-1],0x01);
		printrsp(rsp,lenr);
		printf("\n");
#endif
		return(FALSE);
	}

	printf(" Ok\n");

	return(TRUE);
}

/* Test 3: Request ICC with inserted card */

BOOLEAN test3( WORD ctn, CTAPI_INFO *ci )
{
	BYTE cmd[]={0x20,0x12,0x01,0x00};
	BYTE rsp[260];
	BYTE sad=2, dad=1;
	WORD lenc=4, lenr=2;
	UBYTE ret;

	printf("Test 3:");

	memset( rsp, 0, sizeof(rsp) );

	ret = (*CT_data)( ctn, &dad, &sad, lenc, cmd, &lenr, rsp );

	if( (ret!=CTAPI_OK) || (lenr!=2) || (rsp[lenr-2]!=0x62) ||
		(rsp[lenr-1]!=0x01) ) {
		printf(" Error.\n");
#ifdef VERBOSE
		printf("  ");
		checkret(ret,CTAPI_OK);
		checkad(sad,1,dad,2);
		checklenr(lenr,2);
		if(lenr>=2) checksw(rsp[lenr-2],0x62,rsp[lenr-1],0x01);
		printrsp(rsp,lenr);
		printf("\n");
#endif
		return(FALSE);
	}

	printf(" Ok\n");

	return(TRUE);
}

/* Test 4: Request ICC returning ATR */

BOOLEAN test4( WORD ctn, CTAPI_INFO *ci )
{
	BYTE cmd[]={0x20,0x12,0x01,0x01,0x00};
	BYTE rsp[260];
	BYTE sad=2, dad=1;
	WORD lenc=5, lenr=250;
	UBYTE ret;

	printf("Test 4:");

	eject( ctn );

	memset( rsp, 0, sizeof(rsp) );

	ret = (*CT_data)( ctn, &dad, &sad, lenc, cmd, &lenr, rsp );

	if( (ret!=CTAPI_OK) || (lenr<4) || (rsp[lenr-2]!=0x90) ||
		(rsp[lenr-1]!=0x01) || ((rsp[0]!=0x3B)&&(rsp[0]!=0x3F)) ) {
		printf(" Error.\n");
#ifdef VERBOSE
		printf("  ");
		checkret(ret,CTAPI_OK);
		checkad(sad,1,dad,2);
		if(lenr>=2) checksw(rsp[lenr-2],0x90,rsp[lenr-1],0x01);
		printrsp(rsp,lenr);
		printf("\n");
#endif
		return(FALSE);
	}

	printf(" Ok\n");

#ifdef VERBOSE
	printf("  ");
	printrsp(rsp,lenr);
	printf("\n");
#endif

	return(TRUE);
}

/* Test 5: Request ICC returning ATR and limiting response length */

BOOLEAN test5( WORD ctn, CTAPI_INFO *ci )
{
	BYTE cmd[]={0x20,0x12,0x01,0x01,0x00};
	BYTE rsp[260];
	BYTE sad=2, dad=1;
	WORD lenc=5, lenr=2;
	UBYTE ret;

	printf("Test 5:");

	eject( ctn );

	memset( rsp, 0, sizeof(rsp) );

	ret = (*CT_data)( ctn, &dad, &sad, lenc, cmd, &lenr, rsp );

	if( ret!=CTAPI_ERR_MEMORY ) {
		printf(" Error.\n");
#ifdef VERBOSE
		printf("  ");
		checkret(ret,CTAPI_ERR_MEMORY);
		printf("\n");
#endif
		return(FALSE);
	}

	printf(" Ok\n");

	return(TRUE);
}

/* Test 6: Request ICC returning Historical Bytes */

BOOLEAN test6( WORD ctn, CTAPI_INFO *ci )
{
	BYTE cmd[]={0x20,0x12,0x01,0x02,0x00};
	BYTE rsp[260];
	BYTE sad=2, dad=1;
	WORD lenc=5, lenr=250;
	UBYTE ret;

	printf("Test 6:");

	eject( ctn );

	memset( rsp, 0, sizeof(rsp) );

	ret = (*CT_data)( ctn, &dad, &sad, lenc, cmd, &lenr, rsp );

	if( (ret!=CTAPI_OK) || (lenr<4) || (rsp[lenr-2]!=0x90) ||
		(rsp[lenr-1]!=0x01) ) {
		printf(" Error.\n");
#ifdef VERBOSE
		printf("  ");
		checkret(ret,CTAPI_OK);
		checkad(sad,1,dad,2);
		if(lenr>=2) checksw(rsp[lenr-2],0x90,rsp[lenr-1],0x01);
		printrsp(rsp,lenr);
		printf("\n");
#endif
		return(FALSE);
	}

	printf(" Ok\n");

#ifdef VERBOSE
	printf("  ");
	printrsp(rsp,lenr);
	printf("\n");
#endif

	return(TRUE);
}

/* Test 7: Request ICC returning Historical Bytes and limiting response
 *         length
 */

BOOLEAN test7( WORD ctn, CTAPI_INFO *ci )
{
	BYTE cmd[]={0x20,0x12,0x01,0x02,0x00};
	BYTE rsp[260];
	BYTE sad=2, dad=1;
	WORD lenc=5, lenr=2;
	UBYTE ret;

	printf("Test 7:");

	eject( ctn );

	memset( rsp, 0, sizeof(rsp) );

	ret = (*CT_data)( ctn, &dad, &sad, lenc, cmd, &lenr, rsp );

	if( ret!=CTAPI_ERR_MEMORY ) {
		printf(" Error.\n");
#ifdef VERBOSE
		printf("  ");
		checkret(ret,CTAPI_ERR_MEMORY);
		printf("\n");
#endif
		return(FALSE);
	}

	printf(" Ok\n");

	return(TRUE);
}

/* Test 8: Reset ICC */

BOOLEAN test8( WORD ctn, CTAPI_INFO *ci )
{
	BYTE cmd[]={0x20,0x11,0x01,0x00};
	BYTE rsp[260];
	BYTE sad=2, dad=1;
	WORD lenc=4, lenr=2;
	UBYTE ret;

	printf("Test 8:");

	memset( rsp, 0, sizeof(rsp) );

	ret = (*CT_data)( ctn, &dad, &sad, lenc, cmd, &lenr, rsp );

	if( (ret!=CTAPI_OK) || (lenr!=2) || (rsp[lenr-2]!=0x90) ||
		(rsp[lenr-1]!=0x01) ) {
		printf(" Error.\n");
#ifdef VERBOSE
		printf("  ");
		checkret(ret,CTAPI_OK);
		checkad(sad,1,dad,2);
		checklenr(lenr,2);
		if(lenr>=2) checksw(rsp[lenr-2],0x90,rsp[lenr-1],0x01);
		printf("\n");
#endif
		return(FALSE);
	}

	printf(" Ok\n");

	return(TRUE);
}

/* Test 9: Reset ICC returning ATR */

BOOLEAN test9( WORD ctn, CTAPI_INFO *ci )
{
	BYTE cmd[]={0x20,0x11,0x01,0x01,0x00};
	BYTE rsp[260];
	BYTE sad=2, dad=1;
	WORD lenc=5, lenr=250;
	UBYTE ret;

	printf("Test 9:");

	memset( rsp, 0, sizeof(rsp) );

	ret = (*CT_data)( ctn, &dad, &sad, lenc, cmd, &lenr, rsp );

	if( (ret!=CTAPI_OK) || (lenr<4) || (rsp[lenr-2]!=0x90) ||
		(rsp[lenr-1]!=0x01) || ((rsp[0]!=0x3B)&&(rsp[0]!=0x3F)) ) {
		printf(" Error.\n");
#ifdef VERBOSE
		printf("  ");
		checkret(ret,CTAPI_OK);
		checkad(sad,1,dad,2);
		if(lenr>=2) checksw(rsp[lenr-2],0x90,rsp[lenr-1],0x01);
		printrsp(rsp,lenr);
		printf("\n");
#endif
		return(FALSE);
	}

	printf(" Ok\n");

#ifdef VERBOSE
	printf("  ");
	printrsp(rsp,lenr);
	printf("\n");
#endif

	return(TRUE);
}

/* Test 10: Reset ICC returning ATR and limiting response length */

BOOLEAN test10( WORD ctn, CTAPI_INFO *ci )
{
	BYTE cmd[]={0x20,0x11,0x01,0x01,0x00};
	BYTE rsp[260];
	BYTE sad=2, dad=1;
	WORD lenc=5, lenr=2;
	UBYTE ret;

	printf("Test 10:");

	memset( rsp, 0, sizeof(rsp) );

	ret = (*CT_data)( ctn, &dad, &sad, lenc, cmd, &lenr, rsp );

	if( ret!=CTAPI_ERR_MEMORY ) {
		printf(" Error.\n");
#ifdef VERBOSE
		printf("  ");
		checkret(ret,CTAPI_ERR_MEMORY);
		printf("\n");
#endif
		return(FALSE);
	}

	printf(" Ok\n");

	return(TRUE);
}

/* Test 11: Reset ICC returning Historical Bytes */

BOOLEAN test11( WORD ctn, CTAPI_INFO *ci )
{
	BYTE cmd[]={0x20,0x11,0x01,0x02,0x00};
	BYTE rsp[260];
	BYTE sad=2, dad=1;
	WORD lenc=5, lenr=250;
	UBYTE ret;

	printf("Test 11:");

	memset( rsp, 0, sizeof(rsp) );

	ret = (*CT_data)( ctn, &dad, &sad, lenc, cmd, &lenr, rsp );

	if( (ret!=CTAPI_OK) || (lenr<4) || (rsp[lenr-2]!=0x90) ||
		(rsp[lenr-1]!=0x01) ) {
		printf(" Error.\n");
#ifdef VERBOSE
		printf("  ");
		checkret(ret,CTAPI_OK);
		checkad(sad,1,dad,2);
		if(lenr>=2) checksw(rsp[lenr-2],0x90,rsp[lenr-1],0x01);
		printrsp(rsp,lenr);
		printf("\n");
#endif
		return(FALSE);
	}

	printf(" Ok\n");

#ifdef VERBOSE
	printf("  ");
	printrsp(rsp,lenr);
	printf("\n");
#endif

	return(TRUE);
}

/* Test 12: Reset ICC returning Historical Bytes and limiting response
 *         length
 */

BOOLEAN test12( WORD ctn, CTAPI_INFO *ci )
{
	BYTE cmd[]={0x20,0x11,0x01,0x02,0x00};
	BYTE rsp[260];
	BYTE sad=2, dad=1;
	WORD lenc=5, lenr=2;
	UBYTE ret;

	printf("Test 12:");

	memset( rsp, 0, sizeof(rsp) );

	ret = (*CT_data)( ctn, &dad, &sad, lenc, cmd, &lenr, rsp );

	if( ret!=CTAPI_ERR_MEMORY ) {
		printf(" Error.\n");
#ifdef VERBOSE
		printf("  ");
		checkret(ret,CTAPI_ERR_MEMORY);
		printf("\n");
#endif
		return(FALSE);
	}

	printf(" Ok\n");

	return(TRUE);
}

/* Test 13: Get Status of IFD */

BOOLEAN test13( WORD ctn, CTAPI_INFO *ci )
{
	BYTE cmd[]={0x20,0x13,0x00,0x46,0x00};
	BYTE rsp[260];
	BYTE sad=2, dad=1;
	WORD lenc=5, lenr=250;
	UBYTE ret;

	printf("Test 13:");

	memset( rsp, 0, sizeof(rsp) );

	ret = (*CT_data)( ctn, &dad, &sad, lenc, cmd, &lenr, rsp );

	if( (ret!=CTAPI_OK) || (lenr<17) || (rsp[lenr-2]!=0x90) ||
		(rsp[lenr-1]!=0x00) ) {
		printf(" Error.\n");
#ifdef VERBOSE
		printf("  ");
		checkret(ret,CTAPI_OK);
		checkad(sad,1,dad,2);
		if( lenr<17 ) printf(" lenr=%d",lenr);
		if(lenr>=2) checksw(rsp[lenr-2],0x90,rsp[lenr-1],0x00);
		printrsp(rsp,lenr);
		printf("\n");
#endif
		return(FALSE);
	}

	if( (rsp[0]==0x46) && ((rsp[1]+4)==lenr) )
		ci->bcsver=CTAPI_BCS_VERSION_1_0;
	else
		ci->bcsver=CTAPI_BCS_VERSION_0_9;

	printf(" Ok\n");

#ifdef VERBOSE
	printf("  ");
	printrsp(rsp,lenr);
	printf("\n");
#endif

	return(TRUE);
}

/* Test 14: Get Status of IFD limiting response */

BOOLEAN test14( WORD ctn, CTAPI_INFO *ci )
{
	BYTE cmd[]={0x20,0x13,0x00,0x46,0x00};
	BYTE rsp[260];
	BYTE sad=2, dad=1;
	WORD lenc=5, lenr=2;
	UBYTE ret;

	printf("Test 14:");

	memset( rsp, 0, sizeof(rsp) );

	ret = (*CT_data)( ctn, &dad, &sad, lenc, cmd, &lenr, rsp );

	if( ret!=CTAPI_ERR_MEMORY ) {
		printf(" Error.\n");
#ifdef VERBOSE
		printf("  ");
		checkret(ret,CTAPI_ERR_MEMORY);
		printf("\n");
#endif
		return(FALSE);
	}

	printf(" Ok\n");

	return(TRUE);
}

/* Test 15: Get Status of ICCs */

BOOLEAN test15( WORD ctn, CTAPI_INFO *ci )
{
	BYTE cmd[]={0x20,0x13,0x00,0x80,0x00};
	BYTE rsp[260];
	BYTE sad=2, dad=1;
	WORD lenc=5, lenr=250;
	UBYTE ret;

	printf("Test 15:");

	memset( rsp, 0, sizeof(rsp) );

	ret = (*CT_data)( ctn, &dad, &sad, lenc, cmd, &lenr, rsp );

	if( (ret!=CTAPI_OK) || (lenr<3) || (rsp[lenr-2]!=0x90) ||
		(rsp[lenr-1]!=0x00) ) {
		printf(" Error.\n");
#ifdef VERBOSE
		printf("  ");
		checkret(ret,CTAPI_OK);
		checkad(sad,1,dad,2);
		if( lenr<3 ) printf(" lenr=%d",lenr);
		if(lenr>=2) checksw(rsp[lenr-2],0x90,rsp[lenr-1],0x00);
		printrsp(rsp,lenr);
		printf("\n");
#endif
		return(FALSE);
	}

	if( ci->bcsver==CTAPI_BCS_VERSION_UNKNOWN ) {
		if( (rsp[0]==0x80) && ((rsp[1]+4)==lenr) )
			ci->bcsver=CTAPI_BCS_VERSION_1_0;
		else
			ci->bcsver=CTAPI_BCS_VERSION_0_9;
	} else if( (ci->bcsver==CTAPI_BCS_VERSION_0_9) && (rsp[0]!=0x05) ) {
		printf(" Error.\n");
#ifdef VERBOSE
		printf("  ");
		checkad(sad,1,dad,2);
		printrsp(rsp,lenr);
		printf("\n");
#endif
		return(FALSE);
	} else if( (ci->bcsver==CTAPI_BCS_VERSION_1_0) &&
		( (rsp[0]!=0x80) || ((rsp[1]+4)!=lenr) || (rsp[2]!=0x05) ) ) {
		printf(" Error.\n");
#ifdef VERBOSE
		printf("  ");
		checkad(sad,1,dad,2);
		printrsp(rsp,lenr);
		printf("\n");
#endif
		return(FALSE);
	}

	printf(" Ok\n");

#ifdef VERBOSE
	printf("  ");
	printrsp(rsp,lenr);
	printf("\n");
#endif

	return(TRUE);
}

/* Test 16: Get Status of ICCs limiting response */

BOOLEAN test16( WORD ctn, CTAPI_INFO *ci )
{
	BYTE cmd[]={0x20,0x13,0x00,0x80,0x00};
	BYTE rsp[260];
	BYTE sad=2, dad=1;
	WORD lenc=5, lenr=2;
	UBYTE ret;

	printf("Test 16:");

	memset( rsp, 0, sizeof(rsp) );

	ret = (*CT_data)( ctn, &dad, &sad, lenc, cmd, &lenr, rsp );

	if( ret!=CTAPI_ERR_MEMORY ) {
		printf(" Error.\n");
#ifdef VERBOSE
		printf("  ");
		checkret(ret,CTAPI_ERR_MEMORY);
		printf("\n");
#endif
		return(FALSE);
	}

	printf(" Ok\n");

	return(TRUE);
}

/* Test 17: Eject ICC */
/* INTERPRETATION */

BOOLEAN test17( WORD ctn, CTAPI_INFO *ci )
{
	BYTE cmd[]={0x20,0x15,0x01,0x00};
	BYTE rsp[260];
	BYTE sad=2, dad=1;
	WORD lenc=4, lenr=16;
	UBYTE ret;

	printf("Test 17:");

	memset( rsp, 0, sizeof(rsp) );

	ret = (*CT_data)( ctn, &dad, &sad, lenc, cmd, &lenr, rsp );

	if( (ret!=CTAPI_OK) || (lenr!=2) || (rsp[lenr-2]!=0x90) ||
		((rsp[lenr-1]&0xFE)!=0x00) ) {
		printf(" Error.\n");
#ifdef VERBOSE
		printf("  ");
		checkret(ret,CTAPI_OK);
		checkad(sad,1,dad,2);
		checklenr(lenr,2);
		if(lenr>=2) checkswext(rsp[lenr-2],0x90,rsp[lenr-1]|0x01,0x01,
			rsp[lenr-2],rsp[lenr-1]);
		printrsp(rsp,lenr);
		printf("\n");
#endif
		return(FALSE);
	}

	printf(" Ok\n");

	return(TRUE);
}

/* Test 18: Send command to non-existing slot */
/* INTERPRETATION */

BOOLEAN test18( WORD ctn, CTAPI_INFO *ci )
{
	BYTE cmd[]={0x00,0xA4,0x00,0x00,0x02,0x3F,0x00};
	BYTE rsp[260];
	BYTE sad=2, dad=10;
	WORD lenc=7, lenr=250;
	UBYTE ret;

	printf("Test 18:");

	memset( rsp, 0, sizeof(rsp) );

	ret = (*CT_data)( ctn, &dad, &sad, lenc, cmd, &lenr, rsp );

	if( (ret!=CTAPI_ERR_INVALID) ) {
		printf(" Error.\n");
#ifdef VERBOSE
		printf("  ");
		checkret(ret,CTAPI_ERR_INVALID);
		printf("\n");
#endif
		return(FALSE);
	}

	printf(" Ok\n");

	return(TRUE);
}

/* Test 19: Send command to invalid slot */

BOOLEAN test19( WORD ctn, CTAPI_INFO *ci )
{
	BYTE cmd[]={0x00,0xA4,0x00,0x00,0x02,0x3F,0x00};
	BYTE rsp[260];
	BYTE sad=2, dad=100;
	WORD lenc=7, lenr=250;
	UBYTE ret;

	printf("Test 19:");

	memset( rsp, 0, sizeof(rsp) );

	ret = (*CT_data)( ctn, &dad, &sad, lenc, cmd, &lenr, rsp );

	if( (ret!=CTAPI_ERR_INVALID) ) {
		printf(" Error.\n");
#ifdef VERBOSE
		printf("  ");
		checkret(ret,CTAPI_ERR_INVALID);
		printf("\n");
#endif
		return(FALSE);
	}

	printf(" Ok\n");

	return(TRUE);
}

/* Test 20: Eject ICC with wrong P2. */
/* INTERPRETATION */

BOOLEAN test20( WORD ctn, CTAPI_INFO *ci )
{
	BYTE cmd[]={0x20,0x15,0x01,0x22};
	BYTE rsp[260];
	BYTE sad=2, dad=1;
	WORD lenc=4, lenr=16;
	UBYTE ret;

	printf("Test 20:");

	memset( rsp, 0, sizeof(rsp) );

	ret = (*CT_data)( ctn, &dad, &sad, lenc, cmd, &lenr, rsp );

	if( (ret!=CTAPI_OK) || (lenr!=2) || (rsp[lenr-2]!=0x6A) ||
		(rsp[lenr-1]!=0x00) ) {
		printf(" Error.\n");
#ifdef VERBOSE
		printf("  ");
		checkret(ret,CTAPI_OK);
		checkad(sad,1,dad,2);
		checklenr(lenr,2);
		if(lenr>=2) checksw(rsp[lenr-2],0x6A,rsp[lenr-1],0x00);
		printrsp(rsp,lenr);
		printf("\n");
#endif
		return(FALSE);
	}

	printf(" Ok\n");

	return(TRUE);
}

/* Test 21: Send command with wrong INS. */

BOOLEAN test21( WORD ctn, CTAPI_INFO *ci )
{
	BYTE cmd[]={0x20,0x75,0x01,0x00};
	BYTE rsp[260];
	BYTE sad=2, dad=1;
	WORD lenc=4, lenr=16;
	UBYTE ret;

	printf("Test 21:");

	memset( rsp, 0, sizeof(rsp) );

	ret = (*CT_data)( ctn, &dad, &sad, lenc, cmd, &lenr, rsp );

	if( (ret!=CTAPI_OK) || (lenr!=2) || (rsp[lenr-2]!=0x6D) ||
		(rsp[lenr-1]!=0x00) ) {
		printf(" Error.\n");
#ifdef VERBOSE
		printf("  ");
		checkret(ret,CTAPI_OK);
		checkad(sad,1,dad,2);
		checklenr(lenr,2);
		if(lenr>=2) checksw(rsp[lenr-2],0x6D,rsp[lenr-1],0x00);
		printrsp(rsp,lenr);
		printf("\n");
#endif
		return(FALSE);
	}

	printf(" Ok\n");

	return(TRUE);
}

/* Test 22: Send command with wrong CLA. */

BOOLEAN test22( WORD ctn, CTAPI_INFO *ci )
{
	BYTE cmd[]={0x60,0x15,0x01,0x00};
	BYTE rsp[260];
	BYTE sad=2, dad=1;
	WORD lenc=4, lenr=16;
	UBYTE ret;

	printf("Test 22:");

	memset( rsp, 0, sizeof(rsp) );

	ret = (*CT_data)( ctn, &dad, &sad, lenc, cmd, &lenr, rsp );

	if( (ret!=CTAPI_OK) || (lenr!=2) || (rsp[lenr-2]!=0x6E) ||
		(rsp[lenr-1]!=0x00) ) {
		printf(" Error.\n");
#ifdef VERBOSE
		printf("  ");
		checkret(ret,CTAPI_OK);
		checkad(sad,1,dad,2);
		checklenr(lenr,2);
		if(lenr>=2) checksw(rsp[lenr-2],0x6E,rsp[lenr-1],0x00);
		printrsp(rsp,lenr);
		printf("\n");
#endif
		return(FALSE);
	}

	printf(" Ok\n");

	return(TRUE);
}

/* Test 23: Get Status of ICC */

BOOLEAN test23( WORD ctn, CTAPI_INFO *ci )
{
	BYTE cmd[]={0x20,0x13,0x01,0x80,0x00};
	BYTE rsp[260];
	BYTE sad=2, dad=1;
	WORD lenc=5, lenr=250;
	UBYTE ret;

	printf("Test 23:");

	if( ci->bcsver!=CTAPI_BCS_VERSION_1_0 ) {
		printf(" Skiped\n");
		return(TRUE);
	}

	memset( rsp, 0, sizeof(rsp) );

	ret = (*CT_data)( ctn, &dad, &sad, lenc, cmd, &lenr, rsp );

	if( (ret!=CTAPI_OK) || (lenr!=5) || (rsp[lenr-2]!=0x90) ||
		(rsp[lenr-1]!=0x00) || (rsp[0]!=0x80) || (rsp[1]!=0x01 ) ||
		(rsp[2]!=0x05) ) {
		printf(" Error.\n");
#ifdef VERBOSE
		printf("  ");
		checkret(ret,CTAPI_OK);
		checkad(sad,1,dad,2);
		if( lenr!=5 ) printf(" lenr=%d",lenr);
		if(lenr>=2) checksw(rsp[lenr-2],0x90,rsp[lenr-1],0x00);
		printrsp(rsp,lenr);
		printf("\n");
#endif
		return(FALSE);
	}

	printf(" Ok\n");

#ifdef VERBOSE
	printf("  ");
	printrsp(rsp,lenr);
	printf("\n");
#endif

	return(TRUE);
}

/* Test 24: Get Status of ICC limiting response */

BOOLEAN test24( WORD ctn, CTAPI_INFO *ci )
{
	BYTE cmd[]={0x20,0x13,0x01,0x80,0x00};
	BYTE rsp[260];
	BYTE sad=2, dad=1;
	WORD lenc=5, lenr=2;
	UBYTE ret;

	printf("Test 24:");

	if( ci->bcsver!=CTAPI_BCS_VERSION_1_0 ) {
		printf(" Skiped\n");
		return(TRUE);
	}

	memset( rsp, 0, sizeof(rsp) );

	ret = (*CT_data)( ctn, &dad, &sad, lenc, cmd, &lenr, rsp );

	if( ret!=CTAPI_ERR_MEMORY ) {
		printf(" Error.\n");
#ifdef VERBOSE
		printf("  ");
		checkret(ret,CTAPI_ERR_MEMORY);
		printf("\n");
#endif
		return(FALSE);
	}

	printf(" Ok\n");

	return(TRUE);
}

/* Test 25: Get functional units */

BOOLEAN test25( WORD ctn, CTAPI_INFO *ci )
{
	BYTE cmd[]={0x20,0x13,0x00,0x81,0x00};
	BYTE rsp[260];
	BYTE sad=2, dad=1;
	WORD lenc=5, lenr=250;
	UBYTE ret;

	printf("Test 25:");

	if( ci->bcsver!=CTAPI_BCS_VERSION_1_0 ) {
		printf(" Skiped\n");
		return(TRUE);
	}

	memset( rsp, 0, sizeof(rsp) );

	ret = (*CT_data)( ctn, &dad, &sad, lenc, cmd, &lenr, rsp );

	if( (ret!=CTAPI_OK) || (lenr<5) || (rsp[lenr-2]!=0x90) ||
		(rsp[lenr-1]!=0x00) || (rsp[0]!=0x81) || (rsp[2]!=0x01) ||
		((rsp[1]+4)!=lenr) ) {
		printf(" Error.\n");
#ifdef VERBOSE
		printf("  ");
		checkret(ret,CTAPI_OK);
		checkad(sad,1,dad,2);
		if( lenr<5 ) printf(" lenr=%d",lenr);
		if(lenr>=2) checksw(rsp[lenr-2],0x90,rsp[lenr-1],0x00);
		printrsp(rsp,lenr);
		printf("\n");
#endif
		return(FALSE);
	}

	printf(" Ok\n");

#ifdef VERBOSE
	printf("  ");
	printrsp(rsp,lenr);
	printf("\n");
#endif

	return(TRUE);
}

/* Test 26: Get functional units limiting response */

BOOLEAN test26( WORD ctn, CTAPI_INFO *ci )
{
	BYTE cmd[]={0x20,0x13,0x00,0x81,0x00};
	BYTE rsp[260];
	BYTE sad=2, dad=1;
	WORD lenc=5, lenr=2;
	UBYTE ret;

	printf("Test 26:");

	if( ci->bcsver!=CTAPI_BCS_VERSION_1_0 ) {
		printf(" Skiped\n");
		return(TRUE);
	}

	memset( rsp, 0, sizeof(rsp) );

	ret = (*CT_data)( ctn, &dad, &sad, lenc, cmd, &lenr, rsp );

	if( ret!=CTAPI_ERR_MEMORY ) {
		printf(" Error.\n");
#ifdef VERBOSE
		printf("  ");
		checkret(ret,CTAPI_ERR_MEMORY);
		printf("\n");
#endif
		return(FALSE);
	}

	printf(" Ok\n");

	return(TRUE);
}

/* Test 1b: Reset CT without card */

BOOLEAN test1b( WORD ctn, CTAPI_INFO *ci )
{
	BYTE cmd[]={0x20,0x11,0x00,0x00};
	BYTE rsp[260];
	BYTE sad=2, dad=1;
	WORD lenc=4, lenr=2;
	UBYTE ret;

	printf("Test 1b:");

	memset( rsp, 0, sizeof(rsp) );

	ret = (*CT_data)( ctn, &dad, &sad, lenc, cmd, &lenr, rsp );

	if( (ret!=CTAPI_OK) || (lenr!=2) || (rsp[lenr-2]!=0x90) ||
		(rsp[lenr-1]!=0x00) ) {
		printf(" Error.\n");
#ifdef VERBOSE
		printf("  ");
		checkret(ret,CTAPI_OK);
		checkad(sad,1,dad,2);
		checklenr(lenr,2);
		if(lenr>=2) checksw(rsp[lenr-2],0x90,rsp[lenr-1],0x00);
		printrsp(rsp,lenr);
		printf("\n");
#endif
		return(FALSE);
	}

	printf(" Ok\n");

	return(TRUE);
}

/* Test 2b: Request ICC without card */

BOOLEAN test2b( WORD ctn, CTAPI_INFO *ci )
{
	BYTE cmd[]={0x20,0x12,0x01,0x00};
	BYTE rsp[260];
	BYTE sad=2, dad=1;
	WORD lenc=4, lenr=2;
	UBYTE ret;

	printf("Test 2b");

	memset( rsp, 0, sizeof(rsp) );

	ret = (*CT_data)( ctn, &dad, &sad, lenc, cmd, &lenr, rsp );

	if( (ret!=CTAPI_OK) || (lenr!=2) || ((rsp[lenr-2]&0xF9)!=0x60) ||
		((rsp[lenr-1]&0xFE)!=0x00) ) {
		printf(" Error.\n");
#ifdef VERBOSE
		printf("  ");
		checkret(ret,CTAPI_OK);
		checkad(sad,1,dad,2);
		checklenr(lenr,2);
		if(lenr>=2) checkswext(rsp[lenr-2]&0xF9,0x60,rsp[lenr-1],0x00,
			rsp[lenr-2],rsp[lenr-1]);
		printrsp(rsp,lenr);
		printf("\n");
#endif
		return(FALSE);
	}

	printf(" Ok\n");

	return(TRUE);
}

/* Test 3b: Request ICC without card returning ATR */

BOOLEAN test3b( WORD ctn, CTAPI_INFO *ci )
{
	BYTE cmd[]={0x20,0x12,0x01,0x01,0x00};
	BYTE rsp[260];
	BYTE sad=2, dad=1;
	WORD lenc=5, lenr=250;
	UBYTE ret;

	printf("Test 3b:");

	memset( rsp, 0, sizeof(rsp) );

	ret = (*CT_data)( ctn, &dad, &sad, lenc, cmd, &lenr, rsp );

	if( (ret!=CTAPI_OK) || (lenr!=2) || ((rsp[lenr-2]&0xF9)!=0x60) ||
		((rsp[lenr-1]&0xFE)!=0x00) ) {
		printf(" Error.\n");
#ifdef VERBOSE
		printf("  ");
		checkret(ret,CTAPI_OK);
		checkad(sad,1,dad,2);
		checklenr(lenr,2);
		if(lenr>=2) checkswext(rsp[lenr-2]&0xF9,0x60,rsp[lenr-1],0x00,
			rsp[lenr-2],rsp[lenr-1]);
		printrsp(rsp,lenr);
		printf("\n");
#endif
		return(FALSE);
	}

	printf(" Ok\n");

	return(TRUE);
}

/* Test 5b: Request ICC without card returning Historical Bytes */

BOOLEAN test5b( WORD ctn, CTAPI_INFO *ci )
{
	BYTE cmd[]={0x20,0x12,0x01,0x02,0x00};
	BYTE rsp[260];
	BYTE sad=2, dad=1;
	WORD lenc=5, lenr=250;
	UBYTE ret;

	printf("Test 5b:");

	memset( rsp, 0, sizeof(rsp) );

	ret = (*CT_data)( ctn, &dad, &sad, lenc, cmd, &lenr, rsp );

	if( (ret!=CTAPI_OK) || (lenr!=2) || ((rsp[lenr-2]&0xF9)!=0x60) ||
		((rsp[lenr-1]&0xFE)!=0x00) ) {
		printf(" Error.\n");
#ifdef VERBOSE
		printf("  ");
		checkret(ret,CTAPI_OK);
		checkad(sad,1,dad,2);
		checklenr(lenr,2);
		if(lenr>=2) checkswext(rsp[lenr-2]&0xF9,0x60,rsp[lenr-1],0x00,
			rsp[lenr-2],rsp[lenr-1]);
		printrsp(rsp,lenr);
		printf("\n");
#endif
		return(FALSE);
	}

	printf(" Ok\n");

	return(TRUE);
}

/* Test 7b: Reset ICC without card */

BOOLEAN test7b( WORD ctn, CTAPI_INFO *ci )
{
	BYTE cmd[]={0x20,0x11,0x01,0x00};
	BYTE rsp[260];
	BYTE sad=2, dad=1;
	WORD lenc=4, lenr=2;
	UBYTE ret;

	printf("Test 7b:");

	memset( rsp, 0, sizeof(rsp) );

	ret = (*CT_data)( ctn, &dad, &sad, lenc, cmd, &lenr, rsp );

	if( (ret!=CTAPI_OK) || (lenr!=2) || (rsp[lenr-2]!=0x64) ||
		(rsp[lenr-1]!=0x00) ) {
		printf(" Error.\n");
#ifdef VERBOSE
		printf("  ");
		checkret(ret,CTAPI_OK);
		checkad(sad,1,dad,2);
		checklenr(lenr,2);
		if(lenr>=2) checksw(rsp[lenr-2],0x64,rsp[lenr-1],0x00);
		printrsp(rsp,lenr);
		printf("\n");
#endif
		return(FALSE);
	}

	printf(" Ok\n");

	return(TRUE);
}

/* Test 8b: Reset ICC without card returning ATR */

BOOLEAN test8b( WORD ctn, CTAPI_INFO *ci )
{
	BYTE cmd[]={0x20,0x11,0x01,0x01,0x00};
	BYTE rsp[260];
	BYTE sad=2, dad=1;
	WORD lenc=5, lenr=250;
	UBYTE ret;

	printf("Test 8b:");

	memset( rsp, 0, sizeof(rsp) );

	ret = (*CT_data)( ctn, &dad, &sad, lenc, cmd, &lenr, rsp );

	if( (ret!=CTAPI_OK) || (lenr!=2) || (rsp[lenr-2]!=0x64) ||
		(rsp[lenr-1]!=0x00) ) {
		printf(" Error.\n");
#ifdef VERBOSE
		printf("  ");
		checkret(ret,CTAPI_OK);
		checkad(sad,1,dad,2);
		checklenr(lenr,2);
		if(lenr>=2) checksw(rsp[lenr-2],0x64,rsp[lenr-1],0x00);
		printrsp(rsp,lenr);
		printf("\n");
#endif
		return(FALSE);
	}

	printf(" Ok\n");

	return(TRUE);
}

/* Test 10b: Reset ICC without card returning Historical Bytes */

BOOLEAN test10b( WORD ctn, CTAPI_INFO *ci )
{
	BYTE cmd[]={0x20,0x11,0x01,0x02,0x00};
	BYTE rsp[260];
	BYTE sad=2, dad=1;
	WORD lenc=5, lenr=250;
	UBYTE ret;

	printf("Test 10b:");

	memset( rsp, 0, sizeof(rsp) );

	ret = (*CT_data)( ctn, &dad, &sad, lenc, cmd, &lenr, rsp );

	if( (ret!=CTAPI_OK) || (lenr!=2) || (rsp[lenr-2]!=0x64) ||
		(rsp[lenr-1]!=0x00) ) {
		printf(" Error.\n");
#ifdef VERBOSE
		printf("  ");
		checkret(ret,CTAPI_OK);
		checkad(sad,1,dad,2);
		checklenr(lenr,2);
		if(lenr>=2) checksw(rsp[lenr-2],0x64,rsp[lenr-1],0x00);
		printrsp(rsp,lenr);
		printf("\n");
#endif
		return(FALSE);
	}

	printf(" Ok\n");

#ifdef VERBOSE
	printf("  ");
	printrsp(rsp,lenr);
	printf("\n");
#endif

	return(TRUE);
}

/* Test 12b: Get Status without card of IFD */

BOOLEAN test12b( WORD ctn, CTAPI_INFO *ci )
{
	BYTE cmd[]={0x20,0x13,0x00,0x46,0x00};
	BYTE rsp[260];
	BYTE sad=2, dad=1;
	WORD lenc=5, lenr=250;
	UBYTE ret;

	printf("Test 12b:");

	memset( rsp, 0, sizeof(rsp) );

	ret = (*CT_data)( ctn, &dad, &sad, lenc, cmd, &lenr, rsp );

	if( (ret!=CTAPI_OK) || (lenr<17) || (rsp[lenr-2]!=0x90) ||
		(rsp[lenr-1]!=0x00) ) {
		printf(" Error.\n");
#ifdef VERBOSE
		printf("  ");
		checkret(ret,CTAPI_OK);
		checkad(sad,1,dad,2);
		if( lenr<17 ) printf(" lenr=%d",lenr);
		if(lenr>=2) checksw(rsp[lenr-2],0x90,rsp[lenr-1],0x00);
		printrsp(rsp,lenr);
		printf("\n");
#endif
		return(FALSE);
	}

	if( ci->bcsver==CTAPI_BCS_VERSION_UNKNOWN ) {
		if( (rsp[0]==0x46) && ((rsp[1]+4)==lenr) )
			ci->bcsver=CTAPI_BCS_VERSION_1_0;
		else
			ci->bcsver=CTAPI_BCS_VERSION_0_9;
	} else if( (ci->bcsver==CTAPI_BCS_VERSION_1_0) &&
		( (rsp[0]!=0x46) || ((rsp[1]+4)!=lenr) ) ) {
		printf(" Error.\n");
#ifdef VERBOSE
		printf("  ");
		checkad(sad,1,dad,2);
		printrsp(rsp,lenr);
		printf("\n");
#endif
		return(FALSE);
	}

	printf(" Ok\n");

#ifdef VERBOSE
	printf("  ");
	printrsp(rsp,lenr);
	printf("\n");
#endif

	return(TRUE);
}

/* Test 13b: Get Status without card of IFD limiting response */

BOOLEAN test13b( WORD ctn, CTAPI_INFO *ci )
{
	BYTE cmd[]={0x20,0x13,0x00,0x46,0x00};
	BYTE rsp[260];
	BYTE sad=2, dad=1;
	WORD lenc=5, lenr=2;
	UBYTE ret;

	printf("Test 13b:");

	memset( rsp, 0, sizeof(rsp) );

	ret = (*CT_data)( ctn, &dad, &sad, lenc, cmd, &lenr, rsp );

	if( ret!=CTAPI_ERR_MEMORY ) {
		printf(" Error.\n");
#ifdef VERBOSE
		printf("  ");
		checkret(ret,CTAPI_ERR_MEMORY);
		printf("\n");
#endif
		return(FALSE);
	}

	printf(" Ok\n");

	return(TRUE);
}

/* Test 14b: Get Status of ICCs without card */

BOOLEAN test14b( WORD ctn, CTAPI_INFO *ci )
{
	BYTE cmd[]={0x20,0x13,0x00,0x80,0x00};
	BYTE rsp[260];
	BYTE sad=2, dad=1;
	WORD lenc=5, lenr=250;
	UBYTE ret;

	printf("Test 14b:");

	memset( rsp, 0, sizeof(rsp) );

	ret = (*CT_data)( ctn, &dad, &sad, lenc, cmd, &lenr, rsp );

	if( (ret!=CTAPI_OK) || (lenr<3) || (rsp[lenr-2]!=0x90) ||
		(rsp[lenr-1]!=0x00) ) {
		printf(" Error.\n");
#ifdef VERBOSE
		printf("  ");
		checkret(ret,CTAPI_OK);
		checkad(sad,1,dad,2);
		if( lenr<3 ) printf(" lenr=%d",lenr);
		if(lenr>=2) checksw(rsp[lenr-2],0x90,rsp[lenr-1],0x00);
		printrsp(rsp,lenr);
		printf("\n");
#endif
		return(FALSE);
	}

	if( ci->bcsver==CTAPI_BCS_VERSION_UNKNOWN ) {
		if( (rsp[0]==0x80) && ((rsp[1]+4)==lenr) )
			ci->bcsver=CTAPI_BCS_VERSION_1_0;
		else
			ci->bcsver=CTAPI_BCS_VERSION_0_9;
	} else if( (ci->bcsver==CTAPI_BCS_VERSION_0_9) && (rsp[0]!=0x00) ) {
		printf(" Error.\n");
#ifdef VERBOSE
		printf("  ");
		checkad(sad,1,dad,2);
		printrsp(rsp,lenr);
		printf("\n");
#endif
		return(FALSE);
	} else if( (ci->bcsver==CTAPI_BCS_VERSION_1_0) &&
		( (rsp[0]!=0x80) || ((rsp[1]+4)!=lenr) || (rsp[2]!=0x00) ) ) {
		printf(" Error.\n");
#ifdef VERBOSE
		printf("  ");
		checkad(sad,1,dad,2);
		printrsp(rsp,lenr);
		printf("\n");
#endif
		return(FALSE);
	}

	printf(" Ok\n");

#ifdef VERBOSE
	printf("  ");
	printrsp(rsp,lenr);
	printf("\n");
#endif

	return(TRUE);
}

/* Test 15b: Get Status of ICCs without card limiting response */

BOOLEAN test15b( WORD ctn, CTAPI_INFO *ci )
{
	BYTE cmd[]={0x20,0x13,0x00,0x80,0x00};
	BYTE rsp[260];
	BYTE sad=2, dad=1;
	WORD lenc=5, lenr=2;
	UBYTE ret;

	printf("Test 15b:");

	memset( rsp, 0, sizeof(rsp) );

	ret = (*CT_data)( ctn, &dad, &sad, lenc, cmd, &lenr, rsp );

	if( ret!=CTAPI_ERR_MEMORY ) {
		printf(" Error.\n");
#ifdef VERBOSE
		printf("  ");
		checkret(ret,CTAPI_ERR_MEMORY);
		printf("\n");
#endif
		return(FALSE);
	}

	printf(" Ok\n");

#ifdef VERBOSE
	printf("  ");
	printrsp(rsp,lenr);
	printf("\n");
#endif

	return(TRUE);
}

/* Test 16b: Eject ICC without card */

BOOLEAN test16b( WORD ctn, CTAPI_INFO *ci )
{
	BYTE cmd[]={0x20,0x15,0x01,0x00};
	BYTE rsp[260];
	BYTE sad=2, dad=1;
	WORD lenc=4, lenr=16;
	UBYTE ret;

	printf("Test 16b:");

	memset( rsp, 0, sizeof(rsp) );

	ret = (*CT_data)( ctn, &dad, &sad, lenc, cmd, &lenr, rsp );

	if( (ret!=CTAPI_OK) || (lenr!=2) || (rsp[lenr-2]==0x64) ) {
		printf(" Error.\n");
#ifdef VERBOSE
		printf("  ");
		checkret(ret,CTAPI_OK);
		checkad(sad,1,dad,2);
		checklenr(lenr,2);
		if(lenr>=2) checksw(rsp[lenr-2],0x64,rsp[lenr-1],0x00);
		printrsp(rsp,lenr);
		printf("\n");
#endif
		return(FALSE);
	}

	printf(" Ok\n");

	return(TRUE);
}

/* Test 17b: Send command without card to non-existing slot */
/* INTERPRETATION */

BOOLEAN test17b( WORD ctn, CTAPI_INFO *ci )
{
	BYTE cmd[]={0x00,0xA4,0x00,0x00,0x02,0x3F,0x00};
	BYTE rsp[260];
	BYTE sad=2, dad=10;
	WORD lenc=7, lenr=250;
	UBYTE ret;

	printf("Test 17b:");

	memset( rsp, 0, sizeof(rsp) );

	ret = (*CT_data)( ctn, &dad, &sad, lenc, cmd, &lenr, rsp );

	if( (ret!=CTAPI_ERR_INVALID) ) {
		printf(" Error.\n");
#ifdef VERBOSE
		printf("  ");
		checkret(ret,CTAPI_ERR_INVALID);
		printf("\n");
#endif
		return(FALSE);
	}

	printf(" Ok\n");

	return(TRUE);
}

/* Test 18b: Send command without card to invalid slot */
/* INTERPRETATION */

BOOLEAN test18b( WORD ctn, CTAPI_INFO *ci )
{
	BYTE cmd[]={0x00,0xA4,0x00,0x00,0x02,0x3F,0x00};
	BYTE rsp[260];
	BYTE sad=2, dad=100;
	WORD lenc=7, lenr=250;
	UBYTE ret;

	printf("Test 18b:");

	memset( rsp, 0, sizeof(rsp) );

	ret = (*CT_data)( ctn, &dad, &sad, lenc, cmd, &lenr, rsp );

	if( (ret!=CTAPI_ERR_INVALID) ) {
		printf(" Error.\n");
#ifdef VERBOSE
		printf("  ");
		checkret(ret,CTAPI_ERR_INVALID);
		printf("\n");
#endif
		return(FALSE);
	}

	printf(" Ok\n");

	return(TRUE);
}

/* Test 22b: Send command to card without card */

BOOLEAN test22b( WORD ctn, CTAPI_INFO *ci )
{
	BYTE cmd[]={0x00,0xA4,0x00,0x00,0x02,0x3F,0x00};
	BYTE rsp[260];
	BYTE sad=2, dad=0;
	WORD lenc=7, lenr=250;
	UBYTE ret;

	printf("Test 22b:");

	memset( rsp, 0, sizeof(rsp) );

	ret = (*CT_data)( ctn, &dad, &sad, lenc, cmd, &lenr, rsp );

	if( (ret!=CTAPI_OK) || (lenr!=2) || (rsp[lenr-2]!=0x6F) ||
		(rsp[lenr-1]!=0x00) || (sad!=1) || (dad!=2) ) {
		printf(" Error.\n");
#ifdef VERBOSE
		printf("  ");
		checkret(ret,CTAPI_OK);
		checkad(sad,1,dad,2);
		checklenr(lenr,2);
		if(lenr>=2) checksw(rsp[lenr-2],0x6F,rsp[lenr-1],0x00);
		printrsp(rsp,lenr);
		printf("\n");
#endif
		return(FALSE);
	}

	printf(" Ok\n");

	return(TRUE);
}

/* Test 1c: Request ICC with unusable card returning ATR */

BOOLEAN test1c( WORD ctn, CTAPI_INFO *ci )
{
	BYTE cmd[]={0x20,0x12,0x01,0x01,0x00};
	BYTE rsp[260];
	BYTE sad=2, dad=1;
	WORD lenc=5, lenr=250;
	UBYTE ret;

	printf("Test 1c:");

	memset( rsp, 0, sizeof(rsp) );

	ret = (*CT_data)( ctn, &dad, &sad, lenc, cmd, &lenr, rsp );

	if( (ret!=CTAPI_OK) || (lenr!=2) || ((rsp[lenr-2])!=0x64) ||
		(rsp[lenr-1]!=0x00) ) {
		printf(" Error.\n");
#ifdef VERBOSE
		printf("  ");
		checkret(ret,CTAPI_OK);
		checkad(sad,1,dad,2);
		checklenr(lenr,2);
		if(lenr>=2) checksw(rsp[lenr-2],0x64,rsp[lenr-1],0x00);
		printrsp(rsp,lenr);
		printf("\n");
#endif
		return(FALSE);
	}

	printf(" Ok\n");

	return(TRUE);
}

/* Test 2c: Reset ICC without card returning ATR */

BOOLEAN test2c( WORD ctn, CTAPI_INFO *ci )
{
	BYTE cmd[]={0x20,0x11,0x01,0x01,0x00};
	BYTE rsp[260];
	BYTE sad=2, dad=1;
	WORD lenc=5, lenr=250;
	UBYTE ret;

	printf("Test 2c:");

	memset( rsp, 0, sizeof(rsp) );

	ret = (*CT_data)( ctn, &dad, &sad, lenc, cmd, &lenr, rsp );

	if( (ret!=CTAPI_OK) || (lenr!=2) || (rsp[lenr-2]!=0x64) ||
		(rsp[lenr-1]!=0x00) ) {
		printf(" Error.\n");
#ifdef VERBOSE
		printf("  ");
		checkret(ret,CTAPI_OK);
		checkad(sad,1,dad,2);
		checklenr(lenr,2);
		if(lenr>=2) checksw(rsp[lenr-2],0x64,rsp[lenr-1],0x00);
		printrsp(rsp,lenr);
		printf("\n");
#endif
		return(FALSE);
	}

	printf(" Ok\n");

	return(TRUE);
}

/* Test 3c: Eject ICC */

BOOLEAN test3c( WORD ctn, CTAPI_INFO *ci )
{
	BYTE cmd[]={0x20,0x15,0x01,0x00};
	BYTE rsp[260];
	BYTE sad=2, dad=1;
	WORD lenc=4, lenr=16;
	UBYTE ret;

	printf("Test 3c:");

	memset( rsp, 0, sizeof(rsp) );

	ret = (*CT_data)( ctn, &dad, &sad, lenc, cmd, &lenr, rsp );

	if( (ret!=CTAPI_OK) || (lenr!=2) || (rsp[lenr-2]!=0x90) ) {
		printf(" Error.\n");
#ifdef VERBOSE
		printf("  ");
		checkret(ret,CTAPI_OK);
		checkad(sad,1,dad,2);
		checklenr(lenr,2);
		if(lenr>=2) checkswext(rsp[lenr-2],0x90,rsp[lenr-1]|0x01,0x01,
			rsp[lenr-2],rsp[lenr-1]);
		printrsp(rsp,lenr);
		printf("\n");
#endif
		return(FALSE);
	}

	printf(" Ok\n");

	return(TRUE);
}

/* Test 1d: Reset CT without terminal */
/* INTERPRETATION */

BOOLEAN test1d( WORD ctn, CTAPI_INFO *ci )
{
	BYTE cmd[]={0x20,0x11,0x00,0x00};
	BYTE rsp[260];
	BYTE sad=2, dad=1;
	WORD lenc=4, lenr=2;
	UBYTE ret;

	printf("Test 1d:");

	memset( rsp, 0, sizeof(rsp) );

	ret = (*CT_data)( ctn, &dad, &sad, lenc, cmd, &lenr, rsp );

	if( ret!=CTAPI_ERR_TRANS ) {
		printf(" Error.\n");
#ifdef VERBOSE
		printf("  ");
		checkret(ret,CTAPI_ERR_TRANS);
		printf("\n");
#endif
		return(FALSE);
	}

	printf(" Ok\n");

	return(TRUE);
}

int main( int argc, char *argv[] )
{
	CTAPI_INFO ci;
	WORD ctn=0, pn;
	UBYTE ret;
	int bad=0, good=0, tmp;

	printf("CT-API Library Test\n\n");

	if( argc!=3 ) {
		printf("Wrong number of arguments.\n");
		return(-1);
	}

	if( (sscanf( argv[2], "%2d", &tmp )!=1) || (tmp<0) || (tmp>15) ) {
		printf("Bad argument.\n");
		return(-1);
	}

	pn=tmp & 0xFFFF;

	printf("Loading CT-API library (%s):",argv[1]);

#ifdef HAS_DLOPEN
	LibHandle = dlopen( argv[1], RTLD_LAZY );
	if( !LibHandle ) {
		printf(" Error.\n");
		return(-1);
	}

	pCT_init = (CT_INIT) dlsym( LibHandle, "CT_init" );
	if( dlerror()!=NULL ) {
		unloadlib( LibHandle );
		printf(" Error.\n");
		return(-1);
	}
	pCT_data = (CT_DATA) dlsym( LibHandle, "CT_data" );
	if( dlerror()!=NULL ) {
		unloadlib( LibHandle );
		printf(" Error.\n");
		return(-1);
	}
	pCT_close = (CT_CLOSE) dlsym( LibHandle, "CT_close" );
	if( dlerror()!=NULL ) {
		unloadlib( LibHandle );
		printf(" Error.\n");
		return(-1);
	}
#elif defined(WINDOWS)
	if( ( LibHandle = LoadLibrary( argv[1] ) ) == NULL_HINSTANCE ) {
		printf(" Error.\n");
		return(-1);
	}

	pCT_init = (CT_INIT) GetProcAddress( LibHandle, "CT_init" );
	pCT_data = (CT_DATA) GetProcAddress( LibHandle, "CT_data" );
	pCT_close = (CT_CLOSE) GetProcAddress( LibHandle, "CT_close" );

	/* Make sure we got valid pointers for every card function */
	if( pCT_init == NULL || pCT_data == NULL || pCT_close == NULL ) {
		unloadlib( LibHandle );
		printf(" Error.\n");
		return(-1);
	}
#endif /* HAS_DLOPEN */

	printf(" Ok.\n");

	fprintf(stderr,"Connect terminal, insert card and press Enter");
	tmp=getchar();

	ci.bcsver=CTAPI_BCS_VERSION_UNKNOWN;
	memset( ci.atr, 0, sizeof(ci.atr) );
	ci.atrlen=0;

	/* Init */
	printf("Initialising CT-API library:");
	ret = (*CT_init)( ctn, pn );
	if( ret!=CTAPI_OK ) {
		unloadlib( LibHandle );
		printf(" Error. (%d)\n",ret);
		return(-1);
	}
	printf(" Ok.\n");

	if( test1( ctn, &ci ) ) good++; else bad++;
	if( test2( ctn, &ci ) ) good++; else bad++;
	if( test3( ctn, &ci ) ) good++; else bad++;
	if( test4( ctn, &ci ) ) good++; else bad++;
	if( test5( ctn, &ci ) ) good++; else bad++;
	if( test6( ctn, &ci ) ) good++; else bad++;
	if( test7( ctn, &ci ) ) good++; else bad++;
	if( test8( ctn, &ci ) ) good++; else bad++;
	if( test9( ctn, &ci ) ) good++; else bad++;
	if( test10( ctn, &ci ) ) good++; else bad++;
	if( test11( ctn, &ci ) ) good++; else bad++;
	if( test12( ctn, &ci ) ) good++; else bad++;
	if( test13( ctn, &ci ) ) good++; else bad++;
	if( test14( ctn, &ci ) ) good++; else bad++;
	if( test15( ctn, &ci ) ) good++; else bad++;
	if( test16( ctn, &ci ) ) good++; else bad++;
	if( test23( ctn, &ci ) ) good++; else bad++;
	if( test24( ctn, &ci ) ) good++; else bad++;
	if( test25( ctn, &ci ) ) good++; else bad++;
	if( test26( ctn, &ci ) ) good++; else bad++;
	if( test17( ctn, &ci ) ) good++; else bad++;
	if( test18( ctn, &ci ) ) good++; else bad++;
	if( test19( ctn, &ci ) ) good++; else bad++;
	if( test20( ctn, &ci ) ) good++; else bad++;
	if( test21( ctn, &ci ) ) good++; else bad++;
	if( test22( ctn, &ci ) ) good++; else bad++;
	fprintf(stderr,"Remove card and press Enter");
	tmp=getchar();
	if( test1b( ctn, &ci ) ) good++; else bad++;
	if( test2b( ctn, &ci ) ) good++; else bad++;
	if( test3b( ctn, &ci ) ) good++; else bad++;
	if( test5b( ctn, &ci ) ) good++; else bad++;
	if( test7b( ctn, &ci ) ) good++; else bad++;
	if( test8b( ctn, &ci ) ) good++; else bad++;
	if( test10b( ctn, &ci ) ) good++; else bad++;
	if( test12b( ctn, &ci ) ) good++; else bad++;
	if( test13b( ctn, &ci ) ) good++; else bad++;
	if( test14b( ctn, &ci ) ) good++; else bad++;
	if( test15b( ctn, &ci ) ) good++; else bad++;
	if( test16b( ctn, &ci ) ) good++; else bad++;
	if( test17b( ctn, &ci ) ) good++; else bad++;
	if( test18b( ctn, &ci ) ) good++; else bad++;
	if( test22b( ctn, &ci ) ) good++; else bad++;
	fprintf(stderr,"Insert card with chip down and press Enter");
	tmp=getchar();
	if( test1c( ctn, &ci ) ) good++; else bad++;
	if( test2c( ctn, &ci ) ) good++; else bad++;
	if( test3c( ctn, &ci ) ) good++; else bad++;
	fprintf(stderr,"Disconnect terminal and press Enter");
	tmp=getchar();
	if( test1d( ctn, &ci ) ) good++; else bad++;

	if( ci.bcsver==CTAPI_BCS_VERSION_UNKNOWN )
		printf("Warning: Implemented CT-BCS version could not be determined.\n");
	else if( ci.bcsver==CTAPI_BCS_VERSION_0_9 )
		printf("Warning: Implemented CT-BCS version is 0.9\n");

	/* Close */
	printf("Closing CT-API library:");
	ret = (*CT_close)( ctn );
	if( ret!=CTAPI_OK ) {
		unloadlib( LibHandle );
		printf(" Error. (%d)\n",ret);
		printf("\n%d out of %d tests passed.\n",good,bad+good);
		return(-1);
	}
	printf(" Ok.\n");

	printf("\n%d (including skiped) out of %d tests passed.\n",good,bad+good);
	printf("Correctness of dad and sad handling was not verified.\n");

	return(0);
}



