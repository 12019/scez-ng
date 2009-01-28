/****************************************************************************
*																			*
*					PKCS#15 library - Cryptoflex routines					*
*						Copyright Matthias Bruestle 1999					*
*					For licensing, see the file COPYRIGHT					*
*																			*
****************************************************************************/

/* $Id: p15cryptoflex.h 875 2000-09-01 15:31:27Z zwiebeltu $ */

#ifndef P15_CRYPTOFLEX_H
#define P15_CRYPTOFLEX_H

#ifndef P15_GENERAL_H
#error p15general.h needs to be included before this.
#endif

#define P15_CRYPTOFLEX_EFDIR_SIZE	150

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* Read directory and file locations. */
/* static */ int p15CryptoflexGetPaths( P15_INFO *ci );

/* Write or change EF(DIR) in MF. */
/* static */ int p15CryptoflexWriteEfDir( P15_INFO *pi, char *label );

/* Initialize card. */
int p15CryptoflexInitCard( P15_INFO *pi, char *label, char *manufId, BYTE *sn,
    int snlen );

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* P15_CRYPTOFLEX_H */

