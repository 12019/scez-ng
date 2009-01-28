/****************************************************************************
*																			*
*					SCEZ chipcard library - Multiflex routines				*
*						Copyright Matthias Bruestle 1999					*
*					For licensing, see the file COPYRIGHT					*
*																			*
****************************************************************************/

/* $Id: scmultiflex.h 1119 2005-05-08 13:28:34Z laforge $ */

#ifndef SC_MULTIFLEX_H
#define SC_MULTIFLEX_H

#include <scez/scgeneral.h>

#define SC_MULTIFLEX_PIN_SIZE	8
#define	SC_MULTIFLEX_CHALL_SIZE	8
#define	SC_MULTIFLEX_MAC_SIZE	6

/* Usefull defines for command parameters */

#define	SC_MULTIFLEX_ALGO_DES			0x00	/* SC_CRYPTOFLEX_ALGO_DES */

#define	SC_MULTIFLEX_FILE_TRANSPARENT	0x01
#define	SC_MULTIFLEX_FILE_FIXED			0x02
#define	SC_MULTIFLEX_FILE_VARIABLE		0x04
#define	SC_MULTIFLEX_FILE_CYCLIC		0x06
#define	SC_MULTIFLEX_FILE_DIRECTORY		0x38

#define	SC_MULTIFLEX_BLOCKED			0x00
#define	SC_MULTIFLEX_UNBLOCKED			0x01

#define	SC_MULTIFLEX_ACCESS_ALWAYS		0x00
#define	SC_MULTIFLEX_ACCESS_PIN			0x01
#define	SC_MULTIFLEX_ACCESS_PROT		0x03
#define	SC_MULTIFLEX_ACCESS_AUTH		0x04
#define	SC_MULTIFLEX_ACCESS_PIN_PROT	0x06
#define	SC_MULTIFLEX_ACCESS_PIN_AUTH	0x08
#define	SC_MULTIFLEX_ACCESS_NEVER		0x0F

#define	SC_MULTIFLEX_RECORD_FIRST		0x00
#define	SC_MULTIFLEX_RECORD_LAST		0x01
#define	SC_MULTIFLEX_RECORD_NEXT		0x02
#define	SC_MULTIFLEX_RECORD_PREV		0x03
#define	SC_MULTIFLEX_RECORD_CURR_INDEX	0x04

#define	SC_MULTIFLEX_FID_PIN			0x0000
#define	SC_MULTIFLEX_FID_INTAUTH		0x0001
#define	SC_MULTIFLEX_FID_SERIAL			0x0002
#define	SC_MULTIFLEX_FID_EXTAUTH		0x0011
#define	SC_MULTIFLEX_FID_MF				0x3F00

#define	SC_MULTIFLEX_OFFSET_TRANSPKEY	15

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* Initialize card function pointer */
int scMultiflexInit( SC_CARD_INFO *ci );

#ifndef SWIG
/* Capabilities */
int scMultiflexGetCap( SC_CARD_INFO *ci, SC_CARD_CAP *cp );
#endif /* !SWIG */

/* Fill card data in ci */
int scMultiflexGetCardData( SC_CARD_INFO *ci );

#ifndef SWIG
/* Set F and D. */
int scMultiflexSetFD( SC_READER_INFO *ri, SC_CARD_INFO *ci, LONG fd );
#endif /* !SWIG */

#ifndef SWIG

/* Generate DES Auth */
void scMultiflexGenerateAuth( const BYTE *key, const BYTE *chall, BYTE *auth );

/* Compare DES Auth */
BOOLEAN scMultiflexCompareAuth( const BYTE *key, const BYTE *chall,
	const BYTE *auth );

/* Generates DES MAC */
/* Data has to be:
 * INS | P1 | P2 | Lc (without cryptogram) | Data
 * Overall it can be at most 24 bytes.
 */
void scMultiflexGenerateMAC( const BYTE *key, const BYTE *chall,
	const BYTE *data, int datalen, BYTE *mac, int *maclen );

#endif /* !SWIG */

/* Commands */

int scMultiflexCmdChangePIN( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	const BYTE *oldpin, const BYTE *newpin );

int scMultiflexCmdCreateFile( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	WORD fid, int flen, BYTE ftype, BYTE init, BYTE status,
	BYTE reclen, BYTE recnum, const BYTE *acond, const BYTE *akeys );

int scMultiflexCmdCreateFileMAC( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	WORD fid, int flen, BYTE ftype, BYTE init, BYTE status, BYTE reclen,
	BYTE recnum, const BYTE *acond, const BYTE *akeys, const BYTE *key,
	const BYTE *chall );

int scMultiflexCmdCreateRecord( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	const BYTE *data, int datalen );

int scMultiflexCmdCreateRecordMAC( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	const BYTE *data, int datalen, const BYTE *key, const BYTE *chall );

int scMultiflexCmdDecrease( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	const BYTE *amount, BYTE *resp, int *resplen );

int scMultiflexCmdDecreaseMAC( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	const BYTE *amount, const BYTE *key, const BYTE *chall, BYTE *resp,
	int *resplen );

int scMultiflexCmdDecreaseSt( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	const BYTE *amount, BYTE *resp, int *resplen );

int scMultiflexCmdDecreaseStMAC( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	const BYTE *amount, const BYTE *key, const BYTE *chall, BYTE *resp,
	int *resplen );

int scMultiflexCmdDeleteFile( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	WORD fid );

int scMultiflexCmdDeleteFileMAC( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	WORD fid, const BYTE *key, const BYTE *chall );

int scMultiflexCmdDirectory( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	BYTE *resp, int *resplen );

int scMultiflexCmdExtAuth( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	BYTE keynum, const BYTE *key, const BYTE *chall );

int scMultiflexCmdGetChall( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	BYTE *chall );

int scMultiflexCmdGiveChall( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	const BYTE *chall );

/* sw is used to get the length of the available data. */
int scMultiflexCmdGetResp( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	BYTE *resp, int *resplen );

int scMultiflexCmdIncrease( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	const BYTE *amount, BYTE *resp, int *resplen );

int scMultiflexCmdIncreaseMAC( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	const BYTE *amount, const BYTE *key, const BYTE *chall, BYTE *resp,
	int *resplen );

int scMultiflexCmdIncreaseSt( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	const BYTE *amount, BYTE *resp, int *resplen );

int scMultiflexCmdIncreaseStMAC( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	const BYTE *amount, const BYTE *key, const BYTE *chall, BYTE *resp,
	int *resplen );

int scMultiflexCmdIntAuth( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	BYTE keynum, const BYTE *chall, const BYTE *key );

int scMultiflexCmdInvalidate( SC_READER_INFO *ri, SC_CARD_INFO *ci );

int scMultiflexCmdInvalidateMAC( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	const BYTE *key, const BYTE *chall );

int scMultiflexCmdReadBinary( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	int offset, BYTE *data, int *datalen );

int scMultiflexCmdReadRecord( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	BYTE recnum, BYTE mode, BYTE *data, int *datalen );

int scMultiflexCmdRehabilitate( SC_READER_INFO *ri, SC_CARD_INFO *ci );

int scMultiflexCmdRehabilitateMAC( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	const BYTE *key, const BYTE *chall );

int scMultiflexCmdSeek( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	BYTE offset, BYTE mode, const BYTE *pattern, BYTE patlen );

int scMultiflexCmdSelectFile( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	WORD fid, BYTE *resp, int *resplen );

int scMultiflexCmdUnblockPIN( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	const BYTE *unblock, const BYTE *newpin );

int scMultiflexCmdUpdateBinary( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	int offset, const BYTE *data, BYTE datalen );

int scMultiflexCmdUpdateBinaryMAC( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	int offset, const BYTE *data, BYTE datalen, const BYTE *key,
	const BYTE *chall );

int scMultiflexCmdUpdateRecord( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	BYTE recnum, BYTE mode, const BYTE *data, BYTE datalen );

int scMultiflexCmdUpdateRecordMAC( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	BYTE recnum, BYTE mode, const BYTE *data, BYTE datalen, const BYTE *key,
	const BYTE *chall );

int scMultiflexCmdVerifyKey( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	BYTE keynum, const BYTE *key, BYTE keylen );

int scMultiflexCmdVerifyPIN( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	const BYTE *pin );

int scMultiflexCmdDesCrypt( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	BYTE keynum, BYTE mode, const BYTE *data, BYTE *resp, int *resplen );

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* SC_MULTIFLEX_H */

