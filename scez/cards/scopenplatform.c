/****************************************************************************
*																			*
*				SCEZ chipcard library - OpenPlatform routines				*
*						Copyright Matthias Bruestle 2001					*
*					For licensing, see the file COPYRIGHT					*
*																			*
****************************************************************************/

/* $Id: scopenplatform.c 1617 2005-11-03 17:41:39Z laforge $ */

#include <scez/scinternal.h>
#include <scez/scmacros.h>
#include <scez/cards/scopenplatform.h>

#include <stdio.h>
#include <string.h>

/* TODO: PIN Change, Put Data */

/* TODO: Cryptoversions of commands with DAP/MAC */

#if 0 /* These are only support functions for other cards */
/* Initialize card function pointer */

int scOpenplatformInit( SC_CARD_INFO *ci )
{
	ci->scGetCardData=scOpenplatformGetCardData;

	return( SC_EXIT_OK );
}

/* Fill card data in ci */

int scOpenplatformGetCardData( SC_CARD_INFO *ci )
{
	BYTE swok[] = { 0x02, 0x90, 0x61 };
	BYTE swav[] = { 0x00 };
	int ret;

	if( (ret=scSmartcardProcessATR( ci ))!=SC_EXIT_OK ) return( ret );

	memcpy( ci->swok, swok, sizeof(swok) );
	memcpy( ci->swav, swav, sizeof(swav) );
	ci->memsize=0;

	return( SC_EXIT_OK );
}
#endif

#define SC_OPENPLATFORM_DELETE_LAST		0
#define SC_OPENPLATFORM_DELETE_MORE		1

int scOpenplatformCmdDelete( SC_DRV_STATE_DECL, BYTE refcontr,
	const BYTE *oid, BYTE oidlen )
{
	SC_DRV_VARS( 5+SC_GENERAL_SHORT_DATA_SIZE+1, 2 );

	SC_DRV_PRE();
	if( oid==NULL ) return( SC_EXIT_BAD_PARAM );

	SC_DRV_INITAPDU( SC_APDU_CASE_4_SHORT, 5+oidlen+1 );
	SC_DRV_SETHEADER( 0x80, 0xE4, refcontr, 0x00 );

	cmd[4] = oidlen;
	memcpy( cmd+5, oid, oidlen );

	SC_DRV_SENDAPDU();
	SC_DRV_PROCSW();

	return( SC_EXIT_OK );
}

int scOpenplatformCmdGetData( SC_DRV_STATE_DECL, WORD tag, BYTE *resp,
	int *resplen )
{
	SC_DRV_VARS( 5, SC_GENERAL_SHORT_DATA_SIZE+2 );

	SC_DRV_PRE();
	if( (resp==NULL) || (resplen==NULL) ) return( SC_EXIT_BAD_PARAM );

	SC_DRV_INITAPDU( SC_APDU_CASE_2_SHORT, 5 );
	SC_DRV_SETHEADER( 0x80, 0xCA, tag>>8, tag&0xFF );

	SC_DRV_SENDAPDU();
	SC_DRV_PROCSW();

	memcpy( resp, rsp, apdu.rsplen );
	*resplen = apdu.rsplen;

	SC_DRV_CLRRSP();

	return( SC_EXIT_OK );
}

#define SC_OPENPLATFORM_STATUS_IND_LOADFILE	0x20
#define SC_OPENPLATFORM_STATUS_IND_APP		0x40
#define SC_OPENPLATFORM_STATUS_IND_CARDMAN	0x80

int scOpenplatformCmdGetStatus( SC_DRV_STATE_DECL, BYTE ind, BOOLEAN next,
	const BYTE *data, BYTE datalen, BYTE* resp, int *resplen )
{
	SC_DRV_VARS( 5+SC_GENERAL_SHORT_DATA_SIZE+1, SC_GENERAL_SHORT_DATA_SIZE+2 );

	SC_DRV_PRE();
	if( (data==NULL) || (resp==NULL) || (resplen==NULL) )
		return( SC_EXIT_BAD_PARAM );

	SC_DRV_INITAPDU( SC_APDU_CASE_4_SHORT, 5+datalen+1 );
	SC_DRV_SETHEADER( 0x80, 0xF2, ind, next?0x01:0x00 );

	cmd[4] = datalen;
	memcpy( cmd+5, data, datalen );

	SC_DRV_SENDAPDU();
	SC_DRV_PROCSW();

	memcpy( resp, rsp, apdu.rsplen );
	*resplen = apdu.rsplen;

	SC_DRV_CLRRSP();

	return( SC_EXIT_OK );
}

#define SC_OPENPLATFORM_INSTALL_FOR_LOAD		0x02
#define SC_OPENPLATFORM_INSTALL_FOR_INSTALL		0x04
#define SC_OPENPLATFORM_INSTALL_FOR_SELECTABLE	0x08

int scOpenplatformCmdInstall( SC_DRV_STATE_DECL, BYTE instfor, const BYTE *data,
	BYTE datalen )
{
	SC_DRV_VARS( 5+SC_GENERAL_SHORT_DATA_SIZE+1, SC_GENERAL_SHORT_DATA_SIZE+2 );

	SC_DRV_PRE();
	if( data==NULL ) return( SC_EXIT_BAD_PARAM );

	SC_DRV_INITAPDU( SC_APDU_CASE_4_SHORT, 5+datalen+1 );
	SC_DRV_SETHEADER( 0x80, 0xE6, instfor, 0x00 );

	cmd[4] = datalen;
	memcpy( cmd+5, data, datalen );

	SC_DRV_SENDAPDU();
	SC_DRV_PROCSW();

	return( SC_EXIT_OK );
}

int scOpenplatformCmdLoad( SC_DRV_STATE_DECL, BOOLEAN more, BYTE blocknum,
	const BYTE *data, BYTE datalen )
{
	SC_DRV_VARS( 5+SC_GENERAL_SHORT_DATA_SIZE+1, SC_GENERAL_SHORT_DATA_SIZE+2 );

	SC_DRV_PRE();
	if( data==NULL ) return( SC_EXIT_BAD_PARAM );

	SC_DRV_INITAPDU( SC_APDU_CASE_4_SHORT, 5+datalen+1 );
	SC_DRV_SETHEADER( 0x80, 0xE8, more ? 0x00 : 0x80, blocknum );

	cmd[4] = datalen;
	memcpy( cmd+5, data, datalen );

	SC_DRV_SENDAPDU();
	SC_DRV_PROCSW();

	return( SC_EXIT_OK );
}

int scOpenplatformCmdPutKey( SC_DRV_STATE_DECL, BOOLEAN more, BOOLEAN mult,
	BYTE keysetver, BYTE keyidx, const BYTE *data, BYTE datalen, BYTE* resp,
	int *resplen )
{
	SC_DRV_VARS( 5+SC_GENERAL_SHORT_DATA_SIZE+1, SC_GENERAL_SHORT_DATA_SIZE+2 );

	SC_DRV_PRE();
	if( (data==NULL) || (resp==NULL) || (resplen==NULL) )
		return( SC_EXIT_BAD_PARAM );

	SC_DRV_INITAPDU( SC_APDU_CASE_4_SHORT, 5+datalen+1 );
	SC_DRV_SETHEADER( 0x80, 0xD8, (more?0x80:0x00)|(keysetver&0x7F),
		(mult?0x80:0x00)|(keyidx&0x7F) );

	cmd[4] = datalen;
	memcpy( cmd+5, data, datalen );

	SC_DRV_SENDAPDU();
	SC_DRV_PROCSW();

	memcpy( resp, rsp, apdu.rsplen );
	*resplen = apdu.rsplen;

	SC_DRV_CLRRSP();

	return( SC_EXIT_OK );
}

int scOpenplatformCmdSelect( SC_DRV_STATE_DECL, BOOLEAN byaid, BOOLEAN next,
	const BYTE *data, BYTE datalen, BYTE* resp, int *resplen )
{
	SC_DRV_VARS( 5+SC_GENERAL_SHORT_DATA_SIZE+1, SC_GENERAL_SHORT_DATA_SIZE+2 );

	SC_DRV_PRE();
	if( (data==NULL) || (resp==NULL) || (resplen==NULL) )
		return( SC_EXIT_BAD_PARAM );

	SC_DRV_INITAPDU( SC_APDU_CASE_4_SHORT, 5+datalen+1 );
	SC_DRV_SETHEADER( 0x00, 0xA4, byaid?0x04:0x00, next?0x02:0x00 );

	cmd[4] = datalen;
	memcpy( cmd+5, data, datalen );

	SC_DRV_SENDAPDU();
	SC_DRV_PROCSW();

	memcpy( resp, rsp, apdu.rsplen );
	*resplen = apdu.rsplen;

	SC_DRV_CLRRSP();

	return( SC_EXIT_OK );
}

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

int scOpenplatformCmdSetStatus( SC_DRV_STATE_DECL, BYTE type, BYTE status,
	const BYTE *aid, BYTE aidlen )
{
	SC_DRV_VARS( 5+SC_GENERAL_SHORT_DATA_SIZE+1, SC_GENERAL_SHORT_DATA_SIZE+2 );

	SC_DRV_PRE();
	if( aid==NULL ) return( SC_EXIT_BAD_PARAM );

	SC_DRV_INITAPDU( SC_APDU_CASE_3_SHORT, 5+aidlen );
	SC_DRV_SETHEADER( 0x80, 0xF0, type, status );

	cmd[4] = aidlen;
	memcpy( cmd+5, aid, aidlen );

	SC_DRV_SENDAPDU();
	SC_DRV_PROCSW();

	return( SC_EXIT_OK );
}

/* TODO: Crypto */

int scOpenplatformCmdInitUpdate( SC_DRV_STATE_DECL, BYTE keysetver, BYTE keyidx,
	const BYTE *chall, BYTE challlen, BYTE* resp, int *resplen )
{
	SC_DRV_VARS( 5+8+1, SC_GENERAL_SHORT_DATA_SIZE+2 );

	SC_DRV_PRE();
	if( (chall==NULL) || (challlen!=8) || (resp==NULL) || (resplen==NULL) )
		return( SC_EXIT_BAD_PARAM );

	SC_DRV_INITAPDU( SC_APDU_CASE_4_SHORT, 5+8+1 );
	SC_DRV_SETHEADER( 0x80, 0x50, keysetver, keyidx );

	cmd[4] = challlen;
	memcpy( cmd+5, chall, challlen );

	SC_DRV_SENDAPDU();
	SC_DRV_PROCSW();

	memcpy( resp, rsp, apdu.rsplen );
	*resplen = apdu.rsplen;

	SC_DRV_CLRRSP();

	return( SC_EXIT_OK );
}

#define SC_OPENPLATFORM_SECLEV_NONE		0x00
#define SC_OPENPLATFORM_SECLEV_MAC		0x01
#define SC_OPENPLATFORM_SECLEV_ENC_MAC	0x03

/* TODO: Crypto */

int scOpenplatformCmdExtAuth( SC_DRV_STATE_DECL, BYTE seclev,
	const BYTE *data, BYTE datalen )
{
	SC_DRV_VARS( 5+SC_GENERAL_SHORT_DATA_SIZE+1, SC_GENERAL_SHORT_DATA_SIZE+2 );

	SC_DRV_PRE();
	if( data==NULL ) return( SC_EXIT_BAD_PARAM );

	SC_DRV_INITAPDU( SC_APDU_CASE_3_SHORT, 5+datalen );
	SC_DRV_SETHEADER( 0x84, 0x82, seclev, 0x00 );

	cmd[4] = datalen;
	memcpy( cmd+5, data, datalen );

	SC_DRV_SENDAPDU();
	SC_DRV_PROCSW();

	return( SC_EXIT_OK );
}



