/*
 *
 * Copyright (c) 1996-2018 McAfee, LLC. All Rights Reserved.
 * 
 * Function declarations for interface to AV scanning engine.
 *
 * 
 */

#ifndef AVENGINE_H
#define AVENGINE_H

/* Using C linkage if compiled on C++ compiler */
#ifdef __cplusplus
extern "C"
{
#endif
    
#include "avtypes.h"

/* What version of the engine do these headers belong to? */
#define AV_ENGINEVERSION 6010

/* Possible API versions */
#define AV_APIVERSION_V1 1
#define AV_APIVERSION_V2 2

/* What version of the engine API is being used? */
#if defined AVENGINEAPI_V2
#define AV_APIVERSION AV_APIVERSION_V2
#else
#define AV_APIVERSION AV_APIVERSION_V1
#endif

/* The API version is a joining of engine version and api version */
#define AV_APIVER (AV_ENGINEVERSION << 16 | AV_APIVERSION)


#ifdef AV_API_PREFIX
#include "avprefix.h"
#endif


/*
 * The initialise function sets up the engine for subsequent use. The AV_PARAMETERS
 * structure must contain a named call-back function for message retrieval. The
 * structure AV_INITRESULT contains a handle to the engine which must be used
 * in subsequent engine calls.
 */
AV_ERROR EXPLIBFUNC AVInitialise(AV_PARAMETERS *parameters, AV_INITRESULT *init_result);

/*
 * The AVClose function must be called to free up the instance of the engine
 * referred to by hengine.
 */
AV_ERROR EXPLIBFUNC AVClose(HENGINE hengine);

/*
 * Scan a system object. One of AV_PARAMETERS must be an object to scan; current valid
 * specifiers being:
 *
 * ------------------------------------------------------------------------------
 * | Specifier              | Object being scanned  | Attribute required.       |
 * |-----------------------------------------------------------------------------
 * | AVP_MEMORY             | Memory                | None.                     |
 * | AVP_BOOTSECTOR         | Boot sector           | AVP_DOSDRIVELETTER or     |
 * |                        |                       | AVP_BIOSDEVICENUMBER or   |
 * |                        |                       | AVP_USERFILETABLE         |
 * | AVP_PARTITIONSECTOR    | Partition sector      | AVP_BIOSDEVICENUMBER or   |
 * |                        |                       | AVP_USERFILETABLE         |
 * | AVP_DIRECTORY          | OS directory          | AVP_DOSPATH               |
 * | AVP_FILE               | OS file               | AVP_DOSPATH or            |
 * |                        |                       | AVP_USERFILETABLE         |
 * | AVP_PROCESS            | Win32 process memory  | None                      |
 * | AVP_SCANALLPROCESSES   | Win32 process memory  | None                      |
 * ------------------------------------------------------------------------------
 */
AV_ERROR EXPLIBFUNC AVScanObject(HENGINE hengine, AV_PARAMETERS *parameters, AV_SCANRESULT *result);

AV_ERROR EXPLIBFUNC RetrieveExtensionLists( AV_EXTENSIONLIST* exe_exts,
                                            AV_EXTENSIONLIST* arc_exts,
                                            AV_EXTENSIONLIST* compexe_exts,
                                            AV_EXTENSIONLIST* macro_exts );

AV_ERROR EXPLIBFUNC RetrieveSingleExtensionList( AV_EXTENSIONLIST* exts );


/*
 * Retrieve selected information about the given instance from the engine.
 */
AV_ERROR EXPLIBFUNC AVRetrieveInstanceInfo(HENGINE hengine, AV_PARAMETERS *parameters);

#ifdef AV_APIVERSION_V2
/*
 * Update engine DAT files.
 */
AV_ERROR EXPLIBFUNC AVUpdate(AV_UAPPLY *update, AV_PARAMETERS *parameters);
#endif

#if defined (UNIX) && defined (AV_APIVERSION_V2)

/*
 * Functions for engine to handle calling application using fork() when
 * using the engine in both the parent and child processes.
 */

void EXPLIBFUNC AVPreFork(void);
void EXPLIBFUNC AVPostForkParent(void);
void EXPLIBFUNC AVPostForkChild(void);

#endif

#ifdef __cplusplus
}
#endif

#endif

