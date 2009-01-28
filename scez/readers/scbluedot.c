/****************************************************************************
*																			*
*			SCEZ chipcard library - Blue Dot Connector routines				*
*					Copyright Matthias Bruestle 2001						*
*					For licensing, see the file COPYRIGHT					*
*																			*
****************************************************************************/

/* $Id: scbluedot.c 1119 2005-05-08 13:28:34Z laforge $ */

#include <scez/scinternal.h>
#include <scez/scbluedot.h>

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#ifdef __linux__
#define CIB_UNIX
#endif

#include <jibapi.h>

/* Not declared in jibapi.h */
/* void ReleaseJiBSearchList(); */

/* serial ports to search for */
#if defined(__WIN16__) || defined(__WIN32__)
char *g_DeviceNames[] = { "COM1", "COM2", "COM3", "COM4"};
BYTE g_SerialPortCount = 4;
#else
char *g_DeviceNames[] = { "/dev/ttya", "/dev/ttyS0", "/dev/ttyS1" };
BYTE g_SerialPortCount = 3;
#endif

/****************************************************************************
*																			*
*						 		Init/Shutdown Routines						*
*																			*
****************************************************************************/

/* Initializes reader and sets ri */

int scBluedotInit( SC_READER_INFO *ri, const char *param )
{
	/* It does not make sense to set the parameters here, because
	 * they are deleted by a ReleaseJiBAPIResource or a
	 * ReleaseJiBSearchList.
	 */
#if 0
	char *port;

	if( param==NULL ) return( SC_EXIT_BAD_PARAM );

	if( (strcmp(param,"0")==0) || (strcmp(param,"1")==0) ||
		(strcmp(param,"2")==0) || (strcmp(param,"3")==0) ) {
		port=scReaderExpandPort( param );
		if( port==NULL ) return( SC_EXIT_BAD_PARAM );
	} else port=param;

	SetJiBSearchParamsEx( 1, &port, FALSE );
#endif

	ri->major=SC_READER_BLUEDOT;
	ri->minor=SC_BLUEDOT_BLUEDOT;
	ri->etu=104;
	ri->pinpad=FALSE;
	ri->display=FALSE;

	ri->scShutdown=scBluedotShutdown;
	ri->scGetCap=NULL;
	ri->scActivate=scBluedotActivate;
	ri->scDeactivate=scBluedotDeactivate;
	ri->scWriteBuffer=NULL;
	ri->scReadBuffer=NULL;
	ri->scWriteChar=NULL;
	ri->scReadChar=NULL;
	ri->scWaitForData=NULL;
	ri->scSetSpeed=NULL;
	ri->scCardStatus=scBluedotCardStatus;
	ri->scResetCard=scBluedotResetCard;
	ri->scPTS=NULL;
	ri->scT0=NULL;
	ri->scT1=NULL;
	ri->scSendAPDU=scBluedotSendAPDU;
	ri->scVerifyPIN=NULL;
	ri->scWaitReq=NULL;

	return( SC_EXIT_OK );
}

int scBluedotShutdown( SC_READER_INFO *ri )
{
	ReleaseJiBAPIResources();

	return( SC_EXIT_OK );
}

int scBluedotDetect( SC_READER_DETECT_INFO *rdi )
{
	LPBYTE g_lpJiBIdsFound;
	BYTE g_JiBsFound;
	char *port;

	rdi->prob=0;
	strncpy( rdi->name, "BLUEDOT", SC_READER_NAME_MAXLEN );
	rdi->major=SC_READER_BLUEDOT;
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

	/* Because Detect looks for specific ports and Init not, Detect
	 * will find frewer Blue Dot Connectors than Init.
	 */
	SetJiBSearchParamsEx( 1, &port, FALSE );

	/* Search adapters for attached JiBs */
	g_lpJiBIdsFound = FindJiBs(&g_JiBsFound);
	/* ReleaseJiBSearchList(); */
	ReleaseJiBAPIResources();

	rdi->minor=SC_BLUEDOT_BLUEDOT;

	/* Are enough JiBs there for ri->slots? */
	if( g_JiBsFound ) {
		rdi->prob=250;
	} else {
		rdi->prob=1;
	}

	return( SC_EXIT_OK );
}

/****************************************************************************
*																			*
*							Low Level Functions								*
*																			*
****************************************************************************/

/* Activate Card */

int scBluedotActivate( SC_READER_INFO *ri )
{
	int ret;

	if( (ret=scBluedotCardStatus( ri ))!=SC_EXIT_OK ) return( ret );
	if( !(ri->status&SC_CARD_STATUS_PRESENT) ) return( SC_EXIT_NO_CARD );

	return( SC_EXIT_OK );
}

/* Deactivate Card */

int scBluedotDeactivate( SC_READER_INFO *ri )
{
	return( SC_EXIT_OK );
}

/* Get card status */

int scBluedotCardStatus( SC_READER_INFO *ri )
{
	LPBYTE g_lpJiBIdsFound;
	BYTE g_JiBsFound;

	ri->status=0x00;

	/* Search adapters for attached JiBs */
	SetJiBSearchParamsEx(g_SerialPortCount, g_DeviceNames, USB_DS2490 | PARALLEL_DS1481);
	g_lpJiBIdsFound = FindJiBs(&g_JiBsFound);

	/* Are enough JiBs there for ri->slots? */
	if( g_JiBsFound<ri->slot ) {
		/* ReleaseJiBSearchList(); */
		return( SC_EXIT_OK );
	}

	/* Select JiB */
	if( !SelectJiB( g_lpJiBIdsFound+8*(ri->slot-1) ) ) {
		/* ReleaseJiBSearchList(); */
		return( SC_EXIT_OK );
	}

	/* if( memcmp( ci->atr+sizeof(ci->atr)-4, g_lpJiBIdsFound+8*(ri->slot-1)+4,
		4 ) ) ri->status=SC_CARD_STATUS_CHANGED; */

	ri->status=SC_CARD_STATUS_PRESENT;

	/* ReleaseJiBSearchList(); */

	return( SC_EXIT_OK );
}

/****************************************************************************
*																			*
*							SmartCard Functions								*
*																			*
****************************************************************************/

/* Reset Card */

int scBluedotResetCard( SC_READER_INFO *ri, SC_CARD_INFO *ci )
{
	LPBYTE g_lpJiBIdsFound;
	BYTE g_JiBsFound;
	LPRESPONSEAPDU l_lpResponseAPDU;

	/* Search adapters for attached JiBs */
	SetJiBSearchParamsEx(g_SerialPortCount, g_DeviceNames, USB_DS2490 | PARALLEL_DS1481);
	g_lpJiBIdsFound = FindJiBs(&g_JiBsFound);

	/* Are enough JiBs there for ri->slots? */
	if( g_JiBsFound<ri->slot ) {
		/* ReleaseJiBSearchList(); */
		return( SC_EXIT_NO_CARD );
	}

	/* Select JiB */
	if( !SelectJiB( g_lpJiBIdsFound+8*(ri->slot-1) ) ) {
		/* ReleaseJiBSearchList(); */
		return( SC_EXIT_IO_ERROR );
	}

	/* Store last four bytes of serial number to detect changes */
	/* memcpy( ci->atr+sizeof(ci->atr)-4, g_lpJiBIdsFound+8*(ri->slot-1)+4,
		4 ); */

	/* ReleaseJiBSearchList(); */

	/* What good is this ? */
	/* SetDefaultExecTime( 4000 ); */

	/* Get ATR */
	l_lpResponseAPDU = GetATR();
	if( l_lpResponseAPDU->SW != ERR_ISO_NORMAL_00 ) {
		return( SC_EXIT_UNKNOWN_ERROR );
	}

	/* Copy ATR */
	memcpy( ci->atr, l_lpResponseAPDU->Data,
		l_lpResponseAPDU->Len<sizeof(ci->atr) ? l_lpResponseAPDU->Len :
		sizeof(ci->atr) );
	ci->atrlen=l_lpResponseAPDU->Len;

	return( SC_EXIT_OK );
}

/* Transmit APDU */

int scBluedotSendAPDU( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	SC_APDU *apdu )
{
	COMMANDAPDU l_CommandAPDU;
	LPRESPONSEAPDU l_lpResponseAPDU;

#ifdef READER_DEBUG
	int debi;
	printf(" [CMD: %d /",apdu->cse);
	for(debi=0;debi<apdu->cmdlen;debi++) printf(" %.2X",apdu->cmd[debi]);
	printf("]");
#endif

#ifdef SO_ONLY_TEST
	return( SC_EXIT_OK );
#endif

	if( apdu->cmdlen<4 ) return( SC_EXIT_BAD_PARAM );

	memcpy( l_CommandAPDU.Header, apdu->cmd, 4 );
	if( apdu->cmdlen>4 )
		l_CommandAPDU.Lc  = apdu->cmd[0];
	else
		l_CommandAPDU.Lc  = 0x00;
	if( (apdu->cse==SC_APDU_CASE_2_SHORT) || (apdu->cse==SC_APDU_CASE_4_SHORT) )
		l_CommandAPDU.Le  = apdu->cmd[apdu->cmdlen-1];
	else
		l_CommandAPDU.Le  = JIB_APDU_LE_DEFAULT;
	if( apdu->cmdlen>5 )
		l_CommandAPDU.Data = apdu->cmd+5;

	/* Send the APDU */
	l_lpResponseAPDU = SendAPDU(&l_CommandAPDU, 500);

	if( l_lpResponseAPDU==NULL ) return( SC_EXIT_UNKNOWN_ERROR );

	if( l_lpResponseAPDU->Len>256 ) l_lpResponseAPDU->Len=256;
	memcpy( apdu->rsp, l_lpResponseAPDU->Data, l_lpResponseAPDU->Len );
	apdu->rsp[l_lpResponseAPDU->Len] = (l_lpResponseAPDU->SW>>8)&0xFF;
	apdu->rsp[l_lpResponseAPDU->Len+1] = l_lpResponseAPDU->SW&0xFF;
	apdu->rsplen=l_lpResponseAPDU->Len+2;

	return( SC_EXIT_OK );
}



