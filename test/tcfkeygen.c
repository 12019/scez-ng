/* $Id: tcfkeygen.c 1617 2005-11-03 17:41:39Z laforge $ */

/*
 * Testfile for Cryptoflex RSA key generation.
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
/* #include <unistd.h> */
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
	/* Authkey for CC4 */
	BYTE authkey[]={0x2C,0x15,0xE5,0x26,0xE9,0x3E,0x8A,0x19};
	BYTE pinfile[]={0x01,0xFF,0xFF,0x12,0x34,0x56,0x78,0xFF,0xFF,0xFF,0xFF,
		0x01,0x01,0xAA,0xBB,0xCC,0xDD,0xFF,0xFF,0xFF,0xFF,0x01,0x01};
	BYTE oldpin[]={0x12,0x34,0x56,0x78,0xFF,0xFF,0xFF,0xFF};

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
	if( (ci->type!=SC_CARD_CRYPTOFLEX) &&
		(ci->type!=SC_CARD_CRYPTOFLEX_DES) &&
		(ci->type!=SC_CARD_CRYPTOFLEX_KEYGEN) )
	{ printf("Error: Wrong Card.\n"); goto exit; }

#if 0
	SIO_SetLogFile( ri->si, "LogCflexKeyGen.txt" );
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

/**********************/

	/* Select DF */
	ret = scCryptoflexCmdSelectFile( ri, ci, 0x2000, rbuffer, &resplen );
	if( (ret==0) && (ci->sw[0]==0x90) && (ci->sw[1]==0x00) )
	{ } else
	{ printf(" Error.\n"); goto exit; }

	/* Verify transport key */
	ret = scCryptoflexCmdVerifyKey( ri, ci, 1, authkey, 8 );
	if( (ret==0) && (ci->sw[0]==0x90) && (ci->sw[1]==0x00) )
	{ } else
	{ printf(" Error.\n"); goto exit; }

	/* Verify PIN */
	ret = scCryptoflexCmdVerifyPIN( ri, ci, 1, oldpin );
	if( (ret==0) && (ci->sw[0]==0x90) && (ci->sw[1]==0x00) )
	{ } else
	{ printf(" Error.\n"); goto exit; }

	/* Create transparent file 0012 */
	acond[0]=0x3F; acond[1]=0x44; acond[2]=0xFF; acond[3]=0x44;
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

	/* Select DF */
	ret = scCryptoflexCmdSelectFile( ri, ci, 0x2000, rbuffer, &resplen );
	if( (ret==0) && (ci->sw[0]==0x90) && (ci->sw[1]==0x00) )
	{ } else
	{ printf(" Error.\n"); goto exit; }

	/* Verify PIN */
	ret = scCryptoflexCmdVerifyPIN( ri, ci, 1, oldpin );
	if( (ret==0) && (ci->sw[0]==0x90) && (ci->sw[1]==0x00) )
	{ } else
	{ printf(" Error.\n"); goto exit; }

/* RSA Key Gen */
	/* RSA Key Gen */
	printf("scCryptoflexCmdRsaKeyGen:");
	ret = scCryptoflexCmdRsaKeyGen( ri, ci, 0x00, rbuffer, &resplen );
	if( (ret==0) && (ci->sw[0]==0x90) && (ci->sw[1]==0x00) )
	{ printf(" OK\n"); } else
	{ printf(" Error.\n"); goto exit; }
	printarray( "  rsp:", resplen, rbuffer );

	/* Select EF */
	ret = scCryptoflexCmdSelectFile( ri, ci, 0x0012, rbuffer, &resplen );
	if( (ret==0) && (ci->sw[0]==0x90) && (ci->sw[1]==0x00) )
	{ } else
	{ printf(" Error.\n"); goto exit; }

	resplen=200;
	printf("scCryptoflexCmdReadBinary:");
	ret = scCryptoflexCmdReadBinary( ri, ci, 0, rbuffer, &resplen );
	if( (ret==0) && (ci->sw[0]==0x90) && (ci->sw[1]==0x00) )
	{ printf(" OK\n"); } else
	{ printf(" Error.\n"); goto exit; }
	printarray( "  rsp:", resplen, rbuffer );

	resplen=126;
	printf("scCryptoflexCmdReadBinary:");
	ret = scCryptoflexCmdReadBinary( ri, ci, 200, rbuffer, &resplen );
	if( (ret==0) && (ci->sw[0]==0x90) && (ci->sw[1]==0x00) )
	{ printf(" OK\n"); } else
	{ printf(" Error.\n"); goto exit; }
	printarray( "  rsp:", resplen, rbuffer );

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

exit:
	ret = scReaderDeactivate( ri );
	if( ret!=0 ) printf("Error: scReaderDeactivate\n");

	ret = scReaderShutdown( ri );
	if( ret!=0 ) printf("Error: scReaderShutdown\n");

	scGeneralFreeCard( &ci );
	scGeneralFreeReader( &ri );

	scEnd();

	return(0);
}

