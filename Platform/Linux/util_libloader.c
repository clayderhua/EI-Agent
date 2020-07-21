#include "util_libloader.h"
#include <stdlib.h>
#include <string.h>
#include <dlfcn.h>
#include "util_path.h"

# define LM_ID_BASE	0	/* Initial namespace.  */
# define LM_ID_NEWLM	-1	/* For dlmopen: request new namespace.  */
# define RTLD_DI_LMID 1

#ifndef RTLD_DEEPBIND
# define RTLD_DEEPBIND RTLD_LOCAL
#endif


extern void *dlmopen (long int __nsid, const char*, int);

bool util_dlexist(char* path)
{
	bool bRet = false;
	void * hSAMANAGERDLL = NULL;
	hSAMANAGERDLL = dlopen(path, RTLD_LAZY);
	//hSAMANAGERDLL = dlmopen(LM_ID_NEWLM, path, RTLD_LAZY );/*Not support OpenWRT*/
	if(hSAMANAGERDLL != NULL)
	{
		bRet = true;
		dlclose(hSAMANAGERDLL);
		hSAMANAGERDLL = NULL;
	}
	return bRet;
}

bool util_dlopen(char* path, void ** lib)
{
	bool bRet = false;
	void * hSAMANAGERDLL = NULL;
	hSAMANAGERDLL = dlopen(path, RTLD_LAZY | RTLD_LOCAL | RTLD_DEEPBIND);
	if(hSAMANAGERDLL != NULL)
	{
		bRet = true;
		*lib = hSAMANAGERDLL;
	}
	
	return bRet;
}

bool util_dlclose(void * lib)
{
	bool bRet = true;
	if(lib)
		dlclose(lib);
	return bRet;
}

char* util_dlerror()
{
	char *error = NULL;
	char *err = dlerror();
	if(err) {
		error = calloc(strlen(err)+1, 1);
		strcpy(error, err);
	}
	return error;
}

void util_dlfree_error(char* err)
{
	if(err != NULL)
		free(err);
}

void* util_dlsym( void * handle, const char *name )
{
	return dlsym(handle, name); 
}
