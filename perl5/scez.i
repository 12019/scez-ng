/* $Id: scez.i 1007 2001-02-13 10:43:09Z zwiebeltu $ */

%module scez
%{
#include "scgeneral.h"
#include "screader.h"
#include "scsmartcard.h"
#ifdef WITH_BASICCARD
#include "scbasiccard.h"
#endif
#ifdef WITH_CRYPTOFLEX
#include "sccryptoflex.h"
#endif
#ifdef WITH_CYBERFLEX
#include "sccyberflex.h"
#endif
#ifdef WITH_GELDKARTE
#include "scgeldkarte.h"
#endif
#ifdef WITH_GPK4000
#include "scgpk4000.h"
#endif
#ifdef WITH_GSMSIM
#include "scgsmsim.h"
#endif
#ifdef WITH_MFC
#include "scmfc.h"
#endif
#ifdef WITH_MULTIFLEX
#include "scmultiflex.h"
#endif
#ifdef WITH_OPENPLATFORM
#include "scopenplatform.h"
#endif
#ifdef WITH_PROTON
#include "scproton.h"
#endif
#ifdef WITH_QUICK
#include "scquick.h"
#endif
#ifdef WITH_SMARTCAFE
#include "scsmartcafe.h"
#endif
#ifdef WITH_TCOS
#include "sctcos.h"
#endif
%}

/* int* is probably never a pointer to an array. */

%typemap(perl5,in) int * {
  SV* tempsv;
  static int ivalue;
  if (!SvROK($source)) {
    croak("expected a reference\n");
  }
  tempsv = SvRV($source);
  if (SvTYPE(tempsv) != SVt_IV) {
    croak("expected a integer reference\n");
  }
  ivalue = SvIV(tempsv);
  $target = &ivalue;
}

%typemap(perl5,argout) int * {
  SV *tempsv;
  tempsv = SvRV($arg);
  sv_setiv(tempsv, (IV) *$source);
}


%include "scgeneral.h"
%include "screader.h"
%include "scsmartcard.h"
#ifdef WITH_BASICCARD
%include "scbasiccard.h"
#endif
#ifdef WITH_CRYPTOFLEX
%include "sccryptoflex.h"
#endif
#ifdef WITH_CYBERFLEX
%include "sccyberflex.h"
#endif
#ifdef WITH_GELDKARTE
%include "scgeldkarte.h"
#endif
#ifdef WITH_GPK4000
%include "scgpk4000.h"
#endif
#ifdef WITH_GSMSIM
%include "scgsmsim.h"
#endif
#ifdef WITH_MFC
%include "scmfc.h"
#endif
#ifdef WITH_MULTIFLEX
%include "scmultiflex.h"
#endif
#ifdef WITH_OPENPLATFORM
%include "scopenplatform.h"
#endif
#ifdef WITH_PROTON
%include "scproton.h"
#endif
#ifdef WITH_QUICK
%include "scquick.h"
#endif
#ifdef WITH_SMARTCAFE
%include "scsmartcafe.h"
#endif
#ifdef WITH_TCOS
%include "sctcos.h"
#endif

/* Helper functions for C arrays */

%inline %{

BYTE *BYTE_alloc( int size ) {
	return (BYTE *) malloc( sizeof(BYTE)*size );
}

void BYTE_free( BYTE *a ) {
	free( a );
}

/* Should val be BYTE or int? */

void BYTE_set( BYTE *a, int i, int val ) {
	a[i] = (BYTE) (val & 0xFF);
}

/* Should the return value be BYTE or int? */

int BYTE_get( BYTE *a, int i ) {
	return a[i];
}

void SC_CARD_INFO_type_set( SC_CARD_INFO *ci, LONG type ) {
	if( ci!=NULL ) ci->type=type;
}

LONG SC_CARD_INFO_type_get( SC_CARD_INFO *ci ) {
	if( ci!=NULL ) return ci->type;
	return SC_CARD_UNKNOWN;
}

/* Sets atr and atrlen. */

void SC_CARD_INFO_atr_set( SC_CARD_INFO *ci, BYTE *atr, int atrlen ) {
	if( (ci!=NULL) && (atrlen<=sizeof(ci->atr)) ) {
		memcpy( ci->atr, atr, atrlen );
	}
}

/* Returns atrlen. */

int SC_CARD_INFO_atr_get( SC_CARD_INFO *ci, BYTE *atr ) {
	if( ci!=NULL ) {
		if( atr!=NULL ) memcpy( atr, ci->atr, ci->atrlen );
		return ci->atrlen;
	}
	return 0;
}

void SC_CARD_INFO_protocol_set( SC_CARD_INFO *ci, int protocol ) {
	if( ci!=NULL ) ci->protocol=protocol;
}

int SC_CARD_INFO_protocol_get( SC_CARD_INFO *ci ) {
	if( ci!=NULL ) return ci->protocol;
	return SC_PROTOCOL_UNKNOWN;
}

/* Should cla be BYTE or int? */

void SC_CARD_INFO_cla_set( SC_CARD_INFO *ci, int cla ) {
	if( ci!=NULL ) ci->cla=(BYTE) (cla&0xFF);
}

int SC_CARD_INFO_cla_get( SC_CARD_INFO *ci ) {
	if( ci!=NULL ) return ci->cla;
	return 0;
}

/* Sets swok. */

void SC_CARD_INFO_swok_set( SC_CARD_INFO *ci, BYTE *swok ) {
	if( (ci!=NULL) && (swok!=NULL) ) {
		memcpy( ci->swok, swok, sizeof(ci->swok) );
	}
}

/* Copies swok. */

void SC_CARD_INFO_swok_get( SC_CARD_INFO *ci, BYTE *swok ) {
	if( ci!=NULL ) {
		if( swok!=NULL ) memcpy( swok, ci->swok, sizeof(ci->swok) );
	}
}

/* Sets swav. */

void SC_CARD_INFO_swav_set( SC_CARD_INFO *ci, BYTE *swav ) {
	if( (ci!=NULL) && (swav!=NULL) ) {
		memcpy( ci->swav, swav, sizeof(ci->swav) );
	}
}

/* Copies swav. */

void SC_CARD_INFO_swav_get( SC_CARD_INFO *ci, BYTE *swav ) {
	if( ci!=NULL ) {
		if( swav!=NULL ) memcpy( swav, ci->swav, sizeof(ci->swav) );
	}
}

/* Sets sw. */
/* Should sw be unsigned short or int? */

void SC_CARD_INFO_sw_set( SC_CARD_INFO *ci, int sw ) {
	if( (ci!=NULL) ) {
		ci->sw[0]=(BYTE) ((sw>>8)&0xFF);
		ci->sw[1]=(BYTE) (sw&0xFF);
	}
}

/* Returns sw. */
/* Should sw be unsigned short or int? */

int SC_CARD_INFO_sw_get( SC_CARD_INFO *ci ) {
	if( ci!=NULL ) {
		return ((ci->sw[0]<<8)+ci->sw[1]);
	}
	return 0;
}

void SC_CARD_INFO_free( SC_CARD_INFO *ci ) {
	scGeneralFreeCard( &ci );
}

void SC_READER_INFO_major_set( SC_READER_INFO *ri, int major ) {
	if( ri!=NULL ) ri->major=major;
}

int SC_READER_INFO_major_get( SC_READER_INFO *ri ) {
	if( ri!=NULL ) return ri->major;
	return 0;
}

void SC_READER_INFO_minor_set( SC_READER_INFO *ri, int minor ) {
	if( ri!=NULL ) ri->minor=minor;
}

int SC_READER_INFO_minor_get( SC_READER_INFO *ri ) {
	if( ri!=NULL ) return ri->minor;
	return 0;
}

void SC_READER_INFO_slot_set( SC_READER_INFO *ri, int slot ) {
	if( ri!=NULL ) ri->slot=slot;
}

int SC_READER_INFO_slot_get( SC_READER_INFO *ri ) {
	if( ri!=NULL ) return ri->slot;
	return 0;
}

void SC_READER_INFO_status_set( SC_READER_INFO *ri, int status ) {
	if( ri!=NULL ) ri->status=status;
}

int SC_READER_INFO_status_get( SC_READER_INFO *ri ) {
	if( ri!=NULL ) return ri->status;
	return 0;
}

void SC_READER_INFO_free( SC_READER_INFO *ri ) {
	scGeneralFreeReader( &ri );
}

%}

