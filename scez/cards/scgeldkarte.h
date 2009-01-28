/****************************************************************************
*																			*
*					SCEZ chipcard library - Geldkarte routines				*
*						Copyright Matthias Bruestle 1999					*
*					For licensing, see the file COPYRIGHT					*
*																			*
****************************************************************************/

/* $Id: scgeldkarte.h 1119 2005-05-08 13:28:34Z laforge $ */

#ifndef SC_GELDKARTE_H
#define SC_GELDKARTE_H

#include <scez/scgeneral.h>

#define	SC_GELDKARTE_CHALL_SIZE	8

/* Usefull defines for command parameters */

#define	SC_GELDKARTE_READREC_SELECTED	0x01
#define	SC_GELDKARTE_READREC_SFID		0x02

#define	SC_GELDKARTE_SELECT_MF			0x00
#define	SC_GELDKARTE_SELECT_DF			0x01
#define	SC_GELDKARTE_SELECT_EF			0x02
#define	SC_GELDKARTE_SELECT_BACK		0x03
#define	SC_GELDKARTE_SELECT_AID			0x04

#define	SC_GELDKARTE_SELRESP_NONE		0x0C
#define	SC_GELDKARTE_SELRESP_FCI		0x00
#define	SC_GELDKARTE_SELRESP_FCP		0x04
#define	SC_GELDKARTE_SELRESP_FMD		0x08

#define SC_GELDKARTE_AID				"\xD2\x76\x00\x00\x25\x45\x50\x01\x00"
#define SC_GELDKARTE_AIDLEN				9

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* Initialize card function pointer */
int scGeldkarteInit( SC_CARD_INFO *ci );

/* Fill card data in ci */
int scGeldkarteGetCardData( SC_CARD_INFO *ci );

int scGeldkarteCmdGetChall( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	BYTE *chall );

int scGeldkarteCmdGetStat( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	BYTE *resp, int *resplen );

int scGeldkarteCmdReadRecord( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	BYTE recnum, BYTE mode, BYTE sfid, BYTE *data, int *datalen );

int scGeldkarteCmdSelectFile( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	BYTE selmode, WORD fid, const BYTE *aid, BYTE aidlen, BYTE respmode,
	BYTE *resp, int *resplen );

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* SC_GELDKARTE_H */


