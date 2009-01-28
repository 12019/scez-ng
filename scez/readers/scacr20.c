/****************************************************************************
*																			*
*					SCEZ chipcard library - ACR20 routines					*
*						Copyright Matthias Bruestle 1999					*
*					For licensing, see the file COPYRIGHT					*
*																			*
****************************************************************************/

/* $Id: scacr20.c 1617 2005-11-03 17:41:39Z laforge $ */

/* TODO: Wipe arrays. */

#include <scez/scinternal.h>
#include <scez/sct0.h>
#include <scez/readers/scacr20.h>

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#if !defined(WINDOWS) && !defined(__palmos__)
#include <unistd.h> /* for sleep */
#elif defined(__BORLANDC__)
#include <dos.h>    /* for sleep */
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

int scAcr20Init( SC_READER_INFO *ri, const char *param )
{
	BYTE getacrstat[] = { 0x01, 0x00 };
	BYTE setprotocol[] = { 0x03, 0x02, 0x00, 0x10 };
	BYTE setnotification[] = { 0x06, 0x01, 0x02 };

	/* Length of response to GET_ACR_STAT is 19 bytes. Additional bytes
	 * is here, so that it is possible to check if response is to long.
	 */
	BYTE rsp[ 19+1 ];
	int ret, rsplen;
	char *port;

	if( ri->slot!=1 ) return( SC_EXIT_NO_SLOT );

	if( param==NULL ) return( SC_EXIT_BAD_PARAM );

	if( (strcmp(param,"0")==0) || (strcmp(param,"1")==0) ||
		(strcmp(param,"2")==0) || (strcmp(param,"3")==0) ) {
		port=scReaderExpandPort( param );
		if( port==NULL ) return( SC_EXIT_BAD_PARAM );
	} else port=param;

	if( ri->si!=NULL ) scAcr20Shutdown( ri );

	/* Open port */
	ri->si = SIO_Open( port );
	if( ri->si==NULL ) return( SC_EXIT_IO_ERROR );

	/* Set main port parameters */
	SIO_SetSpeed( ri->si, 9600 );
	SIO_SetDataBits( ri->si, 8 );
	SIO_SetParity( ri->si, SIO_PARITY_NONE );
	SIO_SetStopBits( ri->si, 1 );
	SIO_SetIOMode( ri->si, SIO_IOMODE_DIRECT );
	SIO_WriteSettings( ri->si );

	/* Get unwanted bytes */
#ifdef __palmos__
	SIO_Flush(ri->si);
#else
	while(SIO_ReadChar(ri->si)!=-1);
#endif /* __palmos__ */

#ifdef ACR20_DEBUG
SIO_SetLogFile( ri->si, "LogAcr20.txt" );
printf("[LOG]");
#endif

	/* Wait for status message. */
	SIO_WaitForData( ri->si, 2000000 );

	/* Send GET_ACR_STAT command to see if a Acr20 is there and
	 * to get MAX_C and MAX_R values.
	 */
	rsplen=sizeof(rsp);
	ret = scAcr20SendCmd( ri, getacrstat, rsp, &rsplen, 0 );
	if( ret!=SC_EXIT_OK ) {
		scAcr20Shutdown( ri );
		return( ret );
	}

	if( rsp[0]!=0x90 ) return( SC_EXIT_UNKNOWN_ERROR );

	/* Response should be 19 bytes long. */
	if( rsplen!=19 ) return( SC_EXIT_PROTOCOL_ERROR );

	/* Get maximum data lengths. */
	ri->maxc = rsp[3+10];
	ri->maxr = rsp[3+11];

	/* Send SET_NOTIFICATION command. */
	rsplen=sizeof(rsp);
	ret = scAcr20SendCmd( ri, setnotification, rsp, &rsplen, 0 );
	if( ret!=SC_EXIT_OK ) {
		scAcr20Shutdown( ri );
		return( ret );
	}

	if( rsp[0]!=0x90 ) return( SC_EXIT_UNKNOWN_ERROR );

	/* Send SET_PROTOCOL command. */
	/* For all current ports, i.e. Unix, PalmOS, MS-Windows, set to 38400. */
	rsplen=sizeof(rsp);
	ret = scAcr20SendCmd( ri, setprotocol, rsp, &rsplen, 0 );
	if( ret!=SC_EXIT_OK ) {
		scAcr20Shutdown( ri );
		return( ret );
	}

	if( rsp[0]!=0x90 ) return( SC_EXIT_UNKNOWN_ERROR );

	/* Set port parameters */
	/* For all current ports, i.e. Unix, PalmOS, MS-Windows, set to 38400. */
	SIO_SetSpeed( ri->si, 38400 );
	SIO_WriteSettings( ri->si );

	ri->major=SC_READER_ACR20;
	ri->minor=SC_ACR20_SERIAL;
	ri->etu=104;

	ri->scShutdown=scAcr20Shutdown;
	ri->scGetCap=scAcr20GetCap;
	ri->scActivate=scAcr20Activate;
	ri->scDeactivate=scAcr20Deactivate;
	ri->scWriteBuffer=NULL;
	ri->scReadBuffer=NULL;
	ri->scWriteChar=NULL;
	ri->scReadChar=NULL;
	ri->scSetSpeed=NULL;
	ri->scCardStatus=scAcr20CardStatus;
	ri->scResetCard=scAcr20ResetCard;
	ri->scPTS=NULL;
	ri->scT0=scAcr20T0;
	ri->scT1=scAcr20T1;
	ri->scSendAPDU=scAcr20SendAPDU;
	ri->scVerifyPIN=NULL;
	ri->scWaitReq=NULL;

	return( SC_EXIT_OK );
}

int scAcr20Shutdown( SC_READER_INFO *ri )
{
	if( ri->si==NULL ) return( SC_EXIT_OK );

	SIO_Close( ri->si );
	ri->si=NULL;

	return( SC_EXIT_OK );
}

int scAcr20Detect( SC_READER_DETECT_INFO *rdi )
{
	SIO_INFO *si;
	char *port;
	BYTE buffer[14];

	rdi->prob=0;
	strncpy( rdi->name, "ACR20", SC_READER_NAME_MAXLEN );
	rdi->major=SC_READER_ACR20;
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

	/* Get unwanted bytes */
#ifdef __palmos__
	SIO_Flush(si);
#else
	while(SIO_ReadChar(si)!=-1);
#endif /* __palmos__ */

	/* Wait for status message. */
	SIO_WaitForData( si, 2000000 );

	if( SIO_ReadBuffer( si, buffer, 14 ) != 14 ) {
		SIO_Close( si );
		return( SC_EXIT_OK );
	}

	SIO_Close( si );

	if( memcmp( buffer, "\x02\x30\x31\x46\x46\x30\x30\x30\x31\x31\x32", 11 ) )
		return( SC_EXIT_OK );

	rdi->minor=SC_ACR20_SERIAL;
	/* Unless there is a other reader which sends the same message on
	 * power on it is a sure sign, that his reader is present.
	 */
	rdi->prob=250;

	return( SC_EXIT_OK );
}

/****************************************************************************
*																			*
*							Low Level Functions								*
*																			*
****************************************************************************/

/* Get Capabilities */

int scAcr20GetCap( SC_READER_INFO *ri, SC_READER_CAP *rp )
{
	rp->t0err=TRUE;		/* I think so. */
	rp->t1=TRUE;
	rp->freq=3579500;	/* And others. */
	rp->motor=FALSE;
	rp->slots=1;

	rp->n_fd=1;

	/* Not possible to tell, what the speed is, so just tell the default. */
	/* 9600 at 3.579MHz */
	rp->fd[0]=(((10L<<16)+372L)<<8)+1;
	rp->speed[0]=9600;

	return( SC_EXIT_OK );
}

/* Activate Card */

int scAcr20Activate( SC_READER_INFO *ri )
{
	if( ri->si==NULL ) return( SC_EXIT_BAD_PARAM );

	/* Does nothing, because a RESET command also powers up the card. */

	return( SC_EXIT_OK );
}

/* Deactivate Card */

int scAcr20Deactivate( SC_READER_INFO *ri )
{
	BYTE poweroff[] = { 0x81, 0x00 };
	BYTE rsp[ 3+1 ];
	int ret, rsplen = sizeof( rsp );

	if( ri->si==NULL ) return( SC_EXIT_BAD_PARAM );

	/* Send POWER_OFF command to reader. */
	ret = scAcr20SendCmd( ri, poweroff, rsp, &rsplen, 0 );

	if( (rsplen!=3) || (rsp[0]!=0x90) ) return( SC_EXIT_UNKNOWN_ERROR );

	return( ret );
}

/* Get card status */

int scAcr20CardStatus( SC_READER_INFO *ri )
{
	BYTE getacrstat[] = { 0x01, 0x00 };

	/* Length of response to GET_ACR_STAT is 19 bytes. Additional bytes
	 * is here, so that it is possible to check if response is to long.
	 */
	BYTE rsp[ 19+1 ];
	int ret, rsplen = sizeof( rsp );

	if( ri->si==NULL ) return( SC_EXIT_BAD_PARAM );

	ri->status=0x00;

	/* Send GET_ACR_STAT command to reader. */
	ret = scAcr20SendCmd( ri, getacrstat, rsp, &rsplen, 0 );
	if( ret!=SC_EXIT_OK ) return( ret );

	if( rsp[0]!=0x90 ) return( SC_EXIT_UNKNOWN_ERROR );

	/* Response should be 19 bytes long. */
	if( rsplen!=19 ) return( SC_EXIT_PROTOCOL_ERROR );

	if( rsp[18]!=0x00 ) ri->status=SC_CARD_STATUS_PRESENT;

	return( ret );
}

/* Send command. */
/* rlen specfies the maximum bytes to return and returns the bytes returned.
 * Supports only the short command format.
 * send contains the command without header and checksum.
 * rec returns the response without header and checksum.
 * Send data length is encoded in the send.
 * TODO: Long command format.
 */

int scAcr20SendCmd( SC_READER_INFO *ri, BYTE *send, BYTE *rec, int *rlen,
	LONG wait )
{
	BYTE sbin[ 1+1+3+512+1 ];
	BYTE shex[ 1+sizeof(sbin)*2+1 ];
	BYTE nak[] = { 0x02, 0x30, 0x35, 0x30, 0x35, 0x03 };
	int dsize, slen, len, recerr, senderr, success, i;

	if( ri->si==NULL ) return( SC_EXIT_BAD_PARAM );

	if( (send[1]==0xFF)&&(send[2]>0x01) ) return( SC_EXIT_BAD_PARAM );

	success=FALSE;
	senderr=FALSE;

	while( success!=TRUE ) {
		/* Get unwanted bytes */
#ifdef __palmos__
		SIO_Flush(ri->si);
#else
		while(SIO_ReadChar(ri->si)!=-1);
#endif

		slen=0;
		if( send[1]==0xFF ) dsize=(send[2]<<8)+send[3]+3;
		else dsize=send[1]+1;

		/* Header */
		sbin[0]=0x01;
		/* Message */
		memcpy( sbin+1, send, dsize+1 );
		/* Checksum */
		sbin[dsize+2]=0;
		for( i=0; i<(dsize+2); i++ ) sbin[dsize+2]^=sbin[i];

		/* STX */
		shex[slen++]=0x02;
		/* Convert to ASCII */
		slen += scGeneralBinHex( TRUE, sbin, 2+dsize+1, shex+1 );
		/* ETX */
		shex[slen++]=0x03;

#if 0
		/* Wait for CTS */
		i=0;
		SIO_ReadControlState( ri->si );
		while( SIO_GetControlState( ri->si, SIO_CONTROL_CTS )==0 ) {
			i++;
			/* Abort after 10s. */
			if( i>1000 ) return( SC_EXIT_IO_ERROR );
			SIO_Delay( ri->si, 10000 );
			SIO_ReadControlState( ri->si );
		}
#endif

		/* Send data. */
		if( SIO_WriteBuffer( ri->si, shex, slen )!=slen )
			return( SC_EXIT_IO_ERROR );

		/* Wait 100us. */
		/* Needed? */
		SIO_Delay( ri->si, 100 );

		/* Wait for data. */
		SIO_WaitForData( ri->si, wait );

		/* Receive data. */
		/* Better read until end of message character? */
		if( (len=SIO_ReadBuffer( ri->si, shex, sizeof(shex) ))==-1 )
			return ( SC_EXIT_IO_ERROR );

		recerr=FALSE;

		/* End of message character? */
		if( shex[ len-1 ]!=0x03 ) recerr=TRUE;

		/* Check if len is odd. */
		if( len & 1 ) recerr=TRUE;

		/* Convert to binary. */
		len = scGeneralBinHex( FALSE, shex+1, len-2, sbin );

		if( (len==2) && memcmp( sbin, (BYTE *)"\x05\x05", 2 ) ) {
			if( senderr ) break;
			senderr=TRUE;
			continue;
		}

		/* Check LRC. */
		for( i=0; i<len-1; i++ ) sbin[len-1]^=sbin[i];
		if( sbin[len-1] ) recerr=TRUE;

		/* Check for header. */
		if( sbin[0]!=0x01 ) recerr=TRUE;

		/* Check lenght. */
		if( sbin[3]==0xFF ) {
			if( ((sbin[4]<<8)+sbin[5])!=(len-7) ) recerr=TRUE;
		} else {
			if( sbin[3]!=(len-5) ) recerr=TRUE;
		}

		if( recerr ) {
			/* Send NAK. */
			if( SIO_WriteBuffer( ri->si, nak, sizeof(nak) )!=sizeof(nak) )
				return( SC_EXIT_IO_ERROR );

			/* Wait 100us. */
			/* Needed? */
			SIO_Delay( ri->si, 100 );

			/* Wait for data. */
			SIO_WaitForData( ri->si, wait );

			/* Receive data. */
			/* Better read until end of message character? */
			if( (len=SIO_ReadBuffer( ri->si, shex, sizeof(shex) ))==-1 )
				return ( SC_EXIT_IO_ERROR );

			/* End of message character? */
			if( shex[ len-1 ]!=0x03 ) return( SC_EXIT_PROTOCOL_ERROR );

			/* Check if len is odd. */
			if( len & 1 ) return( SC_EXIT_PROTOCOL_ERROR );

			/* Convert to binary. */
			len = scGeneralBinHex( FALSE, shex+1, len-2, sbin );

			if( (len==2) && memcmp( sbin, (BYTE *)"\x05\x05", 2 ) ) {
				if( senderr ) break;
				senderr=TRUE;
				continue;
			}

			/* Check LRC. */
			for( i=0; i<len-1; i++ ) sbin[len-1]^=sbin[i];
			if( sbin[len-1] ) return( SC_EXIT_BAD_CHECKSUM );

			/* Check for header. */
			if( sbin[0]!=0x01 ) return( SC_EXIT_IO_ERROR );

			/* Check lenght. */
			if( sbin[3]==0xFF ) {
				if( ((sbin[4]<<8)+sbin[5])!=(len-7) ) recerr=TRUE;
			} else {
				if( sbin[3]!=(len-5) ) recerr=TRUE;
			}
		}

		/* Check rlen. */
		if( (len-2)>(*rlen) ) len=(*rlen)+2;

		success=TRUE;
	}

	if( success!=TRUE ) return( SC_EXIT_IO_ERROR );

	/* Return data. */
	memcpy( rec, sbin+1, len-2 );
	*rlen=len-2;

	return( SC_EXIT_OK );
}

/****************************************************************************
*																			*
*							SmartCard Functions								*
*																			*
****************************************************************************/

/* Reset Card */

int scAcr20ResetCard( SC_READER_INFO *ri, SC_CARD_INFO *ci )
{
	BYTE reset[] = { 0x80, 0x00 };
	BYTE sett0[] = { 0x02, 0x01, 0x0C };
	BYTE sett1[] = { 0x02, 0x01, 0x0D };
	BYTE rsp[ 3+sizeof(ci->atr)+1 ];
	int ret, rsplen = sizeof( rsp );

	/* Clean keys. */
	scGeneralCleanCI( ci );

	ci->atrlen=0;

	if( ri->si==NULL ) return( SC_EXIT_BAD_PARAM );

	/* Send RESET command to reader. */
	ret = scAcr20SendCmd( ri, reset, rsp, &rsplen, 2000000 );
	if( ret!=SC_EXIT_OK ) return( ret );

	if( (rsplen<5) || (rsp[0]!=0x90) ) return( SC_EXIT_UNKNOWN_ERROR );

	/* Copy ATR. */
	memcpy( ci->atr, rsp+3, rsplen-3 );
	ci->atrlen=rsplen-3;

	/* Send SET_CARD_TYPE command to reader. */
	if( rsp[1]==0x01 ) {
		ret = scAcr20SendCmd( ri, sett1, rsp, &rsplen, 2000000 );
		if( ret!=SC_EXIT_OK ) return( ret );
	} else if( rsp[1]==0x00 ) {
		ret = scAcr20SendCmd( ri, sett0, rsp, &rsplen, 2000000 );
		if( ret!=SC_EXIT_OK ) return( ret );
	}

	return( SC_EXIT_OK );
}

/* Transmit APDU with protocol T=0 */
/* Command must be encoded that following conditions are fullfilled:
 *  - Lc and Le musst be pressent.
 *  - If no data is sent Lc must be 0x00.
 *  - If no data is expected Le must be 0x00.
 *  - If data is expected, Le mustn't be 0x00. IMO the correct way is
 *    to set it to MAX_R.
 *  - If protocol T=0 is used Le must be 0x00 if Lc is not 0x00.
 */

int scAcr20T0( SC_READER_INFO *ri, SC_CARD_INFO *ci, SC_APDU *apdu )
{
	BYTE cmd[ 5+SC_ACR20_MAX_BUFFER_SIZE+1 ];
	BYTE rsp[ SC_ACR20_MAX_BUFFER_SIZE+2 ];
	int ret, rsplen=sizeof(rsp);
#ifdef ACR20_DEBUG
	int i;
#endif

	if( ri->si==NULL ) return( SC_EXIT_BAD_PARAM );

	if( (ret=scReaderCheckAPDU( apdu, TRUE ))!=SC_EXIT_OK ) return( ret );

	/* Check if command is too long for ACR20. */
	if( (apdu->cmdlen>5) && (apdu->cmd[4]>(ri->maxc+5)) )
		return( SC_EXIT_CMD_TOO_LONG );

	/* Check if response might be too long for ACR20. */
	if( (apdu->cse==SC_APDU_CASE_2_SHORT) && (apdu->cmd[4]>ri->maxr) )
		return( SC_EXIT_RSP_TOO_LONG );
	if( (apdu->cse==SC_APDU_CASE_4_SHORT) &&
		(apdu->cmd[apdu->cmdlen-1]>ri->maxr) )
		return( SC_EXIT_RSP_TOO_LONG );

#ifdef ACR20_DEBUG
printf("[CASE:%d]",apdu->cse);
#endif
	cmd[0]=0xA0;
	if( apdu->cmdlen>0xFE ) {
		cmd[1]=0xFF;
		cmd[2]=(apdu->cmdlen>>8)&0xFF;
		cmd[3]=apdu->cmdlen&0xFF;
		memcpy( cmd+4, apdu->cmd, apdu->cmdlen );
#ifdef ACR20_DEBUG
printarray("[CMDb:",(cmd[2]<<8)+cmd[3]+4,cmd);printf("]");
#endif
		if( (apdu->cse==SC_APDU_CASE_4_SHORT) &&
			(apdu->cmdlen==(5+apdu->cmd[4])) ) {
			if( ci->protocol!=SC_PROTOCOL_T0 )
				cmd[4+apdu->cmdlen]=ri->maxr&0xFF;
			else
				cmd[4+apdu->cmdlen]=0x00;
			cmd[3]++;
			if( cmd[3]==0x00 ) cmd[2]++;
		}
#ifdef ACR20_DEBUG
printarray("[CMDa:",(cmd[2]<<8)+cmd[3]+4,cmd);printf("]");
#endif
	} else {
		cmd[1]=apdu->cmdlen;
		memcpy( cmd+2, apdu->cmd, apdu->cmdlen );
#ifdef ACR20_DEBUG
printarray("[CMDa:",cmd[1]+2,cmd);printf("]");
#endif
		switch( apdu->cse ) {
			case SC_APDU_CASE_1:
				/* Do it independently of the size, because it can't hurt. */
				cmd[1]=6;
				cmd[6]=0x00;
				cmd[7]=0x00;
				break;
			case SC_APDU_CASE_2_SHORT:
				if( apdu->cmdlen==5 ) cmd[7]=cmd[6];
				if( (cmd[7]==0x00) && (ci->protocol!=SC_PROTOCOL_T0) ) cmd[7]=ri->maxr&0xFF;
				cmd[6]=0x00;
				cmd[1]=6;
				break;
			case SC_APDU_CASE_3_SHORT:
				cmd[2+apdu->cmdlen]=0x00;
				cmd[1]++;
				if( cmd[1]==0x00 ) {
					/* Recode length encoding. */
					cmd[1]=0xFF;
					cmd[2]=((apdu->cmdlen+1)>>8)&0xFF;
					cmd[3]=(apdu->cmdlen+1)&0xFF;
					memcpy( cmd+4, apdu->cmd, apdu->cmdlen );
					cmd[4+apdu->cmdlen]=0x00;
				}
				break;
			case SC_APDU_CASE_4_SHORT:
				if( apdu->cmdlen==(5+apdu->cmd[4]) ) {
					if( ci->protocol!=SC_PROTOCOL_T0 )
						cmd[2+apdu->cmdlen]=ri->maxr&0xFF;
					else
						cmd[2+apdu->cmdlen]=0x00;
					cmd[1]++;
					if( cmd[1]==0x00 ) {
						/* Recode length encoding. */
						cmd[1]=0xFF;
						cmd[2]=((apdu->cmdlen+1)>>8)&0xFF;
						cmd[3]=(apdu->cmdlen+1)&0xFF;
						memcpy( cmd+4, apdu->cmd, apdu->cmdlen );
						cmd[4+apdu->cmdlen]=ri->maxr&0xFF;
					}
				} else if( cmd[2+apdu->cmdlen-1]==0x00 ) {
					cmd[2+apdu->cmdlen-1]=ri->maxr&0xFF;
				}
				break;
			default:
				break;
		}
#ifdef ACR20_DEBUG
printarray("[CMDb:",cmd[1]+2,cmd);printf("]");
#endif
	}

	/* Send ExchangeAPDU command to reader. */
	/* Set wait to 1 minute. */
	ret = scAcr20SendCmd( ri, cmd, rsp, &rsplen, 60000000UL );
	if( ret!=SC_EXIT_OK ) return( ret );

	if( (rsplen<3) || (rsp[0]!=0x90) ) return( SC_EXIT_UNKNOWN_ERROR );
	if( rsplen<5 ) return( SC_EXIT_BAD_SW );

	if( rsp[2]==0xFF ) {
		memcpy( apdu->rsp, rsp+5, rsplen-5 );
		apdu->rsplen=rsplen-5;
	} else {
		memcpy( apdu->rsp, rsp+3, rsplen-3 );
		apdu->rsplen=rsplen-3;
	}

	return( SC_EXIT_OK );
}

/* Transmit APDU with protocol T=1 */

int scAcr20T1( SC_READER_INFO *ri, SC_CARD_INFO *ci, SC_APDU *apdu )
{
	int ret;

#ifndef NO_APDU_CHECK
	if( (ret=scReaderCheckAPDU( apdu, FALSE ))!=SC_EXIT_OK ) return( ret );
#endif /* NO_APDU_CHECK */

	return( scAcr20T0( ri, ci, apdu ) );
}

/* Transmit APDU */

int scAcr20SendAPDU( SC_READER_INFO *ri, SC_CARD_INFO *ci,
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
		return( scAcr20T1( ri, ci, apdu ) );
	}

	return( SC_EXIT_NOT_IMPLEMENTED );
}



