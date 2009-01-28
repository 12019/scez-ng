/****************************************************************************
*																			*
*					SCEZ chipcard library - PTS routines					*
*					Copyright Matthias Bruestle 1999,2000					*
*					For licensing, see the file COPYRIGHT					*
*																			*
****************************************************************************/

/* $Id: scpts.c 1056 2001-09-17 23:20:37Z m $ */

#include <scez/scinternal.h>
#include <scez/scpts.h>

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/* Adjustes ptslen for PCK. */
int scPtsCalcPCK( BYTE *pts, int *ptslen )
{
	int i;
	BYTE pck=0;

	for( i=0; i<*ptslen; i++ ) pck^=pts[i];

	pts[i]=pck;
	(*ptslen)++;

	return( SC_EXIT_OK );
}

BOOLEAN scPtsCheckPCK( const BYTE *pts, int ptslen )
{
	int i;
	BYTE xor=0;

	for( i=0; i<ptslen; i++ ) xor^=pts[i];

	if( xor==0 ) return( TRUE );

	return( FALSE );
}

/* Do a PTS handshake. */

int scPtsDoPTS( SC_READER_INFO *ri, SC_CARD_INFO *ci, const BYTE *pts,
	int ptslen )
{
	int ret;
	BYTE buffer[6];

	if( ptslen>6 ) return( SC_EXIT_BAD_PARAM );

	while(scReaderReadChar( ri, ci, SC_TIMEOUT_DEFAULT )!=-1);

	/* Check card status */
	if( (ret = scReaderCardStatus( ri ))!=SC_EXIT_OK ) return( ret );
	if( ri->status & SC_CARD_STATUS_CHANGED ) return( SC_EXIT_CARD_CHANGED );
	if( !(ri->status & SC_CARD_STATUS_PRESENT) ) return( SC_EXIT_NO_CARD );

	if( scReaderWriteBuffer( ri, ci, pts, ptslen ) != ptslen )
		return( SC_EXIT_IO_ERROR );

	scReaderWaitForData( ri, ci, ri->etu * 9600 );

	/* Receive data from card */
	if( (ret=scReaderReadBuffer( ri, ci, buffer, ptslen, ri->etu * 9600 ))
		!=ptslen ) {
		if( ret==-1 ) return( SC_EXIT_TIMEOUT );
		return( SC_EXIT_PROTOCOL_ERROR );
	}

	if( memcmp( pts, buffer, ptslen )!=0 ) return( SC_EXIT_PROTOCOL_ERROR );

	return( SC_EXIT_OK );
}


