/****************************************************************************/
/* Copyright(C) : Advantech Technologies, Inc.								*/
/* Create Date  : 2015/12/31 by Scott Chang									*/
/* Modified Date: 2015/12/31 by Scott Chang									*/
/* Abstract     :  															*/
/* Reference    : None														*/
/****************************************************************************/
#ifndef __export_h__
#define __export_h__

#pragma once

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#include <windows.h>
#ifndef WISEPLATFORM_API
	#define WISEPLATFORM_CALL __stdcall
	#define WISEPLATFORM_API __declspec(dllexport)
#endif
#else
	#define WISEPLATFORM_CALL
	#define WISEPLATFORM_API
#endif

#endif //__export_h__