/* $Id: tctapi.c 1617 2005-11-03 17:41:39Z laforge $ */

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
#define READER_SLOT 0
#endif /* READER_SLOT */
#ifndef READER_PORT
#define READER_PORT "0"
#endif /* READER_PORT */

#define printreturn(); printf(" %d\n", ret); if( ret!=0 ) goto exit;

#define printstatus(); \
	printf(" %d\n", ret); \
	printf("  sw: %.2X %.2X\n", ci->sw[0], ci->sw[1]); \
	if( ret!=0 ) goto exit;

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
	int resplen;

	if( scInit() ) { printf("Exit.\n"); return(1); }

	rc.slot=READER_SLOT;
	rc.param=READER_PORT;

	ret = scReaderGetConfig( argc, argv, &rc );
	if( ret!=SC_EXIT_OK ) {
		printf( "Error getting reader configuration.\n" );
		scEnd();
		return(1);
	};

	ri = scGeneralNewReader( SC_READER_CTAPI, rc.slot );
	if( ri==NULL ) { printf("Exit.\n"); scEnd(); return(1); };

	ci = scGeneralNewCard( );
	if( ci==NULL ) { printf("Exit.\n"); scEnd(); return(1); };

	/* Init Reader */
	printf("scReaderInit:");
	ret = scReaderInit( ri, rc.param );
	printf(" %d\n", ret); if( ret!=0 ) return(1);

	/* Get Card Status */
	printf("scReaderCardStatus:");
	ret = scReaderCardStatus( ri );
	printreturn();
	if( !(ri->status&SC_CARD_STATUS_PRESENT) ) {printf("No Card.\n");goto exit;}

	/* Activate Card */
	printf("scReaderActivate:");
	ret = scReaderActivate( ri );
	printreturn();

	/* Reset Card */
	printf("scReaderResetCard:");
	ret= scReaderResetCard( ri, ci );
	printreturn();
printarray( "  ATR:", ci->atrlen, ci->atr );

	/* Get Card Type */
	printf("scSmartcardGetCardType:");
	ret = scSmartcardGetCardType( ci );
	printreturn();

	/* Select MF */
	printf("scMultiflexCmdSelectFile:");
	ret = scMultiflexCmdSelectFile( ri, ci, 0x3F00, buffer, &resplen );
	printstatus();
	printarray( "  rsp:", resplen, buffer );

exit:
	printf("scReaderDeactivate:");
	ret = scReaderDeactivate( ri );
	printf(" %d\n", ret);

	printf("scReaderShutdown:");
	ret = scReaderShutdown( ri );
	printf(" %d\n", ret);

	scEnd();

	return(0);
}

