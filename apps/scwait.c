/* vim:tw=76
*/
/**************************************************************************
* This program waits for smartcard insertion into a reader, with lots of
* knobs to have fun with.
*
* Programming by Per Harald Myrvang <perm@pasta.cs.uit.no>
* (C) 2000  University of Tromsø, Norway
*
* $Id: scwait.c 1617 2005-11-03 17:41:39Z laforge $
*
* TODO: Fix error that occasionally happens when a card is inserted at the
* same time the reader is powered up
* TODO: Turn this into a library function
*
* This file is distributed under the BSD licence, version 2, which states
* that the licenced object can be used for any purpose, commercial or not,
* and that the licenced object comes with absolutely no warranty! Please see
* the COPYRIGHT file for more information
*
* $Log: scwait.c,v $
* Revision 1.6  2000/10/25 14:56:00  zwiebeltu
* - Moved the status variable from the card object to the reader object.
*
* Revision 1.5  2000/08/28 08:47:43  zwiebeltu
* - Updated to work again under PalmOS.
*
* Revision 1.4  2000/08/27 19:31:30  per
* Lots of additional useful features and options. Contains hooks to support
* waiting for other cards than Schlumberger's Cyberflex.
*
**************************************************************************/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <getopt.h>
#include <sys/time.h>
#if defined(__linux__)
#include <unistd.h>
#endif

#include <scez/scgeneral.h>
#include <scez/readers/sctowitoko.h>
#include <scez/cards/scmultiflex.h>

/* Default stuff */
#ifndef READER_TYPE
#define READER_TYPE SC_READER_TOWITOKO
#endif /* READER_TYPE */
#ifndef READER_SLOT
#define READER_SLOT 1
#endif /* READER_SLOT */
#ifndef READER_PORT
#define READER_PORT "0"
#endif /* READER_PORT */


/* Print usage information on stdout. Never returns (calls exit(0)).
 * @param argv0 - program name, i.e. argv[0] from main().
 */
void usage(char *argv0) {
    printf("Usage: %s [-h][-a][-l][-t type]\n", argv0);
    printf(
"    -a        - accept any type of card (no card type checking)\n"\
"    -h        - this help text\n"\
"    -l        - loop mode, continually probe for smartcard presence,\n"\
"                every 1/2-5 second, displaying the result on stdout\n"\
"                (0 = no card, 1 = card, 2 = card of wrong type).\n"\
"                Writing \"0\\n\" or \"1\\n\" to stdin toggles checking modus,\n"\
"                and writing \"X\\n\" ends loop mode.\n"\
"    -t <type> - type of smartcard to wait for, currently one of\n"\
"                0=cyberflex (default) (TODO: add more types!)\n"\
	  );
    exit(0);	/* EARLY TERMINATION */
}


fd_set fdset;		/* for waiting for stdin action (in loopmode) */
struct timeval tv = {0L, 100000L};	/* 10 microsecond delay */

int loopmode = 0;
int card_type_check = 1;
int required_card_type = 0;

struct smartcard_types_list_s {
    int	id;		/* card id (sequential numbering) */
    char *name;		/* card name */
    int value;		/* expected value of ci->type */
    int mask;		/* mask value of ci->type, applied before comparison */
} smartcard_list[] = {
    {0, "cyberflex", SC_CARD_CYBERFLEX, 0xff00},
    {-1, NULL}		/* List terminator */
};


/* See if there's anything on stdin, then act on the input. Modifies global
 * data. */
void ctrl_check() {
    char buf[4];
    int suspend = 0;

    if (0 == loopmode)
	return;

    /* See if there's anything to read at stdin */
    do {
	FD_ZERO(&fdset);
	FD_SET(0, &fdset);	/* i.e. stdin */
	memset(buf, 0, 4);
	if (1 == select(2, &fdset, NULL, NULL, &tv)) {
	    if (1 != read(0, buf, 1)) {
		fprintf(stderr, "##DEBUG: ctrl_check(): EOF!\n");
		loopmode = 0;
	    }
	    if ('1' == buf[0]) {
#if 0
		/* When we resume after a suspend,
		 * re-activate the reader, reset the card,
		 * and ignore the result. */
		if (1 == suspend) {
		    scReaderActivate(ri);
		    scReaderResetCard(ri, ci);
		}
#endif
		suspend = 0;
	    } else if ('0' == buf[0]) {
#if 0
		/* When we suspend, reset the card, then
		 * deactivate the reader, and ignore the
		 * result. */
		if (0 == suspend) {
		    scReaderResetCard(ri, ci);
		    scReaderDeactivate(ri);
		}
#endif
		suspend = 1;
	    } else if ('X' == buf[0]) {
		loopmode = 0;
		suspend = 0;
	    } else {
		;	/* Ignore anything else */
	    }
	}
	if (suspend)
	    usleep(200000);	/* wait 1/5 second */
    } while (suspend);
}

int main( int argc, char *argv[] ) {
    SC_READER_INFO *ri;
    SC_CARD_INFO *ci;
    SC_READER_CONFIG rc;

    int ret;
    int i;
    int retval = 1;
    char c;

    
    /* do ops */ 
    while(EOF != (c = getopt(argc, argv, "ahlt:"))) {
	switch(c) {
	    case 'a':
		card_type_check = 0;
		break;
	    case 'l':
		loopmode = 1;
		break;
	    case 't':
		required_card_type = atoi(optarg);
		break;
	    case 'h':
	    default:
	    	usage(argv[0]);
		break;
	}
    }

    /* Sanity checks */
    for (i = 0; NULL != smartcard_list[i].name; i++) {
	if (required_card_type == smartcard_list[i].id)
	    break;
    }
    if (NULL == smartcard_list[i].name) {
	fprintf(stderr, "**ERROR: unknown card type %d! Known types are:\n",
		required_card_type);
	for (i = 0; NULL != smartcard_list[i].name; i++) {
	    fprintf(stderr, "\t%d - %s\n",
		    smartcard_list[i].id,
		    smartcard_list[i].name);
	}
	exit(1);	/* EARLY TERMINATION */
    }



    if (scInit()) {
	fprintf(stderr, "**Error: scInit() failed!\n");
	return(1);
    }

    rc.param = READER_PORT;

    ret = scReaderGetConfig( argc, argv, &rc );
    if( ret!=SC_EXIT_OK ) {
	fprintf(stderr, "**Error: scReaderGetConfig() failed!\n" );
	scEnd();
	return(1);
    };

    ri = scGeneralNewReader( READER_TYPE, 1 );
    if( ri==NULL ) {
	fprintf(stderr, "**Error: scGeneralNewReader() failed!\n");
	scEnd();
	return(1);
    };

    ci = scGeneralNewCard( );
    if (ci==NULL) {
	fprintf(stderr, "**Error: scGeneralNewCard() failed!\n");
	scEnd();
	return(1);
    };
    /* Init Reader */
    if (0 == scReaderInit( ri, rc.param )) {
	/* Activate Card */
	if (0 == scReaderActivate(ri)) {
	    /* Loop while loopmodus is set */
	    do {
		/* Get Card Status */
		while(1) {
		    if (0 != scReaderCardStatus( ri )) {
			fprintf(stderr, "scReaderCardStatus Error.\n");
			break;
		    }
		    if (ri->status & SC_CARD_STATUS_PRESENT) {
			/* Re-Activate Card, and _ignore_ return code. This way
			 * we give the card a chance to power up before we go
			 * ahead and reset it below.  */
			scReaderActivate( ri );	
			break;
		    }
		    if (loopmode) {
			fprintf(stdout, "0\n");
			fflush(stdout);
			ctrl_check();
		    }
		    usleep(500000);  /* 1/2 second delay before next check */
		}

		if (loopmode) {
		    ctrl_check();
		}

		/* Get Card Type */
		if (card_type_check) {
		    /* Reset Card */
		    if (0 == scReaderResetCard( ri, ci )) {
			if (0 == scSmartcardGetCardType(ci)) {
			    /* Is card of required type? */
			    if (smartcard_list[required_card_type].value ==
				    (smartcard_list[required_card_type].mask
				     & ci->type)) {
				if (loopmode) {
				    fprintf(stdout, "1\n");
				    fflush(stdout);
				    /* Wait some more, in order not to
				     * stress the card */
				    usleep(200000);  /* wait 1/5 second */
				}
				retval = 0;
			    } else {	/* Card NOT of required type! */
				fprintf(stderr, "Not a %s card!\n",
					smartcard_list[required_card_type].name);
				if (loopmode) {
				    fprintf(stdout, "2\n");
				    fflush(stdout);
				    /* Wait some more, in order not to
				     * stress the card */
				    usleep(200000);  /* wait 1/5 second */
				}
				retval = 1;
			    }
			} else {
			    fprintf(stderr, "scSmartcardGetCardType error.\n");
			}
		    } else {
			/* If we're in loop mode, a scReaderResetCard might
			 * indicate that the some non-smartcard card has
			 * been inserted, so we inform the user that the
			 * card is not of the required type! */
			if (loopmode) {
			    fprintf(stdout, "2\n");
			    fflush(stdout);
			    /* Wait some more, in order not to
			     * stress the card */
			    usleep(200000);  /* wait 1/5 second */
			} else {
			    fprintf(stderr, "scReaderResetCard error.\n");
			}
		    }
		} else { /* i.e. accept any type of card */
		    if (loopmode) {
			fprintf(stdout, "1\n");
			fflush(stdout);
			usleep(200000);  /* 1/5 second before next check */
		    } else {
			retval = 0;
		    }
		}
	    } while(loopmode);

	    if (0 != scReaderDeactivate(ri)) {
		fprintf(stderr, "scReaderDeactivate error!\n");
	    }
	} else {
	    fprintf(stderr, "scReaderActivate error.\n");
	}
	if (0 != scReaderShutdown( ri )) {
	    fprintf(stderr, "scReaderShutdown error.\n");
	}
    } else {
	fprintf(stderr, "scReaderInit error.\n");
    }

    scEnd();

    return retval;
}

