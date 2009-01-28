/* $Id: tgsmsim.c 1617 2005-11-03 17:41:39Z laforge $ */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
/* #include <unistd.h> */
#include <sio/sio.h>
#include <scez/scgeneral.h>
#include <scez/scsmartcard.h>
#include <scez/screader.h>
#include <scez/cards/scgsmsim.h>

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
	/* BYTE buffer[255]; */
	BYTE rbuffer[256];
	int resplen;

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

/* SIO_SetLogFile( ri->si, "LogGsmSim.txt" ); */

		/* Reset Card */
		ret= scReaderResetCard( ri, ci );
		checkreturn("Error: scReaderResetCard\n");

		/* Get Card Type */
		ret = scSmartcardGetCardType( ci );
		checkreturn("Error: scReaderGetCardType\n");
		if( ci->type!=SC_CARD_GSMSIM )
		{ printf("Error: Wrong Card.\n"); break; }

#if 0
	/* Get Resp */
		printf("scGsmsimCmdGetResp:");
		ci->sw[0]=0x9F,ci->sw[1]=0x00;
		ret = scGsmsimCmdGetResp( ri, ci, rbuffer, &resplen );
		if( !( (ret==0) && (ci->sw[0]==0x90) && (ci->sw[1]==0x00) ) )
		{ printf(" Error.\n"); break; }
		printarray( " OK\n  rsp:", resplen, rbuffer );
#endif

	/* Select */
		printf("scGsmsimCmdSelect(MF):");
		ret = scGsmsimCmdSelect( ri, ci, 0x3F00, rbuffer, &resplen );
		if( !( (ret==0) && (ci->sw[0]==0x90) && (ci->sw[1]==0x00) ) )
		{ printf(" Error.\n"); break; }
		printarray( " OK\n  rsp:", resplen, rbuffer );

		/* Select */
		printf("scGsmsimCmdSelect(EF):");
		ret = scGsmsimCmdSelect( ri, ci, 0x2FE2, rbuffer, &resplen );
		if( !( (ret==0) && (ci->sw[0]==0x90) && (ci->sw[1]==0x00) ) )
		{ printf(" Error.\n"); break; }
		printarray( " OK\n  rsp:", resplen, rbuffer );

	/* Read Binary */
		printf("scGsmsimCmdReadBin:");
		resplen=10;
		ret = scGsmsimCmdReadBin( ri, ci, 0, rbuffer, &resplen );
		if( !( (ret==0) && (ci->sw[0]==0x90) && (ci->sw[1]==0x00) ) )
		{ printf(" Error.\n"); break; }
		printarray( " OK\n  rsp:", resplen, rbuffer );

#if 0
		/* Select */
		printf("scGsmsimCmdSelect(EF):");
		ret = scGsmsimCmdSelect( ri, ci, 0x2FE0, rbuffer, &resplen );
		if( !( (ret==0) && (ci->sw[0]==0x90) && (ci->sw[1]==0x00) ) )
		{ printf(" Error.\n"); break; }
		printarray( " OK\n  rsp:", resplen, rbuffer );

		/* Read Binary */
		printf("scGsmsimCmdReadBin:");
		resplen=8;
		ret = scGsmsimCmdReadBin( ri, ci, 0, rbuffer, &resplen );
		if( !( (ret==0) && (ci->sw[0]==0x90) && (ci->sw[1]==0x00) ) )
		{ printf(" Error.\n"); break; }
		printarray( " OK\n  rsp:", resplen, rbuffer );

		/* Select */
		printf("scGsmsimCmdSelect(EF):");
		ret = scGsmsimCmdSelect( ri, ci, 0x2FE1, rbuffer, &resplen );
		if( !( (ret==0) && (ci->sw[0]==0x90) && (ci->sw[1]==0x00) ) )
		{ printf(" Error.\n"); break; }
		printarray( " OK\n  rsp:", resplen, rbuffer );

		/* Read Binary */
		printf("scGsmsimCmdReadBin:");
		resplen=8;
		ret = scGsmsimCmdReadBin( ri, ci, 0, rbuffer, &resplen );
		if( !( (ret==0) && (ci->sw[0]==0x90) && (ci->sw[1]==0x00) ) )
		{ printf(" Error.\n"); break; }
		printarray( " OK\n  rsp:", resplen, rbuffer );

		/* Select */
		printf("scGsmsimCmdSelect(EF):");
		ret = scGsmsimCmdSelect( ri, ci, 0x2FE3, rbuffer, &resplen );
		if( !( (ret==0) && (ci->sw[0]==0x90) && (ci->sw[1]==0x00) ) )
		{ printf(" Error.\n"); break; }
		printarray( " OK\n  rsp:", resplen, rbuffer );

		/* Read Binary */
		printf("scGsmsimCmdReadBin:");
		resplen=1;
		ret = scGsmsimCmdReadBin( ri, ci, 0, rbuffer, &resplen );
		if( !( (ret==0) && (ci->sw[0]==0x90) && (ci->sw[1]==0x00) ) )
		{ printf(" Error.\n"); break; }
		printarray( " OK\n  rsp:", resplen, rbuffer );
#endif /* 0 */

		/* Select */
		printf("scGsmsimCmdSelect(DF):");
		ret = scGsmsimCmdSelect( ri, ci, 0x7F20, rbuffer, &resplen );
		if( !( (ret==0) && (ci->sw[0]==0x90) && (ci->sw[1]==0x00) ) )
		{ printf(" Error.\n"); break; }
		printarray( " OK\n  rsp:", resplen, rbuffer );

#if 0
		/* Select */
		printf("scGsmsimCmdSelect(EF):");
		ret = scGsmsimCmdSelect( ri, ci, 0x6FAE, rbuffer, &resplen );
		if( !( (ret==0) && (ci->sw[0]==0x90) && (ci->sw[1]==0x00) ) )
		{ printf(" Error.\n"); break; }
		printarray( " OK\n  rsp:", resplen, rbuffer );

		/* Read Binary */
		printf("scGsmsimCmdReadBin:");
		resplen=1;
		ret = scGsmsimCmdReadBin( ri, ci, 0, rbuffer, &resplen );
		if( !( (ret==0) && (ci->sw[0]==0x90) && (ci->sw[1]==0x00) ) )
		{ printf(" Error.\n"); break; }
		printarray( " OK\n  rsp:", resplen, rbuffer );

		/* Select */
		printf("scGsmsimCmdSelect(EF):");
		ret = scGsmsimCmdSelect( ri, ci, 0x6F46, rbuffer, &resplen );
		if( !( (ret==0) && (ci->sw[0]==0x90) && (ci->sw[1]==0x00) ) )
		{ printf(" Error.\n"); break; }
		printarray( " OK\n  rsp:", resplen, rbuffer );

		/* Read Binary */
		printf("scGsmsimCmdReadBin:");
		resplen=17;
		ret = scGsmsimCmdReadBin( ri, ci, 0, rbuffer, &resplen );
		if( !( (ret==0) && (ci->sw[0]==0x90) && (ci->sw[1]==0x00) ) )
		{ printf(" Error.\n"); break; }
		printarray( " OK\n  rsp:", resplen, rbuffer );
		printf("%17s\n",rbuffer);
#endif /* 0 */

	} while( 0 );

printf("[ret: %d, sw: %.2X%.2X]",ret,ci->sw[0],ci->sw[1]);

	printf("  SW: %.2X%.2X\n",ci->sw[0],ci->sw[1]);

	ret = scReaderDeactivate( ri );
	if( ret!=0 ) printf("Error: scReaderDeactivate\n");

	ret = scReaderShutdown( ri );
	if( ret!=0 ) printf("Error: scReaderShutdown\n");

	scGeneralFreeCard( &ci );
	scGeneralFreeReader( &ri );

	scEnd();

	return(0);
}

