#include <scez/scgeneral.h>
#include <scez/scsmartcafe.h>

#ifndef READER_TYPE
#define READER_TYPE SC_READER_DUMBMOUSE
#endif /* READER_TYPE */
#ifndef READER_SLOT
#define READER_SLOT 1
#endif /* READER_SLOT */
#ifndef READER_PORT
#define READER_PORT "0"
#endif /* READER_PORT */

int main (int argc, char *argv[] )
{
	SC_READER_INFO *ri;
	SC_CARD_INFO *ci;
	SC_READER_CONFIG rc;

	int ret;

	BYTE resp[ SC_GENERAL_SHORT_DATA_SIZE+2 ];
	int resplen;

	scInit();

	rc.type=READER_TYPE;
	rc.slot=READER_SLOT;
	rc.param=READER_PORT;

	scReaderGetConfig( argc, argv, &rc );

	ri = scGeneralNewReader( rc.type, rc.slot );

	ci = scGeneralNewCard( );

	/* Init Reader */
	scReaderInit( ri, rc.param );

	/* Activate Card */
	scReaderActivate( ri );

	/* Get Card Status */
	scReaderCardStatus( ri );
	if( !(ri->status&SC_CARD_STATUS_PRESENT) ) { /* No card */ }

	/* Reset Card */
	scReaderResetCard( ri, ci );

	/* Get Card Type */
	scSmartcardGetCardType( ci );
	if( (ci->type&0xFFFFFF00)!=SC_CARD_SMARTCAFE ) { /* Wrong card */ }

	ret = scSmartcafeCmdSelect( ri, ci,
			(BYTE *) "\xD2\x76\x00\x00\x92\xFF\xFF\xFF", 9,
			resp, &resplen );
	if( (ret!=SC_EXIT_OK) || (ci->sw[0]!=0x90) || (ci->sw[1]!=0x00) )
	{ /* Some error has occured */ }

	scReaderDeactivate( ri );

	scReaderShutdown( ri );

	scGeneralFreeCard( &ci );
	scGeneralFreeReader( &ri );

	scEnd();

	return( 0 );
}

