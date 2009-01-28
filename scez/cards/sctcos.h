/****************************************************************************
*																			*
*					SCEZ chipcard library - TCOS 2.0 routines				*
*						Copyright Matthias Bruestle 1999					*
*					For licensing, see the file COPYRIGHT					*
*																			*
****************************************************************************/

/* $Id: sctcos.h 1119 2005-05-08 13:28:34Z laforge $ */

#ifndef SC_TCOS_H
#define SC_TCOS_H

#include <scez/scgeneral.h>

/* Usefull defines for command parameters */

/* Change password */

#define SC_TCOS_NULLPIN				"\x00\x00\x00\x00\x00\x00"
#define SC_TCOS_NULLPINLEN			6

/* SM */

typedef struct sc_tcos_sm_info {
	BYTE cencalgo;
	BYTE cenckey[24];
	BYTE cenciv[8];
	BYTE cmacalgo;
	BYTE cmackey[24];
	BYTE cmaciv[8];
	int opts;
	BYTE rencalgo;
	BYTE renckey[24];
	BYTE renciv[8];
	BYTE rmacalgo;
	BYTE rmackey[24];
	BYTE rmaciv[8];
	int flags;
} SC_TCOS_SM_INFO;

/* Encryption options */
#define SC_TCOS_SM_OPTS_MAC_RSP		0x01	/* Response should be MACed */
#define SC_TCOS_SM_OPTS_OBJS		0x02	/* Data are TLV objects */
#define SC_TCOS_SM_OPTS_HEADER		0x04	/* Header should be MACed */
#define SC_TCOS_SM_OPTS_ENVELOPE	0x08	/* Envelope command */
/* Flags */
#define SC_TCOS_SM_FLAG_MAC			0x01	/* Response was MACed */
#define SC_TCOS_SM_FLAG_ENC			0x02	/* Response was encrypted */
#define SC_TCOS_SM_FLAG_MACERR		0x04	/* MAC error */
#define SC_TCOS_SM_FLAG_ENCERR		0x08	/* Encryption error */
/* Algos */
#define SC_TCOS_SM_ALGO_NONE		0x00
#define SC_TCOS_SM_ALGO_DES_ECB		0x01
#define SC_TCOS_SM_ALGO_DES_CBC		0x02
#define SC_TCOS_SM_ALGO_3DES2K_ECB	0x03
#define SC_TCOS_SM_ALGO_3DES2K_CBC	0x04
#define SC_TCOS_SM_ALGO_3DES3K_ECB	0x05
#define SC_TCOS_SM_ALGO_3DES3K_CBC	0x06
#define SC_TCOS_SM_ALGO_IDEA_ECB	0x07
#define SC_TCOS_SM_ALGO_IDEA_CBC	0x08

/* List directory */

#define SC_TCOS_LIST_DF				0x01
#define SC_TCOS_LIST_EF				0x02

/* Manage security environment */

#define SC_TCOS_MSE_APP_SM_APDU		0x11
#define SC_TCOS_MSE_APP_SM_RAPDU	0x21
#define SC_TCOS_MSE_APP_PSO			0x41

#define SC_TCOS_MSE_OP_MAC			0xB4
#define SC_TCOS_MSE_OP_ENCRYPT		0xB8

#define SC_TCOS_MSE_ALGO_IDEA		0x04
#define SC_TCOS_MSE_ALGO_DES		0x08
#define SC_TCOS_MSE_ALGO_DES3		0x0C
#define SC_TCOS_MSE_ALGO_RSA		0x10

#define SC_TCOS_MSE_MODE_ECB		0x00
#define SC_TCOS_MSE_MODE_CBC		0x01

/* Perform security operation */

#define SC_TCOS_PSO_OUT_NONE		0x00	/* Verify hash */
#define SC_TCOS_PSO_OUT_DEC_TXT		0x80	/* Decrypt */
#define SC_TCOS_PSO_OUT_ENC_TXT_TLV	0x84	/* Encrypt */
#define SC_TCOS_PSO_OUT_ENC_TXT		0x86	/* Encrypt */
#define SC_TCOS_PSO_OUT_CKS			0x8E	/* Calculate hash */
#define SC_TCOS_PSO_OUT_SIGN		0x9E	/* Sign */

#define SC_TCOS_PSO_IN_CLEAR_TXT	0x80	/* Calculate hash/Encrypt */
#define	SC_TCOS_PSO_IN_ENC_TXT_TLV	0x84	/* Decrypt */
#define	SC_TCOS_PSO_IN_ENC_TXT		0x86	/* Decrypt */
#define	SC_TCOS_PSO_IN_HASH			0x9A	/* Sign */
#define	SC_TCOS_PSO_IN_VER_TEMPLATE	0xA2	/* Verify hash */

/* Read/Update record */

#define SC_TCOS_RECORD_FIRST		0x00
#define SC_TCOS_RECORD_LAST			0x01
#define SC_TCOS_RECORD_NEXT			0x02
#define SC_TCOS_RECORD_PREV			0x03
#define SC_TCOS_RECORD_ABS			0x04

/* Select */

#define	SC_TCOS_FID_MF				0x3F00

#define SC_TCOS_SELECT_MF			0x00
#define SC_TCOS_SELECT_DF			0x01
#define SC_TCOS_SELECT_EF			0x02
#define SC_TCOS_SELECT_PARENT		0x03
#define SC_TCOS_SELECT_AID			0x04
#define SC_TCOS_SELECT_ABS_PATH		0x08
#define SC_TCOS_SELECT_REL_PATH		0x09

#define SC_TCOS_RDATA_FCI			0x00
#define SC_TCOS_RDATA_FCP			0x04
#define SC_TCOS_RDATA_FMD			0x08
#define SC_TCOS_RDATA_NONE			0x0C

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* Initialize card function pointer */
int scTcosInit( SC_CARD_INFO *ci );

#ifndef SWIG
/* Capabilities */
int scTcosGetCap( SC_CARD_INFO *ci, SC_CARD_CAP *cp );
#endif /* !SWIG */

/* Fill card data in ci */
int scTcosGetCardData( SC_CARD_INFO *ci );

#ifndef SWIG
/* Set F and D. */
int scTcosSetFD( SC_READER_INFO *ri, SC_CARD_INFO *ci, LONG fd );
#endif /* !SWIG */

void scTcosEncCAPDU( SC_APDU *apdu, BYTE encalgo, const BYTE *enckey,
    const BYTE *enciv, BYTE macalgo, const BYTE *mackey, const BYTE *maciv,
    int opts );

void scTcosDecRAPDU( SC_APDU *apdu, BYTE encalgo, const BYTE *enckey,
    const BYTE *enciv, BYTE macalgo, const BYTE *mackey, const BYTE *maciv,
    int flags );

/* TCOS 2.0 R2 p.78 */
int scTcosCmdAppendRecord( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	BYTE sfid, const BYTE *data, BYTE datalen );

/* TCOS 2.0 R2 p.66 */
int scTcosCmdAskRandom( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	BYTE *rand, int *len );

/* TCOS 2.0 R2 p.62 */
int scTcosCmdChangePassword( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	BOOLEAN local, BYTE num, const BYTE *oldpass, BYTE oldlen,
	const BYTE *newpass, BYTE newlen );

/* TCOS 2.0 R2 p.80 */
int scTcosCmdCreate( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	const BYTE *data, BYTE datalen );

/* TCOS 2.0 R2 p.83 */
int scTcosCmdDelete( SC_READER_INFO *ri, SC_CARD_INFO *ci, WORD fid );

/* TCOS 2.0 R2 p.90 */
int scTcosCmdExcludeSFI( SC_READER_INFO *ri, SC_CARD_INFO *ci, BYTE sfi );

/* TCOS 2.0 R2 p.67 */
int scTcosCmdExtAuth( SC_READER_INFO *ri, SC_CARD_INFO *ci );

/* TCOS 2.0 R2 p.77 */
int scTcosCmdGetSessionkey( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	BYTE algo, BYTE *resp, int *resplen );

/* TCOS 2.0 R2 p.88 */
int scTcosCmdIncludeSFI( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	BYTE sfi, const BYTE *path, BYTE pathlen );

/* TCOS 2.0 R2 p.69 */
int scTcosCmdIntAuth( SC_READER_INFO *ri, SC_CARD_INFO *ci );

/* TCOS 2.0 R2 p.86 */
int scTcosCmdInvalidate( SC_READER_INFO *ri, SC_CARD_INFO *ci );

/* TCOS 2.0 R2 p.52 */
int scTcosCmdListDir( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	BYTE ftype, BYTE start, BYTE *resp, int *resplen );

/* TCOS 2.0 R2 p.70 */
int scTcosCmdManageSecEnv( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	BYTE app, BYTE op, const BYTE *data, BYTE datalen );

/* TCOS 2.0 R2 p.73 */
int scTcosCmdPerformSecOp( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	BYTE in, BYTE out, const BYTE *data, BYTE datalen, BYTE *resp,
	int *resplen );

/* offset can be sfid/soffset (highest bit set); */
/* TCOS 2.0 R2 p.53 */
int scTcosCmdReadBinary( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	WORD offset, BYTE *data, int *datalen );

/* Uses sfid if it is in the correct range. */
/* TCOS 2.0 R2 p.55 */
int scTcosCmdReadRecord( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	BYTE rec, BYTE addr, BYTE sfid, BYTE *data, int *datalen );

/* TCOS 2.0 R2 p.87 */
int scTcosCmdRehabilitate( SC_READER_INFO *ri, SC_CARD_INFO *ci );

/* TCOS 2.0 R2 p.50 */
int scTcosCmdSelect( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	BYTE type, WORD fid, const BYTE *aidpath, BYTE aidpathlen, BYTE rtype,
	BYTE *resp, int *resplen );

/* TCOS 2.0 R2 p.85 */
int scTcosCmdSetPermanent( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	BOOLEAN actpin );

/* TCOS 2.0 R2 p.64 */
int scTcosCmdUnblockPassword( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	BOOLEAN local, BYTE num, const BYTE *puk, BYTE puklen,
	const BYTE *newpass, BYTE newlen );

/* offset can be sfid/soffset (highest bit set); */
/* TCOS 2.0 R2 p.54 */
int scTcosCmdUpdateBinary( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	WORD offset, const BYTE *data, BYTE datalen );

/* Uses sfid if it is in the correct range. */
/* TCOS 2.0 R2 p.57 */
int scTcosCmdUpdateRecord( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	BYTE rec, BYTE addr, BYTE sfid, const BYTE *data, BYTE datalen );

/* TCOS 2.0 R2 p.60 */
int scTcosCmdVerifyPassword( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	BOOLEAN local, BYTE num, const BYTE *pass, BYTE passlen );

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* SC_TCOS_H */


