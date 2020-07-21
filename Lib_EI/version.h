#ifndef __EI_CONNECT_VERSION__
#define __EI_CONNECT_VERSION__

#define MAIN_VERSION 1
#define SUB_VERSION 3
#define BUILD_VERSION 9
#define SVN_REVISION 0

#define CREATE_XVER(maj,min,build,fix)    maj ##. ## min ##. ## build ##. ## fix
#define CREATE_XVER_COMMA(maj,min,build,fix)    maj ##, ## min ##, ## build ##, ## fix

#define STRINGIFY(x) #x
#define TOSTRING(x) STRINGIFY(x)

#endif
