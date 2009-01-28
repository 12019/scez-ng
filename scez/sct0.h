/****************************************************************************
*																			*
*					SCEZ chipcard library - T=0 routines					*
*						Copyright Matthias Bruestle 1999					*
*					For licensing, see the file COPYRIGHT					*
*																			*
****************************************************************************/

/* $Id: sct0.h 1056 2001-09-17 23:20:37Z m $ */

#ifndef SC_T0_H
#define SC_T0_H

#include <scez/scgeneral.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* Transmit APDU with protocol T=0 */

/* Supports only cases 1, 2 Short, 3 Short, 4 Short.
 * Case 4 Short:
 *  - You have to get the response data yourself, e.g. with GET RESPONSE
 *  - The le-byte is omitted.
 */

int scT0SendCmd( SC_READER_INFO *ri, SC_CARD_INFO *ci, SC_APDU *apdu );

/* Transmit APDU */

/*  - For case 4 instructions: The le-bytes is striped before calling
 *    scReaderT0.
 */

int scT0SendAPDU( SC_READER_INFO *ri, SC_CARD_INFO *ci, SC_APDU *apdu );

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* SC_T0_H */

