#ifndef _NETINFO_PARSER_H_
#define _NETINFO_PARSER_H_

#include "IoTMessageGenerate.h"
#include "NetInfoList.h"
#include <stdbool.h>

#define NET_MON_INFO_LIST                    "netMonInfoList"
#define NET_MON_INFO                         "netMonInfo"
#define NET_INFO_ADAPTER_NAME                "adapterName"
#define NET_ALIAS			                 "alias"
#define NET_INFO_ADAPTER_DESCRI              "adapterDescription"
#define NET_INFO_INDEX                       "index"
#define NET_INFO_TYPE                        "type"
#define NET_INFO_USAGE                       "netUsage"
#define NET_INFO_SEND_BYTE                   "sendDataByte"
#define NET_INFO_SEND_INTERVAL               "sendDataInterval"
#define NET_INFO_SEND_THROUGHPUT             "sendThroughput"
#define NET_INFO_RECV_BYTE                   "recvDataByte"
#define NET_INFO_RECV_INTERVAL               "recvDataInterval"
#define NET_INFO_RECV_THROUGHPUT             "recvThroughput"
#define NET_INFO_STATUS                      "netStatus"
#define NET_INFO_SPEED_MBPS                  "Link SpeedMbps"

#ifdef __cplusplus
extern "C" {
#endif

	void UpdateNetInfo(MSG_CLASSIFY_T* pNetInfoRoot, net_info_list netInfoList, bool bReset);

#ifdef __cplusplus
}
#endif

#endif