/****************************************************************************
*																			*
*					SCEZ chipcard library - Quick routines					*
*						Copyright Matthias Bruestle 2000					*
*					For licensing, see the file COPYRIGHT					*
*																			*
****************************************************************************/

/* $Id: scquick.h 1119 2005-05-08 13:28:34Z laforge $ */

#ifndef SC_QUICK_H
#define SC_QUICK_H

#include <scez/scgeneral.h>

#define	SC_QUICK_CHALL_SIZE	8

/* Usefull defines for command parameters */

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* Initialize card function pointer */
int scQuickInit( SC_CARD_INFO *ci );

/* Fill card data in ci */
int scQuickGetCardData( SC_CARD_INFO *ci );

int scQuickCmdGetChall( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	BYTE *chall );

int scQuickCmdReadBinary( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	WORD offset, BYTE *data, int *datalen );

int scQuickCmdReadRecord( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	BYTE recnum, BYTE p2, BYTE *data, int *datalen );

int scQuickCmdSelectFile( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	WORD fid );

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* SC_QUICK_H */


