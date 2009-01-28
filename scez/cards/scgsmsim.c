/****************************************************************************
*																			*
*					SCEZ chipcard library - GSM SIM routines				*
*					Copyright Matthias Bruestle 1999,2000					*
*					For licensing, see the file COPYRIGHT					*
*																			*
****************************************************************************/

/* $Id: scgsmsim.c 1617 2005-11-03 17:41:39Z laforge $ */

#include <scez/scinternal.h>
#include <scez/cards/scgsmsim.h>

#include <stdio.h>
#include <string.h>

static char gsm2iso[] = {
	'@', '£', '$', '¥', 'è', 'é', 'ù', 'ì',
	'ò', 'Ç', '\n', 'Ø', 'ø', '\r', 'Å', 'å',
	' ', '_', ' ', ' ', ' ', ' ', ' ', ' ',
	' ', ' ', ' ', ' ', 'Æ', 'æ', 'ß', 'É',
	' ', '!', '"', '#', ' ', '%', '&', '\'',
	'(', ')', '*', '+', ',', '-', '.', '/',
	'0', '1', '2', '3', '4', '5', '6', '7',
	'8', '9', ':', ';', '<', '=', '>', '?',
	'!', 'A', 'B', 'C', 'D', 'E', 'F', 'G',
	'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O',
	'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W',
	'X', 'Y', 'Z', 'Ä', 'Ö', 'Ñ', 'Ü', '§',
	'¿', 'a', 'b', 'c', 'd', 'e', 'f', 'g',
	'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o',
	'p', 'q', 'r', 's', 't', 'u', 'v', 'w',
	'x', 'y', 'z', 'ä', 'ö', 'ñ', 'ü', 'à'
};

static char iso2gsm[] = {
	0x00, ' ', ' ', ' ', ' ', ' ', ' ', ' ', /* 0 */
	' ', ' ', '\n', ' ', ' ', '\r', ' ', ' ',
	' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', /* 1 */
	' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ',
	' ', '!', '"', '#', 0x02, '%', '&', '\'', /* 2 */
	'(', ')', '*', '+', ',', '-', '.', '/',
	'0', '1', '2', '3', '4', '5', '6', '7', /* 3 */
	'8', '9', ':', ';', '<', '=', '>', '?',
	0x00, 'A', 'B', 'C', 'D', 'E', 'F', 'G', /* 4 */
	'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O',
	'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', /* 5 */
	'X', 'Y', 'Z', ' ', ' ', ' ', ' ', 0x11,
	' ', 'a', 'b', 'c', 'd', 'e', 'f', 'g', /* 6 */
	'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o',
	'p', 'q', 'r', 's', 't', 'u', 'v', 'w', /* 7 */
	'x', 'y', 'z', ' ', ' ', ' ', ' ', ' ',
	' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', /* 8 */
	' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ',
	' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', /* 9 */
	' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ',
	' ', ' ', ' ', 0x01, ' ', 0x03, ' ', 0x5F, /* A */
	' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ',
	' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', /* B */
	' ', ' ', ' ', ' ', ' ', ' ', ' ', 0x60,
	' ', ' ', ' ', ' ', 0x5B, 0x0E, 0x1C, 0x09, /* C */
	' ', 0x1F, ' ', ' ', ' ', ' ', ' ', ' ',
	0x5D, ' ', ' ', ' ', ' ', ' ', 0x5C, ' ', /* D */
	0x0B, ' ', ' ', ' ', 0x5E, ' ', ' ', 0x1E,
	0x7F, ' ', ' ', ' ', 0x7B, 0x0F, 0x1D, ' ', /* E */
	0x04, 0x05, ' ', ' ', 0x07, ' ', ' ', ' ',
	0x7D, ' ', 0x08, ' ', ' ', ' ', 0x7C, ' ', /* F */
	0x0C, 0x06, ' ', ' ', 0x7E, ' ', ' ', 0xFF
};

/* Initialize card function pointer */

int scGsmsimInit( SC_CARD_INFO *ci )
{
	ci->scGetCap=scGsmsimGetCap;
	ci->scGetCardData=scGsmsimGetCardData;
	ci->scSetFD=scGsmsimSetFD;

	return( SC_EXIT_OK );
}

/* Capabilities */

int scGsmsimGetCap( SC_CARD_INFO *ci, SC_CARD_CAP *cp )
{
	cp->n_fd=2;

	/* 9600 at 3.579MHz */
	cp->fd[0]=(((10L<<16)+372L)<<8)+1;

	/* TODO: Get value from TA1. */
	/* 56000 at 3.579MHz */
	cp->fd[1]=(((56L<<16)+512L)<<8)+8;

	return( SC_EXIT_OK );
}

/* Fill card data in ci */

int scGsmsimGetCardData( SC_CARD_INFO *ci )
{
	BYTE header[] = { 0xA0, 0xC0, 0x00, 0x00, 0x00 };
	BYTE swok[] = { 0x02, 0x90, 0x91 };
	BYTE swav[] = { 0x01, 0x9F };
	int ret;

	if( (ret=scSmartcardProcessATR( ci ))!=SC_EXIT_OK ) return( ret );

	memcpy( ci->t0.getrsp, header, 5 );
	memcpy( ci->swok, swok, sizeof(swok) );
	memcpy( ci->swav, swav, sizeof(swav) );
	ci->memsize=0;

	return( SC_EXIT_OK );
}

/* Set F and D. */

int scGsmsimSetFD( SC_READER_INFO *ri, SC_CARD_INFO *ci, LONG fd )
{
	BYTE pts1[] = {0xFF, 0x10, 0x11, 0xFE};
	BYTE pts2[] = {0xFF, 0x10, 0x94, 0x7B};

	switch( fd&0xFFFFFF ) {
		case (372L<<8)+1:
			if( (ci->atr[1]&0x10) && (ci->atr[2]!=0x11) )
				return( scReaderPTS( ri, ci, pts1, 4 ) );
			else
				return( SC_EXIT_OK );
		case (512L<<8)+8:
			if( (!(ci->atr[1]&0x10)) || ((ci->atr[1]&0x10) &&
				(ci->atr[2]!=0x94)) )
				return( scReaderPTS( ri, ci, pts2, 4 ) );
			else
				return( SC_EXIT_OK );
		default:
			return( SC_EXIT_BAD_PARAM );
	}
}

/* Translate character set. */

char scGsmsimGsmToIso( char c )
{
	return( gsm2iso[c&0x7F] );
}

char scGsmsimIsoToGsm( char c )
{
	return( iso2gsm[c&0xFF] );
}

/* Packs string as 7bit characters into octets. */

int scGsmsimPackStr( const char *c, int clen, BYTE *b, int *blen )
{
	LONG l1, l2;
	int i;
	BYTE btmp[8];
	char ctmp[8];

	*blen=0;

	while( clen>0 ) {
		/* Copy 8 characters == 7 bytes to ctmp. */
		if( clen>=8 ) {
			memcpy( ctmp, c, 8 );
			c+=8;
		} else {
			memset( ctmp, 0, sizeof(ctmp) );
			memcpy( ctmp, c, clen );
		}

		/* Translate characters. */
		for( i=0; i<8; i++ ) ctmp[i]=iso2gsm[((int)ctmp[i])&0xFF];


		/* Pack. */
		l1=0;
		l2=0;
		for( i=4; i>=0; i-- ) {
			l1<<=7;
			l1|=ctmp[i];
		}
		for( i=3; i>=0; i-- ) {
			l2<<=7;
			l2|=ctmp[i+4];
		}
		l2>>=4;

		/* Move from registers. */
		for( i=0; i<4; i++ ) {
			btmp[i]=l1&0xFF;
			l1>>=8;
			btmp[i+4]=l2&0xFF;
			l2>>=8;
		}

		/* Copy characters to destination. */
		if( clen>=8 ) {
			memcpy( b+*blen, btmp, 7 );
			*blen+=7;
			clen-=8;
		} else {
			memcpy( b+*blen, btmp, clen );
			*blen+=clen;
			clen=0;
		}
	}

	return( SC_EXIT_OK );
}

/* Unpacks 7bit characters. */

int scGsmsimUnpackStr( const BYTE *b, char *c, int clen )
{
	LONG l1, l2;
	int i, clentmp;
	BYTE btmp[8];
	char ctmp[8];

	clentmp=clen;
	clen=0;

	while( (clentmp-clen)>0 ) {
		/* Copy 7 bytes == 8 characters to btmp. */
		if( (clentmp-clen)>=7 ) {
			memcpy( btmp, b, 7 );
			b+=7;
		} else {
			memset( btmp, 0, sizeof(btmp) );
			memcpy( btmp, b, clentmp-clen );
			b+=clentmp-clen;
		}

		/* Move into registers. */
		l1=0;
		l2=0;
		for( i=3; i>=0; i-- ) {
			l1<<=8;
			l1|=btmp[i];
		}
		for( i=2; i>=0; i-- ) {
			l2<<=8;
			l2|=btmp[i+4];
		}

		/* Unpack. */
		for( i=0; i<8; i++ ) {
			ctmp[i]=l1&0x7F;
			l1>>=7;
			/* Move l2 to l1, when it is empty. */
			if( i==3 ) {
				l2<<=4;
				l1|=l2;
			}
		}

		/* Translate characters. */
		for( i=0; i<8; i++ ) ctmp[i]=gsm2iso[((int)ctmp[i])&0xFF];

		/* Copy characters to destination. */
		if( clentmp-clen>=8 ) {
			memcpy( c+clen, ctmp, 8 );
			clen+=8;
		} else {
			memcpy( c+clen, ctmp, clentmp-clen );
			clen=clentmp;
		}
	}

	/* Set end of string. */
	c[clentmp]=0;

	return( SC_EXIT_OK );
}

int scGsmsimCmdSelect( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	WORD fid, BYTE *resp, int *resplen )
{
	BYTE cmd[]={ 0xA0, 0xA4, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00 };
	BYTE rsp[ SC_GENERAL_SHORT_DATA_SIZE+2 ];
	SC_APDU apdu;
	int ret;

	ci->sw[0]=0x00; ci->sw[1]=0x00;

	apdu.cse=SC_APDU_CASE_4_SHORT;
	apdu.cmd=cmd;
	apdu.cmdlen=8;
	apdu.rsp=rsp;
	apdu.rsplen=0;

	cmd[5]=(fid>>8) &0xFF;
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

int scGsmsimCmdStatus( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	BYTE *resp, int *resplen )
{
	BYTE cmd[]={ 0xA0, 0xF2, 0x00, 0x00, 0x00 };
	BYTE rsp[ SC_GENERAL_SHORT_DATA_SIZE+2 ];
	SC_APDU apdu;
	int ret;

	ci->sw[0]=0x00; ci->sw[1]=0x00;

	/* Try Le=0. */
	/* if( *resplen>256 ) return( SC_EXIT_BAD_PARAM ); */

	apdu.cse=SC_APDU_CASE_2_SHORT;
	apdu.cmd=cmd;
	apdu.cmdlen=5;
	apdu.rsp=rsp;
	apdu.rsplen=0;

	/* cmd[4] = (*resplen) & 0xFF; */

	*resplen = 0;

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

int scGsmsimCmdReadBin( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	WORD offset, BYTE *data, int *datalen )
{
	BYTE cmd[]={ 0xA0, 0xB0, 0x00, 0x00, 0x00 };
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

	cmd[2]=(offset>>8) &0xFF;
	cmd[3]=offset & 0xFF;
	cmd[4]=(*datalen) & 0xFF;

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

int scGsmsimCmdUpdateBin( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	WORD offset, const BYTE *data, BYTE datalen )
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

	memset( cmd, 0, sizeof(cmd) );

	cmd[0]=0xA0;
	cmd[1]=0xD6;
	cmd[2]=(offset>>8) &0xFF;
	cmd[3]=offset & 0xFF;
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

int scGsmsimCmdReadRec( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	BYTE recnum, BYTE mode, BYTE *data, int *datalen )
{
	BYTE cmd[]={ 0xA0, 0xB2, 0x00, 0x00, 0x00 };
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

int scGsmsimCmdUpdateRec( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	BYTE recnum, BYTE mode, const BYTE *data, BYTE datalen )
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

	memset( cmd, 0, sizeof(cmd) );

	cmd[0]=0xA0;
	cmd[1]=0xDC;
	cmd[2]=recnum;
	cmd[3]=mode;
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

int scGsmsimCmdSeek( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	BOOLEAN resp, BYTE mode, const BYTE *pattern, BYTE patlen, BYTE *num )
{
	BYTE cmd[ 5+SC_GENERAL_SHORT_DATA_SIZE ];
	BYTE rsp[ SC_GENERAL_SHORT_DATA_SIZE+2 ];
	SC_APDU apdu;
	int ret;

	ci->sw[0]=0x00; ci->sw[1]=0x00;

	apdu.cse=SC_APDU_CASE_4_SHORT;
	apdu.cmd=cmd;
	apdu.cmdlen=5+patlen;
	apdu.rsp=rsp;
	apdu.rsplen=0;

	memset( cmd, 0, sizeof(cmd) );

	cmd[0]=0xA0;
	cmd[1]=0xA2;
	cmd[3]=mode&0x0F;
	if( resp ) cmd[3]|=0x10;

	memcpy( cmd+5, pattern, patlen );

	ret=scReaderSendAPDU( ri, ci, &apdu );
	memset( cmd, 0, sizeof(cmd) );
	if( ret!=SC_EXIT_OK ) return( ret );

	if( apdu.rsplen<2 ) return( SC_EXIT_BAD_SW );

	ci->sw[0] = apdu.rsp[apdu.rsplen-2];
	ci->sw[1] = apdu.rsp[apdu.rsplen-1];

	if( apdu.rsplen==2 ) return( SC_EXIT_OK );

	if( apdu.rsplen>3 ) {
		memset( rsp, 0, sizeof(rsp) );
		return( SC_EXIT_OK );
	}

	*num = rsp[0];

	memset( rsp, 0, sizeof(rsp) );

	return( SC_EXIT_OK );
}

int scGsmsimCmdIncrease( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	const BYTE *amount, BYTE *resp, int *resplen )
{
	BYTE cmd[]={ 0xA0, 0x32, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00 };
	BYTE rsp[ SC_GENERAL_SHORT_DATA_SIZE+2 ];
	SC_APDU apdu;
	int ret;

	ci->sw[0]=0x00; ci->sw[1]=0x00;

	apdu.cse=SC_APDU_CASE_4_SHORT;
	apdu.cmd=cmd;
	apdu.cmdlen=5+3;
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

int scGsmsimCmdVerifyCHV( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	BYTE num, const BYTE *pin )
{
	BYTE cmd[]={ 0xA0, 0x20, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00, 0x00,
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

	cmd[3]=num;

	memcpy( cmd+5, pin, 8 );

	ret=scReaderSendAPDU( ri, ci, &apdu );
	memset( cmd, 0, sizeof(cmd) );
	if( ret!=SC_EXIT_OK ) return( ret );

	if( apdu.rsplen<2 ) return( SC_EXIT_BAD_SW );

	ci->sw[0] = apdu.rsp[apdu.rsplen-2];
	ci->sw[1] = apdu.rsp[apdu.rsplen-1];

	return( SC_EXIT_OK );
}

int scGsmsimCmdChangeCHV( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	BYTE num, const BYTE *oldpin, const BYTE *newpin )
{
	BYTE cmd[ 5+16 ];
	BYTE rsp[ SC_GENERAL_SHORT_DATA_SIZE+2 ];
	SC_APDU apdu;
	int ret;

	ci->sw[0]=0x00; ci->sw[1]=0x00;

	apdu.cse=SC_APDU_CASE_3_SHORT;
	apdu.cmd=cmd;
	apdu.cmdlen=5+16;
	apdu.rsp=rsp;
	apdu.rsplen=0;

	memset( cmd, 0, sizeof(cmd) );

	cmd[0]=0xA0;
	cmd[1]=0x24;
	cmd[3]=num;
	cmd[4]=0x10;

	memcpy( cmd+5, oldpin, 8 );
	memcpy( cmd+5+8, newpin, 8 );

	ret=scReaderSendAPDU( ri, ci, &apdu );
	memset( cmd, 0, sizeof(cmd) );
	if( ret!=SC_EXIT_OK ) return( ret );

	if( apdu.rsplen<2 ) return( SC_EXIT_BAD_SW );

	ci->sw[0] = apdu.rsp[apdu.rsplen-2];
	ci->sw[1] = apdu.rsp[apdu.rsplen-1];

	return( SC_EXIT_OK );
}

int scGsmsimCmdDisableCHV( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	const BYTE *pin )
{
	BYTE cmd[]={ 0xA0, 0x26, 0x00, 0x01, 0x08, 0x00, 0x00, 0x00, 0x00,
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

	memcpy( cmd+5, pin, 8 );

	ret=scReaderSendAPDU( ri, ci, &apdu );
	memset( cmd, 0, sizeof(cmd) );
	if( ret!=SC_EXIT_OK ) return( ret );

	if( apdu.rsplen<2 ) return( SC_EXIT_BAD_SW );

	ci->sw[0] = apdu.rsp[apdu.rsplen-2];
	ci->sw[1] = apdu.rsp[apdu.rsplen-1];

	return( SC_EXIT_OK );
}

int scGsmsimCmdEnableCHV( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	const BYTE *pin )
{
	BYTE cmd[]={ 0xA0, 0x28, 0x00, 0x01, 0x08, 0x00, 0x00, 0x00, 0x00,
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

	memcpy( cmd+5, pin, 8 );

	ret=scReaderSendAPDU( ri, ci, &apdu );
	memset( cmd, 0, sizeof(cmd) );
	if( ret!=SC_EXIT_OK ) return( ret );

	if( apdu.rsplen<2 ) return( SC_EXIT_BAD_SW );

	ci->sw[0] = apdu.rsp[apdu.rsplen-2];
	ci->sw[1] = apdu.rsp[apdu.rsplen-1];

	return( SC_EXIT_OK );
}

int scGsmsimCmdUnblockCHV( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	BYTE num, const BYTE *unblockpin, const BYTE *newpin )
{
	BYTE cmd[ 5+16 ];
	BYTE rsp[ SC_GENERAL_SHORT_DATA_SIZE+2 ];
	SC_APDU apdu;
	int ret;

	ci->sw[0]=0x00; ci->sw[1]=0x00;

	apdu.cse=SC_APDU_CASE_3_SHORT;
	apdu.cmd=cmd;
	apdu.cmdlen=5+16;
	apdu.rsp=rsp;
	apdu.rsplen=0;

	memset( cmd, 0, sizeof(cmd) );

	if(num==1) num=0;

	cmd[0]=0xA0;
	cmd[1]=0x2C;
	cmd[3]=num;
	cmd[4]=0x10;

	memcpy( cmd+5, unblockpin, 8 );
	memcpy( cmd+5+8, newpin, 8 );

	ret=scReaderSendAPDU( ri, ci, &apdu );
	memset( cmd, 0, sizeof(cmd) );
	if( ret!=SC_EXIT_OK ) return( ret );

	if( apdu.rsplen<2 ) return( SC_EXIT_BAD_SW );

	ci->sw[0] = apdu.rsp[apdu.rsplen-2];
	ci->sw[1] = apdu.rsp[apdu.rsplen-1];

	return( SC_EXIT_OK );
}

int scGsmsimCmdInvalidate( SC_READER_INFO *ri, SC_CARD_INFO *ci )
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

	return( SC_EXIT_OK );
}

int scGsmsimCmdRehabilitate( SC_READER_INFO *ri, SC_CARD_INFO *ci )
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

	return( SC_EXIT_OK );
}

int scGsmsimCmdRunGsmAlgo( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	const BYTE *rand, BYTE *sres, BYTE *kc )
{
	BYTE cmd[ 5+16 ];
	BYTE rsp[ SC_GENERAL_SHORT_DATA_SIZE+2 ];
	SC_APDU apdu;
	int ret;

	ci->sw[0]=0x00; ci->sw[1]=0x00;

	apdu.cse=SC_APDU_CASE_4_SHORT;
	apdu.cmd=cmd;
	apdu.cmdlen=5+16;
	apdu.rsp=rsp;
	apdu.rsplen=0;

	memset( cmd, 0, sizeof(cmd) );

	cmd[0]=0xA0;
	cmd[1]=0x88;
	cmd[4]=0x10;

	memcpy( cmd+5, rand, 16 );

	ret=scReaderSendAPDU( ri, ci, &apdu );
	memset( cmd, 0, sizeof(cmd) );
	if( ret!=SC_EXIT_OK ) return( ret );

	if( apdu.rsplen<2 ) return( SC_EXIT_BAD_SW );

	ci->sw[0] = apdu.rsp[apdu.rsplen-2];
	ci->sw[1] = apdu.rsp[apdu.rsplen-1];

	if( apdu.rsplen==2 ) return( SC_EXIT_OK );
	if( apdu.rsplen!=14 ) {
		memset( rsp, 0, sizeof(rsp) );
		return( SC_EXIT_UNKNOWN_ERROR );
	}

	memcpy( sres, apdu.rsp, 4 );
	memcpy( kc, apdu.rsp+4, 8 );

	memset( rsp, 0, sizeof(rsp) );

	return( SC_EXIT_OK );
}

int scGsmsimCmdSleep( SC_READER_INFO *ri, SC_CARD_INFO *ci )
{
	BYTE cmd[]={ 0xA0, 0xFA, 0x00, 0x00 };
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

	return( SC_EXIT_OK );
}

/* sw is used to get the length of the available data. */
int scGsmsimCmdGetResp( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	BYTE *resp, int *resplen )
{
	BYTE cmd[]={ 0xA0, 0xC0, 0x00, 0x00, 0x00 };
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

int scGsmsimCmdTermProf( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	const BYTE *prof, BYTE proflen )
{
	BYTE cmd[ 5+SC_GENERAL_SHORT_DATA_SIZE ];
	BYTE rsp[ SC_GENERAL_SHORT_DATA_SIZE+2 ];
	SC_APDU apdu;
	int ret;

	ci->sw[0]=0x00; ci->sw[1]=0x00;

	apdu.cse=SC_APDU_CASE_3_SHORT;
	apdu.cmd=cmd;
	apdu.cmdlen=5+proflen;
	apdu.rsp=rsp;
	apdu.rsplen=0;

	memset( cmd, 0, sizeof(cmd) );

	cmd[0]=0xA0;
	cmd[1]=0x10;
	cmd[4]=proflen;

	memcpy( cmd+5, prof, proflen );

	ret=scReaderSendAPDU( ri, ci, &apdu );
	memset( cmd, 0, sizeof(cmd) );
	if( ret!=SC_EXIT_OK ) return( ret );

	if( apdu.rsplen<2 ) return( SC_EXIT_BAD_SW );

	ci->sw[0] = apdu.rsp[apdu.rsplen-2];
	ci->sw[1] = apdu.rsp[apdu.rsplen-1];

	memset( rsp, 0, sizeof(rsp) );

	return( SC_EXIT_OK );
}

int scGsmsimCmdEnvelope( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	const BYTE *comd, BYTE comdlen, BYTE *resp, int *resplen )
{
	BYTE cmd[ 5+SC_GENERAL_SHORT_DATA_SIZE ];
	BYTE rsp[ SC_GENERAL_SHORT_DATA_SIZE+2 ];
	SC_APDU apdu;
	int ret;

	ci->sw[0]=0x00; ci->sw[1]=0x00;

	apdu.cse=SC_APDU_CASE_4_SHORT;
	apdu.cmd=cmd;
	apdu.cmdlen=5+comdlen;
	apdu.rsp=rsp;
	apdu.rsplen=0;

	memset( cmd, 0, sizeof(cmd) );

	cmd[0]=0xA0;
	cmd[1]=0xC4;

	memcpy( cmd+5, comd, comdlen );

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

/* sw is used to get the length of the available data. */
int scGsmsimCmdFetch( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	BYTE *resp, int *resplen )
{
	BYTE cmd[]={ 0xA0, 0x12, 0x00, 0x00, 0x00 };
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

int scGsmsimCmdTermResp( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	const BYTE *comd, BYTE comdlen )
{
	BYTE cmd[ 5+SC_GENERAL_SHORT_DATA_SIZE ];
	BYTE rsp[ SC_GENERAL_SHORT_DATA_SIZE+2 ];
	SC_APDU apdu;
	int ret;

	ci->sw[0]=0x00; ci->sw[1]=0x00;

	apdu.cse=SC_APDU_CASE_3_SHORT;
	apdu.cmd=cmd;
	apdu.cmdlen=5+comdlen;
	apdu.rsp=rsp;
	apdu.rsplen=0;

	memset( cmd, 0, sizeof(cmd) );

	cmd[0]=0xA0;
	cmd[1]=0x14;
	cmd[4]=comdlen;

	memcpy( cmd+5, comd, comdlen );

	ret=scReaderSendAPDU( ri, ci, &apdu );
	memset( cmd, 0, sizeof(cmd) );
	if( ret!=SC_EXIT_OK ) return( ret );

	if( apdu.rsplen<2 ) return( SC_EXIT_BAD_SW );

	ci->sw[0] = apdu.rsp[apdu.rsplen-2];
	ci->sw[1] = apdu.rsp[apdu.rsplen-1];

	return( SC_EXIT_OK );
}

