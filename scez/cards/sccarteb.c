/****************************************************************************
*																			*
*				SCEZ chipcard library - Carte Bancaire routines				*
*					Copyright Matthias Bruestle 1999,2000					*
*					For licensing, see the file COPYRIGHT					*
*																			*
****************************************************************************/

/* $Id: sccarteb.c 1617 2005-11-03 17:41:39Z laforge $ */

#include <scez/scinternal.h>
#include <scez/cards/sccarteb.h>

#include <stdio.h>
#include <string.h>

/* Initialize card function pointer */

int scCartebInit( SC_CARD_INFO *ci )
{
	ci->scGetCap=NULL;
	ci->scGetCardData=scCartebGetCardData;
	ci->scSetFD=NULL;

	return( SC_EXIT_OK );
}

/* Capabilities */

/* Fill card data in ci */

int scCartebGetCardData( SC_CARD_INFO *ci )
{
	BYTE swok[] = { 0x00 };
	BYTE swav[] = { 0x00 };
	int ret;

	if( (ret=scSmartcardProcessATR( ci ))!=SC_EXIT_OK ) return( ret );

	memcpy( ci->swok, swok, sizeof(swok) );
	memcpy( ci->swav, swav, sizeof(swav) );
	ci->memsize=0;

	return( SC_EXIT_OK );
}

/* Must start with a 3 */
int scCartebStrip( BYTE *in, int inlen, BYTE *out, int *outlen )
{
	BYTE b[4];
	int i, fill, counter;

	i=0;
	fill=0;
	*outlen=0;

	for( counter=0; counter<inlen; counter++ ) {
		if( counter&0x03 ) {
			/* No stripping needed */
			i=(i<<8)+*in++;
			fill+=2;
		} else {
			/* Strip first nipple */
			i=(i<<4)+*in++;
			fill+=1;
		}
		/* Output data? */
		if( fill==4 ) {
			*out++=(i>>8)&0xFF;
			*outlen+=1;
			i&=0xFF;
			fill=2;
		} else if( fill==5 ) {
			*out++=(i>>12)&0xFF;
			*outlen+=1;
			i&=0xFFF;
			fill=3;
		}
	}

	/* Output rest of data */
	if( fill==2 ) {
		*out++=i&0xFF;
		*outlen+=1;
	} else if( fill==3 ) {
		*out++=(i>>4)&0xFF;
		*outlen+=1;
		*out++=(i<<4)&0xFF;
		*outlen+=1;
	}

	return SC_EXIT_OK;
}

/* This does only padding with F and a shift by two bits. */
int scCartebPin2Hex( BYTE *pin, BYTE *hex )
{
	hex[0]=((pin[0]&0x0F)<<2)+((pin[1]&0x0C)>>2);
	hex[1]=((pin[1]&0x03)<<6)+((pin[2]&0x0F)<<2)+((pin[3]&0x0C)>>2);
	hex[2]=((pin[3]&0x03)<<6)+0x3F;
	hex[3]=0xFF;

	return( SC_EXIT_OK );
}

int scCartebCmdRead( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	WORD offset, BYTE *data, int *datalen )
{
	BYTE cmd[]={ 0xBC, 0xB0, 0x00, 0x00, 0x00 };
	BYTE rsp[ SC_GENERAL_SHORT_DATA_SIZE+2 ];
	SC_APDU apdu;
	int ret;

	ci->sw[0]=0x00; ci->sw[1]=0x00;

	if( *datalen>256 ) return( SC_EXIT_BAD_PARAM );

	apdu.cse=SC_APDU_CASE_2_SHORT;
	apdu.cmd=cmd;
	apdu.cmdlen=5;
	apdu.rsp=rsp;
	apdu.rsplen=0;

	cmd[2]=(offset>>8) &0xFF;
	cmd[3]=offset & 0xFF;
	cmd[4]=(*datalen) & 0xFF;

	ret=scReaderSendAPDU( ri, ci, &apdu );
	memset( cmd, 0, sizeof(cmd) );
	if( ret!=SC_EXIT_OK ) return( ret );

	if( apdu.rsplen<2 ) return( SC_EXIT_BAD_SW );

	ci->sw[0] = apdu.rsp[apdu.rsplen-2];
	ci->sw[1] = apdu.rsp[apdu.rsplen-1];

	apdu.rsplen-=2;
	memcpy( data, apdu.rsp, apdu.rsplen );
	*datalen=apdu.rsplen;

	memset( rsp, 0, sizeof(rsp) );

	return( SC_EXIT_OK );
}

