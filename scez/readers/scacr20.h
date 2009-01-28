/****************************************************************************
*																			*
*					SCEZ chipcard library - ACR20 routines					*
*						Copyright Matthias Bruestle 1999					*
*					For licensing, see the file COPYRIGHT					*
*																			*
****************************************************************************/

/* $Id: scacr20.h 1119 2005-05-08 13:28:34Z laforge $ */

#ifndef SC_ACR20_H
#define SC_ACR20_H

#include <scez/scgeneral.h>

#define SC_ACR20_MAX_BUFFER_SIZE	SC_GENERAL_SHORT_DATA_SIZE+6

/* Reader minor types */

#define SC_ACR20_SERIAL		0x00
#define SC_ACR20_USB		0x01
#define SC_ACR20_PS2		0x02

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* Initialize reader */
int scAcr20Init( SC_READER_INFO *ri, const char *param );

/* Shutdown reader */
int scAcr20Shutdown( SC_READER_INFO *ri );

/* Detect reader */
int scAcr20Detect( SC_READER_DETECT_INFO *rdi );

/* Get Capabilities */
int scAcr20GetCap( SC_READER_INFO *ri, SC_READER_CAP *rp );

/* Activate card */
int scAcr20Activate( SC_READER_INFO *ri );

/* Deactivate card */
int scAcr20Deactivate( SC_READER_INFO *ri );

/* Get card status */
int scAcr20CardStatus( SC_READER_INFO *ri );

/* Send command. */
/* rlen specfies the maximum bytes to return and returns the bytes returned.
 * Supports only the short command format.
 * send contains the command without header and checksum.
 * rec returns the response without header and checksum.
 * Send data length is encoded in the send.
 * TODO: Long command format.
 */
int scAcr20SendCmd( SC_READER_INFO *ri, BYTE *send, BYTE *rec, int *rlen,
	LONG wait );

/* Reset card and read ATR */
int scAcr20ResetCard( SC_READER_INFO *ri, SC_CARD_INFO *ci );

/* Transmit APDU with protocol T=0 */
int scAcr20T0( SC_READER_INFO *ri, SC_CARD_INFO *ci, SC_APDU *apdu );

/* Transmit APDU with protocol T=1 */
int scAcr20T1( SC_READER_INFO *ri, SC_CARD_INFO *ci, SC_APDU *apdu );

/* Transmit APDU */
int scAcr20SendAPDU( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	SC_APDU *apdu );

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* SC_ACR20_H */


