/****************************************************************************
*																			*
*					SCEZ chipcard library - BasicCard routines				*
*						Copyright Matthias Bruestle 1999					*
*					For licensing, see the file COPYRIGHT					*
*																			*
****************************************************************************/

/* $Id: scbasiccard.h 1119 2005-05-08 13:28:34Z laforge $ */

#ifndef SC_BASICCARD_H
#define SC_BASICCARD_H

/* Key format
 * SG-LFSR: PolyA|PolyS|Key[8]
 * DES:     Key[8]|Null[8]
 * 3DES:    Key[16]
 *
 * IV format
 * SG-LFSR: A|S
 * DES/3DES: (BYTE)[8]
 *
 * PolyA, PolyS, A and S is converted by StartEncr into the machine
 * byte order.
 */

#include <scez/scgeneral.h>

/* Usefull defines for command parameters */

/* State of card */
#define	SC_BASICCARD_STATE_NEW		0x00
#define	SC_BASICCARD_STATE_LOAD		0x01
#define	SC_BASICCARD_STATE_TEST		0x02
#define	SC_BASICCARD_STATE_RUN		0x03

/* Encryption algorithm */
#define	SC_BASICCARD_ALGO_LFSR		0x11
#define	SC_BASICCARD_ALGO_LFSR_CRC	0x12
#define	SC_BASICCARD_ALGO_DES		0x21
#define	SC_BASICCARD_ALGO_3DES		0x22

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* Initialize card function pointer */
int scBasiccardInit( SC_CARD_INFO *ci );

#ifndef SWIG
/* Capabilities */
int scBasiccardGetCap( SC_CARD_INFO *ci, SC_CARD_CAP *cp );
#endif /* !SWIG */

/* Fill card data in ci */
int scBasiccardGetCardData( SC_CARD_INFO *ci );

/* Calculates CRC for EEPROM Check. */
WORD scBasiccardCrc( const BYTE *p, int len );

#ifndef SWIG
int scBasiccardEncrCAPDU( SC_CARD_INFO *ci, SC_APDU *apdu );

int scBasiccardDecrRAPDU( SC_CARD_INFO *ci, SC_APDU *apdu );
#endif /* !SWIG */

int scBasiccardCmdGetState( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	BYTE *state, BOOLEAN *enh );

int scBasiccardCmdEepromSize( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	WORD *offset, WORD *length );

/* Do NOT execute this function on a card which had never an image uploaded. */
int scBasiccardCmdClearEeprom( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	WORD offset, WORD length );

int scBasiccardCmdWriteEeprom( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	WORD offset, const BYTE *data, int datalen );	

int scBasiccardCmdReadEeprom( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	WORD offset, BYTE *data, int *datalen );	

/* Do NOT execute this function on a card which had never an image uploaded. */
int scBasiccardCmdEepromCrc( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	WORD offset, WORD length, WORD *crc );	

int scBasiccardCmdSetState( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	BYTE state );

int scBasiccardCmdGetApplId( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	BYTE *applid, int *length );

int scBasiccardCmdStartEncr( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	BYTE algo, BYTE keynum, const BYTE *key, const BYTE *rand );

int scBasiccardCmdEndEncr( SC_READER_INFO *ri, SC_CARD_INFO *ci );

int scBasiccardCmdEcho( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	BYTE incr, const BYTE *data, int datalen, BYTE *resp, int *resplen );

int scBasiccardCmdAssignNad( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	BYTE nad );

int scBasiccardCmdFileIo( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	BYTE syscode, BYTE filenum, const BYTE *data, int datalen, BYTE *status,
	BYTE *resp, int *resplen );

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* SC_BASICCARD_H */

