/****************************************************************************
*																			*
*					SCEZ chipcard library - TCOS 2.0 routines				*
*						Copyright Matthias Bruestle 1999					*
*					For licensing, see the file COPYRIGHT					*
*																			*
****************************************************************************/

/* $Id: sctcos.c 1617 2005-11-03 17:41:39Z laforge $ */

#include <scez/scinternal.h>
#include <scez/cards/sctcos.h>

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

int scTcosInit( SC_CARD_INFO *ci )
{
	ci->scGetCap=scTcosGetCap;
	ci->scGetCardData=scTcosGetCardData;
	ci->scSetFD=scTcosSetFD;

	return( SC_EXIT_OK );
}

/* Capabilities */

int scTcosGetCap( SC_CARD_INFO *ci, SC_CARD_CAP *cp )
{
	cp->n_fd=7;

	/* 9600 at 3.579MHz */
	cp->fd[0]=(((10L<<16)+372L)<<8)+1;

#if 0 /* PTS does not work. Dunno why. */
	/* 19200 at 3.579MHz */
	cp->fd[1]=(((19L<<16)+372L)<<8)+2;

	/* 38400 at 3.579MHz */
	cp->fd[2]=(((38L<<16)+372L)<<8)+4;

	/* 76800 at 3.579MHz */
	cp->fd[3]=(((77L<<16)+372L)<<8)+8;

	/* 153600 at 3.579MHz */
	cp->fd[4]=(((154L<<16)+372L)<<8)+16;

	/* 57600 at 3.579MHz */
	cp->fd[5]=(((58L<<16)+744L)<<8)+12;

	/* 56000 at 3.579MHz */
	cp->fd[6]=(((56L<<16)+512L)<<8)+8;

	/* Add more usefull combinations? */
#endif

	return( SC_EXIT_OK );
}

/* Fill card data in ci */

int scTcosGetCardData( SC_CARD_INFO *ci )
{
	BYTE swok[] = { 0x01, 0x90 };
	BYTE swav[] = { 0x00 };

	/* 3B BA 13 00 81 31 86 5D | 00 64 05 0A 02 01 31 80 90 00 | 8B
	 * F=372
	 * D=4 => 38400bps
	 */

	ci->protocol=SC_PROTOCOL_T1;
	ci->direct=TRUE;
	ci->t1.ifsc=134;
	ci->t1.cwt=8192;
	ci->t1.bwt=3200000;
	ci->t1.rc=SC_T1_CHECKSUM_LRC;
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

int scTcosSetFD( SC_READER_INFO *ri, SC_CARD_INFO *ci, LONG fd )
{
	switch( fd&0xFFFFFF ) {
		case (372L<<8)+1:
			/* return( scReaderPTS( ri, ci, (BYTE *)"\xFF\x10\x11\xFE", 4 ) ); */
			return( SC_EXIT_OK );
#if 0 /* PTS does not work. Dunno why. */
		case (372L<<8)+2:
			return( scReaderPTS( ri, ci, (BYTE *)"\xFF\x10\x12\xFD", 4 ) );
		case (372L<<8)+4:
			return( scReaderPTS( ri, ci, (BYTE *)"\xFF\x10\x13\xFC", 4 ) );
		case (372L<<8)+8:
			return( scReaderPTS( ri, ci, (BYTE *)"\xFF\x10\x14\xFB", 4 ) );
		case (372L<<8)+16:
			return( scReaderPTS( ri, ci, (BYTE *)"\xFF\x10\x15\xFA", 4 ) );
		case (744L<<8)+12:
			return( scReaderPTS( ri, ci, (BYTE *)"\xFF\x10\x38\xD7", 4 ) );
		case (512L<<8)+8:
			return( scReaderPTS( ri, ci, (BYTE *)"\xFF\x10\x94\x7B", 4 ) );
#endif
		default:
			return( SC_EXIT_BAD_PARAM );
	}
}

void scTcosEncCAPDU( SC_APDU *apdu, BYTE encalgo, const BYTE *enckey,
	const BYTE *enciv, BYTE macalgo, const BYTE *mackey, const BYTE *maciv,
	int opts )
{
#ifdef WITH_DES

	return;
#endif /* WITH_DES */
}

void scTcosDecRAPDU( SC_APDU *apdu, BYTE encalgo, const BYTE *enckey,
	const BYTE *enciv, BYTE macalgo, const BYTE *mackey, const BYTE *maciv,
	int flags )
{
#ifdef WITH_DES

	return;
#endif /* WITH_DES */
}

/* TCOS 2.0 R2 p.78 */
int scTcosCmdAppendRecord( SC_READER_INFO *ri, SC_CARD_INFO *ci,
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

/* TCOS 2.0 R2 p.66 */
int scTcosCmdAskRandom( SC_READER_INFO *ri, SC_CARD_INFO *ci,
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

/* TCOS 2.0 R2 p.62 */
int scTcosCmdChangePassword( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	BOOLEAN local, BYTE num, const BYTE *oldpass, BYTE oldlen,
	const BYTE *newpass, BYTE newlen )
{
	BYTE cmd[ 37 ];
	BYTE rsp[ SC_GENERAL_SHORT_DATA_SIZE+2 ];
	SC_APDU apdu;
	int ret;

	ci->sw[0]=0x00; ci->sw[1]=0x00;

	if( num>31 ) return( SC_EXIT_BAD_PARAM );
	if( oldpass==NULL ) oldlen=0;

	apdu.cse=SC_APDU_CASE_3_SHORT;
	apdu.cmd=cmd;
	apdu.cmdlen=5+newlen+oldlen;
	apdu.rsp=rsp;
	apdu.rsplen=0;

	memset( cmd, 0, sizeof( cmd ) );

	cmd[1] = 0x24;
	if( oldlen==0 ) cmd[2]=0x01;
	cmd[3]=num & 0x1F;
	if( local ) cmd[3]|=0x80;
	cmd[4]=oldlen+newlen;

	if( oldlen!=0 ) memcpy( cmd+5, oldpass, oldlen );
	memcpy( cmd+5+oldlen, newpass, newlen );

	ret=scReaderSendAPDU( ri, ci, &apdu );
	memset( cmd, 0, sizeof(cmd) );
	if( ret!=SC_EXIT_OK ) return( ret );

	if( apdu.rsplen!=2 ) return( SC_EXIT_BAD_SW );

	ci->sw[0] = apdu.rsp[0];
	ci->sw[1] = apdu.rsp[1];

	return( SC_EXIT_OK );
}

/* TODO: Build TLV objects in this function */
/* TCOS 2.0 R2 p.80 */
int scTcosCmdCreate( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	const BYTE *data, BYTE datalen )
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

	cmd[0] = 0x80;
	cmd[1] = 0xE0;
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

/* TCOS 2.0 R2 p.83 */
int scTcosCmdDelete( SC_READER_INFO *ri, SC_CARD_INFO *ci, WORD fid )
{
	BYTE cmd[ 2+5 ];
	BYTE rsp[ SC_GENERAL_SHORT_DATA_SIZE+2 ];
	SC_APDU apdu;
	int ret;

	ci->sw[0]=0x00; ci->sw[1]=0x00;

	apdu.cse=SC_APDU_CASE_3_SHORT;
	apdu.cmd=cmd;
	apdu.cmdlen=5+2;
	apdu.rsp=rsp;
	apdu.rsplen=0;

	memset( cmd, 0, sizeof( cmd ) );

	cmd[0] = 0x80;
	cmd[1] = 0xE4;
	cmd[4] = 0x02;
	cmd[5] = (fid>>8)&0xFF;
	cmd[6] = fid&0xFF;

	ret=scReaderSendAPDU( ri, ci, &apdu );
	memset( cmd, 0, sizeof(cmd) );
	if( ret!=SC_EXIT_OK ) return( ret );

	if( apdu.rsplen<2 ) return( SC_EXIT_BAD_SW );

	ci->sw[0] = apdu.rsp[apdu.rsplen-2];
	ci->sw[1] = apdu.rsp[apdu.rsplen-1];

	return( SC_EXIT_OK );
}

/* TCOS 2.0 R2 p.90 */
int scTcosCmdExcludeSFI( SC_READER_INFO *ri, SC_CARD_INFO *ci, BYTE sfi )
{
	BYTE cmd[ 1+5 ];
	BYTE rsp[ SC_GENERAL_SHORT_DATA_SIZE+2 ];
	SC_APDU apdu;
	int ret;

	ci->sw[0]=0x00; ci->sw[1]=0x00;

	apdu.cse=SC_APDU_CASE_3_SHORT;
	apdu.cmd=cmd;
	apdu.cmdlen=5+1;
	apdu.rsp=rsp;
	apdu.rsplen=0;

	memset( cmd, 0, sizeof( cmd ) );

	cmd[0] = 0x80;
	cmd[1] = 0xE8;
	cmd[4] = 1;
	cmd[5] = sfi&0x1F;

	ret=scReaderSendAPDU( ri, ci, &apdu );
	memset( cmd, 0, sizeof(cmd) );
	if( ret!=SC_EXIT_OK ) return( ret );

	if( apdu.rsplen<2 ) return( SC_EXIT_BAD_SW );

	ci->sw[0] = apdu.rsp[apdu.rsplen-2];
	ci->sw[1] = apdu.rsp[apdu.rsplen-1];

	return( SC_EXIT_OK );
}

/* TCOS 2.0 R2 p.67 */
int scTcosCmdExtAuth( SC_READER_INFO *ri, SC_CARD_INFO *ci )
{
	return( SC_EXIT_OK );
}

/* TCOS 2.0 R2 p.77 */
int scTcosCmdGetSessionkey( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	BYTE algo, BYTE *resp, int *resplen )
{
	BYTE cmd[]={ 0x80, 0x52, 0x00, 0x00, 0x00 };
	BYTE rsp[ SC_GENERAL_SHORT_DATA_SIZE+2 ];
	SC_APDU apdu;
	int ret;

	ci->sw[0]=0x00; ci->sw[1]=0x00;

	apdu.cse=SC_APDU_CASE_2_SHORT;
	apdu.cmd=cmd;
	apdu.cmdlen=5;
	apdu.rsp=rsp;
	apdu.rsplen=0;

	cmd[2] = algo;
	/* cmd[4] = *resplen */

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

/* TCOS 2.0 R2 p.88 */
int scTcosCmdIncludeSFI( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	BYTE sfi, const BYTE *path, BYTE pathlen )
{
	BYTE cmd[ SC_GENERAL_SHORT_DATA_SIZE+5 ];
	BYTE rsp[ SC_GENERAL_SHORT_DATA_SIZE+2 ];
	SC_APDU apdu;
	int ret;

	ci->sw[0]=0x00; ci->sw[1]=0x00;

	if( pathlen>252 ) return( SC_EXIT_BAD_PARAM );

	apdu.cse=SC_APDU_CASE_3_SHORT;
	apdu.cmd=cmd;
	apdu.cmdlen=5+3+pathlen;
	apdu.rsp=rsp;
	apdu.rsplen=0;

	memset( cmd, 0, sizeof( cmd ) );

	cmd[0] = 0x80;
	cmd[1] = 0xE6;
	cmd[4] = 3+pathlen;
	cmd[5] = 0x85;
	cmd[6] = 1+pathlen;
	cmd[7] = sfi&0x1F;

	memcpy( cmd+5+3, path, pathlen );

	ret=scReaderSendAPDU( ri, ci, &apdu );
	memset( cmd, 0, sizeof(cmd) );
	if( ret!=SC_EXIT_OK ) return( ret );

	if( apdu.rsplen<2 ) return( SC_EXIT_BAD_SW );

	ci->sw[0] = apdu.rsp[apdu.rsplen-2];
	ci->sw[1] = apdu.rsp[apdu.rsplen-1];

	return( SC_EXIT_OK );
}

/* TCOS 2.0 R2 p.69 */
int scTcosCmdIntAuth( SC_READER_INFO *ri, SC_CARD_INFO *ci )
{
	return( SC_EXIT_OK );
}

/* TCOS 2.0 R2 p.86 */
int scTcosCmdInvalidate( SC_READER_INFO *ri, SC_CARD_INFO *ci )
{
	BYTE cmd[]={ 0x80, 0x04, 0x00, 0x00 };
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

/* TCOS 2.0 R2 p.52 */
int scTcosCmdListDir( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	BYTE ftype, BYTE start, BYTE *resp, int *resplen )
{
	BYTE cmd[]={ 0x80, 0xAA, 0x00, 0x00, 0x00 };
	BYTE rsp[ SC_GENERAL_SHORT_DATA_SIZE+2 ];
	SC_APDU apdu;
	int ret;

	ci->sw[0]=0x00; ci->sw[1]=0x00;

	apdu.cse=SC_APDU_CASE_2_SHORT;
	apdu.cmd=cmd;
	apdu.cmdlen=5;
	apdu.rsp=rsp;
	apdu.rsplen=0;

	cmd[2] = ftype;
	cmd[3] = start;

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

/* TCOS 2.0 R2 p.70 */
int scTcosCmdManageSecEnv( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	BYTE app, BYTE op, const BYTE *data, BYTE datalen )
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

	cmd[1] = 0x22;
	cmd[2] = app;
	cmd[3] = op;
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

/* TCOS 2.0 R2 p.73 */
int scTcosCmdPerformSecOp( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	BYTE in, BYTE out, const BYTE *data, BYTE datalen, BYTE *resp,
	int *resplen )
{
	BYTE cmd[ SC_GENERAL_SHORT_DATA_SIZE+5 ];
	BYTE rsp[ SC_GENERAL_SHORT_DATA_SIZE+2 ];
	SC_APDU apdu;
	int ret;

	ci->sw[0]=0x00; ci->sw[1]=0x00;

	apdu.cse=SC_APDU_CASE_4_SHORT;
	apdu.cmd=cmd;
	apdu.cmdlen=5+datalen+1;
	apdu.rsp=rsp;
	apdu.rsplen=0;

	memset( cmd, 0, sizeof( cmd ) );

	cmd[1]=0x2A;
	cmd[2]=out;
	if( out==SC_TCOS_PSO_OUT_NONE ) {
		apdu.cse=SC_APDU_CASE_3_SHORT;
		apdu.cmdlen--;
	}
	cmd[3]=in;
	cmd[4]=datalen;

	memcpy( cmd+5, data, datalen );

	ret=scReaderSendAPDU( ri, ci, &apdu );
	memset( cmd, 0, sizeof(cmd) );
	if( ret!=SC_EXIT_OK ) return( ret );

	if( apdu.rsplen<2 ) return( SC_EXIT_BAD_SW );

	ci->sw[0] = apdu.rsp[apdu.rsplen-2];
	ci->sw[1] = apdu.rsp[apdu.rsplen-1];

	apdu.rsplen-=2;

	if( resp!=NULL ) {
		memcpy( resp, apdu.rsp, apdu.rsplen );
		*resplen=apdu.rsplen;
	}

	memset( rsp, 0, sizeof(rsp) );

	return( SC_EXIT_OK );
}

/* offset can be sfid/soffset (highest bit set) */
/* TCOS 2.0 R2 p.53 */
int scTcosCmdReadBinary( SC_READER_INFO *ri, SC_CARD_INFO *ci,
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

/* Uses sfid if it is in the correct range. */
/* TCOS 2.0 R2 p.55 */
int scTcosCmdReadRecord( SC_READER_INFO *ri, SC_CARD_INFO *ci,
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

/* TCOS 2.0 R2 p.87 */
int scTcosCmdRehabilitate( SC_READER_INFO *ri, SC_CARD_INFO *ci )
{
	BYTE cmd[]={ 0x80, 0x44, 0x00, 0x00 };
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

/* TCOS 2.0 R2 p.50 */
int scTcosCmdSelect( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	BYTE type, WORD fid, const BYTE *aidpath, BYTE aidpathlen, BYTE rtype,
	BYTE *resp, int *resplen )
{
	BYTE cmd[22];
	BYTE rsp[ SC_GENERAL_SHORT_DATA_SIZE+2 ];
	SC_APDU apdu;
	int ret;

	memset( cmd, 0, sizeof(cmd) );
	cmd[1]=0xA4;

	ci->sw[0]=0x00; ci->sw[1]=0x00;

	if( (aidpathlen>16) && (type>=0x04) ) return( SC_EXIT_BAD_PARAM );

	apdu.cse=SC_APDU_CASE_3_SHORT;
	apdu.cmd=cmd;
	apdu.cmdlen=4;
	apdu.rsp=rsp;
	apdu.rsplen=0;

	cmd[2]=type;
	cmd[3]=rtype;

	if( rtype!=SC_TCOS_RDATA_NONE ) {
		apdu.cmdlen++;
		apdu.cse=SC_APDU_CASE_4_SHORT;
	}

	switch( type ) {
		case SC_TCOS_SELECT_MF:
		case SC_TCOS_SELECT_PARENT:
			if( rtype==SC_TCOS_RDATA_NONE ) apdu.cse=SC_APDU_CASE_1;
			else apdu.cse=SC_APDU_CASE_2_SHORT;
			break;
		case SC_TCOS_SELECT_DF:
		case SC_TCOS_SELECT_EF:
			cmd[4]=0x02;
			cmd[5]=fid>>8;
			cmd[6]=fid & 0xFF;
			apdu.cmdlen+=3;
			break;
		case SC_TCOS_SELECT_AID:
		case SC_TCOS_SELECT_ABS_PATH:
		case SC_TCOS_SELECT_REL_PATH:
			cmd[4]=aidpathlen;
			memcpy( cmd+5, aidpath, aidpathlen );
			apdu.cmdlen+=aidpathlen+1;
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

	if( (resp!=NULL) && (rtype!=0x0C) ) {
		memcpy( resp, apdu.rsp, apdu.rsplen );
		*resplen=apdu.rsplen;
	}

	memset( rsp, 0, sizeof(rsp) );

	return( SC_EXIT_OK );
}

/* TCOS 2.0 R2 p.85 */
int scTcosCmdSetPermanent( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	BOOLEAN actpin )
{
	BYTE cmd[]={ 0x80, 0xEE, 0x00, 0x00 };
	BYTE rsp[ SC_GENERAL_SHORT_DATA_SIZE+2 ];
	SC_APDU apdu;
	int ret;

	ci->sw[0]=0x00; ci->sw[1]=0x00;

	apdu.cse=SC_APDU_CASE_1;
	apdu.cmd=cmd;
	apdu.cmdlen=4;
	apdu.rsp=rsp;
	apdu.rsplen=0;

	if( actpin ) cmd[2] = 0x01;

	ret=scReaderSendAPDU( ri, ci, &apdu );
	memset( cmd, 0, sizeof(cmd) );
	if( ret!=SC_EXIT_OK ) return( ret );

	if( apdu.rsplen<2 ) return( SC_EXIT_BAD_SW );

	ci->sw[0] = apdu.rsp[apdu.rsplen-2];
	ci->sw[1] = apdu.rsp[apdu.rsplen-1];

	memset( rsp, 0, sizeof(rsp) );

	return( SC_EXIT_OK );
}

/* TCOS 2.0 R2 p.64 */
int scTcosCmdUnblockPassword( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	BOOLEAN local, BYTE num, const BYTE *puk, BYTE puklen,
	const BYTE *newpass, BYTE newlen )
{
	BYTE cmd[ SC_GENERAL_SHORT_DATA_SIZE+5 ];
	BYTE rsp[ SC_GENERAL_SHORT_DATA_SIZE+2 ];
	SC_APDU apdu;
	int ret;

	ci->sw[0]=0x00; ci->sw[1]=0x00;

	if( num>31 ) return( SC_EXIT_BAD_PARAM );
	if( puklen+newlen>255 ) return( SC_EXIT_BAD_PARAM );

	memset( cmd, 0, sizeof( cmd ) );

	apdu.cse=SC_APDU_CASE_3_SHORT;
	apdu.cmd=cmd;
	apdu.rsp=rsp;
	apdu.rsplen=0;

	cmd[1] = 0x2C;
	cmd[3]=num & 0x1F;
	if( local ) cmd[3]|=0x80;

	if( (puk!=NULL) && (newpass!=NULL) ) {
		apdu.cmdlen=5+newlen+puklen;

		cmd[2]=0x00;
		cmd[4]=puklen+newlen;

		memcpy( cmd+5, puk, puklen );
		memcpy( cmd+5+puklen, newpass, newlen );
	} else if( puk!=NULL ) {
		apdu.cmdlen=5+puklen;

		cmd[2]=0x01;
		cmd[4]=puklen;

		memcpy( cmd+5, puk, puklen );
	} else if( newpass!=NULL ) {
		apdu.cmdlen=5+newlen;

		cmd[2]=0x02;
		cmd[4]=newlen;

		memcpy( cmd+5, newpass, newlen );
	} else {
		apdu.cse=SC_APDU_CASE_1;
		apdu.cmdlen=4;

		cmd[2]=0x03;
	}

	ret=scReaderSendAPDU( ri, ci, &apdu );
	memset( cmd, 0, sizeof(cmd) );
	if( ret!=SC_EXIT_OK ) return( ret );

	if( apdu.rsplen!=2 ) return( SC_EXIT_BAD_SW );

	ci->sw[0] = apdu.rsp[0];
	ci->sw[1] = apdu.rsp[1];

	return( SC_EXIT_OK );
}

/* offset can be sfid/soffset (highest bit set) */
/* TCOS 2.0 R2 p.54 */
int scTcosCmdUpdateBinary( SC_READER_INFO *ri, SC_CARD_INFO *ci,
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
/* TCOS 2.0 R2 p.57 */
int scTcosCmdUpdateRecord( SC_READER_INFO *ri, SC_CARD_INFO *ci,
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

/* TCOS 2.0 R2 p.60 */
int scTcosCmdVerifyPassword( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	BOOLEAN local, BYTE num, const BYTE *pass, BYTE passlen )
{
	BYTE cmd[ 21 ];
	BYTE rsp[ SC_GENERAL_SHORT_DATA_SIZE+2 ];
	SC_APDU apdu;
	int ret;

	ci->sw[0]=0x00; ci->sw[1]=0x00;

	if( num>31 ) return( SC_EXIT_BAD_PARAM );
	if( pass==NULL ) passlen=0;

	apdu.cse=SC_APDU_CASE_3_SHORT;
	apdu.cmd=cmd;
	apdu.cmdlen=5+passlen;
	apdu.rsp=rsp;
	apdu.rsplen=0;

	memset( cmd, 0, sizeof( cmd ) );

	cmd[1]=0x20;
	cmd[3]=num & 0x1F;
	if( local ) cmd[3]|=0x80;
	cmd[4]=passlen;

	if( passlen!=0 ) memcpy( cmd+5, pass, passlen );

	ret=scReaderSendAPDU( ri, ci, &apdu );
	memset( cmd, 0, sizeof(cmd) );
	if( ret!=SC_EXIT_OK ) return( ret );

	if( apdu.rsplen!=2 ) return( SC_EXIT_BAD_SW );

	ci->sw[0] = apdu.rsp[0];
	ci->sw[1] = apdu.rsp[1];

	return( SC_EXIT_OK );
}

