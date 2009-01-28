/****************************************************************************
*																			*
*					SCEZ chipcard library - Starcos routines				*
*						Copyright Regis Gaschy 2000							*
*			This file is only available under the BSD2 license.				*
*																			*
****************************************************************************/

#include <scez/scinternal.h>
#include <scez/cards/scstarcos.h>

#include <stdio.h>
#include <string.h>

#if !defined(WINDOWS) && !defined(__palmos__)
#include <unistd.h>	/* for sleep */
#elif defined(__BORLANDC__)
#include <dos.h>	/* for sleep */
#elif defined(WINDOWS)
#include <windows.h>
#elif defined(__palmos__)
#define sleep(x) SysTaskDelay(x*sysTicksPerSecond)
/* #define memcpy(x,y,z) (MemMove(x,(VoidPtr)y,z) ? x : x) */
#endif

#if defined( HAVE_LIBDES )
#include <des.h>
#elif defined ( HAVE_LIBCRYPT )
#include <crypt/des.h>
#elif defined ( HAVE_LIBCRYPTO )
#include <openssl/des.h>
#endif /* HAVE_LIBDES */

/* Initialize card function pointer */

int scStarcosInit( SC_CARD_INFO *ci )
{
	ci->scGetCap=scStarcosGetCap;
	ci->scGetCardData=scStarcosGetCardData;
	ci->scSetFD=scStarcosSetFD;

	return( SC_EXIT_OK );
}

/* Capabilities */

int scStarcosGetCap( SC_CARD_INFO *ci, SC_CARD_CAP *cp )
{
  cp->n_fd=5;

  /* 9600 at 3.5712MHz */
  cp->fd[0]=(((10L<<16)+372L)<<8)+1;
  
  /* 56000 at 3.5712MHz */
  cp->fd[1]=(((56L<<16)+64L)<<8)+4;
  
  /* 38400 at 3.5712MHz */
  cp->fd[2]=(((116L<<16)+31L)<<8)+8;
  
  return( SC_EXIT_OK );
}

/* Fill card data in ci */

int scStarcosGetCardData( SC_CARD_INFO *ci )
{
  BYTE header[] = { 0x00, 0xC0, 0x00, 0x00, 0x00 }; /* Get Response cmd */
  BYTE swok[] = { 0x01, 0x90 };
  BYTE swav[] = { 0x01, 0x61 };
  int ret;

  if( (ret=scSmartcardProcessATR( ci ))!=SC_EXIT_OK ) return( ret );

  memcpy( ci->t0.getrsp, header, 5 );
  memcpy( ci->swok, swok, sizeof(swok) );
  memcpy( ci->swav, swav, sizeof(swav) );
  ci->memsize=0;

  return( SC_EXIT_OK );
}

/* Set F and D. */

int scStarcosSetFD( SC_READER_INFO *ri, SC_CARD_INFO *ci, LONG fd )
{
  if (ci->protocol == SC_PROTOCOL_T0) {
    switch( fd&0xFFFFFF ) {
    case (372L<<8)+1:
      return( scReaderPTS( ri, ci, (BYTE *)"\xFF\x10\x11\xFE", 4 ) );
    case (64L<<8)+4:
      return( scReaderPTS( ri, ci, (BYTE *)"\xFF\x10\x94\x7B", 4 ) );
    case (31L<<8)+8:
      return( scReaderPTS( ri, ci, (BYTE *)"\xFF\x10\x18\xF7", 4 ) );
    default:
      return( SC_EXIT_BAD_PARAM );
    }
  }
  else if (ci->protocol == SC_PROTOCOL_T1) {
    switch( fd&0xFFFFFF ) {
    case (372L<<8)+1:
      return( scReaderPTS( ri, ci, (BYTE *)"\xFF\x11\x11\xFE", 4 ) );
    case (64L<<8)+4:
      return( scReaderPTS( ri, ci, (BYTE *)"\xFF\x11\x94\x7B", 4 ) );
    case (31L<<8)+8:
      return( scReaderPTS( ri, ci, (BYTE *)"\xFF\x11\x18\xF7", 4 ) );
    default:
      return( SC_EXIT_BAD_PARAM );
    }
  }
  else
    return( SC_EXIT_BAD_PARAM );
}


