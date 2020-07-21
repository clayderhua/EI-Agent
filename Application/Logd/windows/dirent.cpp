#include "wrapper.h"
#include "dirent.h"

DIR * opendir(const char *name) {
	DIR *dir = (DIR *)malloc(sizeof(DIR));
	dir->hFind = NULL;
	dir->dir.d_name = NULL;
	char pattern[1024];
	snprintf(pattern, sizeof(pattern), "%s\\*", name);
	dir->name = strdup(pattern);
	return dir;
}

struct dirent * readdir(DIR *dirp) {
	WIN32_FIND_DATA FindFileData;
	if (dirp->hFind == NULL) {
#ifdef UNICODE
		WCHAR wsz[256];
		mbstowcs(wsz, dirp->name, sizeof(wsz));
#else
		CHAR wsz[256];
		strncpy(wsz, dirp->name, sizeof(wsz));
#endif
		if ((dirp->hFind = FindFirstFile(wsz, &FindFileData)) != INVALID_HANDLE_VALUE){
#ifdef UNICODE
			char filename[1024];
			wcstombs(filename, FindFileData.cFileName, sizeof(filename));
			dirp->dir.d_name = strdup(filename);
#else
			dirp->dir.d_name = strdup(FindFileData.cFileName);
#endif
		}
		else return NULL;
	}
	else {
		if (!FindNextFile(dirp->hFind, &FindFileData)) return NULL;
		if (dirp->dir.d_name != NULL) free(dirp->dir.d_name);
#ifdef UNICODE
		char filename[1024];
		wcstombs(filename, FindFileData.cFileName, sizeof(filename));
		dirp->dir.d_name = strdup(filename);
#else
		dirp->dir.d_name = strdup(FindFileData.cFileName);
#endif
	}
	return &dirp->dir;
}

int closedir(DIR *dirp) {
	FindClose(dirp->hFind);
	if (dirp->dir.d_name != NULL) free(dirp->dir.d_name);
	free(dirp->name);
	free(dirp);
	return 0;
}