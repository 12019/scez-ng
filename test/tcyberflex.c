/* $Id: tcyberflex.c 1617 2005-11-03 17:41:39Z laforge $ */

/*
 * Testfile for all Cyberflex commands.
 */

#if 0
/* TODO */

int scCyberflexCmdExecuteMethod( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	BYTE type, BYTE *data, BYTE datalen );
int scCyberflexCmdManageInstance( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	BYTE op );
int scCyberflexCmdManageProgram( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	BYTE *data, BYTE datalen );
#endif /* 0 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#if 0
#include <sio/sio.h>
#endif
#include <scez/scgeneral.h>
#include <scez/scsmartcard.h>
#include <scez/screader.h>
#include <scez/cards/sccyberflex.h>

/* printf(" SW: %.2X %.2X\n",ci->sw[0],ci->sw[1]); */

#ifndef READER_TYPE
#define READER_TYPE SC_READER_DUMBMOUSE
#endif /* READER_TYPE */
#ifndef READER_SLOT
#define READER_SLOT 1
#endif /* READER_SLOT */
#ifndef READER_PORT
#define READER_PORT "0"
#endif /* READER_PORT */

#define CLA_BYTE	0x00

#define checkreturn(f); if( ret!=0 ) { printf(f); goto exit; }

#define printarray( name, length, array ); \
	printf(name); \
	for( i=0; i<length; i++ ) printf(" %.2X",array[i]); \
	printf("\n");

int main( int argc, char *argv[] )
{
	SC_READER_INFO *ri;
	SC_CARD_INFO *ci;
	SC_READER_CONFIG rc;

	int ret;
	int i;
	BYTE buffer[255];
	BYTE rbuffer[256];
	int resplen;

	/* Authkeys */
	BYTE aut0[]={0xAD,0x9F,0x61,0xFE,0xFA,0x20,0xCE,0x63};
	BYTE aut1[]={0x38,0x12,0xA4,0x19,0xC6,0x3B,0xE7,0x71};
	/* BYTE aut2[]={0x16,0x4D,0x5E,0x40,0x4F,0x27,0x52,0x32}; */
	/* BYTE aut5[]={0x6A,0x21,0x36,0xF5,0xD8,0x0C,0x47,0x83}; */
	BYTE chv1[]={0x30,0x30,0x30,0x30,0x30,0x30,0x30,0x30};
	BYTE unb1[]={0x31,0x31,0x31,0x31,0x31,0x31,0x31,0x31};

	BYTE acl[8];

	BYTE data1[]={0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0A,0x0B};
	BYTE atr[]={0x16,0x94,0x81,0x10,0x06,0x01};

#ifdef WITH_DES
	BYTE int3auth[]={0x00,0x12,0x00,0x02,0x01,0x02,0x03,0x04,0x05,0x06,0x07,
		0x08,0x11,0x22,0x33,0x44,0x55,0x66,0x77,0x88,0x00,0x00};
	BYTE int3key[]={0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x11,0x22,0x33,0x44,
		0x55,0x66,0x77,0x88};
	BYTE mac[8];
#endif

	if( scInit() ) { printf("Exit.\n"); return(1); }

	rc.type=READER_TYPE;
	rc.slot=READER_SLOT;
	rc.param=READER_PORT;

	ret = scReaderGetConfig( argc, argv, &rc );
	if( ret!=SC_EXIT_OK ) {
		printf( "Error getting reader configuration.\n" );
		scEnd();
		return(1);
	};

	ri = scGeneralNewReader( rc.type, rc.slot );
	if( ri==NULL ) { printf("Exit.\n"); scEnd(); return(1); };

	ci = scGeneralNewCard( );
	if( ci==NULL ) { printf("Exit.\n"); scEnd(); return(1); };

	/* Init Reader */
	ret = scReaderInit( ri, rc.param );
	checkreturn("Error: scReaderInit\n");

	/* Activate Card */
	ret = scReaderActivate( ri );
	checkreturn("Error: scReaderActivate\n");

	/* Get Card Status */
	ret = scReaderCardStatus( ri );
	checkreturn("Error: scReader\n");
	if( !(ri->status&SC_CARD_STATUS_PRESENT) )
	{ printf("Error: No Card.\n"); goto exit; }

	/* Reset Card */
	ret= scReaderResetCard( ri, ci );
	checkreturn("Error: scReaderResetCard\n");

	/* Get Card Type */
	ret = scSmartcardGetCardType( ci );
	checkreturn("Error: scReaderGetCardType\n");
	if( (ci->type&0xFFFFFF00)!=SC_CARD_CYBERFLEX ) 
	{ printf("Error: Wrong Card.\n"); goto exit; }

	ci->cla=CLA_BYTE;

#if 0
	SIO_SetLogFile( ri->si, "LogCyberflex.txt" );
#endif

/* Select */

	/* Select MF */
	printf("scCyberflexCmdSelect:");
	ret = scCyberflexCmdSelect( ri, ci, SC_CYBERFLEX_SELECT_FILE, 0x3F00,
		NULL, 0, rbuffer, &resplen );
	if( (ret==0) && (ci->sw[0]==0x90) && (ci->sw[1]==0x00) )
	{ printf(" OK\n"); } else
	{ printf(" Error.\n"); goto exit; }
	printarray( "  rsp:", resplen, rbuffer );

/* Get File ACL */

	/* Get File ACL */
	printf("scCyberflexCmdGetFileACL:");
	ret = scCyberflexCmdGetFileACL( ri, ci, rbuffer, &resplen );
	if( (ret==0) && (ci->sw[0]==0x90) && (ci->sw[1]==0x00) )
	{ printf(" OK\n"); } else
	{ printf(" Error.\n"); goto exit; }
	printarray( "  rsp:", resplen, rbuffer );

/* GetData */

	/* GetData */
	printf("scCyberflexCmdGetData:");
	ret = scCyberflexCmdGetData( ri, ci, rbuffer, &resplen );
	if( (ret==0) && (ci->sw[0]==0x90) && (ci->sw[1]==0x00) )
	{ printf(" OK\n"); } else
	{ printf(" Error.\n"); goto exit; }
	printarray( "  rsp:", resplen, rbuffer );

/* Verify Key */

	/* Verify AUT0 */
	printf("scCyberflexCmdVerifyKey(AUT0):");
	ret = scCyberflexCmdVerifyKey( ri, ci, 0, aut0, 8 );
	if( (ret==0) && (ci->sw[0]==0x90) && (ci->sw[1]==0x00) )
	{ printf(" OK\n"); } else
	{ printf(" Error.\n"); goto exit; }

/* Verify CHV */

	/* Verify CHV */
	printf("scCyberflexCmdVerifyCHV(CHV1):");
	ret = scCyberflexCmdVerifyCHV( ri, ci, 1, chv1 );
	if( (ret==0) && (ci->sw[0]==0x90) && (ci->sw[1]==0x00) )
	{ printf(" OK\n"); } else
	{ printf(" Error.\n"); goto exit; }

/* Disable CHV */

	/* Disable CHV */
	printf("scCyberflexCmdDisableCHV(CHV1):");
	ret = scCyberflexCmdDisableCHV( ri, ci, chv1 );
	if( (ret==0) && (ci->sw[0]==0x90) && (ci->sw[1]==0x00) )
	{ printf(" OK\n"); } else
	{ printf(" Error.\n"); goto exit; }

/* Enable CHV */

	/* Enable CHV */
	printf("scCyberflexCmdEnableCHV(CHV1):");
	ret = scCyberflexCmdEnableCHV( ri, ci, chv1 );
	if( (ret==0) && (ci->sw[0]==0x90) && (ci->sw[1]==0x00) )
	{ printf(" OK\n"); } else
	{ printf(" Error.\n"); goto exit; }

/* Change CHV */

	/* Change CHV */
	printf("scCyberflexCmdChangeCHV(CHV1):");
	ret = scCyberflexCmdChangeCHV( ri, ci, 1, chv1, chv1 );
	if( (ret==0) && (ci->sw[0]==0x90) && (ci->sw[1]==0x00) )
	{ printf(" OK\n"); } else
	{ printf(" Error.\n"); goto exit; }

/* Unblock CHV */

	/* Unblock CHV */
	printf("scCyberflexCmdUnblockCHV(CHV1):");
	ret = scCyberflexCmdUnblockCHV( ri, ci, 1, unb1, chv1 );
	if( (ret==0) && (ci->sw[0]==0x90) && (ci->sw[1]==0x00) )
	{ printf(" OK\n"); } else
	{ printf(" Error.\n"); goto exit; }

/* Ask Random */

	/* Ask Random */
	printf("scCyberflexCmdAskRandom:");
	resplen=8;
	ret = scCyberflexCmdAskRandom( ri, ci, rbuffer, &resplen );
	if( (ret==0) && (ci->sw[0]==0x90) && (ci->sw[1]==0x00) )
	{ printf(" OK\n"); } else
	{ printf(" Error.\n"); goto exit; }
	printarray( "  rsp:", resplen, rbuffer );

#ifdef WITH_DES
/* External Authenticate */

	/* External Authenticate */
	printf("scCyberflexCmdExtAuthDES(AUT1):");
	ret = scCyberflexCmdExtAuthDES( ri, ci, 0x01, aut1, rbuffer,
		SC_CYBERFLEX_ALGO_DES );
	if( (ret==0) && (ci->sw[0]==0x90) && (ci->sw[1]==0x00) )
	{ printf(" OK\n"); } else
	{ printf(" Error.\n"); goto exit; }
#endif

#if 0
	/* Verify AUT2 */
	printf("scCyberflexCmdVerifyKey(AUT2):");
	ret = scCyberflexCmdVerifyKey( ri, ci, 2, aut2, 8 );
	if( (ret==0) && (ci->sw[0]==0x90) && (ci->sw[1]==0x00) )
	{ printf(" OK\n"); } else
	{ printf(" Error.\n"); goto exit; }
#endif

/* Dir */

	/* Dir */
	printf("scCyberflexCmdDirectory:");
	resplen=0x17;
	ret = scCyberflexCmdDirectory( ri, ci, 0, rbuffer, &resplen );
	if( (ret==0) && (ci->sw[0]==0x90) && (ci->sw[1]==0x00) )
	{ printf(" OK\n"); } else
	{ printf(" Error.\n"); goto exit; }
	printarray( "  rsp:", resplen, rbuffer );

	/* Select DF */
	ret = scCyberflexCmdSelect( ri, ci, SC_CYBERFLEX_SELECT_FILE, 0x2000,
		NULL, 0, rbuffer, &resplen );
	if( ret!=0 ) { printf(" Error.\n"); goto exit; }
	if( (ci->sw[0]==0x90) && (ci->sw[1]==0x00) ) {
		/* Delete directory file 2000 */
		ret = scCyberflexCmdDeleteFile( ri, ci, SC_CYBERFLEX_FID_CURRENT );
		if(ret!=0) { printf(" Error deleting 2000.\n"); goto exit; }

		ret = scCyberflexCmdSelect( ri, ci, SC_CYBERFLEX_SELECT_FILE, 0x3F00,
			NULL, 0, rbuffer, &resplen );
		if( (ret==0) && (ci->sw[0]==0x90) && (ci->sw[1]==0x00) )
		{ } else
		{ printf(" Error.\n"); goto exit; }

		/* Delete directory file 2000 */
		ret = scCyberflexCmdDeleteFile( ri, ci, 0x2000 );
		if(ret!=0) { printf(" Error deleting 2000.\n"); goto exit; }
	}

	/* Delete file 0001 */
	ret = scCyberflexCmdDeleteFile( ri, ci, 0x0001 );
	if(ret!=0) { printf(" Error deleting 0001.\n"); goto exit; }

/* ChangeJavaATR */

	/* ChangeJavaATR */
	printf("scCyberflexCmdChangeJavaATR:");
	ret = scCyberflexCmdChangeJavaATR( ri, ci, atr, sizeof(atr) );
	if( (ret==0) && (ci->sw[0]==0x90) && (ci->sw[1]==0x00) )
	{ printf(" OK\n"); } else
	{ printf(" Error.\n"); goto exit; }

/* Create */

	/* Create Directory 2000 */
	memcpy( acl, "\x00\x00\x00\xFF\x00\x00\x00\x00", sizeof(acl));
	printf("scCyberflexCmdCreateFile(Dedicated File):");
	ret = scCyberflexCmdCreateFile( ri, ci, 0x2000,
		600, SC_CYBERFLEX_FILE_DIRECTORY, SC_CYBERFLEX_STATUS_UNBLOCKED,
		0, 0, acl );
	if( (ret==0) && (ci->sw[0]==0x90) && (ci->sw[1]==0x00) )
	{ printf(" OK\n"); } else
	{ printf(" Error.\n"); goto exit; }

	/* Select DF */
	ret = scCyberflexCmdSelect( ri, ci, SC_CYBERFLEX_SELECT_FILE, 0x2000,
		NULL, 0, rbuffer, &resplen );
	if( (ret==0) && (ci->sw[0]==0x90) && (ci->sw[1]==0x00) )
	{ } else
	{ printf(" Error.\n"); goto exit; }
	printarray( "  rsp:", resplen, rbuffer );

/* Status */

	/* Status */
	printf("scCyberflexCmdStatus:");
	ret = scCyberflexCmdStatus( ri, ci, rbuffer, &resplen );
	if( (ret==0) && (ci->sw[0]==0x90) && (ci->sw[1]==0x00) )
	{ printf(" OK\n"); } else
	{ printf(" Error.\n"); goto exit; }
	printarray( "  rsp:", resplen, rbuffer );

/* Create fixed-record files 2001 */

	/* Create fixed-record file 2001 */
	memcpy( acl, "\x00\x00\x00\xFF\x00\x00\x00\x00", sizeof(acl));
	printf("scCyberflexCmdCreateFile(Fixed-Record File):");
	ret = scCyberflexCmdCreateFile( ri, ci, 0x2001,
		36+16, SC_CYBERFLEX_FILE_FIXED, SC_CYBERFLEX_STATUS_UNBLOCKED,
		6, 5, acl );
	if( (ret==0) && (ci->sw[0]==0x90) && (ci->sw[1]==0x00) )
	{ printf(" OK\n"); } else
	{ printf(" Error.\n"); goto exit; }

	/* Select EF */
	ret = scCyberflexCmdSelect( ri, ci, SC_CYBERFLEX_SELECT_FILE, 0x2001,
		NULL, 0, rbuffer, &resplen );
	if( (ret==0) && (ci->sw[0]==0x90) && (ci->sw[1]==0x00) )
	{ } else
	{ printf(" Error.\n"); goto exit; }
	printarray( "  rsp:", resplen, rbuffer );

/* ChangeFileACL */

	/* ChangeFileACL */
	printf("scCyberflexCmdChangeFileACL:");
	memcpy( acl, "\x00\x00\x00\xFF\x00\x00\x00\x00", sizeof(acl));
	ret = scCyberflexCmdChangeFileACL( ri, ci, acl );
	if( (ret==0) && (ci->sw[0]==0x90) && (ci->sw[1]==0x00) )
	{ printf(" OK\n"); } else
	{ printf(" Error.\n"); goto exit; }
	printarray( "  rsp:", resplen, rbuffer );

/* Update Record */

	/* Update Record */
	printf("scCyberflexCmdUpdateRecord:");
	ret = scCyberflexCmdUpdateRecord( ri, ci, 1, SC_CYBERFLEX_RECORD_ABS,
		"Kleo  ", 6 );
	if( (ret==0) && (ci->sw[0]==0x90) && (ci->sw[1]==0x00) )
	{ printf(" OK\n"); } else
	{ printf(" Error.\n"); goto exit; }
	ret = scCyberflexCmdUpdateRecord( ri, ci, 2, SC_CYBERFLEX_RECORD_ABS,
		"Sonja ", 6 );
	if( (ret==0) && (ci->sw[0]==0x90) && (ci->sw[1]==0x00) )
	{ } else
	{ printf(" Error.\n"); goto exit; }
	ret = scCyberflexCmdUpdateRecord( ri, ci, 3, SC_CYBERFLEX_RECORD_ABS,
		"Madam ", 6 );
	if( (ret==0) && (ci->sw[0]==0x90) && (ci->sw[1]==0x00) )
	{ } else
	{ printf(" Error.\n"); goto exit; }
	ret = scCyberflexCmdUpdateRecord( ri, ci, 4, SC_CYBERFLEX_RECORD_ABS,
		"Isabel", 6 );
	if( (ret==0) && (ci->sw[0]==0x90) && (ci->sw[1]==0x00) )
	{ } else
	{ printf(" Error.\n"); goto exit; }
	ret = scCyberflexCmdUpdateRecord( ri, ci, 5, SC_CYBERFLEX_RECORD_ABS,
		"Gina  ", 6 );
	if( (ret==0) && (ci->sw[0]==0x90) && (ci->sw[1]==0x00) )
	{ } else
	{ printf(" Error.\n"); goto exit; }

/* Append Record */

	/* Append Record */
	printf("scCyberflexCmdAppendRecord:");
	ret = scCyberflexCmdAppendRecord( ri, ci, "Valeri", 6 );
	if( (ret==0) && (ci->sw[0]==0x90) && (ci->sw[1]==0x00) )
	{ printf(" OK\n"); } else
	{ printf(" Error.\n"); goto exit; }

/* Seek */

	/* Seek */
	printf("scCyberflexCmdSeek:");
	ret = scCyberflexCmdSeek( ri, ci, SC_CYBERFLEX_RECORD_FIRST, "M", 1,
		rbuffer );
	if( (ret==0) && (ci->sw[0]==0x90) && (ci->sw[1]==0x00) )
	{ printf(" OK\n"); } else
	{ printf(" Error.\n"); goto exit; }
	printarray( "  rsp:", 1, rbuffer );

/* Read Record */

	/* Read Record */
	printf("scCyberflexCmdReadRecord");
	resplen=6;
	ret = scCyberflexCmdReadRecord( ri, ci, 1, 4, buffer, &resplen );
	if( (ret==0) && (ci->sw[0]==0x90) && (ci->sw[1]==0x00) )
	{ printf(" OK\n"); } else
	{ printf(" Error.\n"); goto exit; }
	printarray( "  rsp:", resplen, buffer );

	/* Select DF */
	ret = scCyberflexCmdSelect( ri, ci, SC_CYBERFLEX_SELECT_FILE, 0x2000,
		NULL, 0, rbuffer, &resplen );
	if( (ret==0) && (ci->sw[0]==0x90) && (ci->sw[1]==0x00) )
	{ } else
	{ printf(" Error.\n"); goto exit; }

/* DeleteFile */

	/* DeleteFile */
	printf("scCyberflexCmdDeleteFile");
	ret = scCyberflexCmdDeleteFile( ri, ci, 0x2001 );
	if( (ret==0) && (ci->sw[0]==0x90) && (ci->sw[1]==0x00) )
	{ printf(" OK\n"); } else
	{ printf(" Error.\n"); goto exit; }

/* Create binary file */

	/* Create binary file 2002 */
	printf("scCyberflexCmdCreateFile(Transparent File):");
	memcpy( acl, "\x00\x00\x00\xFF\x00\x00\x00\x00", sizeof(acl));
	ret = scCyberflexCmdCreateFile( ri, ci, 0x2002,
		10+16, SC_CYBERFLEX_FILE_BINARY, SC_CYBERFLEX_STATUS_UNBLOCKED,
		0, 0, acl );
	if( (ret==0) && (ci->sw[0]==0x90) && (ci->sw[1]==0x00) )
	{ printf(" OK\n"); } else
	{ printf(" Error.\n"); goto exit; }

	/* Select EF */
	ret = scCyberflexCmdSelect( ri, ci, SC_CYBERFLEX_SELECT_FILE, 0x2002,
		NULL, 0, rbuffer, &resplen );
	if( (ret==0) && (ci->sw[0]==0x90) && (ci->sw[1]==0x00) )
	{ } else
	{ printf(" Error.\n"); goto exit; }
	printarray( "  rsp:", resplen, rbuffer );

/* Update Binary */

	/* Update Binary */
	printf("scCyberflexCmdUpdateBinary:");
	ret = scCyberflexCmdUpdateBinary( ri, ci, 0, data1, 10 );
	if( (ret==0) && (ci->sw[0]==0x90) && (ci->sw[1]==0x00) )
	{ printf(" OK\n"); } else
	{ printf(" Error.\n"); goto exit; }

/* Invalidate */

	/* Invalidate */
	printf("scCyberflexCmdInvalidate:");
	ret = scCyberflexCmdInvalidate( ri, ci );
	if( (ret==0) && (ci->sw[0]==0x90) && (ci->sw[1]==0x00) )
	{ printf(" OK\n"); } else
	{ printf(" Error.\n"); goto exit; }

/* Rehabilitate */

	/* Rehabilitate */
	printf("scCyberflexCmdRehabilitate:");
	ret = scCyberflexCmdRehabilitate( ri, ci );
	if( (ret==0) && (ci->sw[0]==0x90) && (ci->sw[1]==0x00) )
	{ printf(" OK\n"); } else
	{ printf(" Error.\n"); goto exit; }

/* Read Binary */

	/* Read Binary */
	resplen=8;
	printf("scCyberflexCmdReadBinary:");
	ret = scCyberflexCmdReadBinary( ri, ci, 2, buffer, &resplen );
	if( (ret==0) && (ci->sw[0]==0x90) && (ci->sw[1]==0x00) )
	{ printf(" OK\n"); } else
	{ printf(" Error.\n"); goto exit; }
	printarray( "  rsp:", 8, buffer );

	/* Select DF */
	ret = scCyberflexCmdSelect( ri, ci, SC_CYBERFLEX_SELECT_FILE, 0x2000,
		NULL, 0, rbuffer, &resplen );
	if( (ret==0) && (ci->sw[0]==0x90) && (ci->sw[1]==0x00) )
	{ } else
	{ printf(" Error.\n"); goto exit; }

/* Sleep */

	/* Sleep */
	buffer[0]=0x01;buffer[1]=0x23;buffer[2]=0x45;
	printf("scCyberflexCmdSleep:");
	ret = scCyberflexCmdSleep( ri, ci );
	if( (ret==0) && (ci->sw[0]==0x90) && (ci->sw[1]==0x00) )
	{ printf(" OK\n"); } else
	{ printf(" Error.\n"); goto exit; }

/* Create cyclic file */

	/* Create cyclic file */
	memcpy( acl, "\x00\x00\x00\xFF\x00\x00\x00\x00", sizeof(acl));
	printf("scCyberflexCmdCreateFile(Cyclic File):");
	ret = scCyberflexCmdCreateFile( ri, ci, 0x2003,
		28+16, SC_CYBERFLEX_FILE_CYCLIC, SC_CYBERFLEX_STATUS_UNBLOCKED|
		SC_CYBERFLEX_STATUS_INCREASE, 7, 4, acl );
	if( (ret==0) && (ci->sw[0]==0x90) && (ci->sw[1]==0x00) )
	{ printf(" OK\n"); } else
	{ printf(" Error.\n"); goto exit; }

	/* Select EF */
	ret = scCyberflexCmdSelect( ri, ci, SC_CYBERFLEX_SELECT_FILE, 0x2003,
		NULL, 0, rbuffer, &resplen );
	if( (ret==0) && (ci->sw[0]==0x90) && (ci->sw[1]==0x00) )
	{ } else
	{ printf(" Error.\n"); goto exit; }
	printarray( "  rsp:", resplen, rbuffer );

	ret = scCyberflexCmdUpdateRecord( ri, ci, 0, SC_CYBERFLEX_RECORD_PREV,
		"\x00\x00\x00", 3 );
	if( (ret!=0) || (ci->sw[0]!=0x90) || (ci->sw[1]!=0x00) )
	{ printf(" Error.\n"); goto exit; }

/* Increase */

	/* Increase */
	buffer[0]=0x01;buffer[1]=0x23;buffer[2]=0x45;
	printf("scCyberflexCmdIncrease:");
	ret = scCyberflexCmdIncrease( ri, ci, buffer, rbuffer, &resplen );
	if( (ret==0) && (ci->sw[0]==0x90) && (ci->sw[1]==0x00) )
	{ printf(" OK\n"); } else
	{ printf(" Error.\n"); goto exit; }
	printarray( "  rsp:", resplen, rbuffer );

	/* Select DF */
	ret = scCyberflexCmdSelect( ri, ci, SC_CYBERFLEX_SELECT_FILE, 0x2000,
		NULL, 0, rbuffer, &resplen );
	if( (ret==0) && (ci->sw[0]==0x90) && (ci->sw[1]==0x00) )
	{ } else
	{ printf(" Error.\n"); goto exit; }

#ifdef WITH_DES
	/* Create transparent file 0001 */
	memcpy( acl, "\x00\x00\x00\xFF\x00\x00\x00\x00", sizeof(acl));
	ret = scCyberflexCmdCreateFile( ri, ci, 0x0001,
		sizeof(int3auth)+16, SC_CYBERFLEX_FILE_BINARY,
		SC_CYBERFLEX_STATUS_UNBLOCKED, 0, 0, acl );
	if( (ret==0) && (ci->sw[0]==0x90) && (ci->sw[1]==0x00) )
	{ } else
	{ printf(" Error.\n"); goto exit; }

	/* Select EF */
	ret = scCyberflexCmdSelect( ri, ci, SC_CYBERFLEX_SELECT_FILE, 0x0001,
		NULL, 0, rbuffer, &resplen );
	if( (ret==0) && (ci->sw[0]==0x90) && (ci->sw[1]==0x00) )
	{ } else
	{ printf(" Error.\n"); goto exit; }

	/* Write IntAuth */
	resplen=sizeof(int3auth);
	ret = scCyberflexCmdUpdateBinary( ri, ci, 0, int3auth, resplen );
	if( (ret==0) && (ci->sw[0]==0x90) && (ci->sw[1]==0x00) )
	{ } else
	{ printf(" Error.\n"); goto exit; }

/* Int Auth DES */

	/* Int Auth DES */
	memcpy( mac, "\x00\x01\x02\x03\x04\x05\x06\x07", sizeof(mac) );
	printf("scCyberflexCmdIntAuthDES:");
	ret = scCyberflexCmdIntAuthDES( ri, ci, 0, mac, int3key,
		SC_CYBERFLEX_ALGO_3DES );
	if( (ret==0) && (ci->sw[0]==0x90) && (ci->sw[1]==0x00) )
	{ printf(" OK\n"); } else
	{ printf(" Error.\n"); goto exit; }

#endif /* WITH_DES */

	/* Select DF */
	ret = scCyberflexCmdSelect( ri, ci, SC_CYBERFLEX_SELECT_FILE, 0x2000,
		NULL, 0, rbuffer, &resplen );
	if( ret!=0 ) { printf(" Error.\n"); goto exit; }
	if( (ci->sw[0]==0x90) && (ci->sw[1]==0x00) ) {
		/* Delete directory file 2000 */
		ret = scCyberflexCmdDeleteFile( ri, ci, SC_CYBERFLEX_FID_CURRENT );
		if(ret!=0) { printf(" Error deleting 2000.\n"); goto exit; }

		ret = scCyberflexCmdSelect( ri, ci, SC_CYBERFLEX_SELECT_FILE, 0x3F00,
			NULL, 0, rbuffer, &resplen );
		if( (ret==0) && (ci->sw[0]==0x90) && (ci->sw[1]==0x00) )
		{ } else
		{ printf(" Error.\n"); goto exit; }

		/* Delete directory file 2000 */
		ret = scCyberflexCmdDeleteFile( ri, ci, 0x2000 );
		if(ret!=0) { printf(" Error deleting 2000.\n"); goto exit; }
	}

	/* Delete file 0001 */
	ret = scCyberflexCmdDeleteFile( ri, ci, 0x0001 );
	if(ret!=0) { printf(" Error deleting 0001.\n"); goto exit; }

/* LogOutAll */

	/* LogOutAll */
	printf("scCyberflexCmdLogOutAll:");
	ret = scCyberflexCmdLogOutAll( ri, ci );
	if( (ret==0) && (ci->sw[0]==0x90) && (ci->sw[1]==0x00) )
	{ printf(" OK\n"); } else
	{ printf(" Error.\n"); goto exit; }

exit:
    printf("ret: %d, SW: %.2X%.2X\n",ret,ci->sw[0],ci->sw[1]);

	ret = scReaderDeactivate( ri );
	if( ret!=0 ) printf("Error: scReaderDeactivate\n");

	ret = scReaderShutdown( ri );
	if( ret!=0 ) printf("Error: scReaderShutdown\n");

	scGeneralFreeCard( &ci );
	scGeneralFreeReader( &ri );

	scEnd();

	return(0);
}


