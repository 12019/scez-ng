/****************************************************************************
*																			*
*					SCEZ chipcard library - GCR 400 routines				*
*						Copyright Matthias Bruestle 1999					*
*					For licensing, see the file COPYRIGHT					*
*																			*
****************************************************************************/

/* $Id: scgcr400.h 1119 2005-05-08 13:28:34Z laforge $ */

#ifndef SC_GCR400_H
#define SC_GCR400_H

#include <scez/scgeneral.h>

#define SC_GCR400_MAX_BUFFER_SIZE	SC_GENERAL_SHORT_DATA_SIZE

/* Reader minor types */

#define SC_GCR400_GCR400	0x00

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* Initialize reader */
int scGcr400Init( SC_READER_INFO *ri, const char *param );

/* Shutdown reader */
int scGcr400Shutdown( SC_READER_INFO *ri );

/* Get Capabilities */
int scGcr400GetCap( SC_READER_INFO *ri, SC_READER_CAP *rp );

/* Activate card */
int scGcr400Activate( SC_READER_INFO *ri );

/* Deactivate card */
int scGcr400Deactivate( SC_READER_INFO *ri );

/* Get card status */
int scGcr400CardStatus( SC_READER_INFO *ri );

/* Send command. */
/* rlen specfies the maximum bytes to return and returns the bytes returned. */
int scGcr400SendCmd( SC_READER_INFO *ri, BYTE *send, BYTE slen, BYTE *rec,
    BYTE *rlen, LONG wait );

/* Reset card and read ATR */
int scGcr400ResetCard( SC_READER_INFO *ri, SC_CARD_INFO *ci );

/* Transmit APDU with protocol T=0 */
int scGcr400T0( SC_READER_INFO *ri, SC_CARD_INFO *ci, SC_APDU *apdu );

/* Transmit APDU with protocol T=1 */
int scGcr400T1( SC_READER_INFO *ri, SC_CARD_INFO *ci, SC_APDU *apdu );

/* Transmit APDU */
int scGcr400SendAPDU( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	SC_APDU *apdu );

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* SC_GCR400_H */


