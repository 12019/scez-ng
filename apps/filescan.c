/****************************************************************************
*																			*
*						Copyright Matthias Bruestle 2002					*
*					For licensing, see the file COPYRIGHT					*
*																			*
****************************************************************************/

/* $Id: filescan.c 1093 2002-03-22 12:42:30Z zwiebeltu $ */

/* #define DEBUG */

#include <ctype.h>
#include <getopt.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#ifdef DEBUG
#include <sio/sio.h>
#endif
#include <scez/scgeneral.h>
#include <scez/screader.h>
#include <scez/scsmartcard.h>

#ifndef READER_TYPE
#define READER_TYPE SC_READER_DUMBMOUSE
#endif /* READER_TYPE */
#ifndef READER_SLOT
#define READER_SLOT 1
#endif /* READER_SLOT */
#ifndef READER_PORT
#define READER_PORT "0"
#endif /* READER_PORT */

#define checkreturn(f); if( ret!=SC_EXIT_OK ) { printf(f); goto end; }

#define printarray( file, name, length, array ); \
	{ int p; fprintf(file,name); \
	for( p=0; p<length; p++ ) fprintf(file," %.2X",array[p]); \
	fprintf(file,"\n"); }

void usage(char *argv) {
    printf("File scanner\n\n");
    printf("Usage:      %s <options>\n"\
	    "    Options:\n"\
        "   -c <case:command in hex> - Command used for file scan preceeded by case\n      and colon\n"\
	    "   -f <filename> - Output file\n"\
	    "   -h - this help text\n"\
	    "   -i <case:command in hex> - Initial command issued before file scan preceeded\n      by case and colon\n"\
	    "   -o <number> - Offset of FID in file scan command\n"\
	    "   -s <fid in hex> - Start scan from this FID\n"\
		"   -t <sw in hex> - Target status word for good selection\n"\
	    "   -x <fid in hex> - Exclude FID from scan (multiple entries not yet possible)\n",
	    argv);
    exit(0);
}

void strcompact( char *str )
{
	int i,j=0;

	for( i=0; i<strlen(str); i++ ) {
		if( !isspace(str[i]) ) str[j++]=tolower( str[i] );
	}

	str[j]=0;
}

int cmdcheck( char *str )
{
	int i;

	if( strlen(str)<10 ) return( SC_EXIT_BAD_PARAM );

	if( (str[0]!='1') && (str[0]!='2') && (str[0]!='3') && (str[0]!='4') ) {
		return( SC_EXIT_BAD_PARAM );
	}

	if( str[1]!=':' ) return( SC_EXIT_BAD_PARAM );

	for( i=2; i<strlen(str); i++ ) {
		if( !isxdigit(str[i]) ) return( SC_EXIT_BAD_PARAM );
	}

	if( strlen(str)&1 ) return( SC_EXIT_BAD_PARAM );

	return( SC_EXIT_OK );
}

int strtobin( char *d, char *s )
{
	LONG l;
	int i, ret;

	for(i=0; i<(strlen(s)>>1); i++ ) {
		ret=sscanf( s+(i<<1), "%2lx", &l );
		if( ret!=1 ) return( SC_EXIT_BAD_PARAM );
		d[i]=l&0xFF;
	}

	return( SC_EXIT_OK );
}

int initapdu( SC_APDU *apdu, char *command )
{
	int ret;
	BYTE buffer[255];

	strcompact( command );

	ret = cmdcheck( command );
	if( ret!=SC_EXIT_OK ) {
		printf( "Error in parameter.\n" );
		return( ret );
	}

	switch( command[0] ) {
		case '1':
			apdu->cse=SC_APDU_CASE_1;
			break;
		case '2':
			apdu->cse=SC_APDU_CASE_2_SHORT;
			break;
		case '3':
			apdu->cse=SC_APDU_CASE_3_SHORT;
			break;
		case '4':
			apdu->cse=SC_APDU_CASE_4_SHORT;
			break;
		default:
			printf( "Wrong APDU case.\n" );
			return( SC_EXIT_BAD_PARAM );
	}

	ret = strtobin( buffer, command+2 );
	if( ret!=SC_EXIT_OK ) {
		printf( "Error in APDU command.\n" );
		return( SC_EXIT_BAD_PARAM );
	}

	memcpy( apdu->cmd, buffer, (strlen(command)>>1)-1 );
	apdu->cmdlen=(strlen(command)>>1)-1;

	printf( "Case: %d\n", apdu->cse );
	printarray( stdout, "Command:", apdu->cmdlen, apdu->cmd );

	return( SC_EXIT_OK );
}

int main( int argc, char *argv[] )
{
	SC_READER_INFO *ri;
	SC_CARD_INFO *ci;
	SC_READER_CONFIG rc;

	int c, i, j, ret;

	/* Options */
	SC_APDU fsapdu;
	BYTE fsapducmd[5+255+1], fsapdursp[256+2];
	BOOLEAN validfsapdu=FALSE;
	SC_APDU iapdu;
	BYTE iapducmd[5+255+1], iapdursp[256+2];
	BOOLEAN validiapdu=FALSE;
	BYTE fname[1024];
	BOOLEAN validfname=FALSE;
	BYTE offset=5;
	WORD startfid=0;
	WORD targetsw=0x9000;
	WORD excludefid=0;
	BOOLEAN validexcludefid=FALSE;

	FILE *f=NULL;

	/* Parameters for options */
	fsapdu.cmd=fsapducmd;
	fsapdu.cmdlen=0;
	fsapdu.rsp=fsapdursp;
	fsapdu.rsplen=0;
	iapdu.cmd=iapducmd;
	iapdu.cmdlen=0;
	iapdu.rsp=iapdursp;
	iapdu.rsplen=0;

	/* c <command in hex> - Command used for file scan
	 * f <filename> - Output file
	 * h - this help text
	 * i <command in hex> - Initial command issued before file scan
	 * o <number> - Offset of FID in file scan command
	 * s <fid in hex> - Start scan from this FID
	 * x <fid in hex> - Exclude FID from scan (multiple entries not yet possible)
	 */
	while( (c=getopt( argc, argv, "c:f:hi:o:s:t:x:" ))!=EOF ) {
		switch( c ) {
		case 'c':
			if( initapdu( &fsapdu, optarg )!=SC_EXIT_OK ) {
				printf( "Error: Error in file scanning command.\n" );
				return(1);
			}
			validfsapdu=TRUE;
			break;
		case 'f':
			if( strlen(optarg)>(sizeof(fname)-1) ) {
				printf("Error: Filename to long.\n");
				return(1);
			}
			strncpy( fname, optarg, sizeof(fname) );
			validfname=TRUE;
			break;
		case 'h':
			usage(argv[0]);
			return(0);
		case 'i':
			if( initapdu( &iapdu, optarg )!=SC_EXIT_OK ) {
				printf( "Error: Error in initialization command.\n" );
				return(1);
			}
			validiapdu=TRUE;
			break;
		case 'o':
			ret=sscanf( optarg, "%d", &i );
			if( ret!=1 ) {
				printf( "Error: Error in offset.\n" );
				return(1);
			}
			offset=i;
			break;
		case 's':
			ret=sscanf( optarg, "%X", &i );
			if( ret!=1 ) {
				printf( "Error: Error in start FID.\n" );
				return(1);
			}
			startfid=i&0xFFFF;
			break;
		case 't':
			ret=sscanf( optarg, "%X", &i );
			if( ret!=1 ) {
				printf( "Error: Error in target status word.\n" );
				return(1);
			}
			targetsw=i&0xFFFF;
			break;
		case 'x':
			ret=sscanf( optarg, "%X", &i );
			if( ret!=1 ) {
				printf( "Error: Error in exclude FID.\n" );
				return(1);
			}
			excludefid=i&0xFFFF;
			validexcludefid=TRUE;
			break;
		default:
			break;
		}
	}

#ifdef DEBUG
printf("validfsapdu: %d\n",validfsapdu);
printarray(stdout, "fsapdu.cmd:",fsapdu.cmdlen,fsapdu.cmd);
printf("fsapdu.cse: %d\n",fsapdu.cse);
printarray(stdout, "fsapdu.rsp:",fsapdu.rsplen,fsapdu.rsp);
printf("validiapdu: %d\n",validiapdu);
printarray(stdout, "iapdu.cmd:",iapdu.cmdlen,iapdu.cmd);
printf("iapdu.cse: %d\n",iapdu.cse);
printarray(stdout, "iapdu.rsp:",iapdu.rsplen,iapdu.rsp);
printf("validfname: %d\n",validfname);
printf("fname: %s\n",fname);
printf("offset: %d\n",offset);
printf("Start FID: %.4X\n",startfid);
printf("Target SW: %.4X\n",targetsw);
printf("validexcludefid: %d\n",validexcludefid);
printf("Exclude FID: %.4X\n",excludefid);
#endif

	if( !validfsapdu ) {
		printf( "Error: File scanning APDU not specified.\n" );
		return(1);
	}

	if( validfname ) {
		if( (f=fopen( fname, "wb" ))==NULL ) {
			printf( "Error: Unable to open output file.\n" );
			return(2);
		}
	}

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

#ifdef DEBUG
	SIO_SetLogFile( ri->si, "LogFilescan.txt" );
#endif

	/* Activate Card */
	ret = scReaderActivate( ri );
	checkreturn("Error: scReaderActivate\n");

	/* Get Card Status */
	ret = scReaderCardStatus( ri );
	checkreturn("Error: scReaderCardStatus\n");
	if( !(ri->status&SC_CARD_STATUS_PRESENT) )
	{ printf("Error: No Card.\n"); goto end; }

	/* Reset Card */
	ret= scReaderResetCard( ri, ci );
	checkreturn("Error: scReaderResetCard\n");

	/* Get Card Type */
	ret = scSmartcardGetCardType( ci );
	checkreturn("Error: scReaderGetCardType\n");

	for( i=startfid; i<0xFFFF; i++ ) {
		if( !(i&0x7FF) ) {
			printf( "Current FID: %.4X\n", i );
		}

		/* Skip exclude FID */
		if( validexcludefid && i==excludefid ) continue;

		/* Reset Card */
		ret= scReaderResetCard( ri, ci );
		checkreturn("Error: scReaderResetCard\n");

		/* Send initial command */
		if( validiapdu ) {
			ret=scReaderSendAPDU( ri, ci, &iapdu );
 
			if( ret!=SC_EXIT_OK ) {
				printf( "Error sending APDU. (%d)\n", ret );
				if( validfname ) {
					fprintf( f, "Aborted at file scan APDU %.4X.\n", i );
				}
				return( ret );
			}
		}

		/* Send file scan command */
		fsapdu.cmd[offset]=(i>>8)&0xFF;
		fsapdu.cmd[offset+1]=i&0xFF;

		ret=scReaderSendAPDU( ri, ci, &fsapdu );

		if( ret!=SC_EXIT_OK ) {
			printf( "Error sending APDU. (%d)\n", ret );
			if( validfname ) {
				fprintf( f, "Aborted at file scan APDU %.4X.\n", i );
			}
			return( ret );
		}

		j=(fsapdu.rsp[fsapdu.rsplen-2]<<8)+fsapdu.rsp[fsapdu.rsplen-1];

		if( (j&0xFF00)==(targetsw&0xFF00) ) {
			/* Good selection */
			printf( "%.4X", i );
			printarray( stdout, ":", fsapdu.rsplen, fsapdu.rsp );
			if( validfname ) {
				fprintf( f, "%.4X", i );
				printarray( f, ":", fsapdu.rsplen, fsapdu.rsp );
				fflush( f );
			}
		}
	}

end:
	if( scReaderDeactivate( ri )!=0 ) printf( "Error: scReaderDeactivate\n" );
	if( scReaderShutdown( ri )!=0 ) printf( "Error: scReaderShutdown\n" );

	scGeneralFreeCard( &ci );
	scGeneralFreeReader( &ri );

	scEnd();
#if 0
#endif

	return( ret );
}

