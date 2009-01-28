/****************************************************************************
*																			*
*					PKCS#15 library - Cryptoflex routines					*
*						Copyright Matthias Bruestle 1999					*
*					For licensing, see the file COPYRIGHT					*
*																			*
****************************************************************************/

/* $Id: p15cryptoflex.c 874 2000-09-01 15:24:13Z zwiebeltu $ */

#include <stdio.h>
#include <string.h>

#include "capi.h"
#include "keymgmt/stream.h"
#include "keymgmt/asn1.h"

#include "scgeneral.h"
#include "p15general.h"
#include "p15cryptoflex.h"

#include "sccryptoflex.h"

/* Read directory and file locations. */

/* static */ int p15CryptoflexGetPaths( P15_INFO *pi )
{
	STREAM stream, *streamptr=&stream;
	BYTE buffer[ SC_GENERAL_SHORT_DATA_SIZE ];
	BYTE sbuffer[ SC_GENERAL_SHORT_DATA_SIZE ];
	int resplen;
	BYTE aid[ 12 ];
	int aidlen;
	int ret;
	long seqlen, nextseq;

	/* Reset paths. */
	pi->dirpathlen=0;
	pi->odfpathlen=0;
	pi->uspathlen=0;
	pi->tipathlen=0;

	/* Select MF. */
	ret=scCryptoflexCmdSelectFile( pi->ri, pi->ci, 0x3F00, buffer, &resplen );
	if( ret!=SC_EXIT_OK ) return( ret );
	if( pi->ci->sw[0]!=0x90 ) return( SC_EXIT_UNKNOWN_ERROR );

	/* Select EF(DIR). */
	ret=scCryptoflexCmdSelectFile( pi->ri, pi->ci, 0x2F00, buffer, &resplen );
	if( ret!=SC_EXIT_OK ) return( ret );
	if( pi->ci->sw[0]!=0x90 ) return( SC_EXIT_FILE_ERROR );

	/* Response OK? */
	if( resplen<14 ) return( SC_EXIT_UNKNOWN_ERROR );

	/* Correct file type? */
	if( buffer[6]!=0x01 ) return( SC_EXIT_FILE_ERROR );

	/* Get file size. */
	resplen=buffer[3];
	if( buffer[2] ) resplen=256;

	/* Read EF(DIR). */
	/* 256 bytes should be enough for EF(DIR). At least until the arival
	 * of the multi multi application card.
	 */
	ret=scCryptoflexCmdReadBinary( pi->ri, pi->ci, 0, buffer, &resplen );
	if( ret!=SC_EXIT_OK ) return( ret );
	if( pi->ci->sw[0]!=0x90 ) return( SC_EXIT_IO_ERROR );

	/* Open stream for ASN.1 routines. */
	sMemOpen( streamptr, sbuffer, sizeof(sbuffer) );

	swrite( streamptr, buffer, resplen );
	sseek( streamptr, 0 );

	/* Process records until EOD. */
	while( (!(sIsEmpty( streamptr ))) &&
		cryptStatusOK( sGetStatus( streamptr ) ) ) {
		/* PKCS15DIRRecord [APPLICATION 1] SEQUENCE */
		if( !checkReadTag( streamptr, BER_APPLICATION+BER_CONSTRUCTED+1 ) ) {
			readTag( streamptr );
			readLength( streamptr, &seqlen );
			nextseq = sMemSize( streamptr ) + seqlen;
			sseek( streamptr, nextseq );
			continue;
		}

		readLength( streamptr, &seqlen );
		nextseq = sMemSize( streamptr ) + seqlen;

		/* aid [APPLICATION 15] OCTET STRING */
		readOctetStringTag( streamptr, aid, &aidlen, sizeof(aid),
			BER_APPLICATION+BER_PRIMITIVE+15 );

		/* Correct AID? */
		if( (aidlen!=12) || memcmp(aid,(BYTE *)"\xA0\x00\x00\x00\x63PKCS-15",
			12) ) {
			sseek( streamptr, nextseq );
			continue;
		}

		/* label [APPLICATION 16] UTF8String OPTIONAL */
		/* We are not interested in the label. */
		if( peekTag( streamptr ) == (BER_APPLICATION+BER_PRIMITIVE+16) ) {
			readTag( streamptr );
			readLength( streamptr, &seqlen );
			sseek( streamptr, sMemSize( streamptr ) + seqlen );
		}

		/* path [APPLICATION 17] OCTET STRING */
		readOctetStringTag( streamptr, pi->dirpath, &(pi->dirpathlen),
			sizeof(pi->dirpath), BER_APPLICATION+17 );

		/* End of SEQUENCE? */
		if( nextseq==sMemSize( streamptr ) ) break;
			
		/* ddo [APPLICATION 19] PKCS15DDO SEQUENCE OPTIONAL */
		if( !checkReadTag( streamptr, BER_APPLICATION+BER_CONSTRUCTED+19 ) )
			break;
		readLength( streamptr, &seqlen );

		/* oid OBJECT IDENTIFIER */
		/* We don't need no stinkin' OID. */
		readTag( streamptr );
		readLength( streamptr, &seqlen );
		sseek( streamptr, sMemSize( streamptr ) + seqlen );

		/* odfPath PKCS15Path SEQUENCE OPTIONAL */
		if( checkReadTag( streamptr, BER_SEQUENCE ) ) {
			readLength( streamptr, &seqlen );

			/* path OCTET STRING */
			readOctetString( streamptr, pi->odfpath, &(pi->odfpathlen),
				sizeof(pi->odfpath) );
		}

		/* tokenInfoPath [0] PKCS15Path SEQUENCE OPTIONAL */
		if( checkReadTag( streamptr, BER_CONTEXT_SPECIFIC+BER_CONSTRUCTED+
			0 ) ) {
			readLength( streamptr, &seqlen );

			/* path OCTET STRING */
			readOctetString( streamptr, pi->tipath, &(pi->tipathlen),
				sizeof(pi->tipath) );
		}

		/* unusedPath [1] PKCS15Path SEQUENCE OPTIONAL */
		if( checkReadTag( streamptr, BER_CONTEXT_SPECIFIC+BER_CONSTRUCTED+
			1 ) ) {
			readLength( streamptr, &seqlen );

			/* path OCTET STRING */
			readOctetString( streamptr, pi->uspath, &(pi->uspathlen),
				sizeof(pi->uspath) );
		}

		break;
	}

	if( cryptStatusError( sGetStatus( streamptr ) ) ) ret=SC_EXIT_DATA_ERROR;

	/* Close stream. */
	sMemClose( streamptr );

	if( pi->odfpathlen==0 ) {
		memcpy( pi->odfpath, pi->dirpath, pi->dirpathlen );
		pi->odfpathlen=pi->dirpathlen;
		pi->odfpath[pi->odfpathlen++]=0x50;
		pi->odfpath[pi->odfpathlen++]=0x31;
	}

	if( pi->tipathlen==0 ) {
		memcpy( pi->tipath, pi->dirpath, pi->dirpathlen );
		pi->tipathlen=pi->dirpathlen;
		pi->tipath[pi->tipathlen++]=0x50;
		pi->tipath[pi->tipathlen++]=0x32;
	}

	if( pi->uspathlen==0 ) {
		memcpy( pi->uspath, pi->dirpath, pi->dirpathlen );
		pi->uspathlen=pi->dirpathlen;
		pi->uspath[pi->uspathlen++]=0x50;
		pi->uspath[pi->uspathlen++]=0x32;
	}

	return( ret );
}

/* Write or change EF(DIR) in MF. */

/* static */ int p15CryptoflexWriteEfDir( P15_INFO *pi, char *label )
{
	STREAM stream, *streamptr=&stream;
	BYTE sbuffer[ P15_CRYPTOFLEX_EFDIR_SIZE ];
	BYTE buffer[ SC_GENERAL_SHORT_DATA_SIZE ];
	BYTE aid[] = { 0xA0,0x00,0x00,0x00,0x63,'P','K','C','S','-','1','5' };
	BYTE path[] = { 0x3F,0x00,0x50,0x15 };
	BYTE chall[8];
	BYTE acond[4], akeys[3];
	int slen, ret, resplen;

	memset( sbuffer, 0, sizeof(sbuffer) );

	/* Open stream for ASN.1 routines. */
	sMemOpen( streamptr, sbuffer, sizeof(sbuffer) );

	/* PKCS15DIRRecord [APPLICATION 1] SEQUENCE */
	writeTag( streamptr, BER_APPLICATION+BER_CONSTRUCTED+1 );
	writeLength( streamptr, sizeofObject(sizeof(aid)) +
		sizeofObject(strlen(label)) + sizeofObject(sizeof(path)) );

	/* aid [APPLICATION 15] OCTET STRING */
	writeOctetString( streamptr, aid, sizeof(aid), BER_APPLICATION+15 );

	/* label [APPLICATION 16] UTF8String OPTIONAL */
	writeCharacterString( streamptr, label, strlen(label), BER_APPLICATION+16 );

	/* path [APPLICATION 17] OCTET STRING */
	writeOctetString( streamptr, path, sizeof(path), BER_APPLICATION+17 );

	if( cryptStatusError( sGetStatus( streamptr ) ) ) {
		sMemClose( streamptr );
		return( SC_EXIT_BAD_PARAM );
	}

	slen = sMemSize( streamptr );

	/* Close stream. */
	sMemClose( streamptr );

	if( pi->sokeylen!=SC_CRYPTOFLEX_DES_SIZE ) return( SC_EXIT_AUTH_ERROR );

	/* Select MF. */
	ret=scCryptoflexCmdSelectFile( pi->ri, pi->ci, 0x3F00, buffer, &resplen );
	if( ret!=SC_EXIT_OK ) return( ret );
	if( pi->ci->sw[0]!=0x90 ) return( SC_EXIT_UNKNOWN_ERROR );

#ifdef P15_AUTH_ALLWAYS
	/* Get Challenge. */
	resplen=SC_CRYPTOFLEX_CHALL_SIZE;
	ret = scCryptoflexCmdGetChall( pi->ri, pi->ci, chall, &resplen );
	if( ret!=SC_EXIT_OK ) return( ret );
	if( (pi->ci->sw[0]!=0x90) || (resplen!=SC_CRYPTOFLEX_CHALL_SIZE) )
		return( SC_EXIT_UNKNOWN_ERROR );

	/* External Authentication with SO key. */
	ret = scCryptoflexCmdExtAuth( pi->ri, pi->ci, 0x01, chall, pi->sokey,
		SC_CRYPTOFLEX_ALGO_DES );
	if( ret!=SC_EXIT_OK ) return( ret );
	if( pi->ci->sw[0]!=0x90 ) return( SC_EXIT_AUTH_ERROR );
#endif /* P15_AUTH_ALLWAYS */

	/* Select EF(DIR). */
	ret=scCryptoflexCmdSelectFile( pi->ri, pi->ci, 0x2F00, buffer, &resplen );
	if( ret!=SC_EXIT_OK ) return( ret );

	if( pi->ci->sw[0]==0x90 ) {
		/* File exists allready. */

		/* Response OK? */
		if( resplen<14 ) return( SC_EXIT_UNKNOWN_ERROR );

		/* Correct file type? */
		if( buffer[6]!=0x01 ) return( SC_EXIT_FILE_ERROR );

		/* Get file size. */
		resplen=buffer[3];
		if( buffer[2] ) resplen=256;

		/* Read EF(DIR). */
		/* 256 bytes should be enough for EF(DIR). At least until the arival
		 * of the multi multi application card.
		 */
		ret=scCryptoflexCmdReadBinary( pi->ri, pi->ci, 0, buffer, &resplen );
		if( ret!=SC_EXIT_OK ) return( ret );
		if( pi->ci->sw[0]!=0x90 ) return( SC_EXIT_IO_ERROR );

		/* TODO */

	} else if( pi->ci->sw[1]==0x82 ) {
		/* File has to be created. */

	    /* Create transparent file 2F00 */
	    acond[0]=0x3F; acond[1]=0x04; acond[2]=0xFF; acond[3]=0xFF;
	    akeys[0]=0xF1; akeys[1]=0xFF; akeys[2]=0xFF;
    	ret = scCryptoflexCmdCreateFile( pi->ri, pi->ci, 0x2F00,
			P15_CRYPTOFLEX_EFDIR_SIZE, SC_CRYPTOFLEX_FILE_TRANSPARENT,
			0x00, SC_CRYPTOFLEX_UNBLOCKED, 0, 0, acond, akeys );
		if( ret!=SC_EXIT_OK ) return( ret );
		if( pi->ci->sw[0]!=0x90 ) return( SC_EXIT_UNKNOWN_ERROR );

		/* Write dir data. */
		ret = scCryptoflexCmdUpdateBinary( pi->ri, pi->ci, 0, sbuffer,
			P15_CRYPTOFLEX_EFDIR_SIZE );
		if( ret!=SC_EXIT_OK ) return( ret );
		if( pi->ci->sw[0]!=0x90 ) return( SC_EXIT_IO_ERROR );
	} else return( SC_EXIT_FILE_ERROR );

	return( SC_EXIT_NOT_IMPLEMENTED );
}

/* Add object to transparent file. */
/* A fid of 0xFFFF means, that the file is allready selected. */

int p15CryptoflexAppendOpjectTR( P15_INFO *pi, WORD fid, BYTE *data,
	int *datalen )
{

	return( SC_EXIT_NOT_IMPLEMENTED );
}

/* Initialize card. */

int p15CryptoflexInitCard( P15_INFO *pi, char *label, char *manufId, BYTE *sn,
    int snlen )
{
	p15CryptoflexWriteEfDir( pi, label );

	return( SC_EXIT_NOT_IMPLEMENTED );
}


