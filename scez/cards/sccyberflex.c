/****************************************************************************
*																			*
*					SCEZ chipcard library - Cyberflex routines				*
*						Copyright Matthias Bruestle 2000					*
*					For licensing, see the file COPYRIGHT					*
*																			*
****************************************************************************/

/* $Id: sccyberflex.c 1617 2005-11-03 17:41:39Z laforge $ */

#include <scez/scinternal.h>
#include <scez/cards/sccyberflex.h>

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

int scCyberflexInit( SC_CARD_INFO *ci )
{
	ci->scGetCap=scCyberflexGetCap;
	ci->scGetCardData=scCyberflexGetCardData;
	ci->scSetFD=scCyberflexSetFD;

	return( SC_EXIT_OK );
}

/* Capabilities */

int scCyberflexGetCap( SC_CARD_INFO *ci, SC_CARD_CAP *cp )
{
	cp->n_fd=1;

	/* 9600 at 3.579MHz */
	cp->fd[0]=(((10L<<16)+372L)<<8)+1;

	return( SC_EXIT_OK );
}

/* Fill card data in ci */

int scCyberflexGetCardData( SC_CARD_INFO *ci )
{
	BYTE header[] = { 0x00, 0xC0, 0x00, 0x00, 0x00 };	/* 00/F0 */
	BYTE swok[] = { 0x01, 0x90 };
	BYTE swav[] = { 0x01, 0x61 };

	ci->protocol=SC_PROTOCOL_T0;
	ci->direct=TRUE;
	ci->t0.d=1;
	ci->t0.wi=10;
	ci->t0.wwt=30720;
	ci->cla=0x00;	/* 00/F0 */
	memcpy( ci->t0.getrsp, header, 5 );
	memcpy( ci->swok, swok, sizeof(swok) );
	memcpy( ci->swav, swav, sizeof(swav) );
	ci->memsize=0;

	return( SC_EXIT_OK );
}

/* Set F and D. */

int scCyberflexSetFD( SC_READER_INFO *ri, SC_CARD_INFO *ci, LONG fd )
{
	if( (fd&0xFFFFFF)==((372L<<8)+1) ) return( SC_EXIT_OK );
	return( SC_EXIT_BAD_PARAM );
}

/* Generate Auth */

void scCyberflexGenerateAuth( const BYTE *key, const BYTE *chall, BYTE *auth,
	BYTE algo )
{
#ifdef WITH_DES
	des_key_schedule schedule;
	des_key_schedule schedule2;
	des_cblock out;

	des_check_key=0;

	if( algo==SC_CYBERFLEX_ALGO_3DES ) {
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

BOOLEAN scCyberflexCompareAuth( const BYTE *key, const BYTE *chall,
	const BYTE *auth, BYTE algo )
{
#ifdef WITH_DES
	des_key_schedule schedule;
	des_key_schedule schedule2;
	des_cblock out;

	des_check_key=0;

	if( algo==SC_CYBERFLEX_ALGO_3DES ) {
		des_set_key( (des_cblock *) key, schedule );
		des_set_key( (des_cblock *) (key+8), schedule2 );
		des_ecb2_encrypt( (des_cblock *) chall, &out, schedule, schedule2,
			DES_DECRYPT );
	} else {
		des_set_key( (des_cblock *) key, schedule );
		des_ecb_encrypt( (des_cblock *) chall, &out, schedule, DES_DECRYPT );
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

/* Sign Code */

void scCyberflexSignCode( const BYTE *key, const BYTE *data, int datalen,
	BYTE *auth, int *authlen, BYTE algo )
{
#ifdef WITH_DES
	des_key_schedule schedule;
	des_key_schedule schedule2;
	des_cblock in, out;
	int i, count=0;

	des_check_key=0;

	if( algo==SC_CYBERFLEX_ALGO_DES ) {
		des_set_key( (des_cblock *) key, schedule );
		*authlen=8;
	} else if( algo==SC_CYBERFLEX_ALGO_3DES ) {
		des_set_key( (des_cblock *) key, schedule );
		des_set_key( (des_cblock *) (key+8), schedule2 );
		*authlen=8;
	} else {
		*authlen=0;
		return;
	}

	memset( &out, 0, sizeof(out) );

	while( datalen ) {
		memcpy( &in, &out, sizeof(in) );

		for( i=0; i<8; i++ ) {
			if( datalen>0 ) {
				in[ i ] ^= data[ count++ ];
				datalen--;
			} else
				in[ i ] ^= 0xFF;
		}

		if( algo==SC_CYBERFLEX_ALGO_DES ) {
			des_ecb_encrypt( &in, &out, schedule, DES_ENCRYPT );
		} else {
			des_ecb2_encrypt( &in, &out, schedule, schedule2, DES_ENCRYPT );
		}
	}

	memcpy( auth, out, *authlen );

	memset( &schedule, 0, sizeof(schedule) );
	memset( &schedule2, 0, sizeof(schedule2) );
	memset( in, 0, sizeof(in) );
	memset( out, 0, sizeof(out) );
#else
	*authlen = 0;
#endif /* WITH_DES */
}


/****************************************************************************
*																			*
*							Administrative Commands							*
*																			*
****************************************************************************/

int scCyberflexCmdAppendRecord( SC_READER_INFO *ri, SC_CARD_INFO *ci,
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

	cmd[0]=ci->cla;
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

int scCyberflexCmdChangeCHV( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	BYTE chvnum, const BYTE *oldpin, const BYTE *newpin )
{
	BYTE cmd[21];
	BYTE rsp[ SC_GENERAL_SHORT_DATA_SIZE+2 ];
	SC_APDU apdu;
	int ret;

	memset( cmd, 0, sizeof(cmd) );
	cmd[1]=0x24;
	cmd[4]=0x10;

	ci->sw[0]=0x00; ci->sw[1]=0x00;

	apdu.cse=SC_APDU_CASE_3_SHORT;
	apdu.cmd=cmd;
	apdu.cmdlen=21;
	apdu.rsp=rsp;
	apdu.rsplen=0;

	cmd[0]=ci->cla;
	cmd[3]=chvnum;

	memcpy( cmd+5, oldpin, SC_CYBERFLEX_PIN_SIZE );
	memcpy( cmd+5+SC_CYBERFLEX_PIN_SIZE, newpin, SC_CYBERFLEX_PIN_SIZE );

	ret=scReaderSendAPDU( ri, ci, &apdu );
	memset( cmd, 0, sizeof(cmd) );
	if( ret!=SC_EXIT_OK ) return( ret );

	if( apdu.rsplen!=2 ) return( SC_EXIT_BAD_SW );

	ci->sw[0] = apdu.rsp[0];
	ci->sw[1] = apdu.rsp[1];

	return( SC_EXIT_OK );
}

int scCyberflexCmdChangeFileACL( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	const BYTE *acl )
{
	BYTE cmd[]={ 0x00, 0xFC, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00, 0x00,
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

	cmd[0]=ci->cla;
	memcpy( cmd+5, acl, SC_CYBERFLEX_ACL_SIZE );

	ret=scReaderSendAPDU( ri, ci, &apdu );
	memset( cmd, 0, sizeof(cmd) );
	if( ret!=SC_EXIT_OK ) return( ret );

	if( apdu.rsplen!=2 ) return( SC_EXIT_BAD_SW );

	ci->sw[0] = apdu.rsp[0];
	ci->sw[1] = apdu.rsp[1];

	return( SC_EXIT_OK );
}

int scCyberflexCmdChangeJavaATR( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	const BYTE *atr, BYTE atrlen )
{
	BYTE cmd[ 17 ];
	BYTE rsp[ SC_GENERAL_SHORT_DATA_SIZE+2 ];
	SC_APDU apdu;
	int ret;

	ci->sw[0]=0x00; ci->sw[1]=0x00;
	if( atrlen>12 ) return( SC_EXIT_BAD_PARAM );

	apdu.cse=SC_APDU_CASE_3_SHORT;
	apdu.cmd=cmd;
	apdu.cmdlen=5+atrlen;
	apdu.rsp=rsp;
	apdu.rsplen=0;

	cmd[0]=ci->cla;
	cmd[1]=0xFA;
	cmd[4]=atrlen;
	memcpy( cmd+5, atr, atrlen );

	ret=scReaderSendAPDU( ri, ci, &apdu );
	memset( cmd, 0, sizeof(cmd) );
	if( ret!=SC_EXIT_OK ) return( ret );

	if( apdu.rsplen!=2 ) return( SC_EXIT_BAD_SW );

	ci->sw[0] = apdu.rsp[0];
	ci->sw[1] = apdu.rsp[1];

	return( SC_EXIT_OK );
}

int scCyberflexCmdCreateFile( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	WORD fid, int flen, BYTE ftype, BYTE status, BYTE reclen, BYTE recnum,
	const BYTE *acl )
{
	BYTE cmd[21];
	BYTE rsp[ SC_GENERAL_SHORT_DATA_SIZE+2 ];
	SC_APDU apdu;
	int ret;

	ci->sw[0]=0x00; ci->sw[1]=0x00;

	apdu.cse=SC_APDU_CASE_3_SHORT;
	apdu.cmd=cmd;
	apdu.cmdlen=21;
	apdu.rsp=rsp;
	apdu.rsplen=0;

	memset( cmd, 0, sizeof(cmd) );
	cmd[0]=ci->cla;
	cmd[1]=0xE0;
	cmd[4]=0x10;

	cmd[5]=(flen >>8) & 0xFF;
	cmd[6]=flen & 0xFF;
	cmd[7]=(fid >> 8) & 0xFF;
	cmd[8]=fid & 0xFF;
	cmd[9]=ftype;
	cmd[10]=status;
	cmd[11]=reclen;
	cmd[12]=recnum;
	memcpy( cmd+13, acl, 8 );

	ret=scReaderSendAPDU( ri, ci, &apdu );
	memset( cmd, 0, sizeof(cmd) );
	if( ret!=SC_EXIT_OK ) return( ret );

	if( apdu.rsplen!=2 ) return( SC_EXIT_BAD_SW );

	ci->sw[0] = apdu.rsp[0];
	ci->sw[1] = apdu.rsp[1];

	return( SC_EXIT_OK );
}

/* Use fid==0xFFFF to delete all files in a DF. */

int scCyberflexCmdDeleteFile( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	WORD fid )
{
	BYTE cmd[]={ 0x00, 0xE4, 0x00, 0x00, 0x02, 0x00, 0x00 };
	BYTE rsp[ SC_GENERAL_SHORT_DATA_SIZE+2 ];
	SC_APDU apdu;
	int ret;

	ci->sw[0]=0x00; ci->sw[1]=0x00;

	apdu.cse=SC_APDU_CASE_3_SHORT;
	apdu.cmd=cmd;
	apdu.cmdlen=7;
	apdu.rsp=rsp;
	apdu.rsplen=0;

	cmd[0]=ci->cla;
	if( fid==0xFFFF ) {
		apdu.cse=SC_APDU_CASE_1;
		apdu.cmdlen=5;
		cmd[2]=0x80;
		cmd[4]=0x00;
	} else {
		cmd[5]=(fid >> 8) & 0xFF;
		cmd[6]=fid & 0xFF;
	}

	ret=scReaderSendAPDU( ri, ci, &apdu );
	memset( cmd, 0, sizeof(cmd) );
	if( ret!=SC_EXIT_OK ) return( ret );

	if( apdu.rsplen!=2 ) return( SC_EXIT_BAD_SW );

	ci->sw[0] = apdu.rsp[0];
	ci->sw[1] = apdu.rsp[1];

	return( SC_EXIT_OK );
}

int scCyberflexCmdDirectory( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	BYTE filenum, BYTE *resp, int *resplen )
{
	BYTE cmd[]={ 0x00, 0xA8, 0x00, 0x00, 0x00 };
	BYTE rsp[ SC_GENERAL_SHORT_DATA_SIZE+2 ];
	SC_APDU apdu;
	int ret;

	ci->sw[0]=0x00; ci->sw[1]=0x00;

	if( (*resplen!=0x00) && (*resplen!=0x0F) && (*resplen!=0x17) &&
		(*resplen!=0x28) )
		return( SC_EXIT_BAD_PARAM );

	apdu.cse=SC_APDU_CASE_2_SHORT;
	apdu.cmd=cmd;
	apdu.cmdlen=5;
	apdu.rsp=rsp;
	apdu.rsplen=0;

	cmd[0] = ci->cla;
	cmd[3] = filenum;
	cmd[4] = *resplen;

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

int scCyberflexCmdExecuteMethod( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	BYTE type, const BYTE *data, BYTE datalen )
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

	cmd[0]=ci->cla;
	cmd[1]=0x0C;
	cmd[2]=type;
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

int scCyberflexCmdGetData( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	BYTE *resp, int *resplen )
{
	BYTE cmd[]={ 0x00, 0xCA, 0x00, 0x01, 0x16 };
	BYTE rsp[ SC_GENERAL_SHORT_DATA_SIZE+2 ];
	SC_APDU apdu;
	int ret;

	ci->sw[0]=0x00; ci->sw[1]=0x00;

	apdu.cse=SC_APDU_CASE_2_SHORT;
	apdu.cmd=cmd;
	apdu.cmdlen=5;
	apdu.rsp=rsp;
	apdu.rsplen=0;

	cmd[0] = ci->cla;

	ret=scReaderSendAPDU( ri, ci, &apdu );
	memset( cmd, 0, sizeof(cmd) );
	if( ret!=SC_EXIT_OK ) return( ret );

	if( apdu.rsplen<2 ) return( SC_EXIT_BAD_SW );

	ci->sw[0] = apdu.rsp[apdu.rsplen-2];
	ci->sw[1] = apdu.rsp[apdu.rsplen-1];

	apdu.rsplen-=2;

	if( apdu.rsplen>0x16 ) return( SC_EXIT_RSP_TOO_LONG );

	memcpy( resp, apdu.rsp, apdu.rsplen );
	*resplen=apdu.rsplen;

	memset( rsp, 0, sizeof(rsp) );

	return( SC_EXIT_OK );
}

int scCyberflexCmdGetFileACL( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	BYTE *resp, int *resplen )
{
	BYTE cmd[]={ 0x00, 0xFE, 0x00, 0x00, 0x08 };
	BYTE rsp[ SC_GENERAL_SHORT_DATA_SIZE+2 ];
	SC_APDU apdu;
	int ret;

	ci->sw[0]=0x00; ci->sw[1]=0x00;

	apdu.cse=SC_APDU_CASE_2_SHORT;
	apdu.cmd=cmd;
	apdu.cmdlen=5;
	apdu.rsp=rsp;
	apdu.rsplen=0;

	cmd[0] = ci->cla;

	ret=scReaderSendAPDU( ri, ci, &apdu );
	memset( cmd, 0, sizeof(cmd) );
	if( ret!=SC_EXIT_OK ) return( ret );

	if( apdu.rsplen<2 ) return( SC_EXIT_BAD_SW );

	ci->sw[0] = apdu.rsp[apdu.rsplen-2];
	ci->sw[1] = apdu.rsp[apdu.rsplen-1];

	apdu.rsplen-=2;

	if( apdu.rsplen>8 ) return( SC_EXIT_RSP_TOO_LONG );

	memcpy( resp, apdu.rsp, apdu.rsplen );
	*resplen=apdu.rsplen;

	memset( rsp, 0, sizeof(rsp) );

	return( SC_EXIT_OK );
}

/* sw is used to get the length of the available data. */

int scCyberflexCmdGetResp( SC_READER_INFO *ri, SC_CARD_INFO *ci,
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

	cmd[0] = ci->cla;
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

int scCyberflexCmdInvalidate( SC_READER_INFO *ri, SC_CARD_INFO *ci )
{
	BYTE cmd[]={ 0x00, 0x04, 0x00, 0x00, 0x00 };
	BYTE rsp[ SC_GENERAL_SHORT_DATA_SIZE+2 ];
	SC_APDU apdu;
	int ret;

	ci->sw[0]=0x00; ci->sw[1]=0x00;

	apdu.cse=SC_APDU_CASE_1;
	apdu.cmd=cmd;
	apdu.cmdlen=5;
	apdu.rsp=rsp;
	apdu.rsplen=0;

	cmd[0] = ci->cla;

	ret=scReaderSendAPDU( ri, ci, &apdu );
	memset( cmd, 0, sizeof(cmd) );
	if( ret!=SC_EXIT_OK ) return( ret );

	if( apdu.rsplen!=2 ) return( SC_EXIT_BAD_SW );

	ci->sw[0] = apdu.rsp[0];
	ci->sw[1] = apdu.rsp[1];

	return( SC_EXIT_OK );
}

int scCyberflexCmdLogOutAll( SC_READER_INFO *ri, SC_CARD_INFO *ci )
{
	BYTE cmd[]={ 0x00, 0x22, 0x07, 0x00, 0x00 };
	BYTE rsp[ SC_GENERAL_SHORT_DATA_SIZE+2 ];
	SC_APDU apdu;
	int ret;

	ci->sw[0]=0x00; ci->sw[1]=0x00;

	apdu.cse=SC_APDU_CASE_1;
	apdu.cmd=cmd;
	apdu.cmdlen=5;
	apdu.rsp=rsp;
	apdu.rsplen=0;

	cmd[0] = ci->cla;

	ret=scReaderSendAPDU( ri, ci, &apdu );
	memset( cmd, 0, sizeof(cmd) );
	if( ret!=SC_EXIT_OK ) return( ret );

	if( apdu.rsplen!=2 ) return( SC_EXIT_BAD_SW );

	ci->sw[0] = apdu.rsp[0];
	ci->sw[1] = apdu.rsp[1];

	return( SC_EXIT_OK );
}

int scCyberflexCmdManageInstance( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	BYTE op )
{
	BYTE cmd[]={ 0x00, 0x08, 0x00, 0x00, 0x00 };
	BYTE rsp[ SC_GENERAL_SHORT_DATA_SIZE+2 ];
	SC_APDU apdu;
	int ret;

	ci->sw[0]=0x00; ci->sw[1]=0x00;

	apdu.cse=SC_APDU_CASE_1;
	apdu.cmd=cmd;
	apdu.cmdlen=5;
	apdu.rsp=rsp;
	apdu.rsplen=0;

	cmd[0] = ci->cla;
	cmd[2] = op;

	ret=scReaderSendAPDU( ri, ci, &apdu );
	memset( cmd, 0, sizeof(cmd) );
	if( ret!=SC_EXIT_OK ) return( ret );

	if( apdu.rsplen!=2 ) return( SC_EXIT_BAD_SW );

	ci->sw[0] = apdu.rsp[0];
	ci->sw[1] = apdu.rsp[1];

	return( SC_EXIT_OK );
}

int scCyberflexCmdManageProgram( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	const BYTE *data, BYTE datalen )
{
	BYTE cmd[ SC_GENERAL_SHORT_DATA_SIZE+5 ];
	BYTE rsp[ SC_GENERAL_SHORT_DATA_SIZE+2 ];
	SC_APDU apdu;
	int ret;

	ci->sw[0]=0x00; ci->sw[1]=0x00;

	apdu.cse=SC_APDU_CASE_1;
	apdu.cmd=cmd;
	apdu.cmdlen=5;
	apdu.rsp=rsp;
	apdu.rsplen=0;

	cmd[0]=ci->cla;
	cmd[1]=0x0A;
	cmd[2]=0x02;
	if( data!=NULL ) {
		apdu.cse=SC_APDU_CASE_3_SHORT;
		apdu.cmdlen+=datalen;
		cmd[2]=0x01;
		cmd[4]=datalen;
		memcpy( cmd+5, data, datalen );
	}

	ret=scReaderSendAPDU( ri, ci, &apdu );
	memset( cmd, 0, sizeof(cmd) );
	if( ret!=SC_EXIT_OK ) return( ret );

	if( apdu.rsplen!=2 ) return( SC_EXIT_BAD_SW );

	ci->sw[0] = apdu.rsp[0];
	ci->sw[1] = apdu.rsp[1];

	return( SC_EXIT_OK );
}

int scCyberflexCmdReadBinary( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	int offset, BYTE *data, int *datalen )
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

	*datalen &= 0xFF;
	offset &= 0xFFFF;

	cmd[0] = ci->cla;
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

int scCyberflexCmdReadRecord( SC_READER_INFO *ri, SC_CARD_INFO *ci,
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

	cmd[0] = ci->cla;
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

int scCyberflexCmdRehabilitate( SC_READER_INFO *ri, SC_CARD_INFO *ci )
{
	BYTE cmd[]={ 0x00, 0x44, 0x00, 0x00, 0x00 };
	BYTE rsp[ SC_GENERAL_SHORT_DATA_SIZE+2 ];
	SC_APDU apdu;
	int ret;

	ci->sw[0]=0x00; ci->sw[1]=0x00;

	apdu.cse=SC_APDU_CASE_1;
	apdu.cmd=cmd;
	apdu.cmdlen=5;
	apdu.rsp=rsp;
	apdu.rsplen=0;

	cmd[0] = ci->cla;

	ret=scReaderSendAPDU( ri, ci, &apdu );
	memset( cmd, 0, sizeof(cmd) );
	if( ret!=SC_EXIT_OK ) return( ret );

	if( apdu.rsplen!=2 ) return( SC_EXIT_BAD_SW );

	ci->sw[0] = apdu.rsp[0];
	ci->sw[1] = apdu.rsp[1];

	return( SC_EXIT_OK );
}

int scCyberflexCmdSelect( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	BYTE type, WORD fid, const BYTE *aid, BYTE aidlen, BYTE *resp,
	int *resplen )
{
	BYTE cmd[ 22 ];
	BYTE rsp[ SC_GENERAL_SHORT_DATA_SIZE+2 ];
	SC_APDU apdu;
	int ret;

	ci->sw[0]=0x00; ci->sw[1]=0x00;
	if( (type==SC_CYBERFLEX_SELECT_APPL) && (aid!=NULL) &&
		((aidlen<5) || (aidlen>16)) )
		return( SC_EXIT_BAD_PARAM );

	apdu.cse=SC_APDU_CASE_2_SHORT;
	apdu.cmd=cmd;
	apdu.cmdlen=5;
	apdu.rsp=rsp;
	apdu.rsplen=0;

	memset( cmd, 0, sizeof(cmd) );
	cmd[0]=ci->cla;
	cmd[1]=0xA4;
	cmd[2]=type;

	switch( type ) {
	case 0x00:
		apdu.cse=SC_APDU_CASE_4_SHORT;
		apdu.cmdlen=8;
		cmd[4]=0x02;
		cmd[5]=(fid >> 8) & 0xFF;
		cmd[6]=fid & 0xFF;
		break;
	case 0x03:
		break;
	case 0x04:
		if( (aid!=NULL) && (aidlen!=0) ) {
			apdu.cse=SC_APDU_CASE_4_SHORT;
			apdu.cmdlen=6+aidlen;
			cmd[4]=aidlen;
			memcpy( cmd+5, aid, aidlen );
		} else
			apdu.cse=SC_APDU_CASE_1;
		break;
	default:
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

int scCyberflexCmdUnblockCHV( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	BYTE chvnum, const BYTE *unblock, const BYTE *newpin )
{
	BYTE cmd[ 21 ];
	BYTE rsp[ SC_GENERAL_SHORT_DATA_SIZE+2 ];
	SC_APDU apdu;
	int ret;

	ci->sw[0]=0x00; ci->sw[1]=0x00;

	apdu.cse=SC_APDU_CASE_3_SHORT;
	apdu.cmd=cmd;
	apdu.cmdlen=5+SC_CYBERFLEX_PIN_SIZE+SC_CYBERFLEX_PIN_SIZE;
	apdu.rsp=rsp;
	apdu.rsplen=0;

	memset( cmd, 0, sizeof(cmd) );
	cmd[0]=ci->cla;
	cmd[1]=0x2C;
	cmd[3]=chvnum;
	cmd[4]=0x10;
	memcpy( cmd+5, unblock, SC_CYBERFLEX_PIN_SIZE );
	memcpy( cmd+5+SC_CYBERFLEX_PIN_SIZE, newpin, SC_CYBERFLEX_PIN_SIZE );

	ret=scReaderSendAPDU( ri, ci, &apdu );
	memset( cmd, 0, sizeof(cmd) );
	if( ret!=SC_EXIT_OK ) return( ret );

	if( apdu.rsplen!=2 ) return( SC_EXIT_BAD_SW );

	ci->sw[0] = apdu.rsp[0];
	ci->sw[1] = apdu.rsp[1];

	return( SC_EXIT_OK );
}

int scCyberflexCmdUpdateBinary( SC_READER_INFO *ri, SC_CARD_INFO *ci,
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

	cmd[0]=ci->cla;
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

int scCyberflexCmdUpdateRecord( SC_READER_INFO *ri, SC_CARD_INFO *ci,
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

	cmd[0]=ci->cla;
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

int scCyberflexCmdVerifyCHV( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	BYTE chvnum, const BYTE *chv )
{
	BYTE cmd[]={ 0x00, 0x20, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00, 0x00,
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

	cmd[0]=ci->cla;
	cmd[3]=chvnum;
	memcpy( cmd+5, chv, SC_CYBERFLEX_PIN_SIZE );

	ret=scReaderSendAPDU( ri, ci, &apdu );
	memset( cmd, 0, sizeof(cmd) );
	if( ret!=SC_EXIT_OK ) return( ret );

	if( apdu.rsplen!=2 ) return( SC_EXIT_BAD_SW );

	ci->sw[0] = apdu.rsp[0];
	ci->sw[1] = apdu.rsp[1];

	return( SC_EXIT_OK );
}

int scCyberflexCmdVerifyKey( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	BYTE keynum, const BYTE *key, BYTE keylen )
{
	BYTE cmd[ SC_GENERAL_SHORT_DATA_SIZE+5 ];
	BYTE rsp[ SC_GENERAL_SHORT_DATA_SIZE+2 ];
	SC_APDU apdu;
	int ret;

	ci->sw[0]=0x00; ci->sw[1]=0x00;
	if( keynum>4 ) return( SC_EXIT_BAD_PARAM );

	apdu.cse=SC_APDU_CASE_3_SHORT;
	apdu.cmd=cmd;
	apdu.cmdlen=5+keylen;
	apdu.rsp=rsp;
	apdu.rsplen=0;

	cmd[0]=ci->cla;
	cmd[1]=0x2A;
	cmd[2]=0x00;
	cmd[3]=keynum & 0x07;
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

/****************************************************************************
*																			*
*							Cryptographic Commands							*
*																			*
****************************************************************************/

int scCyberflexCmdAskRandom( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	BYTE *chall, int *len )
{
	BYTE cmd[]={ 0x00, 0x84, 0x00, 0x00, 0x00 };
	BYTE rsp[ SC_GENERAL_SHORT_DATA_SIZE+2 ];
	SC_APDU apdu;
	int ret;

	ci->sw[0]=0x00; ci->sw[1]=0x00;

	if( (*len<4) || (*len>255) ) return( SC_EXIT_BAD_PARAM );

	apdu.cse=SC_APDU_CASE_2_SHORT;
	apdu.cmd=cmd;
	apdu.cmdlen=5;
	apdu.rsp=rsp;
	apdu.rsplen=0;

	cmd[0] = ci->cla;
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

int scCyberflexCmdExtAuth( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	BYTE keynum, const BYTE *data, BYTE datalen, BYTE algo )
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

	cmd[0]=ci->cla;
	cmd[1]=0x82;
	cmd[2]=algo;
	cmd[3]=keynum;
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

int scCyberflexCmdExtAuthDES( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	BYTE keynum, const BYTE *key, const BYTE *chall, BYTE algo )
{
#ifdef WITH_DES
	BYTE cmd[]={ 0x00, 0x82, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00 };
	BYTE rsp[ SC_GENERAL_SHORT_DATA_SIZE+2 ];
	BYTE mac[ SC_CYBERFLEX_DES_SIZE ];
	SC_APDU apdu;
	int ret;

	ci->sw[0]=0x00; ci->sw[1]=0x00;
	if( (algo!=SC_CYBERFLEX_ALGO_DES) &&
		(algo!=SC_CYBERFLEX_ALGO_3DES) )
		return( SC_EXIT_BAD_PARAM );

	scCyberflexGenerateAuth( key, chall, mac, algo );

	apdu.cse=SC_APDU_CASE_3_SHORT;
	apdu.cmd=cmd;
	apdu.cmdlen=13;
	apdu.rsp=rsp;
	apdu.rsplen=0;

	cmd[0]=ci->cla;
	cmd[2]=algo;
	cmd[3]=keynum;
	cmd[4]=SC_CYBERFLEX_DES_SIZE;
	memcpy( cmd+5, mac, SC_CYBERFLEX_DES_SIZE );
	memset( mac, 0, sizeof(mac) );

	ret=scReaderSendAPDU( ri, ci, &apdu );
	memset( cmd, 0, sizeof(cmd) );
	if( ret!=SC_EXIT_OK ) return( ret );

	if( apdu.rsplen!=2 ) return( SC_EXIT_BAD_SW );

	ci->sw[0] = apdu.rsp[0];
	ci->sw[1] = apdu.rsp[1];

	return( SC_EXIT_OK );
#else
	return( SC_EXIT_NOT_SUPPORTED );
#endif /* WITH_DES */
}

int scCyberflexCmdIntAuth( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	BYTE keynum, const BYTE *data, BYTE datalen, BYTE *resp, int *resplen,
	BYTE algo )
{
	BYTE cmd[ SC_GENERAL_SHORT_DATA_SIZE+5 ];
	BYTE rsp[ SC_GENERAL_SHORT_DATA_SIZE+2 ];
	SC_APDU apdu;
	int ret;

	ci->sw[0]=0x00; ci->sw[1]=0x00;
	*resplen=0;

	apdu.cse=SC_APDU_CASE_4_SHORT;
	apdu.cmd=cmd;
	apdu.cmdlen=5+datalen+1;
	apdu.rsp=rsp;
	apdu.rsplen=0;

	memset( cmd, 0, sizeof(cmd) );
	cmd[0]=ci->cla;
	cmd[1]=0x88;
	cmd[2]=algo;
	cmd[3]=keynum;
	cmd[4]=datalen;
	memcpy( cmd+5, data, datalen );

	ret=scReaderSendAPDU( ri, ci, &apdu );
	memset( cmd, 0, sizeof(cmd) );
	if( ret!=SC_EXIT_OK ) return( ret );

	if( apdu.rsplen<2 ) return( SC_EXIT_BAD_SW );

	ci->sw[0] = apdu.rsp[apdu.rsplen-2];
	ci->sw[1] = apdu.rsp[apdu.rsplen-1];

	memcpy( resp, apdu.rsp, apdu.rsplen-2 );
	memset( rsp, 0, sizeof(rsp) );
	*resplen=apdu.rsplen-2;

	return( SC_EXIT_OK );
}

int scCyberflexCmdIntAuthDES( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	BYTE keynum, const BYTE *chall, const BYTE *key, BYTE algo )
{
#ifdef WITH_DES
	BYTE cmd[]={ 0x00, 0x88, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
	BYTE rsp[ SC_GENERAL_SHORT_DATA_SIZE+2 ];
	SC_APDU apdu;
	int ret;

	ci->sw[0]=0x00; ci->sw[1]=0x00;
	if( (algo!=SC_CYBERFLEX_ALGO_DES) &&
		(algo!=SC_CYBERFLEX_ALGO_3DES) )
		return( SC_EXIT_BAD_PARAM );

	apdu.cse=SC_APDU_CASE_4_SHORT;
	apdu.cmd=cmd;
	apdu.cmdlen=14;
	apdu.rsp=rsp;
	apdu.rsplen=0;

	cmd[0]=ci->cla;
	cmd[2]=algo;
	cmd[3]=keynum;
	memcpy( cmd+5, chall, SC_CYBERFLEX_CHALL_SIZE );

	ret=scReaderSendAPDU( ri, ci, &apdu );
	memset( cmd, 0, sizeof(cmd) );
	if( ret!=SC_EXIT_OK ) return( ret );

	if( apdu.rsplen<2 ) return( SC_EXIT_BAD_SW );

	ci->sw[0] = apdu.rsp[apdu.rsplen-2];
	ci->sw[1] = apdu.rsp[apdu.rsplen-1];

	if( (apdu.rsplen=10) && (scCyberflexCompareAuth( key, chall, rsp,
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

/****************************************************************************
*																			*
*								GSM Commands								*
*																			*
****************************************************************************/

int scCyberflexCmdDisableCHV( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	const BYTE *chv )
{
	BYTE cmd[]={ 0x00, 0x26, 0x00, 0x01, 0x08, 0x00, 0x00, 0x00, 0x00,
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

	cmd[0]=ci->cla;
	memcpy( cmd+5, chv, SC_CYBERFLEX_PIN_SIZE );

	ret=scReaderSendAPDU( ri, ci, &apdu );
	memset( cmd, 0, sizeof(cmd) );
	if( ret!=SC_EXIT_OK ) return( ret );

	if( apdu.rsplen!=2 ) return( SC_EXIT_BAD_SW );

	ci->sw[0] = apdu.rsp[0];
	ci->sw[1] = apdu.rsp[1];

	return( SC_EXIT_OK );
}

int scCyberflexCmdEnableCHV( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	const BYTE *chv )
{
	BYTE cmd[]={ 0x00, 0x28, 0x00, 0x01, 0x08, 0x00, 0x00, 0x00, 0x00,
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

	cmd[0]=ci->cla;
	memcpy( cmd+5, chv, SC_CYBERFLEX_PIN_SIZE );

	ret=scReaderSendAPDU( ri, ci, &apdu );
	memset( cmd, 0, sizeof(cmd) );
	if( ret!=SC_EXIT_OK ) return( ret );

	if( apdu.rsplen!=2 ) return( SC_EXIT_BAD_SW );

	ci->sw[0] = apdu.rsp[0];
	ci->sw[1] = apdu.rsp[1];

	return( SC_EXIT_OK );
}

int scCyberflexCmdIncrease( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	const BYTE *amount, BYTE *resp, int *resplen )
{
	BYTE cmd[]={ 0x00, 0x32, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x00 };
	BYTE rsp[ SC_GENERAL_SHORT_DATA_SIZE+2 ];
	SC_APDU apdu;
	int ret;

	ci->sw[0]=0x00; ci->sw[1]=0x00;

	apdu.cse=SC_APDU_CASE_4_SHORT;
	apdu.cmd=cmd;
	apdu.cmdlen=9;
	apdu.rsp=rsp;
	apdu.rsplen=0;

	cmd[0] = ci->cla;
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

int scCyberflexCmdSeek( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	BYTE mode, const BYTE *pattern, BYTE patlen, BYTE *resp )
{
	BYTE cmd[ SC_GENERAL_SHORT_DATA_SIZE+5 ];
	BYTE rsp[ SC_GENERAL_SHORT_DATA_SIZE+2 ];
	SC_APDU apdu;
	int ret;

	ci->sw[0]=0x00; ci->sw[1]=0x00;
	if( resp!=NULL ) *resp=0;

	apdu.cse=SC_APDU_CASE_3_SHORT;
	apdu.cmd=cmd;
	apdu.cmdlen=5+patlen;
	apdu.rsp=rsp;
	apdu.rsplen=0;

	cmd[0]=ci->cla;
	cmd[1]=0xA2;
	cmd[2]=0x00;
	cmd[3]=mode & 0x0F;
	if( resp!=NULL ) {
		cmd[3]|=0x10;
		apdu.cmdlen+=1;
		apdu.cse=SC_APDU_CASE_4_SHORT;
	}
	cmd[4]=patlen;

	memcpy( cmd+5, pattern, patlen );

	ret=scReaderSendAPDU( ri, ci, &apdu );
	memset( cmd, 0, sizeof(cmd) );
	if( ret!=SC_EXIT_OK ) return( ret );

	if( apdu.rsplen<2 ) return( SC_EXIT_BAD_SW );

	ci->sw[0] = apdu.rsp[apdu.rsplen-2];
	ci->sw[1] = apdu.rsp[apdu.rsplen-1];

	if( resp!=NULL ) {
		if( apdu.rsplen==3 ) *resp=apdu.rsp[0];
		else *resp=0;
	}

	return( SC_EXIT_OK );
}

int scCyberflexCmdSleep( SC_READER_INFO *ri, SC_CARD_INFO *ci )
{
	BYTE cmd[]={ 0x00, 0xFA, 0x00, 0x00, 0x00 };
	BYTE rsp[ SC_GENERAL_SHORT_DATA_SIZE+2 ];
	SC_APDU apdu;
	int ret;

	ci->sw[0]=0x00; ci->sw[1]=0x00;

	apdu.cse=SC_APDU_CASE_1;
	apdu.cmd=cmd;
	apdu.cmdlen=5;
	apdu.rsp=rsp;
	apdu.rsplen=0;

	cmd[0] = ci->cla;

	ret=scReaderSendAPDU( ri, ci, &apdu );
	memset( cmd, 0, sizeof(cmd) );
	if( ret!=SC_EXIT_OK ) return( ret );

	if( apdu.rsplen!=2 ) return( SC_EXIT_BAD_SW );

	ci->sw[0] = apdu.rsp[0];
	ci->sw[1] = apdu.rsp[1];

	return( SC_EXIT_OK );
}

int scCyberflexCmdStatus( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	BYTE *resp, int *resplen )
{
	BYTE cmd[]={ 0x00, 0xF2, 0x00, 0x00, 0x00 };
	BYTE rsp[ SC_GENERAL_SHORT_DATA_SIZE+2 ];
	SC_APDU apdu;
	int ret;

	ci->sw[0]=0x00; ci->sw[1]=0x00;

	if( (*resplen!=0) && (*resplen!=0x17) && (*resplen!=0x28) )
		return( SC_EXIT_BAD_PARAM );

	apdu.cse=SC_APDU_CASE_2_SHORT;
	apdu.cmd=cmd;
	apdu.cmdlen=5;
	apdu.rsp=rsp;
	apdu.rsplen=0;

	cmd[0] = ci->cla;
	cmd[4] = *resplen & 0xFF;

	*resplen = 0;

	ret=scReaderSendAPDU( ri, ci, &apdu );
	memset( cmd, 0, sizeof(cmd) );
	if( ret!=SC_EXIT_OK ) return( ret );

	if( apdu.rsplen<2 ) return( SC_EXIT_BAD_SW );

	ci->sw[0] = apdu.rsp[apdu.rsplen-2];
	ci->sw[1] = apdu.rsp[apdu.rsplen-1];

	memcpy( resp, apdu.rsp, apdu.rsplen-2 );
	*resplen = apdu.rsplen-2;

	memset( rsp, 0, sizeof(rsp) );

	return( SC_EXIT_OK );
}


