/****************************************************************************
*																			*
*						SCEZ chipcard library - MFC routines				*
*						Copyright Matthias Bruestle 2000					*
*					For licensing, see the file COPYRIGHT					*
*																			*
****************************************************************************/

/* $Id: scmfc.c 1617 2005-11-03 17:41:39Z laforge $ */

#include <scez/scinternal.h>
#include <scez/cards/scmfc.h>

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

int scMfcInit( SC_CARD_INFO *ci )
{
	ci->scGetCap=scMfcGetCap;
	ci->scGetCardData=scMfcGetCardData;
	ci->scSetFD=scMfcSetFD;

	return( SC_EXIT_OK );
}

/* Capabilities */

int scMfcGetCap( SC_CARD_INFO *ci, SC_CARD_CAP *cp )
{
	cp->n_fd=7;

	/* 9600 at 3.579MHz */
	cp->fd[0]=(((10L<<16)+372L)<<8)+1;

	return( SC_EXIT_OK );
}

/* Fill card data in ci */

int scMfcGetCardData( SC_CARD_INFO *ci )
{
	BYTE swok[] = { 0x01, 0x90 };
	BYTE swav[] = { 0x00 };
	int ret;

	if( (ret=scSmartcardProcessATR( ci ))!=SC_EXIT_OK ) return( ret );

	ci->t1.ns=0;
	ci->t1.nr=0;
	memcpy( ci->swok, swok, sizeof(swok) );
	memcpy( ci->swav, swav, sizeof(swav) );
	memset( ci->crypt.key, 0, sizeof(ci->crypt.key) );
	memset( ci->crypt.pin, 0, sizeof(ci->crypt.pin) );
	memset( ci->crypt.iv, 0, sizeof(ci->crypt.iv) );
	ci->memsize=0;

	return( SC_EXIT_OK );
}

/* Set F and D. */

int scMfcSetFD( SC_READER_INFO *ri, SC_CARD_INFO *ci, LONG fd )
{
	switch( fd&0xFFFFFF ) {
		case (372L<<8)+1:
			/* return( scReaderPTS( ri, ci, (BYTE *)"\xFF\x10\x11\xFE", 4 ) ); */
			return( SC_EXIT_OK );
		default:
			return( SC_EXIT_BAD_PARAM );
	}
}

/* Generate Auth */

void scMfcGenerateAuth( const BYTE *key, const BYTE *chall, BYTE *auth,
	BYTE algo )
{
#ifdef WITH_DES
	des_key_schedule schedule;
	des_key_schedule schedule2;
	des_cblock out;

	des_check_key=0;

	if( algo==SC_MFC_ALGO_3DES ) {
		des_set_key( (des_cblock *) key, schedule );
		des_set_key( (des_cblock *) (key+8), schedule2 );
		des_ecb2_encrypt( (des_cblock *) chall, &out, schedule, schedule2,
			DES_ENCRYPT );
	} else {
		des_set_key( (des_cblock *) key, schedule );
		des_ecb_encrypt( (des_cblock *) chall, &out, schedule, DES_ENCRYPT );
	}

	memcpy( auth, out, 8 );

	memset( &schedule, 0, sizeof(schedule) );
	memset( &schedule2, 0, sizeof(schedule2) );
	memset( out, 0, sizeof(out) );
#endif /* WITH_DES */
}

/* Compare Auth */

BOOLEAN scMfcCompareAuth( const BYTE *key, const BYTE *chall, const BYTE *auth,
	BYTE algo )
{
#ifdef WITH_DES
	des_key_schedule schedule;
	des_key_schedule schedule2;
	des_cblock out;

	des_check_key=0;

	if( algo==SC_MFC_ALGO_3DES ) {
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

	if( memcmp( auth, out, 8 ) ) return( FALSE );

	memset( out, 0, sizeof(out) );

	return( TRUE );
#else
	return( FALSE );
#endif /* WITH_DES */
}

/* Uses sfid if it is in the correct range. */

int scMfcCmdAppendRecord( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	BYTE sfid, const BYTE *data, BYTE datalen )
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

	memset( cmd, 0, sizeof( cmd ) );

	cmd[1] = 0xE2;
	if( (sfid>0) && (sfid<32) ) cmd[3]=(sfid<<3)&0xFF;
	cmd[4] = datalen;

	memcpy( cmd+5, data, datalen );

	ret=scReaderSendAPDU( ri, ci, &apdu );
	memset( cmd, 0, sizeof(cmd) );
	if( ret!=SC_EXIT_OK ) return( ret );

	if( apdu.rsplen<2 ) return( SC_EXIT_BAD_SW );

	ci->sw[0] = apdu.rsp[apdu.rsplen-2];
	ci->sw[1] = apdu.rsp[apdu.rsplen-1];

	return( SC_EXIT_OK );
}

int scMfcCmdCardBlock( SC_READER_INFO *ri, SC_CARD_INFO *ci )
{
	BYTE cmd[]={ 0x80, 0x16, 0x00, 0x00 };
	BYTE rsp[ SC_GENERAL_SHORT_DATA_SIZE+2 ];
	SC_APDU apdu;
	int ret;

	ci->sw[0]=0x00; ci->sw[1]=0x00;

	apdu.cse=SC_APDU_CASE_1;
	apdu.cmd=cmd;
	apdu.cmdlen=4;
	apdu.rsp=rsp;
	apdu.rsplen=0;

	ret=scReaderSendAPDU( ri, ci, &apdu );
	memset( cmd, 0, sizeof(cmd) );
	if( ret!=SC_EXIT_OK ) return( ret );

	if( apdu.rsplen<2 ) return( SC_EXIT_BAD_SW );

	ci->sw[0] = apdu.rsp[apdu.rsplen-2];
	ci->sw[1] = apdu.rsp[apdu.rsplen-1];

	memset( rsp, 0, sizeof(rsp) );

	return( SC_EXIT_OK );
}

int scMfcCmdChangeCHV( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	BYTE num, const BYTE *oldpin, const BYTE *newpin )
{
	BYTE cmd[ 5+(2*SC_MFC_PIN_SIZE) ];
	BYTE rsp[ SC_GENERAL_SHORT_DATA_SIZE+2 ];
	SC_APDU apdu;
	int ret;

	ci->sw[0]=0x00; ci->sw[1]=0x00;

	if( num>2 ) return( SC_EXIT_BAD_PARAM );

	apdu.cse=SC_APDU_CASE_3_SHORT;
	apdu.cmd=cmd;
	apdu.cmdlen=5+(2*SC_MFC_PIN_SIZE);
	apdu.rsp=rsp;
	apdu.rsplen=0;

	memset( cmd, 0, sizeof( cmd ) );

	cmd[1]=0x24;
	if(num) num--;
	cmd[3]=num;
	cmd[4]=2*SC_MFC_PIN_SIZE;

	memcpy( cmd+5, oldpin, SC_MFC_PIN_SIZE );
	memcpy( cmd+5+SC_MFC_PIN_SIZE, newpin, SC_MFC_PIN_SIZE );

	ret=scReaderSendAPDU( ri, ci, &apdu );
	memset( cmd, 0, sizeof(cmd) );
	if( ret!=SC_EXIT_OK ) return( ret );

	if( apdu.rsplen!=2 ) return( SC_EXIT_BAD_SW );

	ci->sw[0] = apdu.rsp[0];
	ci->sw[1] = apdu.rsp[1];

	return( SC_EXIT_OK );
}

int scMfcCmdCloseAppl( SC_READER_INFO *ri, SC_CARD_INFO *ci )
{
	BYTE cmd[]={ 0xA0, 0xAC, 0x00, 0x00 };
	BYTE rsp[ SC_GENERAL_SHORT_DATA_SIZE+2 ];
	SC_APDU apdu;
	int ret;

	ci->sw[0]=0x00; ci->sw[1]=0x00;

	apdu.cse=SC_APDU_CASE_1;
	apdu.cmd=cmd;
	apdu.cmdlen=4;
	apdu.rsp=rsp;
	apdu.rsplen=0;

	ret=scReaderSendAPDU( ri, ci, &apdu );
	memset( cmd, 0, sizeof(cmd) );
	if( ret!=SC_EXIT_OK ) return( ret );

	if( apdu.rsplen<2 ) return( SC_EXIT_BAD_SW );

	ci->sw[0] = apdu.rsp[apdu.rsplen-2];
	ci->sw[1] = apdu.rsp[apdu.rsplen-1];

	memset( rsp, 0, sizeof(rsp) );

	return( SC_EXIT_OK );
}

/* TODO: Do protection key byte.  */

int scMfcCmdCreateFile( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	WORD fid, WORD flen, BYTE ftype, const BYTE *ac, BYTE status, BYTE recprot,
	const BYTE *data, BYTE datalen )
{
	BYTE cmd[ 5+SC_GENERAL_SHORT_DATA_SIZE ];
	BYTE rsp[ SC_GENERAL_SHORT_DATA_SIZE+2 ];
	SC_APDU apdu;
	int ret;

	ci->sw[0]=0x00; ci->sw[1]=0x00;

	if( datalen>240 ) return( SC_EXIT_BAD_PARAM );

	apdu.cse=SC_APDU_CASE_3_SHORT;
	apdu.cmd=cmd;
	apdu.cmdlen=5+14;
	apdu.rsp=rsp;
	apdu.rsplen=0;

	memset( cmd, 0, sizeof( cmd ) );

	cmd[0]=0xA0;
	cmd[1]=0xE0;
	cmd[4]=14;

	cmd[5]=0x63; /* Hex? */
	cmd[6]=12;
	cmd[7]=flen>>8;
	cmd[8]=flen&0xFF;
	cmd[9]=fid>>8;
	cmd[10]=fid&0xFF;
	memcpy( cmd+11, ac, SC_MFC_AC_SIZE );
	/* Information about file status byte is contradictory. */
	cmd[16]=status;
	/* CHV should be enabled. */
	if( (fid==SC_MFC_FID_EF_CHV1) || (fid==SC_MFC_FID_EF_CHV2) )
		cmd[16]|=0x10;
	cmd[17]=1;
	cmd[18]=ftype;
	if( (ftype==SC_MFC_TYPE_FIXED) || (ftype==SC_MFC_TYPE_CYCLIC) ) {
		cmd[19]=recprot;
		memcpy( cmd+20, data, datalen );
		apdu.cmdlen+=1+datalen;
	} else {
		memcpy( cmd+20, data, datalen );
		apdu.cmdlen+=datalen;
	}

	ret=scReaderSendAPDU( ri, ci, &apdu );
	memset( cmd, 0, sizeof(cmd) );
	if( ret!=SC_EXIT_OK ) return( ret );

	if( apdu.rsplen!=2 ) return( SC_EXIT_BAD_SW );

	ci->sw[0] = apdu.rsp[0];
	ci->sw[1] = apdu.rsp[1];

	return( SC_EXIT_OK );
}

int scMfcCmdDecrease( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	BYTE sfid, const BYTE *amount, BYTE *resp, int *resplen )
{
	BYTE cmd[]={ 0xA0, 0x30, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x00 };
	BYTE rsp[ SC_GENERAL_SHORT_DATA_SIZE+2 ];
	SC_APDU apdu;
	int ret;

	ci->sw[0]=0x00; ci->sw[1]=0x00;

	if( *resplen<0 ) return( SC_EXIT_BAD_PARAM );

	apdu.cse=SC_APDU_CASE_4_SHORT;
	apdu.cmd=cmd;
	apdu.cmdlen=9;
	apdu.rsp=rsp;
	apdu.rsplen=0;

	if( (sfid>0) && (sfid<32) ) cmd[3]=(sfid<<3)&0xFF;
	memcpy( cmd+5, amount, SC_MFC_AMOUNT_SIZE );
	if( (resplen!=NULL) && (*resplen!=00) && (*resplen<256) )
		cmd[8]=*resplen&0xFF;

	ret=scReaderSendAPDU( ri, ci, &apdu );
	memset( cmd, 0, sizeof(cmd) );
	if( ret!=SC_EXIT_OK ) return( ret );

	if( apdu.rsplen<2 ) return( SC_EXIT_BAD_SW );

	ci->sw[0] = apdu.rsp[apdu.rsplen-2];
	ci->sw[1] = apdu.rsp[apdu.rsplen-1];

	apdu.rsplen-=2;

	if( (resp!=NULL) && (resplen!=NULL) ) {
		if( (*resplen!=0) && (*resplen<apdu.rsplen) ) apdu.rsplen=*resplen;
		memcpy( resp, apdu.rsp, apdu.rsplen );
		*resplen=apdu.rsplen;
	}

	memset( rsp, 0, sizeof(rsp) );

	return( SC_EXIT_OK );
}

int scMfcCmdDecreaseStamp( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	BYTE sfid, const BYTE *amount, BYTE *resp, int *resplen )
{
	BYTE cmd[]={ 0xA0, 0x34, 0x01, 0x00, 0x03, 0x00, 0x00, 0x00, 0x00 };
	BYTE rsp[ SC_GENERAL_SHORT_DATA_SIZE+2 ];
	SC_APDU apdu;
	int ret;

	ci->sw[0]=0x00; ci->sw[1]=0x00;

	if( *resplen<0 ) return( SC_EXIT_BAD_PARAM );

	apdu.cse=SC_APDU_CASE_4_SHORT;
	apdu.cmd=cmd;
	apdu.cmdlen=9;
	apdu.rsp=rsp;
	apdu.rsplen=0;

	if( (sfid>0) && (sfid<32) ) cmd[3]=(sfid<<3)&0xFF;
	memcpy( cmd+5, amount, SC_MFC_AMOUNT_SIZE );
	if( (resplen!=NULL) && (*resplen!=00) && (*resplen<256) )
		cmd[8]=*resplen&0xFF;

	ret=scReaderSendAPDU( ri, ci, &apdu );
	memset( cmd, 0, sizeof(cmd) );
	if( ret!=SC_EXIT_OK ) return( ret );

	if( apdu.rsplen<2 ) return( SC_EXIT_BAD_SW );

	ci->sw[0] = apdu.rsp[apdu.rsplen-2];
	ci->sw[1] = apdu.rsp[apdu.rsplen-1];

	apdu.rsplen-=2;

	if( (resp!=NULL) && (resplen!=NULL) ) {
		if( (*resplen!=0) && (*resplen<apdu.rsplen) ) apdu.rsplen=*resplen;
		memcpy( resp, apdu.rsp, apdu.rsplen );
		*resplen=apdu.rsplen;
	}

	memset( rsp, 0, sizeof(rsp) );

	return( SC_EXIT_OK );
}

int scMfcCmdDelete( SC_READER_INFO *ri, SC_CARD_INFO *ci, WORD fid )
{
	BYTE cmd[]={ 0xA0, 0xE4, 0x00, 0x00, 0x02, 0x00, 0x00 };
	BYTE rsp[ SC_GENERAL_SHORT_DATA_SIZE+2 ];
	SC_APDU apdu;
	int ret;

	ci->sw[0]=0x00; ci->sw[1]=0x00;

	apdu.cse=SC_APDU_CASE_3_SHORT;
	apdu.cmd=cmd;
	apdu.cmdlen=5+2;
	apdu.rsp=rsp;
	apdu.rsplen=0;

	cmd[5]=fid>>8;
	cmd[6]=fid&0xFF;

	ret=scReaderSendAPDU( ri, ci, &apdu );
	memset( cmd, 0, sizeof(cmd) );
	if( ret!=SC_EXIT_OK ) return( ret );

	if( apdu.rsplen<2 ) return( SC_EXIT_BAD_SW );

	ci->sw[0] = apdu.rsp[apdu.rsplen-2];
	ci->sw[1] = apdu.rsp[apdu.rsplen-1];

	return( SC_EXIT_OK );
}

int scMfcCmdExtAuth( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	BYTE keynum, const BYTE *key, const BYTE *chall, BYTE algo ) 
{
#ifdef WITH_DES
	BYTE cmd[]={ 0x00, 0x82, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00 };
	BYTE rsp[ SC_GENERAL_SHORT_DATA_SIZE+2 ];
	BYTE mac[ 8 ];
	SC_APDU apdu;
	int ret;

	ci->sw[0]=0x00; ci->sw[1]=0x00;

    scMfcGenerateAuth( key, chall, mac, algo );

	apdu.cse=SC_APDU_CASE_3_SHORT;
	apdu.cmd=cmd;
	apdu.cmdlen=5+8;
	apdu.rsp=rsp;
	apdu.rsplen=0;

	if( keynum ) {
		cmd[4]=0x09;
		cmd[5]=keynum;
		memcpy( cmd+5+1, mac, SC_MFC_BLOCK_SIZE );
	} else {
		memcpy( cmd+5, mac, SC_MFC_BLOCK_SIZE );
	}

	ret=scReaderSendAPDU( ri, ci, &apdu );
	memset( cmd, 0, sizeof(cmd) );
	if( ret!=SC_EXIT_OK ) return( ret );

	if( apdu.rsplen<2 ) return( SC_EXIT_BAD_SW );

	ci->sw[0] = apdu.rsp[apdu.rsplen-2];
	ci->sw[1] = apdu.rsp[apdu.rsplen-1];

	return( SC_EXIT_OK );
#else
	return( SC_EXIT_NOT_SUPPORTED );
#endif /* WITH_DES */
}

int scMfcCmdGetChall( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	BYTE *rand, int *len )
{
	BYTE cmd[]={ 0x00, 0x84, 0x00, 0x00, 0x00 };
	BYTE rsp[ SC_GENERAL_SHORT_DATA_SIZE+2 ];
	SC_APDU apdu;
	int ret;

	ci->sw[0]=0x00; ci->sw[1]=0x00;

	if( *len>24 ) return( SC_EXIT_BAD_PARAM );

	apdu.cse=SC_APDU_CASE_2_SHORT;
	apdu.cmd=cmd;
	apdu.cmdlen=5;
	apdu.rsp=rsp;
	apdu.rsplen=0;

	cmd[4] = *len;

	ret=scReaderSendAPDU( ri, ci, &apdu );
	memset( cmd, 0, sizeof(cmd) );
	if( ret!=SC_EXIT_OK ) return( ret );

	if( apdu.rsplen<2 ) return( SC_EXIT_BAD_SW );

	ci->sw[0] = apdu.rsp[apdu.rsplen-2];
	ci->sw[1] = apdu.rsp[apdu.rsplen-1];

	apdu.rsplen-=2;
	memcpy( rand, apdu.rsp, apdu.rsplen );
	*len=apdu.rsplen;

	memset( rsp, 0, sizeof(rsp) );

	return( SC_EXIT_OK );
}

int scMfcCmdGiveRand( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	const BYTE *rand ) 
{
	BYTE cmd[]={ 0x00, 0x86, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00 };
	BYTE rsp[ SC_GENERAL_SHORT_DATA_SIZE+2 ];
	SC_APDU apdu;
	int ret;

	ci->sw[0]=0x00; ci->sw[1]=0x00;

	apdu.cse=SC_APDU_CASE_3_SHORT;
	apdu.cmd=cmd;
	apdu.cmdlen=5+8;
	apdu.rsp=rsp;
	apdu.rsplen=0;

	memcpy( cmd+5, rand, SC_MFC_BLOCK_SIZE );

	ret=scReaderSendAPDU( ri, ci, &apdu );
	memset( cmd, 0, sizeof(cmd) );
	if( ret!=SC_EXIT_OK ) return( ret );

	if( apdu.rsplen<2 ) return( SC_EXIT_BAD_SW );

	ci->sw[0] = apdu.rsp[apdu.rsplen-2];
	ci->sw[1] = apdu.rsp[apdu.rsplen-1];

	return( SC_EXIT_OK );
}

int scMfcCmdIncrease( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	BYTE sfid, const BYTE *amount, BYTE *resp, int *resplen )
{
	BYTE cmd[]={ 0xA0, 0x32, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x00 };
	BYTE rsp[ SC_GENERAL_SHORT_DATA_SIZE+2 ];
	SC_APDU apdu;
	int ret;

	ci->sw[0]=0x00; ci->sw[1]=0x00;

	if( *resplen<0 ) return( SC_EXIT_BAD_PARAM );

	apdu.cse=SC_APDU_CASE_4_SHORT;
	apdu.cmd=cmd;
	apdu.cmdlen=9;
	apdu.rsp=rsp;
	apdu.rsplen=0;

	if( (sfid>0) && (sfid<32) ) cmd[3]=(sfid<<3)&0xFF;
	memcpy( cmd+5, amount, SC_MFC_AMOUNT_SIZE );
	if( (resplen!=NULL) && (*resplen!=00) && (*resplen<256) )
		cmd[8]=*resplen&0xFF;

	ret=scReaderSendAPDU( ri, ci, &apdu );
	memset( cmd, 0, sizeof(cmd) );
	if( ret!=SC_EXIT_OK ) return( ret );

	if( apdu.rsplen<2 ) return( SC_EXIT_BAD_SW );

	ci->sw[0] = apdu.rsp[apdu.rsplen-2];
	ci->sw[1] = apdu.rsp[apdu.rsplen-1];

	apdu.rsplen-=2;

	if( (resp!=NULL) && (resplen!=NULL) ) {
		if( (*resplen!=0) && (*resplen<apdu.rsplen) ) apdu.rsplen=*resplen;
		memcpy( resp, apdu.rsp, apdu.rsplen );
		*resplen=apdu.rsplen;
	}

	memset( rsp, 0, sizeof(rsp) );

	return( SC_EXIT_OK );
}

int scMfcCmdIncreaseStamp( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	BYTE sfid, const BYTE *amount, BYTE *resp, int *resplen )
{
	BYTE cmd[]={ 0xA0, 0x36, 0x01, 0x00, 0x03, 0x00, 0x00, 0x00, 0x00 };
	BYTE rsp[ SC_GENERAL_SHORT_DATA_SIZE+2 ];
	SC_APDU apdu;
	int ret;

	ci->sw[0]=0x00; ci->sw[1]=0x00;

	if( *resplen<0 ) return( SC_EXIT_BAD_PARAM );

	apdu.cse=SC_APDU_CASE_4_SHORT;
	apdu.cmd=cmd;
	apdu.cmdlen=9;
	apdu.rsp=rsp;
	apdu.rsplen=0;

	if( (sfid>0) && (sfid<32) ) cmd[3]=(sfid<<3)&0xFF;
	memcpy( cmd+5, amount, SC_MFC_AMOUNT_SIZE );
	if( (resplen!=NULL) && (*resplen!=00) && (*resplen<256) )
		cmd[8]=*resplen&0xFF;

	ret=scReaderSendAPDU( ri, ci, &apdu );
	memset( cmd, 0, sizeof(cmd) );
	if( ret!=SC_EXIT_OK ) return( ret );

	if( apdu.rsplen<2 ) return( SC_EXIT_BAD_SW );

	ci->sw[0] = apdu.rsp[apdu.rsplen-2];
	ci->sw[1] = apdu.rsp[apdu.rsplen-1];

	apdu.rsplen-=2;

	if( (resp!=NULL) && (resplen!=NULL) ) {
		if( (*resplen!=0) && (*resplen<apdu.rsplen) ) apdu.rsplen=*resplen;
		memcpy( resp, apdu.rsp, apdu.rsplen );
		*resplen=apdu.rsplen;
	}

	memset( rsp, 0, sizeof(rsp) );

	return( SC_EXIT_OK );
}

int scMfcCmdIntAuth( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	BYTE keynum, const BYTE *chall, const BYTE *key, BYTE algo )
{
#ifdef WITH_DES
	BYTE cmd[]={ 0x00, 0x88, 0x01, 0x00, 0x09, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x08 };
	BYTE rsp[ SC_GENERAL_SHORT_DATA_SIZE+2 ];
	SC_APDU apdu;
	int ret;

	ci->sw[0]=0x00; ci->sw[1]=0x00;

	apdu.cse=SC_APDU_CASE_4_SHORT;
	apdu.cmd=cmd;
	apdu.cmdlen=15;
	apdu.rsp=rsp;
	apdu.rsplen=0;

	cmd[5]=keynum;

	memcpy( cmd+6, chall, SC_MFC_BLOCK_SIZE );

	ret=scReaderSendAPDU( ri, ci, &apdu );
	memset( cmd, 0, sizeof(cmd) );
	if( ret!=SC_EXIT_OK ) return( ret );

	if( apdu.rsplen<2 ) return( SC_EXIT_BAD_SW );

	ci->sw[0] = apdu.rsp[apdu.rsplen-2];
	ci->sw[1] = apdu.rsp[apdu.rsplen-1];

	if( (apdu.rsplen>=8) && (scMfcCompareAuth( key, chall, rsp, algo )
		==TRUE) ) {
		memset( rsp, 0, sizeof(rsp) );
		return( SC_EXIT_OK );
	}

	memset( rsp, 0, sizeof(rsp) );

	return( SC_EXIT_BAD_CHECKSUM );
#else
	return( SC_EXIT_NOT_SUPPORTED );
#endif /* WITH_DES */
}

int scMfcCmdInvalidate( SC_READER_INFO *ri, SC_CARD_INFO *ci )
{
	BYTE cmd[]={ 0xA0, 0x04, 0x00, 0x00 };
	BYTE rsp[ SC_GENERAL_SHORT_DATA_SIZE+2 ];
	SC_APDU apdu;
	int ret;

	ci->sw[0]=0x00; ci->sw[1]=0x00;

	apdu.cse=SC_APDU_CASE_1;
	apdu.cmd=cmd;
	apdu.cmdlen=4;
	apdu.rsp=rsp;
	apdu.rsplen=0;

	ret=scReaderSendAPDU( ri, ci, &apdu );
	memset( cmd, 0, sizeof(cmd) );
	if( ret!=SC_EXIT_OK ) return( ret );

	if( apdu.rsplen<2 ) return( SC_EXIT_BAD_SW );

	ci->sw[0] = apdu.rsp[apdu.rsplen-2];
	ci->sw[1] = apdu.rsp[apdu.rsplen-1];

	memset( rsp, 0, sizeof(rsp) );

	return( SC_EXIT_OK );
}

/* TODO: Do something better. */

int scMfcCmdLoadKeyFile( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	const BYTE *data, BYTE datalen )
{
	BYTE cmd[ 5+SC_GENERAL_SHORT_DATA_SIZE ];
	BYTE rsp[ SC_GENERAL_SHORT_DATA_SIZE+2 ];
	SC_APDU apdu;
	int ret;

	ci->sw[0]=0x00; ci->sw[1]=0x00;

	apdu.cse=SC_APDU_CASE_3_SHORT;
	apdu.cmd=cmd;
	apdu.cmdlen=5+datalen;
	apdu.rsp=rsp;
	apdu.rsplen=0;

	memset( cmd, 0, sizeof( cmd ) );

	cmd[0]=0xA0;
	cmd[1]=0xD8;
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

int scMfcCmdLock( SC_READER_INFO *ri, SC_CARD_INFO *ci, BYTE set,
	BYTE ac, WORD fid )
{
	BYTE cmd[]={ 0xA0, 0x76, 0x00, 0x00, 0x02, 0x00, 0x00 };
	BYTE rsp[ SC_GENERAL_SHORT_DATA_SIZE+2 ];
	SC_APDU apdu;
	int ret;

	ci->sw[0]=0x00; ci->sw[1]=0x00;

	apdu.cse=SC_APDU_CASE_3_SHORT;
	apdu.cmd=cmd;
	apdu.cmdlen=5+2;
	apdu.rsp=rsp;
	apdu.rsplen=0;

	cmd[2]=set;
	cmd[3]=ac;
	cmd[5]=fid>>8;
	cmd[6]=fid&0xFF;

	ret=scReaderSendAPDU( ri, ci, &apdu );
	memset( cmd, 0, sizeof(cmd) );
	if( ret!=SC_EXIT_OK ) return( ret );

	if( apdu.rsplen<2 ) return( SC_EXIT_BAD_SW );

	ci->sw[0] = apdu.rsp[apdu.rsplen-2];
	ci->sw[1] = apdu.rsp[apdu.rsplen-1];

	return( SC_EXIT_OK );
}

int scMfcCmdModBaudRate( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	BYTE div )
{
	BYTE cmd[]={ 0xB6, 0x42, 0x00, 0x00 };
	BYTE rsp[ SC_GENERAL_SHORT_DATA_SIZE+2 ];
	SC_APDU apdu;
	int ret;

	ci->sw[0]=0x00; ci->sw[1]=0x00;

	apdu.cse=SC_APDU_CASE_1;
	apdu.cmd=cmd;
	apdu.cmdlen=4;
	apdu.rsp=rsp;
	apdu.rsplen=0;

	cmd[3]=div;

	ret=scReaderSendAPDU( ri, ci, &apdu );
	memset( cmd, 0, sizeof(cmd) );
	if( ret!=SC_EXIT_OK ) return( ret );

	if( apdu.rsplen<2 ) return( SC_EXIT_BAD_SW );

	ci->sw[0] = apdu.rsp[apdu.rsplen-2];
	ci->sw[1] = apdu.rsp[apdu.rsplen-1];

	memset( rsp, 0, sizeof(rsp) );

	return( SC_EXIT_OK );
}

/* offset can be sfid/soffset (highest bit set) */

int scMfcCmdReadBinary( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	WORD offset, BYTE *data, int *datalen )
{
	BYTE cmd[]={ 0x00, 0xB0, 0x00, 0x00, 0x00 };
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

	cmd[2] = offset >>8;
	cmd[3] = offset & 0xFF;
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

/* TODO: Stamp check. */
/* offset can be sfid/soffset (highest bit set) */

int scMfcCmdReadBinStamp( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	WORD offset, BYTE *data, int *datalen )
{
	BYTE cmd[]={ 0xA4, 0xB4, 0x00, 0x00, 0x00 };
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

	cmd[2] = offset >>8;
	cmd[3] = offset & 0xFF;
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

/* Uses sfid if it is in the correct range. */

int scMfcCmdReadRecord( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	BYTE rec, BYTE addr, BYTE sfid, BYTE *data, int *datalen )
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

	if( addr==SC_MFC_RECORD_CURR ) {
		addr=SC_MFC_RECORD_ABS;
		rec=0;
	}

	cmd[2] = rec;
	cmd[3] = addr & 0x07;
	if( (sfid>0) && (sfid<32) ) cmd[3]|=(sfid<<3)&0xFF;
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

/* TODO: Stamp check. */
/* Uses sfid if it is in the correct range. */

int scMfcCmdReadRecStamp( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	BYTE rec, BYTE addr, BYTE sfid, BYTE *data, int *datalen )
{
	BYTE cmd[]={ 0xA4, 0xB6, 0x00, 0x00, 0x00 };
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

	if( addr==SC_MFC_RECORD_CURR ) {
		addr=SC_MFC_RECORD_ABS;
		rec=0;
	}

	cmd[2] = rec;
	cmd[3] = addr & 0x07;
	if( (sfid>0) && (sfid<32) ) cmd[3]|=(sfid<<3)&0xFF;
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

int scMfcCmdReadStat( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	BYTE *data, int *datalen )
{
	BYTE cmd[]={ 0xB6, 0x40, 0x00, 0x00, 0x00 };
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

int scMfcCmdRehabilitate( SC_READER_INFO *ri, SC_CARD_INFO *ci )
{
	BYTE cmd[]={ 0xA0, 0x44, 0x00, 0x00 };
	BYTE rsp[ SC_GENERAL_SHORT_DATA_SIZE+2 ];
	SC_APDU apdu;
	int ret;

	ci->sw[0]=0x00; ci->sw[1]=0x00;

	apdu.cse=SC_APDU_CASE_1;
	apdu.cmd=cmd;
	apdu.cmdlen=4;
	apdu.rsp=rsp;
	apdu.rsplen=0;

	ret=scReaderSendAPDU( ri, ci, &apdu );
	memset( cmd, 0, sizeof(cmd) );
	if( ret!=SC_EXIT_OK ) return( ret );

	if( apdu.rsplen<2 ) return( SC_EXIT_BAD_SW );

	ci->sw[0] = apdu.rsp[apdu.rsplen-2];
	ci->sw[1] = apdu.rsp[apdu.rsplen-1];

	memset( rsp, 0, sizeof(rsp) );

	return( SC_EXIT_OK );
}

int scMfcCmdSeek( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	BYTE offset, BYTE start, const BYTE *data, BYTE datalen, BYTE *recnum )
{
	BYTE cmd[ 5+SC_GENERAL_SHORT_DATA_SIZE+1 ];
	BYTE rsp[ SC_GENERAL_SHORT_DATA_SIZE+2 ];
	SC_APDU apdu;
	int ret;

	ci->sw[0]=0x00; ci->sw[1]=0x00;

	apdu.cse=SC_APDU_CASE_3_SHORT;
	apdu.cmd=cmd;
	apdu.cmdlen=5+datalen;
	apdu.rsp=rsp;
	apdu.rsplen=0;

	memset( cmd, 0, sizeof(cmd) );

	cmd[0]=0xA0;
	cmd[1]=0xA2;
	cmd[2]=offset;
	cmd[3]=start;
	if( recnum!=NULL ) {
		cmd[3]=0x10;
		apdu.cmdlen++;
		apdu.cse=SC_APDU_CASE_4_SHORT;
	}
	cmd[4]=datalen;
	memcpy( cmd+5, data, datalen );

	ret=scReaderSendAPDU( ri, ci, &apdu );
	memset( cmd, 0, sizeof(cmd) );
	if( ret!=SC_EXIT_OK ) return( ret );

	if( apdu.rsplen<2 ) return( SC_EXIT_BAD_SW );

	ci->sw[0] = apdu.rsp[apdu.rsplen-2];
	ci->sw[1] = apdu.rsp[apdu.rsplen-1];

	apdu.rsplen-=2;

	if( recnum!=NULL ) {
		if( apdu.rsplen ) *recnum=apdu.rsp[0];
		else *recnum=0;
	}

	memset( rsp, 0, sizeof(rsp) );

	return( SC_EXIT_OK );
}

int scMfcCmdSelect( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	BYTE type, WORD fid, const BYTE *aidpath, BYTE aidpathlen, BYTE *resp,
	int *resplen )
{
	BYTE cmd[ 5+16+1 ];
	BYTE rsp[ SC_GENERAL_SHORT_DATA_SIZE+2 ];
	SC_APDU apdu;
	int ret;

	ci->sw[0]=0x00; ci->sw[1]=0x00;

	if( (aidpathlen>16) && (type>=0x04) ) return( SC_EXIT_BAD_PARAM );

	apdu.cse=SC_APDU_CASE_3_SHORT;
	apdu.cmd=cmd;
	apdu.cmdlen=4;
	apdu.rsp=rsp;
	apdu.rsplen=0;

	memset( cmd, 0, sizeof(cmd) );

	cmd[1]=0xA4;
	cmd[2]=type;
	cmd[3]=0x0C;
	if( (resp!=NULL) && (resplen!=NULL) ) cmd[3]=0x00;

	switch( type ) {
		case SC_MFC_SELECT_PARENT:
			if( (resp==NULL) || (resplen==NULL) ) apdu.cse=SC_APDU_CASE_1;
			else apdu.cse=SC_APDU_CASE_2_SHORT;
			break;
		case SC_MFC_SELECT_FID:
			cmd[4]=0x02;
			cmd[5]=fid>>8;
			cmd[6]=fid & 0xFF;
			apdu.cmdlen=5+2;
			break;
		case SC_MFC_SELECT_AID:
		case SC_MFC_SELECT_ABS_PATH:
		case SC_MFC_SELECT_REL_PATH:
			cmd[4]=aidpathlen;
			memcpy( cmd+5, aidpath, aidpathlen );
			apdu.cmdlen=5+aidpathlen;
			break;
		default:
			return( SC_EXIT_BAD_PARAM );
	}

	if( (apdu.cse==SC_APDU_CASE_3_SHORT) && (resp!=NULL) &&
		(resplen!=NULL) ) {
		apdu.cmdlen++;
		apdu.cse=SC_APDU_CASE_4_SHORT;
	}

	ret=scReaderSendAPDU( ri, ci, &apdu );
	memset( cmd, 0, sizeof(cmd) );
	if( ret!=SC_EXIT_OK ) return( ret );

	if( apdu.rsplen<2 ) return( SC_EXIT_BAD_SW );

	ci->sw[0] = apdu.rsp[apdu.rsplen-2];
	ci->sw[1] = apdu.rsp[apdu.rsplen-1];

	apdu.rsplen-=2;

	if( (resp!=NULL) && (resplen!=NULL) ) {
		memcpy( resp, apdu.rsp, apdu.rsplen );
		*resplen=apdu.rsplen;
	}

	memset( rsp, 0, sizeof(rsp) );

	return( SC_EXIT_OK );
}

int scMfcCmdSetExec( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	BOOLEAN exec, WORD fid )
{
	BYTE cmd[]={ 0xA0, 0x78, 0x00, 0x00, 0x02, 0x00, 0x00 };
	BYTE rsp[ SC_GENERAL_SHORT_DATA_SIZE+2 ];
	SC_APDU apdu;
	int ret;

	ci->sw[0]=0x00; ci->sw[1]=0x00;

	apdu.cse=SC_APDU_CASE_3_SHORT;
	apdu.cmd=cmd;
	apdu.cmdlen=5+2;
	apdu.rsp=rsp;
	apdu.rsplen=0;

	if( !exec) cmd[3]=0x80;
	cmd[5]=fid>>8;
	cmd[6]=fid&0xFF;

	ret=scReaderSendAPDU( ri, ci, &apdu );
	memset( cmd, 0, sizeof(cmd) );
	if( ret!=SC_EXIT_OK ) return( ret );

	if( apdu.rsplen<2 ) return( SC_EXIT_BAD_SW );

	ci->sw[0] = apdu.rsp[apdu.rsplen-2];
	ci->sw[1] = apdu.rsp[apdu.rsplen-1];

	return( SC_EXIT_OK );
}

int scMfcCmdUnblockCHV( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	BYTE num, const BYTE *unblock, const BYTE *new )
{
	BYTE cmd[ 5+(2*SC_MFC_PIN_SIZE) ];
	BYTE rsp[ SC_GENERAL_SHORT_DATA_SIZE+2 ];
	SC_APDU apdu;
	int ret;

	ci->sw[0]=0x00; ci->sw[1]=0x00;

	if( num>2 ) return( SC_EXIT_BAD_PARAM );

	apdu.cse=SC_APDU_CASE_3_SHORT;
	apdu.cmd=cmd;
	apdu.cmdlen=5+(2*SC_MFC_PIN_SIZE);
	apdu.rsp=rsp;
	apdu.rsplen=0;

	memset( cmd, 0, sizeof( cmd ) );

	cmd[0]=0xA0;
	cmd[1]=0x2C;
	if(num) num--;
	cmd[3]=num;
	cmd[4]=2*SC_MFC_PIN_SIZE;

	memcpy( cmd+5, unblock, SC_MFC_PIN_SIZE );
	memcpy( cmd+5+SC_MFC_PIN_SIZE, new, SC_MFC_PIN_SIZE );

	ret=scReaderSendAPDU( ri, ci, &apdu );
	memset( cmd, 0, sizeof(cmd) );
	if( ret!=SC_EXIT_OK ) return( ret );

	if( apdu.rsplen!=2 ) return( SC_EXIT_BAD_SW );

	ci->sw[0] = apdu.rsp[0];
	ci->sw[1] = apdu.rsp[1];

	return( SC_EXIT_OK );
}

/* offset can be sfid/soffset (highest bit set) */

int scMfcCmdUpdateBinary( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	WORD offset, const BYTE *data, BYTE datalen )
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

	memset( cmd, 0, sizeof( cmd ) );

	cmd[1] = 0xD6;
	cmd[2] = offset >>8;
	cmd[3] = offset & 0xFF;
	cmd[4] = datalen;

	memcpy( cmd+5, data, datalen );

	ret=scReaderSendAPDU( ri, ci, &apdu );
	memset( cmd, 0, sizeof(cmd) );
	if( ret!=SC_EXIT_OK ) return( ret );

	if( apdu.rsplen<2 ) return( SC_EXIT_BAD_SW );

	ci->sw[0] = apdu.rsp[apdu.rsplen-2];
	ci->sw[1] = apdu.rsp[apdu.rsplen-1];

	return( SC_EXIT_OK );
}

/* Uses sfid if it is in the correct range. */

int scMfcCmdUpdateRecord( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	BYTE rec, BYTE addr, BYTE sfid, const BYTE *data, BYTE datalen )
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

	if( addr==SC_MFC_RECORD_CURR ) {
		addr=SC_MFC_RECORD_ABS;
		rec=0;
	}

	memset( cmd, 0, sizeof( cmd ) );

	cmd[1] = 0xDC;
	cmd[2] = rec;
	cmd[3] = addr & 0x07;
	if( (sfid>0) && (sfid<32) ) cmd[3]|=(sfid<<3)&0xFF;
	cmd[4] = datalen;

	memcpy( cmd+5, data, datalen );

	ret=scReaderSendAPDU( ri, ci, &apdu );
	memset( cmd, 0, sizeof(cmd) );
	if( ret!=SC_EXIT_OK ) return( ret );

	if( apdu.rsplen<2 ) return( SC_EXIT_BAD_SW );

	ci->sw[0] = apdu.rsp[apdu.rsplen-2];
	ci->sw[1] = apdu.rsp[apdu.rsplen-1];

	return( SC_EXIT_OK );
}

int scMfcCmdVerifyCHV( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	BYTE num, const BYTE *pin )
{
	BYTE cmd[ 5+SC_MFC_PIN_SIZE ];
	BYTE rsp[ SC_GENERAL_SHORT_DATA_SIZE+2 ];
	SC_APDU apdu;
	int ret;

	ci->sw[0]=0x00; ci->sw[1]=0x00;

	if( num>2 ) return( SC_EXIT_BAD_PARAM );

	apdu.cse=SC_APDU_CASE_3_SHORT;
	apdu.cmd=cmd;
	apdu.cmdlen=5+SC_MFC_PIN_SIZE;
	apdu.rsp=rsp;
	apdu.rsplen=0;

	memset( cmd, 0, sizeof( cmd ) );

	cmd[1]=0x20;
	if(num) num--;
	cmd[3]=num;
	cmd[4]=SC_MFC_PIN_SIZE;

	memcpy( cmd+5, pin, SC_MFC_PIN_SIZE );

	ret=scReaderSendAPDU( ri, ci, &apdu );
	memset( cmd, 0, sizeof(cmd) );
	if( ret!=SC_EXIT_OK ) return( ret );

	if( apdu.rsplen!=2 ) return( SC_EXIT_BAD_SW );

	ci->sw[0] = apdu.rsp[0];
	ci->sw[1] = apdu.rsp[1];

	return( SC_EXIT_OK );
}

