/****************************************************************************
*																			*
*					SCEZ chipcard library - Cryptoflex routines				*
*						Copyright Matthias Bruestle 1999					*
*					For licensing, see the file COPYRIGHT					*
*																			*
****************************************************************************/

/* $Id: sccryptoflex.c 1617 2005-11-03 17:41:39Z laforge $ */

#include <scez/scinternal.h>
#include <scez/cards/sccryptoflex.h>

#include <stdio.h>
#include <string.h>

#if !defined(WINDOWS) && !defined(__palmos__)
#include <unistd.h>	/* for sleep */
#elif defined(__BORLANDC__)
#include <dos.h>	/* for sleep */
#elif defined(WINDOWS)
#include <windows.h>
#elif defined(__palmos__)
#define sleep(x) SysTaskDelay(x*sysTicksPerSecond)
/* #define memcpy(x,y,z) (MemMove(x,(VoidPtr)y,z) ? x : x) */
#endif

#if defined( HAVE_LIBDES )
#include <des.h>
#elif defined ( HAVE_LIBCRYPT )
#include <crypt/des.h>
#elif defined ( HAVE_LIBCRYPTO )
#include <openssl/des.h>
#endif /* HAVE_LIBDES */

/* Initialize card function pointer */

int scCryptoflexInit( SC_CARD_INFO *ci )
{
	ci->scGetCap=scCryptoflexGetCap;
	ci->scGetCardData=scCryptoflexGetCardData;
	ci->scSetFD=scCryptoflexSetFD;

	return( SC_EXIT_OK );
}

/* Capabilities */

int scCryptoflexGetCap( SC_CARD_INFO *ci, SC_CARD_CAP *cp )
{
	cp->n_fd=1;

	/* 9600 at 3.579MHz */
	cp->fd[0]=(((10L<<16)+372L)<<8)+1;

	return( SC_EXIT_OK );
}

/* Fill card data in ci */

int scCryptoflexGetCardData( SC_CARD_INFO *ci )
{
	BYTE header[] = { 0xC0, 0xC0, 0x00, 0x00, 0x00 };
	BYTE swok[] = { 0x01, 0x90 };
	BYTE swav[] = { 0x01, 0x61 };

	ci->protocol=SC_PROTOCOL_T0;
	ci->direct=TRUE;
	ci->t0.d=1;
	ci->t0.wi=10;
	ci->t0.wwt=30720;
	memcpy( ci->t0.getrsp, header, 5 );
	memcpy( ci->swok, swok, sizeof(swok) );
	memcpy( ci->swav, swav, sizeof(swav) );
	ci->memsize=0;

	return( SC_EXIT_OK );
}

/* Set F and D. */

int scCryptoflexSetFD( SC_READER_INFO *ri, SC_CARD_INFO *ci, LONG fd )
{
	if( (fd&0xFFFFFF)==((372L<<8)+1) ) return( SC_EXIT_OK );
	return( SC_EXIT_BAD_PARAM );
}

/* Generate Auth */

void scCryptoflexGenerateAuth( const BYTE *key, const BYTE *chall, BYTE *auth,
	BYTE algo )
{
#ifdef WITH_DES
	des_key_schedule schedule;
	des_key_schedule schedule2;
	des_cblock out;

	des_check_key=0;

	if( algo==SC_CRYPTOFLEX_ALGO_3DES ) {
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

BOOLEAN scCryptoflexCompareAuth( const BYTE *key, const BYTE *chall,
	const BYTE *auth, BYTE algo )
{
#ifdef WITH_DES
	des_key_schedule schedule;
	des_key_schedule schedule2;
	des_cblock out;

	des_check_key=0;

	if( algo==SC_CRYPTOFLEX_ALGO_3DES ) {
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

/* Generates MAC */

/* Data has to be the unmaced APDU.
 * Overall it can be at most 25 bytes.
 * Mac is the maced APDU.
 */

/* Try out Retail-MAC for 3DES (Suggestion from Matthias Gaertner):
 * 1) Get Cryptoflex Cards again (See note about Aladdin in README.)
 * 2) Compute MAC like with DES
 * 3) Decrypt MAC with key2
 * 4) Encrypt MAC with key1
 * Didn't work, so:
 * 5) Wait for answer from Aladdin.
 * 6) Should be normal 3DES-CBC.
 * 7) fsck!
 * 8) Somebody said it should be a Retail-MAC.
 * 9) Try agin.
 * 10) fsck!
 * 11) Ask Schlumberger Tech-Support.
 * 12) Got no response. fsck!
 * 13) The only thing which works with 3DES PRO commands is, as
 *     someone suggested, they are not supported. (But why 6300?)
 */

void scCryptoflexGenerateMAC( const BYTE *key, const BYTE *chall,
	const BYTE *data, int datalen, BYTE *mac, int *maclen, BYTE algo )
{
#ifdef WITH_DES
	des_key_schedule schedule;
	des_key_schedule schedule2;
	BYTE in[8];
	BYTE out[8];
	BYTE block[24+3];
	int i;
	int tmp;
	int pointer=0;

	/* Copy header to final APDU */
	memcpy( mac, data, 4 );

	/* Strip CLA and limit datalen */
	data++;
	datalen--;
	if( datalen>24 ) datalen=24;

	des_check_key=0;

	/* Do key schedule */
	if( algo==SC_CRYPTOFLEX_ALGO_3DES ) {
		des_set_key( (des_cblock *) key, schedule );
		des_set_key( (des_cblock *) (key+8), schedule2 );
	} else {
		des_set_key( (des_cblock *) key, schedule );
	}

	/* Fill block with 0xFF and then with the data. */
	memset( block, 0xFF, 24+3 );
	memcpy( block, data, datalen );

	/* Copy body to final APDU */
	tmp=(datalen+7)>>3;
	tmp=tmp<<3;
	memcpy( mac+5, block+3, tmp-3 );

	/* Set Lc field */
	mac[4]=(tmp-3+6) & 0xFF;

	/* Set maclen */
	*maclen=tmp-3+6+5;

	/* Copy challenge */
	memcpy( in, chall, 8 );

	/* Could have used the correct mode, but speed is not critical
	 * and a replacement if probably easier.
	 */
	while( datalen > 0 ) {
		for( i=0; i<8; i++ ) in[i]^=block[i+pointer];

		if( algo==SC_CRYPTOFLEX_ALGO_3DES ) {
			des_ecb2_encrypt( (des_cblock *)in, (des_cblock *)out, schedule,
				schedule2, DES_ENCRYPT );
		} else {
			des_ecb_encrypt( (des_cblock *)in, (des_cblock *)out, schedule,
				DES_ENCRYPT );
		}

		memcpy( in, out, 8 );

		pointer+=8;
		datalen-=8;
	}

	/* Copy cryptogram */
	memcpy( mac+5+tmp-3, out, 6 );

	memset( &schedule, 0, sizeof(schedule) );
	memset( &schedule2, 0, sizeof(schedule2) );
	memset( in, 0, sizeof(in) );
	memset( out, 0, sizeof(out) );
	memset( block, 0, sizeof(block) );
#else
	*maclen = 0;
#endif /* WITH_DES */
}


/* Commands */

int scCryptoflexCmdChangePIN( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	BYTE num, const BYTE *oldpin, const BYTE *newpin )
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

	if( ci->type==SC_CARD_CRYPTOFLEX_8K ) {
		switch( num ) {
		case 1:
		case 2:
			cmd[3]=num;
			break;
		default:
			return( SC_EXIT_BAD_PARAM );
		}
	}

	memcpy( cmd+5, oldpin, SC_CRYPTOFLEX_PIN_SIZE );
	memcpy( cmd+5+SC_CRYPTOFLEX_PIN_SIZE, newpin, SC_CRYPTOFLEX_PIN_SIZE );

	ret=scReaderSendAPDU( ri, ci, &apdu );
	memset( cmd, 0, sizeof(cmd) );
	if( ret!=SC_EXIT_OK ) return( ret );

	if( apdu.rsplen!=2 ) return( SC_EXIT_BAD_SW );

	ci->sw[0] = apdu.rsp[0];
	ci->sw[1] = apdu.rsp[1];

	return( SC_EXIT_OK );
}

int scCryptoflexCmdCreateFile( SC_READER_INFO *ri, SC_CARD_INFO *ci,
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

int scCryptoflexCmdCreateFileMAC( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	WORD fid, int flen, BYTE ftype, BYTE init, BYTE status, BYTE reclen,
	BYTE recnum, const BYTE *acond, const BYTE *akeys, const BYTE *key,
	const BYTE *chall, BYTE algo )
{
#ifdef WITH_DES
	BYTE cmd[22];
	BYTE mac[32];
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
	apdu.cmd=mac;
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
		scCryptoflexGenerateMAC( key, chall, cmd, 22, mac, &apdu.cmdlen, algo );
	} else {
		scCryptoflexGenerateMAC( key, chall, cmd, 21, mac, &apdu.cmdlen, algo );
	}

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

int scCryptoflexCmdCreateRecord( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	const BYTE *data, int datalen )
{
	BYTE cmd[ 5+255 ];
	BYTE rsp[ SC_GENERAL_SHORT_DATA_SIZE+2 ];
	SC_APDU apdu;
	int ret;

	ci->sw[0]=0x00; ci->sw[1]=0x00;

	datalen&=0xFF;

	apdu.cse=SC_APDU_CASE_3_SHORT;
	apdu.cmd=cmd;
	apdu.cmdlen=5 + datalen;
	apdu.rsp=rsp;
	apdu.rsplen=0;

	cmd[0]=0xC0;
	cmd[1]=0xE2;
	cmd[2]=0x00;
	cmd[3]=0x00;
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

int scCryptoflexCmdCreateRecordMAC( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	const BYTE *data, int datalen, const BYTE *key, const BYTE *chall,
	BYTE algo )
{
#ifdef WITH_DES
	BYTE cmd[ 25 ];
	BYTE mac[ 32 ];
	BYTE rsp[ SC_GENERAL_SHORT_DATA_SIZE+2 ];
	SC_APDU apdu;
	int ret;

	ci->sw[0]=0x00; ci->sw[1]=0x00;

	if( datalen>20 ) return( SC_EXIT_BAD_PARAM );

	apdu.cse=SC_APDU_CASE_3_SHORT;
	apdu.cmd=mac;
	apdu.rsp=rsp;
	apdu.rsplen=0;

	cmd[0]=0xC0;
	cmd[1]=0xE2;
	cmd[2]=0x00;
	cmd[3]=0x00;
	cmd[4]=datalen;

	memcpy( cmd+5, data, datalen );

	scCryptoflexGenerateMAC( key, chall, cmd, 5+datalen, mac, &apdu.cmdlen, algo );

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

int scCryptoflexCmdDecrease( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	const BYTE *amount, BYTE *resp, int *resplen )
{
	BYTE cmd[]={ 0xF0, 0x30, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x00 };
	BYTE rsp[ SC_GENERAL_SHORT_DATA_SIZE+2 ];
	SC_APDU apdu;
	int ret;

	ci->sw[0]=0x00; ci->sw[1]=0x00;

	apdu.cse=SC_APDU_CASE_4_SHORT;
	apdu.cmd=cmd;
	apdu.cmdlen=9;
	apdu.rsp=rsp;
	apdu.rsplen=0;

	memcpy( apdu.cmd+5, amount, 3 );

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

int scCryptoflexCmdDecreaseMAC( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	const BYTE *amount, const BYTE *key, const BYTE *chall, BYTE algo,
	BYTE *resp, int *resplen )
{
#ifdef WITH_DES
	BYTE cmd[]={ 0xF0, 0x30, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00 };
	BYTE mac[32];
	BYTE rsp[ SC_GENERAL_SHORT_DATA_SIZE+2 ];
	SC_APDU apdu;
	int ret;

	ci->sw[0]=0x00; ci->sw[1]=0x00;

	apdu.cse=SC_APDU_CASE_4_SHORT;
	apdu.cmd=mac;
	apdu.rsp=rsp;
	apdu.rsplen=0;

	memcpy( cmd+5, amount, 3 );

	scCryptoflexGenerateMAC( key, chall, cmd, 5+3, mac, &apdu.cmdlen, algo );

	apdu.cmd[apdu.cmdlen]=0x00;
	apdu.cmdlen++;

	ret=scReaderSendAPDU( ri, ci, &apdu );
	memset( cmd, 0, sizeof(cmd) );
	memset( mac, 0, sizeof(mac) );
	if( ret!=SC_EXIT_OK ) return( ret );

	if( apdu.rsplen<2 ) return( SC_EXIT_BAD_SW );

	ci->sw[0] = apdu.rsp[apdu.rsplen-2];
	ci->sw[1] = apdu.rsp[apdu.rsplen-1];

	apdu.rsplen-=2;

	memcpy( resp, apdu.rsp, apdu.rsplen );
	*resplen=apdu.rsplen;

	memset( rsp, 0, sizeof(rsp) );

	return( SC_EXIT_OK );
#else
	return( SC_EXIT_NOT_SUPPORTED );
#endif /* WITH_DES */
}

int scCryptoflexCmdDeleteFile( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	WORD fid )
{
	BYTE cmd[]={ 0xF0, 0xE4, 0x00, 0x00, 0x02, 0x00, 0x00 };
	BYTE rsp[ SC_GENERAL_SHORT_DATA_SIZE+2 ];
	SC_APDU apdu;
	int ret;

	ci->sw[0]=0x00; ci->sw[1]=0x00;

	apdu.cse=SC_APDU_CASE_3_SHORT;
	apdu.cmd=cmd;
	apdu.cmdlen=7;
	apdu.rsp=rsp;
	apdu.rsplen=0;

	cmd[5]=(fid >> 8) & 0xFF;
	cmd[6]=fid & 0xFF;

	ret=scReaderSendAPDU( ri, ci, &apdu );
	memset( cmd, 0, sizeof(cmd) );
	if( ret!=SC_EXIT_OK ) return( ret );

	if( apdu.rsplen!=2 ) return( SC_EXIT_BAD_SW );

	ci->sw[0] = apdu.rsp[0];
	ci->sw[1] = apdu.rsp[1];

	return( SC_EXIT_OK );
}

int scCryptoflexCmdDeleteFileMAC( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	WORD fid, const BYTE *key, const BYTE *chall, BYTE algo )
{
#ifdef WITH_DES
	BYTE cmd[]={ 0xF0, 0xE4, 0x00, 0x00, 0x02, 0x00, 0x00 };
	BYTE mac[32];
	BYTE rsp[ SC_GENERAL_SHORT_DATA_SIZE+2 ];
	SC_APDU apdu;
	int ret;

	ci->sw[0]=0x00; ci->sw[1]=0x00;

	apdu.cse=SC_APDU_CASE_3_SHORT;
	apdu.cmd=mac;
	apdu.rsp=rsp;
	apdu.rsplen=0;

	cmd[5]=(fid >> 8) & 0xFF;
	cmd[6]=fid & 0xFF;

	scCryptoflexGenerateMAC( key, chall, cmd, 5+2, mac, &apdu.cmdlen, algo );

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

/* Changed in newer Cryptoflex 8K cards. */

int scCryptoflexCmdDirectory( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	BYTE *resp, int *resplen )
{
	BYTE cmd[]={ 0xF0, 0xA8, 0x00, 0x00, 0x00, 0x00 };
	BYTE rsp[ SC_GENERAL_SHORT_DATA_SIZE+2 ];
	SC_APDU apdu;
	int ret;

	ci->sw[0]=0x00; ci->sw[1]=0x00;

	apdu.cse=SC_APDU_CASE_4_SHORT;
	apdu.cmd=cmd;
	apdu.cmdlen=6;
	apdu.rsp=rsp;
	apdu.rsplen=0;

	if( ci->type==SC_CARD_CRYPTOFLEX_8K ) {
		apdu.cse=SC_APDU_CASE_2_SHORT;
		apdu.cmdlen=5;
		cmd[4]=0x0F;
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

int scCryptoflexCmdExtAuth( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	BYTE keynum, const BYTE *key, const BYTE *chall, BYTE algo )
{
#ifdef WITH_DES
	BYTE cmd[]={ 0xC0, 0x82, 0x00, 0x00, 0x07, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00 };
	BYTE rsp[ SC_GENERAL_SHORT_DATA_SIZE+2 ];
	BYTE mac[ 6 ];
	SC_APDU apdu;
	int ret;

	ci->sw[0]=0x00; ci->sw[1]=0x00;

	scCryptoflexGenerateAuth( key, chall, mac, algo );

	apdu.cse=SC_APDU_CASE_3_SHORT;
	apdu.cmd=cmd;
	apdu.cmdlen=12;
	apdu.rsp=rsp;
	apdu.rsplen=0;

	cmd[5]=keynum;

	memcpy( cmd+5+1, mac, SC_CRYPTOFLEX_MAC_SIZE );

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

int scCryptoflexCmdGetChall( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	BYTE *chall, int *len )
{
	BYTE cmd[]={ 0xC0, 0x84, 0x00, 0x00, 0x00 };
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
int scCryptoflexCmdGetResp( SC_READER_INFO *ri, SC_CARD_INFO *ci,
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

int scCryptoflexCmdIncrease( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	const BYTE *amount, BYTE *resp, int *resplen )
{
	BYTE cmd[]={ 0xF0, 0x32, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x00 };
	BYTE rsp[ SC_GENERAL_SHORT_DATA_SIZE+2 ];
	SC_APDU apdu;
	int ret;

	ci->sw[0]=0x00; ci->sw[1]=0x00;

	apdu.cse=SC_APDU_CASE_4_SHORT;
	apdu.cmd=cmd;
	apdu.cmdlen=9;
	apdu.rsp=rsp;
	apdu.rsplen=0;

	memcpy( cmd+5, amount, 3 );

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

int scCryptoflexCmdIncreaseMAC( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	const BYTE *amount, const BYTE *key, const BYTE *chall, BYTE algo,
	BYTE *resp, int *resplen )
{
#ifdef WITH_DES
	BYTE cmd[]={ 0xF0, 0x32, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00 };
	BYTE mac[32];
	BYTE rsp[ SC_GENERAL_SHORT_DATA_SIZE+2 ];
	SC_APDU apdu;
	int ret;

	ci->sw[0]=0x00; ci->sw[1]=0x00;

	apdu.cse=SC_APDU_CASE_4_SHORT;
	apdu.cmd=mac;
	apdu.rsp=rsp;
	apdu.rsplen=0;

	memcpy( cmd+5, amount, 3 );

	scCryptoflexGenerateMAC( key, chall, cmd, 5+3, mac, &apdu.cmdlen, algo );

	apdu.cmd[apdu.cmdlen]=0x00;
	apdu.cmdlen++;

	ret=scReaderSendAPDU( ri, ci, &apdu );
	memset( cmd, 0, sizeof(cmd) );
	memset( mac, 0, sizeof(mac) );
	if( ret!=SC_EXIT_OK ) return( ret );

	if( apdu.rsplen<2 ) return( SC_EXIT_BAD_SW );

	ci->sw[0] = apdu.rsp[apdu.rsplen-2];
	ci->sw[1] = apdu.rsp[apdu.rsplen-1];

	apdu.rsplen-=2;

	memcpy( resp, apdu.rsp, apdu.rsplen );
	*resplen=apdu.rsplen;

	memset( rsp, 0, sizeof(rsp) );

	return( SC_EXIT_OK );
#else
	return( SC_EXIT_NOT_SUPPORTED );
#endif /* WITH_DES */
}

int scCryptoflexCmdIntAuth( SC_READER_INFO *ri, SC_CARD_INFO *ci,
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

	if( (ci->type==SC_CARD_CRYPTOFLEX_DES) ||
		(ci->type==SC_CARD_CRYPTOFLEX_KEYGEN) ||
		(ci->type==SC_CARD_MULTIFLEX_8K_DES) )
		cmd[2]=0x01;

	cmd[3]=keynum;

	memcpy( cmd+5, chall, SC_CRYPTOFLEX_CHALL_SIZE );

	ret=scReaderSendAPDU( ri, ci, &apdu );
	memset( cmd, 0, sizeof(cmd) );
	if( ret!=SC_EXIT_OK ) return( ret );

	if( apdu.rsplen<2 ) return( SC_EXIT_BAD_SW );

	ci->sw[0] = apdu.rsp[apdu.rsplen-2];
	ci->sw[1] = apdu.rsp[apdu.rsplen-1];

	if( (apdu.rsplen>=8) && (scCryptoflexCompareAuth( key, chall, rsp,
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

int scCryptoflexCmdInvalidate( SC_READER_INFO *ri, SC_CARD_INFO *ci )
{
	BYTE cmd[]={ 0xF0, 0x04, 0x00, 0x00, 0x00 };
	BYTE rsp[ SC_GENERAL_SHORT_DATA_SIZE+2 ];
	SC_APDU apdu;
	int ret;

	ci->sw[0]=0x00; ci->sw[1]=0x00;

	apdu.cse=SC_APDU_CASE_1;
	apdu.cmd=cmd;
	apdu.cmdlen=5;
	apdu.rsp=rsp;
	apdu.rsplen=0;

	ret=scReaderSendAPDU( ri, ci, &apdu );
	memset( cmd, 0, sizeof(cmd) );
	if( ret!=SC_EXIT_OK ) return( ret );

	if( apdu.rsplen!=2 ) return( SC_EXIT_BAD_SW );

	ci->sw[0] = apdu.rsp[0];
	ci->sw[1] = apdu.rsp[1];

	return( SC_EXIT_OK );
}

int scCryptoflexCmdInvalidateMAC( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	const BYTE *key, const BYTE *chall, BYTE algo )
{
#ifdef WITH_DES
	BYTE cmd[]={ 0xF0, 0x04, 0x00, 0x00, 0x00 };
	BYTE mac[32];
	BYTE rsp[ SC_GENERAL_SHORT_DATA_SIZE+2 ];
	SC_APDU apdu;
	int ret;

	ci->sw[0]=0x00; ci->sw[1]=0x00;

	apdu.cse=SC_APDU_CASE_3_SHORT;
	apdu.cmd=mac;
	apdu.rsp=rsp;
	apdu.rsplen=0;

	scCryptoflexGenerateMAC( key, chall, cmd, 5, mac, &apdu.cmdlen, algo );

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

int scCryptoflexCmdReadBinary( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	int offset, BYTE *data, int *datalen )
{
	BYTE cmd[]={ 0xC0, 0xB0, 0x00, 0x00, 0x00 };
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
	offset &= 0xFFFF;

	cmd[2] = (offset >>8) & 0xFF;
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

int scCryptoflexCmdReadRecord( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	BYTE recnum, BYTE mode, BYTE *data, int *datalen )
{
	BYTE cmd[]={ 0xC0, 0xB2, 0x00, 0x00, 0x00 };
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

int scCryptoflexCmdRehabilitate( SC_READER_INFO *ri, SC_CARD_INFO *ci )
{
	BYTE cmd[]={ 0xF0, 0x44, 0x00, 0x00, 0x00 };
	BYTE rsp[ SC_GENERAL_SHORT_DATA_SIZE+2 ];
	SC_APDU apdu;
	int ret;

	ci->sw[0]=0x00; ci->sw[1]=0x00;

	apdu.cse=SC_APDU_CASE_1;
	apdu.cmd=cmd;
	apdu.cmdlen=5;
	apdu.rsp=rsp;
	apdu.rsplen=0;

	ret=scReaderSendAPDU( ri, ci, &apdu );
	memset( cmd, 0, sizeof(cmd) );
	if( ret!=SC_EXIT_OK ) return( ret );

	if( apdu.rsplen!=2 ) return( SC_EXIT_BAD_SW );

	ci->sw[0] = apdu.rsp[0];
	ci->sw[1] = apdu.rsp[1];

	return( SC_EXIT_OK );
}

int scCryptoflexCmdRehabilitateMAC( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	const BYTE *key, const BYTE *chall, BYTE algo )
{
#ifdef WITH_DES
	BYTE cmd[]={ 0xF0, 0x44, 0x00, 0x00, 0x00 };
	BYTE mac[32];
	BYTE rsp[ SC_GENERAL_SHORT_DATA_SIZE+2 ];
	SC_APDU apdu;
	int ret;

	ci->sw[0]=0x00; ci->sw[1]=0x00;

	apdu.cse=SC_APDU_CASE_3_SHORT;
	apdu.cmd=mac;
	apdu.rsp=rsp;
	apdu.rsplen=0;

	scCryptoflexGenerateMAC( key, chall, cmd, 5, mac, &apdu.cmdlen, algo );

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

int scCryptoflexCmdSeek( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	BYTE offset, BYTE mode, const BYTE *pattern, BYTE patlen )
{
	BYTE cmd[ SC_GENERAL_SHORT_DATA_SIZE+5 ];
	BYTE rsp[ SC_GENERAL_SHORT_DATA_SIZE+2 ];
	SC_APDU apdu;
	int ret;

	ci->sw[0]=0x00; ci->sw[1]=0x00;

	apdu.cse=SC_APDU_CASE_3_SHORT;
	apdu.cmd=cmd;
	apdu.cmdlen=5+patlen;
	apdu.rsp=rsp;
	apdu.rsplen=0;

	cmd[0]=0xF0;
	cmd[1]=0xA2;
	cmd[2]=offset;
	cmd[3]=mode;
	cmd[4]=patlen;

	memcpy( cmd+5, pattern, patlen );

	ret=scReaderSendAPDU( ri, ci, &apdu );
	memset( cmd, 0, sizeof(cmd) );
	if( ret!=SC_EXIT_OK ) return( ret );

	if( apdu.rsplen!=2 ) return( SC_EXIT_BAD_SW );

	ci->sw[0] = apdu.rsp[0];
	ci->sw[1] = apdu.rsp[1];

	return( SC_EXIT_OK );
}

int scCryptoflexCmdSelectFile( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	WORD fid, BYTE *resp, int *resplen )
{
	BYTE cmd[]={ 0xC0, 0xA4, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00 };
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

int scCryptoflexCmdUnblockPIN( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	BYTE num, const BYTE *unblock, const BYTE *newpin )
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
	apdu.cmdlen=5+SC_CRYPTOFLEX_PIN_SIZE+SC_CRYPTOFLEX_PIN_SIZE;
	apdu.rsp=rsp;
	apdu.rsplen=0;

	if( ci->type==SC_CARD_CRYPTOFLEX_8K ) {
		switch( num ) {
		case 1:
		case 2:
			cmd[3]=num;
			break;
		default:
			return( SC_EXIT_BAD_PARAM );
		}
	}

	memcpy( cmd+5, unblock, SC_CRYPTOFLEX_PIN_SIZE );
	memcpy( cmd+5+SC_CRYPTOFLEX_PIN_SIZE, newpin, SC_CRYPTOFLEX_PIN_SIZE );

	ret=scReaderSendAPDU( ri, ci, &apdu );
	memset( cmd, 0, sizeof(cmd) );
	if( ret!=SC_EXIT_OK ) return( ret );

	if( apdu.rsplen!=2 ) return( SC_EXIT_BAD_SW );

	ci->sw[0] = apdu.rsp[0];
	ci->sw[1] = apdu.rsp[1];

	return( SC_EXIT_OK );
}

int scCryptoflexCmdUpdateBinary( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	int offset, const BYTE *data, BYTE datalen )
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

	cmd[0]=0xC0;
	cmd[1]=0xD6;
	cmd[2]=(offset >>8) & 0xFF;
	cmd[3]=offset & 0xFF;
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

int scCryptoflexCmdUpdateBinaryMAC( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	int offset, const BYTE *data, BYTE datalen, const BYTE *key,
	const BYTE *chall, BYTE algo )
{
#ifdef WITH_DES
	BYTE cmd[ SC_GENERAL_SHORT_DATA_SIZE+5 ];
	BYTE mac[32];
	BYTE rsp[ SC_GENERAL_SHORT_DATA_SIZE+2 ];
	SC_APDU apdu;
	int ret;

	ci->sw[0]=0x00; ci->sw[1]=0x00;

	if( datalen>20 ) return( SC_EXIT_BAD_PARAM );

	apdu.cse=SC_APDU_CASE_3_SHORT;
	apdu.cmd=mac;
	apdu.rsp=rsp;
	apdu.rsplen=0;

	cmd[0]=0xC0;
	cmd[1]=0xD6;
	cmd[2]=(offset >>8) & 0xFF;
	cmd[3]=offset & 0xFF;
	cmd[4]=datalen;

	memcpy( cmd+5, data, datalen );

	scCryptoflexGenerateMAC( key, chall, cmd, 5+datalen, mac, &apdu.cmdlen, algo );

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

int scCryptoflexCmdUpdateRecord( SC_READER_INFO *ri, SC_CARD_INFO *ci,
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

	cmd[0]=0xC0;
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

int scCryptoflexCmdUpdateRecordMAC( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	BYTE recnum, BYTE mode, const BYTE *data, BYTE datalen, const BYTE *key,
	const BYTE *chall, BYTE algo )
{
#ifdef WITH_DES
	BYTE cmd[ SC_GENERAL_SHORT_DATA_SIZE+5 ];
	BYTE mac[32];
	BYTE rsp[ SC_GENERAL_SHORT_DATA_SIZE+2 ];
	SC_APDU apdu;
	int ret;

	ci->sw[0]=0x00; ci->sw[1]=0x00;

	if( datalen>20 ) return( SC_EXIT_BAD_PARAM );

	apdu.cse=SC_APDU_CASE_3_SHORT;
	apdu.cmd=mac;
	apdu.rsp=rsp;
	apdu.rsplen=0;

	cmd[0]=0xC0;
	cmd[1]=0xDC;
	cmd[2]=recnum;
	cmd[3]=mode;
	cmd[4]=datalen;

	memcpy( cmd+5, data, datalen );

	scCryptoflexGenerateMAC( key, chall, cmd, 5+datalen, mac, &apdu.cmdlen, algo );

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

int scCryptoflexCmdVerifyKey( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	BYTE keynum, const BYTE *key, BYTE keylen )
{
	BYTE cmd[ SC_GENERAL_SHORT_DATA_SIZE+5 ];
	BYTE rsp[ SC_GENERAL_SHORT_DATA_SIZE+2 ];
	SC_APDU apdu;
	int ret;

	ci->sw[0]=0x00; ci->sw[1]=0x00;

	apdu.cse=SC_APDU_CASE_3_SHORT;
	apdu.cmd=cmd;
	apdu.cmdlen=5+keylen;
	apdu.rsp=rsp;
	apdu.rsplen=0;

	cmd[0]=0xF0;
	cmd[1]=0x2A;
	cmd[2]=0x00;
	cmd[3]=keynum & 0x0F;
	cmd[4]=keylen;

	memcpy( cmd+5, key, keylen );

	ret=scReaderSendAPDU( ri, ci, &apdu );
	memset( cmd, 0, sizeof(cmd) );
	if( ret!=SC_EXIT_OK ) return( ret );

	if( apdu.rsplen!=2 ) return( SC_EXIT_BAD_SW );

	ci->sw[0] = apdu.rsp[0];
	ci->sw[1] = apdu.rsp[1];

	return( SC_EXIT_OK );
}

int scCryptoflexCmdVerifyPIN( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	BYTE num, const BYTE *pin )
{
	BYTE cmd[]={ 0xC0, 0x20, 0x00, 0x01, 0x08, 0x00, 0x00, 0x00, 0x00,
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

	if( ci->type==SC_CARD_CRYPTOFLEX_8K ) {
		switch( num ) {
		case 1:
		case 2:
			cmd[3]=num;
			break;
		default:
			return( SC_EXIT_BAD_PARAM );
		}
	}

	memcpy( cmd+5, pin, SC_CRYPTOFLEX_PIN_SIZE );

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
*							Cryptography Card Commands						*
*																			*
****************************************************************************/

/* Full DES IntAuth */

int scCryptoflexCmdDesCrypt( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	BYTE keynum, BYTE mode, const BYTE *data, BYTE *resp, int *resplen )
{
	BYTE cmd[]={ 0xC0, 0x88, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
	BYTE rsp[ SC_GENERAL_SHORT_DATA_SIZE+2 ];
	SC_APDU apdu;
	int ret;

	ci->sw[0]=0x00; ci->sw[1]=0x00;

	if( ci->type==SC_CARD_CRYPTOFLEX ) return( SC_EXIT_NOT_SUPPORTED );

	apdu.cse=SC_APDU_CASE_4_SHORT;
	apdu.cmd=cmd;
	apdu.cmdlen=14;
	apdu.rsp=rsp;
	apdu.rsplen=0;

	if( (mode!=SC_CRYPTOFLEX_MODE_DECRYPT) &&
		(mode!=SC_CRYPTOFLEX_MODE_ENCRYPT) )
		return( SC_EXIT_BAD_PARAM );

	cmd[2]=mode;
	cmd[3]=keynum;

	memcpy( cmd+5, data, SC_CRYPTOFLEX_DES_SIZE );

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

int scCryptoflexCmdLoadCert( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	BYTE keynum, const BYTE *data )
{
	BYTE cmd[ 0x85 ];
	BYTE rsp[ SC_GENERAL_SHORT_DATA_SIZE+2 ];
	SC_APDU apdu;
	int ret;

	ci->sw[0]=0x00; ci->sw[1]=0x00;

	apdu.cse=SC_APDU_CASE_3_SHORT;
	apdu.cmd=cmd;
	apdu.cmdlen=0x85;
	apdu.rsp=rsp;
	apdu.rsplen=0;

	memset( cmd, 0, sizeof(cmd) );

	cmd[0]=0xF0;
	cmd[1]=0x84;
	cmd[3]=keynum;
	cmd[4]=0x80;	/* RSA key length */

	memcpy( cmd+5, data, SC_CRYPTOFLEX_RSA_SIZE );

	ret=scReaderSendAPDU( ri, ci, &apdu );
	memset( cmd, 0, sizeof(cmd) );
	if( ret!=SC_EXIT_OK ) return( ret );

	if( apdu.rsplen!=2 ) return( SC_EXIT_BAD_SW );

	ci->sw[0] = apdu.rsp[0];
	ci->sw[1] = apdu.rsp[1];

	return( SC_EXIT_OK );
}

/* Example for a key generated by a Cryptoflex:
 *
 * n:
 *   F7 0B 95 9D CC A1 3A CA 2D BD FA 56 A5 5C BE F8
 *   BE 15 10 01 09 E6 73 52 A1 EC B0 C8 47 B1 F9 E6
 *   88 2B F5 40 7B DB E0 49 21 1D EA 69 B9 38 B6 E5
 *   F9 4A 00 7D 0B 3C 65 1C 00 EA 40 45 00 B5 F1 6D
 *   DE 40 01 EA C2 B6 B3 A4 21 BE AD 93 61 0C 78 6B
 *   3A FA D2 79 F0 B0 C7 86 B0 22 40 41 B1 68 DE 8A
 *   37 5A 92 6E 70 0F 26 AB 63 C2 87 EE 7A F0 79 90
 *   1A A1 31 66 E7 7C F3 28 6A 24 1D C2 09 87 9D CE
 *
 * key file:
 *   01 43 01
 *   DD B4 D3 D0 AF 31 4F 4B 0C AC 88 70 6A 97 FE A5
 *   DB F2 17 D5 C7 4C 64 70 45 AC EC 85 EC 59 81 88
 *   A3 B6 93 3C 9A 5A 5A 8F 1B 8E A8 2C C5 B1 5A AD
 *   7C 2B 82 76 45 E1 46 87 C4 05 AB 3E 88 69 62 EB
 *   E3 9C 69 B5 B0 AA 05 55 92 E0 04 8A FB 38 0F 2A
 *   A4 70 40 1A 1B EB 56 D5 1B 4D 61 EB 50 3D C9 4C
 *   54 7D 23 D7 4A DB 55 7F CF CA A7 AD 96 9D CE 23
 *   69 8B D4 81 EF 34 B2 4A E2 27 65 85 5F 14 B6 E0
 *   16 60 26 B2 F8 D9 52 17 F7 2C 0E D7 6F 8E 6D 04
 *   05 E2 31 C8 9F 76 0A 92 51 3F 2F 69 D4 D7 C0 9A
 *   DD 8D 35 BE BA 39 C7 55 DA EB 08 9F 1C 45 C2 A4
 *   6C 27 77 46 C2 F3 53 DF 59 E5 03 52 C2 1E 3A A9
 *   21 68 CD 04 97 8B 41 44 FA 5F EF B5 3E 5B 50 94
 *   D6 0B 91 F4 3E FD AC 8B 45 F3 BB 78 CA 2D 52 00
 *   C2 A6 E9 D9 FB 96 42 01 C3 FE E2 60 DB 0F 96 6D
 *   FA D6 F6 33 F6 E8 88 E6 8F 48 7B 70 85 B2 10 E6
 *   63 A6 A6 EB 95 E5 B0 43 92 7A 36 7D 4D A2 DB ED
 *   CA C5 E8 19 72 3A 2A E8 D1 71 FE 40 25 25 11 7A
 *   E2 D0 5E 08 64 53 74 C5 DA 08 AC 58 7C B6 D4 05
 *   26 CB 3A F0 AD E0 98 D9 02 A6 BA E2 4B 62 CB 2A
 *   00 00 00
 */

int scCryptoflexCmdRsaKeyGen( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	BYTE keynum, BYTE *resp, int *resplen )
{
	BYTE cmd[]={ 0xF0, 0x14, 0x00, 0x00, 0x00 };
	BYTE rsp[ SC_GENERAL_SHORT_DATA_SIZE+2 ];
	SC_APDU apdu;
	int ret;

	ci->sw[0]=0x00; ci->sw[1]=0x00;

	if( ci->type!=SC_CARD_CRYPTOFLEX_KEYGEN ) return( SC_EXIT_NOT_SUPPORTED );

	apdu.cse=SC_APDU_CASE_1;
	apdu.cmd=cmd;
	apdu.cmdlen=5;
	apdu.rsp=rsp;
	apdu.rsplen=0;

	cmd[3]=keynum;

	ret=scReaderSendAPDU( ri, ci, &apdu );
	memset( cmd, 0, sizeof(cmd) );

	if( (ret!=SC_EXIT_OK) && (ret!=SC_EXIT_PROTOCOL_ERROR) &&
		(ret!=SC_EXIT_TIMEOUT) && (ret!=SC_EXIT_IO_ERROR) ) return( ret );

	/* Wait 4 minutes for key generation. */
#if defined( __WIN32__ ) || defined( __WIN16__ )
	Sleep(240000);
#else
	sleep(240);
#endif

	ci->sw[1]=0x80;

	return( scCryptoflexCmdGetResp( ri, ci, resp, resplen ) );
}

/* The 4 byte exponent has to be LSB as every other big integer. */

int scCryptoflexCmdRsaKeyGenUpd( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	BYTE keysize, BYTE *exp )
{
	BYTE cmd[]={ 0xF0, 0x46, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x00 };
	BYTE rsp[ SC_GENERAL_SHORT_DATA_SIZE+2 ];
	SC_APDU apdu;
	int ret;

	ci->sw[0]=0x00; ci->sw[1]=0x00;

	if( ci->type!=SC_CARD_CRYPTOFLEX_8K ) return( SC_EXIT_NOT_SUPPORTED );

	apdu.cse=SC_APDU_CASE_3_SHORT;
	apdu.cmd=cmd;
	apdu.cmdlen=5+4;
	apdu.rsp=rsp;
	apdu.rsplen=0;

	cmd[3]=keysize;

	memcpy( cmd+5, exp, 4 );

	ret=scReaderSendAPDU( ri, ci, &apdu );
	memset( cmd, 0, sizeof(cmd) );
	if( ret!=SC_EXIT_OK ) return( ret );

	if( apdu.rsplen<2 ) return( SC_EXIT_BAD_SW );

	ci->sw[0] = apdu.rsp[apdu.rsplen-2];
	ci->sw[1] = apdu.rsp[apdu.rsplen-1];

	return( SC_EXIT_OK );
}

/* IntAuth with RSA Key */

int scCryptoflexCmdRsaSign( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	BYTE keynum, const BYTE *data, BYTE *resp, int *resplen )
{
	BYTE cmd[ 0x86 ];
	BYTE rsp[ SC_GENERAL_SHORT_DATA_SIZE+2 ];
	SC_APDU apdu;
	int ret;

	ci->sw[0]=0x00; ci->sw[1]=0x00;

	apdu.cse=SC_APDU_CASE_4_SHORT;
	apdu.cmd=cmd;
	apdu.cmdlen=0x86;
	apdu.rsp=rsp;
	apdu.rsplen=0;

	memset( cmd, 0, sizeof(cmd) );

	cmd[0]=0xC0;
	cmd[1]=0x88;
	cmd[3]=keynum;
	cmd[4]=0x80;	/* RSA key length */

	memcpy( cmd+5, data, SC_CRYPTOFLEX_RSA_SIZE );

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

int scCryptoflexCmdSHA1Interm( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	const BYTE *data )
{
	BYTE cmd[ 5+64 ];
	BYTE rsp[ SC_GENERAL_SHORT_DATA_SIZE+2 ];
	SC_APDU apdu;
	int ret;

	ci->sw[0]=0x00; ci->sw[1]=0x00;

	apdu.cse=SC_APDU_CASE_3_SHORT;
	apdu.cmd=cmd;
	apdu.cmdlen=5+64;
	apdu.rsp=rsp;
	apdu.rsplen=0;

	memset( cmd, 0, sizeof(cmd) );

	cmd[0]=0x14;
	cmd[1]=0x40;
	cmd[4]=0x40;

	memcpy( cmd+5, data, 0x40 );

	ret=scReaderSendAPDU( ri, ci, &apdu );
	memset( cmd, 0, sizeof(cmd) );
	if( ret!=SC_EXIT_OK ) return( ret );

	if( apdu.rsplen<2 ) return( SC_EXIT_BAD_SW );

	ci->sw[0] = apdu.rsp[apdu.rsplen-2];
	ci->sw[1] = apdu.rsp[apdu.rsplen-1];

	return( SC_EXIT_OK );
}

int scCryptoflexCmdSHA1Last( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	const BYTE *data, int datalen, BYTE *resp, int *resplen )
{
	BYTE cmd[ 5+64 ];
	BYTE rsp[ SC_GENERAL_SHORT_DATA_SIZE+2 ];
	SC_APDU apdu;
	int ret;

	ci->sw[0]=0x00; ci->sw[1]=0x00;

	if( datalen>64 ) return( SC_EXIT_BAD_PARAM );

	apdu.cse=SC_APDU_CASE_4_SHORT;
	apdu.cmd=cmd;
	apdu.cmdlen=5+datalen;
	apdu.rsp=rsp;
	apdu.rsplen=0;

	memset( cmd, 0, sizeof(cmd) );

	cmd[0]=0x04;
	cmd[1]=0x40;
	cmd[4]=datalen;

	memcpy( cmd+5, data, datalen );

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

int scCryptoflexCmdUpdateEnc( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	int offset, const BYTE *data, BYTE datalen, const BYTE *key, BYTE algo )
{
#ifdef WITH_DES
	BYTE cmd[ SC_GENERAL_SHORT_DATA_SIZE+5 ];
	BYTE rsp[ SC_GENERAL_SHORT_DATA_SIZE+2 ];
	SC_APDU apdu;
	des_key_schedule ks1;
	des_key_schedule ks2;
	int ret, i;

	ci->sw[0]=0x00; ci->sw[1]=0x00;

	if( (datalen&0x07)||(datalen>248) ) return( SC_EXIT_BAD_PARAM );

	apdu.cse=SC_APDU_CASE_3_SHORT;
	apdu.cmd=cmd;
	apdu.cmdlen=5+datalen;
	apdu.rsp=rsp;
	apdu.rsplen=0;

	memset( cmd, 0, sizeof(cmd) );

	cmd[0]=0xC0;
	cmd[1]=0xDE;
	cmd[2]=(offset >>8) & 0xFF;
	cmd[3]=offset & 0xFF;
	cmd[4]=datalen;

	memcpy( cmd+5, data, datalen );

	des_check_key=0;

	if( algo==SC_CRYPTOFLEX_ALGO_DES ) {
		des_set_key( (des_cblock *)key, ks1 );
		for( i=0; i<datalen; i+=8 ) {
			des_ecb_encrypt( (des_cblock *)(data+i), (des_cblock *)(cmd+5+i),
				ks1, DES_ENCRYPT );
		}
	} else if( algo==SC_CRYPTOFLEX_ALGO_3DES ) {
		des_set_key( (des_cblock *)key, ks1 );
		des_set_key( (des_cblock *)(key+8), ks2 );
		for( i=0; i<datalen; i+=8 ) {
			des_ecb2_encrypt( (des_cblock *)(data+i), (des_cblock *)(cmd+5+i),
				ks1, ks2, DES_ENCRYPT );
		}
	} else {
		return( SC_EXIT_NOT_SUPPORTED );
	}

	memset( &ks1, 0, sizeof(ks1) );
	memset( &ks2, 0, sizeof(ks2) );

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

int scCryptoflexCmdVerifyData( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	BYTE keynum, const BYTE *data, BYTE *resp, int *resplen )
{
	BYTE cmd[ 0x86 ];
	BYTE rsp[ SC_GENERAL_SHORT_DATA_SIZE+2 ];
	SC_APDU apdu;
	int ret;

	ci->sw[0]=0x00; ci->sw[1]=0x00;

	apdu.cse=SC_APDU_CASE_4_SHORT;
	apdu.cmd=cmd;
	apdu.cmdlen=0x86;
	apdu.rsp=rsp;
	apdu.rsplen=0;

	memset( cmd, 0, sizeof(cmd) );

	cmd[0]=0xF0;
	cmd[1]=0x82;
	cmd[3]=keynum;
	cmd[4]=0x80;	/* RSA key length */

	memcpy( cmd+5, data, SC_CRYPTOFLEX_RSA_SIZE );

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

int scCryptoflexCmdVerifyPubKey( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	const BYTE *data )
{
	BYTE cmd[ 0x85 ];
	BYTE rsp[ SC_GENERAL_SHORT_DATA_SIZE+2 ];
	SC_APDU apdu;
	int ret;

	ci->sw[0]=0x00; ci->sw[1]=0x00;

	apdu.cse=SC_APDU_CASE_3_SHORT;
	apdu.cmd=cmd;
	apdu.cmdlen=0x85;
	apdu.rsp=rsp;
	apdu.rsplen=0;

	memset( cmd, 0, sizeof(cmd) );

	cmd[0]=0xF0;
	cmd[1]=0x86;
	cmd[4]=0x80;	/* RSA key length */

	memcpy( cmd+5, data, SC_CRYPTOFLEX_RSA_SIZE );

	ret=scReaderSendAPDU( ri, ci, &apdu );
	memset( cmd, 0, sizeof(cmd) );
	if( ret!=SC_EXIT_OK ) return( ret );

	if( apdu.rsplen!=2 ) return( SC_EXIT_BAD_SW );

	ci->sw[0] = apdu.rsp[0];
	ci->sw[1] = apdu.rsp[1];

	return( SC_EXIT_OK );
}

