/****************************************************************************
*																			*
*						SCEZ chipcard library - flexpasswd					*
*					Copyright Matthias Bruestle 1999,2000					*
*					For licensing, see the file COPYRIGHT					*
*																			*
****************************************************************************/

/* $Id: flexpasswd.c 1617 2005-11-03 17:41:39Z laforge $ */

/* -a xxxxxxxxxxxxxxxx: Authentication key
 * -e n: Index of authentication key
 * -n xxxxxxxxxxxxxxxx: New key
 * -i n: Index of new key
 * -d xxxx: DF path of keyfile. Must begin with 3F00.
 * -c xx: Sets CLA byte for Cyberflex Access.
 * -I: Write Int Auth key instead of Ext Auth key.
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <scez/scgeneral.h>
#include <scez/cards/sccryptoflex.h>
#include <scez/cards/sccyberflex.h>
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

#define printarray( name, length, array ); \
	printf(name); \
	for( i=0; i<length; i++ ) printf(" %.2X",array[i]); \
	printf("\n");

#define printret( text ); \
	if( ret!=SC_EXIT_OK ) { printf( text ); goto exit; }

#define printretsw( text ); \
	if( (ret!=SC_EXIT_OK) || (ci->sw[0]!=0x90) || (ci->sw[1]!=0x00) ) \
	{ printf( text ); goto exit; }

int main( int argc, char *argv[] )
{
	SC_READER_INFO *ri;
	SC_CARD_INFO *ci;
	SC_READER_CONFIG rc;

	BYTE buffer[255];
	BOOLEAN doext=TRUE;
	char string[20];
	int resplen, ret, aindex=1, index=1, c, i, j;

	/* Cryptoflex/Multiflex Transport Key */
	BYTE authkey[]={0x47,0x46,0x58,0x49,0x32,0x56,0x78,0x40};
	/* Aladdin CC3 Transport Key */
	/* BYTE authkey[]={0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF}; */
	/* Aladdin CC4 Transport Key */
	/* BYTE authkey[]={0x2C,0x15,0xE5,0x26,0xE9,0x3E,0x8A,0x19}; */
	/* Cyberflex Access Transport Key */
	/* BYTE authkey[]={0xAD,0x9F,0x61,0xFE,0xFA,0x20,0xCE,0x63}; */
	/* Cryptoflex for Windows 2000 */
	/* BYTE authkey[]={0x4D,0x49,0x43,0x4E,0x4A,0x53,0x46,0x54}; */

	BYTE newkey[]={0x47,0x46,0x58,0x49,0x32,0x56,0x78,0x40};
	WORD dir[4]={0x3F00,0xFFFF,0xFFFF,0xFFFF}; /* Last word is an end marker */

	printf("Beware, this is on your own risk. Changing the administration");
	printf(" key\nis a very good way to make a smart card unusable!\n\n");

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

	switch( ci->type&0xFFFFFF00 ) {
	case SC_CARD_CRYPTOFLEX:
		printf("Card:       Cryptoflex\n");
		break;
	case SC_CARD_CYBERFLEX:
		memcpy( authkey, "\xAD\x9F\x61\xFE\xFA\x20\xCE\x63", 8 );
		memcpy( newkey, "\xAD\x9F\x61\xFE\xFA\x20\xCE\x63", 8 );
		index=0;
		aindex=0;
		printf("Card:       Cyberflex\n");
		break;
	case SC_CARD_MULTIFLEX:
		printf("Card:       Multiflex\n");
		break;
	default:
		printf("Card:       Unsupported\n");
		goto exit;
	}

	optind=0;

	while( (c=getopt( argc, argv, "a:n:i:e:d:c:I" ))!=EOF ) {
		switch( c ) {
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
		case 'n':
			if( strlen(optarg)!=16 ) {
				printf( "Error: Wrong size for new key.\n" );
				goto exit;
			}
			for(i=0; i<(strlen(optarg)>>1); i++) {
				ret=sscanf( optarg+i+i, "%2X", &j );
				if( ret!=1 ) {
					printf( "Error: Error in new key.\n" );
					goto exit;
				}
				newkey[i]=j&0xFF;
			}
			break;
		case 'i':
			ret=sscanf( optarg, "%i", &j );
			if( (ci->type&0xFFFFFF00)==SC_CARD_CYBERFLEX ) {
				if( (ret!=1) || (j<0) || (j>5) ) {
					printf( "Error: Error in Retry Counter.\n" );
					goto exit;
				}
			} else {
				if( (ret!=1) || (j<0) || (j>2) ) {
					printf( "Error: Error in Retry Counter.\n" );
					goto exit;
				}
			}
			index=j;
			break;
		case 'e':
			ret=sscanf( optarg, "%i", &j );
			if( (ci->type&0xFFFFFF00)==SC_CARD_CYBERFLEX ) {
				if( (ret!=1) || (j<0) || (j>5) ) {
					printf( "Error: Error in Retry Counter.\n" );
					goto exit;
				}
			} else {
				if( (ret!=1) || (j<0) || (j>2) ) {
					printf( "Error: Error in Retry Counter.\n" );
					goto exit;
				}
			}
			aindex=j;
			break;
		case 'd':
			if( (strlen(optarg)>12) || (strlen(optarg)&3) ) {
				printf( "Error: Wrong size for DF path.\n" );
				goto exit;
			}
			for(i=0; i<(strlen(optarg)>>2); i++) {
				ret=sscanf( optarg+(i<<2), "%4X", &j );
				if( ret!=1 ) {
					printf( "Error: Error in DF path.\n" );
					goto exit;
				}
				dir[i]=j&0xFFFF;
			}
			break;
		case 'c':
			ret=sscanf( optarg, "%X", &j );
			if( (ci->type&0xFFFFFF00)==SC_CARD_CYBERFLEX ) {
				if( (ret!=1) || ((j!=0x00) && (j!=0xF0)) ) {
					printf( "Error: Error in CLA byte.\n" );
					goto exit;
				}
				ci->cla=j&0xFF;
			}
			break;
		case 'I':
			if( (ci->type&0xFFFFFF00)==SC_CARD_CYBERFLEX ) {
				printf( "Error: Int Auth not supported with the Cyberflex.\n" );
				goto exit;
			}
			doext=FALSE;
			break;
		default:
			break;
		}
	}

	if( (ci->type&0xFFFFFF00)==SC_CARD_CYBERFLEX )
		printf("CLA byte:   %.2X\n",ci->cla);
	printf("Auth key:  ");
	for( i=0; i<sizeof(authkey); i++ ) printf(" %.2X",authkey[i]);
	printf(" (");
	for( i=0; i<sizeof(authkey); i++ ) printf("%c",authkey[i]);
	printf(")\nAuth index: %d", aindex);
	printf("\nNew key:   ");
	for( i=0; i<sizeof(newkey); i++ ) printf(" %.2X",newkey[i]);
	printf(" (");
	for( i=0; i<sizeof(newkey); i++ ) printf("%c",newkey[i]);
	printf(")\nNew index:  %d", index);
	if( !doext ) printf(" Int");
	if( (ci->type&0xFFFFFF00)==SC_CARD_CYBERFLEX ) {
		switch( index ) {
		case 0:
			printf(" (Probably AUT 0)");
			break;
		case 1:
			printf(" (Probably AUT 2)");
			break;
		case 2:
			printf(" (Probably AUT 1)");
			break;
		case 3:
			printf(" (Probably AUT 5)");
			break;
		default:
			break;
		}
	}
	printf("\nDF path:   ");
	i=0;
	while( dir[i]!=0xFFFF ) printf(" %.4X",dir[i++]);
	printf("\n\n");

	printf("Press Ctrl-C to Abort or Enter to Continue. ");
	if( !fgets( string, 20, stdin ) ) exit(2);
	printf("\n");

	if( (ci->type&0xFFFFFF00)==SC_CARD_MULTIFLEX ) {

		i=0;
		while( dir[i]!=0xFFFF ) {
			/* Select */
			ret = scMultiflexCmdSelectFile( ri, ci, dir[i], buffer, &resplen );
			if( (ret!=SC_EXIT_OK) || (ci->sw[0]!=0x90) || (ci->sw[1]!=0x00) ) {
				printf( "Error selecting FID %.4X\n", dir[i] );
				goto exit;
			}
			i++;
		}

		/* Verify transport key */
		ret = scMultiflexCmdVerifyKey( ri, ci, aindex, authkey, 8 );
		printretsw( "Error verifying auth key.\n");

		/* Select EF */
		if( doext ) {   
			ret = scMultiflexCmdSelectFile( ri, ci, 0x0011, buffer, &resplen );
		} else {
			ret = scMultiflexCmdSelectFile( ri, ci, 0x0001, buffer, &resplen );
		}
		printretsw( "Error selecting key file.\n");
 
		/* Write */
		ret = scMultiflexCmdUpdateBinary( ri, ci, 3+(index*12), newkey, 8 );
		printretsw( "Error updating key file.\n");

	} else if( (ci->type&0xFFFFFF00)==SC_CARD_CRYPTOFLEX ) {

#if 0
/* Verify transport key */
ret = scCryptoflexCmdVerifyKey( ri, ci, 1, authkey, 8 );
printretsw( "Error verifying auth key.\n");

/* Select */
ret = scCryptoflexCmdSelectFile( ri, ci, 0x2000, buffer, &resplen );
printretsw( "Error selecting DF.\n");

ret = scCryptoflexCmdCreateFile( ri, ci, 0x0011, 26, 1, 0x00, 1, 0, 0,
	"\x3F\x44\xFF\x44", "\x11\xFF\x11" );
printretsw( "Error creating key file.\n");
 
/* Select */
ret = scCryptoflexCmdSelectFile( ri, ci, 0x0011, buffer, &resplen );
printretsw( "Error selecting key file.\n");
 
/* Write */
ret = scCryptoflexCmdUpdateBinary( ri, ci, 0, "\xFF\x08\x00\x11\x22\x33\x44\x55\x66\x77\x88\x03\x03\x08\x00\x47\x46\x58\x49\x32\x56\x78\x40\x03\x03\x00", 26 );
printretsw( "Error updating key file.\n");

goto exit;
#endif

		i=0;
		while( dir[i]!=0xFFFF ) {
			/* Select */
			ret = scCryptoflexCmdSelectFile( ri, ci, dir[i], buffer, &resplen );
			if( (ret!=SC_EXIT_OK) || (ci->sw[0]!=0x90) || (ci->sw[1]!=0x00) ) {
				printf( "Error selecting FID %.4X\n", dir[i] );
				goto exit;
			}
			i++;
		}

		/* Verify transport key */
		ret = scCryptoflexCmdVerifyKey( ri, ci, aindex, authkey, 8 );
		printretsw( "Error verifying auth key.\n");
   
		/* Select EF */
		if( doext ) {   
			ret = scCryptoflexCmdSelectFile( ri, ci, 0x0011, buffer, &resplen );
		} else {
			ret = scCryptoflexCmdSelectFile( ri, ci, 0x0001, buffer, &resplen );
		}
		printretsw( "Error selecting key file.\n");
   
		/* Write */
		ret = scCryptoflexCmdUpdateBinary( ri, ci, 3+(index*12), newkey, 8 );
		printretsw( "Error updating key file.\n");

	} else if( (ci->type&0xFFFFFF00)==SC_CARD_CYBERFLEX ) {

		i=0;
		while( dir[i]!=0xFFFF ) {
			/* Select */
			ret = scCyberflexCmdSelect( ri, ci, SC_CYBERFLEX_SELECT_FILE,
				dir[i], NULL, 0, buffer, &resplen );
			if( (ret!=SC_EXIT_OK) || (ci->sw[0]!=0x90) || (ci->sw[1]!=0x00) ) {
				printf( "Error selecting FID %.4X\n", dir[i] );
				if( (ci->sw[0]==0x6E) || (ci->sw[0]==0x6D) )
					printf("Probably wrong CLA byte specified.\n");
				goto exit;
			}
			i++;
		}

		/* Verify auth key */
		ret = scCyberflexCmdVerifyKey( ri, ci, aindex, authkey, 8 );
		printretsw( "Error verifying auth key.\n");
   
		/* Select EF */
		ret = scCyberflexCmdSelect( ri, ci, SC_CYBERFLEX_SELECT_FILE,
			0x0011, NULL, 0, buffer, &resplen );
		printretsw( "Error selecting key file.\n");
 
		/* Write */
		ret = scCyberflexCmdUpdateBinary( ri, ci, 4+(index*14), newkey, 8 );
		printretsw( "Error updating key file.\n");

	} else {

		printf("No supported card inserted.\n");
		goto exit;

	}

	printf("Key written.\n");

exit:
	ret = scReaderDeactivate( ri );

	ret = scReaderShutdown( ri );

	scGeneralFreeCard( &ci );
	scGeneralFreeReader( &ri );

	scEnd();

	return(0);
}

