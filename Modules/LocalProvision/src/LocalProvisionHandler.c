#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <math.h>
#include <inttypes.h>
#include <limits.h>

// module include, socket include must before include <pthread.h>, to avoid windows imcompatible between winsock.h and WinSock2.h
#include "lp.h"
#include "mcast-socket.h"
#include "tcp-socket.h"

// socket include must before include <pthread.h>, to avoid windows imcompatible between winsock.h and WinSock2.h
#include <pthread.h>

// thirdparty include
#include <libxml/xpath.h>
#include <libxml/parser.h>
#include "cJSON.h"
#include "cjson_ipso_util.h"
#include "aes.h"

// wise-agent include
#include "srp/susiaccess_handler_api.h"
#include "srp/susiaccess_def.h"
#include "wise/wisepaas_02_def.h"
#ifdef WIN32
	#include "wrapper.h"
	#include "sys/time.h"
#endif


#define CAP_DISCOVER	"discover"
#define CAP_ACTIVE_ALL	"activeAll"
#define CMD_DISCOVER	"discover"
#define CMD_ACTIVE		"active"
#define INVALID_DEVICE_ID	"NA"

#define APPEND_STR(startPtr, curPtr, str) do {\
	if (curPtr == NULL) { curPtr = startPtr; } \
	curPtr += snprintf(curPtr, MAX_BUFFER_SIZE - (curPtr - startPtr), "%s", str) + 1;\
} while (0)

/*
	read next string after startPtr string
	curPtr must pointer to startPtr or NULL
*/
#define READ_NEXT_STR(startPtr, curPtr, length) do {\
	if (curPtr == NULL) { curPtr = startPtr; } \
	else { \
		curPtr += strlen(curPtr) + 1;\
		if ((curPtr - startPtr) >= length || *curPtr == '\0') { curPtr = NULL; }\
	}\
} while (0)

#define READ_NEXT_STR_BREAK(startPtr, curPtr, length) {\
	READ_NEXT_STR(startPtr, curPtr, length);\
	if (curPtr == NULL) break;\
} while(0)

#define FIND_XML_CONTENT(xmlRoot, item, buffer) do {\
	char* value = (char*) find_xml_content(xmlRoot, item);\
	if (value) {\
		strncpy(buffer, value, sizeof(buffer)-1);\
		xmlFree(value);\
	}\
} while(0)

typedef struct TaskST {
	int action;
	int64_t timeout; // timeout time from 1970
	struct TaskST* next;
} Task;

typedef struct AgentInfoST {
	char deviceID[40];
	char addr[40];
	struct AgentInfoST* next;
} AgentInfo;

// reference the definition in ./Include/srp/susiaccess_def.h
typedef struct {
	char deviceID[40];
	char credentialURL[DEF_MAX_STRING_LENGTH];
	char iotkey[DEF_USER_PASS_LENGTH];
	char tlstype[4];
	char cafile[DEF_MAX_PATH];
	char capath[DEF_MAX_PATH];
	char certfile[DEF_MAX_PATH];
	char keyfile[DEF_MAX_PATH];
	char cerpasswd[DEF_USER_PASS_LENGTH];
} LocalAgentInfo;

// action of Task
enum {
	ACT_STOP_DISCOVER,
	ACT_END_LOOP,
	ACT_SEND_CAPABILITY,
};

static Task* g_taskList = NULL;

static AgentInfo* g_agentInfoList = NULL;
static LocalAgentInfo g_agentInfo;

static SOCKET g_readMcastSocket = INVALID_SOCKET;
static SOCKET g_sendMcastSocket = INVALID_SOCKET;
static SOCKET g_localSocket = INVALID_SOCKET;
static struct addrinfo *g_sendMcastAddr;

static mbedtls_aes_context g_aesSendCtx;
static mbedtls_aes_context g_aesReadCtx;

static unsigned char g_aesKey[16] = { LP_AES_KEY };
static unsigned char g_aesIV[16] = { LP_AES_IV };

static SOCKET g_pipeRead = 0;

// action status
static int g_isDiscovering = 0;
static int g_isRunning = 1;

// handler variable
static Handler_info g_pluginInfo;
static pthread_t g_loopThread;
static fd_set g_rfdsCopy;
static int g_fdmax = 0;

static pthread_mutex_t g_socketMutex;

////////////////////////////////////////////////////////////////////////////////
// Utility function
////////////////////////////////////////////////////////////////////////////////
#ifdef WIN32
double round(double number)
{
    return number < 0.0 ? ceil(number - 0.5) : floor(number + 0.5);
}
#endif


static void dump_hex(char* title, unsigned char* raw, int length)
{
	int i;

	printf("dump_hex(%s):\n", title);
	for (i = 0; i < length; i++) {
		printf("%02X ", raw[i]);
	}
	printf("\n");
}

static int64_t get_current_ms_time(void)
{
    int64_t         ms; // Milliseconds
    time_t          s;  // Seconds
    struct timespec spec;

    clock_gettime(CLOCK_REALTIME, &spec);

    s  = spec.tv_sec;
    ms = (int64_t) round(spec.tv_nsec / 1.0e6); // Convert nanoseconds to milliseconds
    if (ms > 999) {
        s++;
        ms = 0;
    }
	return ((int64_t) s) * 1000 + ms;
}

////////////////////////////////////////////////////////////////////////////////
// Agent info function
////////////////////////////////////////////////////////////////////////////////
static int add_agent_info(AgentInfo** list, AgentInfo* target)
{
	if (*list) {
		target->next = *list;
	}
	*list = target;

	return 0;
}

/*
	return agent info in list or NULL if not found
*/
static AgentInfo* find_agent_info(AgentInfo* list, char* targetDeviceID)
{
	AgentInfo* info = list;

	while (info) {
		if (strcmp(info->deviceID, targetDeviceID) == 0) {
			return info;
		}
		info = info->next;
	}
	return NULL;
}

#ifdef DEBUG
static void dump_agent_info_list(char* title, AgentInfo* list)
{
	AgentInfo* info = list;
	int idx = 0;

	LOGD("dump_agent_info_list(%s)", title);
	while (info) {
		LOGD("%d:", idx++);
		LOGD("\tdeviceID=%s", info->deviceID);
		info = info->next;
	}
}
#endif

static void free_agent_info_list(AgentInfo** list)
{
	AgentInfo* info2free = NULL;
	AgentInfo* info = *list;

	while (info) {
		info2free = info;
		info = info2free->next;
		free(info2free);
	}
	*list = NULL;
}


////////////////////////////////////////////////////////////////////////////////
// Schedule task function
////////////////////////////////////////////////////////////////////////////////
static int send_dummy_message()
{
	int ret;
	unsigned char dummy = 0;
	SOCKET sock = init_client_tcp_socket("127.0.0.1", LP_LOCAL_PIPE_PORT);
	if (sock == -1) {
		LOGE("send_dummy_message: init_client_tcp_socket(dummy) fail");
		return -1;
	}
	ret = send(sock, &dummy, 1, 0);
	close(sock);

	return (ret == 1)? 0: -1;
}

static int read_dummy_message(SOCKET pipeRead)
{
	SOCKET sock;
	unsigned char dummy;
	int bytes = -1;

	while (1) {
		sock = accept(pipeRead, NULL, 0);
		if (sock != INVALID_SOCKET || errno != EAGAIN) {
			break;
		}
		usleep(100);
	}
	SOCKET_NOTVALID_GOTO_ERROR(sock, "dummy accept fail");

	bytes = recv(sock, &dummy, 1, 0);
	SOCKET_RESULT_GOTO_ERROR(bytes, "recv(dummy) failed");
	if(bytes != 1) {
		LOGE("dummy read fail, errno=%d", errno);
	}

	return bytes;

error:
	return -1;
}


static Task* remove_timeout_task(Task** list, int64_t timeoutTime)
{
	Task* task = *list;
	Task* prev = NULL;

	while (task) {
		if (task->timeout <= timeoutTime) { // timeout
			if (prev) {
				prev->next = task->next;
			} else {
				*list = task->next;
			}
			task->next = NULL;
			return task;
		}
		prev = task;
		task = task->next;
	}
	return NULL;
}

/*
	return minum task timeout time, if not found, return 36000
*/
static void min_task_timeout_time(Task* list, struct timeval* tv)
{
	int64_t current = get_current_ms_time();
	int64_t timeout = 36000 + current;
	Task* task = list;

	if (!task) {
		tv->tv_sec = 36000;
		tv->tv_usec = 0;
		return;
	}

	while (task) {
		if (timeout > task->timeout) {
			timeout = task->timeout;
		}
		task = task->next;
	}
	tv->tv_sec = 0;
	if ((timeout - current) <= 0) {
		tv->tv_usec = 0;
	} else {
		tv->tv_usec = (long) (timeout - current) * 1000;
	}

}

/*
	Add a new task to task list
*/
static int add_task(Task** list, int action, int64_t timeout)
{
	Task* task = (Task*) calloc(1, sizeof(Task));
	if (!task) {
		return -1;
	}

	// init task
	task->timeout = timeout;
	task->action = action;

	// add to list
	if (*list) {
		task->next = *list;
	}
	*list = task;

	// trigger interrupt to re-calculate timeout time
	send_dummy_message();

	return 0;
}

////////////////////////////////////////////////////////////////////////////////
// Handler Utility Function
////////////////////////////////////////////////////////////////////////////////
/*
	Find content of xml element
	return NULL to continue, others stop
*/
static xmlChar* find_xml_content(xmlNode *node, char* target)
{
   xmlNode *curNode = NULL;
   xmlChar *content = NULL;

   for (curNode = node; curNode; curNode = curNode->next) {
		if (curNode->type == XML_ELEMENT_NODE) {
			if (strcmp((char*) curNode->name, target) == 0) {
				content = xmlNodeGetContent(curNode);
				return content;
			}
		}
		content = find_xml_content(curNode->children, target);
		if (content) {
			return content;
		}
   }

   return NULL;
}

/*
	Update content of xml element
	return 0 to continue, other stop
*/
static int update_xml_content(xmlNode *node, char* target, char* content)
{
   xmlNode *curNode = NULL;
   int rc = 0;

   for (curNode = node; curNode; curNode = curNode->next) {
		if (curNode->type == XML_ELEMENT_NODE) {
			if (strcmp((char*) curNode->name, target) == 0) {
				//content = xmlNodeGetContent(curNode);
				xmlNodeSetContent(curNode, (xmlChar*) content);
				return 1;
			}
		}
		rc = update_xml_content(curNode->children, target, content);
		if (rc) {
			return 1;
		}
   }

   return 0;
}

/*
	get agent info from xml file
*/
static int get_agent_info()
{
	char configFile[PATH_MAX];
	xmlDocPtr doc = NULL;
	xmlNode *root = NULL;
	//char* value = NULL;

	memset(&g_agentInfo, 0, sizeof(g_agentInfo));

	sprintf(configFile, "%s/agent_config.xml", g_pluginInfo.WorkDir);
	doc = xmlReadFile(configFile, "UTF-8", 0);
	if (!doc) {
		LOGE("get_agent_info: xmlReadFile fail");
		return -1;
	}

	root = xmlDocGetRootElement(doc);

	FIND_XML_CONTENT(root, "DevID", g_agentInfo.deviceID);
	FIND_XML_CONTENT(root, "CredentialURL", g_agentInfo.credentialURL);
	FIND_XML_CONTENT(root, "IoTKey", g_agentInfo.iotkey);
	FIND_XML_CONTENT(root, "TLSType", g_agentInfo.tlstype);
	FIND_XML_CONTENT(root, "CAFile", g_agentInfo.cafile);
	FIND_XML_CONTENT(root, "CAPath", g_agentInfo.capath);
	FIND_XML_CONTENT(root, "CertFile", g_agentInfo.certfile);
	FIND_XML_CONTENT(root, "KeyFile", g_agentInfo.keyfile);
	FIND_XML_CONTENT(root, "CertPW", g_agentInfo.cerpasswd);

	xmlFreeDoc(doc);       // free document
	//xmlCleanupParser();    // Free globals

	return 0;
}

/*
{"LocalProvision":{"DeviceList":{"bn":"DeviceList","e":[
{"n":"counter","v":%d,"asm":"r"},
{"n":"time","v":%d,"asm":"r"}]}}}
*/
static char* get_capability_string()
{
	cJSON *root = NULL;
	cJSON *lp1, *lp2;
	cJSON *e, *item;
	cJSON *information;
	AgentInfo* info = g_agentInfoList;
	char* capStr = NULL;

	cJSON_NewItem(root);
	cJSON_AddObjectToObject(root, MODULE_NAME, lp1);
	cJSON_AddObjectToObject(lp1, "Information", information);
	cJSON_AddStringToObject(information, "bn", "Information");
	cJSON_AddBoolToObject(information, "nonSensorData", 1);
	cJSON_AddObjectToObject(lp1, "DeviceList", lp2);
	cJSON_AddStringToObject(lp2, "bn", "DeviceList");

	e = cJSON_CreateArray();
	if (!e) {
		goto error;
	}
	cJSON_AddItemToObject(lp2, "e", e);

	// add discovery
	cJSON_IPSO_NewRWNumberItem(item, CAP_DISCOVER, 0);
	cJSON_AddItemToArray(e, item);

	// add activeAll
	cJSON_IPSO_NewRWNumberItem(item, CAP_ACTIVE_ALL, 0);
	cJSON_AddItemToArray(e, item);

	// device list
	while (info) {
		cJSON_IPSO_NewRWNumberItem(item, info->deviceID, 0);
		cJSON_AddItemToArray(e, item);
		info = info->next;
	}

	capStr = cJSON_PrintUnformatted(root);

error:
	if (root) {
		cJSON_Delete(root);
	}

	return capStr;
}


////////////////////////////////////////////////////////////////////////////////
// Local Provision Protocol Function
////////////////////////////////////////////////////////////////////////////////
#ifdef DEBUG
static void dump_all_message(char* title, char* message, int length)
{
	char* ptr = message;

	LOGD("dump_all_message(%s)", title);

	while (ptr) {
		LOGD("\t[%s]", ptr);
		READ_NEXT_STR(message, ptr, length);
		if (ptr == NULL) {
			break;
		}
	}
}
#endif

static int init_socket(char *multicastIP, char *multicastPort)
{
	// init socket
	pthread_mutex_lock(&g_socketMutex);
	if (g_readMcastSocket == INVALID_SOCKET && strcmp(g_pluginInfo.ServerIP, "0.0.0.0") == 0) { // if no server ip specified
		g_readMcastSocket = init_recv_mcast_sockt(multicastIP, multicastPort);
		if (g_readMcastSocket == INVALID_SOCKET) {
			LOGE("init_recv_mcast_sockt fail 1");
			return -1;
		}
	}
	pthread_mutex_unlock(&g_socketMutex);

	g_sendMcastSocket = init_send_mcast_socket(multicastIP, multicastPort, &g_sendMcastAddr);
	if(g_sendMcastSocket == INVALID_SOCKET) {
		LOGE("init_send_mcast_socket fail");
		return -1;
	}

	g_localSocket = init_server_tcp_socket(NULL, LP_TCP_SERVER_PORT);
	if (g_localSocket == INVALID_SOCKET) {
		LOGE("init_server_tcp_socket(g_localSocket) fail");
	}

	// init a local socket to replace pipe() function to compatible with windows
	g_pipeRead = init_server_tcp_socket("127.0.0.1", LP_LOCAL_PIPE_PORT);
	if (g_pipeRead == INVALID_SOCKET) {
		LOGE("init_server_tcp_socket(g_pipeRead) fail");
	}

	// init aes, we must separator ctx between encode & decode, it is a limit of aes library
	init_aes_encode(&g_aesSendCtx, g_aesKey, 128);
	init_aes_decode(&g_aesReadCtx, g_aesKey, 128);

	return 0;
}


/*
	Read and decrypt aes multicast message
	length:
		the length of message buffer
	return the bytes number of message.
	return -1, read failed
*/
static int read_aes_mcast_message(SOCKET sock, unsigned char* message, int length, char* addr)
{
	unsigned char cipher[MAX_BUFFER_SIZE];
	int readLength;

	// read data
	readLength = read_mcast_message_ex(sock, cipher, sizeof(cipher), addr);
	if (readLength < 0) {
		LOGE("read_mcast_message_ex(cipher) fail");
		return -1;
	}
	if (readLength > length) {
		LOGE("read_aes_tcp_message: buffer is not enough, %d, %d", readLength, length);
		return -1;
	}

	return aes_decode(&g_aesReadCtx, g_aesIV, cipher, message, readLength);
}


/*
	send multicast message with aes encrypt
*/
static int send_aes_mcast_message(SOCKET sock, const unsigned char* message, int length)
{
	unsigned char cipher[MAX_BUFFER_SIZE];
	int32_t aesMsgLength;

	if (length > sizeof(cipher)) {
		LOGE("send_aes_mcast_message: cipher buffer is not enough, %d, %ld", length, sizeof(cipher));
		return -1;
	}

	// encode message
	memset(cipher, 0, sizeof(cipher));
	aesMsgLength = (int32_t) aes_encode(&g_aesSendCtx, g_aesIV, message, length, cipher, sizeof(cipher));
	if (aesMsgLength == -1) {
		LOGE("send_aes_mcast_message: aes encode fail");
		return -1;
	}

	// send data
	return send_mcast_message_ex(sock, g_sendMcastAddr, cipher, aesMsgLength);
}


/*
	Read and decrypt aes tcp message

	return the bytes number of message.
	return -1, read failed
*/
static int read_aes_tcp_message(SOCKET localSocket, unsigned char* message, int length, char* addr)
{
	SOCKET sock;
	int readLength;
	unsigned char cipher[MAX_BUFFER_SIZE];
	struct sockaddr clientAddr;
	socklen_t addrlen = sizeof(struct sockaddr);
	struct sockaddr_in* addr4;
	struct sockaddr_in6* addr6;

	while (1) {
		sock = accept(localSocket, (struct sockaddr *) &clientAddr, &addrlen);
		if (sock != INVALID_SOCKET || errno != EAGAIN) {
			break;
		}
		usleep(100);
	}
	SOCKET_NOTVALID_GOTO_ERROR(sock, "accept fail");

	// resolv addr
	if (addr) {
		if (addrlen == sizeof(struct sockaddr_in)) {
			addr4 = (struct sockaddr_in*) &clientAddr;
			inet_ntop(AF_INET, &(addr4->sin_addr), addr, INET_ADDRSTRLEN);
		} else if (addrlen == sizeof(struct sockaddr_in6)) {
			addr6 = (struct sockaddr_in6*) &clientAddr;
			inet_ntop(AF_INET, &(addr6->sin6_addr), addr, INET6_ADDRSTRLEN);
		}
	}

	// read data
	readLength = read_tcp_message_ex(sock, cipher, sizeof(cipher));
	close(sock);
	if (readLength < 0) {
		LOGE("read_tcp_message_ex(cipher) fail");
		return -1;
	}
	if (readLength > length) {
		LOGE("read_aes_tcp_message: buffer is not enough, %d, %d", readLength, length);
		return -1;
	}

	// decode data
	return aes_decode(&g_aesReadCtx, g_aesIV, cipher, message, (int) readLength);

error:
	return -1;
}

static int send_aes_tcp_message(SOCKET sock, const unsigned char* message, int length)
{
	unsigned char cipher[MAX_BUFFER_SIZE];
	int32_t aesMsgLength;

	if (length > sizeof(cipher)) {
		LOGE("send_aes_tcp_message: cipher buffer is not enough, %d, %ld", length, sizeof(cipher));
		return -1;
	}

	// encode message
	memset(cipher, 0, sizeof(cipher));
	aesMsgLength = (int32_t) aes_encode(&g_aesSendCtx, g_aesIV, message, length, cipher, sizeof(cipher));
	if (aesMsgLength == -1) {
		LOGE("send_aes_tcp_message: aes_encode fail");
		return -1;
	}

	return send_tcp_message_ex(sock, cipher, (int32_t) aesMsgLength);
}

static int send_aes_tcp_message_ex(const char* serverIP, const char* serverPort, const unsigned char* message, int length)
{
	SOCKET sock;
	int ret;

	sock = init_client_tcp_socket(serverIP, serverPort);
	if (sock == INVALID_SOCKET) {
		LOGE("send_aes_tcp_message_ex: init_client_tcp_socket(%s:%s) fail", serverIP, serverPort);
		return -1;
	}

	ret = send_aes_tcp_message(sock, message, length);
	close(sock);

	return ret;
}

/*
	send CMD_DISCOVER request to multicast network
*/
static int send_discover()
{
	char buffer[MAX_BUFFER_SIZE];
	char* ptr = NULL;
	int total = 0;

	LOGD("send_discover");
	free_agent_info_list(&g_agentInfoList);

	// add task to wait discover-ack packet
	g_isDiscovering = 1;
	add_task(&g_taskList, ACT_STOP_DISCOVER, get_current_ms_time()+DISCOVER_WAIT_TIME);

	APPEND_STR(buffer, ptr, CMD_DISCOVER);

	total = ptr - buffer;

	return send_aes_mcast_message(g_sendMcastSocket, (unsigned char*) buffer, total);
}

/*
	send "active" request to specified device with tcp network
*/
/* disable plugin function
static int send_active(char* deviceID)
{
	char buffer[MAX_BUFFER_SIZE];
	char* ptr = NULL;
	int total = 0;
	AgentInfo *info;
	int ret = -1;

	LOGD("send_active(%s)", deviceID);

	APPEND_STR(buffer, ptr, CMD_ACTIVE);
	APPEND_STR(buffer, ptr, deviceID);
	APPEND_STR(buffer, ptr, g_agentInfo.credentialURL);
	APPEND_STR(buffer, ptr, g_agentInfo.iotkey);
	APPEND_STR(buffer, ptr, g_agentInfo.tlstype);
	APPEND_STR(buffer, ptr, g_agentInfo.cafile);
	APPEND_STR(buffer, ptr, g_agentInfo.capath);
	APPEND_STR(buffer, ptr, g_agentInfo.certfile);
	APPEND_STR(buffer, ptr, g_agentInfo.keyfile);
	APPEND_STR(buffer, ptr, g_agentInfo.cerpasswd);

	total = ptr - buffer;

	if (strcmp(deviceID, INVALID_DEVICE_ID)==0) { // send by multicast
		ret = send_aes_mcast_message(g_sendMcastSocket, (unsigned char*) buffer, total);
	} else {
		info = find_agent_info(g_agentInfoList, deviceID);
		if (info) { // send by p2p
			ret = send_aes_tcp_message_ex(info->addr, LP_TCP_SERVER_PORT, (unsigned char*) buffer, total);
		} else {
			LOGE("send_active: deviceID=%s not found", deviceID);
		}
	}
	return ret;
}
*/

/*
	send discover-ack to sender
	format:
		"discover-ack"
		device id
*/
static int recv_discoer(char* message, int length, char* addr)
{
	char buffer[MAX_BUFFER_SIZE];
	char* ptr = NULL;
	int total = 0;

	LOGD("recv_discoer");
	APPEND_STR(buffer, ptr, "discover-ack");
	APPEND_STR(buffer, ptr, g_agentInfo.deviceID);
	total = ptr - buffer;

	return send_aes_tcp_message_ex(addr, LP_TCP_SERVER_PORT, (unsigned char*) buffer, total);
}

// read next string from curPtr, curPtr pointer to NULL if not found
static int recv_discoer_ack(char* message, int length, char* addr)
{
	AgentInfo *info, *newInfo;
	char* ptr = NULL;

	if (!g_isDiscovering) {
		LOGD("recv_discoer_ack: not discovering, return");
		return 0;
	}
	LOGD("recv_discoer_ack");

	// function
	READ_NEXT_STR(message, ptr, length);
	// device id
	READ_NEXT_STR(message, ptr, length);

	// skip self packet
	if (strcmp(ptr, g_agentInfo.deviceID) == 0) {
		LOGD("recv_discoer_ack: skip %s", ptr);
		return 0;
	}

	newInfo = (AgentInfo*) calloc(1, sizeof(AgentInfo));
	if (!newInfo) {
		return -1;
	}
	strncpy(newInfo->deviceID, ptr, sizeof(newInfo->deviceID)-1);
	strncpy(newInfo->addr, addr, sizeof(newInfo->addr)-1);

	// add to list if not exist
	info = find_agent_info(g_agentInfoList, newInfo->deviceID);
	if (info == NULL) {
		add_agent_info(&g_agentInfoList, newInfo);
	} else { // found, copy content
		strncpy(info->deviceID, newInfo->deviceID, sizeof(info->deviceID)-1);
	}

#ifdef DEBUG
	dump_agent_info_list("recv_discoer_ack", g_agentInfoList);
#endif

	return 0;
}

static int recv_active(char* message, int length)
{
	char configFile[PATH_MAX];
	xmlDocPtr doc = NULL;
	xmlNode *root = NULL;
	char *credentialURL, *iotkey, *deviceID;
	char *ptr;

	// read messge
	ptr = message;
	READ_NEXT_STR(message, ptr, length);
	deviceID = ptr;
	READ_NEXT_STR(message, ptr, length);
	credentialURL = ptr;
	READ_NEXT_STR(message, ptr, length);
	iotkey = ptr;

	if (strcmp(deviceID, INVALID_DEVICE_ID) != 0 && // == INVALID_DEVICE_ID is active for all devices
	    strcmp(deviceID, g_agentInfo.deviceID) != 0)
	{
		LOGD("recv_active: skip [%s]", deviceID);
		return 0;
	}

	LOGD("recv_active: deviceID=[%s], url=[%s], iotkey=[%s]", deviceID, credentialURL, iotkey);
	if (!deviceID || !credentialURL || !iotkey) {
		LOGE("recv_active: Invalid deviceID, url, or iotkey!");
		return -1;
	}

	// update agent_config.xml
	sprintf(configFile, "%s/agent_config.xml", g_pluginInfo.WorkDir);
	doc = xmlReadFile(configFile, "UTF-8", 0);
	if (!doc) {
		LOGE("recv_active: xmlReadFile fail");
		return -1;
	}
	root = xmlDocGetRootElement(doc);
	update_xml_content(root, "CredentialURL", credentialURL);
	update_xml_content(root, "IoTKey", iotkey);

	do {
		READ_NEXT_STR_BREAK(message, ptr, length);
		update_xml_content(root, "TLSType", ptr);
		READ_NEXT_STR_BREAK(message, ptr, length);
		update_xml_content(root, "CAFile", ptr);
		READ_NEXT_STR_BREAK(message, ptr, length);
		update_xml_content(root, "CAPath", ptr);
		READ_NEXT_STR_BREAK(message, ptr, length);
		update_xml_content(root, "CertFile", ptr);
		READ_NEXT_STR_BREAK(message, ptr, length);
		update_xml_content(root, "KeyFile", ptr);
		READ_NEXT_STR_BREAK(message, ptr, length);
		update_xml_content(root, "CertPW", ptr);
	} while (0);

	xmlSaveFormatFileEnc(configFile, doc, "UTF-8", 1);

	xmlFreeDoc(doc);       // free document
	//xmlCleanupParser();    // Free globals

	return 0;
}

static int do_socket(fd_set* rfds)
{
	char message[MAX_BUFFER_SIZE];
	int length = 0;
	char addr[40];

	if (g_readMcastSocket != INVALID_SOCKET && FD_ISSET(g_readMcastSocket, rfds)) {
		length = read_aes_mcast_message(g_readMcastSocket, (unsigned char*) message, sizeof(message), addr);
	} else if (g_localSocket != INVALID_SOCKET && FD_ISSET(g_localSocket, rfds)) {
		length = read_aes_tcp_message(g_localSocket, (unsigned char*) message, sizeof(message), addr);
	} else if (g_pipeRead != INVALID_SOCKET && FD_ISSET(g_pipeRead, rfds)) { // interrupt, to stop
		length = read_dummy_message(g_pipeRead);
		return 0; // receive a interrupt event
	} else { // unknown fd input
		return 0;
	}

	if (length <= 0) { // decrypt fail
		return -1;
	}

	// check function
	if (strcmp(message, "send-discover") == 0) {
		send_discover();
	} else if (strcmp(message, CMD_DISCOVER) == 0) {
		recv_discoer(message, length, addr);
	} else if (strcmp(message, "discover-ack") == 0) {
		recv_discoer_ack(message, length, addr);
	} else  if (strcmp(message, CMD_ACTIVE) == 0) {
		recv_active(message, length);
	} else {
		if (message[length-1] == '\0') { // we can print it as string without crush risk
			LOGD("unknown function [%s]", message);
#ifdef DEBUG
			dump_all_message("unknown function", message, length);
#endif
		} else {
			dump_hex("unknown function", (unsigned char*) message, length);
		}
	}

	return 0;
}

static void send_capability()
{
	char* capStr = get_capability_string();

	g_pluginInfo.sendcapabilitycbf(&g_pluginInfo, capStr, strlen(capStr)+1, NULL, NULL);
	g_pluginInfo.sendreportcbf(&g_pluginInfo, capStr, strlen(capStr)+1, NULL, NULL);
	free(capStr);
}

static void do_task()
{
	int64_t timeoutTime = get_current_ms_time();
	Task* task;

	task = remove_timeout_task(&g_taskList, timeoutTime);

	while (task) {
		LOGD("do_task, task=%d", task->action);

		switch (task->action) {
		case ACT_STOP_DISCOVER: {
			g_isDiscovering = 0;
			send_capability();
			break;
		}
		case ACT_END_LOOP:
			g_isRunning = 0;
			send_dummy_message();
		}
		free(task); // free, since we have done the task
		task = remove_timeout_task(&g_taskList, timeoutTime);
	}
}

static void* loop_thread(void *param)
{
	fd_set rfds;
	struct timeval tv;
	int ret;

	// init socket and aes
	if (init_socket(LP_MUTICAST_ADDR, LP_MUTICAST_PORT) != 0) {
		return NULL;
	}
	// init select
	FD_ZERO(&rfds);
	FD_SET(g_localSocket, &rfds);
	if ((int)g_localSocket > g_fdmax) {
		g_fdmax = g_localSocket;
	}
	FD_SET(g_pipeRead, &rfds);
	if ((int)g_pipeRead > g_fdmax) {
		g_fdmax = g_pipeRead;
	}

	pthread_mutex_lock(&g_socketMutex);
	if (g_readMcastSocket != INVALID_SOCKET) { // if no server ip specified
		FD_SET(g_readMcastSocket, &rfds);
		if ((int)g_readMcastSocket > g_fdmax) {
			g_fdmax = g_readMcastSocket;
		}
	}
	pthread_mutex_unlock(&g_socketMutex);

	g_rfdsCopy = rfds;
	tv.tv_sec = 3600;
	tv.tv_usec = 0;

	while (g_isRunning) {
		LOGD("loop waiting, tv_sec=%ld, tv_usec=%ld..............................", tv.tv_sec, tv.tv_usec);

		ret = select(g_fdmax+1, &rfds, NULL, NULL, &tv);
		if (ret == -1) {
			LOGE("select failed: errno=%d", errno);
			break;
		} else if (ret == 0) { // timeout, check task list
			do_task();
		} else { // socket event
			do_socket(&rfds);
		}

		min_task_timeout_time(g_taskList, &tv);
		rfds = g_rfdsCopy;
	}

	free_agent_info_list(&g_agentInfoList);
	if (g_readMcastSocket != INVALID_SOCKET)
		close(g_readMcastSocket);
	if (g_sendMcastSocket != INVALID_SOCKET)
		close(g_sendMcastSocket);
	if (g_pipeRead != INVALID_SOCKET)
		close(g_pipeRead);
	if (g_localSocket != INVALID_SOCKET)
		close(g_localSocket);
	freeaddrinfo(g_sendMcastAddr);

	return NULL;
}

////////////////////////////////////////////////////////////////////////////////
// Handler API
////////////////////////////////////////////////////////////////////////////////
int HANDLER_API Handler_Initialize( HANDLER_INFO *pluginfo )
{
	LOGD("%s call", __FUNCTION__);

	pthread_mutex_init(&g_socketMutex, NULL);

	// copy handler name
	strcpy(pluginfo->Name, MODULE_NAME);

	// save info
	g_pluginInfo = *pluginfo;
	get_agent_info();
	// clear global rfds
	FD_ZERO(&g_rfdsCopy);

	return handler_success;
}

void Handler_Uninitialize()
{
	LOGD("%s call", __FUNCTION__);

	pthread_mutex_destroy(&g_socketMutex);
}

int HANDLER_API Handler_Get_Status( HANDLER_THREAD_STATUS * pOutStatus )
{
	LOGD("%s call", __FUNCTION__);
	return handler_success;
}

void HANDLER_API Handler_OnStatusChange( HANDLER_INFO *pluginfo )
{
	LOGD("%s call", __FUNCTION__);

	g_pluginInfo = *pluginfo;
	if (pluginfo->agentInfo->status == AGENT_STATUS_ONLINE)
	{
		pthread_mutex_lock(&g_socketMutex);
		if (g_readMcastSocket != INVALID_SOCKET) {
			LOGD("close mcast listening socket");
			// clear from FD_SET
			FD_CLR(g_readMcastSocket, &g_rfdsCopy);
			// close mcast read socket
			close(g_readMcastSocket);
			g_readMcastSocket = INVALID_SOCKET;
			// update event thread
			send_dummy_message();
		}
		pthread_mutex_unlock(&g_socketMutex);
		// resync agent info
		get_agent_info();
		// change capability
		// disable plugin function
		//send_capability();
	}
	else
	{
		pthread_mutex_lock(&g_socketMutex);
		if (g_readMcastSocket == INVALID_SOCKET) {
			LOGD("init mcast listening socket");
			// init mcast read socket
			g_readMcastSocket = init_recv_mcast_sockt(LP_MUTICAST_ADDR, LP_MUTICAST_PORT);
			if (g_readMcastSocket == INVALID_SOCKET) {
				LOGE("init_recv_mcast_sockt fail 2");
				return;
			}
			FD_SET(g_readMcastSocket, &g_rfdsCopy);
			if ((int)g_readMcastSocket > g_fdmax) {
				g_fdmax = g_readMcastSocket;
			}
			// update event thread
			send_dummy_message();
		}
		pthread_mutex_unlock(&g_socketMutex);
	}
}

int HANDLER_API Handler_Start( void )
{
	LOGD("%s call", __FUNCTION__);
	pthread_create(&g_loopThread, NULL, loop_thread, NULL);

	return handler_success;
}

int HANDLER_API Handler_Stop( void )
{
	LOGD("%s call", __FUNCTION__);

	add_task(&g_taskList, ACT_END_LOOP, get_current_ms_time());
	pthread_join(g_loopThread, NULL);

	return handler_success;
}

#define cJSON_GetObjectItemError(parent, name, item) do {\
	item = cJSON_GetObjectItem(parent, name);\
	if (!item) goto error;\
} while (0)

/*
	response status 200 directly

REQUEST
{
   "susiCommData":{
      "sensorIDList":{
         "e":[
            {
               "n":"LocalProvision/DeviceList/00000001-0000-0000-0000-000bab8cb228",
			   "v":0,
			   "StatusCode":200
            }
         ]
      },
      "handlerName":"LocalProvision",
      "commCmd":523,
      "sessionID":"1DCFCEBD64AD66DE3612B59C6631013A"
   }
}
RESPONSE
{
  "sessionID":"1DCFCEBD64AD66DE3612B59C6631013A",
  "sensorInfoList":{
	 "e":[
		{
		   "n":"DataSync/RecoverProgress/current",
		   "v":0,
		   "StatusCode":200
		}
	 ]
  }
}
*/
/* disable plugin function
static int do_get(cJSON *root)
{
	cJSON *susiCommData, *item, *sessionID, *sensorIDList, *e, *n;
	char *ptr, *sessionString = NULL, *response = NULL;
	int arraySize = 0, i;
	cJSON *respRoot = NULL, *respE, *sensorInfoList;
	int ret = -1;

	LOGD("do_get");

	// get "sessionID", "e" array from request "root"
	cJSON_GetObjectItemError(root, "susiCommData", susiCommData);
	// sessionID
	cJSON_GetObjectItemError(susiCommData, "sessionID", sessionID);
	sessionString = (char*) malloc(strlen(sessionID->valuestring)+1);
	strcpy(sessionString, sessionID->valuestring);
	// e array
	cJSON_GetObjectItemError(susiCommData, "sensorIDList", sensorIDList);
	cJSON_GetObjectItemError(sensorIDList, "e", e);
	// copy e array item to response respE array
	respE = cJSON_Duplicate(e , 1);
	if (!respE)
		goto error;

	// prepare response
	respRoot = cJSON_CreateObject();
	// fill sessionID
	cJSON_AddStringToObject(respRoot, "sessionID", sessionString);
	cJSON_AddObjectToObject(respRoot, "sensorInfoList", sensorInfoList);
	cJSON_AddItemToObject(sensorInfoList, "e", respE);
	arraySize = cJSON_GetArraySize(respE);
	for (i = 0; i < arraySize; i++) {
		item = cJSON_GetArrayItem(respE, i);
		cJSON_AddNumberToObject(item, "StatusCode", 200); // response 200 as default
		// get "n"
		cJSON_GetObjectItemError(item, "n", n);
		ptr = strstr(n->valuestring, "/" CAP_DISCOVER);
		if (ptr && *(ptr + strlen("/" CAP_DISCOVER)) == '\0') { // if "discover" command
			cJSON_AddNumberToObject(item, "v", g_isDiscovering);
		} else {
			cJSON_AddNumberToObject(item, "v", 0); // response 200 as default
		}
	}

	// send response
	response = cJSON_PrintUnformatted(respRoot);
	g_pluginInfo.sendcbf(&g_pluginInfo, wise_get_sensor_data_rep, response, strlen(response)+1, NULL, NULL);

	ret = 0;

error:
	if (ret)
		LOGE("error in do_get");

	if (sessionString) free(sessionString);
	if (response) free(response);
	if (respRoot) cJSON_Delete(respRoot);

	return ret;
}
*/

/*
{
   "susiCommData":{
      "sensorIDList":{
         "e":[
            {
               "v":1,
               "n":"LocalProvision/DeviceList/discover"
            }
         ]
      },
      "sessionID":"1DCFCEBD64AD66DE3612B59C6631013A",
      "commCmd":525,
      "agentID":"00000001-0000-0000-0000-000bab8cbd56",
      "handlerName":"LocalProvision",
      "sendTS":{
         "$date":1553676369901
      }
   }
}
*/
/* disable plugin function
static int do_set(cJSON *root)
{
	cJSON *susiCommData, *item, *sessionID, *sensorIDList, *e, *n, *v;
	char *nstring = NULL, *device, *saveptr, *sessionString = NULL, *response = NULL;
	int arraySize = 0, i;
	cJSON *respRoot = NULL, *respE, *sensorInfoList;
	int ret = -1;

	LOGD("do_set");

	// get "sessionID", "e" array from request "root"
	cJSON_GetObjectItemError(root, "susiCommData", susiCommData);
	// sessionID
	cJSON_GetObjectItemError(susiCommData, "sessionID", sessionID);
	sessionString = (char*) malloc(strlen(sessionID->valuestring)+1);
	strcpy(sessionString, sessionID->valuestring);
	// e array
	cJSON_GetObjectItemError(susiCommData, "sensorIDList", sensorIDList);
	cJSON_GetObjectItemError(sensorIDList, "e", e);
	// copy e array item to response respE array
	respE = cJSON_Duplicate(e , 1);
	if (!respE)
		goto error;

	// prepare response
	respRoot = cJSON_CreateObject();
	// fill sessionID
	cJSON_AddStringToObject(respRoot, "sessionID", sessionString);
	cJSON_AddObjectToObject(respRoot, "sensorInfoList", sensorInfoList);
	// add e array
	cJSON_AddItemToObject(sensorInfoList, "e", respE);
	arraySize = cJSON_GetArraySize(respE);
	for (i = 0; i < arraySize; i++) {
		item = cJSON_GetArrayItem(respE, i);

		// get "n"
		n = cJSON_GetObjectItem(item, "n");
		if (!n) {
			continue;
		}
		v = cJSON_GetObjectItem(item, "v");
		if (!v) {
			continue;
		}

		// find device name
		nstring = (char*) calloc(1, strlen(n->valuestring)+1);
		strcpy(nstring, n->valuestring);
		strtok_r(nstring, "/", &saveptr);
		strtok_r(NULL, "/", &saveptr);
		device = strtok_r(NULL, "/", &saveptr);
		if (!device) {
			free(nstring);
			continue;
		}

		if (strcmp(device, CAP_DISCOVER) == 0) // if "discover" command
		{
			if (!g_isDiscovering) { // if device is not in discovering proccess
				// send discovery packet
				if (v->valueint == 1) { // check setting value
					ret = send_discover();
				} else {
					ret = -1;
				}

				if (ret) { // fail
					cJSON_AddNumberToObject(item, "StatusCode", 400);
				} else { // ok
					cJSON_AddNumberToObject(item, "StatusCode", 200);
				}
			}
			v->valueint = g_isDiscovering;
			v->valuedouble = g_isDiscovering;
		}
		else // real device(s)
		{
			if (strcmp(device, CAP_ACTIVE_ALL) == 0) { // if "activeAll" command
				device = INVALID_DEVICE_ID;
			}
			if (v->valueint == 1) { // check setting value
				ret = send_active(device);
			} else {
				ret = -1;
			}

			if (ret) { // fail
				cJSON_AddNumberToObject(item, "StatusCode", 400);
			} else { // ok
				cJSON_AddNumberToObject(item, "StatusCode", 200);
			}
			v->valueint = 1;
			v->valuedouble = 1;
		}
		free(nstring);
	}

	// send response
	response = cJSON_PrintUnformatted(respRoot);
	g_pluginInfo.sendcbf(&g_pluginInfo, wise_get_sensor_data_rep, response, strlen(response)+1, NULL, NULL);
	ret = 0;

error:
	if (ret)
		LOGE("error in do_set");

	if (sessionString) free(sessionString);
	if (response) free(response);
	if (respRoot) cJSON_Delete(respRoot);
	return ret;
}
*/

/*
{
   "susiCommData":{
      "sensorIDList":{
         "e":[
            {
               "n":"LocalProvision/DeviceList/00000001-0000-0000-0000-000bab8cb228",
			   "v":0,
			   "StatusCode":200
            }
         ]
      },
      "handlerName":"LocalProvision",
      "commCmd":523,
      "sessionID":"1DCFCEBD64AD66DE3612B59C6631013A"
   }
}
*/
void HANDLER_API Handler_Recv( char * const topic, void* const data, const size_t datalen, void *pRev1, void* pRev2	 )
{
/* disable plugin function
	cJSON *root, *susiCommData, *commCmd;

	LOGD("%s call", __FUNCTION__);
	LOGD("data=[%s]", (char*) data);

	// get device name
	root = cJSON_Parse((const char*) data);
	cJSON_GetObjectItemError(root, "susiCommData", susiCommData);

	cJSON_GetObjectItemError(susiCommData, "commCmd", commCmd);
	if (commCmd->valuedouble == wise_get_sensor_data_req) { // 523
		do_get(root);
	} else if (commCmd->valuedouble == wise_set_sensor_data_req) { // 524
		do_set(root);
	}

error:
	if (root) {
		cJSON_Delete(root);
	}
*/
}

int HANDLER_API Handler_Get_Capability( char ** pOutReply )
{
	return 0;
/* disable plugin function
	LOGD("%s call", __FUNCTION__);

	*pOutReply = get_capability_string();

	if (*pOutReply) {
		return strlen(*pOutReply);
	}
	return 0;
*/
}

void HANDLER_API Handler_AutoReportStart(char *pInQuery)
{
	//LOGD("%s call", __FUNCTION__);
}

void HANDLER_API Handler_AutoReportStop(char *pInQuery)
{
	//LOGD("%s call", __FUNCTION__);
}

void HANDLER_API Handler_MemoryFree(char *pInData)
{
/* disable plugin function
	LOGD("%s call", __FUNCTION__);

	free(pInData);
*/
}
