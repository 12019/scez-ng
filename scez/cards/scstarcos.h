/****************************************************************************
*																			*
*					SCEZ chipcard library - Starcos routines				*
*						Copyright Matthias Bruestle 1999					*
*																			*
****************************************************************************/

#ifndef SC_STARCOS_H
#define SC_STARCOS_H

#include <scez/scgeneral.h>

/* Initialize card function pointer */
int scStarcosInit( SC_CARD_INFO *ci );

/* Capabilities */
int scStarcosGetCap( SC_CARD_INFO *ci, SC_CARD_CAP *cp );

/* Fill card data in ci */
int scStarcosGetCardData( SC_CARD_INFO *ci );

/* Set F and D. */
int scStarcosSetFD( SC_READER_INFO *ri, SC_CARD_INFO *ci, LONG fd );

#endif
