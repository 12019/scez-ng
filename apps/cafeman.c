/****************************************************************************
*																			*
*						Copyright Matthias Bruestle 2001					*
*					For licensing, see the file COPYRIGHT					*
*																			*
****************************************************************************/

/* $Id: cafeman.c 1617 2005-11-03 17:41:39Z laforge $ */

#include <getopt.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#if 0
#include <sio/sio.h>
#endif
#include <scez/scgeneral.h>
#include <scez/cards/scsmartcafe.h>

#ifndef READER_TYPE
#define READER_TYPE SC_READER_DUMBMOUSE
#endif /* READER_TYPE */
#ifndef READER_SLOT
#define READER_SLOT 1
#endif /* READER_SLOT */
#ifndef READER_PORT
#define READER_PORT "0"
#endif /* READER_PORT */

#define checkreturn(f); if( ret!=SC_EXIT_OK ) { printf(f); break; }

#define printarray( name, length, array ); \
	printf(name); \
	for( i=0; i<length; i++ ) printf(" %.2X",array[i]); \
	printf("\n");

/* Parameters:
 * - Do CreateML? Y/N
 * - For CreateML: ACL(SizeOf(MAC),SDPL,SP1,SP2)
 * - Key for CAP encryption
 * - Key for CAP signing
 * - AID
 * - Install parameters
 * - Heap size
 */

void usage(char *argv) {
    printf("G&D SmartCafe Manager\n\n");
    printf("Example: Install encrypted applet.\n");
    printf("\t%s -i applet.cap -C 0123456789ABCDEF -A D2760000925445535432\n\n", argv);
    printf("Usage:      %s <options>\n"\
	    "    Options:\n"\
		"   -a <5-16 bytes in hex (10-32 chars)> - AID\n"\
		"   -c <6 chars> - Create ML (Each sp can be n(ever), a(lways) or p(in)\n"\
		"   -C - Clear Memory\n"\
		"   -d - Delete ML\n"\
		"   -e <8/16 bytes in hex (16/32 chars)> - Applets are/should be encrypted\n      with this key\n"\
		"   -H <int> - Heap size (default: empty)\n"\
		"   -i <filename> - Install CAP file (Use -e/-s with single char dummy\n      parameters to specify if file is signed and/or encrypted.)\n"\
		"   -m <int> - Size of MAC (default: 4)\n"\
		"   -n <1-8 bytes in hex (2-16 chars)> - new PIN (default: 0x30,0x30,0x30,0x30\n      or none)\n"\
		"   -u - Do only update keys\n"\
		"   -o <filename> - Output processed CAP file\n"\
		"   -p <1-8 bytes in hex (2-16 chars)> - PIN (default: 0x30,0x30,0x30,0x30\n      or none)\n"\
		"   -P <1-235 bytes in hex (2-470 chars)> - Install params (default: empty)\n"\
		"   -s <8/16 bytes in hex (16/32 chars)> - Applets are/should be signed with\n      this key\n",
	    argv);
    exit(0);
}

int main( int argc, char *argv[] )
{
	SC_READER_INFO *ri;
	SC_CARD_INFO *ci;
	SC_READER_CONFIG rc;

	int c, i, j, ret;
	BYTE buffer[256];
	int resplen;

	BYTE aidml[]={
		0x53,0x4D,0x40,0x52,0x54,0x20,0x43,0x41,
		0x46,0x45,0x20,0x31,0x31,0x20,0x4D,0x4C};

	/* Options */
	BOOLEAN createml=FALSE;	/* -c */
	BOOLEAN deleteml=FALSE;	/* -d */
	BOOLEAN install=FALSE;	/* -i */
	BOOLEAN output=FALSE;	/* -o */
	BOOLEAN clearmem=FALSE;	/* -C */
	BOOLEAN onlyupdate=FALSE;	/* -u */
	BYTE sdpl=0;
	BYTE sp[]={0x00,0x00,0x00,0x00,0x00,0x00};
	BYTE aid[16], aidlen=0;
	BYTE enckey[16], enckeylen=0, sigkey[16], sigkeylen=0;
	WORD heap=0;
	BOOLEAN validheap=FALSE;
	BYTE maclen=4;
	BYTE instparam[235], instparamlen=0;
	BYTE pin[8]={0x30,0x30,0x30,0x30,0x00,0x00,0x00,0x00}, pinlen=0;
	BYTE newpin[8]={0x30,0x30,0x30,0x30,0x00,0x00,0x00,0x00}, newpinlen=0;
	BYTE fname[1024];
	FILE *f=NULL;
	BYTE *fdata=NULL;
	int fsize=0;
	BOOLEAN fsigned=FALSE;
	BYTE foname[1024];
	FILE *fo=NULL;
	BYTE sigalgo, encalgo;

	/*
	 * -a <5-16 bytes in hex> - AID
	 * -c <6 chars> - Create ML (Each sp can be n(ever), a(lways) or p(in)
	 * -C - Clear Memory
	 * -d - Delete ML
	 * -e <8/16 bytes in hex> - Applets are/should be encrypted with this key
	 * -H <int> - Heap size (default: empty)
	 * -i <filename> - Install CAP file
	 * -m <int> - Size of MAC (default: 4)
	 * -n - Do only install the new keys.
	 * -o <filename> - Output filename
	 * -p <1-8 bytes in hex> - PIN (default: 0x30,0x30,0x30,0x30 or none)
	 * -P <1-235 bytes in hex> - Install params (default: empty)
	 * -s <8/16 bytes in hex> - Applets are/should be signed with this key
	 * -S - Select application
	 */
	while( (c=getopt( argc, argv, "a:c:Cde:hH:i:m:n:o:p:P:s:Su" ))!=EOF ) {
		switch( c ) {
		case 'a':
			if( (strlen(optarg)<10) || (strlen(optarg)>32) ||
				(strlen(optarg)&1) ) {
				printf( "Error: Wrong size for AID.\n" );
				return(1);
			}
			for(i=0; i<(strlen(optarg)>>1); i++) {
				ret=sscanf( optarg+i+i, "%2X", &j );
				if( ret!=1 ) {
					printf( "Error: Error in AID.\n" );
					return(1);
				}
				aid[i]=j&0xFF;
			}
			aidlen=i;
			break;
		case 'c':
			if( strlen(optarg)!=6 ) {
				printf("Error: Wrong size for security parameters.\n");
				return(1);
			}

			for( i=0; i<6; i++ ) {
				switch( optarg[i] ) {
				case 'a':
				case 'A':
					sp[i]=0xFF;
					break;
				case 'p':
				case 'P':
					sp[i]=0x04;
				case 'n':
				case 'N':
					break;
				default:
					printf("Error: Wrong char in security parameters.\n");
					return(1);
				}
			}
			createml=TRUE;
			break;
		case 'C':
			clearmem=TRUE;
			break;
		case 'd':
			deleteml=TRUE;
			break;
		case 'e':
			if( strlen(optarg)==1 ) {
				sdpl|=0x02;
				break;
			}
			if( (strlen(optarg)!=16) && (strlen(optarg)!=32) ) {
				printf( "Error: Wrong size for encryption key.\n" );
				return(1);
			}
			for(i=0; i<(strlen(optarg)>>1); i++) {
				ret=sscanf( optarg+i+i, "%2X", &j );
				if( ret!=1 ) {
					printf( "Error: Error in encryption key.\n" );
					return(1);
				}
				enckey[i]=j&0xFF;
			}
			enckeylen=i;
			sdpl|=0x02;
			break;
		case 'H':
			ret=sscanf( optarg, "%d", &i );
			if( (ret!=1) || (i>0xFFFF) ) {
				printf( "Error: Error in heap size.\n" );
				return(1);
			}
			heap=i&0xFFFF;
			validheap=TRUE;
			break;
		case 'i':
			if( strlen(optarg)>(sizeof(fname)-1) ) {
				printf("Error: Filename to long.\n");
				return(1);
			}
			strncpy( fname, optarg, sizeof(fname) );
			install=TRUE;
			break;
		case 'm':
			ret=sscanf( optarg, "%d", &i );
			if( (ret!=1) || (i<0) || (i>8) ) {
				printf( "Error: Error in MAC size.\n" );
				return(1);
			}
			maclen=i&0xFF;
			break;
		case 'n':
			if( (strlen(optarg)<2) || (strlen(optarg)>16) ||
				(strlen(optarg)&1) ) {
				printf( "Error: Wrong size for PIN.\n" );
				return(1);
			}
			for(i=0; i<(strlen(optarg)>>1); i++) {
				ret=sscanf( optarg+i+i, "%2X", &j );
				if( ret!=1 ) {
					printf( "Error: Error in PIN.\n" );
					return(1);
				}
				newpin[i]=j&0xFF;
			}
			newpinlen=i;
			break;
		case 'o':
			if( strlen(optarg)>(sizeof(foname)-1) ) {
				printf("Error: Output filename to long.\n");
				return(1);
			}
			strncpy( foname, optarg, sizeof(foname) );
			output=TRUE;
			break;
		case 'p':
			if( (strlen(optarg)<2) || (strlen(optarg)>16) ||
				(strlen(optarg)&1) ) {
				printf( "Error: Wrong size for PIN.\n" );
				return(1);
			}
			for(i=0; i<(strlen(optarg)>>1); i++) {
				ret=sscanf( optarg+i+i, "%2X", &j );
				if( ret!=1 ) {
					printf( "Error: Error in PIN.\n" );
					return(1);
				}
				pin[i]=j&0xFF;
			}
			pinlen=i;
			break;
		case 'P':
			if( (strlen(optarg)<2) || (strlen(optarg)>470) ||
				(strlen(optarg)&1) ) {
				printf( "Error: Wrong size for install params.\n" );
				return(1);
			}
			for(i=0; i<(strlen(optarg)>>1); i++) {
				ret=sscanf( optarg+i+i, "%2X", &j );
				if( ret!=1 ) {
					printf( "Error: Error in install params.\n" );
					return(1);
				}
				instparam[i]=j&0xFF;
			}
			instparamlen=i;
			break;
		case 's':
			if( strlen(optarg)==1 ) {
				sdpl|=0x01;
				fsigned=TRUE;
				break;
			}
			if( (strlen(optarg)!=16) && (strlen(optarg)!=32) ) {
				printf( "Error: Wrong size for signature key.\n" );
				return(1);
			}
			for(i=0; i<(strlen(optarg)>>1); i++) {
				ret=sscanf( optarg+i+i, "%2X", &j );
				if( ret!=1 ) {
					printf( "Error: Error in signature key.\n" );
					return(1);
				}
				sigkey[i]=j&0xFF;
			}
			sigkeylen=i;
			sdpl|=0x01;
			break;
		case 'u':
			onlyupdate=TRUE;
			break;
		case 'h':
			usage( argv[0] );
		default:
			break;
		}
	}

	if( install && (aidlen==0) && (!output) ) {
		printf( "Error: AID not specified.\n" );
		return(1);
	}

	if( output && (!install) ) {
		printf( "Error: Input file not specified.\n" );
		return(1);
	}

	if( output && (enckeylen==0) && (sigkeylen==0) ) {
		printf( "Error: Neither encryption key not signature key has been specified.\n" );
		return(1);
	}

	if( fsigned && ((sigkeylen!=0) || (enckeylen!=0)) ) {
		printf( "Error: Input file already signed.\n" );
		return(1);
	}

	if( fsigned && output ) {
		printf( "Error: Processing an already processed file doesn't make any sense.\n" );
		return(1);
	}

	if( install ) {
		if( (f=fopen( fname, "rb" ))==NULL ) {
			printf( "Error: Unable to open input file.\n" );
			return(2);
		}
		if( fseek( f, 0, SEEK_END )==-1 ) {
			printf( "Error: Unable operate on file.\n" );
			return(2);
		}
		if( (fsize = ftell( f ))==-1 ) {
			printf( "Error: Unable operate on file.\n" );
			return(2);
		}
		rewind( f );
		if( (fdata = malloc( fsize+8 ))==NULL ) {
			printf( "Error: File to big for memory allocation.\n" );
			return(2);
		}
		if( fread( fdata, 1, fsize, f )!=fsize ) {
			printf( "Error: Unable to read file.\n" );
			return(2);
		}

		if( fsigned ) fsize-=maclen;

		if( output ) {
			switch( enckeylen ) {
			case 8:
				encalgo=SC_SMARTCAFE_ALGO_DES;
				break;
			case 16:
				encalgo=SC_SMARTCAFE_ALGO_3DES;
				break;
			default:
				encalgo=SC_SMARTCAFE_ALGO_NONE;
				break;
			}

			switch( sigkeylen ) {
			case 8:
				sigalgo=SC_SMARTCAFE_ALGO_DES;
				break;
			case 16:
				sigalgo=SC_SMARTCAFE_ALGO_3DES;
				break;
			default:
				sigalgo=SC_SMARTCAFE_ALGO_NONE;
				break;
			}

			i=fsize;
			if( (encalgo!=SC_SMARTCAFE_ALGO_NONE) ||
				(sigalgo!=SC_SMARTCAFE_ALGO_NONE) ) {
				ret = scSmartcafeAuthApplet( enckey, encalgo, sigkey,
					sigalgo, fdata, &i );
				if( ret!=SC_EXIT_OK ) {
					printf( "Error: Unable to process applet data.\n" );
					return(2);
				}
			}

			if( (fo=fopen( foname, "wb" ))==NULL ) {
				printf( "Error: Unable to open output file.\n" );
				return(2);
			}
			if( fwrite( fdata, 1, i, fo )!=i ) {
				printf( "Error: Unable to write file.\n" );
				return(2);
			}

			return(0);
		}
	}

	do {

		if( scInit() ) { printf("Error: scInit\n"); return(3); }

		rc.type=READER_TYPE;
		rc.slot=READER_SLOT;
		rc.param=READER_PORT;

		ret = scReaderGetConfig( argc, argv, &rc );
		if( ret!=SC_EXIT_OK ) {
			printf( "Error: Error getting reader configuration.\n" );
			scEnd();
			return(3);
		};

		ri = scGeneralNewReader( rc.type, rc.slot );
		if( ri==NULL ) {
			printf("Error: scGeneralNewReader\n"); scEnd(); return(3);
		}

		ci = scGeneralNewCard( );
		if( ci==NULL ) {
			printf("Error: scGeneralNewCard\n"); scEnd(); return(3);
		}

		/* Init Reader */
		ret = scReaderInit( ri, rc.param );
		checkreturn("Error: scReaderInit\n");

		/* Activate Card */
		ret = scReaderActivate( ri );
		checkreturn("Error: scReaderActivate\n");

		/* Get Card Status */
		ret = scReaderCardStatus( ri );
		checkreturn("Error: scReaderCardStatus\n");
		if( !(ri->status&SC_CARD_STATUS_PRESENT) )
		{ printf("Error: No Card.\n"); break; }

		/* Reset Card */
		ret= scReaderResetCard( ri, ci );
		checkreturn("Error: scReaderResetCard\n");

		/* Get Card Type */
		ret = scSmartcardGetCardType( ci );
		checkreturn("Error: scReaderGetCardType\n");
		if( (ci->type&0xFFFFFF00)!=SC_CARD_SMARTCAFE )
		{ printf("Error: Wrong Card.\n"); break; }

#if 0
		/* Change the protocol to T=1 */
		ret=scReaderPTS( ri, ci, "\xFF\x11\x11\xFF", 4 );
		if( ret==SC_EXIT_OK ) {
			scSmartcardProcessATR( ci );
		} else if( ret!=SC_EXIT_NOT_SUPPORTED ) {
			/* Reset Card */
			ret= scReaderResetCard( ri, ci );
			checkreturn("Error: scReaderResetCard\n");
		}
#endif

#if 0
		SIO_SetLogFile( ri->si, "LogCafeman.txt" );
#endif

		if( onlyupdate ) {
			if( pinlen!=0 ) {
				ret = scSmartcafeCmdVerifyPIN( ri, ci, pin, pinlen );
				scSmartcardSimpleProcessSW( ci, &i, NULL );
				if( (ret!=SC_EXIT_OK) || (i!=SC_SW_OK) ) {
					printf( "Error: Unable to verify PIN.\n" );
					ret=4;
					break;
				}
			}

			if( pinlen!=0 ) {
				ret = scSmartcafeCmdSetPIN( ri, ci, newpin, newpinlen );
				scSmartcardSimpleProcessSW( ci, &i, NULL );
				if( (ret!=SC_EXIT_OK) || (i!=SC_SW_OK) ) {
					printf( "Error: Unable to set PIN.\n" );
					ret=4;
					break;
				}
			}

			if( sigkeylen!=0 ) {
				ret = scSmartcafeCmdPutKey( ri, ci, SC_SMARTCAFE_KEYIDX_SIGN,
					sigkey, sigkeylen );
				scSmartcardSimpleProcessSW( ci, &i, NULL );
				if( (ret!=SC_EXIT_OK) || (i!=SC_SW_OK) ) {
					printf( "Error: Unable to put signature key.N\n" );
					ret=4;
					break;
				}
			}

			if( enckeylen!=0 ) {
				ret = scSmartcafeCmdPutKey( ri, ci, SC_SMARTCAFE_KEYIDX_CRYPT,
					enckey, enckeylen );
				scSmartcardSimpleProcessSW( ci, &i, NULL );
				if( (ret!=SC_EXIT_OK) || (i!=SC_SW_OK) ) {
					printf( "Error: Unable to put encryption key.N\n" );
					ret=4;
					break;
				}
			}

			ret=0;
			break;
		}

		if( clearmem ) {
			if( pinlen!=0 ) {
				ret = scSmartcafeCmdVerifyPIN( ri, ci, pin, pinlen );
				scSmartcardSimpleProcessSW( ci, &i, NULL );
				if( (ret!=SC_EXIT_OK) || (i!=SC_SW_OK) ) {
					printf( "Error: Unable to verify PIN.\n" );
					ret=4;
					break;
				}
			}
			ret = scSmartcafeCmdClearMem( ri, ci );
			scSmartcardSimpleProcessSW( ci, &i, NULL );
			if( (ret!=SC_EXIT_OK) || (i!=SC_SW_OK) ) {
				printf( "Error: Unable to clear memory.\n" );
				ret=4;
				break;
			}

			ret=0;
		}

		if( deleteml ) {
			ret = scSmartcafeCmdSelect( ri, ci, aidml, sizeof(aidml),
				buffer, &resplen );
			scSmartcardSimpleProcessSW( ci, &i, NULL );
			if( (ret!=SC_EXIT_OK) || (i!=SC_SW_OK) ) {
				printf( "Error: Unable to select ML.\n" );
				ret=4;
				break;
			}

			if( pinlen!=0 ) {
				ret = scSmartcafeCmdVerifyPIN( ri, ci, pin, pinlen );
				scSmartcardSimpleProcessSW( ci, &i, NULL );
				if( (ret!=SC_EXIT_OK) || (i!=SC_SW_OK) ) {
					printf( "Error: Unable to verify PIN.\n" );
					ret=4;
					break;
				}
			}

			ret = scSmartcafeCmdDeleteML( ri, ci );
			scSmartcardSimpleProcessSW( ci, &i, NULL );
			if( (ret!=SC_EXIT_OK) || (i!=SC_SW_OK) ) {
				printf( "Error: Unable to delete ML.\n" );
				ret=4;
				break;
			}
		}

		if( createml ) {
			ret = scSmartcafeCmdCreateML( ri, ci, TRUE, maclen, sdpl, sp );
			scSmartcardSimpleProcessSW( ci, &i, NULL );
			if( (ret!=SC_EXIT_OK) || (i!=SC_SW_OK) ) {
				printf( "Error: Unable to create ML.\n" );
				ret=4;
				break;
			}

			if( pinlen==0 ) pinlen=4;
			ret = scSmartcafeCmdSetPIN( ri, ci, pin, pinlen );
			scSmartcardSimpleProcessSW( ci, &i, NULL );
			if( (ret!=SC_EXIT_OK) || (i!=SC_SW_OK) ) {
				printf( "Error: Unable to set PIN.\n" );
				ret=4;
				break;
			}

			if( sigkeylen!=0 ) {
				ret = scSmartcafeCmdPutKey( ri, ci, SC_SMARTCAFE_KEYIDX_SIGN,
					sigkey, sigkeylen );
				scSmartcardSimpleProcessSW( ci, &i, NULL );
				if( (ret!=SC_EXIT_OK) || (i!=SC_SW_OK) ) {
					printf( "Error: Unable to put signature key.N\n" );
					ret=4;
					break;
				}
			}

			if( enckeylen!=0 ) {
				ret = scSmartcafeCmdPutKey( ri, ci, SC_SMARTCAFE_KEYIDX_CRYPT,
					enckey, enckeylen );
				scSmartcardSimpleProcessSW( ci, &i, NULL );
				if( (ret!=SC_EXIT_OK) || (i!=SC_SW_OK) ) {
					printf( "Error: Unable to put encryption key.N\n" );
					ret=4;
					break;
				}
			}

			ret = scSmartcafeCmdCreateML( ri, ci, FALSE, 0, 0, NULL );
			scSmartcardSimpleProcessSW( ci, &i, NULL );
			if( (ret!=SC_EXIT_OK) || (i!=SC_SW_OK) ) {
				printf( "Error: Unable to end create ML.\n" );
				ret=4;
				break;
			}
		}

		if( install ) {
			if( createml || (pinlen!=0) ) {
				ret = scSmartcafeCmdVerifyPIN( ri, ci, pin, pinlen );
				scSmartcardSimpleProcessSW( ci, &i, NULL );
				if( (ret!=SC_EXIT_OK) || (i!=SC_SW_OK) ) {
					printf( "Error: Unable to verify PIN.\n" );
					ret=4;
					break;
				}
			}

			switch( enckeylen ) {
			case 8:
				encalgo=SC_SMARTCAFE_ALGO_DES;
				break;
			case 16:
				encalgo=SC_SMARTCAFE_ALGO_3DES;
				break;
			default:
				encalgo=SC_SMARTCAFE_ALGO_NONE;
				break;
			}

			switch( sigkeylen ) {
			case 8:
				sigalgo=SC_SMARTCAFE_ALGO_DES;
				break;
			case 16:
				sigalgo=SC_SMARTCAFE_ALGO_3DES;
				break;
			default:
				sigalgo=SC_SMARTCAFE_ALGO_NONE;
				break;
			}

			if( (encalgo!=SC_SMARTCAFE_ALGO_NONE) ||
				(sigalgo!=SC_SMARTCAFE_ALGO_NONE) ) {
				i=fsize;
				ret = scSmartcafeAuthApplet( enckey, encalgo, sigkey,
					sigalgo, fdata, &i );
				if( ret!=SC_EXIT_OK ) {
					printf( "Error: Unable to process applet data.\n" );
					ret=4;
					break;
				}
			}

			ret = scSmartcafeCmdSelect( ri, ci, aidml, sizeof(aidml),
				buffer, &resplen );
			scSmartcardSimpleProcessSW( ci, &i, NULL );
			if( (ret!=SC_EXIT_OK) || (i!=SC_SW_OK) ) {
				printf( "Error: Unable to select ML.\n" );
				ret=4;
				break;
			}

			ret = scSmartcafeCmdInstall( ri, ci, SC_SMARTCAFE_INSTALL_LOAD,
				sdpl, aid, aidlen, NULL, 0, 0 );
			scSmartcardSimpleProcessSW( ci, &i, NULL );
			if( (ret!=SC_EXIT_OK) || (i!=SC_SW_OK) ) {
				printf( "Error: Unable to initiate load.\n" );
				ret=4;
				break;
			}

			j=0;
			for( i=0; i<fsize; i+=224 ) {
				int size = min( 224, fsize-i );
				int status;

				if( (sigkeylen==0) && ((fsize-i)<=224) && (!fsigned) ) {
					ret = scSmartcafeCmdLoadApplet( ri, ci, TRUE, j,
						 fdata+i, size );
				} else {
					ret = scSmartcafeCmdLoadApplet( ri, ci, FALSE, j,
						fdata+i, size );
				}
				scSmartcardSimpleProcessSW( ci, &status, NULL );
				if( (ret!=SC_EXIT_OK) || (status!=SC_SW_OK) ) {
					printf( "Error: Unable to load applet.\n" );
					ret=4;
					break;
				}
				j++;
			}
			if( ret!=0 ) break;

			if( (sigkeylen!=0) || (fsigned) ) {
				ret = scSmartcafeCmdLoadApplet( ri, ci, TRUE, j, fdata+fsize,
					maclen );
				scSmartcardSimpleProcessSW( ci, &i, NULL );
				if( (ret!=SC_EXIT_OK) || (i!=SC_SW_OK) ) {
					printf( "Error: Bad applet signature.\n" );
					ret=4;
					break;
				}
			}

			if( validheap ) {
				ret = scSmartcafeCmdInstall( ri, ci, SC_SMARTCAFE_INSTALL_INSTHEAP,
					sdpl, aid, aidlen, instparam, instparamlen, heap );
			} else {
				ret = scSmartcafeCmdInstall( ri, ci, SC_SMARTCAFE_INSTALL_INST,
					sdpl, aid, aidlen, instparam, instparamlen, 0 );
			}
			scSmartcardSimpleProcessSW( ci, &i, NULL );
			if( (ret!=SC_EXIT_OK) || (i!=SC_SW_OK) ) {
				printf( "Error: Unable to finalise applet.\n" );
				ret=4;
				break;
			}
		}

		ret=0;
	} while( 0 );

	if( ret!=0 ) {
		if( ci->sw[0]==0 ) printf( "ret: %d\n", ret );
		else printf( "SW: %.2X%.2X\n", ci->sw[0], ci->sw[1] );
	}

	if( scReaderDeactivate( ri )!=0 ) printf( "Error: scReaderDeactivate\n" );
	if( scReaderShutdown( ri )!=0 ) printf( "Error: scReaderShutdown\n" );

	scGeneralFreeCard( &ci );
	scGeneralFreeReader( &ri );

	scEnd();

	return( ret );
}

