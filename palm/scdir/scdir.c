/****************************************************************************
*																			*
*					SCEZ chipcard library - Palm Smart Card Dir				*
*						Copyright Matthias Bruestle 1999					*
*																			*
****************************************************************************/

/* $Id: scdir.c 913 2000-11-13 12:26:59Z zwiebeltu $ */

#define ERROR_CHECK_LEVEL ERROR_CHECK_PARTIAL

#include <Pilot.h>
#ifdef __GCC__
#include "callback.h"
#endif

#include <siolight.h>

#include <scgeneral.h>
#include <scsmartcard.h>
#include <screader.h>

/* The following undefs to make scdir smaller, otherwise it wouldn't fit
 * into 32 kB. If you need some of the cards, make it smaller by undefing
 * some included cards.
 */

#ifdef WITH_GPK4000
#undef WITH_GPK4000
#endif
#ifdef WITH_CRYPTOFLEX
#undef WITH_CRYPTOFLEX
#endif
#ifdef WITH_MULTIFLEX
#undef WITH_MULTIFLEX
#endif
/*
#ifdef WITH_GSMSIM
#undef WITH_GSMSIM
#endif
*/

#ifdef WITH_BASICCARD
#include <scbasiccard.h>
#endif
#ifdef WITH_CRYPTOFLEX
#include <sccryptoflex.h>
#endif
#ifdef WITH_GELDKARTE
#include <scgeldkarte.h>
#endif
#ifdef WITH_GPK4000
#include <scgpk4000.h>
#endif
#ifdef WITH_MULTIFLEX
#include <scmultiflex.h>
#endif
#ifdef WITH_TCOS
#include <sctcos.h>
#endif
#ifdef WITH_GSMSIM
#include <scgsmsim.h>
#endif

#include "scdir.h"

/* Global variables. */

int readerType=SC_READER_DUMBMOUSE;

/* The scrollbar functions are derived from functions in the O'Reilly
 * Palm Programming Book which is on www.palm.com.
 */

void UpdateScrollbar( Word fldindex, Word sbarindex )
{
	FormPtr frm=FrmGetActiveForm();
	FieldPtr fld;
	ScrollBarPtr scroll;
	Word currentPosition;
	Word textHeight;
	Word fieldHeight;
	Word maxValue;

	fld = FrmGetObjectPtr( frm, FrmGetObjectIndex( frm, fldindex ) );
	FldGetScrollValues( fld, &currentPosition, &textHeight, &fieldHeight );

	if( textHeight>fieldHeight )
		maxValue = textHeight - fieldHeight;
	else if( currentPosition )
		maxValue = currentPosition;
	else
		maxValue = 0;

	scroll = FrmGetObjectPtr( frm, FrmGetObjectIndex( frm, sbarindex ) );

	SclSetScrollBar( scroll, currentPosition, 0, maxValue, fieldHeight-1 );
}

void ScrollLines( Word fldindex, Word sbarindex, int numLinesToScroll,
	Boolean redraw )
{
	FormPtr frm=FrmGetActiveForm();
	FieldPtr fld;

	fld = FrmGetObjectPtr( frm, FrmGetObjectIndex( frm, fldindex ) );

	if( numLinesToScroll<0 )
		FldScrollField( fld, -numLinesToScroll, up );
	else
		FldScrollField( fld, numLinesToScroll, down );

	if( (FldGetNumberOfBlankLines(fld) && numLinesToScroll<0) || redraw )
		UpdateScrollbar( fldindex, sbarindex );
}

void PageScroll( Word fldindex, Word sbarindex, DirectionType dir )
{
	FormPtr frm=FrmGetActiveForm();
	FieldPtr fld;

	fld = FrmGetObjectPtr( frm, FrmGetObjectIndex( frm, fldindex ) );

	if( FldScrollable( fld, dir ) ) {
		int linesToScroll = FldGetVisibleLines( fld ) - 1;

		if( dir==up )
			linesToScroll = -linesToScroll;
		ScrollLines( fldindex, sbarindex, linesToScroll, true );
	}
}

/* Field routines. */

int AppendField( FieldPtr fld, CharPtr str, UInt len )
{
	Err err=0;
	CharPtr  s;
	VoidHand h;
	UInt prevlen;

	h=(VoidHand)FldGetTextHandle(fld);

	if(h==NULL) {
		h=MemHandleNew(len+1);
		if(h==NULL) return(-1);
		s=MemHandleLock(h);
		StrNCopy(s, str, len);
		s[len]=0;
		MemHandleUnlock(h);
	} else {
		prevlen=FldGetTextLength(fld);

		FldSetTextHandle(fld, NULL);

		if( MemHandleSize(h)<=(prevlen+len)) {
			err=MemHandleResize( h, prevlen+len+1 );
		}
		if( err!=0 ) return(-1);

		s=MemHandleLock(h);
		StrNCopy(s+prevlen, str, len);
		s[len+prevlen]=0;
		MemHandleUnlock(h);
	}

	FldSetTextHandle(fld, (Handle)h);
	/* FldDrawField(fld); */

	return( 0 );
}

void CleanField( FieldPtr fld )
{
	VoidHand h;

	h = (VoidHand)FldGetTextHandle(fld);
	FldSetTextHandle(fld, NULL);
	if( h!=NULL ) MemHandleFree(h);
}

Word ChooseReader( Word itemNum )
{
	FormPtr prevFrm = FrmGetActiveForm();
	FormPtr frm = FrmInitForm( readerFormID );
	ListPtr lst = FrmGetObjectPtr( frm, FrmGetObjectIndex( frm,
		readerListID ) );
	Word hitButton;
	Word newSel;

	FrmSetActiveForm( frm );

	LstSetSelection( lst, itemNum );

	hitButton = FrmDoDialog( frm );

	newSel = LstGetSelection( lst );
	if( newSel==noListSelection ) newSel=-1;

	if( prevFrm ) FrmSetActiveForm( prevFrm );
	FrmDeleteForm( frm );

	if( hitButton==readerOkButtonID ) return( newSel );
	else return( itemNum );
}

void printarray( FieldPtr fld, BYTE *name, int arraylen, BYTE *array )
{
	int i;
	BYTE sprintb[10];

	AppendField( fld, name, StrLen(name) );
	for( i=0; i<arraylen; i++ ) {
		StrPrintF( sprintb, "%x", array[i] );
		AppendField( fld, " ", 1 );
		AppendField( fld, sprintb+6, 2 );
	}
	AppendField( fld, "\n", 1 );
	FldDrawField(fld);
}

void printarray2( FieldPtr fld, BYTE *name, int arraylen, BYTE *array )
{
	int i;
	BYTE sprintb[10];

	AppendField( fld, name, StrLen(name) );
	AppendField( fld, " ", 1 );
	for( i=0; i<arraylen; i++ ) {
		StrPrintF( sprintb, "%x", array[i] );
		AppendField( fld, sprintb+6, 2 );
	}
	AppendField( fld, "\n", 1 );
	FldDrawField(fld);
}

void f_printbyte( FieldPtr fld, BYTE b )
{
	BYTE sprintb[10];

	StrPrintF( sprintb, "%x", b );
	AppendField( fld, sprintb+6, 2 );
}

#if 0
#define printbyte( value1 ); \
	StrPrintF( sprintb, "%x", value1 ); \
	AppendField( fld, sprintb+6, 2 );

/* Currently not used. */
#define testretsw0(errvalue,sw0); \
	if( (ret!=SC_EXIT_OK) || (ci->sw[0]!=sw0) ) \
	{ scerr=errvalue; goto exit; }
#endif

#define testreturn(errvalue); \
	if(ret!=SC_EXIT_OK){ scerr=errvalue; goto exit; }

#define testretsws(errvalue); \
	scSmartcardSimpleProcessSW( ci, &i, NULL ); \
	if( (ret!=SC_EXIT_OK) || (i!=SC_SW_OK) ) \
	{ scerr=errvalue; goto exit; }

#define testretsw(errvalue,sw0,sw1); \
	if( (ret!=SC_EXIT_OK) || (ci->sw[0]!=sw0) || (ci->sw[1]!=sw1) ) \
	{ scerr=errvalue; goto exit; }

#define printbyte( value1 ); f_printbyte( fld, value1 );

#define myprintf0( text ); \
	AppendField( fld, text, StrLen(text) );

#define myprintf1( text, value1 ); \
	StrPrintF( sprintb, text, value1 ); \
	AppendField( fld, sprintb, StrLen(sprintb) );

#define myprintf3( text, value1, value2, value3 ); \
	StrPrintF( sprintb, text, value1, value2, value3 ); \
	AppendField( fld, sprintb, StrLen(sprintb) );

#define SCDIR_LIB_ERROR		1
#define SCDIR_READER_ERROR	2
#define SCDIR_NO_CARD		3
#define SCDIR_CARD_ERROR	4

int scdir( )
{
	SC_READER_INFO *ri;
	SC_CARD_INFO *ci;

	FieldPtr fld;
	FormPtr frm;

	int scerr=0;

	BYTE sprintb[80];
	BYTE buffer[255];
	int resplen;
	int ret;
	int i;
	LONG l;
	BOOLEAN bool1;
	WORD w1, w2;

	if( scInit() ) return(SCDIR_LIB_ERROR);

    ri = scGeneralNewReader( readerType, 1 );
	if( ri==NULL ) {
		scEnd();
		return(SCDIR_LIB_ERROR);
	}

    ci = scGeneralNewCard( );
	if( ci==NULL ) {
		scGeneralFreeReader( &ri );
		scEnd();
		return(SCDIR_LIB_ERROR);
	};

	/* Init Reader */
	ret = scReaderInit( ri, "1" );
	if( ret!=SC_EXIT_OK ) { scerr=SCDIR_READER_ERROR; goto exit; }

	/* Get Card Status */
	ret = scReaderCardStatus( ri );
	if( ret!=SC_EXIT_OK ) { scerr=SCDIR_READER_ERROR; goto exit; }
	if( !(ri->status&SC_CARD_STATUS_PRESENT) )
	{ scerr=SCDIR_NO_CARD; goto exit; }

	/* Activate Card */
	ret = scReaderActivate( ri );
	if(ret!=SC_EXIT_OK) { scerr=SCDIR_READER_ERROR; goto exit; }

	/* Reset Card */
	ret= scReaderResetCard( ri, ci );
	if(ret!=SC_EXIT_OK) { scerr=SCDIR_CARD_ERROR; goto exit; }

	/* Get Card Type */
	ci->type=0;
	ret = scSmartcardGetCardType( ci );
	if( (ret!=SC_EXIT_OK) && (ret!=SC_EXIT_NOT_SUPPORTED) )
	{ scerr=SCDIR_CARD_ERROR; goto exit; }

	frm = FrmGetActiveForm();
	fld = FrmGetObjectPtr(frm, FrmGetObjectIndex(frm, dataFieldID)); 

	CleanField( fld );

	switch( ci->type )
	{
		case SC_CARD_MULTIFLEX_3K:
			myprintf0( "Card: Schlumberger Multiflex 3K\n" );
			break;
		case SC_CARD_MULTIFLEX_8K:
			myprintf0( "Card: Schlumberger Multiflex 8K\n" );
			break;
		case SC_CARD_MULTIFLEX_8K_DES:
			myprintf0( "Card: Schlumberger Multiflex 8K (with Full DES option)\n" );
			break;
		case SC_CARD_CRYPTOFLEX:
			myprintf0( "Card: Schlumberger Cryptoflex\n" );
			break;
		case SC_CARD_CRYPTOFLEX_DES:
			myprintf0( "Card: Schlumberger Cryptoflex (with Full DES option)\n" );
			break;
		case SC_CARD_CRYPTOFLEX_KEYGEN:
			myprintf0( "Card: Schlumberger Cryptoflex (with RSA key generation)\n" );
			break;
		case SC_CARD_CYBERFLEX:
			myprintf0( "Card: Schlumberger Cyberflex\n" );
			break;
		case SC_CARD_PAYFLEX_1K_USER:
			myprintf0( "Card: Schlumberger Payflex 1K User card\n" );
			break;
		case SC_CARD_PAYFLEX_1K_SAM:
			myprintf0( "Card: Schlumberger Payflex 1K SAM\n" );
			break;
		case SC_CARD_PAYFLEX_4K_USER:
			myprintf0( "Card: Schlumberger Payflex 4K User card\n" );
			break;
		case SC_CARD_PAYFLEX_4K_SAM:
			myprintf0( "Card: Schlumberger Payflex 4K SAM\n" );
			break;
		case SC_CARD_GPK4000_S:
			myprintf0( "Card: Gemplus GPK4000-s\n" );
			break;
		case SC_CARD_GPK4000_SP:
			myprintf0( "Card: Gemplus GPK4000-sp\n" );
			break;
		case SC_CARD_GPK4000_SDO:
			myprintf0( "Card: Gemplus GPK4000-sdo\n" );
			break;
		case SC_CARD_GPK2000_S:
			myprintf0( "Card: Gemplus GPK2000-s\n" );
			break;
		case SC_CARD_GPK2000_SP:
			myprintf0( "Card: Gemplus GPK2000-sp\n" );
			break;
		case SC_CARD_MPCOS_EMV_1B:
			myprintf0( "Card: Gemplus EMV (1-Byte data units)\n" );
			break;
		case SC_CARD_MPCOS_EMV_4B:
			myprintf0( "Card: Gemplus EMV (4-Byte data units)\n" );
			break;
		case SC_CARD_GELDKARTE:
			myprintf0( "Card: Geldkarte\n" );
			break;
		case SC_CARD_TCOS:
			myprintf0("Telesec TCOS 2 card\n");
			break;
		case SC_CARD_TCOS_44:
			myprintf0("Telesec TCOS 2 card (SLE44)\n");
			break;
		case SC_CARD_TCOS_66:
			myprintf0("Telesec TCOS 2 card (SLE66)\n");
			break;
		case SC_CARD_BASICCARD:
			myprintf0( "Card: BasicCard\n" );
			break;
		case SC_CARD_BASICCARD_COMP:
			myprintf0( "Card: Compact BasicCard\n" );
			break;
		case SC_CARD_BASICCARD_ENH:
			myprintf0( "Card: Enhanced BasicCard\n" );
			break;
		case SC_CARD_BRADESCO:
			myprintf0( "Card: Moeda Eletronica Bradesco\n" );
			break;
		case SC_CARD_GSMSIM:
			myprintf0( "Card: GSM SIM card.\n" );
			break;
		case SC_CARD_UNKNOWN:
		default:
			myprintf0( "Card: Unknown\n" );
	}

	FldDrawField(fld);

	printarray( fld, "ATR:", ci->atrlen, ci->atr );

	switch( ci->type )
	{
#ifdef WITH_MULTIFLEX
		case SC_CARD_MULTIFLEX_3K:
		case SC_CARD_MULTIFLEX_8K:
		case SC_CARD_MULTIFLEX_8K_DES:
			/* Select EF */
			ret = scMultiflexCmdSelectFile( ri, ci, 0x0002, buffer,
				&resplen );
			testretsw(SCDIR_CARD_ERROR,0x90,0x00);

			/* Read */
			resplen=8;
			ret = scMultiflexCmdReadBinary( ri, ci, 0, buffer,
				&resplen );
			testretsw(SCDIR_CARD_ERROR,0x90,0x00);

			printarray( fld, "Serial number:", resplen, buffer );

			/* Select MF */
			ret = scMultiflexCmdSelectFile( ri, ci, 0x3F00, buffer,
				&resplen );
			testretsw(SCDIR_CARD_ERROR,0x90,0x00);

			myprintf0("Master File (3F00):\n");

			i=(buffer[2]<<8)+buffer[3];
			myprintf1("  Free: %d Bytes\n",i);

			i=buffer[14];
			myprintf1("  Number of DFs: %d\n",i);

			i=buffer[15];
			myprintf1("  Number of EFs: %d\n",i);

			i=buffer[16];
			myprintf1("  Number of secret codes: %d\n",i);

			i=buffer[18];
			myprintf1("  CHV status: %d\n",i);

			i=buffer[19];
			myprintf1("  CHV unblocking key status: %d\n",i);

			FldDrawField(fld);

			break;
#endif /* WITH_MULTIFLEX */
#ifdef WITH_CRYPTOFLEX
		case SC_CARD_CRYPTOFLEX:
		case SC_CARD_CRYPTOFLEX_DES:
		case SC_CARD_CRYPTOFLEX_KEYGEN:
			/* Select EF */
			ret = scCryptoflexCmdSelectFile( ri, ci, 0x0002, buffer,
				&resplen );
			testretsw(SCDIR_CARD_ERROR,0x90,0x00);

			/* Read */
			resplen=8;
			ret = scCryptoflexCmdReadBinary( ri, ci, 0, buffer,
				&resplen );
			testretsw(SCDIR_CARD_ERROR,0x90,0x00);

			printarray( fld, "Serial number:", resplen, buffer );

			/* Select MF */
			ret = scCryptoflexCmdSelectFile( ri, ci, 0x3F00, buffer,
				&resplen );
			testretsw(SCDIR_CARD_ERROR,0x90,0x00);

			myprintf0("Master File (3F00):\n");

			i=(buffer[2]<<8)+buffer[3];
			myprintf1("  Free: %d Bytes\n",i);

			i=buffer[14];
			myprintf1("  Number of DFs: %d\n",i);

			i=buffer[15];
			myprintf1("  Number of EFs: %d\n",i);

			i=buffer[16];
			myprintf1("  Number of secret codes: %d\n",i);

			i=buffer[18];
			myprintf1("  CHV status: %d\n",i);

			i=buffer[19];
			myprintf1("  CHV unblocking key status: %d\n",i);

			FldDrawField(fld);

			break;
#endif /* WITH_CRYPTOFLEX */
#ifdef WITH_GPK4000
		case SC_CARD_GPK4000_S:
		case SC_CARD_GPK4000_SP:
		case SC_CARD_GPK4000_SDO:
			/* Chip Serial Number */
			ret = scGpk4000CmdGetInfo( ri, ci,
				SC_GPK4000_INFO_CHIP_SN, buffer, &resplen );
			testretsw(SCDIR_CARD_ERROR,0x90,0x00);

			printarray( fld, "Chip serial number:", resplen, buffer );

			/* Card Serial Number */
			ret = scGpk4000CmdGetInfo( ri, ci,
				SC_GPK4000_INFO_CARD_SN, buffer, &resplen );
			testretsw(SCDIR_CARD_ERROR,0x90,0x00);

			printarray( fld, "Card serial number:", resplen, buffer );

			/* Issuer Serial Number */
			ret = scGpk4000CmdGetInfo( ri, ci,
				SC_GPK4000_INFO_ISSUER_SN, buffer, &resplen );
			testretsw(SCDIR_CARD_ERROR,0x90,0x00);

			printarray( fld, "Issuer serial number:", resplen, buffer );

			/* Issuer Reference Number */
			ret = scGpk4000CmdGetInfo( ri, ci,
				SC_GPK4000_INFO_ISSUER_REF, buffer, &resplen );
			testretsw(SCDIR_CARD_ERROR,0x90,0x00);

			printarray( fld, "Issuer reference number:", resplen, buffer );

			/* Pre-issuing data */
			ret = scGpk4000CmdGetInfo( ri, ci,
				SC_GPK4000_INFO_PRE_ISSUING, buffer, &resplen );
			testretsw(SCDIR_CARD_ERROR,0x90,0x00);

			printarray( fld, "Pre-issuing data:", resplen, buffer );

			break;
#endif /* WITH_GPK4000 */
#ifdef WITH_BASICCARD
		case SC_CARD_BASICCARD:
		case SC_CARD_BASICCARD_COMP:
		case SC_CARD_BASICCARD_ENH:
			/* State */
			ret = scBasiccardCmdGetState( ri, ci, buffer, &bool1 );
			testretsws(SCDIR_CARD_ERROR);

			switch( buffer[0] ) {
				case 0:
					myprintf0( "State: NEW\n" );
					break;
				case 1:
					myprintf0( "State: LOAD\n" );
					break;
				case 2:
					myprintf0( "State: TEST\n" );
					break;
				case 3:
					myprintf0( "State: RUN\n" );
					break;
				default:
					myprintf0(" State: Unknown\n" );
			}

			FldDrawField(fld);

			if( (buffer[0]==SC_BASICCARD_STATE_NEW) ||
				(buffer[0]==SC_BASICCARD_STATE_LOAD) ) {
				/* EEPROM Size */
				ret = scBasiccardCmdEepromSize( ri, ci, &w1, &w2 );
				testretsws(SCDIR_CARD_ERROR);

				myprintf0( "EEPROM:\n" );
				myprintf1( "  Start address: %lxh\n", (long)w1 );
				myprintf1( "  Size:          %d\n", w2 );
			}

			FldDrawField(fld);

			if( (buffer[0]==SC_BASICCARD_STATE_TEST) ||
				(buffer[0]==SC_BASICCARD_STATE_RUN) ) {
				/* Application ID */
				ret = scBasiccardCmdGetApplId( ri, ci, buffer, &resplen );
				testretsws(SCDIR_CARD_ERROR);

				printarray( fld, "Application ID:", resplen, buffer );
			}

			break;
#endif /* WITH_BASICCARD */
#ifdef WITH_TCOS
		case SC_CARD_TCOS:
		case SC_CARD_TCOS_44:
		case SC_CARD_TCOS_66:
			/* Select EF(GDO) */
			ret = scTcosCmdSelect( ri, ci, SC_TCOS_SELECT_ABS_PATH, 0,
				"\x2F\x02", 2, SC_TCOS_RDATA_FCI, buffer, &resplen );
			testreturn(SCDIR_CARD_ERROR);
			if( ci->sw[0]==0x90 ) {
				/* Read */
				resplen=0;
				ret = scTcosCmdReadBinary( ri, ci, 0, buffer, &resplen );
				testretsw(SCDIR_CARD_ERROR,0x90,0x00);

				/* Should interpret the BER structure. */
				printarray( fld, "Serial number:", buffer[1], (buffer+2) );
			}

			/* Select EF(DIR) */
			ret = scTcosCmdSelect( ri, ci, SC_TCOS_SELECT_ABS_PATH, 0,
				"\x2F\x00", 2, SC_TCOS_RDATA_FCI, buffer, &resplen );
			testreturn(SCDIR_CARD_ERROR);
			if( ci->sw[0]==0x90 ) {
				/* Read */
				resplen=0;
				ret = scTcosCmdReadBinary( ri, ci, 0, buffer, &resplen );
				testretsw(SCDIR_CARD_ERROR,0x90,0x00);

				/* Should interpret the BER structure. */
				printarray( fld, "AID:", buffer[1], (buffer+2) );
			}

			break;
#endif /* WITH_TCOS */
#ifdef WITH_GPK4000
		case SC_CARD_BRADESCO:
			/* Card Serial Number */
			ret = scGpk4000CmdGetInfo( ri, ci,
				SC_GPK4000_INFO_CARD_SN, buffer, &resplen );
			testretsw(SCDIR_CARD_ERROR,0x90,0x00);

			printarray( fld, "Card serial number:", resplen, buffer );

			/* Issuer Serial Number */
			ret = scGpk4000CmdGetInfo( ri, ci,
				SC_GPK4000_INFO_ISSUER_SN, buffer, &resplen );
			testretsw(SCDIR_CARD_ERROR,0x90,0x00);

			printarray( fld, "Issuer serial number:", resplen, buffer );

			/* Issuer Reference Number */
			ret = scGpk4000CmdGetInfo( ri, ci,
				SC_GPK4000_INFO_ISSUER_REF, buffer, &resplen );
			testretsw(SCDIR_CARD_ERROR,0x90,0x00);

			printarray( fld, "Issuer reference number:", resplen, buffer );

			/* Select DF */
			ret = scGpk4000CmdSelFil( ri, ci, SC_GPK4000_SELECT_DF,
				0x0200, NULL, 0, buffer, &resplen );
			testretsw(SCDIR_CARD_ERROR,0x90,0x00);

			/* Read Binary */
			resplen = 6;
			ret = scGpk4000CmdRdBin( ri, ci, 0x8100, buffer, &resplen );
			testretsw(SCDIR_CARD_ERROR,0x90,0x00);

			myprintf1("Account: %hd/", (buffer[0]<<8)+buffer[1] );
			myprintf1("%ld\n",
				((long)buffer[2]<<24)+((long)buffer[3]<<16)+((long)buffer[4]<<8)+buffer[5]);

			FldDrawField(fld);

			/* Read Binary */
			resplen = 3;
			ret = scGpk4000CmdRdBin( ri, ci, 0x8202, buffer, &resplen );
			testretsw(SCDIR_CARD_ERROR,0x90,0x00);

			l=((long)buffer[0]<<16)+((long)buffer[1]<<8)+buffer[2];
			myprintf1("Balance: R$ %ld.", (long) l/100 );
			myprintf1("%hd", (short) (l/10)%10 );
			myprintf1("%hd\n", (short) l%10 );

			FldDrawField(fld);

			break;
#endif /* WITH_GPK4000 */
		default:
			break;
	}
#ifdef WITH_GELDKARTE
	if( ci->type==SC_CARD_GELDKARTE ) {
			/* EF_ID */
			/* Select MF */
			ret = scGeldkarteCmdSelectFile( ri, ci, 
				SC_GELDKARTE_SELECT_MF, 0, NULL, 0,
				SC_GELDKARTE_SELRESP_NONE, buffer, &resplen );
			testretsw(SCDIR_CARD_ERROR,0x90,0x00);
			/* Select EF */
			ret = scGeldkarteCmdSelectFile( ri, ci, 
				SC_GELDKARTE_SELECT_EF, 0x0003, NULL, 0,
				SC_GELDKARTE_SELRESP_NONE, buffer, &resplen );
			testretsw(SCDIR_CARD_ERROR,0x90,0x00);
			/* Read Record */
			resplen = 22;
			ret = scGeldkarteCmdReadRecord( ri, ci, 1,
				SC_GELDKARTE_READREC_SELECTED, 0, buffer, &resplen );
			testretsw(SCDIR_CARD_ERROR,0x90,0x00);

			printarray2(fld,"Karteninstitut: ", 3, (buffer+1) );
			printarray2(fld,"Kartennummer: ", 5, (buffer+4) );
			myprintf0("Aktivierungsdatum: ");
			printbyte(buffer[14]);
			myprintf0("/");
			printbyte(buffer[13]);
			myprintf0("/");
			printbyte(buffer[12]);
			myprintf0("\nVerfallsdatum: ");
			printbyte(buffer[11]);
			myprintf0("/");
			printbyte(buffer[10]);
			myprintf0("\nLand: ");
			printbyte(buffer[15]);
			printbyte(buffer[16]);
			myprintf3("\nWaehrung: %c%c%c\n", buffer[17], buffer[18],
				buffer[19] );

			/* EF_VERSION */
			/* Select MF */
			ret = scGeldkarteCmdSelectFile( ri, ci, 
				SC_GELDKARTE_SELECT_MF, 0, NULL, 0,
				SC_GELDKARTE_SELRESP_NONE, buffer, &resplen );
			testretsw(SCDIR_CARD_ERROR,0x90,0x00);
			/* Select EF */
			ret = scGeldkarteCmdSelectFile( ri, ci, 
				SC_GELDKARTE_SELECT_EF, 0x0017, NULL, 0,
				SC_GELDKARTE_SELRESP_NONE, buffer, &resplen );
			testreturn(SCDIR_CARD_ERROR);
			if( ci->sw[0]==0x90 ) {
				/* Read Record */
				resplen = 8;
				ret = scGeldkarteCmdReadRecord( ri, ci, 1,
					SC_GELDKARTE_READREC_SELECTED, 0, buffer, &resplen );
				testretsw(SCDIR_CARD_ERROR,0x90,0x00);
			
				printarray(fld,"Version:", 8, buffer );
			}

			/* Select Application */
			ret = scGeldkarteCmdSelectFile( ri, ci, 
				SC_GELDKARTE_SELECT_AID, 0,
				"\xD2\x76\x00\x00\x25\x45\x50\x01\x00", 9,
				SC_GELDKARTE_SELRESP_NONE, buffer, &resplen );
			testretsw(SCDIR_CARD_ERROR,0x90,0x00);

			/* EF_BETRAG */
			/* Read Record */
			resplen = 9;
			ret = scGeldkarteCmdReadRecord( ri, ci, 1,
				SC_GELDKARTE_READREC_SFID, 0x18, buffer, &resplen );
			testretsw(SCDIR_CARD_ERROR,0x90,0x00);

			myprintf0("Betrag: ");
			printbyte(buffer[0]);
			printbyte(buffer[1]);
			myprintf0(".");
			printbyte(buffer[2]);
			myprintf0("\nMax. Betrag: ");
			printbyte(buffer[3]);
			printbyte(buffer[4]);
			myprintf0(".");
			printbyte(buffer[5]);
			myprintf0("\nMax. Transaktion: ");
			printbyte(buffer[6]);
			printbyte(buffer[7]);
			myprintf0(".");
			printbyte(buffer[8]);
			myprintf0("\n");

			/* EF_BOeRSE */
			/* Read Record */
			resplen = 27;
			ret = scGeldkarteCmdReadRecord( ri, ci, 1,
				SC_GELDKARTE_READREC_SFID, 0x19, buffer, &resplen );
			testretsw(SCDIR_CARD_ERROR,0x90,0x00);

			switch( buffer[0] ) {
				case 0x00:
					myprintf0("Kartentyp: Geldkarte\n");
					break;
				case 0xFF:
					myprintf0("Kartentyp: Wertkarte\n");
					break;
				default:
					myprintf0("Kartentyp: unknown\n");
					break;
			}
			myprintf0("Boersenverrechnungskonto:\n");
			printarray2(fld,"  BLZ:         ", 4, (buffer+1) );
			printarray2(fld,"  Kontonummer: ", 5, (buffer+5) );

			/* EF_LSEQ */
			/* Read Record */
			resplen = 2;
			ret = scGeldkarteCmdReadRecord( ri, ci, 1,
				SC_GELDKARTE_READREC_SFID, 0x1A, buffer, &resplen );
			testretsw(SCDIR_CARD_ERROR,0x90,0x00);

			w1=((WORD)buffer[0]<<8)+buffer[1]-1;
			myprintf1("Ladevorgaenge: %hd\n", w1 );

			/* EF_LBEQ */
			/* Read Record */
			resplen = 2;
			ret = scGeldkarteCmdReadRecord( ri, ci, 1,
				SC_GELDKARTE_READREC_SFID, 0x1B, buffer, &resplen );
			testretsw(SCDIR_CARD_ERROR,0x90,0x00);

			w1=((WORD)buffer[0]<<8)+buffer[1]-1;
			myprintf1("Abbuchungen: %hd\n", w1 );

			/* EF_LLOG */
			myprintf0("Lade-/Entladevorgaenge:\n");
			for( i=1; i<4; i++ ) {
				/* Read Record */
				resplen = 33;
				ret = scGeldkarteCmdReadRecord( ri, ci, (BYTE)i,
					SC_GELDKARTE_READREC_SFID, 0x1C, buffer, &resplen );
				if( (ret!=SC_EXIT_OK) || (ci->sw[0]!=0x90) )
					break;

				if( buffer[0]==0x00 ) { break; }

				myprintf1("%d)\n", i );

				myprintf0("  Vorgang: ");
				switch( buffer[0] ) {
					case 0x01:
					case 0x03:
						myprintf0("Laden einleiten\n");
						break;
					case 0x05:
					case 0x07:
						myprintf0("Laden einleiten wiederholen\n");
						break;
					case 0x10:
					case 0x11:
					case 0x12:
					case 0x13:
					case 0x14:
					case 0x15:
					case 0x16:
					case 0x17:
						myprintf0("Laden\n");
						break;
					case 0x21:
						myprintf0("Entladen einleiten\n");
						break;
					case 0x25:
						myprintf0("Endladen einleiten wiederholen\n");
						break;
					case 0x30:
					case 0x31:
					case 0x34:
					case 0x35:
						myprintf0("Entladen\n");
						break;
					default:
						myprintf0("Unbekannt\n");
						break;
				}

				w1=((WORD)buffer[1]<<8)+buffer[2];
				myprintf1("  Ladevorgang: %hd\n", w1 );

				w1=((WORD)buffer[31]<<8)+buffer[32];
				myprintf1("  Abbuchungen: %hd\n", w1 );

				myprintf0("  Betrag: ");
				printbyte(buffer[4]);
				printbyte(buffer[5]);
				myprintf0(".");
				printbyte(buffer[6]);

				printarray2(fld,"\n  Terminal-ID:", 8, (buffer+13) );

				printarray2(fld,"  Terminal-Sequenznummer:", 3, (buffer+21) );

				if( buffer[26]!=0x00 ) {
					myprintf0("  Datum: ");
					printbyte(buffer[27]);
					myprintf0("/");
					printbyte(buffer[26]);
					myprintf0("/");
					printbyte(buffer[24]);
					printbyte(buffer[25]);
					myprintf0("\n");
				}

				if( !( (buffer[28]==0x00) && (buffer[29]==0x00) &&
					(buffer[30]==0x00) ) )  {
					myprintf0("  Uhrzeit: ");
					printbyte(buffer[28]);
					myprintf0(":");
					printbyte(buffer[29]);
					myprintf0(":");
					printbyte(buffer[30]);
					myprintf0("\n");
				}
			}

			/* EF_BLOG */
			myprintf0("Ab-/Rueckbuchungen:\n");
			for( i=1; i<16; i++ ) {
				/* Read Record */
				resplen = 37;
				ret = scGeldkarteCmdReadRecord( ri, ci, (BYTE)i,
					SC_GELDKARTE_READREC_SFID, 0x1D, buffer, &resplen );
				if( (ret!=SC_EXIT_OK) || (ci->sw[0]!=0x90) )
					break;

				if( buffer[0]==0x00 ) { break; }

				myprintf1("%d)\n", i );

				myprintf0("  Vorgang: ");
				switch( buffer[0] ) {
					case 0x50:
					case 0x51:
						myprintf0("Abbuchen\n");
						break;
					case 0x70:
					case 0x71:
						myprintf0("Rueckbuchen\n");
						break;
					default:
						myprintf0("Unbekannt\n");
						break;
				}

				w1=((WORD)buffer[1]<<8)+buffer[2];
				myprintf1("  Abbuchung: %hd\n", w1 );

				w1=((WORD)buffer[3]<<8)+buffer[4];
				myprintf1("  Ladevorgaenge: %hd\n", w1 );

				myprintf0("  Betrag: ");
				printbyte(buffer[5]);
				printbyte(buffer[6]);
				myprintf0(".");
				printbyte(buffer[7]);

				printarray2(fld,"\n  Haendlerkartennummer:", 10, (buffer+8) );

				myprintf1("  Haendlersequenznummer: %ld\n",
					((LONG)buffer[18]<<24)+((LONG)buffer[19]<<16)+
					((LONG)buffer[20]<<8)+buffer[21] );

				myprintf1("  Haendlersummensequenznummer: %ld\n",
					((LONG)buffer[22]<<24)+((LONG)buffer[23]<<16)+
					((LONG)buffer[24]<<8)+buffer[25] );

				if( buffer[31]!=0x00 ) {
					myprintf0("  Datum: ");
					printbyte(buffer[32]);
					myprintf0("/");
					printbyte(buffer[31]);
					myprintf0("/");
					printbyte(buffer[29]);
					printbyte(buffer[30]);
					myprintf0("\n");
				}

				if( !( (buffer[33]==0x00) && (buffer[34]==0x00) &&
					(buffer[35]==0x00) ) )  {
					myprintf0("  Uhrzeit: ");
					printbyte(buffer[33]);
					myprintf0(":");
					printbyte(buffer[34]);
					myprintf0(":");
					printbyte(buffer[35]);
					myprintf0("\n");
				}
			}

	}
#endif /* WITH_GELDKARTE */
#ifdef WITH_GSMSIM
	if( ci->type==SC_CARD_GSMSIM ) {
		/* ICCID */
		/* Select */
		ret = scGsmsimCmdSelect( ri, ci, 0x2FE2, buffer, &resplen );
		if( (ret==0) && (ci->sw[0]==0x90) && (ci->sw[1]==0x00) ) {
			/* Read Binary */
			resplen=10;
			ret = scGsmsimCmdReadBin( ri, ci, 0, buffer, &resplen );
			if( (ret==0) && (ci->sw[0]==0x90) && (ci->sw[1]==0x00) )
				printarray2( fld, "ICCID:", resplen, buffer );
		}
	}
#endif /* WITH_GSMSIM */

exit:
	scReaderDeactivate( ri );
	scReaderShutdown( ri );

	scGeneralFreeCard( &ci );
	scGeneralFreeReader( &ri );

	scEnd();

	return(scerr);
}

static Err StartApplication( void )
{
	/* Open Form */
	FrmGotoForm( mainFormID );

	return 0;
}

static void StopApplication( void )
{
}

static Boolean MyMainFormHandleEvent( EventPtr event )
{
	Boolean handled = false;
	Int i;
	FieldPtr fld;
	FormPtr frm;
	Word readerNum;

#ifdef __GCC__
	CALLBACK_PROLOGUE
#endif

	frm = FrmGetActiveForm();

	switch( event->eType ) {
		case frmOpenEvent:
			UpdateScrollbar( dataFieldID, dataSbarID );
			FrmDrawForm( frm );
			break;
		case ctlSelectEvent:
			if( event->data.ctlSelect.controlID==updateButtonID ) {
				handled=true;

				if( readerType==SC_READER_UNKNOWN ) {
					FrmAlert( alert6FormID );
					break;
				}

				FrmHideObject(frm, FrmGetObjectIndex(frm, updateButtonID));

				i=scdir();

				UpdateScrollbar( dataFieldID, dataSbarID );

				if( i==1 ) FrmAlert(alert1FormID);
				else if( i==2 ) FrmAlert(alert2FormID);
				else if( i==3 ) FrmAlert(alert3FormID);
				else if( i==4 ) FrmAlert(alert4FormID);
				else if( i>4 ) FrmAlert(alert5FormID);

				FrmShowObject(frm, FrmGetObjectIndex(frm, updateButtonID));
			}
			break;
		case menuEvent:
			fld = FrmGetObjectPtr(frm, FrmGetObjectIndex(frm, dataFieldID)); 
			switch( event->data.menu.itemID ) {
				case selectMenuID:
					handled = true;
					FldSetSelection (fld, 0, FldGetTextLength (fld));
					break;
				case copyMenuID:
					handled = true;
					FldCopy(fld);
					break;
				case readerMenuID:
					handled = true;

					switch(readerType) {
						case SC_READER_DUMBMOUSE:
							readerNum=0;
							break;
						case SC_READER_TOWITOKO:
							readerNum=1;
							break;
						default:
							readerNum=-1;
							break;
					}

					readerNum = ChooseReader( readerNum );

					switch(readerNum) {
						case 0:
							readerType=SC_READER_DUMBMOUSE;
							break;
						case 1:
							readerType=SC_READER_TOWITOKO;
							break;
						default:
							readerType=SC_READER_UNKNOWN;
							break;
					}

					break;
				case aboutMenuID:
					handled = true;
					FrmAlert(aboutFormID);
					break;
				case helpMenuID:
					handled = true;
					FrmAlert(helpFormID);
					break;
				default:
					break;
			}
		case fldChangedEvent:
			if( event->data.fldChanged.fieldID==dataFieldID ) {
				UpdateScrollbar( dataFieldID, dataSbarID );
				handled=true;
			}
			break;
		case sclRepeatEvent:
			if( event->data.sclRepeat.scrollBarID==dataSbarID ) {
				ScrollLines( dataFieldID, dataSbarID,
					event->data.sclRepeat.newValue -
					event->data.sclRepeat.value, false );
			}
			break;
		case keyDownEvent:
			if( event->data.keyDown.chr == pageUpChr ) {
				PageScroll( dataFieldID, dataSbarID, up );
				handled = true;
			} else if( event->data.keyDown.chr == pageDownChr ) {
				PageScroll( dataFieldID, dataSbarID, down );
				handled = true;
			}
            break;
		default:
			break;
	}

#ifdef __GCC__
	CALLBACK_EPILOGUE
#endif

	return handled;
}

static Boolean ApplicationHandleEvent( EventPtr event )
{
	FormPtr frm;
	FieldPtr fld;
	FieldAttrType fldattr;
	Int formId;
	Boolean handled = false;

	if( event->eType == frmLoadEvent ) {
		formId = event->data.frmLoad.formID;
		frm = FrmInitForm( formId );
		FrmSetActiveForm( frm );

		switch( formId ) {
			case mainFormID:
				FrmSetEventHandler( frm, MyMainFormHandleEvent );

				fld = FrmGetObjectPtr( frm, FrmGetObjectIndex( frm,
					dataFieldID ) );
				FldGetAttributes( fld, &fldattr );
				fldattr.hasScrollBar=true;
				FldSetAttributes( fld, &fldattr );

				break;
			default:
				break;
		}

		handled = true;
	}

	return handled;
}

static void EventLoop( void )
{
	EventType event;
	UInt error;

	do {
		EvtGetEvent( &event, evtWaitForever );
		if( !SysHandleEvent( &event ) )
			if( !MenuHandleEvent( 0, &event, &error ) )
				if( !ApplicationHandleEvent( &event ) )
					FrmDispatchEvent( &event );
	} while( event.eType != appStopEvent );
}

ULong PilotMain( UInt launchCode, Ptr cmdPBP, UInt launchFlags )
{
	Err err=0;

	if( launchCode == sysAppLaunchCmdNormalLaunch ) {
		if( (err=StartApplication()) == 0 ) {
			EventLoop();
			StopApplication();
		}
	}

	return err;
}



