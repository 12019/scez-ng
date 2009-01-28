/* $Id: tmultiflex.c 1617 2005-11-03 17:41:39Z laforge $ */

/*
 * Testfile for all Multiflex commands.
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#if 0
#include <sio/sio.h>
#endif
#include <scez/scgeneral.h>
#include <scez/scsmartcard.h>
#include <scez/screader.h>
#include <scez/cards/scmultiflex.h>

#define WITH_MULTIFLEX8K
/* #define CLEAN_CC3 */

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
	BYTE authkey[]={0x47,0x46,0x58,0x49,0x32,0x56,0x78,0x40};
	/* Standard Schlumberger key: GFXI2Vx@
	 * BYTE authkey[]={0x47,0x46,0x58,0x49,0x32,0x56,0x78,0x40};
	 * Aladdin CC3 key:
	 * BYTE authkey[]={0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF};
	 */
#ifdef WITH_DES
	BYTE testkey[]={0x47,0x46,0x58,0x49,0x32,0x56,0x78,0x40};
#endif /* WITH_DES */
	BYTE pinfile[]={0x01,0xFF,0xFF,0x12,0x34,0x56,0x78,0xFF,0xFF,0xFF,0xFF,
		0x01,0x01,0xAA,0xBB,0xCC,0xDD,0xFF,0xFF,0xFF,0xFF,0x01,0x01};
	BYTE oldpin[]={0x12,0x34,0x56,0x78,0xFF,0xFF,0xFF,0xFF};
	BYTE unblockpin[]={0xAA,0xBB,0xCC,0xDD,0xFF,0xFF,0xFF,0xFF};
	BYTE newpin[]={0x9A,0xBC,0xDE,0x01,0xFF,0xFF,0xFF,0xFF};
	BYTE intauth[]={0xFF,0x08,0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,
		0xFF,0xFF,0x00};

#ifdef WITH_DES
	BYTE challenge[]={0x64,0x46,0x27,0xE0,0x07,0x9D,0xD8,0x6C};
	BYTE result[]={0x0D,0x31,0xA8,0xF3,0x1C,0xEF};
#endif /* WITH_DES */
	BYTE mac[8];

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

	/* Get Card Status */
	ret = scReaderCardStatus( ri );
	checkreturn("Error: scReader\n");
	if( !(ri->status&SC_CARD_STATUS_PRESENT) )
	{ printf("Error: No Card.\n"); goto exit; }

	/* Activate Card */
	ret = scReaderActivate( ri );
	checkreturn("Error: scReaderActivate\n");

	/* Reset Card */
	ret= scReaderResetCard( ri, ci );
	checkreturn("Error: scReaderResetCard\n");

	/* Get Card Type */
	ret = scSmartcardGetCardType( ci );
	checkreturn("Error: scReaderGetCardType\n");
	if( (ci->type!=SC_CARD_MULTIFLEX_3K) && (ci->type!=SC_CARD_MULTIFLEX_8K) &&
		(ci->type!=SC_CARD_MULTIFLEX_8K_DES) )
	{ printf("Error: Wrong Card.\n"); goto exit; }

	/* Set Speed */
	ret = scSmartcardSetSpeed( ri, ci, SC_SPEED_FASTEST );
	if( ret!=SC_EXIT_OK ) {
		/* Reset Card */
		ret= scReaderResetCard( ri, ci );
		checkreturn("Error: scReaderResetCard\n");
	}

#if 0
	SIO_SetLogFile( ri->si, "LogMultiflex.txt" );
#endif

/* Select */

	/* Select MF */
	printf("scMultiflexCmdSelectFile:");
	ret = scMultiflexCmdSelectFile( ri, ci, 0x3F00, rbuffer, &resplen );
	if( (ret==0) && (ci->sw[0]==0x90) && (ci->sw[1]==0x00) )
	{ printf(" OK\n"); } else
	{ printf(" Error.\n"); goto exit; }
	printarray( "  rsp:", resplen, rbuffer );

/* Verify Key */

	/* Verify transport key */
	printf("scMultiflexCmdVerifyKey:");
	ret = scMultiflexCmdVerifyKey( ri, ci, 1, authkey, 8 );
	if( (ret==0) && (ci->sw[0]==0x90) && (ci->sw[1]==0x00) )
	{ printf(" OK\n"); } else
	{ printf(" Error. (ret: %d: %.2X%.2X)\n",ret,ci->sw[0],ci->sw[1]); goto exit; }

#ifdef WITH_MULTIFLEX8K

/* Dir */

	/* Dir */
	printf("scMultiflexCmdDirectory:");
	ret = scMultiflexCmdDirectory( ri, ci, rbuffer, &resplen );
	if( (ret==0) && (ci->sw[0]==0x90) && (ci->sw[1]==0x00) )
	{ printf(" OK\n"); } else
	{ printf(" Error.\n"); goto exit; }
	printarray( "  rsp:", resplen, rbuffer );

#endif /* WITH_MULTIFLEX8K */

	/* Delete directory file 2000 */
	ret = scMultiflexCmdDeleteFile( ri, ci, 0x2000 );
	if(ret!=0) { printf(" Error deleting 2000.\n"); goto exit; }

#ifdef CLEAN_CC3
	/* Delete directory file 0001 */
	ret = scMultiflexCmdDeleteFile( ri, ci, 0x0001 );
	if(ret!=0) { printf(" Error deleting 0001.\n"); goto exit; }
	/* Delete directory file 0000 */
	ret = scMultiflexCmdDeleteFile( ri, ci, 0x0000 );
	if(ret!=0) { printf(" Error deleting 0000.\n"); goto exit; }
#endif /* CLEAN_CC3 */

/* Create */

	/* Create Directory 2000 */
	acond[0]=0xFF; acond[1]=0x4F; acond[2]=0x44; acond[3]=0x44;
	akeys[0]=0x1F; akeys[1]=0x11; akeys[2]=0x11;
	printf("scMultiflexCmdCreateFile(Directory File):");
	ret = scMultiflexCmdCreateFile( ri, ci, 0x2000,
		200, 0x38, 0x00, 1, 0, 0, acond, akeys );
	if( (ret==0) && (ci->sw[0]==0x90) && (ci->sw[1]==0x00) )
	{ printf(" OK\n"); } else
	{ printf(" Error.\n"); goto exit; }

	/* Select DF */
	ret = scMultiflexCmdSelectFile( ri, ci, 0x2000, rbuffer, &resplen );
	if( (ret==0) && (ci->sw[0]==0x90) && (ci->sw[1]==0x00) )
	{ } else
	{ printf(" Error.\n"); goto exit; }
	printarray( "  rsp:", resplen, rbuffer );

/* Create fixed-record files 2001 */

	/* Create fixed-record file 2001 */
	acond[0]=0x3F; acond[1]=0x44; acond[2]=0xF4; acond[3]=0x44;
	akeys[0]=0x11; akeys[1]=0xF1; akeys[2]=0x11;
	printf("scMultiflexCmdCreateFile(Fixed-Record File):");
	ret = scMultiflexCmdCreateFile( ri, ci, 0x2001,
		36, SC_MULTIFLEX_FILE_FIXED, 0x00, 1, 6, 5, acond, akeys );
	if( (ret==0) && (ci->sw[0]==0x90) && (ci->sw[1]==0x00) )
	{ printf(" OK\n"); } else
	{ printf(" Error.\n"); goto exit; }

	/* Select EF */
	ret = scMultiflexCmdSelectFile( ri, ci, 0x2001, rbuffer, &resplen );
	if( (ret==0) && (ci->sw[0]==0x90) && (ci->sw[1]==0x00) )
	{ } else
	{ printf(" Error.\n"); goto exit; }
	printarray( "  rsp:", resplen, rbuffer );

/* Update Record */

	/* Update Record */
	printf("scMultiflexCmdUpdateRecord:");
	ret = scMultiflexCmdUpdateRecord( ri, ci, 0, 0, "Kleo  ", 6 );
	if( (ret==0) && (ci->sw[0]==0x90) && (ci->sw[1]==0x00) )
	{ printf(" OK\n"); } else
	{ printf(" Error.\n"); goto exit; }
	ret = scMultiflexCmdUpdateRecord( ri, ci, 0, 2, "Sonja ", 6 );
	if( (ret==0) && (ci->sw[0]==0x90) && (ci->sw[1]==0x00) )
	{ } else
	{ printf(" Error.\n"); goto exit; }
	ret = scMultiflexCmdUpdateRecord( ri, ci, 0, 2, "Madam ", 6 );
	if( (ret==0) && (ci->sw[0]==0x90) && (ci->sw[1]==0x00) )
	{ } else
	{ printf(" Error.\n"); goto exit; }
	ret = scMultiflexCmdUpdateRecord( ri, ci, 0, 2, "Isabel", 6 );
	if( (ret==0) && (ci->sw[0]==0x90) && (ci->sw[1]==0x00) )
	{ } else
	{ printf(" Error.\n"); goto exit; }
	ret = scMultiflexCmdUpdateRecord( ri, ci, 0, 2, "Gina  ", 6 );
	if( (ret==0) && (ci->sw[0]==0x90) && (ci->sw[1]==0x00) )
	{ } else
	{ printf(" Error.\n"); goto exit; }

/* Create Record */

	/* Create Record */
	printf("scMultiflexCmdCreateRecord:");
	ret = scMultiflexCmdCreateRecord( ri, ci, "Valeri", 6 );
	if( (ret==0) && (ci->sw[0]==0x90) && (ci->sw[1]==0x00) )
	{ printf(" OK\n"); } else
	{ printf(" Error.\n"); goto exit; }

/* Seek */

	/* Seek */
	printf("scMultiflexCmdSeek:");
	ret = scMultiflexCmdSeek( ri, ci, 0, 0, "M", 1 );
	if( (ret==0) && (ci->sw[0]==0x90) && (ci->sw[1]==0x00) )
	{ printf(" OK\n"); } else
	{ printf(" Error.\n"); goto exit; }

/* Read Record */

	/* Read Record */
	printf("scMultiflexCmdReadRecord");
	resplen=6;
	ret = scMultiflexCmdReadRecord( ri, ci, 0, 4, buffer, &resplen );
	if( (ret==0) && (ci->sw[0]==0x90) && (ci->sw[1]==0x00) )
	{ printf(" OK\n"); } else
	{ printf(" Error.\n"); goto exit; }
	printarray( "  rsp:", resplen, buffer );

	/* Select DF */
	ret = scMultiflexCmdSelectFile( ri, ci, 0x2000, rbuffer, &resplen );
	if( (ret==0) && (ci->sw[0]==0x90) && (ci->sw[1]==0x00) )
	{ } else
	{ printf(" Error.\n"); goto exit; }

/* Create transparent file */

	/* Create transparent file 0000 */
	acond[0]=0x3F; acond[1]=0x44; acond[2]=0xFF; acond[3]=0x44;
	akeys[0]=0x11; akeys[1]=0xFF; akeys[2]=0x11;
	printf("scMultiflexCmdCreateFile(Transparent File):");
	ret = scMultiflexCmdCreateFile( ri, ci, 0x0000,
		23, 1, 0x00, 1, 0, 0, acond, akeys );
	if( (ret==0) && (ci->sw[0]==0x90) && (ci->sw[1]==0x00) )
	{ printf(" OK\n"); } else
	{ printf(" Error.\n"); goto exit; }

	/* Select EF */
	ret = scMultiflexCmdSelectFile( ri, ci, 0x0000, rbuffer, &resplen );
	if( (ret==0) && (ci->sw[0]==0x90) && (ci->sw[1]==0x00) )
	{ } else
	{ printf(" Error.\n"); goto exit; }
	printarray( "  rsp:", resplen, buffer );

/* Update Binary */

	/* Write PIN */
	resplen=0x17;
	printf("scMultiflexCmdUpdateBinary:");
	ret = scMultiflexCmdUpdateBinary( ri, ci, 0, pinfile, resplen );
	if( (ret==0) && (ci->sw[0]==0x90) && (ci->sw[1]==0x00) )
	{ printf(" OK\n"); } else
	{ printf(" Error.\n"); goto exit; }

/* Verify PIN */

	/* Verify PIN */
	printf("scMultiflexCmdVerifyPIN:");
	ret = scMultiflexCmdVerifyPIN( ri, ci, oldpin );
	if( (ret==0) && (ci->sw[0]==0x90) && (ci->sw[1]==0x00) )
	{ printf(" OK\n"); } else
	{ printf(" Error.\n"); goto exit; }

/* Change PIN */

	/* Change PIN */
	printf("scMultiflexCmdChangePIN:");
	ret = scMultiflexCmdChangePIN( ri, ci, oldpin, newpin );
	if( (ret==0) && (ci->sw[0]==0x90) && (ci->sw[1]==0x00) )
	{ printf(" OK\n"); } else
	{ printf(" Error.\n"); goto exit; }

	/* Verify PIN */
	printf("scMultiflexCmdVerifyPIN:");
	ret = scMultiflexCmdVerifyPIN( ri, ci, newpin );
	if( (ret==0) && (ci->sw[0]==0x90) && (ci->sw[1]==0x00) )
	{ printf(" OK\n"); } else
	{ printf(" Error.\n"); goto exit; }

	/* Verify PIN */
	printf("scMultiflexCmdVerifyPIN(should fail):");
	ret = scMultiflexCmdVerifyPIN( ri, ci, oldpin );
	if( (ret==0) && (ci->sw[0]==0x69) && (ci->sw[1]==0x83) )
	{ printf(" OK\n"); } else
	{ printf(" Error.\n"); goto exit; }

	/* Verify PIN */
	printf("scMultiflexCmdVerifyPIN(should fail too):");
	ret = scMultiflexCmdVerifyPIN( ri, ci, newpin );
	if( (ret==0) && (ci->sw[0]==0x69) && (ci->sw[1]==0x83) )
	{ printf(" OK\n"); } else
	{ printf(" Error.\n"); goto exit; }

/* Unblock PIN */

	/* Unblock PIN */
	printf("scMultiflexCmdUnblockPIN:");
	ret = scMultiflexCmdUnblockPIN( ri, ci, unblockpin, oldpin );
	if( (ret==0) && (ci->sw[0]==0x90) && (ci->sw[1]==0x00) )
	{ printf(" OK\n"); } else
	{ printf(" Error.\n"); goto exit; }

	/* Verify PIN */
	printf("scMultiflexCmdVerifyPIN:");
	ret = scMultiflexCmdVerifyPIN( ri, ci, oldpin );
	if( (ret==0) && (ci->sw[0]==0x90) && (ci->sw[1]==0x00) )
	{ printf(" OK\n"); } else
	{ printf(" Error.\n"); goto exit; }

/* Read Binary */

	/* Read Binary */
	resplen=8;
	printf("scMultiflexCmdReadBinary:");
	ret = scMultiflexCmdReadBinary( ri, ci, 3, buffer, &resplen );
	if( (ret==0) && (ci->sw[0]==0x90) && (ci->sw[1]==0x00) )
	{ printf(" OK\n"); } else
	{ printf(" Error.\n"); goto exit; }
	printarray( "  rsp:", 8, buffer );

	/* Select DF */
	ret = scMultiflexCmdSelectFile( ri, ci, 0x2000, rbuffer, &resplen );
	if( (ret==0) && (ci->sw[0]==0x90) && (ci->sw[1]==0x00) )
	{ } else
	{ printf(" Error.\n"); goto exit; }

/* Create cyclic file */

	/* Create cyclic file */
	acond[0]=0xFF; acond[1]=0x44; acond[2]=0x44; acond[3]=0x44;
	akeys[0]=0x11; akeys[1]=0x11; akeys[2]=0x11;
	printf("scMultiflexCmdCreateFile(Cyclic File):");
	ret = scMultiflexCmdCreateFile( ri, ci, 0x2003,
		12, 6, 0x00, 1, 3, 4, acond, akeys );
	if( (ret==0) && (ci->sw[0]==0x90) && (ci->sw[1]==0x00) )
	{ printf(" OK\n"); } else
	{ printf(" Error.\n"); goto exit; }

	/* Select EF */
	ret = scMultiflexCmdSelectFile( ri, ci, 0x2003, rbuffer, &resplen );
	if( (ret==0) && (ci->sw[0]==0x90) && (ci->sw[1]==0x00) )
	{ } else
	{ printf(" Error.\n"); goto exit; }
	printarray( "  rsp:", resplen, rbuffer );

/* Increase */

	/* Increase */
	buffer[0]=0x01;buffer[1]=0x23;buffer[2]=0x45;
	printf("scMultiflexCmdIncrease:");
	ret = scMultiflexCmdIncrease( ri, ci, buffer, rbuffer, &resplen );
	if( (ret==0) && (ci->sw[0]==0x90) && (ci->sw[1]==0x00) )
	{ printf(" OK\n"); } else
	{ printf(" Error.\n"); goto exit; }
	printarray( "  rsp:", resplen, rbuffer );

#ifdef WITH_MULTIFLEX8K

/* Give Challenge */

	/* Give Challenge */
	mac[0]=0x00;mac[1]=0x01;mac[2]=0x02;mac[3]=0x03;
	mac[4]=0x04;mac[5]=0x05;mac[6]=0x06;mac[7]=0x07;
	printf("scMultiflexCmdGiveChall:");
	ret = scMultiflexCmdGiveChall( ri, ci, mac );
	if( (ret==0) && (ci->sw[0]==0x90) && (ci->sw[1]==0x00) )
	{ printf(" OK\n"); } else
	{ printf(" Error.\n"); goto exit; }

/* Increase St */

	/* Increase St */
	buffer[0]=0x01;buffer[1]=0x23;buffer[2]=0x45;
	printf("scMultiflexCmdIncreaseSt:");
	ret = scMultiflexCmdIncreaseSt( ri, ci, buffer, rbuffer,
		&resplen );
	if( (ret==0) && (ci->sw[0]==0x90) && (ci->sw[1]==0x00) )
	{ printf(" OK\n"); } else
	{ printf(" Error.\n"); goto exit; }
	printarray( "  rsp:", resplen, rbuffer );

#endif /* WITH_MULTIFLEX8K */

/* Decrease */

	/* Decrease */
	buffer[0]=0x01;buffer[1]=0x12;buffer[2]=0x34;
	printf("scMultiflexCmdDecrease:");
	ret = scMultiflexCmdDecrease( ri, ci, buffer, rbuffer, &resplen );
	if( (ret==0) && (ci->sw[0]==0x90) && (ci->sw[1]==0x00) )
	{ printf(" OK\n"); } else
	{ printf(" Error.\n"); goto exit; }
	printarray( "  rsp:", resplen, rbuffer );

#ifdef WITH_MULTIFLEX8K

	/* Give Challenge */
	mac[0]=0x00;mac[1]=0x01;mac[2]=0x02;mac[3]=0x03;
	mac[4]=0x04;mac[5]=0x05;mac[6]=0x06;mac[7]=0x07;
	ret = scMultiflexCmdGiveChall( ri, ci, mac );
	if( (ret==0) && (ci->sw[0]==0x90) && (ci->sw[1]==0x00) )
	{ } else
	{ printf(" Error.\n"); goto exit; }

/* Decrease St */

	/* Decrease St */
	buffer[0]=0x00;buffer[1]=0x23;buffer[2]=0x45;
	printf("scMultiflexCmdDecreaseSt:");
	ret = scMultiflexCmdDecreaseSt( ri, ci, buffer, rbuffer,
		&resplen );
	if( (ret==0) && (ci->sw[0]==0x90) && (ci->sw[1]==0x00) )
	{ printf(" OK\n"); } else
	{ printf(" Error.\n"); goto exit; }
	printarray( "  rsp:", resplen, rbuffer );

#endif /* WITH_MULTIFLEX8K */

/* Invalidate */

	/* Invalidate */
	printf("scMultiflexCmdInvalidate:");
	ret = scMultiflexCmdInvalidate( ri, ci );
	if( (ret==0) && (ci->sw[0]==0x90) && (ci->sw[1]==0x00) )
	{ printf(" OK\n"); } else
	{ printf(" Error.\n"); goto exit; }

	/* Increase */
	buffer[0]=0x01;buffer[1]=0x23;buffer[2]=0x45;
	printf("scMultiflexCmdIncrease(Should fail):");
	ret = scMultiflexCmdIncrease( ri, ci, buffer, rbuffer, &resplen );
	if( (ret==0) && (ci->sw[0]==0x62) && (ci->sw[1]==0x83) )
	{ printf(" OK\n"); } else
	{ printf(" Error.\n"); goto exit; }

/* Rehabilitate */

	/* Rehabilitate */
	printf("scMultiflexCmdRehabilitate:");
	ret = scMultiflexCmdRehabilitate( ri, ci );
	if( (ret==0) && (ci->sw[0]==0x90) && (ci->sw[1]==0x00) )
	{ printf(" OK\n"); } else
	{ printf(" Error.\n"); goto exit; }

	/* Increase */
	buffer[0]=0x01;buffer[1]=0x23;buffer[2]=0x45;
	printf("scMultiflexCmdIncrease:");
	ret = scMultiflexCmdIncrease( ri, ci, buffer, rbuffer, &resplen );
	if( (ret==0) && (ci->sw[0]==0x90) && (ci->sw[1]==0x00) )
	{ printf(" OK\n"); } else
	{ printf(" Error.\n"); goto exit; }
	printarray( "  rsp:", resplen, rbuffer );

	/* Select DF */
	ret = scMultiflexCmdSelectFile( ri, ci, 0x2000, rbuffer, &resplen );
	if( (ret==0) && (ci->sw[0]==0x90) && (ci->sw[1]==0x00) )
	{ } else
	{ printf(" Error.\n"); goto exit; }

	/* Create transparent file 0001 */
	acond[0]=0x3F; acond[1]=0xF4; acond[2]=0xFF; acond[3]=0x44;
	akeys[0]=0x11; akeys[1]=0xFF; akeys[2]=0x11;
	ret = scMultiflexCmdCreateFile( ri, ci, 0x0001,
		14, 1, 0x00, 1, 0, 0, acond, akeys );
	if( (ret==0) && (ci->sw[0]==0x90) && (ci->sw[1]==0x00) )
	{ } else
	{ printf(" Error. (ret: %d: %.2X%.2X)\n",ret,ci->sw[0],ci->sw[1]); goto exit; }

	/* Select EF */
	ret = scMultiflexCmdSelectFile( ri, ci, 0x0001, rbuffer, &resplen );
	if( (ret==0) && (ci->sw[0]==0x90) && (ci->sw[1]==0x00) )
	{ } else
	{ printf(" Error. (ret: %d: %.2X%.2X)\n",ret,ci->sw[0],ci->sw[1]); goto exit; }

	/* Write IntAuth */
	resplen=14;
	ret = scMultiflexCmdUpdateBinary( ri, ci, 0, intauth, resplen );
	if( (ret==0) && (ci->sw[0]==0x90) && (ci->sw[1]==0x00) )
	{ } else
	{ printf(" Error. (ret: %d: %.2X%.2X)\n",ret,ci->sw[0],ci->sw[1]); goto exit; }

#ifdef WITH_DES

/* Int Auth */

	/* Int Auth */
	mac[0]=0x00;mac[1]=0x01;mac[2]=0x02;mac[3]=0x03;
	mac[4]=0x04;mac[5]=0x05;mac[6]=0x06;mac[7]=0x07;
	printf("scMultiflexCmdIntAuth:");
	ret = scMultiflexCmdIntAuth( ri, ci, 0, mac, intauth+3 );
	if( (ret==0) && (ci->sw[0]==0x90) && (ci->sw[1]==0x00) )
	{ printf(" OK\n"); } else
	{ printf(" Error. (ret: %d: %.2X%.2X)\n",ret,ci->sw[0],ci->sw[1]); goto exit; }

#endif /* WITH_DES */

	/* Select DF */
	ret = scMultiflexCmdSelectFile( ri, ci, 0x2000, rbuffer, &resplen );
	if( (ret==0) && (ci->sw[0]==0x90) && (ci->sw[1]==0x00) )
	{ } else
	{ printf(" Error. (ret: %d: %.2X%.2X)\n",ret,ci->sw[0],ci->sw[1]); goto exit; }

/* Delete */

	/* Delete file */
	printf("scMultiflexCmdDeleteFile:");
	ret = scMultiflexCmdDeleteFile( ri, ci, 0x0001 );
	if( (ret==0) && (ci->sw[0]==0x90) && (ci->sw[1]==0x00) )
	{ printf(" OK\n"); } else
	{ printf(" Error.\n"); goto exit; }

	/* Delete transparent file 2003 */
	ret = scMultiflexCmdDeleteFile( ri, ci, 0x2003 );
	if( (ret==0) && (ci->sw[0]==0x90) && (ci->sw[1]==0x00) )
	{ } else
	{ printf(" Error.\n"); goto exit; }

	/* Delete transparent file 0000 */
	ret = scMultiflexCmdDeleteFile( ri, ci, 0x0000 );
	if( (ret==0) && (ci->sw[0]==0x90) && (ci->sw[1]==0x00) )
	{ } else
	{ printf(" Error.\n"); goto exit; }

	/* Delete transparent file */
	ret = scMultiflexCmdDeleteFile( ri, ci, 0x2001 );
	if( (ret==0) && (ci->sw[0]==0x90) && (ci->sw[1]==0x00) )
	{ } else
	{ printf(" Error.\n"); goto exit; }

	/* Select MF */
	ret = scMultiflexCmdSelectFile( ri, ci, 0x3F00, rbuffer, &resplen );
	if( (ret==0) && (ci->sw[0]==0x90) && (ci->sw[1]==0x00) )
	{ } else
	{ printf(" Error.\n"); goto exit; }

	/* Delete DF 2000 */
	ret = scMultiflexCmdDeleteFile( ri, ci, 0x2000 );
	if( (ret==0) && (ci->sw[0]==0x90) && (ci->sw[1]==0x00) )
	{ } else
	{ printf(" Error.\n"); goto exit; }

#ifdef WITH_DES

	/* Test MAC commands */

	printf("scMultiflexGenerateAuth:");
	scMultiflexGenerateAuth( testkey, challenge, mac );
	if( memcmp( mac, result, 6 ) ) { printf(" Error\n"); goto exit; }
	printf(" OK\n");

	/* Reset Card */
	printf("scReaderResetCard:");
	ret= scReaderResetCard( ri, ci );
	if( ret!=0 ) { printf(" Error. (%d)\n",ret); goto exit; }
	printf(" OK.\n");

	/* Reset Card */
	printf("scReaderResetCard:");
	ret= scReaderResetCard( ri, ci );
	if( ret!=0 ) { printf(" Error. (%d)\n",ret); goto exit; }
	printf(" OK.\n");

	/* Get Challenge */
	printf("scMultiflexCmdGetChall:");
	ret = scMultiflexCmdGetChall( ri, ci, buffer );
	if( (ret==0) && (ci->sw[0]==0x90) && (ci->sw[1]==0x00) )
	{ printf(" OK\n"); } else
	{ printf(" Error.\n"); goto exit; }
	printarray( "  rsp:", 8, buffer );

	/* External Authenticate */
	printf("scMultiflexCmdExtAuth:");
	ret = scMultiflexCmdExtAuth( ri, ci, 0x01, authkey, buffer );
	if( (ret==0) && (ci->sw[0]==0x90) && (ci->sw[1]==0x00) )
	{ printf(" OK\n"); } else
	{ printf(" Error.\n"); goto exit; }

	/* Verify transport key */
	/* Just in case to prevent blocking of key */
	ret = scMultiflexCmdVerifyKey( ri, ci, 1, authkey, 8 );
	if( (ret==0) && (ci->sw[0]==0x90) && (ci->sw[1]==0x00) )
	{ } else
	{ printf(" Error.\n"); goto exit; }

	/* Create Directory 2000 */
	acond[0]=0xFF; acond[1]=0x3F; acond[2]=0x33; acond[3]=0x33;
	akeys[0]=0x1F; akeys[1]=0x11; akeys[2]=0x11;
	ret = scMultiflexCmdCreateFile( ri, ci, 0x2000,
		200, 0x38, 0x00, 1, 0, 0, acond, akeys );
	if( (ret==0) && (ci->sw[0]==0x90) && (ci->sw[1]==0x00) )
	{ } else
	{ printf(" Error.\n"); goto exit; }

	/* Reset Card */
	printf("scReaderResetCard:");
	ret= scReaderResetCard( ri, ci );
	if( ret!=0 ) { printf(" Error.\n"); goto exit; }
	printf(" OK.\n");

	/* Select DF */
	ret = scMultiflexCmdSelectFile( ri, ci, 0x2000, rbuffer, &resplen );
	if( (ret==0) && (ci->sw[0]==0x90) && (ci->sw[1]==0x00) )
	{ } else
	{ printf(" Error.\n"); goto exit; }

	/* Get Challenge */
	ret = scMultiflexCmdGetChall( ri, ci, mac );
	if( (ret==0) && (ci->sw[0]==0x90) && (ci->sw[1]==0x00) )
	{ } else
	{ printf(" Error.\n"); goto exit; }

/* Create fixed-record file 2001 */

	/* Create fixed-record file 2001 */
	acond[0]=0x3F; acond[1]=0x33; acond[2]=0xF3; acond[3]=0x33;
	akeys[0]=0x11; akeys[1]=0xF1; akeys[2]=0x11;
	printf("scMultiflexCmdCreateFileMAC(Fixed-Record File):");
	ret = scMultiflexCmdCreateFileMAC( ri, ci, 0x2001,
		36, SC_MULTIFLEX_FILE_FIXED, 0x00, 1, 6, 5, acond, akeys,
		authkey, mac );
	if( (ret==0) && (ci->sw[0]==0x90) && (ci->sw[1]==0x00) )
	{ printf(" OK\n"); } else
	{ printf(" Error.\n"); goto exit; }

	/* Select EF */
	ret = scMultiflexCmdSelectFile( ri, ci, 0x2001, rbuffer, &resplen );
	if( (ret==0) && (ci->sw[0]==0x90) && (ci->sw[1]==0x00) )
	{ } else
	{ printf(" Error.\n"); goto exit; }
	printarray( "  rsp:", resplen, rbuffer );

	/* Get Challenge */
	ret = scMultiflexCmdGetChall( ri, ci, mac );
	if( (ret==0) && (ci->sw[0]==0x90) && (ci->sw[1]==0x00) )
	{ } else
	{ printf(" Error.\n"); goto exit; }

/* Update Record */

	/* Update Record */
	printf("scMultiflexCmdUpdateRecordMAC:");
	ret = scMultiflexCmdUpdateRecordMAC( ri, ci, 0, 0, "Kleo  ", 6,
		authkey, mac );
	if( (ret==0) && (ci->sw[0]==0x90) && (ci->sw[1]==0x00) )
	{ printf(" OK\n"); } else
	{ printf(" Error.\n"); goto exit; }

	/* Get Challenge */
	ret = scMultiflexCmdGetChall( ri, ci, mac );
	if( (ret==0) && (ci->sw[0]==0x90) && (ci->sw[1]==0x00) )
	{ } else
	{ printf(" Error.\n"); goto exit; }

/* Create Record */

	/* Create Record */
	printf("scMultiflexCmdCreateRecordMAC:");
	ret = scMultiflexCmdCreateRecordMAC( ri, ci, "Valeri", 6,
		authkey, mac );
	if( (ret==0) && (ci->sw[0]==0x90) && (ci->sw[1]==0x00) )
	{ printf(" OK\n"); } else
	{ printf(" Error.\n"); goto exit; }

	/* Select DF */
	ret = scMultiflexCmdSelectFile( ri, ci, 0x2000, rbuffer, &resplen );
	if( (ret==0) && (ci->sw[0]==0x90) && (ci->sw[1]==0x00) )
	{ } else
	{ printf(" Error.\n"); goto exit; }

	/* Get Challenge */
	ret = scMultiflexCmdGetChall( ri, ci, mac );
	if( (ret==0) && (ci->sw[0]==0x90) && (ci->sw[1]==0x00) )
	{ } else
	{ printf(" Error.\n"); goto exit; }

/* Create transparent file */

	/* Create transparent file 0000 */
	acond[0]=0x3F; acond[1]=0x33; acond[2]=0xFF; acond[3]=0x33;
	akeys[0]=0x11; akeys[1]=0xFF; akeys[2]=0x11;
	printf("scMultiflexCmdCreateFileMAC(Transparent File):");
	ret = scMultiflexCmdCreateFileMAC( ri, ci, 0x0000,
		23, 1, 0x00, 1, 0, 0, acond, akeys, authkey, mac );
	if( (ret==0) && (ci->sw[0]==0x90) && (ci->sw[1]==0x00) )
	{ printf(" OK\n"); } else
	{ printf(" Error.\n"); goto exit; }

	/* Select EF */
	ret = scMultiflexCmdSelectFile( ri, ci, 0x0000, rbuffer, &resplen );
	if( (ret==0) && (ci->sw[0]==0x90) && (ci->sw[1]==0x00) )
	{ } else
	{ printf(" Error.\n"); goto exit; }
	printarray( "  rsp:", resplen, rbuffer );

	/* Get Challenge */
	ret = scMultiflexCmdGetChall( ri, ci, mac );
	if( (ret==0) && (ci->sw[0]==0x90) && (ci->sw[1]==0x00) )
	{ } else
	{ printf(" Error.\n"); goto exit; }

/* Update Binary */

	/* Write PIN */
	resplen=13;
	printf("scMultiflexCmdUpdateBinaryMAC:");
	ret = scMultiflexCmdUpdateBinaryMAC( ri, ci, 0, pinfile, resplen,
		authkey, mac );
	if( (ret==0) && (ci->sw[0]==0x90) && (ci->sw[1]==0x00) )
	{ printf(" OK\n"); } else
	{ printf(" Error.\n"); goto exit; }

	/* Get Challenge */
	ret = scMultiflexCmdGetChall( ri, ci, mac );
	if( (ret==0) && (ci->sw[0]==0x90) && (ci->sw[1]==0x00) )
	{ } else
	{ printf(" Error.\n"); goto exit; }

	/* Write PIN */
	resplen=10;
	ret = scMultiflexCmdUpdateBinaryMAC( ri, ci, 13, pinfile, resplen,
		authkey, mac );
	if( (ret==0) && (ci->sw[0]==0x90) && (ci->sw[1]==0x00) )
	{ } else
	{ printf(" Error.\n"); goto exit; }

	/* Select DF */
	ret = scMultiflexCmdSelectFile( ri, ci, 0x2000, rbuffer, &resplen );
	if( (ret==0) && (ci->sw[0]==0x90) && (ci->sw[1]==0x00) )
	{ } else
	{ printf(" Error.\n"); goto exit; }

	/* Get Challenge */
	ret = scMultiflexCmdGetChall( ri, ci, mac );
	if( (ret==0) && (ci->sw[0]==0x90) && (ci->sw[1]==0x00) )
	{ } else
	{ printf(" Error.\n"); goto exit; }

/* Create cyclic file */

	/* Create cyclic file */
	acond[0]=0xFF; acond[1]=0x33; acond[2]=0x33; acond[3]=0x33;
	akeys[0]=0x11; akeys[1]=0x11; akeys[2]=0x11;
	printf("scMultiflexCmdCreateFileMAC(Cyclic File):");
	ret = scMultiflexCmdCreateFileMAC( ri, ci, 0x2003,
		12, 6, 0x00, 1, 3, 4, acond, akeys, authkey, mac );
	if( (ret==0) && (ci->sw[0]==0x90) && (ci->sw[1]==0x00) )
	{ printf(" OK\n"); } else
	{ printf(" Error.\n"); goto exit; }

	/* Select EF */
	ret = scMultiflexCmdSelectFile( ri, ci, 0x2003, rbuffer, &resplen );
	if( (ret==0) && (ci->sw[0]==0x90) && (ci->sw[1]==0x00) )
	{ } else
	{ printf(" Error.\n"); goto exit; }
	printarray( "  rsp:", resplen, rbuffer );

	/* Get Challenge */
	ret = scMultiflexCmdGetChall( ri, ci, mac );
	if( (ret==0) && (ci->sw[0]==0x90) && (ci->sw[1]==0x00) )
	{ } else
	{ printf(" Error.\n"); goto exit; }

/* Increase */

	/* Increase */
	buffer[0]=0x01;buffer[1]=0x23;buffer[2]=0x45;
	printf("scMultiflexCmdIncreaseMAC:");
	ret = scMultiflexCmdIncreaseMAC( ri, ci, buffer, authkey, mac,
		rbuffer, &resplen );
	if( (ret==0) && (ci->sw[0]==0x90) && (ci->sw[1]==0x00) )
	{ printf(" OK\n"); } else
	{ printf(" Error.\n"); goto exit; }
	printarray( "  rsp:", resplen, rbuffer );

#ifdef WITH_MULTIFLEX8K

	/* Give Challenge */
	mac[0]=0x00;mac[1]=0x01;mac[2]=0x02;mac[3]=0x03;
	mac[4]=0x04;mac[5]=0x05;mac[6]=0x06;mac[7]=0x07;
	ret = scMultiflexCmdGiveChall( ri, ci, mac );
	if( (ret==0) && (ci->sw[0]==0x90) && (ci->sw[1]==0x00) )
	{ } else
	{ printf(" Error.\n"); goto exit; }

	/* Get Challenge */
	ret = scMultiflexCmdGetChall( ri, ci, mac );
	if( (ret==0) && (ci->sw[0]==0x90) && (ci->sw[1]==0x00) )
	{ } else
	{ printf(" Error.\n"); goto exit; }

/* Increase St MAC */

	/* Increase St */
	buffer[0]=0x01;buffer[1]=0x23;buffer[2]=0x45;
	printf("scMultiflexCmdIncreaseStMAC:");
	ret = scMultiflexCmdIncreaseStMAC( ri, ci, buffer, authkey,
		mac, rbuffer, &resplen );
	if( (ret==0) && (ci->sw[0]==0x90) && (ci->sw[1]==0x00) )
	{ printf(" OK\n"); } else
	{ printf(" Error.\n"); goto exit; }
	printarray( "  rsp:", resplen, rbuffer );

#endif /* WITH_MULTIFLEX8K */

	/* Get Challenge */
	ret = scMultiflexCmdGetChall( ri, ci, mac );
	if( (ret==0) && (ci->sw[0]==0x90) && (ci->sw[1]==0x00) )
	{ } else
	{ printf(" Error.\n"); goto exit; }

/* Decrease */

	/* Decrease */
	buffer[0]=0x01;buffer[1]=0x12;buffer[2]=0x34;
	printf("scMultiflexCmdDecreaseMAC:");
	ret = scMultiflexCmdDecreaseMAC( ri, ci, buffer, authkey, mac,
		rbuffer, &resplen );
	if( (ret==0) && (ci->sw[0]==0x90) && (ci->sw[1]==0x00) )
	{ printf(" OK\n"); } else
	{ printf(" Error.\n"); goto exit; }
	printarray( "  rsp:", resplen, rbuffer );

#ifdef WITH_MULTIFLEX8K

	/* Give Challenge */
	mac[0]=0x00;mac[1]=0x01;mac[2]=0x02;mac[3]=0x03;
	mac[4]=0x04;mac[5]=0x05;mac[6]=0x06;mac[7]=0x07;
	ret = scMultiflexCmdGiveChall( ri, ci, mac );
	if( (ret==0) && (ci->sw[0]==0x90) && (ci->sw[1]==0x00) )
	{ } else
	{ printf(" Error.\n"); goto exit; }

	/* Get Challenge */
	ret = scMultiflexCmdGetChall( ri, ci, mac );
	if( (ret==0) && (ci->sw[0]==0x90) && (ci->sw[1]==0x00) )
	{ } else
	{ printf(" Error.\n"); goto exit; }

/* Decrease St MAC */

	/* Decrease St */
	buffer[0]=0x00;buffer[1]=0x23;buffer[2]=0x45;
	printf("scMultiflexCmdDecreaseStMAC:");
	ret = scMultiflexCmdDecreaseStMAC( ri, ci, buffer, authkey,
		mac, rbuffer, &resplen );
	if( (ret==0) && (ci->sw[0]==0x90) && (ci->sw[1]==0x00) )
	{ printf(" OK\n"); } else
	{ printf(" Error.\n"); goto exit; }
	printarray( "  rsp:", resplen, buffer );

#endif /* WITH_MULTIFLEX8K */

	/* Get Challenge */
	ret = scMultiflexCmdGetChall( ri, ci, mac );
	if( (ret==0) && (ci->sw[0]==0x90) && (ci->sw[1]==0x00) )
	{ } else
	{ printf(" Error.\n"); goto exit; }

/* Invalidate */

	/* Invalidate */
	printf("scMultiflexCmdInvalidateMAC:");
	ret = scMultiflexCmdInvalidateMAC( ri, ci, authkey, mac );
	if( (ret==0) && (ci->sw[0]==0x90) && (ci->sw[1]==0x00) )
	{ printf(" OK\n"); } else
	{ printf(" Error.\n"); goto exit; }

	/* Get Challenge */
	ret = scMultiflexCmdGetChall( ri, ci, mac );
	if( (ret==0) && (ci->sw[0]==0x90) && (ci->sw[1]==0x00) )
	{ } else
	{ printf(" Error.\n"); goto exit; }

	/* Increase */
	buffer[0]=0x01;buffer[1]=0x23;buffer[2]=0x45;
	printf("scMultiflexCmdIncreaseMAC(Should fail):");
	ret = scMultiflexCmdIncreaseMAC( ri, ci, buffer, authkey, mac,
		rbuffer, &resplen );
	if( (ret==0) && (ci->sw[0]==0x62) && (ci->sw[1]==0x83) )
	{ printf(" OK\n"); } else
	{ printf(" Error.\n"); goto exit; }

/* Rehabilitate */

	/* Rehabilitate */
	printf("scMultiflexCmdRehabilitateMAC:");
	ret = scMultiflexCmdRehabilitateMAC( ri, ci, authkey, mac );
	if( (ret==0) && (ci->sw[0]==0x90) && (ci->sw[1]==0x00) )
	{ printf(" OK\n"); } else
	{ printf(" Error.\n"); goto exit; }

	/* Get Challenge */
	ret = scMultiflexCmdGetChall( ri, ci, mac );
	if( (ret==0) && (ci->sw[0]==0x90) && (ci->sw[1]==0x00) )
	{ } else
	{ printf(" Error.\n"); goto exit; }

	/* Increase */
	buffer[0]=0x01;buffer[1]=0x23;buffer[2]=0x45;
	printf("scMultiflexCmdIncreaseMAC:");
	ret = scMultiflexCmdIncreaseMAC( ri, ci, buffer, authkey, mac,
		rbuffer, &resplen );
	if( (ret==0) && (ci->sw[0]==0x90) && (ci->sw[1]==0x00) )
	{ printf(" OK\n"); } else
	{ printf(" Error.\n"); goto exit; }
	printarray( "  rsp:", resplen, rbuffer );

	/* Select DF */
	ret = scMultiflexCmdSelectFile( ri, ci, 0x2000, rbuffer, &resplen );
	if( (ret==0) && (ci->sw[0]==0x90) && (ci->sw[1]==0x00) )
	{ } else
	{ printf(" Error.\n"); goto exit; }

	/* Get Challenge */
	ret = scMultiflexCmdGetChall( ri, ci, mac );
	if( (ret==0) && (ci->sw[0]==0x90) && (ci->sw[1]==0x00) )
	{ } else
	{ printf(" Error.\n"); goto exit; }

/* Delete */

	/* Delete file */
	printf("scMultiflexCmdDeleteFileMAC:");
	ret = scMultiflexCmdDeleteFileMAC( ri, ci, 0x2003, authkey, mac );
	if( (ret==0) && (ci->sw[0]==0x90) && (ci->sw[1]==0x00) )
	{ printf(" OK\n"); } else
	{ printf(" Error.\n"); goto exit; }

	/* Get Challenge */
	ret = scMultiflexCmdGetChall( ri, ci, mac );
	if( (ret==0) && (ci->sw[0]==0x90) && (ci->sw[1]==0x00) )
	{ } else
	{ printf(" Error.\n"); goto exit; }

	/* Delete transparent file 0000 */
	ret = scMultiflexCmdDeleteFileMAC( ri, ci, 0x0000, authkey, mac );
	if( (ret==0) && (ci->sw[0]==0x90) && (ci->sw[1]==0x00) )
	{ } else
	{ printf(" Error.\n"); goto exit; }

	/* Get Challenge */
	ret = scMultiflexCmdGetChall( ri, ci, mac );
	if( (ret==0) && (ci->sw[0]==0x90) && (ci->sw[1]==0x00) )
	{ } else
	{ printf(" Error.\n"); goto exit; }

	/* Delete transparent file */
	ret = scMultiflexCmdDeleteFileMAC( ri, ci, 0x2001, authkey, mac );
	if( (ret==0) && (ci->sw[0]==0x90) && (ci->sw[1]==0x00) )
	{ } else
	{ printf(" Error.\n"); goto exit; }

	/* Select MF */
	ret = scMultiflexCmdSelectFile( ri, ci, 0x3F00, rbuffer, &resplen );
	if( (ret==0) && (ci->sw[0]==0x90) && (ci->sw[1]==0x00) )
	{ } else
	{ printf(" Error.\n"); goto exit; }

	/* Verify transport key */
	ret = scMultiflexCmdVerifyKey( ri, ci, 1, authkey, 8 );
	if( (ret==0) && (ci->sw[0]==0x90) && (ci->sw[1]==0x00) )
	{ } else
	{ printf(" Error.\n"); goto exit; }

	/* Delete DF 2000 */
	ret = scMultiflexCmdDeleteFile( ri, ci, 0x2000 );
	if( (ret==0) && (ci->sw[0]==0x90) && (ci->sw[1]==0x00) )
	{ } else
	{ printf(" Error.\n"); goto exit; }

#endif /* WITH_DES */

exit:
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

