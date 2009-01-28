/****************************************************************************
*																			*
*					SCEZ chipcard library - T=0 routines					*
*					Copyright Matthias Bruestle 1999,2000					*
*					For licensing, see the file COPYRIGHT					*
*																			*
****************************************************************************/

/* $Id: sct0.c 1101 2002-06-21 14:38:57Z zwiebeltu $ */

#include <scez/scinternal.h>
#include <scez/sct0.h>

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/* Transmit APDU with protocol T=0 */

/* Supports only cases 1, 2 Short, 3 Short, 4 Short.
 * Case 4 Short:
 *  - You have to get the response data yourself, e.g. with GET RESPONSE
 *  - The le-byte is omitted.
 *
 * Inverse convention is handled by the Read/Write functions.
 */


int scT0SendCmd( SC_READER_INFO *ri, SC_CARD_INFO *ci, SC_APDU *apdu )
{
	BYTE rsp[ SC_GENERAL_SHORT_DATA_SIZE+2 ];
	BYTE header[5];
	BYTE ins = apdu->cmd[1];

	int	lc=apdu->cmd[4];
	int le=apdu->cmd[4];
	int	rsplen=0;
	int ret;
	int byte;
	int pointer=0;
	int spointer=0;
	int null_counter=0;

	/* If command doesn't receive data it doesn't matter which value le has */
	if( le==0x00 ) le=256;

	if( (ret=scReaderCheckAPDU( apdu, TRUE ))!=SC_EXIT_OK ) return( ret );

	/* Get unwanted bytes */
	while(scReaderReadChar( ri, ci, SC_TIMEOUT_DEFAULT )!=-1);

	/* Check card status */
	if( (ret = scReaderCardStatus( ri ))!=SC_EXIT_OK ) return( ret );
	if( ri->status & SC_CARD_STATUS_CHANGED ) return( SC_EXIT_CARD_CHANGED );
	if( !(ri->status & SC_CARD_STATUS_PRESENT) ) return( SC_EXIT_NO_CARD );

	apdu->rsplen=0;

	/* Send header */
	memset( header, 0, 5 );
	if( apdu->cmdlen==4 ) {
		/* Case 1 with left of 0-Byte */
		memcpy( header, apdu->cmd, 4 );
	} else {
		memcpy( header, apdu->cmd, 5 );
	}
	if( scReaderWriteBuffer( ri, ci, header, 5 ) != 5 )
		return( SC_EXIT_IO_ERROR );

	while( TRUE ) {

		/* Read byte */
		if( (byte=scReaderReadChar( ri, ci, ri->etu * ci->t0.wwt ))==-1 ) {
			memset( rsp, 0, sizeof(rsp) );
			return( SC_EXIT_IO_ERROR );
		}

		/* ACK */
		if( (byte&0xFE)==(ins&0xFE) ) {
			/* Direction: To Card */
			if( (apdu->cse==SC_APDU_CASE_3_SHORT) ||
				(apdu->cse==SC_APDU_CASE_4_SHORT) ) {

				/* Send data to card */
				if( scReaderWriteBuffer( ri, ci, apdu->cmd+5+spointer,
					lc-spointer )!= lc-spointer ) {
					memset( rsp, 0, sizeof(rsp) );
					return( SC_EXIT_IO_ERROR );
				}

				continue;
			} else
			/* Direction: From Card */
			if( apdu->cse==SC_APDU_CASE_2_SHORT ) {

				/* Receive data from card */
				if( (ret=scReaderReadBuffer( ri, ci, rsp+pointer,
					le +2 - pointer, ri->etu * ci->t0.wwt ))==-1 ) {
					memset( rsp, 0, sizeof(rsp) );
					return( SC_EXIT_IO_ERROR );
				}

				rsplen += ret;

				/* Should be made simpler, because scReaderReadBuffer
				 * doesn't read more than requested.
				 */
				/* Don't copy more than requested */
				if( rsplen>(le+2) ) {
					memcpy( apdu->rsp, rsp, le );
					memcpy( apdu->rsp+le, rsp+rsplen-2, 2 );
					apdu->rsplen=le+2;
				} else {
					memcpy( apdu->rsp, rsp, rsplen );
					apdu->rsplen=rsplen;
				}

				memset( rsp, 0, sizeof(rsp) );

				return( SC_EXIT_OK );
#ifndef T0_LAX_CASE_HANDLING
			} else {
				memset( rsp, 0, sizeof(rsp) );
				return( SC_EXIT_PROTOCOL_ERROR );
#endif /* T0_LAX_CASE_HANDLING */
			}
		}

		/* ~ACK */
		if( ((~byte)&0xFE)==(ins&0xFE) ) {
			/* Direction: To Card */
			if( (apdu->cse==SC_APDU_CASE_3_SHORT) ||
				(apdu->cse==SC_APDU_CASE_4_SHORT) ) {

				/* Send data to card */
				if( scReaderWriteBuffer( ri, ci, apdu->cmd+5+spointer,
					1 )!= 1 ) {
					memset( rsp, 0, sizeof(rsp) );
					return( SC_EXIT_IO_ERROR );
				}

				spointer++;

				continue;
			} else
			/* Direction: From Card */
			if( apdu->cse==SC_APDU_CASE_2_SHORT ) {

				/* Receive data from card */
				if( (ret=scReaderReadChar( ri, ci, ri->etu*ci->t0.wwt ))==-1 ) {
					memset( rsp, 0, sizeof(rsp) );
					return( SC_EXIT_IO_ERROR );
				}

				if( (sizeof(rsp)-pointer)>0 ) {
					rsp[pointer] = ret;
					rsplen++;
					pointer++;
				}

				continue;
#ifndef T0_LAX_CASE_HANDLING
			} else {
				memset( rsp, 0, sizeof(rsp) );
				return( SC_EXIT_PROTOCOL_ERROR );
#endif /* T0_LAX_CASE_HANDLING */
			}
		}

		/* NULL */
		if( byte==0x60 ) {
#ifdef READER_DEBUG
printf("[*N*]");
#endif /* READER_DEBUG */

			null_counter++;

			if( ri->scWaitReq!=NULL ) {
				ret=ri->scWaitReq( ri, ci, null_counter );
				if( ret!=SC_EXIT_OK ) return( ret );
			} else if( null_counter>50 ) {
				/* Exit when to many NULLs received */
				memset( rsp, 0, sizeof(rsp) );
				return( SC_EXIT_TIMEOUT );
			}

			continue;
		}

		/* SW1 */
		if( ((byte&0xF0)==0x60) || ((byte&0xF0)==0x90) ) {
			if( (sizeof(rsp)-pointer)>0 ) {
				rsp[pointer]=byte;
				pointer++;
				rsplen++;
			}

			/* Read SW2 */
			if( (byte=scReaderReadChar( ri, ci, ri->etu * ci->t0.wwt ))==-1 ) {
				memset( rsp, 0, sizeof(rsp) );
				return( SC_EXIT_IO_ERROR );
			}

			if( (sizeof(rsp)-pointer)>0 ) {
				rsp[pointer]=byte;
				pointer++;
				rsplen++;
			}

			/* Don't copy more than requested */
			if( rsplen>(le+2) ) {
				memcpy( apdu->rsp, rsp, le );
				memcpy( apdu->rsp+le, rsp+rsplen-2, 2 );
				apdu->rsplen=le+2;
			} else {
				memcpy( apdu->rsp, rsp, rsplen );
				apdu->rsplen=rsplen;
			}

			memset( rsp, 0, sizeof(rsp) );

			return( SC_EXIT_OK );
		}

	}

	memset( rsp, 0, sizeof(rsp) );

	return( SC_EXIT_UNKNOWN_ERROR );
}

/* Transmit APDU */

/*  - For case 4 instructions: The le-bytes is striped before calling
 *    scReaderT0.
 */

int scT0SendAPDU( SC_READER_INFO *ri, SC_CARD_INFO *ci, SC_APDU *apdu )
{
	int ret;
	BOOLEAN striped=FALSE;

	if( (ret=scReaderCheckAPDU( apdu, TRUE ))!=SC_EXIT_OK ) return( ret );

	/* Strip le-Byte if present. */
	if( (apdu->cse==SC_APDU_CASE_4_SHORT) &&
		(apdu->cmdlen==(5+apdu->cmd[4]+1)) ) {
		apdu->cmdlen--;
		striped=TRUE;
	}

	if( (ret=scReaderT0( ri, ci, apdu ))!=SC_EXIT_OK ) return( ret );
#ifdef READER_DEBUG
printf(" [RSP:");
for(ret=0;ret<apdu->rsplen;ret++) printf(" %.2X",apdu->rsp[ret]);
printf("]");
#endif

	if( striped ) apdu->cmdlen++;

	/* Reissue command with correct Le if required. */
	if( (apdu->cse==SC_APDU_CASE_2_SHORT) &&
		(apdu->rsplen==2) && (apdu->rsp[0]==0x6C) )
	{
		SC_APDU riapdu;
		BYTE cmd[ 5 ];
		BYTE rsp[ SC_GENERAL_SHORT_DATA_SIZE+2 ];
		int le;

		if( apdu->cmdlen!=5 ) return( SC_EXIT_BAD_PARAM );

		le=apdu->cmd[4];
		if( le==0 ) le=256;

		riapdu.cse=SC_APDU_CASE_2_SHORT;
		riapdu.cmd=cmd;
		memcpy( cmd, apdu->cmd, 5 );
		riapdu.cmd[4]=apdu->rsp[1];
		riapdu.cmdlen=5;
		riapdu.rsp=rsp;
		riapdu.rsplen=0;

		if( (ret=scReaderT0( ri, ci, &riapdu ))!=SC_EXIT_OK ) {
			memset( rsp, 0, sizeof(rsp) );
			return( ret );
		}
#ifdef READER_DEBUG
printf(" [RSP:");
for(ret=0;ret<riapdu.rsplen;ret++) printf(" %.2X",riapdu.rsp[ret]);
printf("]");
#endif

		/* Copy response to user-apdu */
		/* Don't copy more than requested */
		if( riapdu.rsplen>(le+2) ) {
			memcpy( apdu->rsp, riapdu.rsp, le );
			memcpy( apdu->rsp+le,
				riapdu.rsp+riapdu.rsplen-2, 2 );
			apdu->rsplen=le+2;
		} else {
			memcpy( apdu->rsp, riapdu.rsp, riapdu.rsplen );
			apdu->rsplen=riapdu.rsplen;
		}

		memset( rsp, 0, sizeof(rsp) );
	}

	/* There should be a response */
	if( (apdu->cse==SC_APDU_CASE_4_SHORT) ||
		(apdu->cse==SC_APDU_CASE_2_SHORT) ) {
		int s;
		int	n;
		int le;

		if( striped ) le=apdu->cmd[apdu->cmdlen-1];
		else le=256;

		if( le==0x00 ) le=256;

		if( apdu->rsplen<2 ) return( SC_EXIT_BAD_SW );

		memcpy( ci->sw, apdu->rsp+apdu->rsplen-2, 2 );

		if( (ret=scSmartcardSimpleProcessSW( ci, &s, &n ))!=SC_EXIT_OK )
			return( ret );

		if( (s==SC_SW_DATA_AVAIL) && (n>0) && (apdu->rsplen==2) ) {
			SC_APDU grapdu;
			BYTE cmd[5];
			BYTE rsp[ SC_GENERAL_SHORT_DATA_SIZE+2 ];

			/* Setup Get Response */
			grapdu.cse=SC_APDU_CASE_2_SHORT;
			grapdu.cmd=cmd;
			grapdu.cmdlen=5;
			grapdu.rsp=rsp;
			grapdu.rsplen=0;
			memcpy( grapdu.cmd , ci->t0.getrsp, 5 );
			grapdu.cmd[4] = (BYTE) n;

#ifdef READER_DEBUG
printf(" [CMD:");
for(ret=0;ret<grapdu.cmdlen;ret++) printf(" %.2X",grapdu.cmd[ret]);
printf("]");
#endif
			/* Execute Get Response */
			if( (ret=scReaderT0( ri, ci, &grapdu ))!=SC_EXIT_OK ) {
				memset( rsp, 0, sizeof(rsp) );
				return( ret );
			}
#ifdef READER_DEBUG
printf(" [RSP:");
for(ret=0;ret<grapdu.rsplen;ret++) printf(" %.2X",grapdu.rsp[ret]);
printf("]");
#endif

			/* Copy response to user-apdu */
			/* Don't copy more than requested */
			if( grapdu.rsplen>(le+2) ) {
				memcpy( apdu->rsp, grapdu.rsp, le );
				memcpy( apdu->rsp+le,
					grapdu.rsp+grapdu.rsplen-2, 2 );
				apdu->rsplen=le+2;
			} else {
				memcpy( apdu->rsp, grapdu.rsp, grapdu.rsplen );
				apdu->rsplen=grapdu.rsplen;
			}

			memset( rsp, 0, sizeof(rsp) );
		}
	}

	return( SC_EXIT_OK );
}



