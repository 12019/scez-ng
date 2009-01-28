/****************************************************************************
*																			*
*					SCEZ chipcard library - Geldkarte routines				*
*						Copyright Matthias Bruestle 1999					*
*					For licensing, see the file COPYRIGHT					*
*																			*
****************************************************************************/

/* $Id: scgeldkarte.c 1617 2005-11-03 17:41:39Z laforge $ */

#include <scez/scinternal.h>
#include <scez/cards/scgeldkarte.h>
#include <scez/sct1.h>

#include <stdio.h>
#include <string.h>

/* Initialize card function pointer */

int scGeldkarteInit( SC_CARD_INFO *ci )
{
	ci->scGetCardData=scGeldkarteGetCardData;

	return( SC_EXIT_OK );
}

/* Fill card data in ci */

int scGeldkarteGetCardData( SC_CARD_INFO *ci )
{
	BYTE swok[] = { 0x02, 0x90, 0x61 };
	BYTE swav[] = { 0x00 };

	ci->protocol=SC_PROTOCOL_T1;
	ci->direct=TRUE;
	ci->t1.ifsc=80;
	ci->t1.cwt=32;
	ci->t1.bwt=1600000;
	ci->t1.rc=SC_T1_CHECKSUM_LRC;
	ci->t1.ns=0;
	ci->t1.nr=0;
	memcpy( ci->swok, swok, sizeof(swok) );
	memcpy( ci->swav, swav, sizeof(swav) );
	ci->memsize=0;

	return( SC_EXIT_OK );
}

int scGeldkarteCmdGetChall( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	BYTE *chall )
{
	BYTE cmd[]={ 0x00, 0x84, 0x00, 0x00, 0x08 };
	BYTE rsp[ SC_GENERAL_SHORT_DATA_SIZE+2 ];
	SC_APDU apdu;
	int ret;

	apdu.cse=SC_APDU_CASE_2_SHORT;
	apdu.cmd=cmd;
	apdu.cmdlen=5;
	apdu.rsp=rsp;
	apdu.rsplen=0;

	ret=scReaderSendAPDU( ri, ci, &apdu );
	memset( cmd, 0, sizeof(cmd) );
	if( ret!=SC_EXIT_OK ) return( ret );

	if( apdu.rsplen<2 ) return( SC_EXIT_BAD_SW );

	ci->sw[0] = apdu.rsp[apdu.rsplen-2];
	ci->sw[1] = apdu.rsp[apdu.rsplen-1];

	if( apdu.rsplen==2 ) return( SC_EXIT_OK );

	if( apdu.rsplen==10 ) memcpy( chall, apdu.rsp, SC_GELDKARTE_CHALL_SIZE );
	else {
		memset( rsp, 0, sizeof(rsp) );
		return( SC_EXIT_UNKNOWN_ERROR );
	}

	memset( rsp, 0, sizeof(rsp) );

	return( SC_EXIT_OK );
}

int scGeldkarteCmdGetStat( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	BYTE *resp, int *resplen )
{
	BYTE cmd[]={ 0xA0, 0x40, 0x00, 0x00, 0x40 };
	BYTE rsp[ SC_GENERAL_SHORT_DATA_SIZE+2 ];
	SC_APDU apdu;
	int ret;

	if( ci->type==SC_CARD_GELDKARTE_3 ) return( SC_EXIT_NOT_SUPPORTED );

	apdu.cse=SC_APDU_CASE_2_SHORT;
	apdu.cmd=cmd;
	apdu.cmdlen=5;
	apdu.rsp=rsp;
	apdu.rsplen=0;

	ret=scReaderSendAPDU( ri, ci, &apdu );
	memset( cmd, 0, sizeof(cmd) );
	if( ret!=SC_EXIT_OK ) return( ret );

	if( apdu.rsplen<2 ) return( SC_EXIT_BAD_SW );

	ci->sw[0] = apdu.rsp[apdu.rsplen-2];
	ci->sw[1] = apdu.rsp[apdu.rsplen-1];

	if( apdu.rsplen==2 ) return( SC_EXIT_OK );

	apdu.rsplen-=2;

	memcpy( resp, apdu.rsp, apdu.rsplen );
	*resplen=apdu.rsplen;

	return( SC_EXIT_OK );
}

int scGeldkarteCmdReadRecord( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	BYTE recnum, BYTE mode, BYTE sfid, BYTE *data, int *datalen )
{
	BYTE cmd[]={ 0x00, 0xB2, 0x00, 0x00, 0x00 };
	BYTE rsp[ SC_GENERAL_SHORT_DATA_SIZE+2 ];
	SC_APDU apdu;
	int ret;

	ci->sw[0]=0x00; ci->sw[1]=0x00;

	if( *datalen>256 ) return( SC_EXIT_BAD_PARAM );

	apdu.cse=SC_APDU_CASE_2_SHORT;
	apdu.cmd=cmd;
	apdu.cmdlen=5;
	apdu.rsp=rsp;
	apdu.rsplen=0;

	*datalen &= 0xFF;

	cmd[2] = recnum;

	if( mode==SC_GELDKARTE_READREC_SELECTED )
		cmd[3] = 0x04;
	else if( mode==SC_GELDKARTE_READREC_SFID )
		cmd[3] = ((sfid << 3) | 0x04) &0xFF;
	else
		return( SC_EXIT_BAD_PARAM );

	cmd[4] = (*datalen) & 0xFF;

	ret=scReaderSendAPDU( ri, ci, &apdu );
	memset( cmd, 0, sizeof(cmd) );
	if( ret!=SC_EXIT_OK ) return( ret );

	if( apdu.rsplen<2 ) return( SC_EXIT_BAD_SW );

	ci->sw[0] = apdu.rsp[apdu.rsplen-2];
	ci->sw[1] = apdu.rsp[apdu.rsplen-1];

	apdu.rsplen-=2;
	memcpy( data, apdu.rsp, apdu.rsplen );
	*datalen=apdu.rsplen;

	memset( rsp, 0, sizeof(rsp) );

	return( SC_EXIT_OK );
}

int scGeldkarteCmdSelectFile( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	BYTE selmode, WORD fid, const BYTE *aid, BYTE aidlen, BYTE respmode,
	BYTE *resp, int *resplen )
{
	BYTE cmd[ 21 ];
	BYTE rsp[ SC_GENERAL_SHORT_DATA_SIZE+2 ];
	SC_APDU apdu;
	int ret;

	ci->sw[0]=0x00; ci->sw[1]=0x00;

	apdu.cmd=cmd;
	apdu.rsp=rsp;
	apdu.rsplen=0;

	memset( cmd, 0, sizeof(cmd) );

	cmd[1]=0xA4;
	cmd[2]=selmode;
	cmd[3]=respmode;

	apdu.cse=SC_APDU_CASE_3_SHORT;

	if( selmode==SC_GELDKARTE_SELECT_MF ) {
		apdu.cmdlen=4;
		apdu.cse=SC_APDU_CASE_1;
	} else if( (selmode==SC_GELDKARTE_SELECT_EF) ||
		(selmode==SC_GELDKARTE_SELECT_DF) ) {
		apdu.cmdlen=7;
		cmd[4]=0x02;
		cmd[5]=(fid>>8) &0xFF;
		cmd[6]=fid & 0xFF;
	} else if( selmode==SC_GELDKARTE_SELECT_AID ) {
		if( (aidlen<5) || (aidlen>15) ) return( SC_EXIT_BAD_PARAM );
		apdu.cmdlen=5+aidlen;
		cmd[4]=aidlen;
		memcpy( cmd+5, aid, aidlen );
	} else {
		return( SC_EXIT_BAD_PARAM );
	}

	if( (respmode==SC_GELDKARTE_SELRESP_FCI) ||
		(respmode==SC_GELDKARTE_SELRESP_FCP) ||
		(respmode==SC_GELDKARTE_SELRESP_FMD) ) {
		apdu.cmdlen++;
		apdu.cse=SC_APDU_CASE_4_SHORT;
		if( apdu.cmdlen==5 ) apdu.cse=SC_APDU_CASE_2_SHORT;
	} else if( respmode!=SC_GELDKARTE_SELRESP_NONE ) {
		return( SC_EXIT_BAD_PARAM );
	}

	ret=scReaderSendAPDU( ri, ci, &apdu );
	memset( cmd, 0, sizeof(cmd) );
	if( ret!=SC_EXIT_OK ) return( ret );

	if( apdu.rsplen<2 ) return( SC_EXIT_BAD_SW );

	ci->sw[0] = apdu.rsp[apdu.rsplen-2];
	ci->sw[1] = apdu.rsp[apdu.rsplen-1];

	apdu.rsplen-=2;
	memcpy( resp, apdu.rsp, apdu.rsplen );
	*resplen=apdu.rsplen;

	memset( rsp, 0, sizeof(rsp) );

	return( SC_EXIT_OK );
}

int scGeldkarteCmdVerifyPIN( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	BOOLEAN local, BYTE pinnum, BYTE *pin, int pinlen )
{
	BYTE cmd[ 5+SC_GENERAL_SHORT_DATA_SIZE ];
	BYTE rsp[ SC_GENERAL_SHORT_DATA_SIZE+2 ];
	SC_APDU apdu;
	int ret;

	if( (pin==NULL) || (pinlen>SC_GENERAL_SHORT_DATA_SIZE) || (pinnum>1) )
		return( SC_EXIT_BAD_PARAM );

	apdu.cse=SC_APDU_CASE_3_SHORT;
	apdu.cmd=cmd;
	apdu.cmdlen=5+pinlen;
	apdu.rsp=rsp;
	apdu.rsplen=0;

	memset( cmd, 0, sizeof(cmd) );

	cmd[1]=0x20;
	cmd[3]=(local?0x80:0x00)+pinnum;
	cmd[4]=pinlen;

	memcpy( cmd+5, pin, pinlen );

	ret=scReaderSendAPDU( ri, ci, &apdu );
	memset( cmd, 0, sizeof(cmd) );
	if( ret!=SC_EXIT_OK ) return( ret );

	if( apdu.rsplen<2 ) return( SC_EXIT_BAD_SW );

	ci->sw[0] = apdu.rsp[apdu.rsplen-2];
	ci->sw[1] = apdu.rsp[apdu.rsplen-1];

	return( SC_EXIT_OK );
}


