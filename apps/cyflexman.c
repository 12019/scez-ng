/****************************************************************************
*																			*
*					SCEZ chipcard library - Cyberflex Manager				*
*					Copyright Matthias Bruestle 1999,2000					*
*					For licensing, see the file COPYRIGHT					*
*																			*
****************************************************************************/

/* $Id: cyflexman.c 1617 2005-11-03 17:41:39Z laforge $ */

/* -L: Load applet
 * -U: Unload applet
 * -S: Select applet
 * -D: Deselect applet
 * -f <filename>: Applet file
 * -p xxxx: Program FID
 * -c xxxx: Container FID
 * -i n: Instance container size
 * -d n: Instance data size
 * -A xxxxxxxxxx...: AID in hex
 * -a xxxxxxxxxxxxxxxx: Authentication key
 * -s xxxxxxxxxxxxxxxx: Signature key
 * -C xx: Sets CLA byte for Cyberflex Access.
 */

#if HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#if defined( HAVE_LIBDES )
#include <des.h>
#elif defined ( HAVE_LIBCRYPT )
#include <crypt/des.h>
#elif defined ( HAVE_LIBCRYPTO )
#include <openssl/des.h>
#else
#include <openssl/des.h>
#endif /* WITH_LIBDES */
#if 0
#include <sio/sio.h>
#endif
#include <scez/scgeneral.h>
#include <scez/cards/sccyberflex.h>

#ifndef READER_TYPE
#define READER_TYPE SC_READER_DUMBMOUSE
#endif /* READER_TYPE */
#ifndef READER_SLOT
#define READER_SLOT 1
#endif /* READER_SLOT */
#ifndef READER_PORT
#define READER_PORT "0"
#endif /* READER_PORT */

#define CYFLEXMAN_TRANSMIT_BSIZE	0x80

#define CYFLEXMAN_FUNC_UNDEF		0
#define CYFLEXMAN_FUNC_LOAD			1
#define CYFLEXMAN_FUNC_UNLOAD		2
#define CYFLEXMAN_FUNC_SELECT		3
#define CYFLEXMAN_FUNC_DESELECT		4

#define printarray( name, length, array ); \
	printf(name); \
	for( i=0; i<length; i++ ) printf(" %.2X",array[i]); \
	printf("\n");

#define printret( text ); \
	if( ret!=SC_EXIT_OK ) { printf( text ); goto exit; }

#define printretsw( text ); \
	if( (ret!=SC_EXIT_OK) || (ci->sw[0]!=0x90) || (ci->sw[1]!=0x00) ) \
	{ printf( text ); goto exit; }

#define printretswf( text ); \
	if( (ret!=SC_EXIT_OK) || (ci->sw[0]!=0x90) || (ci->sw[1]!=0x00) ) \
	{ printf( text ); return( ret ); }

int cfload( SC_READER_INFO *ri, SC_CARD_INFO *ci, BYTE *authkey,
	BYTE *signkey, char *fname, WORD pfid, WORD cfid, int csize, int dsize,
	BYTE *aid, int aidlen )
{
	des_cblock tmp;
	des_key_schedule ks;
	FILE *fptr;
	BYTE buffer[255], appdata[64*1024+1];
	int ret, resplen, appsize, i, j;

	/* Open file */
	fptr = fopen( fname, "rb" );
	if( fptr==NULL ) {
		printf( "Error: Unable to open file '%s'.\n", fname );
		return( SC_EXIT_UNKNOWN_ERROR );
	}

	/* Read file */
	appsize = fread( appdata, 1, sizeof(appdata), fptr );
	if( (appsize==sizeof(appdata)) || (appsize==0) ) {
		printf( "Error: Unable to read file '%s'.\n", fname );
		return( SC_EXIT_UNKNOWN_ERROR );
	}

	/* Multiple of 8? */
	if( appsize%8 ) {
		printf("Error: File size is not dividable by 8.\n");
		return( SC_EXIT_UNKNOWN_ERROR );
	}

	/* Output file data */
	printf( "File:      %s\nSize:      %d\n", fname, appsize );

	/* Generate signature */
	des_set_key ( (des_cblock *)signkey, ks );
	memset( tmp, 0, sizeof(tmp) );

	for( i=0; i<appsize; i+=8 ) {
		for( j=0; j<sizeof(tmp); j++ ) tmp[j]^=appdata[i+j];

		des_ecb_encrypt( &tmp, &tmp, ks, DES_ENCRYPT );
	}

	/* Output signature */
	printarray( "Signature:", sizeof(tmp), tmp );

	/* Select Default Loader */
	ret = scCyberflexCmdSelect( ri, ci, SC_CYBERFLEX_SELECT_LOADER,
		0, NULL, 0, buffer, &resplen );
	printretswf( "Error selecting default loader.\n");

	/* Select MF */
	ret = scCyberflexCmdSelect( ri, ci, SC_CYBERFLEX_SELECT_FILE,
		0x3F00, NULL, 0, buffer, &resplen );
	printretswf( "Error selecting MF.\n");

	/* Verify auth key */
	ret = scCyberflexCmdVerifyKey( ri, ci, 0, authkey, 8 );
	printretswf( "Error verifying auth key.\n");
   
	/* Create program file */
	ret = scCyberflexCmdCreateFile( ri, ci, pfid, appsize+16,
		SC_CYBERFLEX_FILE_PROGRAM, SC_CYBERFLEX_STATUS_UNBLOCKED,
		0, 0, "\x80\x00\x00\xFF\x00\x00\x00\x00" );
	if( (ret!=SC_EXIT_OK) || (ci->sw[0]!=0x90) || (ci->sw[1]!=0x00) ) {
		printf( "Error creating program file.\n" );
		scCyberflexCmdDeleteFile( ri, ci, pfid );
		return( ret );
	}

	/* Select EF */
	ret = scCyberflexCmdSelect( ri, ci, SC_CYBERFLEX_SELECT_FILE,
		pfid, NULL, 0, buffer, &resplen );
	printretswf( "Error selecting program file.\n");

	/* Update EF */
	i=0;
	while( i<appsize ) {
		resplen = min( appsize-i, CYFLEXMAN_TRANSMIT_BSIZE );
		ret = scCyberflexCmdUpdateBinary( ri, ci, i, appdata+i, resplen );
		printretswf( "Error updateing program file.\n");
		i += resplen;
	}

	/* Validate program */
	ret = scCyberflexCmdManageProgram( ri, ci, tmp, sizeof(tmp) );
	printretswf( "Error validateing program.\n");

	/* Select Default Loader */
	ret = scCyberflexCmdSelect( ri, ci, SC_CYBERFLEX_SELECT_LOADER,
		0, NULL, 0, buffer, &resplen );
	printretswf( "Error selecting default loader.\n");

	/* Execute Install */
	buffer[0] = SC_CYBERFLEX_TYPE_APPLET; /* TODO: Selectable */
	buffer[1] = pfid>>8;
	buffer[2] = pfid&0xFF;
	buffer[3] = csize>>8;
	buffer[4] = csize&0xFF;
	buffer[5] = cfid>>8;
	buffer[6] = cfid&0xFF;
	buffer[7] = dsize>>8;
	buffer[8] = dsize&0xFF;
	buffer[9] = aidlen>>8;
	buffer[10] = aidlen&0xFF;
	memcpy( buffer+11, aid, aidlen );
	resplen = 11+aidlen;
	ret = scCyberflexCmdExecuteMethod( ri, ci, SC_CYBERFLEX_EXECUTE_INSTALL,
		buffer, resplen );
	if( (ret!=SC_EXIT_OK) || (ci->sw[0]!=0x90) || (ci->sw[1]!=0x00) ) {
		printf( "Error executing install.\n" );
		if( ret==SC_EXIT_OK ) printf( "SW: %.2X%.2X\n", ci->sw[0], ci->sw[1] );
		else printf( "ret: %d\n", ret );
		return( ret );
	}

	printf( "\nSuccessfully loaded.\n" );

	return( SC_EXIT_OK );
}

int cfunload( SC_READER_INFO *ri, SC_CARD_INFO *ci, BYTE *authkey,
	WORD pfid, WORD cfid )
{
	BYTE buffer[255];
	int ret, resplen;

	/* Select MF */
	ret = scCyberflexCmdSelect( ri, ci, SC_CYBERFLEX_SELECT_FILE,
		0x3F00, NULL, 0, buffer, &resplen );
	printretswf( "Error selecting MF.\n");

	/* Verify auth key */
	ret = scCyberflexCmdVerifyKey( ri, ci, 0, authkey, 8 );
	printretswf( "Error verifying auth key.\n");
   
	/* Select EF */
	ret = scCyberflexCmdSelect( ri, ci, SC_CYBERFLEX_SELECT_FILE,
		pfid, NULL, 0, buffer, &resplen );
	printretswf( "Error selecting program file.\n");

	/* Reset program */
	ret = scCyberflexCmdManageProgram( ri, ci, NULL, 0 );
	printretswf( "Error reseting program.\n");

	/* Delete program file */
	ret = scCyberflexCmdDeleteFile( ri, ci, pfid );
	printretswf( "Error deleteing program file.\n");

	/* Delete container file */
	ret = scCyberflexCmdDeleteFile( ri, ci, cfid );
	printretswf( "Error deleteing program file.\n");

	printf( "Successfully unloaded.\n" );

	return( SC_EXIT_OK );
}

int cfselect( SC_READER_INFO *ri, SC_CARD_INFO *ci, BYTE *authkey,
	BYTE *aid, int aidlen )
{
	BYTE buffer[255];
	int ret, resplen;

	/* Verify auth key */
	ret = scCyberflexCmdVerifyKey( ri, ci, 0, authkey, 8 );
	printretswf( "Error verifying auth key.\n");

	/* Select application */
	ret = scCyberflexCmdSelect( ri, ci, SC_CYBERFLEX_SELECT_APPL,
		0, aid, aidlen, buffer, &resplen );
	printretswf( "Error selecting application.\n");

	/* Make this permanent */
	ret = scCyberflexCmdManageInstance( ri, ci,
		SC_CYBERFLEX_INSTANCE_INIT_CURRENT );
	printretswf( "Error seting application.\n");

	printf( "Successfully selected.\n" );

	return( SC_EXIT_OK );
}

int cfdeselect( SC_READER_INFO *ri, SC_CARD_INFO *ci )
{
	BYTE buffer[255];
	int ret, resplen;

	/* Select Default Loader */
	ret = scCyberflexCmdSelect( ri, ci, SC_CYBERFLEX_SELECT_LOADER,
		0, NULL, 0, buffer, &resplen );
	printretswf( "Error selecting default loader.\n");

	/* Make this permanent */
	ret = scCyberflexCmdManageInstance( ri, ci,
		SC_CYBERFLEX_INSTANCE_INIT_NONE );
	printretswf( "Error seting application.\n");

	printf( "Successfully deselected.\n" );

	return( SC_EXIT_OK );
}

int main( int argc, char *argv[] )
{
	SC_READER_INFO *ri;
	SC_CARD_INFO *ci;
	SC_READER_CONFIG rc;

	int func=CYFLEXMAN_FUNC_UNDEF;
	char string[20], fname[256];
	int ret, c, i, j;

	/* AID */
	BYTE aid[]={0xD2,0x76,0x00,0x00, 0x92,0x44,0x65,0x66,
		0x61,0x75,0x6C,0x74, 0x00,0x00,0x00,0x00}; /* Default */
	int aidlen=12;

	/* Cyberflex Access Transport Key */
	BYTE authkey[]={0xAD,0x9F,0x61,0xFE,0xFA,0x20,0xCE,0x63};
	BYTE signkey[]={0x6A,0x21,0x36,0xF5,0xD8,0x0C,0x47,0x83};

	WORD cfid=0x1F00;
	WORD pfid=0x1F01;

	int csize=2048;
	int dsize=1024;

	fname[0]=0x00;

	if( scInit() ) { printf("Exit.\n"); exit(1); }

	rc.type=READER_TYPE;
	rc.slot=READER_SLOT;
	rc.param=READER_PORT;

	ret = scReaderGetConfig( argc, argv, &rc );
	if( ret!=SC_EXIT_OK ) {
		printf( "Error getting reader configuration.\n" );
		scEnd();
		exit(1);
	};

	ri = scGeneralNewReader( rc.type, rc.slot );
	if( ri==NULL ) { printf("Exit.\n"); scEnd(); exit(1); };

    ci = scGeneralNewCard( );
	if( ci==NULL ) { printf("Exit.\n"); scEnd(); exit(1); };

	/* Init Reader */
	ret = scReaderInit( ri, rc.param );
	printret( "Error initializing reader.\n" );

	/* Get Card Status */
	ret = scReaderCardStatus( ri );
	printret( "Error geting card status.\n" );
	if( !(ri->status&SC_CARD_STATUS_PRESENT) ) {
		printf("No card inserted.\n");
		goto exit;
	}

	/* Activate Card */
	ret = scReaderActivate( ri );
	printret( "Error activating card.\n" );

	/* Reset Card */
	ret= scReaderResetCard( ri, ci );
	printret( "Error reseting card.\n" );

	/* Get Card Type */
	ret = scSmartcardGetCardType( ci );
	printret( "Error geting card type.\n" );

	if( (ci->type&0xFFFFFF00)!=SC_CARD_CYBERFLEX ) {
		printf("Unsupported card.\n");
		goto exit;
	}

	while( (c=getopt( argc, argv, "LUSDf:p:c:i:d:A:a:s:C:h" ))!=EOF ) {
		switch( c ) {
		case 'L':
			func=CYFLEXMAN_FUNC_LOAD;
			break;
		case 'U':
			func=CYFLEXMAN_FUNC_UNLOAD;
			break;
		case 'S':
			func=CYFLEXMAN_FUNC_SELECT;
			break;
		case 'D':
			func=CYFLEXMAN_FUNC_DESELECT;
			break;
		case 'f':
			if( strlen(optarg)>(sizeof(fname)-1) ) {
				printf("Error: Filename to long.\n");
				goto exit;
			}
			strncpy( fname, optarg, sizeof(fname) );
			break;
		case 'p':
			if( strlen(optarg)!=4 ) {
				printf( "Error: Wrong program FID size.\n" );
				goto exit;
			}
			ret=sscanf( optarg, "%X", &j );
			if( ret!=1 ) {
				printf( "Error: Error in program FID.\n" );
				goto exit;
			}
			pfid=j&0xFFFF;
			break;
		case 'c':
			if( strlen(optarg)!=4 ) {
				printf( "Error: Wrong container FID size.\n" );
				goto exit;
			}
			ret=sscanf( optarg, "%X", &j );
			if( ret!=1 ) {
				printf( "Error: Error in container FID.\n" );
				goto exit;
			}
			cfid=j&0xFFFF;
			break;
		case 'i':
			ret=sscanf( optarg, "%i", &j );
			if( (ret!=1) || (j<0) || (j>0xFFFF) ) {
				printf( "Error: Error in instance container size.\n" );
				goto exit;
			}
			csize=j;
			break;
		case 'd':
			ret=sscanf( optarg, "%i", &j );
			if( (ret!=1) || (j<0) || (j>0xFFFF) ) {
				printf( "Error: Error in instance data size.\n" );
				goto exit;
			}
			dsize=j;
			break;
		case 'A':
			if( (strlen(optarg)<10) || (strlen(optarg)>32) ) {
				printf( "Error: Wrong size for AID.\n" );
				goto exit;
			}
			for(i=0; i<(strlen(optarg)>>1); i++) {
				ret=sscanf( optarg+i+i, "%2X", &j );
				if( ret!=1 ) {
					printf( "Error: Error in AID.\n" );
					goto exit;
				}
				aid[i]=j&0xFF;
			}
			aidlen=i;
			break;
		case 'a':
			if( strlen(optarg)!=16 ) {
				printf( "Error: Wrong size for auth key.\n" );
				goto exit;
			}
			for(i=0; i<(strlen(optarg)>>1); i++) {
				ret=sscanf( optarg+i+i, "%2X", &j );
				if( ret!=1 ) {
					printf( "Error: Error in auth key.\n" );
					goto exit;
				}
				authkey[i]=j&0xFF;
			}
			break;
		case 's':
			if( strlen(optarg)!=16 ) {
				printf( "Error: Wrong size for sign key.\n" );
				goto exit;
			}
			for(i=0; i<(strlen(optarg)>>1); i++) {
				ret=sscanf( optarg+i+i, "%2X", &j );
				if( ret!=1 ) {
					printf( "Error: Error in sign key.\n" );
					goto exit;
				}
				authkey[i]=j&0xFF;
			}
			break;
		case 'C':
			ret=sscanf( optarg, "%X", &j );
			if( (ci->type&0xFFFFFF00)==SC_CARD_CYBERFLEX ) {
				if( (ret!=1) || ((j!=0x00) && (j!=0xF0)) ) {
					printf( "Error: Error in CLA byte.\n" );
					goto exit;
				}
				ci->cla=j&0xFF;
			}
			break;
		case 'h':
			printf("\
Cyberflex Manager\n\
\n\
cyflexman -L|-U|-S|-D -f <filename> [-p xxxx] [-c xxxx] [-i n] [-d n] [-C xx]\n\
          [-A xxxxxxxxxx...] [-a xxxxxxxxxxxxxxxx] [-s xxxxxxxxxxxxxxxx]\n\
\n\
   -L                   Load applet\n\
   -U                   Unload applet\n\
   -S                   Select applet\n\
   -D                   Deselect applet\n\
   -f <filename>        Applet file\n\
   -p xxxx              Program FID\n\
   -c xxxx              Container FID\n\
   -i n                 Instance container size\n\
   -d n                 Instance data size\n\
   -A xxxxxxxxxx...     AID in hex\n\
   -a xxxxxxxxxxxxxxxx  Authentication key\n\
   -s xxxxxxxxxxxxxxxx  Signature key\n\
   -C xx                Set CLA byte\n");
			goto exit;
		default:
			break;
		}
	}

	switch( func ) {
	case CYFLEXMAN_FUNC_LOAD:
		if( !strlen(fname) ) {
			printf("Error: Undefined filename.\n");
			goto exit;
		}

		printf("Function:                Load");
		printf("\nCLA byte:                %.2X",ci->cla);
		printf("\nAuth key:               ");
		for( i=0; i<sizeof(authkey); i++ ) printf(" %.2X",authkey[i]);
		printf("\nSign key:               ");
		for( i=0; i<sizeof(signkey); i++ ) printf(" %.2X",signkey[i]);
		printf("\nApplet file:             %s", fname);
		printf("\nProgram FID:             %.4X", pfid);
		printf("\nContainer FID:           %.4X", cfid);
		printf("\nInstance Container Size: %d", csize);
		printf("\nInstance Data Size:      %d", dsize);
		printf("\nAID:                    ");
		for( i=0; i<aidlen; i++ ) printf(" %.2X",aid[i]);
		printf("\n\n");
		break;
	case CYFLEXMAN_FUNC_UNLOAD:
		printf("Function:                Unload\n");
		printf("\nCLA byte:                %.2X",ci->cla);
		printf("\nAuth key:               ");
		for( i=0; i<sizeof(authkey); i++ ) printf(" %.2X",authkey[i]);
		printf("\nProgram FID:             %.4X", pfid);
		printf("\nContainer FID:           %.4X", cfid);
		printf("\n");
		break;
	case CYFLEXMAN_FUNC_SELECT:
		printf("Function:                Select\n");
		printf("\nCLA byte:                %.2X",ci->cla);
		printf("\nAuth key:               ");
		for( i=0; i<sizeof(authkey); i++ ) printf(" %.2X",authkey[i]);
		printf("\nAID:                    ");
		for( i=0; i<aidlen; i++ ) printf(" %.2X",aid[i]);
		printf("\n\n");
		break;
	case CYFLEXMAN_FUNC_DESELECT:
		printf("Function:                Deselect\n");
		printf("\nCLA byte:                %.2X",ci->cla);
		printf("\n");
		break;
	default:
		printf("Error: Undefined function. Please use one of -L, -U, -S and -D.\n");
		goto exit;
	}

	printf("Press Ctrl-C to Abort or Enter to Continue. ");
	if( !fgets( string, 20, stdin ) ) exit(2);
	printf("\n");

#if 0
SIO_SetLogFile( ri->si, "LogCyflexman.txt" );
#endif

	switch( func ) {
	case CYFLEXMAN_FUNC_LOAD:
		ret=cfload( ri, ci, authkey, signkey, fname, pfid, cfid, csize, dsize,
			aid, aidlen );
		break;
	case CYFLEXMAN_FUNC_UNLOAD:
		ret=cfunload( ri, ci, authkey, pfid, cfid );
		break;
	case CYFLEXMAN_FUNC_SELECT:
		ret=cfselect( ri, ci, authkey, aid, aidlen );
		break;
	case CYFLEXMAN_FUNC_DESELECT:
		ret=cfdeselect( ri, ci );
		break;
	default:
		printf("Error: Undefined function.\n");
		goto exit;
	}

exit:
	ret = scReaderDeactivate( ri );

	ret = scReaderShutdown( ri );

	scGeneralFreeCard( &ci );
	scGeneralFreeReader( &ri );

	scEnd();

	return(0);
}

