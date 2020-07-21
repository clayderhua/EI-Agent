#ifndef _READINI_H_
#define _READINI_H_

#ifdef __cplusplus
extern "C" {
#endif

//Get Current Path
int GetCurrentPath(char buf[],char *pFileName);
//Get a String From INI file
char *GetIniKeyString(const char *title, const char *key, const char *filename);
//Get a Int Value From INI file
int GetIniKeyInt(const char *title, const char *key, const char *filename);
//Get a Int Value From INI file with default value
int GetIniKeyIntDef(const char *title, const char *key, const char *filename, int def);
//Get a Double Value From INI file with default value
double GetIniKeyDoubleDef(const char *title, const char *key, const char *filename, double def);
char *GetIniKeyStringDef(const char *title, const char *key, const char *filename, char* buffer, unsigned long bufferSize, const char* def);

#ifdef __cplusplus
}
#endif

#endif