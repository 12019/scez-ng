/****************************************************************************
*																			*
*					SCEZ chipcard library - GPR 400 routines				*
*						Copyright Matthias Bruestle 2001					*
*					For licensing, see the file COPYRIGHT					*
*																			*
****************************************************************************/

/* $Id: scgpr400.c 1617 2005-11-03 17:41:39Z laforge $ */

/* Installation:
 *
 * 1) Fetch card-0.9.6.tar.gz from:
 * 2) Copy (or patch) it to pcmcia-cs-3.1.x/clients.
 * 3) Make and install it.
 * 4) Make device:
 *    mknod /dev/gpr400 c 123 0
 *    chmod 2666 /dev/gpr400
 *    (The 2 is important for mandatory locking.)
 * 5) Copy smartcard and smartcard.opts to /etc/pcmcia/.
 *    (smartcard must have execute permission.)
 * 6) Add the content of gpr400_cs.conf to /etc/pcmcia/config.
 *    The version string in this file may be not the same than your reader
 *    has. In this case you probably have to change "V1.0" to "*" or use
 *    "manfid 0x0157, 0x3004" instead of the version line.
 */

#include <scez/scinternal.h>
#include <scez/sct0.h>
#include <scez/sct1.h>
#include <scez/readers/scgpr400.h>

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#if defined(__linux__) || defined(__FreeBSD__)
#include <fcntl.h>
#include <unistd.h>
#elif defined(USE_FLOCK)
#include <sys/file.h>
#endif /* USE_FLOCK */
#include <sys/ioctl.h>

int scGpr400TranslStatus( BYTE status )
{
	switch( status ) {
	case 0x00:
	case 0xE5:
	case 0xE7:
		return SC_EXIT_OK;
	case 0x10:
	case 0x1D:
	case 0xA0:
		return SC_EXIT_BAD_ATR;
	case 0xA2:
		return SC_EXIT_TIMEOUT;
	case 0xA3:
		return SC_EXIT_IO_ERROR;
	case 0xE4:
		return SC_EXIT_PROTOCOL_ERROR;
	case 0xF7:
	case 0xFB:
		return SC_EXIT_NO_CARD;
	case 0x15:
	default:
		return SC_EXIT_UNKNOWN_ERROR;
	}
}

/****************************************************************************
*																			*
*						 		Init/Shutdown Routines						*
*																			*
****************************************************************************/

/* Initializes reader and sets ri */

int scGpr400Init( SC_READER_INFO *ri, const char *param )
{
#if defined(__linux__)
	struct flock lock;
#endif
	SC_GPR400_IOCTL_PARAM_TLV selectCard = { SC_GPR400_TAG_SELECT_CARD,
		1, { 0x02 } };
	char *port;

	if( ri->slot!=1 ) return( SC_EXIT_NO_SLOT );

	if( param==NULL ) return( SC_EXIT_BAD_PARAM );

	if( strcmp(param,"0")==0 ) {
		/* Default devicename */
		port="/dev/gpr400";
	} else port=param;

	if( ri->fd!=-1 ) scGpr400Shutdown( ri );

	/* Open port */
    ri->fd=open( port, O_RDWR|O_NONBLOCK );
	if( ri->fd==-1 ) return( SC_EXIT_IO_ERROR );

#if defined(__linux__)
    /* Lock device */
    lock.l_type=F_WRLCK;
    lock.l_start=0;
    lock.l_whence=SEEK_SET;
    lock.l_len=0;
    lock.l_pid=0;
    if( fcntl( ri->fd, F_SETLK, &lock ) ) {
        close( ri->fd );
		ri->fd=-1;
        return( SC_EXIT_IO_ERROR );
    }
#elif defined(USE_FLOCK) /* Which OSes? */
	if( flock( ri->fd, LOCK_EX|LOCK_NB ) ) {
        close( ri->fd );
		fd=-1;
        return( SC_EXIT_IO_ERROR );
	}
#endif /* USE_FLOCK */

#if 0
	/* Reset reader. */
	if( ioctl( ri->fd, SC_GPR400_IOCTL_RESET, (char *)NULL ) != 0 ) {
		scGpr400Shutdown( ri );
printf("[F]\n"); fflush(stdout);
		return( SC_EXIT_IO_ERROR );
	}
#endif

	/* Send Select Card command to see if a GPR 400 is really there. */
	if( ioctl( ri->fd, SC_GPR400_IOCTL_TLV, (char *)&selectCard ) != 0 ) {
		scGpr400Shutdown( ri );
		return( SC_EXIT_IO_ERROR );
	}
	if( (selectCard.t!=SC_GPR400_TAG_SELECT_CARD+2) || (selectCard.l!=1) ||
		(scGpr400TranslStatus( selectCard.v[0] )!=SC_EXIT_OK) ) {
		scGpr400Shutdown( ri );
		return( SC_EXIT_IO_ERROR );
	}

	ri->major=SC_READER_GPR400;
	ri->minor=SC_GPR400_GPR400;
	ri->etu=104;
	ri->pinpad=FALSE;
	ri->display=FALSE;

	ri->scShutdown=scGpr400Shutdown;
	ri->scGetCap=scGpr400GetCap;
	ri->scActivate=scGpr400Activate;
	ri->scDeactivate=scGpr400Deactivate;
	ri->scWriteBuffer=NULL;
	ri->scReadBuffer=NULL;
	ri->scWriteChar=NULL;
	ri->scReadChar=NULL;
	ri->scSetSpeed=NULL;
	ri->scCardStatus=scGpr400CardStatus;
	ri->scResetCard=scGpr400ResetCard;
	ri->scPTS=NULL;
	ri->scT0=scGpr400T0;
	ri->scT1=scGpr400T1;
	ri->scSendAPDU=scGpr400SendAPDU;

	return( SC_EXIT_OK );
}

int scGpr400Shutdown( SC_READER_INFO *ri )
{
	SC_GPR400_IOCTL_PARAM_TLV powerDown = { SC_GPR400_TAG_POWER_DOWN,
		1, { 0x00 } };
#if defined(__linux__)
	struct flock lock;
#endif /* __linux__ */
	if( ri->fd==-1 ) return( SC_EXIT_OK );

#if defined(__linux__)
	lock.l_type=F_UNLCK;
	lock.l_start=0;
	lock.l_whence=SEEK_SET;
	lock.l_len=0;
	lock.l_pid=0;
	fcntl( ri->fd, F_SETLK, &lock );
#elif defined(USE_FLOCK)
	flock( ri->fd, LOCK_EX );
#endif /* USE_FLOCK */

	scGpr400Deactivate( ri );

#if 0
	/* Power down reader. */
	if( ioctl( ri->fd, SC_GPR400_IOCTL_TLV, (char *)&powerDown ) != 0 ) {
		return( SC_EXIT_IO_ERROR );
	}
#endif

	close( ri->fd );
	ri->fd=-1;

	return( SC_EXIT_OK );
}

/****************************************************************************
*																			*
*							Low Level Functions								*
*																			*
****************************************************************************/

/* Get Capabilities */

int scGpr400GetCap( SC_READER_INFO *ri, SC_READER_CAP *rp )
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

int scGpr400Activate( SC_READER_INFO *ri )
{
	SC_GPR400_IOCTL_PARAM_TLV openSession = { SC_GPR400_TAG_OPEN_SESSION,
		0, { 0x00 } };
	int ret;

	if( ri->fd==-1 ) return( SC_EXIT_BAD_PARAM );

	/* Activate and reset card. */
	if( ioctl( ri->fd, SC_GPR400_IOCTL_TLV, (char *)&openSession ) != 0 )
		return( SC_EXIT_IO_ERROR );

	if( openSession.t!=SC_GPR400_TAG_OPEN_SESSION+2 ) 
		return( SC_EXIT_PROTOCOL_ERROR );

	if( openSession.l>SC_GENERAL_SHORT_DATA_SIZE )
		return( SC_EXIT_PROTOCOL_ERROR );

	if(	(ret=scGpr400TranslStatus( openSession.v[0] ))!=SC_EXIT_OK )
		return( ret );

	return( SC_EXIT_OK );
}

/* Deactivate Card */

int scGpr400Deactivate( SC_READER_INFO *ri )
{
	SC_GPR400_IOCTL_PARAM_TLV closeSession = { SC_GPR400_TAG_CLOSE_SESSION,
		0, { 0x00 } };
	int ret;

	if( ri->fd==-1 ) return( SC_EXIT_BAD_PARAM );

	/* Deactivate card. */
	if( ioctl( ri->fd, SC_GPR400_IOCTL_TLV, (char *)&closeSession ) != 0 )
		return( SC_EXIT_IO_ERROR );

	if( closeSession.t!=SC_GPR400_TAG_CLOSE_SESSION+2 ) 
		return( SC_EXIT_PROTOCOL_ERROR );

	if( closeSession.l!=0x01 ) 
		return( SC_EXIT_PROTOCOL_ERROR );

	if(	(ret=scGpr400TranslStatus( closeSession.v[0] ))!=SC_EXIT_OK )
		return( ret );

	return( SC_EXIT_OK );
}

/* Write Buffer Async */

int scGpr400WriteBuffer( SC_READER_INFO *ri, SC_CARD_INFO *ci,
    const BYTE *buffer, int len )
{
	SC_GPR400_IOCTL_PARAM_TLV exchangeAPDU = { SC_GPR400_TAG_APDU_EXCHANGE,
		0, { 0x00 } };

#ifdef READER_DEBUG
    {
        int debi;
        printf(" [T1WRITE:");
        for(debi=0;debi<exchangeAPDU.l;debi++) printf(" %.2X",exchangeAPDU.v[debi]);
        printf("]");
    }
#endif
	return SC_EXIT_OK;
}

/* Read Buffer Async */

int scGpr400ReadBuffer( SC_READER_INFO *ri, SC_CARD_INFO *ci,
    BYTE *buffer, int len , LONG timeout )
{
	SC_GPR400_IOCTL_PARAM_TLV exchangeAPDU = { SC_GPR400_TAG_APDU_EXCHANGE,
		0, { 0x00 } };

#ifdef READER_DEBUG
    {
        int debi;
        printf(" [T1READ:");
        for(debi=0;debi<exchangeAPDU.l;debi++) printf(" %.2X",exchangeAPDU.v[debi]);
        printf("]");
    }
#endif
	return SC_EXIT_OK;
}

/* Wait For Data */

int scGpr400WaitForData( SC_READER_INFO *ri, SC_CARD_INFO *ci, LONG timeout )
{
	return SC_EXIT_OK;
}

/* Get card status */

int scGpr400CardStatus( SC_READER_INFO *ri )
{
	SC_GPR400_IOCTL_PARAM_TLV status = { SC_GPR400_TAG_STATUS,
		1, { 0x02 } };
	int ret;

	if( ri->fd==-1 ) return( SC_EXIT_BAD_PARAM );

	ri->status=0x00;

	/* Get Status. */
	if( ioctl( ri->fd, SC_GPR400_IOCTL_TLV, (char *)&status ) != 0 )
		return( SC_EXIT_IO_ERROR );

	if( status.t!=SC_GPR400_TAG_STATUS+2 ) 
		return( SC_EXIT_PROTOCOL_ERROR );

	if( status.l!=0x04 ) 
		return( SC_EXIT_PROTOCOL_ERROR );

	ret=scGpr400TranslStatus( status.v[2] );
	if(	(ret!=SC_EXIT_OK) && (ret!=SC_EXIT_NO_CARD) )
		return( ret );
#ifdef READER_DEBUG
    {
        int debi;
        printf(" [STATUS:");
        for(debi=0;debi<status.l;debi++) printf(" %.2X",status.v[debi]);
        printf("]");
    }
#endif

	if( (status.v[1]&0x90)==0x80 ) ri->status=SC_CARD_STATUS_PRESENT;

	return( SC_EXIT_OK );
}

/****************************************************************************
*																			*
*							SmartCard Functions								*
*																			*
****************************************************************************/

/* Reset Card */

int scGpr400ResetCard( SC_READER_INFO *ri, SC_CARD_INFO *ci )
{
	SC_GPR400_IOCTL_PARAM_TLV openSession = { SC_GPR400_TAG_OPEN_SESSION,
		0, { 0x00 } };
	int ret;

	if( ri->fd==-1 ) return( SC_EXIT_BAD_PARAM );

	/* Activate and reset card. */
	if( ioctl( ri->fd, SC_GPR400_IOCTL_TLV, (char *)&openSession ) != 0 )
		return( SC_EXIT_IO_ERROR );

	if( openSession.t!=SC_GPR400_TAG_OPEN_SESSION+2 ) 
		return( SC_EXIT_PROTOCOL_ERROR );

	if( openSession.l>SC_GENERAL_SHORT_DATA_SIZE )
		return( SC_EXIT_PROTOCOL_ERROR );

	if(	(ret=scGpr400TranslStatus( openSession.v[0] ))!=SC_EXIT_OK )
		return( ret );

#if 0
	/* Save it here, in case it also needs to be fixed. */
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
#endif

	if( openSession.l-1>32 ) return SC_EXIT_BAD_ATR;

	memcpy( ci->atr, openSession.v+1, openSession.l-1 );
	ci->atrlen=openSession.l-1;

	return( SC_EXIT_OK );
}

/* Transmit APDU with protocol T=0 */

int scGpr400T0( SC_READER_INFO *ri, SC_CARD_INFO *ci, SC_APDU *apdu )
{
	SC_GPR400_IOCTL_PARAM_TLV exchangeAPDU = { SC_GPR400_TAG_APDU_EXCHANGE,
		0, { 0x00 } };
	int ret;

	if( ri->fd==-1 ) return( SC_EXIT_BAD_PARAM );

	/* Check if command is too long for GPR400. */
	if( apdu->cmdlen>255 ) return( SC_EXIT_CMD_TOO_LONG );

	/* Check if response might be too long for GPR400. */
	if( (apdu->cse==SC_APDU_CASE_2_SHORT) && (apdu->cmd[4]>253 ) )
		return( SC_EXIT_RSP_TOO_LONG );

	exchangeAPDU.l = apdu->cmdlen+1;
	memcpy( exchangeAPDU.v+1, apdu->cmd, apdu->cmdlen );

	if( exchangeAPDU.l==5 ) {
		/* For case 1 */
		exchangeAPDU.l=6;
		exchangeAPDU.v[5]=0x00;
	}

	/* DIR */
	if( (apdu->cse==SC_APDU_CASE_2_SHORT) ||
		((apdu->cse==SC_APDU_CASE_4_SHORT) && (apdu->cmdlen==5)) ) 
		exchangeAPDU.v[0]=0x01;
	else
		exchangeAPDU.v[0]=0x00;

#ifdef READER_DEBUG
    {
        int debi;
        printf(" [T0CMD:");
        for(debi=0;debi<exchangeAPDU.l;debi++) printf(" %.2X",exchangeAPDU.v[debi]);
        printf("]");
    }
#endif
	/* Send ISO Input/Output command to reader. */
	if( ioctl( ri->fd, SC_GPR400_IOCTL_TLV, (char *)&exchangeAPDU ) != 0 )
		return( SC_EXIT_IO_ERROR );
#ifdef READER_DEBUG
    {
        int debi;
        printf(" [T0RSP:");
        for(debi=0;debi<exchangeAPDU.l;debi++) printf(" %.2X",exchangeAPDU.v[debi]);
        printf("]");
    }
#endif

	if( exchangeAPDU.t!=SC_GPR400_TAG_APDU_EXCHANGE+2 ) 
		return( SC_EXIT_PROTOCOL_ERROR );

	if( exchangeAPDU.l>SC_GENERAL_SHORT_DATA_SIZE )
		return( SC_EXIT_PROTOCOL_ERROR );

	if(	(ret=scGpr400TranslStatus( exchangeAPDU.v[0] ))!=SC_EXIT_OK )
		return( ret );

	memcpy( apdu->rsp, exchangeAPDU.v+1, exchangeAPDU.l-1 );
	apdu->rsplen=exchangeAPDU.l-1;

	return( SC_EXIT_OK );
}

/* Transmit APDU with protocol T=1 */

int scGpr400T1( SC_READER_INFO *ri, SC_CARD_INFO *ci, SC_APDU *apdu )
{
	if( ri->fd==-1 ) return( SC_EXIT_BAD_PARAM );

	return( scT1SendCmd( ri, ci, apdu ) );
}

/* Transmit APDU */

int scGpr400SendAPDU( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	SC_APDU *apdu )
{
#ifdef READER_DEBUG
int debi;
printf(" [CMD:");
for(debi=0;debi<apdu->cmdlen;debi++) printf(" %.2X",apdu->cmd[debi]);
printf("]");
#endif

	if( ri->fd==-1 ) return( SC_EXIT_BAD_PARAM );

	if( ci->protocol==SC_PROTOCOL_T0 ) {
		return( scT0SendAPDU( ri, ci, apdu ) );
	} else if( ci->protocol==SC_PROTOCOL_T1 ) {
		return( scGpr400T1( ri, ci, apdu ) );
	}

	return( SC_EXIT_NOT_IMPLEMENTED );
}



