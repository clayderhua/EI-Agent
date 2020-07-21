#ifndef _SUSIAUTHENTICATOR_H_
#define _SUSIAUTHENTICATOR_H_

#ifdef __cplusplus
extern "C" {
#endif

int SUSIAuthenticator();

int SUSICheckPlatform(char* getStr, int* getStrLen);

#ifdef __cplusplus
}
#endif

#endif