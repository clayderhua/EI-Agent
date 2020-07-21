/****************************************************************************/
/* Copyright(C) : Advantech Technologies, Inc.								*/
/* Create Date  : 2015/08/18 by Fred Chang									*/
/* Modified Date: 2015/08/18 by Fred Chang									*/
/* Abstract     :  					*/
/* Reference    : None														*/
/****************************************************************************/
#ifndef __getopt_h__
#define __getopt_h__
#ifdef __cplusplus
extern "C" {
#endif
#include "export.h"

	WISEPLATFORM_API int getopt(int argc, char * const argv[],
		const char *optstring);


	WISEPLATFORM_API char *getoptarg();
	WISEPLATFORM_API int getoptind();

#define optarg getoptarg()
#define optind getoptind()
#ifdef __cplusplus
}
#endif
#endif //__getopt_h__





