/****************************************************************************
*																			*
*						Copyright Matthias Bruestle 2000					*
*					For licensing, see the file COPYRIGHT					*
*																			*
****************************************************************************/

/* $Id: cardcheck.c 1617 2005-11-03 17:41:39Z laforge $ */

/* Define DANGER to include for some cards dangerous operations.
 * Not defining DANGER does currently only disable the Erase Card
 * command for the GPK4000.
 */

/* TODO:
 * - Fast CLA/INS scan: First CLA scan, then INS scan.
 * - File scan.
 * - Limit tested CLAs for file scan.
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#if !defined(__WIN32__) && !defined(__WIN16__)
#include <getopt.h>
#include <unistd.h>
#endif /* !__WIN32__ && !__WIN16__ */

#if 0
#include <sio/sio.h>
#endif

#include <scez/scgeneral.h>
#include <scez/cards/scbasiccard.h>
#include <scez/cards/sccryptoflex.h>
#include <scez/cards/sccyberflex.h>
#include <scez/cards/scgeldkarte.h>
#include <scez/cards/scgpk4000.h>
#include <scez/cards/scgsmsim.h>
#include <scez/cards/scmfc.h>
#include <scez/cards/scmultiflex.h>
#include <scez/cards/scpayflex.h>
#include <scez/cards/scproton.h>
#include <scez/cards/scsmartcafe.h>
#include <scez/cards/sctcos.h>

#ifndef READER_TYPE
#define READER_TYPE SC_READER_DUMBMOUSE
#endif /* READER_TYPE */
#ifndef READER_SLOT
#define READER_SLOT 1
#endif /* READER_SLOT */
#ifndef READER_PORT
#define READER_PORT "0"
#endif /* READER_PORT */

/* Declarations */

int checkSelMf1( SC_READER_INFO *ri, SC_CARD_INFO *ci, BYTE cla, BYTE *rsp,
	int *rsplen );

/* Byte block */

typedef struct {
	BYTE data[ 256 ];
	LONG len;
} BYTE_256;

/* AID values */

typedef struct {
	const BYTE *aid;		/* AID */
	const int aidlen;		/* Length of AID */
	const char *desc;		/* Description of Application */
	} AID_VALUE;

static const AID_VALUE aidTable[] = {
	{ (const BYTE *) "\x47\x54\x4F\x4B\x31", 5, "GemPlus GemSAFE (GTOK1)" },
	{ (const BYTE *) "\xA0\x00\x00\x00\x63PKCS-15", 12, "PKCS-15" },
	{ (const BYTE *) "\xA0\x00\x00\x00\x77PK15-10", 12, "CeloCom PKCS-15" },
	{ (const BYTE *) "\xD2\x76\x00\x00\x01\x01", 6, "KVK (German health insurance card)" },
	{ (const BYTE *) "\xD2\x76\x00\x00\x03\x01\x02", 7, "Telesec NetKey" },
	{ (const BYTE *) "\xD2\x76\x00\x00\x25\x45\x50\x01\x00", 9, "Geldkarte" },
	{ (const BYTE *) "\xD2\x76\x00\x00\x37\x02", 6, "Lotto" },
	{ (const BYTE *) "\xD2\x76\x00\x00\x60", 6, "Wolfgang Rankl" },
	{ (const BYTE *) "\xD2\x76\x00\x00\x66\x01", 6, "SigG" },
	{ (const BYTE *) "\xD2\x76\x00\x00\x66\x02", 6, "SigG" },
	{ (const BYTE *) "\xD2\x76\x00\x00\x74\x48\x42\x01\x10", 9,
		"HBCI Bank-Verlag" },
	{ (const BYTE *) "\xD2\x76\x00\x00\x92", 6, "Matthias Bruestle" },
	{ NULL, 0, NULL }
};

/* ATR values for unsupported cards */

static const ATR_VALUE atrTable2[] = {
	{ (const BYTE *) "\x3B\xEF\x00\xFF\x81\x31\x66\x45\x65\x63\x20\x20\x49\x42\x4D\x20\x33\x2E\x31\x20\x20\x20\x20\xCF",
		NULL, 24, 0xE000 }, /* IBM eCash */
	{ (const BYTE *) "\x3B\x82\x81\x31\x76\x43\xC1\x03\xC5",
		NULL, 9, 0xE001 }, /* i.ti (ticket card for Collogne/Bonn) */
	{ (const BYTE *) "\x3B\xA8\x00\x81\x71\x46\x5D\x00\x54\x43\x4F\x53\x31\x2E\x32\x00\x65",
		NULL, 17, 0xE002 }, /* Telesec TCOS 1.2 */
/*	{ (const BYTE *) "",
		NULL, , 0xE0 }, */
	{ NULL, NULL, 0 }
};

/* Command descriptions */

static const char *cmddesc[128] = {
	/* 00 */ "Get State (BasicCard), Get MULTOS Data (MULTOS)",
	/* 02 */ "EEPROM Size (BasicCard), Get Manufacturer Data (MULTOS)",
	/* 04 */ "Invalidate (GSM SIM, Cryptoflex, Cyberflex, MFC), Clear EEPROM (BasicCard)",
	/* 06 */ "Write EEPROM (BasicCard)",
	/* 08 */ "Manage Instance (Cyberflex), Read EEPROM (BasicCard), Card Unblock (MULTOS)",
	/* 0A */ "Manage Programm (Cyberflex), EEPROM CRC (BasicCard)",
	/* 0C */ "Execute Method (Cyberflex), Set State (BasicCard)",
	/* 0E */ "Erase Binary (ISO7816-4), Get Application ID (BasicCard)",
	/* 10 */ "Term Prof (GSM SIM), Create Priv Key File (GPK4000), Start Encryption (BasicCard)",
	/* 12 */ "Fetch (GSM SIM), End Encryption (BasicCard), Open MEL Application (MULTOS)",
	/* 14 */ "Term Response (GSM SIM), RSA Key Gen (Cryptoflex), Switch Speed (GPK4000), Echo (BasicCard)",
	/* 16 */ "Card Block (MFC), Freeze AC (GPK4000), Assign NAD (BasicCard), Create MEL Application (MULTOS)",
	/* 18 */ "Load Priv Key (GPK4000), File I/O (BasicCard), Delete MEL Application (MULTOS)",
	/* 1A */ "Clear Memory (SmartCafe), DES Encr (GPK4000)",
	/* 1C */ "",
	/* 1E */ "Compute DES Key (GPK4000)",
	/* 20 */ "Verify (ISO7816-4, GSM SIM, Cryptoflex, Cyberflex, GPK4000, MFC, SmartCafe, TCOS2), Load DIR File Record (MULTOS)",
	/* 22 */ "Manage Security Environment (TCOS2), Log Out All (Cyberflex), Load FCI Record (MULTOS)",
	/* 24 */ "Change PIN (GSM SIM, Cryptoflex, Cyberflex, GPK4000, MFC, TCOS2), Unblock PIN (GPK4000), Load Code (MULTOS)",
	/* 26 */ "Disable PIN (GSM SIM, Cyberflex), Load Data (MULTOS)",
	/* 28 */ "Enable PIN (GSM SIM, Cyberflex), Select File Key (GPK4000), Load Application Signature (MULTOS)",
	/* 2A */ "Perform Security Operation (TCOS2), Verify Key (Cryptoflex, Cyberflex), Set PIN (SmartCafe), PIN Change (OpenPlatform), Load KTU Ciphertext (MULTOS)",
	/* 2C */ "Unblock PIN (GSM SIM, Cryptoflex, Cyberflex, MFC)",
	/* 2E */ "",
	/* 30 */ "Decrease (Cryptoflex, MFC), Select Purse Key (GPK4000)",
	/* 32 */ "Increase (GSM SIM, Cryptoflex, Cyberflex, MFC), Read Balance (GPK4000)",
	/* 34 */ "Decrease Stamped (GSM SIM, Multiflex), Debit (GPK4000)",
	/* 36 */ "Increase Stamped (MFC, Multiflex), Credit (GPK4000)",
	/* 38 */ "Sign (GPK4000)",
	/* 3A */ "Set Options (GPK4000)",
	/* 3C */ "",
	/* 3E */ "",
	/* 40 */ "Read Stat (MFC)",
	/* 42 */ "Mod Baud Rate (MFC)",
	/* 44 */ "Rehabilitate (GSM SIM, Cryptoflex, Cyberflex, MFC)",
	/* 46 */ "Cancel Debit (GPK4000)",
	/* 48 */ "",
	/* 4A */ "",
	/* 4C */ "",
	/* 4E */ "",
	/* 50 */ "Init Update (OpenPlatform)",
	/* 52 */ "",
	/* 54 */ "",
	/* 56 */ "",
	/* 58 */ "",
	/* 5A */ "",
	/* 5C */ "",
	/* 5E */ "",
	/* 60 */ "",
	/* 62 */ "",
	/* 64 */ "",
	/* 66 */ "",
	/* 68 */ "",
	/* 6A */ "",
	/* 6C */ "",
	/* 6E */ "",
	/* 70 */ "Mange Channel (ISO7816-4)",
	/* 72 */ "",
	/* 74 */ "",
	/* 76 */ "Lock (MFC)",
	/* 78 */ "Set Exec (MFC)",
	/* 7A */ "",
	/* 7C */ "",
	/* 7E */ "",
	/* 80 */ "",
	/* 82 */ "External Authenticate (ISO7816-4, Cryptoflex, Cyberflex, GPK4000, MFC, OpenPlatform), Verify Data (Cryptoflex)",
	/* 84 */ "Get Challenge (Cryptoflex, Cyberflex, Geldkarte, GPK4000, MFC, TCOS2), Load Cert (Cryptoflex)",
	/* 86 */ "Give Random (MFC, Multiflex), Verify Pub Key (Cryptoflex), Pub Key Sign (GPK4000)",
	/* 88 */ "Internal Authenticate (ISO7816-4, Cryptoflex, Cyberflex, GPK4000, MFC), Run GSM Algo (GSM SIM), RSA Sign (Cryptoflex), Pub Key Internal Authentication (GPK4000)",
	/* 8A */ "Pub Key Verify (GPK4000)",
	/* 8C */ "Pub Key Send (GPK4000)",
	/* 8E */ "",
	/* 90 */ "",
	/* 92 */ "",
	/* 94 */ "",
	/* 96 */ "",
	/* 98 */ "",
	/* 9A */ "",
	/* 9C */ "",
	/* 9E */ "",
	/* A0 */ "Pub Key Dir (GPK4000)",
	/* A2 */ "Seek (GSM SIM, Cryptoflex, Cyberflex, MFC)",
	/* A4 */ "Select File (ISO7816-4, GSM SIM, Cryptoflex, Cyberflex, Geldkarte, GPK4000, MFC, Proton, Quick, SmartCafe, TCOS2, OpenPlatform)",
	/* A6 */ "Select Crypto Context (GPK4000)",
	/* A8 */ "Directory (Cryptoflex, Cyberflex)",
	/* AA */ "List Directory (TCOS2)",
	/* AC */ "Close Application (MFC)",
	/* AE */ "",
	/* B0 */ "Read Binary (ISO7816-4, GSM SIM, Cryptoflex, Cyberflex, GPK4000, MFC, Proton, Quick, TCOS2)",
	/* B2 */ "Read Record (ISO7816-4, GSM SIM, Cryptoflex, Cyberflex, Geldkarte, GPK4000, MFC, Quick, TCOS2)",
	/* B4 */ "Get Challenge (ISO7816-4, Proton), Read Binary Stamped (MFC), Lookup Balance (Proton)",
	/* B6 */ "Read Record (Proton), Read Record Stamped (MFC)",
	/* B8 */ "",
	/* BA */ "",
	/* BC */ "",
	/* BE */ "",
	/* C0 */ "Get Response (ISO7816-4, GSM SIM, Cryptoflex, Cyberflex, GPK4000), Get Info (GPK4000)",
	/* C2 */ "Envelope (ISO7816-4)",
	/* C4 */ "Envelope (GSM SIM)",
	/* C6 */ "",
	/* C8 */ "",
	/* CA */ "Get Data (ISO7816-4, Cyberflex, SmartCafe, OpenPlatform)",
	/* CC */ "",
	/* CE */ "",
	/* D0 */ "Write Binary (ISO7816-4, GPK4000)",
	/* D2 */ "Write Record (ISO7816-4)",
	/* D4 */ "",
	/* D6 */ "Update Binary (ISO7816-4, GSM SIM, Cryptoflex, Cyberflex, GPK4000, MFC, TCOS2)",
	/* D8 */ "Put Key (SmartCafe, OpenPlatform), Load Key File (MFC), Set Card Status (GPK4000)",
	/* DA */ "Put Data (ISO7816-4, GPK4000, OpenPlatform)",
	/* DC */ "Update Record (ISO7816-4, GSM SIM, Cryptoflex, Cyberflex, GPK4000, MFC, TCOS2)",
	/* DE */ "Update Encrypted (Cryptoflex), Erase Card (GPK4000)",
	/* E0 */ "Create File (Cryptoflex, Cyberflex, GPK4000, MFC), Create ML (SmartCafe)",
	/* E2 */ "Append Record (ISO7816-4, Cryptoflex, Cyberflex, GPK4000, MFC)",
	/* E4 */ "Delete File (Cryptoflex, Cyberflex, MFC), Delete ML (SmartCafe), Delete (OpenPlatform)",
	/* E6 */ "Install (SmartCafe, OpenPlatform)",
	/* E8 */ "Load Applet (SmartCafe, OpenPlatform)",
	/* EA */ "Init Hashed (GPK4000)",
	/* EC */ "",
	/* EE */ "",
	/* F0 */ "Set Status (OpenPlatform)",
	/* F2 */ "Status (GSM SIM, Cyberflex), Get Status (OpenPlatform)",
	/* F4 */ "",
	/* F6 */ "",
	/* F8 */ "",
	/* FA */ "Sleep (GSM SIM, Cyberflex), Change Java ATR (Cyberflex)",
	/* FC */ "Change File ACL (Cyberflex)",
	/* FE */ "Get File ACL (Cyberflex)",
};

#define testreturn(text); \
	if(ret!=SC_EXIT_OK){ printf(text); goto exit; }

#define testretsw(text,sw0,sw1); \
	if( (ret!=SC_EXIT_OK) || (ci->sw[0]!=sw0) || (ci->sw[1]!=sw1) ) \
	{ printf(text); goto exit; }

#define testretsw0(text,sw0); \
	if( (ret!=SC_EXIT_OK) || (ci->sw[0]!=sw0) ) \
	{ printf(text); goto exit; }

#define testretsws(text); \
	scSmartcardSimpleProcessSW( ci, &i, NULL ); \
	if( (ret!=SC_EXIT_OK) || (i!=SC_SW_OK) ) \
	{ printf(text); goto exit; }

#define printarray( name, length, array ); \
	printf(name); \
	for( i=0; i<length; i++ ) printf(" %.2X",array[i]); \
	printf("\n");

#define printarray2( name, length, array ); \
	printf(name); \
	for( i=0; i<length; i++ ) printf("%.2X",array[i]); \
	printf("\n");

void typeToString( char *str, LONG type )
{
	printf("%s: ", str);
	switch( type ) {
	case SC_CARD_MULTIFLEX:
		printf("Schlumberger Multiflex\n");
		break;
	case SC_CARD_MULTIFLEX_3K:
		printf("Schlumberger Multiflex 3K\n");
		break;
	case SC_CARD_MULTIFLEX_8K:
		printf("Schlumberger Multiflex 8K\n");
		break;
	case SC_CARD_MULTIFLEX_8K_DES:
		printf("Schlumberger Multiflex 8K (with Full DES option)\n");
		break;
	case SC_CARD_MULTIFLEX_4K:
		printf("Schlumberger Multiflex 4K\n");
		break;
	case SC_CARD_CRYPTOFLEX:
		printf("Schlumberger Cryptoflex\n");
		break;
	case SC_CARD_CRYPTOFLEX_DES:
		printf("Schlumberger Cryptoflex (with Full DES option)\n");
		break;
	case SC_CARD_CRYPTOFLEX_KEYGEN:
		printf("Schlumberger Cryptoflex (with RSA key generation)\n");
		break;
	case SC_CARD_CRYPTOFLEX_8K:
		printf("Schlumberger Cryptoflex (with 8KB EEPROM)\n");
		break;
	case SC_CARD_CYBERFLEX:
		printf("Schlumberger Cyberflex\n");
		break;
	case SC_CARD_CYBERFLEX_CRYPTO:
		printf("Schlumberger Cyberflex Access Crypto\n");
		break;
	case SC_CARD_CYBERFLEX_AUGCRYPTO:
		printf("Schlumberger Cyberflex Access Augmented Crypto\n");
		break;
	case SC_CARD_PAYFLEX:
		printf("Schlumberger Payflex\n");
		break;
	case SC_CARD_PAYFLEX_1K_USER:
		printf("Schlumberger Payflex 1K User card\n");
		break;
	case SC_CARD_PAYFLEX_1K_SAM:
		printf("Schlumberger Payflex 1K SAM\n");
		break;
	case SC_CARD_PAYFLEX_4K_USER:
		printf("Schlumberger Payflex 4K User card\n");
		break;
	case SC_CARD_PAYFLEX_4K_SAM:
		printf("Schlumberger Payflex 4K SAM\n");
		break;
	case SC_CARD_PAYFLEX_MICRO:
		printf("Schlumberger MicroPayflex\n");
		break;
	case SC_CARD_GPK4000:
		printf("Gemplus GPK4000\n");
		break;
	case SC_CARD_GPK4000_S:
		printf("Gemplus GPK4000-s\n");
		break;
	case SC_CARD_GPK4000_SP:
		printf("Gemplus GPK4000-sp\n");
		break;
	case SC_CARD_GPK4000_SDO:
		printf("Gemplus GPK4000-sdo\n");
		break;
	case SC_CARD_GPK2000:
		printf("Gemplus GPK2000\n");
		break;
	case SC_CARD_GPK2000_S:
		printf("Gemplus GPK2000-s\n");
		break;
	case SC_CARD_GPK2000_SP:
		printf("Gemplus GPK2000-sp\n");
		break;
	case SC_CARD_MPCOS_EMV:
		printf("Gemplus EMV\n");
		break;
	case SC_CARD_MPCOS_EMV_1B:
		printf("Gemplus EMV (1-Byte data units)\n");
		break;
	case SC_CARD_MPCOS_EMV_4B:
		printf("Gemplus EMV (4-Byte data units)\n");
		break;
	case SC_CARD_GELDKARTE:
		printf("Geldkarte\n");
		break;
	case SC_CARD_TCOS:
		printf("Telesec TCOS 2 card\n");
		break;
	case SC_CARD_TCOS_44:
		printf("Telesec TCOS 2 card (SLE44)\n");
		break;
	case SC_CARD_TCOS_66:
		printf("Telesec TCOS 2 card (SLE66)\n");
		break;
	case SC_CARD_BASICCARD:
		printf("BasicCard\n");
		break;
	case SC_CARD_BASICCARD_COMP:
		printf("Compact BasicCard\n");
		break;
	case SC_CARD_BASICCARD_ENH:
		printf("Enhanced BasicCard\n");
		break;
	case SC_CARD_BASICCARD_ENH_3:
		printf("Enhanced BasicCard v.3\n");
		break;
	case SC_CARD_BRADESCO:
		printf("Moeda Eletronica Bradesco\n");
		break;
	case SC_CARD_GSMSIM:
		printf("GSM SIM card\n");
		break;
	case SC_CARD_PROTON:
		printf("Proton card\n");
		break;
	case SC_CARD_PROTON_CASH:
		printf("Swiss Cash card (based on Proton)\n");
		break;
	case SC_CARD_PROTON_CHIPKNIP:
		printf("Dutch Chipknip card (based on Proton)\n");
		break;
	case SC_CARD_STARCOS:
		printf("Giesecke & Devrient STARCOS\n");
		break;
	case SC_CARD_STARCOS_S21:
		printf("Giesecke & Devrient STARCOS S2.1\n");
		break;
	case SC_CARD_STARCOS_SPK22:
		printf("Giesecke & Devrient STARCOS SPK2.2\n");
		break;
	case SC_CARD_STARCOS_SPK23:
		printf("Giesecke & Devrient STARCOS SPK2.3\n");
		break;
	case SC_CARD_SMARTCAFE:
		printf("Giesecke & Devrient Sm@rtCafe\n");
		break;
	case SC_CARD_SMARTCAFE_11:
		printf("Giesecke & Devrient Sm@rtCafe 1.1\n");
		break;
	case SC_CARD_MFC:
		printf("IBM/Comcard MFC\n");
		break;
	case SC_CARD_MFC_41:
		printf("IBM/Comcard MFC 4.1\n");
		break;
	case SC_CARD_GPK8000:
		printf("Gemplus GPK8000\n");
		break;
	case SC_CARD_GPK8000_8K:
		printf("Gemplus GPK8000\n");
		break;
	case SC_CARD_GPK8000_16K:
		printf("Gemplus GPK16000\n");
		break;
	case SC_CARD_UNKNOWN:
	default:
		printf("Unknown\n");
		break;
	}

	return;
}

int testCyberflex( SC_READER_INFO *ri, SC_CARD_INFO *ci )
{
	BYTE buffer[258];
	int ret, resplen;

	ci->cla=0x00;
	ret = scCyberflexCmdGetData( ri, ci, buffer, &resplen );
	if( (ret==SC_EXIT_OK) && (ci->sw[0]!=0x90) && (ci->sw[1]==0x00) ) {
		ci->cla=0xF0;
		ret = scCyberflexCmdGetData( ri, ci, buffer, &resplen );
	}
	if( (ret!=SC_EXIT_OK) || (ci->sw[0]!=0x90) || (ci->sw[1]!=0x00) )
		return( FALSE );
	if( resplen!=22 ) return( FALSE );

	ret = scCyberflexCmdSelect( ri, ci, SC_CYBERFLEX_SELECT_FILE, 0x3F00,
		NULL, 0, buffer, &resplen );
	if( (ret!=SC_EXIT_OK) || (ci->sw[0]!=0x90) || (ci->sw[1]!=0x00) )
		return( FALSE );
	if( (buffer[6]!=0x01) || (buffer[4]!=0x3F) || (buffer[5]!=0x00) )
		return( FALSE );

	ret = scCyberflexCmdDirectory( ri, ci, 0, buffer, &resplen );
	if( (ret!=SC_EXIT_OK) || (ci->sw[0]!=0x90) || (ci->sw[1]!=0x00) )
		return( FALSE );
	if( (buffer[6]!=0x01) || (buffer[4]!=0x3F) || (buffer[5]!=0x00) )
		return( FALSE );

	return( TRUE );
}

int testMultiflex( SC_READER_INFO *ri, SC_CARD_INFO *ci )
{
	BYTE buffer[258];
	int ret, resplen;

	/* Select MF */
	ret = scMultiflexCmdSelectFile( ri, ci, 0x3F00, buffer, &resplen );
	if( (ret!=SC_EXIT_OK) || (ci->sw[0]!=0x90) || (ci->sw[1]!=0x00) )
		return( FALSE );

	/* Select EF */
	ret = scMultiflexCmdSelectFile( ri, ci, 0x0002, buffer, &resplen );
	if( (ret!=SC_EXIT_OK) || (ci->sw[0]!=0x90) || (ci->sw[1]!=0x00) )
		return( FALSE );

	/* Read */
	resplen=8;
	ret = scMultiflexCmdReadBinary( ri, ci, 0, buffer, &resplen );
	if( (ret!=SC_EXIT_OK) || (ci->sw[0]!=0x90) || (ci->sw[1]!=0x00) )
		return( FALSE );
	if( resplen!=8 ) return( FALSE );

	/* Select MF */
	ret = scMultiflexCmdSelectFile( ri, ci, 0x3F00, buffer, &resplen );
	if( (ret!=SC_EXIT_OK) || (ci->sw[0]!=0x90) || (ci->sw[1]!=0x00) )
		return( FALSE );

	return( TRUE );
}

int testSmartcafe( SC_READER_INFO *ri, SC_CARD_INFO *ci )
{
	BYTE buffer[258];
	int ret, resplen;

	ret = scSmartcafeCmdGetData( ri, ci, SC_SMARTCAFE_TAG_VERSION,
		buffer, &resplen );
	if( (ret!=SC_EXIT_OK) || (ci->sw[0]!=0x90) || (ci->sw[1]!=0x00) )
		return( FALSE );
	if( resplen<9 ) return( FALSE );

	ret = scSmartcafeCmdGetData( ri, ci, SC_SMARTCAFE_TAG_SERIAL,
		buffer, &resplen );
	if( (ret!=SC_EXIT_OK) || (ci->sw[0]!=0x90) || (ci->sw[1]!=0x00) )
		return( FALSE );
	if( resplen!=10 ) return( FALSE );

	/* Select ML */
	ret = scSmartcafeCmdSelect( ri, ci, "SM@RT CAFE 11 ML",
		strlen("SM@RT CAFE 11 ML"), buffer, &resplen );
	if( (ret!=SC_EXIT_OK) || (ci->sw[0]!=0x90) || (ci->sw[1]!=0x00) )
		return( FALSE );
	if( resplen<39 ) return( FALSE );
	if( (buffer[4]!=0x53) || (buffer[5]!=0x4D) || (buffer[6]!=0x40) )
		return( FALSE );

	return( TRUE );
}

int testTcos( SC_READER_INFO *ri, SC_CARD_INFO *ci )
{
	BYTE buffer[258];
	int ret, resplen;

	resplen = 8;
	ret = scTcosCmdAskRandom( ri, ci, buffer, &resplen );
	if( (ret!=SC_EXIT_OK) || (ci->sw[0]!=0x90) || (ci->sw[1]!=0x00) )
		return( FALSE );
	if( resplen!=8 ) return( FALSE );

	ret = scTcosCmdSelect( ri, ci, SC_TCOS_SELECT_MF, 0, NULL, 0,
		SC_TCOS_RDATA_FCI, buffer, &resplen );
	if( (ret!=SC_EXIT_OK) || (ci->sw[0]!=0x90) || (ci->sw[1]!=0x00) )
		return( FALSE );
	if( resplen<30 ) return( FALSE );
	if( (buffer[6]!=0x82) || (buffer[4]!=0x3F) || (buffer[5]!=0x00) )
		return( FALSE );

	/* Select EF(GDO) */
	ret = scTcosCmdSelect( ri, ci, SC_TCOS_SELECT_ABS_PATH, 0, "\x2F\x02",
		2, SC_TCOS_RDATA_FCI, buffer, &resplen );
	if( (ret!=SC_EXIT_OK) || (ci->sw[0]!=0x90) || (ci->sw[1]!=0x00) )
		return( FALSE );
	if( resplen<30 ) return( FALSE );
	if( (buffer[6]!=0x81) || (buffer[4]!=0x2F) || (buffer[5]!=0x02) )
		return( FALSE );

	/* List Dir, MSE */

	return( TRUE );
}

int fingerPrint( SC_READER_INFO *ri, SC_CARD_INFO *ci )
{
	int i, j, ret, rsplen;
	BYTE rsp[256+2];
	char atr[3+65+4];
	FILE *fptr;

	printf( "Producing Fingerprintoutput:\n" );

	if( ci->atrlen>32 ) return( SC_EXIT_BAD_PARAM );

	strcpy( atr, "fp-" );
	for( i=0; i<ci->atrlen; i++ ) {
		sprintf( atr+(i*2)+3, "%.2X", ci->atr[i] );
	}
	strcat( atr, ".bin" );

	fptr = fopen( atr, "wb" );
	if( fptr==NULL ) return( SC_EXIT_FILE_ERROR );

	for( j=0; j<=0xFF; j+=4 ) {
		/* Should be checked too. The problem with 6 and 9 arises only when
		 * the INS byte begins with them.
		 * if( j==0x60 || j==0x90 ) j+=0x10;
		 */

		printf( "%.2X:", j );

		if( (ret=checkSelMf1( ri, ci, (BYTE) j, rsp, &rsplen ))!=SC_EXIT_OK ) {
			printf( " E%d\n", ret );
			fwrite( "\x00\x00", 1, 2, fptr );
			continue;
		}

		printarray( "", rsplen, rsp );
		if( rsplen>=2 ) fwrite( rsp+rsplen-2, 1, 2, fptr );
		else fwrite( "\x00\x00", 1, 2, fptr );

		fflush( stdout );
	}

	printf( "Fingerprint finished.\n" );

	fclose( fptr );

	return( SC_EXIT_OK );
}

int scanClaIns( SC_READER_INFO *ri, SC_CARD_INFO *ci, BYTE_256 *data )
{
	BYTE cmd[]={ 0x00, 0x00, 0x00, 0x00, 0x00 };
	SC_APDU apdu;

	int ret, i;
	BYTE buffer[255];
	int errors=0;
	int validcmd=FALSE;

	data->len=0;

	apdu.cse=SC_APDU_CASE_2_SHORT;
	apdu.cmd=cmd;
	apdu.cmdlen=5;
	apdu.rsp=buffer;
	apdu.rsplen=0;

	/* scReaderSendAPDU */
	while(1) {

		apdu.cse=SC_APDU_CASE_2_SHORT;

		/* Reset Card */
		ret= scReaderResetCard( ri, ci );
		if( ret!=SC_EXIT_OK ) {
			if( errors++>10 ) {
				printf("To many errors while doing CLA INS scan.\n");
				scReaderSendAPDU( ri, ci, &apdu );
				return( 1 );
			}
		}

		ret=scReaderSendAPDU( ri, ci, &apdu );

		if( apdu.rsplen>=2 ) {
			ci->sw[0] = apdu.rsp[apdu.rsplen-2];
			ci->sw[1] = apdu.rsp[apdu.rsplen-1];
		}

		/* 6E and 6D mean with a very high probability, that with this
		 * CLA/INS there is no valid instruction. 6F seams to be used
		 * as a obfuscated 6E or 6D.
		 */
		if( (ret==SC_EXIT_OK) && (ci->sw[0]!=0x6E) && (ci->sw[0]!=0x6D) &&
			(ci->sw[0]!=0x6F) )
		{
			printf( "%.2X %.2X: %.2X%.2X (%s)", cmd[0], cmd[1], ci->sw[0],
				ci->sw[1], cmddesc[ cmd[1]>>1 ] );
			if( apdu.rsplen>2 ) {
				printarray( " Rsp:", apdu.rsplen-2, apdu.rsp );
			} else printf( "\n" );
			validcmd=TRUE;
		}
		if( ret!=SC_EXIT_OK ) {
			printf( "%.2X %.2X: r=%2d (%s)\n", cmd[0], cmd[1], ret,
				cmddesc[ cmd[1]>>1 ] );
		}

		cmd[1]+=2;

#if !defined(DANGER)
		/* To protect GPK4000s */
		if( cmd[0]==0xDB && cmd[1]==0xDE ) {
			printf("%.2X %.2X: skiped (!defined(DANGER))\n", cmd[0], cmd[1] );
			cmd[1]+=2;
		}
#endif /* !DANGER */

		if(cmd[1]==0x60) { cmd[1]=0x70; }
		if(cmd[1]==0x90) { cmd[1]=0xA0; }
		if(cmd[1]==0) {
			if( validcmd ) data->data[ data->len++ ]=cmd[0];
			validcmd=FALSE;
			cmd[0]+=4;
		}

		fflush(stdout);

		if((cmd[0]==0) && (cmd[1]==0)) break;

	}

	return( 0 );
}

/* CLA A4 00 00 00 (MF) */

int checkSelMf1( SC_READER_INFO *ri, SC_CARD_INFO *ci, BYTE cla, BYTE *rsp,
	int *rsplen )
{
	BYTE cmd[]={ cla, 0xA4, 0x00, 0x00, 0x00 };
	BYTE saveswav[5], saveheader[5];
	SC_APDU apdu;
	int ret, i;
	BYTE buffer[255];

	if( rsplen!=NULL ) *rsplen=0;

	if( rsp==NULL || rsplen==NULL )
		printf( "Trying strategy 1 for selecting MF:" );

	apdu.cse=SC_APDU_CASE_2_SHORT;
	apdu.cmd=cmd;
	apdu.cmdlen=5;
	apdu.rsp=buffer;
	apdu.rsplen=0;

	/* Reset Card */
	ret= scReaderResetCard( ri, ci );
	if( ret!=SC_EXIT_OK ) {
		if( rsp==NULL || rsplen==NULL )
			printf( " Failed (Error while rest).\n" );
		return( ret );
	}

	/* Saving Get Response command and setting own command. */
	memcpy( saveheader, ci->t0.getrsp, 5 );
	memcpy( ci->t0.getrsp, "\x00\xC0\x00\x00\x00", 5 );
	ci->t0.getrsp[0] = cla;
	memcpy( saveswav, ci->swav, 5 );
	memcpy( ci->swav, "\x02\x61\x9F\x00\x00", 5 );

	ret=scReaderSendAPDU( ri, ci, &apdu );

	/* Restoring Get Response command */
	memcpy( ci->t0.getrsp, saveheader, 5 );
	memcpy( ci->swav, saveswav, 5 );

	if( (ret!=SC_EXIT_OK) || (apdu.rsplen<2) ) {
		if( rsp==NULL || rsplen==NULL )
			printf( " Failed (Error while sending APDU).\n" );
		return( ret );
	}

	ci->sw[0] = apdu.rsp[apdu.rsplen-2];
	ci->sw[1] = apdu.rsp[apdu.rsplen-1];

	if( rsplen!=NULL && rsp!=NULL ) {
		if( apdu.rsplen>=2 && apdu.rsplen<=258 ) {
			memcpy( rsp, apdu.rsp, apdu.rsplen );
			*rsplen = apdu.rsplen;
			return( SC_EXIT_OK );
		}
		return( SC_EXIT_UNKNOWN_ERROR );
	}

	if( (ci->sw[0]!=0x90) || (ci->sw[1]!=0x00) )
	{
		printf( " Failed (SW=%.2X%.2X).\n", ci->sw[0], ci->sw[1] );
		return( SC_EXIT_FILE_ERROR );
	}

	printf( " Ok." );

	if( apdu.rsplen>2 )
		for( i=0; i<apdu.rsplen-2; i++ ) printf( " %.2X", apdu.rsp[i] );

	printf( "\n" );

	return( SC_EXIT_OK );
}

/* CLA A4 08 00 00 (Absolut Path) */

int checkSelMf2( SC_READER_INFO *ri, SC_CARD_INFO *ci, BYTE cla, BYTE *rsp,
	int *rsplen )
{
	BYTE cmd[]={ cla, 0xA4, 0x08, 0x00, 0x00 };
	BYTE saveswav[5], saveheader[5];
	SC_APDU apdu;
	int ret, i;
	BYTE buffer[255];

	if( rsplen!=NULL ) *rsplen=0;

	if( rsp==NULL || rsplen==NULL )
		printf( "Trying strategy 2 for selecting MF:" );

	apdu.cse=SC_APDU_CASE_2_SHORT;
	apdu.cmd=cmd;
	apdu.cmdlen=5;
	apdu.rsp=buffer;
	apdu.rsplen=0;

	/* Reset Card */
	ret= scReaderResetCard( ri, ci );
	if( ret!=SC_EXIT_OK ) {
		if( rsp==NULL || rsplen==NULL )
			printf( " Failed (Error while rest).\n" );
		return( ret );
	}

	/* Saving Get Response command and setting own command. */
	memcpy( saveheader, ci->t0.getrsp, 5 );
	memcpy( ci->t0.getrsp, "\x00\xC0\x00\x00\x00", 5 );
	ci->t0.getrsp[0] = cla;
	memcpy( saveswav, ci->swav, 5 );
	memcpy( ci->swav, "\x02\x61\x9F\x00\x00", 5 );

	ret=scReaderSendAPDU( ri, ci, &apdu );

	/* Restoring Get Response command */
	memcpy( ci->t0.getrsp, saveheader, 5 );
	memcpy( ci->swav, saveswav, 5 );

	if( (ret!=SC_EXIT_OK) || (apdu.rsplen<2) ) {
		if( rsp==NULL || rsplen==NULL )
			printf( " Failed (Error while sending APDU).\n" );
		return( ret );
	}

	ci->sw[0] = apdu.rsp[apdu.rsplen-2];
	ci->sw[1] = apdu.rsp[apdu.rsplen-1];

	if( rsplen!=NULL && rsp!=NULL ) {
		if( apdu.rsplen>=2 && apdu.rsplen<=258 ) {
			memcpy( rsp, apdu.rsp, apdu.rsplen );
			*rsplen = apdu.rsplen;
			return( SC_EXIT_OK );
		}
		return( SC_EXIT_UNKNOWN_ERROR );
	}

	if( (ci->sw[0]!=0x90) || (ci->sw[1]!=0x00) )
	{
		printf( " Failed (SW=%.2X%.2X).\n", ci->sw[0], ci->sw[1] );
		return( SC_EXIT_FILE_ERROR );
	}

	printf( " Ok." );

	if( apdu.rsplen>2 )
		for( i=0; i<apdu.rsplen-2; i++ ) printf( " %.2X", apdu.rsp[i] );

	printf( "\n" );

	return( SC_EXIT_OK );
}

/* CLA A4 00 00 02 3F 00 00 (FID) */

int checkSelMf3( SC_READER_INFO *ri, SC_CARD_INFO *ci, BYTE cla, BYTE *rsp,
	int *rsplen )
{
	BYTE cmd[]={ cla, 0xA4, 0x00, 0x00, 0x02, 0x3F, 0x00, 0x00 };
	BYTE saveswav[5], saveheader[5];
	SC_APDU apdu;
	int ret, i;
	BYTE buffer[255];

	if( rsplen!=NULL ) *rsplen=0;

	if( rsp==NULL || rsplen==NULL )
		printf( "Trying strategy 3 for selecting MF:" );

	apdu.cse=SC_APDU_CASE_4_SHORT;
	apdu.cmd=cmd;
	apdu.cmdlen=8;
	apdu.rsp=buffer;
	apdu.rsplen=0;

	/* Reset Card */
	ret= scReaderResetCard( ri, ci );
	if( ret!=SC_EXIT_OK ) {
		if( rsp==NULL || rsplen==NULL )
			printf( " Failed (Error while rest).\n" );
		return( ret );
	}

	/* Saving Get Response command and setting own command. */
	memcpy( saveheader, ci->t0.getrsp, 5 );
	memcpy( ci->t0.getrsp, "\x00\xC0\x00\x00\x00", 5 );
	ci->t0.getrsp[0] = cla;
	memcpy( saveswav, ci->swav, 5 );
	memcpy( ci->swav, "\x02\x61\x9F\x00\x00", 5 );

	ret=scReaderSendAPDU( ri, ci, &apdu );

	/* Restoring Get Response command */
	memcpy( ci->t0.getrsp, saveheader, 5 );
	memcpy( ci->swav, saveswav, 5 );

	if( (ret!=SC_EXIT_OK) || (apdu.rsplen<2) ) {
		if( rsp==NULL || rsplen==NULL )
			printf( " Failed (Error while sending APDU).\n" );
		return( ret );
	}

	ci->sw[0] = apdu.rsp[apdu.rsplen-2];
	ci->sw[1] = apdu.rsp[apdu.rsplen-1];

	if( rsplen!=NULL && rsp!=NULL ) {
		if( apdu.rsplen>=2 && apdu.rsplen<=258 ) {
			memcpy( rsp, apdu.rsp, apdu.rsplen );
			*rsplen = apdu.rsplen;
			return( SC_EXIT_OK );
		}
		return( SC_EXIT_UNKNOWN_ERROR );
	}

	if( (ci->sw[0]!=0x90) || (ci->sw[1]!=0x00) )
	{
		printf( " Failed (SW=%.2X%.2X).\n", ci->sw[0], ci->sw[1] );
		return( SC_EXIT_FILE_ERROR );
	}

	printf( " Ok." );

	if( apdu.rsplen>2 )
		for( i=0; i<apdu.rsplen-2; i++ ) printf( " %.2X", apdu.rsp[i] );

	printf( "\n" );

	return( SC_EXIT_OK );
}

/* CLA A4 01 00 02 3F 00 00 (FID) */

int checkSelMf4( SC_READER_INFO *ri, SC_CARD_INFO *ci, BYTE cla )
{
	BYTE cmd[]={ cla, 0xA4, 0x01, 0x00, 0x02, 0x3F, 0x00, 0x00 };
	BYTE saveswav[5], saveheader[5];
	SC_APDU apdu;
	int ret, i;
	BYTE buffer[255];

	printf( "Trying strategy 4 for selecting MF:" );

	apdu.cse=SC_APDU_CASE_4_SHORT;
	apdu.cmd=cmd;
	apdu.cmdlen=8;
	apdu.rsp=buffer;
	apdu.rsplen=0;

	/* Reset Card */
	ret= scReaderResetCard( ri, ci );
	if( ret!=SC_EXIT_OK ) {
		printf( " Failed (Error while rest).\n" );
		return( ret );
	}

	/* Saving Get Response command and setting own command. */
	memcpy( saveheader, ci->t0.getrsp, 5 );
	memcpy( ci->t0.getrsp, "\x00\xC0\x00\x00\x00", 5 );
	ci->t0.getrsp[0] = cla;
	memcpy( saveswav, ci->swav, 5 );
	memcpy( ci->swav, "\x02\x61\x9F\x00\x00", 5 );

	ret=scReaderSendAPDU( ri, ci, &apdu );

	/* Restoring Get Response command */
	memcpy( ci->t0.getrsp, saveheader, 5 );
	memcpy( ci->swav, saveswav, 5 );

	if( (ret!=SC_EXIT_OK) || (apdu.rsplen<2) ) {
		printf( " Failed (Error while sending APDU).\n" );
		return( ret );
	}

	ci->sw[0] = apdu.rsp[apdu.rsplen-2];
	ci->sw[1] = apdu.rsp[apdu.rsplen-1];

	if( (ci->sw[0]!=0x90) || (ci->sw[1]!=0x00) )
	{
		printf( " Failed (SW=%.2X%.2X).\n", ci->sw[0], ci->sw[1] );
		return( SC_EXIT_FILE_ERROR );
	}

	printf( " Ok." );

	if( apdu.rsplen>2 )
		for( i=0; i<apdu.rsplen-2; i++ ) printf( " %.2X", apdu.rsp[i] );

	printf( "\n" );

	return( SC_EXIT_OK );
}

/* CLA A4 02 00 02 3F 00 00 (FID) */

int checkSelMf5( SC_READER_INFO *ri, SC_CARD_INFO *ci, BYTE cla )
{
	BYTE cmd[]={ cla, 0xA4, 0x02, 0x00, 0x02, 0x3F, 0x00, 0x00 };
	BYTE saveswav[5], saveheader[5];
	SC_APDU apdu;
	int ret, i;
	BYTE buffer[255];

	printf( "Trying strategy 5 for selecting MF:" );

	apdu.cse=SC_APDU_CASE_4_SHORT;
	apdu.cmd=cmd;
	apdu.cmdlen=8;
	apdu.rsp=buffer;
	apdu.rsplen=0;

	/* Reset Card */
	ret= scReaderResetCard( ri, ci );
	if( ret!=SC_EXIT_OK ) {
		printf( " Failed (Error while rest).\n" );
		return( ret );
	}

	/* Saving Get Response command and setting own command. */
	memcpy( saveheader, ci->t0.getrsp, 5 );
	memcpy( ci->t0.getrsp, "\x00\xC0\x00\x00\x00", 5 );
	ci->t0.getrsp[0] = cla;
	memcpy( saveswav, ci->swav, 5 );
	memcpy( ci->swav, "\x02\x61\x9F\x00\x00", 5 );

	ret=scReaderSendAPDU( ri, ci, &apdu );

	/* Restoring Get Response command */
	memcpy( ci->t0.getrsp, saveheader, 5 );
	memcpy( ci->swav, saveswav, 5 );

	if( (ret!=SC_EXIT_OK) || (apdu.rsplen<2) ) {
		printf( " Failed (Error while sending APDU).\n" );
		return( ret );
	}

	ci->sw[0] = apdu.rsp[apdu.rsplen-2];
	ci->sw[1] = apdu.rsp[apdu.rsplen-1];

	if( (ci->sw[0]!=0x90) || (ci->sw[1]!=0x00) )
	{
		printf( " Failed (SW=%.2X%.2X).\n", ci->sw[0], ci->sw[1] );
		return( SC_EXIT_FILE_ERROR );
	}

	printf( " Ok." );

	if( apdu.rsplen>2 )
		for( i=0; i<apdu.rsplen-2; i++ ) printf( " %.2X", apdu.rsp[i] );

	printf( "\n" );

	return( SC_EXIT_OK );
}

/* CLA A4 00 0C (MF) */

int checkSelMf6( SC_READER_INFO *ri, SC_CARD_INFO *ci, BYTE cla )
{
	BYTE cmd[]={ cla, 0xA4, 0x00, 0x0C };
	BYTE saveswav[5], saveheader[5];
	SC_APDU apdu;
	int ret, i;
	BYTE buffer[255];

	printf( "Trying strategy 6 for selecting MF:" );

	apdu.cse=SC_APDU_CASE_1;
	apdu.cmd=cmd;
	apdu.cmdlen=4;
	apdu.rsp=buffer;
	apdu.rsplen=0;

	/* Reset Card */
	ret= scReaderResetCard( ri, ci );
	if( ret!=SC_EXIT_OK ) {
		printf( " Failed (Error while rest).\n" );
		return( ret );
	}

	/* Saving Get Response command and setting own command. */
	memcpy( saveheader, ci->t0.getrsp, 5 );
	memcpy( ci->t0.getrsp, "\x00\xC0\x00\x00\x00", 5 );
	ci->t0.getrsp[0] = cla;
	memcpy( saveswav, ci->swav, 5 );
	memcpy( ci->swav, "\x02\x61\x9F\x00\x00", 5 );

	ret=scReaderSendAPDU( ri, ci, &apdu );

	/* Restoring Get Response command */
	memcpy( ci->t0.getrsp, saveheader, 5 );
	memcpy( ci->swav, saveswav, 5 );

	if( (ret!=SC_EXIT_OK) || (apdu.rsplen<2) ) {
		printf( " Failed (Error while sending APDU).\n" );
		return( ret );
	}

	ci->sw[0] = apdu.rsp[apdu.rsplen-2];
	ci->sw[1] = apdu.rsp[apdu.rsplen-1];

	if( (ci->sw[0]!=0x90) || (ci->sw[1]!=0x00) )
	{
		printf( " Failed (SW=%.2X%.2X).\n", ci->sw[0], ci->sw[1] );
		return( SC_EXIT_FILE_ERROR );
	}

	printf( " Ok." );

	if( apdu.rsplen>2 )
		for( i=0; i<apdu.rsplen-2; i++ ) printf( " %.2X", apdu.rsp[i] );

	printf( "\n" );

	return( SC_EXIT_OK );
}

/* CLA A4 08 0C (Absolut Path) */

int checkSelMf7( SC_READER_INFO *ri, SC_CARD_INFO *ci, BYTE cla )
{
	BYTE cmd[]={ cla, 0xA4, 0x08, 0x0C };
	BYTE saveswav[5], saveheader[5];
	SC_APDU apdu;
	int ret, i;
	BYTE buffer[255];

	printf( "Trying strategy 7 for selecting MF:" );

	apdu.cse=SC_APDU_CASE_1;
	apdu.cmd=cmd;
	apdu.cmdlen=4;
	apdu.rsp=buffer;
	apdu.rsplen=0;

	/* Reset Card */
	ret= scReaderResetCard( ri, ci );
	if( ret!=SC_EXIT_OK ) {
		printf( " Failed (Error while rest).\n" );
		return( ret );
	}

	/* Saving Get Response command and setting own command. */
	memcpy( saveheader, ci->t0.getrsp, 5 );
	memcpy( ci->t0.getrsp, "\x00\xC0\x00\x00\x00", 5 );
	ci->t0.getrsp[0] = cla;
	memcpy( saveswav, ci->swav, 5 );
	memcpy( ci->swav, "\x02\x61\x9F\x00\x00", 5 );

	ret=scReaderSendAPDU( ri, ci, &apdu );

	/* Restoring Get Response command */
	memcpy( ci->t0.getrsp, saveheader, 5 );
	memcpy( ci->swav, saveswav, 5 );

	if( (ret!=SC_EXIT_OK) || (apdu.rsplen<2) ) {
		printf( " Failed (Error while sending APDU).\n" );
		return( ret );
	}

	ci->sw[0] = apdu.rsp[apdu.rsplen-2];
	ci->sw[1] = apdu.rsp[apdu.rsplen-1];

	if( (ci->sw[0]!=0x90) || (ci->sw[1]!=0x00) )
	{
		printf( " Failed (SW=%.2X%.2X).\n", ci->sw[0], ci->sw[1] );
		return( SC_EXIT_FILE_ERROR );
	}

	printf( " Ok." );

	if( apdu.rsplen>2 )
		for( i=0; i<apdu.rsplen-2; i++ ) printf( " %.2X", apdu.rsp[i] );

	printf( "\n" );

	return( SC_EXIT_OK );
}

/* CLA A4 00 0C 02 3F 00 (FID) */

int checkSelMf8( SC_READER_INFO *ri, SC_CARD_INFO *ci, BYTE cla )
{
	BYTE cmd[]={ cla, 0xA4, 0x00, 0x0C, 0x02, 0x3F, 0x00 };
	BYTE saveswav[5], saveheader[5];
	SC_APDU apdu;
	int ret, i;
	BYTE buffer[255];

	printf( "Trying strategy 8 for selecting MF:" );

	apdu.cse=SC_APDU_CASE_3_SHORT;
	apdu.cmd=cmd;
	apdu.cmdlen=7;
	apdu.rsp=buffer;
	apdu.rsplen=0;

	/* Reset Card */
	ret= scReaderResetCard( ri, ci );
	if( ret!=SC_EXIT_OK ) {
		printf( " Failed (Error while rest).\n" );
		return( ret );
	}

	/* Saving Get Response command and setting own command. */
	memcpy( saveheader, ci->t0.getrsp, 5 );
	memcpy( ci->t0.getrsp, "\x00\xC0\x00\x00\x00", 5 );
	ci->t0.getrsp[0] = cla;
	memcpy( saveswav, ci->swav, 5 );
	memcpy( ci->swav, "\x02\x61\x9F\x00\x00", 5 );

	ret=scReaderSendAPDU( ri, ci, &apdu );

	/* Restoring Get Response command */
	memcpy( ci->t0.getrsp, saveheader, 5 );
	memcpy( ci->swav, saveswav, 5 );

	if( (ret!=SC_EXIT_OK) || (apdu.rsplen<2) ) {
		printf( " Failed (Error while sending APDU).\n" );
		return( ret );
	}

	ci->sw[0] = apdu.rsp[apdu.rsplen-2];
	ci->sw[1] = apdu.rsp[apdu.rsplen-1];

	if( (ci->sw[0]!=0x90) || (ci->sw[1]!=0x00) )
	{
		printf( " Failed (SW=%.2X%.2X).\n", ci->sw[0], ci->sw[1] );
		return( SC_EXIT_FILE_ERROR );
	}

	printf( " Ok." );

	if( apdu.rsplen>2 )
		for( i=0; i<apdu.rsplen-2; i++ ) printf( " %.2X", apdu.rsp[i] );

	printf( "\n" );

	return( SC_EXIT_OK );
}

/* CLA A4 01 0C 02 3F 00 (FID) */

int checkSelMf9( SC_READER_INFO *ri, SC_CARD_INFO *ci, BYTE cla )
{
	BYTE cmd[]={ cla, 0xA4, 0x01, 0x0C, 0x02, 0x3F, 0x00 };
	BYTE saveswav[5], saveheader[5];
	SC_APDU apdu;
	int ret, i;
	BYTE buffer[255];

	printf( "Trying strategy 9 for selecting MF:" );

	apdu.cse=SC_APDU_CASE_3_SHORT;
	apdu.cmd=cmd;
	apdu.cmdlen=7;
	apdu.rsp=buffer;
	apdu.rsplen=0;

	/* Reset Card */
	ret= scReaderResetCard( ri, ci );
	if( ret!=SC_EXIT_OK ) {
		printf( " Failed (Error while rest).\n" );
		return( ret );
	}

	/* Saving Get Response command and setting own command. */
	memcpy( saveheader, ci->t0.getrsp, 5 );
	memcpy( ci->t0.getrsp, "\x00\xC0\x00\x00\x00", 5 );
	ci->t0.getrsp[0] = cla;
	memcpy( saveswav, ci->swav, 5 );
	memcpy( ci->swav, "\x02\x61\x9F\x00\x00", 5 );

	ret=scReaderSendAPDU( ri, ci, &apdu );

	/* Restoring Get Response command */
	memcpy( ci->t0.getrsp, saveheader, 5 );
	memcpy( ci->swav, saveswav, 5 );

	if( (ret!=SC_EXIT_OK) || (apdu.rsplen<2) ) {
		printf( " Failed (Error while sending APDU).\n" );
		return( ret );
	}

	ci->sw[0] = apdu.rsp[apdu.rsplen-2];
	ci->sw[1] = apdu.rsp[apdu.rsplen-1];

	if( (ci->sw[0]!=0x90) || (ci->sw[1]!=0x00) )
	{
		printf( " Failed (SW=%.2X%.2X).\n", ci->sw[0], ci->sw[1] );
		return( SC_EXIT_FILE_ERROR );
	}

	printf( " Ok." );

	if( apdu.rsplen>2 )
		for( i=0; i<apdu.rsplen-2; i++ ) printf( " %.2X", apdu.rsp[i] );

	printf( "\n" );

	return( SC_EXIT_OK );
}

/* CLA A4 02 0C 02 3F 00 (FID) */

int checkSelMf10( SC_READER_INFO *ri, SC_CARD_INFO *ci, BYTE cla )
{
	BYTE cmd[]={ cla, 0xA4, 0x02, 0x0C, 0x02, 0x3F, 0x00 };
	BYTE saveswav[5], saveheader[5];
	SC_APDU apdu;
	int ret, i;
	BYTE buffer[255];

	printf( "Trying strategy 10 for selecting MF:" );

	apdu.cse=SC_APDU_CASE_3_SHORT;
	apdu.cmd=cmd;
	apdu.cmdlen=7;
	apdu.rsp=buffer;
	apdu.rsplen=0;

	/* Reset Card */
	ret= scReaderResetCard( ri, ci );
	if( ret!=SC_EXIT_OK ) {
		printf( " Failed (Error while rest).\n" );
		return( ret );
	}

	/* Saving Get Response command and setting own command. */
	memcpy( saveheader, ci->t0.getrsp, 5 );
	memcpy( ci->t0.getrsp, "\x00\xC0\x00\x00\x00", 5 );
	ci->t0.getrsp[0] = cla;
	memcpy( saveswav, ci->swav, 5 );
	memcpy( ci->swav, "\x02\x61\x9F\x00\x00", 5 );

	ret=scReaderSendAPDU( ri, ci, &apdu );

	/* Restoring Get Response command */
	memcpy( ci->t0.getrsp, saveheader, 5 );
	memcpy( ci->swav, saveswav, 5 );

	if( (ret!=SC_EXIT_OK) || (apdu.rsplen<2) ) {
		printf( " Failed (Error while sending APDU).\n" );
		return( ret );
	}

	ci->sw[0] = apdu.rsp[apdu.rsplen-2];
	ci->sw[1] = apdu.rsp[apdu.rsplen-1];

	if( (ci->sw[0]!=0x90) || (ci->sw[1]!=0x00) )
	{
		printf( " Failed (SW=%.2X%.2X).\n", ci->sw[0], ci->sw[1] );
		return( SC_EXIT_FILE_ERROR );
	}

	printf( " Ok." );

	if( apdu.rsplen>2 )
		for( i=0; i<apdu.rsplen-2; i++ ) printf( " %.2X", apdu.rsp[i] );

	printf( "\n" );

	return( SC_EXIT_OK );
}

/* CLA A4 00 00 02 3F 00 (FID) */

int checkSelMf11( SC_READER_INFO *ri, SC_CARD_INFO *ci, BYTE cla )
{
	BYTE cmd[]={ cla, 0xA4, 0x00, 0x00, 0x02, 0x3F, 0x00 };
	BYTE saveswav[5], saveheader[5];
	SC_APDU apdu;
	int ret, i;
	BYTE buffer[255];

	printf( "Trying strategy 11 for selecting MF:" );

	apdu.cse=SC_APDU_CASE_3_SHORT;
	apdu.cmd=cmd;
	apdu.cmdlen=7;
	apdu.rsp=buffer;
	apdu.rsplen=0;

	/* Reset Card */
	ret= scReaderResetCard( ri, ci );
	if( ret!=SC_EXIT_OK ) {
		printf( " Failed (Error while rest).\n" );
		return( ret );
	}

	/* Saving Get Response command and setting own command. */
	memcpy( saveheader, ci->t0.getrsp, 5 );
	memcpy( ci->t0.getrsp, "\x00\xC0\x00\x00\x00", 5 );
	ci->t0.getrsp[0] = cla;
	memcpy( saveswav, ci->swav, 5 );
	memcpy( ci->swav, "\x02\x61\x9F\x00\x00", 5 );

	ret=scReaderSendAPDU( ri, ci, &apdu );

	/* Restoring Get Response command */
	memcpy( ci->t0.getrsp, saveheader, 5 );
	memcpy( ci->swav, saveswav, 5 );

	if( (ret!=SC_EXIT_OK) || (apdu.rsplen<2) ) {
		printf( " Failed (Error while sending APDU).\n" );
		return( ret );
	}

	ci->sw[0] = apdu.rsp[apdu.rsplen-2];
	ci->sw[1] = apdu.rsp[apdu.rsplen-1];

	if( (ci->sw[0]!=0x90) || (ci->sw[1]!=0x00) )
	{
		printf( " Failed (SW=%.2X%.2X).\n", ci->sw[0], ci->sw[1] );
		return( SC_EXIT_FILE_ERROR );
	}

	printf( " Ok." );

	if( apdu.rsplen>2 )
		for( i=0; i<apdu.rsplen-2; i++ ) printf( " %.2X", apdu.rsp[i] );

	printf( "\n" );

	return( SC_EXIT_OK );
}

int checkSelMf( SC_READER_INFO *ri, SC_CARD_INFO *ci, BYTE cla, LONG *strategies )
{
	*strategies = 0;

	if( checkSelMf1( ri, ci, cla, NULL, NULL )==SC_EXIT_OK )
		*strategies |= 0x00000001;
	if( checkSelMf2( ri, ci, cla, NULL, NULL )==SC_EXIT_OK )
		*strategies |= 0x00000002;
	if( checkSelMf3( ri, ci, cla, NULL, NULL )==SC_EXIT_OK )
		*strategies |= 0x00000004;
	if( checkSelMf4( ri, ci, cla )==SC_EXIT_OK ) *strategies |= 0x00000008;
	if( checkSelMf5( ri, ci, cla )==SC_EXIT_OK ) *strategies |= 0x00000010;
	if( checkSelMf6( ri, ci, cla )==SC_EXIT_OK ) *strategies |= 0x00000020;
	if( checkSelMf7( ri, ci, cla )==SC_EXIT_OK ) *strategies |= 0x00000040;
	if( checkSelMf8( ri, ci, cla )==SC_EXIT_OK ) *strategies |= 0x00000080;
	if( checkSelMf9( ri, ci, cla )==SC_EXIT_OK ) *strategies |= 0x00000100;
	if( checkSelMf10( ri, ci, cla )==SC_EXIT_OK ) *strategies |= 0x00000200;
	if( checkSelMf11( ri, ci, cla )==SC_EXIT_OK ) *strategies |= 0x00000400;

	fflush(stdout);

	return( SC_EXIT_OK );
}

int main( int argc, char *argv[] )
{
	SC_READER_INFO *ri;
	SC_CARD_INFO *ci;
	SC_READER_CONFIG rc;

	BYTE_256 data256;
	LONG selstrats[3] = { 0, 0, 0 };

	int ret;
	int i;

	printf( "WARNING!\n" );
	printf( "\n" );
	printf( "THIS PROGRAM CAN MAKE A CARD UNUSABLE!\n" );
	printf( "\n" );

#if defined(__WIN32__) || defined(__WIN16__)
	Sleep(3000);
#else
	sleep(3);
#endif /* !__WIN32__ && !__WIN16__ */

	if( scInit() ) { printf("Exit.\n"); return(1); }

	rc.type=READER_TYPE;
	rc.slot=READER_SLOT;
	rc.param=READER_PORT;

	ret = scReaderGetConfig( argc, argv, &rc );
	if( ret!=SC_EXIT_OK ) {
		printf( "Error getting reader configuration.\n" );
		scEnd();
		return(1);
	};

	ri = scGeneralNewReader( rc.type, rc.slot );
	if( ri==NULL ) { printf("Memory error.\n"); scEnd(); return(1); }

	ci = scGeneralNewCard( );
	if( ci==NULL ) { printf("Memory error.\n"); scEnd(); return(1); }

	/* Init Reader */
	ret = scReaderInit( ri, rc.param );
	if( ret!=SC_EXIT_OK ) {
		printf("Initialization error.\n");
		return(1);
	}

	/* Get Card Status */
	ret = scReaderCardStatus( ri );
	if( ret!=SC_EXIT_OK ) {
		printf("Status error.\n");
		return(1);
	}
	if( !(ri->status&SC_CARD_STATUS_PRESENT) ) {
		printf("No card present.\n");
		return(1);
	}

	/* Activate Card */
	ret = scReaderActivate( ri );
	testreturn("Card activation error.\n");

#if 0
SIO_SetLogFile( ri->si, "LogScscan.txt" );
#endif

	/* Reset Card */
	ret= scReaderResetCard( ri, ci );
	testreturn("Card reset error.\n");

	/* Get Card Type */
	ret = scSmartcardGetCardType( ci );

	printarray( "ATR:", ci->atrlen, ci->atr );

	typeToString( "Card type from ATR", ci->type );

	switch( ci->protocol ) {
	case SC_PROTOCOL_T0:
		printf("Protocol: T=0\n");
		break;
	case SC_PROTOCOL_T1:
		printf("Protocol: T=1\n");
		break;
	default:
		printf("Protocol: Unknown\n");
		break;
	}

	printf( "Card type from commands:" );
	if( testCyberflex( ri, ci ) ) printf( " Cyberflex" );
	if( testMultiflex( ri, ci ) ) printf( " Multiflex" );
	if( testSmartcafe( ri, ci ) ) printf( " Smartcafe" );
	if( testTcos( ri, ci ) ) printf( " TCOS2" );
	printf( "\n" );

	fingerPrint( ri, ci );

	printf("WARNING! THE NEXT TEST REALLY MAY DAMAGE YOUR CARD!\n");
	printf("YOU HAVE 10 SECONDS TO ABORT THIS PROGRAMM WITH CTRL-C!\n");
#if defined(__WIN32__) || defined(__WIN16__)
	Sleep(10000);
#else
	sleep(10);
#endif /* !__WIN32__ && !__WIN16__ */
	printf("Doing CLA INS scan. This may run from halve an hour up to some hours.\n");
	data256.len=0;
	if( scanClaIns( ri, ci, &data256 )==0 )
		printf("CLA INS scan successfully completed.\n");
	else
		printf("CLA INS scan not successfully completed.\n");

	printarray("CLAs:", data256.len, data256.data );

	printf( "Trying selection strategies:\n" );
	if( data256.len ) {
		for( i=0; i<data256.len; i++ ) {
			printf( "CLA: %.2X\n", data256.data[i] );
			checkSelMf( ri, ci, data256.data[i], &selstrats[0] );
		}
	}

	printf( "Done.\n" );

exit:
	scReaderDeactivate( ri );
	scReaderShutdown( ri );

	scGeneralFreeCard( &ci );
	scGeneralFreeReader( &ri );

	scEnd();

	fflush(stdout);

	return(0);
}


