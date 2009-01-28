/****************************************************************************
*																			*
*				SCEZ chipcard library - EasyCheck routines					*
*					Copyright Matthias Bruestle 1999,2000					*
*					For licensing, see the file COPYRIGHT					*
*																			*
****************************************************************************/

/* $Id: sceasycheck.h 1119 2005-05-08 13:28:34Z laforge $ */

#ifndef SC_EASYCHECK_H
#define SC_EASYCHECK_H

#include <scez/scgeneral.h>

/* #define SC_EASYCHECK_MAX_BUFFER_SIZE	SC_GENERAL_SHORT_DATA_SIZE+2 */

/* Reader minor types */

#define SC_EASYCHECK_EASYCHECK	0x00
#define	SC_EASYCHECK_EASYCHECK	0x00

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
int scEasycheckInit( SC_READER_INFO *ri, const char *param );

/* Shutdown reader */
int scEasycheckShutdown( SC_READER_INFO *ri );

/* Detect reader */
int scEasycheckDetect( SC_READER_DETECT_INFO *rdi );

/* Get Capabilities */
int scEasycheckGetCap( SC_READER_INFO *ri, SC_READER_CAP *rp );

/* Activate card */
int scEasycheckActivate( SC_READER_INFO *ri );

/* Deactivate card */
int scEasycheckDeactivate( SC_READER_INFO *ri );

/* Write Buffer Async */
int scEasycheckWriteBuffer( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	const BYTE *buffer, int len );

/* Read Buffer Async */
int scEasycheckReadBuffer( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	BYTE *buffer, int len, LONG timeout );

/* Write Char Async */
int scEasycheckWriteChar( SC_READER_INFO *ri, SC_CARD_INFO *ci, int ch );

/* Read Char Async */
int scEasycheckReadChar( SC_READER_INFO *ri, SC_CARD_INFO *ci, LONG timeout );

/* Wait For Data */
int scEasycheckWaitForData( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	LONG timeout );

/* Set Speed */
int scEasycheckSetSpeed( SC_READER_INFO *ri, LONG speed );

/* Get card status */
int scEasycheckCardStatus( SC_READER_INFO *ri );

/* Reset and fetch TS */
int scEasycheckGetTS( SC_READER_INFO *ri, SC_CARD_INFO *ci );

/* Get Rest of ATR */
int scEasycheckGetATR( SC_READER_INFO *ri, SC_CARD_INFO *ci );

/* Reset card and read ATR */
int scEasycheckResetCard( SC_READER_INFO *ri, SC_CARD_INFO *ci );

/* Transmit APDU with protocol T=0 */
/* Supports only cases 1, 2 Short, 3 Short, 4 Short.
 * You have to get the response data with Case 4 Short yourself,
 * e.g. GET RESPONSE
 */
int scEasycheckT0( SC_READER_INFO *ri, SC_CARD_INFO *ci, SC_APDU *apdu );

/* Transmit APDU with protocol T=1 */
int scEasycheckT1( SC_READER_INFO *ri, SC_CARD_INFO *ci, SC_APDU *apdu );

/* Transmit APDU */
int scEasycheckSendAPDU( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	SC_APDU *apdu );

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* SC_EASYCHECK_H */


