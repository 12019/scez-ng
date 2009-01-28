/* $Id: varadic.h 875 2000-09-01 15:31:27Z zwiebeltu $ */

#ifndef HEADER_VARADIC_H 
#define HEADER_VARADIC_H 

/* Tim Hudson's portable varargs stuff */

#ifndef NOPROTO
#define VAR_ANSI        /* select ANSI version by default */
#endif
 
#ifdef VAR_ANSI
/* ANSI version of a "portable" macro set for variable length args */
#ifndef __STDARG_H__ /**/
#include <stdarg.h>
#endif /**/
 
#define VAR_PLIST(arg1type,arg1)    arg1type arg1, ...
#define VAR_PLIST2(arg1type,arg1,arg2type,arg2) arg1type arg1,arg2type arg2,...
#define VAR_ALIST
#define VAR_BDEFN(args,arg1type,arg1)   va_list args
#define VAR_BDEFN2(args,arg1type,arg1,arg2type,arg2)    va_list args
#define VAR_INIT(args,arg1type,arg1)    va_start(args,arg1);
#define VAR_INIT2(args,arg1type,arg1,arg2type,arg2) va_start(args,arg2);
#define VAR_ARG(args,type,arg)  arg=va_arg(args,type)
#define VAR_END(args)           va_end(args);
 
#else
 
/* K&R version of a "portable" macro set for variable length args */
#ifndef __VARARGS_H__
#include <varargs.h>
#endif
 
#define VAR_PLIST(arg1type,arg1)        va_alist
#define VAR_PLIST2(arg1type,arg1,arg2type,arg2) va_alist
#define VAR_ALIST               va_dcl
#define VAR_BDEFN(args,arg1type,arg1)   va_list args; arg1type arg1
#define VAR_BDEFN2(args,arg1type,arg1,arg2type,arg2)    va_list args; \
        arg1type arg1; arg2type arg2
#define VAR_INIT(args,arg1type,arg1)    va_start(args); \
        arg1=va_arg(args,arg1type);
#define VAR_INIT2(args,arg1type,arg1,arg2type,arg2) va_start(args); \
        arg1=va_arg(args,arg1type);     arg2=va_arg(args,arg2type);
#define VAR_ARG(args,type,arg)          arg=va_arg(args,type)
#define VAR_END(args)                   va_end(args);
 
#endif
 
#endif /* HEADER_VARADIC_H */

