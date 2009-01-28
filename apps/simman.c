/****************************************************************************
*																			*
*				Copyright Matthias Bruestle, Gerd Rausch 1999				*
*					For licensing, see the file COPYRIGHT					*
*																			*
****************************************************************************/

/* $Id: simman.c 1617 2005-11-03 17:41:39Z laforge $ */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#if !defined(WINDOWS) && !defined(__palmos__)
#include <unistd.h> /* for sleep */
#elif defined(__BORLANDC__)
#include <dos.h>			 /* for sleep */
#elif defined(WINDOWS)
#include <windows.h>
#endif
#include <stdarg.h>
#include <errno.h>
#include <scez/scgeneral.h>
#include <scez/cards/scgsmsim.h>
#include "simman.h"

#ifndef READER_TYPE
#define READER_TYPE SC_READER_DUMBMOUSE
#endif /* READER_TYPE */
#ifndef READER_SLOT
#define READER_SLOT 1
#endif /* READER_SLOT */
#ifndef READER_PORT
#define READER_PORT "0"
#endif /* READER_PORT */

#define printarray( name, length, array ); \
    printf(name); \
    for( i=0; i<length; i++ ) printf(" %.2X",array[i]); \
    printf("\n");

char *prgName;

static char bcd2digit[]={
	'0', '1', '2', '3' , '4', '5', '6', '7', '8', '9',
	'*', '#', '.', 0, 0, 0
};

void fatal(SC_READER_INFO *ri, char *format, ...)
{
	va_list ap;

	if(ri!=NULL) {
		scReaderDeactivate( ri );
		scReaderShutdown( ri );
	}
	scEnd();

	va_start(ap, format);
	fprintf(stderr,"%s: ",prgName);
	vfprintf(stderr,format,ap);
	fprintf(stderr,"\n");
	va_end(ap);
	exit(1);
}

static void dump_info(FILE *fs, SC_READER_INFO *ri, SC_CARD_INFO *ci)
{ 
	BYTE buf[300], number[300], *bp;
	char c;
	int n, i, count;

	fprintf(fs, "[Info]\n");

	/* ICCID */

	if(scGsmsimCmdSelect(ri, ci, 0x3F00, buf, &n) || ci->sw[0]!=0x90 )
		fatal(ri, "Error selecting MF.");

	if(scGsmsimCmdSelect(ri, ci, 0x2FE2, buf, &n) || ci->sw[0]!=0x90 )
		fatal(ri, "Error selecting EFiccid.");

	n=10;
	if(scGsmsimCmdReadBin(ri, ci, 0, buf, &n) || ci->sw[0]!=0x90 || n!=10 )
		fatal(ri, "Error reading EFiccid.");

	fprintf(fs, "ICCID=");
	for( i=0; i<10; i++ ) {
		if( (buf[i]&0xF) < 10 ) fprintf(fs, "%c", (buf[i]&0xF)+0x30 );
		if( (buf[i]>>4) < 10 ) fprintf(fs, "%c", (buf[i]>>4)+0x30 );
	}
	fprintf(fs, "\n");

	/* Provider */

	if(scGsmsimCmdSelect(ri, ci, 0x7F20, buf, &n) || ci->sw[0]!=0x90 )
		fatal(ri, "Error selecting DFgsm.");

	if(!scGsmsimCmdSelect(ri, ci, 0x6F46, buf, &n) && ci->sw[0]==0x90 ) {
		n=17;
		if(scGsmsimCmdReadBin(ri, ci, 0, buf, &n) || ci->sw[0]!=0x90 || n!=17 )
			fatal(ri, "Error reading EFspn.");

		fprintf(fs, "Provider=");
		i=1;
		while( (i<17) && (buf[i]!=0xFF) ) fprintf(fs, "%c", buf[i++]);
		fprintf(fs, "\n");
	}

	/* Phase */

	if(scGsmsimCmdSelect(ri, ci, 0x6FAE, buf, &n) || ci->sw[0]!=0x90 )
		fatal(ri, "Error selecting EFphase.");

	n=1;
	if(scGsmsimCmdReadBin(ri, ci, 0, buf, &n) || ci->sw[0]!=0x90 || n!=1 )
		fatal(ri, "Error reading EFphase.");

	switch( buf[0] ) {
	case 0x00:
		fprintf(fs, "Phase=1\n");
		break;
	case 0x02:
		fprintf(fs, "Phase=2\n");
		break;
	case 0x03:
		fprintf(fs, "Phase=2+\n");
		break;
	default:
		fprintf(fs, "Phase=unknown\n");
	}

	/* EmergencyCallCodes */

	if(!scGsmsimCmdSelect(ri, ci, 0x6FB7, buf, &n) && ci->sw[0]==0x90 &&
		n>=14 ) {
		n=buf[3]; i=buf[3];
		if( n==0 ) fatal(ri, "Invalid size of EFecc.");
		if( n%3 ) fatal(ri, "Error selecting EFecc");

		if(scGsmsimCmdReadBin(ri, ci, 0, buf, &n) || ci->sw[0]!=0x90 ||
			n!=i )
			fatal(ri, "Error reading EFecc.");

		fprintf(fs, "EmergencyCallCodes=");
		i=0;
		while( i<n ) {
			bp=number;
			for( count=0; count<3; count++ ) {
				if((c=bcd2digit[buf[i]&0xF])) *bp++=c;
				if((c=bcd2digit[buf[i]>>4])) *bp++=c;
				i++;
			}
			*bp=0;
			fprintf(fs, "%s", number);
			if( i<n ) fprintf(fs, ",");
		}

		fprintf(fs, "\n");
	}

	/* MaxLanguageEntries */

	if(scGsmsimCmdSelect(ri, ci, 0x6F05, buf, &n) || ci->sw[0]!=0x90 )
		fatal(ri, "Error selecting EFlp.");

	fprintf(fs, "MaxLanguageEntries=%d\n", buf[3]);

	/* MaxFixedDialingNumbers */

	if(scGsmsimCmdSelect(ri, ci, 0x7F10, buf, &n) || ci->sw[0]!=0x90 )
		fatal(ri, "Error selecting DFtelecom.");

	if(!scGsmsimCmdSelect(ri, ci, 0x6F3B, buf, &n) && ci->sw[0]==0x90 ) {
		fprintf(fs, "MaxFixedDialingNumbers=%d\n", (buf[2]*256+buf[3])/buf[14]);
	}

	/* MaxPhonebookEntries */

	if(!scGsmsimCmdSelect(ri, ci, 0x6F3A, buf, &n) && ci->sw[0]==0x90 ) {
		fprintf(fs, "MaxPhonebookEntries=%d\n", (buf[2]*256+buf[3])/buf[14]);
	}

	/* MaxSmsEntries */

	if(!scGsmsimCmdSelect(ri, ci, 0x6F3C, buf, &n) && ci->sw[0]==0x90 &&
		n>=15 ) {
		fprintf(fs, "MaxSmsEntries=%d\n", (buf[2]*256+buf[3])/buf[14]);
	}

	fprintf(fs, "\n");
}

static void dump_misc(FILE *fs, SC_READER_INFO *ri, SC_CARD_INFO *ci)
{ 
	BYTE buf[300];
	int n, i;

	fprintf(fs, "[Misc]\n");

	/* LanguagePreferences */

	/*  0: German
	 *  1: English
	 *  2: Italian
	 *  3: French
	 *  4: Spanish
	 *  5: Dutch
	 *  6: Swedish
	 *  7: Danish
	 *  8: Portuguese
	 *  9: Finnish
	 * 10: Norwegian
	 * 11: Greek
	 * 12: Turkish
	 * 13: Hungarian
	 * 14: Polish
	 * 15: unspecified
	 */

	if(scGsmsimCmdSelect(ri, ci, 0x3F00, buf, &n) || ci->sw[0]!=0x90 )
		fatal(ri, "Error selecting MF.");

	if(scGsmsimCmdSelect(ri, ci, 0x7F20, buf, &n) || ci->sw[0]!=0x90 )
		fatal(ri, "Error selecting DFgsm.");

	if(scGsmsimCmdSelect(ri, ci, 0x6F05, buf, &n) || ci->sw[0]!=0x90 || n<14 )
		fatal(ri, "Error selecting EFlp.");

	n=buf[3]; i=buf[3];
	if( n==0 ) fatal(ri, "Invalid size of EFlp.");

	if(scGsmsimCmdReadBin(ri, ci, 0, buf, &n) || ci->sw[0]!=0x90 || n!=i )
		fatal(ri, "Error reading EFlp.");

	fprintf(fs, "LanguagePreferences=");
	for( i=0; i<n; i++ ) {
		if( buf[i]==0xFF ) break;
		if( i!=0 ) fprintf(fs, ",");
		fprintf(fs, "%d", buf[i]);
	}
	fprintf(fs, "\n");

	fprintf(fs, "\n");
}

static void dump_phone(FILE *fs, SC_READER_INFO *ri, SC_CARD_INFO *ci)
{ 
	BYTE buf[300], number[50], *name, *bp;
	char c;
	int n, i, count;
	int recordSize, recordIdx;

	fprintf(fs, "[Phonebook]\n");

	if(scGsmsimCmdSelect(ri, ci, 0x3F00, buf, &n) || ci->sw[0]!=0x90 )
		fatal(ri, "Error selecting MF.");

	if(scGsmsimCmdSelect(ri, ci, 0x7F10, buf, &n) || ci->sw[0]!=0x90 )
		fatal(ri, "Error selecting DFtelecom.");

	if(scGsmsimCmdSelect(ri, ci, 0x6F3A, buf, &n) || ci->sw[0]!=0x90 )
		fatal(ri, "Error selecting EFadn.");
	recordSize=buf[14];

	for(recordIdx=1; ; recordIdx++) {
		n=recordSize;
		if(scGsmsimCmdReadRec(ri, ci, recordIdx, SC_GSMSIM_RECMODE_ABS, buf, &n))
			fatal(ri, "Error reading record.");

		if(ci->sw[0]==0x94 && ci->sw[1]==0x02)
			break;
		if(ci->sw[0]!=0x90 || ci->sw[1]!=0x00)
			fatal(ri, "Error reading record.");

		count=buf[n-14];

		for(i=0; i<n-14 && buf[i]!=0xFF; i++) buf[i]=scGsmsimGsmToIso(buf[i]);
		buf[i]=0;

		name=buf;
		if(count<255) {
			i=n-12;
			bp=number;
			if((buf[n-13]&0x70)==0x10)
				*bp++='+';
			while(count-->0) {
				if((c=bcd2digit[buf[i]&0xF]))
					*bp++=c;
				if((c=bcd2digit[buf[i]>>4]))
					*bp++=c;
				i++;
			}
			*bp=0;

			fprintf(fs, "%3d: %s (%s)\n", recordIdx, name, number);
		}
	}

	fprintf(fs, "\n");
}

static void dump_sms(FILE *fs, SC_READER_INFO *ri, SC_CARD_INFO *ci)
{ 
	BYTE buf[300], ud[161], number[50], status, scts[7], mr=0, pid, vp[7], dcs;
	int vpl, udl;
	BOOLEAN received;
	char c;
	int n, i=0, count, ptr;
	int recordSize, recordIdx;

	fprintf(fs, "[SMS]\n");

	if(scGsmsimCmdSelect(ri, ci, 0x3F00, buf, &n) || ci->sw[0]!=0x90 )
		fatal(ri, "Error selecting MF.");

	if(scGsmsimCmdSelect(ri, ci, 0x7F10, buf, &n) || ci->sw[0]!=0x90 )
		fatal(ri, "Error selecting DFtelecom.");

	if(scGsmsimCmdSelect(ri, ci, 0x6F3C, buf, &n) || ci->sw[0]!=0x90 )
		fatal(ri, "Error selecting EFsms.");
	recordSize=buf[14];

	for(recordIdx=1; ; recordIdx++) {
		n=recordSize;
		if(scGsmsimCmdReadRec(ri, ci, recordIdx, SC_GSMSIM_RECMODE_ABS, buf, &n))
			fatal(ri, "Error reading record.");

		if(ci->sw[0]==0x94 && ci->sw[1]==0x02)
			break;
		if(ci->sw[0]!=0x90 || ci->sw[1]!=0x00)
			fatal(ri, "Error reading record.");

		/* Empty? */
		/* Card of a friend has 0xFF for an empty record. */
		if( (!(buf[0]&0x07)) || (buf[0]==0xFF) ) continue;

		fprintf(fs, "%3d:", recordIdx);

		/* Parse record. */

		/* Recevied SMS? */
		if( buf[0]&0x04 ) received=FALSE; else received=TRUE;

		/* Status */
		status=buf[0];

		ptr=2+buf[1];

		/* mti */
		if( !( (received && (buf[ptr]&0x03)==0) ||
			(!received && (buf[ptr]&0x03)==1) ) ) {
			for( i=0; i<recordSize; i++ ) fprintf(fs, " %.2X",buf[i]);
			fprintf(fs, " (Unknown)\n");
			continue;
		}

		/* vpl */
		vpl=0;
		if( !received ) {
			if( (buf[ptr]&0x18)==0x10 ) vpl=1;
			else if( (buf[ptr]&0x18)==0x18 ) vpl=7;
		}

		/* mr */
		if( !received ) {
			ptr++;
			mr=buf[ptr];
		}
		ptr++;

		/* number */
		i=0;
		count=(buf[ptr++]+1)>>1;
		/* ...==0x85: Reserved, but used by www.lycos.com. */
		/* ...==0x50: Used by www.free-sms.com. */
		if(((buf[ptr]&0xEF)==0x81) || ((buf[ptr]&0xEF)==0x83) ||
			((buf[ptr]&0xEF)==0x85)) {
			if((buf[ptr++]&0x70)==0x10) 
				number[i++]='+';
			while(count-->0) {
				if((c=bcd2digit[buf[ptr]&0xF]))
					number[i++]=c;
				if((c=bcd2digit[buf[ptr]>>4]))
					number[i++]=c;
				ptr++;
			}
			number[i]=0;
		} else if((buf[ptr]&0x70)==0x50) {
			/* Packed 7bit. */
			ptr++;
			if(count>=7) count++;
			scGsmsimUnpackStr( buf+ptr, number, count );
			while( count ) {
				count--;
				switch( number[count] ) {
				case 'A':
					number[count]='*';
					break;
				case 'B':
					number[count]='#';
					break;
				case 'C':
					number[count]='.';
					break;
				default:
					break;
				}
			}
			ptr+=(buf[ptr-2]+1)>>1;
		} else {
			ptr++;
			ptr+=(buf[ptr-2]+1)>>1;
			number[0]='0';
			number[1]=0;
		}

		/* pid */
		pid=buf[ptr++];

		/* dcs */
		dcs=buf[ptr++];

		if( received ) {
			/* scts */
			memcpy( scts, buf+ptr, 7 );
			for(i=0; i<7; i++) scts[i]=(scts[i]>>4)|(scts[i]<<4);
			ptr+=7;
		} else {
			/* vp */
			memcpy( vp, buf+ptr, vpl );
			if( vpl==7 ) for(i=0; i<7; i++) vp[i]=(vp[i]>>4)|(vp[i]<<4);
			ptr+=vpl;
		}

		/* udl */
		udl=buf[ptr];
		ptr++;

		/* ud */
		if( dcs ) {
			/* Unknown character set. */
			memcpy( ud, buf+ptr, udl);
		} else {
			/* Packed 7bit. */
			scGsmsimUnpackStr( buf+ptr, ud, udl );
		}

		/* Output data. */

		/* ud */
		if( dcs ) {
			/* Unknown character set. */
			fprintf(fs, " ");
			for( i=0; i<udl; i++ ) fprintf(fs, "%.2X",ud[i]);
		} else {
			/* Packed 7bit. */
			for(i=0; i<strlen(ud); i++) if( isspace(ud[i]) ) ud[i]=' ';
			fprintf(fs, " %s", ud);
		}

		/* number */
		fprintf(fs, " (%s", number);

		/* status */
		if( (status&0x07)==3 )
			fprintf(fs, ",Received");
		else if( (status&0x07)==1 )
			fprintf(fs, ",Read");
		else if( (status&0x07)==5 )
			fprintf(fs, ",Sent");
		else
			fprintf(fs, ",Unsent");

		/* binary */
		if( dcs ) {
			/* Unknown character set. */
			fprintf(fs, ",Binary=%d,Dcs=%d", udl, dcs);
		}

		/* scts */
		if( received ) {
			fprintf(fs, ",TS=");
			for( i=0; i<7; i++ ) fprintf(fs, "%.2X",scts[i]);
		}

		/* vp */
		if( vpl==1 ) {
#if 0 /* Plain value to make parsing easier. */
			fprintf(fs, ",VP=");
			if( vp[0]<=143 ) {
				fprintf(fs, "%dm", vp[0]*5);
			} else if( vp[0]<=167 ) {
				if( (vp[0]-143)&1 )
					fprintf(fs, "%d.5h", 12+((vp[0]-143)>>1));
				else
					fprintf(fs, "%dh", 12+((vp[0]-143)>>1));
			} else if( vp[0]<=196 ) {
				fprintf(fs, "%dd", (vp[0]-166));
			} else {
				fprintf(fs, "%dd", (vp[0]-192)*7);
			}
#else
			fprintf(fs, ",VP=%d", vp[0]);
#endif
		} else if( vpl==7 ) {
			fprintf(fs, ",VP=");
			for( i=0; i<7; i++ ) fprintf(fs, "%.2X",vp[i]);
		}

		/* pid */
		fprintf(fs, ",PID=%.2X", pid );

		/* mr */
		if( !received ) fprintf(fs, ",MR=%d", mr );

		fprintf(fs, ")\n");
	}

	fprintf(fs, "\n");
}

static void restore_info(SIMDATA *sd, SC_READER_INFO *ri, SC_CARD_INFO *ci)
{ 
	BYTE buf[300], number[21];
	int n, i;

	if(scGsmsimCmdSelect(ri, ci, 0x3F00, buf, &n) || ci->sw[0]!=0x90 )
		fatal(ri, "Error selecting MF.");

	/* ICCID */

	if( sd->iccid!=NULL ) {
		if(scGsmsimCmdSelect(ri, ci, 0x2FE2, buf, &n) || ci->sw[0]!=0x90 )
			fatal(ri, "Error selecting EFiccid.");

		n=10;
		if(scGsmsimCmdReadBin(ri, ci, 0, buf, &n) || ci->sw[0]!=0x90 || n!=10 )
			fatal(ri, "Error reading EFiccid.");

		n=0;
		for( i=0; i<10; i++ ) {
			if( (buf[i]&0xF) < 10 ) number[n++]=(buf[i]&0xF)+0x30;
			if( (buf[i]>>4) < 10 ) number[n++]=(buf[i]>>4)+0x30;
		}
		number[n]=0;

		if( strcmp(sd->iccid, number)!=0 )
			fatal(ri, "ICCID in data file does not match ICCID of card.\n");
	}
}

static void restore_misc(SIMDATA *sd, SC_READER_INFO *ri, SC_CARD_INFO *ci)
{ 
	BYTE buf[300];
	int n, i;

	if(scGsmsimCmdSelect(ri, ci, 0x3F00, buf, &n) || ci->sw[0]!=0x90 )
		fatal(ri, "Error selecting MF.");

	/* LanguagePreferences */

	if( sd->lp!=NULL ) {
		if( sd->lsize>256 ) fatal(ri, "Invalid size of LanguagePreferences.");

		/* LanguagePreferences */

		if(scGsmsimCmdSelect(ri, ci, 0x7F20, buf, &n) || ci->sw[0]!=0x90 )
			fatal(ri, "Error selecting DFgsm.");

		if(scGsmsimCmdSelect(ri, ci, 0x6F05, buf, &n) || ci->sw[0]!=0x90 ||
			n<14 )
			fatal(ri, "Error selecting EFlp.");

		n=buf[3]; i=buf[3];
		if( n>sd->lsize ) fatal(ri, "LanguagePreferences to long.");

		memset(buf, 0xFF, sizeof(buf));
		memcpy(buf, sd->lp, sd->lsize);

		if(scGsmsimCmdUpdateBin(ri, ci, 0, buf, n) || ci->sw[0]!=0x90 )
			fatal(ri, "Error updating EFlp.");
	}
}

static void restore_phone(SIMDATA *sd, SC_READER_INFO *ri, SC_CARD_INFO *ci)
{
	BYTE buf[300];
	int n;
	int recordSize, recordIdx;

	if(scGsmsimCmdSelect(ri, ci, 0x3F00, buf, &n) || ci->sw[0]!=0x90 )
		fatal(ri, "Error selecting MF.");

	/* Phonebook */

	if( sd->prec!=NULL ) {
		if(scGsmsimCmdSelect(ri, ci, 0x7F10, buf, &n) || ci->sw[0]!=0x90 )
			fatal(ri, "Error selecting DFtelecom.");

		if(scGsmsimCmdSelect(ri, ci, 0x6F3A, buf, &n) || ci->sw[0]!=0x90 )
			fatal(ri, "Error selecting EFadn.");
		recordSize=buf[14];

		if( ((buf[2]*256+buf[3])/buf[14])<sd->psize )
			fatal(ri, "To many entries for EFadn.");

		/* restore records */

		for(recordIdx=1; recordIdx<=sd->psize; recordIdx++) {
			memset(buf, 0xFF, recordSize);

			/* Empty entry? */
			if(sd->prec[recordIdx-1].name[0]==0xFF) {
				if(scGsmsimCmdUpdateRec(ri, ci, recordIdx,
					SC_GSMSIM_RECMODE_ABS, buf, recordSize) || ci->sw[0]!=0x90 )
					fatal(ri, "Error updating record of EFadn.");

				continue;
			}

			for(n=0; (n<recordSize-14) && (sd->prec[recordIdx-1].name[n]!=0); n++)
				buf[n]=scGsmsimIsoToGsm(sd->prec[recordIdx-1].name[n]);

			buf[recordSize-14]=sd->prec[recordIdx-1].numlen;
			if( (buf[recordSize-14]!=0xFF) && (buf[recordSize-14]>11) )
				fatal(ri, "Wrong phone number size in entry %d.", recordIdx);

			memcpy(buf+recordSize-13, sd->prec[recordIdx-1].number,
				sizeof(sd->prec[recordIdx-1].number));

			if(scGsmsimCmdUpdateRec(ri, ci, recordIdx, SC_GSMSIM_RECMODE_ABS,
				buf, recordSize) || ci->sw[0]!=0x90 )
				fatal(ri, "Error updating record of EFadn.");
		}
	}
}

static void restore_sms(SIMDATA *sd, SC_READER_INFO *ri, SC_CARD_INFO *ci)
{ 
	BYTE buf[300], smss[2], pind, sca[12], pid, vp;
	int n, ptr, recordIdx, recordSize;

	if( sd->srec==NULL ) return;

	if(scGsmsimCmdSelect(ri, ci, 0x3F00, buf, &n) || ci->sw[0]!=0x90 )
		fatal(ri, "Error selecting MF.");

	if(scGsmsimCmdSelect(ri, ci, 0x7F10, buf, &n) || ci->sw[0]!=0x90 )
		fatal(ri, "Error selecting MF.");

	if(scGsmsimCmdSelect(ri, ci, 0x6F42, buf, &n) || ci->sw[0]!=0x90 )
		fatal(ri, "Error selecting EFsmsp.");
	recordSize=buf[14];

	n=recordSize;
	if(scGsmsimCmdReadRec(ri, ci, 1, SC_GSMSIM_RECMODE_ABS, buf, &n) ||
		ci->sw[0]!=0x90 )
		fatal(ri, "Error reading record from EFsmsp.");

	pind=buf[n-28];
	memcpy( sca, buf+n-15, sizeof(sca) );
	pid=buf[n-3];
	vp=buf[n-1];

	if( pind&0x02 )
		fatal(ri, "EFsmsp contains not Service Center Address.");

	if(scGsmsimCmdSelect(ri, ci, 0x7F10, buf, &n) || ci->sw[0]!=0x90 )
		fatal(ri, "Error selecting DFtelecom.");

	if(scGsmsimCmdSelect(ri, ci, 0x6F43, buf, &n) || ci->sw[0]!=0x90 )
		fatal(ri, "Error selecting EFsmss.");

	n=2;
	if(scGsmsimCmdReadBin(ri, ci, 0, buf, &n) || ci->sw[0]!=0x90 || n!=2 )
		fatal(ri, "Error reading EFsmss.");
	smss[0]=buf[0];
	smss[1]=0xFE;

	/* fill up missing data */

	for(recordIdx=1; recordIdx<=sd->ssize; recordIdx++) {
		if(!sd->srec[recordIdx-1].status) {
			smss[1]=0xFF;
		} else if(sd->srec[recordIdx-1].dump) {
		} else if(sd->srec[recordIdx-1].status&0x04) {
			/* Sent/To be sent */
			if(!sd->srec[recordIdx-1].mrset) {
				sd->srec[recordIdx-1].mr=0;
				sd->srec[recordIdx-1].mrset=TRUE;
			}
			if(!sd->srec[recordIdx-1].numlen)
				fatal(ri, "Phonenumber missing.");
			if(!sd->srec[recordIdx-1].pidset) {
				if(pind&0x04) sd->srec[recordIdx-1].pid=0x00;
				else sd->srec[recordIdx-1].pid=pid;
				sd->srec[recordIdx-1].pidset=TRUE;
			}
			if(!sd->srec[recordIdx-1].udl && !sd->srec[recordIdx-1].udlb)
				fatal(ri, "Message missing.");
		} else {
			/* Received/Read */
			if(!sd->srec[recordIdx-1].numlen)
				fatal(ri, "Phonenumber missing.");
			if(!sd->srec[recordIdx-1].pidset) {
				if(pind&0x04) sd->srec[recordIdx-1].pid=0x00;
				else sd->srec[recordIdx-1].pid=pid;
				sd->srec[recordIdx-1].pidset=TRUE;
			}
			if(!sd->srec[recordIdx-1].sctsset) {
				memset( sd->srec[recordIdx-1].scts, 0,
					sizeof(sd->srec[recordIdx-1].scts));
				sd->srec[recordIdx-1].sctsset=TRUE;
			}
			if(!sd->srec[recordIdx-1].udl && !sd->srec[recordIdx-1].udlb)
				fatal(ri, "Message missing.");
		}
	}

	/* writing EFsmss */
	if(scGsmsimCmdUpdateBin(ri, ci, 0, smss, 2) || ci->sw[0]!=0x90 )
		fatal(ri, "Error updating EFsmss.");

	if(scGsmsimCmdSelect(ri, ci, 0x7F10, buf, &n) || ci->sw[0]!=0x90 )
		fatal(ri, "Error selecting MF.");

	if(scGsmsimCmdSelect(ri, ci, 0x6F3C, buf, &n) || ci->sw[0]!=0x90 )
		fatal(ri, "Error selecting EFsms.");
	recordSize=buf[14];

	if( ((buf[2]*256+buf[3])/buf[14])<sd->ssize )
		fatal(ri, "To many entries for EFsms.");

	/* restore records */

	for(recordIdx=1; recordIdx<=sd->ssize; recordIdx++) {
		memset(buf, 0xFF, recordSize);

		/* Empty entry? */
		if(sd->srec[recordIdx-1].status==0x00) {
			/* Clear record */
			buf[0]=0;

			if(scGsmsimCmdUpdateRec(ri, ci, recordIdx, SC_GSMSIM_RECMODE_ABS,
				buf, recordSize) || ci->sw[0]!=0x90 )
				fatal(ri, "Error updating record of EFsms.");

			continue;
		}

		if( sd->srec[recordIdx-1].dump ) {
			memcpy(buf, sd->srec[recordIdx-1].ud, sd->srec[recordIdx-1].udlb);
		} else {
			/* status */
			buf[0]=sd->srec[recordIdx-1].status;
			/* sca */
			memcpy(buf+1, sca, sca[0]+1);
			ptr=2+sca[0];
			/* mti, mr */
			if(sd->srec[recordIdx-1].status&0x04) {
				if(sd->srec[recordIdx-1].vpl==1) buf[ptr++]=0x11;
				else if(sd->srec[recordIdx-1].vpl==7) buf[ptr++]=0x19;
				else buf[ptr++]=0x01;
				buf[ptr++]=sd->srec[recordIdx-1].mr;
			} else buf[ptr++]=0x04;
			/* address */
			memcpy(buf+ptr, sd->srec[recordIdx-1].number,
				sd->srec[recordIdx-1].numlen);
			ptr+=sd->srec[recordIdx-1].numlen;
			/* pid */
			buf[ptr++]=sd->srec[recordIdx-1].pid;
			/* dcs */
			buf[ptr++]=sd->srec[recordIdx-1].dcs;
			/* scts, vp */
			if(sd->srec[recordIdx-1].status&0x04) {
				if(sd->srec[recordIdx-1].vpl) {
					memcpy(buf+ptr, sd->srec[recordIdx-1].vp,
						sd->srec[recordIdx-1].vpl);
					ptr+=sd->srec[recordIdx-1].vpl;
				}
			} else {
				memcpy(buf+ptr, sd->srec[recordIdx-1].scts, 7);
				ptr+=7;
			}
			/* udl */
			buf[ptr++]=sd->srec[recordIdx-1].udl;
			/* ud */
			memcpy(buf+ptr, sd->srec[recordIdx-1].ud,
				sd->srec[recordIdx-1].udlb);
		}

		if(scGsmsimCmdUpdateRec(ri, ci, recordIdx, SC_GSMSIM_RECMODE_ABS,
			buf, recordSize) || ci->sw[0]!=0x90 )
			fatal(ri, "Error updating record of EFsms.");
	}
}

int main(int argc, char *argv[])
{
	BYTE header[] = { 0xA0, 0xC0, 0x00, 0x00, 0x00 };
	BYTE swok[] = { 0x02, 0x90, 0x91 };
	BYTE swav[] = { 0x01, 0x9F };

	SC_READER_INFO *ri=NULL;
	SC_CARD_INFO *ci=NULL;
	SC_READER_CONFIG rc;
	SIMDATA sd;
	char pin[8], *cp;
	BYTE buf[11];
	int n;

	prgName=argv[0];

	if(scInit())
		fatal(ri, "couldn't initialize library");

	rc.type=READER_TYPE;
	rc.slot=READER_SLOT;
	rc.param=READER_PORT;

	if(scReaderGetConfig(argc, argv, &rc))
		fatal(ri, "Error getting reader configuration.");

	if(!(ri=scGeneralNewReader(rc.type, rc.slot)))
		fatal(ri, "Error creating reader object.");

	if(!(ci=scGeneralNewCard()))
		fatal(ri, "Error creating card object.");

	if(scReaderInit(ri, rc.param))
		fatal(ri, "Couldn't initialize reader.");

	if(scReaderActivate(ri))
		fatal(ri, "Couldn't activate reader.");

	if(scReaderCardStatus(ri))
		fatal(ri, "Couldn't retrieve card status.");

	if(!(ri->status&SC_CARD_STATUS_PRESENT))
		fatal(ri, "Please insert a card into the reader.");

	if(scReaderResetCard(ri, ci))
		fatal(ri, "Error while resetting card.");

	if(scSmartcardGetCardType(ci))
		fatal(ri, "Couldn't query card type.");

	/* if(ci->type!=SC_CARD_GSMSIM) */
	if( (ci->type!=SC_CARD_GSMSIM) && (ci->type!=SC_CARD_UNKNOWN) )
		fatal(ri, "No GSM card in reader.");

	if( ci->type==SC_CARD_UNKNOWN ) {
		/* Process ATR */
		if( scSmartcardProcessATR( ci ) )
			fatal(ri, "Error in ATR.");

		/* Set other parameters */
		memcpy( ci->t0.getrsp, header, 5 );
		memcpy( ci->swok, swok, sizeof(swok) );
		memcpy( ci->swav, swav, sizeof(swav) );
		ci->memsize=0;
	}

	/* Get ICCID to test if it is an GSM card. */
	if(scGsmsimCmdSelect(ri, ci, 0x3F00, buf, &n) || ci->sw[0]!=0x90 )
		fatal(ri, "Unsupported GSM card in reader.");
	if(scGsmsimCmdSelect(ri, ci, 0x2FE2, buf, &n) || ci->sw[0]!=0x90 )
		fatal(ri, "Unsupported GSM card in reader.");
	n=10;
	if(scGsmsimCmdReadBin(ri, ci, 0, buf, &n) || ci->sw[0]!=0x90 || n!=10 )
		fatal(ri, "Unsupported GSM card in reader.");

	cp=getpass("Please enter the PIN: ");
	n=strlen(cp);
	if( (n<4) || (n>8) )
		fatal(ri, "Invalid PIN.");
	if(n>sizeof(pin))
		n=sizeof(pin);
	memcpy(pin, cp, n);
	if(n<sizeof(pin))
		memset(pin+n, 0xFF, sizeof(pin)-n);

	if(scGsmsimCmdVerifyCHV(ri, ci, 1, pin) ||
		ci->sw[0]!=0x90 || ci->sw[1]!=0x00)
		fatal(ri, "PIN was rejected from card.");

	if((argc==2) && (strcmp("-r",argv[1])==0)) {
		parse_data(stdin, &sd, ri);
		restore_info(&sd, ri, ci);	/* Does only compare ICCID. */
		restore_misc(&sd, ri, ci);
		restore_phone(&sd, ri, ci);
		restore_sms(&sd, ri, ci);
	} else {
		fprintf(stdout, "%% SIMMan v0.3\n\n" );
		dump_info(stdout, ri, ci);
		dump_misc(stdout, ri, ci);
		dump_phone(stdout, ri, ci);
		dump_sms(stdout, ri, ci);
	}

	scReaderDeactivate( ri );
	scReaderShutdown( ri );

	scGeneralFreeCard( &ci );
	scGeneralFreeReader( &ri );

	scEnd();

	return 0;
}

