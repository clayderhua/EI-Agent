#ifndef _VERION_INTER_H_ 
#define _VERION_INTER_H_ 
#define MAIN_VERSION 1
#define SUB_VERSION 0
#define BUILD_VERSION 3
#define SVN_REVISION 0

#define CREATE_XVER(maj,min,build,fix)			maj ##. ## min ##. ## build ##. ## fix
#define CREATE_XVER_COMMA(maj,min,build,fix)	maj ##, ## min ##, ## build ##, ## fix

#define STRINGIFY(x) #x
#define TOSTRING(x) STRINGIFY(x)

#endif /* _VERION_INTER_H_ */ 
