/****************************************************************************
*																			*
*				SCEZ chipcard library - Reflex 60 routines					*
*					Copyright Matthias Bruestle 1999,2000					*
*					For licensing, see the file COPYRIGHT					*
*																			*
****************************************************************************/

/* $Id: screflex60.c 1617 2005-11-03 17:41:39Z laforge $ */

#include <scez/scinternal.h>
#include <scez/readers/screflex60.h>
#include <scez/sct0.h>
#include <scez/sct1.h>

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/* #define REFLEX60_DEBUG */
/* #define READER_DEBUG */

/****************************************************************************
*																			*
*						 		Init/Shutdown Routines						*
*																			*
****************************************************************************/

/* Initializes reader and sets ri */

int scReflex60Init( SC_READER_INFO *ri, const char *param )
{
	char *port;
	BYTE rsp[1+2+256+2];
	int rsplen, ret;

	if( ri->slot!=1 ) return( SC_EXIT_NO_SLOT );

	if( param==NULL ) return( SC_EXIT_BAD_PARAM );

	if( (strcmp(param,"0")==0) || (strcmp(param,"1")==0) ||
		(strcmp(param,"2")==0) || (strcmp(param,"3")==0) ) {
		port=scReaderExpandPort( param );
		if( port==NULL ) return( SC_EXIT_BAD_PARAM );
	} else port=param;

	if( ri->si!=NULL ) scReflex60Shutdown( ri );

	/* Open port */
	ri->si = SIO_Open( port );
	if( ri->si==NULL ) return( SC_EXIT_IO_ERROR );

/* SIO_SetLogFile( ri->si, "LogReflex60.txt" ); */

	ri->major=SC_READER_REFLEX60;
	ri->minor=SC_REFLEX60_REFLEX60;
	ri->etu=104;
	ri->pinpad=FALSE;
	ri->display=FALSE;

	ri->scShutdown=scReflex60Shutdown;
	ri->scGetCap=scReflex60GetCap;
	ri->scActivate=scReflex60Activate;
	ri->scDeactivate=scReflex60Deactivate;
	ri->scWriteBuffer=scReflex60WriteBuffer;
	ri->scReadBuffer=scReflex60ReadBuffer;
	ri->scWriteChar=NULL;
	ri->scReadChar=scReflex60ReadChar;
	ri->scWaitForData=scReflex60WaitForData;
	ri->scSetSpeed=scReflex60SetSpeed;
	ri->scCardStatus=scReflex60CardStatus;
	ri->scResetCard=scReflex60ResetCard;
	ri->scPTS=NULL;
	ri->scT0=scReflex60T0;
	ri->scT1=scReflex60T1;
	ri->scSendAPDU=scReflex60SendAPDU;
	ri->scVerifyPIN=scReflex60VerifyPIN;
	ri->scWaitReq=NULL;

	/* Set main port parameters */
	SIO_SetSpeed( ri->si, 9600 );
	SIO_SetDataBits( ri->si, 8 );
	SIO_SetParity( ri->si, SIO_PARITY_EVEN );
	SIO_SetStopBits( ri->si, 1 );
	SIO_SetIOMode( ri->si, SIO_IOMODE_DIRECT );
	SIO_WriteSettings( ri->si );

	/* Probably not needed. */
	SIO_SetReadTimeout( ri->si, 20000 ); 

	/* GetRdrType */
	if( (ret=scReflex60SendCmd( ri, (BYTE *)"\x60", 1, rsp, &rsplen ))!=SC_EXIT_OK )
		return( ret );
	if( rsp[0]!=0x60 ) return(SC_EXIT_PROTOCOL_ERROR);

#ifdef REFLEX60_DEBUG
	do{
		int i;
		printf("[Type:");
		for(i=0;i<rsplen;i++) printf(" %.2X",rsp[i]);
		printf("]");
	} while(FALSE);
#endif /* REFLEX60_DEBUG */

	scReaderDelay(1500000);

	/* GetCap */
	if( (ret=scReflex60SendCmd( ri, "\x69", 1, rsp, &rsplen ))!=SC_EXIT_OK )
		return( ret );
	if( rsp[0]!=0x68 ) return(SC_EXIT_PROTOCOL_ERROR);

#ifdef REFLEX60_DEBUG
	do{
		int i;
		printf("[Cap:");
		for(i=0;i<rsplen;i++) printf(" %.2X",rsp[i]);
		printf("]");
	} while(FALSE);
#endif /* REFLEX60_DEBUG */

	if( (rsplen==7) && (rsp[1]==0x00) && (rsp[2]==0x04) ) {
		ri->minor=SC_REFLEX60_REFLEX64;
		ri->pinpad=TRUE;
	} else
		ri->minor=SC_REFLEX60_REFLEX62;

	if( (ret=scReflex60SendCmd( ri, "\x63", 1, rsp, &rsplen ))!=SC_EXIT_OK )
		return( ret );
	if( rsp[0]!=SC_REFLEX60_ACK ) return(SC_EXIT_PROTOCOL_ERROR);

#ifdef REFLEX60_DEBUG
	printf("[CardPowerOff]");
#endif /* REFLEX60_DEBUG */

	return( SC_EXIT_OK );
}

int scReflex60Shutdown( SC_READER_INFO *ri )
{
	BYTE rsp[1+2+256+2];
	int rsplen, ret;

	if( ri->si==NULL ) return( SC_EXIT_OK );

	/* CardPowerOff */
	if( (ret=scReflex60SendCmd( ri, "\x63", 1, rsp, &rsplen ))!=SC_EXIT_OK )
		return( ret );
	if( rsp[0]!=SC_REFLEX60_ACK ) return(SC_EXIT_PROTOCOL_ERROR);

#ifdef REFLEX60_DEBUG
	printf("[CardPowerOff]");
#endif /* REFLEX60_DEBUG */

	/* DeActivateRdr */
	if( (ret=scReflex60SendCmd( ri, "\x6A", 1, rsp, &rsplen ))!=SC_EXIT_OK )
		return( ret );
	if( rsp[0]!=SC_REFLEX60_ACK ) return(SC_EXIT_PROTOCOL_ERROR);

#ifdef REFLEX60_DEBUG
	printf("[DeActivateRdr]");
#endif /* REFLEX60_DEBUG */

	SIO_Close( ri->si );
	ri->si=NULL;

	return( SC_EXIT_OK );
}

int scReflex60Detect( SC_READER_DETECT_INFO *rdi )
{
	SC_READER_INFO ri;
	char *port;
	int ret;

	rdi->prob=0;
	strncpy( rdi->name, "REFLEX60", SC_READER_NAME_MAXLEN );
	rdi->major=SC_READER_REFLEX60;
	rdi->minor=0;
	rdi->slots=1;
	rdi->pinpad=FALSE;	/* In principle TRUE, but it is not supported. */
	rdi->display=FALSE;

	if( strlen(rdi->param)>=SC_READER_PARAM_MAXLEN )
		return( SC_EXIT_BAD_PARAM );

	if( (strcmp(rdi->param,"0")==0) || (strcmp(rdi->param,"1")==0) ||
		(strcmp(rdi->param,"2")==0) || (strcmp(rdi->param,"3")==0) ) {
		port=scReaderExpandPort( rdi->param );
		if( port==NULL ) return( SC_EXIT_BAD_PARAM );
	} else port=rdi->param;

	ri.si=NULL;
	ri.slot=1;

	ret=scReflex60Init( &ri, rdi->param );
	if( ret==SC_EXIT_OK ) {
		rdi->prob=200;
		rdi->minor=ri.minor;
	}

	scReflex60Shutdown( &ri );

	return( SC_EXIT_OK );
}

/****************************************************************************
*																			*
*							Low Level Functions								*
*																			*
****************************************************************************/

/* Get Capabilities */

int scReflex60GetCap( SC_READER_INFO *ri, SC_READER_CAP *rp )
{
	rp->t0err=FALSE; /* Documentation doesn't tell it. */
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

int scReflex60Activate( SC_READER_INFO *ri )
{
	BYTE rsp[1+2+256+2];
	int rsplen, ret;

	if( ri->si==NULL ) return( SC_EXIT_BAD_PARAM );

	/* CardPowerOn */
	if( (ret=scReflex60SendCmd( ri, "\x62", 1, rsp, &rsplen ))!=SC_EXIT_OK )
		return( ret );
	if( rsp[0]!=0x64 ) return(SC_EXIT_PROTOCOL_ERROR);

#ifdef REFLEX60_DEBUG
	printf("[CardPowerOn]");
#endif /* REFLEX60_DEBUG */

	return( SC_EXIT_OK );
}

/* Deactivate Card */

int scReflex60Deactivate( SC_READER_INFO *ri )
{
	BYTE rsp[1+2+256+2];
	int rsplen, ret;

	if( ri->si==NULL ) return( SC_EXIT_BAD_PARAM );

	/* CardPowerOff */
	if( (ret=scReflex60SendCmd( ri, "\x63", 1, rsp, &rsplen ))!=SC_EXIT_OK )
		return( ret );
	if( rsp[0]!=SC_REFLEX60_ACK ) return(SC_EXIT_PROTOCOL_ERROR);

#ifdef REFLEX60_DEBUG
	printf("[CardPowerOff]");
#endif /* REFLEX60_DEBUG */

	return( SC_EXIT_OK );
}

/* The following Read/Write commands are probably only usable for sct1. */

/* Write Buffer Async */

int scReflex60WriteBuffer( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	const BYTE *buffer, int len )
{
	BYTE cmd[3+3+5+255+1+2];
	int ch, cmdlen;

	if(len>(3+5+255+2+3)) return(-1);
	if(buffer==NULL) return(-1);

	cmd[0]=0x67;
	cmd[1]=(len>>8)&0xFF;
	cmd[2]=len&0xFF;
	cmdlen=3+len;
	memcpy( cmd+3, buffer, len );

	if( SIO_WriteBuffer( ri->si, cmd, cmdlen )!=cmdlen ) {
		memset( cmd, 0, sizeof(cmd) );
		return(-1);
	}

	SIO_SetReadTimeout( ri->si, 250000 ); 
	if((ch=SIO_ReadChar( ri->si ))==-1) {
		memset( cmd, 0, sizeof(cmd) );
		SIO_SetReadTimeout( ri->si, 20000 ); 
		return(-1);
	}
	SIO_SetReadTimeout( ri->si, 20000 ); 
	if( ch!=SC_REFLEX60_ACK ) {
		memset( cmd, 0, sizeof(cmd) );
		return(-1);
	}

	memset( cmd, 0, sizeof(cmd) );

	return( len );
}

/* Read Buffer Async */

int scReflex60ReadBuffer( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	BYTE *buffer, int len , LONG timeout )
{
	BYTE rsp[3+3+256+2+2];
	int ch, i;

	if(len>(3+5+255+2+3)) return(-1);
	if(buffer==NULL) return(-1);

	SIO_WaitForData( ri->si, 5*60*1000000L ); /* Wait max. 5 Minutes. */

	if( (ch=SIO_ReadChar( ri->si ))==-1 ) {
		memset( rsp, 0, sizeof(rsp) );
		return(-1);
	}
	rsp[0]=(BYTE) ch;

	switch( ch ) {
	case 0x64:
		if( (i=SIO_ReadBuffer(ri->si, rsp+1, 2))!=2 )
			return(-1);

		if( (((int)rsp[1]<<8)+rsp[2])>258 )
			return(-1);

		if( (i=SIO_ReadBuffer(ri->si, rsp+3, ((int)rsp[1]<<8)+rsp[2]))
			!=(((int)rsp[1]<<8)+rsp[2]) ) {
			memset( rsp, 0, sizeof(rsp) );
			return(-1);
		}

		if( (i==-1) || (i>len) ) {
			memset( rsp, 0, sizeof(rsp) );
			return(-1);
		}

		memcpy( buffer, rsp+3, i );

		memset( rsp, 0, sizeof(rsp) );

		return(i);
	default:
		return(-1);
	}

}

/* Read Char Async */

int scReflex60ReadChar( SC_READER_INFO *ri, SC_CARD_INFO *ci, LONG timeout )
{
	return(-1);
}

/* Wait For Data */

int scReflex60WaitForData( SC_READER_INFO *ri, SC_CARD_INFO *ci, LONG timeout )
{
	return( SIO_WaitForData( ri->si, timeout ) );
}

/* Set Speed */

int scReflex60SetSpeed( SC_READER_INFO *ri, LONG speed )
{
	if( ri->si==NULL ) return( SC_EXIT_BAD_PARAM );

	switch( speed ) {
	case 9600:
		return( SC_EXIT_OK );
	default:
		return( SC_EXIT_BAD_PARAM );
	}
}

/* Get card status */

int scReflex60CardStatus( SC_READER_INFO *ri )
{
	BYTE rsp[1+2+256+2];
	int rsplen, ret;

	ri->status=0x00;

	if( ri->si==NULL ) return( SC_EXIT_BAD_PARAM );

	/* GetRdrStatus */
	if( (ret=scReflex60SendCmd( ri, "\x65", 1, rsp, &rsplen ))!=SC_EXIT_OK )
		return( ret );
	if( (rsplen!=5) || (rsp[0]!=0x61) ) return(SC_EXIT_PROTOCOL_ERROR);

#ifdef REFLEX60_DEBUG
	do{
		int i;
		printf("[Status:");
		for(i=0;i<rsplen;i++) printf(" %.2X",rsp[i]);
		printf("]");
	} while(FALSE);
#endif /* REFLEX60_DEBUG */

	if( rsp[1]&0x08 ) ri->status=SC_CARD_STATUS_PRESENT;

	return( SC_EXIT_OK );
}

int scReflex60SendCmd( SC_READER_INFO *ri, BYTE *cmd, int cmdlen, BYTE *rsp,
	int *rsplen )
{
	int ch=0, i;

	if( cmdlen<1 ) return( SC_EXIT_BAD_PARAM );
	if( ri->si==NULL ) return( SC_EXIT_BAD_PARAM );

	*rsplen=0;

	/* Get unwanted bytes */
#ifdef __palmos__
	SIO_Flush(ri->si);
#else
	while(SIO_ReadChar(ri->si)!=-1);
#endif

#ifdef REFLEX60_DEBUG
	printf("[Cmd:%.2X]",cmd[0]);
#endif /* REFLEX60_DEBUG */

	switch( cmd[0] ) {
	/* PC_to_RDR_GetRdrType */
	case 0x60:
		SIO_WriteChar( ri->si, cmd[0] );

		SIO_WaitForData( ri->si, 2000000 );

		if( (i=SIO_ReadBuffer( ri->si, rsp, 13 ))!=13 )
			return(SC_EXIT_PROTOCOL_ERROR);
		if( rsp[0]!=0x60 )
			return(SC_EXIT_PROTOCOL_ERROR);

		*rsplen=i;

		return(SC_EXIT_OK);
	/* PC_to_RDR_SetMode */
	case 0x61:
		for( i=0; i<cmdlen; i++ ) {
			SIO_WriteChar( ri->si, cmd[i] );

			if((ch=SIO_ReadChar( ri->si ))==-1) return(SC_EXIT_IO_ERROR);
			if( ch!=SC_REFLEX60_ACK ) return(SC_EXIT_PROTOCOL_ERROR);
		}

		rsp[0]=(BYTE) ch;
		*rsplen=1;

		return(SC_EXIT_OK);
	/* PC_to_RDR_CardPowerOff */
	case 0x63:
	/* PC_to_RDR_DeActivateRdr */
	case 0x6A:
		SIO_WriteChar( ri->si, cmd[0] );

		if((ch=SIO_ReadChar( ri->si ))==-1) return(SC_EXIT_IO_ERROR);
		if( ch!=SC_REFLEX60_ACK ) return(SC_EXIT_PROTOCOL_ERROR);

		rsp[0]=(BYTE) ch;
		*rsplen=1;

		return(SC_EXIT_OK);
	/* PC_to_RDR_CardPowerOn */
	case 0x62:
	/* PC_to_RDR_Reset */
	case 0x64:
		SIO_WriteChar( ri->si, cmd[0] );

		if((ch=SIO_ReadChar( ri->si ))==-1) return(SC_EXIT_IO_ERROR);
		if( ch!=SC_REFLEX60_ACK ) return(SC_EXIT_PROTOCOL_ERROR);

		SIO_WaitForData( ri->si, 1500000L );

		if( (ch=SIO_ReadChar( ri->si ))==-1 ) return(SC_EXIT_TIMEOUT);
		rsp[0]=(BYTE) ch;

		switch( ch ) {
		case 0x64:
			if( (i=SIO_ReadBuffer(ri->si, rsp+1, 2))!=2 )
				return(SC_EXIT_PROTOCOL_ERROR);

			if( (((int)rsp[1]<<8)+rsp[2])>258 )
				return(SC_EXIT_PROTOCOL_ERROR);

			if( (i=SIO_ReadBuffer(ri->si, rsp+3, ((int)rsp[1]<<8)+rsp[2]))
				!=(((int)rsp[1]<<8)+rsp[2]) )
				return(SC_EXIT_PROTOCOL_ERROR);

			*rsplen=3+i;

			return(SC_EXIT_OK);
		case 0x65:
		case 0x66:
			return(SC_EXIT_NO_CARD);
		case 0x67:
			return(SC_EXIT_IO_ERROR);
		default:
			return(SC_EXIT_PROTOCOL_ERROR);
		}
	/* PC_to_RDR_GetRdrStatus */
	case 0x65:
		SIO_WriteChar( ri->si, cmd[0] );

		if( SIO_ReadBuffer( ri->si, rsp, 5 )!=5 )
			return(SC_EXIT_IO_ERROR);
		if( rsp[0]!=0x61 )
			return(SC_EXIT_PROTOCOL_ERROR);

		*rsplen=5;

		return(SC_EXIT_OK);
	/* PC_to_RDR_SendRdrBlock */
	case 0x67:
		if( cmdlen<7 ) return(SC_EXIT_BAD_PARAM);
		if( cmdlen>(1+2+261) ) return(SC_EXIT_BAD_PARAM);

		if( SIO_WriteBuffer( ri->si, cmd, cmdlen )!=cmdlen )
			return(SC_EXIT_IO_ERROR);

		SIO_WaitForData( ri->si, 250000L );	/* Wait for ACK */

		if((ch=SIO_ReadChar( ri->si ))==-1) return(SC_EXIT_IO_ERROR);
		if( ch!=SC_REFLEX60_ACK ) return(SC_EXIT_PROTOCOL_ERROR);

		SIO_WaitForData( ri->si, 5*60*1000000L ); /* Wait max. 5 Minutes. */

		if( (ch=SIO_ReadChar( ri->si ))==-1 ) return(SC_EXIT_TIMEOUT);
		rsp[0]=(BYTE) ch;

		switch( ch ) {
		case 0x64:
			if( (i=SIO_ReadBuffer(ri->si, rsp+1, 2))!=2 )
				return(SC_EXIT_PROTOCOL_ERROR);

			if( (((int)rsp[1]<<8)+rsp[2])>258 )
				return(SC_EXIT_PROTOCOL_ERROR);

			if( (i=SIO_ReadBuffer(ri->si, rsp+3, ((int)rsp[1]<<8)+rsp[2]))
				!=(((int)rsp[1]<<8)+rsp[2]) )
				return(SC_EXIT_PROTOCOL_ERROR);

			*rsplen=3+i;

			return(SC_EXIT_OK);
		case 0x65:
		case 0x66:
			return(SC_EXIT_NO_CARD);
		case 0x67:
			return(SC_EXIT_IO_ERROR);
		default:
			return(SC_EXIT_PROTOCOL_ERROR);
		}
	/* PC_to_RDR_ResendBlock */
	case 0x68:
		return(SC_EXIT_NOT_IMPLEMENTED);
	/* PC_to_RDR_GetRdrCap */
	case 0x69:
		SIO_WriteChar( ri->si, cmd[0] );

		if( (i=SIO_ReadBuffer(ri->si, rsp, 3))!=3 )
			return(SC_EXIT_PROTOCOL_ERROR);
		if( rsp[0]!=0x68 )
			return(SC_EXIT_PROTOCOL_ERROR);
		if( (((int)rsp[1]<<8)+rsp[2])>258 )
			return(SC_EXIT_PROTOCOL_ERROR);

		if( (i=SIO_ReadBuffer(ri->si, rsp+3, ((int)rsp[1]<<8)+rsp[2]))
			!=(((int)rsp[1]<<8)+rsp[2]) )
			return(SC_EXIT_PROTOCOL_ERROR);

		*rsplen=3+i;

		return(SC_EXIT_OK);
	/* PC_to_RDR_Escape */
	case 0x6B:
		for( i=0; i<cmdlen; i++ ) {
			SIO_WriteChar( ri->si, cmd[i] );

			if((ch=SIO_ReadChar( ri->si ))==-1) return(SC_EXIT_IO_ERROR);
			if( ch!=SC_REFLEX60_ACK ) return(SC_EXIT_PROTOCOL_ERROR);
		}

		SIO_WaitForData( ri->si, 5*60*1000000L ); /* Wait max. 5 Minutes. */

		if( (ch=SIO_ReadChar( ri->si ))==-1 ) return(SC_EXIT_TIMEOUT);
		rsp[0]=(BYTE) ch;

		switch( ch ) {
		case 0x69:
			if( (i=SIO_ReadBuffer(ri->si, rsp+1, 2))!=2 )
				return(SC_EXIT_PROTOCOL_ERROR);

			if( (((int)rsp[1]<<8)+rsp[2])>258 )
				return(SC_EXIT_PROTOCOL_ERROR);

			if( (i=SIO_ReadBuffer(ri->si, rsp+3, ((int)rsp[1]<<8)+rsp[2]))
				!=(((int)rsp[1]<<8)+rsp[2]) )
				return(SC_EXIT_PROTOCOL_ERROR);

			*rsplen=3+i;

			return(SC_EXIT_OK);
		case 0x65:
		case 0x66:
			return(SC_EXIT_NO_CARD);
		case 0x67:
			return(SC_EXIT_IO_ERROR);
		default:
			return(SC_EXIT_PROTOCOL_ERROR);
		}
	default:
		return(SC_EXIT_BAD_PARAM);
	}
}

/****************************************************************************
*																			*
*							SmartCard Functions								*
*																			*
****************************************************************************/

/* Reset Card */

int scReflex60ResetCard( SC_READER_INFO *ri, SC_CARD_INFO *ci )
{
	BYTE rsp[1+2+256+2], mode[]={0x61,0x00,0x00,0x00,0x00,0x00};
	int rsplen, ret;
	int p;
	int	tdp;
	BYTE tt=0x0A;
	BYTE uu=0x4D;

	scGeneralCleanCI( ci );

	if( ri->si==NULL ) return( SC_EXIT_BAD_PARAM );

	/* Reset */
	if( (ret=scReflex60SendCmd( ri, "\x64", 1, rsp, &rsplen ))!=SC_EXIT_OK )
		return( ret );

	if( ((((int)rsp[1]<<8)+rsp[2])<2) || ((((int)rsp[1]<<8)+rsp[2])>32) )
		return( SC_EXIT_BAD_ATR );

	ci->atrlen=((int)rsp[1]<<8)+rsp[2];

	memcpy( ci->atr, rsp+3, ci->atrlen );

	tdp=1;
	p=2;

	/* TA1 */
	if( ci->atr[tdp]&0x10 ) p++;
	/* TB1 */
	if( ci->atr[tdp]&0x20 ) p++;
	/* TC1 */
	if( p >= ci->atrlen ) return( SC_EXIT_BAD_ATR );
	if( ci->atr[tdp]&0x40 ) {
		mode[2]=ci->atr[p];
		p++;
	}
	if( p >= ci->atrlen ) return( SC_EXIT_BAD_ATR );
	/* TD1 */
	if( ci->atr[tdp]&0x80 ) {
		if( (ci->atr[p]&0x0F)==0x01 ) mode[1]=0x10;

		tdp=p;
		p++;

		/* TA2 */
		if( ci->atr[tdp]&0x10 ) p++;
		/* TB2 */
		if( ci->atr[tdp]&0x20 ) p++;
		if( p >= ci->atrlen ) return( SC_EXIT_BAD_ATR );
		/* TC2 */
		if( ci->atr[tdp]&0x40 ) {
			tt=ci->atr[p];
			p++;
		}
		if( p >= ci->atrlen ) return( SC_EXIT_BAD_ATR );
	}
	/* TD2 */
	if( ci->atr[tdp]&0x80 ) {

		tdp=p;
		p++;

		if( p >= ci->atrlen ) return( SC_EXIT_BAD_ATR );
		/* TA3 */
		if( ci->atr[tdp]&0x10 ) p++;
		if( p >= ci->atrlen ) return( SC_EXIT_BAD_ATR );
		/* TB3 */
		if( ci->atr[tdp]&0x20 ) {
			uu=ci->atr[p];
			p++;
		}
		if( p >= ci->atrlen ) return( SC_EXIT_BAD_ATR );
		/* TC3 */
		if( ci->atr[tdp]&0x40 ) {
			if( (ci->atr[p]&0x01) && (mode[1]==0x10) ) mode[1]=0x11;
			p++;
		}
	}

	if( mode[1]==0x00 ) {
		mode[3]=tt;
	} else {
		mode[4]=uu;
	}

	/* SetMode */
	if( (ret=scReflex60SendCmd( ri, mode, 6, rsp, &rsplen ))!=SC_EXIT_OK )
		return( ret );

	return( SC_EXIT_OK );
}

/* Transmit APDU with protocol T=0 */

/* Supports only cases 1, 2 Short, 3 Short, 4 Short.
 * Case 4 Short:
 *  - You have to get the response data yourself, e.g. with GET RESPONSE
 *  - The le-byte is omitted.
 */

int scReflex60T0( SC_READER_INFO *ri, SC_CARD_INFO *ci, SC_APDU *apdu )
{
	BYTE cmd[1+2+5+255+1], rsp[1+2+256+2];
	int rsplen, ret;
	
	if( (ret=scReaderCheckAPDU( apdu, TRUE ))!=SC_EXIT_OK ) return( ret );

	if( ri->si==NULL ) return( SC_EXIT_BAD_PARAM );

	cmd[0]=0x67;
	cmd[1]=apdu->cmdlen>>8;
	cmd[2]=apdu->cmdlen&0xFF;
	memcpy( cmd+3, apdu->cmd, apdu->cmdlen );

	/* SendRdrBlock */
	if( (ret=scReflex60SendCmd( ri, cmd, apdu->cmdlen+3, rsp, &rsplen ))
		!=SC_EXIT_OK )
		return( ret );

	apdu->rsplen=((int)rsp[1]<<8)+rsp[2];

	if( apdu->rsplen>258 ) {
		apdu->rsplen=0;
		return( SC_EXIT_PROTOCOL_ERROR );
	}

	memcpy( apdu->rsp, rsp+3, apdu->rsplen );

	return( SC_EXIT_OK );
}

/* Transmit APDU with protocol T=1 */

int scReflex60T1( SC_READER_INFO *ri, SC_CARD_INFO *ci, SC_APDU *apdu )
{
	if( ri->si==NULL ) return( SC_EXIT_BAD_PARAM );

	return( scT1SendCmd( ri, ci, apdu ) );
}

/* Transmit APDU */

int scReflex60SendAPDU( SC_READER_INFO *ri, SC_CARD_INFO *ci,
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
		return( scReflex60T1( ri, ci, apdu ) );
	}

	return( SC_EXIT_NOT_IMPLEMENTED );
}

/* Transmit command using PIN pad */

int scReflex60VerifyPIN( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	SC_APDU *apdu, const char *message, int pinlen, int pincoding, int pinpos )
{
	return( SC_EXIT_NOT_IMPLEMENTED );
}


