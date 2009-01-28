/****************************************************************************
*																			*
*						Copyright Matthias Bruestle 2000					*
*					For licensing, see the file COPYRIGHT					*
*																			*
****************************************************************************/

/* $Id: simman.h 875 2000-09-01 15:31:27Z zwiebeltu $ */

#define PART_NONE	0
#define PART_INFO	1
#define PART_MISC	2
#define PART_PHONE	3
#define PART_SMS	4
#define PART_DONE	5

#define MAX_LINE_LEN	1024

typedef struct phonenumber {
	BYTE name[242];
	int	numlen;
	BYTE number[11];
} PHONENUMBER;

typedef struct sms {
	BYTE status;
	BOOLEAN mrset;
	BYTE mr;
	int numlen;
	BYTE number[13];
	BOOLEAN pidset;
	BYTE pid;
	BYTE dcs;
	BOOLEAN sctsset;
	BYTE scts[7];
	int	vpl;
	BYTE vp[7];
	int udl;
	int udlb;
	BOOLEAN dump;
	BYTE ud[255];
} SMS;

typedef struct simdata {
	int part;

	/* Info */
	char *iccid;

	/* Misc */
	int lsize;
	BYTE *lp;

	/* Phonebook */
	int psize;
	PHONENUMBER *prec;

	/* SMS */
	int ssize;
	SMS *srec;
} SIMDATA;

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

void fatal(SC_READER_INFO *ri, char *format, ...);

void parse_data(FILE *fs, SIMDATA *sd, SC_READER_INFO *ri);

#ifdef __cplusplus
}
#endif /* __cplusplus */

