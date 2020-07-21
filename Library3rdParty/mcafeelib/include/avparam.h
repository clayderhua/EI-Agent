/*
 *
 * Copyright (c) 1996-2018 McAfee, LLC. All Rights Reserved.
 * 
 * Functions for adding, removing and finding parameter codes in
 * AV_PARAMETERS structure.
 *
 * 
 */

#ifndef AVPARAM__H
#define AVPARAM__H


#include "avtypes.h"
#include <stdio.h>


#ifdef __cplusplus
    #define	__DEFAULT_PARAMETER_VALUE(_value_)		= _value_
    extern "C"
    {
#else
    #define	__DEFAULT_PARAMETER_VALUE(_value_)		/* nothing */
#endif


/* AVFindParameter accepts a parameters structure and searches for the required
 * parameter code. If found the found_param structure is filled in and the index
 * returned. Otherwise zero is returned.
 */
WORD AVFindParameter(AV_PARAMETERS *parameters, 
                     AV_PARAMETERCODE parameter_code, 
                     AV_SINGLEPARAMETER *found_param __DEFAULT_PARAMETER_VALUE(NULL), 
                     WORD start __DEFAULT_PARAMETER_VALUE(0)	);


/* AVDeleteParameter() removes the 1st occurance of 'avpCode' from
 * the AV_PARAMETERS list 'pAVP'. Returns the number of parameters
 * remaining in pAVP.
 */
WORD AVDeleteParameter( AV_PARAMETERS*    pAVP,
                        AV_PARAMETERCODE  avpCode,
                        WORD              wStartFrom __DEFAULT_PARAMETER_VALUE((WORD)0)
                      );

/*
 * AVAddBoolParameters is a simple macro which adds a boolean parameter to the
 * parameter list (pl) at position i. c is one of AV_PARAMETERCODE.
 * Note that the index, i is incremented.
 */
#define AVAddBoolParameter(pl, i, c) \
{	pl[i].structure_size = sizeof(AV_SINGLEPARAMETER); \
	pl[i].parameter_code = c; \
	pl[i].attributes_size = 0; \
	pl[i].attributes = NULL; \
	i++; \
}

/*
 * As above, but adds attributes as well.
 */
#define AVAddParameter(pl, i, c, a, s) \
{	pl[i].structure_size = sizeof(AV_SINGLEPARAMETER); \
	pl[i].parameter_code = c; \
	pl[i].attributes_size = s; \
	pl[i].attributes = a; \
	i++; \
}

#ifdef __cplusplus
    }
#endif

#endif

