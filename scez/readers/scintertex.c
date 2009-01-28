/****************************************************************************
*																			*
*					SCEZ chipcard library - Intertex routines				*
*						Copyright Matthias Bruestle 2000					*
*					For licensing, see the file COPYRIGHT					*
*																			*
****************************************************************************/

/* $Id: scintertex.c 1617 2005-11-03 17:41:39Z laforge $ */

#include <scez/scinternal.h>
#include <scez/sct0.h>
#include <scez/sct1.h>
#include <scez/scpts.h>
#include <scez/readers/scintertex.h>

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#if !defined(WINDOWS) && !defined(__palmos__)
#include <unistd.h> /* for sleep */
#elif defined(__BORLANDC__)
#include <dos.h>	/* for sleep */
#elif defined(WINDOWS)
#include <windows.h>
#elif defined(__palmos__)
#define sleep(x) SysTaskDelay(x*sysTicksPerSecond)
#endif

/****************************************************************************
*																			*
*						 		Init/Shutdown Routines						*
*																			*
****************************************************************************/

/* Initializes reader and sets ri */

int scIntertexInit( SC_READER_INFO *ri, char *param )
{
	BYTE buffer[ SC_INTERTEX_MAX_CMD_SIZE ];
	int i, ret;
	char *port;

	if( ri->slot!=1 ) return( SC_EXIT_NO_SLOT );

	if( param==NULL ) return( SC_EXIT_BAD_PARAM );

	if( (strcmp(param,"0")==0) || (strcmp(param,"1")==0) ||
		(strcmp(param,"2")==0) || (strcmp(param,"3")==0) ) {
		port=scReaderExpandPort( param );
		if( port==NULL ) return( SC_EXIT_BAD_PARAM );
	} else port=param;

	if( ri->si!=NULL ) scIntertexShutdown( ri );

	/* Open port */
	ri->si = SIO_Open( port );
	if( ri->si==NULL ) return( SC_EXIT_IO_ERROR );

	/* Set main port parameters */
	SIO_SetSpeed( ri->si, 115200 );
	SIO_SetDataBits( ri->si, 8 );
	SIO_SetParity( ri->si, SIO_PARITY_EVEN );
	SIO_SetStopBits( ri->si, 2 );
	SIO_SetIOMode( ri->si, SIO_IOMODE_DIRECT );

	SIO_WriteSettings( ri->si );

#ifdef READER_DEBUG
	SIO_SetLogFile( ri->si, "LogIntertex.txt" );
#endif /* READER_DEBUG */

	/* Get unwanted bytes */
#ifdef __palmos__
	SIO_Flush(ri->si);
#else
	while(SIO_ReadChar(ri->si)!=-1);
#endif /* __palmos__ */

	SIO_WriteChar( ri->si, '\n' );
	SIO_WriteChar( ri->si, '\n' );
	SIO_WriteChar( ri->si, '\n' );

	/* Get unwanted bytes */
#ifdef __palmos__
	SIO_Flush(ri->si);
#else
	while(SIO_ReadChar(ri->si)!=-1);
#endif /* __palmos__ */

	/* Command 8: Set/reset change alert */
	buffer[ 0 ] = SC_INTERTEX_CMD_SET_ALERT;
	buffer[ 1 ] = 0;

	if( (ret=scIntertexSendCmd( ri, SC_INTERTEX_DIR_SND_REC, buffer, 2, buffer,
		&i, 5*1000*1000 )) ==SC_EXIT_BAD_CHECKSUM )
		ret=scIntertexSendCmd( ri, SC_INTERTEX_DIR_SND_REC, buffer, 2, buffer,
			&i, 5*1000*1000 );

	if( (ret!=SC_EXIT_OK) || (buffer[0]!=SC_INTERTEX_CMD_SET_ALERT) ) {
		scIntertexShutdown( ri );
		return( ret );
	}

	/* Set variables */
	ri->major=SC_READER_INTERTEX;
	/* The etu may be really shorter in the reader. */
	ri->etu=104; /* 104us */
	ri->pinpad=FALSE;
	ri->display=FALSE;

	ri->scShutdown=scIntertexShutdown;
	ri->scActivate=scIntertexActivate;
	ri->scDeactivate=scIntertexDeactivate;
	ri->scWriteBuffer=scIntertexWriteBuffer;
	ri->scReadBuffer=scIntertexReadBuffer;
	ri->scWriteChar=NULL;
	ri->scReadChar=NULL;
	ri->scWaitForData=scIntertexWaitForData;
	ri->scSetSpeed=NULL;
	ri->scCardStatus=scIntertexCardStatus;
	ri->scResetCard=scIntertexResetCard;
	ri->scPTS=scPtsDoPTS;
	ri->scT0=scIntertexT0;
	ri->scT1=scIntertexT1;
	ri->scSendAPDU=scIntertexSendAPDU;
	ri->scVerifyPIN=NULL;
	ri->scWaitReq=NULL;

	return( SC_EXIT_OK );
}

int scIntertexShutdown( SC_READER_INFO *ri )
{
	if( ri->si==NULL ) return( SC_EXIT_OK );

	SIO_Close( ri->si );
	ri->si=NULL;

	return( SC_EXIT_OK );
}

int scIntertexDetect( SC_READER_DETECT_INFO *rdi )
{
	SC_READER_INFO ri;
	BYTE buffer[ SC_INTERTEX_MAX_CMD_SIZE ];
	int i, ret;
	char *port;

	rdi->prob=0;
	strncpy( rdi->name, "INTERTEX", SC_READER_NAME_MAXLEN );
	rdi->major=SC_READER_INTERTEX;
	rdi->minor=0;
	rdi->slots=1;
	rdi->pinpad=FALSE;
	rdi->display=FALSE;

	if( strlen(rdi->param)>=SC_READER_PARAM_MAXLEN )
		return( SC_EXIT_BAD_PARAM );

	if( (strcmp(rdi->param,"0")==0) || (strcmp(rdi->param,"1")==0) ||
		(strcmp(rdi->param,"2")==0) || (strcmp(rdi->param,"3")==0) ) {
		port=scReaderExpandPort( rdi->param );
		if( port==NULL ) return( SC_EXIT_BAD_PARAM );
	} else port=rdi->param;

	/* Open port */
	ri.si = SIO_Open( port );
	if( ri.si==NULL ) return( SC_EXIT_IO_ERROR );

	/* Set main port parameters */
	SIO_SetSpeed( ri.si, 115200 );
	SIO_SetDataBits( ri.si, 8 );
	SIO_SetParity( ri.si, SIO_PARITY_EVEN );
	SIO_SetStopBits( ri.si, 2 );
	SIO_SetIOMode( ri.si, SIO_IOMODE_DIRECT );

	SIO_WriteSettings( ri.si );

	/* Get unwanted bytes */
#ifdef __palmos__
	SIO_Flush(ri.si);
#else
	while(SIO_ReadChar(ri.si)!=-1);
#endif /* __palmos__ */

	SIO_WriteChar( ri.si, '\n' );
	SIO_WriteChar( ri.si, '\n' );
	SIO_WriteChar( ri.si, '\n' );

	/* Get unwanted bytes */
#ifdef __palmos__
	SIO_Flush(ri.si);
#else
	while(SIO_ReadChar(ri.si)!=-1);
#endif /* __palmos__ */

	/* Command 8: Set/reset change alert */
	buffer[ 0 ] = SC_INTERTEX_CMD_SET_ALERT;
	buffer[ 1 ] = 0;

	if( (ret=scIntertexSendCmd( &ri, SC_INTERTEX_DIR_SND_REC, buffer, 2, buffer,
		&i, 5*1000*1000 )) ==SC_EXIT_BAD_CHECKSUM )
		ret=scIntertexSendCmd( &ri, SC_INTERTEX_DIR_SND_REC, buffer, 2, buffer,
			&i, 5*1000*1000 );

	if( (ret!=SC_EXIT_OK) || (buffer[0]!=SC_INTERTEX_CMD_SET_ALERT) ) {
		scIntertexShutdown( &ri );
		return( ret );
	}

	rdi->prob=220;

	return( SC_EXIT_OK );
}

/****************************************************************************
*																			*
*							Low Level Functions								*
*																			*
****************************************************************************/

/* Get Capabilities */

int scIntertexGetCap( SC_READER_INFO *ri, SC_READER_CAP *rp )
{
	rp->t0err=FALSE; /* Not documented, so FALSE is assumed. */
	rp->t1=TRUE;
	rp->freq=3579500;
	rp->motor=FALSE;
	rp->slots=1;

	rp->n_fd=1;

	/* 9600 at 3.579MHz */
	rp->fd[0]=(((10L<<16)+372L)<<8)+1;
	rp->speed[0]=9600;

	return( SC_EXIT_OK );
}

/* Activate Card */

int scIntertexActivate( SC_READER_INFO *ri )
{
	BYTE buffer[ SC_INTERTEX_MAX_CMD_SIZE ];
	int i, ret;

	if( ri->si==NULL ) return( SC_EXIT_BAD_PARAM );

	/* Command 20: Activate asynchronous card */
	buffer[ 0 ] = SC_INTERTEX_CMD_ACTIVATE_ASY;
	buffer[ 1 ] = 0; /* Issue for T=0 */

	if( (ret=scIntertexSendCmd( ri, SC_INTERTEX_DIR_SND_REC, buffer, 2, buffer,
		&i, 5*1000*1000 )) ==SC_EXIT_BAD_CHECKSUM )
		ret=scIntertexSendCmd( ri, SC_INTERTEX_DIR_SND_REC, buffer, 2, buffer,
			&i, 5*1000*1000 );

	if( ret!=SC_EXIT_OK ) return( ret );

	if( /* (buffer[1]==SC_INTERTEX_RSP_CARD_T0) || */
		(buffer[1]==SC_INTERTEX_RSP_CARD_T1) ) {
		/* Command 20: Activate asynchronouse card */
		buffer[ 0 ] = SC_INTERTEX_CMD_ACTIVATE_ASY;
		buffer[ 1 ] = 1; /* Issue for T=1 */

		if( (ret=scIntertexSendCmd( ri, SC_INTERTEX_DIR_SND_REC, buffer, 2,
			buffer, &i, 5*1000*1000 )) ==SC_EXIT_BAD_CHECKSUM )
			ret=scIntertexSendCmd( ri, SC_INTERTEX_DIR_SND_REC, buffer, 2,
				buffer, &i, 5*1000*1000 );

		if( ret!=SC_EXIT_OK ) return( ret );
	}

	if( buffer[0]!=SC_INTERTEX_CMD_ACTIVATE_ASY )
		return( SC_EXIT_UNKNOWN_ERROR );

	switch( buffer[1] ) {
	case SC_INTERTEX_RSP_OK:
		return( SC_EXIT_OK );
	case SC_INTERTEX_RSP_REMOVED:
		return( SC_EXIT_NO_CARD );
	default:
		return( SC_EXIT_UNKNOWN_ERROR );
	}

	return( SC_EXIT_OK );
}

/* Deactivate Card */

int scIntertexDeactivate( SC_READER_INFO *ri )
{
	BYTE buffer[ SC_INTERTEX_MAX_CMD_SIZE ];
	int i, ret;

	if( ri->si==NULL ) return( SC_EXIT_BAD_PARAM );

	/* Command 2: Deactivate */
	buffer[ 0 ] = SC_INTERTEX_CMD_DEACTIVATE;
	buffer[ 1 ] = 0;

	if( (ret=scIntertexSendCmd( ri, SC_INTERTEX_DIR_SND_REC, buffer, 2, buffer,
		&i, 5*1000*1000 )) ==SC_EXIT_BAD_CHECKSUM )
		ret=scIntertexSendCmd( ri, SC_INTERTEX_DIR_SND_REC, buffer, 2, buffer,
			&i, 5*1000*1000 );

	if( ret!=SC_EXIT_OK ) return( ret );

	if( buffer[0]!=SC_INTERTEX_CMD_DEACTIVATE ) return( SC_EXIT_UNKNOWN_ERROR );
	if( buffer[1]!=SC_INTERTEX_RSP_OK ) return( SC_EXIT_UNKNOWN_ERROR );

	return( SC_EXIT_OK );
}

/* Write Buffer Async */

int scIntertexWriteBuffer( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	const BYTE *buffer, int len )
{
	BYTE rbuffer[ SC_INTERTEX_MAX_CMD_SIZE ];
	int ret;

	if( ri->si==NULL ) return( SC_EXIT_BAD_PARAM );
	if( len>SC_INTERTEX_MAX_CMD_SIZE-2 ) return( SC_EXIT_CMD_TOO_LONG );

	/* Command 21: Data to asynchronous card */
	rbuffer[ 0 ] = SC_INTERTEX_CMD_DATA_TO_ASY;
	rbuffer[ 1 ] = 0;
	memcpy( rbuffer+2, buffer, len );

	ret=scIntertexSendCmd( ri, SC_INTERTEX_DIR_SND, rbuffer, len+2, NULL,
		NULL, 0 );

	if( ret!=SC_EXIT_OK ) return( -1 );

	return( len );
}

/* Read Buffer Async */

int scIntertexReadBuffer( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	BYTE *buffer, int len , LONG timeout )
{
	BYTE rbuffer[ SC_INTERTEX_MAX_CMD_SIZE ];
	int i, ret;

	if( ri->si==NULL ) return( SC_EXIT_BAD_PARAM );
	if( len>SC_INTERTEX_MAX_CMD_SIZE-2 ) return( SC_EXIT_RSP_TOO_LONG );

	/* Fetch only data */
	ret=scIntertexSendCmd( ri, SC_INTERTEX_DIR_REC, NULL, 0, rbuffer, &i, 0 );

	if( ret!=SC_EXIT_OK ) return( -1 );
	if( rbuffer[0]!=SC_INTERTEX_CMD_DATA_TO_ASY ) return( -1 );
	if( rbuffer[1]!=SC_INTERTEX_RSP_OK ) return( -1 );
	if( (i-2)>len ) return( -1 ); /* Simple solution. */

	memcpy( buffer, rbuffer+2, i-2 );

	return( i-2 );
}

/* Wait For Data */

int scIntertexWaitForData( SC_READER_INFO *ri, SC_CARD_INFO *ci, LONG timeout )
{
	return( SIO_WaitForData( ri->si, timeout ) );
}

/* Get card status */

int scIntertexCardStatus( SC_READER_INFO *ri )
{
	BYTE buffer[ SC_INTERTEX_MAX_CMD_SIZE ];
	int i, ret;

	if( ri->si==NULL ) return( SC_EXIT_BAD_PARAM );

	/* Command 2: Deactivate */
	buffer[ 0 ] = SC_INTERTEX_CMD_GET_STATUS;
	buffer[ 1 ] = 0;

	if( (ret=scIntertexSendCmd( ri, SC_INTERTEX_DIR_SND_REC, buffer, 2, buffer,
		&i, 5*1000*1000 )) ==SC_EXIT_BAD_CHECKSUM )
		ret=scIntertexSendCmd( ri, SC_INTERTEX_DIR_SND_REC, buffer, 2, buffer,
			&i, 5*1000*1000 );

	if( ret!=SC_EXIT_OK ) return( ret );

	if( buffer[0]!=SC_INTERTEX_CMD_GET_STATUS ) return( SC_EXIT_UNKNOWN_ERROR );
	if( (buffer[1]&0x80) || (i!=2) ) return( SC_EXIT_UNKNOWN_ERROR );

	ri->status = 0;
	if( (buffer[1]==SC_INTERTEX_CARD_PRESENT) ||
		(buffer[1]==SC_INTERTEX_CARD_ACTIVATED) )
		ri->status|=SC_CARD_STATUS_PRESENT;
	if( buffer[1]==SC_INTERTEX_CARD_ABSENT_CHANGE )
		ri->status|=SC_CARD_STATUS_CHANGED;

	return( SC_EXIT_OK );
}

/* Transmit Command */
/* Adds LRC itself.
 * Reissue, if SC_EXIT_BAD_CHECKSUM is returned.
 * The direction parameter is a cludge to allow the separation into
 * WriteBuffer and ReadBuffer
 */
int scIntertexSendCmd( SC_READER_INFO *ri, int dir, const BYTE *snd,
	int sndlen, BYTE *rec, int *reclen, LONG timeout )
{
	BYTE buffer[ SC_INTERTEX_MAX_BUFFER_SIZE+1 ], xor;
	int ret, i, j;
	BOOLEAN eod, lastdle;

#ifdef READER_DEBUG
	{
		int debi;
		printf(" [IXCMD:");
		for(debi=0;debi<sndlen;debi++) printf(" %.2X",snd[debi]);
		printf("]");
	}
#endif
	if( dir&SC_INTERTEX_DIR_SND ) {
		if( (snd==NULL) || (sndlen>SC_INTERTEX_MAX_CMD_SIZE) )
		return( SC_EXIT_BAD_PARAM );
	}
	if( (dir&SC_INTERTEX_DIR_REC) && (rec==NULL) )
		return( SC_EXIT_BAD_PARAM );

	if( dir&SC_INTERTEX_DIR_SND ) {
		/* Clear port */
#ifdef __palmos__
		SIO_Flush(ri->si);
#else
		while(SIO_ReadChar(ri->si)!=-1);
#endif /* __palmos__ */

		/* Sent "AT*SC" */
		if( SIO_WriteBuffer( ri->si, "AT*SC\x0D", 6 )!=6 )
			return( SC_EXIT_IO_ERROR );
		SIO_ReadBuffer( ri->si, buffer, 6 );

		/* Wait for connect */
		SIO_WaitForData( ri->si, 1*1000*1000 );

		/* Receive Connect */
		if( SIO_ReadBuffer( ri->si, buffer, 11 )!=11 )
			return( SC_EXIT_IO_ERROR );
		if( memcmp( buffer, "\x0D\x0A\x43ONNICC\x0D\x0A", 11 ) )
			return( SC_EXIT_PROTOCOL_ERROR );

		/* Build command */
		buffer[ 0 ] = SC_INTERTEX_CHAR_DLE;
		buffer[ 1 ] = SC_INTERTEX_CHAR_STX;
		j = 2;
		xor = 0;
		for( i=0; i<sndlen; i++ ) {
			buffer[ j++ ] = snd[ i ];
			xor ^= snd[ i ];
			if( snd[ i ] == SC_INTERTEX_CHAR_DLE ) {
				buffer[ j++ ] = SC_INTERTEX_CHAR_DLE;
			}
		}
		buffer[ j++ ] = xor;
		if( xor == SC_INTERTEX_CHAR_DLE )
			buffer[ j++ ] = SC_INTERTEX_CHAR_DLE;
		buffer[ j++ ] = SC_INTERTEX_CHAR_DLE;
		buffer[ j++ ] = SC_INTERTEX_CHAR_ETX;

		/* Send command */
		if( SIO_WriteBuffer( ri->si, buffer, j )!=j )
			return( SC_EXIT_IO_ERROR );
	}

	if( dir&SC_INTERTEX_DIR_REC ) {
		/* Wait for response block */
		SIO_WaitForData( ri->si, timeout );

		/* Receive response */
		eod = FALSE;
		lastdle = FALSE;
		i = 0;
		while( !eod ) {
			SIO_WaitForData( ri->si, SIO_READ_WAIT_DEFAULT );

			ret = SIO_ReadChar( ri->si );
			if( ret==-1 ) break;

			switch( ret ) {
			case SC_INTERTEX_CHAR_DLE:
				if( lastdle ) {
					lastdle = FALSE;
					continue;
				}
				else {
					lastdle = TRUE;
				}
				break;			
			case SC_INTERTEX_CHAR_ETX:
				if( lastdle ) {
					lastdle = FALSE;
					eod = TRUE;
				}
				break;
			default:
				lastdle = FALSE;
				break;
			}

			buffer[ i++ ] = ret & 0xFF;

			if( i>=(sizeof(buffer)-6) ) return( SC_EXIT_PROTOCOL_ERROR );
		}

		if( !eod ) return( SC_EXIT_PROTOCOL_ERROR );

		if( SIO_ReadBuffer( ri->si, buffer+i, 6)!=6 )
			return( SC_EXIT_IO_ERROR );
		i+=6;

		/* TODO: Allways OK? */
		if( (buffer[0]!=SC_INTERTEX_CHAR_DLE) ||
			(buffer[1]!=SC_INTERTEX_CHAR_STX) ||
			memcmp( buffer+i-6, "\x0D\x0AOK\x0D\x0A", 6) )
			return( SC_EXIT_PROTOCOL_ERROR );

		/* Check LRC */
		xor = 0;
		for( j=2; j<(i-8); j++ ) xor ^= buffer[ j ];
		if( xor ) {
			/* Command 5: Repeat last message */
			buffer[ 0 ] = SC_INTERTEX_CMD_REPEAT_LAST;
			buffer[ 1 ] = 0;

			ret=scIntertexSendCmd( ri, SC_INTERTEX_DIR_SND_REC, buffer, 2,
				buffer, &i, 5*1000*1000 );

			if( ret!=SC_EXIT_OK ) return( ret );

			/* Copy response */
			memcpy( rec, buffer, i );
			*reclen = i;
#ifdef READER_DEBUG
			{
				int debi;
				printf(" [IXRSP:");
				for(debi=0;debi<*reclen;debi++) printf(" %.2X",rec[debi]);
				printf("]");
			}
#endif
		} else {
		/* Copy response */
		if( (i-11)>SC_INTERTEX_MAX_CMD_SIZE ) return( SC_EXIT_RSP_TOO_LONG );
		memcpy( rec, buffer+2, i-11 );
		*reclen = i-11;

		if( *reclen<2 ) return( SC_EXIT_PROTOCOL_ERROR );

		if( (buffer[1]==SC_INTERTEX_RSP_BAD_FORMAT) ||
			(buffer[1]==SC_INTERTEX_RSP_LRC_ERROR) )
			return( SC_EXIT_BAD_CHECKSUM );
		}
#ifdef READER_DEBUG
		{
			int debi;
			printf(" [IXRSP:");
			for(debi=0;debi<*reclen;debi++) printf(" %.2X",rec[debi]);
			printf("]");
		}
#endif
	}

	return( SC_EXIT_OK );
}

/****************************************************************************
*																			*
*							SmartCard Functions								*
*																			*
****************************************************************************/

/* Reset Card */

int scIntertexResetCard( SC_READER_INFO *ri, SC_CARD_INFO *ci )
{
	BYTE buffer[ SC_INTERTEX_MAX_CMD_SIZE ];
	int i, ret;

	scGeneralCleanCI( ci );

	if( ri->si==NULL ) return( SC_EXIT_BAD_PARAM );

	if( (ret=scIntertexDeactivate( ri ))!=SC_EXIT_OK ) return( ret );

	if( (ret=scIntertexActivate( ri ))!=SC_EXIT_OK ) return( ret );

	/* Command 2: Get ATR */
	buffer[ 0 ] = SC_INTERTEX_CMD_GET_ATR;
	buffer[ 1 ] = 0;

	if( (ret=scIntertexSendCmd( ri, SC_INTERTEX_DIR_SND_REC, buffer, 2, buffer,
		&i, 5*1000*1000 ))==SC_EXIT_BAD_CHECKSUM )
		ret=scIntertexSendCmd( ri, SC_INTERTEX_DIR_SND_REC, buffer, 2, buffer,
			&i, 5*1000*1000 );

	if( ret!=SC_EXIT_OK ) return( ret );

	if( buffer[0]!=SC_INTERTEX_CMD_GET_ATR ) return( SC_EXIT_UNKNOWN_ERROR );
	switch( buffer[1] ) {
	case SC_INTERTEX_RSP_OK:
		break;
	case SC_INTERTEX_RSP_REMOVED:
		return( SC_EXIT_NO_CARD );
	default:
		return( SC_EXIT_UNKNOWN_ERROR );
	}

	if( i>(32+2) ) return( SC_EXIT_BAD_ATR );
	memcpy( ci->atr, buffer+2, i-2 );
	ci->atrlen=i-2;

	scIntertexCardStatus( ri );

	return( SC_EXIT_OK );
}

/* Transmit APDU with protocol T=0 */

/* Supports only cases 1, 2 Short, 3 Short, 4 Short.
 * Case 4 Short:
 *  - You have to get the response data yourself, e.g. with GET RESPONSE
 *  - The le-byte is omitted.
 */

int scIntertexT0( SC_READER_INFO *ri, SC_CARD_INFO *ci, SC_APDU *apdu )
{
	BYTE buffer[ SC_INTERTEX_MAX_CMD_SIZE ];
	int i, ret;

	if( (ri->si==NULL) || (apdu==NULL) ) return( SC_EXIT_BAD_PARAM );
	if( apdu->cmdlen>SC_INTERTEX_MAX_CMD_SIZE-2 ) return( SC_EXIT_BAD_PARAM );

	switch( apdu->cse ) {
	case SC_APDU_CASE_2_SHORT:
		/* Command 22: Data from asynchronous card */
		buffer[ 0 ] = SC_INTERTEX_CMD_DATA_FROM_ASY;
		break;
	default:
		/* Command 21: Data to asynchronous card */
		buffer[ 0 ] = SC_INTERTEX_CMD_DATA_TO_ASY;
		break;
	}
	buffer[ 1 ] = 0;
	memcpy( buffer+2, apdu->cmd, apdu->cmdlen );
	if( apdu->cmdlen==4 ) {
		buffer[2+apdu->cmdlen]=0;
		apdu->cmdlen=5;
	}

	if( (ret=scIntertexSendCmd( ri, SC_INTERTEX_DIR_SND_REC, buffer,
		apdu->cmdlen+2, buffer, &i, 5*60*1000*1000 ))==SC_EXIT_BAD_CHECKSUM )
		ret=scIntertexSendCmd( ri, SC_INTERTEX_DIR_SND_REC, buffer,
			apdu->cmdlen+2, buffer, &i, 5*60*1000*1000 );

	if( ret!=SC_EXIT_OK ) return( ret );

	if( (buffer[0]!=SC_INTERTEX_CMD_DATA_TO_ASY) &&
		(buffer[0]!=SC_INTERTEX_CMD_DATA_FROM_ASY) )
		return( SC_EXIT_UNKNOWN_ERROR );

	switch( buffer[1] ) {
	case SC_INTERTEX_RSP_OK:
	case SC_INTERTEX_RSP_NOT_9000:
	case SC_INTERTEX_RSP_EARLY_SW:
		memcpy( apdu->rsp, buffer+2, i-2 );
		apdu->rsplen=i-2;
		break;
	case SC_INTERTEX_RSP_REMOVED:
		return( SC_EXIT_NO_CARD );
	default:
		return( SC_EXIT_UNKNOWN_ERROR );
	}

	return( SC_EXIT_OK );
}

/* Transmit APDU with protocol T=1 */

int scIntertexT1( SC_READER_INFO *ri, SC_CARD_INFO *ci, SC_APDU *apdu )
{
	if( ri->si==NULL ) return( SC_EXIT_BAD_PARAM );

	return( scT1SendCmd( ri, ci, apdu ) );
}

/* Transmit APDU */

int scIntertexSendAPDU( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	SC_APDU *apdu )
{
#ifdef READER_DEBUG
	{
		int debi;
		printf(" [CMD:");
		for(debi=0;debi<apdu->cmdlen;debi++) printf(" %.2X",apdu->cmd[debi]);
		printf("]");
	}
#endif

	if( ri->si==NULL ) return( SC_EXIT_BAD_PARAM );

	if( ci->protocol==SC_PROTOCOL_T0 ) {
		return( scT0SendAPDU( ri, ci, apdu ) );
	} else if( ci->protocol==SC_PROTOCOL_T1 ) {
		return( scIntertexT1( ri, ci, apdu ) );
	}

	return( SC_EXIT_NOT_IMPLEMENTED );
}

