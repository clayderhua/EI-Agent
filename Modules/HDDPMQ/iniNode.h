#ifndef	_ININODE_H_
#define _ININODE_H_

#include <stdbool.h>

#define STRLEN			260

typedef struct commentNode
{
	char str[STRLEN];

	struct commentNode *next;
} commentNode_t, pcommentNode_t;

typedef struct keyNode
{
	char name[STRLEN];
	char val[STRLEN];

	struct keyNode *next;
} keyNode_t, *pkeyNode_t;

typedef struct sectionNode
{
	char name[STRLEN];
	pkeyNode_t key;

	struct sectionNode *next;
} sectionNode_t, *psectionNode_t;

typedef struct iniFileNode
{
	char name[STRLEN];
	char dir[STRLEN];
	psectionNode_t section;

	struct iniFileNode *next;	
} iniFileNode_t, *piniFileNode_t;

pkeyNode_t getKeyinSection(psectionNode_t node, char *keyName);
psectionNode_t getSection(piniFileNode_t node, char *sectionName);
pkeyNode_t New_key(char *str);
psectionNode_t New_Section(char *str);
piniFileNode_t New_iniFile(char *path, char *filename);
bool LoadDefaultConfig(piniFileNode_t node);
piniFileNode_t Get_iniFile(char *dir, char *filename);
bool Set_iniFile(piniFileNode_t node);
bool UnInitFileNode(piniFileNode_t node);

#endif