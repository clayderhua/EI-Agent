#include "iniNode.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

int find_last_not_of(char *str, char ch)
{
	int len;
	int i = 0;

	len = strlen(str);

	for (i = len - 1; i >= 0; i--)
	{
		if (str[i] != ch)
			break;
	}

	return i;
}

int find_last_of(char *str, char ch)
{
	int len;
	int i = 0;

	len = strlen(str);

	for (i = len - 1; i >= 0; i--)
	{
		if (str[i] == ch)
			break;
	}

	return i;
}

int find_first_not_of(char *str, char ch)
{
	int len;
	int i = 0;

	len = strlen(str);

	for (i = 0; i < len; i++)
	{
		if (str[i] != ch)
			break;
	}

	return i;
}

int find_first_of(char *str, char ch)
{
	int len;
	int i = 0;

	len = strlen(str);

	for (i = 0; i < len; i++)
	{
		if (str[i] == ch)
			break;
	}

	return i;
}

bool substr(char *str, char *out, int spos, int epos)
{
	int len;
	int i = 0, j = 0;

	if (str == 0)
		return false;

	len = strlen(str);

	for (i = spos, j = 0; i <= epos; i++, j++)
		out[j] = str[i];

	return true;
}

bool IsCommentLine(char *buf)
{
	if (buf == NULL)
		return false;

	if (buf[0] == ';')
		return true;
	else
		return false;
}

bool IsKeyLine(char *buf)
{
	int equlpos = -1;

	if (buf == NULL)
		return false;

	if (buf[0] == 0)
		return false;

	equlpos = find_first_of(buf, '=');

	if (equlpos != 0)
		return true;
	else
		return false;
}

bool IsSectionLine(char *buf)
{
	if (buf == NULL)
		return false;

	if (buf[0] == '[')
		return true;
	else
		return false;
}

bool TrimLine(char *buf)
{
	int len;
	int x, y;
	char outStr[STRLEN];

	if (buf == NULL || strlen(buf) == 0)
		return false;

	memset(outStr, 0, STRLEN);

	x = find_first_not_of(buf, ' ');
	y = find_last_not_of(buf, ' ');

	substr(buf, outStr, x, y);
	sprintf(buf, "%s", outStr);

	return true;
}

pkeyNode_t getKeyinSection(psectionNode_t node, char *keyName)
{
	pkeyNode_t keyTmp = NULL;

	if (node == NULL)
		return NULL;

	keyTmp = node->key;
	while (keyTmp != NULL)
	{
		if (strcmp(keyTmp->name, keyName) == 0)
			return keyTmp;

		keyTmp = keyTmp->next;
	}

	return NULL;
}

psectionNode_t getSection(piniFileNode_t node, char *sectionName)
{
	psectionNode_t sectionTmp = NULL;

	if (node == NULL)
		return NULL;

	sectionTmp = node->section;
	while (sectionTmp != NULL)
	{
		if (strcmp(sectionTmp->name, sectionName) == 0)
			return sectionTmp;

		sectionTmp = sectionTmp->next;
	}

	return NULL;
}

pkeyNode_t New_key(char *str)
{
	pkeyNode_t outNode = NULL;
	int len;
	int equalpos;
	char sbuff[STRLEN];
	
	if (str == NULL)
		return NULL;

	outNode = (pkeyNode_t)malloc(sizeof(keyNode_t));

	len = strlen(str);
	equalpos = find_first_of(str, '=');

	memset(sbuff, 0, STRLEN);
	substr(str, sbuff, 0, equalpos - 1);
	sprintf(outNode->name, "%s", sbuff);

	memset(sbuff, 0, STRLEN);
	substr(str, sbuff, equalpos + 1, len);
	sprintf(outNode->val, "%s", sbuff);

	outNode->next = NULL;

	return outNode;
}

psectionNode_t New_Section(char *str)
{
	psectionNode_t outNode = NULL;
	int spos, epos;
	char sbuff[STRLEN];

	if (str == NULL)
		return NULL;

	memset(sbuff, 0, STRLEN);
	spos = find_first_not_of(str, '[');
	epos = find_last_not_of(str, ']');
	substr(str, sbuff, spos, epos);

	outNode = (psectionNode_t)malloc(sizeof(sectionNode_t));
	sprintf(outNode->name, "%s", sbuff);
	outNode->key = NULL;
	outNode->next = NULL;

	return outNode;
}

piniFileNode_t New_iniFile(char *path, char *filename)
{
	piniFileNode_t outNode = NULL;
	FILE *fp;

	if (path == NULL || filename == NULL)
		return NULL;

	outNode = (piniFileNode_t)malloc(sizeof(iniFileNode_t));

	sprintf(outNode->dir, "%s", path);
	sprintf(outNode->name, "%s", filename);
	outNode->section = NULL;
	outNode->next = NULL;

	return outNode;
}

bool LoadDefaultConfig(piniFileNode_t node)
{
	if (node == NULL)
		return false;

	node->section = New_Section("ReportConfig");

	if (node->section != NULL)
	{
		node->section->key = New_key("EnableReport=True");
		node->section->key->next = New_key("ReportInterval=60");
	}

	return true;
}

piniFileNode_t Get_iniFile(char *dir, char *filename)
{
	piniFileNode_t outNode = NULL;
	psectionNode_t prevSection = NULL, currentSection = NULL;
	pkeyNode_t prevKey = NULL, currentKey = NULL;
	FILE *fp;
	char loadpath[STRLEN];
	char strline[STRLEN];
	char ch;
	int i = 0;
	
	if (dir == NULL || filename == NULL)
		return NULL;
	
	memset(loadpath, 0, STRLEN);
	memset(strline, 0, STRLEN);

	sprintf(loadpath, "%s%s", dir, filename);

	fp = fopen(loadpath, "r");

	if (!fp)
		return NULL;

	outNode = New_iniFile(dir, filename);

	while (!feof(fp))
	{
		fread(&ch, 1, 1, fp);

		if (ch == '\n')
		{
			TrimLine(strline);

			if (IsSectionLine(strline))
			{
				currentSection = New_Section(strline);

				if (outNode->section == NULL)
					outNode->section = currentSection;
				else
					prevSection->next = currentSection;

				prevSection = currentSection;
			}
			else if (IsCommentLine(strline))
			{
				
			}
			else if (IsKeyLine(strline))
			{
				currentKey = New_key(strline);

				if (currentSection->key == NULL)
					currentSection->key = currentKey;
				else
					prevKey->next = currentKey;

				prevKey = currentKey;
			}

			memset(strline, 0, STRLEN);
			i = 0;
		}
		else
		{
			strline[i] = ch;
			i++;
		}
	}
	
	fclose(fp);
	return outNode;
}

bool Set_iniFile(piniFileNode_t node)
{
	FILE *fp;
	char loadpath[STRLEN];
	piniFileNode_t iniTmp = NULL;
	psectionNode_t sectionTmp = NULL;
	pkeyNode_t keyTmp = NULL;
	
	if (node == NULL)
		return false;

	sprintf(loadpath, "%s%s", node->dir, node->name);

	fp = fopen(loadpath, "w+");

	if (!fp)
		return false;

	iniTmp = node;
	while(iniTmp != NULL)
	{
		sectionTmp = iniTmp->section;
		while (sectionTmp != NULL)
		{
			fwrite("[", sizeof(char), strlen("["), fp);
			fwrite(sectionTmp->name, sizeof(char), strlen(sectionTmp->name), fp);
			fwrite("]\n", sizeof(char), strlen("]\n"), fp);

			keyTmp = sectionTmp->key;
			while (keyTmp != NULL)
			{
				fwrite(keyTmp->name, sizeof(char), strlen(keyTmp->name), fp);
				fwrite("=", sizeof(char), strlen("="), fp);
				fwrite(keyTmp->val, sizeof(char), strlen(keyTmp->val), fp);
				fwrite("\n", sizeof(char), strlen("\n"), fp);

				keyTmp = keyTmp->next;
			}

			sectionTmp = sectionTmp->next;
		}

		iniTmp = iniTmp->next;
	}

	fclose(fp);
	return true;
}

bool UnInitFileNode(piniFileNode_t node)
{
	piniFileNode_t iniHead = NULL, iniTmp = NULL;
	psectionNode_t sectionHead = NULL, sectionTmp = NULL;
	pkeyNode_t keyHead = NULL, keyTmp = NULL;

	if (node == NULL)
		return false;

	iniHead = node;
	while (iniHead != NULL)
	{
		sectionHead = iniHead->section;
		while (sectionHead != NULL)
		{
			keyHead = sectionHead->key;
			while (keyHead != NULL)
			{
				keyTmp = keyHead;
				keyHead = keyHead->next;
				free(keyTmp);
				keyTmp = NULL;
			}

			sectionTmp = sectionHead;
			sectionHead = sectionHead->next;
			free(sectionTmp);
			sectionTmp = NULL;
		}

		iniTmp = iniHead;
		iniHead = iniHead->next;
		free(iniTmp);
		iniTmp = NULL;
	}

	return true;
}

