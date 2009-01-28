/****************************************************************************
*																			*
*					SCEZ chipcard library - GPK4000 routines				*
*					Copyright Matthias Bruestle 1999,2000					*
*					For licensing, see the file COPYRIGHT					*
*																			*
****************************************************************************/

/* $Id: scgpk4000.c 1617 2005-11-03 17:41:39Z laforge $ */

#include <scez/scinternal.h>
#include <scez/cards/scgpk4000.h>

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

int scGpk4000Init( SC_CARD_INFO *ci )
{
	ci->scGetCap=scGpk4000GetCap;
	ci->scGetCardData=scGpk4000GetCardData;
	ci->scSetFD=scGpk4000SetFD;

	return( SC_EXIT_OK );
}

/* Capabilities */

int scGpk4000GetCap( SC_CARD_INFO *ci, SC_CARD_CAP *cp )
{
	if( ((ci->type&0xFFFFFF00)==SC_CARD_BRADESCO) ||
		((ci->type&0xFFFFFF00)==SC_CARD_GPK8000) ) {
		cp->n_fd=1;

		/* 9600 at 3.579MHz */
		cp->fd[0]=(((10L<<16)+372L)<<8)+1;

		return( SC_EXIT_OK );
	}

	cp->n_fd=8;

	/* 9600 at 3.579MHz */
	cp->fd[0]=(((10L<<16)+372L)<<8)+1;

	/* 19200 at 3.579MHz */
	cp->fd[1]=(((19L<<16)+744L)<<8)+4;

	/* 57600 at 3.579MHz */
	cp->fd[2]=(((58L<<16)+744L)<<8)+12;

	/*  at 3.579MHz */
	cp->fd[3]=(((56L<<16)+512L)<<8)+8;

	/*  at 3.579MHz */
	cp->fd[4]=(((112L<<16)+512L)<<8)+16;

	/*  at 3.579MHz */
	cp->fd[5]=(((9L<<16)+768L)<<8)+2;

	/*  at 3.579MHz */
	cp->fd[6]=(((19L<<16)+768L)<<8)+4;

	/*  at 3.579MHz */
	cp->fd[7]=(((37L<<16)+768L)<<8)+8;

	return( SC_EXIT_OK );
}

/* Fill card data in ci */

int scGpk4000GetCardData( SC_CARD_INFO *ci )
{
	BYTE header[] = { 0x00, 0xC0, 0x00, 0x00, 0x00 };
	BYTE swok[] = { 0x01, 0x90 };
	BYTE swav[] = { 0x01, 0x61 };
	int ret;

	if( (ret=scSmartcardProcessATR( ci ))!=SC_EXIT_OK ) return( ret );

#if 0
	ci->protocol=SC_PROTOCOL_T0;
	ci->direct=TRUE;
	ci->t0.d=1;
	ci->t0.wi=10;
	ci->t0.wwt=9600;
#endif

	memcpy( ci->t0.getrsp, header, 5 );
	memcpy( ci->swok, swok, sizeof(swok) );
	memcpy( ci->swav, swav, sizeof(swav) );
	ci->memsize=0;

	return( SC_EXIT_OK );
}

/* Set F and D. */

int scGpk4000SetFD( SC_READER_INFO *ri, SC_CARD_INFO *ci, LONG fd )
{
	if( ci->type==SC_CARD_BRADESCO ) {
		if( (fd&0xFFFFF)==((372L<<8)+1) ) return( SC_EXIT_OK );
		return( SC_EXIT_BAD_PARAM );
	}

	switch( fd&0xFFFFFF ) {
		case (372L<<8)+1:
			return( scGpk4000CmdSwtSpd( ri, ci, 0x11, 0 ) );
		case (744L<<8)+4:
			return( scGpk4000CmdSwtSpd( ri, ci, 0x33, 0 ) );
		case (744L<<8)+12:
			return( scGpk4000CmdSwtSpd( ri, ci, 0x38, 0 ) );
		case (512L<<8)+8:
			return( scGpk4000CmdSwtSpd( ri, ci, 0x94, 0 ) );
		case (512L<<8)+16:
			return( scGpk4000CmdSwtSpd( ri, ci, 0x16, 0 ) );
		case (768L<<8)+2:
			return( scGpk4000CmdSwtSpd( ri, ci, 0xA2, 0 ) );
		case (768L<<8)+4:
			return( scGpk4000CmdSwtSpd( ri, ci, 0xA3, 0 ) );
		case (768L<<8)+8:
			return( scGpk4000CmdSwtSpd( ri, ci, 0xA4, 0 ) );
		default:
			return( SC_EXIT_BAD_PARAM );
	}
}

/* Generate EAC */

void scGpk4000GenerateEac( const BYTE *key, const BYTE *chall, BYTE *auth )
{
#ifdef WITH_DES
	des_key_schedule k1;
	des_key_schedule k2;
	des_cblock out;

	des_check_key=0;

	des_set_key( (des_cblock *) key, k1 );
	des_set_key( (des_cblock *) (key+8), k2 );
	des_ecb3_encrypt( (des_cblock *) chall, &out, k1, k2, k1, DES_ENCRYPT );

	memcpy( auth, out, 4 );

	memset( &k1, 0, sizeof(k1) );
	memset( &k2, 0, sizeof(k2) );
	memset( out, 0, sizeof(out) );
#endif /* WITH_DES */
}

/* Compare EAC */

BOOLEAN scGpk4000CompareEac( const BYTE *key, const BYTE *chall,
	const BYTE *auth )
{
#ifdef WITH_DES
	des_key_schedule k1;
	des_key_schedule k2;
	des_cblock out;

	des_check_key=0;

	des_set_key( (des_cblock *) key, k1 );
	des_set_key( (des_cblock *) (key+8), k2 );
	des_ecb3_encrypt( (des_cblock *) chall, &out, k1, k2, k1, DES_ENCRYPT );

	memset( &k1, 0, sizeof(k1) );
	memset( &k2, 0, sizeof(k2) );

	if( memcmp( auth, out+4, 4 ) ) return( FALSE );

	memset( out, 0, sizeof(out) );

	return( TRUE );
#else
	return( FALSE );
#endif /* WITH_DES */
}

/* Generate diversified key */

int scGpk4000GenDivKey( const BYTE *key, const BYTE *chall, const BYTE *r_rn,
	BYTE *kats )
{
#ifdef WITH_DES
	des_key_schedule k1;
	des_key_schedule k2;
	des_cblock out;

	des_check_key=0;

	/* Generate session key */
	des_set_key( (des_cblock *) key, k1 );
	des_set_key( (des_cblock *) (key+8), k2 );

	des_ecb3_encrypt( (des_cblock *)(r_rn+4), &out, k1, k2, k1, DES_ENCRYPT );
	memcpy( kats, &out, 8 );
	des_ecb3_encrypt( (des_cblock *)(r_rn+4), &out, k2, k1, k2, DES_ENCRYPT );
	memcpy( kats+8, &out, 8 );

	/* Generate cryptogram */
	des_set_key( (des_cblock *) kats, k1 );
	des_set_key( (des_cblock *) (kats+8), k2 );

	des_ecb3_encrypt( (des_cblock *)chall, &out, k1, k2, k1, DES_ENCRYPT );

	memset( &k1, 0, sizeof(k1) );
	memset( &k2, 0, sizeof(k2) );

	/* Check cryptogram */
	if( memcmp( r_rn, out+4, 4 )!=0 ) return( FALSE );

	memset( out, 0, sizeof(out) );

	return( TRUE );
#else
	return( FALSE );
#endif /* WITH_DES */
}

/* Generate diversified key PK */

int scGpk4000GenDivKeyPK( const BYTE *key, const BYTE *chall,
	const BYTE *cr_ctc, BYTE *kpts )
{
#ifdef WITH_DES
	des_key_schedule k1;
	des_key_schedule k2;
	des_cblock out;
	des_cblock ctc;

	des_check_key=0;

	/* Generate session key */
	des_set_key( (des_cblock *) key, k1 );
	des_set_key( (des_cblock *) (key+8), k2 );

	memset( ctc, 0x00, sizeof(ctc) );
	memcpy( ctc+5, cr_ctc+5, 3 );

	des_ecb3_encrypt( &ctc, &out, k1, k2, k1, DES_ENCRYPT );
	memcpy( kpts, &out, 8 );
	des_ecb3_encrypt( &ctc, &out, k2, k1, k2, DES_ENCRYPT );
	memcpy( kpts+8, &out, 8 );

	/* Generate cryptogram */
	des_set_key( (des_cblock *) kpts, k1 );
	des_set_key( (des_cblock *) (kpts+8), k2 );

	des_ecb3_encrypt( (des_cblock *)chall, &out, k1, k2, k1, DES_ENCRYPT );

	memset( &k1, 0, sizeof(k1) );
	memset( &k2, 0, sizeof(k2) );
	memset( ctc, 0, sizeof(ctc) );

	/* Check cryptogram */
	if( memcmp( cr_ctc, out+4, 4 ) ) return( FALSE );

	memset( out, 0, sizeof(out) );

	return( TRUE );
#else
	return( FALSE );
#endif /* WITH_DES */
}

/* Generate CRYCKS */

void scGpk4000GenCrycks( const BYTE *key, const BYTE *data, int datalen,
	BYTE *crycks0, BYTE *crycks1 )
{
#ifdef WITH_DES
	des_key_schedule k1;
	des_key_schedule k2;
	BYTE in[8];
	BYTE out[8];
	BYTE block[64];
	int i;
	int pointer=0;

	if( datalen>64 ) datalen=64;

	des_check_key=0;

	/* Do key schedule */
	des_set_key( (des_cblock *) key, k1 );
	des_set_key( (des_cblock *) (key+8), k2 );

	/* Fill block with 0x00 and then with the data. */
	memset( block, 0x00, 64 );
	memcpy( block, data, datalen );

	/* Set IV */
	memset( in, 0x00, 8 );

	/* Could have used the correct mode, but speed is not critical
	 * and a replacement if probably easier.
	 */
	while( datalen > 0 ) {
		for( i=0; i<8; i++ ) in[i]^=block[i+pointer];
		des_ecb3_encrypt( (des_cblock *)in, (des_cblock *)out, k1, k2, k1,
			DES_ENCRYPT );
		memcpy( in, out, 8 );

		pointer+=8;
		datalen-=8;
	}

	/* Copy cryptogram */
	if( crycks0!=NULL ) memcpy( crycks0, out+5, 3 );
	if( crycks1!=NULL ) memcpy( crycks1, out, 3 );

	memset( &k1, 0, sizeof(k1) );
	memset( &k2, 0, sizeof(k2) );
	memset( in, 0, sizeof(in) );
	memset( out, 0, sizeof(out) );
	memset( block, 0, sizeof(block) );
#endif /* WITH_DES */
}

/* Check Signature */
int scGpk4000CheckSign( const BYTE *key, BYTE pfile, const BYTE *ctc,
	const BYTE *tv, const BYTE *bal, const BYTE *sign )
{
#ifdef WITH_DES
	des_key_schedule k1;
	des_key_schedule k2;
	des_cblock kt1;
	des_cblock kt2;
	BYTE in[8];
	des_cblock out;

	memset( in, 0, sizeof( in ) );
	memcpy( in+5, ctc, 3 );

	des_check_key=0;

	des_set_key( (des_cblock *) key, k1 );
	des_set_key( (des_cblock *) (key+8), k2 );

	des_ecb3_encrypt( &in, &kt1, k1, k2, k1, DES_ENCRYPT );
	des_ecb3_encrypt( &in, &kt2, k2, k1, k2, DES_ENCRYPT );

	des_set_key( &kt1, k1 );
	des_set_key( &kt2, k2 );
   
	memset( in, 0, 8 );
	in[1]=pfile&0x1F;
	memcpy( in+2, tv, 3 );
	memcpy( in+5, bal, 3 );

	des_ecb3_encrypt( (des_cblock *)in, &out, k1, k2, k1, DES_ENCRYPT );

	memset( &k1, 0, sizeof(k1) );
	memset( &k2, 0, sizeof(k2) );
	memset( kt1, 0, sizeof(kt1) );
	memset( kt2, 0, sizeof(kt2) );
	memset( in, 0, sizeof(in) );

	if( memcmp( out+4, sign, 4 ) ) return( FALSE );

	memset( out, 0, sizeof(out) );

	return( TRUE );
#else
	return( FALSE );
#endif /* WITH_DES */
}

/* Generate CKS for Key Elements */

BYTE scGpk4000GenCKS( BYTE tag, const BYTE *data, int datalen )
{
	int i;

	for( i=0; i<datalen; i++ ) tag^=data[i];

	return( ~tag );
}

/* Extracts last nibble to form the Secret Code.
 * in is 8 bytes long, out is 4 bytes long.
 */
void scGpk4000ComprSC( const BYTE *in, BYTE *out )
{
	int i;

	for( i=0; i<4; i++ )
		out[i] = (in[i*2] << 4) | (in[i*2+1] & 0x0F);

	return;
}

/****************************************************************************
*																			*
*							Administration Commands							*
*																			*
****************************************************************************/

int scGpk4000CmdApdRec( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	BYTE sfid, const BYTE *data, BYTE datalen )
{
	BYTE cmd[ 5+256 ];
	BYTE rsp[ SC_GENERAL_SHORT_DATA_SIZE+2 ];
	SC_APDU apdu;
	int ret;

	ci->sw[0]=0x00; ci->sw[1]=0x00;

	apdu.cse=SC_APDU_CASE_3_SHORT;
	apdu.cmd=cmd;
	apdu.cmdlen=5 + datalen;
	apdu.rsp=rsp;
	apdu.rsplen=0;

	memcpy( cmd+5, data, datalen );

	cmd[0]=0x00;
	cmd[1]=0xE2;
	cmd[2]=0x00;
	cmd[3]=sfid<<3;
	cmd[4]=datalen;

	ret=scReaderSendAPDU( ri, ci, &apdu );
	memset( cmd, 0, sizeof(cmd) );
	if( ret!=SC_EXIT_OK ) return( ret );

	if( apdu.rsplen!=2 ) return( SC_EXIT_BAD_SW );

	ci->sw[0] = apdu.rsp[0];
	ci->sw[1] = apdu.rsp[1];

	return( SC_EXIT_OK );
}

int scGpk4000CmdApdRecCrycks( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	BYTE sfid, const BYTE *data, BYTE datalen )
{
#ifdef WITH_DES
	BYTE cmd[ 64+5+3+1 ];
	BYTE rsp[ SC_GENERAL_SHORT_DATA_SIZE+2 ];
	BYTE crycks[ 3 ];
	SC_APDU apdu;
	int ret;

	ci->sw[0]=0x00; ci->sw[1]=0x00;

	if( datalen>61 ) return( SC_EXIT_BAD_PARAM );

	if( !ci->crypt.encrypt ) return( SC_EXIT_BAD_PARAM );

	apdu.cse=SC_APDU_CASE_4_SHORT;
	apdu.cmd=cmd;
	apdu.cmdlen=5+datalen+3;
	apdu.rsp=rsp;
	apdu.rsplen=0;

	memcpy( cmd+5, data, datalen );

	cmd[0]=0x04;
	cmd[1]=0xE2;
	cmd[2]=0x00;
	cmd[3]=sfid<<3;
	cmd[4]=datalen+3;

	scGpk4000GenCrycks( ci->crypt.key, cmd, 5+datalen, cmd+5+datalen,
		crycks );

	apdu.cmd[apdu.cmdlen]=0x03;
	apdu.cmdlen++;

	ret=scReaderSendAPDU( ri, ci, &apdu );
	memset( cmd, 0, sizeof(cmd) );
	if( ret!=SC_EXIT_OK ) return( ret );

	if( apdu.rsplen<2 ) return( SC_EXIT_BAD_SW );

	ci->sw[0] = apdu.rsp[apdu.rsplen-2];
	ci->sw[1] = apdu.rsp[apdu.rsplen-1];

	if( (apdu.rsplen==5) && !memcmp( crycks, rsp, 3 ) ) return( SC_EXIT_OK );

	return( SC_EXIT_BAD_CHECKSUM );
#else
	return( SC_EXIT_NOT_SUPPORTED );
#endif /* WITH_DES */
}

int scGpk4000CmdCrtFile( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	WORD fid, WORD flen, BYTE fdb, BYTE reclen, const BYTE *ac )
{
	BYTE cmd[ 5+12 ];
	BYTE rsp[ SC_GENERAL_SHORT_DATA_SIZE+2 ];
	SC_APDU apdu;
	int ret;

	ci->sw[0]=0x00; ci->sw[1]=0x00;

	apdu.cse=SC_APDU_CASE_3_SHORT;
	apdu.cmd=cmd;
	apdu.cmdlen=17;
	apdu.rsp=rsp;
	apdu.rsplen=0;

	cmd[0]=0x80;
	cmd[1]=0xE0;
	cmd[2]=0x02;
	cmd[3]=0x00;
	cmd[4]=0x0C;
	cmd[5]=(fid>>8)&0xFF;
	cmd[6]=fid&0xFF;
	cmd[7]=fdb;
	cmd[8]=reclen;
	cmd[9]=(flen>>8)&0xFF;
	cmd[10]=flen&0xFF;
	memcpy( cmd+11, ac, 6 );

	ret=scReaderSendAPDU( ri, ci, &apdu );
	memset( cmd, 0, sizeof(cmd) );
	if( ret!=SC_EXIT_OK ) return( ret );

	if( apdu.rsplen!=2 ) return( SC_EXIT_BAD_SW );

	ci->sw[0] = apdu.rsp[0];
	ci->sw[1] = apdu.rsp[1];

	return( SC_EXIT_OK );
}

int scGpk4000CmdCrtFileCrycks( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	WORD fid, WORD flen, BYTE fdb, BYTE reclen, const BYTE *ac )
{
#ifdef WITH_DES
	BYTE cmd[ 5+12+3+1 ];
	BYTE rsp[ SC_GENERAL_SHORT_DATA_SIZE+2 ];
	BYTE crycks[ 3 ];
	SC_APDU apdu;
	int ret;

	ci->sw[0]=0x00; ci->sw[1]=0x00;

	if( !ci->crypt.encrypt ) return( SC_EXIT_BAD_PARAM );

	apdu.cse=SC_APDU_CASE_4_SHORT;
	apdu.cmd=cmd;
	apdu.cmdlen=17+3;
	apdu.rsp=rsp;
	apdu.rsplen=0;

	cmd[0]=0x84;
	cmd[1]=0xE0;
	cmd[2]=0x02;
	cmd[3]=0x00;
	cmd[4]=0x0C+3;
	cmd[5]=(fid>>8)&0xFF;
	cmd[6]=fid&0xFF;
	cmd[7]=fdb;
	cmd[8]=reclen;
	cmd[9]=(flen>>8)&0xFF;
	cmd[10]=flen&0xFF;
	memcpy( cmd+11, ac, 6 );

	scGpk4000GenCrycks( ci->crypt.key, cmd, 5+12, cmd+5+12, crycks );

	apdu.cmd[apdu.cmdlen]=0x03;
	apdu.cmdlen++;

	ret=scReaderSendAPDU( ri, ci, &apdu );
	memset( cmd, 0, sizeof(cmd) );
	if( ret!=SC_EXIT_OK ) return( ret );

	if( apdu.rsplen<2 ) return( SC_EXIT_BAD_SW );

	ci->sw[0] = apdu.rsp[apdu.rsplen-2];
	ci->sw[1] = apdu.rsp[apdu.rsplen-1];

	if( (apdu.rsplen==5) && !memcmp( crycks, rsp, 3 ) ) return( SC_EXIT_OK );

	return( SC_EXIT_BAD_CHECKSUM );
#else
	return( SC_EXIT_NOT_SUPPORTED );
#endif /* WITH_DES */
}

int scGpk4000CmdCrtDir( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	WORD fid, const BYTE *name, BYTE namelen, BYTE opt, const BYTE *ac )
{
	BYTE cmd[ 5+28 ];
	BYTE rsp[ SC_GENERAL_SHORT_DATA_SIZE+2 ];
	SC_APDU apdu;
	int ret;

	ci->sw[0]=0x00; ci->sw[1]=0x00;

	if( namelen>16 ) return( SC_EXIT_BAD_PARAM );

	apdu.cse=SC_APDU_CASE_3_SHORT;
	apdu.cmd=cmd;
	apdu.cmdlen=17;
	apdu.rsp=rsp;
	apdu.rsplen=0;

	cmd[0]=0x80;
	cmd[1]=0xE0;
	cmd[2]=0x01;
	cmd[3]=0x00;
	cmd[4]=(0x0C+namelen)&0xFF;
	cmd[5]=(fid>>8)&0xFF;
	cmd[6]=fid&0xFF;
	cmd[7]=0x38;
	cmd[8]=opt;
	cmd[9]=0x00;
	cmd[10]=namelen;
	memcpy( cmd+11, ac, 4 );
	cmd[15]=0x00;
	cmd[16]=0x00;
	memcpy( cmd+17, name, namelen );
	apdu.cmdlen+=namelen;

	ret=scReaderSendAPDU( ri, ci, &apdu );
	memset( cmd, 0, sizeof(cmd) );
	if( ret!=SC_EXIT_OK ) return( ret );

	if( apdu.rsplen!=2 ) return( SC_EXIT_BAD_SW );

	ci->sw[0] = apdu.rsp[0];
	ci->sw[1] = apdu.rsp[1];

	return( SC_EXIT_OK );
}

int scGpk4000CmdCrtDirCrycks( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	WORD fid, const BYTE *name, BYTE namelen, BYTE opt, const BYTE *ac )
{
#ifdef WITH_DES
	BYTE cmd[ 5+28+3+1 ];
	BYTE rsp[ SC_GENERAL_SHORT_DATA_SIZE+2 ];
	BYTE crycks[ 3 ];
	SC_APDU apdu;
	int ret;

	ci->sw[0]=0x00; ci->sw[1]=0x00;

	if( namelen>16 ) return( SC_EXIT_BAD_PARAM );

	if( !ci->crypt.encrypt ) return( SC_EXIT_BAD_PARAM );

	apdu.cse=SC_APDU_CASE_4_SHORT;
	apdu.cmd=cmd;
	apdu.cmdlen=17+3+namelen;
	apdu.rsp=rsp;
	apdu.rsplen=0;

	cmd[0]=0x84;
	cmd[1]=0xE0;
	cmd[2]=0x01;
	cmd[3]=0x00;
	cmd[4]=(0x0C+namelen+3)&0xFF;
	cmd[5]=(fid>>8)&0xFF;
	cmd[6]=fid&0xFF;
	cmd[7]=0x38;
	cmd[8]=opt;
	cmd[9]=0x00;
	cmd[10]=namelen;
	memcpy( cmd+11, ac, 4 );
	cmd[15]=0x00;
	cmd[16]=0x00;
	memcpy( cmd+17, name, namelen );

	scGpk4000GenCrycks( ci->crypt.key, cmd, 5+12+namelen, cmd+5+12+namelen,
		crycks );

	apdu.cmd[apdu.cmdlen]=0x03;
	apdu.cmdlen++;

	ret=scReaderSendAPDU( ri, ci, &apdu );
	memset( cmd, 0, sizeof(cmd) );
	if( ret!=SC_EXIT_OK ) return( ret );

	if( apdu.rsplen<2 ) return( SC_EXIT_BAD_SW );

	ci->sw[0] = apdu.rsp[apdu.rsplen-2];
	ci->sw[1] = apdu.rsp[apdu.rsplen-1];

	if( (apdu.rsplen==5) && !memcmp( crycks, rsp, 3 ) ) return( SC_EXIT_OK );

	return( SC_EXIT_BAD_CHECKSUM );
#else
	return( SC_EXIT_NOT_SUPPORTED );
#endif /* WITH_DES */
}

int scGpk4000CmdExtAuth( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	BYTE keynum, BOOLEAN global, BYTE sfid, const BYTE *key,
	const BYTE *chall )
{
#ifdef WITH_DES
	BYTE cmd[]={ 0x00, 0x82, 0x00, 0x80, 0x06, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00 };
	BYTE rsp[ SC_GENERAL_SHORT_DATA_SIZE+2 ];
	SC_APDU apdu;
	int ret;

	ci->sw[0]=0x00; ci->sw[1]=0x00;

	apdu.cse=SC_APDU_CASE_3_SHORT;
	apdu.cmd=cmd;
	apdu.cmdlen=11;
	apdu.rsp=rsp;
	apdu.rsplen=0;

	if( global ) cmd[3]=0x00;
	cmd[5]=keynum<<1;
	cmd[6]=sfid;

	scGpk4000GenerateEac( key, chall, cmd+7 );

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

int scGpk4000CmdFreezeAc( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	BOOLEAN file, WORD fid, const BYTE *control )
{
	BYTE cmd[]={ 0x80, 0x16, 0x01, 0x00, 0x05, 0x00, 0x00, 0x00,
		0x00, 0x00 };
	BYTE rsp[ SC_GENERAL_SHORT_DATA_SIZE+2 ];
	SC_APDU apdu;
	int ret;

	ci->sw[0]=0x00; ci->sw[1]=0x00;

	apdu.cse=SC_APDU_CASE_3_SHORT;
	apdu.cmd=cmd;
	apdu.cmdlen=10;
	apdu.rsp=rsp;
	apdu.rsplen=0;

	if( file==TRUE ) cmd[2]=0x02;
	cmd[5]=(fid>>8)&0xFF;
	cmd[6]=fid&0xFF;
	memcpy( cmd+7, control, 3 );

	ret=scReaderSendAPDU( ri, ci, &apdu );
	memset( cmd, 0, sizeof(cmd) );
	if( ret!=SC_EXIT_OK ) return( ret );

	if( apdu.rsplen!=2 ) return( SC_EXIT_BAD_SW );

	ci->sw[0] = apdu.rsp[0];
	ci->sw[1] = apdu.rsp[1];

	return( SC_EXIT_OK );
}

int scGpk4000CmdFreezeAcCrycks( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	BOOLEAN file, WORD fid, const BYTE *control )
{
#ifdef WITH_DES
	BYTE cmd[]={ 0x84, 0x16, 0x01, 0x00, 0x08, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x03 };
	BYTE rsp[ SC_GENERAL_SHORT_DATA_SIZE+2 ];
	BYTE crycks[ 3 ];
	SC_APDU apdu;
	int ret;

	ci->sw[0]=0x00; ci->sw[1]=0x00;

	if( !ci->crypt.encrypt ) return( SC_EXIT_BAD_PARAM );

	apdu.cse=SC_APDU_CASE_4_SHORT;
	apdu.cmd=cmd;
	apdu.cmdlen=14;
	apdu.rsp=rsp;
	apdu.rsplen=0;

	if( file==TRUE ) cmd[2]=0x02;
	cmd[5]=(fid>>8)&0xFF;
	cmd[6]=fid&0xFF;
	memcpy( cmd+7, control, 3 );

	scGpk4000GenCrycks( ci->crypt.key, cmd, 5+5, cmd+5+5, crycks );

	ret=scReaderSendAPDU( ri, ci, &apdu );
	memset( cmd, 0, sizeof(cmd) );
	if( ret!=SC_EXIT_OK ) return( ret );

	if( apdu.rsplen<2 ) return( SC_EXIT_BAD_SW );

	ci->sw[0] = apdu.rsp[apdu.rsplen-2];
	ci->sw[1] = apdu.rsp[apdu.rsplen-1];

	if( (apdu.rsplen==5) && !memcmp( crycks, rsp, 3 ) ) return( SC_EXIT_OK );

	return( SC_EXIT_BAD_CHECKSUM );
#else
	return( SC_EXIT_NOT_SUPPORTED );
#endif /* WITH_DES */
}

int scGpk4000CmdGetChall( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	BYTE *chall, BOOLEAN longchall )
{
	BYTE cmd[]={ 0x00, 0x84, 0x00, 0x00, 0x08 };
	BYTE rsp[ SC_GENERAL_SHORT_DATA_SIZE+2 ];
	SC_APDU apdu;
	int ret;

	ci->sw[0]=0x00; ci->sw[1]=0x00;

	apdu.cse=SC_APDU_CASE_2_SHORT;
	apdu.cmd=cmd;
	apdu.cmdlen=5;
	apdu.rsp=rsp;
	apdu.rsplen=0;

	if( longchall ) {
		cmd[0]=0x80;
		cmd[4]=0x20;
	}

	ret=scReaderSendAPDU( ri, ci, &apdu );
	memset( cmd, 0, sizeof(cmd) );
	if( ret!=SC_EXIT_OK ) return( ret );

	if( apdu.rsplen<2 ) return( SC_EXIT_BAD_SW );

	ci->sw[0] = apdu.rsp[apdu.rsplen-2];
	ci->sw[1] = apdu.rsp[apdu.rsplen-1];

	if( apdu.rsplen==2 ) return( SC_EXIT_UNKNOWN_ERROR );
	if( longchall ) {
		if( apdu.rsplen!=34 )
			return( SC_EXIT_UNKNOWN_ERROR );
	} else {
		if( apdu.rsplen!=10 )
			return( SC_EXIT_UNKNOWN_ERROR );
	}

	memcpy( chall, apdu.rsp, apdu.rsplen-2 );

	memset( rsp, 0, sizeof(rsp) );

	return( SC_EXIT_OK );
}

/* For SC_GPK4000_INFO_KEY_DATA resplen is used to suppley expected length. */

int scGpk4000CmdGetInfo( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	BYTE p2, BYTE *resp, int *resplen )
{
	BYTE cmd[]={ 0x80, 0xC0, 0x02, 0x00, 0x00 };
	BYTE rsp[ SC_GENERAL_SHORT_DATA_SIZE+2 ];
	SC_APDU apdu;
	int ret;

	ci->sw[0]=0x00; ci->sw[1]=0x00;

	apdu.cse=SC_APDU_CASE_2_SHORT;
	apdu.cmd=cmd;
	apdu.cmdlen=5;
	apdu.rsp=rsp;
	apdu.rsplen=0;

	cmd[3] = p2;

	switch( p2 ) {
		case SC_GPK4000_INFO_KEY_DATA:
			if( *resplen>0x256 ) return( SC_EXIT_BAD_PARAM );
			cmd[4]=*resplen&0xFF;
			break;
		case SC_GPK4000_INFO_DESC_ADDR:
			cmd[4]=0x02;
			break;
		case SC_GPK4000_INFO_CHIP_SN:
			cmd[4]=0x08;
			break;
		case SC_GPK4000_INFO_CARD_SN:
			cmd[4]=0x08;
			break;
		case SC_GPK4000_INFO_ISSUER_SN:
			cmd[4]=0x08;
			break;
		case SC_GPK4000_INFO_ISSUER_REF:
			cmd[4]=0x04;
			break;
		case SC_GPK4000_INFO_PRE_ISSUING:
			cmd[4]=0x0D;
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

	return( SC_EXIT_OK );
}

/* sw is used to get the length of the available data. */
int scGpk4000CmdGetResp( SC_READER_INFO *ri, SC_CARD_INFO *ci,
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

int scGpk4000CmdIntAuth( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	BYTE keynum, BOOLEAN global, BYTE sfid, const BYTE *key,
	const BYTE *chall )
{
#ifdef WITH_DES
	BYTE cmd[16];
	BYTE rsp[ SC_GENERAL_SHORT_DATA_SIZE+2 ];
	SC_APDU apdu;
	int ret;

	memset( cmd, 0, sizeof(cmd) );
	cmd[1]=0x88;
	cmd[3]=0x80;
	cmd[4]=0x0A;

	ci->sw[0]=0x00; ci->sw[1]=0x00;

	apdu.cse=SC_APDU_CASE_4_SHORT;
	apdu.cmd=cmd;
	apdu.cmdlen=16;
	apdu.rsp=rsp;
	apdu.rsplen=0;

	if( global ) cmd[3]=0x00;
	cmd[5]=keynum<<1;
	cmd[6]=sfid;
	memcpy( cmd+7, chall, 8 );

	ret=scReaderSendAPDU( ri, ci, &apdu );
	memset( cmd, 0, sizeof(cmd) );
	if( ret!=SC_EXIT_OK ) return( ret );

	if( apdu.rsplen<2 ) return( SC_EXIT_BAD_SW );

	ci->sw[0] = apdu.rsp[apdu.rsplen-2];
	ci->sw[1] = apdu.rsp[apdu.rsplen-1];

	if( scGpk4000CompareEac( key, chall, rsp ) ) {
		memset( rsp, 0, sizeof(rsp) );
		return( SC_EXIT_OK );
	}

	memset( rsp, 0, sizeof(rsp) );

	return( SC_EXIT_BAD_CHECKSUM );
#else
	return( SC_EXIT_NOT_SUPPORTED );
#endif /* WITH_DES */
}

/* offset can be sfid/soffset */
int scGpk4000CmdRdBin( SC_READER_INFO *ri, SC_CARD_INFO *ci,
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

/* offset can be sfid/soffset */
int scGpk4000CmdRdBinCrycks( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	WORD offset, BYTE *data, int *datalen )
{
#ifdef WITH_DES
	BYTE cmd[ 5+64 ];
	BYTE rsp[ SC_GENERAL_SHORT_DATA_SIZE+2 ];
	BYTE crycks[ 3 ];
	SC_APDU apdu;
	int ret;

	ci->sw[0]=0x00; ci->sw[1]=0x00;

	if( *datalen>61 ) return( SC_EXIT_BAD_PARAM );

	if( !ci->crypt.encrypt ) return( SC_EXIT_BAD_PARAM );

	apdu.cse=SC_APDU_CASE_2_SHORT;
	apdu.cmd=cmd;
	apdu.cmdlen=5;
	apdu.rsp=rsp;
	apdu.rsplen=0;

	memset( cmd, 0, sizeof(cmd) );
	cmd[0] = 0x04;
	cmd[1] = 0xB0;
	cmd[2] = offset>>8;
	cmd[3] = offset&0xFF;
	cmd[4] = *datalen+3;

	ret=scReaderSendAPDU( ri, ci, &apdu );
	if( ret!=SC_EXIT_OK ) return( ret );

	if( apdu.rsplen<2 ) return( SC_EXIT_BAD_SW );

	ci->sw[0] = apdu.rsp[apdu.rsplen-2];
	ci->sw[1] = apdu.rsp[apdu.rsplen-1];

	if( apdu.rsplen>=5 ) {
		apdu.rsplen-=5;
		memcpy( data, apdu.rsp, apdu.rsplen );
		*datalen=apdu.rsplen;

		memcpy( cmd+5, apdu.rsp, apdu.rsplen );
		scGpk4000GenCrycks( ci->crypt.key, cmd, 5+apdu.rsplen, NULL, crycks );
	    memset( cmd, 0, sizeof(cmd) );

		if( !memcmp( crycks, rsp+apdu.rsplen, 3 ) ) {
			memset( rsp, 0, sizeof(rsp) );
			return( SC_EXIT_OK );
		}
	} else {
		*datalen=0;
	}

	memset( rsp, 0, sizeof(rsp) );

	return( SC_EXIT_BAD_CHECKSUM );
#else
	return( SC_EXIT_NOT_SUPPORTED );
#endif /* WITH_DES */
}

/* Uses sfid if it is in the correct range. */
int scGpk4000CmdRdRec( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	BYTE recnum, BYTE sfid, BYTE *data, int *datalen )
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

	cmd[2] = recnum;
	cmd[3] = 0x04;
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

/* Uses sfid if it is in the correct range. */
int scGpk4000CmdRdRecCrycks( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	BYTE recnum, BYTE sfid, BYTE *data, int *datalen )
{
#ifdef WITH_DES
	BYTE cmd[ 5+64 ];
	BYTE rsp[ SC_GENERAL_SHORT_DATA_SIZE+2 ];
	BYTE crycks[ 3 ];
	SC_APDU apdu;
	int ret;

	ci->sw[0]=0x00; ci->sw[1]=0x00;

	if( *datalen>61 ) return( SC_EXIT_BAD_PARAM );

	if( !ci->crypt.encrypt ) return( SC_EXIT_BAD_PARAM );

	apdu.cse=SC_APDU_CASE_2_SHORT;
	apdu.cmd=cmd;
	apdu.cmdlen=5;
	apdu.rsp=rsp;
	apdu.rsplen=0;

	memset( cmd, 0, sizeof(cmd) );
	cmd[0] = 0x04;
	cmd[1] = 0xB2;
	cmd[2] = recnum;
	cmd[3] = 0x04;
	if( (sfid>0) && (sfid<32) ) cmd[3]|=(sfid<<3)&0xFF;
	cmd[4] = *datalen+3;

	ret=scReaderSendAPDU( ri, ci, &apdu );
	if( ret!=SC_EXIT_OK ) return( ret );

	if( apdu.rsplen<2 ) return( SC_EXIT_BAD_SW );

	ci->sw[0] = apdu.rsp[apdu.rsplen-2];
	ci->sw[1] = apdu.rsp[apdu.rsplen-1];

	if( apdu.rsplen>=5 ) {
		apdu.rsplen-=5;
		memcpy( data, apdu.rsp, apdu.rsplen );
		*datalen=apdu.rsplen;

		memcpy( cmd+5, apdu.rsp, apdu.rsplen );
		scGpk4000GenCrycks( ci->crypt.key, cmd, 5+apdu.rsplen, NULL, crycks );
	    memset( cmd, 0, sizeof(cmd) );
		if( !memcmp( crycks, rsp+apdu.rsplen, 3 ) ) {
			memset( rsp, 0, sizeof(rsp) );
			return( SC_EXIT_OK );
		}
	} else {
		*datalen=0;
	}

	memset( rsp, 0, sizeof(rsp) );

	return( SC_EXIT_BAD_CHECKSUM );
#else
	return( SC_EXIT_NOT_SUPPORTED );
#endif /* WITH_DES */
}

int scGpk4000CmdSelFil( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	BYTE type, WORD fid, const BYTE *aid, BYTE aidlen, BYTE *resp,
	int *resplen )
{
	BYTE cmd[22];
	BYTE rsp[ SC_GENERAL_SHORT_DATA_SIZE+2 ];
	SC_APDU apdu;
	int ret;

	memset( cmd, 0, sizeof(cmd) );
	cmd[1]=0xA4;
	cmd[3]=0x0C;
	cmd[4]=0x02;

	ci->sw[0]=0x00; ci->sw[1]=0x00;

	if( (aidlen>16) && (type==0x04) ) return( SC_EXIT_BAD_PARAM );
	if( type>0x04 ) return( SC_EXIT_BAD_PARAM );

	apdu.cse=SC_APDU_CASE_3_SHORT;
	apdu.cmd=cmd;
	apdu.cmdlen=7;
	apdu.rsp=rsp;
	apdu.rsplen=0;

	cmd[2]=type;

	if( resp!=NULL ) {
		cmd[3]=0x00;
		apdu.cmdlen++;
		if( apdu.cmdlen==5 ) apdu.cse=SC_APDU_CASE_2_SHORT;
		else apdu.cse=SC_APDU_CASE_4_SHORT;
	}

	if( type==0x04 ) {
		cmd[4]=aidlen;
		memcpy( cmd+5, aid, aidlen );
		apdu.cmdlen=5+aidlen;
		if( apdu.cse==SC_APDU_CASE_4_SHORT ) apdu.cmdlen++;
	} else {
		cmd[5]=fid>>8;
		cmd[6]=fid & 0xFF;
	}

	if( type==0x03 ) {
		if( ci->type!=SC_CARD_BRADESCO ) return( SC_EXIT_BAD_PARAM );

		apdu.cmdlen=5;
		cmd[4]=0x00;
		/* This command is in principle a case 2 command, but it is
		 * handled by the card as a case 4. With this some intelligent
		 * readers have problems.
		 */
		apdu.cse=SC_APDU_CASE_4_SHORT;
	}

	ret=scReaderSendAPDU( ri, ci, &apdu );
	memset( cmd, 0, sizeof(cmd) );
	if( ret!=SC_EXIT_OK ) return( ret );

	if( apdu.rsplen<2 ) return( SC_EXIT_BAD_SW );

	ci->sw[0] = apdu.rsp[apdu.rsplen-2];
	ci->sw[1] = apdu.rsp[apdu.rsplen-1];

	apdu.rsplen-=2;

	if( (resp!=NULL) ) {
		memcpy( resp, apdu.rsp, apdu.rsplen );
		*resplen=apdu.rsplen;
	}

	memset( rsp, 0, sizeof(rsp) );

	return( SC_EXIT_OK );
}

int scGpk4000CmdSelFk( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	BYTE keynum, BOOLEAN global, BYTE sfid, const BYTE *key, const BYTE *rand )
{
#ifdef WITH_DES
	BYTE cmd[]={ 0x80, 0x28, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
	BYTE rsp[ SC_GENERAL_SHORT_DATA_SIZE+2 ];
	SC_APDU apdu;
	int ret;

	ci->sw[0]=0x00; ci->sw[1]=0x00;

	apdu.cse=SC_APDU_CASE_4_SHORT;
	apdu.cmd=cmd;
	apdu.cmdlen=14;
	apdu.rsp=rsp;
	apdu.rsplen=0;

	cmd[2]=(keynum<<1)&0xFE;
	cmd[3]=sfid&0x1F;
	if( !global ) cmd[3]|=0x20;
	memcpy( cmd+5, rand, 8 );

	ret=scReaderSendAPDU( ri, ci, &apdu );
	memset( cmd, 0, sizeof(cmd) );
	if( ret!=SC_EXIT_OK ) return( ret );

	if( apdu.rsplen<2 ) return( SC_EXIT_BAD_SW );

	ci->sw[0] = apdu.rsp[apdu.rsplen-2];
	ci->sw[1] = apdu.rsp[apdu.rsplen-1];

	apdu.rsplen-=2;

	ci->crypt.encrypt=FALSE;

	if( apdu.rsplen==12 ) {
		if( scGpk4000GenDivKey( key, rand, rsp, ci->crypt.key ) ) {
			ci->crypt.encrypt=TRUE;
			memset( rsp, 0, sizeof(rsp) );
			return( SC_EXIT_OK );
		}
	}

	memset( rsp, 0, sizeof(rsp) );

	return( SC_EXIT_BAD_CHECKSUM );
#else
	return( SC_EXIT_NOT_SUPPORTED );
#endif /* WITH_DES */
}

int scGpk4000CmdSetCardStatus( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	BYTE system )
{
	BYTE cmd[]={ 0x80, 0xD8, 0xFF, 0xFF, 0x01, 0x00 };
	BYTE rsp[ SC_GENERAL_SHORT_DATA_SIZE+2 ];
	SC_APDU apdu;
	int ret;

	ci->sw[0]=0x00; ci->sw[1]=0x00;

	apdu.cse=SC_APDU_CASE_3_SHORT;
	apdu.cmd=cmd;
	apdu.cmdlen=6;
	apdu.rsp=rsp;
	apdu.rsplen=0;

	cmd[5]=system;

	ret=scReaderSendAPDU( ri, ci, &apdu );
	memset( cmd, 0, sizeof(cmd) );
	if( ret!=SC_EXIT_OK ) return( ret );

	if( apdu.rsplen!=2 ) return( SC_EXIT_BAD_SW );

	ci->sw[0] = apdu.rsp[0];
	ci->sw[1] = apdu.rsp[1];

	return( SC_EXIT_OK );
}

/* auth is either the current secret code or the unlock secret code. */
int scGpk4000CmdSetCod( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	BOOLEAN unlock, BYTE scn, const BYTE *auth, const BYTE *newpin )
{
	BYTE cmd[]={ 0x80, 0x24, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00 };
	BYTE rsp[ SC_GENERAL_SHORT_DATA_SIZE+2 ];
	SC_APDU apdu;
	int ret;

	ci->sw[0]=0x00; ci->sw[1]=0x00;

	apdu.cse=SC_APDU_CASE_3_SHORT;
	apdu.cmd=cmd;
	apdu.cmdlen=5+4+4;
	apdu.rsp=rsp;
	apdu.rsplen=0;

	if( unlock ) cmd[2]=0x01;
	cmd[3]=scn;
	memcpy( cmd+5, auth, 4 );
	memcpy( cmd+5+4, newpin, 4 );

	ret=scReaderSendAPDU( ri, ci, &apdu );
	memset( cmd, 0, sizeof(cmd) );
	if( ret!=SC_EXIT_OK ) return( ret );

	if( apdu.rsplen!=2 ) return( SC_EXIT_BAD_SW );

	ci->sw[0] = apdu.rsp[0];
	ci->sw[1] = apdu.rsp[1];

	return( SC_EXIT_OK );
}

/* auth is either the current secret code or the unlock secret code. */
int scGpk4000CmdSetCodCrycks( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	BOOLEAN unlock, BYTE scn, const BYTE *auth, const BYTE *newpin )
{
#ifdef WITH_DES
	BYTE cmd[]={ 0x84, 0x24, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00 };
	BYTE rsp[ SC_GENERAL_SHORT_DATA_SIZE+2 ];
	BYTE sc[ 8 ];
	SC_APDU apdu;
	int ret;
	des_key_schedule ks1;
	des_key_schedule ks2;

	ci->sw[0]=0x00; ci->sw[1]=0x00;

	if( !ci->crypt.encrypt ) return( SC_EXIT_BAD_PARAM );

	apdu.cse=SC_APDU_CASE_3_SHORT;
	apdu.cmd=cmd;
	apdu.cmdlen=5+4+4;
	apdu.rsp=rsp;
	apdu.rsplen=0;

	if( unlock ) cmd[2]=0x01;
	cmd[3]=scn;
	memcpy( sc, auth, 4 );
	memcpy( sc+4, newpin, 4 );

	des_check_key=0;

	des_set_key( (des_cblock *) ci->crypt.key, ks1 );
	des_set_key( (des_cblock *) (ci->crypt.key+8), ks2 );

	des_ecb2_encrypt( (des_cblock *)sc, (des_cblock *)(cmd+5), ks1, ks2,
		DES_DECRYPT );

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

int scGpk4000CmdSwtSpd( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	BYTE fd, BYTE cegt )
{
	BYTE cmd[]={ 0x80, 0x14, 0x00, 0x00 };
	BYTE rsp[ SC_GENERAL_SHORT_DATA_SIZE+2 ];
	SC_APDU apdu;
	int ret;

	ci->sw[0]=0x00; ci->sw[1]=0x00;

	apdu.cse=SC_APDU_CASE_1;
	apdu.cmd=cmd;
	apdu.cmdlen=4;
	apdu.rsp=rsp;
	apdu.rsplen=0;

	cmd[2]=fd;
	cmd[3]=0xC0 | (cegt & 0x3F); /* 0xb makes no sense. */

	ret=scReaderSendAPDU( ri, ci, &apdu );
	memset( cmd, 0, sizeof(cmd) );
	if( ret!=SC_EXIT_OK ) return( ret );

	if( apdu.rsplen!=2 ) return( SC_EXIT_BAD_SW );

	ci->sw[0] = apdu.rsp[0];
	ci->sw[1] = apdu.rsp[1];

	return( SC_EXIT_OK );
}

/* offset can be sfid/soffset */
int scGpk4000CmdUpdBin( SC_READER_INFO *ri, SC_CARD_INFO *ci,
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

	cmd[0]=0x00;
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

/* offset can be sfid/soffset */
int scGpk4000CmdUpdBinCrycks( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	int offset, const BYTE *data, BYTE datalen )
{
#ifdef WITH_DES
	BYTE cmd[ 64+5+1 ];
	BYTE rsp[ SC_GENERAL_SHORT_DATA_SIZE+2 ];
	BYTE crycks[ 3 ];
	SC_APDU apdu;
	int ret;

	ci->sw[0]=0x00; ci->sw[1]=0x00;

	if( datalen>61 ) return( SC_EXIT_BAD_PARAM );

	if( !ci->crypt.encrypt ) return( SC_EXIT_BAD_PARAM );

	apdu.cse=SC_APDU_CASE_4_SHORT;
	apdu.cmd=cmd;
	apdu.cmdlen=5+datalen+3;
	apdu.rsp=rsp;
	apdu.rsplen=0;

	cmd[0]=0x04;
	cmd[1]=0xD6;
	cmd[2]=(offset >>8) & 0xFF;
	cmd[3]=offset & 0xFF;
	cmd[4]=datalen+3;

	memcpy( cmd+5, data, datalen );

	scGpk4000GenCrycks( ci->crypt.key, cmd, 5+datalen, cmd+5+datalen,
		crycks );

	apdu.cmd[apdu.cmdlen]=0x03;
	apdu.cmdlen++;

	ret=scReaderSendAPDU( ri, ci, &apdu );
	memset( cmd, 0, sizeof(cmd) );
	if( ret!=SC_EXIT_OK ) return( ret );

	if( apdu.rsplen<2 ) return( SC_EXIT_BAD_SW );

	ci->sw[0] = apdu.rsp[apdu.rsplen-2];
	ci->sw[1] = apdu.rsp[apdu.rsplen-1];

	if( (apdu.rsplen==5) && !memcmp( crycks, rsp, 3 ) ) return( SC_EXIT_OK );

	return( SC_EXIT_BAD_CHECKSUM );
#else
	return( SC_EXIT_NOT_SUPPORTED );
#endif /* WITH_DES */
}

/* offset can be sfid/soffset */
int scGpk4000CmdUpdBinEncr( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	int offset, const BYTE *data )
{
#ifdef WITH_DES
	BYTE cmd[ 5+16+1 ];
	BYTE rsp[ SC_GENERAL_SHORT_DATA_SIZE+2 ];
	BYTE crycks[ 3 ];
	BYTE tmp[ 16 ];
	SC_APDU apdu;
	WORD sum=0;
	int ret, i;
	des_key_schedule ks1;
	des_key_schedule ks2;

	ci->sw[0]=0x00; ci->sw[1]=0x00;

	if( !ci->crypt.encrypt ) return( SC_EXIT_BAD_PARAM );

	apdu.cse=SC_APDU_CASE_4_SHORT;
	apdu.cmd=cmd;
	apdu.cmdlen=5+16+1;
	apdu.rsp=rsp;
	apdu.rsplen=0;

	memset( cmd, 0, sizeof(cmd) );

	cmd[0]=0x04;
	cmd[1]=0xD6;
	cmd[2]=(offset >>8) & 0xFF;
	cmd[3]=offset & 0xFF;
	cmd[4]=16;

	/* Encrypt Data */
	memcpy( tmp, data, 16 );

	tmp[12]=(offset >>8) & 0xFF;
	tmp[13]=offset & 0xFF;

	for( i=0; i<14; i++ ) sum+=tmp[i];
	tmp[14]=(sum >>8) & 0xFF;
	tmp[15]=sum & 0xFF;

	des_check_key=0;

	des_set_key( (des_cblock *) ci->crypt.key, ks1 );
	des_set_key( (des_cblock *) (ci->crypt.key+8), ks2 );

	des_ecb2_encrypt( (des_cblock *)(tmp), (des_cblock *)(cmd+5), ks1, ks2,
		DES_DECRYPT );
	des_ecb2_encrypt( (des_cblock *)(tmp+8), (des_cblock *)(cmd+13), ks1, ks2,
		DES_DECRYPT );

	/* Generate Crycks */
	memcpy( tmp+4, tmp+12, 4 );

	des_ecb2_encrypt( (des_cblock *)(tmp), (des_cblock *)(tmp+8), ks1, ks2,
		DES_ENCRYPT );

	memcpy( crycks, tmp+8, 3 );

	memset( &ks1, 0, sizeof(ks1) );
	memset( &ks2, 0, sizeof(ks2) );
	memset( tmp, 0, sizeof(tmp) );

	ret=scReaderSendAPDU( ri, ci, &apdu );
	memset( cmd, 0, sizeof(cmd) );
	if( ret!=SC_EXIT_OK ) return( ret );

	if( apdu.rsplen<2 ) return( SC_EXIT_BAD_SW );

	ci->sw[0] = apdu.rsp[apdu.rsplen-2];
	ci->sw[1] = apdu.rsp[apdu.rsplen-1];

	if( (apdu.rsplen==5) && !memcmp( crycks, rsp, 3 ) ) return( SC_EXIT_OK );

	return( SC_EXIT_BAD_CHECKSUM );
#else
	return( SC_EXIT_NOT_SUPPORTED );
#endif /* WITH_DES */
}

/* Uses sfid if it is in the correct range. */
int scGpk4000CmdUpdRec( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	BYTE recnum, BYTE sfid, const BYTE *data, BYTE datalen )
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
	cmd[3]=0x04;
	if( (sfid>0) && (sfid<32) ) cmd[3]|=(sfid<<3)&0xFF;
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

/* Uses sfid if it is in the correct range. */
int scGpk4000CmdUpdRecCrycks( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	BYTE recnum, BYTE sfid, const BYTE *data, BYTE datalen )
{
#ifdef WITH_DES
	BYTE cmd[ 64+5+1 ];
	BYTE rsp[ SC_GENERAL_SHORT_DATA_SIZE+2 ];
	BYTE crycks[ 3 ];
	SC_APDU apdu;
	int ret;

	ci->sw[0]=0x00; ci->sw[1]=0x00;

	if( !ci->crypt.encrypt ) return( SC_EXIT_BAD_PARAM );

	apdu.cse=SC_APDU_CASE_4_SHORT;
	apdu.cmd=cmd;
	apdu.cmdlen=5+datalen+3;
	apdu.rsp=rsp;
	apdu.rsplen=0;

	cmd[0]=0x04;
	cmd[1]=0xDC;
	cmd[2]=recnum;
	cmd[3]=0x04;
	if( (sfid>0) && (sfid<32) ) cmd[3]|=(sfid<<3)&0xFF;
	cmd[4]=datalen+3;

	memcpy( cmd+5, data, datalen );

	scGpk4000GenCrycks( ci->crypt.key, cmd, 5+datalen, cmd+5+datalen,
		crycks );

	apdu.cmd[apdu.cmdlen]=0x03;
	apdu.cmdlen++;

	ret=scReaderSendAPDU( ri, ci, &apdu );
	memset( cmd, 0, sizeof(cmd) );
	if( ret!=SC_EXIT_OK ) return( ret );

	if( apdu.rsplen<2 ) return( SC_EXIT_BAD_SW );

	ci->sw[0] = apdu.rsp[apdu.rsplen-2];
	ci->sw[1] = apdu.rsp[apdu.rsplen-1];

	if( (apdu.rsplen==5) && !memcmp( crycks, rsp, 3 ) ) return( SC_EXIT_OK );

	return( SC_EXIT_BAD_CHECKSUM );
#else
	return( SC_EXIT_NOT_SUPPORTED );
#endif /* WITH_DES */
}

int scGpk4000CmdVerify( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	BYTE scn, const BYTE *sc )
{
	BYTE cmd[]={ 0x00, 0x20, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00 };
	BYTE rsp[ SC_GENERAL_SHORT_DATA_SIZE+2 ];
	SC_APDU apdu;
	int ret;

	ci->sw[0]=0x00; ci->sw[1]=0x00;

	apdu.cse=SC_APDU_CASE_3_SHORT;
	apdu.cmd=cmd;
	apdu.cmdlen=5+8;
	apdu.rsp=rsp;
	apdu.rsplen=0;

	cmd[3]=scn;

	memcpy( cmd+5, sc, 8 );

	ret=scReaderSendAPDU( ri, ci, &apdu );
	memset( cmd, 0, sizeof(cmd) );
	if( ret!=SC_EXIT_OK ) return( ret );

	if( apdu.rsplen!=2 ) return( SC_EXIT_BAD_SW );

	ci->sw[0] = apdu.rsp[0];
	ci->sw[1] = apdu.rsp[1];

	return( SC_EXIT_OK );
}

int scGpk4000CmdVerifyEncr( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	BYTE scn, const BYTE *sc )
{
#ifdef WITH_DES
	BYTE cmd[]={ 0x04, 0x20, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00 };
	BYTE rsp[ SC_GENERAL_SHORT_DATA_SIZE+2 ];
	SC_APDU apdu;
	int ret;
	des_key_schedule ks1;
	des_key_schedule ks2;

	ci->sw[0]=0x00; ci->sw[1]=0x00;

	if( !ci->crypt.encrypt ) return( SC_EXIT_BAD_PARAM );

	apdu.cse=SC_APDU_CASE_3_SHORT;
	apdu.cmd=cmd;
	apdu.cmdlen=5+8;
	apdu.rsp=rsp;
	apdu.rsplen=0;

	cmd[3]=scn;

	des_check_key=0;

	des_set_key( (des_cblock *) ci->crypt.key, ks1 );
	des_set_key( (des_cblock *) (ci->crypt.key+8), ks2 );

	des_ecb2_encrypt( (des_cblock *)sc, (des_cblock *)(cmd+5), ks1, ks2,
		DES_DECRYPT );

	memset( &ks1, 0, sizeof(ks1) );
	memset( &ks2, 0, sizeof(ks2) );

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

/* offset can be sfid/soffset */
int scGpk4000CmdWrBin( SC_READER_INFO *ri, SC_CARD_INFO *ci,
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

	cmd[0]=0x00;
	cmd[1]=0xD0;
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

/* offset can be sfid/soffset */
int scGpk4000CmdWrBinCrycks( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	int offset, const BYTE *data, BYTE datalen )
{
#ifdef WITH_DES
	BYTE cmd[ 64+5+1 ];
	BYTE rsp[ SC_GENERAL_SHORT_DATA_SIZE+2 ];
	BYTE crycks[ 3 ];
	SC_APDU apdu;
	int ret;

	ci->sw[0]=0x00; ci->sw[1]=0x00;

	if( datalen>61 ) return( SC_EXIT_BAD_PARAM );

	if( !ci->crypt.encrypt ) return( SC_EXIT_BAD_PARAM );

	apdu.cse=SC_APDU_CASE_4_SHORT;
	apdu.cmd=cmd;
	apdu.cmdlen=5+datalen+3;
	apdu.rsp=rsp;
	apdu.rsplen=0;

	cmd[0]=0x04;
	cmd[1]=0xD0;
	cmd[2]=(offset >>8) & 0xFF;
	cmd[3]=offset & 0xFF;
	cmd[4]=datalen+3;

	memcpy( cmd+5, data, datalen );

	scGpk4000GenCrycks( ci->crypt.key, cmd, 5+datalen, cmd+5+datalen,
		crycks );

	apdu.cmd[apdu.cmdlen]=0x03;
	apdu.cmdlen++;

	ret=scReaderSendAPDU( ri, ci, &apdu );
	memset( cmd, 0, sizeof(cmd) );
	if( ret!=SC_EXIT_OK ) return( ret );

	if( apdu.rsplen<2 ) return( SC_EXIT_BAD_SW );

	ci->sw[0] = apdu.rsp[apdu.rsplen-2];
	ci->sw[1] = apdu.rsp[apdu.rsplen-1];

	if( (apdu.rsplen==5) && !memcmp( crycks, rsp, 3 ) ) return( SC_EXIT_OK );

	return( SC_EXIT_BAD_CHECKSUM );
#else
	return( SC_EXIT_NOT_SUPPORTED );
#endif /* WITH_DES */
}

/* offset can be sfid/soffset */
int scGpk4000CmdWrBinEncr( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	int offset, const BYTE *data )
{
#ifdef WITH_DES
	BYTE cmd[ 5+16+1 ];
	BYTE rsp[ SC_GENERAL_SHORT_DATA_SIZE+2 ];
	BYTE crycks[ 3 ];
	BYTE tmp[ 16 ];
	SC_APDU apdu;
	WORD sum=0;
	int ret, i;
	des_key_schedule ks1;
	des_key_schedule ks2;

	ci->sw[0]=0x00; ci->sw[1]=0x00;

	if( !ci->crypt.encrypt ) return( SC_EXIT_BAD_PARAM );

	apdu.cse=SC_APDU_CASE_4_SHORT;
	apdu.cmd=cmd;
	apdu.cmdlen=5+16+1;
	apdu.rsp=rsp;
	apdu.rsplen=0;

	memset( cmd, 0, sizeof(cmd) );

	cmd[0]=0x04;
	cmd[1]=0xD0;
	cmd[2]=(offset >>8) & 0xFF;
	cmd[3]=offset & 0xFF;
	cmd[4]=16;

	/* Encrypt Data */
	memcpy( tmp, data, 16 );

	tmp[12]=(offset >>8) & 0xFF;
	tmp[13]=offset & 0xFF;

	for( i=0; i<14; i++ ) sum+=tmp[i];
	tmp[14]=(sum >>8) & 0xFF;
	tmp[15]=sum & 0xFF;

	des_check_key=0;

	des_set_key( (des_cblock *) ci->crypt.key, ks1 );
	des_set_key( (des_cblock *) (ci->crypt.key+8), ks2 );

	des_ecb2_encrypt( (des_cblock *)(tmp), (des_cblock *)(cmd+5), ks1, ks2,
		DES_DECRYPT );
	des_ecb2_encrypt( (des_cblock *)(tmp+8), (des_cblock *)(cmd+13), ks1, ks2,
		DES_DECRYPT );

	/* Generate Crycks */
	memcpy( tmp+4, tmp+12, 4 );

	des_ecb2_encrypt( (des_cblock *)(tmp), (des_cblock *)(tmp+8), ks1, ks2,
		DES_ENCRYPT );

	memcpy( crycks, tmp+8, 3 );

	memset( &ks1, 0, sizeof(ks1) );
	memset( &ks2, 0, sizeof(ks2) );
	memset( tmp, 0, sizeof(tmp) );

	ret=scReaderSendAPDU( ri, ci, &apdu );
	memset( cmd, 0, sizeof(cmd) );
	if( ret!=SC_EXIT_OK ) return( ret );

	if( apdu.rsplen<2 ) return( SC_EXIT_BAD_SW );

	ci->sw[0] = apdu.rsp[apdu.rsplen-2];
	ci->sw[1] = apdu.rsp[apdu.rsplen-1];

	if( (apdu.rsplen==5) && !memcmp( crycks, rsp, 3 ) ) return( SC_EXIT_OK );

	return( SC_EXIT_BAD_CHECKSUM );
#else
	return( SC_EXIT_NOT_SUPPORTED );
#endif /* WITH_DES */
}

/****************************************************************************
*																			*
*							Payment Commands								*
*																			*
****************************************************************************/

int scGpk4000CmdCanDeb( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	const BYTE *last, const BYTE *newdeb, const BYTE *ttc, BYTE *resp,
	int *resplen )
{
#ifdef WITH_DES
	BYTE cmd[18];
	BYTE rsp[ SC_GENERAL_SHORT_DATA_SIZE+2 ];
	SC_APDU apdu;
	int ret;

	des_key_schedule k1;
	des_key_schedule k2;
	des_cblock in;
	des_cblock out;

	memset( cmd, 0, sizeof(cmd) );
	cmd[0]=0x80;
	cmd[1]=0x46;
	cmd[4]=0x0C;

	ci->sw[0]=0x00; ci->sw[1]=0x00;

	if( !ci->crypt.encrypt ) return( SC_EXIT_BAD_PARAM );

	apdu.cse=SC_APDU_CASE_4_SHORT;
	apdu.cmd=cmd;
	apdu.cmdlen=18;
	apdu.rsp=rsp;
	apdu.rsplen=0;

	des_check_key=0;

	/* Generate session key */
	des_set_key( (des_cblock *) ci->crypt.key, k1 );
	des_set_key( (des_cblock *) (ci->crypt.key+8), k2 );

	memset( in, 0x00, sizeof(des_cblock) );
	if( newdeb!=NULL ) memcpy( in+1, newdeb, 3 );
	memcpy( in+4, ttc, 4 );

	des_ecb3_encrypt( &in, &out, k1, k2, k1, DES_DECRYPT );

	memcpy( cmd+6, last, 3 );
	memcpy( cmd+9, out, 8 );

	memset( &k1, 0, sizeof(k1) );
	memset( &k2, 0, sizeof(k2) );
	memset( in, 0, sizeof(in) );
	memset( out, 0, sizeof(out) );

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
#else
	return( SC_EXIT_NOT_SUPPORTED );
#endif /* WITH_DES */
}

int scGpk4000CmdCredit( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	BYTE keynum, const BYTE *kc, const BYTE *ctc, const BYTE *credit,
	BYTE pfile )
{
#ifdef WITH_DES
	BYTE cmd[17];
	BYTE rsp[ SC_GENERAL_SHORT_DATA_SIZE+2 ];
	SC_APDU apdu;
	int ret;

	des_key_schedule k1;
	des_key_schedule k2;
	des_cblock kt1;
	des_cblock kt2;
	des_cblock in;
	des_cblock out;

	memset( cmd, 0, sizeof(cmd) );
	cmd[0]=0x80;
	cmd[1]=0x36;
	cmd[4]=0x0C;

	ci->sw[0]=0x00; ci->sw[1]=0x00;

	if( !ci->crypt.encrypt ) return( SC_EXIT_BAD_PARAM );

	apdu.cse=SC_APDU_CASE_3_SHORT;
	apdu.cmd=cmd;
	apdu.cmdlen=17;
	apdu.rsp=rsp;
	apdu.rsplen=0;

	cmd[2]=(keynum<<1)&0xFE;

	des_check_key=0;

	/* Generate session key */
	des_set_key( (des_cblock *) kc, k1 );
	des_set_key( (des_cblock *) (kc+8), k2 );

	memset( in, 0, sizeof( in ) );
	memcpy( in+5, ctc, 3 );

	des_ecb3_encrypt( &in, &kt1, k1, k2, k1, DES_ENCRYPT );
	des_ecb3_encrypt( &in, &kt2, k2, k1, k2, DES_ENCRYPT );

	des_set_key( &kt1, k1 );
	des_set_key( &kt2, k2 );

	memset( in, 0, 8 );
	in[1]=pfile&0x1F;
	memcpy( in+2, credit, 3 );

	des_ecb3_encrypt( &in, &out, k1, k2, k1, DES_DECRYPT );

	des_set_key( (des_cblock *)ci->crypt.key, k1 );
	des_set_key( (des_cblock *)(ci->crypt.key+8), k2 );

	des_ecb3_encrypt( &out, &in, k1, k2, k1, DES_DECRYPT );

	memcpy( cmd+6, credit, 3 );
	memcpy( cmd+9, in, 8 );

	memset( &k1, 0, sizeof(k1) );
	memset( &k2, 0, sizeof(k2) );
	memset( kt1, 0, sizeof(kt1) );
	memset( kt2, 0, sizeof(kt2) );
	memset( in, 0, sizeof(in) );
	memset( out, 0, sizeof(out) );

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

int scGpk4000CmdDebit( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	BYTE pfile, const BYTE *debit, const BYTE *ttc )
{
#ifdef WITH_DES
	BYTE cmd[]={ 0x80, 0x34, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
	BYTE rsp[ SC_GENERAL_SHORT_DATA_SIZE+2 ];
	SC_APDU apdu;
	int ret;
	des_key_schedule k1;
	des_key_schedule k2;
	BYTE in[8];
	des_cblock out;

	ci->sw[0]=0x00; ci->sw[1]=0x00;

	if( !ci->crypt.encrypt ) return( SC_EXIT_BAD_PARAM );

	apdu.cse=SC_APDU_CASE_4_SHORT;
	apdu.cmd=cmd;
	apdu.cmdlen=14;
	apdu.rsp=rsp;
	apdu.rsplen=0;

	memcpy( cmd+5+1, debit, 3 );
	memcpy( cmd+5+4, ttc, 4 );

	ret=scReaderSendAPDU( ri, ci, &apdu );
	memset( cmd, 0, sizeof(cmd) );
	if( ret!=SC_EXIT_OK ) return( ret );

	if( apdu.rsplen<2 ) return( SC_EXIT_BAD_SW );

	ci->sw[0] = apdu.rsp[apdu.rsplen-2];
	ci->sw[1] = apdu.rsp[apdu.rsplen-1];

	apdu.rsplen-=2;

	if( apdu.rsplen==2 ) {
		memset( in, 0, sizeof( in ) );
		in[1]=pfile;
		memcpy( in+2, debit, 3 );
		memcpy( in+5, ttc+1, 3 );

		des_check_key=0;

		des_set_key( (des_cblock *) ci->crypt.key, k1 );
		des_set_key( (des_cblock *) (ci->crypt.key+8), k2 );

		des_ecb3_encrypt( (des_cblock *)in, &out, k1, k2, k1, DES_ENCRYPT );

		memset( &k1, 0, sizeof(k1) );
		memset( &k2, 0, sizeof(k2) );
		memset( in, 0, sizeof(in) );

		if( !memcmp( out, rsp, 2 ) ) {
			memset( out, 0, sizeof(out) );
			memset( rsp, 0, sizeof(rsp) );
			return( SC_EXIT_OK );
		}
	}

	memset( out, 0, sizeof(out) );
	memset( rsp, 0, sizeof(rsp) );

	return( SC_EXIT_BAD_CHECKSUM );
#else
	return( SC_EXIT_NOT_SUPPORTED );
#endif /* WITH_DES */
}

int scGpk4000CmdRdBal( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	BYTE pfile, const BYTE *ttc, BYTE *balance )
{
#ifdef WITH_DES
	BYTE cmd[]={ 0x80, 0x32, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00 };
	BYTE rsp[ SC_GENERAL_SHORT_DATA_SIZE+2 ];
	SC_APDU apdu;
	int ret;
	des_key_schedule k1;
	des_key_schedule k2;
	BYTE in[8];
	des_cblock out;

	ci->sw[0]=0x00; ci->sw[1]=0x00;

	if( !ci->crypt.encrypt ) return( SC_EXIT_BAD_PARAM );

	apdu.cse=SC_APDU_CASE_4_SHORT;
	apdu.cmd=cmd;
	apdu.cmdlen=10;
	apdu.rsp=rsp;
	apdu.rsplen=0;

	cmd[3]=pfile&0x1F;
	memcpy( cmd+5, ttc, 4 );

	ret=scReaderSendAPDU( ri, ci, &apdu );
	memset( cmd, 0, sizeof(cmd) );
	if( ret!=SC_EXIT_OK ) return( ret );

	if( apdu.rsplen<2 ) return( SC_EXIT_BAD_SW );

	ci->sw[0] = apdu.rsp[apdu.rsplen-2];
	ci->sw[1] = apdu.rsp[apdu.rsplen-1];

	apdu.rsplen-=2;

	if( apdu.rsplen==6 ) {
		memset( in, 0, sizeof( in ) );
		in[1]=pfile;
		memcpy( in+2, rsp+1, 3 );
		memcpy( in+5, ttc+1, 3 );

		des_check_key=0;

		des_set_key( (des_cblock *) ci->crypt.key, k1 );
		des_set_key( (des_cblock *) (ci->crypt.key+8), k2 );

		des_ecb3_encrypt( (des_cblock *)in, &out, k1, k2, k1, DES_ENCRYPT );

		memcpy( balance, rsp+1, 3 );

		memset( &k1, 0, sizeof(k1) );
		memset( &k2, 0, sizeof(k2) );
		memset( in, 0, sizeof(in) );

		if( !memcmp( out+2, rsp+4, 2 ) ) {
			memset( rsp, 0, sizeof(rsp) );
			memset( out, 0, sizeof(out) );
			return( SC_EXIT_OK );
		}
	}

	memset( out, 0, sizeof(out) );
	memset( rsp, 0, sizeof(rsp) );

	return( SC_EXIT_BAD_CHECKSUM );
#else
	return( SC_EXIT_NOT_SUPPORTED );
#endif /* WITH_DES */
}

int scGpk4000CmdSelPk( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	BYTE keynum, BYTE keyfile, BYTE pfile, const BYTE *key, const BYTE *rand,
	BYTE *ctc )
{
#ifdef WITH_DES
	BYTE cmd[16];
	BYTE rsp[ SC_GENERAL_SHORT_DATA_SIZE+2 ];
	SC_APDU apdu;
	int ret;

	memset( cmd, 0, sizeof(cmd) );
	cmd[0]=0x80;
	cmd[1]=0x30;
	cmd[4]=0x0A;

	ci->sw[0]=0x00; ci->sw[1]=0x00;

	apdu.cse=SC_APDU_CASE_4_SHORT;
	apdu.cmd=cmd;
	apdu.cmdlen=16;
	apdu.rsp=rsp;
	apdu.rsplen=0;

	cmd[2]=(keynum<<1)&0xFE;
	cmd[3]=keyfile&0x1F;
	cmd[6]=pfile&0x1F;
	memcpy( cmd+5+2, rand, 8 );

	ret=scReaderSendAPDU( ri, ci, &apdu );
	memset( cmd, 0, sizeof(cmd) );
	if( ret!=SC_EXIT_OK ) return( ret );

	if( apdu.rsplen<2 ) return( SC_EXIT_BAD_SW );

	ci->sw[0] = apdu.rsp[apdu.rsplen-2];
	ci->sw[1] = apdu.rsp[apdu.rsplen-1];

	apdu.rsplen-=2;

	ci->crypt.encrypt=FALSE;

	if( apdu.rsplen==8 ) {
		memcpy( ctc, rsp+5, 3 );
		if( scGpk4000GenDivKeyPK( key, rand, rsp, ci->crypt.key ) ) {
			ci->crypt.encrypt=TRUE;
			memset( rsp, 0, sizeof(rsp) );
			return( SC_EXIT_OK );
		}
	}

	memset( rsp, 0, sizeof(rsp) );

	return( SC_EXIT_BAD_CHECKSUM );
#else
	return( SC_EXIT_NOT_SUPPORTED );
#endif /* WITH_DES */
}

int scGpk4000CmdSetOpts( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	BYTE option )
{
	BYTE cmd[]={ 0x80, 0x3A, 0x00, 0x00 };
	BYTE rsp[ SC_GENERAL_SHORT_DATA_SIZE+2 ];
	SC_APDU apdu;
	int ret;

	ci->sw[0]=0x00; ci->sw[1]=0x00;

	apdu.cse=SC_APDU_CASE_1;
	apdu.cmd=cmd;
	apdu.cmdlen=4;
	apdu.rsp=rsp;
	apdu.rsplen=0;

	cmd[3]=option&0x03;

	ret=scReaderSendAPDU( ri, ci, &apdu );
	memset( cmd, 0, sizeof(cmd) );
	if( ret!=SC_EXIT_OK ) return( ret );

	if( apdu.rsplen!=2 ) return( SC_EXIT_BAD_SW );

	ci->sw[0] = apdu.rsp[apdu.rsplen-2];
	ci->sw[1] = apdu.rsp[apdu.rsplen-1];

	return( SC_EXIT_OK );
}

int scGpk4000CmdSign( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	BYTE keynum, BYTE sfid, BYTE *resp, int *resplen )
{
#ifdef WITH_DES
	BYTE cmd[]={ 0x80, 0x38, 0x00, 0x00, 0x04 };
	BYTE rsp[ SC_GENERAL_SHORT_DATA_SIZE+2 ];
	SC_APDU apdu;
	int ret;

	ci->sw[0]=0x00; ci->sw[1]=0x00;

	apdu.cse=SC_APDU_CASE_2_SHORT;
	apdu.cmd=cmd;
	apdu.cmdlen=5;
	apdu.rsp=rsp;
	apdu.rsplen=0;

	cmd[2]=(keynum<<1)&0xFE;
	cmd[3]=sfid&0x1F;

	ret=scReaderSendAPDU( ri, ci, &apdu );
	memset( cmd, 0, sizeof(cmd) );
	if( ret!=SC_EXIT_OK ) return( ret );

	if( apdu.rsplen<2 ) return( SC_EXIT_BAD_SW );

	ci->sw[0] = apdu.rsp[apdu.rsplen-2];
	ci->sw[1] = apdu.rsp[apdu.rsplen-1];

	apdu.rsplen-=2;

	*resplen=0;

	if( apdu.rsplen==4 ) {
		memcpy( resp, apdu.rsp, apdu.rsplen-2 );
		*resplen=apdu.rsplen;
	}

	memset( rsp, 0, sizeof(rsp) );

	return( SC_EXIT_OK );
#else
	return( SC_EXIT_NOT_SUPPORTED );
#endif /* WITH_DES */
}

/****************************************************************************
*																			*
*							Public Key Commands								*
*																			*
****************************************************************************/

int scGpk4000CmdCompDesKey( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	BYTE modulus, BYTE *resp, int *resplen )
{
	BYTE cmd[]={ 0x80, 0x1E, 0x00, 0x00, 0x40 };
	BYTE rsp[ SC_GENERAL_SHORT_DATA_SIZE+2 ];
	SC_APDU apdu;
	int ret;

	ci->sw[0]=0x00; ci->sw[1]=0x00;

	if( modulus>0x40 ) return( SC_EXIT_BAD_PARAM );

	apdu.cse=SC_APDU_CASE_2_SHORT;
	apdu.cmd=cmd;
	apdu.cmdlen=5;
	apdu.rsp=rsp;
	apdu.rsplen=0;

	cmd[4]=modulus;

	ret=scReaderSendAPDU( ri, ci, &apdu );
	memset( cmd, 0, sizeof(cmd) );
	if( ret!=SC_EXIT_OK ) return( ret );

	if( apdu.rsplen<2 ) return( SC_EXIT_BAD_SW );

	ci->sw[0] = apdu.rsp[apdu.rsplen-2];
	ci->sw[1] = apdu.rsp[apdu.rsplen-1];

	apdu.rsplen-=2;

	memcpy( resp, apdu.rsp, apdu.rsplen-2 );
	*resplen=apdu.rsplen;

	memset( rsp, 0, sizeof(rsp) );

	return( SC_EXIT_OK );
}

int scGpk4000CmdCrtPrivKeyFile( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	BYTE sfid, BYTE words )
{
	BYTE cmd[]={ 0x80, 0x12, 0x00, 0x00 };
	BYTE rsp[ SC_GENERAL_SHORT_DATA_SIZE+2 ];
	SC_APDU apdu;
	int ret;

	ci->sw[0]=0x00; ci->sw[1]=0x00;

	apdu.cse=SC_APDU_CASE_1;
	apdu.cmd=cmd;
	apdu.cmdlen=4;
	apdu.rsp=rsp;
	apdu.rsplen=0;

	cmd[2]=sfid&0x1F;
	cmd[3]=words;

	ret=scReaderSendAPDU( ri, ci, &apdu );
	memset( cmd, 0, sizeof(cmd) );
	if( ret!=SC_EXIT_OK ) return( ret );

	if( apdu.rsplen!=2 ) return( SC_EXIT_BAD_SW );

	ci->sw[0] = apdu.rsp[0];
	ci->sw[1] = apdu.rsp[1];

	return( SC_EXIT_OK );
}

int scGpk4000CmdDesEnc( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	const BYTE *enc, BYTE *resp, int *resplen )
{
	BYTE cmd[ 5+0x40+1 ];
	BYTE rsp[ SC_GENERAL_SHORT_DATA_SIZE+2 ];
	SC_APDU apdu;
	int ret;

	ci->sw[0]=0x00; ci->sw[1]=0x00;

	apdu.cse=SC_APDU_CASE_4_SHORT;
	apdu.cmd=cmd;
	apdu.cmdlen=5+0x40+1;
	apdu.rsp=rsp;
	apdu.rsplen=0;

	memset( cmd, 0, sizeof(cmd) );
	cmd[0]=0x80;
	cmd[1]=0x1A;
	cmd[4]=0x40;
	memcpy( cmd+5, enc, 0x40 );
	cmd[0x45]=0x40;

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

/* Offset is normally 0x07 with GPK4000-s. */
int scGpk4000CmdEraseCard( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	BYTE offset )
{
	BYTE cmd[]={ 0xDB, 0xDE, 0x00, 0x00 };
	BYTE rsp[ SC_GENERAL_SHORT_DATA_SIZE+2 ];
	SC_APDU apdu;
	int ret;

	ci->sw[0]=0x00; ci->sw[1]=0x00;

	apdu.cse=SC_APDU_CASE_1;
	apdu.cmd=cmd;
	apdu.cmdlen=4;
	apdu.rsp=rsp;
	apdu.rsplen=0;

	cmd[3]=offset;

	ret=scReaderSendAPDU( ri, ci, &apdu );
	memset( cmd, 0, sizeof(cmd) );
	if( ret!=SC_EXIT_OK ) return( ret );

	if( apdu.rsplen!=2 ) return( SC_EXIT_BAD_SW );

	ci->sw[0] = apdu.rsp[0];
	ci->sw[1] = apdu.rsp[1];

	return( SC_EXIT_OK );
}

/* Is the CLA correct? */
int scGpk4000CmdInitHashed( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	const BYTE *hash, BYTE len )
{
	BYTE cmd[ 5+0x24 ];
	BYTE rsp[ SC_GENERAL_SHORT_DATA_SIZE+2 ];
	SC_APDU apdu;
	int ret;

	ci->sw[0]=0x00; ci->sw[1]=0x00;

	if( len>0x24 ) return( SC_EXIT_BAD_PARAM );

	apdu.cse=SC_APDU_CASE_3_SHORT;
	apdu.cmd=cmd;
	apdu.cmdlen=5+len;
	apdu.rsp=rsp;
	apdu.rsplen=0;

	memset( cmd, 0, sizeof(cmd) );
	cmd[0]=0x80;
	cmd[1]=0xEA;
	cmd[2]=0x00;
	cmd[3]=0x00;
	cmd[4]=len;
	memcpy( cmd+5, hash, len );

	ret=scReaderSendAPDU( ri, ci, &apdu );
	memset( cmd, 0, sizeof(cmd) );
	if( ret!=SC_EXIT_OK ) return( ret );

	if( apdu.rsplen!=2 ) return( SC_EXIT_BAD_SW );

	ci->sw[0] = apdu.rsp[0];
	ci->sw[1] = apdu.rsp[1];

	return( SC_EXIT_OK );
}

int scGpk4000CmdLoadPrivKey( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	BYTE sfid, BYTE prklen, const BYTE *key, BYTE len )
{
	BYTE cmd[ 5+SC_GENERAL_SHORT_DATA_SIZE ];
	BYTE rsp[ SC_GENERAL_SHORT_DATA_SIZE+2 ];
	SC_APDU apdu;
	int ret;

	ci->sw[0]=0x00; ci->sw[1]=0x00;

	apdu.cse=SC_APDU_CASE_3_SHORT;
	apdu.cmd=cmd;
	apdu.cmdlen=5+len;
	apdu.rsp=rsp;
	apdu.rsplen=0;

	memset( cmd, 0, sizeof(cmd) );
	cmd[0]=0x80;
	cmd[1]=0x18;
	cmd[2]=sfid&0x1F;
	cmd[3]=prklen;
	cmd[4]=len;
	memcpy( cmd+5, key, len );

	ret=scReaderSendAPDU( ri, ci, &apdu );
	memset( cmd, 0, sizeof(cmd) );
	if( ret!=SC_EXIT_OK ) return( ret );

	if( apdu.rsplen!=2 ) return( SC_EXIT_BAD_SW );

	ci->sw[0] = apdu.rsp[0];
	ci->sw[1] = apdu.rsp[1];

	return( SC_EXIT_OK );
}

/* Le not known in advance. Which is correct?
 * - Case 2 with Le=0x00
 * - Case 4 with Lc=0x00 and Le=0x00
 */
int scGpk4000CmdPkDir( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	BYTE *resp, int *resplen )
{
	BYTE cmd[]={ 0x80, 0xA0, 0x00, 0x00, 0x00 };
	BYTE rsp[ SC_GENERAL_SHORT_DATA_SIZE+2 ];
	SC_APDU apdu;
	int ret;

	ci->sw[0]=0x00; ci->sw[1]=0x00;

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

	apdu.rsplen-=2;

	memcpy( resp, apdu.rsp, apdu.rsplen );
	*resplen=apdu.rsplen;

	memset( rsp, 0, sizeof(rsp) );

	return( SC_EXIT_OK );
}

int scGpk4000CmdPkIntAuth( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	BYTE modulus, BYTE *resp, int *resplen )
{
	BYTE cmd[]={ 0x80, 0x88, 0x00, 0x00, 0x00 };
	BYTE rsp[ SC_GENERAL_SHORT_DATA_SIZE+2 ];
	SC_APDU apdu;
	int ret;

	ci->sw[0]=0x00; ci->sw[1]=0x00;

	apdu.cse=SC_APDU_CASE_2_SHORT;
	apdu.cmd=cmd;
	apdu.cmdlen=5;
	apdu.rsp=rsp;
	apdu.rsplen=0;

	cmd[4]=modulus;

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

int scGpk4000CmdPkSend( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	BYTE sfid, BYTE pklen, const BYTE *key, BYTE len )
{
	BYTE cmd[ 5+SC_GENERAL_SHORT_DATA_SIZE ];
	BYTE rsp[ SC_GENERAL_SHORT_DATA_SIZE+2 ];
	SC_APDU apdu;
	int ret;

	ci->sw[0]=0x00; ci->sw[1]=0x00;

	apdu.cse=SC_APDU_CASE_3_SHORT;
	apdu.cmd=cmd;
	apdu.cmdlen=5+len;
	apdu.rsp=rsp;
	apdu.rsplen=0;

	memset( cmd, 0, sizeof(cmd) );
	cmd[0]=0x80;
	cmd[1]=0x8C;
	cmd[2]=sfid&0x1F;
	cmd[3]=pklen;
	cmd[4]=len;
	memcpy( cmd+5, key, len );

	ret=scReaderSendAPDU( ri, ci, &apdu );
	memset( cmd, 0, sizeof(cmd) );
	if( ret!=SC_EXIT_OK ) return( ret );

	if( apdu.rsplen!=2 ) return( SC_EXIT_BAD_SW );

	ci->sw[0] = apdu.rsp[0];
	ci->sw[1] = apdu.rsp[1];

	return( SC_EXIT_OK );
}

int scGpk4000CmdPkSign( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	BYTE modulus, BYTE *resp, int *resplen )
{
	BYTE cmd[]={ 0x80, 0x86, 0x00, 0x00, 0x00 };
	BYTE rsp[ SC_GENERAL_SHORT_DATA_SIZE+2 ];
	SC_APDU apdu;
	int ret;

	ci->sw[0]=0x00; ci->sw[1]=0x00;

	apdu.cse=SC_APDU_CASE_2_SHORT;
	apdu.cmd=cmd;
	apdu.cmdlen=5;
	apdu.rsp=rsp;
	apdu.rsplen=0;

	cmd[4]=modulus;

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

int scGpk4000CmdPkVerify( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	BYTE verify, const BYTE *sig, BYTE siglen, BYTE *resp, int *resplen )
{
	BYTE cmd[ 5+SC_GENERAL_SHORT_DATA_SIZE ];
	BYTE rsp[ SC_GENERAL_SHORT_DATA_SIZE+2 ];
	SC_APDU apdu;
	int ret;

	ci->sw[0]=0x00; ci->sw[1]=0x00;

	apdu.cse=SC_APDU_CASE_4_SHORT;
	apdu.cmd=cmd;
	apdu.cmdlen=5+siglen+1;
	apdu.rsp=rsp;
	apdu.rsplen=0;

	memset( cmd, 0, sizeof(cmd) );
	cmd[0]=0x80;
	cmd[1]=0x8A;
	cmd[2]=verify;
	cmd[3]=0x00;
	cmd[4]=siglen;
	memcpy( cmd+5, sig, siglen );

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

int scGpk4000CmdPutCryptoData( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	BYTE block, BYTE blklen, const BYTE *data, BYTE datalen )
{
	BYTE cmd[ 5+SC_GENERAL_SHORT_DATA_SIZE ];
	BYTE rsp[ SC_GENERAL_SHORT_DATA_SIZE+2 ];
	SC_APDU apdu;
	int ret;

	ci->sw[0]=0x00; ci->sw[1]=0x00;

	if( datalen>64 ) return( SC_EXIT_BAD_PARAM );

	apdu.cse=SC_APDU_CASE_3_SHORT;
	apdu.cmd=cmd;
	apdu.cmdlen=5+datalen;
	apdu.rsp=rsp;
	apdu.rsplen=0;

	cmd[0]=0x80;
	cmd[1]=0xDA;
	cmd[2]=block&0x11;
	cmd[3]=blklen;
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

int scGpk4000CmdSelCryptoContext( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	BYTE sfid, BYTE mode )
{
	BYTE cmd[]={ 0x80, 0xA6, 0x00, 0x00 };
	BYTE rsp[ SC_GENERAL_SHORT_DATA_SIZE+2 ];
	SC_APDU apdu;
	int ret;

	ci->sw[0]=0x00; ci->sw[1]=0x00;

	apdu.cse=SC_APDU_CASE_1;
	apdu.cmd=cmd;
	apdu.cmdlen=4;
	apdu.rsp=rsp;
	apdu.rsplen=0;

	cmd[2]=sfid&0x1F;
	cmd[3]=mode;

	ret=scReaderSendAPDU( ri, ci, &apdu );
	memset( cmd, 0, sizeof(cmd) );
	if( ret!=SC_EXIT_OK ) return( ret );

	if( apdu.rsplen!=2 ) return( SC_EXIT_BAD_SW );

	ci->sw[0] = apdu.rsp[0];
	ci->sw[1] = apdu.rsp[1];

	return( SC_EXIT_OK );
}

