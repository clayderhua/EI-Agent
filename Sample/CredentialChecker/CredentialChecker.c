/* standard includes */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "CredentialHelper.h"

//-------------------------Memory leak check define--------------------------------
#ifdef MEM_LEAK_CHECK
#include <crtdbg.h>
_CrtMemState memStateStart, memStateEnd, memStateDiff;
#endif
//---------------------------------------------------------------------------------
#define CREDENTIAL_API "https://api-dccs.wise-paas.com/v1/serviceCredentials/%s"

int main(int argc, char *argv[]) {
	char iotkey[64] = "IoTKey";
	char url[256] = {0};
	char* data = NULL;
	char* name = NULL;
	char* pass = NULL;
	char* host = NULL;
	char* port = NULL;
#ifdef MEM_LEAK_CHECK
	_CrtSetDbgFlag ( _CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF ); 
	//_CrtSetBreakAlloc(3719);
	_CrtMemCheckpoint( &memStateStart);
#endif
	sprintf(url, CREDENTIAL_API, iotkey);
	data = Cred_GetCredential(url, NULL);
	printf("%s \n", data);

	name = Cred_ParseCredential(data, "/credential/protocols/mqtt/username");
	printf("username: %s \n", name);
	if(name)
		Cred_FreeBuffer(name);

	pass = Cred_ParseCredential(data, "/credential/protocols/mqtt/password");
	printf("password: %s \n", pass);
	if(pass)
		Cred_FreeBuffer(pass);

	port = Cred_ParseCredential(data, "/credential/protocols/mqtt/port");
	printf("port: %s \n", port);
	if(port)
		Cred_FreeBuffer(port);

	host = Cred_ParseCredential(data, "/serviceHost");
	printf("host: %s \n", host);
	if(host)
		Cred_FreeBuffer(host);

	printf("Click enter to exit");
	fgetc(stdin);
    /* exit */
	if(data)
		Cred_FreeBuffer(data);
	
	//data = Cred_ZeroConfig("00000001-1000-0000-0000-415A3A700198");
	data = Cred_ZeroConfig("\"005056c00004\"");
	printf("%s \n", data);


	name = Cred_ParseCredential(data, "/credential");
	printf("name: %s \n", name);
	if(name)
	{
		if(strcmp(name, "null")!=0)
		{
			host = Cred_ParseCredential(data, "/credential/url");
			printf("host: %s \n", host);
			if(host)
				Cred_FreeBuffer(host);

			pass = Cred_ParseCredential(data, "/credential/connectionKey");
			printf("password: %s \n", pass);
			if(pass)
				Cred_FreeBuffer(pass);
		}
		Cred_FreeBuffer(name);
	}


	printf("Click enter to exit");
	fgetc(stdin);
    /* exit */
	if(data)
		Cred_FreeBuffer(data);

#ifdef MEM_LEAK_CHECK
	_CrtMemCheckpoint( &memStateEnd );
	if ( _CrtMemDifference( &memStateDiff, &memStateStart, &memStateEnd) )
		_CrtMemDumpStatistics( &memStateDiff );
#endif


    return 0;
}

