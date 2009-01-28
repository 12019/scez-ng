/****************************************************************************
*																			*
*					SCEZ chipcard library - MFC routines					*
*						Copyright Matthias Bruestle 1999					*
*					For licensing, see the file COPYRIGHT					*
*																			*
****************************************************************************/

/* $Id: scmfc.h 1119 2005-05-08 13:28:34Z laforge $ */

#ifndef SC_MFC_H
#define SC_MFC_H

#include <scez/scgeneral.h>

#define SC_MFC_AC_SIZE				5
#define SC_MFC_AMOUNT_SIZE			3
#define SC_MFC_BLOCK_SIZE			8
#define SC_MFC_PIN_SIZE				8

/* Usefull defines for command parameters */

/* Change password */

#define SC_MFC_CHV1					1
#define SC_MFC_CHV2					2

/* Create */

#define SC_MFC_STATUS_INVALID		0x01
#define SC_MFC_STATUS_READABLE		0x02
#define SC_MFC_STATUS_LOCKCHV		0x04
#define SC_MFC_STATUS_LOCKDF		0x20
#define SC_MFC_STATUS_LOCKASC		0x20

#define SC_MFC_TYPE_TRANS			0x00
#define SC_MFC_TYPE_FIXED			0x01
#define SC_MFC_TYPE_VARIABLE		0x02
#define SC_MFC_TYPE_CYCLIC			0x03
#define SC_MFC_TYPE_ASC				0x06
#define SC_MFC_TYPE_DF				0x10
#define SC_MFC_TYPE_DF_ASC			0x11

/* Ext Auth */

#define SC_MFC_ALGO_DES				0
#define SC_MFC_ALGO_3DES			1

/* Lock */

#define SC_MFC_SET_AC_READ			0x01
#define SC_MFC_SET_AC_UPDATE		0x02
#define SC_MFC_SET_AC_DELETE		0x04
#define SC_MFC_SET_AC_CREATE		0x08
#define SC_MFC_SET_AC_REHABILITATE	0x10
#define SC_MFC_SET_AC_INVALIDATE	0x20

/* Modify Baud Rate */

#define SC_MFC_DIV_93				0x80
#define SC_MFC_DIV_186				0x40
#define SC_MFC_DIV_372				0x00

/* Read/Update record */

#define SC_MFC_RECORD_FIRST			0x00
#define SC_MFC_RECORD_LAST			0x01
#define SC_MFC_RECORD_NEXT			0x02
#define SC_MFC_RECORD_PREV			0x03
#define SC_MFC_RECORD_ABS			0x04
#define SC_MFC_RECORD_CURR			0x05 /* 0x04 can also be used, but 0x05
										  * makes sure, that rec is 0.
										  */

/* Seek */

#define SC_MFC_SEEK_FORW_FIRST		0x00
#define SC_MFC_SEEK_BACKW_LAST		0x01
#define SC_MFC_SEEK_FORW_CURR		0x02
#define SC_MFC_SEEK_BACKW_CURR		0x03

/* Select */

#define	SC_MFC_FID_MF				0x3F00
#define	SC_MFC_FID_EF_CHV1			0x0000
#define	SC_MFC_FID_EF_CHV2			0x0100
#define	SC_MFC_FID_EF_KEY_MAN		0x0011
#define	SC_MFC_FID_EF_KEY			0x0001
#define	SC_MFC_FID_EF_ICC			0x0002
#define	SC_MFC_FID_EF_ID			0x0003
#define	SC_MFC_FID_EF_IC			0x0005
#define	SC_MFC_FID_EF_DIR			0x2F00
#define	SC_MFC_FID_EF_ATR			0x2F01
#define	SC_MFC_FID_EF_ASC			0x9F02
#define	SC_MFC_FID_EF_AUT			0x9F03
#define	SC_MFC_FID_EF_LOG			0x9F05
#define	SC_MFC_FID_EF_DIR_MFC		0xFD00
#define	SC_MFC_FID_EF_INFO			0xFD20
#define	SC_MFC_FID_EF_KEY_PERS		0xFD21

#define SC_MFC_SELECT_FID			0x00
#define SC_MFC_SELECT_PARENT		0x03
#define SC_MFC_SELECT_AID			0x04
#define SC_MFC_SELECT_ABS_PATH		0x08
#define SC_MFC_SELECT_REL_PATH		0x09

#define SC_MFC_RDATA_FCI			0x00
#define SC_MFC_RDATA_NONE			0x0C

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* Initialize card function pointer */
int scMfcInit( SC_CARD_INFO *ci );

#ifndef SWIG
/* Capabilities */
int scMfcGetCap( SC_CARD_INFO *ci, SC_CARD_CAP *cp );
#endif /* !SWIG */

/* Fill card data in ci */
int scMfcGetCardData( SC_CARD_INFO *ci );

#ifndef SWIG
/* Set F and D. */
int scMfcSetFD( SC_READER_INFO *ri, SC_CARD_INFO *ci, LONG fd );
#endif /* !SWIG */

/* Generate Auth */
void scMfcGenerateAuth( const BYTE *key, const BYTE *chall, BYTE *auth,
	BYTE algo );

/* Compare Auth */
BOOLEAN scMfcCompareAuth( const BYTE *key, const BYTE *chall, const BYTE *auth,
	BYTE algo );

/* Uses sfid if it is in the correct range. */
int scMfcCmdAppendRecord( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	BYTE sfid, const BYTE *data, BYTE datalen );

int scMfcCmdCardBlock( SC_READER_INFO *ri, SC_CARD_INFO *ci );

int scMfcCmdChangeCHV( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	BYTE num, const BYTE *oldpin, const BYTE *newpin );

int scMfcCmdCloseAppl( SC_READER_INFO *ri, SC_CARD_INFO *ci );

/* TODO: Do protection key byte.  */
int scMfcCmdCreateFile( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	WORD fid, WORD flen, BYTE ftype, const BYTE *ac, BYTE status, BYTE recprot,
	const BYTE *data, BYTE datalen );

int scMfcCmdDecrease( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	BYTE sfid, const BYTE *amount, BYTE *resp, int *resplen );

int scMfcCmdDecreaseStamp( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	BYTE sfid, const BYTE *amount, BYTE *resp, int *resplen );

int scMfcCmdDelete( SC_READER_INFO *ri, SC_CARD_INFO *ci, WORD fid );

int scMfcCmdExtAuth( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	BYTE keynum, const BYTE *key, const BYTE *chall, BYTE algo ); 

int scMfcCmdGetChall( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	BYTE *rand, int *len );

int scMfcCmdGiveRand( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	const BYTE *rand ); 

int scMfcCmdIncrease( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	BYTE sfid, const BYTE *amount, BYTE *resp, int *resplen );

int scMfcCmdIncreaseStamp( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	BYTE sfid, const BYTE *amount, BYTE *resp, int *resplen );

int scMfcCmdIntAuth( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	BYTE keynum, const BYTE *chall, const BYTE *key, BYTE algo );

int scMfcCmdInvalidate( SC_READER_INFO *ri, SC_CARD_INFO *ci );

/* TODO: Do something better. */
int scMfcCmdLoadKeyFile( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	const BYTE *data, BYTE datalen );

int scMfcCmdLock( SC_READER_INFO *ri, SC_CARD_INFO *ci, BYTE set,
	BYTE ac, WORD fid );

int scMfcCmdModBaudRate( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	BYTE div );

/* offset can be sfid/soffset (highest bit set); */
int scMfcCmdReadBinary( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	WORD offset, BYTE *data, int *datalen );

/* TODO: Stamp check. */
/* offset can be sfid/soffset (highest bit set); */
int scMfcCmdReadBinStamp( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	WORD offset, BYTE *data, int *datalen );

/* Uses sfid if it is in the correct range. */
int scMfcCmdReadRecord( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	BYTE rec, BYTE addr, BYTE sfid, BYTE *data, int *datalen );

/* TODO: Stamp check. */
/* Uses sfid if it is in the correct range. */
int scMfcCmdReadRecStamp( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	BYTE rec, BYTE addr, BYTE sfid, BYTE *data, int *datalen );

int scMfcCmdReadStat( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	BYTE *data, int *datalen );

int scMfcCmdRehabilitate( SC_READER_INFO *ri, SC_CARD_INFO *ci );

int scMfcCmdSeek( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	BYTE offset, BYTE start, const BYTE *data, BYTE datalen, BYTE *recnum );

int scMfcCmdSelect( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	BYTE type, WORD fid, const BYTE *aidpath, BYTE aidpathlen, BYTE *resp,
	int *resplen );

int scMfcCmdSetExec( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	BOOLEAN exec, WORD fid );

int scMfcCmdUnblockCHV( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	BYTE num, const BYTE *unblock, const BYTE *newpin );

/* offset can be sfid/soffset (highest bit set); */
int scMfcCmdUpdateBinary( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	WORD offset, const BYTE *data, BYTE datalen );

/* Uses sfid if it is in the correct range. */
int scMfcCmdUpdateRecord( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	BYTE rec, BYTE addr, BYTE sfid, const BYTE *data, BYTE datalen );

int scMfcCmdVerifyCHV( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	BYTE num, const BYTE *pin );

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* SC_MFC_H */


