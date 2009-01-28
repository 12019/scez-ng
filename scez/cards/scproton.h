/****************************************************************************
*																			*
*					SCEZ chipcard library - Proton routines					*
*						Copyright Matthias Bruestle 1999					*
*					For licensing, see the file COPYRIGHT					*
*																			*
****************************************************************************/

/* $Id: scproton.h 1119 2005-05-08 13:28:34Z laforge $ */

#ifndef SC_PROTON_H
#define SC_PROTON_H

#include <scez/scgeneral.h>

/* Usefull defines for command parameters */

#define SC_PROTON_EF_FAB		0x17FF
#define SC_PROTON_EF_PURSE		0x2901

#define	SC_PROTON_RECORD_ABS	0x00
#define	SC_PROTON_RECORD_FIRST	0x01
#define	SC_PROTON_RECORD_PREV	0x02
#define	SC_PROTON_RECORD_NEXT	0x03

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* Initialize card function pointer */
int scProtonInit( SC_CARD_INFO *ci );

/* Fill card data in ci */
int scProtonGetCardData( SC_CARD_INFO *ci );

int scProtonCmdLookupBalance( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	BYTE *data, int *datalen );

int scProtonCmdReadBinary( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	WORD offset, BYTE *data, int *datalen );

int scProtonCmdReadRecord( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	BYTE recnum, BYTE mode, BYTE *data, int *datalen );

int scProtonCmdSelectFile( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	WORD fid, BYTE *resp, int *resplen );

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* SC_PROTON_H */


