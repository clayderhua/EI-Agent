/****************************************************************************/
/* Copyright(C) : Advantech Technologies, Inc.								*/
/* Create Date  : 2015/08/18 by 											*/
/* Modified Date: 2015/08/18 by 											*/
/* Abstract     :  															*/
/* Reference    : None														*/
/****************************************************************************/
#ifndef __wrapper_h__
#define __wrapper_h__

/*Define for OpenWRT*/
#ifndef PRIu64
# define PRIu64      "llu"
#endif
#ifndef PRId64
#define PRId64      "I64d"
#endif
#ifndef PRIu32
#define PRIu32 "u"
#endif
#ifndef UINT32_MAX
#define UINT32_MAX ((uint32_t)(0xffffffffUL))
#endif
#ifndef UINT64_MAX
#define UINT64_MAX ((uint64_t)(0xffffffffffffffffULL))
#endif


#ifdef __cplusplus
extern "C" {
#endif

#ifdef __cplusplus
}
#endif
#endif //__wrapper_h__