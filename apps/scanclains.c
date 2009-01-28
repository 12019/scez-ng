/****************************************************************************
*																			*
*						Copyright Matthias Bruestle 2000					*
*					For licensing, see the file COPYRIGHT					*
*																			*
****************************************************************************/

/* $Id: scanclains.c 1056 2001-09-17 23:20:37Z m $ */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
/* #include <unistd.h> */
#if 0
#include <sio/sio.h>
#endif
#include <scez/scgeneral.h>

#ifndef READER_TYPE
#define READER_TYPE SC_READER_DUMBMOUSE
#endif /* READER_TYPE */
#ifndef READER_SLOT
#define READER_SLOT 1
#endif /* READER_SLOT */
#ifndef READER_PORT
#define READER_PORT "0"
#endif /* READER_PORT */

#define checkreturn(f); if( ret!=SC_EXIT_OK ) { printf(f); goto exit; }

#define printreturn(); printf(" %d\n", ret); if( ret!=SC_EXIT_OK ) goto exit;

#define printstatus(); \
	printf(" %d\n", ret); \
	printf("  sw: %.2X %.2X\n", ci->sw[0], ci->sw[1]); \
	if( ret!=SC_EXIT_OK ) goto exit;

#define printarray( name, length, array ); \
	printf(name); \
	for( i=0; i<length; i++ ) printf(" %.2X",array[i]); \
	printf("\n");

int main( int argc, char *argv[] )
{
	SC_READER_INFO *ri;
	SC_CARD_INFO *ci;
	SC_READER_CONFIG rc;

	BYTE cmd[]={ 0x00, 0x00, 0x00, 0x00, 0x00 };
	SC_APDU apdu;

	int ret;
	BYTE buffer[255];

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
	if( ret!=SC_EXIT_OK ) { printf("Error.\n"); goto exit; }

	/* Activate Card */
	ret = scReaderActivate( ri );
	if( ret!=SC_EXIT_OK )
	{ printf("Error.\n"); goto exit; }

	/* Get Card Status */
	ret = scReaderCardStatus( ri );
	if( ret!=SC_EXIT_OK )
	{ printf("Error.\n"); goto exit; }
	if( !(ri->status&SC_CARD_STATUS_PRESENT) )
	{ printf("Error: No Card.\n"); goto exit; }

	/* Reset Card */
	ret= scReaderResetCard( ri, ci );
	if( ret!=SC_EXIT_OK )
	{ printf("Error.\n"); goto exit; }

	/* Get Card Type */
	ret = scSmartcardGetCardType( ci );
	checkreturn("Error: scReaderGetCardType\n");

#if 0
	SIO_SetLogFile( ri->si, "LogScanClaIns.txt" );
#endif

	apdu.cse=SC_APDU_CASE_2_SHORT;
	apdu.cmd=cmd;
	apdu.cmdlen=5;
	apdu.rsp=buffer;
	apdu.rsplen=0;

	/* scReaderSendAPDU */
	while(1) {

		/* Reset Card */
		ret=scReaderResetCard( ri, ci );
		if( ret!=SC_EXIT_OK )
		{ printf("Error.\n"); goto exit; }

		ret=scReaderSendAPDU( ri, ci, &apdu );

		if( apdu.rsplen>=2 ) {
			ci->sw[0] = apdu.rsp[apdu.rsplen-2];
			ci->sw[1] = apdu.rsp[apdu.rsplen-1];
		}

		if( (ret==SC_EXIT_OK) && (ci->sw[0]!=0x6E) && (ci->sw[0]!=0x6D) )
		{ printf("%.2X %.2X: %.2X%.2X\n",cmd[0],cmd[1],ci->sw[0],ci->sw[1]); }
		if( ret!=SC_EXIT_OK ) printf("%.2X %.2X: Warning: ret=%d\n",cmd[0],cmd[1],ret);

		cmd[1]+=2;
		if(cmd[1]==0x60) { cmd[1]=0x70; }
		if(cmd[1]==0x90) { cmd[1]=0xA0; }
		if(cmd[1]==0) {
			printf("CLA %.2X done\n",cmd[0]);
			cmd[0]+=4;
		}

		fflush(stdout);

		if((cmd[0]==0) && (cmd[1]==0)) exit;
	}

exit:
	ret = scReaderDeactivate( ri );
	if( ret!=SC_EXIT_OK )
	{ printf("Error.\n"); }

	printf("scReaderShutdown:");
	ret = scReaderShutdown( ri );
	if( ret!=SC_EXIT_OK )
	{ printf("Error.\n"); }

	scEnd();

	return(0);
}

