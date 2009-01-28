/****************************************************************************
*																			*
*					SCEZ chipcard library - Towitoko routines				*
*						Copyright Matthias Bruestle 1999					*
*					For licensing, see the file COPYRIGHT					*
*																			*
****************************************************************************/

/* $Id: sctowitoko.h 1119 2005-05-08 13:28:34Z laforge $ */

#ifndef SC_TOWITOKO_H
#define SC_TOWITOKO_H

#include <scez/scgeneral.h>

#define SC_TOWITOKO_MAX_CMD_SIZE	15
#define SC_TOWITOKO_MAX_BUFFER_SIZE	SC_GENERAL_SHORT_DATA_SIZE+2

/* Reader LED colour settings */

#define SC_TOWITOKO_LED_OFF		0
#define SC_TOWITOKO_LED_ON		1
#define SC_TOWITOKO_LED_RED		1
#define SC_TOWITOKO_LED_GREEN	2
#define SC_TOWITOKO_LED_YELLOW	3

/* Reader minor type (Terminal return codes) */

/* Addition to those below exists:
 *   0x64: Chipdrive Micro
 */

#define SC_TOWITOKO_CHIPDRIVE_MICRO		0x61
#define SC_TOWITOKO_KARTENZWERG			0x80
#define SC_TOWITOKO_CHIPDRIVE_EXTERN	0x84
#define SC_TOWITOKO_CHIPDRIVE_TWIN		0x88
#define SC_TOWITOKO_CHIPDRIVE_INTERN	0x90

/* Card status codes */

/* #define SC_TOWITOKO_????		0x10 */
/* #define SC_TOWITOKO_????		0x20 */
#define SC_TOWITOKO_CARD		0x40
#define SC_TOWITOKO_CHANGE		0x80

/* Update Checksum */
#define scTowitokoUpdateCS( cs, byte )   cs = (( ((cs ^ byte) << 1) | ((cs ^ byte) >> 7) ) & 0xFF ) ^ 0x01

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* Initialize reader */
int scTowitokoInit( SC_READER_INFO *ri, const char *param );

/* Shutdown reader */
int scTowitokoShutdown( SC_READER_INFO *ri );

/* Detect reader */
int scTowitokoDetect( SC_READER_DETECT_INFO *rdi );

/* Get Capabilities */
int scTowitokoGetCap( SC_READER_INFO *ri, SC_READER_CAP *rp );

/* Activate card */
int scTowitokoActivate( SC_READER_INFO *ri );

/* Deactivate card */
int scTowitokoDeactivate( SC_READER_INFO *ri );

/* Write Buffer Async */
int scTowitokoWriteBuffer( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	const BYTE *buffer, int len );

/* Read Buffer Async */
int scTowitokoReadBuffer( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	BYTE *buffer, int len, LONG timeout );

/* Write Char Async */
int scTowitokoWriteChar( SC_READER_INFO *ri, SC_CARD_INFO *ci, int ch );

/* Read Char Async */
int scTowitokoReadChar( SC_READER_INFO *ri, SC_CARD_INFO *ci, LONG timeout );

/* Wait For Data */
int scTowitokoWaitForData( SC_READER_INFO *ri, SC_CARD_INFO *ci, LONG timeout );

/* Set Speed */
int scTowitokoSetSpeed( SC_READER_INFO *ri, LONG speed );

/* Set LED On/Off */
int scTowitokoLED( SC_READER_INFO *ri, int status );

/* Get card status */
int scTowitokoCardStatus( SC_READER_INFO *ri );

/* Transmit data bytes transparently to card */
int scTowitokoSendData( SC_READER_INFO *ri, BYTE *data, int len );

/* Set parity of reader */
int scTowitokoSetParity( SC_READER_INFO *ri, int parity );

/* Reset card and read ATR */
int scTowitokoResetCard( SC_READER_INFO *ri, SC_CARD_INFO *ci );

/* Transmit APDU with protocol T=0 */
/* Supports only cases 1, 2 Short, 3 Short, 4 Short.
 * You have to get the response data with Case 4 Short yourself,
 * e.g. GET RESPONSE
 */
int scTowitokoT0( SC_READER_INFO *ri, SC_CARD_INFO *ci, SC_APDU *apdu );

/* Transmit APDU with protocol T=1 */
int scTowitokoT1( SC_READER_INFO *ri, SC_CARD_INFO *ci, SC_APDU *apdu );

/* Transmit APDU */
int scTowitokoSendAPDU( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	SC_APDU *apdu );

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* SC_TOWITOKO_H */

