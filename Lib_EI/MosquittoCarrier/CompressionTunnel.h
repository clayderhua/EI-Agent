/****************************************************************************/
/* Copyright(C) : Advantech Technologies, Inc.								*/
/* Create Date  : 2018/12/24 by Fred Chang								    */
/* Modified Date: 2018/12/24 by Fred Chang									*/
/* Abstract     : Compression tunnel                                    	*/
/* Reference    : None														*/
/****************************************************************************/

#ifndef _COMPRESSION_TUNNEL_H_
#define _COMPRESSION_TUNNEL_H_

#ifdef __cplusplus
extern "C" {
#endif

	char *CT_QueueToPayload(void **pq, time_t *time, char *topic, char *payload, int payloadlen, int *resultLen, int second, char *method);

#ifdef __cplusplus
}
#endif
#endif