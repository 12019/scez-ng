/****************************************************************************
*																			*
*					SCEZ chipcard library - Reader routines					*
*					Copyright Matthias Bruestle 1999,2000					*
*					For licensing, see the file COPYRIGHT					*
*																			*
****************************************************************************/

/* $Id: screader.c 1617 2005-11-03 17:41:39Z laforge $ */

#include <scez/scinternal.h>
#ifdef WITH_ACR20
#include <scez/readers/scacr20.h>
#endif
#ifdef WITH_B1
#include <scez/scb1.h>
#endif
#ifdef WITH_BLUEDOT
#include <scez/readers/scbluedot.h>
#endif
#ifdef WITH_CTAPI
#include <scez/scctapi.h>
#endif
#ifdef WITH_DUMBMOUSE
#include <scez/readers/scdumbmouse.h>
#endif
#ifdef WITH_EASYCHECK
#include <scez/cards/sceasycheck.h>
#endif
#ifdef WITH_GCR400
#include <scez/readers/scgcr400.h>
#endif
#ifdef WITH_GPR400
#include <scez/readers/scgpr400.h>
#endif
#ifdef WITH_INTERTEX
#include <scez/readers/scintertex.h>
#endif
#ifdef WITH_REFLEX20
#include <scez/readers/screflex20.h>
#endif
#ifdef WITH_REFLEX60
#include <scez/readers/screflex60.h>
#endif
#ifdef WITH_TOWITOKO
#include <scez/readers/sctowitoko.h>
#endif

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>
#if defined(__linux__)
#include <getopt.h>
#endif /* __linux__ */
#ifdef WINDOWS
#include <windows.h>
#elif defined( __palmos__ )
#include <Pilot.h>
#else
#include <unistd.h>
#endif

#ifdef HAVE_LIBKEEPER
#include <keeper.h>
#endif /* HAVE_LIBKEEPER */

#define SC_READER_MAX_PORTS	4

static char *Ports[] = {
#if defined(__linux__)
	"/dev/ttyS0", "/dev/ttyS1", "/dev/ttyS2", "/dev/ttyS3"
#elif defined(__FreeBSD__)
	/* Only for FreeBSD 2? */
	"/dev/cuaa0", "/dev/cuaa1", "/dev/cuaa2", "/dev/cuaa3"
#elif (defined(__alpha) && defined(__osf__)) || defined (__NetBSD__)
	/* I hope this is correct. */
	"/dev/tty00", "/dev/tty01", "/dev/tty02", "/dev/tty03"
#elif defined(sun) && (OSVERSION>4)
	/* Or /dev/cua/[abcd]? */
	"/dev/ttya", "/dev/ttyb", "/dev/ttyc", "/dev/ttyd"
#elif defined(WINDOWS)
	"COM1:", "COM2:", "COM3:", "COM4:"
	/* Or
	 *    "\\\\.\\COM1", "\\\\.\\COM2", "\\\\.\\COM3", "\\\\.\\COM4"
	 * ?
	 */
#else
	"0", "1", "2", "3"
#endif /* __linux__ */
};

#ifndef __palmos__
NAMENUMBER rdlookup[] = {
	{ "ACR20", SC_READER_ACR20 },
	{ "B1", SC_READER_B1 },
	{ "BLUEDOT", SC_READER_BLUEDOT },
	{ "CHIPI", SC_READER_CHIPI },
	{ "CTAPI", SC_READER_CTAPI },
	{ "DUMBMOUSE", SC_READER_DUMBMOUSE },
	{ "EASYCHECK", SC_READER_EASYCHECK },
	{ "GCR400", SC_READER_GCR400 },
	{ "GPR400", SC_READER_GPR400 },
	{ "INTERTEX", SC_READER_INTERTEX },
	{ "REFLEX20", SC_READER_REFLEX20 },
	{ "REFLEX60", SC_READER_REFLEX60 },
	{ "TOWITOKO", SC_READER_TOWITOKO },
	{ NULL, 0 }
};
#endif /* !__palmos__ */

/* Expands serial port number to OS specific device name */
/* pn: "0" - "3" */

char *scReaderExpandPort( const char *pn )
{
	if( strcmp(pn,"0")==0 ) return( Ports[0] );
	if( strcmp(pn,"1")==0 ) return( Ports[1] );
	if( strcmp(pn,"2")==0 ) return( Ports[2] );
	if( strcmp(pn,"3")==0 ) return( Ports[3] );

	return( NULL );
}

/* rc should contain the compile time defaults. If there are no defaults
 * for some/all settings, type should be SC_READER_UNKNOWN, slot should
 * be 0 and param should be NULL.
 *
 * scReaderGetConfig looks in following order at these resources and
 * substitues the value in rc if it finds good values:
 *
 *  - Compile time defaults. (They are already in rc.)
 *  - keeper: global, local, user settings
 *  - Environment.
 *  - Program arguments.
 *
 * Environment:
 *
 * The variable scReaderGetConfig looks at is SCEZ_READER.
 * The format is:
 *
 *    type,slot,param
 *
 *    type: The string after SC_READER_, e.g. DUMBMOUSE, CTAPI.
 *       Must be currently in upper case letters.
 *    subtype: Not used.
 *    slot: 1, 2, ...
 *    param: 0, 1, 2, 3, device names or other parameters.
 *
 * Examples:
 *    DUMBMOUSE,1,0
 *    TOWITOKO,1,/dev/ttyS1
 *
 * Program arguments:
 *
 *    -T <type>
 *    -S <slot>
 *    -P <param>
 *
 * The data is the same as above. The argument switches have to be written
 * in uppercase.
 *
 * The program arguments method is currently only enabled with Linux,
 * but it could be enabled for every platform getopt is available.
 */

int scReaderGetConfig( int argc, char *argv[], SC_READER_CONFIG *rc )
{
#ifndef __palmos__
	char *s1=NULL, *s1cpy, s2[20], *p=NULL;
	int i, ret;
#if 0
#if defined(__linux__)
	int c;
#endif /* __linux__ */
#endif /* 0 */

#ifdef WITH_KEEPER
	/*
	 *  Keeper
	 */
	if( !kp_get_int( "g/SCEZ/type", &i ) ) {
		rc->type=i;
#ifdef DEBUG
		printf( "[global/SCEZ/type=%d]", i );
#endif /* DEBUG */
	}
	if( !kp_get_int( "g/SCEZ/slot", &i ) ) {
		rc->slot=i;
#ifdef DEBUG
		printf( "[global/SCEZ/slot=%d]", i );
#endif /* DEBUG */
	}
	if( !kp_get_string( "g/SCEZ/param", &s1 ) ) {
		if( p!=NULL ) free( p );
		p=s1;
#ifdef DEBUG
		printf( "[global/SCEZ/param=%s]", p );
#endif /* DEBUG */
	}

	if( !kp_get_int( "l/SCEZ/type", &i ) ) {
		rc->type=i;
#ifdef DEBUG
		printf( "[local/SCEZ/type=%d]", i );
#endif /* DEBUG */
	}
	if( !kp_get_int( "l/SCEZ/slot", &i ) ) {
		rc->slot=i;
#ifdef DEBUG
		printf( "[local/SCEZ/slot=%d]", i );
#endif /* DEBUG */
	}
	if( !kp_get_string( "l/SCEZ/param", &s1 ) ) {
		if( p!=NULL ) free( p );
		p=s1;
#ifdef DEBUG
		printf( "[local/SCEZ/param=%s]", p );
#endif /* DEBUG */
	}

	if( !kp_get_int( "u/SCEZ/type", &i ) ) {
		rc->type=i;
#ifdef DEBUG
		printf( "[user/SCEZ/type=%d]", i );
#endif /* DEBUG */
	}
	if( !kp_get_int( "u/SCEZ/slot", &i ) ) {
		rc->slot=i;
#ifdef DEBUG
		printf( "[user/SCEZ/slot=%d]", i );
#endif /* DEBUG */
	}
	if( !kp_get_string( "u/SCEZ/param", &s1 ) ) {
		if( p!=NULL ) free( p );
		p=s1;
#ifdef DEBUG
		printf( "[user/SCEZ/param=%s]", p );
#endif /* DEBUG */
	}
#endif /* WITH_KEEPER */

	/*
	 *  Environment
	 */

	s1=getenv(SC_ENV);

	if( s1!=NULL ) {
		/* Copy string and save pointer. */
		s1cpy = malloc( strlen(s1)+1 );
		strcpy( s1cpy, s1 );
		s1 = s1cpy;

		/* Look for type. */
		i=0;
		while( (i<sizeof(s2)+1) && (s1[i]!=',') && (s1[i]!=0) ) {
			s2[i]=toupper(s1[i]);
			i++;
		}
		s2[i]=0;

		i=0;
		while( (rdlookup[i].name!=NULL) && (strcmp(rdlookup[i].name,s2)) ) i++;

		if( rdlookup[i].name!=NULL ) rc->type=rdlookup[i].number;

		/* Look for slot. */
		s1=strchr( s1, ',' );

		if( s1!=NULL ) {
			s1++;
			i=0;
			while( (i<sizeof(s2)+1) && (s1[i]!=',') && (s1[i]!=0) ) {
				s2[i]=s1[i];
				i++;
			}
			s2[i]=0;

			ret=sscanf( s2, "%d", &i );

			if( ret==1 ) rc->slot=i;
		}

		/* Look for param. */
		if( s1!=NULL ) s1=strchr( s1, ',' );

		if( s1!=NULL ) {
			s1++;
			if( *s1!=0 ) {
				if( p!=NULL ) free( p );
				p = malloc( strlen( s1 )+1 );
				strcpy( p, s1 );
			}
		}

		free( s1cpy );
	}

#if 0
	/*
	 * Program arguments
	 */

#if defined(__linux__)
	if( (argc>0) && (argv!=NULL) ) {
		opterr=0;

		while( (c=getopt( argc, argv, "T:S:P:" ))!=EOF ) {
			switch( c ) {
				/* Get type. */
				case 'T':
					for(i=0; i<strlen(optarg); i++)
						optarg[i]=toupper(optarg[i]);
					i=0;
					while( (rdlookup[i].name!=NULL) &&
						(strcmp(rdlookup[i].name,optarg)) )
						i++;

					if( rdlookup[i].name!=NULL ) rc->type=rdlookup[i].number;

					break;

				/* Get slot. */
				case 'S':
					ret=sscanf( optarg, "%d", &i );

					if( ret==1 ) rc->slot=i;

					break;

				/* Get param. */
				case 'P':
					if( p!=NULL ) free( p );
					p = malloc( strlen( optarg ) );
					strcpy( p, optarg );

					break;

				default:
			}
		}
	}
#endif /* __linux__ */
#endif /* 0 */

	if( p!=NULL ) rc->param=p;
#endif /* !__palmos__ */

#ifdef DEBUG
	printf( "[Final: type=%d, slot=%d, param=%s]", rc->type, rc->slot,
		rc->param );
#endif /* DEBUG */

	return( SC_EXIT_OK );
}

/* Check APDU */

int scReaderCheckAPDU( const SC_APDU *apdu, BOOLEAN t0 )
{
	switch( apdu->cse ) {
	case SC_APDU_CASE_1:
		if( apdu->cmdlen<4 ) return( SC_EXIT_CMD_TOO_SHORT );
		if( t0 ) {
			if( apdu->cmdlen>5 ) return( SC_EXIT_CMD_TOO_LONG );
		} else {
			if( apdu->cmdlen>4 ) return( SC_EXIT_CMD_TOO_LONG );
		}
		break;
	case SC_APDU_CASE_2_SHORT:
		if( apdu->cmdlen<5 ) return( SC_EXIT_CMD_TOO_SHORT );
		if( apdu->cmdlen>5 ) return( SC_EXIT_CMD_TOO_LONG );
		break;
	case SC_APDU_CASE_3_SHORT:
		if( apdu->cmdlen<(5+apdu->cmd[4]) ) return( SC_EXIT_CMD_TOO_SHORT );
		if( apdu->cmdlen>(5+apdu->cmd[4]) ) return( SC_EXIT_CMD_TOO_LONG );
		break;
	case SC_APDU_CASE_4_SHORT:
		if( t0 ) {
			if( apdu->cmdlen<(5+apdu->cmd[4]) )
				return( SC_EXIT_CMD_TOO_SHORT );
		} else {
			if( apdu->cmdlen<(5+apdu->cmd[4]+1) )
				return( SC_EXIT_CMD_TOO_SHORT );
		}
		if( apdu->cmdlen>(5+apdu->cmd[4]+1) ) return( SC_EXIT_CMD_TOO_LONG );
		break;
	default:
		return( SC_EXIT_NOT_SUPPORTED );
		break;
	}

	return( SC_EXIT_OK );
}

/* Initialize reader */

int scReaderInit( SC_READER_INFO *ri, const char *param )
{
	switch( ri->major )
	{
#ifdef WITH_ACR20
		case SC_READER_ACR20:
			return( scAcr20Init( ri, param ) );
#endif
#ifdef WITH_B1
		case SC_READER_B1:
			return( scB1Init( ri, param ) );
#endif
#ifdef WITH_BLUEDOT
		case SC_READER_BLUEDOT:
			return( scBluedotInit( ri, param ) );
#endif
#ifdef WITH_CTAPI
		case SC_READER_CTAPI:
			return( scCtapiInit( ri, param ) );
#endif
#ifdef WITH_DUMBMOUSE
		case SC_READER_DUMBMOUSE:
			return( scDumbmouseInit( ri, param ) );
#endif
#ifdef WITH_EASYCHECK
		case SC_READER_EASYCHECK:
			return( scEasycheckInit( ri, param ) );
#endif
#ifdef WITH_GCR400
		case SC_READER_GCR400:
			return( scGcr400Init( ri, param ) );
#endif
#ifdef WITH_GPR400
		case SC_READER_GPR400:
			return( scGpr400Init( ri, param ) );
#endif
#ifdef WITH_INTERTEX
		case SC_READER_INTERTEX:
			return( scIntertexInit( ri, param ) );
#endif
#ifdef WITH_REFLEX20
		case SC_READER_REFLEX20:
			return( scReflex20Init( ri, param ) );
#endif
#ifdef WITH_REFLEX60
		case SC_READER_REFLEX60:
			return( scReflex60Init( ri, param ) );
#endif
#ifdef WITH_TOWITOKO
		case SC_READER_TOWITOKO:
			return( scTowitokoInit( ri, param ) );
#endif
		default:
			return( SC_EXIT_NOT_SUPPORTED );
	}

	return( SC_EXIT_UNKNOWN_ERROR );
}

/* Following functions are only there to make it more convenient to
 * handle the function pointers.
 */

/* Shutdown reader */

int scReaderShutdown( SC_READER_INFO *ri )
{
	if( ri->scShutdown==NULL ) return( SC_EXIT_NOT_SUPPORTED );
	return( ri->scShutdown( ri ) );
}

/* Get Capabilities */

int scReaderGetCap( SC_READER_INFO *ri, SC_READER_CAP *rp )
{
	if( ri->scGetCap==NULL ) return( SC_EXIT_NOT_SUPPORTED );
	return( ri->scGetCap( ri, rp ) );
}

/* Activate card */

int scReaderActivate( SC_READER_INFO *ri)
{
	if( ri->scActivate==NULL ) return( SC_EXIT_NOT_SUPPORTED );
	return( ri->scActivate( ri ) );
}

/* Deactivate card */

int scReaderDeactivate( SC_READER_INFO *ri)
{
	if( ri->scDeactivate==NULL ) return( SC_EXIT_NOT_SUPPORTED );
	return( ri->scDeactivate( ri ) );
}

/* Write Buffer Async */

int scReaderWriteBuffer( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	const BYTE *buffer, int len )
{
	if( ri->scWriteBuffer==NULL ) return( -1 );
	return( ri->scWriteBuffer( ri, ci, buffer, len ) );
}

/* Read Buffer Async */

int scReaderReadBuffer( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	BYTE *buffer, int len, LONG timeout )
{
	if( ri->scReadBuffer==NULL ) return( -1 );
	return( ri->scReadBuffer( ri, ci, buffer, len, timeout ) );
}

/* Write Char Async */

int scReaderWriteChar( SC_READER_INFO *ri, SC_CARD_INFO *ci, int ch )
{
	if( ri->scWriteChar==NULL ) return( -1 );
	return( ri->scWriteChar( ri, ci, ch ) );
}

/* Read Char Async */

int scReaderReadChar( SC_READER_INFO *ri, SC_CARD_INFO *ci, LONG timeout )
{
	if( ri->scReadChar==NULL ) return( -1 );
	return( ri->scReadChar( ri, ci, timeout ) );
}

/* Wait For Data */

int scReaderWaitForData( SC_READER_INFO *ri, SC_CARD_INFO *ci, LONG timeout )
{
	if( ri->scWaitForData==NULL ) return( -1 );
	return( ri->scWaitForData( ri, ci, timeout ) );
}

/* Delay */

int scReaderDelay( LONG timeout )
{
#ifdef WINDOWS
	timeout/=1000;
	if (timeout==0) timeout=1;
	Sleep(timeout);
#elif defined( __palmos__ )
	if( SysTaskDelay( ((timeout*sysTicksPerSecond)/1000000)+1 )!=0 )
		return(-1);
#else
	if (timeout==0)
		timeout=1;

	usleep(timeout);
#endif
	return (0);
}

/* Set speed */

int scReaderSetSpeed( SC_READER_INFO *ri, LONG speed )
{
	if( ri->scSetSpeed==NULL ) return( SC_EXIT_NOT_SUPPORTED );
	return( ri->scSetSpeed( ri, speed ) );
}

/* Card status */

int scReaderCardStatus( SC_READER_INFO *ri )
{
	if( ri->scCardStatus==NULL ) return( SC_EXIT_NOT_SUPPORTED );
	return( ri->scCardStatus( ri ) );
}

/* Reset card and read ATR */

int scReaderResetCard( SC_READER_INFO *ri, SC_CARD_INFO *ci )
{
	if( ri->scResetCard==NULL ) return( SC_EXIT_NOT_SUPPORTED );

	/* Here and in every reader specific ResetCard. Just to be sure. */
	scGeneralCleanCI( ci );

	return( ri->scResetCard( ri, ci ) );
}

/* Do a PTS handshake. */

int scReaderPTS( SC_READER_INFO *ri, SC_CARD_INFO *ci, const BYTE *pts,
	int ptslen )
{
	if( ri->scPTS==NULL ) return( SC_EXIT_NOT_SUPPORTED );
	return( ri->scPTS( ri, ci, pts, ptslen ) );
}

/* Send and process T=0 command */

int scReaderT0( SC_READER_INFO *ri, SC_CARD_INFO *ci, SC_APDU *apdu )
{
	if( ri->scT0==NULL ) return( SC_EXIT_NOT_SUPPORTED );

	if( apdu->rsp==NULL ) return( SC_EXIT_BAD_PARAM );

	return( ri->scT0( ri, ci, apdu ) );
}

/* Transmit APDU with protocol T=1 */

int scReaderT1( SC_READER_INFO *ri, SC_CARD_INFO *ci, SC_APDU *apdu )
{
	if( ri->scT1==NULL ) return( SC_EXIT_NOT_SUPPORTED );

	if( apdu->rsp==NULL ) return( SC_EXIT_BAD_PARAM );

	return( ri->scT1( ri, ci, apdu ) );
}

/* Transmit APDU */

int scReaderSendAPDU( SC_READER_INFO *ri, SC_CARD_INFO *ci, SC_APDU *apdu )
{
	if( ri->scSendAPDU==NULL ) return( SC_EXIT_NOT_SUPPORTED );

	if( apdu->rsp==NULL ) return( SC_EXIT_BAD_PARAM );

	return( ri->scSendAPDU( ri, ci, apdu ) );
}

/* Transmit command using PIN pad */

int scReaderVerifyPIN( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	SC_APDU *apdu, const char *message, int pinlen, int pincoding, int pinpos )
{
	if( ri->scVerifyPIN==NULL ) return( SC_EXIT_NOT_SUPPORTED );
	return( ri->scVerifyPIN( ri, ci, apdu, message, pinlen, pincoding,
		pinpos ) );
}

/* NULL scWaitReq function, which in fact disables maximum number of
 * WTX etc. requests.
 *
 * Normal operation:
 * When SC_EXIT_OK is returned, the protocol continues normal.
 * When anything other is returned, the protocol stops and returns this
 * return value. So when count is to high for you, you should return
 * a SC_EXIT_TIMEOUT.
 */

int scReaderWaitReqNull( SC_READER_INFO *ri, SC_CARD_INFO *ci, int count )
{
#ifdef READER_DEBUG
	printf("[WTX]");
#endif /* READER_DEBUG */
	return( SC_EXIT_OK );
}

