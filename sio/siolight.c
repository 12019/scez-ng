/* sio.c	- serial I/O handler
 *
 * Copyright 1993-1997, Tim Hudson. All rights reserved.
 * Copyright 1999-2000, Matthias Bruestle. All rights reserved.
 *
 * You can pretty much do what you like with this code except pretend that 
 * you wrote it provided that any derivative of this code includes the
 * above comments unchanged. If you put this in a product then attribution
 * is mandatory. See the details in the COPYING file.
 *
 * 26-Dec-99 mb         Striped down to neccessary funktions and added
 * .................... PalmSIO.
 * 26-Sep-97 tjh        SetSettingsString() now accepts scam style 
 * .................... settings ->  Baud<SPACE>DataParityStop<SPACE>Conv
 * 15-Sep-97 tjh        overlapped I/O for WIN32
 * 30-Aug-97 tjh	reworked into this form from a pile of other
 * .................... code tied into various things written over the
 * .................... last 12 years (I finally got tierd of seeing all
 * .................... the non-portable interfaces in use)
 *
 * Tim Hudson
 * tjh@cryptsoft.com
 *
 *
 */

/* $Id: siolight.c 1062 2001-09-24 13:12:06Z zwiebeltu $ */

/* Try and figure out if we're running under Windows and/or Win32 */

#if !defined( WINDOWS ) && ( defined( _Windows ) || defined( _WINDOWS ) || \
		 defined( WIN32 ) || defined( _WIN32 ) )
#define WINDOWS
#endif /* Windows-detection code */

#ifndef __palmos__
#include <stdio.h>
#include <ctype.h>
#endif /* !__palmos__ */

#include <stdlib.h>

#ifdef WINDOWS
#include <windows.h>
#include <io.h>

/* Time in ms added to timeouts.
 */
#define ADD_DELAY 15

/* there is a threaded way of doing things and a non-threaded
 * Win95 works with the threaded way (I was avoiding overlapped
 * I/O for a pile of reasons) but WinNT does not
 * 
 * so overlapped I/O is what is done now
 */
#define NO_THREADS

#elif !defined(__palmos__) /* !WINDOWS && !__palmos__ */

#include <unistd.h>

#endif /* WINDOWS */

#if defined(LINUX)
#define USE_FCNTL
#endif

#if defined(sgi) || defined(_AIX) || defined(SVR4) || defined(SUNOS5) || defined(__sparc) || defined(__hpux)
#define USE_TERMIO
#endif

#if defined(LINUX) || defined(linux) || defined(__bsdi__) || defined(__osf__) || defined(__FreeBSD__)
#define USE_TERMIOS
#endif

#if defined(SCO) || defined(_M_XENIX)
#define USE_TERMIO
#define NO_MODEM_CONTROL
#endif

#ifdef USE_TERMIO
#define TERM_GET(fd,ptr) ioctl(fd,TCGETA,ptr)
#define TERM_SET(fd,ptr) ioctl(fd,TCSETA,ptr)
#define TERM_STATE struct termio
#define TERM_GETSPEED(fd,ptr) (ptr->c_cflag & CBAUD)
#define TERM_SETSPEED(fd,ptr,val) (ptr->c_cflag &= ~CBAUD),(ptr->c_cflag|=val)
#endif /* USE_TERMIO */

#ifdef USE_TERMIOS
#define TERM_GET(fd,ptr) tcgetattr(fd,ptr)
#define TERM_SET(fd,ptr) tcsetattr(fd,TCSANOW,ptr)
#define TERM_GETSPEED(fd,ptr) cfgetospeed(ptr)
#define TERM_SETSPEED(fd,ptr,val) cfsetospeed(ptr,val),cfsetispeed(ptr,val)
#define TERM_STATE struct termios
#endif /* USE_TERMIOS */

#ifdef USE_SGTTY
#define TERM_GET(fd,ptr) ioctl(fd,TIOCGETP,ptr)
#define TERM_SET(fd,ptr) ioctl(fd,TIOCSETN,ptr)
#define TERM_STATE struct sgttyb 
#endif /* USE_SGTTY */

/* I was planning on some independant stuff with the following
 * however the flags are not different between the three varients
 */
/*
#define TERM_CONTROL_GET(fd,ptr) ioctl(fd,TIOCMGET,ptr)
#define TERM_CONTROL_SET(fd,ptr) ioctl(fd,TIOCMSET,ptr)
*/

#ifdef WINDOWS
typedef struct {
  int dummy;
  DCB dcb;
} TERM_STATE;
#endif

#ifndef __palmos__
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>
#endif /* !__palmos__ */

#if defined(unix) || defined(__unix) || defined(__unix__)
#include <sys/ioctl.h>
#include <sys/time.h>
#endif

#ifdef USE_SGTTY
#include <sgtty.h>
#endif
#ifdef USE_TERMIO
#include <sys/termio.h>
#endif
#ifdef USE_TERMIOS
#include <sys/termios.h>
#endif

#if defined(USE_FLOCK)
#include <sys/file.h>
#endif /* USE_FLOCK */

#ifdef __palmos__
#ifdef PALM_III
#define WITH_CONTROL
#endif

#include <Pilot.h>
#include <SerialMgr.h>

#define memcpy(x,y,z)	(MemMove(x,(VoidPtr)y,z) ? x : x)
#define memset(x,y,z)	(MemSet(x,z,y))
#define memcmp(x,y,z)	(MemCmp(x,y,z))

#define UART_TRANS_REG	0xFFFFF906
#define UART_MISC_REG	0xFFFFF908
#endif /* __palmos__ */

/* this is the internal virtualised view of a serial comms device
 * that we are not exposing to the caller so that they will not
 * do nasty things and make their application code non-portable
 * because they play with the internals
 */
typedef struct sio_info {
#ifdef WINDOWS
  HANDLE fd;		/* file descriptor/comms handle */
#elif defined( __palmos__ )
  UInt fd;			/* Serial library reference number */
  SerSettingsType serSettings;	/* Serial settings */
    /* UInt32 baudRate; - Baud rate
     * UInt32 flags;    - Flags
     * Long ctsTimeout; - CTS timeout
     */
#else /* !WINDOWS && !__palmos__ */
  int fd;		/* file descriptor/comms handle */
#endif /* WINDOWS */

  long speed;		/* virtual speed as an integer */
  int databits;		/* databits as an int */
  int stopbits;		/* stopbits as an int */
  int parity;		/* parity as one of SIO_PARITY_XXXX */
  int control_state;	/* virtual control state as one of SIO_CONTROL_XXXX */

  int update_required;	/* settings changed but not yet updated */
  int update_control;   /* control settings changed but not yet updated */

  int iomode;		/* direct or indirect - ISO7816 stuff! */

#ifndef __palmos__
  void *term_state;	    /* native state ... */
#endif /* !__palmos__ */

  long read_timeout;	/* timeout for reading */

#ifdef WINDOWS
  HANDLE hStartWait;	/* thread waits on this before doing WaitCommEvent */
  HANDLE hFinishedWait; /* thread signals this if WaitCommEvent finishes */
  HANDLE hWaitThread;   /* handle to the worker thread */
  DWORD  threadID;	/* ID of the worker thread */
  DWORD  eventMask;
  OVERLAPPED *overlapped;
#endif /* WINDOWS */
} SIO_INFO;

#ifdef WINDOWS
#define BAD_FD (HANDLE)-1
#elif defined( __palmos__ )
#define BAD_FD 0xFFFF
#else
#define BAD_FD -1
#endif

/* pull in the interface defn leaving out the SIO_INFO typedef */
#define SIO_INTERNAL_BUILD
#include "siolight.h"

#ifndef __palmos__
/* convert from numeric speeds into the internal format required by
 * the terminal handling routines that we sit on top of 
 */
static long speedcvt[] =
{
#ifdef WINDOWS
  /* I *know* that these are direct mappings ... but they might not be 
   * in future and the constants are there so we are using them
   */
  0, 0, 110, CBR_110, 300, CBR_300, 600, CBR_600, 1200, CBR_1200, 
  2400, CBR_2400, 4800, CBR_4800, 9600, CBR_9600, 14400, CBR_14400, 
  19200, CBR_19200, 38400, CBR_38400, 56000, CBR_56000, 128000, CBR_128000, 
  256000, CBR_256000
#else /* !WINDOWS */
  /* map things across ... some platforms will not have the full list so 
   * it will need some work done to allow for this
   */
  0, B0, 50, B50, 75, B75, 110, B110, 134, B134, 150, B150, 200, B200,
  300, B300, 600, B600, 1200, B1200, 1800, B1800, 2400, B2400, 4800, B4800,
  9600, B9600, 19200, B19200, 
#ifdef B38400
  38400L, B38400, 
#endif
#ifdef B57600
  57600L, B57600, 
#endif
#ifdef B115200
  115200L, B115200,
#endif
#ifdef B230400
  230400L, B230400,
#endif
  /* finish the table cleanly */
  0,0
#endif /* WINDOWS */
};
#define N_speedcvt (sizeof(speedcvt)/sizeof(speedcvt[0]))

static long speed2internal(long s)
{
  int i;

  for (i=0;i<N_speedcvt;i+=2) {
    if (speedcvt[i]==s)
      return speedcvt[i+1];
  }
  return 0;
}

static long internal2speed(long s)
{
  int i;

  for (i=0;i<N_speedcvt;i+=2) {
    if (speedcvt[i+1]==s)
      return speedcvt[i];
  }
  return 0;
}

static int databitscvt[] = { 
#ifdef WINDOWS
  /* hmm ... what range is supported? */
  7,7,8,8
#else /* !WINDOWS */
  5, CS5, 6, CS6, 7, CS7, 8, CS8 
#endif /* WINDOWS */
};
#define N_databitscvt (sizeof(databitscvt)/sizeof(databitscvt[0]))

static int databits2internal(int s)
{
  int i;

  for (i=0;i<N_databitscvt;i+=2) {
    if (databitscvt[i]==s)
      return databitscvt[i+1];
  }
  return 0;
}

static int internal2databits(int s)
{
  int i;
  for (i=0;i<N_databitscvt;i+=2) {
    if (databitscvt[i+1]==s)
      return databitscvt[i];
  }
  return 0;
}
#endif /* !__palmos__ */

static unsigned char reverse(unsigned char c)
{
  unsigned char s;
  unsigned char r;
  int i;

  s=c;
  r=s & 0x1;
  for(i=0;i<7;i++) {
    r<<=1; 
    s>>=1;
    r=r|(s & 0x1);
  }

  s=0;

  return r;
}

static void do_translate(SIO_INFO *s,unsigned char *buf,int len,int dir)
{
  int i;

  /* dir:0=read,1=write */
  if (s->iomode==SIO_IOMODE_DIRECT)
    goto done_it;
  for(i=0;i<len;i++) {
    buf[i]=reverse((unsigned char)(~buf[i]));
  }

done_it: ;
}

#ifdef WINDOWS

static int doSleep(unsigned long msecs)
{
  unsigned long val;
 
  val=msecs/100;
  if (val==0)
    val=1;
  Sleep(val);
  return (0);
}

static long sio_thread_proc(LPVOID arg)
{
  SIO_INFO *s=(SIO_INFO *)arg;
  DWORD dwErr;

  while(1) {
    /* wait for something to do */
    WaitForSingleObject(s->hStartWait,INFINITE);

    /* at this point all events should be cleared */
    ResetEvent(s->hFinishedWait);
    ResetEvent(s->hStartWait);

    if (WaitCommEvent(s->fd,&s->eventMask,NULL)==FALSE) {
      dwErr=GetLastError();
      if (dwErr==ERROR_IO_PENDING) {
	/* overlapped things ... handle with timeout on wait? */
      }
    }

    SetEvent(s->hFinishedWait);
  }
  return(0);
}


static HANDLE COMM_OPEN(SIO_INFO *s, char *dev)
{
  HANDLE fd;

  fd=CreateFile( dev, GENERIC_READ|GENERIC_WRITE,
                  0,                    /* exclusive access */
                  NULL,                 /* no security attrs */
                  OPEN_EXISTING,
                  FILE_ATTRIBUTE_NORMAL 
#ifdef NO_THREADS
		  |FILE_FLAG_OVERLAPPED /* overlapped I/O */
#endif /* NO_THREADS */
                  ,NULL );

  if (fd==(HANDLE)-1) {
    return(HANDLE)(-1);
  }

  /* we are interested in knowing when input data is available! */
  SetCommMask(fd,EV_RXCHAR);

  /* recommend 2k buffers ... we might just want to ram lots
   * of data up and down ... maybe move this to 4k later
   */
  SetupComm(fd,2048,2048) ;

  return(fd);
}

static int COMM_CLOSE(SIO_INFO *s)
{
  if (s!=NULL) {
    CloseHandle(s->fd);
    s->fd=BAD_FD;
    return(1);
  } else {
    return(-1);
  }
}

int CONTROL_GET(SIO_INFO *s)
{
  DCB dcb;

  if (s!=NULL) {
    s->control_state=0;

#if 1
    /* GetCommModemStatus */

    /* we tell lots of lies about this as it appears to be
     * beyond the standard API to return us this level of
     * detail in terms of what is happening!
     */
    s->control_state|=SIO_CONTROL_DTR;
    s->control_state|=SIO_CONTROL_RTS;
#endif
    if (GetCommState(s->fd,&dcb)) {
    }
    return(1);
  } else {
    return(-1);
  }
}

int CONTROL_SET(SIO_INFO *s)
{
  BOOL b;

  if (s!=NULL) {
    if (s->control_state & SIO_CONTROL_RTS)
      b=EscapeCommFunction(s->fd,SETRTS);
    else
      b=EscapeCommFunction(s->fd,CLRRTS);
    if (s->control_state & SIO_CONTROL_DTR)
      b=EscapeCommFunction(s->fd,SETDTR);
    else
      b=EscapeCommFunction(s->fd,CLRDTR);
    PurgeComm(s->fd,PURGE_RXABORT|PURGE_RXCLEAR);
    doSleep(100); /* 10 is too short */
  } else {
    return(-1);
  }
}

int TERM_SET(HANDLE fd,TERM_STATE *ts)
{
  if (SetCommState(fd,&ts->dcb))
    return(1);
  else
    return(-1);
}

int TERM_GET(HANDLE fd,TERM_STATE *ts)
{
  if (GetCommState(fd,&ts->dcb)) {
    /* we must have control over RTS! */
    ts->dcb.fRtsControl=RTS_CONTROL_ENABLE;
    /* and probably DTR too */
    ts->dcb.fDtrControl=DTR_CONTROL_ENABLE;
    SetCommState(fd,&ts->dcb);
    return(1);
  } else
    return(-1);
}

static int sio_init_overlapped(SIO_INFO *s) 
{
#ifdef NO_THREADS 
  /* make sure the event is non-signalled */
  ResetEvent(s->hFinishedWait);

  /* clear out whatever is there and set the event */
  memset(s->overlapped,0,sizeof(OVERLAPPED));
  s->overlapped->hEvent=s->hFinishedWait;
#endif /* NO_THREADS */

  return(1);
}

static int sio_wait_overlapped(SIO_INFO *s,int timedelay) 
{
  DWORD err;
  int ret;

  timedelay/=1000;
  timedelay+=ADD_DELAY;

  err=WaitForSingleObject(s->hFinishedWait,timedelay);

  switch(err) {
    case WAIT_OBJECT_0:
      ret=1;
      break;
    case WAIT_ABANDONED:
    case WAIT_TIMEOUT:
    default:
      ret=0;
      break;
  }
  return (ret);
}

#elif defined( __palmos__ )

static int doSleep(unsigned long msecs)
{
  if( SysTaskDelay( ((msecs*sysTicksPerSecond)/1000000)+1 )==0 ) return(0);
  else return(-1);
}

static int COMM_OPEN(SIO_INFO *s, char *dev)
{
  Err e;
  UInt fd;

  if( strcmp( dev, "1" ) ) return( BAD_FD );

  e = SysLibFind("Serial Library", &fd );
  if( e ) return( BAD_FD );

  e = SerOpen( fd, 0, 9600 );
  if( e==serErrAlreadyOpen ) {
    SerClose( fd );
    return( BAD_FD );
  } else if( e==serErrBadParam ) {
    return( BAD_FD );
  }

  return(fd);
}

static int COMM_CLOSE(SIO_INFO *s)
{
  if (s!=NULL) {
    if( s->fd!=BAD_FD ) {
      SerReceiveFlush( s->fd, 0 );
      SerSetReceiveBuffer( s->fd, NULL, 0 );
      SerClose( s->fd );
    }
    s->fd=BAD_FD;
    return(1);
  } else {
    return(-1);
  }
}

int CONTROL_GET(SIO_INFO *s)
{
#ifdef NO_MODEM_CONTROL
  return(-1);
#else /* !NO_MODEM_CONTROL */

#ifdef WITH_CONTROL
  Word *uarttrans=(Word *)UART_TRANS_REG, *uartmisc=(Word *)UART_MISC_REG, w;

  if( s==NULL ) return(-1);

  s->control_state = 0;

  /* Read CTS. */
  w = *uarttrans;
  if( w & 0x0200 ) s->control_state |= SIO_CONTROL_CTS;
  else s->control_state &= (~SIO_CONTROL_CTS);

  /* Read RTS. */
  w = *uartmisc;
  if( w & 0x0040 ) s->control_state |= SIO_CONTROL_RTS;
  else s->control_state &= (~SIO_CONTROL_RTS);
#endif /* WITH_CONTROL */

  return(1);
#endif /* NO_MODEM_CONTROL */
}

int CONTROL_SET(SIO_INFO *s)
{
#ifdef NO_MODEM_CONTROL
  return(-1);
#else /* !NO_MODEM_CONTROL */

#ifdef WITH_CONTROL
  Word *uartmisc=(Word *)UART_MISC_REG, w;

  if( s==NULL ) return(-1);

  w = *uartmisc;
  w &= (~0x00C0);
  if( s->control_state && SIO_CONTROL_RTS ) w |= 0x0040;
  *uartmisc = w;
#endif /* WITH_CONTROL */

  return(1);
#endif /* NO_MODEM_CONTROL */
}

#else /* !WINDOWS && !__palmos__ */

/* this must be a real (i.e. Unix) operating system */

static int doSleep(unsigned long msecs)
{
  unsigned long val;
 
  val=msecs;
  if (val==0)
    val=1;

  /* TODO XXXX - this will need porting to various platforms where
   *             usleep isn't already defined
   */
#ifndef sgi
  usleep(val);
#endif /* sgi */
  return (0);
}

static int COMM_OPEN(SIO_INFO *s, char *dev)
{
  int fd;
#if defined(USE_FCNTL)
  struct flock lock;
#endif /* USE_FCNTL */

  /* This apparently weird construct is needed at least on NetBSD, which
   * expects there to be a modem attached to the serial port, and waits for DTR
   * to activate. If this is too much of a kludge, there is always the
   * /dev/dty00 ports available, but it's better to be explicit, yes? --Perm */
#if defined(O_NDELAY) && defined(F_SETFL)
   int n;
   fd = open(dev, O_RDWR|O_NDELAY);
   if (fd >= 0){
     /* Cancel the O_NDELAY flag. */
     n = fcntl(fd, F_GETFL, 0);
     (void) fcntl(fd, F_SETFL, n & ~O_NDELAY);
#if defined(USE_FLOCK)
     if( flock( fd, LOCK_EX|LOCK_NB ) ) {
       fd=BAD_FD;
     }
#elif defined(USE_FCNTL)
     lock.l_type=F_WRLCK;
     lock.l_start=0;
     lock.l_whence=SEEK_SET;
     lock.l_len=0;
     lock.l_pid=0;
     if( fcntl( fd, F_SETLK, &lock ) ) {
       fd=BAD_FD;
       if (sio_debug) printf( "Fcntl: errno=%d\n", errno);
     }
#endif /* USE_FLOCK */
   }

#else
   fd = open(dev, O_RDWR);

#if defined(USE_FLOCK)
   if (fd >= 0){
     if( flock( fd, LOCK_EX|LOCK_NB ) ) {
       fd=BAD_FD;
     }
   }
#elif defined(USE_FCNTL)
   if (fd >= 0){
     lock.l_type=F_WRLCK;
     lock.l_start=0;
     lock.l_whence=SEEK_SET;
     lock.l_len=0;
     lock.l_pid=0;
     if( fcntl( fd, F_SETLK, &lock ) ) {
       fd=BAD_FD;
       if (sio_debug) printf( "Fcntl: errno=%d\n", errno);
     }
   }
#endif /* USE_FLOCK */
#endif

  return(fd);
}

static int COMM_CLOSE(SIO_INFO *s)
{
  if (s!=NULL) {
    if( s->fd!=BAD_FD ) {
#if defined(USE_FLOCK)
      flock( s->fd, LOCK_EX );
#elif defined(USE_FCNTL)
      struct flock lock;

      lock.l_type=F_UNLCK;
      lock.l_start=0;
      lock.l_whence=SEEK_SET;
      lock.l_len=0;
      lock.l_pid=0;
      if( fcntl( s->fd, F_SETLK, &lock ) ) {
        if (sio_debug) printf( "Fcntl(F_UNLCK): errno=%d\n", errno);
      }
#endif /* USE_FLOCK */
      close(s->fd);
      s->fd=BAD_FD;
    }
    return(1);
  } else {
    return(-1);
  }
}

int CONTROL_GET(SIO_INFO *s)
{
  int ret,val;

#ifdef NO_MODEM_CONTROL
  return(-1);
#else /* !NO_MODEM_CONTROL */
  /* sanity check */
  if (s==NULL)
    return(-1);
  s->control_state=0;
  ret=ioctl(s->fd,TIOCMGET,&val);
  if (ret>=0) {
    /* convert from device view to our virtualised view */
    if (val & TIOCM_RTS)
      s->control_state|=SIO_CONTROL_RTS;
    if (val & TIOCM_DTR)
      s->control_state|=SIO_CONTROL_DTR;
    if (val & TIOCM_DSR)
      s->control_state|=SIO_CONTROL_DSR;
    if (val & TIOCM_CTS)
      s->control_state|=SIO_CONTROL_CTS;
    if (val & TIOCM_CD)                         /* Custom */
      s->control_state|=SIO_CONTROL_DCD;
    return(1);
  } else {
    return(-1);
  }
#endif /* NO_MODEM_CONTROL */
}

int CONTROL_SET(SIO_INFO *s)
{
  int ret,val;

#ifdef NO_MODEM_CONTROL
  return(-1);
#else /* !NO_MODEM_CONTROL */
  /* sanity check */
  if (s==NULL)
    return(-1);

  /* get the current settings as we do not want
   * to disturb anything that we don't know about
   * (as we only understand the useful subset)
   */
  val=0;
  ret=ioctl(s->fd,TIOCMGET,&val);
  if (ret<0)
    return(-1);

  /* convert */
  if (s->control_state & SIO_CONTROL_RTS)
    val|=TIOCM_RTS;
  else
    val&=(~TIOCM_RTS);
  if (s->control_state & SIO_CONTROL_DTR)
    val|=TIOCM_DTR;
  else
    val&=(~TIOCM_DTR);
  if (s->control_state & SIO_CONTROL_DSR)
    val|=TIOCM_DSR;
  else
    val&=(~TIOCM_DSR);
  if (s->control_state & SIO_CONTROL_CTS)
    val|=TIOCM_CTS;
  else
    val&=(~TIOCM_CTS);
  if (s->control_state & SIO_CONTROL_DCD)       /* Custom */
    val|=TIOCM_CD;
  else
    val&=(~TIOCM_CD);

  ret=ioctl(s->fd,TIOCMSET,&val);

  if (ret>=0)
    return(1);
  else
    return(-1);
#endif /* NO_MODEM_CONTROL */
}

#endif /* WINDOWS */

static int SIO_Internal2Settings(SIO_INFO *s)
{
#ifndef __palmos__
  TERM_STATE *ts;
#endif

  /* sanity check */
  if (s==NULL)
    return -1;

#ifdef WINDOWS
  /* having this handy avoids zillions of casts */
  ts=(TERM_STATE *)s->term_state;

  s->speed=internal2speed(ts->dcb.BaudRate);
  s->databits=ts->dcb.ByteSize;
  switch(ts->dcb.StopBits) {
    case 0: s->stopbits=1; break;  /* 1 */
    case 1: s->stopbits=0; break;  /* 1.5 - is this used anywhere? */
    case 2: s->stopbits=2; break;  /* 2 */
    default: s->stopbits=0;
  }
  if (ts->dcb.fParity) {
    switch(ts->dcb.Parity) {
      case NOPARITY: s->parity=SIO_PARITY_NONE; break;
      case ODDPARITY: s->parity=SIO_PARITY_ODD; break;
      case EVENPARITY: s->parity=SIO_PARITY_EVEN; break;
      default: s->parity=0; break;
    }
  } else {
    s->parity=SIO_PARITY_IGNORE;
  }

#elif defined( __palmos__ )
  s->speed=s->serSettings.baudRate;

  if( s->serSettings.flags & serSettingsFlagBitsPerChar6 ) s->databits=6;
  else if( s->serSettings.flags & serSettingsFlagBitsPerChar7 ) s->databits=7;
  else if( (s->serSettings.flags & serSettingsFlagBitsPerChar8) ==
    serSettingsFlagBitsPerChar8 ) s->databits=8;
  else s->databits=5;

  if( s->serSettings.flags & serSettingsFlagParityOnM ) {
    if( s->serSettings.flags & serSettingsFlagParityEvenM )
      s->parity=SIO_PARITY_EVEN;
    else
      s->parity=SIO_PARITY_ODD;
  } else s->parity=SIO_PARITY_NONE;

  if( s->serSettings.flags & serSettingsFlagStopBits2 ) s->stopbits=2;
  else s->stopbits=1;

#else /* !WINDOWS && !__palmos__ */
  /* having this handy avoids zillions of casts */
  ts=(TERM_STATE *)s->term_state;

  /* disect into something more useful to an app */
  s->speed=internal2speed(TERM_GETSPEED(s->fd,ts));
  s->databits=internal2databits(ts->c_cflag & CSIZE);
  s->stopbits=(ts->c_cflag & CSTOPB) ? 2 : 1;
  if (ts->c_cflag & PARENB) {
    s->parity=(ts->c_cflag & PARODD) ? SIO_PARITY_ODD : SIO_PARITY_EVEN;
  } else {
    s->parity=SIO_PARITY_NONE;
  }
  if (ts->c_iflag & IGNPAR)
    s->parity=SIO_PARITY_IGNORE;
#endif /* WINDOWS */

  return(1);
}

static int SIO_Settings2Internal(SIO_INFO *s)
{
#ifndef __palmos__
  TERM_STATE *ts;
#endif

  /* sanity check */
  if (s==NULL)
    return -1;

#ifdef WINDOWS
  /* having this handy avoids zillions of casts */
  ts=(TERM_STATE *)s->term_state;

  ts->dcb.BaudRate=speed2internal(s->speed);
  ts->dcb.ByteSize=s->databits;
  s->databits=ts->dcb.ByteSize;
  switch(s->stopbits) {
    case 0: ts->dcb.StopBits=0; break;
    case 1: ts->dcb.StopBits=0; break;
    case 2: ts->dcb.StopBits=2; break;
    default: ts->dcb.StopBits=0; break;
  }
  if (s->parity==SIO_PARITY_IGNORE) {
    ts->dcb.fParity=FALSE;
  } else {
    ts->dcb.fParity=TRUE;
    switch(s->parity) {
  case SIO_PARITY_NONE: ts->dcb.Parity=NOPARITY; break;
  case SIO_PARITY_ODD: ts->dcb.Parity=ODDPARITY; break;
  case SIO_PARITY_EVEN: ts->dcb.Parity=EVENPARITY; break;
  default: ts->dcb.Parity=0; break;
    }
  }

  if (TERM_SET(s->fd,ts)>=0) 
    return (1);
  else
    return (-1);

#elif defined( __palmos__ )
  s->serSettings.flags=0;

  s->serSettings.baudRate=s->speed;

  switch( s->databits ) {
    case 6:
      s->serSettings.flags |= serSettingsFlagBitsPerChar6;
      break;
    case 7:
      s->serSettings.flags |= serSettingsFlagBitsPerChar7;
      break;
    case 8:
      s->serSettings.flags |= serSettingsFlagBitsPerChar8;
      break;
    default:
      break;
  }

  switch(s->parity) {
    case SIO_PARITY_ODD:
      s->serSettings.flags |= serSettingsFlagParityOnM;
      s->serSettings.flags &= ~serSettingsFlagParityEvenM;
      break;
    case SIO_PARITY_EVEN:
      s->serSettings.flags |= serSettingsFlagParityOnM;
      s->serSettings.flags |= serSettingsFlagParityEvenM;
      break;
    case SIO_PARITY_NONE:
    case SIO_PARITY_IGNORE:
    default:
      break;
  }

  if( s->stopbits==2 ) s->serSettings.flags |= serSettingsFlagStopBits2;

  return(1);

#else /* !WINDOWS && !__palmos__ */
  /* having this handy avoids zillions of casts */
  ts=(TERM_STATE *)s->term_state;

  /* blank things we don't care about */
  memset(ts,0,sizeof(TERM_STATE));
  /*
  ts->c_oflag=0;
  ts->c_lflag=0;
  ts->c_line=0;
  ts->c_cflag=0;
  ts->c_iflag=0;
  */

  /* do whatever it is that has to be done to change
   * the speed in the config ...
   */
  TERM_SETSPEED(s->fd,ts,speed2internal(s->speed));

  /* set the things that matter */
  ts->c_cflag|=databits2internal(s->databits);
  ts->c_cflag|=CREAD|HUPCL|CLOCAL;

  /* if we have an extra stop bit then set it */
  if (s->stopbits==2)
    ts->c_cflag|=CSTOPB;

  switch(s->parity) {
    case SIO_PARITY_ODD: ts->c_cflag|=PARODD; ts->c_cflag|=PARENB; break;
    case SIO_PARITY_EVEN: ts->c_cflag&=(~PARODD); ts->c_cflag|=PARENB; break;
    case SIO_PARITY_NONE: break;
    case SIO_PARITY_IGNORE: ts->c_iflag|=IGNPAR; break;
    default: break;
  }

  /* set input modes so we get raw handling */
  ts->c_iflag &= ~(IGNPAR|PARMRK|INLCR|IGNCR|ICRNL);
  ts->c_iflag |= BRKINT;
  ts->c_lflag &= ~(ICANON|ECHO|ISTRIP);
  ts->c_cc[VMIN] = 1;
  ts->c_cc[VTIME] = 1;

  if (TERM_SET(s->fd,ts)>=0) 
    return (1);
  else
    return (-1);
#endif /* WINDOWS */
}

SIO_INFO *SIO_Open(char *dev)
{
  SIO_INFO *s;

  /* allocate a new data structure to hold these details */
  s=(SIO_INFO *)malloc(sizeof(SIO_INFO));
  if (s==NULL)
    goto err;

  /* "blank" to start with */
  memset(s,0,sizeof(SIO_INFO));
  s->fd=BAD_FD;
  s->update_required=0;
  s->update_control=0;
  s->iomode=SIO_IOMODE_DIRECT;
  s->read_timeout=SIO_READ_WAIT_DEFAULT;

  s->fd=COMM_OPEN(s,dev);
  if ((int)s->fd<0)
    goto err;

#ifndef __palmos__
  /* get space to hold the terminal state */
  s->term_state=(TERM_STATE *)malloc(sizeof(TERM_STATE));
  if (s->term_state==NULL)
    goto err;
#endif /* !__palmos__ */

  if (SIO_ReadSettings(s)<0) {
    COMM_CLOSE(s);
    goto err;
  }

  /* now get the control settings */
  if (SIO_ReadControlState(s)<0) {
    COMM_CLOSE(s);
    goto err;
  }

#ifdef WINDOWS

  if( (s->hStartWait=CreateEvent(NULL,TRUE,FALSE,NULL))==NULL )
    goto err;
  if( (s->hFinishedWait=CreateEvent(NULL,TRUE,FALSE,NULL))==NULL ) {
    CloseHandle( s->hStartWait );
    goto err;
  }
#ifdef NO_THREADS
  if( (s->overlapped=(OVERLAPPED *)malloc(sizeof(OVERLAPPED)))==NULL ) {
    CloseHandle( s->hStartWait );
    CloseHandle( s->hFinishedWait );
    goto err;
  }
#else /* !NO_THREADS */
  s->hWaitThread=CreateThread(NULL,0,(LPTHREAD_START_ROUTINE)sio_thread_proc,(LPVOID)s,0,&(s->threadID));
  s->overlapped=NULL;
#endif

#endif /* WINDOWS */

  /* it all worked ... return details to the caller! */
  return(s);
  
err: ; 
  if (s!=NULL) {
    free(s);
    s=NULL;
  }
  return(NULL);
}

int SIO_Close(SIO_INFO *s)
{
  if (s!=NULL) {

    if (s->fd!=BAD_FD)
      COMM_CLOSE(s);
    s->fd=BAD_FD;
#ifdef WINDOWS
    if( s->hStartWait!=NULL )
      CloseHandle( s->hStartWait );
    if( s->hFinishedWait!=NULL )
      CloseHandle( s->hFinishedWait );
#endif /* WINDOWS */
    free(s);
    s=NULL;
  }
  return(1);
}

int SIO_ReadSettings(SIO_INFO *s)
{
  /* sanity check */
  if (s==NULL)
    return -1;

  /* get the current terminal settings */
#ifdef __palmos__
  if( SerGetSettings( s->fd, &(s->serSettings) )==0 ) {
#else /* !__palmos__ */
  if (TERM_GET(s->fd,s->term_state)>=0) {
#endif /* __palmos__ */
    /* convert across to internal format */
    if (SIO_Internal2Settings(s)>=0)
      return(1);
    else
  return(-1);
  } else {
    return(-1);
  }
}

int SIO_WriteSettings(SIO_INFO *s)
{
  int update_done;

  update_done=0;

  /* sanity check */
  if (s==NULL)
    return -1;

  SIO_Settings2Internal(s);
  if (s->update_required) {
    update_done=1;
#ifdef __palmos__
    if ( SerSetSettings( s->fd, &(s->serSettings) )==0 )
#else /* !__palmos__ */
    if (TERM_SET(s->fd,s->term_state)<0)
#endif
  s->update_required=0;
  }
  if (s->update_control) {
    update_done=1;
    if (CONTROL_SET(s)<0)
  s->update_control=0;
  }
  /* pause when settings are changed ... */
  if (update_done) {
    SIO_Delay(s,25);
  }
  return(1);
}

int SIO_WaitForData(SIO_INFO *s,long timedelay)
{
#ifdef WINDOWS
  int ret;
  long tms;
  DWORD err;
  DWORD emask;
  COMMTIMEOUTS ct;
  COMSTAT comstat;

  /* sanity check */
  if (s==NULL)
    return -1;

  tms=timedelay/1000;
  tms+=ADD_DELAY;

  ret=0;
  GetCommTimeouts(s->fd,&ct);
  if (timedelay==SIO_READ_WAIT_FOREVER) {
    ct.ReadIntervalTimeout=MAXDWORD;
    ct.ReadTotalTimeoutMultiplier=0;
    ct.ReadTotalTimeoutConstant=MAXDWORD;
  } else {
    ct.ReadIntervalTimeout=tms;
    ct.ReadTotalTimeoutMultiplier=10;
    ct.ReadTotalTimeoutConstant=tms;
    /*
    ct.ReadIntervalTimeout=10;
    ct.ReadTotalTimeoutMultiplier=10;
    ct.ReadTotalTimeoutConstant=100;
    */
  }
  SetCommTimeouts(s->fd,&ct);

  /* have a quick look to see if there is data in the buffers
   * still ... and return notification to the caller without
   * doing any of the messy thread stuff with timeouts
   */
  ClearCommError(s->fd,&emask,&comstat);
  if (comstat.cbInQue>0)
    return(1);

#if 0
  /* now wait for a character to arrive ... unfortunately this
   * operation does not have any form of timeout on it which
   * makes it rather useless for detecting IO!
   */
  emask=EV_RXCHAR|EV_ERR|EV_RLSD|EV_CTS|EV_DSR;
  ret=WaitCommEvent(s->fd,&emask,NULL);
  if (ret) {
    /* we have a character we can read ... */
    return(1);
  } else {
    err=GetLastError();
    return(-1);
  }
#endif

  /* get the worker thread to start doing something */
  s->eventMask=EV_RXCHAR|EV_ERR|EV_RLSD|EV_CTS|EV_DSR; /* Custom: DCD? */

#ifdef NO_THREADS
  /* use overlapped I/O notification to simulate having
   * a timeout on WaitCommEvent() 
   */
  sio_init_overlapped(s);
  if (WaitCommEvent(s->fd,&s->eventMask,s->overlapped)==FALSE) {
    err=GetLastError();
    if (err==ERROR_IO_PENDING) {
  if (sio_wait_overlapped(s,timedelay)) {
    return(1);
  } else {
    return(-1);
  }
    }
  } else {
    return(1);
  }
#else /* !NO_THREADS */

  /* this thread stuff works fine under Win95 but doesn't under WinNT */
  SetEvent(s->hStartWait);
  err=WaitForSingleObject(s->hFinishedWait,tms);

  switch(err) {
    case WAIT_OBJECT_0:
      return(1);
    case WAIT_ABANDONED:
    case WAIT_TIMEOUT:
    default:
      {
        DWORD oldMask;

        /* make WaitCommEvent return in the worker thread */
        GetCommMask(s->fd,&oldMask);
        SetCommMask(s->fd,0);
        SetCommMask(s->fd,oldMask);

      }
        return(-1);
  }
#endif /* NO_THREADS */

#elif defined( __palmos__ )
  Long timeout;

  /* sanity check */
  if (s==NULL)
    return -1;

  if( timedelay==SIO_READ_WAIT_FOREVER ) {
    timeout = -1;
  } else {
    timeout = ( ( timedelay * sysTicksPerSecond ) / 1000000 ) + 1;
  }

  if( SerReceiveWait( s->fd, 1, (Long) timeout )!=0 ) return( -1 );

  return( 1 );

#else /* !WINDOWS && !__palmos__ */
  int ret;
  fd_set readfds;
  struct timeval tv, *tvwait;

  /* sanity check */
  if (s==NULL)
    return -1;

  /* isn't the Unix way of doing this so much more elegant :-) */
  FD_ZERO(&readfds); 
  FD_SET(s->fd,&readfds);

  tv.tv_sec=0; 
  tv.tv_usec=timedelay;

  if (timedelay==SIO_READ_WAIT_FOREVER)
    tvwait=NULL;
  else
    tvwait=&tv;

  ret=select(s->fd+1,&readfds,NULL,NULL,tvwait);

  /* if there is one descriptor ready for read then it is
   * a success ... so we can just use the return from select
   */
  return(ret);
#endif /* WINDOWS */

}

int SIO_ReadChar(SIO_INFO *s)
{
  unsigned char ch;

  if (s!=NULL) {
#ifdef WINDOWS
    if (SIO_WaitForData(s,s->read_timeout)>0) {
      DWORD got,err;

      sio_init_overlapped(s);
      if (!ReadFile(s->fd,&ch,1,&got,s->overlapped)) {
        /* handle overlapped stuff */
        err=GetLastError();
        if (err==ERROR_IO_PENDING) {
	  if (!sio_wait_overlapped(s,s->read_timeout)) {
	    got=0;
	  }
	} else return(-1);
      }
      if (got==0)
	return(-1);
#elif defined( __palmos__ )
    Long timeout;

    timeout = (( s->read_timeout * sysTicksPerSecond ) / 1000000 ) + 1;
    if( SerReceive10( s->fd, &ch, 1, timeout )==0 ) {

#else /* !WINDOWS && !__palmos__ */
    if (SIO_WaitForData(s,s->read_timeout)>0) {
      if (read(s->fd,&ch,1)!=1)
        return (-1);
#endif /* WINDOWS */
      do_translate(s,&ch,1,0);
      return (ch);
    }
  }
  return(-1);
}

int SIO_ReadBuffer(SIO_INFO *s,char *buf,int len)
{
  int i,ch;

  /* the following simply calls into SIO_ReadChar and works
   * fine ... I've changed to this as the Win32 stuff wasn't
   * behaving the way I expected for multiple chars and at
   * the moment I don't want to think too hard about it
   */
  /* Also fine for PalmOS 1 compatibility.
   */
  for(i=0;i<len;i++) {
    ch=SIO_ReadChar(s);
    if (ch==-1) 
      break;
    buf[i]=ch & 0xff;
  }
  if (i==0)
    return(-1);
  else
    return (i);
}

int SIO_WriteChar(SIO_INFO *s,int data)
{
#ifndef __palmos__ 
  int ret;
#endif
  unsigned char ch;

  if (s!=NULL) {
    ch=data & 0xff;
    do_translate(s,&ch,1,1);
#ifdef WINDOWS
    {
      DWORD got,err;
    
      sio_init_overlapped(s);
      if (WriteFile(s->fd,&ch,1,&got,s->overlapped)) {
	ret=got;
      } else {
	/* handle overlapped stuff */
	err=GetLastError();
        if (err==ERROR_IO_PENDING) {
	  /* TODO: write timeout should be handled differently! */
	  if (sio_wait_overlapped(s,s->read_timeout)) 
	    ret=1;	
	  else
	    ret=(-1);
	} else {
	  /* some error other than overlapped I/O not yet
	   * complete is a real error
	   */
	  ret=(-1);
	}
      }
    }
    return(ret);

#elif defined( __palmos__ )
    if( SerSend10( s->fd, &ch, 1 )==0 )
        if( SerSendWait( s->fd, -1 )==0 ) return( 1 );

#else /* !WINDOWS && !__palmos__ */
    ret=write(s->fd,&ch,1);
    return(ret);
#endif /* WINDOWS */
  } 
  return(-1);
}

int SIO_WriteBuffer(SIO_INFO *s,char *buf,int len)
{
#ifndef __palmos__ 
  int ret;
#endif

  if (s!=NULL) {
    do_translate(s,buf,len,1);
#ifdef WINDOWS
    {
      DWORD got,err;
    
      sio_init_overlapped(s);
      if (WriteFile(s->fd,buf,len,&got,s->overlapped)) {
	ret=got;
      } else {
	/* handle overlapped stuff */
	err=GetLastError();
        if (err==ERROR_IO_PENDING) {
	  /* TODO: write timeout should be handled differently! */
	  if (sio_wait_overlapped(s,s->read_timeout)) 
	    ret=len;
	  else
	    ret=(-1);
	} else {
	  /* some error other than overlapped I/O not yet
	   * complete is a real error
	   */
	  ret=(-1);
	}
      }
    }
    return(ret);

#elif defined( __palmos__ )
    if( SerSend10( s->fd, buf, (ULong)len )==0 ) {
        if( SerSendWait( s->fd, -1 )==0 ) return( len );
        else return(-2);
    } else return(-3);

#else /* !WINDOWS && !__palmos__ */
    ret=write(s->fd,buf,len);
    return(ret);
#endif /* WINDOWS */
  } 
  return(-1);
}

int SIO_SetIOMode(SIO_INFO *s,int mode)
{
  if (s!=NULL) {
    s->iomode=mode;
    return(1);
  } else {
    return(-1);
  }
}

int SIO_GetIOMode(SIO_INFO *s)
{
  if (s!=NULL) 
    return(s->iomode);
  else
    return(-1);
}

int SIO_SetReadTimeout(SIO_INFO *s,long val)
{
  if (s!=NULL) {
    s->read_timeout=val;
    return(1);
  } else {
    return(-1);
  }
}

long SIO_GetReadTimeout(SIO_INFO *s)
{
  if (s!=NULL) {
    return(s->read_timeout);
  } else {
    return(-1);
  }
}

int SIO_SetSpeed(SIO_INFO *s,long speed)
{
  if (s!=NULL) {
    s->speed=speed;
    s->update_required=1;
    return(1);
  } else {
    return(-1);
  }
}

long SIO_GetSpeed(SIO_INFO *s)
{
  if (s!=NULL)
    return(s->speed);
  else
    return(-1);
}

int SIO_SetDataBits(SIO_INFO *s,int databits)
{
  if (s!=NULL) {
    s->databits=databits;
    s->update_required=1;
    return(1);
  } else {
    return(-1);
  }
}

int SIO_GetDataBits(SIO_INFO *s)
{
  return(s->databits);
}

int SIO_SetStopBits(SIO_INFO *s,int stopbits)
{
  if (s!=NULL) {
    s->stopbits=stopbits;
    s->update_required=1;
    return(1);
  } else {
    return(-1);
  }
}

int SIO_GetStopBits(SIO_INFO *s)
{
  if (s!=NULL)
    return(s->stopbits);
  else
    return(-1);
}

int SIO_SetParity(SIO_INFO *s,int parity)
{
  if (s!=NULL) {
    s->parity=parity;
    return(1);
  } else {
    return(-1);
  }
}

int SIO_GetParity(SIO_INFO *s)
{
  if (s!=NULL)
    return(s->parity);
  else
    return(-1);
}

int SIO_ReadControlState(SIO_INFO *s)
{
  if (s!=NULL) {
    if (CONTROL_GET(s)<0)
      return(-1);
    else
      return(1);
  } else {
    return(-1);
  }
}

int SIO_WriteControlState(SIO_INFO *s)
{
  if (s!=NULL) {
    if (CONTROL_SET(s)<0)
      return(-1);
    else
      return(1);
  } else {
    return(-1);
  }
}

int SIO_GetControlState(SIO_INFO *s,int ctrl)
{
  int ret;

  if (s!=NULL) {
    ret=0;
    switch(ctrl) {
      case SIO_CONTROL_RTS: ret=(s->control_state & ctrl); break;
      case SIO_CONTROL_DTR: ret=(s->control_state & ctrl); break;
      case SIO_CONTROL_DSR: ret=(s->control_state & ctrl); break;
      case SIO_CONTROL_CTS: ret=(s->control_state & ctrl); break;
      case SIO_CONTROL_DCD: ret=(s->control_state & ctrl); break; /* Custom */
      default: break;
    }
    return(ret);
  } else {
    return(-1);
  }
}

int SIO_SetControlState(SIO_INFO *s,int ctrl,int val)
{
  if (s!=NULL) {
    switch(ctrl) {
      case SIO_CONTROL_RTS: 
        if (val) 
          s->control_state|=SIO_CONTROL_RTS;
        else
          s->control_state&=(~SIO_CONTROL_RTS);
        break;
      case SIO_CONTROL_DTR:
        if (val) 
          s->control_state|=SIO_CONTROL_DTR;
        else
          s->control_state&=(~SIO_CONTROL_DTR);
        break;
      case SIO_CONTROL_DSR:
        if (val) 
          s->control_state|=SIO_CONTROL_DSR;
        else
          s->control_state&=(~SIO_CONTROL_DSR);
        break;
      case SIO_CONTROL_CTS:
        if (val) 
          s->control_state|=SIO_CONTROL_CTS;
        else
          s->control_state&=(~SIO_CONTROL_CTS);
        break;
      case SIO_CONTROL_DCD:                             /* Custom */
        if (val)
          s->control_state|=SIO_CONTROL_DCD;
        else
          s->control_state&=(~SIO_CONTROL_DCD);
        break;
      default: break;
    }
    return(1);
  } else {
    return(-1);
  }
}

int SIO_Delay(SIO_INFO *s,unsigned long delay)
{
  return doSleep(delay);
}

void SIO_Flush(SIO_INFO *s)
{
#ifdef __palmos__
  SerReceiveFlush( s->fd, 0 );
#else
  while(SIO_ReadChar(s)!=-1);
#endif
}

