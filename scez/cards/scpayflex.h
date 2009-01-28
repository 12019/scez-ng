/****************************************************************************
*																			*
*					SCEZ chipcard library - Payflex routines				*
*						Copyright Matthias Bruestle 2000					*
*					For licensing, see the file COPYRIGHT					*
*																			*
****************************************************************************/

/* $Id: scpayflex.h 1119 2005-05-08 13:28:34Z laforge $ */

#ifndef SC_PAYFLEX_H
#define SC_PAYFLEX_H

#include <scez/scgeneral.h>

#define SC_PAYFLEX_PIN_SIZE			0x08
#define	SC_PAYFLEX_CHALL_SIZE		0x08
#define	SC_PAYFLEX_MAC_SIZE			0x06

#define	SC_PAYFLEX_ALGO_DES			0x00
#define	SC_PAYFLEX_ALGO_3DES		0x02

#define	SC_PAYFLEX_RECORD_FIRST		0x00
#define	SC_PAYFLEX_RECORD_LAST		0x01
#define	SC_PAYFLEX_RECORD_NEXT		0x02
#define	SC_PAYFLEX_RECORD_PREV		0x03
#define	SC_PAYFLEX_RECORD_CURR_INDEX	0x04

#if 0

#define	SC_PAYFLEX_DES_SIZE			0x08
#define	SC_PAYFLEX_RSA_SIZE			0x80

/* Usefull defines for command parameters */

#define	SC_PAYFLEX_MODE_DECRYPT		0x00
#define	SC_PAYFLEX_MODE_ENCRYPT		0x01

#define	SC_PAYFLEX_FILE_TRANSPARENT	0x01
#define	SC_PAYFLEX_FILE_FIXED		0x02
#define	SC_PAYFLEX_FILE_VARIABLE	0x04
#define	SC_PAYFLEX_FILE_CYCLIC		0x06
#define	SC_PAYFLEX_FILE_DIRECTORY	0x38

#define	SC_PAYFLEX_BLOCKED			0x00
#define	SC_PAYFLEX_UNBLOCKED		0x01

#define	SC_PAYFLEX_ACCESS_ALWAYS	0x00
#define	SC_PAYFLEX_ACCESS_PIN		0x01
#define	SC_PAYFLEX_ACCESS_PROT		0x03
#define	SC_PAYFLEX_ACCESS_AUTH		0x04
#define	SC_PAYFLEX_ACCESS_PIN_PROT	0x06
#define	SC_PAYFLEX_ACCESS_PIN_AUTH	0x08
#define	SC_PAYFLEX_ACCESS_NEVER		0x0F

#define	SC_PAYFLEX_FID_PIN			0x0000
#define	SC_PAYFLEX_FID_INTAUTH		0x0001
#define	SC_PAYFLEX_FID_SERIAL		0x0002
#define	SC_PAYFLEX_FID_EXTAUTH		0x0011
#define	SC_PAYFLEX_FID_SECKEY		0x0012
#define	SC_PAYFLEX_FID_PUBKEY		0x1012
#define	SC_PAYFLEX_FID_MF			0x3F00

#define	SC_PAYFLEX_OFFSET_TRANSPKEY	15

#endif

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* Initialize card function pointer */
int scPayflexInit( SC_CARD_INFO *ci );

#ifndef SWIG
/* Capabilities */
int scPayflexGetCap( SC_CARD_INFO *ci, SC_CARD_CAP *cp );
#endif /* !SWIG */

/* Fill card data in ci */
int scPayflexGetCardData( SC_CARD_INFO *ci );

#ifndef SWIG
/* Set F and D. */
int scPayflexSetFD( SC_READER_INFO *ri, SC_CARD_INFO *ci, LONG fd );

/* Generate Auth */
void scPayflexGenerateAuth( const BYTE *key, const BYTE *chall, BYTE *auth,
    BYTE algo );

/* Compare Auth */
BOOLEAN scPayflexCompareAuth( const BYTE *key, const BYTE *chall,
	const BYTE *auth, BYTE algo );
#endif /* !SWIG */

/* Commands */
int scPayflexCmdChange( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	const BYTE *oldpin, const BYTE *newpin );

int scPayflexCmdCreateFile( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	WORD fid, int flen, BYTE ftype, BYTE init, BYTE status, BYTE reclen,
	BYTE recnum, const BYTE *acond, const BYTE *akeys );

int scPayflexCmdExtAuth( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	BYTE keynum, const BYTE *key, const BYTE *chall, BYTE algo );

int scPayflexCmdGetChall( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	BYTE *chall, int *len );

/* sw is used to get the length of the available data. */
int scPayflexCmdGetResp( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	BYTE *resp, int *resplen );

int scPayflexCmdIntAuth( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	BYTE keynum, const BYTE *chall, const BYTE *key, BYTE algo );

int scPayflexCmdReadRecord( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	BYTE recnum, BYTE mode, BYTE *data, int *datalen );

int scPayflexCmdSelect( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	WORD fid, BYTE *resp, int *resplen );

int scPayflexCmdUnblock( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	const BYTE *unblock, const BYTE *newpin );

int scPayflexCmdUpdateRecord( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	BYTE recnum, BYTE mode, const BYTE *data, BYTE datalen );

int scPayflexCmdVerify( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	const BYTE *pin );

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* SC_PAYFLEX_H */

