/****************************************************************************
*																			*
*					SCEZ chipcard library - Smart Card Shell				*
*						Copyright Matthias Bruestle 1999					*
*					For licensing, see the file COPYRIGHT					*
*																			*
****************************************************************************/

/* $Id: scsh.c 1095 2002-06-21 14:28:00Z zwiebeltu $ */

/* #define DEBUG */

#if HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#ifdef HAVE_READLINE_HISTORY_H
#include <readline/history.h>
#endif
#ifdef HAVE_READLINE_READLINE_H
#include <readline/readline.h>
#endif
#ifdef DEBUG
#include <sio/sio.h>
#endif
#include <scez/scgeneral.h>

#ifndef READER_TYPE
#define READER_TYPE SC_READER_DUMBMOUSE
#endif /* READER_TYPE */
#ifndef READER_SLOT
#define READER_SLOT 1
#endif /* READER_SLOT */
#ifndef READER_PORT
#define READER_PORT "0"
#endif /* READER_PORT */

#define MAX_COMMAND_SIZE	700
#define PROGRAM	"ScSh - Copyright 1999 by Matthias Bruestle\n"

#define printarray( name, length, array ); \
	printf(name); \
	for( i=0; i<length; i++ ) printf(" %.2X",array[i]); \
	printf("\n");

int activate( SC_READER_INFO *ri, SC_CARD_INFO *ci )
{
	int ret;

	/* Get Card Status */
	ret = scReaderCardStatus( ri );
	if( ret!=SC_EXIT_OK ) {
		printf( "Error getting card status. (%d)\n", ret );
		return( ret );
	}
	if( !(ri->status&SC_CARD_STATUS_PRESENT) ) {
		printf( "Not card present.\n" );
		return( SC_EXIT_NO_CARD );
	}

	/* Activate Card */
	ret = scReaderActivate( ri );
	if( ret!=SC_EXIT_OK ) {
		printf( "Error activating card. (%d)\n", ret );
		return( ret );
	}

	printf( "Ok.\n" );

	return( SC_EXIT_OK );
}

int deactivate( SC_READER_INFO *ri, SC_CARD_INFO *ci )
{
	int ret;

	ret = scReaderDeactivate( ri );
	if( ret!=SC_EXIT_OK ) {
		printf( "Error deactivating card. (%d)\n", ret );
		return( ret );
	}

	printf( "Ok.\n" );

	return( SC_EXIT_OK );
}

int reset( SC_READER_INFO *ri, SC_CARD_INFO *ci )
{
	int i, ret;

	/* Get Card Status */
	ret = scReaderCardStatus( ri );
	if( ret!=SC_EXIT_OK ) {
		printf( "Error getting card status. (%d)\n", ret );
		return( ret );
	}
	if( !(ri->status&SC_CARD_STATUS_PRESENT) ) {
		printf( "Not card present.\n" );
		return( SC_EXIT_NO_CARD );
	}

	/* Reset Card */
	ret= scReaderResetCard( ri, ci );
	if( ret!=SC_EXIT_OK ) {
		printf( "Error reseting card. (%d)\n", ret );
		return( ret );
	}

	scSmartcardGetCardType( ci );
#if 0
	/* Get Card Type */
	ci->type=SC_CARD_UNKNOWN;
	scSmartcardGetCardType( ci );

	/* Process ATR */
	ret=scSmartcardProcessATR( ci );
	if( ret!=SC_EXIT_OK ) {
		printf( "Error processing ATR. (%d)\n", ret );
		return( ret );
	}
#endif

	printf( "Card type: %.8X\n", (unsigned int) ci->type );

	printarray( "ATR:", ci->atrlen, ci->atr );

	printf("Ok.\n");

	return( SC_EXIT_OK );
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

int sendapdu( SC_READER_INFO *ri, SC_CARD_INFO *ci, char *command )
{
	SC_APDU apdu;
	char data[MAX_COMMAND_SIZE];

	int i, ret;
	BYTE buffer[256+2];

	ret = cmdcheck( command );
	if( ret!=SC_EXIT_OK ) {
		printf( "Error in parameter.\n" );
		return( ret );
	}

	switch( command[0] ) {
		case '1':
			apdu.cse=SC_APDU_CASE_1;
			break;
		case '2':
			apdu.cse=SC_APDU_CASE_2_SHORT;
			break;
		case '3':
			apdu.cse=SC_APDU_CASE_3_SHORT;
			break;
		case '4':
			apdu.cse=SC_APDU_CASE_4_SHORT;
			break;
		default:
			printf( "Wrong APDU cse.\n" );
			return( SC_EXIT_BAD_PARAM );
	}

	ret = strtobin( data, command+2 );
	if( ret!=SC_EXIT_OK ) {
		printf( "Error in APDU command.\n" );
		return( SC_EXIT_BAD_PARAM );
	}

	apdu.cmd=data;
	apdu.cmdlen=(strlen(command)>>1)-1;
	apdu.rsp=buffer;
	apdu.rsplen=0;

	printf( "Case: %d\n", apdu.cse );
	printarray( "Command:", apdu.cmdlen, apdu.cmd );

	ret=scReaderSendAPDU( ri, ci, &apdu );

	if( ret!=SC_EXIT_OK ) {
		printf( "Error sending APDU. (%d)\n", ret );
		return( ret );
	}

	if( apdu.rsplen<2 ) {
		printf( "Error receiving response.\n" );
		return( SC_EXIT_UNKNOWN_ERROR );
	}

	printf("SW: %.2X%.2X\n",buffer[apdu.rsplen-2],buffer[apdu.rsplen-1]);
	if( apdu.rsplen>2 ) {
		printarray( "Response:", apdu.rsplen-2, buffer );
	}

	printf("Ok.\n");

	return( SC_EXIT_OK );
}

void help( )
{
	printf( PROGRAM );
	printf( "ac - activate\n" );
	printf( "de - deactivate\n" );
	printf( "re - reset\n" );
	printf( "se n:xxxxxxxx... - send APDU (n: Case, xx: data has hex, spaces are ignored)\n" );
	printf( "qu - quit\n" );
	printf( "he - help\n" );
}

int main( int argc, char *argv[] )
{
	SC_READER_INFO *ri;
	SC_CARD_INFO *ci;
	SC_READER_CONFIG rc;
	BOOLEAN activated=FALSE, reseted=FALSE;
#if defined(HAVE_READLINE_HISTORY_H) && defined(HAVE_READLINE_READLINE_H)
	char *command=NULL;
	HIST_ENTRY **the_history_list;
#else
	char command[MAX_COMMAND_SIZE];
#endif
	char *cptr;
	int i, ret;

	printf( PROGRAM );

#if defined(HAVE_READLINE_HISTORY_H) && defined(HAVE_READLINE_READLINE_H)
	using_history();
	stifle_history( 100 );
#endif

	if( scInit() ) {
		printf( "Error initializing library.\n" );
		return(1);
	}

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
	if( ri==NULL ) {
		printf( "Error building reader structure.\n" );
		scEnd();
		return(1);
	};

	ci = scGeneralNewCard( );
	if( ci==NULL ) {
		printf( "Error building card structure.\n" );
		scEnd();
		return(1);
	};

	/* Init Reader */
	ret = scReaderInit( ri, rc.param );
	if( ret!=SC_EXIT_OK ) {
		printf( "Error initializing reader. (%d)\n", ret );
		scReaderShutdown( ri );
		scEnd();
		return(1);
	}

#ifdef DEBUG
SIO_SetLogFile( ri->si, "LogScsh.txt" );
#endif

	printf( "Ready.\n" );

	/* Command loop */
	while( 1 ) {
#if defined(HAVE_READLINE_HISTORY_H) && defined(HAVE_READLINE_READLINE_H)
		if( command!=NULL ) free(command);
		command = readline( "scsh> " );
		if( command==NULL ) continue;
		add_history( command );
#else
		printf( "scsh> " );
		fgets( command, MAX_COMMAND_SIZE-1, stdin );
#endif
		
		for( i=0; i<2; i++ ) command[i] = tolower( command[i] );
		strcompact( command );

		/* Comment */
		if( command[0]==';' ) {
			continue;
		}

		/* Activate */
		if( strncmp( "ac", command, 2 )==0 ) {
			ret = activate( ri, ci );
			if( ret==SC_EXIT_OK ) activated=TRUE;
			reseted=FALSE;
			continue;
		}

		/* Deactivate */
		if( strncmp( "de", command, 2 )==0 ) {
			ret = deactivate( ri, ci );
			if( ret==SC_EXIT_OK ) activated=FALSE;
			reseted=FALSE;
			continue;
		}

		/* Reset */
		if( strncmp( "re", command, 2 )==0 ) {
			if( !activated ) {
				printf( "Card needs to be activated before a reset.\n" );
				continue;
			}
			ret = reset( ri, ci );
			if( ret==SC_EXIT_OK ) reseted=TRUE;
			continue;
		}

		/* Send APDU */
		if( strncmp( "se", command, 2 )==0 ) {
			if( !reseted ) {
				printf( "Card needs to be reseted before a command can be sent.\n" );
				continue;
			}
			cptr = command+2;
			if( cptr==NULL ) {
				printf( "Format error.\n");
				continue;
			}
			ret = sendapdu( ri, ci, cptr );
			continue;
		}

		/* Help */
		if( strncmp( "he", command, 2 )==0 ) {
			help( );
			continue;
		}

		/* Quit */
		if( strncmp( "qu", command, 2 )==0 ) {
			printf( "Mahlzeit.\n" );
			break;
		}

		/* Exit */
		if( strncmp( "ex", command, 2 )==0 ) {
			printf( "Mahlzeit.\n" );
			break;
		}

		printf( "Command not recognized. Type 'he' for help.\n" );
	}

	scReaderDeactivate( ri );
	scReaderShutdown( ri );
	scEnd();

	return(0);
}

