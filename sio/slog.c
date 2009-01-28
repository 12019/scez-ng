/* slog.c	- SIO logging support functions
 *
 * Copyright 1993-1997, Tim Hudson. All rights reserved.
 *
 * You can pretty much do what you like with this code except pretend that 
 * you wrote it provided that any derivative of this code includes the
 * above comments unchanged. If you put this in a product then attribution
 * is mandatory. See the details in the COPYING file.
 *
 * Tim Hudson
 * tjh@cryptsoft.com
 *
 */

/* $Id: slog.c 874 2000-09-01 15:24:13Z zwiebeltu $ */

#include "platform.h"

#ifdef USE_STDIO
#include <stdio.h>
#endif /* USE_STDIO */

#include <sys/types.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>

#ifdef WINDOWS
#include <windows.h>
#include <io.h>
#else /* !WINDOWS */
#include <unistd.h>
#endif /* WINDOWS */

#include <fcntl.h>

#include "slog.h"

SLOG *SLOG_open(char *filename,int buffered)
{
  SLOG s,*sp;

  /* start with a blank entry */
  memset(&s,0,sizeof(s));
  s.hooked=0;

  if (buffered) {
#ifdef USE_STDIO
    if (strcmp(filename,"-")==0) 
      s.private=(void *)stdout;
    else
      s.private=(void *)fopen(filename,"w");
    if (s.private==NULL) 
      goto err;
#else /* !USE_STDIO */
    /* ignore the user for the moment if we are complied with
     * no stdio support - we do things non-buffered until I can
     * be bothered writing the code for a simple output buffer
     */
    goto non_buffered;
#endif /* USE_STDIO */
  } else {
#ifndef USE_STDIO
non_buffered: ;
#endif
    /* non-buffered */
    if (strcmp(filename,"-")==0) {
      s.fd=1;
      s.noclose=1;
    } else {
      s.fd=open(filename,O_WRONLY|O_TRUNC|O_CREAT,0600);
      s.noclose=0;
    }
    if (s.fd<0) 
      goto err;
  }

  /* copy things that may be of interest later or during debugging */
  s.buffered=buffered;
  s.filename=strdup(filename);

  /* everything worked ... alloc and copy values */
  sp=(SLOG *)malloc(sizeof(SLOG));
  if (sp==NULL)
    goto err;
  memcpy(sp,&s,sizeof(SLOG));

  return(sp);

err: ;
  return(NULL);
}

int SLOG_printf( VAR_PLIST( SLOG *, slog ) )
{
  VAR_BDEFN(args, BIO *, bio);
  char *format;
  int ret=0;
  char hugebuf[1024*2]; /* 2k in one chunk is the limit */

  VAR_INIT(args, SLOG *, slog);
  VAR_ARG(args, char *, format);

  /* ignore being called with a NULL SLOG */
  if (slog==NULL)
    goto skip;

  hugebuf[0]='\0';

  vsprintf(hugebuf,format,args);

  if (slog->hooked) {
    if (slog->hookwrite)
      (void)(*slog->hookwrite)(slog->hookparam,hugebuf,strlen(hugebuf));
  } else {
    if (slog->buffered) {
#ifdef USE_STDIO
      ret=fwrite(hugebuf,strlen(hugebuf),1,(FILE *)(slog->private));
      fflush((FILE *)(slog->private));
#else /* !USE_STDIO */
      goto non_buffered;
#endif /* USE_STDIO */
    } else {
#ifndef USE_STDIO
non_buffered: ;
#endif
      ret=write(slog->fd,hugebuf,strlen(hugebuf));
    }
  }

skip: ;

  VAR_END( args );
  return(ret);
}

int SLOG_flush(SLOG *s)
{
  if (s==NULL)
    return(-1);
  if (s->hooked) {
    if (s->hookflush)
      (void)(*s->hookflush)(s->hookparam);
  } else {
    if (s->buffered) {
#ifdef USE_STDIO
      fflush((FILE *)s->private);
#endif /* USE_STDIO */
    }
  }
  return (1);
}

int SLOG_close(SLOG *s)
{
  if (s==NULL)
    return(-1);
  if (s->buffered) {
#ifdef USE_STDIO
    fclose((FILE *)s->private);
#endif /* USE_STDIO */
  } else {
    if (!s->noclose)
      close(s->fd);
  }
  /* we have finished! */
  free(s);
  return (1);
}

void SLOG_dump(SLOG *fp,char *buf,int len,int text)
{
  int i;
  unsigned char ch;

  for(i=0;i<len;i++) {
    ch=buf[i] & 0xff;
    if (text)
	SLOG_printf(fp,"%2c ",isprint(ch)?ch:'.');
    if ((text==0) || (text>1))
	SLOG_printf(fp,"%02x ",ch);
  }
  SLOG_flush(fp);
}

SLOG *SLOG_openhook(void *hookparam,int (*hookwrite)(),int (*hookflush)())
{
  SLOG s,*sp;

  /* start with a blank entry */
  memset(&s,0,sizeof(s));
  s.hooked=1;

  if ((hookparam==NULL)||(hookwrite==NULL))
    goto err;

  s.hookparam=hookparam;
  s.hookwrite=hookwrite;
  s.hookflush=hookflush;

  /* everything worked ... alloc and copy values */
  sp=(SLOG *)malloc(sizeof(SLOG));
  if (sp==NULL)
    goto err;
  memcpy(sp,&s,sizeof(SLOG));

  return(sp);

err: ;
  return(NULL);
}

