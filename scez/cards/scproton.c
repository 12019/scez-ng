/****************************************************************************
*																			*
*					SCEZ chipcard library - Proton routines					*
*						Copyright Matthias Bruestle 2000					*
*					For licensing, see the file COPYRIGHT					*
*																			*
****************************************************************************/

/* $Id: scproton.c 1617 2005-11-03 17:41:39Z laforge $ */

#include <scez/scinternal.h>
#include <scez/cards/scproton.h>
#include <scez/sct1.h>

#include <stdio.h>
#include <string.h>

/* Most Proton commands are from the SIO distribution. */

/* Valid commands:
 * CLA INS
 * BC  10, 14, 1C, 20, 50, A0, A4, A8, B0
 * E1  22, 50, 52, 54, 56, 58, 5A, B4, B6, C0
 *
 * Cmd: E1 B4 00 03 05
 * Cmd: BC 10 00 00 08 ?
 * Cmd: BC 14 00 00 08 ?
 * Cmd: BC 1C 00 00 08 ?
 * Cmd: BC 20 00 00 08 ?
 * Cmd: BC 50 ?? ?? 00
 * Cmd: BC A0 00 00 00  SW:9F08  BC C0 00 00 08   8 byte data
 * Cmd: BC A8 00 00 00  SW:9F00
 * Cmd: E1 22 00 00 00  SW:9000
 * Cmd: E1 50 00 00 0D-1A  SW:6A86
 * Cmd: E1 52 00 00 08  SW:6A86
 * Cmd: E1 54 00 00 ??  SW:6A86
 * Cmd: E1 56 00 00 ??
 * Cmd: E1 58 00 00 ??
 * Cmd: E1 5A 00 00 08  SW:9580
 */

/* Initialize card function pointer */

int scProtonInit( SC_CARD_INFO *ci )
{
	ci->scGetCardData=scProtonGetCardData;

	return( SC_EXIT_OK );
}

/* Fill card data in ci */

int scProtonGetCardData( SC_CARD_INFO *ci )
{
	BYTE header[] = { 0xBC, 0xC0, 0x00, 0x00, 0x00 };
	BYTE swok[] = { 0x02, 0x90, 0x9F };
	BYTE swav[] = { 0x02, 0x61, 0x60 };
	int ret;

	if( (ret=scSmartcardProcessATR( ci ))!=SC_EXIT_OK ) return( ret );

	memcpy( ci->t0.getrsp, header, 5 );
	memcpy( ci->swok, swok, sizeof(swok) );
	memcpy( ci->swav, swav, sizeof(swav) );
	ci->memsize=0;

	return( SC_EXIT_OK );
}

int scProtonCmdLookupBalance( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	BYTE *data, int *datalen )
{
	BYTE cmd[]={ 0xE1, 0xB4, 0x00, 0x01, 0x05 };
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

	*datalen &= 0xFF;

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

/* Offeset in 4 byte blocks. */
int scProtonCmdReadBinary( SC_READER_INFO *ri, SC_CARD_INFO *ci,
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

	*datalen &= 0xFF;

	cmd[2]=(offset>>8) &0xFF;
	cmd[3]=offset & 0xFF;
	cmd[4] = (*datalen) & 0xFF;

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

int scProtonCmdReadRecord( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	BYTE recnum, BYTE mode, BYTE *data, int *datalen )
{
	BYTE cmd[]={ 0xE1, 0xB6, 0x00, 0x00, 0x00 };
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

	*datalen &= 0xFF;

	cmd[2] = recnum;
	cmd[3] = mode;
	cmd[4] = (*datalen) & 0xFF;

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

/* 2 highes bits of FID are ignored. */
int scProtonCmdSelectFile( SC_READER_INFO *ri, SC_CARD_INFO *ci,
	WORD fid, BYTE *resp, int *resplen )
{
	BYTE cmd[]={ 0xBC, 0xA4, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00 };
	BYTE rsp[ SC_GENERAL_SHORT_DATA_SIZE+2 ];
	SC_APDU apdu;
	int ret;

	ci->sw[0]=0x00; ci->sw[1]=0x00;

	apdu.cse=SC_APDU_CASE_4_SHORT;
	apdu.cmd=cmd;
	apdu.cmdlen=8;
	apdu.rsp=rsp;
	apdu.rsplen=0;

	cmd[5]=(fid>>8) &0xFF;
	cmd[6]=fid & 0xFF;

	ret=scReaderSendAPDU( ri, ci, &apdu );
	memset( cmd, 0, sizeof(cmd) );
	if( ret!=SC_EXIT_OK ) return( ret );

	if( apdu.rsplen<2 ) return( SC_EXIT_BAD_SW );

	ci->sw[0] = apdu.rsp[apdu.rsplen-2];
	ci->sw[1] = apdu.rsp[apdu.rsplen-1];

	apdu.rsplen-=2;
	memcpy( resp, apdu.rsp, apdu.rsplen );
	*resplen=apdu.rsplen;

	memset( rsp, 0, sizeof(rsp) );

	return( SC_EXIT_OK );
}


