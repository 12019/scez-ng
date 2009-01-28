/****************************************************************************
*																			*
*					SCEZ chipcard library - Reader routines					*
*						Copyright Matthias Bruestle 1999					*
*					For licensing, see the file COPYRIGHT					*
*																			*
****************************************************************************/

/* $Id: screader.h 1617 2005-11-03 17:41:39Z laforge $ */

#ifndef SC_READER_H
#define SC_READER_H

#include <scez/scgeneral.h>

#define	SC_READER_PINCODING_ASCII	0x01
#define	SC_READER_PINCODING_BCD		0x02
#define	SC_READER_PINCODING_BINARY	0x03
#define	SC_READER_PINCODING_HBCI	0x04

#ifndef __palmos__
#define SC_ENV	"SCEZ_READER"
#endif /* !__palmos__ */

#ifndef __palmos__
typedef struct namenumber {
    char *name;
    int number;
} NAMENUMBER;
#endif /* !__palmos__ */

#ifndef __palmos__
#ifndef SWIG
#define SC_READER_PARAM_MAXLEN		20
#define SC_READER_NAME_MAXLEN		20

typedef struct sc_reader_detect_info {
	BYTE	prob;
	char	name[SC_READER_NAME_MAXLEN];
	int		major;
	int		minor;
	int		slots;
	BOOLEAN	pinpad;
	BOOLEAN	display;
	char	param[SC_READER_PARAM_MAXLEN];
} SC_READER_DETECT_INFO;
#endif /* !SWIG */
#endif /* !__palmos__ */

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* Expands serial port number to OS specific device name */
char *scReaderExpandPort( const char *pn );

/* Get reader configuration. */
int scReaderGetConfig( int argc, char *argv[], SC_READER_CONFIG *rc );

/* Check APDU */
int scReaderCheckAPDU( const SC_APDU *apdu, BOOLEAN t0 );

/* Initialize reader */
int scReaderInit( SC_READER_INFO *ri, const char *param );

/* Shutdown reader */
int scReaderShutdown( SC_READER_INFO *ri );

#ifndef SWIG
/* Get Capabilities */
int scReaderGetCap( SC_READER_INFO *ri, SC_READER_CAP *rp );
#endif /* !SWIG */

/* Activate card */
int scReaderActivate( SC_READER_INFO *ri );

/* Deactivate card */
int scReaderDeactivate( SC_READER_INFO *ri );

#ifndef SWIG
/* Write Buffer Async */
int scReaderWriteBuffer( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	const BYTE *buffer, int len );

/* Read Buffer Async */
int scReaderReadBuffer( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	BYTE *buffer, int len, LONG timeout );

/* Write Char Async */
int scReaderWriteChar( SC_READER_INFO *ri, SC_CARD_INFO *ci, int ch );

/* Read Char Async */
int scReaderReadChar( SC_READER_INFO *ri, SC_CARD_INFO *ci, LONG timeout );

/* Wait For Data */
int scReaderWaitForData( SC_READER_INFO *ri, SC_CARD_INFO *ci, LONG timeout );

/* Delay */
int scReaderDelay( LONG timeout );

/* Set speed */
int scReaderSetSpeed( SC_READER_INFO *ri, LONG speed );
#endif /* !SWIG */

/* Get card status */
int scReaderCardStatus( SC_READER_INFO *ri );

/* Reset card and read ATR */
int scReaderResetCard( SC_READER_INFO *ri, SC_CARD_INFO *ci );

#ifndef SWIG
/* Do a PTS handshake. */
int scReaderPTS( SC_READER_INFO *ri, SC_CARD_INFO *ci, const BYTE *pts,
	int ptslen );

/* Send and process T=0 command */
int scReaderT0( SC_READER_INFO *ri, SC_CARD_INFO *ci, SC_APDU *apdu );

/* Transmit APDU with protocol T=1 */
int scReaderT1( SC_READER_INFO *ri, SC_CARD_INFO *ci, SC_APDU *apdu );
#endif /* !SWIG */

/* Transmit APDU */
int scReaderSendAPDU( SC_READER_INFO *ri, SC_CARD_INFO *ci, SC_APDU *apdu );

/* Transmit command using PIN pad */
int scReaderVerifyPIN( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	SC_APDU *apdu, const char *message, int pinlen, int pincoding, int pinpos );

/* NULL scWaitReq function, which in fact disables maximum number of
 * WTX etc. requests.
 *
 * Normal operation:
 * When SC_EXIT_OK is returned, the protocol continues normal.
 * When anything other is returned, the protocol stops and returns this
 * return value. So when count is to high for you, you should return
 * a SC_EXIT_TIMEOUT.
 */
int scReaderWaitReqNull( SC_READER_INFO *ri, SC_CARD_INFO *ci, int count );

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* SC_READER_H */

