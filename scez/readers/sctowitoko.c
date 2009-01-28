/****************************************************************************
*																			*
*					SCEZ chipcard library - Towitoko routines				*
*					Copyright Matthias Bruestle 1999,2000					*
*					For licensing, see the file COPYRIGHT					*
*																			*
****************************************************************************/

/* $Id: sctowitoko.c 1617 2005-11-03 17:41:39Z laforge $ */

#include <scez/scinternal.h>
#include <scez/sct0.h>
#include <scez/sct1.h>
#include <scez/scpts.h>
#include <scez/readers/sctowitoko.h>

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#if !defined(WINDOWS) && !defined(__palmos__)
#include <sys/time.h>
#include <sys/types.h>
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

int scTowitokoInit( SC_READER_INFO *ri, const char *param )
{
	BYTE buffer[3];
	BYTE cs;
	int i;
	char *port;

	if( ri->slot!=1 ) return( SC_EXIT_NO_SLOT );

	if( param==NULL ) return( SC_EXIT_BAD_PARAM );

	if( (strcmp(param,"0")==0) || (strcmp(param,"1")==0) ||
		(strcmp(param,"2")==0) || (strcmp(param,"3")==0) ) {
		port=scReaderExpandPort( param );
		if( port==NULL ) return( SC_EXIT_BAD_PARAM );
	} else port=param;

	if( ri->si!=NULL ) scTowitokoShutdown( ri );

	/* Open port */
	ri->si = SIO_Open( port );
	if( ri->si==NULL ) return( SC_EXIT_IO_ERROR );

	/* Set main port parameters */
	SIO_SetSpeed( ri->si, 9600 );
	SIO_SetDataBits( ri->si, 8 );
	SIO_SetParity( ri->si, SIO_PARITY_EVEN );
	SIO_SetStopBits( ri->si, 2 );
	SIO_RaiseRTS( ri->si ); /* +12V */
	SIO_RaiseDTR( ri->si ); /* +12V */
	SIO_SetIOMode( ri->si, SIO_IOMODE_DIRECT );

	SIO_WriteSettings( ri->si );

	/* Give it some time to collect some electrons. */
#if defined( __linux__ )
	{
		struct timeval tv;
		fd_set fds;

		FD_ZERO(&fds);
		tv.tv_sec = 1;
		tv.tv_usec = 0;
		/* this only works on Linux, because on Linux, timeout
		 * is modified to reflect the amount of time not slept.
		 */
		do {
			i = select(0, &fds, &fds, &fds, &tv);
		} while (i == -1);
	}
#elif defined( __palmos__ )
	sleep(2);
#elif defined( WINDOWS )
	Sleep(1000);
#else
	sleep(1);
#endif /* __palmos__ */

	/* Get unwanted bytes */
#ifdef __palmos__
	SIO_Flush(ri->si);
#else
	while(SIO_ReadChar(ri->si)!=-1);
#endif /* __palmos__ */

	/* Build command */
	buffer[0] = 0x00;
	buffer[1] = 0x01;

	/* Send command */
	if( SIO_WriteBuffer( ri->si, buffer, 2 ) != 2 ) {
		scTowitokoShutdown( ri );
		return( SC_EXIT_IO_ERROR );
	}

	/* Read answer */
	if( SIO_ReadBuffer( ri->si, buffer, 3 ) != 3 ) {
		scTowitokoShutdown( ri );
		return( SC_EXIT_IO_ERROR );
	}

	/* Check checksum */
	cs=0x01;

	for( i=0; i<3-1; i++ ) {
		scTowitokoUpdateCS( cs , buffer[i] );
	}

	if( cs != buffer[2] ) {
		scTowitokoShutdown( ri );
		return( SC_EXIT_BAD_CHECKSUM );
	}

	/* Set variables */
	ri->major=SC_READER_TOWITOKO;
	ri->etu=104; /* 104us */
	ri->pinpad=FALSE;
	ri->display=FALSE;

	if( (buffer[0]==SC_TOWITOKO_CHIPDRIVE_TWIN) || \
		(buffer[0]==SC_TOWITOKO_CHIPDRIVE_EXTERN) || \
		(buffer[0]==SC_TOWITOKO_CHIPDRIVE_INTERN) || \
		(buffer[0]==SC_TOWITOKO_KARTENZWERG) || \
		(buffer[0]==SC_TOWITOKO_CHIPDRIVE_MICRO) || \
		0x64 /* Some other Chipdrive Micro. */ ) {
		ri->minor=buffer[0];
		if( ri->minor==0x64 ) ri->minor=SC_TOWITOKO_CHIPDRIVE_MICRO;
	} else {
		scTowitokoShutdown( ri );
		return( SC_EXIT_NOT_SUPPORTED );
	}

	ri->scShutdown=scTowitokoShutdown;
	ri->scActivate=scTowitokoActivate;
	ri->scDeactivate=scTowitokoDeactivate;
	ri->scWriteBuffer=scTowitokoWriteBuffer;
	ri->scReadBuffer=scTowitokoReadBuffer;
	ri->scWriteChar=scTowitokoWriteChar;
	ri->scReadChar=scTowitokoReadChar;
	ri->scWaitForData=scTowitokoWaitForData;
	ri->scSetSpeed=scTowitokoSetSpeed;
	ri->scCardStatus=scTowitokoCardStatus;
	ri->scResetCard=scTowitokoResetCard;
	ri->scPTS=scPtsDoPTS;
	ri->scT0=scTowitokoT0;
	ri->scT1=scTowitokoT1;
	ri->scSendAPDU=scTowitokoSendAPDU;
	ri->scVerifyPIN=NULL;
	ri->scWaitReq=NULL;

	/* Set remaining port parameters */
	if( ri->minor==SC_TOWITOKO_KARTENZWERG ) {
		SIO_SetStopBits( ri->si, 1 );
		SIO_SetParity( ri->si, SIO_PARITY_NONE );
	} else {
		SIO_SetStopBits( ri->si, 2 );
		SIO_SetParity( ri->si, SIO_PARITY_EVEN );
	}

	if( ri->minor==SC_TOWITOKO_CHIPDRIVE_TWIN ) {
		SIO_DropRTS( ri->si ); /* -12V */
		SIO_RaiseDTR( ri->si ); /* +12V */
	} else {
		SIO_RaiseRTS( ri->si ); /* +12V */
		SIO_RaiseDTR( ri->si ); /* +12V */
	}

	SIO_WriteSettings( ri->si );

	scTowitokoLED(ri, SC_TOWITOKO_LED_ON);

	return( SC_EXIT_OK );
}

int scTowitokoShutdown( SC_READER_INFO *ri )
{
	if( ri->si==NULL ) return( SC_EXIT_OK );

	scTowitokoLED(ri, SC_TOWITOKO_LED_OFF);

	SIO_Close( ri->si );
	ri->si=NULL;

	return( SC_EXIT_OK );
}

int scTowitokoDetect( SC_READER_DETECT_INFO *rdi )
{
	SIO_INFO *si;
	char *port;
	BYTE buffer[3];
	BYTE cs;
	int i;

	rdi->prob=0;
	strncpy( rdi->name, "TOWITOKO", SC_READER_NAME_MAXLEN );
	rdi->major=SC_READER_TOWITOKO;
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
	SIO_RaiseRTS( si ); /* +12V */
	SIO_RaiseDTR( si ); /* +12V */
	SIO_SetIOMode( si, SIO_IOMODE_DIRECT );

	SIO_WriteSettings( si );

	/* Give it some time to save some electrons. */
#if defined( __linux__ )
	{
		struct timeval tv;
		fd_set fds;

		FD_ZERO(&fds);
		tv.tv_sec = 1;
		tv.tv_usec = 0;
		/* this only works on Linux, because on Linux, timeout
		 * is modified to reflect the amount of time not slept.
		 */
		do {
			i = select(0, &fds, &fds, &fds, &tv);
		} while (i == -1);
	}
#elif defined( __palmos__ )
	sleep(2);
#elif defined( WINDOWS )
	Sleep(1000);
#else
	sleep(1);
#endif /* __palmos__ */

	/* Get unwanted bytes */
#ifdef __palmos__
	SIO_Flush(si);
#else
	while(SIO_ReadChar(si)!=-1);
#endif /* __palmos__ */

	/* Send command */
	if( SIO_WriteBuffer( si, "\x00\x01", 2 ) != 2 ) {
		SIO_Close( si );
		return( SC_EXIT_IO_ERROR );
	}

	/* Read answer */
	if( SIO_ReadBuffer( si, buffer, 3 ) != 3 ) {
		SIO_Close( si );
		return( SC_EXIT_OK );
	}

	SIO_Close( si );

	/* Check checksum */
	cs=0x01;

	for( i=0; i<3-1; i++ ) scTowitokoUpdateCS( cs , buffer[i] );

	if( cs != buffer[2] ) return( SC_EXIT_OK );

	/* Set variables */
	if( (buffer[0]==SC_TOWITOKO_CHIPDRIVE_TWIN) || 
		(buffer[0]==SC_TOWITOKO_CHIPDRIVE_EXTERN) || 
		(buffer[0]==SC_TOWITOKO_CHIPDRIVE_INTERN) || 
		(buffer[0]==SC_TOWITOKO_KARTENZWERG) || 
		(buffer[0]==SC_TOWITOKO_CHIPDRIVE_MICRO) || 
		(buffer[0]==0x64) /* Some other Chipdrive Micro. */ ) {
        if( buffer[0]==0x64 ) buffer[0]=SC_TOWITOKO_CHIPDRIVE_MICRO;

		rdi->major=SC_READER_TOWITOKO;
		rdi->minor=buffer[0];
		/* Compared to random data it has a false identification of at most
		 * 1:13000. In real life false identifications should be much less
		 * common. It could be improved by sending some more commands to
		 * the reader.
		 */
		rdi->prob=200;
	}

	return( SC_EXIT_OK );
}

/****************************************************************************
*																			*
*							Low Level Functions								*
*																			*
****************************************************************************/

/* Get Capabilities */

int scTowitokoGetCap( SC_READER_INFO *ri, SC_READER_CAP *rp )
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

int scTowitokoActivate( SC_READER_INFO *ri )
{
	BYTE cmd[]={ 0x60, 0x0F, 0x9C };

	if( ri->si==NULL ) return( SC_EXIT_BAD_PARAM );

	if( SIO_WriteBuffer( ri->si, cmd, 3 )!= 3 ) {
		return( SC_EXIT_IO_ERROR );
	}

	if( SIO_ReadChar( ri->si ) != 0x01 ) {
		return( SC_EXIT_IO_ERROR );
	}

#ifdef TOWITOKO_LOW_POWER
#if defined( __linux__ )
	{
		struct timeval tv;
		fd_set fds;
		int ret;
		
		FD_ZERO(&fds);
		tv.tv_sec = 1;
		tv.tv_usec = 0;
		/* this only works in Linux, because on Linux, timeout
		 * is modified to reflect the amount of time not slept.
		 */
		do {
			ret = select(0, &fds, &fds, &fds, &tv);
		} while (ret == -1);
	}
#elif defined( WINDOWS )
	Sleep(1000);
#else
	sleep(1);
#endif /* __palmos__ */
#endif /* TOWITOKO_LOW_POWER */

	return( SC_EXIT_OK );
}

/* Deactivate Card */

int scTowitokoDeactivate( SC_READER_INFO *ri )
{
	BYTE cmd[]={ 0x61, 0x0F, 0x98 };

	if( ri->si==NULL ) return( SC_EXIT_BAD_PARAM );

	if( SIO_WriteBuffer( ri->si, cmd, 3 )!= 3 ) {
		return( SC_EXIT_IO_ERROR );
	}

	if( SIO_ReadChar( ri->si ) != 0x01 ) {
		return( SC_EXIT_IO_ERROR );
	}

	return( SC_EXIT_OK );
}

/* Write Buffer Async */

int scTowitokoWriteBuffer( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	const BYTE *buffer, int len )
{
	BYTE *tmp;

	if( (tmp=malloc( len ))==NULL ) return( -1 );
	memcpy( tmp, buffer, len );

	if( ci->direct==FALSE )
		scGeneralReverseString( tmp, len );

	if( scTowitokoSendData( ri, tmp, len ) == SC_EXIT_OK ) {
		memset( tmp, 0, sizeof(tmp) );
		if( tmp!=NULL ) free( tmp );
		return( len );
	} else {
		memset( tmp, 0, sizeof(tmp) );
		if( tmp!=NULL ) free( tmp );
		return( -1 );
	}
}

/* Read Buffer Async */

int scTowitokoReadBuffer( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	BYTE *buffer, int len , LONG timeout )
{
	int ret;

	if( timeout==SC_TIMEOUT_DEFAULT ) timeout=SIO_READ_WAIT_DEFAULT;


	if( timeout<50000L ) timeout=50000L;
	SIO_SetReadTimeout( ri->si, timeout );
	ret=SIO_ReadBuffer( ri->si, buffer, len );
	SIO_SetReadTimeout( ri->si, SIO_READ_WAIT_DEFAULT );

	if( ci->direct==FALSE )
		scGeneralReverseString( buffer, len );

	return( ret );
}

/* Write Char Async */

int scTowitokoWriteChar( SC_READER_INFO *ri, SC_CARD_INFO *ci, int ch )
{
	BYTE b;
	int i;

	if( ci->direct==TRUE ) {
		b=ch;
	} else {
		b=0x00;
		for( i=0; i<8; i++ ) {
			if( ch & (1<<i) ) b |= 0x80 >> i;
		}
		/* Hmm... */
		b^=0xFF;
	}

	if( scTowitokoSendData( ri, &b, 1 ) == SC_EXIT_OK ) {
		b=0;
		return( 1 );
	} else {
		b=0;
		return( -1 );
	}
}

/* Read Char Async */

int scTowitokoReadChar( SC_READER_INFO *ri, SC_CARD_INFO *ci, LONG timeout )
{
	int ret;
	BYTE b;
	int i;

	if( timeout==SC_TIMEOUT_DEFAULT ) timeout=SIO_READ_WAIT_DEFAULT;

	SIO_SetReadTimeout( ri->si, timeout );
	ret=SIO_ReadChar( ri->si );
	SIO_SetReadTimeout( ri->si, SIO_READ_WAIT_DEFAULT );

	if( ret==-1 ) return( ret );

	if( ci->direct==FALSE ) {	
		b=0x00;
		for( i=0; i<8; i++ ) {
			if( ret & (1<<i) ) b |= 0x80 >> i;
		}
		/* Hmm... */
		b^=0xFF;

		ret=b;
	}

	b=0;

	return( ret );
}

/* Wait For Data */

int scTowitokoWaitForData( SC_READER_INFO *ri, SC_CARD_INFO *ci, LONG timeout )
{
	return( SIO_WaitForData( ri->si, timeout ) );
}

/* Set Speed */

int scTowitokoSetSpeed( SC_READER_INFO *ri, LONG speed )
{
	if( ri->si==NULL ) return( SC_EXIT_BAD_PARAM );

	/* At least the Chipdrive Micro does only 9600 bps. */
	if( speed!=9600 ) return( SC_EXIT_BAD_PARAM );

	return( SC_EXIT_OK );
}

/* Set LED On/Off */

int scTowitokoLED( SC_READER_INFO *ri, int ledstatus )
{
	int i;
	BYTE buffer[5] = { 0x6F, 0x00, 0x6A, 0x0F, 0x00 };

	switch(ledstatus) {
	case SC_TOWITOKO_LED_RED:
	case SC_TOWITOKO_LED_GREEN:
	case SC_TOWITOKO_LED_YELLOW:
		ledstatus = SC_TOWITOKO_LED_RED;
		break;
	case SC_TOWITOKO_LED_OFF:
		break;
	default:	/* i.e. none of the above */
		return SC_EXIT_BAD_PARAM;
		break;
	}

	buffer[1] = ledstatus;
	buffer[4] = ri->slot - 1;
	for (i = 0; i < 4; i++) {
		scTowitokoUpdateCS(buffer[4], buffer[i]);
	}

	if( SIO_WriteBuffer(ri->si, buffer, 5 )!= 5 ) {
		return( SC_EXIT_IO_ERROR );
	}
	if( SIO_ReadChar( ri->si ) != 0x01 ) {
		return( SC_EXIT_IO_ERROR );
	}
	return SC_EXIT_OK;
}

/* Get card status */

int scTowitokoCardStatus( SC_READER_INFO *ri )
{
	BYTE cmd[]={ 0x03, 0x07 };
	BYTE resp[2];

	if( ri->si==NULL ) return( SC_EXIT_BAD_PARAM );

	if( SIO_WriteBuffer( ri->si, cmd, 2 )!= 2 ) {
		return( SC_EXIT_IO_ERROR );
	}

	if( SIO_ReadBuffer( ri->si, resp, 2 )!= 2 ) {
		return( SC_EXIT_IO_ERROR );
	}

	ri->status = 0;
	if( resp[0] & SC_TOWITOKO_CARD )
		ri->status|=SC_CARD_STATUS_PRESENT;
	if( resp[0] & SC_TOWITOKO_CHANGE )
		ri->status|=SC_CARD_STATUS_CHANGED;

	/* Turn on/off reader LED depending on card presence --perm */
	if (ri->status & SC_CARD_STATUS_PRESENT)
		scTowitokoLED(ri, SC_TOWITOKO_LED_ON);
	else
		scTowitokoLED(ri, SC_TOWITOKO_LED_OFF);

	return( SC_EXIT_OK );
}

/* Send Data to Reader */

int scTowitokoSendData( SC_READER_INFO *ri, BYTE *data, int len )
{
	BYTE buffer[ SC_TOWITOKO_MAX_BUFFER_SIZE ];
	int count=0;
	int send;

	if( ri->si==NULL ) return( SC_EXIT_BAD_PARAM );

	/* Static part of reader command */
	buffer[0] = 0x6F;
	buffer[2] = 0x05;

	while( count<len)
	{
		/* Bigger than maximum transfer ri->size? */
#if 0
		send = min( len-count, SC_TOWITOKO_MAX_CMD_SIZE);
#endif
		send = min( len-count, 255);

		/* Append data to command */
		memcpy( buffer+4, data+count, send );

		/* Build variable part of command */
		buffer[1] = (BYTE) send;
		buffer[3] = (((buffer[1] <<2) | (buffer[1] >>6)) & 0xFF ) ^ 0x76;

		count += send;
		send += 4;

		/* Send data to reader */
		if( SIO_WriteBuffer( ri->si, buffer, send )!= send ) {
			memset( buffer, 0, sizeof(buffer) );
			return( SC_EXIT_IO_ERROR );
		}

	/* if( count ) scReaderDelay( 1000 );*/ /* eZ */
	}

	memset( buffer, 0, sizeof(buffer) );

	return( SC_EXIT_OK );
}

/* Set Parity */
int scTowitokoSetParity( SC_READER_INFO *ri, int parity )
{
	BYTE even[]={ 0x6F, 0x40, 0x6A, 0x0F, 0x4C };
	BYTE odd[]={ 0x6F, 0x80, 0x6A, 0x0F, 0x4A };

	if( ri->si==NULL ) return( SC_EXIT_BAD_PARAM );

#ifdef __palmos__
		SIO_Flush(ri->si);
#else
		while(SIO_ReadChar(ri->si)!=-1);
#endif

	if( parity==SIO_PARITY_EVEN ) {
		if( SIO_WriteBuffer( ri->si, even, 5 )!= 5 ) {
			return( SC_EXIT_IO_ERROR );
		}

		if( SIO_ReadChar( ri->si ) != 0x01 ) {
			return( SC_EXIT_IO_ERROR );
		}
		return( SC_EXIT_OK );
	}

	if( parity==SIO_PARITY_ODD ) {
		if( SIO_WriteBuffer( ri->si, odd, 5 )!= 5 ) {
			return( SC_EXIT_IO_ERROR );
		}

		if( SIO_ReadChar( ri->si ) != 0x01 ) {
			return( SC_EXIT_IO_ERROR );
		}
		return( SC_EXIT_OK );
	}

	return( SC_EXIT_NOT_SUPPORTED );
}

int scTowitokoGetATR( SC_READER_INFO *ri, SC_CARD_INFO *ci )
{
	BYTE b;
	int i;
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
	if( ci->direct==FALSE ) {	
		b=0x00;
		for( i=0; i<8; i++ ) {
			if( c & (1<<i) ) b |= 0x80 >> i;
		}
		b^=0xFF;
		c=b;
	}
	ci->atr[ci->atrlen++]=c;

	hist=c&0x0F;

	while( (count=((c>>4)&1)+((c>>5)&1)+((c>>6)&1)+((c>>7)&1)) > 0 ) {
		if( (ci->atrlen+count) > sizeof(ci->atr) )
			return( SC_EXIT_PROTOCOL_ERROR );
		if( (ret=SIO_ReadBuffer( ri->si, ci->atr+ci->atrlen, count ))
			!= count ) return( SC_EXIT_IO_ERROR );
		if( ci->direct==FALSE )
			scGeneralReverseString( ci->atr+ci->atrlen, count );
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
		if( ci->direct==FALSE )
			scGeneralReverseString( ci->atr+ci->atrlen, hist );
		ci->atrlen+=hist;
	}

	/* TCK */
	if( prot!=SC_PROTOCOL_T0 ) {
		if( (ci->atrlen+1) > sizeof(ci->atr) ) return( SC_EXIT_PROTOCOL_ERROR );
		if( (c=SIO_ReadChar(ri->si))==-1 ) return( SC_EXIT_IO_ERROR );
		if( ci->direct==FALSE ) {	
			b=0x00;
			for( i=0; i<8; i++ ) {
				if( c & (1<<i) ) b |= 0x80 >> i;
			}
			b^=0xFF;
			c=b;
		}
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

int scTowitokoResetCard( SC_READER_INFO *ri, SC_CARD_INFO *ci )
{
	BYTE buffer[ SC_TOWITOKO_MAX_BUFFER_SIZE ];
	static BYTE reset1[] = { 0x80, 0x6F, 0x00, 0x05, 0x76 };
	static BYTE reset2[] = { 0xA0, 0x6F, 0x00, 0x05, 0x74 };
	int i;
	int ret;
	BOOLEAN valid=FALSE;

	scGeneralCleanCI( ci );

	if( ri->si==NULL ) return( SC_EXIT_BAD_PARAM );

	/* Set Parity Even */
	ret = scTowitokoSetParity( ri, SIO_PARITY_EVEN );
	if( ret ) return ( ret );
	SIO_SetParity( ri->si, SIO_PARITY_EVEN );
	SIO_WriteSettings( ri->si );

#ifdef __palmos__
	SIO_Flush(ri->si);
#else
	while(SIO_ReadChar(ri->si)!=-1);
#endif

	for( i=0; i<3; i++ ) {
		if( SIO_WriteBuffer( ri->si, reset1, 5 )!=5 ) {
			return( SC_EXIT_IO_ERROR );
		}

		if( SIO_ReadBuffer( ri->si, buffer, 1 )==1 ) {
			if( (buffer[0]==0x3B) || (buffer[0]==0x03) ) {
				valid=TRUE;
				break;
			}
		}

#ifdef __palmos__
		SIO_Flush(ri->si);
#else
		while(SIO_ReadChar(ri->si)!=-1);
#endif

		if( SIO_WriteBuffer( ri->si, reset2, 5 )!=5 ) {
			return( SC_EXIT_IO_ERROR );
		}

		if( SIO_ReadBuffer( ri->si, buffer, 1 )==1 ) {
			if( (buffer[0]==0x3B) || (buffer[0]==0x03) ) {
				valid=TRUE;
				break;
			}
		}

#ifdef __palmos__
		SIO_Flush(ri->si);
#else
		while(SIO_ReadChar(ri->si)!=-1);
#endif
	}

	if( !valid ) {
		/* Set Parity Odd */
		ret = scTowitokoSetParity( ri, SIO_PARITY_ODD );
		if( ret ) return ( ret );
		SIO_SetParity( ri->si, SIO_PARITY_ODD );
		SIO_WriteSettings( ri->si );

		for( i=0; i<3; i++ ) {
			if( SIO_WriteBuffer( ri->si, reset1, 5 )!=5 ) {
				return( SC_EXIT_IO_ERROR );
			}

			if( SIO_ReadBuffer( ri->si, buffer, 1 )==1 ) {
				if( (buffer[0]==0x3B) || (buffer[0]==0x03) ) {
					valid=TRUE;
					break;
				}
			}

#ifdef __palmos__
			SIO_Flush(ri->si);
#else
			while(SIO_ReadChar(ri->si)!=-1);
#endif

			if( SIO_WriteBuffer( ri->si, reset2, 5 )!=5 ) {
				return( SC_EXIT_IO_ERROR );
			}

			if( SIO_ReadBuffer( ri->si, buffer, 1 )==1 ) {
				if( (buffer[0]==0x3B) || (buffer[0]==0x03) ) {
					valid=TRUE;
					break;
				}
			}

#ifdef __palmos__
			SIO_Flush(ri->si);
#else
			while(SIO_ReadChar(ri->si)!=-1);
#endif
		}
	}

	if( !valid ) {
		return( SC_EXIT_BAD_ATR );
	}

	ci->atr[0]=buffer[0];

	if( buffer[0]==0x03 ) {
		/* Reverse bit order. */
		scGeneralReverseString( ci->atr, 1);
		ci->direct=FALSE;
	} else ci->direct=TRUE;

	if( (ret=scTowitokoGetATR( ri, ci ))!=SC_EXIT_OK ) {
		SIO_SetReadTimeout(ri->si, SIO_READ_WAIT_DEFAULT );
		return( ret );
	}

	/* Set Parity according to ATR */
	if( (buffer[0]==0x3B) && (SIO_GetParity(ri->si)==SIO_PARITY_ODD) ) {
		ret = scTowitokoSetParity( ri, SIO_PARITY_EVEN );
		if( ret ) return ( ret );
		SIO_SetParity( ri->si, SIO_PARITY_EVEN );
		SIO_WriteSettings( ri->si );
	}

	if( (buffer[0]==0x03) && (SIO_GetParity(ri->si)==SIO_PARITY_EVEN) ) {
		ret = scTowitokoSetParity( ri, SIO_PARITY_ODD );
		if( ret ) return ( ret );
		SIO_SetParity( ri->si, SIO_PARITY_ODD );
		SIO_WriteSettings( ri->si );
	}

	scTowitokoCardStatus( ri );

	return( SC_EXIT_OK );
}

/* Transmit APDU with protocol T=0 */

/* Supports only cases 1, 2 Short, 3 Short, 4 Short.
 * Case 4 Short:
 *  - You have to get the response data yourself, e.g. with GET RESPONSE
 *  - The le-byte is omitted.
 */

int scTowitokoT0( SC_READER_INFO *ri, SC_CARD_INFO *ci, SC_APDU *apdu )
{
	if( ri->si==NULL ) return( SC_EXIT_BAD_PARAM );

	return( scT0SendCmd( ri, ci, apdu ) );
}

/* Transmit APDU with protocol T=1 */

int scTowitokoT1( SC_READER_INFO *ri, SC_CARD_INFO *ci, SC_APDU *apdu )
{
	if( ri->si==NULL ) return( SC_EXIT_BAD_PARAM );

	return( scT1SendCmd( ri, ci, apdu ) );
}

/* Transmit APDU */

int scTowitokoSendAPDU( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	SC_APDU *apdu )
{
#ifdef READER_DEBUG
int debi;
printf(" [CMD:");
for(debi=0;debi<apdu->cmdlen;debi++) printf(" %.2X",apdu->cmd[debi]);
printf("]");
#endif

	if( ri->si==NULL ) return( SC_EXIT_BAD_PARAM );

	if( ci->protocol==SC_PROTOCOL_T0 ) {
		return( scT0SendAPDU( ri, ci, apdu ) );
	} else if( ci->protocol==SC_PROTOCOL_T1 ) {
		return( scTowitokoT1( ri, ci, apdu ) );
	}

	return( SC_EXIT_NOT_IMPLEMENTED );
}



