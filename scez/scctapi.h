/****************************************************************************
*																			*
*					SCEZ chipcard library - CT-API routines					*
*						Copyright Matthias Bruestle 1999					*
*					For licensing, see the file COPYRIGHT					*
*																			*
****************************************************************************/

/* $Id: scctapi.h 1056 2001-09-17 23:20:37Z m $ */

#ifndef SC_CTAPI_H
#define SC_CTAPI_H

#include <scez/scgeneral.h>

#if defined(__linux__)
#define	SC_CTAPI_LIB	"libctapi.so"

#elif defined(__FreeBSD__)
#define	SC_CTAPI_LIB	"/usr/lib/libctapi.so"

#elif defined(__alpha) && defined(__osf__)
#define	SC_CTAPI_LIB	"libctapi.so"

#elif defined( sun ) && ( OSVERSION > 4 )
#define	SC_CTAPI_LIB	"libctapi.so"

#elif defined(__WIN32__)
#define	SC_CTAPI_LIB	"ctapiw32.dll"

#elif defined(__WIN16__)
#define	SC_CTAPI_LIB	"ctapiw16.dll"
#endif

#define	SC_CTAPI_MAX_BUFFER_SIZE	SC_GENERAL_SHORT_DATA_SIZE+2

#define	SC_CTAPI_CARD	0x01

#define	SC_CTAPI_EXIT_OK			0		/* SC_EXIT_OK */
#define	SC_CTAPI_EXIT_ERR_INVALID	-1		/* SC_EXIT_BAD_PARAM */
#define	SC_CTAPI_EXIT_ERR_CT		-8		/* SC_EXIT_UNKNOWN_ERROR */
#define	SC_CTAPI_EXIT_ERR_TRANS		-10		/* SC_EXIT_IO_ERROR */
#define	SC_CTAPI_EXIT_ERR_MEMORY	-11		/* SC_EXIT_MALLOC_ERROR */
#define	SC_CTAPI_EXIT_ERR_HOST		-127	/* SC_EXIT_UNKNOWN_ERROR */
#define	SC_CTAPI_EXIT_ERR_HTSI		-128	/* SC_EXIT_UNKNOWN_ERROR */

#define	SC_CTAPI_REQUEST_NOTHING	0x00
#define	SC_CTAPI_REQUEST_ATR		0x01
#define	SC_CTAPI_REQUEST_HISTORICAL	0x02

#define	SC_CTAPI_UNIT_CT			0x00
#define	SC_CTAPI_UNIT_ICC			0x01

#define	SC_CTAPI_TIMEOUT_NONE		0xFF

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* Initialize reader */
int scCtapiInit( SC_READER_INFO *ri, const char *param );

/* Shutdown reader */
int scCtapiShutdown( SC_READER_INFO *ri );

/* Detect reader */
int scCtapiDetect( SC_READER_DETECT_INFO *rdi );

/* CT-BCS */

/* RESET CT */
/* atr/atrlen can be NULL, when not ATR is requested. */
int scCtapiBcsResetCT( SC_READER_INFO *ri, BYTE *sw, BYTE unit,
	BYTE qualifier, BYTE *atr, int *atrlen );

/* REQUEST ICC */
/* atr/atrlen can be NULL, when not ATR is requested.
 * atrlen: Specifies maximum ATR length and returns ATR length.
 *         If atrlen is to small for ATR it returns -1.
 */
int scCtapiBcsRequestICC( SC_READER_INFO *ri, BYTE *sw, BYTE qualifier,
	char *message, BYTE timeout, BYTE *atr, int *atrlen );

/* GET STATUS */
/* len: Specifies maximum status length and returns status length.
 *      If len is to small for status it returns -1.
 */
int scCtapiBcsGetStatus( SC_READER_INFO *ri, BYTE *sw, BYTE unit,
	BYTE *status, int *len );

/* EJECT ICC */
int scCtapiBcsEjectICC( SC_READER_INFO *ri, BYTE *sw, BYTE qualifier,
	char *message, BYTE timeout );

/* INPUT */
int scCtapiBcsInput( SC_READER_INFO *ri, BYTE *sw, BYTE qualifier,
	char *message, BYTE timeout, BYTE *input, int *inputlen );

/* OUTPUT */
int scCtapiBcsOutput( SC_READER_INFO *ri, BYTE *sw, char *message );

/* PERFORM VERIFICATION */
int scCtapiBcsPerformVer( SC_READER_INFO *ri, BYTE *sw, BYTE *data,
	BYTE datalen, char *message, BYTE timeout );

/* MODIFY VERIFICATION DATA */
int scCtapiBcsModVerData( SC_READER_INFO *ri, BYTE *sw, BYTE *data,
	BYTE datalen, char *message, BYTE timeout );

/* Activate card */
int scCtapiActivate( SC_READER_INFO *ri);

/* Deactivate card */
int scCtapiDeactivate( SC_READER_INFO *ri);

/* Get card status */
int scCtapiCardStatus( SC_READER_INFO *ri );

/* Reset card and read ATR */
int scCtapiResetCard( SC_READER_INFO *ri, SC_CARD_INFO *ci );

/* Transmit APDU with protocol T=0 */
int scCtapiT0( SC_READER_INFO *ri, SC_CARD_INFO *ci, SC_APDU *apdu );

/* Transmit APDU with protocol T=1 */
int scCtapiT1( SC_READER_INFO *ri, SC_CARD_INFO *ci, SC_APDU *apdu );

/* Transmit APDU */
int scCtapiSendAPDU( SC_READER_INFO *ri, SC_CARD_INFO *ci, SC_APDU *apdu );

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* SC_CTAPI_H */

