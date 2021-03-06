/****************************************************************************
*																			*
*						Copyright Matthias Bruestle 1999					*
*					For licensing, see the file COPYRIGHT					*
*																			*
****************************************************************************/

/* $Id: bcupload.c 1617 2005-11-03 17:41:39Z laforge $ */

/* bcupload file.img|-n [c]
 *
 * file.img: Filename of image file generated by zcbasic or wzcbasic.
 * -n:       Specifies card state. 1: LOAD, 2: TEST, 3: RUN
 */
 
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sio/sio.h>
#include <scez/scgeneral.h>
#include <scez/cards/scbasiccard.h>

#define CHUNK_SIZE 32

#ifndef READER_TYPE
#define READER_TYPE SC_READER_DUMBMOUSE
#endif /* READER_TYPE */
#ifndef READER_SLOT
#define READER_SLOT 1
#endif /* READER_SLOT */
#ifndef READER_PORT
#define READER_PORT "0"
#endif /* READER_PORT */

#define testreturn(text); \
	if(ret!=SC_EXIT_OK){ printf(text); goto exit; }

#define testretsws(text); \
	scSmartcardSimpleProcessSW( ci, &i, NULL ); \
	if( (ret!=SC_EXIT_OK) || (i!=SC_SW_OK) ) \
	{ printf(text); goto exit; }

#define printarray( name, length, array ); \
    printf(name); \
    for( i=0; i<length; i++ ) printf(" %.2X",array[i]); \
    printf("\n");

int f2i( BYTE *ptr, int size )
{
	int i, integer;

	integer=0;
	for( i=0; i<size; i++ ) {
		integer|=*(ptr++)<<(i*8);
	}

	return( integer );
}

int main( int argc, char *argv[] )
{
	SC_READER_INFO *ri;
	SC_CARD_INFO *ci;
	SC_READER_CONFIG rc;
	BYTE zcif[]={'Z','C','I','F'};
	BYTE eepr[]={'E','E','P','R'};
	FILE *file=NULL;
	int fileptr=0;
	BYTE *fileimg;
	int filesize;
	int recnum;		/* Numer of EEPROM records available. */
	int start;
	int length;
	int wrtsize;
	int ret;
	int i;
	BYTE buffer[255];
	BOOLEAN bool1;

	if( argc<2 ) {
		printf("No argument specified.\n");
		exit(1);
	}

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
	if( ri==NULL ) { printf("Memory error.\n"); scEnd(); exit(1); };

    ci = scGeneralNewCard( );
	if( ci==NULL ) { printf("Memory error.\n"); scEnd(); exit(1); };

	/* Init Reader */
	ret = scReaderInit( ri, rc.param );
	if( ret!=SC_EXIT_OK ) {
		printf("Initialization error.\n");
		goto exit;
	}

	/* Get Card Status */
	ret = scReaderCardStatus( ri );
	testreturn("Status error.\n");
	if( !(ri->status&SC_CARD_STATUS_PRESENT) ) {
		printf("No card present.\n");
		goto exit;
	}

	/* Activate Card */
	ret = scReaderActivate( ri );
	testreturn("Card activation error.\n");

#if 0
SIO_SetLogFile( ri->si, "LogBcupload.txt" );
#endif

	/* Reset Card */
	ret=scReaderResetCard( ri, ci );
#if 0
    /* Hae? */
	if( argv[1][0]!='-' ) {
#endif
	if( ret!=SC_EXIT_OK ) {
		testreturn("Card reset error.\n");
	}

	if( argv[1][0]=='-' ) {
		ci->type=SC_CARD_BASICCARD_COMP;

		scBasiccardGetCardData( ci );

		printf("Seting card state: ");

		switch(argv[1][1]) {
			case 0x31:
			case 0x32:
			case 0x33:
				ret = scBasiccardCmdSetState( ri, ci, argv[1][1]&0x0F );
				testretsws("\nCommand error.\n");
				printf("Ok.\n");
				goto exit;
			default:
				printf("Wrong state.\n");
				goto exit;	
		}
	}

	/* Get Card Type */
	ret = scSmartcardGetCardType( ci );
	testreturn("Wrong Card\n");
	if( (ci->type&0xFFFFFF00)!=SC_CARD_BASICCARD )
	{ printf("Wrong Card.\n"); goto exit; }

	if( (file=fopen( argv[1], "rb" ))==NULL ) {
		printf("Error opening file.\n");
		goto exit;	
	}

	if( fseek( file, 0, SEEK_END ) ) {
		printf("Error accessing file.\n");
		goto exit;	
	}

	if( (filesize=ftell( file ))<26 ) {
		printf("File too small.\n");
		goto exit;	
	}

	rewind( file );

	if( filesize>100000 ) {
		printf("File too big.\n");
		goto exit;	
	}

	fileimg=malloc( filesize );

	if( fread( fileimg, 1, filesize, file )!=filesize ) {
		printf("Error reading file.\n");
		goto exit;	
	}

	if( memcmp( zcif, fileimg, 4 ) ) {
		printf("Wrong file format.\n");
		goto exit;	
	}
	fileptr+=4;

	if( (f2i( fileimg+fileptr, 4 )+8) != filesize ) {
		printf("Wrong file format.\n");
		goto exit;	
	}
	fileptr+=4;

	while( memcmp( eepr, fileimg+fileptr, 4 ) ) {
		fileptr+=4;
		fileptr+=4+f2i( fileimg+fileptr, 4 );

		if( (fileptr+4)>filesize ) {
			printf("Wrong file format.\n");
			goto exit;	
		}
	}

	fileptr+=12;

	recnum=f2i( fileimg+fileptr, 2 );
	fileptr+=2;
	printf("Number of EEPROM records: %d\n",recnum);

	/* State */
	ret = scBasiccardCmdGetState( ri, ci, buffer, &bool1 );
	testretsws("Command error.\n");

	if( buffer[0]!=SC_BASICCARD_STATE_LOAD ) {
		printf("Card is not in LOAD state.\n");
		goto exit;
	}

	printf("Writing data ");

	while( recnum-- ) {
		start=f2i( fileimg+fileptr, 2 );
		fileptr+=2;

		length=f2i( fileimg+fileptr, 2 );
		fileptr+=2;

#if 0
		printf( "[start: %.4X, length: %d]", start, length );
#endif

		while( length ) {
			wrtsize=min(length,CHUNK_SIZE);

			printf("+");
			fflush(stdout);

			ret = scBasiccardCmdWriteEeprom( ri, ci, start, fileimg+fileptr,
				wrtsize );
			testretsws("\nCommand error.\n");
#if 0
			printf( "[fileptr: %d, start: %.4X, length: %d]", fileptr,
				start, wrtsize );
			printarray( " ", wrtsize, (fileimg+fileptr) );
#endif

			fileptr+=wrtsize;
			start+=wrtsize;
			length-=wrtsize;
		}
	}

	printf("\n");

exit:
	if( file!=NULL ) fclose( file );

	scReaderDeactivate( ri );
	scReaderShutdown( ri );

	scGeneralFreeCard( &ci );
	scGeneralFreeReader( &ri );

	scEnd();

	return(0);
}


