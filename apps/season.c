/****************************************************************************
*																			*
*					Copyright Matthias Bruestle 2002						*
*					For licensing, see the file COPYRIGHT					*
*																			*
****************************************************************************/

/* $Id: season.c 1089 2002-03-08 12:37:53Z zwiebeltu $ */

#include <stdio.h>
#include <sys/time.h>
#include <sio/sio.h>

int main( int argc, char *argv[] )
{
	struct timeval tv;
	long startsec;
	SIO_INFO *si;
	int ch;

	si = SIO_Open( "/dev/ttyS0" );

	SIO_SetSpeed( si, 9600 );
	SIO_SetDataBits( si, 8 );
	SIO_SetParity( si, SIO_PARITY_EVEN );
	SIO_SetStopBits( si, 2 );
	SIO_SetIOMode( si, SIO_IOMODE_DIRECT );
	SIO_WriteSettings( si );

	SIO_SetReadTimeout( si, SIO_READ_WAIT_FOREVER );

	gettimeofday( &tv, NULL );
	startsec=tv.tv_sec;

	for(;;) {
		ch = SIO_ReadChar( si );
		gettimeofday( &tv, NULL );

		printf( "%d.%.6d: %.2X\n", tv.tv_sec-startsec, tv.tv_usec, ch );
	}
}

