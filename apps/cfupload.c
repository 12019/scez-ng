/****************************************************************************
*																			*
*						Copyright Matthias Bruestle 2000					*
*					For licensing, see the file COPYRIGHT					*
*																			*
****************************************************************************/

/* $Id: cfupload.c 1617 2005-11-03 17:41:39Z laforge $ */

#include <getopt.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
/* #include <unistd.h> */
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

#define checkreturn(f); if( ret!=SC_EXIT_OK ) { printf(f); break; }

#define printarray( name, length, array ); \
	printf(name); \
	for( i=0; i<length; i++ ) printf(" %.2X",array[i]); \
	printf("\n");

void usage(char *argv) {
    printf("Cyberflex file loader\n\n");
    printf("Example (reads CHV1 (PIN) file 0000 in root and stores it in /tmp/0000:\n");
    printf("\t;%s -r -p 3f000000 -f /tmp/0000\n\n", argv);
    printf("Usage:      %s <options>\n"\
	    "    Options:\n"\
	    "   -a <8 bytes in hex (16 chars)> - ACL\n"\
        "   -C <1 byte in hex (2 chars)> - CLA byte\n"\
	    "   -d <directory size> - Create DF (Directory file)\n"\
	    "   -e - Erase file\n"\
	    "   -f <filename> - host file\n"\
	    "   -h - this help text\n"\
	    "   -k <8 bytes in hex (16 chars)> - Administration key (AUT0) in hex\n"\
	    "   -K <8 characters> - Administration key (AUT0) as a string\n"\
	    "   -p <path in hex> - Path of the file to create/write/read. Must start\n"\
	    "                      with the master file 3F00.\n"\
	    "   -r - Read from card\n"\
	    "   WARNING: If you enter the wrong AUT0 enough times, the card will lock up!\n",
	    argv);
    exit(0);
}


int main( int argc, char *argv[] )
{
	SC_READER_INFO *ri;
	SC_CARD_INFO *ci;
	SC_READER_CONFIG rc;

	int c, i, j, ptr, ret;
	BYTE buffer[256];
	int resplen;

	/* Options */
	BYTE acl[]={0x00,0x00,0x00,0xFF,0x00,0x00,0x00,0x00};
	BYTE cla=0x00;
	BOOLEAN createdf=FALSE;
	int dsize=0;
	BOOLEAN erase=FALSE;
	BYTE fname[1024];
	BOOLEAN validfname=FALSE;
	int fsize=0;
	BYTE authkey[8];	/* Administration key */
	BOOLEAN validkey=FALSE;
	BYTE path[8];
	int pathlen=0;
	BOOLEAN validpath=FALSE;
	BOOLEAN readfile=FALSE;

#if 0
	BYTE defkey[]={0xAD,0x9F,0x61,0xFE,0xFA,0x20,0xCE,0x63};
#endif

	FILE *f=NULL;

	/* a: ACL.
     * C: CLA byte.
	 * d: Create DF.
	 * e: Erase file.
	 * f: Host file.
	 * k: Administration key in hex.
	 * K: Administration key as string.
	 * p: Path of the file to creat/write. This must begin with the MF.
	 * r: Read from card.
	 *
	 * TODO:
	 * n: Number of records.
	 * r: Record size.
	 * t: File type. (Instead of 'd'.)
	 */
	while( (c=getopt( argc, argv, "a:C:d:ef:k:K:p:r" ))!=EOF ) {
		switch( c ) {
		case 'a':
			if( strlen(optarg)!=16 ) {
				printf( "Error: Wrong size for ACL.\n" );
				return(1);
			}
			for(i=0; i<(strlen(optarg)>>1); i++) {
				ret=sscanf( optarg+i+i, "%2X", &j );
				if( ret!=1 ) {
					printf( "Error: Error in ACL.\n" );
					return(1);
				}
				acl[i]=j&0xFF;
			}
			break;
		case 'C':
			ret=sscanf( optarg, "%X", &j );
			if( (ret!=1) || ((j!=0x00) && (j!=0xF0)) ) {
				printf( "Error: Error in CLA byte.\n" );
				return(1);
			}
			cla=j&0xFF;
			break;
		case 'd':
			createdf=TRUE;
			ret=sscanf( optarg, "%d", &i );
			if( ret!=1 ) {
				printf( "Error: Error in directory size.\n" );
				return(1);
			}
			dsize=i;
			break;
		case 'e':
			erase=TRUE;
			break;
		case 'f':
			if( strlen(optarg)>(sizeof(fname)-1) ) {
				printf("Error: Filename to long.\n");
				return(1);
			}
			strncpy( fname, optarg, sizeof(fname) );
			validfname=TRUE;
			break;
		case 'k':
			if( strlen(optarg)!=16 ) {
				printf( "Error: Wrong size for administration key.\n" );
				return(1);
			}
			for(i=0; i<(strlen(optarg)>>1); i++) {
				ret=sscanf( optarg+i+i, "%2X", &j );
				if( ret!=1 ) {
					printf( "Error: Error in administration key.\n" );
					return(1);
				}
				authkey[i]=j&0xFF;
			}
			validkey=TRUE;
			break;
		case 'K':
			if( strlen(optarg)<(sizeof(authkey)) ) {
				printf("Error: Adiminstration key to short.\n");
				return(1);
			}
			memcpy( authkey, optarg, sizeof(authkey) );
			validkey=TRUE;
			break;
		case 'p':
			if( strlen(optarg)&3 ) {
				printf( "Error: Pathlength is not divisable by 4.\n" );
				return(1);
			}
			if( strlen(optarg)>16 ) {
				printf( "Error: Path to long..\n" );
				return(1);
			}
			for(i=0; i<(strlen(optarg)>>1); i++) {
				ret=sscanf( optarg+i+i, "%2X", &j );
				if( ret!=1 ) {
					printf( "Error: Error in path.\n" );
					return(1);
				}
				path[i]=j&0xFF;
			}
			pathlen=i;
			validpath=TRUE;
			break;
		case 'r':
			readfile=TRUE;
			break;
		case 'h':
		default:
			break;
		}
	}

	if( !createdf && !erase && !validfname ) {
		printf( "Error: Filename not specified.\n" );
		return(1);
	}

	if( !validpath ) {
		printf( "Error: Path not specified.\n" );
		return(1);
	}

	if( memcmp( path, (BYTE *)"\x3F\x00", 2 ) ) {
		printf( "Error: Path not specified.\n" );
		return(1);
	}

	if( !validkey ) {
		printf( "Error: Administration key not specified.\n" );
		return(1);
	}

	if( !createdf && !erase && !readfile ) {
		if( (f=fopen( fname, "rb" ))==NULL ) {
			printf( "Error: Unable to open input file.\n" );
			return(2);
		}
		if( fseek( f, 0, SEEK_END )==-1 ) {
			printf( "Error: Unable operate on file.\n" );
			return(2);
		}
		if( (fsize = ftell( f ))==-1 ) {
			printf( "Error: Unable operate on file.\n" );
			return(2);
		}
		rewind( f );
		if( fsize>65000 ) {
			printf( "Error: File to big.\n" );
			return(2);
		}
	}

	if( !createdf && !erase && readfile ) {
		if( (f=fopen( fname, "wb" ))==NULL ) {
			printf( "Error: Unable to open output file.\n" );
			return(2);
		}
	}

#if 0
memcpy( authkey, defkey, 8 );

printarray("path:",8,path);
printf("pathlen: %d\n",pathlen);
printf("dsize: %d\n",dsize);
printf("fname: %s\n",fname);
printf("fsize: %d\n",fsize);
printarray("authkey:",8,authkey);
printf("autkey: %s\n",authkey);
printf("readfile: %d\n",readfile);
#endif

	do {

		if( scInit() ) { printf("Error: scInit\n"); return(3); }

		rc.type=READER_TYPE;
		rc.slot=READER_SLOT;
		rc.param=READER_PORT;

		ret = scReaderGetConfig( argc, argv, &rc );
		if( ret!=SC_EXIT_OK ) {
			printf( "Error: Error getting reader configuration.\n" );
			scEnd();
			return(3);
		};

		ri = scGeneralNewReader( rc.type, rc.slot );
		if( ri==NULL ) {
			printf("Error: scGeneralNewReader\n"); scEnd(); return(3);
		}

		ci = scGeneralNewCard( );
		if( ci==NULL ) {
			printf("Error: scGeneralNewCard\n"); scEnd(); return(3);
		}

		/* Init Reader */
		ret = scReaderInit( ri, rc.param );
		checkreturn("Error: scReaderInit\n");

		/* Activate Card */
		ret = scReaderActivate( ri );
		checkreturn("Error: scReaderActivate\n");

		/* Get Card Status */
		ret = scReaderCardStatus( ri );
		checkreturn("Error: scReaderCardStatus\n");
		if( !(ri->status&SC_CARD_STATUS_PRESENT) )
		{ printf("Error: No Card.\n"); break; }

		/* Reset Card */
		ret= scReaderResetCard( ri, ci );
		checkreturn("Error: scReaderResetCard\n");

		/* Get Card Type */
		ret = scSmartcardGetCardType( ci );
		checkreturn("Error: scReaderGetCardType\n");
		if( (ci->type&0xFFFFFF00)!=SC_CARD_CYBERFLEX )
		{ printf("Error: Wrong Card.\n"); break; }

		ci->cla=cla;

#if 0
		SIO_SetLogFile( ri->si, "LogCfupload.txt" );
#endif

		ret = scCyberflexCmdVerifyKey( ri, ci, 0, authkey, 8 );
		scSmartcardSimpleProcessSW( ci, &i, NULL );
		if( (ret!=SC_EXIT_OK) || (i!=SC_SW_OK) ) {
			printf( "Error: Administration key rejected.\n" );
			ret=4;
			break;
		}

		for( i=0; i<pathlen-2; i+=2 ) {
			ret = scCyberflexCmdSelect( ri, ci, SC_CYBERFLEX_SELECT_FILE,
				(path[i]<<8)+path[i+1], NULL, 0, buffer, &resplen );
			scSmartcardSimpleProcessSW( ci, &j, NULL );
			if( (ret!=SC_EXIT_OK) || (j!=SC_SW_OK) ) {
				printf( "Error: Unable to select file %.2X%.2X.\n", path[i],
					path[i+1] );
				ret=4;
				break;
			}
		}

		if( erase ) {
			ret = scCyberflexCmdDeleteFile( ri, ci, (path[i]<<8)+path[i+1] );
			scSmartcardSimpleProcessSW( ci, &i, NULL );
			if( (ret!=SC_EXIT_OK) || (i!=SC_SW_OK) ) {
				printf( "Error: Unable to delete file %.2X%.2X.\n", path[i],
					path[i+1] );
				ret=4;
				break;
			}

			ret=0;
			break;
		}

		if( createdf ) {
			ret = scCyberflexCmdCreateFile( ri, ci, (path[i]<<8)+path[i+1],
				dsize, SC_CYBERFLEX_FILE_DIRECTORY,
				SC_CYBERFLEX_STATUS_UNBLOCKED, 0, 0,
				(BYTE *)"\x00\x00\x00\xFF\x00\x00\x00\x00" );
			scSmartcardSimpleProcessSW( ci, &i, NULL );
			if( (ret!=SC_EXIT_OK) || (i!=SC_SW_OK) ) {
				printf( "Error: Unable to create directory %.2X%.2X.\n",
					path[i], path[i+1] );
				ret=4;
				break;
			}

			ret=0;
			break;
		}

		if( readfile ) {
			ret = scCyberflexCmdSelect( ri, ci, SC_CYBERFLEX_SELECT_FILE,
				(path[i]<<8)+path[i+1], NULL, 0, buffer, &resplen );
			scSmartcardSimpleProcessSW( ci, &j, NULL );
			if( (ret!=SC_EXIT_OK) || (j!=SC_SW_OK) ) {
				printf( "Error: Unable to select file %.2X%.2X.\n", path[i],
					path[i+1] );
				ret=4;
				break;
			}

			fsize=(buffer[2]<<8)+buffer[3];

			ptr=0;
			while( ptr<fsize ) {
				j=128;
				if( fsize-ptr<128 ) j=fsize-ptr; /* Use trinary operator. */
				ret = scCyberflexCmdReadBinary( ri, ci, ptr, buffer, &j );
				scSmartcardSimpleProcessSW( ci, &i, NULL );
				if( (ret!=SC_EXIT_OK) || (i!=SC_SW_OK) ) {
					printf( "Error: Unable to read from file.\n" );
					ret=4;
					break;
				}
				if( fwrite( buffer, 1, j, f )!=j ) {
					printf( "Error: Unable to write output to host file.\n" );
					ret=4;
					break;
				}
				ptr+=j;
			}

			ret=0;
			break;
		}

		ret = scCyberflexCmdCreateFile( ri, ci, (path[i]<<8)+path[i+1],
			fsize+16, SC_CYBERFLEX_FILE_BINARY, SC_CYBERFLEX_STATUS_UNBLOCKED,
			0, 0, (BYTE *)"\x00\x00\x00\xFF\x00\x00\x00\x00" );
		scSmartcardSimpleProcessSW( ci, &j, NULL );
		if( (ret!=SC_EXIT_OK) || (j!=SC_SW_OK) ) {
			printf( "Error: Unable to create file %.2X%.2X.\n", path[i],
				path[i+1] );
			ret=4;
			break;
		}

		ret = scCyberflexCmdSelect( ri, ci, SC_CYBERFLEX_SELECT_FILE,
			(path[i]<<8)+path[i+1], NULL, 0, buffer, &resplen );
		scSmartcardSimpleProcessSW( ci, &j, NULL );
		if( (ret!=SC_EXIT_OK) || (j!=SC_SW_OK) ) {
			printf( "Error: Unable to select file %.2X%.2X.\n", path[i],
				path[i+1] );
			ret=4;
			break;
		}

		ptr=0;
		while( (j=fread( buffer, 1, 128, f )) ) {
			ret = scCyberflexCmdUpdateBinary( ri, ci, ptr, buffer, j );
			scSmartcardSimpleProcessSW( ci, &i, NULL );
			if( (ret!=SC_EXIT_OK) || (i!=SC_SW_OK) ) {
				printf( "Error: Unable to write to file.\n" );
				ret=4;
				break;
			}
			ptr+=j;
		}

		ret=0;
	} while( 0 );

	printf( "ret: %d, SW: %.2X%.2X\n", ret, ci->sw[0], ci->sw[1] );

	if( scReaderDeactivate( ri )!=0 ) printf( "Error: scReaderDeactivate\n" );
	if( scReaderShutdown( ri )!=0 ) printf( "Error: scReaderShutdown\n" );

	scGeneralFreeCard( &ci );
	scGeneralFreeReader( &ri );

	scEnd();

	return( ret );
}

