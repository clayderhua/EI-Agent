/*
 *
 * Copyright (c) 1996-2018 McAfee, LLC. All Rights Reserved.
 * 
 * Functions for removing and finding parameter codes in
 * AV_PARAMETERS structure.
 *
 * 
 */

#include "avparam.h"
#include <string.h>


/* AVFindParameter accepts a parameters structure and searches for the required
 * parameter code. If found the found_param structure is filled in and the index
 * returned. Otherwise zero is returned.
 */
WORD AVFindParameter(AV_PARAMETERS *parameters,
                     AV_PARAMETERCODE parameter_code,
                     AV_SINGLEPARAMETER *found_param,
                     WORD start)
{
    /* Rather inefficient - though presumably the system will never have to cope with
     * too many parameters
     */
    int i;
    for(i = start; i < parameters->nparameters; i++)
    {
        if(parameters->parameters[i].parameter_code == parameter_code)
        {
            /* Parameter found - copy it and return TRUE. */
            if(found_param != NULL)
                memcpy(found_param, &parameters->parameters[i], sizeof(AV_SINGLEPARAMETER));

            /* Return 1 based index (zero reserved for error) */
            return (WORD) (i + 1);
        }
    }

    return 0;
}

/* AVDeleteParameter() removes the 1st occurance of 'avpCode' from
 * the AV_PARAMETERS list 'pAVP'. Returns the number of parameters
 * remaining in pAVP.
 */
WORD AVDeleteParameter( AV_PARAMETERS*   pAVP,
                        AV_PARAMETERCODE avpCode,
                        WORD             wStartFrom
                      )
{
    WORD wIndex0;
    WORD wIndex1;
    
    for( wIndex0 = wStartFrom; wIndex0 < pAVP->nparameters; wIndex0++ )
    {
        if( pAVP->parameters[wIndex0].parameter_code == avpCode )
        {
            pAVP->nparameters--;

            /* move down everything by 1 */
            for( wIndex1 = wIndex0; wIndex1 < pAVP->nparameters; wIndex1++ )
            {
                pAVP->parameters[ wIndex1 ] = pAVP->parameters[ wIndex1 + 1 ];
            }

            /* don't need to loop any more */
            break;
        }
    }

    return( pAVP->nparameters );
}

