/* slog.h	- SIO logging support functions
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

/* $Id: slog.h 875 2000-09-01 15:31:27Z zwiebeltu $ */

#ifndef HEADER_SLOG_H
#define HEADER_SLOG_H

/* we don't want application code to have to worry what to include
 * to get the right prototypes for varadic functions
 */
#include "varadic.h"

typedef struct {
  char *filename; /* the name of the file */
  int fd;         /* underlying descriptor */
  int noclose;    /* don't close descriptor on SLOG_close */
  int buffered;   /* if true then data is internally buffered */
  char *buf;      /* any buffered data sits here */
  int buf_max;    /* allocated size of the buffer */
  int buf_count;  /* actual bytes remaining to write */
  void *private;

  /* now for an interface for easily hooking the SLOG stuff
   * for things like SSLeay BIOs
   */
  int hooked;
  void *hookparam;
  int (*hookwrite)(void *hookparam,char *data,int len);
  int (*hookflush)(void *hookparam);

} SLOG;

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

SLOG *SLOG_open(char *filename,int buffered);
int SLOG_flush(SLOG *s);
int SLOG_close(SLOG *s);

int SLOG_printf( VAR_PLIST( SLOG *, slog ) );

void SLOG_dump(SLOG *fp,char *buf,int len,int text);

SLOG *SLOG_openhook(void *hookparam,int (*hookwrite)(),int (*hookflush)());

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* HEADER_SLOG_H */

