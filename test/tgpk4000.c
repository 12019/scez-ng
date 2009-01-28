/* $Id: tgpk4000.c 1617 2005-11-03 17:41:39Z laforge $ */

/*
 * Testfile for Gpk4000 commands. If you use this with other cards than
 * the GPK4000-s be carefull with the offset of the EraseCard command.
 * A wrong offset may destroy the card.
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
/* #include <unistd.h> */
#if 0
#include <sio/sio.h>
#endif
#include <scez/scgeneral.h>
#include <scez/scsmartcard.h>
#include <scez/screader.h>
#include <scez/cards/scgpk4000.h>

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

#ifdef WITH_DES
	BYTE ac[6];
	BYTE admkey[]={'T','E','S','T',' ','K','E','Y',
		'T','E','S','T',' ','K','E','Y'};
	BYTE authkey[]={0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,
		0x11,0x12,0x13,0x14,0x15,0x16,0x17,0x18};
	BYTE chall[8];
	BYTE ctc[3];
	BYTE update[]={0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08};
	BYTE write[]={0x10,0x20,0x30,0x40,0x50,0x60,0x70,0x80};
	BYTE paykey[]={0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,
		0x11,0x12,0x13,0x14,0x15,0x16,0x17,0x18};
	BYTE keyfile[]={
		0x01,0x00,0x00,0xF6, /* Payment */
		0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,
		0x01,0x00,0x00,0xF6, /* Payment */
		0x11,0x12,0x13,0x14,0x15,0x16,0x17,0x18,
		0x03,0x00,0x00,0xF4, /* Auth */
		0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,
		0x03,0x00,0x00,0xF4, /* Auth */
		0x11,0x12,0x13,0x14,0x15,0x16,0x17,0x18 };
	BYTE pinfile[]={
		0x03,0x00,0x05,0x00,0x12,0x34,0x56,0x78, /* plain */
		0x12,0x34,0x56,0x78,
		0x43,0x00,0x00,0x00,0x11,0x22,0x33,0x44, /* cipherd, unlock */
		0x11,0x22,0x33,0x44 };
	BYTE pin[]={0x31,0x32,0x33,0x34,0x35,0x36,0x37,0x38};
	BYTE unlock[]={0x31,0x31,0x32,0x32,0x33,0x33,0x34,0x34};
	BYTE purse[]={
		0x00,0x12,0x34,	/* max balance */
		0x01,			/* key sfid */
		0x00,0x01,0x00, /* max free debit */
		0x00,			/* Cd/Cb */
		0x00,0x01,0x00,	/* current balance */
		0xFE,			/* cks */
		0x00,0x00,0x00,	/* backup balance */
		0xFF,			/* cks */
		0x00,0x00,		/* TTC */
		0x00,0x00		/* unused */
		};
	BYTE tm[]={
		0x00,0x00,0x00,0xFF,
		0x00,0x00,0x00,0xFF
		};
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
		if( ci->type!=SC_CARD_GPK4000_S )
		{ printf("Error: Wrong Card.\n"); break; }

#if 0
		SIO_SetLogFile( ri->si, "LogGpk4000.txt" );
#endif

/* Erase Card */
		printf("scGpk4000CmdEraseCard:");
		switch( ci->type ) {
		case SC_CARD_GPK4000_S:
			ret = scGpk4000CmdEraseCard( ri, ci, 7 );
			break;
		case SC_CARD_GPK8000:
		case SC_CARD_GPK8000_8K:
		case SC_CARD_GPK8000_16K:
			ret = scGpk4000CmdEraseCard( ri, ci, 0 );
			break;
		/* Don't know what to do with the other GPK4000s. */
		case SC_CARD_GPK4000:
		case SC_CARD_GPK4000_SP:
		case SC_CARD_GPK4000_SDO:
		default:
			printf( " Unknown card type" );
			ret = SC_EXIT_UNKNOWN_ERROR;
			break;
		}
		if( !( (ret==0) && (ci->sw[0]==0x90) && (ci->sw[1]==0x00) ) )
		{ printf(" Error.\n"); break; }
		printf(" OK.\n");

/* Get Info */
		/* Get Info */
		printf("scGpk4000CmdGetInfo(SN):");
		ret = scGpk4000CmdGetInfo( ri, ci, SC_GPK4000_INFO_CHIP_SN,
			rbuffer, &resplen );
		if( !( (ret==0) && (ci->sw[0]==0x90) && (ci->sw[1]==0x00) ) )
		{ printf(" Error.\n"); break; }
		printarray( " OK\n  rsp:", resplen, rbuffer );

/* Select */
		/* Select MF */
		printf("scGpk4000CmdSelFil(MF):");
		ret = scGpk4000CmdSelFil( ri, ci, SC_GPK4000_SELECT_MF,
			SC_GPK4000_FID_MF, NULL, 0, rbuffer, &resplen );
		if( !( (ret==0) && (ci->sw[0]==0x90) && (ci->sw[1]==0x00) ) )
		{ printf(" Error.\n"); break; }
		printarray( " OK\n  rsp:", resplen, rbuffer );

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

		/* Select MF */
		ret = scGpk4000CmdSelFil( ri, ci, SC_GPK4000_SELECT_MF,
			SC_GPK4000_FID_MF, NULL, 0, rbuffer, &resplen );
		if( !( (ret==0) && (ci->sw[0]==0x90) && (ci->sw[1]==0x00) ) )
		{ printf(" Error.\n"); break; }

#ifdef WITH_DES

/* SelFk */
		/* SelFk */
		chall[0]=0x00;chall[1]=0x01;chall[2]=0x02;chall[3]=0x03;
		chall[4]=0x04;chall[5]=0x05;chall[6]=0x06;chall[7]=0x07;
		printf("scGpk4000CmdSelFk:");
		ret = scGpk4000CmdSelFk( ri, ci, 0, TRUE, 0x01, admkey,
			chall );
		if( !( (ret==0) && (ci->sw[0]==0x90) && (ci->sw[1]==0x00) ) )
		{ printf(" Error.\n"); break; }
		printf(" OK.\n");

/* CrtDirCrycks */
		/* CrtDirCrycks */
		ac[0]=0x01;ac[1]=0x00;ac[2]=0x01;ac[3]=0x00;
		printf("scGpk4000CmdCrtDirCrycks:");
		ret = scGpk4000CmdCrtDirCrycks( ri, ci, 0x2000,
			"\xD2\x76\x00\x00\x92\x11\x11\x05\x00\x00\x01\x01\x00", 13,
			0x00, "\x01\x00\x01\x00" );
		if( !( (ret==0) && (ci->sw[0]==0x90) && (ci->sw[1]==0x00) ) )
		{ printf(" Error.\n"); break; }
		printf(" OK.\n");

		/* Select DF 2000 */
		printf("scGpk4000CmdSelFil(AID):");
		ret = scGpk4000CmdSelFil( ri, ci, SC_GPK4000_SELECT_AID,
			0, "\xD2\x76\x00\x00\x92\x11\x11\x05\x00\x00\x01\x01\x00", 13,
			rbuffer, &resplen );
		if( !( (ret==0) && (ci->sw[0]==0x90) && (ci->sw[1]==0x00) ) )
		{ printf(" Error.\n"); break; }
		printarray( " OK\n  rsp:", resplen, rbuffer );

/* CrtFileCrycks */
		/* CrtFileCrycks */
		printf("scGpk4000CmdCrtFileCrycks(TR):");
		ret = scGpk4000CmdCrtFileCrycks( ri, ci, 0x2001, 100,
			SC_GPK4000_FTYPE_TRANSPARENT, 0, "\x01\x00\x01\x00\x01\x00" );
		if( !( (ret==0) && (ci->sw[0]==0x90) && (ci->sw[1]==0x00) ) )
		{ printf(" Error.\n"); break; }
		printf(" OK.\n");

		/* Select EF 2001 */
		ret = scGpk4000CmdSelFil( ri, ci, SC_GPK4000_SELECT_EF,
			0x2001, NULL, 0, rbuffer, &resplen );
		if( !( (ret==0) && (ci->sw[0]==0x90) && (ci->sw[1]==0x00) ) )
		{ printf(" Error.\n"); break; }

/* UpdBinCrycks */
		/* UpdBinCrycks */
		printf("scGpk4000CmdUpdBinCrycks:");
		ret = scGpk4000CmdUpdBinCrycks( ri, ci, 1, update, sizeof(update) );
		if( !( (ret==0) && (ci->sw[0]==0x90) && (ci->sw[1]==0x00) ) )
		{ printf(" Error.\n"); break; }
		printf(" OK.\n");

/* WrBinCrycks */
		/* WrBinCrycks */
		printf("scGpk4000CmdWrBinCrycks:");
		ret = scGpk4000CmdWrBinCrycks( ri, ci, 2, write, sizeof(write) );
		if( !( (ret==0) && (ci->sw[0]==0x90) && (ci->sw[1]==0x00) ) )
		{ printf(" Error.\n"); break; }
		printf(" OK.\n");

/* RdBinCrycks */
		/* RdBinCrycks */
		printf("scGpk4000CmdRdBinCrycks:");
		resplen=8;
		ret = scGpk4000CmdRdBinCrycks( ri, ci, 2, rbuffer, &resplen );
		if( !( (ret==0) && (ci->sw[0]==0x90) && (ci->sw[1]==0x00) ) )
		{ printf(" Error.\n"); break; }
		printarray( " OK\n  rsp:", resplen, rbuffer );

		/* Select DF 2000 */
		ret = scGpk4000CmdSelFil( ri, ci, SC_GPK4000_SELECT_AID,
			0, "\xD2\x76\x00\x00\x92\x11\x11\x05\x00\x00\x01\x01\x00", 13,
			rbuffer, &resplen );
		if( !( (ret==0) && (ci->sw[0]==0x90) && (ci->sw[1]==0x00) ) )
		{ printf(" Error.\n"); break; }

		/* SelFk */
		chall[0]=0x00;chall[1]=0x01;chall[2]=0x02;chall[3]=0x03;
		chall[4]=0x04;chall[5]=0x05;chall[6]=0x06;chall[7]=0x07;
		printf("scGpk4000CmdSelFk:");
		ret = scGpk4000CmdSelFk( ri, ci, 0, TRUE, 0x01, admkey,
			chall );
		if( !( (ret==0) && (ci->sw[0]==0x90) && (ci->sw[1]==0x00) ) )
		{ printf(" Error.\n"); break; }

/* CrtFileCrycks */
		/* CrtFileCrycks */
		printf("scGpk4000CmdCrtFileCrycks(LF):");
		ret = scGpk4000CmdCrtFileCrycks( ri, ci, 0x2002, 100,
			SC_GPK4000_FTYPE_FIXED, 12, "\x01\x00\x01\x00\x01\x00" );
		if( !( (ret==0) && (ci->sw[0]==0x90) && (ci->sw[1]==0x00) ) )
		{ printf(" Error.\n"); break; }
		printf(" OK.\n");

/* ApdRecCrycks */
		/* ApdRecCrycks */
		printf("scGpk4000CmdApdRecCrycks(SFID):");
		ret = scGpk4000CmdApdRecCrycks( ri, ci, 2, authkey, 12 );
		if( !( (ret==0) && (ci->sw[0]==0x90) && (ci->sw[1]==0x00) ) )
		{ printf(" Error.\n"); break; }
		printf(" OK.\n");

		/* Select EF 2002 */
		ret = scGpk4000CmdSelFil( ri, ci, SC_GPK4000_SELECT_EF,
			0x2002, NULL, 0, rbuffer, &resplen );
		if( !( (ret==0) && (ci->sw[0]==0x90) && (ci->sw[1]==0x00) ) )
		{ printf(" Error.\n"); break; }

/* ApdRecCrycks */
		/* ApdRecCrycks */
		printf("scGpk4000CmdApdRecCrycks:");
		ret = scGpk4000CmdApdRecCrycks( ri, ci, 0, authkey, 12 );
		if( !( (ret==0) && (ci->sw[0]==0x90) && (ci->sw[1]==0x00) ) )
		{ printf(" Error.\n"); break; }
		printf(" OK.\n");

/* UpdRecCrycks */
		/* UpdRecCrycks */
		printf("scGpk4000CmdUpdRecCrycks:");
		ret = scGpk4000CmdUpdRecCrycks( ri, ci, 2, 0, authkey, 12 );
		if( !( (ret==0) && (ci->sw[0]==0x90) && (ci->sw[1]==0x00) ) )
		{ printf(" Error.\n"); break; }
		printf(" OK.\n");

/* RdRecCrycks */
		/* RdRecCrycks */
		printf("scGpk4000CmdRdRecCrycks:");
		resplen=10;
		ret = scGpk4000CmdRdRecCrycks( ri, ci, 1, 0, rbuffer, &resplen );
		if( !( (ret==0) && (ci->sw[0]==0x90) && (ci->sw[1]==0x00) ) )
		{ printf(" Error.\n"); break; }
		printarray( " OK\n  rsp:", resplen, rbuffer );

		/* Select DF 2000 */
		ret = scGpk4000CmdSelFil( ri, ci, SC_GPK4000_SELECT_AID,
			0, "\xD2\x76\x00\x00\x92\x11\x11\x05\x00\x00\x01\x01\x00", 13,
			rbuffer, &resplen );
		if( !( (ret==0) && (ci->sw[0]==0x90) && (ci->sw[1]==0x00) ) )
		{ printf(" Error.\n"); break; }

		/* SelFk */
		chall[0]=0x00;chall[1]=0x01;chall[2]=0x02;chall[3]=0x03;
		chall[4]=0x04;chall[5]=0x05;chall[6]=0x06;chall[7]=0x07;
		ret = scGpk4000CmdSelFk( ri, ci, 0, TRUE, 0x01, admkey, chall );
		if( !( (ret==0) && (ci->sw[0]==0x90) && (ci->sw[1]==0x00) ) )
		{ printf(" Error.\n"); break; }

/* FreezeAcCrycks */
		/* FreezeAcCrycks */
		printf("scGpk4000CmdFreezeAcCrycks:");
		resplen=10;
		ret = scGpk4000CmdFreezeAcCrycks( ri, ci, TRUE, 0x2001,
			"\x01\x01\x01" );
		if( !( (ret==0) && (ci->sw[0]==0x90) && (ci->sw[1]==0x00) ) )
		{ printf(" Error.\n"); break; }
		printf(" OK.\n");


/************************************************************************/

		/* Select MF */
		ret = scGpk4000CmdSelFil( ri, ci, SC_GPK4000_SELECT_MF,
			SC_GPK4000_FID_MF, NULL, 0, rbuffer, &resplen );
		if( !( (ret==0) && (ci->sw[0]==0x90) && (ci->sw[1]==0x00) ) )
		{ printf(" Error.\n"); break; }

		/* SelFk */
		chall[0]=0x00;chall[1]=0x01;chall[2]=0x02;chall[3]=0x03;
		chall[4]=0x04;chall[5]=0x05;chall[6]=0x06;chall[7]=0x07;
		ret = scGpk4000CmdSelFk( ri, ci, 0, TRUE, 0x01, admkey, chall );
		if( !( (ret==0) && (ci->sw[0]==0x90) && (ci->sw[1]==0x00) ) )
		{ printf(" Error.\n"); break; }

		/* CrtDirCrycks */
		ret = scGpk4000CmdCrtDirCrycks( ri, ci, 0x3000, NULL, 0,
			0x00, "\x00\x00\x00\x00" );
		if( !( (ret==0) && (ci->sw[0]==0x90) && (ci->sw[1]==0x00) ) )
		{ printf(" Error.\n"); break; }

		/* Select DF 3000 */
		ret = scGpk4000CmdSelFil( ri, ci, SC_GPK4000_SELECT_DF,
			0x3000, NULL, 0, rbuffer, &resplen );
		if( !( (ret==0) && (ci->sw[0]==0x90) && (ci->sw[1]==0x00) ) )
		{ printf(" Error.\n"); break; }

/* CrtFile */
		/* CrtFile */
		printf("scGpk4000CmdCrtFile(TR):");
		ret = scGpk4000CmdCrtFile( ri, ci, 0x3001, 100,
			SC_GPK4000_FTYPE_TRANSPARENT, 0, "\x00\x00\x00\x00\x00\x00" );
		if( !( (ret==0) && (ci->sw[0]==0x90) && (ci->sw[1]==0x00) ) )
			
		{ printf(" Error.\n"); break; }
		printf(" OK.\n");

		/* Select EF 3001 */
		ret = scGpk4000CmdSelFil( ri, ci, SC_GPK4000_SELECT_EF,
			0x3001, NULL, 0, rbuffer, &resplen );
		if( !( (ret==0) && (ci->sw[0]==0x90) && (ci->sw[1]==0x00) ) )
		{ printf(" Error.\n"); break; }

/* UpdBin */
		/* UpdBin */
		printf("scGpk4000CmdUpdBin:");
		ret = scGpk4000CmdUpdBin( ri, ci, 1, update, sizeof(update) );
		if( !( (ret==0) && (ci->sw[0]==0x90) && (ci->sw[1]==0x00) ) )
		{ printf(" Error.\n"); break; }
		printf(" OK.\n");

/* WrBin */
		/* WrBin */
		printf("scGpk4000CmdWrBin:");
		ret = scGpk4000CmdWrBin( ri, ci, 2, write, sizeof(write) );
		if( !( (ret==0) && (ci->sw[0]==0x90) && (ci->sw[1]==0x00) ) )
		{ printf(" Error.\n"); break; }
		printf(" OK.\n");

/* RdBin */
		/* RdBin */
		printf("scGpk4000CmdRdBin:");
		resplen=8;
		ret = scGpk4000CmdRdBin( ri, ci, 2, rbuffer, &resplen );
		if( !( (ret==0) && (ci->sw[0]==0x90) && (ci->sw[1]==0x00) ) )
		{ printf(" Error.\n"); break; }
		printarray( " OK\n  rsp:", resplen, rbuffer );

		/* Select MF */
		ret = scGpk4000CmdSelFil( ri, ci, SC_GPK4000_SELECT_MF,
			SC_GPK4000_FID_MF, NULL, 0, rbuffer, &resplen );
		if( !( (ret==0) && (ci->sw[0]==0x90) && (ci->sw[1]==0x00) ) )
		{ printf(" Error.\n"); break; }

		/* Select DF 3000 */
		ret = scGpk4000CmdSelFil( ri, ci, SC_GPK4000_SELECT_DF,
			0x3000, NULL, 0, rbuffer, &resplen );
		if( !( (ret==0) && (ci->sw[0]==0x90) && (ci->sw[1]==0x00) ) )
		{ printf(" Error.\n"); break; }

/* CrtFile */
		/* CrtFile */
		printf("scGpk4000CmdCrtFile(LF):");
		ret = scGpk4000CmdCrtFile( ri, ci, 0x3002, 100,
			SC_GPK4000_FTYPE_FIXED, 12, "\x00\x00\x00\x00\x00\x00" );
		if( !( (ret==0) && (ci->sw[0]==0x90) && (ci->sw[1]==0x00) ) )
		{ printf(" Error.\n"); break; }
		printf(" OK.\n");

/* ApdRec */
		/* ApdRec */
		printf("scGpk4000CmdApdRec(SFID):");
		ret = scGpk4000CmdApdRec( ri, ci, 2, authkey, 12 );
		if( !( (ret==0) && (ci->sw[0]==0x90) && (ci->sw[1]==0x00) ) )
		{ printf(" Error.\n"); break; }
		printf(" OK.\n");

		/* Select EF 3002 */
		ret = scGpk4000CmdSelFil( ri, ci, SC_GPK4000_SELECT_EF,
			0x3002, NULL, 0, rbuffer, &resplen );
		if( !( (ret==0) && (ci->sw[0]==0x90) && (ci->sw[1]==0x00) ) )
		{ printf(" Error.\n"); break; }

/* ApdRec */
		/* ApdRec */
		printf("scGpk4000CmdApdRec:");
		ret = scGpk4000CmdApdRec( ri, ci, 0, authkey, 12 );
		if( !( (ret==0) && (ci->sw[0]==0x90) && (ci->sw[1]==0x00) ) )
		{ printf(" Error.\n"); break; }
		printf(" OK.\n");

/* UpdRec */
		/* UpdRec */
		printf("scGpk4000CmdUpdRec:");
		ret = scGpk4000CmdUpdRec( ri, ci, 2, 0, authkey, 12 );
		if( !( (ret==0) && (ci->sw[0]==0x90) && (ci->sw[1]==0x00) ) )
		{ printf(" Error.\n"); break; }
		printf(" OK.\n");

/* RdRec */
		/* RdRec */
		printf("scGpk4000CmdRdRec:");
		resplen=10;
		ret = scGpk4000CmdRdRec( ri, ci, 1, 0, rbuffer, &resplen );
		if( !( (ret==0) && (ci->sw[0]==0x90) && (ci->sw[1]==0x00) ) )
		{ printf(" Error.\n"); break; }
		printarray( " OK\n  rsp:", resplen, rbuffer );

		/* Select MF */
		ret = scGpk4000CmdSelFil( ri, ci, SC_GPK4000_SELECT_MF,
			SC_GPK4000_FID_MF, NULL, 0, rbuffer, &resplen );
		if( !( (ret==0) && (ci->sw[0]==0x90) && (ci->sw[1]==0x00) ) )
		{ printf(" Error.\n"); break; }

		/* Select DF 3000 */
		ret = scGpk4000CmdSelFil( ri, ci, SC_GPK4000_SELECT_DF,
			0x3000, NULL, 0, rbuffer, &resplen );
		if( !( (ret==0) && (ci->sw[0]==0x90) && (ci->sw[1]==0x00) ) )
		{ printf(" Error.\n"); break; }

/* FreezeAc */
		/* FreezeAc */
		printf("scGpk4000CmdFreezeAc:");
		ret = scGpk4000CmdFreezeAc( ri, ci, TRUE, 0x3001, "\x01\x01\x01" );
		if( !( (ret==0) && (ci->sw[0]==0x90) && (ci->sw[1]==0x00) ) )
		{ printf(" Error.\n"); break; }
		printf(" OK.\n");

/************************************************************************/

		/* CrtFile */
		ret = scGpk4000CmdCrtFile( ri, ci, 0x3003, 48,
			SC_GPK4000_FTYPE_DES_KEY, 0, "\x01\x00\x01\x00\x01\x00" );
		if( !( (ret==0) && (ci->sw[0]==0x90) && (ci->sw[1]==0x00) ) )
		{ printf(" Error.\n"); break; }
		printf(" OK.\n");

		/* Select EF 3003 */
		ret = scGpk4000CmdSelFil( ri, ci, SC_GPK4000_SELECT_EF,
			0x3003, NULL, 0, rbuffer, &resplen );
		if( !( (ret==0) && (ci->sw[0]==0x90) && (ci->sw[1]==0x00) ) )
		{ printf(" Error.\n"); break; }

		/* SelFk */
		chall[0]=0x00;chall[1]=0x01;chall[2]=0x02;chall[3]=0x03;
		chall[4]=0x04;chall[5]=0x05;chall[6]=0x06;chall[7]=0x07;
		ret = scGpk4000CmdSelFk( ri, ci, 0, TRUE, 0x01, admkey, chall );
		if( !( (ret==0) && (ci->sw[0]==0x90) && (ci->sw[1]==0x00) ) )
		{ printf(" Error.\n"); break; }

		printf("scGpk4000CmdUpdBinEncr(DES):");
		ret = scGpk4000CmdUpdBinEncr( ri, ci, 0, keyfile );
		if( !( (ret==0) && (ci->sw[0]==0x90) && (ci->sw[1]==0x00) ) )
		{ printf(" Error.\n"); break; }
		printf(" OK.\n");

		ret = scGpk4000CmdUpdBinEncr( ri, ci, 3, keyfile+12 );
		if( !( (ret==0) && (ci->sw[0]==0x90) && (ci->sw[1]==0x00) ) )
		{ printf(" Error.\n"); break; }

		ret = scGpk4000CmdUpdBinEncr( ri, ci, 6, keyfile+24 );
		if( !( (ret==0) && (ci->sw[0]==0x90) && (ci->sw[1]==0x00) ) )
		{ printf(" Error.\n"); break; }

		ret = scGpk4000CmdUpdBinEncr( ri, ci, 9, keyfile+36 );
		if( !( (ret==0) && (ci->sw[0]==0x90) && (ci->sw[1]==0x00) ) )
		{ printf(" Error.\n"); break; }

		printf("scGpk4000CmdWrBinEncr:");
		ret = scGpk4000CmdWrBinEncr( ri, ci, 0, keyfile );
		if( !( (ret==0) && (ci->sw[0]==0x90) && (ci->sw[1]==0x00) ) )
		{ printf(" Error.\n"); break; }
		printf(" OK.\n");

/* Get Chal */
		/* Get Chal */
		printf("scGpk4000CmdGetChall:");
		ret = scGpk4000CmdGetChall( ri, ci, chall, FALSE );
		if( !( (ret==0) && (ci->sw[0]==0x90) && (ci->sw[1]==0x00) ) )
		{ printf(" Error.\n"); break; }
		printarray( " OK\n  rsp:", 8, chall );

/* Ext Auth */
		/* Ext Auth */
		printf("scGpk4000CmdExtAuth:");
		ret = scGpk4000CmdExtAuth( ri, ci, 2, FALSE, 0x03, authkey,
			chall );
		if( !( (ret==0) && (ci->sw[0]==0x90) && (ci->sw[1]==0x00) ) )
		{ printf(" Error.\n"); break; }
		printf(" OK.\n");

/* Int Auth */
		/* Int Auth */
		printf("scGpk4000CmdIntAuth:");
		chall[0]=0x00;chall[1]=0x01;chall[2]=0x02;chall[3]=0x03;
		chall[4]=0x04;chall[5]=0x05;chall[6]=0x06;chall[7]=0x07;
		ret = scGpk4000CmdIntAuth( ri, ci, 2, FALSE, 0x03, authkey, chall );
		if( !( (ret==0) && (ci->sw[0]==0x90) && (ci->sw[1]==0x00) ) )
		{ printf(" Error.\n"); break; }
		printf(" OK.\n");

		/* Select MF */
		ret = scGpk4000CmdSelFil( ri, ci, SC_GPK4000_SELECT_MF,
			SC_GPK4000_FID_MF, NULL, 0, rbuffer, &resplen );
		if( !( (ret==0) && (ci->sw[0]==0x90) && (ci->sw[1]==0x00) ) )
		{ printf(" Error.\n"); break; }

		/* Select DF 3000 */
		ret = scGpk4000CmdSelFil( ri, ci, SC_GPK4000_SELECT_DF,
			0x3000, NULL, 0, rbuffer, &resplen );
		if( !( (ret==0) && (ci->sw[0]==0x90) && (ci->sw[1]==0x00) ) )
		{ printf(" Error.\n"); break; }

		/* CrtFile */
		ret = scGpk4000CmdCrtFile( ri, ci, 0x3004, 16,
			SC_GPK4000_FTYPE_SECRET_CODE, 0, "\x01\x00\x01\x00\x01\x00" );
		if( !( (ret==0) && (ci->sw[0]==0x90) && (ci->sw[1]==0x00) ) )
		{ printf(" Error.\n"); break; }
		printf(" OK.\n");

		/* Select EF 3004 */
		ret = scGpk4000CmdSelFil( ri, ci, SC_GPK4000_SELECT_EF,
			0x3004, NULL, 0, rbuffer, &resplen );
		if( !( (ret==0) && (ci->sw[0]==0x90) && (ci->sw[1]==0x00) ) )
		{ printf(" Error.\n"); break; }

		/* SelFk */
		chall[0]=0x00;chall[1]=0x01;chall[2]=0x02;chall[3]=0x03;
		chall[4]=0x04;chall[5]=0x05;chall[6]=0x06;chall[7]=0x07;
		ret = scGpk4000CmdSelFk( ri, ci, 0, TRUE, 0x01, admkey,
			chall );
		if( !( (ret==0) && (ci->sw[0]==0x90) && (ci->sw[1]==0x00) ) )
		{ printf(" Error.\n"); break; }

		printf("scGpk4000CmdUpdBinEncr(SC):");
		ret = scGpk4000CmdUpdBinEncr( ri, ci, 0, pinfile );
		if( !( (ret==0) && (ci->sw[0]==0x90) && (ci->sw[1]==0x00) ) )
		{ printf(" Error.\n"); break; }
		printf(" OK.\n");

		ret = scGpk4000CmdUpdBinEncr( ri, ci, 2, pinfile+12 );
		if( !( (ret==0) && (ci->sw[0]==0x90) && (ci->sw[1]==0x00) ) )
		{ printf(" Error.\n"); break; }

/* Verify */
		/* Verify */
		printf("scGpk4000CmdVerify:");
		ret = scGpk4000CmdVerify( ri, ci, 0, pin );
		if( !( (ret==0) && (ci->sw[0]==0x90) && (ci->sw[1]==0x00) ) )
		{ printf(" Error.\n"); break; }
		printf(" OK.\n");

/* Verify Encr */
		/* Verify Encr */
		printf("scGpk4000CmdVerifyEncr:");
		ret = scGpk4000CmdVerifyEncr( ri, ci, 1, unlock );
		if( !( (ret==0) && (ci->sw[0]==0x90) && (ci->sw[1]==0x00) ) )
		{ printf(" Error.\n"); break; }
		printf(" OK.\n");

/* Set Code */
		/* Set Code */
		printf("scGpk4000CmdSetCod:");
		ret = scGpk4000CmdSetCod( ri, ci, FALSE, 0, pinfile+4, pinfile+16 );
		if( !( (ret==0) && (ci->sw[0]==0x90) && (ci->sw[1]==0x00) ) )
		{ printf(" Error.\n"); break; }
		printf(" OK.\n");

/* Set Code Crycks */
		/* Set Code Crycks */
		printf("scGpk4000CmdSetCodCrycks:");
		ret = scGpk4000CmdSetCodCrycks( ri, ci, FALSE, 1, pinfile+16,
			pinfile+4 );
		if( !( (ret==0) && (ci->sw[0]==0x90) && (ci->sw[1]==0x00) ) )
		{ printf(" Error.\n"); break; }
		printf(" OK.\n");

		/* Verify */
		printf("scGpk4000CmdVerify(Wrong):");
		ret = scGpk4000CmdVerify( ri, ci, 0, pin );
		if( !( (ret==0) && (ci->sw[0]==0x63) ) )
		{ printf(" Error.\n"); break; }

		/* Verify */
		ret = scGpk4000CmdVerify( ri, ci, 0, pin );
		if( !( (ret==0) && (ci->sw[0]==0x63) ) )
		{ printf(" Error.\n"); break; }

		/* Verify */
		ret = scGpk4000CmdVerify( ri, ci, 0, pin );
		if( !( (ret==0) && (ci->sw[0]==0x63) ) )
		{ printf(" Error.\n"); break; }

		/* Verify */
		ret = scGpk4000CmdVerify( ri, ci, 0, pin );
		if( !( (ret==0) && (ci->sw[0]==0x69) ) )
		{ printf(" Error.\n"); break; }
		printf(" OK.\n");

/******************** purse commands *******************************/

		/* Select MF */
		ret = scGpk4000CmdSelFil( ri, ci, SC_GPK4000_SELECT_MF,
			SC_GPK4000_FID_MF, NULL, 0, rbuffer, &resplen );
		if( !( (ret==0) && (ci->sw[0]==0x90) && (ci->sw[1]==0x00) ) )
		{ printf(" Error.\n"); break; }

		/* SelFk */
		chall[0]=0x00;chall[1]=0x01;chall[2]=0x02;chall[3]=0x03;
		chall[4]=0x04;chall[5]=0x05;chall[6]=0x06;chall[7]=0x07;
		ret = scGpk4000CmdSelFk( ri, ci, 0, TRUE, 0x01, admkey, chall );
		if( !( (ret==0) && (ci->sw[0]==0x90) && (ci->sw[1]==0x00) ) )
		{ printf(" Error.\n"); break; }

		/* CrtDirCrycks */
		ret = scGpk4000CmdCrtDirCrycks( ri, ci, 0,
			"\xD2\x76\x00\x00\x92\x11\x11\x05\x00\x00\x02\x01\x00", 13,
			0x00, "\x00\x00\x00\x00" );
		if( !( (ret==0) && (ci->sw[0]==0x90) && (ci->sw[1]==0x00) ) )
		{ printf(" Error.\n"); break; }

		/* Select DF 4000 */
		ret = scGpk4000CmdSelFil( ri, ci, SC_GPK4000_SELECT_AID,
			0, "\xD2\x76\x00\x00\x92\x11\x11\x05\x00\x00\x02\x01\x00", 13,
			rbuffer, &resplen );
		if( !( (ret==0) && (ci->sw[0]==0x90) && (ci->sw[1]==0x00) ) )
		{ printf(" Error.\n"); break; }

		/* CrtFile */
		ret = scGpk4000CmdCrtFile( ri, ci, 0x4001, 48,
			SC_GPK4000_FTYPE_DES_KEY, 0, "\x01\x00\x01\x00\x01\x00" );
		if( !( (ret==0) && (ci->sw[0]==0x90) && (ci->sw[1]==0x00) ) )
		{ printf(" Error.\n"); break; }

		/* Select EF 4001 */
		ret = scGpk4000CmdSelFil( ri, ci, SC_GPK4000_SELECT_EF,
			0x4001, NULL, 0, rbuffer, &resplen );
		if( !( (ret==0) && (ci->sw[0]==0x90) && (ci->sw[1]==0x00) ) )
		{ printf(" Error.\n"); break; }

		ret = scGpk4000CmdUpdBinEncr( ri, ci, 0, keyfile );
		if( !( (ret==0) && (ci->sw[0]==0x90) && (ci->sw[1]==0x00) ) )
		{ printf(" Error.\n"); break; }

		ret = scGpk4000CmdUpdBinEncr( ri, ci, 3, keyfile+12 );
		if( !( (ret==0) && (ci->sw[0]==0x90) && (ci->sw[1]==0x00) ) )
		{ printf(" Error.\n"); break; }

		/* Select DF 4000 */
		ret = scGpk4000CmdSelFil( ri, ci, SC_GPK4000_SELECT_AID,
			0, "\xD2\x76\x00\x00\x92\x11\x11\x05\x00\x00\x02\x01\x00", 13,
			rbuffer, &resplen );
		if( !( (ret==0) && (ci->sw[0]==0x90) && (ci->sw[1]==0x00) ) )
		{ printf(" Error.\n"); break; }

		/* SelFk */
		chall[0]=0x00;chall[1]=0x01;chall[2]=0x02;chall[3]=0x03;
		chall[4]=0x04;chall[5]=0x05;chall[6]=0x06;chall[7]=0x07;
		ret = scGpk4000CmdSelFk( ri, ci, 0, TRUE, 0x01, admkey, chall );
		if( !( (ret==0) && (ci->sw[0]==0x90) && (ci->sw[1]==0x00) ) )
		{ printf(" Error.\n"); break; }

		/* CrtFile */
		ret = scGpk4000CmdCrtFile( ri, ci, 0x4002, 20,
			SC_GPK4000_FTYPE_PURSE, 0, "\x00\x00\x00\x00\x00\x00" );
		if( !( (ret==0) && (ci->sw[0]==0x90) && (ci->sw[1]==0x00) ) )
		{ printf(" Error.\n"); break; }

		/* Select EF 4002 */
		ret = scGpk4000CmdSelFil( ri, ci, SC_GPK4000_SELECT_EF,
			0x4002, NULL, 0, rbuffer, &resplen );
		if( !( (ret==0) && (ci->sw[0]==0x90) && (ci->sw[1]==0x00) ) )
		{ printf(" Error.\n"); break; }

		ret = scGpk4000CmdUpdBin( ri, ci, 0, purse, sizeof(purse) );
		if( !( (ret==0) && (ci->sw[0]==0x90) && (ci->sw[1]==0x00) ) )
		{ printf(" Error.\n"); break; }

		/* Select DF 4000 */
		ret = scGpk4000CmdSelFil( ri, ci, SC_GPK4000_SELECT_AID,
			0, "\xD2\x76\x00\x00\x92\x11\x11\x05\x00\x00\x02\x01\x00", 13,
			rbuffer, &resplen );
		if( !( (ret==0) && (ci->sw[0]==0x90) && (ci->sw[1]==0x00) ) )
		{ printf(" Error.\n"); break; }

		/* SelFk */
		chall[0]=0x00;chall[1]=0x01;chall[2]=0x02;chall[3]=0x03;
		chall[4]=0x04;chall[5]=0x05;chall[6]=0x06;chall[7]=0x07;
		ret = scGpk4000CmdSelFk( ri, ci, 0, TRUE, 0x01, admkey, chall );
		if( !( (ret==0) && (ci->sw[0]==0x90) && (ci->sw[1]==0x00) ) )
		{ printf(" Error.\n"); break; }

		/* CrtFile */
		ret = scGpk4000CmdCrtFile( ri, ci, 0x4003, 8,
			SC_GPK4000_FTYPE_TRANSACTION, 0, "\x00\x00\x00\x00\x00\x00" );
		if( !( (ret==0) && (ci->sw[0]==0x90) && (ci->sw[1]==0x00) ) )
		{ printf(" Error.\n"); break; }

		/* Select EF 4003 */
		ret = scGpk4000CmdSelFil( ri, ci, SC_GPK4000_SELECT_EF,
			0x4003, NULL, 0, rbuffer, &resplen );
		if( !( (ret==0) && (ci->sw[0]==0x90) && (ci->sw[1]==0x00) ) )
		{ printf(" Error.\n"); break; }

		ret = scGpk4000CmdUpdBin( ri, ci, 0, tm, sizeof(tm) );
		if( !( (ret==0) && (ci->sw[0]==0x90) && (ci->sw[1]==0x00) ) )
		{ printf(" Error.\n"); break; }

		/* Select DF 4000 */
		ret = scGpk4000CmdSelFil( ri, ci, SC_GPK4000_SELECT_AID,
			0, "\xD2\x76\x00\x00\x92\x11\x11\x05\x00\x00\x02\x01\x00", 13,
			rbuffer, &resplen );
		if( !( (ret==0) && (ci->sw[0]==0x90) && (ci->sw[1]==0x00) ) )
		{ printf(" Error.\n"); break; }

/* SelPk */
		/* SelPk */
		chall[0]=0x00;chall[1]=0x01;chall[2]=0x02;chall[3]=0x03;
		chall[4]=0x04;chall[5]=0x05;chall[6]=0x06;chall[7]=0x07;
		printf("scGpk4000CmdSelPk:");
		ret = scGpk4000CmdSelPk( ri, ci, 0, 0x01, 0x02, paykey, chall, ctc );
		if( !( (ret==0) && (ci->sw[0]==0x90) && (ci->sw[1]==0x00) ) )
		{ printf(" Error.\n"); break; }
		printf(" OK.\n");

/* Set Opts */
		/* Set Opts */
		printf("scGpk4000CmdSetOpts:");
		ret = scGpk4000CmdSetOpts( ri, ci, 0 );
		if( !( (ret==0) && (ci->sw[0]==0x90) && (ci->sw[1]==0x00) ) )
		{ printf(" Error.\n"); break; }
		printf(" OK.\n");

/* Credit */
		/* Credit */
		printf("scGpk4000CmdCredit:");
		ret = scGpk4000CmdCredit( ri, ci, 0, paykey, ctc, "\x00\x00\xEE",
			2 );
		if( !( (ret==0) && (ci->sw[0]==0x90) && (ci->sw[1]==0x00) ) )
		{ printf(" Error.\n"); break; }
		printf(" OK.\n");

/* Read Balance */
		/* RdBal */
		printf("scGpk4000CmdRdBal:");
		ret = scGpk4000CmdRdBal( ri, ci, 2, "\x00\x01\x02\x03", rbuffer );
		if( !( (ret==0) && (ci->sw[0]==0x90) && (ci->sw[1]==0x00) ) )
		{ printf(" Error.\n"); break; }
		printf(" OK.\n");
		printarray( "  rsp:", 3, rbuffer );

		/* SelPk */
		chall[0]=0x00;chall[1]=0x01;chall[2]=0x02;chall[3]=0x03;
		chall[4]=0x04;chall[5]=0x05;chall[6]=0x06;chall[7]=0x07;
		ret = scGpk4000CmdSelPk( ri, ci, 0, 0x01, 0x02, paykey, chall, ctc );
		if( !( (ret==0) && (ci->sw[0]==0x90) && (ci->sw[1]==0x00) ) )
		{ printf(" Error.\n"); break; }

/* Debit */
		/* Debit */
		printf("scGpk4000CmdDebit:");
		ret = scGpk4000CmdDebit( ri, ci, 2, "\x00\x00\x11",
			"\x00\x01\x02\x04" );
		if( !( (ret==0) && (ci->sw[0]==0x90) && (ci->sw[1]==0x00) ) )
		{ printf(" Error.\n"); break; }
		printf(" OK.\n");

/* Sign */
		/* Sign */
		printf("scGpk4000CmdSign:");
		ret = scGpk4000CmdSign( ri, ci, 0, 1, rbuffer, &resplen );
		if( !( (ret==0) && (ci->sw[0]==0x90) && (ci->sw[1]==0x00) ) )
		{ printf(" Error.\n"); break; }
		printf(" OK.\n");
		printarray( "  rsp:", resplen, rbuffer );

		/* SelPk */
		chall[0]=0x00;chall[1]=0x01;chall[2]=0x02;chall[3]=0x03;
		chall[4]=0x04;chall[5]=0x05;chall[6]=0x06;chall[7]=0x07;
		ret = scGpk4000CmdSelPk( ri, ci, 0, 0x01, 0x02, paykey, chall, ctc );
		if( !( (ret==0) && (ci->sw[0]==0x90) && (ci->sw[1]==0x00) ) )
		{ printf(" Error.\n"); break; }

		ret = scGpk4000CmdDebit( ri, ci, 2, "\x00\x00\x11",
			"\x00\x01\x02\x04" );
		if( !( (ret==0) && (ci->sw[0]==0x90) && (ci->sw[1]==0x00) ) )
		{ printf(" Error.\n"); break; }

/* Cancel Debit */
		/* Cancel Debit */
		printf("scGpk4000CmdCanDeb:");
		ret = scGpk4000CmdCanDeb( ri, ci, "\x00\x00\x11", "\x00\x00\x22",
			"\x00\x01\x02\x05", rbuffer, &resplen );
		if( !( (ret==0) && (ci->sw[0]==0x90) && (ci->sw[1]==0x00) ) )
		{ printf(" Error.\n"); break; }
		printf(" OK.\n");
		printarray( "  rsp:", resplen, rbuffer );

		ret = scGpk4000CmdEraseCard( ri, ci, 7 );
		if( !( (ret==0) && (ci->sw[0]==0x90) && (ci->sw[1]==0x00) ) )
		{ printf(" Error.\n"); break; }

#endif /* WITH_DES */

/* printf("[ret: %d, sw: %.2X%.2X, s: %d]",ret,ci->sw[0],ci->sw[1]); */
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

