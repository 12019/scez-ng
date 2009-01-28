/****************************************************************************
*																			*
*					SCEZ chipcard library - Cryptoflex routines				*
*						Copyright Matthias Bruestle 1999					*
*					For licensing, see the file COPYRIGHT					*
*																			*
****************************************************************************/

/* $Id: sccryptoflex.h 1119 2005-05-08 13:28:34Z laforge $ */

#ifndef SC_CRYPTOFLEX_H
#define SC_CRYPTOFLEX_H

#include <scez/scgeneral.h>

#define SC_CRYPTOFLEX_PIN_SIZE			0x08
#define	SC_CRYPTOFLEX_CHALL_SIZE		0x08
#define	SC_CRYPTOFLEX_MAC_SIZE			0x06
#define	SC_CRYPTOFLEX_DES_SIZE			0x08
#define	SC_CRYPTOFLEX_RSA_SIZE			0x80

/* Usefull defines for command parameters */

#define	SC_CRYPTOFLEX_ALGO_DES			0x00	/* SC_MULTIFLEX_ALGO_DES */
#define	SC_CRYPTOFLEX_ALGO_3DES			0x02

#define	SC_CRYPTOFLEX_MODE_DECRYPT		0x00
#define	SC_CRYPTOFLEX_MODE_ENCRYPT		0x01

#define	SC_CRYPTOFLEX_FILE_TRANSPARENT	0x01
#define	SC_CRYPTOFLEX_FILE_FIXED		0x02
#define	SC_CRYPTOFLEX_FILE_VARIABLE		0x04
#define	SC_CRYPTOFLEX_FILE_CYCLIC		0x06
#define	SC_CRYPTOFLEX_FILE_DIRECTORY	0x38

#define	SC_CRYPTOFLEX_BLOCKED			0x00
#define	SC_CRYPTOFLEX_UNBLOCKED			0x01

#define	SC_CRYPTOFLEX_ACCESS_ALWAYS		0x00
#define	SC_CRYPTOFLEX_ACCESS_PIN		0x01
#define	SC_CRYPTOFLEX_ACCESS_PROT		0x03
#define	SC_CRYPTOFLEX_ACCESS_AUTH		0x04
#define	SC_CRYPTOFLEX_ACCESS_PIN_PROT	0x06
#define	SC_CRYPTOFLEX_ACCESS_PIN_AUTH	0x08
#define	SC_CRYPTOFLEX_ACCESS_NEVER		0x0F

#define	SC_CRYPTOFLEX_RECORD_FIRST		0x00
#define	SC_CRYPTOFLEX_RECORD_LAST		0x01
#define	SC_CRYPTOFLEX_RECORD_NEXT		0x02
#define	SC_CRYPTOFLEX_RECORD_PREV		0x03
#define	SC_CRYPTOFLEX_RECORD_CURR_INDEX	0x04

#define	SC_CRYPTOFLEX_FID_PIN			0x0000
#define	SC_CRYPTOFLEX_FID_INTAUTH		0x0001
#define	SC_CRYPTOFLEX_FID_SERIAL		0x0002
#define	SC_CRYPTOFLEX_FID_EXTAUTH		0x0011
#define	SC_CRYPTOFLEX_FID_SECKEY		0x0012
#define	SC_CRYPTOFLEX_FID_PUBKEY		0x1012
#define	SC_CRYPTOFLEX_FID_MF			0x3F00

#define	SC_CRYPTOFLEX_OFFSET_TRANSPKEY	15

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* Initialize card function pointer */
int scCryptoflexInit( SC_CARD_INFO *ci );

#ifndef SWIG
/* Capabilities */
int scCryptoflexGetCap( SC_CARD_INFO *ci, SC_CARD_CAP *cp );
#endif /* !SWIG */

/* Fill card data in ci */
int scCryptoflexGetCardData( SC_CARD_INFO *ci );

#ifndef SWIG
/* Set F and D. */
int scCryptoflexSetFD( SC_READER_INFO *ri, SC_CARD_INFO *ci, LONG fd );
#endif /* !SWIG */

#ifndef SWIG
/* Generate DES Auth */
void scCryptoflexGenerateAuth( const BYTE *key, const BYTE *chall, BYTE *auth,
	BYTE algo );

/* Compare DES Auth */
BOOLEAN scCryptoflexCompareAuth( const BYTE *key, const BYTE *chall,
	const BYTE *auth, BYTE algo );

/* Generates DES MAC */
/* Data has to be:
 * INS | P1 | P2 | Lc (without cryptogram) | Data
 * Overall it can be at most 24 bytes.
 */
void scCryptoflexGenerateMAC( const BYTE *key, const BYTE *chall,
	const BYTE *data, int datalen, BYTE *mac, int *maclen, BYTE algo );
#endif /* !SWIG */

/* Commands */

int scCryptoflexCmdChangePIN( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	BYTE num, const BYTE *oldpin, const BYTE *newpin );

int scCryptoflexCmdCreateFile( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	WORD fid, int flen, BYTE ftype, BYTE init, BYTE status, BYTE reclen,
	BYTE recnum, const BYTE *acond, const BYTE *akeys );

int scCryptoflexCmdCreateFileMAC( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	WORD fid, int flen, BYTE ftype, BYTE init, BYTE status, BYTE reclen,
	BYTE recnum, const BYTE *acond, const BYTE *akeys, const BYTE *key,
	const BYTE *chall, BYTE algo );

int scCryptoflexCmdCreateRecord( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	const BYTE *data, int datalen );

int scCryptoflexCmdCreateRecordMAC( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	const BYTE *data, int datalen, const BYTE *key, const BYTE *chall,
	BYTE algo );

int scCryptoflexCmdDecrease( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	const BYTE *amount, BYTE *resp, int *resplen );

int scCryptoflexCmdDecreaseMAC( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	const BYTE *amount, const BYTE *key, const BYTE *chall, BYTE algo,
	BYTE *resp, int *resplen );

int scCryptoflexCmdDeleteFile( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	WORD fid );

int scCryptoflexCmdDeleteFileMAC( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	WORD fid, const BYTE *key, const BYTE *chall, BYTE algo );

int scCryptoflexCmdDirectory( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	BYTE *resp, int *resplen );

int scCryptoflexCmdExtAuth( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	BYTE keynum, const BYTE *key, const BYTE *chall, BYTE algo );

int scCryptoflexCmdGetChall( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	BYTE *chall, int *len );

/* sw is used to get the length of the available data. */
int scCryptoflexCmdGetResp( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	BYTE *resp, int *resplen );

int scCryptoflexCmdIncrease( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	const BYTE *amount, BYTE *resp, int *resplen );

int scCryptoflexCmdIncreaseMAC( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	const BYTE *amount, const BYTE *key, const BYTE *chall, BYTE algo,
	BYTE *resp, int *resplen );

int scCryptoflexCmdIntAuth( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	BYTE keynum, const BYTE *chall, const BYTE *key, BYTE algo );

int scCryptoflexCmdInvalidate( SC_READER_INFO *ri, SC_CARD_INFO *ci );

int scCryptoflexCmdInvalidateMAC( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	const BYTE *key, const BYTE *chall, BYTE algo );

int scCryptoflexCmdReadBinary( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	int offset, BYTE *data, int *datalen );

int scCryptoflexCmdReadRecord( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	BYTE recnum, BYTE mode, BYTE *data, int *datalen );

int scCryptoflexCmdRehabilitate( SC_READER_INFO *ri, SC_CARD_INFO *ci );

int scCryptoflexCmdRehabilitateMAC( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	const BYTE *key, const BYTE *chall, BYTE algo );

int scCryptoflexCmdSeek( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	BYTE offset, BYTE mode, const BYTE *pattern, BYTE patlen );

int scCryptoflexCmdSelectFile( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	WORD fid, BYTE *resp, int *resplen );

int scCryptoflexCmdUnblockPIN( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	BYTE num, const BYTE *unblock, const BYTE *newpin );

int scCryptoflexCmdUpdateBinary( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	int offset, const BYTE *data, BYTE datalen );

int scCryptoflexCmdUpdateBinaryMAC( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	int offset, const BYTE *data, BYTE datalen, const BYTE *key,
	const BYTE *chall, BYTE algo );

int scCryptoflexCmdUpdateRecord( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	BYTE recnum, BYTE mode, const BYTE *data, BYTE datalen );

int scCryptoflexCmdUpdateRecordMAC( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	BYTE recnum, BYTE mode, const BYTE *data, BYTE datalen, const BYTE *key,
	const BYTE *chall, BYTE algo );

int scCryptoflexCmdVerifyKey( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	BYTE keynum, const BYTE *key, BYTE keylen );

int scCryptoflexCmdVerifyPIN( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	BYTE num, const BYTE *pin );

/* Full DES IntAuth */
int scCryptoflexCmdDesCrypt( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	BYTE keynum, BYTE mode, const BYTE *data, BYTE *resp, int *resplen );

int scCryptoflexCmdLoadCert( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	BYTE keynum, const BYTE *data );

/* Waits 4 minutes for key generation and issues than a Get Response. */
int scCryptoflexCmdRsaKeyGen( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	BYTE keynum, BYTE *resp, int *resplen );

/* The 4 byte exponent has to be LSB as every other big integer. */
int scCryptoflexCmdRsaKeyGenUpd( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	BYTE keysize, BYTE *exp );

/* IntAuth with RSA Key */
int scCryptoflexCmdRsaSign( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	BYTE keynum, const BYTE *data, BYTE *resp, int *resplen );

int scCryptoflexCmdSHA1Interm( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	const BYTE *data );

int scCryptoflexCmdSHA1Last( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	const BYTE *data, int datalen, BYTE *resp, int *resplen );

int scCryptoflexCmdUpdateEnc( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	int offset, const BYTE *data, BYTE datalen, const BYTE *key, BYTE algo );

int scCryptoflexCmdVerifyPubKey( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	const BYTE *data );

int scCryptoflexCmdVerifyData( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	BYTE keynum, const BYTE *data, BYTE *resp, int *resplen );

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* SC_CRYPTOFLEX_H */

