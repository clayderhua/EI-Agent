/****************************************************************************/
/* Copyright(C) : Advantech Technologies, Inc.								*/
/* Create Date  : 2017/07/17 by Scott Chang								    */
/* Modified Date: 2017/07/17 by Scott Chang									*/
/* Abstract     : WISE-PaaS basic definition								*/
/* Reference    : None														*/
/****************************************************************************/

#ifndef _WISEPAAS_BASE_DEF_H_
#define _WISEPAAS_BASE_DEF_H_

#define DEF_FILENAME_LENGTH			32
#define DEF_DEVID_LENGTH			37
#define DEF_FW_DESCRIPTION_LENGTH	128
#define DEF_PROCESSOR_NAME_LEN		64

#define DEF_MARK_LENGTH				8
#define DEF_ACCOUNT_LENGTH			32
#define DEF_TYPE_LENGTH				32
#define DEF_HOSTNAME_LENGTH			42
#define DEF_SN_LENGTH				32
#define DEF_MAC_LENGTH				32
#define DEF_LAL_LENGTH				20
#define DEF_VERSION_LENGTH			32
#define DEF_MAX_STRING_LENGTH		128
#define DEF_RUN_MODE_LENGTH			32
#define DEF_ENABLE_LENGTH			8
#define DEF_USER_PASS_LENGTH		512
#define DEF_PORT_LENGTH				8
#define DEF_KVM_MODE_LENGTH			16
#define MAX_TOPIC_LEN               32
#define DEF_OSVERSION_LEN			64
#define DEF_MAX_PATH				260
#define DEF_MAX_CIPHER				4095
#define MAX_SESSION_LEN				33

typedef enum{
	tls_type_unknown = -1,
	tls_type_none = 0,
	tls_type_tls,
	tls_type_psk,
}tls_type;

#endif //_WISEPAAS_BASE_DEF_H_
