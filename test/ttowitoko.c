/* $Id: ttowitoko.c 1617 2005-11-03 17:41:39Z laforge $ */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
/* #include <unistd.h> */
#include <scez/scgeneral.h>
#include <scez/scsmartcard.h>
#include <scez/screader.h>
#include <scez/readers/sctowitoko.h>
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

	rc.param=READER_PORT;

	ret = scReaderGetConfig( argc, argv, &rc );
	if( ret!=SC_EXIT_OK ) {
		printf( "Error getting reader configuration.\n" );
		scEnd();
		return(1);
	};

	ri = scGeneralNewReader( SC_READER_TOWITOKO, 1 );
	if( ri==NULL ) { printf("Exit.\n"); scEnd(); return(1); };

	ci = scGeneralNewCard( );
	if( ci==NULL ) { printf("Exit.\n"); scEnd(); return(1); };

	/* Init Reader */
	printf("scReaderInit:");
	ret = scReaderInit( ri, rc.param );
	if( ret==0 )
	{ printf(" OK\n"); } else
	{ printf(" Error.\n"); goto exit; }

	/* Activate Card */
	printf("scReaderActivate:");
	ret = scReaderActivate( ri );
	if( ret==0 )
	{ printf(" OK\n"); } else
	{ printf(" Error.\n"); goto exit; }

	/* Get Card Status */
	printf("scReaderCardStatus:");
	ret = scReaderCardStatus( ri );
	if( ret==0 )
	{ printf(" OK\n"); } else
	{ printf(" Error.\n"); goto exit; }
	if( !(ri->status&SC_CARD_STATUS_PRESENT) )
	{ printf("Error: No Card.\n"); goto exit; }

	/* Reset Card */
	printf("scReaderResetCard:");
	ret= scReaderResetCard( ri, ci );
	if( ret==0 )
	{ printf(" OK\n"); } else
	{ printf(" Error.\n"); goto exit; }

	/* Get Card Type */
	ret = scSmartcardGetCardType( ci );
	checkreturn("Error: scReaderGetCardType\n");
	if( (ci->type!=SC_CARD_MULTIFLEX_3K) && (ci->type!=SC_CARD_MULTIFLEX_8K) &&
		(ci->type!=SC_CARD_MULTIFLEX_8K_DES) )
	{ printf("Error: Wrong Card.\n"); goto exit; }

	/* Select MF */
	printf("scMultiflexCmdSelectFile:");
	ret = scMultiflexCmdSelectFile( ri, ci, 0x3F00, buffer, &resplen );
	if( (ret==0) && (ci->sw[0]==0x90) && (ci->sw[1]==0x00) )
	{ printf(" OK\n"); } else
	{ printf(" Error.\n"); goto exit; }
	printarray( "  rsp:", resplen, buffer );

	/* scReaderSendAPDU */
	{
		BYTE cmd[]={ 0xC0, 0xA4, 0x00, 0x00, 0x02, 0x3F, 0x00, 0x00 };
		SC_APDU apdu;

		apdu.cse=SC_APDU_CASE_4_SHORT;
		apdu.cmd=cmd;
		apdu.cmdlen=8;
		apdu.rsp=buffer;
		apdu.rsplen=0;

		printf("scReaderSendAPDU:");
		ret=scReaderSendAPDU( ri, ci, &apdu );

		if( (ret==0) && (apdu.rsp[apdu.rsplen-2]==0x90) &&
			(apdu.rsp[apdu.rsplen-1]==0x00) )
		{ printf(" OK\n"); } else
		{ printf(" Error.\n"); goto exit; }
		printarray( "  rsp:", apdu.rsplen, buffer );
	}

exit:
	printf("scReaderDeactivate:");
	ret = scReaderDeactivate( ri );
	if( ret==0 )
	{ printf(" OK\n"); } else
	{ printf(" Error.\n"); }

	printf("scReaderShutdown:");
	ret = scReaderShutdown( ri );
	if( ret==0 )
	{ printf(" OK\n"); } else
	{ printf(" Error.\n"); }

	scEnd();

	return(0);
}

