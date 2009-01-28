/****************************************************************************
*																			*
*			SCEZ chipcard library - parsing routines for SIM dump			*
*						Copyright Matthias Bruestle 2000					*
*					For licensing, see the file COPYRIGHT					*
*																			*
****************************************************************************/

/* $Id: simparse.c 1617 2005-11-03 17:41:39Z laforge $ */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <scez/scgeneral.h>
#include <scez/cards/scgsmsim.h>
#include "simman.h"

extern char *prgName;
extern char bcd2digit[];

/* Clean up for all lines. */

static int precleanup( char *str )
{
	int i, j;

	if( str==NULL ) return( SC_EXIT_BAD_PARAM );

	/* Remove last characters if they are \n or \r. */
	while( (str[strlen(str)-1]=='\n') || (str[strlen(str)-1]=='\r') )
		str[strlen(str)-1]=0;

	/* Convert tabs to spaces. */
	for( i=0; i<strlen(str); i++ )
		if( str[i]=='\t' || str[i]=='\v' ) str[i]=' ';

	/* Remove comments starting with '%'. */
	for( i=0; i<strlen(str); i++ )
		if( str[i]=='%' ) str[i]=0;

	/* Remove all leading spaces. */
	i=0; j=0;
	while( (j<strlen(str)) && (str[j]==' ') ) j++;
	while( str[j]!=0 ) str[i++]=str[j++];
	str[i]=0;

	return( SC_EXIT_OK );
}

/* Clean up for title lines and lines with '='. */

static int cleanup1( char *str )
{
	int i, j;

	/* Remove all spaces. */
	j=0;
	for( i=0; i<=strlen(str); i++ )
		if( str[i]!=' ' ) str[j++]=str[i];

	/* To upper case. */
	for( i=0; i<strlen(str); i++ )
		str[i]=toupper(str[i]);

	return( SC_EXIT_OK );
}

/* Clean up for lines with ':'. */

static int cleanup2( char *str )
{
	int i, j;
	BOOLEAN found, done;

	/* Remove spaces at the line end. */
	while( str[strlen(str)-1]==' ' ) str[strlen(str)-1]=0;

	/* Remove spaces before first ':'. */
	j=0; found=FALSE;
	for( i=0; i<=strlen(str); i++ ) {
		if( str[i]==':' ) found=TRUE;
		if( (str[i]!=' ') || found ) str[j++]=str[i];
	}

	/* Remove spaces directly after first ':'. */
	j=0; found=FALSE; done=FALSE;
	for( i=0; i<=strlen(str); i++ ) {
		if( found && (str[i]!=' ') ) { done=TRUE; found=FALSE; }
		if( !found ) str[j++]=str[i];
		if( !done && (str[i]==':') ) found=TRUE;
	}

	/* Remove spaces directly before '('. */
	i=strlen(str);
	while( (i>0) && (str[i]!='(') ) i--;
	if( str[i]=='(' ) {
		i--;
		while( (i>0) && (str[i]==' ') ) i--;
		i++;
		j=i;
		while( (i<=strlen(str)) && (str[i]==' ') ) i++;
		while( i<=strlen(str) ) str[j++]=str[i++];
	}

	/* Remove all spaces after '(' and convert to upper case. */
	i=strlen(str);
	while( (i>0) && (str[i]!='(') ) i--;
	if( str[i]=='(' ) {
		j=i;
		while( i<=strlen(str) ) {
			if( !(str[i]==' ') ) str[j++]=toupper(str[i]);
			i++;
		}
	}

	/* To upper case before first ':'. */
	i=0;
	while( (i<strlen(str)) && (str[i]!=':') ) {
		str[i]=toupper(str[i]);
		i++;
	}

	return( SC_EXIT_OK );
}

/* Parse data. */

void parse_data(FILE *fs, SIMDATA *sd, SC_READER_INFO *ri)
{
	char buffer[MAX_LINE_LEN+3], *ptr, *ptr2;
	int count=0, i, j, nibble, ibuf[8];

	sd->part=PART_NONE;
	sd->iccid=NULL;
	sd->lsize=0;
	sd->lp=NULL;
	sd->psize=0;
	sd->prec=NULL;
	sd->ssize=0;
	sd->srec=NULL;

	while( fgets(buffer, MAX_LINE_LEN+2, fs)!=NULL ) {
		count++;

		if( strlen(buffer)>MAX_LINE_LEN+1 )
			fatal(ri, "Error in line %d: Line to long.\n", count);

		/* First cleanup. */
		if( precleanup(buffer)!=SC_EXIT_OK )
			fatal(ri, "Error in line %d: Unknown error.\n", count);

		/* Line empty. */
		if( strlen(buffer)==0 ) continue;

		/* Second cleanup. */
		if( (buffer[0]=='[') || (sd->part==PART_INFO) ||
			(sd->part==PART_MISC) ) {
			if( cleanup1(buffer)!=SC_EXIT_OK )
				fatal(ri, "Error in line %d: Unknown error.\n", count);
		} else {
			if( cleanup2(buffer)!=SC_EXIT_OK )
				fatal(ri, "Error in line %d: Unknown error.\n", count);
		}

		/* Header line. */
		if( buffer[0]=='[' ) {
			if( strcmp("[INFO]", buffer)==0 ) {
				sd->part=PART_INFO;
				continue;
			}
			if( strcmp("[MISC]", buffer)==0 ) {
				sd->part=PART_MISC;
				continue;
			}
			if( strcmp("[PHONEBOOK]", buffer)==0 ) {
				sd->part=PART_PHONE;
				continue;
			}
			if( strcmp("[SMS]", buffer)==0 ) {
				sd->part=PART_SMS;
				continue;
			}
		}

		switch( sd->part ) {
		case PART_INFO:
			if( memcmp("ICCID=", buffer, strlen("ICCID="))==0 ) {
				if(sd->iccid!=NULL) free(sd->iccid);

				sd->iccid=(char *)malloc(strlen(buffer+strlen("ICCID="))+1);
				if(sd->iccid==NULL) fatal(ri, "Error allocating memory\n");

				strcpy(sd->iccid, buffer+strlen("ICCID=") );
			}
			if( memcmp("MAXLANGUAGEENTRIES=", buffer,
				strlen("MAXLANGUAGEENTRIES="))==0 ) {
				if(sd->lp!=NULL) free(sd->lp);

				if( sscanf(buffer+strlen("MAXLANGUAGEENTRIES="), "%d", &i)!=1 )
					fatal(ri, "Error in line %d: Wrong format\n", count);

				sd->lsize=i;

				sd->lp=(BYTE *)malloc(i*sizeof(BYTE));
				if(sd->lp==NULL) fatal(ri, "Error allocating memory\n");

				memset(sd->lp, 0xFF, sd->lsize);
			}
			if( memcmp("MAXPHONEBOOKENTRIES=", buffer,
				strlen("MAXPHONEBOOKENTRIES="))==0 ) {
				if(sd->prec!=NULL) free(sd->prec);

				if( sscanf(buffer+strlen("MAXPHONEBOOKENTRIES="), "%d", &i)!=1 )
					fatal(ri, "Error in line %d: Wrong format\n", count);

				sd->psize=i;

				sd->prec=(PHONENUMBER *)malloc(i*sizeof(PHONENUMBER));
				if(sd->prec==NULL) fatal(ri, "Error allocating memory\n");

				memset(sd->prec, 0xFF, sizeof(PHONENUMBER)*i);
			}
			if( memcmp("MAXSMSENTRIES=", buffer,
				strlen("MAXSMSENTRIES="))==0 ) {
				if(sd->srec!=NULL) free(sd->srec);

				if( sscanf(buffer+strlen("MAXSMSENTRIES="), "%d", &i)!=1 )
					fatal(ri, "Error in line %d: Wrong format\n", count);

				sd->ssize=i;

				sd->srec=(SMS *)malloc(i*sizeof(SMS));
				if(sd->srec==NULL) fatal(ri, "Error allocating memory\n");

				memset(sd->srec, 0x00, sizeof(SMS)*i);
			}
			break;
		case PART_MISC:
			if( memcmp("LANGUAGEPREFERENCES=", buffer,
				strlen("LANGUAGEPREFERENCES="))==0 ) {
				if( sd->lp==NULL )
					fatal(ri, "Error in line %d: MaxLanguageEntries must come before.\n", count);

				ptr=strchr(buffer, '=');
				if(ptr==NULL)
					fatal(ri, "Error in line %d: Wrong format\n", count);
				ptr++;

				i=0;
				while( (i<sd->lsize) && (*ptr!=0) &&
					(sscanf(ptr, "%d", &j)==1) ) {

					/* Copy value to sd->lp. */
					sd->lp[i++]=j & 0xFF;

					/* Go over all decimal digits. */
					while( (*ptr!=0) && (*ptr<='9') && (*ptr>='0') ) ptr++;

					/* Check if EOS. */
					if(*ptr==0) break;

					/* Check if ','. */
					if(*ptr!=',')
						fatal(ri, "Error in line %d: Wrong format\n", count);

					/* Char after ','. */
					ptr++;
				}
			}
			break;
		case PART_PHONE:
			if( sd->prec==NULL )
				fatal(ri, "Error in line %d: MaxPhonebookEntries must come before.\n", count);

			if( sscanf( buffer, "%d:", &i )!=1 )
				fatal(ri, "Error in line %d: Wrong entry number.\n", count);

			if( i>sd->psize )
				fatal(ri, "Error in line %d: Entry number to high.\n", count);

			if( i<1 )
				fatal(ri, "Error in line %d: Wrong entry number.\n", count);

			i--;
			memset(sd->prec[i].name, 0xFF, sizeof(sd->prec[i].name));
			memset(sd->prec[i].number, 0xFF, sizeof(sd->prec[i].number));

			ptr=strchr(buffer, ':');
			ptr++;
			if( *ptr==0 )
				fatal(ri, "Error in line %d: Missing name.\n", count);

			ptr2=strrchr(buffer, '(');
			if( ptr2==NULL )
				fatal(ri, "Error in line %d: Missing parameters.\n", count);
			*ptr2=0;
			strcpy(sd->prec[i].name, ptr);
			*ptr2='(';
			ptr=ptr2;

			ptr++;
			if( *ptr==0 )
				fatal(ri, "Error in line %d: Missing phone number.\n", count);

			if( *ptr=='+' ) {
				sd->prec[i].number[0]=0x91;
				ptr++;
			} else
				sd->prec[i].number[0]=0x81;

			if( *ptr==0 )
				fatal(ri, "Error in line %d: Missing phone number.\n", count);

			j=0;
			while( (ptr[j]!=0) && (ptr[j]!=')') ) {
				if( (ptr[j]>='0') && (ptr[j]<='9') )
					nibble=ptr[j]-'0';
				else
					switch(ptr[j]) {
					case '*':
						nibble=0xA;
						break;
					case '#':
						nibble=0xB;
						break;
					case '.':
						nibble=0xC;
						break;
					default:
						nibble=-1;
					}

				if(nibble==-1)
					fatal(ri, "Error in line %d: Error in phone number.\n", count);
				if( j&1 ) {
					sd->prec[i].number[1+(j>>1)]&=0x0F;
					sd->prec[i].number[1+(j>>1)]|=nibble<<4;
				} else {
					sd->prec[i].number[1+(j>>1)]=0xF0|nibble;
				}

				j++;
			}
			if( j&1 )
				sd->prec[i].numlen=2+(j>>1);
			else
				sd->prec[i].numlen=1+(j>>1);

			break;
		case PART_SMS:
			if( sd->srec==NULL )
				fatal(ri, "Error in line %d: MaxSmsEntries must come before.\n", count);

			if( sscanf( buffer, "%d:", &i )!=1 )
				fatal(ri, "Error in line %d: Wrong entry number.\n", count);

			if( i>sd->ssize )
				fatal(ri, "Error in line %d: Entry number to high.\n", count);

			if( i<1 )
				fatal(ri, "Error in line %d: Wrong entry number.\n", count);

			i--;

			if( (ptr=strrchr(buffer, '('))==NULL )
				fatal(ri, "Error in line %d: Missing descriptors.\n", count);
			ptr++;

			/* Dump? */
			if( memcmp(ptr, "UNKNOWN", sizeof("UNKNOWN"))==0 ) {
				sd->srec[i].dump=TRUE;

				if( (ptr=strchr(buffer, ':'))==NULL )
					fatal(ri, "Error in line %d: Message missing.\n", count);
				ptr++;

				/* Check hex string */
				j=0;
				while( ptr[j]!='(' ) {
					if( !isxdigit(ptr[j]) && !isspace(ptr[j]) )
						fatal(ri, "Error in line %d: Non hex character.\n", count);
					j++;
				}

				/* Convert to binary */
				sd->srec[i].udlb=0;
				while( sscanf( ptr, "%2X", &j )==1 ) {
					sd->srec[i].ud[sd->srec[i].udlb++]=j&0xFF;

					ptr++;
					if( (*ptr=='(') || (*ptr==0) ) break;
					while( isspace(*ptr) ) ptr++;
					ptr++;
					if( (*ptr=='(') || (*ptr==0) ) break;
					while( isspace(*ptr) ) ptr++;
				}

				sd->srec[i].status=sd->srec[i].ud[0];
				break;
			} else if( strstr( ptr, "BINARY=")!=NULL ) {
				sd->srec[i].dump=FALSE;

				ptr=strchr(buffer, ':');
				if(ptr==NULL)
					fatal(ri, "Error in line %d: Message missing.\n", count);
				ptr++;

				if( sscanf( strstr( ptr, "BINARY="), "BINARY=%d", &j )!=1 )
					fatal(ri, "Error in line %d: Error in parameter.\n", count);
				sd->srec[i].udl=j;

				/* Check hex string */
				j=0;
				while( ptr[j]!='(' ) {
					if( !isxdigit(ptr[j]) && !isspace(ptr[j]) )
						fatal(ri, "Error in line %d: Non hex character.\n", count);
					j++;
				}

				/* Convert to binary */
				sd->srec[i].udlb=0;
				while( sscanf( ptr, "%2X", &j )==1 ) {
					sd->srec[i].ud[sd->srec[i].udlb++]=j&0xFF;

					ptr++;
					if( (*ptr=='(') || (*ptr==0) ) break;
					while( isspace(*ptr) ) ptr++;
					ptr++;
					if( (*ptr=='(') || (*ptr==0) ) break;
					while( isspace(*ptr) ) ptr++;
				}

				sd->srec[i].status=0x07;
			} else {
				sd->srec[i].dump=FALSE;

				ptr=strchr(buffer, ':');
				if(ptr==NULL)
					fatal(ri, "Error in line %d: Message missing.\n", count);
				ptr++;
				if( *ptr==0 )
					fatal(ri, "Error in line %d: Message missing.\n", count);

				ptr2=strrchr(buffer, '(');
				if( ptr2==NULL )
					fatal(ri, "Error in line %d: Missing parameters.\n", count);
				*ptr2=0;
				strcpy(sd->srec[i].ud, ptr);
				sd->srec[i].udl=strlen(ptr);
				*ptr2='(';

				/* Convert to binary */
				if( scGsmsimPackStr(ptr, sd->srec[i].udl, sd->srec[i].ud,
					&sd->srec[i].udlb ) )
					fatal(ri, "Error in line %d: Error packing message.\n", count);

				sd->srec[i].status=0x07;
			}

			if( (ptr=strrchr(buffer, '('))==NULL )
				fatal(ri, "Error in line %d: Missing descriptors.\n", count);
			ptr++;

			if( *ptr=='+' ) {
				sd->srec[i].number[1]=0x91;
				ptr++;
			} else
				sd->srec[i].number[1]=0x81;

			if( *ptr==0 )
				fatal(ri, "Error in line %d: Missing phone number.\n", count);

			j=0;
			while( (ptr[j]!=0) && (ptr[j]!=')') && (ptr[j]!=',') && (j<20) ) {
				if( (ptr[j]>='0') && (ptr[j]<='9') )
					nibble=ptr[j]-'0';
				else
					switch(ptr[j]) {
					case '*':
						nibble=0xA;
						break;
					case '#':
						nibble=0xB;
						break;
					case '.':
					default:
						nibble=0xC;
						break;
					/* Some Web SMS services send invalid phone numbers. */
					/*
					default:
						nibble=-1;
					*/
					}

				if(nibble==-1)
					fatal(ri, "Error in line %d: Error in phone number.\n", count);
				if( j&1 ) {
					sd->srec[i].number[2+(j>>1)]&=0x0F;
					sd->srec[i].number[2+(j>>1)]|=nibble<<4;
				} else {
					sd->srec[i].number[2+(j>>1)]=0xF0|nibble;
				}

				j++;

				sd->srec[i].number[0]=j;
			}
			if( j>=20 )
				fatal(ri, "Error in line %d: Error in phone number.\n", count);
			if( j&1 )
				sd->srec[i].numlen=3+(j>>1);
			else
				sd->srec[i].numlen=2+(j>>1);
			if( sd->srec[i].number[2]==0 )
				sd->srec[i].number[1]=0x83;
			ptr+=j;

			/* Initialize parameters in struct. */
			sd->srec[i].mrset=FALSE;
			sd->srec[i].pidset=FALSE;
			sd->srec[i].dcs=0;
			sd->srec[i].sctsset=FALSE;
			sd->srec[i].vpl=0;

			/* Process parameters. */
			while( (ptr!=NULL) && (*ptr!=')') ) {
				ptr++;
				if( !isalpha(*ptr) )
					fatal(ri, "Error in line %d: Error in parameters\n", count);

				if( memcmp("RECEIVED", ptr, strlen("RECEIVED"))==0 ) {
					sd->srec[i].status=0x03;
				} else if( memcmp("READ", ptr, strlen("READ"))==0 ) {
					sd->srec[i].status=0x01;
				} else if( memcmp("SENT", ptr, strlen("SENT"))==0 ) {
					sd->srec[i].status=0x05;
				} else if( memcmp("UNSENT", ptr, strlen("UNSENT"))==0 ) {
				} else if( memcmp("BINARY=", ptr, strlen("BINARY="))==0 ) {
				} else if( memcmp("TS=", ptr, strlen("TS="))==0 ) {
					if(sscanf( ptr, "TS=%2X%2X%2X%2X%2X%2X%2X%2X", &ibuf[0],
						&ibuf[1], &ibuf[2], &ibuf[3], &ibuf[4], &ibuf[5],
						&ibuf[6], &ibuf[7])!=7 )
						fatal(ri, "Error in line %d: Error in parameters\n", count);
					for(j=0; j<7; j++)
						sd->srec[i].scts[j]=((ibuf[j]>>4)&0xF)|((ibuf[j]<<4)&0xF0);
					sd->srec[i].sctsset=TRUE;
				} else if( memcmp("MR=", ptr, strlen("MR="))==0 ) {
					if(sscanf( ptr, "MR=%d", &j)!=1)
						fatal(ri, "Error in line %d: Error in parameters\n", count);
					sd->srec[i].mr=j&0xFF;
					sd->srec[i].mrset=TRUE;
				} else if( memcmp("PID=", ptr, strlen("PID="))==0 ) {
					if(sscanf( ptr, "PID=%2X", &j)!=1)
						fatal(ri, "Error in line %d: Error in parameters\n", count);
					sd->srec[i].pid=j&0xFF;
					sd->srec[i].pidset=TRUE;
				} else if( memcmp("DCS=", ptr, strlen("DCS="))==0 ) {
					if(sscanf( ptr, "DCS=%d", &j)!=1)
						fatal(ri, "Error in line %d: Error in parameters\n", count);
					sd->srec[i].dcs=j&0xFF;
				} else if( memcmp("VP=", ptr, strlen("VP="))==0 ) {
					if(sscanf( ptr, "VP=%2X%2X%2X%2X%2X%2X%2X%2X", &ibuf[0],
						&ibuf[1], &ibuf[2], &ibuf[3], &ibuf[4], &ibuf[5],
						&ibuf[6], &ibuf[7])==7 ) {
						for(j=0; j<7; j++)
							sd->srec[i].vp[j]=((ibuf[j]>>4)&0xF)|((ibuf[j]<<4)&0xF0);
						sd->srec[i].vpl=7;
					} else if(sscanf( ptr, "VP=%d", &j)==1) {
						sd->srec[i].vp[0]=j&0xFF;
						sd->srec[i].vpl=1;
					} else
						fatal(ri, "Error in line %d: Error in parameters\n", count);
				} else {
					fatal(ri, "Error in line %d: Unrecognised parameters.\n", count);
				}

				ptr=strchr(ptr, ',');
			}

			break;
		case PART_NONE:
		case PART_DONE:
		default:
			fatal(ri, "Error in line %d: Data here not expected\n", count);
		}
	}

	sd->part=PART_DONE;
}

