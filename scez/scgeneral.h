/****************************************************************************
*																			*
*					SCEZ chipcard library - General routines				*
*					Copyright Matthias Bruestle 1999-2001					*
*					For licensing, see the file COPYRIGHT					*
*																			*
****************************************************************************/

/* $Id: scgeneral.h 1617 2005-11-03 17:41:39Z laforge $ */

#ifndef SC_GENERAL_H
#define SC_GENERAL_H

/**************************************************************************/
/* Configuration defines. Preliminary place. */

/*
 * Port where reader is attached. This is used by all test programs.
 *
 * CT-API: Meaning of port is driver specific. The linux/solaris CT-API
 *    drivers normaly use port 0-3 for COM 1-4.
 *
 * Dumbmouse,Towitoko: "1","2","3","4" is substituted by the device name
 *    of the serial port. See sc_reader.c. If no apropriate substitution
 *    devices are available READER_PORT can contain the full devicename.
 */
#define READER_PORT "0"

/*
 * Reader type
 */
#define READER_TYPE SC_READER_DUMBMOUSE

/*
 * Card slot in reader
 * 
 * CT-API: The linux/solaris CT-API drivers normaly use slot 0.
 * Dumb Mouse: Slot 1.
 * Towitoko: Currently only slot 1.
 */
#define READER_SLOT 1

/*
# RESPECTS_LE: When defined my T=1 implementation limits response
#     length to Le bytes. This is how I understand ISO7816-4 5.3
#     and A 2S, A 4S.
 */
/* #define RESPECTS_LE */

/*
# T0_LAX_CASE_HANDLING: When defined handle case more freely,
#     e.g. accept in case 1 response data.
 */
#define T0_LAX_CASE_HANDLING /* */

/*
 * NO_APDU_CHECK: When defined my T=1 implementation is not so
 *     pedantic with APDU cases.
 */
/* #define NO_APDU_CHECK */

/*
 * READER_DEBUG: Prints communication data to stdout.
 */
/* #define READER_DEBUG */
/* #define SO_ONLY_TEST */

/*
 * TOWITOKO_LOW_POWER
 *
 * For some cards and Towitoko Chipdrives which get their power only form
 * the serial port, there needs to be a longer pause after turning on the
 * reader. You have then to set this define. But it may be, that even this
 * is not enough to get a power draining card working.
 * This define does not hurt, if the Chipdrive has another power supply,
 * but you will need more patience.
 */
#define TOWITOKO_LOW_POWER

/**************************************************************************/

#ifdef __palmos__
#include <Pilot.h>
#endif

#ifdef __BORLANDC__
#include <windows.h>
#ifndef LONG
#define LONG	DWORD
#endif /* LONG */
#else /* Not __BORLANDC__ */
#ifndef BYTE
#define BYTE	unsigned char
#endif /* BYTE */
#if !defined(SWIG) && !defined(SWIG_C)
#ifndef WORD
#define WORD	unsigned short
#endif /* WORD */
#endif /* !SWIG */
#ifndef LONG
#if defined(__alpha) || (__mips==4) || (defined(__hpux)&&defined(__LP64__))
#define LONG	unsigned int
#else /* !__alpha */
#define LONG	unsigned long
#endif /* __alpha */
#endif /* LONG */
#endif /* __BORLANDC__ */

#define BOOLEAN	int

#ifndef FALSE
#define FALSE	0
#endif
#ifndef TRUE
#define TRUE	!0
#endif

#ifndef max
#define max( a, b )   ( ( ( a ) > ( b ) ) ? ( ( int ) a ) : ( ( int ) b ) )
#endif /* !max */

#ifndef min
#define min( a, b )   ( ( ( a ) < ( b ) ) ? ( ( int ) a ) : ( ( int ) b ) )
#endif /* !min */

#if !defined(HEADER_SIO_TJH_H)
typedef struct sio_info SIO_INFO;
#endif /* !HEADER_SIO_TJH_H */

#if defined(WITH_LIBDES) || defined(HAVE_LIBCRYPT) || defined(HAVE_LIBCRYPTO)
#ifndef WITH_DES
#define WITH_DES
#endif /* !WITH_DES */
#endif /* WITH_LIBDES || HAVE_LIBCRYPT || HAVE_LIBCRYPTO */

/** Defines **/

#define	SC_GENERAL_SHORT_DATA_SIZE	256
#define	SC_GENERAL_EXTENDED_DATA_SIZE	65536
#define	SC_GENERAL_MAX_DATA_SIZE	65536

/* Reader (main types) */

#define SC_READER_UNKNOWN		0x00
#define SC_READER_DUMBMOUSE		0x01
#define SC_READER_TOWITOKO		0x02
#define	SC_READER_CTAPI			0x03
#define SC_READER_CHIPI			0x04
#define SC_READER_ACR20			0x05
#define SC_READER_B1			0x06
#define SC_READER_GCR400		0x07
#define SC_READER_REFLEX60		0x08
#define SC_READER_REFLEX20		0x09
#define SC_READER_INTERTEX		0x0A
#define SC_READER_GPR400		0x0B
#define SC_READER_BLUEDOT		0x0C
#define SC_READER_EASYCHECK		0x0D
#define	SC_READER_VENDOR1		0xF0
#define	SC_READER_VENDOR2		0xF1
#define	SC_READER_VENDOR3		0xF2
#define SC_READER_AUTO			0xFE

/* Timeout */

#define SC_TIMEOUT_DEFAULT		-1

/* Cards */

/* high byte: major card type
 * low byte: minor card type
 */

#define SC_CARD_UNKNOWN			0x0000
#define SC_CARD_MULTIFLEX		0x0100
#define SC_CARD_MULTIFLEX_3K		0x0101 /* 3kB EEPROM */
#define SC_CARD_MULTIFLEX_8K		0x0102 /* 8kB EEPROM, more features */
#define SC_CARD_MULTIFLEX_8K_DES	0x0103 /* Full DES option */
#define SC_CARD_MULTIFLEX_4K		0x0104 /* 4kB EEPROM */
#define SC_CARD_CRYPTOFLEX		0x0200
#define SC_CARD_CRYPTOFLEX_DES		0x0201 /* Full DES option */
#define SC_CARD_CRYPTOFLEX_KEYGEN	0x0202 /* Full DES / RSA Keygen */
#define SC_CARD_CRYPTOFLEX_8K		0x0203 /* with 8kB EEPROM */
#define SC_CARD_CRYPTOFLEX_16K		0x0203 /* with 16kB EEPROM */
#define SC_CARD_CYBERFLEX		0x0300 /* JC 2.0 series Cyberflex */
#define SC_CARD_CYBERFLEX_CRYPTO	0x0301
#define SC_CARD_CYBERFLEX_AUGCRYPTO	0x0302
#define SC_CARD_PAYFLEX			0x0400
#define	SC_CARD_PAYFLEX_1K_USER		0x0401
#define	SC_CARD_PAYFLEX_1K_SAM		0x0402
#define	SC_CARD_PAYFLEX_4K_USER		0x0403
#define	SC_CARD_PAYFLEX_4K_SAM		0x0404
#define SC_CARD_PAYFLEX_MICRO		0x0405
#define SC_CARD_GPK4000			0x0500
#define SC_CARD_GPK4000_S		0x0501
#define SC_CARD_GPK4000_SP		0x0502 /* "privacy" */
#define SC_CARD_GPK4000_SDO		0x0503 /* Encryption/Key Generation */
#define SC_CARD_GPK4000_SU256		0x0504
#define	SC_CARD_GPK2000			0x0600
#define	SC_CARD_GPK2000_S		0x0601
#define	SC_CARD_GPK2000_SP		0x0602 /* "privacy" */
#define SC_CARD_MPCOS_EMV		0x0700
#define	SC_CARD_MPCOS_EMV_1B		0x0701 /* 1 Byte data units */
#define	SC_CARD_MPCOS_EMV_4B		0x0702 /* 4 Byte data units */
#define SC_CARD_GELDKARTE		0x0800
#define SC_CARD_GELDKARTE_3		0x0801 /* Geldkarte v3 */
#define SC_CARD_TCOS			0x0900
#define SC_CARD_TCOS_44			0x0901 /* SLE44 Chip */
#define SC_CARD_TCOS_66			0x0902 /* SLE66 Chip */
#define SC_CARD_TCOS_66P		0x0903 /* SLE66P Chip */
#define SC_CARD_BASICCARD		0x0A00
#define SC_CARD_BASICCARD_COMP		0x0A01
#define SC_CARD_BASICCARD_ENH		0x0A02
#define SC_CARD_BASICCARD_ENH_3		0x0A03
#define SC_CARD_BRADESCO		0x0B00 /* Moeda Eletronica */
#define SC_CARD_GSMSIM			0x0C00
#define SC_CARD_CHIPPER			0x0D00 /* Dutch Post money card */
#define SC_CARD_PROTON			0x0E00
#define SC_CARD_PROTON_CASH		0x0E01 /* Swiss Cash card */
#define SC_CARD_PROTON_CHIPKNIP		0x0E02 /* Dutch bank money card */
#define SC_CARD_STARCOS			0x0F00 /* From G&D. */
#define SC_CARD_STARCOS_S21		0x0F01
#define SC_CARD_STARCOS_SPK22		0x0F02
#define SC_CARD_STARCOS_SPK23		0x0F03
#define SC_CARD_STARCOS_SPK24		0x0F04
#define SC_CARD_SMARTCAFE		0x1000 /* JavaCard from G&D */
#define SC_CARD_SMARTCAFE_11		0x1001
#define SC_CARD_MFC			0x1100 /* Comcard (Ex-IBM) MFC */
#define SC_CARD_MFC_41			0x1101
#define SC_CARD_GPK8000			0x1200
#define SC_CARD_GPK8000_8K		0x1201 /* This is the real GPK8000 */
#define SC_CARD_GPK8000_16K		0x1202 /* This is the GPK16000 */
#define SC_CARD_PAYCARD			0x1300
#define SC_CARD_QUICK			0x1400
#define SC_CARD_GEMXPRESSO		0x1500
#define SC_CARD_GEMXPRESSO_211PK	0x1501
#define SC_CARD_JIB			0x1600
#define SC_CARD_CYBERFLEX2		0x1700 /* JC 2.1 series Cyberflex */
#define SC_CARD_CYBERFLEX2_DEV_32K	0x1701 /* Cyberflex Access Developer 32k */
#define SC_CARD_CARTEB			0x1800
#define	SC_CARD_VENDOR1			0xFFFFFD00
#define	SC_CARD_VENDOR2			0xFFFFFE00
#define	SC_CARD_VENDOR3			0xFFFFFF00

/* Card status flags */

#define	SC_CARD_STATUS_PRESENT	0x01	/* Card present. */
#define	SC_CARD_STATUS_CHANGED	0x02	/* Card changed. Not reliable. */

/* T=1 checksum */

#define	SC_T1_CHECKSUM_LRC	0x00
#define	SC_T1_CHECKSUM_CRC	0x01

/* Protocols */

#define	SC_PROTOCOL_UNKNOWN	0xFF
#define SC_PROTOCOL_T0		0x00
#define SC_PROTOCOL_T1		0x01
#define SC_PROTOCOL_T14		0x0E
#define SC_PROTOCOL_I2C		0x10
#define SC_PROTOCOL_2WIRE	0x20
#define SC_PROTOCOL_3WIRE	0x40
#define SC_PROTOCOL_1WIRE	0x80

/* Exit codes */

#define	SC_EXIT_OK					0
#define	SC_EXIT_UNKNOWN_ERROR		1	/* Error code for everything */
#define	SC_EXIT_IO_ERROR			2
#define	SC_EXIT_NOT_SUPPORTED		3
#define	SC_EXIT_NO_ACK				4
#define	SC_EXIT_BAD_ATR				5
#define	SC_EXIT_PROBE_ERROR			6
#define	SC_EXIT_BAD_CHECKSUM		7
#define	SC_EXIT_TIMEOUT				8
#define	SC_EXIT_CARD_CHANGED		9
#define	SC_EXIT_NO_CARD				10
#define	SC_EXIT_NOT_IMPLEMENTED		11
#define	SC_EXIT_CMD_TOO_SHORT		12
#define	SC_EXIT_MALLOC_ERROR		13
#define	SC_EXIT_BAD_SW				14
#define	SC_EXIT_BAD_PARAM			15
#define	SC_EXIT_NO_SLOT				16
#define	SC_EXIT_LIB_ERROR			17
#define SC_EXIT_PROTOCOL_ERROR		18
#define SC_EXIT_LOCKED				19
#define	SC_EXIT_NO_MATCH			20
#define	SC_EXIT_CMD_TOO_LONG		21
#define	SC_EXIT_RSP_TOO_LONG		22

/* For PKCS#15. */

#define SC_EXIT_FILE_ERROR			1001
#define SC_EXIT_DATA_ERROR			1002
#define SC_EXIT_AUTH_ERROR			1003
#define SC_EXIT_NO_SPACE			1004

/* APDU Casees */

/*
 * Case 1: lc=0, le=0
 * Case 2 Short: lc=0, le<=256
 * Case 3 Short: lc<=255, le=0
 * Case 4 Short: lc<=255, le<=256
 * Case 2 Extended: lc=0, le<=65536
 * Case 3 Extended: lc<=65535, le=0
 * Case 4 Extended: lc<=65535, le<=65536
 *
 * T=0: Case 4 becomes Case 3
 */

#define	SC_APDU_CASE_NONE	0
#define	SC_APDU_CASE_1		1
#define	SC_APDU_CASE_2_SHORT	2
#define	SC_APDU_CASE_3_SHORT	3
#define	SC_APDU_CASE_4_SHORT	4
#define	SC_APDU_CASE_2_EXT	5
#define	SC_APDU_CASE_3_EXT	6
#define	SC_APDU_CASE_4_EXT	7

/* Status words */

#define	SC_SW_OK		0
#define	SC_SW_DATA_AVAIL	1
#define	SC_SW_UNKNOWN		2

/** Structs **/

#ifdef SWIG

typedef struct sc_apdu {
	int	cse;		/* APDU case */
	BYTE	*cmd;		/* C-APDU */
	int	cmdlen;		/* length of C-APDU */
	BYTE	*rsp;		/* R-APDU */
	int	rsplen;		/* length of R-APDU */
} SC_APDU;

typedef struct sc_crypt_info SC_CRYPT_INFO;
typedef struct sc_t0_info SC_T0_INFO;
typedef struct sc_t1_info SC_T1_INFO;
typedef struct sc_card_info SC_CARD_INFO;
typedef struct sc_reader_info SC_READER_INFO;

#define	SC_READER_CONF_DEFAULT	0
#define	SC_READER_CONF_1	1
#define	SC_READER_CONF_2	2
#define	SC_READER_CONF_3	3
#define	SC_READER_CONF_4	4

typedef struct sc_reader_config {
	int	nr;	/* To allow multiple reader configurations.
			 * 0 is the default. */
	int	type;	/* Reader type. SC_READER_* */
	int	slot;	/* Slot. */
	char	*param;	/* Parameter containing serial port, etc.. */
} SC_READER_CONFIG;

#else /* !SWIG */

struct sc_reader_info;

/* APDU */

typedef struct sc_apdu {
	int	cse;		/* APDU case */
	BYTE	*cmd;		/* C-APDU */
	int	cmdlen;		/* length of C-APDU */
	BYTE	*rsp;		/* R-APDU */
	int	rsplen;		/* length of R-APDU */
} SC_APDU;

/* Data block */

typedef struct sc_data {
	LONG	len;		/* data length */
	LONG	offset;		/* data offset, e.g. from begining of card */
	BYTE	*data;		/* data */
} SC_DATA;

/* Card capabilities */
/* Data about the card which is not often needed or very large.
 * Split from SC_CARD_INFO to reduce memory requirements, e.g. on the
 * Palm Pilot.
 */

#define MAX_FD_VALUES	10

typedef struct sc_card_cap {
	int	n_fd;		/* Possible f/d pairs. */
	LONG	fd[MAX_FD_VALUES];
		/* kbps at 3.579MHz[8]|f[16]|d[8], 1/x=-x */
} SC_CARD_CAP;

/* Card Info */

/* Crypt info */

typedef struct sc_crypt_info {
	BOOLEAN	encrypt;	/* Encrypt commands */
	BOOLEAN mac;		/* Append MAC to commands */
	int	algo;		/* Encryption algorithm */
	int	keynum;		/* Key number */
	BYTE	iv[8];		/* Initialisation vector */
	BYTE	key[16];	/* Encryption key */
	BYTE	pin[8];		/* PIN */
} SC_CRYPT_INFO;

/* T=0 info */

typedef struct sc_t0_info {
	BYTE	d;			/* D */
	BYTE	wi;			/* WI */
	LONG	wwt;		/* Work Waiting Time in etu */
	BYTE	getrsp[5];	/* GET RESPONSE Header */
} SC_T0_INFO;

/* T=1 info */

typedef struct sc_t1_info {
	BYTE	nad;		/* NAD */
	BYTE	ns;			/* N(S) */
	BYTE	nr;			/* N(R) */
	BYTE	ifsc;		/* Information Field Size Card */
	BYTE	ifsd;		/* Information Field Size Device */
	BOOLEAN	ifsreq;		/* S(IFS Req) already sent? */
	LONG	cwt;		/* Character Waiting Time in etu -11 etu */
	LONG	bwt;		/* Block Waiting Time in us */
	BYTE	rc;			/* Redundancy Check (LRC/CRC) */
} SC_T1_INFO;

typedef struct sc_card_info {
	LONG	type;		/* Card type */
	BYTE	atr[32];	/* ATR */
	int		atrlen;		/* Length of ATR */
	int		protocol;	/* Used protocol */
	BOOLEAN		direct;		/* Direct convention */
	SC_T0_INFO	t0;		/* for T=0 */
	SC_T1_INFO	t1;		/* for T=1 */
	SC_CRYPT_INFO	crypt;	/* Encryption status */
	BYTE	cla;		/* CLA byte for cyberflex. */
	BYTE	swok[5];	/* swok[0]: length, swok[1...]: SW[0]==SC_SW_OK */
	BYTE	swav[5];	/* swav[0]: length, swav[1...]: SW[0]==SC_SW_AVAIL */
	LONG	memsize;	/* EEPROM size (I2C,2W,3W) */
	BYTE	sw[2];		/* SW1 SW2 */
	/* function pointers */
	int		(* scGetCap)( struct sc_card_info *ci,
			struct sc_card_cap *cp );
	int		(* scGetCardData)( struct sc_card_info *ci );
	int		(* scSetFD)( struct sc_reader_info *ri, struct sc_card_info *ci,
			LONG fd );
} SC_CARD_INFO;

/* Reader capabilities */
/* Data about the reader which is not often needed or very large.
 * Split from SC_READER_INFO to reduce memory requirements, e.g. on the
 * Palm Pilot.
 */
/* Not so sure about motor. */

typedef struct sc_reader_cap {
	BOOLEAN		t0err;	/* Detects and handles T=0 error signalig. */
	BOOLEAN		t1;	/* Can do T=1. */
	LONG		freq;	/* frequency of clock. */
	BOOLEAN		motor;	/* card reader has motor to slurp in card. */
	BYTE		slots;	/* number of slots. */
	int		n_fd;	/* Possible f/d pairs. */
	LONG		fd[MAX_FD_VALUES];
				/* kbps at 3.579MHz[8]|f[16]|d[8], 1/x=-x */
	LONG		speed[MAX_FD_VALUES]; /* Speeds for fd values. */
} SC_READER_CAP;

/* Reader info */

typedef struct sc_reader_info {
	int		major;	/* main reader type */
	int		minor;	/* minor reader type */
	int		slot;	/* slot number (ICC1:1, ICC2:2, ...) */
	int		status;	/* Card Status */
	BOOLEAN		pinpad;	/* card reader has PIN pad. */
	BOOLEAN		display;	/* card reader has display. */
	LONG		etu;	/* etu in us */
	int		maxc;	/* Maximum length of command. Handling driver
				 * specific. */
	int		maxr;	/* Maximum length of response. Handling driver
				 * specific. */
	SIO_INFO	*si;	/* serial port handle (ACR20S, Dumbmouse,
				 * GCR 400, Towitoko) */
	WORD		ctn;	/* cardterminal number (CT-API) */
	int		fd;	/* file descriptor (GPR400) */
	SC_T1_INFO	t1;	/* T=1PC readers (B1) */
	/* function pointers */
	int	(* scShutdown)( struct sc_reader_info *ri );
	int	(* scGetCap)( struct sc_reader_info *ri,
			struct sc_reader_cap *rp );
	int	(* scActivate)( struct sc_reader_info *ri );
	int	(* scDeactivate)( struct sc_reader_info *ri );
	int	(* scWriteBuffer)( struct sc_reader_info *ri, struct
				sc_card_info *ci, const BYTE *buffer, int len );
	int	(* scReadBuffer)( struct sc_reader_info *ri, struct
				sc_card_info *ci, BYTE *buffer, int len, LONG timeout );
	int	(* scWriteChar)( struct sc_reader_info *ri, struct
				sc_card_info *ci, int ch );
	int	(* scReadChar)( struct sc_reader_info *ri, struct
				sc_card_info *ci, LONG timeout );
	int	(* scWaitForData)( struct sc_reader_info *ri, struct
				sc_card_info *ci, LONG timeout );
	int	(* scSetSpeed)( struct sc_reader_info *ri, LONG speed );
	int	(* scCardStatus)( struct sc_reader_info *ri );
	int	(* scResetCard)( struct sc_reader_info *ri, struct
				sc_card_info *ci );
	int	(* scPTS)( struct sc_reader_info *ri, struct sc_card_info *ci,
				const BYTE *pts, int ptslen );
	int	(* scT0)( struct sc_reader_info *ri, struct sc_card_info *ci,
				struct sc_apdu *apdu );
	int	(* scT1)( struct sc_reader_info *ri, struct sc_card_info *ci,
				struct sc_apdu *apdu );
	int	(* scSendAPDU)( struct sc_reader_info *ri, struct sc_card_info
				*ci, struct sc_apdu *apdu );
	int	(* scVerifyPIN)( struct sc_reader_info *ri, struct sc_card_info
				*ci, struct sc_apdu *apdu, const char *message,
				int pinlem, int pincoding, int pinpos );
	/* Callback function pointers */
	int	(* scWaitReq)( struct sc_reader_info *ri, struct sc_card_info
				*ci, int count );
} SC_READER_INFO;

#define	SC_READER_CONF_DEFAULT	0
#define	SC_READER_CONF_1	1
#define	SC_READER_CONF_2	2
#define	SC_READER_CONF_3	3
#define	SC_READER_CONF_4	4

typedef struct sc_reader_config {
	int	nr;		/* To allow multiple reader configurations.
				 * 0 is the default. */
	int	type;	/* Reader type. SC_READER_* */
	int	slot;	/* Slot. */
	char	*param;	/* Parameter containing serial port, etc.. */
} SC_READER_CONFIG;

#endif /* !SWIG */

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* Init library */
int scInit();

/* Clean up library */
int scEnd();

/* Create reader */
SC_READER_INFO *scGeneralNewReader( int type, int slot );

/* Remove reader */
void scGeneralFreeReader( SC_READER_INFO **ri );

#ifndef SWIG
/* Create reader capabilities structure */
SC_READER_CAP *scGeneralNewReaderCap();

/* Free reader capabilities structure */
void scGeneralFreeReaderCap( SC_READER_CAP **rp );
#endif /* !SWIG */

/* Create card */
SC_CARD_INFO *scGeneralNewCard();

/* Remove card */
void scGeneralFreeCard( SC_CARD_INFO **ci );

#ifndef SWIG
/* Create card capabilities structure */
SC_CARD_CAP *scGeneralNewCardCap();

/* Free card capabilities structure */
void scGeneralFreeCardCap( SC_CARD_CAP **cp );
#endif /* !SWIG */

/* Create APDU */
SC_APDU *scGeneralNewAPDU();

/* Remove APDU */
void scGeneralFreeAPDU( SC_APDU **apdu );

/* Reverse bitorder of each byte */
void scGeneralReverseString( BYTE *data, int len);

/* Get random bytes */
/* NO TRUE RANDOM NUMBERS! */
int scGeneralGetRandStr( BYTE *data, int len );

/* Clean keys in SC_CARD_INFO. */
void scGeneralCleanKeys( SC_CARD_INFO *ci );

/* Cleans SC_CARD_INFO. */
void scGeneralCleanCI( SC_CARD_INFO *ci );

/* Convert binary to ascii coded hex and back. Returns length of out. */
int scGeneralBinHex( BOOLEAN tohex, const BYTE *in, int inlen, BYTE *out );

#ifdef __cplusplus
}
#endif /* __cplusplus */

#include <scez/screader.h>
#include <scez/scsmartcard.h>

#endif /* SC_GENERAL_H */

