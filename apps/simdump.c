/****************************************************************************
*																			*
*				Copyright Gerd Rausch <gerd@alf.gun.de> 1999				*
*					For licensing, see the file COPYRIGHT					*
*			This file is only available under the BSD2 license.				*
*																			*
****************************************************************************/

/* $Id: simdump.c 1617 2005-11-03 17:41:39Z laforge $ */

/* If the programs name is "simrestore" it reads the phonebook dump from
 * stdin and writes it to the SIM card. In all other cases it dumps the
 * phonebook to stdout.
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#if !defined(WINDOWS) && !defined(__palmos__)
#include <unistd.h> /* for sleep */
#elif defined(__BORLANDC__)
#include <dos.h>       /* for sleep */
#elif defined(WINDOWS)
#include <windows.h>
#endif
#include <stdarg.h>
#include <errno.h>
#include <sio/sio.h>
#include <scez/scgeneral.h>
#include <scez/cards/scgsmsim.h>

#ifndef READER_TYPE
#define READER_TYPE SC_READER_DUMBMOUSE
#endif /* READER_TYPE */
#ifndef READER_SLOT
#define READER_SLOT 1
#endif /* READER_SLOT */
#ifndef READER_PORT
#define READER_PORT "0"
#endif /* READER_PORT */

char *prgName;

static void fatal(char *format, ...)
{
  va_list ap;

  va_start(ap, format);
  fprintf(stderr,"%s: ",prgName);
  vfprintf(stderr,format,ap);
  fprintf(stderr,"\n");
  va_end(ap);
  exit(1);
}

static void dump(SC_READER_INFO *ri, SC_CARD_INFO *ci)
{ 
  static char bcd2digit[]={
    '0', '1', '2', '3' , '4', '5', '6', '7', '8', '9',
    '*', '#', '.', 0, 0, 0
  };
  BYTE buf[1024], number[1024], *name, *bp;
  char c;
  int len, n, i, count;
  int recordSize, recordIdx, printLabel;

  if(scGsmsimCmdSelect(ri, ci, 0x7F10, buf, &len))
    fatal("error selecting DFtelecom");

  if(scGsmsimCmdSelect(ri, ci, 0x6F3A, buf, &len))
    fatal("error selecting EFadn");
  recordSize=buf[len-1];

  printLabel=1;
  for(recordIdx=1; ; recordIdx++) {
    n=recordSize;
    if(scGsmsimCmdReadRec(ri, ci, recordIdx, SC_GSMSIM_RECMODE_ABS, buf, &n))
      fatal("error reading record");
    if(ci->sw[0]==0x94 && ci->sw[1]==0x02)
      break;
    if(ci->sw[0]!=0x90 || ci->sw[1]!=0x00)
      fatal("error reading record");
    count=buf[n-14];
    for(i=0; i<n-14 && buf[i]!=0xFF; i++);
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

      if(printLabel) {
	printf(":%d\n", recordIdx);
	printLabel=0;
      }
      printf("%-20s %s\n", number, name);
    } else
      printLabel=1;
  }
}

static void restore(SC_READER_INFO *ri, SC_CARD_INFO *ci)
{
  char line[1024], *cp, *number, *name;
  BYTE buf[1024], *bp;
  int len, n, i, nibble, v, shift;
  int recordSize, recordIdx;

  if(scGsmsimCmdSelect(ri, ci, 0x7F10, buf, &len))
    fatal("error selecting DFtelecom");

  if(scGsmsimCmdSelect(ri, ci, 0x6F3A, buf, &len))
    fatal("error selecting EFadn");
  recordSize=buf[len-1];

  /* clear all records first */
  memset(buf, 0xFF, recordSize);
  for(recordIdx=1; ; recordIdx++) {
    if(scGsmsimCmdUpdateRec(ri, ci, recordIdx, SC_GSMSIM_RECMODE_ABS, buf, recordSize))
      fatal("error updating record");
    if(ci->sw[0]==0x94 && ci->sw[1]==0x02)
      break;
    if(ci->sw[0]!=0x90 || ci->sw[1]!=0x00)
      fatal("error updating record");
  }

  /* restore records */
  recordIdx=1;
  for(;;) {
    if(!fgets(line, sizeof(line), stdin))
      break;
    cp=line;
    while(*cp && isspace(*cp)) cp++;
    if(*cp==':') {
      cp++;
      recordIdx=atoi(cp);
      continue;
    }
    number=cp;
    while(*cp && !isspace(*cp)) cp++;
    if(*cp)
      *cp++=0;
    if(!*number)
      continue;

    while(*cp && isspace(*cp)) cp++;
    name=cp;
    len=strlen(name);
    while(len>0 && isspace(name[len-1]))
      len--;
    name[len]=0;

    if(*number) {
      memset(buf, 0xFF, recordSize);
      if(*number=='+') {
	buf[recordSize-13]=0x91;
	number++;
      } else
	buf[recordSize-13]=0x81;
      for(i=0; i<recordSize-14 && name[i]; i++)
	buf[i]=name[i];
      bp=buf+recordSize-12;
      n=v=shift=0;
      for(i=0; i<20 && number[i]; i++) {
	if(number[i]>='0' && number[i]<='9')
	  nibble=number[i]-'0';
	else
	  switch(number[i]) {
	  case '*':
	    nibble=10;
	    break;
	  case '#':
	    nibble=11;
	    break;
	  case '.':
	    nibble=12;
	    break;
	  default:
	    nibble=-1;
	  }
	if(nibble<0)
	  continue;
	v+=nibble<<shift;
	shift+=4;
	if(shift>=8) {
	  *bp++=v;
	  n++;
	  v=shift=0;
	}
      }
      if(shift>0) {
	*bp++=v|0xF0;
	n++;
      }
      buf[recordSize-14]=n;

      if(scGsmsimCmdUpdateRec(ri, ci, recordIdx, SC_GSMSIM_RECMODE_ABS, buf, recordSize))
	fatal("error updating record");
      if(ci->sw[0]==0x94 && ci->sw[1]==0x02)
	break;
      if(ci->sw[0]!=0x90 || ci->sw[1]!=0x00)
	fatal("error updating record");
    }

    recordIdx++;
  }

}

int main(int argc, char *argv[])
{
  SC_READER_INFO *ri;
  SC_CARD_INFO *ci;
  SC_READER_CONFIG rc;
  char pin[8], *cp;
  int n;

  prgName=argv[0];

  if(scInit())
    fatal("couldn't initialize library");

  rc.type=READER_TYPE;
  rc.slot=READER_SLOT;
  rc.param=READER_PORT;

  if(scReaderGetConfig(argc, argv, &rc))
    fatal("error getting reader configuration");

  if(!(ri=scGeneralNewReader(rc.type, rc.slot)))
    fatal("error creating reader object");

  if(!(ci=scGeneralNewCard()))
    fatal("error creating card object");

  if(scReaderInit(ri, rc.param))
    fatal("couldn't initialize reader");

  if(scReaderActivate(ri))
    fatal("couldn't activate reader");

  if(scReaderCardStatus(ri))
    fatal("couldn't retrieve card status");

  if(!(ri->status&SC_CARD_STATUS_PRESENT))
    fatal("Please insert a card into the reader");

  if(scReaderResetCard(ri, ci))
    fatal("Error while resetting card");

  if(scSmartcardGetCardType(ci))
    fatal("Couldn't query card type");

  if(ci->type!=SC_CARD_GSMSIM)
    fatal("Unsupported GSM card in reader");

  cp=getpass("Please enter the PIN: ");
  n=strlen(cp);
  if(n>sizeof(pin))
    n=sizeof(pin);
  memcpy(pin, cp, n);
  if(n<sizeof(pin))
    memset(pin+n, 0xFF, sizeof(pin)-n);

  if(scGsmsimCmdVerifyCHV(ri, ci, 1, pin) ||
     ci->sw[0]!=0x90 || ci->sw[1]!=0x00)
    fatal("Invalid PIN");

  if(strlen(prgName)>=7 &&
     strcmp(prgName+strlen(prgName)-7, "simrestore")==0)
    restore(ri, ci);
  else
    dump(ri, ci);

  scReaderDeactivate( ri );

  scReaderShutdown( ri );

  scGeneralFreeCard( &ci );
  scGeneralFreeReader( &ri );

  scEnd();

  return 0;
}
