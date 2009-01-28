/****************************************************************************
*																			*
*						Copyright Matthias Bruestle 1999					*
*					For licensing, see the file COPYRIGHT					*
*																			*
****************************************************************************/

/* $Id: gendivkey.c 1069 2002-01-25 11:27:03Z zwiebeltu $ */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
/* #include <unistd.h> */
#include <scez/scgeneral.h>

#ifdef HAVE_CONFIG_H
#include <config.h>
#if defined( HAVE_LIBDES )
#include <des.h>
#elif defined ( HAVE_LIBCRYPT )
#include <crypt/des.h>
#elif defined ( HAVE_LIBCRYPTO )
#include <openssl/des.h>
#endif /* HAVE_LIBDES */
#else
#include <des.h.>
#endif

#define printarray( name, length, array ); \
	printf(name); \
	for( i=0; i<length; i++ ) printf(" %.2X",array[i]); \
	printf("\n");

int main( int argc, char *argv[] )
{
	int ret;
	int i;

	BYTE masterkey[ 8 ];
	BYTE sn[ 8 ];
	int scanbyte[8];
	char string[20];

	des_key_schedule schedule;
	BYTE out[ 8 ];

	printf("Master Key (XXXXXXXXXXXXXXXX): ");
	if( !fgets( string, 20, stdin ) ) exit(2);
	ret=sscanf(string,"%2x%2x%2x%2x%2x%2x%2x%2x",&scanbyte[0],&scanbyte[1],
		&scanbyte[2],&scanbyte[3],&scanbyte[4],&scanbyte[5],&scanbyte[6],
		&scanbyte[7]);
	if( (ret!=8) && (ret!=-1) ) { printf("Key to short.\n"); exit(1); }
	if( ret==8 ) for( i=0; i<8; i++ ) masterkey[i]=scanbyte[i]&0xFF;
	printf("Master Key is:");
	for( i=0; i<8; i++ ) printf(" %.2X",masterkey[i]);
	printf("\n\n");

	printf("Serial Number (XXXXXXXXXXXXXXXX): ");
	if( !fgets( string, 20, stdin ) ) exit(2);
	ret=sscanf(string,"%2x%2x%2x%2x%2x%2x%2x%2x",&scanbyte[0],&scanbyte[1],
		&scanbyte[2],&scanbyte[3],&scanbyte[4],&scanbyte[5],&scanbyte[6],
		&scanbyte[7]);
	if( (ret!=8) && (ret!=-1) ) { printf("Key to short.\n"); exit(1); }
	if( ret==8 ) for( i=0; i<8; i++ ) sn[i]=scanbyte[i]&0xFF;
	printf("Serial Number is:");
	for( i=0; i<8; i++ ) printf(" %.2X",sn[i]);
	printf("\n\n");

	des_check_key=0;
	des_set_key( (des_cblock *) masterkey, schedule );

	des_ecb_encrypt( (des_cblock *) sn, (des_cblock *)out, schedule,
		DES_ENCRYPT );

	printarray( "Diversified Key:", 8, out );

	return( 0 );
}

