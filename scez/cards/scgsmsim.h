/****************************************************************************
*																			*
*					SCEZ chipcard library - GSM SIM routines				*
*						Copyright Matthias Bruestle 1999					*
*					For licensing, see the file COPYRIGHT					*
*																			*
****************************************************************************/

/* $Id: scgsmsim.h 1119 2005-05-08 13:28:34Z laforge $ */

#ifndef SC_GSMSIM_H
#define SC_GSMSIM_H

#include <scez/scgeneral.h>

/* Usefull defines for command parameters */

#define	SC_GSMSIM_RECMODE_NEXT	0x02
#define	SC_GSMSIM_RECMODE_PREV	0x03
#define	SC_GSMSIM_RECMODE_ABS	0x04
#define	SC_GSMSIM_RECMODE_CUR	0x04

#define	SC_GSMSIM_RECNUM_CUR	0x00

#define	SC_GSMSIM_SEEKMODE_BEG	0x00
#define	SC_GSMSIM_SEEKMODE_END	0x01
#define	SC_GSMSIM_SEEKMODE_NEXT	0x02
#define	SC_GSMSIM_SEEKMODE_PREV	0x03

#define	SC_GSMSIM_CHV1			0x01
#define	SC_GSMSIM_CHV2			0x02

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* Initialize card function pointer */
int scGsmsimInit( SC_CARD_INFO *ci );

#ifndef SWIG
/* Capabilities */
int scGsmsimGetCap( SC_CARD_INFO *ci, SC_CARD_CAP *cp );
#endif /* !SWIG */

/* Fill card data in ci */
int scGsmsimGetCardData( SC_CARD_INFO *ci );

#ifndef SWIG
/* Set F and D. */
int scGsmsimSetFD( SC_READER_INFO *ri, SC_CARD_INFO *ci, LONG fd );
#endif /* !SWIG */

/* Misc */

/* Translate character set. */
char scGsmsimGsmToIso( char c );
char scGsmsimIsoToGsm( char c );

/* Packs string as 7bit characters into octets. */
int scGsmsimPackStr( const char *c, int clen, BYTE *b, int *blen );

/* Unpacks 7bit characters. */
int scGsmsimUnpackStr( const BYTE *b, char *c, int clen );

/* Commands */

int scGsmsimCmdSelect( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	WORD fid, BYTE *resp, int *resplen );

int scGsmsimCmdStatus( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	BYTE *resp, int *resplen );

int scGsmsimCmdReadBin( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	WORD offset, BYTE *data, int *datalen );

int scGsmsimCmdUpdateBin( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	WORD offset, const BYTE *data, BYTE datalen );

int scGsmsimCmdReadRec( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	BYTE recnum, BYTE mode, BYTE *data, int *datalen );

int scGsmsimCmdUpdateRec( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	BYTE recnum, BYTE mode, const BYTE *data, BYTE datalen );

int scGsmsimCmdSeek( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	BOOLEAN resp, BYTE mode, const BYTE *pattern, BYTE patlen, BYTE *num );

int scGsmsimCmdIncrease( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	const BYTE *amount, BYTE *resp, int *resplen );

int scGsmsimCmdVerifyCHV( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	BYTE num, const BYTE *pin );

int scGsmsimCmdChangeCHV( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	BYTE num, const BYTE *oldpin, const BYTE *newpin );

int scGsmsimCmdDisableCHV( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	const BYTE *pin );

int scGsmsimCmdEnableCHV( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	const BYTE *pin );

int scGsmsimCmdUnblockCHV( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	BYTE num, const BYTE *unblockpin, const BYTE *newpin );

int scGsmsimCmdInvalidate( SC_READER_INFO *ri, SC_CARD_INFO *ci );

int scGsmsimCmdRehabilitate( SC_READER_INFO *ri, SC_CARD_INFO *ci );

int scGsmsimCmdRunGsmAlgo( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	const BYTE *rand, BYTE *sres, BYTE *kc );

int scGsmsimCmdSleep( SC_READER_INFO *ri, SC_CARD_INFO *ci );

/* sw is used to get the length of the available data. */
int scGsmsimCmdGetResp( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	BYTE *resp, int *resplen );

int scGsmsimCmdTermProf( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	const BYTE *prof, BYTE proflen );

int scGsmsimCmdEnvelope( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	const BYTE *comd, BYTE comdlen, BYTE *resp, int *resplen );

/* sw is used to get the length of the available data. */
int scGsmsimCmdFetch( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	BYTE *resp, int *resplen );

int scGsmsimCmdTermResp( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	const BYTE *comd, BYTE comdlen );

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* SC_GSMSIM_H */

