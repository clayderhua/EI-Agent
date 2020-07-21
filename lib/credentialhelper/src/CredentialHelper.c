#include "CredentialHelper.h"
/* standard includes */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
/* libcurl (http://curl.haxx.se/libcurl/c) */
#include <curl/curl.h>
#include "cJSON.h"
#include "util_path.h"

typedef enum {
	RMM,
	AZURE_PAAS = 11,
} PaaSSolution;

/* holder for curl fetch */
struct curl_fetch_st {
    char *payload;
    size_t size;
};

/* callback for curl fetch */
size_t curl_callback (void *contents, size_t size, size_t nmemb, void *userp) {
    size_t realsize = size * nmemb;                             /* calculate buffer size */
    struct curl_fetch_st *p = (struct curl_fetch_st *) userp;   /* cast pointer to fetch struct */

    /* expand buffer */
    p->payload = (char *) realloc(p->payload, p->size + realsize + 1);

    /* check buffer */
    if (p->payload == NULL) {
      /* this isn't good */
      fprintf(stderr, "ERROR: Failed to expand buffer in curl_callback\n");
      /* free buffer */
      free(p->payload);
      /* return */
      return -1;
    }

    /* copy contents to buffer */
    memcpy(&(p->payload[p->size]), contents, realsize);

    /* set new buffer size */
    p->size += realsize;

    /* ensure null termination */
    p->payload[p->size] = 0;

    /* return size */
    return realsize;
}

/* fetch and return url body via curl */
CURLcode curl_fetch_url(CURL *ch, const char *url, struct curl_fetch_st *fetch) {
    CURLcode rcode;                   /* curl result code */

    /* init payload */
    fetch->payload = (char *) calloc(1, sizeof(fetch->payload));

    /* check payload */
    if (fetch->payload == NULL) {
        /* log error */
        fprintf(stderr, "ERROR: Failed to allocate payload in curl_fetch_url\n");
        /* return error */
        return CURLE_FAILED_INIT;
    }

    /* init size */
    fetch->size = 0;

    /* set url to fetch */
    curl_easy_setopt(ch, CURLOPT_URL, url);

    /* set calback function */
    curl_easy_setopt(ch, CURLOPT_WRITEFUNCTION, curl_callback);

    /* pass fetch struct pointer */
    curl_easy_setopt(ch, CURLOPT_WRITEDATA, (void *) fetch);

    /* set default user agent */
    curl_easy_setopt(ch, CURLOPT_USERAGENT, "libcurl-agent/1.0");

    /* set timeout */
    curl_easy_setopt(ch, CURLOPT_TIMEOUT, 30);

    /* enable location redirects */
    curl_easy_setopt(ch, CURLOPT_FOLLOWLOCATION, 1);

    /* set maximum allowed redirects */
    curl_easy_setopt(ch, CURLOPT_MAXREDIRS, 1);

    /* fetch the url */
    rcode = curl_easy_perform(ch);

    /* return */
    return rcode;
}

CREDENTIAL_HELPER_API char* Cred_GetCredentialWithDevId(char* url, char* auth, char *deviceId, char** response)
{
	CURL *ch;                                               /* curl handle */
    CURLcode rcode;                                         /* curl result code */
	PaaSSolution sol = RMM;
    struct curl_fetch_st curl_fetch;                        /* curl fetch struct */
    struct curl_fetch_st *cf = &curl_fetch;                 /* pointer to fetch struct */
    struct curl_slist *headers = NULL;                      /* http headers to send with request */
    bool ssl = false;
	char *pos = NULL;
	char* result = NULL;
	//char *url = "https://api-dccs-develop.wise-paas.com/v1/credential/LKASJFLMN2s0d9NSkj32";
	char content[256] = { 0 };
	char realUrl[256] = { 0 };

	if(url == NULL)
	{
		/* log error */
        fprintf(stderr, "ERROR: URL is empty\n");
        /* return error */
		if(response)
			*response = strdup("ERROR: URL is empty");
        return result;
    }
	else
	{
		if(strstr(url, "https://") !=0)
			ssl = true;
	}

    /* init curl handle */
    if ((ch = curl_easy_init()) == NULL) {
        /* log error */
        fprintf(stderr, "ERROR: Failed to create curl handle in fetch_session\n");
        /* return error */
		if (response)
			*response = strdup("ERROR: Failed to create curl handle in fetch_session");
        return result;
    }

    /* set content type */
	pos = strstr(url,"Azure-PaaS=");
	if (pos != NULL) sol = AZURE_PAAS;

	/* set curl options */
	if (RMM != sol) {
		curl_easy_setopt(ch, CURLOPT_CUSTOMREQUEST, "POST");
		sprintf(content, "x-functions-key: %s", pos + sol);
		headers = curl_slist_append(headers, content);
		sprintf(content, "{\"deviceId\":\"%s\",\"auto_enrollment\":true}", deviceId);
		curl_easy_setopt(ch, CURLOPT_POSTFIELDS, content);
	}

    headers = curl_slist_append(headers, "Accept: application/json");
    headers = curl_slist_append(headers, "Content-Type: application/json");
	headers = curl_slist_append(headers, "charsets: utf-8");

	if(auth != NULL)
		headers = curl_slist_append(headers, auth);

	/* set curl options */
	curl_easy_setopt(ch, CURLOPT_HTTPHEADER, headers);
	if(ssl)
	{
		curl_easy_setopt(ch, CURLOPT_SSL_VERIFYPEER, 0L);
		curl_easy_setopt(ch, CURLOPT_SSL_VERIFYHOST, 0L);
	}

	if (RMM != sol) {
		strncpy(realUrl, url, (int)pos - (int)url);
		url = realUrl;
	}

    /* fetch page and capture return code */
    rcode = curl_fetch_url(ch, url, cf);

    /* cleanup curl handle */
    curl_easy_cleanup(ch);

    /* free headers */
    curl_slist_free_all(headers);

    /* check return code */
    if (rcode != CURLE_OK || cf->size < 1) {
        /* log error */
        fprintf(stderr, "ERROR: Failed to fetch url (%s) - curl said: %s\n", url, curl_easy_strerror(rcode));

		if (cf->payload != NULL)
			 free(cf->payload);

        /* return error */
		if (response)
			*response = strdup(curl_easy_strerror(rcode));
        return result;
    }

    /* check payload */
    if (cf->payload != NULL) {
        /* print result */
        //printf("CURL Returned: \n%s\n", cf->payload);
        /* parse return */
        result = strdup(cf->payload);
		if (response)
			*response = strdup(cf->payload);
        /* free payload */
        free(cf->payload);
    } else {
        /* error */
        fprintf(stderr, "ERROR: Failed to populate payload\n");
        /* free payload */
        free(cf->payload);
        /* return */
		if (response)
			*response = strdup("ERROR: Failed to populate payload");
        return result;
    }

 	return result;
}

CREDENTIAL_HELPER_API char* Cred_ParseCredential(char* credential, char* path)
{
	cJSON* root = NULL;
	cJSON* target = NULL;
	char* result = NULL;
	char* token = NULL;
	char* str = NULL;
	char* delim="/";
	if(credential == NULL)
		return result;
	
	root = cJSON_Parse(credential);

	if(root == NULL)
		return result;
	
	target = root;

	if(path != NULL)
	{
		str = strdup(path);
		token = strtok(str, delim);
	}
	
	while(token)
	{
		if(target->type == cJSON_Array)
		{
			int id = atoi(token);
			target = cJSON_GetArrayItem(target, id);
		}
		else
		{
			target = cJSON_GetObjectItem(target, token);	
		}
		if(target == NULL)
		{
			cJSON_Delete(root);
			if(str)
				free(str);
			return result;
		}
		token = strtok(NULL, delim);
	}
	if(target->type == cJSON_String)
		result = strdup(target->valuestring);
	else
		result = cJSON_PrintUnformatted(target);
	cJSON_Delete(root);
	if(str)
		free(str);
	return result;

}

bool Cred_LoadConfig(char const * workdir, char * url)
{
	//char *def_url = "https://adpserver.azurewebsites.net/api/adpquery";
	char *def_url = "https://aadp.wise-paas.com/api/adpquery";
	char ConfigPath[260] = {0};
	
	if(url == NULL) return false;
	
	if(workdir)
		util_path_combine(ConfigPath, workdir, "Credential.cfg");
	else
		strcpy(ConfigPath, "Credential.cfg");
	
	if(util_is_file_exist(ConfigPath))
	{
		char buf[512] = {0};
		long length = util_file_size_get(ConfigPath);
		if(util_file_read(ConfigPath, buf, length)>0)
		{
			cJSON* cURL =  NULL;
			cJSON* root = cJSON_Parse(buf);
			if(root == NULL)
				goto WRITE_DEFAULT;
			cURL = cJSON_GetObjectItem(root, "url");

			if(cURL == NULL)
				goto WRITE_DEFAULT;

			strcpy(url, cURL->valuestring);

			cJSON_Delete(root);
			return true;
		}
	}
	
WRITE_DEFAULT:
	{
		char buf[512] = {0};
		sprintf(buf, "{\"url\":\"%s\"}", def_url);
		util_file_write(ConfigPath, buf);
		strcpy(url, def_url);
		return true;
	}
}

CREDENTIAL_HELPER_API char* Cred_ZeroConfig(char *deviceId) /*Input JSON string*/
{
	CURL *ch;                                               /* curl handle */
    CURLcode rcode;                                         /* curl result code */
    struct curl_fetch_st curl_fetch;                        /* curl fetch struct */
    struct curl_fetch_st *cf = &curl_fetch;                 /* pointer to fetch struct */
    struct curl_slist *headers = NULL;                      /* http headers to send with request */
    bool ssl = true;
	char *pos = NULL;
	char* result = NULL;
	char workdir[260] = {0};
	char url[260] = {0};
	char *key = "YSvnYLjUGiv7/SV8XhRGNza14yQ1ESA6aJIVElyglDnjGV3GGI5Axg==";
	char content[256] = { 0 };
	char realUrl[256] = { 0 };

	util_module_path_get(workdir);
	
	Cred_LoadConfig(workdir, url);

	/* init curl handle */
    if ((ch = curl_easy_init()) == NULL) {
        /* log error */
        fprintf(stderr, "ERROR: Failed to create curl handle in fetch_session\n");
        /* return error */
        return result;
    }

    /* set content type */
	curl_easy_setopt(ch, CURLOPT_CUSTOMREQUEST, "POST");
	sprintf(content, "x-functions-key: %s", key);
	headers = curl_slist_append(headers, content);
	sprintf(content, "{\"deviceId\":%s}", deviceId);
	curl_easy_setopt(ch, CURLOPT_POSTFIELDS, content);

    headers = curl_slist_append(headers, "Accept: application/json");
    headers = curl_slist_append(headers, "Content-Type: application/json");
	headers = curl_slist_append(headers, "charsets: utf-8");

	/* set curl options */
	curl_easy_setopt(ch, CURLOPT_HTTPHEADER, headers);
	if(ssl)
	{
		curl_easy_setopt(ch, CURLOPT_SSL_VERIFYPEER, 0L);
		curl_easy_setopt(ch, CURLOPT_SSL_VERIFYHOST, 0L);
	}

	/* fetch page and capture return code */
    rcode = curl_fetch_url(ch, url, cf);

    /* cleanup curl handle */
    curl_easy_cleanup(ch);

    /* free headers */
    curl_slist_free_all(headers);

    /* check return code */
    if (rcode != CURLE_OK || cf->size < 1) {
        /* log error */
        fprintf(stderr, "ERROR: Failed to fetch url (%s) - curl said: %s\n", url, curl_easy_strerror(rcode));
        /* return error */
        return result;
    }

    /* check payload */
    if (cf->payload != NULL) {
        /* print result */
        //printf("CURL Returned: \n%s\n", cf->payload);
        /* parse return */
        result = strdup(cf->payload);
        /* free payload */
        free(cf->payload);
    } else {
        /* error */
        fprintf(stderr, "ERROR: Failed to populate payload\n");
        /* free payload */
        free(cf->payload);
        /* return */
        return result;
    }

 	return result;
}

CREDENTIAL_HELPER_API void Cred_FreeBuffer(char* buf)
{
	if (buf)
		free(buf);
}