/****************************************************************************
*																			*
*					SCEZ chipcard library - Sm@rtCafe routines				*
*						Copyright Matthias Bruestle 1999					*
*					For licensing, see the file COPYRIGHT					*
*																			*
****************************************************************************/

/* $Id: scsmartcafe.h 1119 2005-05-08 13:28:34Z laforge $ */

#ifndef SC_SMARTCAFE_H
#define SC_SMARTCAFE_H

#include <scez/scgeneral.h>

#define SC_SMARTCAFE_MAX_AIDLEN		16
#define SC_SMARTCAFE_MAX_KEYLEN		16
#define SC_SMARTCAFE_MAX_PINLEN		8

#define SC_SMARTCAFE_ALGO_NONE		0
#define SC_SMARTCAFE_ALGO_DES		1
#define SC_SMARTCAFE_ALGO_3DES		2

/* Usefull defines for command parameters */

#define SC_SMARTCAFE_SPL_SIGN		0x01
#define SC_SMARTCAFE_SPL_CRYPT		0x02

#define SC_SMARTCAFE_SP_INSTALL		0x00
#define SC_SMARTCAFE_SP_LOAD		0x01
#define SC_SMARTCAFE_SP_DELETE		0x02
#define SC_SMARTCAFE_SP_CLEAR		0x03
#define SC_SMARTCAFE_SP_PUTKEY		0x04
#define SC_SMARTCAFE_SP_SETPIN		0x05

#define SC_SMARTCAFE_ACCESS_NEV		0x00
#define SC_SMARTCAFE_ACCESS_PIN		0x03
#define SC_SMARTCAFE_ACCESS_ALW		0xFF

#define SC_SMARTCAFE_TAG_VERSION	0xC2
#define SC_SMARTCAFE_TAG_SERIAL		0xC3

#define SC_SMARTCAFE_INSTALL_LOAD		0x02
#define SC_SMARTCAFE_INSTALL_INST		0x04
#define SC_SMARTCAFE_INSTALL_INSTHEAP	0x84

#define SC_SMARTCAFE_KEYIDX_CRYPT	0x01
#define SC_SMARTCAFE_KEYIDX_SIGN	0x02

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* Initialize card function pointer */
int scSmartcafeInit( SC_CARD_INFO *ci );

/* Fill card data in ci */
int scSmartcafeGetCardData( SC_CARD_INFO *ci );

/* The length of enckey and sigkey depends on the algorithm.
 * For singing the size of data must be 8 more than the size of the applet to
 * have enough space for the MAC.
 */
int scSmartcafeAuthApplet( const BYTE *enckey, BYTE encalgo,
	const BYTE *sigkey, BYTE sigalgo, BYTE *data, int *datalen );

int scSmartcafeCmdGetResp( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	BYTE *resp, int *resplen );

int scSmartcafeCmdClearMem( SC_READER_INFO *ri, SC_CARD_INFO *ci );

/* sp contains sp1|sp2. */
int scSmartcafeCmdCreateML( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	BOOLEAN start, BYTE maclen, BYTE spl, const BYTE *sp );

int scSmartcafeCmdDeleteML( SC_READER_INFO *ri, SC_CARD_INFO *ci );

int scSmartcafeCmdGetData( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	BYTE tag, BYTE *data, int *datalen );

int scSmartcafeCmdInstall( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	BYTE rcp, BYTE spl, const BYTE *aid, BYTE aidlen, const BYTE *param,
	BYTE paramlen, WORD heap );

int scSmartcafeCmdLoadApplet( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	BOOLEAN last, BYTE blknum, const BYTE *data, BYTE datalen );

int scSmartcafeCmdPutKey( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	BYTE keyidx, const BYTE *key, BYTE keylen );

int scSmartcafeCmdSelect( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	const BYTE *aid, BYTE aidlen, BYTE *resp, int *resplen );

int scSmartcafeCmdSetPIN( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	const BYTE *pin, BYTE pinlen );

int scSmartcafeCmdVerifyPIN( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	const BYTE *pin, BYTE pinlen );

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* SC_SMARTCAFE_H */

