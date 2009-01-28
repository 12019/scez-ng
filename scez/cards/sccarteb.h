/****************************************************************************
*																			*
*				SCEZ chipcard library - Carte Bancaire routines				*
*						Copyright Matthias Bruestle 1999					*
*					For licensing, see the file COPYRIGHT					*
*																			*
****************************************************************************/

/* $Id: sccarteb.h 1119 2005-05-08 13:28:34Z laforge $ */

#ifndef SC_CARTEB_H
#define SC_CARTEB_H

#include <scez/scgeneral.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* Initialize card function pointer */
int scCartebInit( SC_CARD_INFO *ci );

/* Fill card data in ci */
int scCartebGetCardData( SC_CARD_INFO *ci );

/* Must start with a 3 */
int scCartebStrip( BYTE *in, int inlen, BYTE *out, int *outlen );

int scCartebPin2Hex( BYTE *pin, BYTE *hex );

int scCartebCmdReadBin( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	WORD offset, BYTE *data, int *datalen );

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* SC_CARTEB_H */

