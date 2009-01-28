/* $Id: tbasiccard.c 1617 2005-11-03 17:41:39Z laforge $ */

/*
 * Testfile for BasicCard commands.
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
/* #include <unistd.h> */
#include <scez/scgeneral.h>
#include <scez/scsmartcard.h>
#include <scez/screader.h>
#include <scez/cards/scbasiccard.h>

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

	int ret;
	int i;
	BYTE rbuffer[256];
	int resplen;

	WORD w1, w2, w3;
	BOOLEAN bool1;
	BYTE data[]={0x01,0x02,0x03,0x04,0x05};
#ifdef WITH_DES
	BYTE keye0[]={0x01,0x23,0x45,0x67,0x89,0xAB,0xCD,0xEF,
		0x89,0xAB,0xCD,0xEF,0x01,0x23,0x45,0x67};
	BYTE keye1[]={0x01,0x23,0x45,0x67,0x89,0xAB,0xCD,0xEF};
#endif /* WITH_DES */
	BYTE keyc0[]={0x57,0x03,0xCF,0x4C,0xC7,0xD5,0x62,0x1F,
		0x01,0x23,0x45,0x67,0x89,0xAB,0xCD,0xEF};

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
		if( (ci->type&0xFFFFFF00)!=SC_CARD_BASICCARD )
		{ printf("Error: Wrong Card.\n"); break; }

/* Get State */
		printf("scBasiccardCmdGetState:");
		ret = scBasiccardCmdGetState( ri, ci, rbuffer, &bool1 );
		scSmartcardSimpleProcessSW( ci, &i, NULL );
		if( !( (ret==0) && (i==SC_SW_OK) ) )
		{ printf(" Error.\n"); break; }
		printf(" OK\n");
		if( bool1 ) { printarray("  rsp:", 3, rbuffer ); }
		else { printarray("  State:", 1, rbuffer ); }

/* Set State */
		printf("scBasiccardCmdSetState:");
		ret = scBasiccardCmdSetState( ri, ci, 0x01 );
		scSmartcardSimpleProcessSW( ci, &i, NULL );
		if( !( (ret==0) && (i==SC_SW_OK) ) )
		{ printf(" Error.\n"); break; }
		printf(" OK\n");

		/* Reset Card */
		ret= scReaderResetCard( ri, ci );
		checkreturn("Error: scReaderResetCard\n");

		/* Get Card Type */
		ret = scSmartcardGetCardType( ci );
		checkreturn("Error: scReaderGetCardType\n");
		if( (ci->type&0xFFFFFF00)!=SC_CARD_BASICCARD )
		{ printf("Error: Wrong Card.\n"); break; }

/* EEPROM Size */
		printf("scBasiccardCmdEepromSize:");
		ret = scBasiccardCmdEepromSize( ri, ci, &w1, &w2 );
		scSmartcardSimpleProcessSW( ci, &i, NULL );
		if( !( (ret==0) && (i==SC_SW_OK) ) )
		{ printf(" Error.\n"); break; }
		printf(" OK\n");
		printf( "  Start address: %.4Xh\n", w1 );
		printf( "  Size:          %d\n", w2 );

		if( ci->type!=SC_CARD_BASICCARD_ENH_3 ) {
/* Clear EEPROM */
			printf("scBasiccardCmdClearEeprom:");
			ret = scBasiccardCmdClearEeprom( ri, ci, w1+600, 10 );
			scSmartcardSimpleProcessSW( ci, &i, NULL );
			if( !( (ret==0) && (i==SC_SW_OK) ) )
			{ printf(" Error.\n"); break; }
			printf(" OK\n");
		}

/* Not executed by default, because it damages cards,
 * if the filesystem has not been initialised. */

#if 0
/* EEPROM CRC */
		printf("scBasiccardCmdEepromCrc:");
		ret = scBasiccardCmdEepromCrc( ri, ci, w1+600, 10, &w3 );
		scSmartcardSimpleProcessSW( ci, &i, NULL );
		if( !( (ret==0) && (i==SC_SW_OK) ) )
		{ printf(" Error.\n"); break; }
		printf(" OK\n");
		printf( "  CRC: %.4Xh\n", w3 );
#endif

		if( ci->type!=SC_CARD_BASICCARD_ENH_3 ) {
/* Write EEPROM */
			printf("scBasiccardCmdWriteEeprom:");
			ret = scBasiccardCmdWriteEeprom( ri, ci, w1+600, data, 5 );
			scSmartcardSimpleProcessSW( ci, &i, NULL );
			if( !( (ret==0) && (i==SC_SW_OK) ) )
			{ printf(" Error.\n"); break; }
			printf(" OK\n");
		}

/* Read EEPROM */
		printf("scBasiccardCmdReadEeprom:");
		resplen=5;
		ret = scBasiccardCmdReadEeprom( ri, ci, w1+600, rbuffer, &resplen );
		scSmartcardSimpleProcessSW( ci, &i, NULL );
		if( !( (ret==0) && (i==SC_SW_OK) ) )
		{ printf(" Error.\n"); break; }
		printf(" OK\n");
		printarray("  Read:", resplen, rbuffer );

/* Echo */
		printf("scBasiccardCmdEcho:");
		ret = scBasiccardCmdEcho( ri, ci, 1, data, 5, rbuffer, &resplen );
		scSmartcardSimpleProcessSW( ci, &i, NULL );
		if( !( (ret==0) && (i==SC_SW_OK) ) )
		{ printf(" Error.\n"); break; }
		printf(" OK\n");
		printarray("  Echo:", resplen, rbuffer );

		printf("scBasiccardCmdSetState:");
		ret = scBasiccardCmdSetState( ri, ci, 0x02 );
		scSmartcardSimpleProcessSW( ci, &i, NULL );
		if( !( (ret==0) && (i==SC_SW_OK) ) )
		{ printf(" Error.\n"); break; }
		printf(" OK\n");

		/* Reset Card */
		ret= scReaderResetCard( ri, ci );
		checkreturn("Error: scReaderResetCard\n");

		/* Get Card Type */
		ret = scSmartcardGetCardType( ci );
		checkreturn("Error: scReaderGetCardType\n");
#if 0
		if( (ci->type&0xFFFFFF00)!=SC_CARD_BASICCARD )
		{ printf("Error: Wrong Card.\n"); break; }
#endif

/* Get Application ID */
		printf("scBasiccardCmdGetApplId:");
		ret = scBasiccardCmdGetApplId( ri, ci, rbuffer, &resplen );
		scSmartcardSimpleProcessSW( ci, &i, NULL );
		if( !( (ret==0) && (i==SC_SW_OK) ) )
		{ printf(" Error.\n"); break; }
		printf(" OK\n");
		printarray("  Application ID:", resplen, rbuffer );

		if( ci->type==SC_CARD_BASICCARD_ENH ) {

#ifdef WITH_DES

/* Start Encryption */
			printf("scBasiccardCmdStartEncr:");
			ret = scBasiccardCmdStartEncr( ri, ci, SC_BASICCARD_ALGO_3DES,
				0, keye0, data );
			scSmartcardSimpleProcessSW( ci, &i, NULL );
			if( !( (ret==0) && (i==SC_SW_OK) ) )
			{ printf(" Error.\n"); break; }
			printf(" OK\n");

			printf("scBasiccardCmdEcho(3DES):");
			ret = scBasiccardCmdEcho( ri, ci, 1, data, 5, rbuffer,
				&resplen );
			scSmartcardSimpleProcessSW( ci, &i, NULL );
			if( !( (ret==0) && (i==SC_SW_OK) ) )
			{ printf(" Error.\n"); break; }
			printf(" OK\n");
			printarray("  Echo:", resplen, rbuffer );

			printf("scBasiccardCmdGetApplId(3DES):");
			ret = scBasiccardCmdGetApplId( ri, ci, rbuffer, &resplen );
			scSmartcardSimpleProcessSW( ci, &i, NULL );
			if( !( (ret==0) && (i==SC_SW_OK) ) )
			{ printf(" Error.\n"); break; }
			printf(" OK\n");
			printarray("  Application ID:", resplen, rbuffer );

/* End Encryption */
			printf("scBasiccardCmdEndEncr:");
			ret = scBasiccardCmdEndEncr( ri, ci );
			scSmartcardSimpleProcessSW( ci, &i, NULL );
			if( !( (ret==0) && (i==SC_SW_OK) ) )
			{ printf(" Error.\n"); break; }
			printf(" OK\n");

			printf("scBasiccardCmdStartEncr:");
			ret = scBasiccardCmdStartEncr( ri, ci, SC_BASICCARD_ALGO_DES,
				1, keye1, data );
			scSmartcardSimpleProcessSW( ci, &i, NULL );
			if( !( (ret==0) && (i==SC_SW_OK) ) )
			{ printf(" Error.\n"); break; }
			printf(" OK\n");

			printf("scBasiccardCmdEcho(DES):");
			ret = scBasiccardCmdEcho( ri, ci, 1, data, 5, rbuffer,
				&resplen );
			scSmartcardSimpleProcessSW( ci, &i, NULL );
			if( !( (ret==0) && (i==SC_SW_OK) ) )
			{ printf(" Error.\n"); break; }
			printf(" OK\n");
			printarray("  Echo:", resplen, rbuffer );

			printf("scBasiccardCmdEndEncr:");
			ret = scBasiccardCmdEndEncr( ri, ci );
			scSmartcardSimpleProcessSW( ci, &i, NULL );
			if( !( (ret==0) && (i==SC_SW_OK) ) )
			{ printf(" Error.\n"); break; }
			printf(" OK\n");

#endif

/* File I/O */
			printf("scBasiccardCmdFileIo:");
			resplen=0;
			ret = scBasiccardCmdFileIo( ri, ci, 0x84, 0, (BYTE *)"\\F*",
				3, rbuffer, rbuffer+1, &resplen );
			scSmartcardSimpleProcessSW( ci, &i, NULL );
			if( !( (ret==0) && (i==SC_SW_OK) ) )
			{ printf(" Error.\n"); break; }
			printf(" OK\n");
			printarray("  Status:", 1, rbuffer );
			printarray("  Files: ", resplen, (rbuffer+1) );
		} else if( ci->type==SC_CARD_BASICCARD_COMP ) {
/* Start Encryption */
			printf("scBasiccardCmdStartEncr:");
			ret = scBasiccardCmdStartEncr( ri, ci, SC_BASICCARD_ALGO_LFSR,
				0, keyc0, data );
			scSmartcardSimpleProcessSW( ci, &i, NULL );
			if( !( (ret==0) && (i==SC_SW_OK) ) )
			{ printf(" Error.\n"); break; }
			printf(" OK\n");

			printf("scBasiccardCmdEcho(LFSR):");
			ret = scBasiccardCmdEcho( ri, ci, 1, data, 5, rbuffer,
				&resplen );
			scSmartcardSimpleProcessSW( ci, &i, NULL );
			if( !( (ret==0) && (i==SC_SW_OK) ) )
			{ printf(" Error.\n"); break; }
			printf(" OK\n");
			printarray("  Echo:", resplen, rbuffer );

			printf("scBasiccardCmdGetApplId(LFSR):");
			ret = scBasiccardCmdGetApplId( ri, ci, rbuffer, &resplen );
			scSmartcardSimpleProcessSW( ci, &i, NULL );
			if( !( (ret==0) && (i==SC_SW_OK) ) )
			{ printf(" Error.\n"); break; }
			printf(" OK\n");
			printarray("  Application ID:", resplen, rbuffer );

/* End Encryption */
			printf("scBasiccardCmdEndEncr:");
			ret = scBasiccardCmdEndEncr( ri, ci );
			scSmartcardSimpleProcessSW( ci, &i, NULL );
			if( !( (ret==0) && (i==SC_SW_OK) ) )
			{ printf(" Error.\n"); break; }
			printf(" OK\n");

/* Start Encryption */
			printf("scBasiccardCmdStartEncr:");
			ret = scBasiccardCmdStartEncr( ri, ci,
				SC_BASICCARD_ALGO_LFSR_CRC, 0, keyc0, data );
			scSmartcardSimpleProcessSW( ci, &i, NULL );
			if( !( (ret==0) && (i==SC_SW_OK) ) )
			{ printf(" Error.\n"); break; }
			printf(" OK\n");

			printf("scBasiccardCmdEcho(LFSR/CRC):");
			ret = scBasiccardCmdEcho( ri, ci, 1, data, 5, rbuffer,
				&resplen );
			scSmartcardSimpleProcessSW( ci, &i, NULL );
			if( !( (ret==0) && (i==SC_SW_OK) ) )
			{ printf(" Error.\n"); break; }
			printf(" OK\n");
			printarray("  Echo:", resplen, rbuffer );

			printf("scBasiccardCmdGetApplId(LFSR/CRC):");
			ret = scBasiccardCmdGetApplId( ri, ci, rbuffer, &resplen );
			scSmartcardSimpleProcessSW( ci, &i, NULL );
			if( !( (ret==0) && (i==SC_SW_OK) ) )
			{ printf(" Error.\n"); break; }
			printf(" OK\n");
			printarray("  Application ID:", resplen, rbuffer );

/* End Encryption */
			printf("scBasiccardCmdEndEncr:");
			ret = scBasiccardCmdEndEncr( ri, ci );
			scSmartcardSimpleProcessSW( ci, &i, NULL );
			if( !( (ret==0) && (i==SC_SW_OK) ) )
			{ printf(" Error.\n"); break; }
			printf(" OK\n");
		}

/* Assign NAD */
		printf("scBasiccardCmdAssignNad:");
		ret = scBasiccardCmdAssignNad( ri, ci, 0x00 );
		scSmartcardSimpleProcessSW( ci, &i, NULL );
		if( !( (ret==0) && (i==SC_SW_OK) ) )
		{ printf(" Error.\n"); break; }
		printf(" OK\n");

		printf("scBasiccardCmdSetState:");
		ret = scBasiccardCmdSetState( ri, ci, 0x01 );
		scSmartcardSimpleProcessSW( ci, &i, NULL );
		if( !( (ret==0) && (i==SC_SW_OK) ) )
		{ printf(" Error.\n"); break; }
		printf(" OK\n");

	} while( 0 );

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

