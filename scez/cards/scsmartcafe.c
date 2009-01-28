/****************************************************************************
*																			*
*					SCEZ chipcard library - Sm@rtCafe routines				*
*						Copyright Matthias Bruestle 2000					*
*					For licensing, see the file COPYRIGHT					*
*																			*
****************************************************************************/

/* $Id: scsmartcafe.c 1617 2005-11-03 17:41:39Z laforge $ */

#include <scez/scinternal.h>
#include <scez/cards/scsmartcafe.h>
#include <scez/sct1.h>

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

int scSmartcafeInit( SC_CARD_INFO *ci )
{
	ci->scGetCardData=scSmartcafeGetCardData;

	return( SC_EXIT_OK );
}

/* Fill card data in ci */

int scSmartcafeGetCardData( SC_CARD_INFO *ci )
{
	BYTE header[] = { 0x00, 0xC0, 0x00, 0x00, 0x00 };
	BYTE swok[] = { 0x01, 0x90 };
	BYTE swav[] = { 0x01, 0x61 };
	int ret;

	if( (ret=scSmartcardProcessATR( ci ))!=SC_EXIT_OK ) return( ret );

    ci->direct=TRUE;
    ci->t0.d=1;
    ci->t0.wi=10;
    ci->t0.wwt=9600*20; /* Not enough for Load Applet */
    ci->t1.ns=0;
    ci->t1.nr=0;

    memcpy( ci->t0.getrsp, header, 5 );
	memcpy( ci->swok, swok, sizeof(swok) );
	memcpy( ci->swav, swav, sizeof(swav) );
	ci->memsize=0;

	return( SC_EXIT_OK );
}

/* The length of enckey and sigkey depends on the algorithm.
 * For signing the size of data must be 8 more than the size of the applet to
 * have enough space for the MAC.
 */

int scSmartcafeAuthApplet( const BYTE *enckey, BYTE encalgo,
	const BYTE *sigkey, BYTE sigalgo, BYTE *data, int *datalen )
{
#ifdef WITH_DES
	des_key_schedule schedule;
	des_key_schedule schedule2;
	des_cblock in, out;
	int i, j;

	if( *datalen & 0x1F ) return( SC_EXIT_BAD_PARAM );

	des_check_key=0;

	memset( in, 0, sizeof(in) );

	if( (encalgo==SC_SMARTCAFE_ALGO_3DES) && (enckey!=NULL) ) {
		des_set_key( (des_cblock *) enckey, schedule );
		des_set_key( (des_cblock *) (enckey+8), schedule2 );
		des_ede2_cbc_encrypt( (des_cblock *)data, (des_cblock *)data,
			*datalen, schedule, schedule2, (des_cblock *) in, DES_ENCRYPT );
	} else if( (encalgo==SC_SMARTCAFE_ALGO_DES) && (enckey!=NULL) ) {
		des_set_key( (des_cblock *) enckey, schedule );
		des_cbc_encrypt( (des_cblock *)data, (des_cblock *)data,
			*datalen, schedule, (des_cblock *) in, DES_ENCRYPT );
	}

	memset( in, 0, sizeof(in) );
	j=0;

	/* CBC-MAC with padding "80 00 00 00 00 00 00 00" */
	if( (sigalgo==SC_SMARTCAFE_ALGO_3DES) && (sigkey!=NULL) ) {
		des_set_key( (des_cblock *) sigkey, schedule );
		des_set_key( (des_cblock *) (sigkey+8), schedule2 );

		memset( data+*datalen, 0, 8 );
		data[ *datalen ] = 0x80;

		while( j<(*datalen+8) ) {
			for( i=0; i<8; i++ ) in[i]^=data[i+j];
			des_ecb2_encrypt( (des_cblock *)in, (des_cblock *)out, schedule,
				schedule2, DES_ENCRYPT );
			memcpy( in, out, 8 );
			j+=8;
		}
	} else if( (sigalgo==SC_SMARTCAFE_ALGO_DES) && (sigkey!=NULL) ) {
		des_set_key( (des_cblock *) sigkey, schedule );

		memset( data+*datalen, 0, 8 );
		data[ *datalen ] = 0x80;

		while( j<(*datalen+8) ) {
			for( i=0; i<8; i++ ) in[i]^=data[i+j];
			des_ecb_encrypt( (des_cblock *)in, (des_cblock *)out, schedule,
				DES_ENCRYPT );
			memcpy( in, out, 8 );
			j+=8;
		}
	}

	memcpy( data+*datalen, in, 8 );
	*datalen+=8;

	memset( &schedule, 0, sizeof(schedule) );
	memset( &schedule2, 0, sizeof(schedule2) );
	memset( in, 0, sizeof(in) );
	memset( out, 0, sizeof(out) );

	return( SC_EXIT_OK );
#else
	return( SC_EXIT_NOT_SUPPORTED );
#endif /* WITH_DES */
}

int scSmartcafeCmdGetResp( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	BYTE *resp, int *resplen )
{
	BYTE cmd[]={ 0x00, 0xC0, 0x00, 0x00, 0x00 };
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

int scSmartcafeCmdClearMem( SC_READER_INFO *ri, SC_CARD_INFO *ci )
{
	BYTE cmd[]={ 0x80, 0x1A, 0x00, 0x00 };
	BYTE rsp[ SC_GENERAL_SHORT_DATA_SIZE+2 ];
	SC_APDU apdu;
	int ret;

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

	return( SC_EXIT_OK );
}

/* sp contains sp1|sp2. */

int scSmartcafeCmdCreateML( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	BOOLEAN start, BYTE maclen, BYTE spl, const BYTE *sp )
{
	BYTE cmd[ 5+11 ];
	BYTE rsp[ SC_GENERAL_SHORT_DATA_SIZE+2 ];
	SC_APDU apdu;
	int ret;

	ci->sw[0]=0x00; ci->sw[1]=0x00;

	apdu.cse=SC_APDU_CASE_3_SHORT;
	apdu.cmd=cmd;
	apdu.cmdlen=5;
	apdu.rsp=rsp;
	apdu.rsplen=0;

	memset( cmd, 0, sizeof( cmd ) );

	cmd[0] = 0x80;
	cmd[1] = 0xE0;
	if( start ) {
		apdu.cmdlen=5+11;
		cmd[2]=0x01;
		cmd[4]=0x0B;
		cmd[5]=0x0A;
		cmd[6]=maclen;
		cmd[7]=0x00;
		cmd[8]=spl;
		memcpy( cmd+9, sp, 2 );
		cmd[11]=0x00;
		memcpy( cmd+12, sp+2, 4 );
	} else {
		apdu.cse=SC_APDU_CASE_1;
		apdu.cmdlen=4;
		cmd[2]=0x05;
	}

	ret=scReaderSendAPDU( ri, ci, &apdu );
	memset( cmd, 0, sizeof(cmd) );
	if( ret!=SC_EXIT_OK ) return( ret );

	if( apdu.rsplen<2 ) return( SC_EXIT_BAD_SW );

	ci->sw[0] = apdu.rsp[apdu.rsplen-2];
	ci->sw[1] = apdu.rsp[apdu.rsplen-1];

	return( SC_EXIT_OK );
}

int scSmartcafeCmdDeleteML( SC_READER_INFO *ri, SC_CARD_INFO *ci )
{
	BYTE cmd[]={ 0x80, 0xE4, 0x00, 0x00 };
	BYTE rsp[ SC_GENERAL_SHORT_DATA_SIZE+2 ];
	SC_APDU apdu;
	int ret;

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

	return( SC_EXIT_OK );
}

int scSmartcafeCmdGetData( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	BYTE tag, BYTE *data, int *datalen )
{
	BYTE cmd[]={ 0x80, 0xCA, 0x00, 0x00, 0x00 };
	BYTE rsp[ SC_GENERAL_SHORT_DATA_SIZE+2 ];
	SC_APDU apdu;
	int ret;

	ci->sw[0]=0x00; ci->sw[1]=0x00;

	apdu.cse=SC_APDU_CASE_2_SHORT;
	apdu.cmd=cmd;
	apdu.cmdlen=5;
	apdu.rsp=rsp;
	apdu.rsplen=0;

	cmd[3]=tag;

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

int scSmartcafeCmdInstall( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	BYTE rcp, BYTE spl, const BYTE *aid, BYTE aidlen, const BYTE *param,
	BYTE paramlen, WORD heap )
{
	BYTE cmd[ 5+SC_GENERAL_SHORT_DATA_SIZE ];
	BYTE rsp[ SC_GENERAL_SHORT_DATA_SIZE+2 ];
	SC_APDU apdu;
	int ret;

	ci->sw[0]=0x00; ci->sw[1]=0x00;

	if( aidlen>16 ) return( SC_EXIT_BAD_PARAM );
	if( (rcp!=SC_SMARTCAFE_INSTALL_LOAD) && (paramlen>240) )
		return( SC_EXIT_BAD_PARAM );

	apdu.cse=SC_APDU_CASE_3_SHORT;
	apdu.cmd=cmd;
	apdu.cmdlen=5;
	apdu.rsp=rsp;
	apdu.rsplen=0;

	memset( cmd, 0, sizeof( cmd ) );

	cmd[0]=0x80;
	cmd[1]=0xE6;
	cmd[2]=rcp;
	cmd[3]=spl;

	switch( rcp ) {
	case SC_SMARTCAFE_INSTALL_LOAD:
		apdu.cmdlen=5+1+aidlen+2;	/* Last byte is missing in dox. */
		cmd[4]=aidlen+3;
		cmd[5]=aidlen;
		memcpy( cmd+6, aid, aidlen );
		break;
	case SC_SMARTCAFE_INSTALL_INST:
		apdu.cmdlen=5+1+aidlen+2;
		cmd[5]=aidlen;
		memcpy( cmd+6, aid, aidlen );
		if( (paramlen>0) && (param!=NULL) ) {
			apdu.cmdlen+=paramlen;
			cmd[5+1+aidlen+1]=paramlen;
			memcpy( cmd+5+1+aidlen+2, param, paramlen );
		} else {
			paramlen=0;
		}
		cmd[4]=aidlen+paramlen+3;
		break;
	case SC_SMARTCAFE_INSTALL_INSTHEAP:
		apdu.cmdlen=5+1+aidlen+2+2;
		cmd[5]=aidlen;
		memcpy( cmd+6, aid, aidlen );
		if( (paramlen>0) && (param!=NULL) ) {
			apdu.cmdlen+=paramlen;
			cmd[5+1+aidlen+1]=paramlen;
			memcpy( cmd+5+1+aidlen+2, param, paramlen );
		} else {
			paramlen=0;
		}
		cmd[4]=aidlen+paramlen+5;
		cmd[5+1+aidlen+2+paramlen]=heap>>8;
		cmd[5+1+aidlen+2+paramlen+1]=heap&0xFF;
		break;
	default:
		return( SC_EXIT_BAD_PARAM );
		break;
	}

	ret=scReaderSendAPDU( ri, ci, &apdu );
	memset( cmd, 0, sizeof(cmd) );
	if( ret!=SC_EXIT_OK ) return( ret );

	if( apdu.rsplen<2 ) return( SC_EXIT_BAD_SW );

	ci->sw[0] = apdu.rsp[apdu.rsplen-2];
	ci->sw[1] = apdu.rsp[apdu.rsplen-1];

	return( SC_EXIT_OK );
}

int scSmartcafeCmdLoadApplet( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	BOOLEAN last, BYTE blknum, const BYTE *data, BYTE datalen )
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

	cmd[0]=0x80;
	cmd[1]=0xE8;
	if( last ) cmd[2]=0x80;
	cmd[3]=blknum;
	cmd[4]=datalen;

	memcpy( cmd+5, data, datalen );

	ret=scReaderSendAPDU( ri, ci, &apdu );
	memset( cmd, 0, sizeof(cmd) );
	if( ret!=SC_EXIT_OK ) return( ret );

	if( apdu.rsplen<2 ) return( SC_EXIT_BAD_SW );

	ci->sw[0] = apdu.rsp[apdu.rsplen-2];
	ci->sw[1] = apdu.rsp[apdu.rsplen-1];

	return( SC_EXIT_OK );
}

int scSmartcafeCmdPutKey( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	BYTE keyidx, const BYTE *key, BYTE keylen )
{
	BYTE cmd[ 5+SC_SMARTCAFE_MAX_KEYLEN ];
	BYTE rsp[ SC_GENERAL_SHORT_DATA_SIZE+2 ];
	SC_APDU apdu;
	int ret;

	ci->sw[0]=0x00; ci->sw[1]=0x00;

	if( (keylen!=8) && (keylen!=16) ) return( SC_EXIT_BAD_PARAM );

	apdu.cse=SC_APDU_CASE_3_SHORT;
	apdu.cmd=cmd;
	apdu.cmdlen=5+keylen;
	apdu.rsp=rsp;
	apdu.rsplen=0;

	memset( cmd, 0, sizeof( cmd ) );

	cmd[0]=0x80;
	cmd[1]=0xD8;
	cmd[3]=keyidx;
	cmd[4]=keylen;

	memcpy( cmd+5, key, keylen );

	ret=scReaderSendAPDU( ri, ci, &apdu );
	memset( cmd, 0, sizeof(cmd) );
	if( ret!=SC_EXIT_OK ) return( ret );

	if( apdu.rsplen<2 ) return( SC_EXIT_BAD_SW );

	ci->sw[0] = apdu.rsp[apdu.rsplen-2];
	ci->sw[1] = apdu.rsp[apdu.rsplen-1];

	return( SC_EXIT_OK );
}

int scSmartcafeCmdSelect( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	const BYTE *aid, BYTE aidlen, BYTE *resp, int *resplen )
{
	BYTE cmd[ 5+SC_SMARTCAFE_MAX_AIDLEN+1 ];
	BYTE rsp[ SC_GENERAL_SHORT_DATA_SIZE+2 ];
	SC_APDU apdu;
	int ret;

	ci->sw[0]=0x00; ci->sw[1]=0x00;

	if( aidlen>16 ) return( SC_EXIT_BAD_PARAM );

	apdu.cse=SC_APDU_CASE_4_SHORT;
	apdu.cmd=cmd;
	apdu.cmdlen=5+aidlen+1;
	apdu.rsp=rsp;
	apdu.rsplen=0;

	memset( cmd, 0, sizeof( cmd ) );

	cmd[0]=0x00;
	cmd[1]=0xA4;
	cmd[2]=0x04;
	cmd[4]=aidlen;

	memcpy( cmd+5, aid, aidlen );

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

int scSmartcafeCmdSetPIN( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	const BYTE *pin, BYTE pinlen )
{
	BYTE cmd[ 5+SC_SMARTCAFE_MAX_PINLEN ];
	BYTE rsp[ SC_GENERAL_SHORT_DATA_SIZE+2 ];
	SC_APDU apdu;
	int ret;

	ci->sw[0]=0x00; ci->sw[1]=0x00;

	if( pinlen>SC_SMARTCAFE_MAX_PINLEN ) return( SC_EXIT_BAD_PARAM );

	apdu.cse=SC_APDU_CASE_3_SHORT;
	apdu.cmd=cmd;
	apdu.cmdlen=5+pinlen;
	apdu.rsp=rsp;
	apdu.rsplen=0;

	memset( cmd, 0, sizeof( cmd ) );

	cmd[0]=0x80;
	cmd[1]=0x2A;
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

int scSmartcafeCmdVerifyPIN( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	const BYTE *pin, BYTE pinlen )
{
	BYTE cmd[ 5+SC_SMARTCAFE_MAX_PINLEN ];
	BYTE rsp[ SC_GENERAL_SHORT_DATA_SIZE+2 ];
	SC_APDU apdu;
	int ret;

	ci->sw[0]=0x00; ci->sw[1]=0x00;

	if( pinlen>SC_SMARTCAFE_MAX_PINLEN ) return( SC_EXIT_BAD_PARAM );

	apdu.cse=SC_APDU_CASE_3_SHORT;
	apdu.cmd=cmd;
	apdu.cmdlen=5+pinlen;
	apdu.rsp=rsp;
	apdu.rsplen=0;

	memset( cmd, 0, sizeof( cmd ) );

	cmd[0]=0x00;
	cmd[1]=0x20;
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

