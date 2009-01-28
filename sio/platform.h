/* platform.h - figure out if we are using Windows or not
 *
 * Thanks to Peter Gutmann <pgut001@cs.auckland.ac.nz> for this 
 * wonderful "mess" :-)
 *
 */

/* $Id: platform.h 875 2000-09-01 15:31:27Z zwiebeltu $ */

/* Try and figure out if we're running under Windows and/or Win32.  We have
   to jump through all sorts of hoops later on, not helped by the fact that
   the method of detecting Windows at compile time changes with different
   versions of Visual C (it's different for each of VC 2.0, 2.1, 4.0, and
   4.1.  It actually remains the same from 4.1 to 4.2) */

#if !defined( __WINDOWS__ ) && ( defined( _Windows ) || defined( _WINDOWS ) )
  #define __WINDOWS__
#endif /* !__WINDOWS__ && ( _Windows || _WINDOWS ) */
#if !defined( __WIN32__ ) && ( defined( WIN32 ) || defined( _WIN32 ) )
  #ifndef __WINDOWS__
    #define __WINDOWS__
  #endif /* __WINDOWS__ */
  #define __WIN32__
#endif /* !__WIN32__ && ( WIN32 || _WIN32 ) */
#if defined( __WINDOWS__ ) && !defined( __WIN32__ )
  #define __WIN16__
#endif /* __WINDOWS__ && !__WIN32__ */
 
/* and now I map it to my version of the above define --tjh */
#ifdef __WINDOWS__
#ifndef WINDOWS
#define WINDOWS
#endif
#endif /* __WINDOWS__ */

#ifdef WINDOWS
#define STRCASECMP(X,Y)  stricmp(X,Y)
#else
#define STRCASECMP(X,Y)  strcasecmp(X,Y)
#endif

/* map from the OS/Compiler defines into something more sensible so
 * the stupid "dance" of the compiler defines is only in one place
 */
#ifdef _M_XENIX
#ifndef XENIX
#define XENIX
#endif
#endif

#ifdef __hpux
#ifndef HPUX
#define HPUX
#endif
#endif

#ifdef linux
#ifndef LINUX
#define LINUX
#endif
#endif

#if ( defined( __FreeBSD__ ) || defined ( __NetBSD__ ) )
#ifndef FREEBSD
#define FREEBSD
#endif
#endif

#if defined(__alpha) && defined(__osf__)
#ifndef ALPHA_OSF
#define ALPHA_OSF
#endif
#endif

#ifdef sgi
#ifndef IRIX
#define IRIX
#endif
#endif

#ifdef aix
#ifndef AIX
#define AIX
#endif
#endif

/* and a yucky couple to keep Peter happy */
#if defined(__sparc)
#ifndef SOLARIS
#define SOLARIS
#endif
#endif

#if defined(sparc)
#ifndef SUNOS4
#define SUNOS4
#endif
#endif

/* either version is still SUNOS and it isn't unreasonable to have
 * common code ... so we fix any oversights here
 */
#if defined(SUNOS4) || defined(SOLARIS)
#ifndef SUNOS
#define SUNOS
#endif
#endif

/* for the moment I want to use stdio on all platforms as tscam
 * itself needs this and I don't want to change the code yet
 */
#define USE_STDIO

