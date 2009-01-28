/* $Id: ttcos.c 1617 2005-11-03 17:41:39Z laforge $ */

/*
 * Testfile for TCOS commands. Tested with a card from Telesec.
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
/* #include <unistd.h> */
#include <scez/scgeneral.h>
#include <scez/scsmartcard.h>
#include <scez/screader.h>
#include <scez/cards/sctcos.h>

#ifndef PIN1
#define PIN1 "111111"
#endif

#ifndef PIN1LEN
#define PIN1LEN 6
#endif

/* #define UNLOCK_PIN */

#ifndef READER_TYPE
#define READER_TYPE SC_READER_DUMBMOUSE
#endif /* READER_TYPE */
#ifndef READER_SLOT
#define READER_SLOT 1
#endif /* READER_SLOT */
#ifndef READER_PORT
#define READER_PORT "0"
#endif /* READER_PORT */

#define checkreturn(f); if( ret!=0 ) { printf(f); break; }

#define printarray( name, length, array ); \
	printf(name); \
	for( i=0; i<length; i++ ) printf(" %.2X",array[i]); \
	printf("\n");

int main( int argc, char *argv[] )
{
	SC_READER_INFO *ri;
	SC_CARD_INFO *ci;
	SC_READER_CONFIG rc;

	int ret=0;
	int i;
	/* BYTE buffer[255]; */
	BYTE rbuffer[256];
	int resplen;
	/* Default AID */
	BYTE aid[16] = { 0xD2, 0x76, 0x00, 0x00, 0x66, 0x01 };
	int aidlen=6;

#ifdef WRITE_CERT
	FILE *file;
#endif /* WRITE_CERT */

	/* Example SHA1 ASN.1 encoded according to PKCS#1:
	 * T:30 L:1F V:[T:30 L:07 V:[06 05 2B 0E 63 02 1A] T:04 L:14
	 * V:[20 Bytes SHA1]]
	 */

	BYTE sha1[] = {
		0x30, 0x1F, 0x30, 0x07, 0x06, 0x05, 0x2B, 0x0E,
		0x63, 0x02, 0x1A, 0x04, 0x14, 0xFF, 0xFF, 0xFF,
		0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
		0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
		0xFF
	};

	do {

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
		{ printf("Error: No Card.\n"); break; }

		/* Reset Card */
		ret= scReaderResetCard( ri, ci );
		checkreturn("Error: scReaderResetCard\n");

		/* Get Card Type */
		ret = scSmartcardGetCardType( ci );
		checkreturn("Error: scReaderGetCardType\n");
		if( (ci->type&0xFFFFFF00)!=SC_CARD_TCOS )
		{ printf("Error: Wrong Card.\n"); break; }

		/* Select EF(DIR) */
		ret = scTcosCmdSelect( ri, ci, SC_TCOS_SELECT_ABS_PATH, 0,
				       "\x2F\x00", 2, SC_TCOS_RDATA_FCI, rbuffer, &resplen );
		checkreturn("File selection error.\n");
		if( ci->sw[0]==0x90 ) {
			/* Read */
			resplen=0;
			ret = scTcosCmdReadBinary( ri, ci, 0, rbuffer, &resplen );
			if( !( (ret==0) && (ci->sw[0]==0x90) && (ci->sw[1]==0x00) ) )
			{ printf(" Error.\n"); break; }
			
			/* Should parse the BER structure. */
			printarray( "AID:", rbuffer[1], (rbuffer+2) );
			aidlen=rbuffer[1];
			memcpy(aid, (rbuffer+2), aidlen);
		}

/* Select */
		/* Select MF */
		printf("scTcosCmdSelect(MF):");
		ret = scTcosCmdSelect( ri, ci, SC_TCOS_SELECT_MF, 0, NULL, 0,
			SC_TCOS_RDATA_FCP, rbuffer, &resplen );
		if( !( (ret==0) && (ci->sw[0]==0x90) && (ci->sw[1]==0x00) ) )
		{ printf(" Error.\n"); break; }
		printarray( " OK\n  rsp:", resplen, rbuffer );

/* Ask Random */
		/* Ask Random */
		resplen=8;
		printf("scTcosCmdAskRandom:");
		ret = scTcosCmdAskRandom( ri, ci, rbuffer, &resplen );
		if( !( (ret==0) && (ci->sw[0]==0x90) && (ci->sw[1]==0x00) ) )
		{ printf(" Error.\n"); break; }
		printarray( " OK\n  rsp:", resplen, rbuffer );

		/* Select EF */
		printf("scTcosCmdSelFil(EF):");
		ret = scTcosCmdSelect( ri, ci, SC_TCOS_SELECT_EF, 0x2F00, NULL, 0,
			SC_TCOS_RDATA_FCP, rbuffer, &resplen );
		if( !( (ret==0) && (ci->sw[0]==0x90) && (ci->sw[1]==0x00) ) )
		{ printf(" Error.\n"); break; }
		printarray( " OK\n  rsp:", resplen, rbuffer );

/* Read Binary */
		/* Read Binary */
		resplen = 36;
		printf("scTcosCmdReadBinary:");
		ret = scTcosCmdReadBinary( ri, ci, 0, rbuffer, &resplen );
		if( !( (ret==0) && (ci->sw[0]==0x90) && (ci->sw[1]==0x00) ) )
		{ printf(" Error.\n"); break; }
		printarray( " OK\n  rsp:", resplen, rbuffer );

		/* Select Path */
		printf("scTcosCmdSelFil(Absolute Path):");
		ret = scTcosCmdSelect( ri, ci, SC_TCOS_SELECT_ABS_PATH, 0,
			"\x2F\x02", 2, SC_TCOS_RDATA_FCP, rbuffer, &resplen );
		if( !( (ret==0) && (ci->sw[0]==0x90) && (ci->sw[1]==0x00) ) )
		{ printf(" Error.\n"); break; }
		printarray( " OK\n  rsp:", resplen, rbuffer );

		/* Read Binary */
		resplen = 12;
		printf("scTcosCmdReadBinary:");
		ret = scTcosCmdReadBinary( ri, ci, 0, rbuffer, &resplen );
		if( !( (ret==0) && (ci->sw[0]==0x90) && (ci->sw[1]==0x00) ) )
		{ printf(" Error.\n"); break; }
		printarray( " OK\n  rsp:", resplen, rbuffer );

		/* Select AID */
		printf("scTcosCmdSelFil(AID):");
		ret = scTcosCmdSelect( ri, ci, SC_TCOS_SELECT_AID, 0,
			aid, aidlen, SC_TCOS_RDATA_FCP, rbuffer,
			&resplen );
		if( !( (ret==0) && (ci->sw[0]==0x90) && (ci->sw[1]==0x00) ) )
		{ printf(" Error.\n"); break; }
		printarray( " OK\n  rsp:", resplen, rbuffer );

/* List directory */
		/* List directory */
		printf("scTcosCmdListDir:");
		ret = scTcosCmdListDir( ri, ci, SC_TCOS_LIST_EF, 0,
			rbuffer, &resplen );
		if( !( (ret==0) && (ci->sw[0]==0x90) && (ci->sw[1]==0x00) ) )
		{ printf(" Error.\n"); break; }
		printarray( " OK\n  rsp:", resplen, rbuffer );

#ifdef UNLOCK_PIN
/* Change password */
		/* Change password */
		printf("scTcosCmdChangePassword:");
		ret = scTcosCmdChangePassword( ri, ci, TRUE, 1,
			SC_TCOS_NULLPIN, SC_TCOS_NULLPINLEN, PIN1, PIN1LEN );
		if( !( (ret==0) && (ci->sw[0]==0x90) && (ci->sw[1]==0x00) ) )
		{ printf(" Error. (SW=%.2X%.2X)\n", ci->sw[0], ci->sw[1] ); break; }
		printf(" OK\n");
#endif /* UNLOCK_PIN */

		/* Select AID */
		ret = scTcosCmdSelect( ri, ci, SC_TCOS_SELECT_AID, 0,
			aid, aidlen, SC_TCOS_RDATA_FCP, rbuffer,
			&resplen );
		if( !( (ret==0) && (ci->sw[0]==0x90) && (ci->sw[1]==0x00) ) )
		{ printf(" Error.\n"); break; }

		/* Select EF */
		ret = scTcosCmdSelect( ri, ci, SC_TCOS_SELECT_EF, 0x4531, NULL, 0,
			SC_TCOS_RDATA_FCP, rbuffer, &resplen );
		if( !( (ret==0) && (ci->sw[0]==0x90) && (ci->sw[1]==0x00) ) )
		{ printf(" Error.\n"); break; }

/* Read Record */
		/* Read Record */
		resplen=0x00;
		printf("scTcosCmdReadRecord:");
		ret = scTcosCmdReadRecord( ri, ci, 0x02, SC_TCOS_RECORD_ABS,
			0, rbuffer, &resplen );
		if( !( (ret==0) && (ci->sw[0]==0x90) && (ci->sw[1]==0x00) ) )
		{ printf(" Error.\n"); break; }
		printarray( " OK\n  rsp:", resplen, rbuffer );

		/* Select AID */
		ret = scTcosCmdSelect( ri, ci, SC_TCOS_SELECT_AID, 0,
			aid, aidlen, SC_TCOS_RDATA_FCP, rbuffer,
			&resplen );
		if( !( (ret==0) && (ci->sw[0]==0x90) && (ci->sw[1]==0x00) ) )
		{ printf(" Error.\n"); break; }

/* Verify password */
		/* Verify password */
		printf("scTcosCmdVerifyPassword:");
		ret = scTcosCmdVerifyPassword( ri, ci, TRUE, 1, PIN1, PIN1LEN );
		if( !( (ret==0) && (ci->sw[0]==0x90) && (ci->sw[1]==0x00) ) )
		{ printf(" Error.\n"); break; }
		printf(" OK\n");

/* Perform security operation */
		/* Perform security operation */
		printf("scTcosCmdPerformSecOp:");
		ret = scTcosCmdPerformSecOp( ri, ci, SC_TCOS_PSO_IN_HASH,
			SC_TCOS_PSO_OUT_SIGN, sha1, sizeof( sha1 ), rbuffer, &resplen );
		if( !( (ret==0) && (ci->sw[0]==0x90) && (ci->sw[1]==0x00) ) )
		{ printf(" Error.\n"); break; }
		printarray( " OK\n  rsp:", resplen, rbuffer );

	} while( 0 );

	printf("  Ret: %d, SW: %.2X%.2X\n",ret,ci->sw[0],ci->sw[1]);

	ret = scReaderDeactivate( ri );
	if( ret!=0 ) printf("Error: scReaderDeactivate\n");

	ret = scReaderShutdown( ri );
	if( ret!=0 ) printf("Error: scReaderShutdown\n");

	scGeneralFreeCard( &ci );
	scGeneralFreeReader( &ri );

	scEnd();

	return(0);
}

