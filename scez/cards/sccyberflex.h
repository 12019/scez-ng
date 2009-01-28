/****************************************************************************
*																			*
*					SCEZ chipcard library - Cyberflex routines				*
*						Copyright Matthias Bruestle 2000					*
*					For licensing, see the file COPYRIGHT					*
*																			*
****************************************************************************/

/* $Id: sccyberflex.h 1119 2005-05-08 13:28:34Z laforge $ */

#ifndef SC_CYBERFLEX_H
#define SC_CYBERFLEX_H

#include <scez/scgeneral.h>

#define SC_CYBERFLEX_ACL_SIZE			0x08
#define SC_CYBERFLEX_PIN_SIZE			0x08
#define	SC_CYBERFLEX_CHALL_SIZE			0x08
#define	SC_CYBERFLEX_DES_SIZE			0x08
#define	SC_CYBERFLEX_RSA512_SIZE		0x40
#define	SC_CYBERFLEX_RSA768_SIZE		0x60
#define	SC_CYBERFLEX_RSA1024_SIZE		0x80

/* EF ACL flags */

#define SC_CYBERFLEX_ACL_EF_READ		0x01
#define SC_CYBERFLEX_ACL_EF_WRITE		0x02
#define SC_CYBERFLEX_ACL_EF_EXECUTE		0x04
#define SC_CYBERFLEX_ACL_EF_CREATEREC	0x04
#define SC_CYBERFLEX_ACL_EF_INVALID		0x08
#define SC_CYBERFLEX_ACL_EF_REHABIL		0x10
#define SC_CYBERFLEX_ACL_EF_DECREASE	0x40
#define SC_CYBERFLEX_ACL_EF_INCREASE	0x80

/* DF ACL flags */

#define SC_CYBERFLEX_ACL_DF_LIST		0x01
#define SC_CYBERFLEX_ACL_DF_DELETE		0x02
#define SC_CYBERFLEX_ACL_DF_CHANGEACL	0x04
#define SC_CYBERFLEX_ACL_DF_CREATEFILE	0x20
#define SC_CYBERFLEX_ACL_DF_MANAGE		0x40

/* Usefull defines for command parameters */

#define	SC_CYBERFLEX_ALGO_DES			0x00
#define	SC_CYBERFLEX_ALGO_IDPIN			0x01
#define	SC_CYBERFLEX_ALGO_3DES			0x02
#define	SC_CYBERFLEX_ALGO_UNBLPIN		0x03
#define	SC_CYBERFLEX_ALGO_RSA512_PRIV	0xC4
#define	SC_CYBERFLEX_ALGO_RSA512_PUB	0xC5
#define	SC_CYBERFLEX_ALGO_RSA768_PRIV	0xC6
#define	SC_CYBERFLEX_ALGO_RSA768_PUB	0xC7
#define	SC_CYBERFLEX_ALGO_RSA1024_PRIV	0xC8
#define	SC_CYBERFLEX_ALGO_RSA1024_PUB	0xC9

#define	SC_CYBERFLEX_MODE_DECRYPT		0x00
#define	SC_CYBERFLEX_MODE_ENCRYPT		0x01

#define	SC_CYBERFLEX_FILE_BINARY		0x02	/* Transparent File */
#define	SC_CYBERFLEX_FILE_PROGRAM		0x03
#define	SC_CYBERFLEX_FILE_FIXED			0x0C
#define	SC_CYBERFLEX_FILE_VARIABLE		0x19
#define	SC_CYBERFLEX_FILE_CYCLIC		0x1D
#define	SC_CYBERFLEX_FILE_DIRECTORY		0x20	/* Dedicated File */
#define	SC_CYBERFLEX_FILE_APPLICATION	0x21

#define	SC_CYBERFLEX_STATUS_BLOCKED		0x00
#define	SC_CYBERFLEX_STATUS_UNBLOCKED	0x01
#define SC_CYBERFLEX_STATUS_INCREASE	0x02

#define	SC_CYBERFLEX_RECORD_FIRST		0x00
#define	SC_CYBERFLEX_RECORD_LAST		0x01
#define	SC_CYBERFLEX_RECORD_NEXT		0x02
#define	SC_CYBERFLEX_RECORD_PREV		0x03
#define	SC_CYBERFLEX_RECORD_ABS			0x04

#define	SC_CYBERFLEX_FID_CHV1			0x0000
#define	SC_CYBERFLEX_FID_INTAUTH		0x0001
#define	SC_CYBERFLEX_FID_EXTAUTH		0x0011
#define	SC_CYBERFLEX_FID_SECKEY			0x0012
#define	SC_CYBERFLEX_FID_CHV2			0x0100
#define	SC_CYBERFLEX_FID_PUBKEY			0x1012
#define	SC_CYBERFLEX_FID_MF				0x3F00
#define SC_CYBERFLEX_FID_CURRENT		0xFFFF	/* For Delete All Files */

#define	SC_CYBERFLEX_SELECT_FILE		0x00
#define	SC_CYBERFLEX_SELECT_PARENT		0x03
#define	SC_CYBERFLEX_SELECT_APPL		0x04
#define	SC_CYBERFLEX_SELECT_LOADER		0x04

#define SC_CYBERFLEX_INSTANCE_DELETE	0x02
#define SC_CYBERFLEX_INSTANCE_RESET		0x03
#define SC_CYBERFLEX_INSTANCE_INIT_CURRENT	0x04
#define SC_CYBERFLEX_INSTANCE_INIT_NONE	0x05
#define SC_CYBERFLEX_INSTANCE_BLOCK		0x07

#define SC_CYBERFLEX_EXECUTE_MAIN		0x02
#define SC_CYBERFLEX_EXECUTE_INSTALL	0x13

#define SC_CYBERFLEX_TYPE_APPLET		0x01
#define SC_CYBERFLEX_TYPE_APPLICATION	0x02

#define SC_CYBERFLEX_STATUSLEN_REFULAR	0x17
#define SC_CYBERFLEX_STATUSLEN_APPL		0x28

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* Initialize card function pointer */
int scCyberflexInit( SC_CARD_INFO *ci );

#ifndef SWIG
/* Capabilities */
int scCyberflexGetCap( SC_CARD_INFO *ci, SC_CARD_CAP *cp );
#endif /* !SWIG */

/* Fill card data in ci */
int scCyberflexGetCardData( SC_CARD_INFO *ci );

#ifndef SWIG
/* Set F and D. */
int scCyberflexSetFD( SC_READER_INFO *ri, SC_CARD_INFO *ci, LONG fd );
#endif /* !SWIG */

#ifndef SWIG
/* Generate Auth */
void scCyberflexGenerateAuth( const BYTE *key, const BYTE *chall, BYTE *auth,
	BYTE algo );

/* Compare Auth */
BOOLEAN scCyberflexCompareAuth( const BYTE *key, const BYTE *chall,
	const BYTE *auth, BYTE algo );
#endif /* !SWIG */

/* Sign Code */
void scCyberflexSignCode( const BYTE *key, const BYTE *data, int datalen,
	BYTE *auth, int *authlen, BYTE algo );

/****************************************************************************
*																			*
*							Administrative Commands							*
*																			*
****************************************************************************/

int scCyberflexCmdAppendRecord( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	const BYTE *data, int datalen );

int scCyberflexCmdChangeCHV( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	BYTE chvnum, const BYTE *oldpin, const BYTE *newpin );

int scCyberflexCmdChangeFileACL( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	const BYTE *acl );

int scCyberflexCmdChangeJavaATR( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	const BYTE *atr, BYTE atrlen );

int scCyberflexCmdCreateFile( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	WORD fid, int flen, BYTE ftype, BYTE status, BYTE reclen, BYTE recnum,
	const BYTE *acl );

/* Use fid==0xFFFF to delete all files in a DF. */
int scCyberflexCmdDeleteFile( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	WORD fid );

int scCyberflexCmdDirectory( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	BYTE filenum, BYTE *resp, int *resplen );

int scCyberflexCmdExecuteMethod( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	BYTE type, const BYTE *data, BYTE datalen );

int scCyberflexCmdGetData( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	BYTE *resp, int *resplen );

int scCyberflexCmdGetFileACL( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	BYTE *resp, int *resplen );

/* sw is used to get the length of the available data. */
int scCyberflexCmdGetResp( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	BYTE *resp, int *resplen );

int scCyberflexCmdInvalidate( SC_READER_INFO *ri, SC_CARD_INFO *ci );

int scCyberflexCmdLogOutAll( SC_READER_INFO *ri, SC_CARD_INFO *ci );

int scCyberflexCmdManageInstance( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	BYTE op );

int scCyberflexCmdManageProgram( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	const BYTE *data, BYTE datalen );

int scCyberflexCmdReadBinary( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	int offset, BYTE *data, int *datalen );

int scCyberflexCmdReadRecord( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	BYTE recnum, BYTE mode, BYTE *data, int *datalen );

int scCyberflexCmdRehabilitate( SC_READER_INFO *ri, SC_CARD_INFO *ci );

int scCyberflexCmdSelect( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	BYTE type, WORD fid, const BYTE *aid, BYTE aidlen, BYTE *resp,
	int *resplen );

int scCyberflexCmdUnblockCHV( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	BYTE chvnum, const BYTE *unblock, const BYTE *newpin );

int scCyberflexCmdUpdateBinary( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	int offset, const BYTE *data, BYTE datalen );

int scCyberflexCmdUpdateRecord( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	BYTE recnum, BYTE mode, const BYTE *data, BYTE datalen );

int scCyberflexCmdVerifyCHV( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	BYTE chvnum, const BYTE *chv );

int scCyberflexCmdVerifyKey( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	BYTE keynum, const BYTE *key, BYTE keylen );

/****************************************************************************
*																			*
*							Cryptographic Commands							*
*																			*
****************************************************************************/

int scCyberflexCmdAskRandom( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	BYTE *chall, int *len );

int scCyberflexCmdExtAuth( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	BYTE keynum, const BYTE *data, BYTE datalen, BYTE algo );

int scCyberflexCmdExtAuthDES( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	BYTE keynum, const BYTE *key, const BYTE *chall, BYTE algo );

int scCyberflexCmdIntAuth( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	BYTE keynum, const BYTE *data, BYTE datalen, BYTE *resp, int *resplen,
	BYTE algo );

int scCyberflexCmdIntAuthDES( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	BYTE keynum, const BYTE *chall, const BYTE *key, BYTE algo );

/****************************************************************************
*																			*
*								GSM Commands								*
*																			*
****************************************************************************/

int scCyberflexCmdDisableCHV( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	const BYTE *chv );

int scCyberflexCmdEnableCHV( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	const BYTE *chv );

int scCyberflexCmdIncrease( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	const BYTE *amount, BYTE *resp, int *resplen );

int scCyberflexCmdSeek( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	BYTE mode, const BYTE *pattern, BYTE patlen, BYTE *resp );

int scCyberflexCmdSleep( SC_READER_INFO *ri, SC_CARD_INFO *ci );

int scCyberflexCmdStatus( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	BYTE *resp, int *resplen );

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* SC_CYBERFLEX_H */

