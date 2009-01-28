/****************************************************************************
*																			*
*				SCEZ chipcard library - Macros for card drivers				*
*						Copyright Matthias Bruestle 2001					*
*					For licensing, see the file COPYRIGHT					*
*																			*
****************************************************************************/

/* $Id: scmacros.h 1005 2001-02-13 10:41:52Z zwiebeltu $ */

#ifndef SC_MACROS_H
#define SC_MACROS_H

#define SC_DRV_CLRCMD() \
	memset( cmd, 0, sizeof( cmd ) );
#define SC_DRV_CLRRSP() \
	memset( rsp, 0, sizeof( rsp ) );
#define SC_DRV_STATE \
	ri, ci
#define SC_DRV_STATE_DECL \
	SC_READER_INFO *ri, SC_CARD_INFO *ci
#define SC_DRV_VARS(cmdsize,rspsize) \
	BYTE cmd[cmdsize], rsp[ SC_GENERAL_SHORT_DATA_SIZE+2 ]; \
	SC_APDU apdu; \
	int ret;
#define SC_DRV_PRE() \
	ci->sw[0]=0x00; ci->sw[1]=0x00;
#define SC_DRV_INITAPDU(case,cmdlength) \
	apdu.cse=case; \
	apdu.cmd=cmd; \
	apdu.cmdlen=cmdlength; \
	apdu.rsp=rsp; \
	apdu.rsplen=0; \
	SC_DRV_CLRCMD(); \
	SC_DRV_CLRRSP();
#define SC_DRV_SETHEADER(cla,ins,p1,p2) \
	cmd[0]=cla; \
	cmd[1]=ins; \
	cmd[2]=p1; \
	cmd[3]=p2;
#if 0
#define SC_DRV_SETCMDLEN(len) \
	apdu.cmdlen=len;
#define SC_DRV_GETCMDLEN() \
	apdu.cmdlen
#define SC_DRV_SETRSPLEN(len) \
	apdu.rsplen=len;
#define SC_DRV_GETRSPLEN() \
	apdu.rsplen
#endif
#define SC_DRV_SENDAPDU() \
	ret=scReaderSendAPDU( ri, ci, &apdu ); \
	SC_DRV_CLRCMD(); \
	if( ret!=SC_EXIT_OK ) { \
		SC_DRV_CLRRSP(); \
		return( ret ); \
	}
#define SC_DRV_PROCSW() \
	if( apdu.rsplen<2 ) return( SC_EXIT_BAD_SW ); \
	ci->sw[0] = apdu.rsp[apdu.rsplen-2]; \
	ci->sw[1] = apdu.rsp[apdu.rsplen-1]; \
	apdu.rsplen-=2;

#endif /* SC_MACROS_H */

