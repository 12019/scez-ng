/****************************************************************************
*																			*
*					Copyright Matthias Bruestle 1999,2000					*
*					For licensing, see the file COPYRIGHT					*
*																			*
****************************************************************************/

/* $Id: scdir.c 1617 2005-11-03 17:41:39Z laforge $ */

/* #define DO_LOG */

#define WITH_BASICCARD
#define WITH_CRYPTOFLEX
#define WITH_CYBERFLEX
#define WITH_GELDKARTE
#define WITH_GPK4000
#define WITH_GSMSIM
#define WITH_MFC
#define WITH_MULTIFLEX
#define WITH_PAYFLEX
#define WITH_PROTON
#define WITH_QUICK
#define WITH_SMARTCAFE
#define WITH_TCOS

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#if !defined(__WIN32__) && !defined(__WIN16__)
#include <getopt.h>
#endif /* !__WIN32__ && !__WIN16__ */
#if defined(DO_LOG)
#include <sio/sio.h>
#endif
#include <scez/scgeneral.h>
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
#ifdef WITH_TCOS
#include <scez/cards/sctcos.h>
#endif /* WITH_TCOS */

#ifndef READER_TYPE
#define READER_TYPE SC_READER_DUMBMOUSE
#endif /* READER_TYPE */
#ifndef READER_SLOT
#define READER_SLOT 1
#endif /* READER_SLOT */
#ifndef READER_PORT
#define READER_PORT "0"
#endif /* READER_PORT */

/* AID values */

typedef struct {
	const BYTE *aid;		/* AID */
	const int aidlen;		/* Length of AID */
	const char *desc;		/* Description of Application */
	} AID_VALUE;

static const AID_VALUE aidTable[] = {
	{ (const BYTE *) "\x47\x54\x4F\x4B\x31", 5,
		"GemPlus GemSAFE (GTOK1)" },
	{ (const BYTE *) "\xA0\x00\x00\x00\x03\x00\x01", 7,
		"VOP Security Domain Class" },
	{ (const BYTE *) "\xA0\x00\x00\x00\x18\x0F\x00\x01\x63\x00\x01", 11,
		"GemPlus GemSAFE" },
	{ (const BYTE *) "\xA0\x00\x00\x00\x18\x43\x4D", 7,
		"VOP Default Manager Instance" },
	{ (const BYTE *) "\xA0\x00\x00\x00\x18\x56\x4F\x50", 8,
		"VOP Default Manager Package" },
	{ (const BYTE *) "\xA0\x00\x00\x00\x63PKCS-15", 12,
		"PKCS#15" },
	{ (const BYTE *) "\xA0\x00\x00\x00\x77PK15-10",
		12, "CeloCom (PKCS#15 ?)" },
	{ (const BYTE *) "\xD2\x76\x00\x00\x01\x01", 6,
		"KVK (German health insurance card)" },
	{ (const BYTE *) "\xD2\x76\x00\x00\x03\x01\x02", 7,
		"Telesec NetKey" },
	{ (const BYTE *) "\xD2\x76\x00\x00\x22\x00\x00\x00\x60", 9,
		"IBM PKCS#11" },
	{ (const BYTE *) "\xD2\x76\x00\x00\x25\x45\x43\x01\x00", 9,
		"Electronic Cash (EC)" },
	{ (const BYTE *) "\xD2\x76\x00\x00\x25\x45\x50\x01\x00", 9,
		"GeldKarte v2 (EP)" },	/* DF: A200 */
	{ (const BYTE *) "\xD2\x76\x00\x00\x25\x45\x50\x02\x00", 9,
		"GeldKarte v3 (EP)" },	/* DF: A200 */
	{ (const BYTE *) "\xD2\x76\x00\x00\x25\x48\x42\x01\x00", 9,
		"Banking 1 (HBCI)" },	/* DF: A600 */
	{ (const BYTE *) "\xD2\x76\x00\x00\x25\x48\x42\x02\x00", 9,
		"Banking 2 (HBCI)" },	/* DF: A600 */
	{ (const BYTE *) "\xD2\x76\x00\x00\x25\x5A\x41\x01\x00", 9,
		"Zusatzapplikationen (ZA)" },	/* DF: A700 */
	{ (const BYTE *) "\xD2\x76\x00\x00\x37\x02", 6,
		"Lotto" },
	{ (const BYTE *) "\xD2\x76\x00\x00\x60", 5,
		"Wolfgang Rankl" },
	{ (const BYTE *) "\xD2\x76\x00\x00\x63\x10", 6,
		"Der Beck customer card" },
	{ (const BYTE *) "\xD2\x76\x00\x00\x66\x01", 6,
		"SigG" },
	{ (const BYTE *) "\xD2\x76\x00\x00\x66\x02", 6,
		"SigG Additional Security Services" },
	{ (const BYTE *) "\xD2\x76\x00\x00\x66\x03", 6,
		"SigG Identifiction Data" },
	{ (const BYTE *) "\xD2\x76\x00\x00\x74\x48\x42\x01\x10", 9,
		"HBCI Bank-Verlag" },
	{ (const BYTE *) "\xD2\x76\x00\x00\x92", 5,
		"Matthias Bruestle" },
	{ NULL, 0, NULL }
};

/* ATR values for unsupported cards */

static const ATR_VALUE atrTable2[] = {
/*	{ (const BYTE *) "",
		NULL, , 0xE0 }, */
	{ NULL, NULL, 0 }
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
	for( i=0; i<length; i++ ) printf(" %.2X",(array)[i]); \
	printf("\n");

#define printarray2( name, length, array ); \
	printf(name); \
	for( i=0; i<length; i++ ) printf("%.2X",(array)[i]); \
	printf("\n");

int main( int argc, char *argv[] )
{
	SC_READER_INFO *ri;
	SC_CARD_INFO *ci;
	SC_READER_CONFIG rc;

	int ret;
	int i, j;
	BYTE buffer[255];
	int resplen;
	BOOLEAN bool1;
	WORD w1, w2;
	LONG l;

	BOOLEAN key=FALSE;
	int ifid;
	BYTE authkey[8];

	char c;

#if !defined(__WIN32__) && !defined(__WIN16__)
	while( (c=getopt( argc, argv, "k:K:" ))!=EOF ) {
		switch( c ) {
		case 'k':
			if( strlen(optarg)!=16 ) {
				printf( "Error: Wrong size for administration key.\n" );
				return(1);
			}
			for(i=0; i<(strlen(optarg)>>1); i++) {
				ret=sscanf( optarg+i+i, "%2X", &j );
				if( ret!=1 ) {
					printf( "Error: Error in administration key.\n" );
					return(1);
				}
				authkey[i]=j&0xFF;
			}
			key=TRUE;
			break;
		case 'K':
			if( strlen(optarg)<(sizeof(authkey)) ) {
				printf("Error: Adiminstration key to short.\n");
				return(1);
			}
			memcpy( authkey, optarg, sizeof(authkey) );
			key=TRUE;
			break;
		default:
			break;
		}
	}
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

#if defined(DO_LOG)
	SIO_SetLogFile( ri->si, "LogScdir.txt" );
#endif

	/* Reset Card */
	ret= scReaderResetCard( ri, ci );
	testreturn("Card reset error.\n");

	/* Get Card Type */
	ret = scSmartcardGetCardType( ci );

	if( (ci->scGetCap!=NULL) && (ci->scSetFD!=NULL) &&
		(ri->scGetCap!=NULL) && (ri->scSetSpeed!=NULL) ) {
		/* Set Speed */
		ret = scSmartcardSetSpeed( ri, ci, SC_SPEED_FAST );
		if( ret!=SC_EXIT_OK ) {
			/* Reset Card */
			ret= scReaderResetCard( ri, ci );
			testreturn("Card reset error.\n");
		}
	}

/* printf("[speed2: %ld]",SIO_GetSpeed(ri->si)); */

	printf("Card: ");
	switch( ci->type )
	{
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
			printf("GeldKarte v2\n");
			break;
		case SC_CARD_GELDKARTE_3:
			printf("GeldKarte v3\n");
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
		case SC_CARD_TCOS_66P:
			printf("Telesec TCOS 2 card (SLE66P)\n");
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
		case SC_CARD_QUICK:
			printf("Austrian Quick card\n");
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
			printf("Gemplus GPK8000 or variant\n");
			break;
		case SC_CARD_GPK8000_8K:
			printf("Gemplus GPK8000\n");
			break;
		case SC_CARD_GPK8000_16K:
			printf("Gemplus GPK16000\n");
			break;
		case SC_CARD_CARTEB:
			printf("Carte Bancaire\n");
			break;
		case SC_CARD_UNKNOWN:
		default:
			printf("Unknown\n");
		break;
	}

	printarray( "ATR:", ci->atrlen, ci->atr );

	switch( ci->type )
	{
		case SC_CARD_MULTIFLEX_3K:
		case SC_CARD_MULTIFLEX_8K:
		case SC_CARD_MULTIFLEX_8K_DES:
			/* Select EF */
			ret = scMultiflexCmdSelectFile( ri, ci, 0x0002, buffer,
				&resplen );
			testretsw("File selection error.\n",0x90,0x00);

			/* Read */
			resplen=8;
			ret = scMultiflexCmdReadBinary( ri, ci, 0, buffer,
				&resplen );
			testretsw("Read error.\n",0x90,0x00);

			printarray( "Serial number:", resplen, buffer );

			/* Select MF */
			ret = scMultiflexCmdSelectFile( ri, ci, 0x3F00, buffer,
				&resplen );
			testretsw("File selection error.\n",0x90,0x00);

			printf("Master File (3F00):\n");

			i=(buffer[2]<<8)+buffer[3];
			printf("  Free: %d Bytes\n",i);

			i=buffer[14];
			printf("  Number of DFs: %d\n",i);

			i=buffer[15];
			printf("  Number of EFs: %d\n",i);

			i=buffer[16];
			printf("  Number of secret codes: %d\n",i);

			i=buffer[18];
			printf("  CHV status: %d\n",i);

			i=buffer[19];
			printf("  CHV unblocking key status: %d\n",i);

			if( ((ci->type==SC_CARD_MULTIFLEX_8K) ||
				(ci->type==SC_CARD_MULTIFLEX_8K_DES)) && key ) {

				/* Verify Key */
				ret = scMultiflexCmdVerifyKey( ri, ci, 0x01, authkey, 8 );
				testretsw("Authentication error.\n",0x90,0x00);

				/* Directory */
				ret = scMultiflexCmdDirectory( ri, ci, buffer, &resplen );
				testretsw("Directory error.\n",0x90,0x00);

				printf("  Files:\n");

				for( i=0; i<resplen; i+=8 ) {
					printf("    %.2X%.2X ",buffer[i+2],buffer[i+3]);
					ifid=(buffer[i+2]<<8)+buffer[i+3];
					if( ifid==0x0000 ) printf("PIN file ");
					if( ifid==0x0001 ) printf("IntAuth file ");
					if( ifid==0x0002 ) printf("Serial number file ");
					if( ifid==0x0011 ) printf("ExtAuth file ");
					if( ifid==0x2F00 ) printf("DIR file ");
					if( ifid==0x3F00 ) printf("Master file ");
					printf("(Size: %d, Type: ",((buffer[i]<<8)+buffer[i+1]));
					switch(buffer[i+4]) {
						case SC_MULTIFLEX_FILE_TRANSPARENT:
							printf("transparent");
							break;
						case SC_MULTIFLEX_FILE_FIXED:
							printf("fixed-length");
							break;
						case SC_MULTIFLEX_FILE_VARIABLE:
							printf("variable-length");
							break;
						case SC_MULTIFLEX_FILE_CYCLIC:
							printf("cyclic");
							break;
						case SC_MULTIFLEX_FILE_DIRECTORY:
							printf("directory");
							break;
						default:
							printf("unknown");
					}
					printf(", Status: ");
					switch(buffer[i+5]) {
						case SC_MULTIFLEX_BLOCKED:
							printf("blocked");
							break;
						case SC_MULTIFLEX_UNBLOCKED:
							printf("unblocked");
							break;
						default:
							printf("unknown");
					}
					if( buffer[i+4]==SC_MULTIFLEX_FILE_DIRECTORY ) {
						printf(", NSD: %d, NSE: %d)\n",buffer[i+6],buffer[i+7]);
					} else if( (buffer[i+4]==SC_MULTIFLEX_FILE_FIXED) ||
						(buffer[i+4]==SC_MULTIFLEX_FILE_CYCLIC) ) {
						printf(", Record length: %d, ",buffer[i+6]);
						printf("Number of records: %d)\n",buffer[i+7]);
					} else if( (buffer[i+4]==SC_MULTIFLEX_FILE_VARIABLE) ) {
						printf(", Number of records: %d)\n",buffer[i+7]);
					} else {
						printf(")\n");
					}
				}
			}

			break;
		case SC_CARD_CRYPTOFLEX:
		case SC_CARD_CRYPTOFLEX_DES:
		case SC_CARD_CRYPTOFLEX_KEYGEN:
		case SC_CARD_CRYPTOFLEX_8K:
			/* Select EF */
			ret = scCryptoflexCmdSelectFile( ri, ci, 0x0002, buffer,
				&resplen );
			testretsw("File selection error.\n",0x90,0x00);

			/* Read */
			resplen=8;
			ret = scCryptoflexCmdReadBinary( ri, ci, 0, buffer,
				&resplen );
			testretsw("Read error.\n",0x90,0x00);

			printarray( "Serial number:", resplen, buffer );

			/* Select MF */
			ret = scCryptoflexCmdSelectFile( ri, ci, 0x3F00, buffer,
				&resplen );
			testretsw("File selection error.\n",0x90,0x00);

			printf("Master File (3F00):\n");

			i=(buffer[2]<<8)+buffer[3];
			printf("  Free: %d Bytes\n",i);

			i=buffer[14];
			printf("  Number of DFs: %d\n",i);

			i=buffer[15];
			printf("  Number of EFs: %d\n",i);

			i=buffer[16];
			printf("  Number of secret codes: %d\n",i);

			i=buffer[18];
			printf("  CHV status: %d\n",i);

			i=buffer[19];
			printf("  CHV unblocking key status: %d\n",i);

			if( key ) {

				/* Verify Key */
				ret = scCryptoflexCmdVerifyKey( ri, ci, 0x01, authkey, 8 );
				testretsw("Authentication error.\n",0x90,0x00);
  
				if( ci->type!=SC_CARD_CRYPTOFLEX_8K ) {
					/* Directory */
					ret = scCryptoflexCmdDirectory( ri, ci, buffer, &resplen );
					testretsw("Directory error.\n",0x90,0x00);
      
					printf("  Files:\n");
      
					for( i=0; i<resplen; i+=8 ) {
						printf("    %.2X%.2X ",buffer[i+2],buffer[i+3]);
						ifid=(buffer[i+2]<<8)+buffer[i+3];
						if( ifid==0x0000 ) printf("PIN file ");
						if( ifid==0x0001 ) printf("IntAuth file ");
						if( ifid==0x0002 ) printf("Serial number file ");
						if( ifid==0x0011 ) printf("ExtAuth file ");
						if( ifid==0x2F00 ) printf("DIR file ");
						if( ifid==0x3F00 ) printf("Master file ");
						printf("(Size: %d, Type: ",((buffer[i]<<8)+buffer[i+1]));
						switch(buffer[i+4]) {
							case SC_CRYPTOFLEX_FILE_TRANSPARENT:
								printf("transparent");
								break;
							case SC_CRYPTOFLEX_FILE_FIXED:
								printf("fixed-length");
								break;
							case SC_CRYPTOFLEX_FILE_VARIABLE:
								printf("variable-length");
								break;
							case SC_CRYPTOFLEX_FILE_CYCLIC:
								printf("cyclic");
								break;
							case SC_CRYPTOFLEX_FILE_DIRECTORY:
								printf("directory");
								break;
							default:
								printf("unknown");
						}
						printf(", Status: ");
						switch(buffer[i+5]) {
							case SC_CRYPTOFLEX_BLOCKED:
								printf("blocked");
								break;
							case SC_CRYPTOFLEX_UNBLOCKED:
								printf("unblocked");
								break;
							default:
								printf("unknown");
						}
						printf(", ");
						if( buffer[i+4]==SC_CRYPTOFLEX_FILE_DIRECTORY ) {
							printf("NSD: %d, NSE: %d)\n",buffer[i+6],buffer[i+7]);
						} else if( (buffer[i+4]==SC_CRYPTOFLEX_FILE_FIXED) ||
							(buffer[i+4]==SC_CRYPTOFLEX_FILE_CYCLIC) ) {
							printf("Record length: %d, ",buffer[i+6]);
							printf("Number of records: %d)\n",buffer[i+7]);
						} else if( (buffer[i+4]==SC_CRYPTOFLEX_FILE_VARIABLE) ) {
							printf("Number of records: %d)\n",buffer[i+7]);
						} else {
							printf(")\n");
						}
					}
				}
			}

			break;
		case SC_CARD_CYBERFLEX:
		case SC_CARD_CYBERFLEX_CRYPTO:
		case SC_CARD_CYBERFLEX_AUGCRYPTO:
			/* GetData */
			ci->cla=0x00;
			ret = scCyberflexCmdGetData( ri, ci, buffer, &resplen );
			if( (ret==SC_EXIT_OK) && (ci->sw[0]!=0x90) && (ci->sw[1]==0x00) ) {
				ci->cla=0xF0;
				ret = scCyberflexCmdGetData( ri, ci, buffer, &resplen );
			}
			testretsw("Get Data error.\n",0x90,0x00);

			printarray( "Serial number:", 6, buffer );

			printf( "Software version: %d.%.2d\n", buffer[7], buffer[8] );

			switch( buffer[9] ) {
			case 0x0B:
				printf( "Card type: Crypto\n" );
				break;
			case 0x0C:
				printf( "Card type: Augmented Crypto\n" );
				break;
			default:
				printf( "Card type: Unknown\n" );
			}

			printf( "Card string: " );
			for( i=10; i<=18; i++ ) printf( "%c", buffer[i] );
			printf( "\n" );

			printf( "CLA byte: %.2X\n", buffer[19] );

			/* Select MF */
			ret = scCyberflexCmdSelect( ri, ci, SC_CYBERFLEX_SELECT_FILE,
				0x3F00, NULL, 0, buffer, &resplen );
			testretsw("File selection error.\n",0x90,0x00);

			j=0;
			while( (scCyberflexCmdDirectory( ri, ci, j, buffer, &resplen ) ==
				SC_EXIT_OK) && (ci->sw[0]==0x90) && (ci->sw[1]==0x00) &&
				(j<255) ) {
				/* Directory */
				testretsw("Directory error.\n",0x90,0x00);
				/* printarray( "Resp:", resplen, buffer ); */

				switch( buffer[6] ) {
					/* MF */
				case 0x01:
					/* DF */
				case 0x02:
					i=(buffer[4]<<8)+buffer[5];
					if( i==0xFFFF ) break;
					if( buffer[6]==0x01 )
						printf( "%d: Master File (%.4X)\n", j, i );
					else
						printf( "%d: Dedicated File (%.4X)\n", j, i );

					i=(buffer[2]<<8)+buffer[3];
					printf("  Free: %d Bytes\n",i);

					switch( buffer[9] ) {
					case 0x01:
						printf("  Application type: Applet\n");
						break;
					case 0x02:
						printf("  Application type: Application\n");
						break;
					case 0x03:
						printf("  Application type: Applet and Application\n");
						break;
					default:
						break;
					}

					switch( buffer[10] ) {
					case 0x01:
						printf("  Application status: Created\n");
						break;
					case 0x02:
						printf("  Application status: Installed\n");
						break;
					case 0x03:
						printf("  Application status: Registered\n");
						break;
					default:
						break;
					}

					i=buffer[14];
					printf("  Number of DFs: %d\n",i);

					i=buffer[15];
					printf("  Number of EFs: %d\n",i);

					i=buffer[16];
					printf("  Number of secret codes: %d\n",i);

					i=buffer[18];
					printf("  CHV status: %d\n",i);

					i=buffer[19];
					printf("  CHV unblocking key status: %d\n",i);
					break;
					/* EF */
				case 0x04:
					i=(buffer[4]<<8)+buffer[5];
					if( i==0xFFFF ) break;
					printf( "%d: Elementary File (%.4X)\n", j, i );

					i=(buffer[2]<<8)+buffer[3];
					printf("  Size: %d Bytes\n",i);

					switch( buffer[13] ) {
					case 0x00:
						printf("  File type: Transparent\n");
						break;
					case 0x01:
						printf("  File type: Fixed-length record\n");
						break;
					case 0x02:
						printf("  File type: Variable-length record\n");
						break;
					case 0x03:
						printf("  File type: Cyclic\n");
						break;
					case 0x04:
						printf("  File type: Program\n");
						break;
					default:
						break;
					}

					break;
				default:
					printf( "%d: Unknown file type\n", j );
					break;
				}

				j++;
			}

			break;
#ifdef WITH_PAYFLEX
		case SC_CARD_PAYFLEX:
		case SC_CARD_PAYFLEX_1K_USER:
		case SC_CARD_PAYFLEX_1K_SAM:
		case SC_CARD_PAYFLEX_4K_USER:
		case SC_CARD_PAYFLEX_4K_SAM:
		case SC_CARD_PAYFLEX_MICRO:
			/* Select EF */
			ret = scPayflexCmdSelect( ri, ci, 0x0002, buffer, &resplen );
			testretsw("File selection error.\n",0x90,0x00);

			/* Read Record */
			resplen=8;
			ret = scPayflexCmdReadRecord( ri, ci, 1,
				SC_PAYFLEX_RECORD_CURR_INDEX, buffer, &resplen );
			testretsw("Read error.\n",0x90,0x00);

			printarray( "Serial number:", resplen, buffer );

			break;
#endif /* WITH_PAYFLEX */
		case SC_CARD_GPK4000:
		case SC_CARD_GPK4000_S:
		case SC_CARD_GPK4000_SP:
		case SC_CARD_GPK4000_SDO:
		case SC_CARD_GPK8000:
		case SC_CARD_GPK8000_8K:
		case SC_CARD_GPK8000_16K:
			/* Chip Serial Number */
			ret = scGpk4000CmdGetInfo( ri, ci,
				SC_GPK4000_INFO_CHIP_SN, buffer, &resplen );
			testretsw("Read error.\n",0x90,0x00);

			printarray( "Chip serial number:", resplen, buffer );

			/* Card Serial Number */
			ret = scGpk4000CmdGetInfo( ri, ci,
				SC_GPK4000_INFO_CARD_SN, buffer, &resplen );
			testretsw("Read error.\n",0x90,0x00);

			printarray( "Card serial number:", resplen, buffer );

			/* Issuer Serial Number */
			ret = scGpk4000CmdGetInfo( ri, ci,
				SC_GPK4000_INFO_ISSUER_SN, buffer, &resplen );
			testretsw("Read error.\n",0x90,0x00);

			printarray( "Issuer serial number:", resplen, buffer );

			/* Issuer Reference Number */
			ret = scGpk4000CmdGetInfo( ri, ci,
				SC_GPK4000_INFO_ISSUER_REF, buffer, &resplen );
			testretsw("Read error.\n",0x90,0x00);

			printarray( "Issuer reference number:", resplen, buffer );

			/* Pre-issuing data */
			ret = scGpk4000CmdGetInfo( ri, ci,
				SC_GPK4000_INFO_PRE_ISSUING, buffer, &resplen );
			testretsw("Read error.\n",0x90,0x00);

			printarray( "Pre-issuing data:", resplen, buffer );

			if( !(
				/* Select MF*/
				scGpk4000CmdSelFil( ri, ci, SC_GPK4000_SELECT_MF, 0x3F00,
					NULL, 0, buffer, &resplen ) ||
				/* Select DF */
				scGpk4000CmdSelFil( ri, ci, SC_GPK4000_SELECT_DF, 0x0100,
					NULL, 0, buffer, &resplen ) ||
				/* Select EF */
				scGpk4000CmdSelFil( ri, ci, SC_GPK4000_SELECT_EF, 0x01FF,
					NULL, 0, buffer, &resplen )
				) ) {
				/* Read Binary */
				resplen = 1;
				if( (scGpk4000CmdRdBin( ri, ci, 0, buffer, &resplen )==
					SC_EXIT_OK) && (resplen==1) && (ci->sw[0]==0x90) )
					printf("Max. session key: %d bits\n", buffer[0]*8);
			}

			break;
		case SC_CARD_BASICCARD:
		case SC_CARD_BASICCARD_COMP:
		case SC_CARD_BASICCARD_ENH:
			/* State */
			ret = scBasiccardCmdGetState( ri, ci, buffer, &bool1 );
			testretsws("Command error.\n");

			switch( buffer[0] ) {
				case 0:
					printf( "State: NEW\n" );
					break;
				case 1:
					printf( "State: LOAD\n" );
					break;
				case 2:
					printf( "State: TEST\n" );
					break;
				case 3:
					printf( "State: RUN\n" );
					break;
				default:
					printf(" State: Unknown\n" );
			}

			if( (buffer[0]==SC_BASICCARD_STATE_NEW) ||
				(buffer[0]==SC_BASICCARD_STATE_LOAD) ) {
				/* EEPROM Size */
				ret = scBasiccardCmdEepromSize( ri, ci, &w1, &w2 );
				testretsws("Command error.\n");

				printf( "EEPROM:\n" );
				printf( "  Start address: %.4Xh\n", w1 );
				printf( "  Size:          %d\n", w2 );
			}

			if( (buffer[0]==SC_BASICCARD_STATE_TEST) ||
				(buffer[0]==SC_BASICCARD_STATE_RUN) ) {
				/* Application ID */
				ret = scBasiccardCmdGetApplId( ri, ci, buffer, &resplen );
				testretsws("Command error.\n");

				printarray( "Application ID:", resplen, buffer );
			}

			break;
		case SC_CARD_TCOS:
		case SC_CARD_TCOS_44:
		case SC_CARD_TCOS_66:
		case SC_CARD_TCOS_66P:
			/* Select EF(GDO) */
			ret = scTcosCmdSelect( ri, ci, SC_TCOS_SELECT_ABS_PATH, 0,
				"\x2F\x02", 2, SC_TCOS_RDATA_FCI, buffer, &resplen );
			testreturn("File selection error.\n");
			if( ci->sw[0]==0x90 ) {
				/* Read */
				resplen=0;
				ret = scTcosCmdReadBinary( ri, ci, 0, buffer, &resplen );
				testretsw("Read error.\n",0x90,0x00);

				/* Should interpret the BER structure. */
				printarray( "Serial number:", buffer[1], (buffer+2) );
			}

			/* Select EF(DIR) */
			ret = scTcosCmdSelect( ri, ci, SC_TCOS_SELECT_ABS_PATH, 0,
				"\x2F\x00", 2, SC_TCOS_RDATA_FCI, buffer, &resplen );
			testreturn("File selection error.\n");
			if( ci->sw[0]==0x90 ) {
				/* Read */
				resplen=0;
				ret = scTcosCmdReadBinary( ri, ci, 0, buffer, &resplen );
				testretsw("Read error.\n",0x90,0x00);

				/* Should interpret the BER structure. */
				printarray( "AID:", buffer[1], (buffer+2) );

				i=0;
				while( aidTable[i].aidlen ) {
					if( (buffer[1]==aidTable[i].aidlen) &&
						(memcmp(buffer+2, aidTable[i].aid, buffer[1])==0) )
						printf("Application: %s\n", aidTable[i].desc);
					i++;
				}
			}

			break;
		case SC_CARD_BRADESCO:
			/* Card Serial Number */
			ret = scGpk4000CmdGetInfo( ri, ci,
				SC_GPK4000_INFO_CARD_SN, buffer, &resplen );
			testretsw("Read error.\n",0x90,0x00);

			printarray( "Card serial number:", resplen, buffer );

			/* Issuer Serial Number */
			ret = scGpk4000CmdGetInfo( ri, ci,
				SC_GPK4000_INFO_ISSUER_SN, buffer, &resplen );
			testretsw("Read error.\n",0x90,0x00);

			printarray( "Issuer serial number:", resplen, buffer );

			/* Issuer Reference Number */
			ret = scGpk4000CmdGetInfo( ri, ci,
				SC_GPK4000_INFO_ISSUER_REF, buffer, &resplen );
			testretsw("Read error.\n",0x90,0x00);

			printarray( "Issuer reference number:", resplen, buffer );

			/* Select DF */
			ret = scGpk4000CmdSelFil( ri, ci, SC_GPK4000_SELECT_DF,
				0x0200, NULL, 0, buffer, &resplen );
			testretsw("Directory error.\n",0x90,0x00);

			/* Read Binary */
			resplen = 6;
			ret = scGpk4000CmdRdBin( ri, ci, 0x8100, buffer, &resplen );
			testretsw("Read error.\n",0x90,0x00);

			printf("Account: %.3d/%.8d\n", (buffer[0]<<8)+buffer[1],
				(buffer[2]<<24)+(buffer[3]<<16)+(buffer[4]<<8)+buffer[5]);

			/* Read Binary */
			resplen = 3;
			ret = scGpk4000CmdRdBin( ri, ci, 0x8202, buffer, &resplen );
			testretsw("Read error.\n",0x90,0x00);

			printf("Balance: R$ %.2f\n",(double) ((buffer[0]<<16)+
				(buffer[1]<<8)+buffer[2])/100);

			break;
		case SC_CARD_GELDKARTE:
		case SC_CARD_GELDKARTE_3:
			/* EF_ID */
			/* Select MF */
			ret = scGeldkarteCmdSelectFile( ri, ci, 
				SC_GELDKARTE_SELECT_MF, 0, NULL, 0,
				SC_GELDKARTE_SELRESP_NONE, buffer, &resplen );
			testretsw("File selection error.\n",0x90,0x00);
			/* Select EF */
			ret = scGeldkarteCmdSelectFile( ri, ci, 
				SC_GELDKARTE_SELECT_EF, 0x0003, NULL, 0,
				SC_GELDKARTE_SELRESP_NONE, buffer, &resplen );
			testretsw("File selection error.\n",0x90,0x00);

			/* Read Record */
			if( ci->type==SC_CARD_GELDKARTE_3 ) resplen = 256;
			else resplen = 22;
			ret = scGeldkarteCmdReadRecord( ri, ci, 1,
				SC_GELDKARTE_READREC_SELECTED, 0, buffer, &resplen );
			testretsw("Read error.\n",0x90,0x00);

			printarray2("Karteninstitut: ", 3, (buffer+1) );
			printarray2("Kartennummer: ", 5, (buffer+4) );
			printf("Aktivierungsdatum: %.2X/%.2X/%.2X\n", buffer[14],
				buffer[13], buffer[12] );
			printf("Verfallsdatum: %.2X/%.2X\n", buffer[11], buffer[10] );
			printf("Land: %.2X%.2X\n", buffer[15], buffer[16] );
			printf("Waehrung: %c%c%c\n", buffer[17], buffer[18], buffer[19] );

			/* EF_VERSION */
			/* Select MF */
			ret = scGeldkarteCmdSelectFile( ri, ci, 
				SC_GELDKARTE_SELECT_MF, 0, NULL, 0,
				SC_GELDKARTE_SELRESP_NONE, buffer, &resplen );
			testretsw("File selection error.\n",0x90,0x00);
			/* Select EF */
			ret = scGeldkarteCmdSelectFile( ri, ci, 
				SC_GELDKARTE_SELECT_EF, 0x0017, NULL, 0,
				SC_GELDKARTE_SELRESP_NONE, buffer, &resplen );
			testreturn("File selection error.\n");
			if( ci->sw[0]==0x90 ) {
				/* Read Record */
				if( ci->type==SC_CARD_GELDKARTE_3 ) resplen = 256;
				else resplen = 8;
				ret = scGeldkarteCmdReadRecord( ri, ci, 1,
					SC_GELDKARTE_READREC_SELECTED, 0, buffer, &resplen );
				testretsw("Read error.\n",0x90,0x00);
			
				printarray("Version:", 8, buffer );
			}

			/* Select Application */
			if( ci->type==SC_CARD_GELDKARTE_3 ) {
				ret = scGeldkarteCmdSelectFile( ri, ci, 
					SC_GELDKARTE_SELECT_AID, 0,
					"\xD2\x76\x00\x00\x25\x45\x43\x02\x00", 9,
					SC_GELDKARTE_SELRESP_NONE, buffer, &resplen );
			} else {
				ret = scGeldkarteCmdSelectFile( ri, ci, 
					SC_GELDKARTE_SELECT_AID, 0,
					"\xD2\x76\x00\x00\x25\x45\x43\x01\x00", 9,
					SC_GELDKARTE_SELRESP_NONE, buffer, &resplen );
			}
			if( (ret==SC_EXIT_OK) && (ci->sw[0]==0x90) ) {
				printf("\nApplikation \"Electronic Cash\" vorhanden.\n");
			} else {
				ret = scGeldkarteCmdSelectFile( ri, ci, 
					SC_GELDKARTE_SELECT_DF, 0xA100, NULL, 0,
					SC_GELDKARTE_SELRESP_NONE, buffer, &resplen );

				if( (ret==SC_EXIT_OK) && (ci->sw[0]==0x90) ) {
					printf("\nApplikation \"Electronic Cash\" vorhanden.\n");
				}
			}

			/* Select Application */
			if( ci->type==SC_CARD_GELDKARTE_3 ) {
				ret = scGeldkarteCmdSelectFile( ri, ci, 
					SC_GELDKARTE_SELECT_AID, 0,
					"\xD2\x76\x00\x00\x25\x5A\x41\x02\x00", 9,
					SC_GELDKARTE_SELRESP_NONE, buffer, &resplen );
			} else {
				ret = scGeldkarteCmdSelectFile( ri, ci, 
					SC_GELDKARTE_SELECT_AID, 0,
					"\xD2\x76\x00\x00\x25\x5A\x41\x01\x00", 9,
					SC_GELDKARTE_SELRESP_NONE, buffer, &resplen );
			}
			if( (ret==SC_EXIT_OK) && (ci->sw[0]==0x90) ) {
				printf("\nApplikation \"Zusatzapplikationen\" vorhanden.\n");
			} else {
				ret = scGeldkarteCmdSelectFile( ri, ci, 
					SC_GELDKARTE_SELECT_DF, 0xA700, NULL, 0,
					SC_GELDKARTE_SELRESP_NONE, buffer, &resplen );

				if( (ret==SC_EXIT_OK) && (ci->sw[0]==0x90) ) {
					printf("\nApplikation \"Zusatzapplikationen\" vorhanden.\n");
				}
			}

			/* Select Application */
			if( ci->type==SC_CARD_GELDKARTE_3 ) {
				ret = scGeldkarteCmdSelectFile( ri, ci, 
					SC_GELDKARTE_SELECT_AID, 0,
					"\xD2\x76\x00\x00\x25\x48\x42\x02\x00", 9,
					SC_GELDKARTE_SELRESP_NONE, buffer, &resplen );
			} else {
				ret = scGeldkarteCmdSelectFile( ri, ci, 
					SC_GELDKARTE_SELECT_AID, 0,
					"\xD2\x76\x00\x00\x25\x48\x42\x01\x00", 9,
					SC_GELDKARTE_SELRESP_NONE, buffer, &resplen );
			}
			if( (ret==SC_EXIT_OK) && (ci->sw[0]==0x90) ) {
				printf("\nApplikation \"Banking\" (HBCI) vorhanden.\n");

				/* EF_BNK */
				ret = scGeldkarteCmdSelectFile( ri, ci, 
					SC_GELDKARTE_SELECT_EF, 0x0301, NULL, 0,
					SC_GELDKARTE_SELRESP_NONE, buffer, &resplen );
				testreturn("Error selecting EF_BNK.\n");
				if( (ci->sw[0]==0x90) && (ci->sw[1]==0x00) )
				for( i=1; i<=5; i++ ) {
					/* Read Record */
					resplen = 88;
					ret = scGeldkarteCmdReadRecord( ri, ci, i,
						SC_GELDKARTE_READREC_SELECTED, 0, buffer, &resplen );
					testreturn("Read error.\n");
					if( (ci->sw[0]!=0x90) && (ci->sw[0]!=0x61) ) break;
					/* Is this record empty? */
					if( buffer[0]==0x20 ) continue;

					printf( "Bank %d:\n", i );
					/*
					Byte   Länge  Wert           Erläuterung
					1-20   20     'aa .. aa'     Kurzbezeichner des Kreditinstituts 
					21-24  4      'nn nn nn nn'  Bankleitzahl des kontoführenden Instituts 
					25-25  1      'n'            Kommunikationsdienst
					26-53  28     'aa .. aa'     Kommunikationsadresse
					54-55  2      'aa aa'        Kommunikationsadressenzusatz 
					56-58  3      'aa aa aa'     Länderkennzeichen des kontoführenden Instituts 
					59-88  30     'aa .. aa'     Benutzerkennung
					*/
					/* Kurzbezeichner */
					buffer[250]=buffer[20];
					buffer[20]=0x00;
					printf( "  Kurzbezeichner: %s\n", buffer );
					buffer[20]=buffer[250];
					/* BLZ */
					printarray2( "  BLZ: ", 4, buffer+20 );
					/* Benutzerkennung */
					buffer[88]=0x00;
					printf( "  Benutzerkennung: %s\n", buffer+58 );
					/* Länderkennung */
					buffer[58]=0x00;
					printf( "  Länderkennung: %s\n", buffer+55 );
					/* Kommunikationsdienst */
					printf( "  Kommunikationsdienst: %d\n", buffer[24] );
					/* Kommunikationsadresse */
					buffer[250]=buffer[53];
					buffer[53]=0x00;
					printf( "  Kommunikationsadresse: %s\n", buffer+25 );
					buffer[53]=buffer[250];
					/* Kommunikationsadressenzusatz */
					buffer[55]=0x00;
					printf( "  Kommunikationsadressenzusatz: %s\n", buffer+53 );
				}

				/* EF_SEQ, FID 0303, 2 Byte sequence number */
				ret = scGeldkarteCmdSelectFile( ri, ci, 
					SC_GELDKARTE_SELECT_EF, 0x0303, NULL, 0,
					SC_GELDKARTE_SELRESP_NONE, buffer, &resplen );
				testreturn("Error selecting EF_SEQ.\n");
				if( (ci->sw[0]==0x90) && (ci->sw[1]==0x00) ) {
					/* Read Record */
					resplen = 2;
					ret = scGeldkarteCmdReadRecord( ri, ci, 1,
						SC_GELDKARTE_READREC_SELECTED, 0, buffer, &resplen );
					testreturn("Read error.\n");
					if( (ci->sw[0]==0x90) || (ci->sw[0]==0x61) ) {
						i=(buffer[0]<<8)+buffer[1];
						printf( "Sequenzzähler: %d\n", i );
					}
				}
			}

			end_geldkarte_hbci:

			/* Select Application */
			if( ci->type==SC_CARD_GELDKARTE_3 ) {
				ret = scGeldkarteCmdSelectFile( ri, ci, 
					SC_GELDKARTE_SELECT_AID, 0,
					"\xD2\x76\x00\x00\x25\x45\x50\x02\x00", 9,
					SC_GELDKARTE_SELRESP_NONE, buffer, &resplen );
			} else {
				ret = scGeldkarteCmdSelectFile( ri, ci, 
					SC_GELDKARTE_SELECT_AID, 0,
					"\xD2\x76\x00\x00\x25\x45\x50\x01\x00", 9,
					SC_GELDKARTE_SELRESP_NONE, buffer, &resplen );
			}
			if( (ret!=SC_EXIT_OK) || (ci->sw[0]!=0x90) || (ci->sw[1]!=0x00) ) {
				printf("Using FID for application selection.\n");
				ret = scGeldkarteCmdSelectFile( ri, ci, 
					SC_GELDKARTE_SELECT_DF, 0xA200, NULL, 0,
					SC_GELDKARTE_SELRESP_NONE, buffer, &resplen );

				testreturn("Application selection error.\n");
				testretsw("",0x90,0x00);
			}

			printf("\nApplikation \"GeldKarte\" vorhanden.\n");

			/* EF_BETRAG */
			/* Read Record */
			if( ci->type==SC_CARD_GELDKARTE_3 ) resplen = 256;
			else resplen = 9;
			ret = scGeldkarteCmdReadRecord( ri, ci, 1,
				SC_GELDKARTE_READREC_SFID, 0x18, buffer, &resplen );
			testretsw("Read error.\n",0x90,0x00);

			printf("Betrag: %.2X%.2X.%.2X\n", buffer[0], buffer[1],
				buffer[2] );
			printf("Max. Betrag: %.2X%.2X.%.2X\n", buffer[3], buffer[4],
				buffer[5] );
			printf("Max. Transaktion: %.2X%.2X.%.2X\n", buffer[6], buffer[7],
				buffer[8] );

			/* EF_BOeRSE */
			/* Read Record */
			if( ci->type==SC_CARD_GELDKARTE_3 ) resplen = 256;
			else resplen = 27;
			ret = scGeldkarteCmdReadRecord( ri, ci, 1,
				SC_GELDKARTE_READREC_SFID, 0x19, buffer, &resplen );
			testretsw("Read error.\n",0x90,0x00);

			switch( buffer[0] ) {
				case 0x00:
					printf("Kartentyp: Geldkarte\n");
					break;
				case 0xFF:
					printf("Kartentyp: Wertkarte\n");
					break;
				default:
					printf("Kartentyp: unknown\n");
					break;
			}
			printf("Boersenverrechnungskonto:\n");
			printarray2("  BLZ:         ", 4, (buffer+1) );
			printarray2("  Kontonummer: ", 5, (buffer+5) );

			/* EF_LSEQ */
			/* Read Record */
			if( ci->type!=SC_CARD_GELDKARTE_3 ) resplen = 256;
			else resplen = 2;
			ret = scGeldkarteCmdReadRecord( ri, ci, 1,
				SC_GELDKARTE_READREC_SFID, 0x1A, buffer, &resplen );
			if( ci->sw[0]==0x90 ) {
				w1=((WORD)buffer[0]<<8)+buffer[1]-1;
				printf("Ladevorgaenge: %d\n", w1 );
			}

			/* EF_LBEQ */
			/* Read Record */
			if( ci->type==SC_CARD_GELDKARTE_3 ) resplen = 256;
			else resplen = 2;
			ret = scGeldkarteCmdReadRecord( ri, ci, 1,
				SC_GELDKARTE_READREC_SFID, 0x1B, buffer, &resplen );
			if( ci->sw[0]==0x90 ) {
				w1=((WORD)buffer[0]<<8)+buffer[1]-1;
				printf("Abbuchungen: %d\n", w1 );
			}

			/* EF_LLOG */
			printf("Lade-/Entladevorgaenge:\n");
			for( j=1; j<4; j++ ) {
				/* Read Record */
				if( ci->type==SC_CARD_GELDKARTE_3 ) resplen = 256;
				else resplen = 33;
				ret = scGeldkarteCmdReadRecord( ri, ci, (BYTE)j,
					SC_GELDKARTE_READREC_SFID, 0x1C, buffer, &resplen );
				if( (ret!=SC_EXIT_OK) || (ci->sw[0]!=0x90) )
					break;

				if( buffer[0]==0x00 ) { break; }

				printf("%d)\n", j );

				printf("  Vorgang:                ");
				switch( buffer[0] ) {
					case 0x01:
					case 0x03:
						printf("Laden einleiten\n");
						break;
					case 0x05:
					case 0x07:
						printf("Laden einleiten wiederholen\n");
						break;
					case 0x10:
					case 0x11:
					case 0x12:
					case 0x13:
					case 0x14:
					case 0x15:
					case 0x16:
					case 0x17:
						printf("Laden\n");
						break;
					case 0x21:
						printf("Entladen einleiten\n");
						break;
					case 0x25:
						printf("Endladen einleiten wiederholen\n");
						break;
					case 0x30:
					case 0x31:
					case 0x34:
					case 0x35:
						printf("Entladen\n");
						break;
					default:
						printf("Unbekannt\n");
						break;
				}

				w1=((WORD)buffer[1]<<8)+buffer[2];
				printf("  Ladevorgang:            %d\n", w1 );

				w1=((WORD)buffer[31]<<8)+buffer[32];
				printf("  Abbuchungen:            %d\n", w1 );

				printf("  Betrag:                 %.2X%.2X.%.2X\n", buffer[4], buffer[5],
					buffer[6] );

				printarray2("  Terminal-ID:            ", 8, (buffer+13) );

				printarray2("  Terminal-Sequenznummer: ", 3, (buffer+21) );

				if( buffer[26]!=0x00 ) {
					printf("  Datum:                  %.2X/%.2X/%.2X%.2X\n",
						buffer[27], buffer[26], buffer[24], buffer[25] );
				}

				if( !( (buffer[28]==0x00) && (buffer[29]==0x00) &&
					(buffer[30]==0x00) ) )  {
					printf("  Uhrzeit:                %.2X:%.2X:%.2X\n",
						buffer[28], buffer[29], buffer[30] );
				}
			}

			/* EF_BLOG */
			printf("Ab-/Rueckbuchungen:\n");
			for( j=1; j<16; j++ ) {
				/* Read Record */
				if( ci->type==SC_CARD_GELDKARTE_3 ) resplen = 256;
				else resplen = 37;
				ret = scGeldkarteCmdReadRecord( ri, ci, (BYTE)j,
					SC_GELDKARTE_READREC_SFID, 0x1D, buffer, &resplen );
				if( (ret!=SC_EXIT_OK) || (ci->sw[0]!=0x90) )
					break;

				if( buffer[0]==0x00 ) { break; }

				printf("%d)\n", j );

				printf("  Vorgang:                     ");
				switch( buffer[0] ) {
					case 0x50:
					case 0x51:
						printf("Abbuchen\n");
						break;
					case 0x70:
					case 0x71:
						printf("Rueckbuchen\n");
						break;
					default:
						printf("Unbekannt\n");
						break;
				}

				w1=((WORD)buffer[1]<<8)+buffer[2];
				printf("  Abbuchung:                   %d\n", w1 );

				w1=((WORD)buffer[3]<<8)+buffer[4];
				printf("  Ladevorgaenge:               %d\n", w1 );

				printf("  Betrag:                      %.2X%.2X.%.2X\n",
					buffer[5], buffer[6], buffer[7] );

				printarray2("  Haendlerkartennummer:        ", 10, (buffer+8) );

				printf("  Haendlersequenznummer:       %ld\n", ((LONG)buffer[18]<<24)+
					((LONG)buffer[19]<<16)+((LONG)buffer[20]<<8)+buffer[21] );

				printf("  Haendlersummensequenznummer: %ld\n",
					((LONG)buffer[22]<<24)+((LONG)buffer[23]<<16)+
					((LONG)buffer[24]<<8)+buffer[25] );

				if( buffer[31]!=0x00 ) {
					printf("  Datum:                       %.2X/%.2X/%.2X%.2X\n",
						buffer[32], buffer[31], buffer[29], buffer[30] );
				}

				if( !( (buffer[33]==0x00) && (buffer[34]==0x00) &&
					(buffer[35]==0x00) ) )  {
					printf("  Uhrzeit:                     %.2X:%.2X:%.2X\n",
						buffer[33], buffer[34], buffer[35] );
				}
			}

			if( ci->type!=SC_CARD_GELDKARTE_3 ) {
				ret = scGeldkarteCmdGetStat( ri, ci, buffer, &resplen );
				if( (ret==SC_EXIT_OK) && (ci->sw[0]==0x90) &&
					(resplen==0x40) ) {
					printf( "\nChipstatistik:\n" );
					printf( "  Kennzeichen Personalisierer:  %.2X\n",
						buffer[0x00] );
					printf( "  ID Personalisierungsanlage:   %.2X\n",
						buffer[0x01] );
					printarray2( "  Personalisierungdatum:        ", 3,
						buffer+0x02 );
					printarray2( "  Personalisierungsinformation: ", 11,
						buffer+0x05 );
					printf( "  Kennzeichen Initialisierer:   %.2X\n",
						buffer[0x10] );
					printf( "  ID Initialisierungsanlage:    %.2X\n",
						buffer[0x11] );
					printarray2( "  Initialisierungdatum:         ", 3,
						buffer+0x12 );
					printarray2( "  Initialisierungsinformation:  ", 11,
						buffer+0x15 );
					printarray2( "  Modulherstellerländercode:    ", 2,
						buffer+0x20 );
					printf( "  Modulherstellerkennzeichen:   %.2X\n",
						buffer[0x22] );
					printf( "  Modultyp Kennzeichen:         %.2X\n",
						buffer[0x23] );
					printf( "  Modultyp Versionsnummer:      %.2X\n",
						buffer[0x24] );
					printarray2( "  Modulinformation:             ", 11,
						buffer+0x25 );
					printf( "  Chiphersteller Kennzeichen:   %.2X\n",
						buffer[0x30] );
					printf( "  Chiptyp:                      %.2X\n",
						buffer[0x31] );
					printf( "  Version ROM Maske:            %.2X\n",
						buffer[0x32] );
					printarray2( "  Chipseriennummer:             ", 4,
						buffer+0x33 );
					printarray2( "  Version OS:                   ", 2,
						buffer+0x37 );
					printarray2( "  Seriennummer Ergänzung:       ", 4,
						buffer+0x39 );
					printf( "  ROM Maske Kennzeichen:        %.2X\n",
						buffer[0x3D] );
					printarray2( "  Chipinformation:              ", 2,
						buffer+0x3E );
				}
			}
			break;
		case SC_CARD_GSMSIM:
			/* EF_ICCID */
			/* Select EF_ICCID */
			ret = scGsmsimCmdSelect( ri, ci, 0x2FE2, buffer, &resplen );
			if( (ret==SC_EXIT_OK) && (ci->sw[0]==0x90) && (ci->sw[1]==0x00) ) {
				/* Read Binary */
				resplen=10;
				ret = scGsmsimCmdReadBin( ri, ci, 0, buffer, &resplen );
				if( (ret==SC_EXIT_OK) && (ci->sw[0]==0x90) &&
					(ci->sw[1]==0x00) ) {
					for(i=0; i<10; i++)
						buffer[i]=(buffer[i]>>4)|(buffer[i]<<4);
					printarray2( "ICC ID: ", resplen, buffer );
					for(i=0; i<10; i++)
						buffer[i]=(buffer[i]>>4)|(buffer[i]<<4);
					printarray2( "    or: ", resplen, buffer );
				}
			}

			/* EF_LP, EF_SPN, EF_PHASE */
			/* Select DF_GSM */
			ret = scGsmsimCmdSelect( ri, ci, 0x7F20, buffer, &resplen );
			if( (ret==SC_EXIT_OK) && (ci->sw[0]==0x90) && (ci->sw[1]==0x00) ) {
				/* Select EF_LP */
				ret = scGsmsimCmdSelect( ri, ci, 0x6F05, buffer, &resplen );
				if( (ret==SC_EXIT_OK) && (ci->sw[0]==0x90) &&
					(ci->sw[1]==0x00) ) {

					/* Read Binary */
					resplen=4;
					ret = scGsmsimCmdReadBin( ri, ci, 0, buffer, &resplen );
					if( (ret==SC_EXIT_OK) && (ci->sw[0]==0x90) &&
						(ci->sw[1]==0x00) ) {
						printarray( "Language Preferences:", resplen, buffer );
					}
				}

				/* Select EF_SPN */
				ret = scGsmsimCmdSelect( ri, ci, 0x6F46, buffer, &resplen );
				if( (ret==SC_EXIT_OK) && (ci->sw[0]==0x90) &&
					(ci->sw[1]==0x00) ) {

					/* Read Binary */
					resplen=17;
					ret = scGsmsimCmdReadBin( ri, ci, 0, buffer, &resplen );
					if( (ret==SC_EXIT_OK) && (ci->sw[0]==0x90) &&
						(ci->sw[1]==0x00) ) {

						printf("Service Provider: ");
						i=1;
						while( (i<17) && (buffer[i]!=0xFF) )
							printf("%c",buffer[i++]);
						printf("\n");
					}
				}

				/* Select EF_PHASE */
				ret = scGsmsimCmdSelect( ri, ci, 0x6FAE, buffer, &resplen );
				if( (ret==SC_EXIT_OK) && (ci->sw[0]==0x90) &&
					(ci->sw[1]==0x00) ) {

					/* Read Binary */
					resplen=1;
					ret = scGsmsimCmdReadBin( ri, ci, 0, buffer, &resplen );
					if( (ret==SC_EXIT_OK) && (ci->sw[0]==0x90) &&
						(ci->sw[1]==0x00) ) {

						switch( buffer[0] ) {
							case 0:
								printf("Phase 1\n");
								break;
							case 2:
								printf("Phase 2\n");
								break;
							case 3:
								printf("Phase 2+\n");
								break;
							default:
								printf("Phase unknown\n");
								break;
						}
					}
				}

			}

			if( key ) {
			}

			break;
		case SC_CARD_PROTON:
		case SC_CARD_PROTON_CASH:
		case SC_CARD_PROTON_CHIPKNIP:
			/* EF_FAB */
			/* Select EF_FAB */
			ret = scProtonCmdSelectFile( ri, ci, SC_PROTON_EF_FAB, buffer,
				&resplen );
			if( (ret==SC_EXIT_OK) && ((ci->sw[0]&0xF0)==0x90) &&
				(ci->sw[1]==0x00) ) {

				/* Read Binary */
				resplen=8;
				ret = scProtonCmdReadBinary( ri, ci, 0, buffer, &resplen );
				if((ret==SC_EXIT_OK) && ((ci->sw[0]&0xF0)==0x90) &&
					(ci->sw[1]==0x00)) {

					printarray2( "ICC ID: ", resplen, buffer );
				}
			}

			/* Lookup Balance */
			ret = scProtonCmdLookupBalance( ri, ci, buffer, &resplen );
			if( (ret==SC_EXIT_OK) && ((ci->sw[0]&0xF0)==0x90) &&
				(ci->sw[1]==0x00) ) {

				if( resplen==5 ) {
					printarray2( "Balance: ", 3, buffer );
					printarray2( "Currency: ", 2, (buffer+3) );
				}
			}

			/* EF_PURSE */
			/* Select EF_PURSE */
			ret = scProtonCmdSelectFile( ri, ci, SC_PROTON_EF_PURSE, buffer,
				&resplen );
			if( (ret==SC_EXIT_OK) && ((ci->sw[0]&0xF0)==0x90) &&
				(ci->sw[1]==0x00) ) {

				/* Read Binary */
				resplen=0x8C;
				ret = scProtonCmdReadBinary( ri, ci, 0, buffer, &resplen );
				if((ret==SC_EXIT_OK) && ((ci->sw[0]&0xF0)==0x90) &&
					(ci->sw[1]==0x00)) {

					/* printarray2( "EFpurse: ", resplen, buffer ); */
					printarray2( "Purse number: ", 5, (buffer+3) );
					printarray2( "Max. balance: ", 3, (buffer+16) );
					printf( "Activated: %.2X/%.2X/%.2X\n", buffer[32],
						buffer[33], buffer[34] );
					printf( "Valid until: %.2X/%.2X/%.2X\n", buffer[28],
						buffer[29], buffer[30] );
				}
			}

			break;
		case SC_CARD_QUICK:
			ret = scQuickCmdSelectFile( ri, ci, 0x3F00 );
			testretsw("Select error.\n",0x90,0x00);

			ret = scQuickCmdSelectFile( ri, ci, 0xDF01 );
			testretsw("Select error.\n",0x90,0x00);

			ret = scQuickCmdSelectFile( ri, ci, 0x0101 );
			if( (ret==SC_EXIT_OK) && (ci->sw[0]==0x90) &&
				(ci->sw[1]==0x00) ) {
				/* Read Binary */
				resplen=0x05;
				ret = scQuickCmdReadBinary( ri, ci, 3, buffer, &resplen );
				if((ret==SC_EXIT_OK) && (ci->sw[0]==0x90) &&
					(ci->sw[1]==0x00)) {
					/* Most significant byte is lost. */
					l = 0;
					l += buffer[1];
					l <<= 8;
					l += buffer[2];
					l <<= 8;
					l += buffer[3];
					l <<= 8;
					l += buffer[4];
					printf( "Serial number: %.12ld\n", l );
				}
			}
			break;
		case SC_CARD_SMARTCAFE:
		case SC_CARD_SMARTCAFE_11:
			/* Get Data */
			ret = scSmartcafeCmdGetData( ri, ci, SC_SMARTCAFE_TAG_VERSION,
				buffer, &resplen );
			testretsw("Read error.\n",0x90,0x00);

			if( resplen>=9 ) {
				printarray( "Version number:", resplen-2, (buffer+2) );
				if( buffer[8]&0x80 ) {
					printf( "   Delete Completion: enabled\n");
				} else {
					printf( "   Delete Completion: disabled\n");
				}
			}

			/* Get Data */
			ret = scSmartcafeCmdGetData( ri, ci, SC_SMARTCAFE_TAG_SERIAL,
				buffer, &resplen );
			testretsw("Read error.\n",0x90,0x00);

			if( resplen>=10 ) {
				printarray( "Card serial number:", resplen-2, (buffer+2) );
			}

			/* Select ML */
			ret = scSmartcafeCmdSelect( ri, ci, "SM@RT CAFE 11 ML", 16,
				buffer, &resplen );
			if( (ret==SC_EXIT_OK) && (ci->sw[0]==0x90) && (ci->sw[1]==0x00) ) {
				printarray( "FCI of ML:", resplen, buffer );
				if( resplen>=39 ) {
					printf("   MAC size: %d\n", buffer[25] );
					switch( buffer[26] ) {
					case 0x00:
						printf("   SDPL: no security check\n");
						break;
					case 0x01:
						printf("   SDPL: signature check\n");
						break;
					case 0x02:
						printf("   SDPL: decrypt method\n");
						break;
					case 0x03:
						printf("   SDPL: signature check and decrypt method\n");
						break;
					default:
						printf("   SDPL: unkown security parameters\n");
						break;
					}
					printf("   SP:\n");
					switch( buffer[28] ) {
					case 0x00:
                    	printf("      INSTALL:      never\n");
						break;
					case 0xFF:
                    	printf("      INSTALL:      always\n");
						break;
					default:
                    	printf("      INSTALL:      after VERIFY PIN\n");
						break;
					}
					switch( buffer[29] ) {
					case 0x00:
                    	printf("      LOAD APPLET:  never\n");
						break;
					case 0xFF:
                    	printf("      LOAD APPLET:  always\n");
						break;
					default:
                    	printf("      LOAD APPLET:  after VERIFY PIN\n");
						break;
					}
					switch( buffer[31] ) {
					case 0x00:
                    	printf("      DELETE ML:    never\n");
						break;
					case 0xFF:
                    	printf("      DELETE ML:    always\n");
						break;
					default:
                    	printf("      DELETE ML:    after VERIFY PIN\n");
						break;
					}
					switch( buffer[32] ) {
					case 0x00:
                    	printf("      CLEAR MEMORY: never\n");
						break;
					case 0xFF:
                    	printf("      CLEAR MEMORY: always\n");
						break;
					default:
                    	printf("      CLEAR MEMORY: after VERIFY PIN\n");
						break;
					}
					switch( buffer[33] ) {
					case 0x00:
                    	printf("      PUT KEY:      never\n");
						break;
					case 0xFF:
                    	printf("      PUT KEY:      always\n");
						break;
					default:
                    	printf("      PUT KEY:      after VERIFY PIN\n");
						break;
					}
					switch( buffer[34] ) {
					case 0x00:
                    	printf("      SET PIN:      never\n");
						break;
					case 0xFF:
                    	printf("      SET PIN:      always\n");
						break;
					default:
                    	printf("      SET PIN:      after VERIFY PIN\n");
						break;
					}
				}
			} else {
				printf( "No ML created.\n" );
			}

			break;
		case SC_CARD_MFC:
		case SC_CARD_MFC_41:
			/* Select MF */
			ret = scMfcCmdSelect( ri, ci, SC_MFC_SELECT_FID, 0x3F00,
				NULL, 0, buffer, &resplen );
			testretsw("Select error.\n",0x90,0x00);

			printarray( "FCI of MF:", resplen, buffer );

			/* Read Statistics */
			resplen=0;
			ret = scMfcCmdReadStat( ri, ci, buffer, &resplen );
			testretsw("Command error.\n",0x90,0x00);

			printarray( "Chip protocol-data:", 16, buffer );
			printarray( "Module protocol-data:", 16, (buffer+16) );
			printarray( "Init. protocol-data (code):", 16, (buffer+32) );
			printarray( "Init. protocol-data (fs):", 16, (buffer+48) );
			printarray( "Pers. protocol-data:", 16, (buffer+64) );
			printarray( "Range/checksum EEPROM-code:", 6, (buffer+80) );
			printarray( "Range/checksum init fs:", 6, (buffer+86) );
			printarray( "EEPROM status:", 1, (buffer+92) );
			printarray( "Chip-password retry-counter:", 1, (buffer+93) );
			if( resplen==97 ) {
				printarray( "Erase-password retry-counter:", 1, (buffer+94) );
				printarray( "Address of MF:", 2, (buffer+95) );
			} else {
				printarray( "Address of MF:", 2, (buffer+94) );
			}

			break;
		case SC_CARD_CARTEB:
			/* Read */
			resplen=0x20;
			ret = scCartebCmdRead( ri, ci, 0x09C0, buffer, &resplen );
			testretsw0("Read error.\n",0x90);

			printarray( "Card type:", 2, buffer+16 );
			printf("Region addresses:\n");
			w1=(((((WORD)buffer[4]<<8)+buffer[5])/0x20)*0x8);
			w2=w1;
			printf("   ADL: %.4X (Name and secret: ADL - 09C7)\n",w1);
			w1=(((((WORD)buffer[6]<<8)+buffer[7])/0x20)*0x8);
			printf("   ADT: %.4X (Log zone: ADT - ADL-1)\n",w1);
			w1=(((((WORD)buffer[8]<<8)+buffer[9])/0x20)*0x8);
			printf("   ADC: %.4X\n",w1);
			w1=(((((WORD)buffer[10]<<8)+buffer[11])/0x20)*0x8);
			printf("   ADM: %.4X\n",w1);
			w1=(((((WORD)buffer[12]<<8)+buffer[13])/0x20)*0x8);
			printf("   AD2: %.4X\n",w1);
			w1=(((((WORD)buffer[14]<<8)+buffer[15])/0x20)*0x8);
			printf("   ADS: %.4X\n",w1);
			w1=(((((WORD)buffer[20]<<8)+buffer[21])/0x20)*0x8);
			printf("   AD1: %.4X\n",w1);

			if( w2==0xFFFF ) break;
			printf("ADL:\n");
			/* Read */
			resplen=0x04;
			ret = scCartebCmdRead( ri, ci, w2, buffer, &resplen );
			testretsw0("Read error.\n",0x90);

			if( buffer[1]==0x03 ) {
				printf("Size of the authentication secret: %d bytes\n",
					buffer[2]);
			}

			break;
		default:
			break;
	}

exit:
	scReaderDeactivate( ri );
	scReaderShutdown( ri );

	scGeneralFreeCard( &ci );
	scGeneralFreeReader( &ri );

	scEnd();

	return(0);
}


