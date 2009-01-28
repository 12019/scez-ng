/****************************************************************************
*																			*
*					SCEZ chipcard library - T=1 routines					*
*	This is probably the first full free/open source T=1 implementation.	*
*					Copyright Matthias Bruestle 1999,2000					*
*					For licensing, see the file COPYRIGHT					*
*																			*
****************************************************************************/

/* $Id: sct1.c 1100 2002-06-21 14:38:00Z zwiebeltu $ */

#include <scez/scinternal.h>
#include <scez/sct1.h>

#include <stdio.h>
#include <string.h>

#if defined(WINDOWS) && !defined(__BORLANDC__)
#include <memory.h>
#endif

/* ISO STD 3309 */
/* From: medin@catbyte.b30.ingr.com (Dave Medin)
 * Subject: CCITT checksums
 * Newsgroups: sci.electronics
 * Date: Mon, 7 Dec 1992 17:33:39 GMT
 */

/* Correct Table? */

static WORD crctab[256] = {
0x0000, 0x1189, 0x2312, 0x329b, 0x4624, 0x57ad, 0x6536, 0x74bf,
0x8c48, 0x9dc1, 0xaf5a, 0xbed3, 0xca6c, 0xdbe5, 0xe97e, 0xf8f7,
0x1081, 0x0108, 0x3393, 0x221a, 0x56a5, 0x472c, 0x75b7, 0x643e,
0x9cc9, 0x8d40, 0xbfdb, 0xae52, 0xdaed, 0xcb64, 0xf9ff, 0xe876,
0x2102, 0x308b, 0x0210, 0x1399, 0x6726, 0x76af, 0x4434, 0x55bd,
0xad4a, 0xbcc3, 0x8e58, 0x9fd1, 0xeb6e, 0xfae7, 0xc87c, 0xd9f5,
0x3183, 0x200a, 0x1291, 0x0318, 0x77a7, 0x662e, 0x54b5, 0x453c,
0xbdcb, 0xac42, 0x9ed9, 0x8f50, 0xfbef, 0xea66, 0xd8fd, 0xc974,
0x4204, 0x538d, 0x6116, 0x709f, 0x0420, 0x15a9, 0x2732, 0x36bb,
0xce4c, 0xdfc5, 0xed5e, 0xfcd7, 0x8868, 0x99e1, 0xab7a, 0xbaf3,
0x5285, 0x430c, 0x7197, 0x601e, 0x14a1, 0x0528, 0x37b3, 0x263a,
0xdecd, 0xcf44, 0xfddf, 0xec56, 0x98e9, 0x8960, 0xbbfb, 0xaa72,
0x6306, 0x728f, 0x4014, 0x519d, 0x2522, 0x34ab, 0x0630, 0x17b9,
0xef4e, 0xfec7, 0xcc5c, 0xddd5, 0xa96a, 0xb8e3, 0x8a78, 0x9bf1,
0x7387, 0x620e, 0x5095, 0x411c, 0x35a3, 0x242a, 0x16b1, 0x0738,
0xffcf, 0xee46, 0xdcdd, 0xcd54, 0xb9eb, 0xa862, 0x9af9, 0x8b70,
0x8408, 0x9581, 0xa71a, 0xb693, 0xc22c, 0xd3a5, 0xe13e, 0xf0b7,
0x0840, 0x19c9, 0x2b52, 0x3adb, 0x4e64, 0x5fed, 0x6d76, 0x7cff,
0x9489, 0x8500, 0xb79b, 0xa612, 0xd2ad, 0xc324, 0xf1bf, 0xe036,
0x18c1, 0x0948, 0x3bd3, 0x2a5a, 0x5ee5, 0x4f6c, 0x7df7, 0x6c7e,
0xa50a, 0xb483, 0x8618, 0x9791, 0xe32e, 0xf2a7, 0xc03c, 0xd1b5,
0x2942, 0x38cb, 0x0a50, 0x1bd9, 0x6f66, 0x7eef, 0x4c74, 0x5dfd,
0xb58b, 0xa402, 0x9699, 0x8710, 0xf3af, 0xe226, 0xd0bd, 0xc134,
0x39c3, 0x284a, 0x1ad1, 0x0b58, 0x7fe7, 0x6e6e, 0x5cf5, 0x4d7c,
0xc60c, 0xd785, 0xe51e, 0xf497, 0x8028, 0x91a1, 0xa33a, 0xb2b3,
0x4a44, 0x5bcd, 0x6956, 0x78df, 0x0c60, 0x1de9, 0x2f72, 0x3efb,
0xd68d, 0xc704, 0xf59f, 0xe416, 0x90a9, 0x8120, 0xb3bb, 0xa232,
0x5ac5, 0x4b4c, 0x79d7, 0x685e, 0x1ce1, 0x0d68, 0x3ff3, 0x2e7a,
0xe70e, 0xf687, 0xc41c, 0xd595, 0xa12a, 0xb0a3, 0x8238, 0x93b1,
0x6b46, 0x7acf, 0x4854, 0x59dd, 0x2d62, 0x3ceb, 0x0e70, 0x1ff9,
0xf78f, 0xe606, 0xd49d, 0xc514, 0xb1ab, 0xa022, 0x92b9, 0x8330,
0x7bc7, 0x6a4e, 0x58d5, 0x495c, 0x3de3, 0x2c6a, 0x1ef1, 0x0f78
};

/* Returns LRC of data */

BYTE scT1Lrc( const BYTE *data, int datalen)
{
	BYTE lrc=0x00;
	int i;

	for( i=0; i<datalen; i++ ) lrc^=data[i];

	return lrc;
}

/* Calculates CRC of data */

void scT1Crc( const BYTE *data, int datalen, BYTE *crc )
{
	int i;
	WORD tmpcrc=0xFFFF;

	for (i=0; i<datalen; i++)
		tmpcrc = ((tmpcrc >> 8) & 0xFF) ^ crctab[(tmpcrc ^ *data++) & 0xFF];

	crc[0]=(tmpcrc >> 8) & 0xFF;
	crc[1]=tmpcrc & 0xFF;
}

/* Appends RC */

int scT1AppendRc( SC_T1_INFO *t1, BYTE *data, int *datalen )
{
	if( t1->rc==SC_T1_CHECKSUM_LRC ) {
		data[*datalen]=scT1Lrc( data, *datalen );
		*datalen+=1;
		return( SC_EXIT_OK );
	} else if( t1->rc==SC_T1_CHECKSUM_CRC) {
		scT1Crc( data, *datalen, data+*datalen );
		*datalen+=2;
		return( SC_EXIT_OK );
	}

	return SC_EXIT_BAD_PARAM;
}

/* Checks RC. */

BOOLEAN scT1CheckRc( SC_T1_INFO *t1, const BYTE *data, int datalen )
{
	BYTE rc[2];
	BYTE cmp[2];

	if( t1->rc==SC_T1_CHECKSUM_LRC ) {
		/* Check LEN. */
		if( (data[2]+3+1)!=datalen ) {
			return FALSE;
		}

		rc[1]=data[datalen-1];
		cmp[1] = scT1Lrc( data, datalen-1 );
		if( rc[1]==cmp[1] ) {
			return TRUE;
		}

		return FALSE;
	} else if( t1->rc==SC_T1_CHECKSUM_CRC) {
		/* Check LEN. */
		if( (data[2]+3+2)!=datalen ) {
			return FALSE;
		}

		scT1Crc( data, datalen-2, cmp );
		if( memcmp( data+datalen-2, cmp, 2 )==0 ) return( TRUE );
		return FALSE;
	}

	return FALSE;
}

/* Builds S-Block */

int scT1SBlock( SC_T1_INFO *t1, int type, int dir, int param, BYTE *block,
	int *len )
{
	int ret;

	block[0]=t1->nad;

	switch( type ) {
		case SC_T1_S_RESYNCH:
			if( dir==SC_T1_S_REQUEST ) block[1]=0xC0;
			else block[1]=0xE0;
			block[2]=0x00;
			*len=3;
			break;

		case SC_T1_S_IFS:
			if( dir==SC_T1_S_REQUEST ) block[1]=0xC1;
			else block[1]=0xE1;
			block[2]=0x01;
			block[3]=(BYTE) param;
			*len=4;
			break;

		case SC_T1_S_ABORT:
			if( dir==SC_T1_S_REQUEST ) block[1]=0xC2;
			else block[1]=0xE2;
			block[2]=0x00;
			*len=3;
			break;

		case SC_T1_S_WTX:
			if( dir==SC_T1_S_REQUEST ) block[1]=0xC3;
			else block[1]=0xE3;
			block[2]=0x01;
			block[3]=(BYTE) param;
			*len=4;
			break;

		default:
			return( SC_EXIT_BAD_PARAM );
	}
	ret=scT1AppendRc( t1, block, len );
	if( ret ) {
		return ret;
	}

	return( SC_EXIT_OK );
}

/* Builds R-Block */

int scT1RBlock( SC_T1_INFO *t1, int type, BYTE *block, int *len )
{
	int ret;

	block[0]=t1->nad;
	block[2]=0x00;

	switch( type ) {
		case SC_T1_R_OK:
			if( t1->nr ) block[1]=0x90;
			else block[1]=0x80;
			break;

		case SC_T1_R_EDC_ERROR:
			if( t1->nr ) block[1]=0x91;
			else block[1]=0x81;
			break;

		case SC_T1_R_OTHER_ERROR:
			if( t1->nr ) block[1]=0x92;
			else block[1]=0x82;
			break;

		default:
			return( SC_EXIT_BAD_PARAM );
	}

	*len=3;
	ret=scT1AppendRc( t1, block, len );
	if( ret ) {
		return ret;
	}

	return SC_EXIT_OK;
}

/* Builds I-Block */

int scT1IBlock( SC_T1_INFO *t1, BOOLEAN more, const BYTE *data, int datalen,
	BYTE *block, int *blocklen )
{
	int ret;

	block[0]=t1->nad;

	block[1]=0x00;
	if( t1->ns ) block[1]|=0x40;
	if( more ) block[1]|=0x20;

	if( datalen>t1->ifsc ) {
		return SC_EXIT_BAD_PARAM;
	}
	block[2]=(BYTE) datalen;

	memcpy( block+3, data, datalen );

	*blocklen=datalen+3;
	ret=scT1AppendRc( t1, block, blocklen );
	if( ret ) {
		return ret;
	}

	return SC_EXIT_OK;
}

/* Returns N(R) or N(S) from R/I-Block. */

int scT1GetN( const BYTE *block )
{
	/* R-Block */
	if( (block[1]&0xC0)==0x80 ) {
		return( (block[1]>>4)&0x01 );
	}

	/* I-Block */
	if( (block[1]&0x80)==0x00 ) {
		return( (block[1]>>6)&0x01 );
	}

	return 0;
}

int scT1SendRecvBlock( SC_READER_INFO *ri, SC_CARD_INFO *ci, SC_T1_INFO *t1,
				BYTE *block, int blocklen, BYTE *rblock, int *rblocklen)
{
	int ret;

#ifdef READER_DEBUG
	printf(" [OUT:");
	for (ret=0;ret<blocklen;ret++) {
			printf(" %.2X",block[ret]);
	}
	printf("]\n");
#endif

	/* Get unwanted bytes */
	while(scReaderReadChar( ri, ci, SC_TIMEOUT_DEFAULT )!=-1);

	ret = scReaderWriteBuffer( ri, ci, block, blocklen );
	if( ret != blocklen )
		return SC_EXIT_IO_ERROR;

	/* Wait BWT. */
	scReaderWaitForData( ri, ci, t1->bwt );

	ret=scReaderReadBuffer( ri, ci, rblock,
		*rblocklen, ri->etu * t1->cwt );
	if( ret==-1 ) {
		return SC_EXIT_IO_ERROR;
	}
	*rblocklen=ret;

#ifdef READER_DEBUG
	printf(" [IN:");
	for (ret=0;ret<rblocklen;ret++)
			printf(" %.2X",rblock[ret]);
	printf("]\n");
#endif


	return SC_EXIT_OK;
}

/* Change IFSD. */

int scT1ChangeIFSD( SC_READER_INFO *ri, SC_CARD_INFO *ci, BYTE ifsd )
{
	SC_T1_INFO *t1;

	BYTE block[ SC_T1_MAX_SBLKLEN ];
	int blocklen;

	BYTE rblock[ SC_T1_MAX_SBLKLEN ];
	int rblocklen;

	BOOLEAN success=FALSE;
	int errors=0;
	int ret;

#ifdef WITH_B1
	if( ri->major==SC_READER_B1 ) t1=&ri->t1;
	else t1=&ci->t1;
#else /* !WITH_B1 */
	t1=&ci->t1;
#endif /* WITH_B1 */

	ret = scT1SBlock(t1, SC_T1_S_IFS, SC_T1_S_REQUEST, ifsd, block, &blocklen);
	if( ret != SC_EXIT_OK ) {
		return ret;
	}

	while( !success ) {
		rblocklen=sizeof(rblock);
		ret = scT1SendRecvBlock( ri, ci, t1, block, blocklen,
						rblock, &rblocklen);
		if(ret != SC_EXIT_OK) {
			return ret;
		}

		if( (rblocklen==blocklen) && (rblock[1]==0xE1) &&
			scT1CheckRc( t1, rblock, rblocklen ) ) {
			t1->ifsreq=TRUE;
			t1->ifsd=rblock[3];
			success=TRUE;
		} else {
			errors++;
		}

		if( errors>2 ) {
			t1->ifsreq=TRUE;
			/* Easy exit. If there is more wrong the following
			 * communication will show it.
			 */
			success=TRUE;
		}
	}

	return SC_EXIT_OK;
}

/* Transmit APDU with protocol T=1 */

/* Supports only cases 1, 2 Short, 3 Short, 4 Short.
 * Does not do the full Resync/Reset game but exits very early with
 * an error.
 * Inverse convention is handled by the Read/Write functions.
 */

/* RC error results in a printf("[x]); */

int scT1SendCmd( SC_READER_INFO *ri, SC_CARD_INFO *ci, SC_APDU *apdu )
{
	SC_T1_INFO *t1;

	int sendptr=0;	/* Points to begining of unsent data. */
	int sendlen;

	BYTE block[ SC_T1_MAX_BLKLEN ];
	BYTE rblock[ SC_T1_MAX_BLKLEN ];
	int blocklen;
	int rblocklen;

	BYTE rsp[ SC_GENERAL_SHORT_DATA_SIZE+3 ];
	int rsplen=0;

#ifdef RESPECTS_LE
	int maxlen=apdu->cmd[apdu->cmdlen-1];
#endif
	int cpylen;

	BOOLEAN more=TRUE;		/* More data to send. */
	BOOLEAN lastiicc=FALSE;	/* It's ICCs turn to send I-Blocks. */

	int ret;
	int wtxcntr=0;
	int timeouts=0;
	int errcntr=0;
	int rerrcntr=0;

#ifndef NO_APDU_CHECK
	ret=scReaderCheckAPDU( apdu, FALSE );
	if(ret!=SC_EXIT_OK ) return( ret );
#endif /* NO_APDU_CHECK */

#ifdef RESPECTS_LE
	/* If le=0 le is really 256. */
	if( !maxlen ) maxlen=256;
#endif /* RESPECTS_LE */

#ifdef WITH_B1
	if( ri->major==SC_READER_B1 ) t1=&ri->t1;
	else t1=&ci->t1;
#else /* !WITH_B1 */
	t1=&ci->t1;
#endif /* WITH_B1 */

	/* Get unwanted bytes */
	while(scReaderReadChar( ri, ci, SC_TIMEOUT_DEFAULT )!=-1);

	/* Check card status */
	ret=scReaderCardStatus( ri );
	if( ret != SC_EXIT_OK ) return( ret );
	if( ri->status & SC_CARD_STATUS_CHANGED ) return( SC_EXIT_CARD_CHANGED );
	if( !(ri->status & SC_CARD_STATUS_PRESENT) ) return( SC_EXIT_NO_CARD );

	apdu->rsplen=0;

	/* Change IFSD if not allready changed. */
	if( !t1->ifsreq ) {
		ret=scT1ChangeIFSD( ri, ci, 0xFE );
		if( ret ) {
			return ret;
		}
	}

	sendlen=min( apdu->cmdlen-sendptr, t1->ifsc );
	if( sendlen==(apdu->cmdlen-sendptr) ) more=FALSE;
	ret = scT1IBlock( t1, more, apdu->cmd, sendlen, block, &blocklen );
	if( ret != SC_EXIT_OK ) {
		memset( block, 0, sizeof(block) );
		return( ret );
	}
	sendptr+=sendlen;

	while( TRUE ) {
		rblocklen=sizeof(rblock);
		ret = scT1SendRecvBlock( ri, ci, t1, block, blocklen, rblock,
				&rblocklen);

		/* communication errors? retry at most three times */
		if( ret ) {
			timeouts++;

			if( timeouts>3 ) {
				ret=SC_EXIT_IO_ERROR;
				goto finish;
			}

			ret=scT1RBlock( t1, SC_T1_R_OTHER_ERROR, block,
					&blocklen);
			if( ret ) {
				goto finish;
			}
			continue;
		} 
	
		timeouts=0;

		scReaderDelay( ri->etu * 11 );

		/* length or rc errors? retry at most three times */
		if( !scT1CheckRc( t1, rblock, rblocklen ) ) {
#ifdef READER_DEBUG
			printf("[R]");
#endif
			errcntr++;

			if( errcntr>3 ) {
				ret=SC_EXIT_IO_ERROR;
				goto finish;
			}

			ret=scT1RBlock( t1, SC_T1_R_EDC_ERROR, block,
					&blocklen);
			if( ret !=SC_EXIT_OK ) {
				goto finish;
			}
			continue;
		}
		errcntr=0;

		/* R-Block */
		if( (rblock[1]&0xC0)==0x80 ) {
#ifdef READER_DEBUG
			printf("[R]");
#endif
			rerrcntr++;

			if( rerrcntr>3 ) {
				ret = SC_EXIT_IO_ERROR;
				goto finish;
			}

			if( lastiicc ) {
				/* Card is sending I-Blocks, so send R-Block. */
				ret=scT1RBlock( t1, SC_T1_R_OK, block, &blocklen );
				if( ret != SC_EXIT_OK ) {
					goto finish;
				}
				continue;
			} 

			if( scT1GetN( rblock )==t1->ns ) {
				/* N(R) is old N(S), so resend I-Block. */
				sendptr-=sendlen;
			} else {
				/* N(R) is next N(S),
				 * so make next I-Block and send it. */

				/* Check if data available. */
				if( more==FALSE ) {
					ret = SC_EXIT_PROTOCOL_ERROR;
					goto finish;
				}

				/* Change N(S) to new value. */
				t1->ns^=1;

				/* Reset also here the error counter */
				rerrcntr=0;
			}
			/* Make next I-Block. */
			sendlen=min( apdu->cmdlen-sendptr, t1->ifsc );
			if( sendlen==(apdu->cmdlen-sendptr) ) more=FALSE;
			ret = scT1IBlock( t1, more, apdu->cmd+sendptr,
				sendlen, block, &blocklen );
			if( ret != SC_EXIT_OK ) {
				goto finish;
			}
			sendptr+=sendlen;

			continue;
		}

		/* Reset rerrcntr, because when it is here it had not received
		 * an R-Block.
		 */
		rerrcntr=0;

		/* I-Block */
		if( (rblock[1]&0x80)==0x00 ) {
#ifdef READER_DEBUG
				printf("[I]\n");
#endif
				if( !lastiicc ) {
						/* Change N(S) to new value. */
						t1->ns^=1;
				}

				lastiicc=TRUE;

				if( scT1GetN( rblock )!=t1->nr ) {
						/* Card is sending wrong I-Block,
						 * so send R-Block. */
						ret=scT1RBlock( t1, SC_T1_R_OTHER_ERROR, block,
										&blocklen );
						if( ret != SC_EXIT_OK ) {
								goto finish;
						}
						continue;
				}

				/* Copy data. */
				if( rblock[2]>(SC_GENERAL_SHORT_DATA_SIZE+2-rsplen) ) {
						ret=SC_EXIT_PROTOCOL_ERROR;
						goto finish;
				}
				memcpy( rsp+rsplen, rblock+3, rblock[2] );
				rsplen+=rblock[2];

				if( (rblock[1]>>5) & 1 ) {
						/* More data available. */

						/* Change N(R) to new value. */
						t1->nr^=1;

						/* Send R-Block. */
						ret=scT1RBlock( t1, SC_T1_R_OK, block,
										&blocklen );
						if( ret != SC_EXIT_OK ) {
								goto finish;
						}

						continue;
				} 

				/* Last block. */

				/* Change N(R) to new value. */
				t1->nr^=1;

				if( rsplen<2 ) {
						ret=SC_EXIT_BAD_SW;
						goto finish;
				}
#ifndef NO_APDU_CHECK
				if( (apdu->cse==SC_APDU_CASE_2_SHORT) ||
								(apdu->cse==SC_APDU_CASE_4_SHORT) ) {
#else
						if( TRUE ) {
#endif /* NO_APDU_CHECK */
								/* Copy response and SW. */
#ifdef RESPECTS_LE
								cpylen = min( rsplen-2, maxlen );
#else
								cpylen = min( rsplen-2, SC_GENERAL_SHORT_DATA_SIZE );
#endif /* RESPECTS_LE */
								memcpy( apdu->rsp, rsp, cpylen );
								memcpy( apdu->rsp+cpylen, rsp+rsplen-2, 2 );
								apdu->rsplen=cpylen+2;
						} else {
								/* Copy only SW. */
								memcpy( apdu->rsp, rsp+rsplen-2, 2 );
								apdu->rsplen=2;
						}

						/* wow, we are done ! */
						ret = SC_EXIT_OK;
						goto finish;
				}

				/* S-Block IFS Request */
				if( rblock[1]==0xC1 ) {
#ifdef READER_DEBUG
						printf("[F]\n");
#endif

			t1->ifsc=rblock[3];
			ret=scT1SBlock( t1, SC_T1_S_IFS, SC_T1_S_RESPONSE,
					rblock[3], block, &blocklen );
			if( ret !=SC_EXIT_OK ) {
				goto finish;
			}
			continue;
		}

		/* S-Block ABORT Request */
		if( rblock[1]==0xC2 ) {
#ifdef READER_DEBUG
			printf("[A]\n");
#endif
			ret=scT1SBlock( t1, SC_T1_S_ABORT, SC_T1_S_RESPONSE,
					0x00, block, &blocklen );
			if( ret !=SC_EXIT_OK ) {
				goto finish;
			}

			/* Get next block to get sending rights back. (This only helps,
			 * when the card sends an R(X)) Better to return to the main loop?
			 * I really don't know, why a card sends an ABORT block.
			 * I would have made sense, when a card sends after an ABORT
			 * of IFS I-Blocks a SW in an I-Block, but in all examples of
			 * ISO7816-4 Amd.1 the sender of the aborted chain begins with
			 * sending a new (chain of) I-Block(s). So when the ICC ABORTS
			 * the IFD it just aborts this command without any status?
			 * And when the ICC aborts itself it begins another time with
			 * its response?
			 */
	
			rblocklen=sizeof(rblock);
			ret = scT1SendRecvBlock( ri, ci, t1, block, blocklen,
					rblock, &rblocklen);
			if( ret ) {
				ret=SC_EXIT_IO_ERROR;
				goto finish;
			}

			ret = SC_EXIT_UNKNOWN_ERROR;
			goto finish;
		}

		/* S-Block WTX Request */
		if( rblock[1]==0xC3 ) {
#ifdef READER_DEBUG
			printf("[W]");
#endif
			wtxcntr+=rblock[3];

			if( ri->scWaitReq!=NULL ) {
				ret=ri->scWaitReq( ri, ci, wtxcntr );
				if( ret!=SC_EXIT_OK ) {
					goto finish;
				}
			} else if( wtxcntr>200 ) {
				/* 200*BWT has to be enough. This is normally over 5 minutes. */
				ret=SC_EXIT_PROTOCOL_ERROR;
				goto finish;
			}

			ret=scT1SBlock( t1, SC_T1_S_WTX, SC_T1_S_RESPONSE, rblock[3],
				block, &blocklen );
			if( ret !=SC_EXIT_OK ) {
				goto finish;
			}
			continue;
		}

	}

	/* Ooops! Should never be here. */
	ret = SC_EXIT_UNKNOWN_ERROR;

finish:
	memset( block, 0, sizeof(block) );
	memset( rblock, 0, sizeof(rblock) );
	memset( rsp, 0, sizeof(rsp) );
	return( ret );
}


