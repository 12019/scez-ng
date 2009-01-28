/****************************************************************************
*																			*
*					SCEZ chipcard library - Reflex 60 routines				*
*						Copyright Matthias Bruestle 2000					*
*					For licensing, see the file COPYRIGHT					*
*																			*
****************************************************************************/

/* $Id: screflex60.h 1119 2005-05-08 13:28:34Z laforge $ */

#ifndef SC_REFLEX60_H
#define SC_REFLEX60_H

#include <scez/scgeneral.h>

/* Reader minor types */

#define SC_REFLEX60_REFLEX60	0x00
#define	SC_REFLEX60_REFLEX62	0x02
#define SC_REFLEX60_REFLEX64	0x04

/* Defines */

#define SC_REFLEX60_ACK		0x62

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* Initializes reader and sets ri */
int scReflex60Init( SC_READER_INFO *ri, const char *param );

int scReflex60Shutdown( SC_READER_INFO *ri );

/* Detect reader */
int scReflex60Detect( SC_READER_DETECT_INFO *rdi );

/* Get Capabilities */
int scReflex60GetCap( SC_READER_INFO *ri, SC_READER_CAP *rp );

/* Activate Card */
int scReflex60Activate( SC_READER_INFO *ri );

/* Deactivate Card */
int scReflex60Deactivate( SC_READER_INFO *ri );

/* The following Read/Write commands are probably only usable for sct1. */

/* Write Buffer Async */
int scReflex60WriteBuffer( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	const BYTE *buffer, int len );

/* Read Buffer Async */
int scReflex60ReadBuffer( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	BYTE *buffer, int len , LONG timeout );

/* Read Char Async */
int scReflex60ReadChar( SC_READER_INFO *ri, SC_CARD_INFO *ci, LONG timeout );

/* Wait For Data */
int scReflex60WaitForData( SC_READER_INFO *ri, SC_CARD_INFO *ci, LONG timeout );

/* Set Speed */
int scReflex60SetSpeed( SC_READER_INFO *ri, LONG speed );

/* Get card status */
int scReflex60CardStatus( SC_READER_INFO *ri );

int scReflex60SendCmd( SC_READER_INFO *ri, BYTE *cmd, int cmdlen, BYTE *rsp,
	int *rsplen );

/* Reset Card */
int scReflex60ResetCard( SC_READER_INFO *ri, SC_CARD_INFO *ci );

/* Transmit APDU with protocol T=0 */
/* Supports only cases 1, 2 Short, 3 Short, 4 Short.
 * Case 4 Short:
 *  - You have to get the response data yourself, e.g. with GET RESPONSE
 *  - The le-byte is omitted.
 */
int scReflex60T0( SC_READER_INFO *ri, SC_CARD_INFO *ci, SC_APDU *apdu );

/* Transmit APDU with protocol T=1 */
int scReflex60T1( SC_READER_INFO *ri, SC_CARD_INFO *ci, SC_APDU *apdu );

/* Transmit APDU */
int scReflex60SendAPDU( SC_READER_INFO *ri, SC_CARD_INFO *ci, SC_APDU *apdu );

/* Transmit command using PIN pad */
int scReflex60VerifyPIN( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	SC_APDU *apdu, const char *message, int pinlen, int pincoding, int pinpos );

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* SC_REFLEX60_H */

