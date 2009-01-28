/****************************************************************************
*																			*
*					SCEZ chipcard library - Smart Card routines				*
*						Copyright Matthias Bruestle 1999					*
*					For licensing, see the file COPYRIGHT					*
*																			*
****************************************************************************/

/* $Id: scsmartcard.h 1056 2001-09-17 23:20:37Z m $ */

#ifndef SC_SMARTCARD_H
#define SC_SMARTCARD_H

#include <scez/scgeneral.h>

#define SC_SPEED_STANDARD	1	/* In most cases 9600bps */
#define	SC_SPEED_FAST		2	/* A fast and probably realiable speed */
#define	SC_SPEED_FASTEST	3	/* The fastest speed available */

/* ATR values for supported cards */

#ifndef SWIG
typedef struct {
	const BYTE *atr;    		/* ATR for card */
	const BYTE *atrMask;		/* Mask for bytes to ignore */
	const int atrLength;		/* Length of ATR */
	const int type;     		/* Card type */
} ATR_VALUE;
#endif /* !SWIG */

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* Determine the card type based on the ATR and fill data in ci */
int scSmartcardGetCardType( SC_CARD_INFO *ci );
#ifndef SWIG
int scSmartcardGetCardTypeExt( SC_CARD_INFO *ci, const ATR_VALUE *atrTab );
#endif /* !SWIG */

/* Process ATR and write results into ci */
int scSmartcardProcessATR( SC_CARD_INFO *ci );

/* Process status word */
int scSmartcardSimpleProcessSW( SC_CARD_INFO *ci, int *status, int *number );

/* Initialize card function pointer */
int scSmartcardInit( SC_CARD_INFO *ci );

#ifndef SWIG
/* Fill card data in cp */
int scSmartcardGetCap( SC_CARD_INFO *ci, SC_CARD_CAP *cp );
#endif /* !SWIG */

/* Fill card data in ci */
int scSmartcardGetCardData( SC_CARD_INFO *ci );

#ifndef SWIG
/* Set F and D. Usable only directly after a reset. */
int scSmartcardSetFD( SC_READER_INFO *ri, SC_CARD_INFO *ci, LONG fd );

/* Sets transmission speed to card.
 * With most cards this can only be called directly aftern a reset.
 * Also with most cards if there is an error the card should be reseted.
 * It would be nice to test the necessary function pointers before
 * calling this function, so that no useless reset is done.
 * Option can be SC_SPEED_STANDARD, SC_SPEED_FAST and SC_SPEED_FASTEST.
 */
int scSmartcardSetSpeed( SC_READER_INFO *ri, SC_CARD_INFO *ci, int option );
#endif /* !SWIG */

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* SC_SMARTCARD_H */

