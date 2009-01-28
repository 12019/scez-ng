/****************************************************************************
*																			*
*					SCEZ chipcard library - GCR 400 routines				*
*						Copyright Matthias Bruestle 1999					*
*					For licensing, see the file COPYRIGHT					*
*																			*
****************************************************************************/

/* $Id: scgcr400.c 1617 2005-11-03 17:41:39Z laforge $ */

#include <scez/scinternal.h>
#include <scez/sct0.h>
#include <scez/readers/scgcr400.h>

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/****************************************************************************
*																			*
*						 		Init/Shutdown Routines						*
*																			*
****************************************************************************/

/* Initializes reader and sets ri */

int scGcr400Init( SC_READER_INFO *ri, const char *param )
{
	BYTE setmode[] = { 0x01, 0x00 };
	BYTE defcard[] = { 0x02, 0x02 };
	BYTE confsio[] = { 0x0A, 0x00 };
	BYTE rsp[ 2 ], rsplen;
	int ret;
	char *port;

	if( ri->slot!=1 ) return( SC_EXIT_NO_SLOT );

	if( param==NULL ) return( SC_EXIT_BAD_PARAM );

	if( (strcmp(param,"0")==0) || (strcmp(param,"1")==0) ||
		(strcmp(param,"2")==0) || (strcmp(param,"3")==0) ) {
		port=scReaderExpandPort( param );
		if( port==NULL ) return( SC_EXIT_BAD_PARAM );
	} else port=param;

	if( ri->si!=NULL ) scGcr400Shutdown( ri );

	/* Open port */
	ri->si = SIO_Open( port );
	if( ri->si==NULL ) return( SC_EXIT_IO_ERROR );

#if 0
	SIO_SetLogFile( ri->si, "LogGcr400.txt" );
#endif

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

	/* Send Set Mode command to see if a GCR 400 is there. */
	rsplen=sizeof(rsp);
	ret = scGcr400SendCmd( ri, setmode, sizeof(setmode), rsp, &rsplen, 0 );
	if( ret!=SC_EXIT_OK ) {
		scGcr400Shutdown( ri );
		return( ret );
	}

	/* TODO: Check response. */

	/* Send Define Card Type command, although not really necessary. */
	rsplen=sizeof(rsp);
	ret = scGcr400SendCmd( ri, defcard, sizeof(defcard), rsp, &rsplen, 0 );
	if( ret!=SC_EXIT_OK ) {
		scGcr400Shutdown( ri );
		return( ret );
	}

	/* TODO: Check response. */

	/* Build Configure SIO Line command. */
	/* For all current ports, i.e. Unix, PalmOS, MS-Windows, set to 38400. */
	confsio[1]=0x02;

#if 0
	/* Send Configure SIO Line command. */
	rsplen=sizeof(rsp);
	ret = scGcr400SendCmd( ri, confsio, sizeof(confsio), rsp, &rsplen, 0 );
	if( ret!=SC_EXIT_OK ) {
		scGcr400Shutdown( ri );
		return( ret );
	}

	/* Set port parameters */
	/* For all current ports, i.e. Unix, PalmOS, MS-Windows, set to 38400. */
	SIO_SetSpeed( ri->si, 38400 );
	SIO_WriteSettings( ri->si );
#endif

	ri->major=SC_READER_GCR400;
	ri->minor=SC_GCR400_GCR400;
	ri->etu=104;
	ri->pinpad=FALSE;
	ri->display=FALSE;

	ri->scShutdown=scGcr400Shutdown;
	ri->scGetCap=scGcr400GetCap;
	ri->scActivate=scGcr400Activate;
	ri->scDeactivate=scGcr400Deactivate;
	ri->scWriteBuffer=NULL;
	ri->scReadBuffer=NULL;
	ri->scWriteChar=NULL;
	ri->scReadChar=NULL;
	ri->scSetSpeed=NULL;
	ri->scCardStatus=scGcr400CardStatus;
	ri->scResetCard=scGcr400ResetCard;
	ri->scPTS=NULL;
	ri->scT0=scGcr400T0;
	ri->scT1=scGcr400T1;
	ri->scSendAPDU=scGcr400SendAPDU;

	return( SC_EXIT_OK );
}

int scGcr400Shutdown( SC_READER_INFO *ri )
{
	if( ri->si==NULL ) return( SC_EXIT_OK );

	SIO_Close( ri->si );
	ri->si=NULL;

	return( SC_EXIT_OK );
}

/****************************************************************************
*																			*
*							Low Level Functions								*
*																			*
****************************************************************************/

/* Get Capabilities */

int scGcr400GetCap( SC_READER_INFO *ri, SC_READER_CAP *rp )
{
	rp->t0err=TRUE;		/* I think so. */
	rp->t1=TRUE;
	rp->freq=3579500;	/* I think so. */
	rp->motor=FALSE;
	rp->slots=1;

	rp->n_fd=1;

	/* 9600 at 3.579MHz */
	rp->fd[0]=(((10L<<16)+372L)<<8)+1;
	rp->speed[0]=9600;

	return( SC_EXIT_OK );
}

/* Activate Card */

int scGcr400Activate( SC_READER_INFO *ri )
{
	if( ri->si==NULL ) return( SC_EXIT_BAD_PARAM );

	/* Does nothing, because a Power Up command also resets the card. */

	return( SC_EXIT_OK );
}

/* Deactivate Card */

int scGcr400Deactivate( SC_READER_INFO *ri )
{
	BYTE cmd[] = { 0x4D, 0x00, 0x00, 0x00 };
	BYTE rsp[ 1 ];
	BYTE rsplen = sizeof( rsp );
	int ret;

	if( ri->si==NULL ) return( SC_EXIT_BAD_PARAM );

	/* Send Power Down command to reader. */
	rsplen=sizeof(rsp);
	ret = scGcr400SendCmd( ri, cmd, sizeof(cmd), rsp, &rsplen, 0 );

	return( ret );
}

/* Get card status */

int scGcr400CardStatus( SC_READER_INFO *ri )
{
	BYTE cmd[] = { 0x17 };
	BYTE rsp[ 7 ];
	BYTE rsplen = sizeof( rsp );
	int ret;

	if( ri->si==NULL ) return( SC_EXIT_BAD_PARAM );

	ri->status=0x00;

	/* Send Set Mode command to reader. */
	/* Does this really help to detect if a card is present? */
	rsplen=sizeof(rsp);
	ret = scGcr400SendCmd( ri, cmd, sizeof(cmd), rsp, &rsplen, 0 );
	if( ret!=SC_EXIT_OK ) return( ret );

	if( rsp[1] & 0x04 ) ri->status=SC_CARD_STATUS_PRESENT;

	return( ret );
}

/* Send command. */
/* rlen specfies the maximum bytes to return and returns the bytes returned. */

int scGcr400SendCmd( SC_READER_INFO *ri, BYTE *send, BYTE slen, BYTE *rec,
	BYTE *rlen, LONG wait )
{
	BYTE sbin[ 1+1+255+1 ];
	BYTE shex[ (1+1+255+1)*2+1 ];
	int len, i;

	if( ri->si==NULL ) return( SC_EXIT_BAD_PARAM );

	/* ACK */
	sbin[0]=0x60;
	/* Message lenght */
	sbin[1]=slen;
	/* Message */
	memcpy( sbin+2, send, slen );
	/* Checksum */
	sbin[slen+2]=0;
	for( i=0; i<slen+2; i++ ) sbin[slen+2]^=sbin[i];

	/* Convert to ASCII */
	len = scGeneralBinHex( TRUE, sbin, 2+slen+1, shex );

	/* Append 0x03 */
	shex[len++]=0x03;

	/* Get unwanted bytes */
#ifdef __palmos__
	SIO_Flush(ri->si);
#else
	while(SIO_ReadChar(ri->si)!=-1);
#endif

	/* Send data. */
	if( SIO_WriteBuffer( ri->si, shex, len )!=len ) return( SC_EXIT_IO_ERROR );

	/* Wait 100us. */
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
	if( !(len & 1) ) return( SC_EXIT_PROTOCOL_ERROR );

	/* Convert to binary. */
	len = scGeneralBinHex( FALSE, shex, len, sbin );

	/* Check LRC. */
	for( i=0; i<len-1; i++ ) sbin[len-1]^=sbin[i];
	if( sbin[len-1] ) return( SC_EXIT_BAD_CHECKSUM );

	/* Check for ACK. */
	if( sbin[0]!=0x60 ) return( SC_EXIT_IO_ERROR );

	/* Check lenght. */
	if( sbin[1]!=(len-3) ) return( SC_EXIT_PROTOCOL_ERROR );

	/* Check rlen. */
	if( (len-3)>(*rlen) ) len=(*rlen)+3;
	if( len<0 ) len=3;

	/* Return data. */
	memcpy( rec, sbin+2, len-3 );
	*rlen=len-3;

	return( SC_EXIT_OK );
}

/****************************************************************************
*																			*
*							SmartCard Functions								*
*																			*
****************************************************************************/

/* Reset Card */

int scGcr400ResetCard( SC_READER_INFO *ri, SC_CARD_INFO *ci )
{
	BYTE cmd[1];
	BYTE rsp[ 4+sizeof(ci->atr)+3 ];
	BYTE rsplen = sizeof( rsp );
	int ret;

	/* Clean keys. */
	scGeneralCleanCI( ci );

	ci->atrlen=0;

	if( ri->si==NULL ) return( SC_EXIT_BAD_PARAM );

	if( ri->slot==1 ) {
		cmd[0] = 0x12;
	} else {
		cmd[0] = 0x1A;
	}

	/* Send Power Up command to reader. */
	rsplen=sizeof(rsp);
	ret = scGcr400SendCmd( ri, cmd, sizeof(cmd), rsp, &rsplen, 2000000 );
	if( ret!=SC_EXIT_OK ) return( ret );

	/* Check status byte. */
	if( rsp[0]==0xFB ) return( SC_EXIT_NO_CARD );
	if( rsp[0]==0xF7 ) return( SC_EXIT_NO_CARD );
	if( rsp[0]!=0x00 ) return( SC_EXIT_UNKNOWN_ERROR );

	if( (rsplen-1) >=6 ) {
		ci->atr[ 0 ] = rsp[ 1 ];
		ci->atr[ 1 ] = rsp[ 2 ];
		ci->atrlen=2;
		if( ci->atr[1] & 0x10 ) ci->atr[ ci->atrlen++ ] = rsp[ 3 ];
		if( ci->atr[1] & 0x20 ) ci->atr[ ci->atrlen++ ] = rsp[ 4 ];
		if( ci->atr[1] & 0x40 ) ci->atr[ ci->atrlen++ ] = rsp[ 5 ];
		if( ci->atr[1] & 0x80 ) ci->atr[ ci->atrlen++ ] = rsp[ 6 ];
		memcpy( ci->atr+ci->atrlen, rsp+7, rsplen-7 );
		ci->atrlen+=rsplen-7;
	} else {
		memcpy( ci->atr, rsp+1, rsplen-1 );
		ci->atrlen=rsplen-1;
	}

	return( SC_EXIT_OK );
}

/* Transmit APDU with protocol T=0 */

int scGcr400T0( SC_READER_INFO *ri, SC_CARD_INFO *ci, SC_APDU *apdu )
{
	BYTE cmd[ SC_GCR400_MAX_BUFFER_SIZE ];
	BYTE rsp[ SC_GCR400_MAX_BUFFER_SIZE-1 ], rsplen;
	int ret, cmdlen;

	if( ri->si==NULL ) return( SC_EXIT_BAD_PARAM );

	/* Check if command is too long for GCR400. */
	if( apdu->cmdlen>253 ) return( SC_EXIT_CMD_TOO_LONG );

	/* Check if response might be too long for GCR400. */
	if( (apdu->cse==SC_APDU_CASE_2_SHORT) && (apdu->cmd[4]>252 ) )
		return( SC_EXIT_RSP_TOO_LONG );

	if( ri->slot==1 ) {
		if( apdu->cse==SC_APDU_CASE_2_SHORT ) cmd[0]=0x13;
		else cmd[0]=0x14;
	} else {
		if( apdu->cse==SC_APDU_CASE_2_SHORT ) cmd[0]=0x1B;
		else cmd[0]=0x1C;
	}

	cmdlen = apdu->cmdlen;
	memcpy( cmd+1, apdu->cmd, cmdlen );

	if( cmdlen==4 ) {
		cmdlen=5;
		cmd[5]=0;
	}

	/* Send ISO Input/Output command to reader. */
	/* Set wait to 1 minute. */
	rsplen=sizeof(rsp);
	ret = scGcr400SendCmd( ri, cmd, (BYTE)cmdlen+1, rsp, &rsplen,
		60000000UL );
	if( ret!=SC_EXIT_OK ) return( ret );

	if( (rsp[0]==0xFB) || (rsp[0]==0xF7) ) return( SC_EXIT_NO_CARD );
	if( (rsp[0]==0xA3) ) return( SC_EXIT_IO_ERROR );
	if( rsplen<3 ) return( SC_EXIT_UNKNOWN_ERROR );

	memcpy( apdu->rsp, rsp+1, rsplen-1 );
	apdu->rsplen=rsplen-1;

	return( SC_EXIT_OK );
}

/* Transmit APDU with protocol T=1 */

int scGcr400T1( SC_READER_INFO *ri, SC_CARD_INFO *ci, SC_APDU *apdu )
{
	BYTE cmd[ SC_GCR400_MAX_BUFFER_SIZE ];
	BYTE rsp[ SC_GCR400_MAX_BUFFER_SIZE-1 ], rsplen;
	int ret;

	if( ri->si==NULL ) return( SC_EXIT_BAD_PARAM );

	/* Check if command is too long for GCR400. */
	if( apdu->cmdlen>248 ) return( SC_EXIT_CMD_TOO_LONG );

	/* Check if response might be too long for GCR400. */
	/* TODO: What do with Le==0? */
#ifndef NO_APDU_CHECK
	if( (apdu->cse==SC_APDU_CASE_2_SHORT) && (apdu->cmd[4]>252) )
		return( SC_EXIT_RSP_TOO_LONG );
	if( (apdu->cse==SC_APDU_CASE_4_SHORT)&&(apdu->cmd[apdu->cmdlen-1]>252) )
		return( SC_EXIT_RSP_TOO_LONG );
#endif

	if( ri->slot==1 ) {
		cmd[0]=0x15;
	} else {
		cmd[0]=0x1D;
	}

	memcpy( cmd+1, apdu->cmd, apdu->cmdlen );

	/* Send ISO Exchange APDU command to reader. */
	/* Set wait to 1 minute. */
	rsplen=sizeof(rsp);
	ret = scGcr400SendCmd( ri, cmd, (BYTE)apdu->cmdlen+1, rsp, &rsplen,
		60000000UL );
	if( ret!=SC_EXIT_OK ) return( ret );

	if( (rsp[0]==0xFB) || (rsp[0]==0xF7) ) return( SC_EXIT_NO_CARD );
	if( (rsp[0]==0xA3) ) return( SC_EXIT_IO_ERROR );
	if( rsplen<3 ) return( SC_EXIT_UNKNOWN_ERROR );

	memcpy( apdu->rsp, rsp+1, rsplen-1 );
	apdu->rsplen=rsplen-1;

	return( SC_EXIT_OK );
}

/* Transmit APDU */

int scGcr400SendAPDU( SC_READER_INFO *ri, SC_CARD_INFO *ci,
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
		return( scGcr400T1( ri, ci, apdu ) );
	}

	return( SC_EXIT_NOT_IMPLEMENTED );
}



