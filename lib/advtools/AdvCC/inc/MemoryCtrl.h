#ifndef __MEMORY_CONTROL_H__
#define __MEMORY_CONTROL_H__

void *__malloc__(int size);
#define AdvMalloc(size) __malloc__(size)
void __free__(void **target);
#define AdvFree(target) __free__((void **)target)

#ifdef MEMORY_DEBUG
void memory_not_free();
#endif

#endif //__MEMORY_CONTROL_H__

