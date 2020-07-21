#include <stdio.h>
#include <limits.h>
#include <stdarg.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>

#ifndef WIN32
#include <sys/wait.h>
#endif

#include "lp.h"
#include "XmlHelperLib.h"
#include "util_path.h"
#include "srp/susiaccess_def.h"

#define CMD_ACTIVE		"active"
#define INVALID_DEVICE_ID	"NA"

#ifdef WIN32
	#define TCPSENDER_BIN	"lptcpsender.exe"
	#define MCASTSENDER_BIN	"lpmcastsender.exe"
#else
	#define TCPSENDER_BIN	"lptcpsender"
	#define MCASTSENDER_BIN	"lpmcastsender"
#endif

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

static LocalAgentInfo g_agentInfo;

static void print_usage(char* binName)
{
	LOGI("%s [activeall/active] [ip] [deviceID]", binName);
}

static int get_agent_info(char* workDir)
{
	char configFile[DEF_MAX_PATH];
	PXmlDoc doc = NULL;

	memset(&g_agentInfo, 0, sizeof(g_agentInfo));

	sprintf(configFile, "%s%cagent_config.xml", workDir, FILE_SEPARATOR);
	doc = CreateXmlDocFromFile(configFile);
	if (!doc) {
		LOGE("get_agent_info: CreateXmlDocFromFile(%s) fail", configFile);
		return -1;
	}
	
	GetItemValueFormDoc(doc, "/XMLConfigSettings/Profiles/DevID", g_agentInfo.deviceID, sizeof(g_agentInfo.deviceID));
	GetItemValueFormDoc(doc, "/XMLConfigSettings/BaseSettings/CredentialURL", g_agentInfo.credentialURL, sizeof(g_agentInfo.credentialURL));
	GetItemValueFormDoc(doc, "/XMLConfigSettings/BaseSettings/IoTKey", g_agentInfo.iotkey, sizeof(g_agentInfo.iotkey));
	GetItemValueFormDoc(doc, "/XMLConfigSettings/BaseSettings/TLSType", g_agentInfo.tlstype, sizeof(g_agentInfo.tlstype));
	GetItemValueFormDoc(doc, "/XMLConfigSettings/BaseSettings/CAFile", g_agentInfo.cafile, sizeof(g_agentInfo.cafile));
	GetItemValueFormDoc(doc, "/XMLConfigSettings/BaseSettings/CAPath", g_agentInfo.capath, sizeof(g_agentInfo.capath));
	GetItemValueFormDoc(doc, "/XMLConfigSettings/BaseSettings/CertFile", g_agentInfo.certfile, sizeof(g_agentInfo.certfile));
	GetItemValueFormDoc(doc, "/XMLConfigSettings/BaseSettings/KeyFile", g_agentInfo.keyfile, sizeof(g_agentInfo.keyfile));
	GetItemValueFormDoc(doc, "/XMLConfigSettings/BaseSettings/CertPW", g_agentInfo.cerpasswd, sizeof(g_agentInfo.cerpasswd));
	DestoryXmlDoc(doc);

	return 0;
}

static int run_command(int isVerbose, const char* fmt, ...)
{
#ifndef DUMMY_RUN_COMMAND
	int wstatus;
#endif
	char cmdCopy[1024];
	int cmdLen = 0;
	va_list argptr;

	memset(cmdCopy, 0, sizeof(cmdCopy));

	// format the input command
	va_start(argptr, fmt);
	vsnprintf(cmdCopy, sizeof(cmdCopy)-1, fmt, argptr);
	va_end(argptr);

	cmdLen = strlen(cmdCopy);

	if (isVerbose) {
		if (cmdLen > sizeof(cmdCopy)) {
			LOGE("command is tool large, size=%lu", sizeof(cmdCopy));
			return -1;
		}
#ifndef DUMMY_RUN_COMMAND
		LOGD("run_command(%s)", cmdCopy);
#endif
	} else {
		if (cmdLen > sizeof(cmdCopy)-16) { // 16 is " >/dev/null 2>&1"
			LOGE("command is tool large, size=%lu", sizeof(cmdCopy));
			return -1;
		}

		if (strchr(cmdCopy, '>') == NULL) { // if cmdCopy has no '>' character, means we can redirect stdout & stderr to /dev/null
			strncat(cmdCopy, " >/dev/null 2>&1", sizeof(cmdCopy)-1);
		} // else // if cmdCopy include '>' character, means it need to redirect output, we should not redirect stdout & stderr to /dev/null
	}
#ifdef DUMMY_RUN_COMMAND
	LOGD("dummy_run_command(%s)", cmdCopy);
	return 0;
#else

	wstatus = system(cmdCopy);

#ifdef WIN32
	if (wstatus == 0) {
		return 0;
	} else if (wstatus == -1) { // error from system()
		LOGD("system() call fail, errno=%d", errno);
	} else { // error from command
		LOGD("command return fail, wstatus=%d", wstatus);
	}
#else
	// wstatus != -1, system() call success
	// WIFEXITED(wstatus) == true, command terminate normally, means no interrupt or killed
	// WEXITSTATUS(wstatus) == 0, command return successed
	if (wstatus != -1 && WIFEXITED(wstatus) && WEXITSTATUS(wstatus) == 0) {
		return 0;
	}
	else if (isVerbose) // print error message
	{
		if (wstatus == -1) {
			LOGD("system() call fail");
		} else if (!WIFEXITED(wstatus)) {
			LOGD("command is interrupted");
		} else if (WEXITSTATUS(wstatus) != 0) {
			LOGD("command return fail, ret=%d", WEXITSTATUS(wstatus));
		}
	}
#endif

	return -1;
#endif
}

static int send_active(char* workDir, char* ip, char* deviceID)
{
	int ret;
	
	ret = run_command(1, "\"%s%c%s\" %s %s %s %s %s %s %s %s %s %s %s %s",
					workDir,
					FILE_SEPARATOR,
					TCPSENDER_BIN,
					ip,
					LP_TCP_SERVER_PORT,
					CMD_ACTIVE,
					deviceID,
					g_agentInfo.credentialURL,
					g_agentInfo.iotkey,
					g_agentInfo.tlstype,
					g_agentInfo.cafile,
					g_agentInfo.capath,
					g_agentInfo.certfile,
					g_agentInfo.keyfile,
					g_agentInfo.cerpasswd
					);
	return ret;
}

static int send_activeall(char* workDir)
{
	int ret;

	ret = run_command(1, "\"%s%c%s\" %s %s %s %s %s %s %s %s %s %s",
				workDir,
				FILE_SEPARATOR,
				MCASTSENDER_BIN,
				CMD_ACTIVE,
				INVALID_DEVICE_ID,
				g_agentInfo.credentialURL,
				g_agentInfo.iotkey,
				g_agentInfo.tlstype,
				g_agentInfo.cafile,
				g_agentInfo.capath,
				g_agentInfo.certfile,
				g_agentInfo.keyfile,
				g_agentInfo.cerpasswd
				);

	return ret;
}

#ifdef WIN32
static int get_agent_install_dir(char* configDir)
{
	HKEY hk;
	char regName[] = "SOFTWARE\\Advantech\\WISE-Agent";
	DWORD dwBufferSize = DEF_MAX_PATH;
	DWORD dwRet;

	dwRet = RegOpenKeyEx(HKEY_LOCAL_MACHINE, regName, 0, KEY_READ, &hk);
	if(dwRet != ERROR_SUCCESS) {
		// try x64
		dwRet = RegOpenKeyEx(HKEY_LOCAL_MACHINE, regName, 0, KEY_READ|KEY_WOW64_64KEY, &hk);
		if(dwRet != ERROR_SUCCESS) {
			return -1;
		}
	}
	if(ERROR_SUCCESS != RegQueryValueEx(hk, "Path", 0, NULL, (LPBYTE)configDir, &dwBufferSize)) {
		RegCloseKey(hk);
		return -1;
	}
	RegCloseKey(hk);

	if (dwBufferSize == 0) {
		return -1;
	}

	return 0;
}
#else
static int get_agent_install_dir(char* configDir)
{
	strcpy(configDir, "/usr/local/AgentService");
	return 0;
}
#endif

int main(int argc, char* argv[])
{
	char workDir[MAX_PATH];
	char configDir[MAX_PATH];
	
	if (argc < 2) {
		print_usage(argv[0]);
		return 0;
	} else if (strcmp(argv[1], "active") == 0 && argc < 4) {
		print_usage(argv[0]);
		return 0;
	}

	util_module_path_get(workDir);
	sprintf(configDir, "%s%cagent_config.xml", workDir, FILE_SEPARATOR);
	if (util_is_file_exist(configDir)) {
		// workDir/agent_config.xml exist, set workDir as configDir
		strcpy(configDir, workDir);
	} else { // if no agent_config.xml in workDir
		// try to get agent_config.xml from agent install path
		get_agent_install_dir(configDir);
	}
	if (get_agent_info(configDir)) {
		LOGE("No valid agent_config.xml");
		return -1;
	}

	if (strcmp(argv[1], "activeall") == 0) {
		send_activeall(workDir);
	} else if (strcmp(argv[1], "active") == 0) {
		send_active(workDir, argv[2], argv[3]);
	} else {
		LOGE("unknown action!");
	}
}
