/****************************************************************************
*																			*
*					SCEZ chipcard library - Intertex routines				*
*						Copyright Matthias Bruestle 2000					*
*					For licensing, see the file COPYRIGHT					*
*																			*
****************************************************************************/

/* $Id: scintertex.h 1119 2005-05-08 13:28:34Z laforge $ */

#ifndef SC_INTERTEX_H
#define SC_INTERTEX_H

#include <scez/scgeneral.h>

/* Intertex headers+LRC + T=1 headers+RC + maximum command length
 * And twice, because everything could be a <DLE>.
 * And then add DLE/STX, DLE/ETX.
 */
#define SC_INTERTEX_MAX_CMD_SIZE	(2+2+5+SC_GENERAL_SHORT_DATA_SIZE+1+2)
#define SC_INTERTEX_MAX_BUFFER_SIZE	(2+((2+2+5+SC_GENERAL_SHORT_DATA_SIZE+1+2+1)*2)+2)

/* Special characters */
#define SC_INTERTEX_CHAR_DLE	0x10
#define SC_INTERTEX_CHAR_STX	0x02
#define SC_INTERTEX_CHAR_ETX	0x03
#define SC_INTERTEX_CHAR_CR		0x0D
#define SC_INTERTEX_CHAR_LF		0x0A

/* Command directions */
#define SC_INTERTEX_DIR_SND		0x01
#define SC_INTERTEX_DIR_REC		0x02
#define SC_INTERTEX_DIR_SND_REC	(SC_INTERTEX_DIR_SND|SC_INTERTEX_DIR_REC)

/* Commands */
#define SC_INTERTEX_CMD_SET_VPP			0
#define SC_INTERTEX_CMD_GET_ATR			1
#define SC_INTERTEX_CMD_DEACTIVATE		2
#define SC_INTERTEX_CMD_GET_STATUS		3
#define SC_INTERTEX_CMD_TEST_CARD		4
#define SC_INTERTEX_CMD_REPEAT_LAST		5
#define SC_INTERTEX_CMD_GET_TIMEOUT		6
#define SC_INTERTEX_CMD_PIN_ENTRY		7
#define SC_INTERTEX_CMD_SET_ALERT		8
#define SC_INTERTEX_CMD_GET_CONFIG		9
#define SC_INTERTEX_CMD_ACTIVATE_ASY	20
#define SC_INTERTEX_CMD_DATA_TO_ASY		21
#define SC_INTERTEX_CMD_DATA_FROM_ASY	22
#define SC_INTERTEX_CMD_CHANGE_FREQ		23
#define SC_INTERTEX_CMD_GET_PARAM		24
#define SC_INTERTEX_CMD_ACTIVATE_SY		30
#define SC_INTERTEX_CMD_DATA_TO_SY		31
#define SC_INTERTEX_CMD_DATA_FROM_SY	32
#define SC_INTERTEX_CMD_SHOW_CRD		60
#define SC_INTERTEX_CMD_TOGGLE_IN		61
#define SC_INTERTEX_CMD_TOGGLE_OUT		62
#define SC_INTERTEX_CMD_FLASH_ERR		63
#define SC_INTERTEX_CMD_FLASH_PIN		64
#define SC_INTERTEX_CMD_RESTORE_DISP	65
#define SC_INTERTEX_CMD_BEEP			66
#define SC_INTERTEX_CMD_EXIT			99

#define SC_INTERTEX_RSP_OK				126
#define SC_INTERTEX_RSP_REMOVED			128
#define SC_INTERTEX_RSP_UNRESPONSIVE	129
#define SC_INTERTEX_RSP_PARITY_ERROR	130
#define SC_INTERTEX_RSP_WRONG_CARD		131
#define SC_INTERTEX_RSP_UNKNOWN_CARD	132
#define SC_INTERTEX_RSP_BAD_COMMAND		133
#define SC_INTERTEX_RSP_NOT_ACTIVE		134
#define SC_INTERTEX_RSP_NOT_9000		135
#define SC_INTERTEX_RSP_BAD_PARAM		136
#define SC_INTERTEX_RSP_NO_VPP			137
#define SC_INTERTEX_RSP_BAD_ATR			138
#define SC_INTERTEX_RSP_BAD_PTS			139
#define SC_INTERTEX_RSP_BAD_PB			140
#define SC_INTERTEX_RSP_EARLY_SW		141
#define SC_INTERTEX_RSP_BAD_FORMAT		142
#define SC_INTERTEX_RSP_CARD_T0			143
#define SC_INTERTEX_RSP_CARD_T1			144
#define SC_INTERTEX_RSP_BAD_TS			145
#define SC_INTERTEX_RSP_BAD_LENGTH		146
#define SC_INTERTEX_RSP_NO_DATA			147
#define SC_INTERTEX_RSP_PIN_TO_LONG		148
#define SC_INTERTEX_RSP_PIN_TIMEOUT		149
#define SC_INTERTEX_RSP_BAD_CHAR		150
#define SC_INTERTEX_RSP_NO_PIN			151
#define SC_INTERTEX_RSP_LRC_ERROR		255

/* Card status */
#define SC_INTERTEX_CARD_PRESENT		1
#define SC_INTERTEX_CARD_ACTIVATED		2
#define SC_INTERTEX_CARD_ABSENT			3
#define SC_INTERTEX_CARD_ABSENT_CHANGE	4

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* Initialize reader */
int scIntertexInit( SC_READER_INFO *ri, char *param );

/* Shutdown reader */
int scIntertexShutdown( SC_READER_INFO *ri );

/* Detect reader */
int scIntertexDetect( SC_READER_DETECT_INFO *rdi );

/* Get Capabilities */
int scIntertexGetCap( SC_READER_INFO *ri, SC_READER_CAP *rp );

/* Activate card */
int scIntertexActivate( SC_READER_INFO *ri );

/* Deactivate card */
int scIntertexDeactivate( SC_READER_INFO *ri );

/* Write Buffer Async */
int scIntertexWriteBuffer( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	const BYTE *buffer, int len );

/* Read Buffer Async */
int scIntertexReadBuffer( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	BYTE *buffer, int len, LONG timeout );

/* Write Char Async */
int scIntertexWriteChar( SC_READER_INFO *ri, SC_CARD_INFO *ci, int ch );

/* Read Char Async */
int scIntertexReadChar( SC_READER_INFO *ri, SC_CARD_INFO *ci, LONG timeout );

/* Wait For Data */
int scIntertexWaitForData( SC_READER_INFO *ri, SC_CARD_INFO *ci, LONG timeout );

/* Get card status */
int scIntertexCardStatus( SC_READER_INFO *ri );

/* Transmit Command */
int scIntertexSendCmd( SC_READER_INFO *ri, int dir, const BYTE *snd,
	int sndlen, BYTE *rec, int *reclen, LONG timeout );

/* Reset card and read ATR */
int scIntertexResetCard( SC_READER_INFO *ri, SC_CARD_INFO *ci );

/* Transmit APDU with protocol T=0 */
/* Supports only cases 1, 2 Short, 3 Short, 4 Short.
 * You have to get the response data with Case 4 Short yourself,
 * e.g. GET RESPONSE
 */
int scIntertexT0( SC_READER_INFO *ri, SC_CARD_INFO *ci, SC_APDU *apdu );

/* Transmit APDU with protocol T=1 */
int scIntertexT1( SC_READER_INFO *ri, SC_CARD_INFO *ci, SC_APDU *apdu );

/* Transmit APDU */
int scIntertexSendAPDU( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	SC_APDU *apdu );

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* SC_INTERTEX_H */

