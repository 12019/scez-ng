/****************************************************************************
*																			*
*			SCEZ chipcard library - Blue Dot Connector routines				*
*					Copyright Matthias Bruestle 2001						*
*					For licensing, see the file COPYRIGHT					*
*																			*
****************************************************************************/

/* $Id: scbluedot.h 1119 2005-05-08 13:28:34Z laforge $ */

#ifndef SC_BLUEDOT_H
#define SC_BLUEDOT_H

#include <scez/scgeneral.h>

/* Reader minor types */

#define SC_BLUEDOT_BLUEDOT	0x00

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* Initialize reader */
int scBluedotInit( SC_READER_INFO *ri, const char *param );

/* Shutdown reader */
int scBluedotShutdown( SC_READER_INFO *ri );

/* Detect reader */
int scBluedotDetect( SC_READER_DETECT_INFO *rdi );

/* Activate card */
int scBluedotActivate( SC_READER_INFO *ri );

/* Deactivate card */
int scBluedotDeactivate( SC_READER_INFO *ri );

/* Get card status */
int scBluedotCardStatus( SC_READER_INFO *ri );

/* Reset card and read ATR */
int scBluedotResetCard( SC_READER_INFO *ri, SC_CARD_INFO *ci );

/* Transmit APDU */
int scBluedotSendAPDU( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	SC_APDU *apdu );

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* SC_BLUEDOT_H */


