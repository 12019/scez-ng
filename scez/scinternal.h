/****************************************************************************
*																			*
*					SCEZ chipcard library - Internal defines				*
*						Copyright Matthias Bruestle 2001					*
*					For licensing, see the file COPYRIGHT					*
*																			*
****************************************************************************/

/* $Id: scinternal.h 1098 2002-06-21 14:34:56Z zwiebeltu $ */

#ifndef SC_INTERNAL_H
#define SC_INTERNAL_H

#if HAVE_CONFIG_H
#include <config.h>
#endif

/* Included card drivers. */

#define WITH_BASICCARD
#define WITH_CARTEB
#define WITH_CRYPTOFLEX
#define WITH_CYBERFLEX
/* #define WITH_CYBERFLEX2 */
#define WITH_GELDKARTE
/* #define WITH_GEMXPRESSO */
/* #define WITH_GPK2000 */
#define WITH_GPK4000
#define WITH_GPK8000
#define WITH_GSMSIM
#define WITH_JIB
#define WITH_MFC
/* #define WITH_MPCOS */
#define WITH_MULTIFLEX
#define WITH_OPENPLATFORM
/* #define WITH_PAYFLEX */
#define WITH_PROTON
#define WITH_QUICK
#define WITH_SMARTCAFE
#define WITH_STARCOS
#define WITH_TCOS

/* Included reader drivers. */

#define WITH_ACR20
/* #define WITH_B1 */
/* #define WITH_BLUEDOT */
#define WITH_CTAPI
#define WITH_DUMBMOUSE
#define WITH_EASYCHECK
#define WITH_GCR400
#define WITH_GPR400
#define WITH_INTERTEX
/* #define WITH_REFLEX20 */
#define WITH_REFLEX60
#define WITH_TOWITOKO

/* Undefines if unsupported on this platform. */

#ifdef __palmos__
#ifdef WITH_ACR20
#undef WITH_ACR20
#endif
#ifdef WITH_B1
#undef WITH_B1
#endif
#ifdef WITH_BLUEDOT
#undef WITH_BLUEDOT
#endif
#ifdef WITH_CTAPI
#undef WITH_CTAPI
#endif
#ifdef WITH_GCR400
#undef WITH_GCR400
#endif
#ifdef WITH_GPR400
#undef WITH_GPR400
#endif
#ifdef WITH_INTERTEX
#undef WITH_INTERTEX
#endif
#ifdef WITH_REFLEX20
#undef WITH_REFLEX20
#endif
#ifdef WITH_REFLEX60
#undef WITH_REFLEX60
#endif
#ifdef memcpy
#undef memcpy
#endif /* memcpy */
#define memcpy(x,y,z) (MemMove(x,(VoidPtr)y,z))
#define memset(x,y,z) (MemSet(x,z,y))
#define memcmp(x,y,z) (MemCmp(x,y,z))
#endif /* __palmos__ */

#ifdef __BORLANDC__
#ifdef WITH_DUMBMOUSE	/* Because of buggy SIO. */
#undef WITH_DUMBMOUSE
#endif
#ifdef WITH_TOWITOKO	/* Because of buggy SIO. */
#undef WITH_TOWITOKO
#endif
#endif /* __BORLANDC__ */

#define printarray( name, length, array ); \
    printf(name); \
    for( i=0; i<length; i++ ) printf(" %.2X",array[i]); \
    printf("\n");

#if defined( HAVE_LIBCRYPT )
#include <misc/sio.h>
#elif defined( __palmos__ )
#include <sio/siolight.h>
#else
#include <sio/sio.h>
#endif /* HAVE_LIBCRYPT */

#include <scez/scgeneral.h>
#include <scez/scmacros.h>

#endif /* SC_INTERNAL_H */

