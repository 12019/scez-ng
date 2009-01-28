/****************************************************************************
*																			*
*					Copyright Matthias Bruestle 1999,2000					*
*					For licensing, see the file COPYRIGHT					*
*																			*
****************************************************************************/

/* $Id: getcert.c 1617 2005-11-03 17:41:39Z laforge $ */

/*
 * Reads certificates and public key files from a Telesec SigG card.
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <scez/scgeneral.h>
#include <scez/cards/sctcos.h>

#ifndef READER_TYPE
#define READER_TYPE SC_READER_DUMBMOUSE
#endif /* READER_TYPE */
#ifndef READER_SLOT
#define READER_SLOT 1
#endif /* READER_SLOT */
#ifndef READER_PORT
#define READER_PORT "0"
#endif /* READER_PORT */

#define checkreturn(f); if( ret!=SC_EXIT_OK ) { printf(f); break; }

#define printarray( name, length, array ); \
    printf(name); \
    for( i=0; i<length; i++ ) printf(" %.2X",array[i]); \
    printf("\n");

int main( int argc, char *argv[] )
{
	SC_READER_INFO *ri;
	SC_CARD_INFO *ci;
	SC_READER_CONFIG rc;

	FILE *file;

	char *aid;
	int aidlen;

	BYTE rbuffer[256];
	int resplen;
	int ret=0;

	int i, pointer, size;

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
		if( (ci->type&0xFFFFFF00)!=SC_CARD_TCOS )
		{ printf("Error: Wrong Card.\n"); break; }

		if( ci->type==SC_CARD_TCOS_44 ) {
			aid="\xD2\x76\x00\x00\x66\x01";
			aidlen=6;
		} else {
			aid="\xD2\x76\x00\x00\x03\x01\x02";
			aidlen=7;
		}
		/* New card has same ATR as NetKey card, so it must be
		 * distinguished by EF_DIR.
		 */
		aid="\xD2\x76\x00\x00\x66\x01";
		aidlen=6;

		/* Select AID */
		ret = scTcosCmdSelect( ri, ci, SC_TCOS_SELECT_AID, 0, aid, aidlen,
			SC_TCOS_RDATA_FCP, rbuffer, &resplen );
		if( !( (ret==SC_EXIT_OK) && (ci->sw[0]==0x90) && (ci->sw[1]==0x00) ) )
		{ printf("Error selecting application.\n"); break; }

		/* Select EF */
		ret = scTcosCmdSelect( ri, ci, SC_TCOS_SELECT_EF, 0xC000, NULL, 0,
			SC_TCOS_RDATA_FCP, rbuffer, &resplen );
		if( !( (ret==SC_EXIT_OK) && (ci->sw[0]==0x90) && (ci->sw[1]==0x00) ) )
		{ printf("Error selecting signature key certificate.\n"); break; }

		i=2;
		size=0;
		while( i<resplen ) {
			if( rbuffer[i]==0x81 ) {
				i+=2;
				size=rbuffer[i];
				size<<=8;
				i++;
				size+=rbuffer[i];
				break;
			} else {
				i++;
				i+=rbuffer[i];
				i++;
			}
		}

		if( size ) {
			printf("Signature Key Certificate (%d bytes)\n",size);

			file=fopen( "sign.crt", "wb" );

			pointer=0;
			while( pointer<size ) {
				resplen = min( 200, size-pointer);
				ret = scTcosCmdReadBinary( ri, ci, pointer, rbuffer,
					&resplen );
				if( !( (ret==SC_EXIT_OK) && (ci->sw[0]==0x90) &&
					(ci->sw[1]==0x00) ) )
					{ printf("Error reading data.\n"); break; }
				pointer+=resplen;
				fwrite( rbuffer, 1, resplen, file );
			}

			fclose( file );
		}

		/* Select AID */
		ret = scTcosCmdSelect( ri, ci, SC_TCOS_SELECT_AID, 0, aid, aidlen,
			SC_TCOS_RDATA_FCP, rbuffer, &resplen );
		if( !( (ret==SC_EXIT_OK) && (ci->sw[0]==0x90) && (ci->sw[1]==0x00) ) )
		{ printf("Error selecting application.\n"); break; }

		/* Select EF */
		ret = scTcosCmdSelect( ri, ci, SC_TCOS_SELECT_EF, 0xB000, NULL, 0,
			SC_TCOS_RDATA_FCP, rbuffer, &resplen );
		if( !( (ret==SC_EXIT_OK) && (ci->sw[0]==0x90) && (ci->sw[1]==0x00) ) )
		{ printf("Error selecting root CA key.\n"); break; }

		i=2;
		size=0;
		while( i<resplen ) {
			if( rbuffer[i]==0x81 ) {
				i+=2;
				size=rbuffer[i];
				size<<=8;
				i++;
				size+=rbuffer[i];
				break;
			} else {
				i++;
				i+=rbuffer[i];
				i++;
			}
		}

		if( size ) {
			printf("Root CA Key (%d bytes)\n",size);

			file=fopen( "rootca.crt", "wb" );

			pointer=0;
			while( pointer<size ) {
				resplen = min( 200, size-pointer);
				ret = scTcosCmdReadBinary( ri, ci, pointer, rbuffer,
					&resplen );
				if( !( (ret==SC_EXIT_OK) && (ci->sw[0]==0x90) &&
					(ci->sw[1]==0x00) ) )
					{ printf("Error reading data.\n"); break; }
				pointer+=resplen;
				fwrite( rbuffer, 1, resplen, file );
			}

			fclose( file );
		}

		/* Select AID */
		ret = scTcosCmdSelect( ri, ci, SC_TCOS_SELECT_AID, 0, aid, aidlen,
			SC_TCOS_RDATA_FCP, rbuffer, &resplen );
		if( !( (ret==SC_EXIT_OK) && (ci->sw[0]==0x90) && (ci->sw[1]==0x00) ) )
		{ printf("Error selecting application.\n"); break; }

		/* Select EF */
		ret = scTcosCmdSelect( ri, ci, SC_TCOS_SELECT_EF, 0xC200, NULL, 0,
			SC_TCOS_RDATA_FCP, rbuffer, &resplen );
		if( !( (ret==SC_EXIT_OK) && (ci->sw[0]==0x90) && (ci->sw[1]==0x00) ) )
		{ printf("Error selecting encryption key certificate.\n"); break; }

		i=2;
		size=0;
		while( i<resplen ) {
			if( rbuffer[i]==0x81 ) {
				i+=2;
				size=rbuffer[i];
				size<<=8;
				i++;
				size+=rbuffer[i];
				break;
			} else {
				i++;
				i+=rbuffer[i];
				i++;
			}
		}

		if( size ) {
			printf("Encryption Key Certificate (%d bytes)\n",size);

			file=fopen( "encr.crt", "wb" );

			pointer=0;
			while( pointer<size ) {
				resplen = min( 200, size-pointer);
				ret = scTcosCmdReadBinary( ri, ci, pointer, rbuffer,
					&resplen );
				if( !( (ret==SC_EXIT_OK) && (ci->sw[0]==0x90) &&
					(ci->sw[1]==0x00) ) )
					{ printf("Error reading data.\n"); break; }
				pointer+=resplen;
				fwrite( rbuffer, 1, resplen, file );
			}

			fclose( file );
		}

	} while( 0 );

	ret = scReaderDeactivate( ri );
	if( ret!=SC_EXIT_OK ) printf("Error: scReaderDeactivate\n");

	ret = scReaderShutdown( ri );
	if( ret!=SC_EXIT_OK ) printf("Error: scReaderShutdown\n");

	scGeneralFreeCard( &ci );
	scGeneralFreeReader( &ri );

	scEnd();

	return(0);
}

