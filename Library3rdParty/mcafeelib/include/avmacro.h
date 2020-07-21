/*
 *
 * Copyright (c) 1996-2018 McAfee, LLC. All Rights Reserved.
 * 
 * Basic macros and definitions required by the Olympus scanning
 * engine.
 *
 * 
 */

#ifndef AVMACRO__H
#define AVMACRO__H

/************************************************
 * Macro's break down in the following tree:    *
 *                                              *
 * MSDOS ------ CLI ------- DOS_16              *
 *                      Ý                       *
 *                      Ý-- DOS_32              *
 *                                              *
 * WINDOWS ---- GUI ------- WIN_16              *
 *                      Ý                       *
 *                      Ý-- WIN_32              *
 *                                              *
 * WINDOWS ---- CLI ------- WIN_32              *
 *                                              *
 * OS2 -------- GUI ------- OS2_32              *
 *                                              *
 * OS2 -------- CLI ------- OS2_16              *
 *                      Ý                       *
 *                      Ý-- OS2_32              *
 *                                              *
 * NETWARE ---- CLI                             *
 *                                              *
 * UNIX ------- CLI ------- SOLARIS --- SUN     *
 *                      Ý           Ý           *
 *                      Ý           Ý-- X86     *
 *                      Ý-- SCO                 *
 *                      Ý                       *
 *                      Ý-- LINUX               *
 *                                              *
 * VXD                                          *
 ************************************************/

/**********************************
 * Setup the default API version *
 **********************************/

/* The default API version is V2. */
#if !defined AVENGINEAPI_V1
  #if !defined AVENGINEAPI_V2
  #define AVENGINEAPI_V2
  #endif
#endif

/* If The V2 API is specified, ensure it overrides the V1 API */
#if defined AVENGINEAPI_V2
  #if defined AVENGINEAPI_V1
  #undef AVENGINEAPI_V1
  #endif
#endif

/**********************************
 * Start platform macro expansion *
 **********************************/

/* Added for MSVC projects [kn 9-1-98] */
#if defined(WIN32) && !defined(WIN_32)
  #define WIN_32
#endif

/* MS/DOS */
#if    defined WIN_32
#if !defined CLI && !defined GUI
#define GUI
#endif

#ifndef WINDOWS
#define WINDOWS
#endif

/* MacOS */
#elif defined MacOS
#undef GUI
#define CLI
#define	BIGEND

/* UNIX */
#elif defined UNIX
#undef GUI
#define CLI

#if defined SUN
#if !defined SOLARIS
#define SOLARIS
#endif
#elif defined HPUX10 || defined HPUX11 || defined HPUX
#if !defined HPUX
#define HPUX
#endif
#elif defined AIX
#elif defined LINUX
#if !defined X86 && !defined PPC && !defined S390X
#error Please define Linux variant
#endif
#elif defined SCO
#elif defined FREEBSD
#elif defined DARWIN_OSX
#else
#error Please define Unix variant (e.g. LINUX)
#endif

/* No appropriate platform defined!! */
#else
#error Please define a platform macro (e.g. WIN_32)
#endif

#if defined WINDOWS
#define NECPC98
#define FDDREPAIR
#define FDD3MODE
#endif

#if (defined SUN && !defined X86) || defined HPUX || defined AIX || (defined DARWIN_OSX && defined __BIG_ENDIAN__) || (defined LINUX && (defined PPC || defined S390X))
#define BIGEND
#define ALIGNMENT
#endif


#if defined WIN_64 && !defined (_64BIT_)
#define _64BIT_
#endif

#if defined _64BIT_
#define ALIGNMENT
#endif

/*****************************************
 * File system constants - DO NOT CHANGE *
 *****************************************/

/****************************************************
 * Olympus needs a MAXPATH (directory + file spec)  *
 * and a MAXEXT (file extension) definition         *
 ****************************************************/

#if defined MAXPATH
#undef MAXPATH
#endif

#if defined UNIX
#include <limits.h>
#endif

/* If PATH_MAX isn't defined in limits.h, assume 1024 */
#if defined UNIX && !defined PATH_MAX
#define PATH_MAX 1024
#endif

#if    defined DATAPOL
#define MAXPATH         260
#define MAXEXT          256
#elif defined UNIX && !defined LINUX
#define MAXPATH         PATH_MAX
#define MAXEXT          MAXPATH
#elif defined LINUX
#define MAXPATH         4096
#define MAXEXT          MAXPATH
#elif defined WIN_32
#define MAXPATH         360 
#define MAXEXT          256
#elif defined MacOS
#define MAXPATH         260
#define MAXEXT          256
#endif

/*********************************************
 * Virus related definitions - DO NOT CHANGE *
 *********************************************/

#define MAXVIRUSNAME_V1 30     /* Max length of virus name in the V1 API*/

#if !defined AV_NO_GENERIC_TYPES
#if !defined AVENGINEAPI_V2
#define MAXVIRUSNAME MAXVIRUSNAME_V1
#endif
#endif

/********************************************************************
 * Define basic types. These may be altered, as long as the type's  *
 * size remains constant - e.g. sizeof(WORD) == 2                   *
 ********************************************************************/

#if defined SCO
/* Under SCO the signed keyword generates a warning */
#define signed
#endif

#if defined WINDOWS
/* Include Windows header - it gives us some types which we'll use in 
 * preference to defining our own. (Assuming MS doesn't decide to change
 * the definition of BYTE!!)
 * Avoid any redefinition warnings... [kn 9-1-98]
 */
#ifndef STRICT
  #define STRICT
#endif
#include <windows.h>
#endif





#if !defined WINDOWS
/* Define standard types */
#if defined UNIX && defined _64BIT_
typedef unsigned long long ULONGLONG;
typedef unsigned int DWORD;
typedef unsigned short WORD;
typedef unsigned char BYTE;
#else
typedef unsigned long long ULONGLONG;
typedef unsigned long DWORD;
typedef unsigned short WORD;
typedef unsigned char BYTE;
#endif
#endif

#if !defined WINDOWS
typedef int BOOL;

#if !defined FALSE
#define FALSE 0
#endif

#if !defined TRUE
#define TRUE 1
#endif
#endif

/* Define signed versions of standard types */
#if defined UNIX && defined _64BIT_
typedef signed int SDWORD;
#else
typedef signed long SDWORD;
#endif
typedef signed short SWORD;
typedef signed char SBYTE;

#ifdef DATAPOL
#define ULONG DWORD
#define USHORT WORD
typedef ULONG* PULONG;
typedef USHORT* PUSHORT;
typedef BYTE* PBYTE;
#endif

#if defined AVENGINEAPI_V2 || defined AV_NO_GENERIC_TYPES
/* Removed this type from the V1 API as it does not conform
 * to the C89 standard.
 */
#if defined UNIX
    #if defined _64BIT_
        typedef unsigned long QWORD;
        typedef long  SQWORD;
    #else
        typedef unsigned long long QWORD;
        typedef long long SQWORD;
    #endif

    #define LIT64(x) x##ll
#elif defined WINDOWS
    typedef unsigned __int64 QWORD;
    typedef __int64  SQWORD;

    #define LIT64(x) x##i64
#else
	#error Define QWORD type!
#endif
#endif


#endif

