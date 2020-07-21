#ifndef __LP_H__
#define __LP_H__

#define LOG_TAG "LP"
#include "Log.h"

#define MODULE_NAME "LocalProvision"

#define DISCOVER_WAIT_TIME	10000	// msec.
#define MAX_BUFFER_SIZE		3000	// DevID + CredentialURL + IoTKey + TLSType + CAFile + CAPath + CertFile + KeyFile + CertPW
#define LP_AES_KEY			0x86, 0x53, 0x8B, 0x25, 0x46, 0xCF, 0x35, 0x22, 0xE5, 0x65, 0xD8, 0x36, 0x78, 0x81, 0x66, 0xA4
#define LP_AES_IV			0xB5, 0x88, 0x6D, 0x37, 0xF8, 0x77, 0x1F, 0xCC, 0x51, 0x12, 0x05, 0x19, 0xA0, 0x62, 0x79, 0x90
#define LP_MUTICAST_ADDR	"224.0.0.173"
#define LP_MUTICAST_PORT	"9178"
#define LP_MUTICAST_TTL		60
#define LP_TCP_SERVER_PORT	"9177"
#define LP_LOCAL_PIPE_PORT	"9176"

#endif
