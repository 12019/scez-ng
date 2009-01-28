/****************************************************************************
*																			*
*					SCEZ chipcard library - T=1 routines					*
*	This is probably the first full free/open source T=1 implementation.	*
*						Copyright Matthias Bruestle 1999					*
*					For licensing, see the file COPYRIGHT					*
*																			*
****************************************************************************/

/* $Id: sct1.h 1056 2001-09-17 23:20:37Z m $ */

#ifndef SC_T1_H
#define SC_T1_H

#include <scez/scgeneral.h>

#define SC_T1_MAX_BLKLEN	3+SC_GENERAL_SHORT_DATA_SIZE+2+2
#define SC_T1_MAX_SBLKLEN	3+1+2

/* S-Block parameter */

#define	SC_T1_S_RESYNCH		0x00
#define	SC_T1_S_IFS			0x01
#define	SC_T1_S_ABORT		0x02
#define	SC_T1_S_WTX			0x03

#define	SC_T1_S_REQUEST		0x00
#define	SC_T1_S_RESPONSE	0x01

#define SC_T1_S_IFS_MAX		0xFE

/* R-Block parameter */

#define	SC_T1_R_OK				0x00
#define	SC_T1_R_EDC_ERROR		0x01
#define	SC_T1_R_OTHER_ERROR		0x02

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* Returns LRC of data */
BYTE scT1Lrc( const BYTE *data, int datalen);

/* Calculates CRC of data */
void scT1Crc( const BYTE *data, int datalen, BYTE *crc );

/* Appends RC */
int scT1AppendRc( SC_T1_INFO *t1, BYTE *data, int *datalen );

/* Checks RC. */
BOOLEAN scT1CheckRc( SC_T1_INFO *t1, const BYTE *data, int datalen );

/* Builds S-Block */
int scT1SBlock( SC_T1_INFO *t1, int type, int dir, int param, BYTE *block,
	int *len );

/* Builds R-Block */
int scT1RBlock( SC_T1_INFO *t1, int type, BYTE *block, int *len );

/* Builds I-Block */
int scT1IBlock( SC_T1_INFO *t1, BOOLEAN more, const BYTE *data, int datalen,
	BYTE *block, int *blocklen );

/* Returns N(R) or N(S) from R/I-Block. */
int scT1GetN( const BYTE *block );

/* Transmit APDU with protocol T=1 */
/* Supports only cases 1, 2 Short, 3 Short, 4 Short.
 * Does not do the full Resync/Reset game but exits very early with
 * an error.
 * Inverse convention is handled by the Read/Write functions.
 */
int scT1SendCmd( SC_READER_INFO *ri, SC_CARD_INFO *ci, SC_APDU *apdu );

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* SC_T1_H */

