#ifndef _SUSIACCESSAGENT_VERSION_H_
#define _SUSIACCESSAGENT_VERSION_H_
#define VER_MAJOR 1
#define VER_MINOR 0
#define VER_BUILD 24
#define VER_FIX 0
#define U32VER (VER_MAJOR << 24 | VER_MINOR << 16 | VER_BUILD)
#define CREATE_XVER(maj,min,build,fix) maj ##, ## min ##, ## build ##, ## fix
#endif /* _DRV_VERSION_H_ */
