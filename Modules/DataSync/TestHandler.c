/* This is a module for test purpose, you can control it with unix socket */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>

#include "srp/susiaccess_handler_api.h"

#define DEBUG_TEST_EVENT
#define MODULE_NAME "TestHandler"

#define TestLog(fmt, ...)  do { \
	fprintf(stderr, "[%s] " fmt "\n", g_PluginInfo.Name, ##__VA_ARGS__); } while(0)


#define CAP_STR	"{\"%s\":{\"TestValue\":{\"bn\":\"TestValue\",\"e\":[" \
				"{\"n\":\"counter\",\"v\":%d,\"asm\":\"r\"}," \
				"{\"n\":\"random\",\"v\":%ld,\"asm\":\"r\"}]}}}"

#define REP_STR	"{\"%s\":{\"TestValue\":{\"bn\":\"TestValue\",\"e\":[" \
				"{\"n\":\"counter\",\"v\":%d}," \
				"{\"n\":\"random\",\"v\":%ld}]}}}"

#define EVT_STR "{\"subtype\":\"TestEvent\"," \
				"\"msg\":\"%s\"}"

#define UNIX_SOCKET_NAME "test-socket"
#define MSG_LEN_SIZE 2
#define MSG_VAL_SIZE 128

char g_repBuffer[512];
int reportRunning = -1; // 0: stop, 1: start, -1: stop complete
Handler_info g_PluginInfo;
int g_reportInterval = 1;
int g_instance = 0;
char g_testBuffer[MSG_VAL_SIZE*2+1]; // key=value
pthread_t g_reportThread;
static int g_counter = 0;

/*================== control thread =============================*/


int writeString(int sock, char* buffer)
{
	char LenBuffer[MSG_LEN_SIZE];
	int msg_len = strlen(buffer);

	LenBuffer[0] = msg_len & 0x00FF;
	LenBuffer[1] = (msg_len & 0xFF00) >> 8;

	// write length
	if (write(sock, LenBuffer, MSG_LEN_SIZE) < 0) {
        perror("writing on stream socket");
		return -1;
	}

	// write data
    if (write(sock, buffer, msg_len) < 0) {
        perror("writing on stream socket");
		return -1;
	}

	return 0;
}

int readString(int msgsock, char* buffer, int buffer_len)
{
	int msg_len, rval;

	if ((rval = read(msgsock, buffer, MSG_LEN_SIZE)) < 0) {
		perror("reading stream message");
		return -1;
	} else if (rval == 0) {
		TestLog("Ending connection");
		return -1;
	}

	msg_len = (buffer[1] << 8) | buffer[0];
	if (msg_len > buffer_len) {
		TestLog("Buffer is not engouth");
		return -1;
	}
	memset(buffer, 0, buffer_len);

	if (msg_len == 0) {
		return 0;
	} else if ((rval = read(msgsock, buffer, msg_len)) < 0) {
		perror("reading stream message");
		return -1;
	} else if (rval == 0) {
		TestLog("Ending connection");
		return -1;
	}

	return 0;
}

int doServer(int msgsock)
{
	char *key, *value;
	int isResponse = -1; // 0: no response, 1: response sucess, -1: response fail, others: not support
	char buffer[512];

	do {
		if (readString(msgsock, g_testBuffer, sizeof(g_testBuffer))) {
			return -1;
		}

		key = g_testBuffer;
		value = strchr(key, '=');
		if (value != NULL) {
			*value = '\0';
			value++;
		}

		TestLog("key=[%s], value=[%s]", key, value);
		if (strcmp(key, "help") == 0) {
			strcpy(buffer, "help <any>\n");
			strcat(buffer, "reportInterval <interval>\n");
			strcat(buffer, "sendEvent msg\n");
			strcat(buffer, "sendCapability default\n");
			if (writeString(msgsock, buffer)) {
				return -1;
			}
			isResponse = 0;
		}
		else if (strcmp(key, "reportInterval") == 0) {
			g_reportInterval = strtol(value, NULL, 10);
			if (g_reportInterval == 0) {
				Handler_AutoReportStop(NULL);
			} else {
				Handler_AutoReportStart(NULL);
			}
			isResponse = 1;
		}
		else if (strcmp(key, "sendEvent") == 0) {
			sprintf(buffer, EVT_STR,
					value);
			g_PluginInfo.sendeventcbf(&g_PluginInfo, Severity_Informational, buffer, strlen(buffer)+1, NULL, NULL);
			isResponse = 1;
		}
		else if (strcmp(key, "sendCapability") == 0) {
			g_counter++;
			sprintf(buffer, CAP_STR,
					g_PluginInfo.Name,
					g_counter,
					random());
			g_PluginInfo.sendcapabilitycbf(&g_PluginInfo, buffer, strlen(buffer)+1, NULL, NULL);
			isResponse = 1;
		}

		switch (isResponse) {
			case 0: // no response, others: not support
				// do nothing
				break;
			case 1: // response sucess
				strcpy(g_testBuffer, "Success");
				if (writeString(msgsock, g_testBuffer)) {
					return -1;
				}
				break;
			case -1: // response fail
				strcpy(g_testBuffer, "Fail");
				if (writeString(msgsock, g_testBuffer)) {
					return -1;
				}
				break;

			default:
				strcpy(g_testBuffer, "Not support");
				if (writeString(msgsock, g_testBuffer)) {
					return -1;
				}
		}
	} while (1);

	return 0;
}

static void* controlThread(void* args)
{
	int sock, msgsock;
	struct sockaddr_un server;

	sock = socket(AF_UNIX, SOCK_STREAM, 0);
	if (sock < 0) {
		perror("opening stream socket");
		return NULL;
	}
	server.sun_family = AF_UNIX;
	sprintf(server.sun_path, "%s%d", UNIX_SOCKET_NAME, g_instance);
	unlink(server.sun_path);

	if (bind(sock, (struct sockaddr *) &server, sizeof(struct sockaddr_un))) {
		perror("binding stream socket");
		return NULL;
	}
	TestLog("Socket has name %s", server.sun_path);
	listen(sock, 1);

	for (;;) {
		msgsock = accept(sock, 0, 0);
		if (msgsock == -1)
			perror("accept");
		else
			doServer(msgsock);
		close(msgsock);
	}
	close(sock);
	unlink(UNIX_SOCKET_NAME);

	return NULL;
}

/*================== API =============================*/
int HANDLER_API Handler_Initialize( HANDLER_INFO *pluginfo )
{
	// save info
	g_PluginInfo = *pluginfo;

	TestLog("%s call", __FUNCTION__);

	sscanf(pluginfo->Name, MODULE_NAME "%d", &g_instance);
	TestLog("[%s], find new insatnce=%d", pluginfo->Name, g_instance);

	pthread_t pHandler;
	pthread_create(&pHandler, NULL, controlThread, NULL);

	return handler_success;
}

void Handler_Uninitialize()
{
	TestLog("%s call", __FUNCTION__);
}

int HANDLER_API Handler_Get_Status( HANDLER_THREAD_STATUS * pOutStatus )
{
	TestLog("%s call", __FUNCTION__);
	return handler_success;
}

void HANDLER_API Handler_OnStatusChange( HANDLER_INFO *pluginfo )
{
	TestLog("%s call", __FUNCTION__);
}

int HANDLER_API Handler_Start( void )
{
	TestLog("%s call", __FUNCTION__);
	return handler_success;
}

int HANDLER_API Handler_Stop( void )
{
	TestLog("%s call", __FUNCTION__);
	Handler_AutoReportStop(NULL);
	return handler_success;
}

void HANDLER_API Handler_Recv( char * const topic, void* const data, const size_t datalen, void *pRev1, void* pRev2	 )
{
	TestLog("%s call", __FUNCTION__);
}

/*
{"TestModule":{"TestValue":{"bn":"TestValue","e":[
{"n":"counter","v":%d,"asm":"r"},
{"n":"time","v":%d,"asm":"r"}]}}}
*/
int HANDLER_API Handler_Get_Capability( char ** pOutReply )
{
	g_counter++;

	memset(g_repBuffer, 0, sizeof(g_repBuffer));
	sprintf(g_repBuffer, CAP_STR,
			g_PluginInfo.Name,
			g_counter,
			random());

	*pOutReply = g_repBuffer;
	TestLog("%s call, buf=[%s]", __FUNCTION__, *pOutReply);

	return strlen(*pOutReply) + 1;
}

/*
{"TestModule":{"TestValue":{"bn":"TestValue","e":[
{"n":"counter","v":%d},
{"n":"time","v":%d}]}}}
*/
static void* autoReportThread(void* args)
{
#ifdef DEBUG_TEST_EVENT
	char msg[128];
	char buffer[512];
	int count = 0;
#endif

	while (reportRunning == 1) {
		g_counter++;
		memset(g_repBuffer, 0, sizeof(g_repBuffer));
		sprintf(g_repBuffer, REP_STR,
				g_PluginInfo.Name,
				g_counter,
				random());

		TestLog("%s call, buf=[%s]", __FUNCTION__, g_repBuffer);
		g_PluginInfo.sendreportcbf(&g_PluginInfo, g_repBuffer, strlen(g_repBuffer)+1, NULL, NULL);

#ifdef DEBUG_TEST_EVENT
		sprintf(msg, "%s - event %d", g_PluginInfo.Name, count);
		sprintf(buffer, EVT_STR, "");
		g_PluginInfo.sendeventcbf(&g_PluginInfo, Severity_Informational, buffer, strlen(buffer)+1, NULL, NULL);
		count++;
#endif
		sleep(g_reportInterval);
	}
	reportRunning = -1;

	return 0;
}

void HANDLER_API Handler_AutoReportStart(char *pInQuery)
{
	TestLog("%s call", __FUNCTION__);
	int i;

	// waiting thread stop
	if (reportRunning == 0) {
		for (i = 0; i < 10; i++) {
			if (reportRunning == -1) { // thread stoped
				break;
			}
			sleep(10);
		}
	}

	if (reportRunning == -1) {
		reportRunning = 1;
		pthread_create(&g_reportThread, NULL, autoReportThread, NULL);
	}
}

void HANDLER_API Handler_AutoReportStop(char *pInQuery)
{
	TestLog("%s call", __FUNCTION__);
	reportRunning = 0;
	pthread_join(g_reportThread, NULL);
}

void HANDLER_API Handler_MemoryFree(char *pInData)
{
	TestLog("%s call", __FUNCTION__);
}