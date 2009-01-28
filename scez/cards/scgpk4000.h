/****************************************************************************
*																			*
*					SCEZ chipcard library - GPK4000 routines				*
*						Copyright Matthias Bruestle 1999					*
*					For licensing, see the file COPYRIGHT					*
*																			*
****************************************************************************/

/* $Id: scgpk4000.h 1119 2005-05-08 13:28:34Z laforge $ */

#ifndef SC_GPK4000_H
#define SC_GPK4000_H

#include <scez/scgeneral.h>

/* Usefull defines for command parameters */

#define	SC_GPK4000_FID_MF				0x3F00
#define	SC_GPK4000_FID_EF_KEY			0x3F01
#define	SC_GPK4000_FID_DF_SYSTEM		0x0100
#define	SC_GPK4000_FID_EF_CARD			0x0101
#define	SC_GPK4000_FID_EF_ISSUER		0x0102

#define SC_GPK4000_SELECT_MF			0x00
#define SC_GPK4000_SELECT_SFID			0x00
#define SC_GPK4000_SELECT_DF			0x01
#define SC_GPK4000_SELECT_EF			0x02
#define SC_GPK4000_SELECT_PARENT		0x03
#define SC_GPK4000_SELECT_AID			0x04

#define	SC_GPK4000_SFID_EF_KEY			0x01

#define	SC_GPK4000_FTYPE_TRANSPARENT	0x01
#define	SC_GPK4000_FTYPE_FIXED			0x02
#define	SC_GPK4000_FTYPE_FIXED_TLV		0x03
#define	SC_GPK4000_FTYPE_VARIABLE		0x04
#define	SC_GPK4000_FTYPE_VARIABLE_TLV	0x05
#define	SC_GPK4000_FTYPE_CYCLIC			0x06
#define	SC_GPK4000_FTYPE_CYCLIC_TLV		0x07
#define	SC_GPK4000_FTYPE_IADF			0x09
#define	SC_GPK4000_FTYPE_TRANSACTION	0x11
#define	SC_GPK4000_FTYPE_PURSE			0x19
#define	SC_GPK4000_FTYPE_SECRET_CODE	0x21
#define	SC_GPK4000_FTYPE_DES_KEY		0x29
#define	SC_GPK4000_FTYPE_PUBLIC_KEY		0x2C

#define SC_GPK4000_INFO_KEY_DATA		0x05
#define SC_GPK4000_INFO_DESC_ADDR		0x06
#define SC_GPK4000_INFO_CHIP_SN			0xA0
#define SC_GPK4000_INFO_CARD_SN			0xA1
#define SC_GPK4000_INFO_ISSUER_SN		0xA2
#define SC_GPK4000_INFO_ISSUER_REF		0xA3
#define SC_GPK4000_INFO_PRE_ISSUING		0xA4

#define	SC_GPK4000_KEYTYPE_ADMIN		0x00
#define	SC_GPK4000_KEYTYPE_PAYMENT		0x01
#define	SC_GPK4000_KEYTYPE_LOG			0x02
#define	SC_GPK4000_KEYTYPE_AUTH			0x03
#define	SC_GPK4000_KEYTYPE_SIGN			0x10

#define SC_GPK4000_MODE_VERIFY			0x00
#define SC_GPK4000_MODE_VERIFY_CERT		0xFF

#define SC_GPK4000_BLOCK_FIRST			0x01
#define SC_GPK4000_BLOCK_NEXT			0x00
#define SC_GPK4000_BLOCK_LAST			0x10
#define SC_GPK4000_BLOCK_SINGLE			0x11

#define SC_GPK4000_OP_SIGN_RSA_MD5		0x11
#define SC_GPK4000_OP_SIGN_RSA_SHA		0x12
#define SC_GPK4000_OP_SIGN_DSA_SHA		0x13
#define SC_GPK4000_OP_SIGN_RSA_SSL		0x18
#define SC_GPK4000_OP_VERIFY_RSA_MD5	0x21
#define SC_GPK4000_OP_VERIFY_RSA_SHA	0x22
#define SC_GPK4000_OP_VERIFY_DSA_SHA	0x23
#define SC_GPK4000_OP_AUTH_RSA_MD5		0x31
#define SC_GPK4000_OP_AUTH_RSA_SHA		0x32
#define SC_GPK4000_OP_AUTH_DSA_SHA		0x33
#define SC_GPK4000_OP_AUTH_RSA_SSL		0x38
#define SC_GPK4000_OP_DES				0x66

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* Initialize card function pointer */
int scGpk4000Init( SC_CARD_INFO *ci );

#ifndef SWIG
/* Capabilities */
int scGpk4000GetCap( SC_CARD_INFO *ci, SC_CARD_CAP *cp );
#endif /* !SWIG */

/* Fill card data in ci */
int scGpk4000GetCardData( SC_CARD_INFO *ci );

#ifndef SWIG
/* Set F and D. */
int scGpk4000SetFD( SC_READER_INFO *ri, SC_CARD_INFO *ci, LONG fd );
#endif /* !SWIG */

#ifndef SWIG
/* Generate EAC */
void scGpk4000GenerateEac( const BYTE *key, const BYTE *chall, BYTE *auth );

/* Compare EAC */
BOOLEAN scGpk4000CompareEac( const BYTE *key, const BYTE *chall,
	const BYTE *auth );

/* Generate diversified key */
int scGpk4000GenDivKey( const BYTE *key, const BYTE *chall, const BYTE *r_rn,
	BYTE *kats );

/* Generate diversified key PK */
int scGpk4000GenDivKeyPK( const BYTE *key, const BYTE *chall,
	const BYTE *cr_ctc, BYTE *kpts );

/* Generate CRYCKS */
void scGpk4000GenCrycks( const BYTE *key, const BYTE *data, int datalen,
	BYTE *crycks0, BYTE *crycks1 );

/* Check Signature */
int scGpk4000CheckSign( const BYTE *key, const BYTE pfile, const BYTE *ctc,
	const BYTE *tv, const BYTE *bal, const BYTE *sign );
#endif /* !SWIG */

/* Generate CKS for Key Elements */

BYTE scGpk4000GenCKS( BYTE tag, const BYTE *data, int datalen );

/* Extracts last nibble to form the Secret Code.
 * in is 8 bytes long, out is 4 bytes long.
 */
void scGpk4000ComprSC( const BYTE *in, BYTE *out );

/****************************************************************************
*																			*
*							Administration Commands							*
*																			*
****************************************************************************/

int scGpk4000CmdApdRec( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	BYTE sfid, const BYTE *data, BYTE datalen );

int scGpk4000CmdApdRecCrycks( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	BYTE sfid, const BYTE *data, BYTE datalen );

int scGpk4000CmdCrtFile( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	WORD fid, WORD flen, BYTE fdb, BYTE reclen, const BYTE *ac );

int scGpk4000CmdCrtFileCrycks( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	WORD fid, WORD flen, BYTE fdb, BYTE reclen, const BYTE *ac );

int scGpk4000CmdCrtDir( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	WORD fid, const BYTE *name, BYTE namelen, BYTE opt, const BYTE *ac );

int scGpk4000CmdCrtDirCrycks( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	WORD fid, const BYTE *name, BYTE namelen, BYTE opt, const BYTE *ac );

int scGpk4000CmdExtAuth( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	BYTE keynum, BOOLEAN global, BYTE sfid, const BYTE *key,
	const BYTE *chall );

int scGpk4000CmdFreezeAc( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	BOOLEAN file, WORD fid, const BYTE *control );

int scGpk4000CmdFreezeAcCrycks( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	BOOLEAN file, WORD fid, const BYTE *control );

int scGpk4000CmdGetChall( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	BYTE *chall, BOOLEAN longchall );

int scGpk4000CmdGetInfo( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	BYTE p2, BYTE *resp, int *resplen );

/* sw is used to get the length of the available data. */
int scGpk4000CmdGetResp( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	BYTE *resp, int *resplen );

int scGpk4000CmdIntAuth( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	BYTE keynum, BOOLEAN global, BYTE sfid, const BYTE *key,
	const BYTE *chall );

/* offset can be sfid/soffset */
int scGpk4000CmdRdBin( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	WORD offset, BYTE *data, int *datalen );

/* offset can be sfid/soffset */
int scGpk4000CmdRdBinCrycks( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	WORD offset, BYTE *data, int *datalen );

/* Uses sfid if it is in the correct range. */
int scGpk4000CmdRdRec( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	BYTE recnum, BYTE sfid, BYTE *data, int *datalen );

/* Uses sfid if it is in the correct range. */
int scGpk4000CmdRdRecCrycks( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	BYTE recnum, BYTE sfid, BYTE *data, int *datalen );

int scGpk4000CmdSelFil( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	BYTE type, WORD fid, const BYTE *aid, BYTE aidlen, BYTE *resp,
	int *resplen );

int scGpk4000CmdSelFk( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	BYTE keynum, BOOLEAN global, BYTE sfid, const BYTE *key, const BYTE *rand );

int scGpk4000CmdSetCardStatus( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	BYTE system );

/* auth is either the current secret code or the unlock secret code. */
int scGpk4000CmdSetCod( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	BOOLEAN unlock, BYTE scn, const BYTE *auth, const BYTE *newpin );

/* auth is either the current secret code or the unlock secret code. */
int scGpk4000CmdSetCodCrycks( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	BOOLEAN unlock, BYTE scn, const BYTE *auth, const BYTE *newpin );

int scGpk4000CmdSwtSpd( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	BYTE fd, BYTE cegt );

/* offset can be sfid/soffset */
int scGpk4000CmdUpdBin( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	int offset, const BYTE *data, BYTE datalen );

/* offset can be sfid/soffset */
int scGpk4000CmdUpdBinCrycks( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	int offset, const BYTE *data, BYTE datalen );

/* offset can be sfid/soffset */
int scGpk4000CmdUpdBinEncr( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	int offset, const BYTE *data );

/* Uses sfid if it is in the correct range. */
int scGpk4000CmdUpdRec( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	BYTE recnum, BYTE sfid, const BYTE *data, BYTE datalen );

/* Uses sfid if it is in the correct range. */
int scGpk4000CmdUpdRecCrycks( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	BYTE recnum, BYTE sfid, const BYTE *data, BYTE datalen );

int scGpk4000CmdVerify( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	BYTE scn, const BYTE *sc );

int scGpk4000CmdVerifyEncr( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	BYTE scn, const BYTE *sc );

/* offset can be sfid/soffset */
int scGpk4000CmdWrBin( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	int offset, const BYTE *data, BYTE datalen );

/* offset can be sfid/soffset */
int scGpk4000CmdWrBinCrycks( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	int offset, const BYTE *data, BYTE datalen );

/* offset can be sfid/soffset */
int scGpk4000CmdWrBinEncr( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	int offset, const BYTE *data );

/****************************************************************************
*																			*
*							Payment Commands								*
*																			*
****************************************************************************/

int scGpk4000CmdCanDeb( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	const BYTE *last, const BYTE *newdeb, const BYTE *ttc, BYTE *resp,
	int *resplen );

int scGpk4000CmdCredit( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	BYTE keynum, const BYTE *kc, const BYTE *ctc, const BYTE *credit,
	BYTE pfile );

int scGpk4000CmdDebit( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	BYTE pfile, const BYTE *debit, const BYTE *ttc );

int scGpk4000CmdRdBal( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	BYTE pfile, const BYTE *ttc, BYTE *balance );

int scGpk4000CmdSelPk( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	BYTE keynum, BYTE keyfile, BYTE pfile, const BYTE *key, const BYTE *rand,
	BYTE *ctc );

int scGpk4000CmdSetOpts( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	BYTE option );

int scGpk4000CmdSign( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	BYTE keynum, BYTE sfid, BYTE *resp, int *resplen );

/****************************************************************************
*																			*
*							Public Key Commands								*
*																			*
****************************************************************************/

int scGpk4000CmdCompDesKey( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	BYTE modulus, BYTE *resp, int *resplen );

int scGpk4000CmdCrtPrivKeyFile( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	BYTE sfid, BYTE words );

int scGpk4000CmdDesEnc( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	const BYTE *enc, BYTE *resp, int *resplen );

/* Offset is normally 0x07 with GPK4000-s. */
int scGpk4000CmdEraseCard( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	BYTE offset );

/* Is the CLA correct? */
int scGpk4000CmdInitHashed( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	const BYTE *hash, BYTE len );

int scGpk4000CmdLoadPrivKey( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	BYTE sfid, BYTE prklen, const BYTE *key, BYTE len );

/* Le not known in advance. Which is correct?
 * - Case 2 with Le=0x00
 * - Case 4 with Lc=0x00 and Le=0x00
 */
int scGpk4000CmdPkDir( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	BYTE *resp, int *resplen );

int scGpk4000CmdPkIntAuth( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	BYTE modulus, BYTE *resp, int *resplen );

int scGpk4000CmdPkSend( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	BYTE sfid, BYTE pklen, const BYTE *key, BYTE len );

int scGpk4000CmdPkSign( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	BYTE modulus, BYTE *resp, int *resplen );

int scGpk4000CmdPkVerify( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	BYTE verify, const BYTE *sig, BYTE siglen, BYTE *resp, int *resplen );

int scGpk4000CmdPutCryptoData( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	BYTE block, BYTE blklen, const BYTE *data, BYTE datalen );

int scGpk4000CmdSelCryptoContext( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	BYTE sfid, BYTE mode );

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* SC_GPK4000_H */


