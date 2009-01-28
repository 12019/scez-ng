/****************************************************************************
*																			*
*					SCEZ chipcard library - Payflex routines				*
*						Copyright Matthias Bruestle 2000					*
*					For licensing, see the file COPYRIGHT					*
*																			*
****************************************************************************/

/* $Id: scpayflex.c 1617 2005-11-03 17:41:39Z laforge $ */

#include <scez/scinternal.h>
#include <scez/cards/scpayflex.h>

#include <stdio.h>
#include <string.h>

#if defined( HAVE_LIBDES )
#include <des.h>
#elif defined ( HAVE_LIBCRYPT )
#include <crypt/des.h>
#elif defined ( HAVE_LIBCRYPTO )
#include <openssl/des.h>
#endif /* HAVE_LIBDES */

/* Initialize card function pointer */

int scPayflexInit( SC_CARD_INFO *ci )
{
	ci->scGetCap=scPayflexGetCap;
	ci->scGetCardData=scPayflexGetCardData;
	ci->scSetFD=scPayflexSetFD;

	return( SC_EXIT_OK );
}

/* Capabilities */

int scPayflexGetCap( SC_CARD_INFO *ci, SC_CARD_CAP *cp )
{
	cp->n_fd=1;

	/* 9600 at 3.579MHz */
	cp->fd[0]=(((10L<<16)+372L)<<8)+1;

	return( SC_EXIT_OK );
}

/* Fill card data in ci */

int scPayflexGetCardData( SC_CARD_INFO *ci )
{
	BYTE header[] = { 0x00, 0xC0, 0x00, 0x00, 0x00 };
	BYTE swok[] = { 0x01, 0x90 };
	BYTE swav[] = { 0x01, 0x61 };
	int ret;

	if( (ret=scSmartcardProcessATR( ci ))!=SC_EXIT_OK ) return( ret );
	memcpy( ci->t0.getrsp, header, 5 );
	memcpy( ci->swok, swok, sizeof(swok) );
	memcpy( ci->swav, swav, sizeof(swav) );
	ci->memsize=0;

	return( SC_EXIT_OK );
}

/* Set F and D. */

int scPayflexSetFD( SC_READER_INFO *ri, SC_CARD_INFO *ci, LONG fd )
{
	if( (fd&0xFFFFFF)==((372L<<8)+1) ) return( SC_EXIT_OK );
	return( SC_EXIT_BAD_PARAM );
}

/* Generate Auth */

void scPayflexGenerateAuth( const BYTE *key, const BYTE *chall, BYTE *auth,
	BYTE algo )
{
#ifdef WITH_DES
	des_key_schedule schedule;
	des_key_schedule schedule2;
	des_cblock out;

	des_check_key=0;

	if( algo==SC_PAYFLEX_ALGO_3DES ) {
		des_set_key( (des_cblock *) key, schedule );
		des_set_key( (des_cblock *) (key+8), schedule2 );
		des_ecb2_encrypt( (des_cblock *) chall, &out, schedule, schedule2,
			DES_ENCRYPT );
	} else {
		des_set_key( (des_cblock *) key, schedule );
		des_ecb_encrypt( (des_cblock *) chall, &out, schedule, DES_ENCRYPT );
	}

	memcpy( auth, out, 6 );

	memset( &schedule, 0, sizeof(schedule) );
	memset( &schedule2, 0, sizeof(schedule2) );
	memset( out, 0, sizeof(out) );
#endif /* WITH_DES */
}

/* Compare Auth */

BOOLEAN scPayflexCompareAuth( const BYTE *key, const BYTE *chall,
	const BYTE *auth, BYTE algo )
{
#ifdef WITH_DES
	des_key_schedule schedule;
	des_key_schedule schedule2;
	des_cblock out;

	des_check_key=0;

	if( algo==SC_PAYFLEX_ALGO_3DES ) {
		des_set_key( (des_cblock *) key, schedule );
		des_set_key( (des_cblock *) (key+8), schedule2 );
		des_ecb2_encrypt( (des_cblock *) chall, &out, schedule, schedule2,
			DES_ENCRYPT );
	} else {
		des_set_key( (des_cblock *) key, schedule );
		des_ecb_encrypt( (des_cblock *) chall, &out, schedule, DES_ENCRYPT );
	}

	memset( &schedule, 0, sizeof(schedule) );
	memset( &schedule2, 0, sizeof(schedule2) );

	if( memcmp( auth, out, 6 ) ) return( FALSE );

	memset( out, 0, sizeof(out) );

	return( TRUE );
#else
	return( FALSE );
#endif /* WITH_DES */
}

/* Commands */

int scPayflexCmdChange( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	const BYTE *oldpin, const BYTE *newpin )
{
	BYTE cmd[21];
	BYTE rsp[ SC_GENERAL_SHORT_DATA_SIZE+2 ];
	SC_APDU apdu;
	int ret;

	memset( cmd, 0, sizeof(cmd) );
	cmd[0]=0xF0;
	cmd[1]=0x24;
	cmd[3]=0x01;
	cmd[4]=0x10;

	ci->sw[0]=0x00; ci->sw[1]=0x00;

	apdu.cse=SC_APDU_CASE_3_SHORT;
	apdu.cmd=cmd;
	apdu.cmdlen=21;
	apdu.rsp=rsp;
	apdu.rsplen=0;

	memcpy( cmd+5, oldpin, SC_PAYFLEX_PIN_SIZE );
	memcpy( cmd+5+SC_PAYFLEX_PIN_SIZE, newpin, SC_PAYFLEX_PIN_SIZE );

	ret=scReaderSendAPDU( ri, ci, &apdu );
	memset( cmd, 0, sizeof(cmd) );
	if( ret!=SC_EXIT_OK ) return( ret );

	if( apdu.rsplen!=2 ) return( SC_EXIT_BAD_SW );

	ci->sw[0] = apdu.rsp[0];
	ci->sw[1] = apdu.rsp[1];

	return( SC_EXIT_OK );
}

int scPayflexCmdCreateFile( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	WORD fid, int flen, BYTE ftype, BYTE init, BYTE status, BYTE reclen,
	BYTE recnum, const BYTE *acond, const BYTE *akeys )
{
	BYTE cmd[22];
	BYTE rsp[ SC_GENERAL_SHORT_DATA_SIZE+2 ];
	SC_APDU apdu;
	int ret;

	memset( cmd, 0, sizeof(cmd) );
	cmd[0]=0xF0;
	cmd[1]=0xE0;
	cmd[4]=0x10;
	cmd[5]=0xFF;
	cmd[6]=0xFF;
	cmd[17]=0x03;

	ci->sw[0]=0x00; ci->sw[1]=0x00;

	apdu.cse=SC_APDU_CASE_3_SHORT;
	apdu.cmd=cmd;
	apdu.cmdlen=21;
	apdu.rsp=rsp;
	apdu.rsplen=0;

	cmd[2]=init;
	cmd[7]=(flen >>8) & 0xFF;
	cmd[8]=flen & 0xFF;
	cmd[9]=(fid >> 8) & 0xFF;
	cmd[10]=fid & 0xFF;
	cmd[11]=ftype;
	memcpy( cmd+12, acond, 4 );
	cmd[16]=status;
	memcpy( cmd+18, akeys, 3 );

	if( (ftype==0x02) || (ftype==0x06) ) {
		cmd[3]=recnum;
		cmd[4]=0x11;
		cmd[17]=0x04;
		cmd[21]=reclen;
		apdu.cmdlen=22;
	}

	ret=scReaderSendAPDU( ri, ci, &apdu );
	memset( cmd, 0, sizeof(cmd) );
	if( ret!=SC_EXIT_OK ) return( ret );

	if( apdu.rsplen!=2 ) return( SC_EXIT_BAD_SW );

	ci->sw[0] = apdu.rsp[0];
	ci->sw[1] = apdu.rsp[1];

	return( SC_EXIT_OK );
}

int scPayflexCmdExtAuth( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	BYTE keynum, const BYTE *key, const BYTE *chall, BYTE algo )
{
#ifdef WITH_DES
	BYTE cmd[]={ 0x00, 0x82, 0x00, 0x00, 0x07, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00 };
	BYTE rsp[ SC_GENERAL_SHORT_DATA_SIZE+2 ];
	BYTE mac[ 6 ];
	SC_APDU apdu;
	int ret;

	ci->sw[0]=0x00; ci->sw[1]=0x00;

	scPayflexGenerateAuth( key, chall, mac, algo );

	apdu.cse=SC_APDU_CASE_3_SHORT;
	apdu.cmd=cmd;
	apdu.cmdlen=12;
	apdu.rsp=rsp;
	apdu.rsplen=0;

	cmd[5]=keynum;

	memcpy( cmd+5+1, mac, SC_PAYFLEX_MAC_SIZE );

	ret=scReaderSendAPDU( ri, ci, &apdu );
	memset( cmd, 0, sizeof(cmd) );
	memset( mac, 0, sizeof(mac) );
	if( ret!=SC_EXIT_OK ) return( ret );

	if( apdu.rsplen!=2 ) return( SC_EXIT_BAD_SW );

	ci->sw[0] = apdu.rsp[0];
	ci->sw[1] = apdu.rsp[1];

	return( SC_EXIT_OK );
#else
	return( SC_EXIT_NOT_SUPPORTED );
#endif /* WITH_DES */
}

int scPayflexCmdGetChall( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	BYTE *chall, int *len )
{
	BYTE cmd[]={ 0x00, 0x84, 0x00, 0x00, 0x00 };
	BYTE rsp[ SC_GENERAL_SHORT_DATA_SIZE+2 ];
	SC_APDU apdu;
	int ret;

	ci->sw[0]=0x00; ci->sw[1]=0x00;

	if( (*len<-1) || (*len>255) ) return( SC_EXIT_BAD_PARAM );

	apdu.cse=SC_APDU_CASE_2_SHORT;
	apdu.cmd=cmd;
	apdu.cmdlen=5;
	apdu.rsp=rsp;
	apdu.rsplen=0;

	if( *len==-1 ) *len = 8;
	cmd[4] = *len & 0xFF;

	*len = 0;

	ret=scReaderSendAPDU( ri, ci, &apdu );
	memset( cmd, 0, sizeof(cmd) );
	if( ret!=SC_EXIT_OK ) return( ret );

	if( apdu.rsplen<2 ) return( SC_EXIT_BAD_SW );

	ci->sw[0] = apdu.rsp[apdu.rsplen-2];
	ci->sw[1] = apdu.rsp[apdu.rsplen-1];

	memcpy( chall, apdu.rsp, apdu.rsplen-2 );
	*len = apdu.rsplen-2;

	memset( rsp, 0, sizeof(rsp) );

	return( SC_EXIT_OK );
}

/* sw is used to get the length of the available data. */
int scPayflexCmdGetResp( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	BYTE *resp, int *resplen )
{
	BYTE cmd[]={ 0xC0, 0xC0, 0x00, 0x00, 0x00 };
	BYTE rsp[ SC_GENERAL_SHORT_DATA_SIZE+2 ];
	SC_APDU apdu;
	int ret;

	apdu.cse=SC_APDU_CASE_2_SHORT;
	apdu.cmd=cmd;
	apdu.cmdlen=5;
	apdu.rsp=rsp;
	apdu.rsplen=0;

	cmd[4] = ci->sw[1];

	ci->sw[0]=0x00; ci->sw[1]=0x00;

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

int scPayflexCmdIntAuth( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	BYTE keynum, const BYTE *chall, const BYTE *key, BYTE algo )
{
#ifdef WITH_DES
	BYTE cmd[]={ 0xC0, 0x88, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
	BYTE rsp[ SC_GENERAL_SHORT_DATA_SIZE+2 ];
	SC_APDU apdu;
	int ret;

	ci->sw[0]=0x00; ci->sw[1]=0x00;

	apdu.cse=SC_APDU_CASE_4_SHORT;
	apdu.cmd=cmd;
	apdu.cmdlen=14;
	apdu.rsp=rsp;
	apdu.rsplen=0;

	cmd[3]=keynum;

	memcpy( cmd+5, chall, SC_PAYFLEX_CHALL_SIZE );

	ret=scReaderSendAPDU( ri, ci, &apdu );
	memset( cmd, 0, sizeof(cmd) );
	if( ret!=SC_EXIT_OK ) return( ret );

	if( apdu.rsplen<2 ) return( SC_EXIT_BAD_SW );

	ci->sw[0] = apdu.rsp[apdu.rsplen-2];
	ci->sw[1] = apdu.rsp[apdu.rsplen-1];

	if( (apdu.rsplen>=8) && (scPayflexCompareAuth( key, chall, rsp,
		algo )==TRUE) ) {
		memset( rsp, 0, sizeof(rsp) );
		return( SC_EXIT_OK );
	}

	memset( rsp, 0, sizeof(rsp) );

	return( SC_EXIT_BAD_CHECKSUM );
#else
	return( SC_EXIT_NOT_SUPPORTED );
#endif /* WITH_DES */
}

int scPayflexCmdReadRecord( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	BYTE recnum, BYTE mode, BYTE *data, int *datalen )
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
	cmd[3] = mode;
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

int scPayflexCmdSelect( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	WORD fid, BYTE *resp, int *resplen )
{
	BYTE cmd[]={ 0x00, 0xA4, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00 };
	BYTE rsp[ SC_GENERAL_SHORT_DATA_SIZE+2 ];
	SC_APDU apdu;
	int ret;

	ci->sw[0]=0x00; ci->sw[1]=0x00;

	apdu.cse=SC_APDU_CASE_4_SHORT;
	apdu.cmd=cmd;
	apdu.cmdlen=8;
	apdu.rsp=rsp;
	apdu.rsplen=0;

	cmd[5]=(fid >> 8) & 0xFF;
	cmd[6]=fid & 0xFF;

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

int scPayflexCmdUnblock( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	const BYTE *unblock, const BYTE *new )
{
	BYTE cmd[21];
	BYTE rsp[ SC_GENERAL_SHORT_DATA_SIZE+2 ];
	SC_APDU apdu;
	int ret;

	memset( cmd, 0, sizeof(cmd) );
	cmd[0]=0xF0;
	cmd[1]=0x2C;
	cmd[3]=0x01;
	cmd[4]=0x10;

	ci->sw[0]=0x00; ci->sw[1]=0x00;

	apdu.cse=SC_APDU_CASE_3_SHORT;
	apdu.cmd=cmd;
	apdu.cmdlen=5+SC_PAYFLEX_PIN_SIZE+SC_PAYFLEX_PIN_SIZE;
	apdu.rsp=rsp;
	apdu.rsplen=0;

	memcpy( cmd+5, unblock, SC_PAYFLEX_PIN_SIZE );
	memcpy( cmd+5+SC_PAYFLEX_PIN_SIZE, new, SC_PAYFLEX_PIN_SIZE );

	ret=scReaderSendAPDU( ri, ci, &apdu );
	memset( cmd, 0, sizeof(cmd) );
	if( ret!=SC_EXIT_OK ) return( ret );

	if( apdu.rsplen!=2 ) return( SC_EXIT_BAD_SW );

	ci->sw[0] = apdu.rsp[0];
	ci->sw[1] = apdu.rsp[1];

	return( SC_EXIT_OK );
}

int scPayflexCmdUpdateRecord( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	BYTE recnum, BYTE mode, const BYTE *data, BYTE datalen )
{
	BYTE cmd[ SC_GENERAL_SHORT_DATA_SIZE+5 ];
	BYTE rsp[ SC_GENERAL_SHORT_DATA_SIZE+2 ];
	SC_APDU apdu;
	int ret;

	ci->sw[0]=0x00; ci->sw[1]=0x00;

	apdu.cse=SC_APDU_CASE_3_SHORT;
	apdu.cmd=cmd;
	apdu.cmdlen=5+datalen;
	apdu.rsp=rsp;
	apdu.rsplen=0;

	cmd[0]=0x00;
	cmd[1]=0xDC;
	cmd[2]=recnum;
	cmd[3]=mode;
	cmd[4]=datalen;

	memcpy( cmd+5, data, datalen );

	ret=scReaderSendAPDU( ri, ci, &apdu );
	memset( cmd, 0, sizeof(cmd) );
	if( ret!=SC_EXIT_OK ) return( ret );

	if( apdu.rsplen!=2 ) return( SC_EXIT_BAD_SW );

	ci->sw[0] = apdu.rsp[0];
	ci->sw[1] = apdu.rsp[1];

	return( SC_EXIT_OK );
}

int scPayflexCmdVerify( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	const BYTE *pin )
{
	BYTE cmd[]={ 0x00, 0x20, 0x00, 0x01, 0x08, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00 };
	BYTE rsp[ SC_GENERAL_SHORT_DATA_SIZE+2 ];
	SC_APDU apdu;
	int ret;

	ci->sw[0]=0x00; ci->sw[1]=0x00;

	apdu.cse=SC_APDU_CASE_3_SHORT;
	apdu.cmd=cmd;
	apdu.cmdlen=13;
	apdu.rsp=rsp;
	apdu.rsplen=0;

	memcpy( cmd+5, pin, SC_PAYFLEX_PIN_SIZE );

	ret=scReaderSendAPDU( ri, ci, &apdu );
	memset( cmd, 0, sizeof(cmd) );
	if( ret!=SC_EXIT_OK ) return( ret );

	if( apdu.rsplen!=2 ) return( SC_EXIT_BAD_SW );

	ci->sw[0] = apdu.rsp[0];
	ci->sw[1] = apdu.rsp[1];

	return( SC_EXIT_OK );
}

/****************************************************************************
*																			*
*								Payment Commands							*
*																			*
****************************************************************************/

#if 0
int scPayflexUpdateMaxAmount
int scPayflexCertCredit
int scPayflexCertDebit
int scPayflexCredit
int scPayflexDebit
int scPayflexGenDivCKey
int scPayflexGenDivPVKey
#endif


