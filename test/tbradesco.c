/* $Id: tbradesco.c 1617 2005-11-03 17:41:39Z laforge $ */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#if 0
#include <sio/sio.h>
#endif
#include <scez/scgeneral.h>
#include <scez/scsmartcard.h>
#include <scez/screader.h>
#include <scez/cards//scgpk4000.h>

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

/* This function is scGpk4000CmdDebit striped of the MAC check. */

#if 0
int scBradescoCmdDebit( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	BYTE pfile, BYTE *debit, BYTE *ttc )
{
	BYTE cmd[]={ 0x80, 0x34, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
	BYTE rsp[ 2+2 ];
	SC_APDU apdu;
	int ret;

	ci->sw[0]=0x00; ci->sw[1]=0x00;

	apdu.cse=SC_APDU_CASE_4_SHORT;
	apdu.cmd=cmd;
	apdu.cmdlen=14;
	apdu.rsp=rsp;
	apdu.rsplen=0;

	memcpy( cmd+5+1, debit, 3 );
	memcpy( cmd+5+4, ttc, 4 );

	ret=scReaderSendAPDU( ri, ci, &apdu );
	memset( cmd, 0, sizeof(cmd) );
	if( ret!=SC_EXIT_OK ) return( ret );

	if( apdu.rsplen<2 ) return( SC_EXIT_BAD_SW );

	ci->sw[0] = apdu.rsp[apdu.rsplen-2];
	ci->sw[1] = apdu.rsp[apdu.rsplen-1];

	apdu.rsplen-=2;

	return( SC_EXIT_OK );
}
#endif

int main( int argc, char *argv[] )
{
	SC_READER_INFO *ri;
	SC_CARD_INFO *ci;
	SC_READER_CONFIG rc;

	int ret;
	int i;
	BYTE rbuffer[256];
	int resplen;
#ifdef WITH_DES
#if 0
	BYTE chall[8], ctc[3];
	BYTE nullkey[16]={0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
#endif
#endif /* WITH_DES */

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
		if( ci->type!=SC_CARD_BRADESCO )
		{ printf("Error: Wrong Card.\n"); break; }

#if 0
		SIO_SetLogFile( ri->si, "LogTBradesco.txt" );
#endif

/* Get Response */
		/* Get Response */
		printf("scGpk4000CmdGetResp:");
		ci->sw[1]=0x12;
		ret = scGpk4000CmdGetResp( ri, ci, rbuffer, &resplen );
		if( !( (ret==0) && (ci->sw[0]==0x90) && (ci->sw[1]==0x00) ) )
		{ printf(" Error.\n"); break; }
		printarray( " OK\n  rsp:", resplen, rbuffer );

/* Get Info */
		/* Get Info */
		printf("scGpk4000CmdGetInfo(Card SN):");
		ret = scGpk4000CmdGetInfo( ri, ci, SC_GPK4000_INFO_CARD_SN,
			rbuffer, &resplen );
		if( !( (ret==0) && (ci->sw[0]==0x90) && (ci->sw[1]==0x00) ) )
		{ printf(" Error.\n"); break; }
		printarray( " OK\n  rsp:", resplen, rbuffer );

		/* Get Info */
		printf("scGpk4000CmdGetInfo(Issuer SN):");
		ret = scGpk4000CmdGetInfo( ri, ci, SC_GPK4000_INFO_ISSUER_SN,
			rbuffer, &resplen );
		if( !( (ret==0) && (ci->sw[0]==0x90) && (ci->sw[1]==0x00) ) )
		{ printf(" Error.\n"); break; }
		printarray( " OK\n  rsp:", resplen, rbuffer );

		/* Get Info */
		printf("scGpk4000CmdGetInfo(Issuer REF):");
		ret = scGpk4000CmdGetInfo( ri, ci, SC_GPK4000_INFO_ISSUER_REF,
			rbuffer, &resplen );
		if( !( (ret==0) && (ci->sw[0]==0x90) && (ci->sw[1]==0x00) ) )
		{ printf(" Error.\n"); break; }
		printarray( " OK\n  rsp:", resplen, rbuffer );

		/* Get Info */
		printf("scGpk4000CmdGetInfo(Desc Addr):");
		ret = scGpk4000CmdGetInfo( ri, ci, SC_GPK4000_INFO_DESC_ADDR,
			rbuffer, &resplen );
		if( !( (ret==0) && (ci->sw[0]==0x90) && (ci->sw[1]==0x00) ) )
		{ printf(" Error.\n"); break; }
		printarray( " OK\n  rsp:", resplen, rbuffer );

/* Select */
		/* Select DF System */
		printf("scGpk4000CmdSelFil(DF):");
		ret = scGpk4000CmdSelFil( ri, ci, SC_GPK4000_SELECT_DF,
			SC_GPK4000_FID_DF_SYSTEM, NULL, 0, rbuffer, &resplen );
		if( !( (ret==0) && (ci->sw[0]==0x90) && (ci->sw[1]==0x00) ) )
		{ printf(" Error.\n"); break; }
		printarray( " OK\n  rsp:", resplen, rbuffer );

		/* Select EF Card */
		printf("scGpk4000CmdSelFil(EF):");
		ret = scGpk4000CmdSelFil( ri, ci, SC_GPK4000_SELECT_EF,
			SC_GPK4000_FID_EF_CARD, NULL, 0, rbuffer, &resplen );
		if( !( (ret==0) && (ci->sw[0]==0x90) && (ci->sw[1]==0x00) ) )
		{ printf(" Error.\n"); break; }
		printarray( " OK\n  rsp:", resplen, rbuffer );

/* Read Binary */
		/* Read Binary */
		resplen = 12;
		printf("scGpk4000CmdRdBin:");
		ret = scGpk4000CmdRdBin( ri, ci, 0, rbuffer, &resplen );
		if( !( (ret==0) && (ci->sw[0]==0x90) && (ci->sw[1]==0x00) ) )
		{ printf(" Error.\n"); break; }
		printarray( " OK\n  rsp:", resplen, rbuffer );

		/* Read Binary */
		resplen = 2;
		printf("scGpk4000CmdRdBin(Offset):");
		ret = scGpk4000CmdRdBin( ri, ci, 1, rbuffer, &resplen );
		if( !( (ret==0) && (ci->sw[0]==0x90) && (ci->sw[1]==0x00) ) )
		{ printf(" Error.\n"); break; }
		printarray( " OK\n  rsp:", resplen, rbuffer );

		/* Read Binary */
		resplen = 12;
		printf("scGpk4000CmdRdBin(SFID):");
		ret = scGpk4000CmdRdBin( ri, ci, 0x8200, rbuffer, &resplen );
		if( !( (ret==0) && (ci->sw[0]==0x90) && (ci->sw[1]==0x00) ) )
		{ printf(" Error.\n"); break; }
		printarray( " OK\n  rsp:", resplen, rbuffer );

		/* Select Parent */
		printf("scGpk4000CmdSelFil(Parent):");
		ret = scGpk4000CmdSelFil( ri, ci, SC_GPK4000_SELECT_PARENT,
			0, NULL, 0, rbuffer, &resplen );
		if( !( (ret==0) && (ci->sw[0]==0x90) && (ci->sw[1]==0x00) ) )
		{ printf(" Error.\n"); break; }
		printarray( " OK\n  rsp:", resplen, rbuffer );

		/* Select DF System */
		ret = scGpk4000CmdSelFil( ri, ci, SC_GPK4000_SELECT_DF,
			0x0200, NULL, 0, rbuffer, &resplen );
		if( !( (ret==0) && (ci->sw[0]==0x90) && (ci->sw[1]==0x00) ) )
		{ printf(" Error.\n"); break; }

#ifdef WITH_DES
#if 0
/* SelPk */
		/* SelPk */
		chall[0]=0x00;chall[1]=0x01;chall[2]=0x02;chall[3]=0x03;
		chall[4]=0x04;chall[5]=0x05;chall[6]=0x06;chall[7]=0x07;
		printf("scGpk4000CmdSelPk:");
		ret = scGpk4000CmdSelPk( ri, ci, 0, 3, 2, nullkey, chall, ctc );
		if( !( (ret==SC_EXIT_BAD_CHECKSUM) && (ci->sw[0]==0x90) &&
			(ci->sw[1]==0x00) ) )
		{ printf(" Error.\n"); break; }
		printf(" OK.\n");

/* Debit */
		/* Debit */
		printf("scBradescoCmdDebit:");
		ret = scBradescoCmdDebit( ri, ci, 2, "\x00\x00\x01",
			"\x00\x01\x02\x04" );
		if( !( (ret==0) && (ci->sw[0]==0x90) && (ci->sw[1]==0x00) ) )
		{ printf(" Error.\n"); break; }
		printf(" OK.\n");
		/* Returns 9804 */
#endif
#endif /* WITH_DES */

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

