#ifndef _SUSIACCESSAGENT_H_
#define _SUSIACCESSAGENT_H_

//-------------------------Memory leak check define--------------------------------
#ifdef MEM_LEAK_CHECK
#include <crtdbg.h>
#define _CRTDBG_MAP_ALLOC  
_CrtMemState memStateStart, memStateEnd, memStateDiff;
#endif
//---------------------------------------------------------------------------------


#endif