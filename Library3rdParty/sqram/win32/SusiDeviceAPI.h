#ifndef _SUSIDEVICE_API_H_
#define _SUSIDEVICE_API_H_

#include "OsDeclarations.h"
#include "Susi4.h"

#ifdef __cplusplus
extern "C" {
#endif

// Condition								| Return Values 
// -----------------------------------------+------------------------------
// Library Uninitialized					| SUSI_STATUS_NOT_INITIALIZED
// pValue==NULL								| SUSI_STATUS_INVALID_PARAMETER
// Unknown ID								| SUSI_STATUS_UNSUPPORTED
// Else										| SUSI_STATUS_SUCCESS
SusiStatus_t SUSI_API SusiDeviceGetValue(SusiId_t Id,		// IN	Value Id
										 uint32_t *pValue);	// OUT	Return Value

// Condition								| Return Values 
// -----------------------------------------+------------------------------
// Library Uninitialized					| SUSI_STATUS_NOT_INITIALIZED
// Unknown ID								| SUSI_STATUS_UNSUPPORTED
// Else										| SUSI_STATUS_SUCCESS
SusiStatus_t SUSI_API SusiDeviceSetValue(SusiId_t Id,		// IN	Value Id
										 uint32_t Value);		// IN	Value

#ifdef __cplusplus
}
#endif

#endif /* _SUSIDEVICE_API_H_ */
