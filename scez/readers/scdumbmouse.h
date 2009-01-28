/****************************************************************************
*																			*
*				SCEZ chipcard library - Dumb Mouse routines					*
*					Copyright Matthias Bruestle 1999,2000					*
*					For licensing, see the file COPYRIGHT					*
*																			*
****************************************************************************/

/* $Id: scdumbmouse.h 1119 2005-05-08 13:28:34Z laforge $ */

#ifndef SC_DUMBMOUSE_H
#define SC_DUMBMOUSE_H

#include <scez/scgeneral.h>

/* #define SC_DUMBMOUSE_MAX_BUFFER_SIZE	SC_GENERAL_SHORT_DATA_SIZE+2 */

/* Reader minor types */

#define SC_DUMBMOUSE_DUMBMOUSE	0x00
#define	SC_DUMBMOUSE_UNIPROG	0x01	/* Card detection only by DCD. */
#define SC_DUMBMOUSE_DEJAN		0x02	/* No card detection. */
#define SC_DUMBMOUSE_PHOENIX	0x03	/* Echo cancelation required. */

#if defined(__linux__) || defined(__FreeBSD__) || \
	( defined(__alpha) && defined(__osf__) )
#ifndef HAVE_LIBCRYPT
#ifndef WITH_DCD
#define WITH_DCD
#endif /* WITH_DCD */
#endif /* HAVE_LIBCRYPT */
#endif /* __linux__ || __FreeBSD__ */

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* Initialize reader */
int scDumbmouseInit( SC_READER_INFO *ri, const char *param );

/* Shutdown reader */
int scDumbmouseShutdown( SC_READER_INFO *ri );

/* Detect reader */
int scDumbmouseDetect( SC_READER_DETECT_INFO *rdi );

/* Get Capabilities */
int scDumbmouseGetCap( SC_READER_INFO *ri, SC_READER_CAP *rp );

/* Activate card */
int scDumbmouseActivate( SC_READER_INFO *ri );

/* Deactivate card */
int scDumbmouseDeactivate( SC_READER_INFO *ri );

/* Write Buffer Async */
int scDumbmouseWriteBuffer( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	const BYTE *buffer, int len );

/* Read Buffer Async */
int scDumbmouseReadBuffer( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	BYTE *buffer, int len, LONG timeout );

/* Write Char Async */
int scDumbmouseWriteChar( SC_READER_INFO *ri, SC_CARD_INFO *ci, int ch );

/* Read Char Async */
int scDumbmouseReadChar( SC_READER_INFO *ri, SC_CARD_INFO *ci, LONG timeout );

/* Wait For Data */
int scDumbmouseWaitForData( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	LONG timeout );

/* Set Speed */
int scDumbmouseSetSpeed( SC_READER_INFO *ri, LONG speed );

/* Get card status */
int scDumbmouseCardStatus( SC_READER_INFO *ri );

/* Reset and fetch TS */
int scDumbmouseGetTS( SC_READER_INFO *ri, SC_CARD_INFO *ci );

/* Get Rest of ATR */
int scDumbmouseGetATR( SC_READER_INFO *ri, SC_CARD_INFO *ci );

/* Reset card and read ATR */
int scDumbmouseResetCard( SC_READER_INFO *ri, SC_CARD_INFO *ci );

/* Transmit APDU with protocol T=0 */
/* Supports only cases 1, 2 Short, 3 Short, 4 Short.
 * You have to get the response data with Case 4 Short yourself,
 * e.g. GET RESPONSE
 */
int scDumbmouseT0( SC_READER_INFO *ri, SC_CARD_INFO *ci, SC_APDU *apdu );

/* Transmit APDU with protocol T=1 */
int scDumbmouseT1( SC_READER_INFO *ri, SC_CARD_INFO *ci, SC_APDU *apdu );

/* Transmit APDU */
int scDumbmouseSendAPDU( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	SC_APDU *apdu );

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* SC_DUMBMOUSE_H */


