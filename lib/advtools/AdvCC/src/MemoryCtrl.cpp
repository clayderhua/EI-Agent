#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "MemoryCtrl.h"


#ifdef MEMORY_DEBUG
static int gIndex = 1;

typedef struct memory_info {
    int id;
    void *addr;
} memory_info;

#define MEM_TABLE_MAX 10240
memory_info memtable[MEM_TABLE_MAX];
#endif



void *__malloc__(int size) {
    void *addr = NULL;
    addr = malloc(size);
    if(addr != NULL) {
#ifdef MEMORY_DEBUG
        memtable[gIndex].id = gIndex;
        memtable[gIndex].addr = addr;
        printf("@@@@@@@@@@@@@@@@@@@@@@@@@@@@@malloc %p [%d]\n",addr,gIndex);
        gIndex++;
#endif
        memset(addr,0,size);
    }
    return addr;
}

void __free__(void **target) {
    if(*target != NULL) {
#ifdef MEMORY_DEBUG
        int i = 0;
        for(i = 0 ; i < MEM_TABLE_MAX ; i++) {
            if(memtable[i].addr == *target) {
                memtable[i].id = 0;
                memtable[i].addr = NULL;
                break;
            }
        }
        printf("@@@@@@@@@@@@@@@@@@@@@@@@@@@@@free %p [%d]\n",*target,i);
#endif
        free(*target);
        *target = NULL;
    }
}

#ifdef MEMORY_DEBUG
void memory_not_free() {
    int i = 0, count = 0;
    for(i = 0 ; i < MEM_TABLE_MAX ; i++) {
        if(memtable[i].id != 0) {
            count++;
            printf("[%d]\n",memtable[i].id);
            printf("\taddr = %p\n",memtable[i].addr);
        }
    }
    printf("count = %d\n",count);
}
#endif
