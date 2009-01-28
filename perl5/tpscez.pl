#!/usr/bin/perl
use scez;
package scez;

$reader_type = $SC_READER_TOWITOKO;
$reader_slot = 1;
$reader_param = "0";

$buffer = BYTE_alloc( 256 );
$len = 0;

$ret = scInit();
printf "scInit(): $ret\n";

$ri = scGeneralNewReader( $reader_type, $reader_slot );
printf "scGeneralNewReader( $reader_type, $reader_slot ): $ri\n";

$ci = scGeneralNewCard();
printf "scGeneralNewCard(): $ci\n";

$ret = scReaderInit( $ri, $reader_param );
printf "scReaderInit( $ri, $reader_param ): $ret\n";

$ret = scReaderActivate( $ri );
printf "scReaderActivate( $ri ): $ret\n";

$ret = scReaderCardStatus( $ri );
printf "scReaderCardStatus( $ri ): $ret\n";
$status = SC_READER_INFO_status_get( $ri );
printf "  ri->status: $status\n";

$ret = scReaderResetCard( $ri, $ci );
printf "scReaderResetCard( $ri, $ci ): $ret\n";

$ret = scSmartcardGetCardType( $ci );
printf "scSmartcardGetCardType( $ci ): $ret\n";
$type = SC_CARD_INFO_type_get( $ci );
printf "  ci->type: $type\n";

$ret = scMultiflexCmdSelectFile( $ri, $ci, 0x3F00, $buffer, \$len );
printf "scMultiflexCmdSelectFile( $ri, $ci, 0x3F00, $buffer, ".\$len." ): $ret\n";
$sw = SC_CARD_INFO_sw_get( $ci );
printf "  sw: %.4X\n", $sw;
printf "  rsp:";
for( $i=0; $i<$len; $i++ ) {
	printf " %.2X", BYTE_get( $buffer, $i );
}
printf "\n";

$ret = scReaderDeactivate( $ri );
printf "scReaderDeactivate( $ri ): $ret\n";

$ret = scReaderShutdown( $ri );
printf "scReaderShutdown( $ri ): $ret\n";

# Dunno how to get a pointer from a pointer. It seams perl moves the
# pointer around in memory, so it does not help to write a C function
# to convert it.

# scGeneralFreeCard( $ciptr );
# scGeneralFreeReader( $riptr );

# So I wrote a wrapper around the original function:

SC_CARD_INFO_free( $ci );
$ci = 0;
SC_READER_INFO_free( $ri );
$ri = 0;

BYTE_free( $buffer );

scEnd();

