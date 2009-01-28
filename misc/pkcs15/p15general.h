/****************************************************************************
*																			*
*						PKCS#15 library - General routines					*
*						Copyright Matthias Bruestle 1999					*
*					For licensing, see the file COPYRIGHT					*
*																			*
****************************************************************************/

/* $Id: p15general.h 875 2000-09-01 15:31:27Z zwiebeltu $ */

#ifndef P15_GENERAL_H
#define P15_GENERAL_H

/* If P15_AUTH_ALLWAYS is defined External Authentication and Verify PIN
 * is done in every function which requires this, else it asumes it
 * allready happened and does it, when it leaves and reenters the
 * directories, where authentication is required.
 */
#define P15_AUTH_ALLWAYS

/* Structs */

typedef struct p15_index {
	BYTE	path[10];
	int		pathlen;
	WORD	index;
	WORD	length;
} P15_INDEX;

typedef struct p15_info {
	SC_READER_INFO	*ri;
	SC_CARD_INFO	*ci;
	BYTE	dirpath[8];		/* DF(PKCS#15) path. */
	int		dirpathlen;
	BYTE	odfpath[10];	/* EF(ODF) path. */
	int		odfpathlen;
	BYTE	tipath[10];		/* EF(TokenInfo) path. */
	int		tipathlen;
	BYTE	uspath[10];		/* EF(UnusedSpace) path. */
	int		uspathlen;
	BYTE	sokey[16];		/* SO key. */
	BYTE	sokeylen;		/* SO key length. */
	BYTE	pin1[8];		/* PIN1. */
	BYTE	pin1nr;			/* Number of PIN1. */
	BYTE	pin1len;		/* PIN1 length. */
	BYTE	pin2[8];		/* PIN2. */
	BYTE	pin2nr;			/* Number of PIN2. */
	BYTE	pin2len;		/* PIN2 length. */
	P15_INDEX	index;		/* Current filestatus. */
} P15_INFO;

#define	P15_PUBALGO_RSA	0x01	/* Elements: e,n */
#define	P15_PUBALGO_DSA	0x02	/* Elements:  */
#define	P15_PUBALGO_ECC	0x03	/* Elements:  */

typedef struct p15_key {
	BYTE	algo;
	WORD	keylen;
	struct p15_keypart	*next;
} P15_KEY;

#define P15_KEYPART_RSA_N	0x10
#define P15_KEYPART_RSA_E	0x11
#define P15_KEYPART_RSA_D	0x12
#define P15_KEYPART_RSA_J0	0x13
#define P15_KEYPART_RSA_H	0x14
#define P15_KEYPART_RSA_P	0x15
#define P15_KEYPART_RSA_Q	0x16
#define P15_KEYPART_RSA_A	0x17	/* Q^-1 mod P */
#define P15_KEYPART_RSA_C	0x18	/* D mod (P-1) */
#define P15_KEYPART_RSA_F	0x19	/* D mod (Q-1) */
#define P15_KEYPART_DSA_P	0x20
#define P15_KEYPART_DSA_Q	0x21
#define P15_KEYPART_DSA_G	0x22
#define P15_KEYPART_DSA_Y	0x23
#define P15_KEYPART_DSA_Z	0x24
#define P15_KEYPART_ECC_A	0x30
#define P15_KEYPART_ECC_B	0x31
#define P15_KEYPART_ECC_R	0x32
#define P15_KEYPART_ECC_K	0x33
#define P15_KEYPART_ECC_G	0x34
/* #define P15_KEYPART_ECC_???	0x35 */
#define P15_KEYPART_ECC_S	0x36	/* Secret Key */

typedef struct p15_keypart {
	BYTE	keypart;
	WORD	partlen;
	BYTE	*part;
	struct p15_keypart	*next;
} P15_KEYPART;

/* For padding see GPK4000 manual.

#define P15_PADDING_NONE	0x01
#define P15_PADDING_
#define P15_PADDING_
#define P15_PADDING_
*/

typedef struct p15_algoparam {
	BYTE	algo;		/* Public key algorythm. */
	WORD	bits;		/* Keysize in bits. */
	WORD	sbits;		/* Size of chunk to sign. */
	BYTE	padding;	/* Type of padding. */
	LONG	keyparts;	/* Key parts required if secret key is loaded onto the
						 * card. Lower nibble of P15_KEYPART_* specifies bit
						 * number.
						 */
	struct p15_algoparam	*next;
} P15_ALGOPARAM;

#endif /* P15_GENERAL_H */

