/****************************************************************************
*																			*
*				SCEZ chipcard library - OpenPlatform routines				*
*						Copyright Matthias Bruestle 2000					*
*					For licensing, see the file COPYRIGHT					*
*																			*
****************************************************************************/

/* $Id: scopenplatform.h 1119 2005-05-08 13:28:34Z laforge $ */

#ifndef SC_OPENPLATFORM_H
#define SC_OPENPLATFORM_H

#include <scez/scgeneral.h>

#define SC_OPENPLATFORM_DELETE_LAST		0
#define SC_OPENPLATFORM_DELETE_MORE		1

#define SC_OPENPLATFORM_STATUS_IND_LOADFILE	0x20
#define SC_OPENPLATFORM_STATUS_IND_APP		0x40
#define SC_OPENPLATFORM_STATUS_IND_CARDMAN	0x80

#define SC_OPENPLATFORM_INSTALL_FOR_LOAD		0x02
#define SC_OPENPLATFORM_INSTALL_FOR_INSTALL		0x04
#define SC_OPENPLATFORM_INSTALL_FOR_SELECTABLE	0x08

#define SC_OPENPLATFORM_SET_STATUS_APPL		0x40
#define SC_OPENPLATFORM_SET_STATUS_CARDMAN	0x80

#define SC_OPENPLATFORM_LC_STATUS_LOADFILE_DELETED	0x00	
#define SC_OPENPLATFORM_LC_STATUS_LOADFILE_LOADED	0x01

#define SC_OPENPLATFORM_LC_STATUS_APPL_DELETED		0x00
#define SC_OPENPLATFORM_LC_STATUS_APPL_INSTALLED	0x03
#define SC_OPENPLATFORM_LC_STATUS_APPL_SELECTABLE	0x07
#define SC_OPENPLATFORM_LC_STATUS_APPL_PERSONALIZED	0x0F
#define SC_OPENPLATFORM_LC_STATUS_APPL_BLOCKED		0x7F
#define SC_OPENPLATFORM_LC_STATUS_APPL_LOCKED		0xFF

#define SC_OPENPLATFORM_LC_STATUS_CARDMAN_OP_READY		0x01
#define SC_OPENPLATFORM_LC_STATUS_CARDMAN_INITIALIZED	0x07
#define SC_OPENPLATFORM_LC_STATUS_CARDMAN_SECURED		0x0F
#define SC_OPENPLATFORM_LC_STATUS_CARDMAN_CMLOCKED		0x7F
#define SC_OPENPLATFORM_LC_STATUS_CARDMAN_TERMINATED	0xFF

#define SC_OPENPLATFORM_SECLEV_NONE		0x00
#define SC_OPENPLATFORM_SECLEV_MAC		0x01
#define SC_OPENPLATFORM_SECLEV_ENC_MAC	0x03

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* TODO:
 * Missing:
 *  - Pin Change Unblock (VOP)
 *  - Put Data (OP)
 */

int scOpenplatformCmdDelete( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	BYTE refcontr, const BYTE *oid, BYTE oidlen );

int scOpenplatformCmdGetData( SC_READER_INFO *ri, SC_CARD_INFO *ci, WORD tag,
	BYTE *resp, int *resplen );

int scOpenplatformCmdGetStatus( SC_READER_INFO *ri, SC_CARD_INFO *ci, BYTE ind,
	BOOLEAN next, const BYTE *data, BYTE datalen, BYTE* resp, int *resplen );

int scOpenplatformCmdInstall( SC_READER_INFO *ri, SC_CARD_INFO *ci, BYTE instfor,
	const BYTE *data, BYTE datalen );

int scOpenplatformCmdLoad( SC_READER_INFO *ri, SC_CARD_INFO *ci, BOOLEAN more,
	BYTE blocknum, const BYTE *data, BYTE datalen );

int scOpenplatformCmdPutKey( SC_READER_INFO *ri, SC_CARD_INFO *ci, BOOLEAN more,
	BOOLEAN mult, BYTE keysetver, BYTE keyidx, const BYTE *data, BYTE datalen,
	BYTE* resp, int *resplen );

int scOpenplatformCmdSelect( SC_READER_INFO *ri, SC_CARD_INFO *ci, BOOLEAN byaid,
	BOOLEAN next, const BYTE *data, BYTE datalen, BYTE* resp, int *resplen );

int scOpenplatformCmdSetStatus( SC_READER_INFO *ri, SC_CARD_INFO *ci, BYTE type,
	BYTE status, const BYTE *aid, BYTE aidlen );

int scOpenplatformCmdInitUpdate( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	BYTE keysetver, BYTE keyidx, const BYTE *chall, BYTE challlen, BYTE* resp,
	int *resplen );

int scOpenplatformCmdExtAuth( SC_READER_INFO *ri, SC_CARD_INFO *ci, BYTE seclev,
	const BYTE *data, BYTE datalen );

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* SC_OPENPLATFORM_H */




