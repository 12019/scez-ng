/****************************************************************************
*																			*
*					SCEZ chipcard library - BasicCard routines				*
*						Copyright Matthias Bruestle 1999					*
*					For licensing, see the file COPYRIGHT					*
*																			*
****************************************************************************/

/* $Id: scbasiccard.c 1617 2005-11-03 17:41:39Z laforge $ */

#include <scez/scinternal.h>
#include <scez/cards/scbasiccard.h>
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

int scBasiccardInit( SC_CARD_INFO *ci )
{
	ci->scGetCap=scBasiccardGetCap;
	ci->scGetCardData=scBasiccardGetCardData;

	return( SC_EXIT_OK );
}

/* Capabilities */

int scBasiccardGetCap( SC_CARD_INFO *ci, SC_CARD_CAP *cp )
{
	cp->n_fd=1;

	cp->fd[0]=(((10L<<16)+372L)<<8)+1;

	return( SC_EXIT_OK );
}

/* Fill card data in ci */

int scBasiccardGetCardData( SC_CARD_INFO *ci )
{
	BYTE swok[] = { 0x02, 0x90, 0x61 };
	BYTE swav[] = { 0x00 };

	scSmartcardProcessATR( ci );

#if 0
	ci->protocol=SC_PROTOCOL_T1;
	ci->direct=TRUE;
	if( ci->type==SC_CARD_BASICCARD_COMP ) {
		ci->t1.ifsc=80;
	} else {
		ci->t1.ifsc=32;
	}
	ci->t1.cwt=32;
	ci->t1.bwt=1600000;
	ci->t1.rc=SC_T1_CHECKSUM_LRC;
#endif
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

/* Calculates CRC for EEPROM Check. */
/* From the BasicCard manual. */

WORD scBasiccardCrc( const BYTE *p, int len )
{
	WORD crc=0;
	BYTE b;
	int i;

	while( len-- ) {
		b=*p++;

		for( i=0; i<8 ; i++, b>>=1 ) {
			if( (crc^b) & 1 ) {
				crc>>=1;
				crc^=0xCA00;	
			} else
				crc>>=1;
		}
	}

	return( crc );
}

/* Encrypts a byte array with a SG-LFSR. */
/* From BasicCard Development Kit. */

int scBasiccardSgLfsr( SC_CARD_INFO *ci, BYTE *data, int datalen )
{
	LONG pa, ps, la, ls;
	BYTE byte=0, bit;
	int i;

	pa=*(LONG *)(ci->crypt.key);
	ps=*(LONG *)(ci->crypt.key+4);
	if( (la=*(LONG *)(ci->crypt.iv) & 0x7FFFFFFF)==0 ) la++;
	if( (ls=*(LONG *)(ci->crypt.iv+4))==0 ) ls++;

	while(datalen--) {
		for( i=0; i<8; i++ ) {
			byte<<=1;
			while( !(ls&1) ) {
				ls>>=1;
				if( la&1 ) { la>>=1; la^=pa; }
				else la>>=1;
			}
			ls>>=1;
			ls^=ps;
			bit=la&1;
			la>>=1;
			if( bit ) la^=pa;
			byte|=bit;
		}

		*data++^=byte;
	}

	*(LONG *)(ci->crypt.iv)=la;
	*(LONG *)(ci->crypt.iv+4)=ls;

	pa=0; ps=0; la=0; ls=0; byte=0; bit=0;

	return( SC_EXIT_OK );
}

int scBasiccardEncrCAPDU( SC_CARD_INFO *ci, SC_APDU *apdu )
{
	BYTE ba[ SC_GENERAL_SHORT_DATA_SIZE+5+1 ];
	WORD crc;
	int balen, ret;
#ifdef WITH_DES
	BYTE baout[ SC_GENERAL_SHORT_DATA_SIZE+5+1 ];
	int blocks;
	int m;	/* length % 8 */
#endif /* WITH_DES */

	if( !ci->crypt.encrypt ) return( SC_EXIT_OK );

	if( (ci->crypt.algo&0xF0)==0x10 ) {
		/* LFSR algorithm */

		if( apdu->cmdlen>(254+5+1) ) return( SC_EXIT_BAD_PARAM );

		if( ci->crypt.algo==SC_BASICCARD_ALGO_LFSR ) {

			if( (apdu->cse==SC_APDU_CASE_3_SHORT) )
				scBasiccardSgLfsr( ci, apdu->cmd+5, apdu->cmdlen-5 );
			else if( (apdu->cse==SC_APDU_CASE_4_SHORT) )
				scBasiccardSgLfsr( ci, apdu->cmd+5, apdu->cmdlen-6 );

		} else if( ci->crypt.algo==SC_BASICCARD_ALGO_LFSR_CRC ) {

			memset( ba, 0, sizeof( ba ) );
			memcpy( ba, apdu->cmd, apdu->cmdlen );
			balen=apdu->cmdlen;

			switch( apdu->cse ) {
				case SC_APDU_CASE_1:
					ba[4]=2;
					if( (ret=scGeneralGetRandStr( ba+5, 2 ))!=SC_EXIT_OK )
						return( ret );
					
					balen+=3;

					crc = scBasiccardCrc( ba, balen );
					ba[7]=(crc>>8)&0xFF;
					ba[8]=crc&0xFF;
					ba[4]=4;
					balen+=2;

					scBasiccardSgLfsr( ci, ba+5, balen-5 );

					memcpy( apdu->cmd, ba, balen );
					apdu->cmdlen=balen;

					break;
				case SC_APDU_CASE_2_SHORT:
					ba[4]=2;
					ba[7]=apdu->cmd[4];
					ba[9]=apdu->cmd[4];
					if( (ret=scGeneralGetRandStr( ba+5, 2 ))!=SC_EXIT_OK )
						return( ret );
					balen+=3;

					crc = scBasiccardCrc( ba, balen );
					ba[7]=(crc>>8)&0xFF;
					ba[8]=crc&0xFF;
					ba[4]=4;
					balen+=2;

					scBasiccardSgLfsr( ci, ba+5, balen-6 );

					memcpy( apdu->cmd, ba, balen );
					apdu->cmdlen=balen;

					break;
				case SC_APDU_CASE_3_SHORT:
					ba[4]+=2;
					if( (ret=scGeneralGetRandStr( ba+balen, 2 ))!=SC_EXIT_OK )
						return( ret );
					balen+=2;

					crc = scBasiccardCrc( ba, balen );
					ba[balen++]=(crc>>8)&0xFF;
					ba[balen++]=crc&0xFF;
					ba[4]+=2;

					scBasiccardSgLfsr( ci, ba+5, balen-5 );

					memcpy( apdu->cmd, ba, balen );
					apdu->cmdlen=balen;

					break;
				case SC_APDU_CASE_4_SHORT:
					balen-=1; /* Strip Le */
					ba[4]+=2;
					ba[balen+2]=ba[balen];
					ba[balen+4]=ba[balen];
					if( (ret=scGeneralGetRandStr( ba+balen, 2 ))!=SC_EXIT_OK )
						return( ret );
					balen+=2;

					crc = scBasiccardCrc( ba, balen+1 );
					ba[balen++]=(crc>>8)&0xFF;
					ba[balen++]=crc&0xFF;
					ba[4]+=2;

					balen++; /* Add Le */

					scBasiccardSgLfsr( ci, ba+5, balen-6 );

					memcpy( apdu->cmd, ba, balen );
					apdu->cmdlen=balen;

					break;
				default:
					memset( ba, 0, sizeof(ba) );
					return( SC_EXIT_BAD_PARAM );
			}

			/* End Encr does not send a CRC. */
			if( !((apdu->cmd[0]==0xC0) && (apdu->cmd[1]==0x12)) )
				apdu->cse=SC_APDU_CASE_4_SHORT;
			else
				apdu->cse=SC_APDU_CASE_3_SHORT;

			memset( ba, 0, sizeof(ba) );

		} else {
				return( SC_EXIT_BAD_PARAM );
		}

	} else if( (ci->crypt.algo&0xF0)==0x20 ) {
#ifdef WITH_DES
		/* DES algorithm */

		des_key_schedule ks1, ks2;

		des_check_key=0;

		if( apdu->cmdlen>(254+5+1-8) ) return( SC_EXIT_BAD_PARAM );

		memset( ba, 0, sizeof( ba ) );
		memcpy( ba, apdu->cmd, apdu->cmdlen );

		switch( apdu->cse ) {
			case SC_APDU_CASE_1:
				balen=8;
				break;
			case SC_APDU_CASE_2_SHORT:
				ba[5]=ba[4];
				ba[4]=0;
				balen=8;
				break;
			case SC_APDU_CASE_3_SHORT:
				balen=apdu->cmdlen+3;
				break;
			case SC_APDU_CASE_4_SHORT:
				balen=apdu->cmdlen+2;
				break;
			default:
				memset( ba, 0, sizeof(ba) );
				return( SC_EXIT_BAD_PARAM );
		}

		m=balen%8;
		blocks=balen;
		if( m ) blocks+=8-m;
		blocks>>=3;

		des_set_key( (des_cblock *)ci->crypt.key, ks1 );
		if( ci->crypt.algo==SC_BASICCARD_ALGO_3DES )
			des_set_key( (des_cblock *)(ci->crypt.key+8), ks2 );

		if( ci->crypt.algo==SC_BASICCARD_ALGO_DES )
#if defined(HAVE_LIBCRYPT) || defined(HAVE_LIBDES)
			des_ncbc_encrypt( (des_cblock *)ba, (des_cblock *)baout,
				blocks*8, ks1, (des_cblock *)ci->crypt.iv, DES_ENCRYPT );
#else
			des_ncbc_encrypt( ba, baout, blocks*8, ks1,
				(des_cblock *)ci->crypt.iv, DES_ENCRYPT );
#endif /* HAVE_LIBCRYPTO */
		else if( ci->crypt.algo==SC_BASICCARD_ALGO_3DES )
#if defined(HAVE_LIBCRYPT) || defined(HAVE_LIBDES)
			des_ede2_cbc_encrypt( (des_cblock *)ba, (des_cblock *)baout,
				blocks*8, ks1, ks2, (des_cblock *)ci->crypt.iv, DES_ENCRYPT );
#else
			des_ede2_cbc_encrypt( ba, baout, blocks*8, ks1, ks2,
				(des_cblock *)ci->crypt.iv, DES_ENCRYPT );
#endif /* HAVE_LIBCRYPTO */
		else {
			memset( &ks1, 0, sizeof(ks1) );
			memset( &ks2, 0, sizeof(ks2) );
			memset( ba, 0, sizeof(ba) );
			memset( baout, 0, sizeof(baout) );

			return( SC_EXIT_BAD_PARAM );
		}

		memset( &ks1, 0, sizeof(ks1) );
		memset( &ks2, 0, sizeof(ks2) );

		if(m) memcpy( baout+(blocks*8)-16+m, baout+(blocks*8)-8, 8 );
		if( (apdu->cse==SC_APDU_CASE_2_SHORT ) ||
			(apdu->cse==SC_APDU_CASE_4_SHORT ) ) {
			apdu->cmd[ 5+balen ]=apdu->cmd[ apdu->cmdlen-1 ];
		}
		memcpy( apdu->cmd, ba, 4 );
		apdu->cmd[4]=ba[4]+8;
		memcpy( apdu->cmd+5, baout, balen );
		apdu->cmdlen=5+balen;
		if( (apdu->cse==SC_APDU_CASE_2_SHORT ) ||
			(apdu->cse==SC_APDU_CASE_4_SHORT ) ) {
			apdu->cmdlen++;
		}

		/* End Encr does not send ciphertext. */
		if( !((apdu->cmd[0]==0xC0) && (apdu->cmd[1]==0x12)) )
			apdu->cse=SC_APDU_CASE_4_SHORT;
		else
			apdu->cse=SC_APDU_CASE_3_SHORT;

		memset( ba, 0, sizeof(ba) );
		memset( baout, 0, sizeof(baout) );
#else
		return( SC_EXIT_NOT_SUPPORTED );
#endif /* WITH_DES */
	} else {
			return( SC_EXIT_BAD_PARAM );
	}

	return( SC_EXIT_OK );
}

int scBasiccardDecrRAPDU( SC_CARD_INFO *ci, SC_APDU *apdu )
{
	WORD crc;
	WORD cmp;
#ifdef WITH_DES
	BYTE ba[ SC_GENERAL_SHORT_DATA_SIZE+5+1 ];
	int balen;
	BYTE baout[ SC_GENERAL_SHORT_DATA_SIZE+5+1 ];
	BYTE bl[8];
	BYTE iv[8];
	int blocks;
	int m;	/* length % 8 */
	int i;
#endif /* WITH_DES */

	if( (!ci->crypt.encrypt) || (apdu->rsplen<=2) ) return( SC_EXIT_OK );

	if( apdu->rsplen>(256+2) ) return( SC_EXIT_BAD_PARAM );

	if( (ci->crypt.algo&0xF0)==0x10 ) {
		/* LFSR algorithm */

		if( ci->crypt.algo==SC_BASICCARD_ALGO_LFSR ) {

			scBasiccardSgLfsr( ci, apdu->rsp, apdu->rsplen-2 );

		} else if( ci->crypt.algo==SC_BASICCARD_ALGO_LFSR_CRC ) {

			scBasiccardSgLfsr( ci, apdu->rsp, apdu->rsplen-2 );

			cmp = (apdu->rsp[apdu->rsplen-4]<<8) |
					apdu->rsp[apdu->rsplen-3];
			memcpy( apdu->rsp+apdu->rsplen-4, apdu->rsp+apdu->rsplen-2, 2 );
			crc = scBasiccardCrc( apdu->rsp, apdu->rsplen-2 );
			apdu->rsplen-=2;
			memcpy( apdu->rsp+apdu->rsplen-4, apdu->rsp+apdu->rsplen-2, 2 );
			apdu->rsplen-=2;

			if( crc!=cmp ) return( SC_EXIT_BAD_CHECKSUM );

		} else {
				return( SC_EXIT_BAD_PARAM );
		}

	} else if( (ci->crypt.algo&0xF0)==0x20 ) {
#ifdef WITH_DES
		/* DES algorithm */

		des_key_schedule ks1, ks2;

		des_check_key=0;

		memcpy( ba, apdu->rsp, apdu->rsplen-2 );
		balen=apdu->rsplen-2;

		m=balen%8;
		blocks=balen;
		if( m ) blocks-=8+m;
		blocks>>=3;

		des_set_key( (des_cblock *)ci->crypt.key, ks1 );

		if( ci->crypt.algo==SC_BASICCARD_ALGO_3DES )
			des_set_key( (des_cblock *)(ci->crypt.key+8), ks2 );

		if( ci->crypt.algo==SC_BASICCARD_ALGO_DES )
#if defined(HAVE_LIBCRYPT) || defined(HAVE_LIBDES)
			des_ncbc_encrypt( (des_cblock *)ba, (des_cblock *)baout,
				blocks*8, ks1, (des_cblock *)ci->crypt.iv, DES_DECRYPT );
#else
			des_ncbc_encrypt( ba, baout, blocks*8, ks1,
				(des_cblock *)ci->crypt.iv, DES_DECRYPT );
#endif /* HAVE_LIBCRYPTO */
		else if( ci->crypt.algo==SC_BASICCARD_ALGO_3DES )
#if defined(HAVE_LIBCRYPT) || defined(HAVE_LIBDES)
			des_ede2_cbc_encrypt( (des_cblock *)ba, (des_cblock *)baout,
				blocks*8, ks1, ks2, (des_cblock *)ci->crypt.iv, DES_DECRYPT );
#else
			des_ede2_cbc_encrypt( ba, baout, blocks*8, ks1, ks2,
				(des_cblock *)ci->crypt.iv, DES_DECRYPT );
#endif /* HAVE_LIBCRYPTO */
		else {
			memset( &ks1, 0, sizeof(ks1) );
			memset( &ks2, 0, sizeof(ks2) );
			memset( ba, 0, sizeof(ba) );
			memset( baout, 0, sizeof(baout) );

			return( SC_EXIT_BAD_PARAM );
		}

		/* Save IV. */
		memcpy( iv, ba+balen-8, 8 );

		/* Now do last 2 blocks. */
		if(m) {
			/* Decrypt Cn */
			if( ci->crypt.algo==SC_BASICCARD_ALGO_DES )
				des_ecb_encrypt( (des_cblock *)(ba+balen-8),
					(des_cblock *)(baout+8+blocks*8), ks1, DES_DECRYPT );
			else
				des_ecb2_encrypt( (des_cblock *)(ba+balen-8),
					(des_cblock *)(baout+8+blocks*8), ks1, ks2, DES_DECRYPT );

			memset( bl, 0, 8 );
			memcpy( bl, ba+blocks*8, m );
			for( i=0; i<8; i++ ) *(baout+8+i+blocks*8)^=bl[i];
			/* Copy C' */
			memcpy( ba+balen-8, baout+8+m+blocks*8, 8-m );
			/* Decrypt Cn-1|C' */
			if( ci->crypt.algo==SC_BASICCARD_ALGO_DES )
				des_ecb_encrypt( (des_cblock *)(ba+blocks*8),
					(des_cblock *)(baout+blocks*8), ks1, DES_DECRYPT );
			else
				des_ecb2_encrypt( (des_cblock *)(ba+blocks*8),
					(des_cblock *)(baout+blocks*8), ks1, ks2, DES_DECRYPT );
			if( blocks ) {
				for( i=0; i<8; i++ )
					*(baout+i+blocks*8)^=*(ba+i-8+blocks*8);
			} else {
				for( i=0; i<8; i++ )
					baout[i]^=ci->crypt.iv[i];
			}
		}

		memset( &ks1, 0, sizeof(ks1) );
		memset( &ks2, 0, sizeof(ks2) );

		/* Copy IV. */
		memcpy( ci->crypt.iv, iv, 8 );

		/* Check SW. */
		if( memcmp( baout+balen-8, apdu->rsp+apdu->rsplen-2, 2 ) ) {
			memset( ba, 0, sizeof(ba) );
			memset( baout, 0, sizeof(baout) );

			return( SC_EXIT_BAD_CHECKSUM );
		}
		/* Check zeros. */
		memset( bl, 0, 8 );
		if( memcmp( baout+balen-6, bl, 6 ) ) {
			memset( ba, 0, sizeof(ba) );
			memset( baout, 0, sizeof(baout) );

			return( SC_EXIT_BAD_CHECKSUM );
		}

		memcpy( apdu->rsp, baout, balen-6 );
		apdu->rsplen=balen-6;

		memset( ba, 0, sizeof(ba) );
		memset( baout, 0, sizeof(baout) );
#else
		return( SC_EXIT_NOT_SUPPORTED );
#endif /* WITH_DES */
	}

	return( SC_EXIT_OK );
}

int scBasiccardCmdGetState( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	BYTE *state, BOOLEAN *enh )
{
	BYTE cmd[ 5+10 ];
	BYTE rsp[ SC_GENERAL_SHORT_DATA_SIZE+2 ];
	SC_APDU apdu;
	int ret;

	ci->sw[0]=0x00; ci->sw[1]=0x00;

	apdu.cse=SC_APDU_CASE_2_SHORT;
	apdu.cmd=cmd;
	apdu.cmdlen=5;
	apdu.rsp=rsp;
	apdu.rsplen=0;

	memset( cmd, 0, sizeof(cmd) );

	cmd[0]=0xC0;
	cmd[4]=0x03;

	*enh=FALSE;

	if( (ret=scBasiccardEncrCAPDU( ci, &apdu )) !=SC_EXIT_OK ) return( ret );
	ret=scReaderSendAPDU( ri, ci, &apdu );
	memset( cmd, 0, sizeof(cmd) );
	if( ret!=SC_EXIT_OK ) return( ret );
	if( (ret=scBasiccardDecrRAPDU( ci, &apdu )) !=SC_EXIT_OK ) return( ret );

	if( apdu.rsplen<2 ) return( SC_EXIT_BAD_SW );

	ci->sw[0] = rsp[apdu.rsplen-2];
	ci->sw[1] = rsp[apdu.rsplen-1];

	if( (apdu.rsplen!=5) && (apdu.rsplen!=3) )
		return( SC_EXIT_UNKNOWN_ERROR );

	memcpy( state, rsp, apdu.rsplen-2 );

	if( apdu.rsplen==5 ) *enh=TRUE;

	return( SC_EXIT_OK );
}

int scBasiccardCmdEepromSize( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	WORD *offset, WORD *length )
{
	BYTE cmd[ 5+10 ];
	BYTE rsp[ SC_GENERAL_SHORT_DATA_SIZE+2 ];
	SC_APDU apdu;
	int ret;

	ci->sw[0]=0x00; ci->sw[1]=0x00;

	apdu.cse=SC_APDU_CASE_2_SHORT;
	apdu.cmd=cmd;
	apdu.cmdlen=5;
	apdu.rsp=rsp;
	apdu.rsplen=0;

	memset( cmd, 0, sizeof(cmd) );

	cmd[0]=0xC0;
	cmd[1]=0x02;
	cmd[4]=0x04;

	if( (ret=scBasiccardEncrCAPDU( ci, &apdu )) !=SC_EXIT_OK ) return( ret );
	ret=scReaderSendAPDU( ri, ci, &apdu );
	memset( cmd, 0, sizeof(cmd) );
	if( ret!=SC_EXIT_OK ) return( ret );
	if( (ret=scBasiccardDecrRAPDU( ci, &apdu )) !=SC_EXIT_OK ) return( ret );

	if( apdu.rsplen<2 ) return( SC_EXIT_BAD_SW );

	ci->sw[0] = rsp[apdu.rsplen-2];
	ci->sw[1] = rsp[apdu.rsplen-1];

	if( apdu.rsplen!=6 ) {
		*offset=0;
		*length=0;
	} else {
		*offset=(rsp[0]<<8)|rsp[1];
		*length=(rsp[2]<<8)|rsp[3];
	}

	return( SC_EXIT_OK );
}

/* Do NOT execute this function on a card which had never an image uploaded. */
int scBasiccardCmdClearEeprom( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	WORD offset, WORD length )
{
	BYTE cmd[ 7+10 ];
	BYTE rsp[ SC_GENERAL_SHORT_DATA_SIZE+2 ];
	SC_APDU apdu;
	int ret;

	if( ci->type==SC_CARD_BASICCARD_ENH_3 ) return( SC_EXIT_NOT_SUPPORTED );

	ci->sw[0]=0x00; ci->sw[1]=0x00;

	apdu.cse=SC_APDU_CASE_3_SHORT;
	apdu.cmd=cmd;
	apdu.cmdlen=7;
	apdu.rsp=rsp;
	apdu.rsplen=0;

	memset( cmd, 0, sizeof(cmd) );

	cmd[0]=0xC0;
	cmd[1]=0x04;
	cmd[2]=(offset>>8)&0xFF;
	cmd[3]=offset&0xFF;
	cmd[4]=0x02;
	cmd[5]=(length>>8)&0xFF;
	cmd[6]=length&0xFF;

	if( (ret=scBasiccardEncrCAPDU( ci, &apdu )) !=SC_EXIT_OK ) return( ret );
	ret=scReaderSendAPDU( ri, ci, &apdu );
	memset( cmd, 0, sizeof(cmd) );
	if( ret!=SC_EXIT_OK ) return( ret );
	if( (ret=scBasiccardDecrRAPDU( ci, &apdu )) !=SC_EXIT_OK ) return( ret );

	if( apdu.rsplen!=2 ) return( SC_EXIT_BAD_SW );

	ci->sw[0] = rsp[apdu.rsplen-2];
	ci->sw[1] = rsp[apdu.rsplen-1];

	return( SC_EXIT_OK );
}

int scBasiccardCmdWriteEeprom( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	WORD offset, const BYTE *data, int datalen )	
{
	BYTE cmd[ SC_GENERAL_SHORT_DATA_SIZE+5+1 ];
	BYTE rsp[ SC_GENERAL_SHORT_DATA_SIZE+2 ];
	SC_APDU apdu;
	int ret;

	if( ci->type==SC_CARD_BASICCARD_ENH_3 ) return( SC_EXIT_NOT_SUPPORTED );

	ci->sw[0]=0x00; ci->sw[1]=0x00;

	if( datalen>255 ) return( SC_EXIT_BAD_PARAM );

	apdu.cse=SC_APDU_CASE_3_SHORT;
	apdu.cmd=cmd;
	apdu.cmdlen=5+datalen;
	apdu.rsp=rsp;
	apdu.rsplen=0;

	memset( cmd, 0, sizeof(cmd) );

	cmd[0]=0xC0;
	cmd[1]=0x06;
	cmd[2]=(offset>>8)&0xFF;
	cmd[3]=offset&0xFF;
	cmd[4]=datalen;

	memcpy( cmd+5, data, datalen );

	if( (ret=scBasiccardEncrCAPDU( ci, &apdu )) !=SC_EXIT_OK ) return( ret );
	ret=scReaderSendAPDU( ri, ci, &apdu );
	memset( cmd, 0, sizeof(cmd) );
	if( ret!=SC_EXIT_OK ) return( ret );
	if( (ret=scBasiccardDecrRAPDU( ci, &apdu )) !=SC_EXIT_OK ) return( ret );

	if( apdu.rsplen!=2 ) return( SC_EXIT_BAD_SW );

	ci->sw[0] = rsp[apdu.rsplen-2];
	ci->sw[1] = rsp[apdu.rsplen-1];

	return( SC_EXIT_OK );
}

int scBasiccardCmdReadEeprom( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	WORD offset, BYTE *data, int *datalen )	
{
	BYTE cmd[ 5+10 ];
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

	memset( cmd, 0, sizeof(cmd) );

	cmd[0]=0xC0;
	cmd[1]=0x08;
	cmd[2]=(offset>>8)&0xFF;
	cmd[3]=offset&0xFF;
	cmd[4]=*datalen;

	if( (ret=scBasiccardEncrCAPDU( ci, &apdu )) !=SC_EXIT_OK ) return( ret );
	ret=scReaderSendAPDU( ri, ci, &apdu );
	memset( cmd, 0, sizeof(cmd) );
	if( ret!=SC_EXIT_OK ) return( ret );
	if( (ret=scBasiccardDecrRAPDU( ci, &apdu )) !=SC_EXIT_OK ) return( ret );

	if( apdu.rsplen<2 ) return( SC_EXIT_BAD_SW );

	ci->sw[0] = rsp[apdu.rsplen-2];
	ci->sw[1] = rsp[apdu.rsplen-1];

	memcpy( data, rsp, apdu.rsplen-2 );
	*datalen=apdu.rsplen-2;

	memset( rsp, 0, sizeof(rsp) );

	return( SC_EXIT_OK );
}

/* Do NOT execute this function on a card which had never an image uploaded. */
int scBasiccardCmdEepromCrc( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	WORD offset, WORD length, WORD *crc )	
{
	BYTE cmd[ 8+10 ];
	BYTE rsp[ SC_GENERAL_SHORT_DATA_SIZE+2 ];
	SC_APDU apdu;
	int ret;

	ci->sw[0]=0x00; ci->sw[1]=0x00;

	apdu.cse=SC_APDU_CASE_4_SHORT;
	apdu.cmd=cmd;
	apdu.cmdlen=8;
	apdu.rsp=rsp;
	apdu.rsplen=0;

	memset( cmd, 0, sizeof(cmd) );

	cmd[0]=0xC0;
	cmd[1]=0x0A;
	cmd[2]=(offset>>8)&0xFF;
	cmd[3]=offset&0xFF;
	cmd[4]=0x02;
	cmd[5]=(length>>8)&0xFF;
	cmd[6]=length&0xFF;
	cmd[7]=0x02;

	if( (ret=scBasiccardEncrCAPDU( ci, &apdu )) !=SC_EXIT_OK ) return( ret );
	ret=scReaderSendAPDU( ri, ci, &apdu );
	memset( cmd, 0, sizeof(cmd) );
	if( ret!=SC_EXIT_OK ) return( ret );
	if( (ret=scBasiccardDecrRAPDU( ci, &apdu )) !=SC_EXIT_OK ) return( ret );

	if( apdu.rsplen<2 ) return( SC_EXIT_BAD_SW );

	ci->sw[0] = rsp[apdu.rsplen-2];
	ci->sw[1] = rsp[apdu.rsplen-1];

	*crc=0;
	if( apdu.rsplen==4 ) {
		*crc=(rsp[0]<<8)|rsp[1];
	}

	return( SC_EXIT_OK );
}

int scBasiccardCmdSetState( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	BYTE state )
{
	BYTE cmd[ 4+10 ];
	BYTE rsp[ SC_GENERAL_SHORT_DATA_SIZE+2 ];
	SC_APDU apdu;
	int ret;

	ci->sw[0]=0x00; ci->sw[1]=0x00;

	apdu.cse=SC_APDU_CASE_1;
	apdu.cmd=cmd;
	apdu.cmdlen=4;
	apdu.rsp=rsp;
	apdu.rsplen=0;

	memset( cmd, 0, sizeof(cmd) );

	cmd[0]=0xC0;
	cmd[1]=0x0C;
	cmd[2]=state;

	if( (ret=scBasiccardEncrCAPDU( ci, &apdu )) !=SC_EXIT_OK ) return( ret );
	ret=scReaderSendAPDU( ri, ci, &apdu );
	memset( cmd, 0, sizeof(cmd) );
	if( ret!=SC_EXIT_OK ) return( ret );
	if( (ret=scBasiccardDecrRAPDU( ci, &apdu )) !=SC_EXIT_OK ) return( ret );

	if( apdu.rsplen!=2 ) return( SC_EXIT_BAD_SW );

	ci->sw[0] = rsp[apdu.rsplen-2];
	ci->sw[1] = rsp[apdu.rsplen-1];

	return( SC_EXIT_OK );
}

int scBasiccardCmdGetApplId( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	BYTE *applid, int *length )
{
	BYTE cmd[ 5+10 ];
	BYTE rsp[ SC_GENERAL_SHORT_DATA_SIZE+2 ];
	SC_APDU apdu;
	int ret;

	ci->sw[0]=0x00; ci->sw[1]=0x00;

	apdu.cse=SC_APDU_CASE_2_SHORT;
	apdu.cmd=cmd;
	apdu.cmdlen=5;
	apdu.rsp=rsp;
	apdu.rsplen=0;

	memset( cmd, 0, sizeof(cmd) );

	cmd[0]=0xC0;
	cmd[1]=0x0E;
	/* For ACR20 compatibility not 0x00. */
	cmd[4]=0xFF;

	if( (ret=scBasiccardEncrCAPDU( ci, &apdu )) !=SC_EXIT_OK ) return( ret );
	ret=scReaderSendAPDU( ri, ci, &apdu );
	memset( cmd, 0, sizeof(cmd) );
	if( ret!=SC_EXIT_OK ) return( ret );
	if( (ret=scBasiccardDecrRAPDU( ci, &apdu )) !=SC_EXIT_OK ) return( ret );

	if( apdu.rsplen<2 ) return( SC_EXIT_BAD_SW );

	ci->sw[0] = rsp[apdu.rsplen-2];
	ci->sw[1] = rsp[apdu.rsplen-1];

	memcpy( applid, rsp, apdu.rsplen-2 );
	*length = apdu.rsplen-2;

	memset( rsp, 0, sizeof(rsp) );

	return( SC_EXIT_OK );
}

int scBasiccardCmdStartEncr( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	BYTE algo, BYTE keynum, const BYTE *key, const BYTE *rand )
{
	BYTE cmd[]={ 0xC0, 0x10, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x00, 0x04 };
	BYTE rsp[ SC_GENERAL_SHORT_DATA_SIZE+2 ];
	SC_APDU apdu;
	int ret;
	int i;
	LONG lfsr;

	ci->sw[0]=0x00; ci->sw[1]=0x00;

	apdu.cse=SC_APDU_CASE_4_SHORT;
	apdu.cmd=cmd;
	apdu.cmdlen=10;
	apdu.rsp=rsp;
	apdu.rsplen=0;

	cmd[2]=algo;
	cmd[3]=keynum;

	memcpy( cmd+5, rand, 4 );

	ci->crypt.encrypt=FALSE;
	ci->crypt.mac=FALSE;
	ci->crypt.algo=0;
	ci->crypt.keynum=0;
	memset( ci->crypt.iv, 0, 8 );
	memset( ci->crypt.key, 0, 16 );

	ret=scReaderSendAPDU( ri, ci, &apdu );
	memset( cmd, 0, sizeof(cmd) );
	if( ret!=SC_EXIT_OK ) return( ret );

	if( apdu.rsplen<2 ) return( SC_EXIT_BAD_SW );

	ci->sw[0] = apdu.rsp[apdu.rsplen-2];
	ci->sw[1] = apdu.rsp[apdu.rsplen-1];

	if( apdu.rsplen==6 ) {
		ci->crypt.encrypt=TRUE;
		if( algo==SC_BASICCARD_ALGO_LFSR_CRC) {
			ci->crypt.mac=TRUE;
		} else {
			ci->crypt.mac=FALSE;
		}
		ci->crypt.algo=algo;
		ci->crypt.keynum=keynum;
		memcpy( ci->crypt.iv, rand, 2 );
		memcpy( ci->crypt.iv+2, apdu.rsp, 4 );
		memcpy( ci->crypt.iv+6, rand+2, 2 );
		if( algo==SC_BASICCARD_ALGO_DES ) {
			memcpy( ci->crypt.key, key, 8 );
		} else {
			memcpy( ci->crypt.key, key, 16 );
		}
		/* Algo==LFSR */
		if( (algo&0xF0)==0x10 ) {
			/* Xor Key into IV. */
			for( i=0; i<8; i++ ) ci->crypt.iv[i]^=ci->crypt.key[i+8];

			/* Convert into native format. */
			/* LFSR registers. */
			lfsr=((LONG)ci->crypt.iv[0]<<24) |
				((LONG)ci->crypt.iv[1]<<16) |
				(ci->crypt.iv[2]<<8) | ci->crypt.iv[3];
			lfsr&=0x7FFFFFFF;
			if( !lfsr ) lfsr=1;
			*(LONG *)(ci->crypt.iv)=lfsr;

			lfsr=((LONG)ci->crypt.iv[4]<<24) |
				((LONG)ci->crypt.iv[5]<<16) |
				(ci->crypt.iv[6]<<8) | ci->crypt.iv[7];
			if( !lfsr ) lfsr=1;
			*(LONG *)(ci->crypt.iv+4)=lfsr;

			/* Polynom */
			lfsr=((LONG)ci->crypt.key[0]<<24) |
				((LONG)ci->crypt.key[1]<<16) |
				(ci->crypt.key[2]<<8) | ci->crypt.key[3];
			*(LONG *)(ci->crypt.key)=lfsr;

			lfsr=((LONG)ci->crypt.key[4]<<24) |
				((LONG)ci->crypt.key[5]<<16) |
				(ci->crypt.key[6]<<8) | ci->crypt.key[7];
			*(LONG *)(ci->crypt.key+4)=lfsr;
		}
	}

	memset( rsp, 0, sizeof(rsp) );

	return( SC_EXIT_OK );
}

int scBasiccardCmdEndEncr( SC_READER_INFO *ri, SC_CARD_INFO *ci )
{
	BYTE cmd[ 4+10 ];
	BYTE rsp[ SC_GENERAL_SHORT_DATA_SIZE+2 ];
	SC_APDU apdu;
	int ret;

	ci->sw[0]=0x00; ci->sw[1]=0x00;

	apdu.cse=SC_APDU_CASE_1;
	apdu.cmd=cmd;
	apdu.cmdlen=4;
	apdu.rsp=rsp;
	apdu.rsplen=0;

	memset( cmd, 0, sizeof(cmd) );

	cmd[0]=0xC0;
	cmd[1]=0x12;

	if( (ret=scBasiccardEncrCAPDU( ci, &apdu )) !=SC_EXIT_OK ) return( ret );

	ci->crypt.encrypt=FALSE;
	ci->crypt.mac=FALSE;
	ci->crypt.algo=0;
	ci->crypt.keynum=0;
	memset( ci->crypt.iv, 0, 8 );
	memset( ci->crypt.key, 0, 16 );

	ret=scReaderSendAPDU( ri, ci, &apdu );
	memset( cmd, 0, sizeof(cmd) );
	if( ret!=SC_EXIT_OK ) return( ret );

	if( apdu.rsplen!=2 ) return( SC_EXIT_BAD_SW );

	ci->sw[0] = rsp[apdu.rsplen-2];
	ci->sw[1] = rsp[apdu.rsplen-1];

	return( SC_EXIT_OK );
}

int scBasiccardCmdEcho( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	BYTE incr, const BYTE *data, int datalen, BYTE *resp, int *resplen )
{
	BYTE cmd[ SC_GENERAL_SHORT_DATA_SIZE+5+1 ];
	BYTE rsp[ SC_GENERAL_SHORT_DATA_SIZE+2 ];
	SC_APDU apdu;
	int ret;

	ci->sw[0]=0x00; ci->sw[1]=0x00;

	if( datalen>255 ) return( SC_EXIT_BAD_PARAM );

	apdu.cse=SC_APDU_CASE_4_SHORT;
	apdu.cmd=cmd;
	apdu.cmdlen=5+1+datalen;
	apdu.rsp=rsp;
	apdu.rsplen=0;

	memset( cmd, 0, sizeof(cmd) );

	cmd[0]=0xC0;
	cmd[1]=0x14;
	cmd[2]=incr;
	cmd[4]=datalen;
	cmd[5+datalen]=datalen;

	memcpy( cmd+5, data, datalen );

	if( (ret=scBasiccardEncrCAPDU( ci, &apdu )) !=SC_EXIT_OK ) return( ret );
	ret=scReaderSendAPDU( ri, ci, &apdu );
	memset( cmd, 0, sizeof(cmd) );
	if( ret!=SC_EXIT_OK ) return( ret );
	if( (ret=scBasiccardDecrRAPDU( ci, &apdu )) !=SC_EXIT_OK ) return( ret );

	if( apdu.rsplen<2 ) return( SC_EXIT_BAD_SW );

	ci->sw[0] = rsp[apdu.rsplen-2];
	ci->sw[1] = rsp[apdu.rsplen-1];

	memcpy( resp, rsp, apdu.rsplen-2 );
	*resplen = apdu.rsplen-2;

	memset( rsp, 0, sizeof(rsp) );

	return( SC_EXIT_OK );
}

int scBasiccardCmdAssignNad( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	BYTE nad )
{
	BYTE cmd[ 4+10 ];
	BYTE rsp[ SC_GENERAL_SHORT_DATA_SIZE+2 ];
	SC_APDU apdu;
	int ret;

	ci->sw[0]=0x00; ci->sw[1]=0x00;

	apdu.cse=SC_APDU_CASE_1;
	apdu.cmd=cmd;
	apdu.cmdlen=4;
	apdu.rsp=rsp;
	apdu.rsplen=0;

	memset( cmd, 0, sizeof(cmd) );

	cmd[0]=0xC0;
	cmd[1]=0x16;
	cmd[2]=nad;

	if( (ret=scBasiccardEncrCAPDU( ci, &apdu )) !=SC_EXIT_OK ) return( ret );
	ret=scReaderSendAPDU( ri, ci, &apdu );
	memset( cmd, 0, sizeof(cmd) );
	if( ret!=SC_EXIT_OK ) return( ret );
	if( (ret=scBasiccardDecrRAPDU( ci, &apdu )) !=SC_EXIT_OK ) return( ret );

	if( apdu.rsplen!=2 ) return( SC_EXIT_BAD_SW );

	ci->sw[0] = rsp[apdu.rsplen-2];
	ci->sw[1] = rsp[apdu.rsplen-1];

	return( SC_EXIT_OK );
}

int scBasiccardCmdFileIo( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	BYTE syscode, BYTE filenum, const BYTE *data, int datalen, BYTE *status,
	BYTE *resp, int *resplen )
{
	BYTE cmd[ SC_GENERAL_SHORT_DATA_SIZE+5+1 ];
	BYTE rsp[ SC_GENERAL_SHORT_DATA_SIZE+2 ];
	SC_APDU apdu;
	int ret;

	ci->sw[0]=0x00; ci->sw[1]=0x00;

	if( datalen>255 ) return( SC_EXIT_BAD_PARAM );
	if( *resplen>256 ) return( SC_EXIT_BAD_PARAM );

	apdu.cse=SC_APDU_CASE_4_SHORT;
	apdu.cmd=cmd;
	apdu.cmdlen=5+1+datalen;
	apdu.rsp=rsp;
	apdu.rsplen=0;

	memset( cmd, 0, sizeof(cmd) );

	cmd[0]=0xC0;
	cmd[1]=0x18;
	cmd[2]=syscode;
	cmd[3]=filenum;
	cmd[4]=datalen;
	cmd[5+datalen]=*resplen&0xFF;

	if( data!=NULL ) memcpy( cmd+5, data, datalen );

	if( (ret=scBasiccardEncrCAPDU( ci, &apdu )) !=SC_EXIT_OK ) return( ret );
	ret=scReaderSendAPDU( ri, ci, &apdu );
	memset( cmd, 0, sizeof(cmd) );
	if( ret!=SC_EXIT_OK ) return( ret );
	if( (ret=scBasiccardDecrRAPDU( ci, &apdu )) !=SC_EXIT_OK ) return( ret );

	if( apdu.rsplen<2 ) return( SC_EXIT_BAD_SW );

	ci->sw[0] = rsp[apdu.rsplen-2];
	ci->sw[1] = rsp[apdu.rsplen-1];

	*resplen = 0;
	*status = 0xFF;

	if( apdu.rsplen>=3 ) {
		*status=rsp[0];
		memcpy( resp, rsp+1, apdu.rsplen-3 );
		*resplen = apdu.rsplen-3;
	}

	memset( rsp, 0, sizeof(rsp) );

	return( SC_EXIT_OK );
}


