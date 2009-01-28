/****************************************************************************
*																			*
*				SCEZ chipcard library - Dumb Mouse routines					*
*					Copyright Matthias Bruestle 1999,2000					*
*					For licensing, see the file COPYRIGHT					*
*																			*
****************************************************************************/

/* $Id: scdumbmouse.c 1617 2005-11-03 17:41:39Z laforge $ */

#include <scez/scinternal.h>
#include <scez/readers/scdumbmouse.h>
#include <scez/sct0.h>
#include <scez/sct1.h>
#include <scez/scpts.h>

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/****************************************************************************
*																			*
*						 		Init/Shutdown Routines						*
*																			*
****************************************************************************/

/* Initializes reader and sets ri */

int scDumbmouseInit( SC_READER_INFO *ri, const char *param )
{
	char *port;

	if( ri->slot!=1 ) return( SC_EXIT_NO_SLOT );

	if( param==NULL ) return( SC_EXIT_BAD_PARAM );

	if( (strcmp(param,"0")==0) || (strcmp(param,"1")==0) ||
		(strcmp(param,"2")==0) || (strcmp(param,"3")==0) ) {
		port=scReaderExpandPort( param );
		if( port==NULL ) return( SC_EXIT_BAD_PARAM );
	} else port=param;

	if( ri->si!=NULL ) scDumbmouseShutdown( ri );

	/* Open port */
	ri->si = SIO_Open( port );
	if( ri->si==NULL ) return( SC_EXIT_IO_ERROR );

	ri->major=SC_READER_DUMBMOUSE;
	ri->minor=SC_DUMBMOUSE_DUMBMOUSE;
	ri->etu=104;
	ri->pinpad=FALSE;
	ri->display=FALSE;

	ri->scShutdown=scDumbmouseShutdown;
	ri->scGetCap=scDumbmouseGetCap;
	ri->scActivate=scDumbmouseActivate;
	ri->scDeactivate=scDumbmouseDeactivate;
	ri->scWriteBuffer=scDumbmouseWriteBuffer;
	ri->scReadBuffer=scDumbmouseReadBuffer;
	ri->scWriteChar=scDumbmouseWriteChar;
	ri->scReadChar=scDumbmouseReadChar;
	ri->scWaitForData=scDumbmouseWaitForData;
	ri->scSetSpeed=scDumbmouseSetSpeed;
	ri->scCardStatus=scDumbmouseCardStatus;
	ri->scResetCard=scDumbmouseResetCard;
	ri->scPTS=scPtsDoPTS;
	ri->scT0=scDumbmouseT0;
	ri->scT1=scDumbmouseT1;
	ri->scSendAPDU=scDumbmouseSendAPDU;
	ri->scVerifyPIN=NULL;
	ri->scWaitReq=NULL;

	/* Set main port parameters */
	SIO_SetSpeed( ri->si, 9600 );
	SIO_SetDataBits( ri->si, 8 );
	SIO_SetParity( ri->si, SIO_PARITY_EVEN );
	SIO_SetStopBits( ri->si, 2 );
	SIO_SetIOMode( ri->si, SIO_IOMODE_DIRECT );
	SIO_WriteSettings( ri->si );

	SIO_RaiseDTR( ri->si ); /* +12V */
	/* Raise RTS for Active Low and UniProg detection. */
	SIO_RaiseRTS( ri->si ); /* +12V */
	SIO_ReadControlState( ri->si );

	/* Get unwanted bytes */
#ifdef __palmos__
	SIO_Flush(ri->si);
#else
	while(SIO_ReadChar(ri->si)!=-1);
#endif

	if( SIO_GetControlState( ri->si, SIO_CONTROL_CTS )==0 ) {
		ri->minor=SC_DUMBMOUSE_UNIPROG;
		SIO_FilterEcho(ri->si,1);
	} else { /* Check if the reader echoes sent characters */
		SIO_WriteChar( ri->si, 0x12 );	/* Send an arbitrary byte */
		if (SIO_ReadChar( ri->si ) == 0x12) {
			ri->minor=SC_DUMBMOUSE_PHOENIX;
			SIO_FilterEcho(ri->si,1);
		}
	}

	return( SC_EXIT_OK );
}

int scDumbmouseShutdown( SC_READER_INFO *ri )
{
	if( ri->si==NULL ) return( SC_EXIT_OK );

	SIO_Close( ri->si );
	ri->si=NULL;

	return( SC_EXIT_OK );
}

int scDumbmouseDetect( SC_READER_DETECT_INFO *rdi )
{
	SIO_INFO *si;
	SC_READER_INFO ri;
	SC_CARD_INFO ci;
	char *port;
	int ret;

	rdi->prob=0;
	strncpy( rdi->name, "DUMBMOUSE", SC_READER_NAME_MAXLEN );
	rdi->major=SC_READER_DUMBMOUSE;
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
	si = SIO_Open( port );
	if( si==NULL ) return( SC_EXIT_IO_ERROR );

	/* Set main port parameters */
	SIO_SetSpeed( si, 9600 );
	SIO_SetDataBits( si, 8 );
	SIO_SetParity( si, SIO_PARITY_EVEN );
	SIO_SetStopBits( si, 2 );
	SIO_SetIOMode( si, SIO_IOMODE_DIRECT );
	SIO_WriteSettings( si );

	SIO_DropRTS( si );
	SIO_ReadControlState( si );
	if( SIO_GetControlState( si, SIO_CONTROL_CTS )==1 ) {
		SIO_Close( si );
		return( SC_EXIT_OK );
	}

	SIO_RaiseRTS( si ); /* +12V */
	SIO_ReadControlState( si );
	if( SIO_GetControlState( si, SIO_CONTROL_CTS )==0 ) {
		rdi->minor=SC_DUMBMOUSE_UNIPROG;
		/* We have successfully opened a serial port and it is possible,
		 * that someone has attached a UniProg at it.
		 */
		rdi->prob=1;
	} else {
		/* We have successfully opened a serial port and RTS is connected
		 * to CTS. The Dumb Mouse and many other devices, e.g. ACR20S,
		 * or Phoenix qualify for this.
		 */
          
          	/* Get unwanted bytes */
#ifdef __palmos__
   		SIO_Flush(si);
#else
   		while(SIO_ReadChar(si)!=-1);
#endif

		rdi->prob=16;
    	rdi->minor=SC_DUMBMOUSE_DUMBMOUSE;

        /* Check if the reader echoes sent characters */
		SIO_WriteChar( si, 0x12 );	/* Send an arbitrary byte */
		if (SIO_ReadChar( si ) == 0x12) {
			rdi->prob=32;
			rdi->minor=SC_DUMBMOUSE_PHOENIX;
			SIO_FilterEcho(si,1);
		}
	}

	SIO_Close( si );

	ri.si=NULL;
	ri.slot=1;

	ret=scDumbmouseInit( &ri, port );
	if( ret!=SC_EXIT_OK ) {
		rdi->prob=0;
		scDumbmouseShutdown( &ri );
		return( SC_EXIT_OK );
	}

	/* We just try a reset. Maybe we have luck and the user has left a
	 * smart card stuck in the reader. If we get a valid ATR back, the
	 * device has to be at least similar to a Dumb Mouse.
	 */
	ret=scDumbmouseResetCard( &ri, &ci );
	if( ret==SC_EXIT_OK ) rdi->prob=100;

	scDumbmouseShutdown( &ri );

	return( SC_EXIT_OK );
}

/****************************************************************************
*																			*
*							Low Level Functions								*
*																			*
****************************************************************************/

/* Get Capabilities */

int scDumbmouseGetCap( SC_READER_INFO *ri, SC_READER_CAP *rp )
{
	rp->t0err=FALSE; /* Serial port can't it. */
	rp->t1=TRUE;
	rp->freq=3579500;
	rp->motor=FALSE;
	rp->slots=1;

#if defined(__WIN32__)||defined(__WIN16__)
	rp->n_fd=6;

	/* 9600 at 3.579MHz */
	rp->fd[0]=(((10L<<16)+372L)<<8)+1;
	rp->speed[0]=9600;

	/* 19200 at 3.579MHz */
	rp->fd[1]=(((19L<<16)+372L)<<8)+2;
	rp->speed[1]=19200;
	rp->fd[2]=(((19L<<16)+744L)<<8)+4;
	rp->speed[2]=19200;

	/* 38400 at 3.579MHz */
	rp->fd[3]=(((38L<<16)+372L)<<8)+4;
	rp->speed[3]=38400;
	rp->fd[4]=(((38L<<16)+744L)<<8)+8;
	rp->speed[4]=38400;

	/* 56000 at 3.579MHz */
	rp->fd[5]=(((56L<<16)+512L)<<8)+8;
	rp->speed[5]=56000;

	/* 128000 at 3.579MHz */

	/* 256000 at 3.579MHz */

#else /* UNIX and PalmOS */	
	rp->n_fd=6;

	/* 9600 at 3.579MHz */
	rp->fd[0]=(((10L<<16)+372L)<<8)+1;
	rp->speed[0]=9600;

	/* 19200 at 3.579MHz */
	rp->fd[1]=(((19L<<16)+372L)<<8)+2;
	rp->speed[1]=19200;
	rp->fd[2]=(((19L<<16)+744L)<<8)+4;
	rp->speed[2]=19200;

	/* 38400 at 3.579MHz */
	rp->fd[3]=(((38L<<16)+372L)<<8)+4;
	rp->speed[3]=38400;
	rp->fd[4]=(((38L<<16)+744L)<<8)+8;
	rp->speed[4]=38400;

	/* 57600 at 3.579MHz */
	rp->fd[5]=(((58L<<16)+744L)<<8)+12;
	rp->speed[5]=57600;

	/* 115200 at 3.579MHz */

	/* 230400 at 3.579MHz */

#endif

	return( SC_EXIT_OK );
}

/* Activate Card */

int scDumbmouseActivate( SC_READER_INFO *ri )
{
	if( ri->si==NULL ) return( SC_EXIT_BAD_PARAM );

	return( SC_EXIT_OK );
}

/* Deactivate Card */

int scDumbmouseDeactivate( SC_READER_INFO *ri )
{
	if( ri->si==NULL ) return( SC_EXIT_BAD_PARAM );

	return( SC_EXIT_OK );
}

/* Write Buffer Async */

int scDumbmouseWriteBuffer( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	const BYTE *buffer, int len )
{
	return( SIO_WriteBuffer( ri->si, buffer, len ) );
}

/* Read Buffer Async */

int scDumbmouseReadBuffer( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	BYTE *buffer, int len, LONG timeout )
{
	int ret;

	if( timeout==SC_TIMEOUT_DEFAULT ) timeout=SIO_READ_WAIT_DEFAULT;

	SIO_SetReadTimeout( ri->si, timeout );
	ret=SIO_ReadBuffer( ri->si, buffer, len );
	SIO_SetReadTimeout( ri->si, SIO_READ_WAIT_DEFAULT );

	return( ret );
}

/* Write Char Async */

int scDumbmouseWriteChar( SC_READER_INFO *ri, SC_CARD_INFO *ci, int ch )
{
	return( SIO_WriteChar( ri->si, ch ) );
}

/* Read Char Async */

int scDumbmouseReadChar( SC_READER_INFO *ri, SC_CARD_INFO *ci, LONG timeout )
{
	int ret;

	if( timeout==SC_TIMEOUT_DEFAULT ) timeout=SIO_READ_WAIT_DEFAULT;

	SIO_SetReadTimeout( ri->si, timeout );
	ret=SIO_ReadChar( ri->si );
	SIO_SetReadTimeout( ri->si, SIO_READ_WAIT_DEFAULT );

	return( ret );
}

/* Wait For Data */

int scDumbmouseWaitForData( SC_READER_INFO *ri, SC_CARD_INFO *ci, LONG timeout )
{
	return( SIO_WaitForData( ri->si, timeout ) );
}

/* Set Speed */

/* Valid SIO speeds:
 * Windows:
 *    110, 300, 600, 1200, 2400, 4800, 9600, 14400, 19200, 38400, 56000, 128000,
 *    256000
 * Unix:
 *    50, 75, 110, 134, 150, 200, 300, 600, 1200, 1800, 2400, 4800, 9600, 19200,
 *    38400, 57600, 115200, 230400
 * PalmOS:
 *    9600, ???
 */

int scDumbmouseSetSpeed( SC_READER_INFO *ri, LONG speed )
{
	if( ri->si==NULL ) return( SC_EXIT_BAD_PARAM );

	switch( speed ) {
#if defined(__WIN32__)||defined(__WIN16__)
		case 9600:
		case 14400:
		case 19200:
		case 38400:
		case 56000:
		case 128000:
		case 256000:
#else /* UNIX and PalmOS */
		case 9600:
		case 19200:
		case 38400:
		case 57600:
		case 115200:
		case 230400:
#endif
			break;
		default:
			return( SC_EXIT_BAD_PARAM );
	}

	SIO_SetSpeed( ri->si, speed );
	SIO_WriteSettings( ri->si );

	ri->etu=1000000/speed;

	return( SC_EXIT_OK );
}

/* Get card status */

int scDumbmouseCardStatus( SC_READER_INFO *ri )
{
	int ret;

	ri->status=0x00;

	if( ri->si==NULL ) return( SC_EXIT_BAD_PARAM );

	/* For readers like the one designed by Dejan Kaljevic which have no
	 * line to detect if a card is inserted.
	 */
	if( ri->minor==SC_DUMBMOUSE_DEJAN ) ri->status=SC_CARD_STATUS_PRESENT;

	/* DCD would be better, because it would work with UniProg,
	 * but DCD is not by default included and I don't have the
	 * knowledge to include support for DCD into the windows
	 * version.
	 * The palm has no DCD, but the adapter can be changed to wire
	 * DCD to CTS.
	 */
	SIO_ReadControlState( ri->si );
#ifdef WITH_DCD
	if( (ret=SIO_GetControlState( ri->si, SIO_CONTROL_DCD ))==-1 )
		return( SC_EXIT_IO_ERROR );

	/* uhn: UNIPROG clears DCD when a card is inserted */
	if( ri->minor==SC_DUMBMOUSE_UNIPROG ) {
		if( ret==0 ) ri->status=SC_CARD_STATUS_PRESENT;
	} else {
		if ( ret ) ri->status=SC_CARD_STATUS_PRESENT;
	}
#else
#ifdef __palmos__
	/* DSR is in my adapter CTS because there is no DSR. */
	if( (ret=SIO_GetControlState( ri->si, SIO_CONTROL_CTS ))==-1 )
		return( SC_EXIT_IO_ERROR );
#else
	/* Set allways SC_CARD_STATUS_PRESENT for UniProg. (See above.) */
	if( ri->minor==SC_DUMBMOUSE_UNIPROG ) ri->status=SC_CARD_STATUS_PRESENT;

	if( (ret=SIO_GetControlState( ri->si, SIO_CONTROL_DSR ))==-1 )
		return( SC_EXIT_IO_ERROR );
#endif /* __palmos__ */

	if( ret ) ri->status=SC_CARD_STATUS_PRESENT;
#endif /* WITH_DCD */

	return( SC_EXIT_OK );
}

/* Reset and fetch TS */

int scDumbmouseGetTS( SC_READER_INFO *ri, SC_CARD_INFO *ci )
{
	int c;

#ifdef __palmos__
	SIO_Flush(ri->si);
#else
	while(SIO_ReadChar(ri->si)!=-1);
#endif

	/* Reset Card */
	if (ri->minor == SC_DUMBMOUSE_UNIPROG) {
		SIO_RaiseRTS(ri->si);
		SIO_Delay(ri->si,25);
		SIO_DropRTS(ri->si);
		SIO_Delay(ri->si,25);
	} else {
		SIO_DropRTS(ri->si);
		SIO_Delay(ri->si,25);

		if( (c=SIO_ReadChar(ri->si))!=-1 ) return( c );

		SIO_RaiseRTS(ri->si);
		SIO_Delay(ri->si,25);
	}

	return( SIO_ReadChar(ri->si) );
}

/* Get Rest of ATR */

int scDumbmouseGetATR( SC_READER_INFO *ri, SC_CARD_INFO *ci )
{
	int ret;
	int c;
	int count;
	int prot=SC_PROTOCOL_T0;
	int hist;

	ci->atrlen=1;

	/* Initial Waiting time of 9600etu */
	SIO_SetReadTimeout(ri->si,1000000);

	/* T0 */
	if( (c=SIO_ReadChar(ri->si))==-1 ) return( SC_EXIT_IO_ERROR );
	ci->atr[ci->atrlen++]=c;

	hist=c&0x0F;

	while( (count=((c>>4)&1)+((c>>5)&1)+((c>>6)&1)+((c>>7)&1)) > 0 ) {
		if( (ci->atrlen+count) > sizeof(ci->atr) )
			return( SC_EXIT_PROTOCOL_ERROR );
		if( (ret=SIO_ReadBuffer( ri->si, ci->atr+ci->atrlen, count ))
			!= count ) return( SC_EXIT_IO_ERROR );
		ci->atrlen+=count;

		/* TDx present */
		if( c&0x80 ) {
			c=ci->atr[ci->atrlen-1];
			prot=c&0x0F;
		} else {
			c=0x00;
		}
	}

	/* Historical */
	if( (ci->atrlen+hist) > sizeof(ci->atr) ) return( SC_EXIT_PROTOCOL_ERROR );
	if( hist ) {
		if( (ret=SIO_ReadBuffer( ri->si, ci->atr+ci->atrlen, hist ))
			!= hist ) return( SC_EXIT_IO_ERROR );
		ci->atrlen+=hist;
	}

	/* TCK */
	if( prot!=SC_PROTOCOL_T0 ) {
		if( (ci->atrlen+1) > sizeof(ci->atr) ) return( SC_EXIT_PROTOCOL_ERROR );
		if( (c=SIO_ReadChar(ri->si))==-1 ) return( SC_EXIT_IO_ERROR );
		ci->atr[ci->atrlen++]=c;
	}

	SIO_SetReadTimeout(ri->si,SIO_READ_WAIT_DEFAULT);

	return( SC_EXIT_OK );
}

/****************************************************************************
*																			*
*							SmartCard Functions								*
*																			*
****************************************************************************/

/* Reset Card */

int scDumbmouseResetCard( SC_READER_INFO *ri, SC_CARD_INFO *ci )
{
	int i, c;
	int ret;
	BOOLEAN valid=FALSE;

	scGeneralCleanCI( ci );

	if( ri->si==NULL ) return( SC_EXIT_BAD_PARAM );

	SIO_SetSpeed( ri->si, 9600 );
	SIO_SetDataBits( ri->si, 8 );
	SIO_SetParity( ri->si, SIO_PARITY_EVEN );
	SIO_SetStopBits( ri->si, 2 );
	SIO_SetIOMode( ri->si, SIO_IOMODE_DIRECT );
	SIO_WriteSettings( ri->si );

	for( i=0; i<3; i++ ) {
		/* Set Port for direct convention */
		SIO_SetParity( ri->si, SIO_PARITY_EVEN );
		SIO_SetIOMode( ri->si, SIO_IOMODE_DIRECT );
		SIO_WriteSettings( ri->si );

		if( (c=scDumbmouseGetTS( ri, ci ))!=-1 ) {
			if( c==0x3B ) {
				ci->atr[0]=c;
				if( (ret=scDumbmouseGetATR( ri, ci ))!=SC_EXIT_OK ) {
					SIO_SetReadTimeout(ri->si,SIO_READ_WAIT_DEFAULT);
					return( ret );
				}

				valid=TRUE;
				break;
			}
		}

		/* Set Port for inverse convention */
		SIO_SetParity( ri->si, SIO_PARITY_ODD );
		SIO_SetIOMode( ri->si, SIO_IOMODE_INDIRECT );
		SIO_WriteSettings( ri->si );

		if( (c=scDumbmouseGetTS( ri, ci ))!=-1 ) {
			if( c==0x3F ) {
				ci->atr[0]=c;
				if( (ret=scDumbmouseGetATR( ri, ci ))!=SC_EXIT_OK ) {
					SIO_SetReadTimeout(ri->si,SIO_READ_WAIT_DEFAULT);
					return( ret );
				}

				valid=TRUE;
				break;
			}
		}
	}

	if( !valid ) return( SC_EXIT_BAD_ATR );

	return( SC_EXIT_OK );
}

/* Transmit APDU with protocol T=0 */

/* Supports only cases 1, 2 Short, 3 Short, 4 Short.
 * Case 4 Short:
 *  - You have to get the response data yourself, e.g. with GET RESPONSE
 *  - The le-byte is omitted.
 */

int scDumbmouseT0( SC_READER_INFO *ri, SC_CARD_INFO *ci, SC_APDU *apdu )
{
	if( ri->si==NULL ) return( SC_EXIT_BAD_PARAM );

	return( scT0SendCmd( ri, ci, apdu ) );
}

/* Transmit APDU with protocol T=1 */

int scDumbmouseT1( SC_READER_INFO *ri, SC_CARD_INFO *ci, SC_APDU *apdu )
{
	if( ri->si==NULL ) return( SC_EXIT_BAD_PARAM );

	return( scT1SendCmd( ri, ci, apdu ) );
}

/* Transmit APDU */

int scDumbmouseSendAPDU( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	SC_APDU *apdu )
{
#ifdef READER_DEBUG
	int debi;
	printf(" [CMD: %d /",apdu->cse);
	for(debi=0;debi<apdu->cmdlen;debi++) printf(" %.2X",apdu->cmd[debi]);
	printf("]");
#endif

#ifdef SO_ONLY_TEST
	return( SC_EXIT_OK );
#endif

	if( ri->si==NULL ) return( SC_EXIT_BAD_PARAM );

	if( ci->protocol==SC_PROTOCOL_T0 ) {
		return( scT0SendAPDU( ri, ci, apdu ) );
	} else if( ci->protocol==SC_PROTOCOL_T1 ) {
		return( scDumbmouseT1( ri, ci, apdu ) );
	}

	return( SC_EXIT_NOT_IMPLEMENTED );
}



