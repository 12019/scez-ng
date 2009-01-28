/* $Id: tcryptoflex.c 1617 2005-11-03 17:41:39Z laforge $ */

/*
 * Testfile for all Cryptoflex commands.
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#ifdef WITH_FGMP
#include <fgmp.h>
#endif
#if 0
#include <sio/sio.h>
#endif
#include <scez/scgeneral.h>
#include <scez/scsmartcard.h>
#include <scez/screader.h>
#include <scez/cards/sccryptoflex.h>

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

	BYTE acond[4];
	BYTE akeys[3];
	/* Transport key for Aladdin CC4 */
	/* BYTE authkey[]={0x2C,0x15,0xE5,0x26,0xE9,0x3E,0x8A,0x19}; */
	/* Transport key for Schlumberger Cryptoflex */
	BYTE authkey[]={0x47,0x46,0x58,0x49,0x32,0x56,0x78,0x40};

	BYTE pinfile[]={0x01,0xFF,0xFF,0x12,0x34,0x56,0x78,0xFF,0xFF,0xFF,0xFF,
		0x01,0x01,0xAA,0xBB,0xCC,0xDD,0xFF,0xFF,0xFF,0xFF,0x01,0x01};
	BYTE oldpin[]={0x12,0x34,0x56,0x78,0xFF,0xFF,0xFF,0xFF};
	BYTE unblockpin[]={0xAA,0xBB,0xCC,0xDD,0xFF,0xFF,0xFF,0xFF};
	BYTE newpin[]={0x9A,0xBC,0xDE,0x01,0xFF,0xFF,0xFF,0xFF};

	BYTE p1[]={0x6F,0x36,0x32,0x5E,0xA9,0xC0,0xE1,0x0D,0x4F,0x3C,0xC6,
	0x22,0x0C,0xE1,0x86,0xF0,0x09,0xA3,0x07,0x76,0x7B,0x0A,0x44,0xF8,
	0x69,0xF1,0xCA,0x8B,0xAC,0x97,0xEB,0xF3,0xCB,0x02,0x89,0x6B,0xA5,
	0x61,0x80,0xD8,0x20,0x86,0x80,0xA4,0x11,0xAE,0xED,0x9E,0x3D,0x20,
	0x23,0x67,0xFD,0x4A,0x5F,0x45,0x2A,0x9B,0xC3,0x51,0x00,0xC3,0xE2,
	0xBC};
	BYTE q1[]={0x6B,0x60,0xCF,0x47,0x5B,0xBB,0x4E,0xC8,0x51,0x01,0x4B,
	0x2E,0x39,0xAD,0xD4,0xF2,0xF4,0x58,0x43,0x78,0xD1,0xAC,0x35,0x9F,
	0xEB,0x71,0x5B,0x87,0x7F,0x7F,0x49,0x42,0xFD,0x95,0x4B,0x68,0x2B,
	0xF0,0xC1,0x3C,0x3A,0x75,0xDE,0x4F,0x1C,0xA3,0xB8,0x6D,0x9F,0x62,
	0x81,0x20,0x78,0x19,0xD3,0xEC,0x4E,0x9F,0x3E,0x80,0x08,0x16,0xFA,
	0xB8};
	BYTE a1[]={0x2A,0x32,0x26,0xA4,0x73,0x12,0x72,0xB2,0x50,0x7D,0x09,
	0xE1,0xD6,0xE5,0x0C,0xF6,0x22,0x39,0x04,0x6F,0xFD,0xE8,0xA3,0x0A,
	0xDF,0xEC,0xC2,0x53,0xC3,0x89,0xBD,0x38,0xD5,0xDB,0xE5,0xE4,0x17,
	0x1A,0x81,0x7E,0xEF,0x29,0xAE,0xF8,0xB1,0x91,0xF9,0xA4,0xEA,0x50,
	0xBB,0xD0,0x9B,0x03,0x3E,0x47,0xEC,0x96,0x46,0x30,0xDB,0xE6,0x5A,
	0x99};
	BYTE c1[]={0x53,0x78,0x82,0x50,0x39,0x42,0x74,0x8E,0x25,0x88,0x87,
	0x56,0x48,0x5F,0x74,0xBF,0xB3,0xBC,0xE6,0x0B,0x0C,0x7E,0x9C,0xC3,
	0xB2,0x88,0x9E,0x16,0xDF,0x0E,0xA1,0xD2,0x20,0x0A,0xE0,0x2B,0xBB,
	0x1B,0xAA,0xFB,0xED,0xF8,0x61,0xDE,0xE8,0xF6,0x25,0xCE,0xA6,0x32,
	0x9E,0x3F,0x35,0x7C,0x4A,0x7F,0x74,0x94,0x57,0x9F,0x5D,0x98,0xB2,
	0x3D};
	BYTE f1[]={0xB3,0x59,0x6A,0x12,0x53,0x58,0x26,0x9C,0x42,0xBE,0xEB,
	0x15,0xCE,0x84,0xFF,0x8D,0x52,0xD1,0x51,0x03,0x84,0x08,0x95,0x04,
	0xFE,0x2A,0x9F,0x0C,0x89,0xF8,0x0A,0xE5,0xC5,0x4F,0xFB,0x81,0xD6,
	0x92,0xBB,0x0F,0x5F,0x5F,0xD0,0xBF,0x7C,0x77,0xF7,0xF5,0x8A,0xF6,
	0x0E,0x82,0xCF,0x2D,0x34,0xC5,0x17,0x66,0xD4,0x31,0x37,0x04,0x1F,
	0x4A};
#ifdef WITH_FGMP
#endif /* WITH_FGMP */

#ifdef WITH_DES
	BYTE intauth[]={0xFF,0x08,0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,
		0xFF,0xFF,0x00};
	BYTE ext3auth[]={0xFF,0x10,0x02,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,
		0x09,0x0A,0x0B,0x0C,0x0D,0x0E,0x0F,0x10,0x03,0x03,0x10,0x02,
		0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0A,0x0B,0x0C,0x0D,
		0x0E,0x0F,0x10,0x03,0x03,0x00};
	BYTE int3auth[]={0xFF,0x10,0x02,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,
		0x09,0x0A,0x0B,0x0C,0x0D,0x0E,0x0F,0x10,0xFF,0xFF,0x00};

	BYTE refkey[]={0x47,0x46,0x58,0x49,0x32,0x56,0x78,0x40};
	BYTE challenge[]={0x64,0x46,0x27,0xE0,0x07,0x9D,0xD8,0x6C};
	BYTE result[]={0x0D,0x31,0xA8,0xF3,0x1C,0xEF};
	BYTE mac[8];
#endif /* WITH_DES */

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
	if( (ci->type&0xFFFFFF00)!=SC_CARD_CRYPTOFLEX )
	{ printf("Error: Wrong Card.\n"); goto exit; }

#if 0
	SIO_SetLogFile( ri->si, "LogCflex.txt" );
#endif

/* Select */

	/* Select MF */
	printf("scCryptoflexCmdSelectFile:");
	ret = scCryptoflexCmdSelectFile( ri, ci, 0x3F00, rbuffer, &resplen );
	if( (ret==0) && (ci->sw[0]==0x90) && (ci->sw[1]==0x00) )
	{ printf(" OK\n"); } else
	{ printf(" Error.\n"); goto exit; }
	printarray( "  rsp:", resplen, rbuffer );

/* Verify Key */

	/* Verify transport key */
	printf("scCryptoflexCmdVerifyKey:");
	ret = scCryptoflexCmdVerifyKey( ri, ci, 1, authkey, 8 );
	if( (ret==0) && (ci->sw[0]==0x90) && (ci->sw[1]==0x00) )
	{ printf(" OK\n"); } else
	{ printf(" Error.\n"); goto exit; }

/* Dir */

	/* Dir */
	printf("scCryptoflexCmdDirectory:");
	ret = scCryptoflexCmdDirectory( ri, ci, rbuffer, &resplen );
	if( (ret==0) && (ci->sw[0]==0x90) && (ci->sw[1]==0x00) )
	{ printf(" OK\n"); } else
	{ printf(" Error.\n"); goto exit; }
	printarray( "  rsp:", resplen, rbuffer );

	/* Delete directory file 2000 */
	ret = scCryptoflexCmdDeleteFile( ri, ci, 0x2000 );
	if(ret!=0) { printf(" Error deleting 2000.\n"); goto exit; }

/* Create */

	/* Create Directory 2000 */
	acond[0]=0xFF; acond[1]=0x4F; acond[2]=0x44; acond[3]=0x44;
	akeys[0]=0x1F; akeys[1]=0x11; akeys[2]=0x11;
	printf("scCryptoflexCmdCreateFile(Directory File):");
	ret = scCryptoflexCmdCreateFile( ri, ci, 0x2000,
		600, SC_CRYPTOFLEX_FILE_DIRECTORY, 0x00, SC_CRYPTOFLEX_UNBLOCKED,
		0, 0, acond, akeys );
	if( (ret==0) && (ci->sw[0]==0x90) && (ci->sw[1]==0x00) )
	{ printf(" OK\n"); } else
	{ printf(" Error.\n"); goto exit; }

	/* Select DF */
	ret = scCryptoflexCmdSelectFile( ri, ci, 0x2000, rbuffer, &resplen );
	if( (ret==0) && (ci->sw[0]==0x90) && (ci->sw[1]==0x00) )
	{ } else
	{ printf(" Error.\n"); goto exit; }
	printarray( "  rsp:", resplen, rbuffer );

/* Create fixed-record files 2001 */

	/* Create fixed-record file 2001 */
	acond[0]=0x3F; acond[1]=0x44; acond[2]=0xF4; acond[3]=0x44;
	akeys[0]=0x11; akeys[1]=0xF1; akeys[2]=0x11;
	printf("scCryptoflexCmdCreateFile(Fixed-Record File):");
	ret = scCryptoflexCmdCreateFile( ri, ci, 0x2001,
		36, SC_CRYPTOFLEX_FILE_FIXED, 0x00, SC_CRYPTOFLEX_UNBLOCKED,
		6, 5, acond, akeys );
	if( (ret==0) && (ci->sw[0]==0x90) && (ci->sw[1]==0x00) )
	{ printf(" OK\n"); } else
	{ printf(" Error.\n"); goto exit; }

	/* Select EF */
	ret = scCryptoflexCmdSelectFile( ri, ci, 0x2001, rbuffer, &resplen );
	if( (ret==0) && (ci->sw[0]==0x90) && (ci->sw[1]==0x00) )
	{ } else
	{ printf(" Error.\n"); goto exit; }
	printarray( "  rsp:", resplen, rbuffer );

/* Update Record */

	/* Update Record */
	printf("scCryptoflexCmdUpdateRecord:");
	ret = scCryptoflexCmdUpdateRecord( ri, ci, 0, 0, "Kleo  ", 6 );
	if( (ret==0) && (ci->sw[0]==0x90) && (ci->sw[1]==0x00) )
	{ printf(" OK\n"); } else
	{ printf(" Error.\n"); goto exit; }
	ret = scCryptoflexCmdUpdateRecord( ri, ci, 0, 2, "Sonja ", 6 );
	if( (ret==0) && (ci->sw[0]==0x90) && (ci->sw[1]==0x00) )
	{ } else
	{ printf(" Error.\n"); goto exit; }
	ret = scCryptoflexCmdUpdateRecord( ri, ci, 0, 2, "Madam ", 6 );
	if( (ret==0) && (ci->sw[0]==0x90) && (ci->sw[1]==0x00) )
	{ } else
	{ printf(" Error.\n"); goto exit; }
	ret = scCryptoflexCmdUpdateRecord( ri, ci, 0, 2, "Isabel", 6 );
	if( (ret==0) && (ci->sw[0]==0x90) && (ci->sw[1]==0x00) )
	{ } else
	{ printf(" Error.\n"); goto exit; }
	ret = scCryptoflexCmdUpdateRecord( ri, ci, 0, 2, "Gina  ", 6 );
	if( (ret==0) && (ci->sw[0]==0x90) && (ci->sw[1]==0x00) )
	{ } else
	{ printf(" Error.\n"); goto exit; }

/* Create Record */

	/* Create Record */
	printf("scCryptoflexCmdCreateRecord:");
	ret = scCryptoflexCmdCreateRecord( ri, ci, "Valeri", 6 );
	if( (ret==0) && (ci->sw[0]==0x90) && (ci->sw[1]==0x00) )
	{ printf(" OK\n"); } else
	{ printf(" Error.\n"); goto exit; }

/* Seek */

	/* Seek */
	printf("scCryptoflexCmdSeek:");
	ret = scCryptoflexCmdSeek( ri, ci, 0, 0, "M", 1 );
	if( (ret==0) && (ci->sw[0]==0x90) && (ci->sw[1]==0x00) )
	{ printf(" OK\n"); } else
	{ printf(" Error.\n"); goto exit; }

/* Read Record */

	/* Read Record */
	printf("scCryptoflexCmdReadRecord");
	resplen=6;
	ret = scCryptoflexCmdReadRecord( ri, ci, 0, 4, buffer, &resplen );
	if( (ret==0) && (ci->sw[0]==0x90) && (ci->sw[1]==0x00) )
	{ printf(" OK\n"); } else
	{ printf(" Error.\n"); goto exit; }
	printarray( "  rsp:", resplen, buffer );

	/* Select DF */
	ret = scCryptoflexCmdSelectFile( ri, ci, 0x2000, rbuffer, &resplen );
	if( (ret==0) && (ci->sw[0]==0x90) && (ci->sw[1]==0x00) )
	{ } else
	{ printf(" Error.\n"); goto exit; }

/* Create transparent file */

	/* Create transparent file 0000 */
	acond[0]=0x3F; acond[1]=0x44; acond[2]=0xFF; acond[3]=0x44;
	akeys[0]=0x11; akeys[1]=0xFF; akeys[2]=0x11;
	printf("scCryptoflexCmdCreateFile(Transparent File):");
	ret = scCryptoflexCmdCreateFile( ri, ci, 0x0000,
		23, SC_CRYPTOFLEX_FILE_TRANSPARENT, 0x00, SC_CRYPTOFLEX_UNBLOCKED,
		0, 0, acond, akeys );
	if( (ret==0) && (ci->sw[0]==0x90) && (ci->sw[1]==0x00) )
	{ printf(" OK\n"); } else
	{ printf(" Error.\n"); goto exit; }

	/* Select EF */
	ret = scCryptoflexCmdSelectFile( ri, ci, 0x0000, rbuffer, &resplen );
	if( (ret==0) && (ci->sw[0]==0x90) && (ci->sw[1]==0x00) )
	{ } else
	{ printf(" Error.\n"); goto exit; }
	printarray( "  rsp:", resplen, rbuffer );

/* Update Binary */

	/* Write PIN */
	resplen=0x17;
	printf("scCryptoflexCmdUpdateBinary:");
	ret = scCryptoflexCmdUpdateBinary( ri, ci, 0, pinfile, resplen );
	if( (ret==0) && (ci->sw[0]==0x90) && (ci->sw[1]==0x00) )
	{ printf(" OK\n"); } else
	{ printf(" Error.\n"); goto exit; }

/* Verify PIN */

	/* Verify PIN */
	printf("scCryptoflexCmdVerifyPIN:");
	ret = scCryptoflexCmdVerifyPIN( ri, ci, 1, oldpin );
	if( (ret==0) && (ci->sw[0]==0x90) && (ci->sw[1]==0x00) )
	{ printf(" OK\n"); } else
	{ printf(" Error.\n"); goto exit; }

#ifdef WITH_DES

/* Update Enciphered */

	/* Update Enciphered */
	printf("scCryptoflexCmdUpdateEnc:");
	ret = scCryptoflexCmdUpdateEnc( ri, ci, 0, pinfile, 16, authkey,
		SC_CRYPTOFLEX_ALGO_DES );
	if( (ret==0) && (ci->sw[0]==0x90) && (ci->sw[1]==0x00) )
	{ printf(" OK\n"); } else
	{ printf(" Error.\n"); goto exit; }

#endif /* WITH_DES */

/* Change PIN */

	/* Change PIN */
	printf("scCryptoflexCmdChangePIN:");
	ret = scCryptoflexCmdChangePIN( ri, ci, 1, oldpin, newpin );
	if( (ret==0) && (ci->sw[0]==0x90) && (ci->sw[1]==0x00) )
	{ printf(" OK\n"); } else
	{ printf(" Error.\n"); goto exit; }

	/* Verify PIN */
	printf("scCryptoflexCmdVerifyPIN:");
	ret = scCryptoflexCmdVerifyPIN( ri, ci, 1, newpin );
	if( (ret==0) && (ci->sw[0]==0x90) && (ci->sw[1]==0x00) )
	{ printf(" OK\n"); } else
	{ printf(" Error.\n"); goto exit; }

	/* Verify PIN */
	printf("scCryptoflexCmdVerifyPIN(should fail):");
	ret = scCryptoflexCmdVerifyPIN( ri, ci, 1, oldpin );
	if( (ret==0) && (ci->sw[0]==0x69) && (ci->sw[1]==0x83) )
	{ printf(" OK\n"); } else
	{ printf(" Error.\n"); goto exit; }

	/* Verify PIN */
	printf("scCryptoflexCmdVerifyPIN(should fail too):");
	ret = scCryptoflexCmdVerifyPIN( ri, ci, 1, newpin );
	if( (ret==0) && (ci->sw[0]==0x69) && (ci->sw[1]==0x83) )
	{ printf(" OK\n"); } else
	{ printf(" Error.\n"); goto exit; }

/* Unblock PIN */

	/* Unblock PIN */
	printf("scCryptoflexCmdUnblockPIN:");
	ret = scCryptoflexCmdUnblockPIN( ri, ci, 1, unblockpin, oldpin );
	if( (ret==0) && (ci->sw[0]==0x90) && (ci->sw[1]==0x00) )
	{ printf(" OK\n"); } else
	{ printf(" Error.\n"); goto exit; }

	/* Verify PIN */
	printf("scCryptoflexCmdVerifyPIN:");
	ret = scCryptoflexCmdVerifyPIN( ri, ci, 1, oldpin );
	if( (ret==0) && (ci->sw[0]==0x90) && (ci->sw[1]==0x00) )
	{ printf(" OK\n"); } else
	{ printf(" Error.\n"); goto exit; }

/* Read Binary */

	/* Read Binary */
	resplen=8;
	printf("scCryptoflexCmdReadBinary:");
	ret = scCryptoflexCmdReadBinary( ri, ci, 3, buffer, &resplen );
	if( (ret==0) && (ci->sw[0]==0x90) && (ci->sw[1]==0x00) )
	{ printf(" OK\n"); } else
	{ printf(" Error.\n"); goto exit; }
	printarray( "  rsp:", 8, buffer );

	/* Select DF */
	ret = scCryptoflexCmdSelectFile( ri, ci, 0x2000, rbuffer, &resplen );
	if( (ret==0) && (ci->sw[0]==0x90) && (ci->sw[1]==0x00) )
	{ } else
	{ printf(" Error.\n"); goto exit; }

/* Create cyclic file */

	/* Create cyclic file */
	acond[0]=0xFF; acond[1]=0x44; acond[2]=0x44; acond[3]=0x44;
	akeys[0]=0x11; akeys[1]=0x11; akeys[2]=0x11;
	printf("scCryptoflexCmdCreateFile(Cyclic File):");
	ret = scCryptoflexCmdCreateFile( ri, ci, 0x2003,
		12, SC_CRYPTOFLEX_FILE_CYCLIC, 0x00, SC_CRYPTOFLEX_UNBLOCKED,
		3, 4, acond, akeys );
	if( (ret==0) && (ci->sw[0]==0x90) && (ci->sw[1]==0x00) )
	{ printf(" OK\n"); } else
	{ printf(" Error.\n"); goto exit; }

	/* Select EF */
	ret = scCryptoflexCmdSelectFile( ri, ci, 0x2003, rbuffer, &resplen );
	if( (ret==0) && (ci->sw[0]==0x90) && (ci->sw[1]==0x00) )
	{ } else
	{ printf(" Error.\n"); goto exit; }
	printarray( "  rsp:", resplen, rbuffer );

/* Increase */

	/* Increase */
	buffer[0]=0x01;buffer[1]=0x23;buffer[2]=0x45;
	printf("scCryptoflexCmdIncrease:");
	ret = scCryptoflexCmdIncrease( ri, ci, buffer, rbuffer, &resplen );
	if( (ret==0) && (ci->sw[0]==0x90) && (ci->sw[1]==0x00) )
	{ printf(" OK\n"); } else
	{ printf(" Error.\n"); goto exit; }
	printarray( "  rsp:", resplen, rbuffer );

/* Decrease */

	/* Decrease */
	buffer[0]=0x01;buffer[1]=0x12;buffer[2]=0x34;
	printf("scCryptoflexCmdDecrease:");
	ret = scCryptoflexCmdDecrease( ri, ci, buffer, rbuffer, &resplen );
	if( (ret==0) && (ci->sw[0]==0x90) && (ci->sw[1]==0x00) )
	{ printf(" OK\n"); } else
	{ printf(" Error.\n"); goto exit; }
	printarray( "  rsp:", resplen, rbuffer );

/* Invalidate */

	/* Invalidate */
	printf("scCryptoflexCmdInvalidate:");
	ret = scCryptoflexCmdInvalidate( ri, ci );
	if( (ret==0) && (ci->sw[0]==0x90) && (ci->sw[1]==0x00) )
	{ printf(" OK\n"); } else
	{ printf(" Error.\n"); goto exit; }

	/* Increase */
	buffer[0]=0x01;buffer[1]=0x23;buffer[2]=0x45;
	printf("scCryptoflexCmdIncrease(Should fail):");
	ret = scCryptoflexCmdIncrease( ri, ci, buffer, rbuffer, &resplen );
	if( (ret==0) && (ci->sw[0]==0x62) && (ci->sw[1]==0x83) )
	{ printf(" OK\n"); } else
	{ printf(" Error.\n"); goto exit; }

/* Rehabilitate */

	/* Rehabilitate */
	printf("scCryptoflexCmdRehabilitate:");
	ret = scCryptoflexCmdRehabilitate( ri, ci );
	if( (ret==0) && (ci->sw[0]==0x90) && (ci->sw[1]==0x00) )
	{ printf(" OK\n"); } else
	{ printf(" Error.\n"); goto exit; }

	/* Increase */
	buffer[0]=0x01;buffer[1]=0x23;buffer[2]=0x45;
	printf("scCryptoflexCmdIncrease:");
	ret = scCryptoflexCmdIncrease( ri, ci, buffer, rbuffer, &resplen );
	if( (ret==0) && (ci->sw[0]==0x90) && (ci->sw[1]==0x00) )
	{ printf(" OK\n"); } else
	{ printf(" Error.\n"); goto exit; }
	printarray( "  rsp:", resplen, rbuffer );

	/* Select DF */
	ret = scCryptoflexCmdSelectFile( ri, ci, 0x2000, rbuffer, &resplen );
	if( (ret==0) && (ci->sw[0]==0x90) && (ci->sw[1]==0x00) )
	{ } else
	{ printf(" Error.\n"); goto exit; }

	/* Create transparent file 0001 */
	acond[0]=0x3F; acond[1]=0xF4; acond[2]=0xFF; acond[3]=0x44;
	akeys[0]=0x11; akeys[1]=0xFF; akeys[2]=0x11;
	ret = scCryptoflexCmdCreateFile( ri, ci, 0x0001,
		14, SC_CRYPTOFLEX_FILE_TRANSPARENT, 0x00, SC_CRYPTOFLEX_UNBLOCKED,
		0, 0, acond, akeys );
	if( (ret==0) && (ci->sw[0]==0x90) && (ci->sw[1]==0x00) )
	{ } else
	{ printf(" Error.\n"); goto exit; }

#ifdef WITH_DES
	/* Select EF */
	ret = scCryptoflexCmdSelectFile( ri, ci, 0x0001, rbuffer, &resplen );
	if( (ret==0) && (ci->sw[0]==0x90) && (ci->sw[1]==0x00) )
	{ } else
	{ printf(" Error.\n"); goto exit; }

	/* Write IntAuth */
	resplen=14;
	ret = scCryptoflexCmdUpdateBinary( ri, ci, 0, intauth, resplen );
	if( (ret==0) && (ci->sw[0]==0x90) && (ci->sw[1]==0x00) )
	{ } else
	{ printf(" Error.\n"); goto exit; }

/* Int Auth */

	/* Int Auth */
	mac[0]=0x00;mac[1]=0x01;mac[2]=0x02;mac[3]=0x03;
	mac[4]=0x04;mac[5]=0x05;mac[6]=0x06;mac[7]=0x07;
	printf("scCryptoflexCmdIntAuth:");
	ret = scCryptoflexCmdIntAuth( ri, ci, 0, mac, intauth+3,
		SC_CRYPTOFLEX_ALGO_DES );
	if( (ret==0) && (ci->sw[0]==0x90) && (ci->sw[1]==0x00) )
	{ printf(" OK\n"); } else
	{ printf(" Error.\n"); goto exit; }

	if( ci->type!=SC_CARD_CRYPTOFLEX ) {
/* Des Crypt */

		/* Des Crypt */
		memset( buffer, 0x33, 8 );
		printf("scCryptoflexCmdDesCrypt(Enc):");
		ret = scCryptoflexCmdDesCrypt( ri, ci, 0, SC_CRYPTOFLEX_MODE_ENCRYPT,
			buffer, rbuffer, &resplen );
		if( (ret==0) && (ci->sw[0]==0x90) && (ci->sw[1]==0x00) )
		{ printf(" OK\n"); } else
		{ printf(" Error.\n"); goto exit; }
		printarray( "  rsp:", resplen, rbuffer );
   
		/* Des Crypt */
		memcpy( buffer, rbuffer, 8 );
		printf("scCryptoflexCmdDesCrypt(Dec):");
		ret = scCryptoflexCmdDesCrypt( ri, ci, 0, SC_CRYPTOFLEX_MODE_DECRYPT,
			buffer, rbuffer, &resplen );
		if( (ret==0) && (ci->sw[0]==0x90) && (ci->sw[1]==0x00) )
		{ printf(" OK\n"); } else
		{ printf(" Error.\n"); goto exit; }
		printarray( "  rsp:", resplen, rbuffer );
	}

#endif /* WITH_DES */

	/* Select DF */
	ret = scCryptoflexCmdSelectFile( ri, ci, 0x2000, rbuffer, &resplen );
	if( (ret==0) && (ci->sw[0]==0x90) && (ci->sw[1]==0x00) )
	{ } else
	{ printf(" Error.\n"); goto exit; }

/* Delete */

	/* Delete file */
	printf("scCryptoflexCmdDeleteFile:");
	ret = scCryptoflexCmdDeleteFile( ri, ci, 0x0001 );
	if( (ret==0) && (ci->sw[0]==0x90) && (ci->sw[1]==0x00) )
	{ printf(" OK\n"); } else
	{ printf(" Error.\n"); goto exit; }

	/* Delete transparent file 2003 */
	ret = scCryptoflexCmdDeleteFile( ri, ci, 0x2003 );
	if( (ret==0) && (ci->sw[0]==0x90) && (ci->sw[1]==0x00) )
	{ } else
	{ printf(" Error.\n"); goto exit; }

#ifdef WITH_DES

	/* Verify PIN */
	printf("scCryptoflexCmdVerifyPIN:");
	ret = scCryptoflexCmdVerifyPIN( ri, ci, 1, oldpin );
	if( (ret==0) && (ci->sw[0]==0x90) && (ci->sw[1]==0x00) )
	{ printf(" OK\n"); } else
	{ printf(" Error.\n"); goto exit; }

	/* Create transparent file 0001 */
	acond[0]=0x3F; acond[1]=0xF4; acond[2]=0xFF; acond[3]=0x44;
	akeys[0]=0x11; akeys[1]=0xFF; akeys[2]=0x11;
	ret = scCryptoflexCmdCreateFile( ri, ci, 0x0001,
		22, SC_CRYPTOFLEX_FILE_TRANSPARENT, 0x00, SC_CRYPTOFLEX_UNBLOCKED,
		0, 0, acond, akeys );
	if( (ret==0) && (ci->sw[0]==0x90) && (ci->sw[1]==0x00) )
	{ } else
	{ printf(" Error.\n"); goto exit; }

	/* Select EF */
	ret = scCryptoflexCmdSelectFile( ri, ci, 0x0001, rbuffer, &resplen );
	if( (ret==0) && (ci->sw[0]==0x90) && (ci->sw[1]==0x00) )
	{ } else
	{ printf(" Error.\n"); goto exit; }

	/* Write Int3Auth */
	resplen=22;
	ret = scCryptoflexCmdUpdateBinary( ri, ci, 0, int3auth, resplen );
	if( (ret==0) && (ci->sw[0]==0x90) && (ci->sw[1]==0x00) )
	{ } else
	{ printf(" Error.\n"); goto exit; }

/* Int Auth */

	/* Int Auth */
	mac[0]=0x00;mac[1]=0x01;mac[2]=0x02;mac[3]=0x03;
	mac[4]=0x04;mac[5]=0x05;mac[6]=0x06;mac[7]=0x07;
	printf("scCryptoflexCmdIntAuth(3DES):");
	ret = scCryptoflexCmdIntAuth( ri, ci, 0, mac, int3auth+3,
		SC_CRYPTOFLEX_ALGO_3DES );
	if( (ret==0) && (ci->sw[0]==0x90) && (ci->sw[1]==0x00) )
	{ printf(" OK\n"); } else
	{ printf(" Error.\n"); goto exit; }

	if( ci->type!=SC_CARD_CRYPTOFLEX ) {
/* Des Crypt */

		/* Des Crypt */
		memset( buffer, 0x33, 8 );
		printf("scCryptoflexCmdDesCrypt(Enc):");
		ret = scCryptoflexCmdDesCrypt( ri, ci, 0, SC_CRYPTOFLEX_MODE_ENCRYPT,
			buffer, rbuffer, &resplen );
		if( (ret==0) && (ci->sw[0]==0x90) && (ci->sw[1]==0x00) )
		{ printf(" OK\n"); } else
		{ printf(" Error.\n"); goto exit; }
		printarray( "  rsp:", resplen, rbuffer );
   
		/* Des Crypt */
		memcpy( buffer, rbuffer, 8 );
		printf("scCryptoflexCmdDesCrypt(Dec):");
		ret = scCryptoflexCmdDesCrypt( ri, ci, 0, SC_CRYPTOFLEX_MODE_DECRYPT,
			buffer, rbuffer, &resplen );
		if( (ret==0) && (ci->sw[0]==0x90) && (ci->sw[1]==0x00) )
		{ printf(" OK\n"); } else
		{ printf(" Error.\n"); goto exit; }
		printarray( "  rsp:", resplen, rbuffer );
	}

	/* Select DF */
	ret = scCryptoflexCmdSelectFile( ri, ci, 0x2000, rbuffer, &resplen );
	if( (ret==0) && (ci->sw[0]==0x90) && (ci->sw[1]==0x00) )
	{ } else
	{ printf(" Error.\n"); goto exit; }

	/* Delete file */
	ret = scCryptoflexCmdDeleteFile( ri, ci, 0x0001 );
	if( (ret==0) && (ci->sw[0]==0x90) && (ci->sw[1]==0x00) )
	{ } else
	{ printf(" Error.\n"); goto exit; }

	/* Create transparent file 0011 */
	acond[0]=0x3F; acond[1]=0xF4; acond[2]=0xFF; acond[3]=0x44;
	akeys[0]=0x11; akeys[1]=0xFF; akeys[2]=0x11;
	ret = scCryptoflexCmdCreateFile( ri, ci, 0x0011,
		42, SC_CRYPTOFLEX_FILE_TRANSPARENT, 0x00, SC_CRYPTOFLEX_UNBLOCKED,
		0, 0, acond, akeys );
	if( (ret==0) && (ci->sw[0]==0x90) && (ci->sw[1]==0x00) )
	{ } else
	{ printf(" Error.\n"); goto exit; }

	/* Select EF */
	ret = scCryptoflexCmdSelectFile( ri, ci, 0x0011, rbuffer, &resplen );
	if( (ret==0) && (ci->sw[0]==0x90) && (ci->sw[1]==0x00) )
	{ } else
	{ printf(" Error.\n"); goto exit; }

	/* Write Ext3Auth */
	resplen=42;
	ret = scCryptoflexCmdUpdateBinary( ri, ci, 0, ext3auth, resplen );
	if( (ret==0) && (ci->sw[0]==0x90) && (ci->sw[1]==0x00) )
	{ } else
	{ printf(" Error.\n"); goto exit; }

/* 3DES Verify */

	/* Verify transport key */
	printf("scCryptoflexCmdVerifyKey(3DES):");
	ret = scCryptoflexCmdVerifyKey( ri, ci, 1, ext3auth+23, 16 );
	if( (ret==0) && (ci->sw[0]==0x90) && (ci->sw[1]==0x00) )
	{ printf(" OK\n"); } else
	{ printf(" Error.\n"); goto exit; }

	/* Get Challenge */
	resplen=8;
	ret = scCryptoflexCmdGetChall( ri, ci, buffer, &resplen );
	if( (ret==0) && (ci->sw[0]==0x90) && (ci->sw[1]==0x00) )
	{ } else
	{ printf(" Error.\n"); goto exit; }

/* 3DES ExtAuth */

	/* External Authenticate */
	printf("scCryptoflexCmdExtAuth(3DES):");
	ret = scCryptoflexCmdExtAuth( ri, ci, 1, ext3auth+23, buffer,
		SC_CRYPTOFLEX_ALGO_3DES );
	if( (ret==0) && (ci->sw[0]==0x90) && (ci->sw[1]==0x00) )
	{ printf(" OK\n"); } else
	{ printf(" Error.\n"); goto exit; }

	/* Select EF */
	ret = scCryptoflexCmdSelectFile( ri, ci, 0x0000, rbuffer, &resplen );
	if( (ret==0) && (ci->sw[0]==0x90) && (ci->sw[1]==0x00) )
	{ } else
	{ printf(" Error.\n"); goto exit; }

#if 0
	/* Select MF */
	ret = scCryptoflexCmdSelectFile( ri, ci, 0x2000, rbuffer, &resplen );
	if( (ret==0) && (ci->sw[0]==0x90) && (ci->sw[1]==0x00) )
	{ } else
	{ printf(" Error.\n"); goto exit; }

	/* Verify transport key */
	ret = scCryptoflexCmdVerifyKey( ri, ci, 1, ext3auth+23, 16 );
	if( (ret==0) && (ci->sw[0]==0x90) && (ci->sw[1]==0x00) )
	{ } else
	{ printf(" Error.\n"); goto exit; }

	/* Create transparent file 2004 */
	acond[0]=0x3F; acond[1]=0x33; acond[2]=0xFF; acond[3]=0x33;
	akeys[0]=0x11; akeys[1]=0xFF; akeys[2]=0x11;
	ret = scCryptoflexCmdCreateFile( ri, ci, 0x2004,
		23, SC_CRYPTOFLEX_FILE_TRANSPARENT, 0x00, SC_CRYPTOFLEX_UNBLOCKED,
		0, 0, acond, akeys );
	if( (ret==0) && (ci->sw[0]==0x90) && (ci->sw[1]==0x00) )
	{ } else
	{ printf(" Error.\n"); goto exit; }

	/* Select EF */
	ret = scCryptoflexCmdSelectFile( ri, ci, 0x2004, rbuffer, &resplen );
	if( (ret==0) && (ci->sw[0]==0x90) && (ci->sw[1]==0x00) )
	{ } else
	{ printf(" Error.\n"); goto exit; }

	/* Get Challenge */
	resplen=8;
	ret = scCryptoflexCmdGetChall( ri, ci, mac, &resplen );
	if( (ret==0) && (ci->sw[0]==0x90) && (ci->sw[1]==0x00) )
	{ } else
	{ printf(" Error.\n"); goto exit; }

/* Update Binary */

	/* Write PIN */
	printf("scCryptoflexCmdUpdateBinaryMAC(3DES):");
	ret = scCryptoflexCmdUpdateBinaryMAC( ri, ci, 0, pinfile, 1,
		(ext3auth+23), mac, SC_CRYPTOFLEX_ALGO_3DES );
printf("ret: %d: %.2X%.2X\n",ret,ci->sw[0],ci->sw[1]);
	if( (ret==0) && (ci->sw[0]==0x90) && (ci->sw[1]==0x00) )
	{ printf(" OK\n"); } else
	{ printf(" Error.\n"); goto exit; }
#endif /* 0 */

/**********************/

	/* Select DF */
	ret = scCryptoflexCmdSelectFile( ri, ci, 0x2000, rbuffer, &resplen );
	if( (ret==0) && (ci->sw[0]==0x90) && (ci->sw[1]==0x00) )
	{ } else
	{ printf(" Error.\n"); goto exit; }

	/* Verify transport key */
	ret = scCryptoflexCmdVerifyKey( ri, ci, 1, ext3auth+23, 16 );
	if( (ret==0) && (ci->sw[0]==0x90) && (ci->sw[1]==0x00) )
	{ } else
	{ printf(" Error.\n"); goto exit; }

	/* Verify PIN */
	ret = scCryptoflexCmdVerifyPIN( ri, ci, 1, oldpin );
	if( (ret==0) && (ci->sw[0]==0x90) && (ci->sw[1]==0x00) )
	{ } else
	{ printf(" Error.\n"); goto exit; }

	/* Create transparent file 0012 */
	acond[0]=0x3F; acond[1]=0xF4; acond[2]=0xFF; acond[3]=0x44;
	akeys[0]=0x11; akeys[1]=0xFF; akeys[2]=0x11;
	ret = scCryptoflexCmdCreateFile( ri, ci, 0x0012,
		326, SC_CRYPTOFLEX_FILE_TRANSPARENT, 0x00, SC_CRYPTOFLEX_UNBLOCKED,
		0, 0, acond, akeys );
	if( (ret==0) && (ci->sw[0]==0x90) && (ci->sw[1]==0x00) )
	{ } else
	{ printf(" Error.\n"); goto exit; }

	/* Select EF */
	ret = scCryptoflexCmdSelectFile( ri, ci, 0x0012, rbuffer, &resplen );
	if( (ret==0) && (ci->sw[0]==0x90) && (ci->sw[1]==0x00) )
	{ } else
	{ printf(" Error.\n"); goto exit; }

	/* Write Secret Key File */
	ret = scCryptoflexCmdUpdateBinary( ri, ci, 0, "\x01\x43\x01", 3 );
	if( (ret==0) && (ci->sw[0]==0x90) && (ci->sw[1]==0x00) )
	{ } else
	{ printf(" Error.\n"); goto exit; }

/* Update Enciphered */

	/* Update Enciphered */
	printf("scCryptoflexCmdUpdateEnc(3DES - not!):");
	ret = scCryptoflexCmdUpdateEnc( ri, ci, 3, p1, 64, ext3auth+23,
		SC_CRYPTOFLEX_ALGO_DES );
	if( (ret==0) && (ci->sw[0]==0x90) && (ci->sw[1]==0x00) )
	{ printf(" OK\n"); } else
	{ printf(" Error.\n"); goto exit; }

#if 0
	ret = scCryptoflexCmdUpdateBinary( ri, ci, 3, p1, 64 );
	if( (ret==0) && (ci->sw[0]==0x90) && (ci->sw[1]==0x00) )
	{ } else
	{ printf(" Error.\n"); goto exit; }
#endif /* 0 */

	ret = scCryptoflexCmdUpdateBinary( ri, ci, 3+64, q1, 64 );
	if( (ret==0) && (ci->sw[0]==0x90) && (ci->sw[1]==0x00) )
	{ } else
	{ printf(" Error.\n"); goto exit; }

	ret = scCryptoflexCmdUpdateBinary( ri, ci, 3+128, a1, 64 );
	if( (ret==0) && (ci->sw[0]==0x90) && (ci->sw[1]==0x00) )
	{ } else
	{ printf(" Error.\n"); goto exit; }

	ret = scCryptoflexCmdUpdateBinary( ri, ci, 3+128+64, c1, 64 );
	if( (ret==0) && (ci->sw[0]==0x90) && (ci->sw[1]==0x00) )
	{ } else
	{ printf(" Error.\n"); goto exit; }

	ret = scCryptoflexCmdUpdateBinary( ri, ci, 3+256, f1, 64 );
	if( (ret==0) && (ci->sw[0]==0x90) && (ci->sw[1]==0x00) )
	{ } else
	{ printf(" Error.\n"); goto exit; }

	ret = scCryptoflexCmdUpdateBinary( ri, ci, 3+256+64, "\x00\x00\x00", 3 );
	if( (ret==0) && (ci->sw[0]==0x90) && (ci->sw[1]==0x00) )
	{ } else
	{ printf(" Error.\n"); goto exit; }

/* RSA Sign */
	/* RSA Sign */
	memset( buffer, 0x6A, sizeof(buffer) );
	printf("scCryptoflexCmdRsaSign:");
	ret = scCryptoflexCmdRsaSign( ri, ci, 0x00, buffer, rbuffer,
		&resplen );
	if( (ret==0) && (ci->sw[0]==0x90) && (ci->sw[1]==0x00) )
	{ printf(" OK\n"); } else
	{ printf(" Error.\n"); goto exit; }
	printarray( "  rsp:", resplen, rbuffer );
	printf("  should be: B6 DA 2A 3A 13 75 25 1F ...\n");

#endif /* WITH_DES */

	/* Select MF */
	ret = scCryptoflexCmdSelectFile( ri, ci, 0x3F00, rbuffer, &resplen );
	if( (ret==0) && (ci->sw[0]==0x90) && (ci->sw[1]==0x00) )
	{ } else
	{ printf(" Error.\n"); goto exit; }

	/* Verify transport key */
	ret = scCryptoflexCmdVerifyKey( ri, ci, 1, authkey, 8 );
	if( (ret==0) && (ci->sw[0]==0x90) && (ci->sw[1]==0x00) )
	{ } else
	{ printf(" Error.\n"); goto exit; }

	/* Delete DF 2000 */
	ret = scCryptoflexCmdDeleteFile( ri, ci, 0x2000 );
	if( (ret==0) && (ci->sw[0]==0x90) && (ci->sw[1]==0x00) )
	{ } else
	{ printf(" Error.\n"); goto exit; }

#ifdef WITH_DES

	/* Test MAC commands */

	printf("scCryptoflexGenerateAuth:");
	scCryptoflexGenerateAuth( refkey, challenge, mac,
		SC_CRYPTOFLEX_ALGO_DES );
	if( memcmp( mac, result, 6 ) ) { printf(" Error\n"); goto exit; }
	printf(" OK\n");

	/* Reset Card */
	printf("scReaderResetCard:");
	ret= scReaderResetCard( ri, ci );
	if( ret!=0 ) { printf(" Error.\n"); goto exit; }
	printf(" OK.\n");

	/* Get Challenge */
	printf("scCryptoflexCmdGetChall:");
	resplen=8;
	ret = scCryptoflexCmdGetChall( ri, ci, buffer, &resplen );
	if( (ret==0) && (ci->sw[0]==0x90) && (ci->sw[1]==0x00) )
	{ printf(" OK\n"); } else
	{ printf(" Error.\n"); goto exit; }
	printarray( "  rsp:", 8, buffer );

	/* External Authenticate */
	printf("scCryptoflexCmdExtAuth:");
	ret = scCryptoflexCmdExtAuth( ri, ci, 0x01, authkey, buffer,
		SC_CRYPTOFLEX_ALGO_DES );
	if( (ret==0) && (ci->sw[0]==0x90) && (ci->sw[1]==0x00) )
	{ printf(" OK\n"); } else
	{ printf(" Error.\n"); goto exit; }

	/* Verify transport key */
	/* Just in case to prevent blocking of key */
	ret = scCryptoflexCmdVerifyKey( ri, ci, 1, authkey, 8 );
	if( (ret==0) && (ci->sw[0]==0x90) && (ci->sw[1]==0x00) )
	{ } else
	{ printf(" Error.\n"); goto exit; }

	/* Create Directory 2000 */
	acond[0]=0xFF; acond[1]=0x3F; acond[2]=0x33; acond[3]=0x33;
	akeys[0]=0x1F; akeys[1]=0x11; akeys[2]=0x11;
	ret = scCryptoflexCmdCreateFile( ri, ci, 0x2000,
		200, SC_CRYPTOFLEX_FILE_DIRECTORY, 0x00, SC_CRYPTOFLEX_UNBLOCKED,
		0, 0, acond, akeys );
	if( (ret==0) && (ci->sw[0]==0x90) && (ci->sw[1]==0x00) )
	{ } else
	{ printf(" Error.\n"); goto exit; }

	/* Reset Card */
	printf("scReaderResetCard:");
	ret= scReaderResetCard( ri, ci );
	if( ret!=0 ) { printf(" Error.\n"); goto exit; }
	printf(" OK.\n");

	/* Select DF */
	ret = scCryptoflexCmdSelectFile( ri, ci, 0x2000, rbuffer, &resplen );
	if( (ret==0) && (ci->sw[0]==0x90) && (ci->sw[1]==0x00) )
	{ } else
	{ printf(" Error.\n"); goto exit; }

	/* Get Challenge */
	resplen=8;
	ret = scCryptoflexCmdGetChall( ri, ci, mac, &resplen );
	if( (ret==0) && (ci->sw[0]==0x90) && (ci->sw[1]==0x00) )
	{ } else
	{ printf(" Error.\n"); goto exit; }

/* Create fixed-record file 2001 */

	/* Create fixed-record file 2001 */
	acond[0]=0x3F; acond[1]=0x33; acond[2]=0xF3; acond[3]=0x33;
	akeys[0]=0x11; akeys[1]=0xF1; akeys[2]=0x11;
	printf("scCryptoflexCmdCreateFileMAC(Fixed-Record File):");
	ret = scCryptoflexCmdCreateFileMAC( ri, ci, 0x2001,
		36, SC_CRYPTOFLEX_FILE_FIXED, 0x00, SC_CRYPTOFLEX_UNBLOCKED,
		6, 5, acond, akeys, authkey, mac, SC_CRYPTOFLEX_ALGO_DES );
	if( (ret==0) && (ci->sw[0]==0x90) && (ci->sw[1]==0x00) )
	{ printf(" OK\n"); } else
	{ printf(" Error.\n"); goto exit; }

	/* Select EF */
	ret = scCryptoflexCmdSelectFile( ri, ci, 0x2001, rbuffer, &resplen );
	if( (ret==0) && (ci->sw[0]==0x90) && (ci->sw[1]==0x00) )
	{ } else
	{ printf(" Error.\n"); goto exit; }
	printarray( "  rsp:", resplen, rbuffer );

	/* Get Challenge */
	resplen=8;
	ret = scCryptoflexCmdGetChall( ri, ci, mac, &resplen );
	if( (ret==0) && (ci->sw[0]==0x90) && (ci->sw[1]==0x00) )
	{ } else
	{ printf(" Error.\n"); goto exit; }

/* Update Record */

	/* Update Record */
	printf("scCryptoflexCmdUpdateRecordMAC:");
	ret = scCryptoflexCmdUpdateRecordMAC( ri, ci, 0, 0, "Kleo  ", 6,
		authkey, mac, SC_CRYPTOFLEX_ALGO_DES );
	if( (ret==0) && (ci->sw[0]==0x90) && (ci->sw[1]==0x00) )
	{ printf(" OK\n"); } else
	{ printf(" Error.\n"); goto exit; }

	/* Get Challenge */
	resplen=8;
	ret = scCryptoflexCmdGetChall( ri, ci, mac, &resplen );
	if( (ret==0) && (ci->sw[0]==0x90) && (ci->sw[1]==0x00) )
	{ } else
	{ printf(" Error.\n"); goto exit; }

/* Create Record */

	/* Create Record */
	printf("scCryptoflexCmdCreateRecordMAC:");
	ret = scCryptoflexCmdCreateRecordMAC( ri, ci, "Valeri", 6,
		authkey, mac, SC_CRYPTOFLEX_ALGO_DES );
	if( (ret==0) && (ci->sw[0]==0x90) && (ci->sw[1]==0x00) )
	{ printf(" OK\n"); } else
	{ printf(" Error.\n"); goto exit; }

	/* Select DF */
	ret = scCryptoflexCmdSelectFile( ri, ci, 0x2000, rbuffer, &resplen );
	if( (ret==0) && (ci->sw[0]==0x90) && (ci->sw[1]==0x00) )
	{ } else
	{ printf(" Error.\n"); goto exit; }

	/* Get Challenge */
	resplen=8;
	ret = scCryptoflexCmdGetChall( ri, ci, mac, &resplen );
	if( (ret==0) && (ci->sw[0]==0x90) && (ci->sw[1]==0x00) )
	{ } else
	{ printf(" Error.\n"); goto exit; }

/* Create transparent file */

	/* Create transparent file 0000 */
	acond[0]=0x3F; acond[1]=0x33; acond[2]=0xFF; acond[3]=0x33;
	akeys[0]=0x11; akeys[1]=0xFF; akeys[2]=0x11;
	printf("scCryptoflexCmdCreateFileMAC(Transparent File):");
	ret = scCryptoflexCmdCreateFileMAC( ri, ci, 0x0000,
		23, SC_CRYPTOFLEX_FILE_TRANSPARENT, 0x00, SC_CRYPTOFLEX_UNBLOCKED,
		0, 0, acond, akeys, authkey, mac, SC_CRYPTOFLEX_ALGO_DES );
	if( (ret==0) && (ci->sw[0]==0x90) && (ci->sw[1]==0x00) )
	{ printf(" OK\n"); } else
	{ printf(" Error.\n"); goto exit; }

	/* Select EF */
	ret = scCryptoflexCmdSelectFile( ri, ci, 0x0000, rbuffer, &resplen );
	if( (ret==0) && (ci->sw[0]==0x90) && (ci->sw[1]==0x00) )
	{ } else
	{ printf(" Error.\n"); goto exit; }
	printarray( "  rsp:", resplen, rbuffer );

	/* Get Challenge */
	resplen=8;
	ret = scCryptoflexCmdGetChall( ri, ci, mac, &resplen );
	if( (ret==0) && (ci->sw[0]==0x90) && (ci->sw[1]==0x00) )
	{ } else
	{ printf(" Error.\n"); goto exit; }

/* Update Binary */

	/* Write PIN */
	resplen=13;
	printf("scCryptoflexCmdUpdateBinaryMAC:");
	ret = scCryptoflexCmdUpdateBinaryMAC( ri, ci, 0, pinfile, resplen,
		authkey, mac, SC_CRYPTOFLEX_ALGO_DES );
	if( (ret==0) && (ci->sw[0]==0x90) && (ci->sw[1]==0x00) )
	{ printf(" OK\n"); } else
	{ printf(" Error.\n"); goto exit; }

	/* Get Challenge */
	resplen=8;
	ret = scCryptoflexCmdGetChall( ri, ci, mac, &resplen );
	if( (ret==0) && (ci->sw[0]==0x90) && (ci->sw[1]==0x00) )
	{ } else
	{ printf(" Error.\n"); goto exit; }

	/* Write PIN */
	resplen=10;
	ret = scCryptoflexCmdUpdateBinaryMAC( ri, ci, 13, pinfile, resplen,
		authkey, mac, SC_CRYPTOFLEX_ALGO_DES );
	if( (ret==0) && (ci->sw[0]==0x90) && (ci->sw[1]==0x00) )
	{ } else
	{ printf(" Error.\n"); goto exit; }

	/* Select DF */
	ret = scCryptoflexCmdSelectFile( ri, ci, 0x2000, rbuffer, &resplen );
	if( (ret==0) && (ci->sw[0]==0x90) && (ci->sw[1]==0x00) )
	{ } else
	{ printf(" Error.\n"); goto exit; }

	/* Get Challenge */
	resplen=8;
	ret = scCryptoflexCmdGetChall( ri, ci, mac, &resplen );
	if( (ret==0) && (ci->sw[0]==0x90) && (ci->sw[1]==0x00) )
	{ } else
	{ printf(" Error.\n"); goto exit; }

/* Create cyclic file */

	/* Create cyclic file */
	acond[0]=0xFF; acond[1]=0x33; acond[2]=0x33; acond[3]=0x33;
	akeys[0]=0x11; akeys[1]=0x11; akeys[2]=0x11;
	printf("scCryptoflexCmdCreateFileMAC(Cyclic File):");
	ret = scCryptoflexCmdCreateFileMAC( ri, ci, 0x2003,
		12, SC_CRYPTOFLEX_FILE_CYCLIC, 0x00, SC_CRYPTOFLEX_UNBLOCKED,
		3, 4, acond, akeys, authkey, mac, SC_CRYPTOFLEX_ALGO_DES );
	if( (ret==0) && (ci->sw[0]==0x90) && (ci->sw[1]==0x00) )
	{ printf(" OK\n"); } else
	{ printf(" Error.\n"); goto exit; }

	/* Select EF */
	ret = scCryptoflexCmdSelectFile( ri, ci, 0x2003, rbuffer, &resplen );
	if( (ret==0) && (ci->sw[0]==0x90) && (ci->sw[1]==0x00) )
	{ } else
	{ printf(" Error.\n"); goto exit; }
	printarray( "  rsp:", resplen, rbuffer );

	/* Get Challenge */
	resplen=8;
	ret = scCryptoflexCmdGetChall( ri, ci, mac, &resplen );
	if( (ret==0) && (ci->sw[0]==0x90) && (ci->sw[1]==0x00) )
	{ } else
	{ printf(" Error.\n"); goto exit; }

/* Increase */

	/* Increase */
	buffer[0]=0x01;buffer[1]=0x23;buffer[2]=0x45;
	printf("scCryptoflexCmdIncreaseMAC:");
	ret = scCryptoflexCmdIncreaseMAC( ri, ci, buffer, authkey, mac,
		SC_CRYPTOFLEX_ALGO_DES, rbuffer, &resplen );
	if( (ret==0) && (ci->sw[0]==0x90) && (ci->sw[1]==0x00) )
	{ printf(" OK\n"); } else
	{ printf(" Error.\n"); goto exit; }
	printarray( "  rsp:", resplen, rbuffer );

	/* Get Challenge */
	resplen=8;
	ret = scCryptoflexCmdGetChall( ri, ci, mac, &resplen );
	if( (ret==0) && (ci->sw[0]==0x90) && (ci->sw[1]==0x00) )
	{ } else
	{ printf(" Error.\n"); goto exit; }

/* Decrease */

	/* Decrease */
	buffer[0]=0x01;buffer[1]=0x12;buffer[2]=0x34;
	printf("scCryptoflexCmdDecreaseMAC:");
	ret = scCryptoflexCmdDecreaseMAC( ri, ci, buffer, authkey, mac,
		SC_CRYPTOFLEX_ALGO_DES, rbuffer, &resplen );
	if( (ret==0) && (ci->sw[0]==0x90) && (ci->sw[1]==0x00) )
	{ printf(" OK\n"); } else
	{ printf(" Error.\n"); goto exit; }
	printarray( "  rsp:", resplen, rbuffer );

	/* Get Challenge */
	resplen=8;
	ret = scCryptoflexCmdGetChall( ri, ci, mac, &resplen );
	if( (ret==0) && (ci->sw[0]==0x90) && (ci->sw[1]==0x00) )
	{ } else
	{ printf(" Error.\n"); goto exit; }

/* Invalidate */

	/* Invalidate */
	printf("scCryptoflexCmdInvalidateMAC:");
	ret = scCryptoflexCmdInvalidateMAC( ri, ci, authkey, mac,
		SC_CRYPTOFLEX_ALGO_DES );
	if( (ret==0) && (ci->sw[0]==0x90) && (ci->sw[1]==0x00) )
	{ printf(" OK\n"); } else
	{ printf(" Error.\n"); goto exit; }

	/* Get Challenge */
	resplen=8;
	ret = scCryptoflexCmdGetChall( ri, ci, mac, &resplen );
	if( (ret==0) && (ci->sw[0]==0x90) && (ci->sw[1]==0x00) )
	{ } else
	{ printf(" Error.\n"); goto exit; }

	/* Increase */
	buffer[0]=0x01;buffer[1]=0x23;buffer[2]=0x45;
	printf("scCryptoflexCmdIncreaseMAC(Should fail):");
	ret = scCryptoflexCmdIncreaseMAC( ri, ci, buffer, authkey, mac,
		SC_CRYPTOFLEX_ALGO_DES, rbuffer, &resplen );
	if( (ret==0) && (ci->sw[0]==0x62) && (ci->sw[1]==0x83) )
	{ printf(" OK\n"); } else
	{ printf(" Error.\n"); goto exit; }

	/* Get Challenge */
	resplen=8;
	ret = scCryptoflexCmdGetChall( ri, ci, mac, &resplen );
	if( (ret==0) && (ci->sw[0]==0x90) && (ci->sw[1]==0x00) )
	{ } else
	{ printf(" Error.\n"); goto exit; }

/* Rehabilitate */

	/* Rehabilitate */
	printf("scCryptoflexCmdRehabilitateMAC:");
	ret = scCryptoflexCmdRehabilitateMAC( ri, ci, authkey, mac,
		SC_CRYPTOFLEX_ALGO_DES );
	if( (ret==0) && (ci->sw[0]==0x90) && (ci->sw[1]==0x00) )
	{ printf(" OK\n"); } else
	{ printf(" Error.\n"); goto exit; }

	/* Get Challenge */
	resplen=8;
	ret = scCryptoflexCmdGetChall( ri, ci, mac, &resplen );
	if( (ret==0) && (ci->sw[0]==0x90) && (ci->sw[1]==0x00) )
	{ } else
	{ printf(" Error.\n"); goto exit; }

	/* Increase */
	buffer[0]=0x01;buffer[1]=0x23;buffer[2]=0x45;
	printf("scCryptoflexCmdIncreaseMAC:");
	ret = scCryptoflexCmdIncreaseMAC( ri, ci, buffer, authkey, mac,
		SC_CRYPTOFLEX_ALGO_DES, rbuffer, &resplen );
	if( (ret==0) && (ci->sw[0]==0x90) && (ci->sw[1]==0x00) )
	{ printf(" OK\n"); } else
	{ printf(" Error.\n"); goto exit; }
	printarray( "  rsp:", resplen, rbuffer );

	/* Select DF */
	ret = scCryptoflexCmdSelectFile( ri, ci, 0x2000, rbuffer, &resplen );
	if( (ret==0) && (ci->sw[0]==0x90) && (ci->sw[1]==0x00) )
	{ } else
	{ printf(" Error.\n"); goto exit; }

	/* Get Challenge */
	resplen=8;
	ret = scCryptoflexCmdGetChall( ri, ci, mac, &resplen );
	if( (ret==0) && (ci->sw[0]==0x90) && (ci->sw[1]==0x00) )
	{ } else
	{ printf(" Error.\n"); goto exit; }

/* Delete */

	/* Delete file */
	printf("scCryptoflexCmdDeleteFileMAC:");
	ret = scCryptoflexCmdDeleteFileMAC( ri, ci, 0x2003, authkey, mac,
		SC_CRYPTOFLEX_ALGO_DES );
	if( (ret==0) && (ci->sw[0]==0x90) && (ci->sw[1]==0x00) )
	{ printf(" OK\n"); } else
	{ printf(" Error.\n"); goto exit; }

	/* Get Challenge */
	resplen=8;
	ret = scCryptoflexCmdGetChall( ri, ci, mac, &resplen );
	if( (ret==0) && (ci->sw[0]==0x90) && (ci->sw[1]==0x00) )
	{ } else
	{ printf(" Error.\n"); goto exit; }

	/* Delete transparent file 0000 */
	ret = scCryptoflexCmdDeleteFileMAC( ri, ci, 0x0000, authkey, mac,
		SC_CRYPTOFLEX_ALGO_DES );
	if( (ret==0) && (ci->sw[0]==0x90) && (ci->sw[1]==0x00) )
	{ } else
	{ printf(" Error.\n"); goto exit; }

	/* Get Challenge */
	resplen=8;
	ret = scCryptoflexCmdGetChall( ri, ci, mac, &resplen );
	if( (ret==0) && (ci->sw[0]==0x90) && (ci->sw[1]==0x00) )
	{ } else
	{ printf(" Error.\n"); goto exit; }

	/* Delete transparent file */
	ret = scCryptoflexCmdDeleteFileMAC( ri, ci, 0x2001, authkey, mac,
		SC_CRYPTOFLEX_ALGO_DES );
	if( (ret==0) && (ci->sw[0]==0x90) && (ci->sw[1]==0x00) )
	{ } else
	{ printf(" Error.\n"); goto exit; }

	/* Select MF */
	ret = scCryptoflexCmdSelectFile( ri, ci, 0x3F00, rbuffer, &resplen );
	if( (ret==0) && (ci->sw[0]==0x90) && (ci->sw[1]==0x00) )
	{ } else
	{ printf(" Error.\n"); goto exit; }

	/* Verify transport key */
	ret = scCryptoflexCmdVerifyKey( ri, ci, 1, authkey, 8 );
	if( (ret==0) && (ci->sw[0]==0x90) && (ci->sw[1]==0x00) )
	{ } else
	{ printf(" Error.\n"); goto exit; }

	/* Delete DF 2000 */
	ret = scCryptoflexCmdDeleteFile( ri, ci, 0x2000 );
	if( (ret==0) && (ci->sw[0]==0x90) && (ci->sw[1]==0x00) )
	{ } else
	{ printf(" Error.\n"); goto exit; }

#endif /* WITH_DES */

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

