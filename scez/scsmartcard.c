/****************************************************************************
*																			*
*					SCEZ chipcard library - Smart Card routines				*
*					Copyright Matthias Bruestle 1999,2000					*
*					For licensing, see the file COPYRIGHT					*
*																			*
****************************************************************************/

/* $Id: scsmartcard.c 1617 2005-11-03 17:41:39Z laforge $ */

#include <scez/scinternal.h>
#ifdef WITH_BASICCARD
#include <scez/cards/scbasiccard.h>
#endif /* WITH_BASICCARD */
#ifdef WITH_CRYPTOFLEX
#include <scez/cards/sccryptoflex.h>
#endif /* WITH_CRYPTOFLEX */
#ifdef WITH_CYBERFLEX
#include <scez/cards/sccyberflex.h>
#endif /* WITH_CYBERFLEX */
#ifdef WITH_GELDKARTE
#include <scez/cards/scgeldkarte.h>
#endif /* WITH_GELDKARTE */
#ifdef WITH_GPK4000
#include <scez/cards/scgpk4000.h>
#endif /* WITH_GPK4000 */
#ifdef WITH_GSMSIM
#include <scez/cards/scgsmsim.h>
#endif /* WITH_GSMSIM */
#ifdef WITH_MFC
#include <scez/cards/scmfc.h>
#endif /* WITH_MFC */
#ifdef WITH_MULTIFLEX
#include <scez/cards/scmultiflex.h>
#endif /* WITH_MULTIFLEX */
#ifdef WITH_PAYFLEX
#include <scez/cards/scpayflex.h>
#endif /* WITH_PAYFLEX */
#ifdef WITH_PROTON
#include <scez/cards/scproton.h>
#endif /* WITH_PROTON */
#ifdef WITH_QUICK
#include <scez/cards/scquick.h>
#endif /* WITH_QUICK */
#ifdef WITH_SMARTCAFE
#include <scez/cards/scsmartcafe.h>
#endif /* WITH_SMARTCAFE */
#ifdef WITH_STARCOS
#include <scez/cards/scstarcos.h>
#endif /* WITH_STARCOS */
#ifdef WITH_TCOS
#include <scez/cards/sctcos.h>
#endif /* WITH_TCOS */

#include <stdio.h>
#include <string.h>

#if defined(WINDOWS) && !defined(__BORLANDC__)
#include <memory.h>
#endif

static ATR_VALUE atrTable[] = {
#if defined( WITH_MULTIFLEX )
	{ (const BYTE *) "\x3B\x02\x14\x50",
		NULL, 4, SC_CARD_MULTIFLEX_3K },
	{ (const BYTE *) "\x3B\x19\x14\x55\x90\x01\x02\x01\x00\x05\x04\xB0",
		NULL, 12, SC_CARD_MULTIFLEX_4K },
	{ (const BYTE *) "\x3B\x32\x15\x00\x06\x80",
		NULL, 6, SC_CARD_MULTIFLEX_8K },
	{ (const BYTE *) "\x3B\x32\x15\x00\x06\x95",
		NULL, 6, SC_CARD_MULTIFLEX_8K_DES },
	{ (const BYTE *) "\x3B\x19\x14\x59\x01\x01\x0F\x01\x00\x05\x08\xB0",
		NULL, 12, SC_CARD_MULTIFLEX_8K },
	{ (const BYTE *) "\x3B\x19\x14\x55\x90\x01\x01\x01\x00\x05\x08\xB0",
		NULL, 12, SC_CARD_MULTIFLEX_8K },
	/* Matthias Eckermann, mge, 20010212, 
		suggestion for MULTIFLEX_4/8K:
		-- or did I misunderstand the "mask"??
	{ (const BYTE *) "\x3B\x19\x14\x55\x90\x01\x02\x01\x00\x05\x04\xB0",
	  (const BYTE *) "\x01\x01\x01\x00\x00\x00\x00\x00\x01\x01\x01\x01",
		NULL, 12, SC_CARD_MULTIFLEX_4K },
	{ (const BYTE *) "\x3B\x19\x14\x59\x01\x01\x0F\x01\x00\x05\x08\xB0",
	  (const BYTE *) "\x01\x01\x01\x00\x00\x00\x00\x00\x01\x01\x01\x01",
		NULL, 12, SC_CARD_MULTIFLEX_8K },
	*/
#endif /* WITH_MULTIFLEX */
#if defined( WITH_CRYPTOFLEX )
	{ (const BYTE *) "\x3B\xE2\x00\x00\x40\x20\x49\x06",
		NULL, 8, SC_CARD_CRYPTOFLEX },
	{ (const BYTE *) "\x3B\xE2\x00\x00\x40\x20\x49\x05",
		NULL, 8, SC_CARD_CRYPTOFLEX_DES },
	{ (const BYTE *) "\x3B\xE2\x00\x00\x40\x20\x49\x07",
		NULL, 8, SC_CARD_CRYPTOFLEX_KEYGEN },
	{ (const BYTE *) "\x3B\x85\x40\x20\x68\x01\x01\x03\x05",
		NULL, 9, SC_CARD_CRYPTOFLEX_KEYGEN },
	{ (const BYTE *) "\x3B\x85\x40\x20\x68\x01\x01\x05\x01",
		NULL, 9, SC_CARD_CRYPTOFLEX_8K },
	{ (const BYTE *) "\x3B\x95\x94\x40\xFF\x63\x01\x01\x02\x01",
		NULL, 10, SC_CARD_CRYPTOFLEX_16K },
#endif /* WITH_CRYPTOFLEX */
#if defined( WITH_CYBERFLEX )
	{ (const BYTE *) "\x3B\x16\x94\x81\x10\x06\x01\x81\x3F",
		NULL, 9, SC_CARD_CYBERFLEX_CRYPTO },
	{ (const BYTE *) "\x3B\x16\x94\x81\x10\x06\x01\x81\x2F",
		NULL, 9, SC_CARD_CYBERFLEX_AUGCRYPTO },
#endif /* WITH_CYBERFLEX */
#if defined( WITH_CYBERFLEX2 )
	{ (const BYTE *) "\x3B\x17\x13\x9C\x12\x02\x01\x01\x07\40",
		NULL, 10, SC_CARD_CYBERFLEX2_DEV_32K },
#endif /* WITH_CYBERFLEX2 */
#if defined( WITH_PAYFLEX )
	{ (const BYTE *) "\x3B\x23\x00\x35\x11\x80",
		(const BYTE *) "\x01\x01\x01\x01\x00\x01",
		6, SC_CARD_PAYFLEX_1K_USER },
	{ (const BYTE *) "\x3B\x23\x00\x35\x11\x81",
		(const BYTE *) "\x01\x01\x01\x01\x00\x01",
		6, SC_CARD_PAYFLEX_1K_SAM },
	{ (const BYTE *) "\x3B\x63\x00\x00\x36\x41\x80",
		(const BYTE *) "\x01\x01\x01\x01\x01\x00\x01",
		7, SC_CARD_PAYFLEX_4K_USER },
	{ (const BYTE *) "\x3B\x23\x00\x00\x36\x41\x81",
		(const BYTE *) "\x01\x01\x01\x01\x01\x00\x01",
		7, SC_CARD_PAYFLEX_4K_SAM },
	{ (const BYTE *) "\x3B\x23\x00\x35\x13\xFF",
		(const BYTE *) "\x01\x01\x01\x01\x00\x01",
		6, SC_CARD_PAYFLEX_MICRO },
#endif /* WITH_PAYFLEX */
#if defined( WITH_GPK4000 )
	{ (const BYTE *) "\x3B\x27\x00\x80\x65\xA2\x04\x01\x01\x37",
		NULL, 10, SC_CARD_GPK4000_S },
	{ (const BYTE *) "\x3B\x27\x00\x80\x65\xA2\x05\x01\x01\x37",
		NULL, 10, SC_CARD_GPK4000_SP },
	{ (const BYTE *) "\x3B\x27\x00\x80\x65\xA2\x0C\x01\x01\x37",
		NULL, 10, SC_CARD_GPK4000_SU256 },
	{ (const BYTE *) "\x3B\xA7\x00\x40\x14\x80\x65\xA2\x14\x01\x01\x37",
		NULL, 12, SC_CARD_GPK4000_SDO },
	{ (const BYTE *) "\x3B\x29\x00\x80\x72\xA4\x45\x64\x00\x00\xD0\x15",
		NULL, 12, SC_CARD_BRADESCO },
#endif /* WITH_GPK4000 */
#if defined( WITH_GPK2000 )
	{ (const BYTE *) "\x3B\x27\x00\x80\x65\xA2\x02\x02\x82\x37",
		NULL, 10, SC_CARD_GPK2000_S },
	{ (const BYTE *) "\x3B\x27\x00\x80\x65\xA2\x02\x03\x82\x37",
		NULL, 10, SC_CARD_GPK2000_SP },
#endif /* WITH_GPK2000 */
#if defined( WITH_MPCOS )
	{ (const BYTE *) "\x3B\x2A\x00\x80\x65\xA2\x01\x00\x00\x00\x72\xD6\x41",
		(const BYTE *) "\x01\x01\x01\x01\x01\x01\x01\x00\x00\x00\x01\x01\x01",
		13, SC_CARD_MPCOS_EMV_1B },
	{ (const BYTE *) "\x3B\x2A\x00\x80\x65\xA2\x01\x00\x00\x00\x72\xD6\x43",
		(const BYTE *) "\x01\x01\x01\x01\x01\x01\x01\x00\x00\x00\x01\x01\x01",
		13, SC_CARD_MPCOS_EMV_4B },
#endif /* WITH_MPCOS */
#if defined( WITH_GELDKARTE )
	/* Geldkarte v3 */
	/* E.g: Student card FH Frankfurt: 3b ff 18 00 ff 81 31 3c 45 65 63 0d 02 31 02 50 00 10 90 01 44 00 04 10 6a */
	{ (const BYTE *) "\x3B\xFF\x18\x00\xFF\x81\x31\x50\x45\x65\x63\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00",
		(const BYTE *) "\x01\x01\x01\x01\x01\x01\x01\x00\x01\x01\x01\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00",
		25, SC_CARD_GELDKARTE_3 },
	/* Geldkarte v2 */
	{ (const BYTE *) "\x3B\xEF\x00\xFF\x81\x31\x50\x45\x65\x63\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00",
		(const BYTE *) "\x01\x01\x01\x01\x01\x01\x00\x01\x01\x01\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00",
		24, SC_CARD_GELDKARTE },
#endif /* WITH_GELDKARTE */
#if defined( WITH_TCOS )
	{ (const BYTE *) "\x3B\xBA\x13\x00\x81\x31\x86\x5D\x00\x64\x05\x0A\x02\x01\x31\x80\x90\x00\x8B",
		NULL, 19, SC_CARD_TCOS_44 },
	{ (const BYTE *) "\x3B\xBA\x14\x00\x81\x31\x86\x5D\x00\x64\x05\x14\x02\x02\x31\x80\x90\x00\x91",
		NULL, 19, SC_CARD_TCOS_66 },
	{ (const BYTE *) "\x3B\xBA\x96\x00\x81\x31\x86\x5D\x00\x64\x05\x60\x02\x03\x31\x80\x90\x00\x66",
		NULL, 19, SC_CARD_TCOS_66P },
#endif /* WITH_TCOS */
#if defined( WITH_BASICCARD )
	{ (const BYTE *) "\x3B\xEF\x00\xFF\x81\x31\x50\x45\x42\x61\x73\x69\x63\x43\x61\x72\x64\x20\x5A\x43\x31\x2E\x31\xCC",
		(const BYTE *) "\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x00\x00",
		24, SC_CARD_BASICCARD_COMP },
	{ (const BYTE *) "\x3B\xEF\x00\xFF\x81\x31\x20\x45\x42\x61\x73\x69\x63\x43\x61\x72\x64\x20\x5A\x43\x32\x2E\x33\xBD",
		(const BYTE *) "\x01\x01\x01\x01\x01\x01\x00\x00\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x00\x00",
		24, SC_CARD_BASICCARD_ENH },
	{ (const BYTE *) "\x3B\xEF\x00\xFF\x81\x31\x20\x75\x42\x61\x73\x69\x63\x43\x61\x72\x64\x20\x5A\x43\x33\x2E\x33\x8C",
		(const BYTE *) "\x01\x01\x01\x01\x01\x01\x00\x00\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x00\x00",
		24, SC_CARD_BASICCARD_ENH_3 },
#endif /* WITH_BASICCARD */
#if defined( WITH_GSMSIM )
	/* T-Mobil D1 GSM (900MHz) */
	{ (const BYTE *) "\x3B\x83\x00\x12\x10\x96",
		NULL, 6, SC_CARD_GSMSIM },
	/* T-Mobil D1 GSM (900MHz) */
	{ (const BYTE *) "\x3B\x8A\x00\x91\x01\x00\x16\x00\x01\x16\x01\x00\x96",
		(const BYTE *) "\x01\x01\x01\x01\x01\x01\x01\x01\x01\x00\x00\x01\x01",
		13, SC_CARD_GSMSIM },
	/* Victorvox D1 GSM (900MHz) */
	{ (const BYTE *) "\x3B\x85\x00\x12\x02\x01\x00\x96",
		NULL, 8, SC_CARD_GSMSIM },
	/* Victorvox D1 GSM (900MHz) */
	{ (const BYTE *) "\x3B\x9A\x94\x00\x91\x01\x00\x17\x00\x01\x23\x10\x00\x96",
		NULL, 14, SC_CARD_GSMSIM },
	/* D2 CallYa GSM (900MHz) */
	{ (const BYTE *) "\x3F\x2F\x00\x80\x69\xAF\x02\x04\x01\x36\x00\x02\x0A\x0E\x83\x3E\x9F\x16",
		NULL, 18, SC_CARD_GSMSIM },
	/* Debitel D2 GSM (900MHz) */
	{ (const BYTE *) "\x3F\x2F\x00\x80\x69\xAF\x03\x07\x03\x52\x00\x0D\x0A\x0E\x83\x3E\x9F\x16",
		NULL, 18, SC_CARD_GSMSIM },
	/* e-plus GSM (1800MHz) */
	{ (const BYTE *) "\x3F\x2F\x00\x80\x69\xAE\x02\x02\x01\x36\x00\x00\x0A\x0E\x83\x3E\x9F\x16",
		NULL, 18, SC_CARD_GSMSIM },
	/* Viag Interkom E2 GSM (1800MHz) */
	{ (const BYTE *) "\x3B\x85\x00\x87\x25\x01\x38\x02",
		NULL, 8, SC_CARD_GSMSIM },
	/* Viag Interkom E2 Loop GSM (1800MHz) */
    /*  3B BA 94 00 40 14 "GG3RS716S0" ??? */
	/*  3B BA 94 00 40 14 "GG3RS732S0" ??? */
	/* diAx Swiss (aka sunrise) pronto */
	/*  3B BA 94 00 40 14 "GG3RS716S0" ??? */
	{ (const BYTE *) "\x3B\xBA\x94\x00\x40\x14\x47\x47\x33\x52\x53\x37\x31\x36\x53\x30",
		(const BYTE *) "\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x00\x00\x01\x01",
		16, SC_CARD_GSMSIM },
	/* French GSM SIM card (900MHz) */
	{ (const BYTE *) "\x3F\x69\x00\x00\x24\xAF\x01\x70\x01\x01\xFF\x90\x00",
		NULL, 13, SC_CARD_GSMSIM },
	/* BEN GSM (1800MHz) */
	{ (const BYTE *) "\x3B\x0A\x20\x62\x0C\x01\x4F\x53\x45\x99\x14\xAA",
		NULL, 12, SC_CARD_GSMSIM },
	/* Libertel GSM (900MHz) */
	{ (const BYTE *) "\x3B\xAA\x00\x40\x14\x47\x47\x32\x47\x54\x35\x53\x34\x38\x30",
		NULL, 15, SC_CARD_GSMSIM },
#endif /* WITH_GSMSIM */
#if defined( WITH_PROTON )
	/* Swiss Cash card */
	{ (const BYTE *) "\x3F\x67\x25\x00\x2A\x20\x00\x40\x68\x9F\x00",
		NULL, 11, SC_CARD_PROTON_CASH },
	/* New Swiss Cash card */
	{ (const BYTE *) "\x3B\x67\x00\x00\x2D\x20\x36\x00\x78\x90\x00",
		NULL, 11, SC_CARD_PROTON_CASH },
	/* Dutch ChipKnip */
	{ (const BYTE *) "\x3F\x67\x25\x00\x2A\x20\x00\x41\x68\x90\x00",
		NULL, 11, SC_CARD_PROTON_CHIPKNIP },
#endif /* WITH_PROTON */
#if defined( WITH_STARCOS )
	{ (const BYTE *) "\x3B\xBF\x18\x00\x81\x31\x70\x55\x53\x54\x41\x52\x43\x4F\x53\x20\x53\x32\x31\x20\x43\x90\x00\xFA",
		NULL, 24, SC_CARD_STARCOS_S21 },
	{ (const BYTE *) "\x3B\xB7\x94\x00\x81\x31\xFE\x65\x53\x50\x4B\x32\x32\x90\x00\xD0",
		(const BYTE *) "\x01\x01\x01\x01\x01\x01\x01\x00\x01\x01\x01\x01\x01\x01\x01\x00",
		16, SC_CARD_STARCOS_SPK22 },
	{ (const BYTE *) "\x3B\xB7\x18\x00\x81\x31\xFE\x65\x53\x50\x4B\x32\x34\x90\x00\x5A",
		NULL, 16, SC_CARD_STARCOS_SPK24 },
#endif /* WITH_STARCOS */
#if defined( WITH_SMARTCAFE )
	{ (const BYTE *) "\x3B\xBF\x11\x00\xC0\x10\x31\xFE\x44\x53\x4D\x40\x52\x54\x20\x43\x41\x46\x45\x20\x31\x2E\x31\x43\xC1",
		NULL, 25, SC_CARD_SMARTCAFE_11 },
#endif /* WITH_SMARTCAFE */
#if defined( WITH_MFC )
	{ (const BYTE *) "\x3B\xEF\x00\xFF\x81\x31\x86\x45\x49\x42\x4D\x20\x4D\x46\x43\x34\x30\x30\x30\x30\x38\x33\x31\x43",
		(const BYTE *) "\x01\x01\x01\x01\x01\x01\x00\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x00\x01\x01\x01\x01\x00",
		24, SC_CARD_MFC_41 },
#endif /* WITH_MFC */
#if defined( WITH_GPK8000 )
	{ (const BYTE *) "\x3B\xA7\x00\x40\x18\x80\x65\xA2\x08\x01\x01\x52",
		(const BYTE *) "\x01\x01\x01\x01\x01\x01\x00\x01\x01\x01\x01\x00",
		12, SC_CARD_GPK8000_8K },
	{ (const BYTE *) "\x3B\xA7\x00\x40\x18\x80\x65\xA2\x09\x01\x01\x52",
		(const BYTE *) "\x01\x01\x01\x01\x01\x01\x00\x01\x01\x01\x01\x00",
		12, SC_CARD_GPK8000_16K },
#endif /* WITH_GPK8000 */
#ifdef WITH_QUICK
	/* Austrian Quick E-purse */
	{ (const BYTE *) "\x3B\xBF\x11\x00\x81\x31\x45\x45\x45\x50\x41\x00\x00\x00\x00\x10\x36\x68\x47\x00\x00\x00\x00\x43",
		(const BYTE *) "\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x00\x00\x00\x00\x00\x00\x00\x00\x01\x01\x01\x01\x00",
		24, SC_CARD_QUICK },
#endif /* WITH_QUICK */
#ifdef WITH_GEMXPRESSO
	/* Gemplus GemXpresso 211PK */
	{ (const BYTE *) "\x3B\xAD\x00\x40\xFF\x80\x31\x80\x65\xB0\x05\x01\x01\x5E\x83\x00\x90\x00",
		NULL, 18, SC_CARD_GEMXPRESSO_211PK },
#endif /* WITH_GEMXPRESSO */
#ifdef WITH_CARTEB
	/* Card Bancaire */
	{ (const BYTE *) "\x3F\x65\x25\x08\x33\x04\x6C\x90\x00",
		"\x01\x01\x01\x01\x00\x01\x01\x01\x01", 9, SC_CARD_CARTEB },
#endif /* WITH_CARTEB */
#ifdef WITH_JIB
	/* Dallas Semiconductor iButton */
	{ (const BYTE *) "\x00\x8F\x0E\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x04\x00\x00\x34\x90",
		NULL, 18, SC_CARD_JIB },
#endif /* WITH_JIB */
#if 0
	/* Dutch Post (Chipper) */
	{ (const BYTE *) "\x3B\xEB\x00\x00\x81\x31\x42\x45\x4E\x4C\x43\x68\x69\x70\x70\x65\x72\x30\x31\x0A",
		NULL, 20, SC_CARD_CHIPPER },
	/* Dutch University accesscard & Electronic purse & telphone card */
	{ (const BYTE *) "\x3B\xEB\x00\x00\x81\x31\x42\x45\x4E\x4C\x43\x68\x53\x43\x4B\x30\x34\x30\x31\x2B",
		NULL, 20, SC_CARD_UNKNOWN },
	/* Telekom Paycard */
	{ (const BYTE *) "\x3B\x87\x81\x31\x40\x43\x4D\x46\x43\x20\x31\x33\x31\x6F",
		NULL, 9, SC_CARD_PAYCARD },
	/* D-Trust Card */
	{ (const BYTE *) "\x3F\x65\x25\x08\x33\x04\x20\x90\x00",
		NULL, 9, SC_CARD_DTRUST },
	/* SamOS 2.7 */
	{ (const BYTE *) "\x3B\xB0\x11\x00\x81\x31\x90\x73\xF2",
		NULL, 9, SC_CARD_SAMOS_27 },
	/* Telesec TCOS 1.2 */
	{ (const BYTE *) "\x3B\xA8\x00\x81\x71\x46\x5D\x00\x54\x43\x4F\x53\x31\x2E\x32\x00\x65",
		NULL, 17, SC_CARD_TCOS1 },
	/* CeloCom Card with TCOS 1.2 */
	{ (const BYTE *) "\x3B\xA8\x00\x81\x71\x46\x5D\x00\x54\x43\x4F\x53\x31\x2E\x32\x4B\x2E",
		NULL, 17, SC_CARD_TCOS1 },
	/* UBS Internet Card (ATR:IBM JetZ M2) */
	{ (const BYTE *) "\x3B\xEF\x00\xFF\x81\x31\xFE\x45\x80\x31\xC0\x6B\x49\x42\x4D\x20\x4A\x65\x74\x5A\x20\x4D\x32\x39",
		NULL, 24, SC_CARD_UBS },
	/* SmarTEC */
	{ (const BYTE *) "\x3B\xE0\x00\x00\x81\x31\x20\x40\x30",
		NULL, 9, SC_CARD_SMARTEC },
	/* IBM eCash */
	{ (const BYTE *) "\x3B\xEF\x00\xFF\x81\x31\x66\x45\x65\x63\x20\x20\x49\x42\x4D\x20\x33\x2E\x31\x20\x20\x20\x20\xCF",
		NULL, 24, SC_CARD_ECASH },
	/* i.ti (ticket card for Collogne/Bonn) */
	{ (const BYTE *) "\x3B\x82\x81\x31\x76\x43\xC1\x03\xC5",
		NULL, 9, SC_CARD_ITI },
	/* GCOS-MDK */
	{ (const BYTE *) "\x3B\xEA\x00\xFF\x81\x31\x20\x75\x00\x64\x05\x14\x01\x02\x31\x00\x90\x00\x27",
		NULL, 19, SC_CARD_GCOSMDK },
	/* Siemens CardOS/M V1.4 */
	{ (const BYTE *) "\x3B\xB7\x11\x00\x81\x31\x90\x43\xA5\x17\x08\xA2\x0E\x04\x0C\xDB",
		NULL, 16, SC_CARD_CARDOS },
	/* Towitoko P64 SIgn&Crypt (Siemens CardOS) */
	{ (const BYTE *) "\x3B\xB7\x11\x00\x81\x31\x90\x43\xA5\x17\x0C\xD4\x17\x2B\x15\x86",
		NULL, 16, SC_CARD_CARDOS },
#endif /* 0 */
	{ NULL, NULL, 0 }
};

/* Determine the card type based on the ATR and fill data in ci */
/*
 * Stolen from scmisc.c from cryptlib.    Saenks!
 */

int scSmartcardGetCardType( SC_CARD_INFO *ci )
{
	return( scSmartcardGetCardTypeExt( ci, atrTable ) );
}

int scSmartcardGetCardTypeExt( SC_CARD_INFO *ci, const ATR_VALUE *atrTab )
{
	int i, ret;
	BOOLEAN found=FALSE;

	for( i = 0; atrTab[ i ].atr != NULL; i++ ) {
		const BYTE *atrMask = atrTab[ i ].atrMask;
		int length = atrTab[ i ].atrLength;

		if( length != ci->atrlen )
			continue;	/* Quick check for length match */
		if( atrMask == NULL ) {
			if( !memcmp(ci->atr, atrTab[ i ].atr, length)) {
				ci->type = atrTab[ i ].type;
				found=TRUE;
				break;
			}
		} else {
			int j;

			/* There's a mask for the ATR, compare only the bytes which
			   aren't masked out */
			for( j = 0; j < length; j++ )
				if( atrTab[ i ].atrMask[ j ] && \
					atrTab[ i ].atr[ j ] != ci->atr[ j ] )
					break;
			if( j == length ) {
				ci->type = atrTab[ i ].type;
				found=TRUE;
				break;
			}
		}
	}

	if( !found ) {
		ci->type = SC_CARD_UNKNOWN;
		return( scSmartcardProcessATR( ci ) );
	} else {
		if( (ret=scSmartcardInit( ci ))==SC_EXIT_OK ) {
			return( ci->scGetCardData( ci ) );
		} else if( ret==SC_EXIT_NOT_SUPPORTED ) {
			return( scSmartcardProcessATR( ci ) );
		} else {
			return( ret );
		}
	}
}

/* Process ATR and write results into ci */
/*
 * To set:
 *   protocol	TDi
 *   direct		TS
 *   wwt		TC2
 *   cwt		TBi i>2
 *   bwt		TBi i>2
 *   cs			TCi i>2
 *   ifsc		TAi i>2
 */

int scSmartcardProcessATR( SC_CARD_INFO *ci )
{
	int p;
	int	tdp;
	int i;
	int cks;

	/* Defaults */
	ci->direct=TRUE;
	ci->protocol=SC_PROTOCOL_T0;
	ci->t0.d=1;
	ci->t0.wi=10;
	ci->t0.wwt=9600;
	ci->t1.ifsc=32;
	ci->t1.cwt=8192;
	ci->t1.bwt=1600000;
	ci->t1.rc=SC_T1_CHECKSUM_LRC;

	/* Is it a JiB? */
	if( (ci->atr[0]==0x00) && ((ci->type&0xFF00)==SC_CARD_JIB) ) {
		ci->protocol=SC_PROTOCOL_1WIRE;
		return( SC_EXIT_OK );
	}

	if( ci->atr[0]==0x3F ) ci->direct=FALSE;

	tdp=1;
	p=2;

	/* TA1 */
	if( ci->atr[tdp]&0x10 ) p++;
	/* TB1 */
	if( ci->atr[tdp]&0x20 ) p++;
	/* TC1 */
	if( ci->atr[tdp]&0x40 ) p++;
	if( p >= ci->atrlen ) return( SC_EXIT_BAD_ATR );
	/* TD1 */
	if( ci->atr[tdp]&0x80 ) ci->protocol=ci->atr[p]&0x0F;
	else return( SC_EXIT_OK );

	tdp=p;
	p++;

	/* TA2 */
	if( ci->atr[tdp]&0x10 ) p++;
	/* TB2 */
	if( ci->atr[tdp]&0x20 ) p++;
	if( p >= ci->atrlen ) return( SC_EXIT_BAD_ATR );
	/* TC2 */
	if( ci->atr[tdp]&0x40 ) {
		ci->t0.wi=ci->atr[p];
		ci->t0.wwt=960*ci->t0.d*ci->t0.wi;
		p++;
	}
	if( p >= ci->atrlen ) return( SC_EXIT_BAD_ATR );
	/* TD2 */
	/* Use first protocol setting in TD1 */
	if( ci->atr[tdp]&0x80 ) { /* ci->protocol=ci->atr[p]&0x0F; */ }
	else return( SC_EXIT_OK );

	tdp=p;
	p++;

	if( p >= ci->atrlen ) return( SC_EXIT_BAD_ATR );
	/* TA3 */
	if( ci->atr[tdp]&0x10 ) {
		ci->t1.ifsc=ci->atr[p];
		p++;
	}
	if( p >= ci->atrlen ) return( SC_EXIT_BAD_ATR );
	/* TB3 */
	if( ci->atr[tdp]&0x20 ) {
		ci->t1.cwt=1 << ( ci->atr[p] & 0x0F );
		ci->t1.bwt=( 1 << ( (ci->atr[p]>>4) & 0x0F ) ) * 100000;
		p++;
	}
	if( p >= ci->atrlen ) return( SC_EXIT_BAD_ATR );
	/* TC3 */
	if( ci->atr[tdp]&0x40 ) {
		if( ci->atr[p]&0x01 ) ci->t1.rc=SC_T1_CHECKSUM_CRC;
		else ci->t1.rc=SC_T1_CHECKSUM_LRC;
		p++;
	}

	if( ci->protocol==SC_PROTOCOL_T1 ) {
		cks=0;
		p+=ci->atr[1]&0xF;
		for( i=1; i<=p; i++ ) cks^=ci->atr[i];
		if( cks ) return( SC_EXIT_BAD_ATR );
	}

	return( SC_EXIT_OK );
}

/* Process status word */

/* Returns only SC_SW_OK, SC_SW_DATA_AVAIL and SC_SW_UNKNOWN.
 * Uses ci->swok and ci->swav.
 */

int scSmartcardSimpleProcessSW( SC_CARD_INFO *ci, int *status, int *number )
{
	int i;

	if( (ci->swok[0]>4) || (ci->swav[0]>4) )
		return( SC_EXIT_BAD_PARAM );

	for( i=1; i<=ci->swok[0]; i++ ) {
		if( ci->sw[0]==ci->swok[i] ) {
			if( number!=NULL ) *number=0;
			*status=SC_SW_OK;
			return( SC_EXIT_OK );
		}
	}

	for( i=1; i<=ci->swav[0]; i++ ) {
		if( ci->sw[0]==ci->swav[i] ) {
			if( number!=NULL ) *number=ci->sw[1];
			*status=SC_SW_DATA_AVAIL;
			return( SC_EXIT_OK );
		}
	}

	*status=SC_SW_UNKNOWN;
	return( SC_EXIT_OK );
}

/* Initialize card function pointer */

int scSmartcardInit( SC_CARD_INFO *ci )
{
	switch( ci->type&0xFFFFFF00 )
	{
#ifdef WITH_BASICCARD
		case SC_CARD_BASICCARD:
			return( scBasiccardInit( ci ) );
#endif
#ifdef WITH_CRYPTOFLEX
		case SC_CARD_CRYPTOFLEX:
			return( scCryptoflexInit( ci ) );
#endif
#ifdef WITH_CYBERFLEX
		case SC_CARD_CYBERFLEX:
			return( scCyberflexInit( ci ) );
#endif
#ifdef WITH_GELDKARTE
		case SC_CARD_GELDKARTE:
			return( scGeldkarteInit( ci ) );
#endif
#ifdef WITH_GPK2000
		case SC_CARD_GPK2000:
			return( SC_EXIT_NOT_SUPPORTED );
#endif
#ifdef WITH_GPK4000
		case SC_CARD_GPK4000:
		case SC_CARD_BRADESCO:
		case SC_CARD_GPK8000:
			return( scGpk4000Init( ci ) );
#endif
#ifdef WITH_GSMSIM
		case SC_CARD_GSMSIM:
			return( scGsmsimInit( ci ) );
#endif
#ifdef WITH_MFC
		case SC_CARD_MFC:
			return( scMfcInit( ci ) );
#endif
#ifdef WITH_MPCOS
		case SC_CARD_MPCOS_EMV:
			return( SC_EXIT_NOT_SUPPORTED );
#endif
#ifdef WITH_MULTIFLEX
		case SC_CARD_MULTIFLEX:
			return( scMultiflexInit( ci ) );
#endif
#ifdef WITH_PAYFLEX
		case SC_CARD_PAYFLEX:
			return( scPayflexInit( ci ) );
#endif
#ifdef WITH_PROTON
		case SC_CARD_PROTON:
			return( scProtonInit( ci ) );
#endif
#ifdef WITH_QUICK
		case SC_CARD_QUICK:
			return( scQuickInit( ci ) );
#endif
#ifdef WITH_SMARTCAFE
		case SC_CARD_SMARTCAFE:
			return( scSmartcafeInit( ci ) );
#endif
#ifdef WITH_STARCOS
		case SC_CARD_STARCOS:
			return( scStarcosInit( ci ) );
#endif
#ifdef WITH_TCOS
		case SC_CARD_TCOS:
			return( scTcosInit( ci ) );
#endif
		case SC_CARD_VENDOR1:
		case SC_CARD_VENDOR2:
		case SC_CARD_VENDOR3:
		case SC_CARD_UNKNOWN:
		default:
			return( SC_EXIT_NOT_SUPPORTED );
	}

	return( SC_EXIT_UNKNOWN_ERROR );
}

/* Fill card data in cp */

int scSmartcardGetCap( SC_CARD_INFO *ci, SC_CARD_CAP *cp )
{
	if( ci->scGetCap==NULL ) return( SC_EXIT_NOT_SUPPORTED );
	return( ci->scGetCap( ci, cp ) );
}

/* Fill card data in ci */

int scSmartcardGetCardData( SC_CARD_INFO *ci )
{
	if( ci->scGetCardData==NULL ) return( SC_EXIT_NOT_SUPPORTED );
	return( ci->scGetCardData( ci ) );
}

/* Set F and D. Usable only directly after a reset. */

int scSmartcardSetFD( SC_READER_INFO *ri, SC_CARD_INFO *ci, LONG fd )
{
	if( ci->scSetFD==NULL ) return( SC_EXIT_NOT_SUPPORTED );
	return( ci->scSetFD( ri, ci, fd ) );
}

/* Sets transmission speed to card.
 * With most cards this can only be called directly aftern a reset.
 * Also with most cards if there is an error the card should be reseted.
 * It would be nice to test the necessary function pointers before
 * calling this function, so that no useless reset is done.
 * Option can be SC_SPEED_STANDARD, SC_SPEED_FAST and SC_SPEED_FASTEST.
 */

int scSmartcardSetSpeed( SC_READER_INFO *ri, SC_CARD_INFO *ci, int option )
{
	SC_READER_CAP *rp;
	SC_CARD_CAP *cp;
	int i, j, n_fd=0, bestfd=-1, ret=SC_EXIT_OK;
	LONG fd[MAX_FD_VALUES], speed[MAX_FD_VALUES], fast=60000;

	if( (ci->scGetCap==NULL) || (ci->scSetFD==NULL) ||
		(ri->scGetCap==NULL) || (ri->scSetSpeed==NULL) )
		return( SC_EXIT_NOT_SUPPORTED );

	rp=scGeneralNewReaderCap();
	cp=scGeneralNewCardCap();
	if( (cp==NULL) || (rp==NULL) ) {
		ret=SC_EXIT_MALLOC_ERROR;
		goto err;
	}

	if( (ret=scSmartcardGetCap( ci, cp ))!=SC_EXIT_OK ) goto err;
	if( (ret=scReaderGetCap( ri, rp ))!=SC_EXIT_OK ) goto err;

	for( i=0; i<cp->n_fd; i++ ) {
		for( j=0; j<rp->n_fd; j++ ) {
			if( cp->fd[i]==rp->fd[j] ) {
				fd[n_fd]=rp->fd[j];
				speed[n_fd++]=rp->speed[j];
			}
		}
	}

	if( n_fd==0 ) {
		ret=SC_EXIT_NO_MATCH;
		goto err;
	}

	switch( option ) {
		case SC_SPEED_STANDARD:
			for( i=0; i<n_fd; i++ ) {
				if( (fd[i]&0xFFFFFF)==((372L<<8)+1L) ) bestfd=i;
			}
			break;
		case SC_SPEED_FAST:
			/* Reduce save speed, if reader does no handle T=0 errors
			 * properly.
			 * Should also have info about FIFO etc..
			 */
			if( (ci->protocol==SC_PROTOCOL_T0) &&
				(rp->t0err==FALSE) ) fast=20000;

			for( i=0; i<n_fd; i++ ) {
				if( (speed[i]<=fast) && ((bestfd==-1) ||
					(speed[i]>speed[bestfd])) ) bestfd=i;
			}
			break;
		case SC_SPEED_FASTEST:
			bestfd=0;
			for( i=0; i<n_fd; i++ ) {
				if( speed[i]>speed[bestfd] ) bestfd=i;
			}
			break;
		default:
			ret=SC_EXIT_BAD_PARAM;
			goto err;
			break;
	}

	/* Set speed of card. */
	if( (ret=scSmartcardSetFD( ri, ci, fd[bestfd] ))!=SC_EXIT_OK ) goto err;

	/* Set speed of reader. */
	if( (ret=scReaderSetSpeed( ri, speed[bestfd] ))!=SC_EXIT_OK ) goto err;

	/* Change WWT in case of T=0. */
	if( ci->protocol==SC_PROTOCOL_T0 ) {
		ci->t0.d=fd[bestfd]&0xFF;
		if( ci->t0.d&0x80 ) {
			/* d is 1/d */
			ci->t0.wwt=(960*ci->t0.wi)/(100-ci->t0.d);
		} else {
			ci->t0.wwt=960*ci->t0.d*ci->t0.wi;
		}
	}

/* printf("[speed: %ld]",speed[bestfd]); */

	ret=SC_EXIT_OK;

err:
	scGeneralFreeReaderCap( &rp );
	scGeneralFreeCardCap( &cp );

	return( ret );
}

