/*************************************************************************\
 * os2life.inc
 * This file contains all the include directives for os2life. This
 * methode has the advantage, that all header files can be precompiled
 * by Watcom C. The effect is a better turnaround time when recompiling
 * without changing any compiler switches.
\*************************************************************************/
#include <os2.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#if defined  (__EMX__) 
#include <alloca.h>
#elif defined (__IBMC__)
#include <malloc.h>
#elif defined (__WATCOMC__)
#include <process.h>
#include <malloc.h>
#else
#warning Make sure that the header files declaring alloca 
#warning and _beginthread are included here.
#endif
