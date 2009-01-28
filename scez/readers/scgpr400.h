/****************************************************************************
*																			*
*					SCEZ chipcard library - GPR 400 routines				*
*						Copyright Matthias Bruestle 2001					*
*					For licensing, see the file COPYRIGHT					*
*																			*
****************************************************************************/

/* $Id: scgpr400.h 1119 2005-05-08 13:28:34Z laforge $ */

#ifndef SC_GPR400_H
#define SC_GPR400_H

#include <scez/scgeneral.h>

#define SC_GPR400_MAX_BUFFER_SIZE	SC_GENERAL_SHORT_DATA_SIZE

/* Reader minor types */

#define SC_GPR400_GPR400	0x00

/* Parameter for IOCTL */
typedef struct sc_gpr400_ioctl_param_tlv {
	unsigned char t;
	unsigned short l;
	unsigned char v[300];	/* It really can do only 256 bytes, but
							   the PCCard driver uses this struct
							   as parameter. */
} SC_GPR400_IOCTL_PARAM_TLV;

#define SC_GPR400_IOCTL_RESET	_IO('g', 0x01)
#define SC_GPR400_IOCTL_TLV		_IOWR('g', 0x0a, SC_GPR400_IOCTL_PARAM_TLV)

#define SC_GPR400_TAG_CLOSE_SESSION	0x10
#define SC_GPR400_TAG_OPEN_SESSION	0x20
#define SC_GPR400_TAG_APDU_EXCHANGE	0x30
#define SC_GPR400_TAG_POWER_DOWN	0x40
#define SC_GPR400_TAG_SELECT_CARD	0x50
#define SC_GPR400_TAG_STATUS		0xA0

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* Initialize reader */
int scGpr400Init( SC_READER_INFO *ri, const char *param );

/* Shutdown reader */
int scGpr400Shutdown( SC_READER_INFO *ri );

/* Get Capabilities */
int scGpr400GetCap( SC_READER_INFO *ri, SC_READER_CAP *rp );

/* Activate card */
int scGpr400Activate( SC_READER_INFO *ri );

/* Deactivate card */
int scGpr400Deactivate( SC_READER_INFO *ri );

/* Write Buffer Async */
int scGpr400WriteBuffer( SC_READER_INFO *ri, SC_CARD_INFO *ci,
    const BYTE *buffer, int len );

/* Read Buffer Async */
int scGpr400ReadBuffer( SC_READER_INFO *ri, SC_CARD_INFO *ci,
    BYTE *buffer, int len , LONG timeout );

/* Wait For Data */
int scGpr400WaitForData( SC_READER_INFO *ri, SC_CARD_INFO *ci, LONG timeout );

/* Get card status */
int scGpr400CardStatus( SC_READER_INFO *ri );

/* Reset card and read ATR */
int scGpr400ResetCard( SC_READER_INFO *ri, SC_CARD_INFO *ci );

/* Transmit APDU with protocol T=0 */
int scGpr400T0( SC_READER_INFO *ri, SC_CARD_INFO *ci, SC_APDU *apdu );

/* Transmit APDU with protocol T=1 */
int scGpr400T1( SC_READER_INFO *ri, SC_CARD_INFO *ci, SC_APDU *apdu );

/* Transmit APDU */
int scGpr400SendAPDU( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	SC_APDU *apdu );

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* SC_GPR400_H */


