/****************************************************************************
*																			*
*					SCEZ chipcard library - PTS routines					*
*						Copyright Matthias Bruestle 1999					*
*					For licensing, see the file COPYRIGHT					*
*																			*
****************************************************************************/

/* $Id: scpts.h 1056 2001-09-17 23:20:37Z m $ */

#ifndef SC_PTS_H
#define SC_PTS_H

#include <scez/scgeneral.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* Adjustes ptslen for PCK. */
int scPtsCalcPCK( BYTE *pts, int *ptslen );

BOOLEAN scPtsCheckPCK( const BYTE *pts, int ptslen );

/* Do a PTS handshake. */
int scPtsDoPTS( SC_READER_INFO *ri, SC_CARD_INFO *ci, const BYTE *pts,
	int ptslen );

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* SC_PTS_H */

