/****************************************************************************
*																			*
*					SCEZ chipcard library - General routines				*
*					Copyright Matthias Bruestle 1999,2000					*
*					For licensing, see the file COPYRIGHT					*
*																			*
****************************************************************************/

/* $Id: scgeneral.c 1102 2002-06-21 14:40:53Z zwiebeltu $ */

#include <scez/scinternal.h>

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#ifndef __palmos__
#include <time.h>
#endif

#if defined(HAVE_LIBCRYPT)
#include <capi.h>
#endif /* HAVE_LIBCRYPT */

int scInit()
{
#if defined(HAVE_LIBCRYPT)
	if( cryptInit() )
		return( SC_EXIT_LIB_ERROR);
	if( cryptAddRandom( NULL, CRYPT_RANDOM_SLOWPOLL ) )
		return( SC_EXIT_LIB_ERROR);
#endif /* HAVE_LIBCRYPT */

	return( SC_EXIT_OK );
}

int scEnd()
{
#if defined(HAVE_LIBCRYPT)
#if 0
	if( cryptEnd() )
		return( SC_EXIT_LIB_ERROR);
#endif
#endif /* HAVE_LIBCRYPT */

	return( SC_EXIT_OK );
}

/* Create reader */

SC_READER_INFO *scGeneralNewReader( int type, int slot )
{
	SC_READER_INFO *ri;

	ri = (SC_READER_INFO *)malloc( sizeof(SC_READER_INFO) );
	if( ri==NULL ) return(NULL);

	ri->major=type;
	ri->minor=0;
	ri->slot=slot;
	ri->status=0;
	ri->etu=104;
	ri->maxc=0;
	ri->maxr=0;
	ri->pinpad=FALSE;
	ri->display=FALSE;
	ri->si=NULL;
	ri->ctn=0;
	ri->fd=-1;

	ri->scShutdown=NULL;
	ri->scGetCap=NULL;
	ri->scActivate=NULL;
	ri->scDeactivate=NULL;
	ri->scWriteBuffer=NULL;
	ri->scReadBuffer=NULL;
	ri->scWriteChar=NULL;
	ri->scReadChar=NULL;
	ri->scWaitForData=NULL;
	ri->scSetSpeed=NULL;
	ri->scCardStatus=NULL;
	ri->scResetCard=NULL;
	ri->scT0=NULL;
	ri->scT1=NULL;
	ri->scSendAPDU=NULL;
	ri->scVerifyPIN=NULL;
	ri->scWaitReq=NULL;

	return( ri );
}

/* Remove reader */

void scGeneralFreeReader( SC_READER_INFO **ri )
{
	if( *ri!=NULL ) free( *ri );
	*ri=NULL;
}

/* Create reader capabilities structure */

SC_READER_CAP *scGeneralNewReaderCap()
{
	SC_READER_CAP *rp;

	rp = (SC_READER_CAP *)malloc( sizeof(SC_READER_CAP) );
	if( rp==NULL ) return( NULL );

	rp->t0err=FALSE;
	rp->t1=FALSE;
	rp->freq=3579500;
	rp->motor=FALSE;
	rp->slots=1;
	rp->n_fd=0;
	memset( rp->fd, 0, sizeof(rp->fd) );
	memset( rp->speed, 0, sizeof(rp->speed) );

	return( rp );
}

/* Free reader capabilities structure */

void scGeneralFreeReaderCap( SC_READER_CAP **rp )
{
	if( *rp!=NULL ) free( *rp );
	*rp=NULL;
}

/* Create card */

SC_CARD_INFO *scGeneralNewCard()
{
	SC_CARD_INFO *ci;

	if( (ci=(SC_CARD_INFO *)malloc(sizeof(SC_CARD_INFO)))==NULL )
		return( NULL );

	ci->type=SC_CARD_UNKNOWN;
	memset( ci->atr, 0, sizeof(ci->atr) );
	ci->atrlen=0;
	ci->protocol=SC_PROTOCOL_UNKNOWN;
	ci->direct=TRUE;
	ci->cla=0x00;
	ci->swok[0]=0;
	ci->swav[0]=0;
	ci->sw[0]=0;
	ci->sw[1]=0;
	ci->t0.d=1;
	ci->t0.wi=10;
	ci->t0.wwt=9600;
	memset( ci->t0.getrsp, 0, sizeof(ci->t0.getrsp) );
	ci->memsize=0;
	ci->t1.ns=0;
	ci->t1.nr=0;
	ci->t1.ifsc=32;
	ci->t1.ifsd=255;
	ci->t1.ifsreq=FALSE;
	ci->t1.cwt=8192;
	ci->t1.bwt=1600000;
	ci->t1.rc=SC_T1_CHECKSUM_LRC;
	ci->crypt.encrypt=FALSE;
	ci->crypt.mac=FALSE;
	ci->crypt.keynum=0;
	ci->crypt.algo=0;
	memset( ci->crypt.key, 0, sizeof(ci->crypt.key) );
	memset( ci->crypt.pin, 0, sizeof(ci->crypt.pin) );
	memset( ci->crypt.iv, 0, sizeof(ci->crypt.iv) );

	ci->scGetCardData=NULL;
	ci->scGetCap=NULL;

	return( ci );
}

/* Remove card */

void scGeneralFreeCard( SC_CARD_INFO **ci )
{
	if( *ci!=NULL ) {
		memset( (*ci)->crypt.key, 0, sizeof((*ci)->crypt.key) );
		memset( (*ci)->crypt.pin, 0, sizeof((*ci)->crypt.pin) );
		memset( (*ci)->crypt.iv, 0, sizeof((*ci)->crypt.iv) );
		free( *ci );
	}
	*ci=NULL;
}

/* Create card capabilities structure */

SC_CARD_CAP *scGeneralNewCardCap()
{
	SC_CARD_CAP *cp;

	cp = (SC_CARD_CAP *)malloc( sizeof(SC_CARD_CAP) );
	if( cp==NULL ) return( NULL );

	cp->n_fd=0;
	memset( cp->fd, 0, sizeof(cp->fd) );

	return( cp );
}

/* Free card capabilities structure */

void scGeneralFreeCardCap( SC_CARD_CAP **cp )
{
	if( *cp!=NULL ) free( *cp );
	*cp=NULL;
}

/* Create APDU */

SC_APDU *scGeneralNewAPDU()
{
	SC_APDU *apdu;

	if( (apdu=(SC_APDU *)malloc(sizeof(SC_APDU)))==NULL ) return( NULL );

	apdu->cse=SC_APDU_CASE_NONE;
	apdu->cmd=NULL;
	apdu->cmdlen=0;
	apdu->rsp=NULL;
	apdu->rsplen=0;

	return( apdu );
}

/* Remove APDU */

void scGeneralFreeAPDU( SC_APDU **apdu )
{
	if( *apdu!=NULL ) free( *apdu );
	*apdu=NULL;
}

/* Reverse bitorder of each byte */

void scGeneralReverseString( BYTE *data, int len)
{
	int	i;
	BYTE	b;

	while( len ) {
		len--;
		b=0;
		for( i=0; i<8; i++ ) {
			if( data[len] & (1<<i) ) b |= 0x80 >> i;
		}
		/* Hmm... */
		data[len]=b^0xFF;
	}

	b=0;
}

/* Get random bytes */
/* NO TRUE RANDOM NUMBERS! */

/* For Linux use /dev/random. */

int scGeneralGetRandStr( BYTE *data, int len )
{
#if defined(HAVE_LIBCRYPT)
	cryptGetRandom( data, len );
#elif defined(__palmos__)
	SysRandom( TimGetTicks() );

	while( len-- ){
		*data++ = SysRandom(0) & 0xFF;
	}
#elif defined(__linux__)
	FILE *fptr;

	fptr = fopen( "/dev/urandom", "rb" );
	if( fptr==NULL ) return( SC_EXIT_UNKNOWN_ERROR );

	if( fread( data, 1, len, fptr )!=len ) return( SC_EXIT_UNKNOWN_ERROR );
#else
	srand( time( NULL ) );

	while( len-- ){
		*data++ = rand() & 0xFF;
	}
#endif /* HAVE_LIBCRYPT */

	return( SC_EXIT_OK );
}

/* Clean keys in SC_CARD_INFO. */

void scGeneralCleanKeys( SC_CARD_INFO *ci )
{
	ci->crypt.encrypt=FALSE;
	ci->crypt.mac=FALSE;
	ci->crypt.keynum=0;
	ci->crypt.algo=0;
	memset( ci->crypt.key, 0, sizeof(ci->crypt.key) );
	memset( ci->crypt.pin, 0, sizeof(ci->crypt.pin) );
	memset( ci->crypt.iv, 0, sizeof(ci->crypt.iv) );
}

/* Cleans SC_CARD_INFO. */

void scGeneralCleanCI( SC_CARD_INFO *ci )
{
	scGeneralCleanKeys( ci );
	ci->t1.nad=0;
	ci->t1.ns=0;
	ci->t1.nr=0;
	ci->t1.ifsd=255;
	ci->t1.ifsreq=FALSE;
}

/* Convert binary to ascii coded hex and back. Returns length of out. */

int scGeneralBinHex( BOOLEAN tohex, const BYTE *in, int inlen, BYTE *out )
{
	BYTE b2h[]={ 0x30,0x31,0x32,0x33,0x34,0x35,0x36,0x37,0x38,0x39,
		0x41,0x42,0x43,0x44,0x45,0x46 };
	int i;
	BYTE b=0;

	if( tohex ) {
		for( i=0; i<inlen; i++ ) {
			out[i<<1] = b2h[ in[i]>>4 ];
			out[(i<<1)+1] = b2h[ in[i]&0xF ];
		}
		return( i<<1 );
	} else {
		for( i=0; i<inlen; i++ ) {
			if( i&1 ) {
				if( (in[i]&0xF0)==0x30 ) b |= in[i] & 0xF;
				else b |= (in[i]&0xF) + 9;
				out[i>>1] = b;
			} else {
				if( (in[i]&0xF0)==0x30 ) b = in[i] & 0xF;
				else b = (in[i]&0xF) + 9;
				b<<=4;
			}
		}

		if( inlen&1 ) {
			out[inlen>>1] = b;
			return( inlen>>1 );
		}

		return( inlen>>1 );
	}

	/* Should be never here. */
	return( -1 );
}


