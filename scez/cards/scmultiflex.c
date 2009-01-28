/****************************************************************************
*																			*
*					SCEZ chipcard library - Multiflex routines				*
*						Copyright Matthias Bruestle 1999					*
*					For licensing, see the file COPYRIGHT					*
*																			*
****************************************************************************/

/* $Id: scmultiflex.c 1617 2005-11-03 17:41:39Z laforge $ */

#include <scez/scinternal.h>
#include <scez/cards/scmultiflex.h>
#include <scez/cards/sccryptoflex.h>

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

int scMultiflexInit( SC_CARD_INFO *ci )
{
	ci->scGetCap=scMultiflexGetCap;
	ci->scGetCardData=scMultiflexGetCardData;
	ci->scSetFD=scMultiflexSetFD;

	return( SC_EXIT_OK );
}

/* Capabilities */

int scMultiflexGetCap( SC_CARD_INFO *ci, SC_CARD_CAP *cp )
{
	if( ci->type==SC_CARD_MULTIFLEX_3K ) {
		cp->n_fd=1;

		/* 9600 at 3.579MHz */
		cp->fd[0]=(((10L<<16)+372L)<<8)+1;
	} else {
		cp->n_fd=5;

		/* 9600 at 3.579MHz */
		cp->fd[0]=(((10L<<16)+372L)<<8)+1;

		/* 19200 at 3.579MHz */
		cp->fd[1]=(((19L<<16)+372L)<<8)+2;

		/* 38400 at 3.579MHz */
		cp->fd[2]=(((38L<<16)+372L)<<8)+4;

		/* 76800 at 3.579MHz */
		cp->fd[3]=(((77L<<16)+372L)<<8)+8;

		/* 153600 at 3.579MHz */
		cp->fd[4]=(((154L<<16)+372L)<<8)+16;
	}

	return( SC_EXIT_OK );
}

/* Fill card data in ci */

int scMultiflexGetCardData( SC_CARD_INFO *ci )
{
	BYTE header[] = { 0xC0, 0xC0, 0x00, 0x00, 0x00 };
	BYTE swok[] = { 0x01, 0x90 };
	BYTE swav[] = { 0x01, 0x61 };

	ci->protocol=SC_PROTOCOL_T0;
	ci->direct=TRUE;
	ci->t0.d=1;
	ci->t0.wi=10;
	ci->t0.wwt=9600;
	memcpy( ci->t0.getrsp, header, 5 );
	memcpy( ci->swok, swok, sizeof(swok) );
	memcpy( ci->swav, swav, sizeof(swav) );
	ci->memsize=0;

	return( SC_EXIT_OK );
}

/* Set F and D. */

int scMultiflexSetFD( SC_READER_INFO *ri, SC_CARD_INFO *ci, LONG fd )
{
	if( ci->type==SC_CARD_MULTIFLEX_3K ) {
		if( (fd&0xFFFFFF)==((372L<<8)+1) ) return( SC_EXIT_OK );
		return( SC_EXIT_BAD_PARAM );
	}

	switch( fd&0xFFFFFF ) {
		case (372L<<8)+1:
			return( SC_EXIT_OK );
		case (372L<<8)+2:
			return( scReaderPTS( ri, ci, (BYTE *)"\xFF\x10\x12\xFD", 4 ) );
		case (372L<<8)+4:
			return( scReaderPTS( ri, ci, (BYTE *)"\xFF\x10\x13\xFC", 4 ) );
		case (372L<<8)+8:
			return( scReaderPTS( ri, ci, (BYTE *)"\xFF\x10\x14\xFB", 4 ) );
		case (372L<<8)+16:
			return( scReaderPTS( ri, ci, (BYTE *)"\xFF\x10\x15\xFA", 4 ) );
		default:
			return( SC_EXIT_BAD_PARAM );
	}
}

/* Generate Auth */

void scMultiflexGenerateAuth( const BYTE *key, const BYTE *chall, BYTE *auth )
{
	scCryptoflexGenerateAuth( key, chall, auth, SC_CRYPTOFLEX_ALGO_DES );
}

/* Compare Auth */

BOOLEAN scMultiflexCompareAuth( const BYTE *key, const BYTE *chall,
	const BYTE *auth )
{
#ifdef WITH_DES
	des_key_schedule schedule;
	des_cblock out;

	des_check_key=0;

	des_set_key( (des_cblock *) key, schedule );
	des_ecb_encrypt( (des_cblock *) chall, &out, schedule, DES_ENCRYPT );

	memset( &schedule, 0, sizeof(schedule) );

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

void scMultiflexGenerateMAC( const BYTE *key, const BYTE *chall,
	const BYTE *data, int datalen, BYTE *mac, int *maclen )
{
	scCryptoflexGenerateMAC( key, chall, data, datalen, mac, maclen,
		SC_CRYPTOFLEX_ALGO_DES );
}


/* Commands */

int scMultiflexCmdChangePIN( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	const BYTE *oldpin, const BYTE *newpin )
{
	return( scCryptoflexCmdChangePIN( ri, ci, 1, oldpin, newpin ) );
}

int scMultiflexCmdCreateFile( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	WORD fid, int flen, BYTE ftype, BYTE init, BYTE status, BYTE reclen,
	BYTE recnum, const BYTE *acond, const BYTE *akeys )
{
	return( scCryptoflexCmdCreateFile( ri, ci, fid, flen, ftype,
		init, status, reclen, recnum, acond, akeys ) );
}

int scMultiflexCmdCreateFileMAC( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	WORD fid, int flen, BYTE ftype, BYTE init, BYTE status, BYTE reclen,
	BYTE recnum, const BYTE *acond, const BYTE *akeys, const BYTE *key,
	const BYTE *chall )
{
	return( scCryptoflexCmdCreateFileMAC( ri, ci, fid, flen, ftype,
		init, status, reclen, recnum, acond, akeys, key, chall,
		SC_CRYPTOFLEX_ALGO_DES ) );
}

int scMultiflexCmdCreateRecord( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	const BYTE *data, int datalen )
{
	return( scCryptoflexCmdCreateRecord( ri, ci, data, datalen ) );
}

int scMultiflexCmdCreateRecordMAC( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	const BYTE *data, int datalen, const BYTE *key, const BYTE *chall )
{
	return( scCryptoflexCmdCreateRecordMAC( ri, ci, data, datalen,
		key, chall, SC_CRYPTOFLEX_ALGO_DES ) );
}

int scMultiflexCmdDecrease( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	const BYTE *amount, BYTE *resp, int *resplen )
{
	return( scCryptoflexCmdDecrease( ri, ci, amount, resp, resplen ) );
}

int scMultiflexCmdDecreaseMAC( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	const BYTE *amount, const BYTE *key, const BYTE *chall, BYTE *resp,
	int *resplen )
{
	return( scCryptoflexCmdDecreaseMAC( ri, ci, amount, key, chall,
		SC_CRYPTOFLEX_ALGO_DES, resp, resplen ) );
}

int scMultiflexCmdDecreaseSt( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	const BYTE *amount, BYTE *resp, int *resplen )
{
	BYTE cmd[]={ 0xF0, 0x34, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x00 };
	BYTE rsp[ SC_GENERAL_SHORT_DATA_SIZE+2 ];
	SC_APDU apdu;
	int ret;

	ci->sw[0]=0x00; ci->sw[1]=0x00;

	if( ci->type==SC_CARD_MULTIFLEX_3K ) return( SC_EXIT_NOT_SUPPORTED );

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

int scMultiflexCmdDecreaseStMAC( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	const BYTE *amount, const BYTE *key, const BYTE *chall, BYTE *resp,
	int *resplen )
{
#ifdef WITH_DES
	BYTE cmd[]={ 0xF0, 0x34, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00 };
	BYTE mac[32];
	BYTE rsp[ SC_GENERAL_SHORT_DATA_SIZE+2 ];
	SC_APDU apdu;
	int ret;

	ci->sw[0]=0x00; ci->sw[1]=0x00;

	if( ci->type==SC_CARD_MULTIFLEX_3K ) return( SC_EXIT_NOT_SUPPORTED );

	apdu.cse=SC_APDU_CASE_4_SHORT;
	apdu.cmd=mac;
	apdu.rsp=rsp;
	apdu.rsplen=0;

	memcpy( cmd+5, amount, 3 );

	scMultiflexGenerateMAC( key, chall, cmd, 5+3, mac, &apdu.cmdlen );

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

int scMultiflexCmdDeleteFile( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	WORD fid )
{
	return( scCryptoflexCmdDeleteFile( ri, ci, fid ) );
}

int scMultiflexCmdDeleteFileMAC( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	WORD fid, const BYTE *key, const BYTE *chall )
{
	return( scCryptoflexCmdDeleteFileMAC( ri, ci, fid, key,
		chall, SC_CRYPTOFLEX_ALGO_DES ) );
}

int scMultiflexCmdDirectory( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	BYTE *resp, int *resplen )
{
	if( ci->type==SC_CARD_MULTIFLEX_3K ) return( SC_EXIT_NOT_SUPPORTED );

	return( scCryptoflexCmdDirectory( ri, ci, resp, resplen ) );
}

int scMultiflexCmdExtAuth( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	BYTE keynum, const BYTE *key, const BYTE *chall )
{
	return( scCryptoflexCmdExtAuth( ri, ci, keynum, key, chall,
		SC_CRYPTOFLEX_ALGO_DES ) );
}

int scMultiflexCmdGetChall( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	BYTE *chall )
{
	int len=8;

	return( scCryptoflexCmdGetChall( ri, ci, chall, &len ) );
}

int scMultiflexCmdGiveChall( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	const BYTE *chall )
{
	BYTE cmd[]={ 0xF0, 0x86, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00 };
	BYTE rsp[ SC_GENERAL_SHORT_DATA_SIZE+2 ];
	SC_APDU apdu;
	int ret;

	ci->sw[0]=0x00; ci->sw[1]=0x00;

	if( ci->type==SC_CARD_MULTIFLEX_3K ) return( SC_EXIT_NOT_SUPPORTED );

	apdu.cse=SC_APDU_CASE_3_SHORT;
	apdu.cmd=cmd;
	apdu.cmdlen=13;
	apdu.rsp=rsp;
	apdu.rsplen=0;

	memcpy( cmd+5, chall, SC_MULTIFLEX_CHALL_SIZE );

	ret=scReaderSendAPDU( ri, ci, &apdu );
	memset( cmd, 0, sizeof(cmd) );
	if( ret!=SC_EXIT_OK ) return( ret );

	if( apdu.rsplen!=2 ) return( SC_EXIT_BAD_SW );

	ci->sw[0] = apdu.rsp[0];
	ci->sw[1] = apdu.rsp[1];

	return( SC_EXIT_OK );
}

/* sw is used to get the length of the available data. */
int scMultiflexCmdGetResp( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	BYTE *resp, int *resplen )
{
	return( scCryptoflexCmdGetResp( ri, ci, resp, resplen ) );
}

int scMultiflexCmdIncrease( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	const BYTE *amount, BYTE *resp, int *resplen )
{
	return( scCryptoflexCmdIncrease( ri, ci, amount, resp, resplen ) );
}

int scMultiflexCmdIncreaseMAC( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	const BYTE *amount, const BYTE *key, const BYTE *chall, BYTE *resp,
	int *resplen )
{
	return( scCryptoflexCmdIncreaseMAC( ri, ci, amount, key,
	chall, SC_CRYPTOFLEX_ALGO_DES, resp, resplen ) );
}

int scMultiflexCmdIncreaseSt( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	const BYTE *amount, BYTE *resp, int *resplen )
{
	BYTE cmd[]={ 0xF0, 0x36, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x00 };
	BYTE rsp[ SC_GENERAL_SHORT_DATA_SIZE+2 ];
	SC_APDU apdu;
	int ret;

	ci->sw[0]=0x00; ci->sw[1]=0x00;

	if( ci->type==SC_CARD_MULTIFLEX_3K ) return( SC_EXIT_NOT_SUPPORTED );

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

int scMultiflexCmdIncreaseStMAC( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	const BYTE *amount, const BYTE *key, const BYTE *chall, BYTE *resp,
	int *resplen )
{
#ifdef WITH_DES
	BYTE cmd[]={ 0xF0, 0x36, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x00 };
	BYTE mac[32];
	BYTE rsp[ SC_GENERAL_SHORT_DATA_SIZE+2 ];
	SC_APDU apdu;
	int ret;

	ci->sw[0]=0x00; ci->sw[1]=0x00;

	if( ci->type==SC_CARD_MULTIFLEX_3K ) return( SC_EXIT_NOT_SUPPORTED );

	apdu.cse=SC_APDU_CASE_4_SHORT;
	apdu.cmd=mac;
	apdu.rsp=rsp;
	apdu.rsplen=0;

	memcpy( cmd+5, amount, 3 );

	scMultiflexGenerateMAC( key, chall, cmd, 5+3, mac, &apdu.cmdlen );

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

int scMultiflexCmdIntAuth( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	BYTE keynum, const BYTE *chall, const BYTE *key )
{
	return( scCryptoflexCmdIntAuth( ri, ci, keynum, chall, key,
		SC_CRYPTOFLEX_ALGO_DES ) );
}

int scMultiflexCmdInvalidate( SC_READER_INFO *ri, SC_CARD_INFO *ci )
{
	return( scCryptoflexCmdInvalidate( ri, ci ) );
}

int scMultiflexCmdInvalidateMAC( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	const BYTE *key, const BYTE *chall )
{
	return( scCryptoflexCmdInvalidateMAC( ri, ci, key, chall,
		SC_CRYPTOFLEX_ALGO_DES ) );
}

int scMultiflexCmdReadBinary( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	int offset, BYTE *data, int *datalen )
{
	return( scCryptoflexCmdReadBinary( ri, ci, offset, data,
		datalen ) );
}

int scMultiflexCmdReadRecord( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	BYTE recnum, BYTE mode, BYTE *data, int *datalen )
{
	return( scCryptoflexCmdReadRecord( ri, ci, recnum, mode,
		data, datalen ) );
}

int scMultiflexCmdRehabilitate( SC_READER_INFO *ri, SC_CARD_INFO *ci )
{
	return( scCryptoflexCmdRehabilitate( ri, ci ) );
}

int scMultiflexCmdRehabilitateMAC( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	const BYTE *key, const BYTE *chall )
{
	return( scCryptoflexCmdRehabilitateMAC( ri, ci, key, chall,
		SC_CRYPTOFLEX_ALGO_DES ) );
}

int scMultiflexCmdSeek( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	BYTE offset, BYTE mode, const BYTE *pattern, BYTE patlen )
{
	return( scCryptoflexCmdSeek( ri, ci, offset, mode, pattern,
		patlen ) );
}

int scMultiflexCmdSelectFile( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	WORD fid, BYTE *resp, int *resplen )
{
	return( scCryptoflexCmdSelectFile( ri, ci, fid, resp, resplen ) );
}

int scMultiflexCmdUnblockPIN( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	const BYTE *unblock, const BYTE *new )
{
	return( scCryptoflexCmdUnblockPIN( ri, ci, 1, unblock, new ) );
}

int scMultiflexCmdUpdateBinary( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	int offset, const BYTE *data, BYTE datalen )
{
	return( scCryptoflexCmdUpdateBinary( ri, ci, offset, data,
		datalen ) );
}

int scMultiflexCmdUpdateBinaryMAC( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	int offset, const BYTE *data, BYTE datalen, const BYTE *key,
	const BYTE *chall )
{
	return( scCryptoflexCmdUpdateBinaryMAC( ri, ci, offset, data,
		datalen, key, chall, SC_CRYPTOFLEX_ALGO_DES ) );
}

int scMultiflexCmdUpdateRecord( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	BYTE recnum, BYTE mode, const BYTE *data, BYTE datalen )
{
	return( scCryptoflexCmdUpdateRecord( ri, ci, recnum, mode,
		data, datalen ) );
}

int scMultiflexCmdUpdateRecordMAC( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	BYTE recnum, BYTE mode, const BYTE *data, BYTE datalen, const BYTE *key,
	const BYTE *chall )
{
	return( scCryptoflexCmdUpdateRecordMAC( ri, ci, recnum,
		mode, data, datalen, key, chall, SC_CRYPTOFLEX_ALGO_DES ) );
}

int scMultiflexCmdVerifyKey( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	BYTE keynum, const BYTE *key, BYTE keylen )
{
	return( scCryptoflexCmdVerifyKey( ri, ci, keynum, key, keylen ) );
}

int scMultiflexCmdVerifyPIN( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	const BYTE *pin )
{
	return( scCryptoflexCmdVerifyPIN( ri, ci, 1, pin ) );
}

int scMultiflexCmdDesCrypt( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	BYTE keynum, BYTE mode, const BYTE *data, BYTE *resp, int *resplen )
{
	if( ci->type!=SC_CARD_MULTIFLEX_8K_DES ) return( SC_EXIT_NOT_SUPPORTED );

	return( scCryptoflexCmdDesCrypt( ri, ci, keynum, mode, data,
		resp, resplen ) );
}


