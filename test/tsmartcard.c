/* $Id: tsmartcard.c 1617 2005-11-03 17:41:39Z laforge $ */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
/* #include <unistd.h> */
#include <scez/scgeneral.h>
#include <scez/scsmartcard.h>
#include <scez/screader.h>
#include <scez/cards/scmultiflex.h>

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

	/* Test scSmartcardProcessATR */

	ci->atr[0]=0x3B;
	ci->atr[1]=0xE2;
	ci->atr[2]=0x00;
	ci->atr[3]=0x00;
	ci->atr[4]=0x40;
	ci->atr[5]=0x20;
	ci->atr[6]=0x49;
	ci->atr[7]=0x07;
	ci->atrlen=8;

	ret=scSmartcardProcessATR( ci );
	if( ret!=0 ) { printf("Error: scSmartcardProcessATR\n"); return(1); }

	printf("scSmartcardProcessATR:\n");
	printf("  Protocol: %d (0)\n  Convention: %d (1)\n",
		ci->protocol, ci->direct);
	printf("  WWT: %ld (30720)\n",ci->t0.wwt);

	ci->atr[0]=0x3B;
	ci->atr[1]=0xE1;
	ci->atr[2]=0x00;
	ci->atr[3]=0xFF;
	ci->atr[4]=0x81;
	ci->atr[5]=0x31;
	ci->atr[6]=0x50;
	ci->atr[7]=0x45;
	ci->atr[8]=0x12;
	ci->atr[9]=0xA9;
	ci->atrlen=10;

	ret=scSmartcardProcessATR( ci );
	if( ret!=0 ) { printf("Error: scSmartcardProcessATR\n"); return(1); }

	printf("scSmartcardProcessATR:\n");
	printf("  Protocol: %d (1)\n  Convention: %d (1)\n",
		ci->protocol, ci->direct);
	printf("  IFSC: %d (80)\n",ci->t1.ifsc);
	printf("  CWT: %ld (32)\n",ci->t1.cwt);
	printf("  BWT: %ld (1600000)\n",ci->t1.bwt);
	printf("  RC: %d (0)\n",ci->t1.rc);

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
	if( (ci->type!=SC_CARD_MULTIFLEX_3K) && (ci->type!=SC_CARD_MULTIFLEX_8K) &&
		(ci->type!=SC_CARD_MULTIFLEX_8K_DES) )
	{ printf("Error: Wrong Card.\n"); goto exit; }

exit:
	ret = scReaderDeactivate( ri );
	if( ret!=0 ) printf("Error: scReaderDeactivate\n");

	ret = scReaderShutdown( ri );
	if( ret!=0 ) printf("Error: scReaderShutdown\n");

	scEnd();

	return(0);
}

