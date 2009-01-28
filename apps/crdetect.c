/****************************************************************************
*																			*
*					SCEZ chipcard library - Reader detect					*
*						Copyright Matthias Bruestle 2001					*
*					For licensing, see the file COPYRIGHT					*
*																			*
****************************************************************************/

/* $Id: crdetect.c 1617 2005-11-03 17:41:39Z laforge $ */

/* #define DEBUG */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include <scez/scinternal.h>
#ifdef WITH_ACR20
#include <scez/readers/scacr20.h>
#endif
#ifdef WITH_BLUEDOT
#include <scez/readers/scbluedot.h>
#endif
#ifdef WITH_CTAPI
#include <scez/scctapi.h>
#endif
#ifdef WITH_DUMBMOUSE
#include <scez/readers/scdumbmouse.h>
#endif
#ifdef WITH_EASYCHECK
#include <scez/cards/sceasycheck.h>
#endif
#ifdef WITH_GCR400
#include <scez/readers/scgcr400.h>
#endif
#ifdef WITH_INTERTEX
#include <scez/readers/scintertex.h>
#endif
#ifdef WITH_REFLEX20
#include <scez/readers/screflex20.h>
#endif
#ifdef WITH_REFLEX60
#include <scez/readers/screflex60.h>
#endif
#ifdef WITH_TOWITOKO
#include <scez/readers/sctowitoko.h>
#endif

#define MAX_RDI	20

int main( int argc, char *argv[] )
{
	SC_READER_DETECT_INFO rdi[MAX_RDI];
	char *ports[]={"0","1","2","3"};
	BOOLEAN verbose=FALSE;
	int i, ret, usage=0, maxprob=0, maxprobidx=-1, maxprob2=0, maxprob2idx=-1;

	if( (argc==2) && (*argv[1]=='v') ) {
#ifdef DEBUG
		printf("[VERBOSE]\n");
#endif /* DEBUG */
		verbose = TRUE;
	}

	for( i=0; i<4; i++ ) {
		if( verbose ) printf( "Port %d:\n", i );

		/* TODO: Look for lock files. */

#ifdef WITH_ACR20
		strncpy( rdi[usage].param, ports[i], SC_READER_PARAM_MAXLEN );
		ret=scAcr20Detect( &rdi[usage] );
		if( (ret==SC_EXIT_OK) && (rdi[usage].prob!=0) ) {
			if( rdi[usage].prob>=maxprob ) {
				maxprob2idx=maxprobidx;
				maxprob2=maxprob;
				maxprob=rdi[usage].prob;
				maxprobidx=usage;
			} else if( rdi[usage].prob>=maxprob2 ) {
				maxprob2=rdi[usage].prob;
				maxprob2idx=usage;
			}
			if( verbose ) {
				printf( "  ACR20: Probability %d, Major %d, Minor %d\n",
					rdi[usage].prob, rdi[usage].major, rdi[usage].minor );
			}
			usage++;
			if( usage==MAX_RDI ) break;
		}
#endif /* WITH_ACR20 */

#ifdef WITH_BLUEDOT
		strncpy( rdi[usage].param, ports[i], SC_READER_PARAM_MAXLEN );
		ret=scBluedotDetect( &rdi[usage] );
		if( (ret==SC_EXIT_OK) && (rdi[usage].prob!=0) ) {
			if( rdi[usage].prob>=maxprob ) {
				maxprob2idx=maxprobidx;
				maxprob=rdi[usage].prob;
				maxprobidx=usage;
			} else if( rdi[usage].prob>=maxprob2 ) {
				maxprob2=rdi[usage].prob;
				maxprob2idx=usage;
			}
			if( verbose ) {
				printf( "  Blue Dot Connector: Probability %d, Major %d, Minor %d\n",
					rdi[usage].prob, rdi[usage].major, rdi[usage].minor );
			}
			usage++;
			if( usage==MAX_RDI ) break;
		}
#endif /* WITH_CT-API */

#ifdef WITH_CTAPI
		strncpy( rdi[usage].param, ports[i], SC_READER_PARAM_MAXLEN );
		ret=scCtapiDetect( &rdi[usage] );
		if( (ret==SC_EXIT_OK) && (rdi[usage].prob!=0) ) {
			if( rdi[usage].prob>=maxprob ) {
				maxprob2idx=maxprobidx;
				maxprob=rdi[usage].prob;
				maxprobidx=usage;
			} else if( rdi[usage].prob>=maxprob2 ) {
				maxprob2=rdi[usage].prob;
				maxprob2idx=usage;
			}
			if( verbose ) {
				printf( "  CT-API: Probability %d, Major %d, Minor %d\n",
					rdi[usage].prob, rdi[usage].major, rdi[usage].minor );
			}
			usage++;
			if( usage==MAX_RDI ) break;
		}
#endif /* WITH_CT-API */

#ifdef WITH_DUMBMOUSE
		strncpy( rdi[usage].param, ports[i], SC_READER_PARAM_MAXLEN );
		ret=scDumbmouseDetect( &rdi[usage] );
		if( (ret==SC_EXIT_OK) && (rdi[usage].prob!=0) ) {
			if( rdi[usage].prob>=maxprob ) {
				maxprob2idx=maxprobidx;
				maxprob=rdi[usage].prob;
				maxprobidx=usage;
			} else if( rdi[usage].prob>=maxprob2 ) {
				maxprob2=rdi[usage].prob;
				maxprob2idx=usage;
			}
			if( verbose ) {
				printf( "  Dumb Mouse: Probability %d, Major %d, Minor %d\n",
					rdi[usage].prob, rdi[usage].major, rdi[usage].minor );
			}
			usage++;
			if( usage==MAX_RDI ) break;
		}
#endif /* WITH_DUMBMOUSE */

#ifdef WITH_EASYCHECK
		strncpy( rdi[usage].param, ports[i], SC_READER_PARAM_MAXLEN );
		ret=scEasycheckDetect( &rdi[usage] );
		if( (ret==SC_EXIT_OK) && (rdi[usage].prob!=0) ) {
			if( rdi[usage].prob>=maxprob ) {
				maxprob2idx=maxprobidx;
				maxprob=rdi[usage].prob;
				maxprobidx=usage;
			} else if( rdi[usage].prob>=maxprob2 ) {
				maxprob2=rdi[usage].prob;
				maxprob2idx=usage;
			}
			if( verbose ) {
				printf( "  EasyCheck: Probability %d, Major %d, Minor %d\n",
					rdi[usage].prob, rdi[usage].major, rdi[usage].minor );
			}
			usage++;
			if( usage==MAX_RDI ) break;
		}
#endif /* WITH_DUMBMOUSE */

#ifdef WITH_INTERTEX
		strncpy( rdi[usage].param, ports[i], SC_READER_PARAM_MAXLEN );
		ret=scIntertexDetect( &rdi[usage] );
		if( (ret==SC_EXIT_OK) && (rdi[usage].prob!=0) ) {
			if( rdi[usage].prob>=maxprob ) {
				maxprob2idx=maxprobidx;
				maxprob=rdi[usage].prob;
				maxprobidx=usage;
			} else if( rdi[usage].prob>=maxprob2 ) {
				maxprob2=rdi[usage].prob;
				maxprob2idx=usage;
			}
			if( verbose ) {
				printf( "  Intertex: Probability %d, Major %d, Minor %d\n",
					rdi[usage].prob, rdi[usage].major, rdi[usage].minor );
			}
			usage++;
			if( usage==MAX_RDI ) break;
		}
#endif /* WITH_INTERTEX */

#ifdef WITH_REFLEX60
		strncpy( rdi[usage].param, ports[i], SC_READER_PARAM_MAXLEN );
		ret=scReflex60Detect( &rdi[usage] );
		if( (ret==SC_EXIT_OK) && (rdi[usage].prob!=0) ) {
			if( rdi[usage].prob>=maxprob ) {
				maxprob2idx=maxprobidx;
				maxprob=rdi[usage].prob;
				maxprobidx=usage;
			} else if( rdi[usage].prob>=maxprob2 ) {
				maxprob2=rdi[usage].prob;
				maxprob2idx=usage;
			}
			if( verbose ) {
				printf( "  Reflex 62/64: Probability %d, Major %d, Minor %d\n",
					rdi[usage].prob, rdi[usage].major, rdi[usage].minor );
			}
			usage++;
			if( usage==MAX_RDI ) break;
		}
#endif /* WITH_REFLEX60 */

#ifdef WITH_TOWITOKO
		strncpy( rdi[usage].param, ports[i], SC_READER_PARAM_MAXLEN );
		ret=scTowitokoDetect( &rdi[usage] );
		if( (ret==SC_EXIT_OK) && (rdi[usage].prob!=0) ) {
			if( rdi[usage].prob>=maxprob ) {
				maxprob2idx=maxprobidx;
				maxprob=rdi[usage].prob;
				maxprobidx=usage;
			} else if( rdi[usage].prob>=maxprob2 ) {
				maxprob2=rdi[usage].prob;
				maxprob2idx=usage;
			}
			if( verbose ) {
				printf( "  Towitoko: Probability %d, Major %d, Minor %d\n",
					rdi[usage].prob, rdi[usage].major, rdi[usage].minor );
			}
			usage++;
			if( usage==MAX_RDI ) break;
		}
#endif /* WITH_TOWITOKO */
	}

	if( maxprobidx!=-1 ) {
		if( verbose ) {
			printf( "\n" );
			printf( "Best hit:\n" );
			printf( "  Probability: %d\n", rdi[maxprobidx].prob );
			printf( "  Name: %s\n", rdi[maxprobidx].name );
			printf( "  Param: \"%s\"\n", rdi[maxprobidx].param );
			printf( "  Major: %d\n", rdi[maxprobidx].major );
			printf( "  Minor: %d\n", rdi[maxprobidx].minor );
			printf( "  Slots: %d\n", rdi[maxprobidx].slots );
			printf( "  PIN-Pad: %s\n", rdi[maxprobidx].pinpad ? "yes" : "no" );
			printf( "  Display: %s\n", rdi[maxprobidx].display ? "yes" : "no" );
			if( maxprob2idx!=-1 ) {
				printf( "\n" );
				printf( "Second best hit:\n" );
				printf( "  Probability: %d\n", rdi[maxprob2idx].prob );
				printf( "  Name: %s\n", rdi[maxprob2idx].name );
				printf( "  Param: \"%s\"\n", rdi[maxprob2idx].param );
				printf( "  Major: %d\n", rdi[maxprob2idx].major );
				printf( "  Minor: %d\n", rdi[maxprob2idx].minor );
				printf( "  Slots: %d\n", rdi[maxprob2idx].slots );
				printf( "  PIN-Pad: %s\n", rdi[maxprob2idx].pinpad ? "yes" :
					"no" );
				printf( "  Display: %s\n", rdi[maxprob2idx].display ? "yes" :
					"no" );
			}
		}
		if( verbose ) printf( "\nSCEZ_READER=" );
		printf( "%s,1,%s", rdi[maxprobidx].name, rdi[maxprobidx].param );
		if( verbose ) printf( "\n" );
	}

	return(0);
}



