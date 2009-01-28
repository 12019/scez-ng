/* $Id: tsmartcafe.c 1617 2005-11-03 17:41:39Z laforge $ */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#if 0
#include <sio/sio.h>
#endif
#include <scez/scgeneral.h>
#include <scez/scsmartcard.h>
#include <scez/screader.h>
#include <scez/cards/scsmartcafe.h>

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
	BYTE aidml[]={
		0x53,0x4D,0x40,0x52,0x54,0x20,0x43,0x41,
		0x46,0x45,0x20,0x31,0x31,0x20,0x4D,0x4C};
	BYTE aidapp[]={'1','1','1','1','1'};
#if 0
	BYTE aidapp[]={0xD2,0x76,0x00,0x00,0x92,'T','e','s','t'};
#endif
	BYTE pin[]={'1','2','3','4'};
	BYTE keycrypt[]={'A','B','C','D','E','F','G','H'};
	BYTE keysign[]={'H','G','F','E','D','C','B','A'};
	BYTE key3crypt[]=
		{'A','B','C','D','E','F','G','H','H','G','F','E','D','C','B','A'};
	BYTE key3sign[]=
		{'H','G','F','E','D','C','B','A','A','B','C','D','E','F','G','H'};
	BYTE cap[]={
		0xDE,0xCA,0xFF,0xED,0x01,0x01,0x01,0x10,
		0x37,0x30,0x31,0x38,0x30,0x32,0x37,0x33,
		0x30,0x20,0x20,0x20,0x20,0x20,0x20,0x00,
		0x01,0x00,0x01,0x00,0x32,0x20,0x00,0x0A,
		0x00,0x00,0x00,0x00,0x10,0x00,0x00,0x08, /* 40 */
		0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
		0xFF,0xFF,0xFF,0xFF,0x00,0x19,0xFF,0xFF,
		0x00,0x00,0x22,0x49,0x01,0x11,0x11,0x56,
		0x00,0x2D,0x11,0x57,0x01,0x04,0x49,0x01,
		0x33,0x59,0x00,0x01,0x56,0x00,0x1C,0x49, /* 80 */
		0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
		0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
		0x00,0x00,0x10,0x37,0x30,0x31,0x38,0x30,
		0x32,0x37,0x33,0x30,0x20,0x20,0x20,0x20,
		0x20,0x20,0x00,0x01,0x01,0x01,0x00,0x32, /* 120 */
		0x00,0x30,0x00,0x00,0x00,0x00,0x00,0x00,
		0x00,0x01,0x05,0x31,0x31,0x31,0x31,0x31,
		0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
		0x00,0x00,0x00,0x00,0x27,0x00,0x00,0x00,
		0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00, /* 160 */
		0x00,0x0B,0xFF,0xFF,0x00,0x01,0x00,0x01,
		0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
		0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
		0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
		0x00,0x0C,0xFF,0xFF,0x00,0x02,0x00,0x1C, /* 200 */
		0x00,0x27,0x00,0x00,0x00,0x00,0x00,0x00,
		0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
		0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00, /* 224 */
		0x00,0x02,0x01,0x00,0x10,0x2D,0x31,0x34,
		0x34,0x39,0x32,0x31,0x31,0x35,0x30,0x33,
		0x20,0x20,0x20,0x20,0x04,0x00,0x00,0x00,
		0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
		0x00,0x03,0xFF,0xFF,0x00,0x01,0x00,0x03, /* 40 */
		0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
		0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
		0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
		0x00,0x04,0xFF,0xFF,0x00,0x01,0x00,0x20,
		0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00, /* 80 */
		0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
		0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
		0x00,0x07,0xFF,0xFF,0x00,0x01,0x00,0x01,
		0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
		0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00, /* 120 */
		0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
		0x00,0x08,0xFF,0xFF,0x00,0x03,0x00,0x14,
		0x00,0x2A,0x00,0x2D,0x00,0x00,0x00,0x00,
		0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
		0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00, /* 160 */
		0x00,0x0A,0x00,0x01,0x00,0x00,0x00,0x00,
		0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
		0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
		0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00, /* 192 */
		0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF};
	BYTE cap2[]={
		0xDE,0xCA,0xFF,0xED,0x01,0x01,0x01,0x10,
		0x37,0x30,0x31,0x38,0x30,0x32,0x37,0x33,
		0x30,0x20,0x20,0x20,0x20,0x20,0x20,0x00,
		0x01,0x00,0x01,0x00,0x32,0x20,0x00,0x0A,
		0x00,0x00,0x00,0x00,0x10,0x00,0x00,0x08, /* 40 */
		0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
		0xFF,0xFF,0xFF,0xFF,0x00,0x19,0xFF,0xFF,
		0x00,0x00,0x22,0x49,0x01,0x11,0x11,0x56,
		0x00,0x2D,0x11,0x57,0x01,0x04,0x49,0x01,
		0x33,0x59,0x00,0x01,0x56,0x00,0x1C,0x49, /* 80 */
		0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
		0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
		0x00,0x00,0x10,0x37,0x30,0x31,0x38,0x30,
		0x32,0x37,0x33,0x30,0x20,0x20,0x20,0x20,
		0x20,0x20,0x00,0x01,0x01,0x01,0x00,0x32, /* 120 */
		0x00,0x30,0x00,0x00,0x00,0x00,0x00,0x00,
		0x00,0x01,0x05,0x31,0x31,0x31,0x31,0x31,
		0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
		0x00,0x00,0x00,0x00,0x27,0x00,0x00,0x00,
		0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00, /* 160 */
		0x00,0x0B,0xFF,0xFF,0x00,0x01,0x00,0x01,
		0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
		0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
		0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
		0x00,0x0C,0xFF,0xFF,0x00,0x02,0x00,0x1C, /* 200 */
		0x00,0x27,0x00,0x00,0x00,0x00,0x00,0x00,
		0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
		0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00, /* 224 */
		0x00,0x02,0x01,0x00,0x10,0x2D,0x31,0x34,
		0x34,0x39,0x32,0x31,0x31,0x35,0x30,0x33,
		0x20,0x20,0x20,0x20,0x04,0x00,0x00,0x00,
		0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
		0x00,0x03,0xFF,0xFF,0x00,0x01,0x00,0x03, /* 40 */
		0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
		0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
		0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
		0x00,0x04,0xFF,0xFF,0x00,0x01,0x00,0x20,
		0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00, /* 80 */
		0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
		0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
		0x00,0x07,0xFF,0xFF,0x00,0x01,0x00,0x01,
		0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
		0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00, /* 120 */
		0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
		0x00,0x08,0xFF,0xFF,0x00,0x03,0x00,0x14,
		0x00,0x2A,0x00,0x2D,0x00,0x00,0x00,0x00,
		0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
		0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00, /* 160 */
		0x00,0x0A,0x00,0x01,0x00,0x00,0x00,0x00,
		0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
		0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
		0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00, /* 192 */
		0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF};

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
		if( (ci->type&0xFFFFFF00)!=SC_CARD_SMARTCAFE )
		{ printf("Error: Wrong Card.\n"); break; }

#if 0
		SIO_SetLogFile( ri->si, "LogTSmartcafe.txt" );
#endif

#ifdef WITH_DES
		printf("scSmartcafeAuthApplet(Sign,DES):");
		resplen=sizeof(cap)-8;
		ret = scSmartcafeAuthApplet( NULL, SC_SMARTCAFE_ALGO_NONE,
			keysign, SC_SMARTCAFE_ALGO_DES, cap, &resplen );
		if( ret!=SC_EXIT_OK ) { printf(" Error.\n"); break; }
		printarray( " OK\n  sig:", 8, (cap+224+192) );

		printf("scSmartcafeAuthApplet(Crypt&Sign,DES):");
		resplen=sizeof(cap)-8;
		ret = scSmartcafeAuthApplet( keycrypt, SC_SMARTCAFE_ALGO_DES,
			keysign, SC_SMARTCAFE_ALGO_DES, cap, &resplen );
		if( ret!=SC_EXIT_OK ) { printf(" Error.\n"); break; }
		printarray( " OK\n  sig:", 8, (cap+224+192) );

		printf("scSmartcafeAuthApplet(Crypt&Sign,3DES):");
		resplen=sizeof(cap2)-8;
		ret = scSmartcafeAuthApplet( key3crypt, SC_SMARTCAFE_ALGO_3DES,
			key3sign, SC_SMARTCAFE_ALGO_3DES, cap2, &resplen );
		if( ret!=SC_EXIT_OK ) { printf(" Error.\n"); break; }
		printarray( " OK\n  sig:", 8, (cap2+224+192) );

#endif /* WITH_DES */

/* Get Data */
		printf("scSmartcafeCmdGetData:");
		ret = scSmartcafeCmdGetData( ri, ci, SC_SMARTCAFE_TAG_VERSION,
			rbuffer, &resplen );
		if( !( (ret==0) && (ci->sw[0]==0x90) && (ci->sw[1]==0x00) ) )
		{ printf(" Error.\n"); break; }
		printarray( " OK\n  rsp:", resplen, rbuffer );

		/* Get Data */
		printf("scSmartcafeCmdGetData:");
		ret = scSmartcafeCmdGetData( ri, ci, SC_SMARTCAFE_TAG_SERIAL,
			rbuffer, &resplen );
		if( !( (ret==0) && (ci->sw[0]==0x90) && (ci->sw[1]==0x00) ) )
		{ printf(" Error.\n"); break; }
		printarray( " OK\n  rsp:", resplen, rbuffer );

		scSmartcafeCmdClearMem( ri, ci );
		scSmartcafeCmdDeleteML( ri, ci );

/* Create ML */
		printf("scSmartcafeCmdCreateML(start):");
#ifdef WITH_DES
		ret = scSmartcafeCmdCreateML( ri, ci, TRUE, 8, SC_SMARTCAFE_SPL_SIGN,
			"\xFF\xFF\xFF\xFF\xFF\xFF" );
#else
		ret = scSmartcafeCmdCreateML( ri, ci, TRUE, 8, 0,
			"\xFF\xFF\xFF\xFF\xFF\xFF" );
#endif
		if( !( (ret==0) && (ci->sw[0]==0x90) && (ci->sw[1]==0x00) ) )
		{ printf(" Error.\n"); break; }
		printf(" OK\n");

/* Set PIN */
		printf("scSmartcafeCmdSetPIN:");
		ret = scSmartcafeCmdSetPIN( ri, ci, pin, 4 );
		if( !( (ret==0) && (ci->sw[0]==0x90) && (ci->sw[1]==0x00) ) )
		{ printf(" Error.\n"); break; }
		printf(" OK\n");

/* Put Key */
		printf("scSmartcafeCmdPutKey(Sign):");
		ret = scSmartcafeCmdPutKey( ri, ci, SC_SMARTCAFE_KEYIDX_SIGN,
			keysign, 8 );
		if( !( (ret==0) && (ci->sw[0]==0x90) && (ci->sw[1]==0x00) ) )
		{ printf(" Error.\n"); break; }
		printf(" OK\n");

		printf("scSmartcafeCmdPutKey(Crypt):");
		ret = scSmartcafeCmdPutKey( ri, ci, SC_SMARTCAFE_KEYIDX_CRYPT,
			keycrypt, 8 );
		if( !( (ret==0) && (ci->sw[0]==0x90) && (ci->sw[1]==0x00) ) )
		{ printf(" Error.\n"); break; }
		printf(" OK\n");

		printf("scSmartcafeCmdCreateML(end):");
		ret = scSmartcafeCmdCreateML( ri, ci, FALSE, 0, 0, 0 );
		if( !( (ret==0) && (ci->sw[0]==0x90) && (ci->sw[1]==0x00) ) )
		{ printf(" Error.\n"); break; }
		printf(" OK\n");

/* Verify PIN */
		printf("scSmartcafeCmdVerifyPIN:");
		ret = scSmartcafeCmdVerifyPIN( ri, ci, pin, 4 );
		if( !( (ret==0) && (ci->sw[0]==0x90) && (ci->sw[1]==0x00) ) )
		{ printf(" Error.\n"); break; }
		printf(" OK\n");

/* Select */
		printf("scSmartcafeCmdSelect(ML):");
		ret = scSmartcafeCmdSelect( ri, ci, aidml, sizeof(aidml), rbuffer,
			&resplen );
		if( !( (ret==0) && (ci->sw[0]==0x90) && (ci->sw[1]==0x00) ) )
		{ printf(" Error.\n"); break; }
		printarray( " OK\n  rsp:", resplen, rbuffer );

/* Install */
#ifdef WITH_DES
		printf("scSmartcafeCmdInstall(Load,Crypt&Sign,DES):");
		ret = scSmartcafeCmdInstall( ri, ci, SC_SMARTCAFE_INSTALL_LOAD,
			SC_SMARTCAFE_SPL_SIGN|SC_SMARTCAFE_SPL_CRYPT,
			aidapp, sizeof(aidapp), NULL, 0, 0 );
#else /* !WITH_DES */
		printf("scSmartcafeCmdInstall(Load):");
		ret = scSmartcafeCmdInstall( ri, ci, SC_SMARTCAFE_INSTALL_LOAD,
			0, aidapp, sizeof(aidapp), NULL, 0, 0 );
#endif /* WITH_DES */
		if( !( (ret==0) && (ci->sw[0]==0x90) && (ci->sw[1]==0x00) ) )
		{ printf(" Error.\n"); break; }
		printf(" OK\n");

/* Load Applet */
		printf("scSmartcafeCmdLoadApplet:");
		ret = scSmartcafeCmdLoadApplet( ri, ci, FALSE, 0, cap, 224 );
		if( !( (ret==0) && (ci->sw[0]==0x90) && (ci->sw[1]==0x00) ) )
		{ printf(" Error.\n"); break; }
		printf(" OK\n");

		printf("scSmartcafeCmdLoadApplet:");
#ifdef WITH_DES
		ret = scSmartcafeCmdLoadApplet( ri, ci, FALSE, 1, cap+224, 192 );
#else /* !WITH_DES */
		ret = scSmartcafeCmdLoadApplet( ri, ci, TRUE, 1, cap+224, 192 );
#endif /* WITH_DES */
		if( !( (ret==0) && (ci->sw[0]==0x90) && (ci->sw[1]==0x00) ) )
		{ printf(" Error.\n"); break; }
		printf(" OK\n");

#ifdef WITH_DES
		printf("scSmartcafeCmdLoadApplet:");
		ret = scSmartcafeCmdLoadApplet( ri, ci, TRUE, 2, cap+224+192, 8 );
		if( !( (ret==0) && (ci->sw[0]==0x90) && (ci->sw[1]==0x00) ) )
		{ printf(" Error.\n"); break; }
		printf(" OK\n");
#endif /* WITH_DES */

		printf("scSmartcafeCmdInstall(Install):");
#ifdef WITH_DES
		ret = scSmartcafeCmdInstall( ri, ci, SC_SMARTCAFE_INSTALL_INST,
			SC_SMARTCAFE_SPL_SIGN|SC_SMARTCAFE_SPL_CRYPT,
			aidapp, sizeof(aidapp), NULL, 0, 0 );
#else /* !WITH_DES */
		ret = scSmartcafeCmdInstall( ri, ci, SC_SMARTCAFE_INSTALL_INST,
			0, aidapp, sizeof(aidapp), NULL, 0, 0 );
#endif /* WITH_DES */
		if( !( (ret==0) && (ci->sw[0]==0x90) && (ci->sw[1]==0x00) ) )
		{ printf(" Error.\n"); break; }
		printf(" OK\n");

		printf("scSmartcafeCmdSelect(App):");
		ret = scSmartcafeCmdSelect( ri, ci, aidapp, sizeof(aidapp), rbuffer,
			&resplen );
		if( !( (ret==0) && (ci->sw[0]==0x90) && (ci->sw[1]==0x00) ) )
		{ printf(" Error.\n"); break; }
		printarray( " OK\n  rsp:", resplen, rbuffer );

		printf("scSmartcafeCmdSelect(ML):");
		ret = scSmartcafeCmdSelect( ri, ci, aidml, sizeof(aidml), rbuffer,
			&resplen );
		if( !( (ret==0) && (ci->sw[0]==0x90) && (ci->sw[1]==0x00) ) )
		{ printf(" Error.\n"); break; }
		printarray( " OK\n  rsp:", resplen, rbuffer );

/* Clear Memory */
		printf("scSmartcafeCmdClearMem:");
		ret = scSmartcafeCmdClearMem( ri, ci );
		if( !( (ret==0) && (ci->sw[0]==0x90) && (ci->sw[1]==0x00) ) )
		{ printf(" Error.\n"); break; }
		printf(" OK\n");

/* Delete ML */
		printf("scSmartcafeCmdDeleteML:");
		ret = scSmartcafeCmdDeleteML( ri, ci );
		if( !( (ret==0) && (ci->sw[0]==0x90) && (ci->sw[1]==0x00) ) )
		{ printf(" Error.\n"); break; }
		printf(" OK\n");

#ifdef WITH_DES
		printf("scSmartcafeCmdCreateML(start):");
		ret = scSmartcafeCmdCreateML( ri, ci, TRUE, 8, SC_SMARTCAFE_SPL_SIGN,
			"\xFF\xFF\xFF\xFF\xFF\xFF" );
		if( !( (ret==0) && (ci->sw[0]==0x90) && (ci->sw[1]==0x00) ) )
		{ printf(" Error.\n"); break; }
		printf(" OK\n");

		printf("scSmartcafeCmdSetPIN:");
		ret = scSmartcafeCmdSetPIN( ri, ci, pin, 4 );
		if( !( (ret==0) && (ci->sw[0]==0x90) && (ci->sw[1]==0x00) ) )
		{ printf(" Error.\n"); break; }
		printf(" OK\n");

		printf("scSmartcafeCmdPutKey(Sign,3DES):");
		ret = scSmartcafeCmdPutKey( ri, ci, SC_SMARTCAFE_KEYIDX_SIGN,
			key3sign, 16 );
		if( !( (ret==0) && (ci->sw[0]==0x90) && (ci->sw[1]==0x00) ) )
		{ printf(" Error.\n"); break; }
		printf(" OK\n");

		printf("scSmartcafeCmdPutKey(Crypt,3DES):");
		ret = scSmartcafeCmdPutKey( ri, ci, SC_SMARTCAFE_KEYIDX_CRYPT,
			key3crypt, 16 );
		if( !( (ret==0) && (ci->sw[0]==0x90) && (ci->sw[1]==0x00) ) )
		{ printf(" Error.\n"); break; }
		printf(" OK\n");

		printf("scSmartcafeCmdCreateML(end):");
		ret = scSmartcafeCmdCreateML( ri, ci, FALSE, 0, 0, 0 );
		if( !( (ret==0) && (ci->sw[0]==0x90) && (ci->sw[1]==0x00) ) )
		{ printf(" Error.\n"); break; }
		printf(" OK\n");

		printf("scSmartcafeCmdVerifyPIN:");
		ret = scSmartcafeCmdVerifyPIN( ri, ci, pin, 4 );
		if( !( (ret==0) && (ci->sw[0]==0x90) && (ci->sw[1]==0x00) ) )
		{ printf(" Error.\n"); break; }
		printf(" OK\n");

		printf("scSmartcafeCmdSelect(ML):");
		ret = scSmartcafeCmdSelect( ri, ci, aidml, sizeof(aidml), rbuffer,
			&resplen );
		if( !( (ret==0) && (ci->sw[0]==0x90) && (ci->sw[1]==0x00) ) )
		{ printf(" Error.\n"); break; }
		printarray( " OK\n  rsp:", resplen, rbuffer );

		printf("scSmartcafeCmdInstall(Load,Crypt&Sign,3DES):");
		ret = scSmartcafeCmdInstall( ri, ci, SC_SMARTCAFE_INSTALL_LOAD,
			SC_SMARTCAFE_SPL_SIGN|SC_SMARTCAFE_SPL_CRYPT,
			aidapp, sizeof(aidapp), NULL, 0, 0 );
		if( !( (ret==0) && (ci->sw[0]==0x90) && (ci->sw[1]==0x00) ) )
		{ printf(" Error.\n"); break; }
		printf(" OK\n");

		printf("scSmartcafeCmdLoadApplet:");
		ret = scSmartcafeCmdLoadApplet( ri, ci, FALSE, 0, cap2, 224 );
		if( !( (ret==0) && (ci->sw[0]==0x90) && (ci->sw[1]==0x00) ) )
		{ printf(" Error.\n"); break; }
		printf(" OK\n");

		printf("scSmartcafeCmdLoadApplet:");
		ret = scSmartcafeCmdLoadApplet( ri, ci, FALSE, 1, cap2+224, 192 );
		if( !( (ret==0) && (ci->sw[0]==0x90) && (ci->sw[1]==0x00) ) )
		{ printf(" Error.\n"); break; }
		printf(" OK\n");

		printf("scSmartcafeCmdLoadApplet:");
		ret = scSmartcafeCmdLoadApplet( ri, ci, TRUE, 2, cap2+224+192, 8 );
		if( !( (ret==0) && (ci->sw[0]==0x90) && (ci->sw[1]==0x00) ) )
		{ printf(" Error.\n"); break; }
		printf(" OK\n");

		printf("scSmartcafeCmdInstall(Install):");
		ret = scSmartcafeCmdInstall( ri, ci, SC_SMARTCAFE_INSTALL_INST,
			SC_SMARTCAFE_SPL_SIGN|SC_SMARTCAFE_SPL_CRYPT,
			aidapp, sizeof(aidapp), NULL, 0, 0 );
		if( !( (ret==0) && (ci->sw[0]==0x90) && (ci->sw[1]==0x00) ) )
		{ printf(" Error.\n"); break; }
		printf(" OK\n");

		printf("scSmartcafeCmdSelect(App):");
		ret = scSmartcafeCmdSelect( ri, ci, aidapp, sizeof(aidapp), rbuffer,
			&resplen );
		if( !( (ret==0) && (ci->sw[0]==0x90) && (ci->sw[1]==0x00) ) )
		{ printf(" Error.\n"); break; }
		printarray( " OK\n  rsp:", resplen, rbuffer );

		printf("scSmartcafeCmdSelect(ML):");
		ret = scSmartcafeCmdSelect( ri, ci, aidml, sizeof(aidml), rbuffer,
			&resplen );
		if( !( (ret==0) && (ci->sw[0]==0x90) && (ci->sw[1]==0x00) ) )
		{ printf(" Error.\n"); break; }
		printarray( " OK\n  rsp:", resplen, rbuffer );

		printf("scSmartcafeCmdClearMem:");
		ret = scSmartcafeCmdClearMem( ri, ci );
		if( !( (ret==0) && (ci->sw[0]==0x90) && (ci->sw[1]==0x00) ) )
		{ printf(" Error.\n"); break; }
		printf(" OK\n");

		printf("scSmartcafeCmdDeleteML:");
		ret = scSmartcafeCmdDeleteML( ri, ci );
		if( !( (ret==0) && (ci->sw[0]==0x90) && (ci->sw[1]==0x00) ) )
		{ printf(" Error.\n"); break; }
		printf(" OK\n");
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
