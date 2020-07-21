/****************************************************************************/
/* Copyright(C) : Advantech Technologies, Inc.(��ج�ުѥ��������q)				 */
/* Create Date  : 2013/05/20 by Eric Liang															     */
/* Modified Date: 2013/05/20 by Eric Liang															 */
/* Abstract       :  Array Library Def          			    										             */
/* Reference    : None																									 */
/****************************************************************************/
#include "platform.h"
#include "ArrayLib.h"
#include "HashTable.h"
#include <limits.h>

void HashTableTest() {
  //printf("\n=======HashTableTest()==========\n");
  char *names[] = { "John", "Mary", "George", "Mickey", "Snoopy", "Bob", "Jack" };
  char *ids[]    = { "1",    "2",    "3",      "4",      "5",      "6",   "7" };
  HashTable* table = HashTableNew(3);
  int i;
  for (i=0; i<5; i++)
    HashTablePut(table, names[i], ids[i]);
//  for (i=0; i<7; i++)
//    printf("id=%s\n", HashTableGet(table, names[i]));
//  HashTableEach(table, strPrintln);
  HashTableFree(table);
}

int hash(  char *key, int range ) {
 
	unsigned long int hashval = 0;
	int i = 0;
 
	/* Convert our string to an integer */
	while( hashval < ULONG_MAX && i < strlen( key ) ) {
		hashval = hashval << 8;
		hashval += key[ i ];
		i++;
	}
 
	return hashval %range;
}

int hash2(char *key, int range) {
  int i, hashCode=0;
  for (i=0; i<strlen(key); i++) {
    BYTE value = (BYTE) key[i];
    hashCode += value;
    hashCode %= range;
  }
  return hashCode;
}

Entry* EntryNew(char *key, void *data) {
  Entry* e = (Entry*)ObjNew(Entry, 1);
  memset(e->key,0,sizeof(e->key));
  //e->key = key;
  snprintf(e->key,sizeof(e->key),"%s",key);
  e->data = data;
  return e;
}

void EntryFree(Entry *e) {
  ObjFree(e);
}

int EntryCompare(Entry *e1, Entry *e2) {
  return strcmp(e1->key, e2->key);
}

HashTable* HashTableNew(int size) {
  Array *table = ArrayNew(size);
  int i;
  for (i=0; i<table->size; i++)
    ArrayAdd(table, ArrayNew(1));
  return table;
}

void HashTableFree(HashTable *table) {
  int ti, hi;
  for (ti=0; ti<table->size; ti++) {
    Array *hitArray = (Array *)table->item[ti];
    ArrayFree(hitArray, (FuncPtr1) EntryFree);
  }
  ArrayFree(table, NULL);
}


// �M������� key �ҹ����������öǦ^
void *HashTableGet(HashTable *table, char *key) { 
  Entry keyEntry;
  int slot;
  Array *hitArray;
  int keyIdx;
  Entry *e;
  memset(&keyEntry,0,sizeof(keyEntry));

  slot = hash(key, table->size);            // ���o����� (�C��) 

  hitArray = (Array*) table->item[slot]; // ���o�ӦC
  // ��X�ӦC���O�_���]�t key ���t��
  //keyEntry.key = key;
  snprintf(keyEntry.key,sizeof(keyEntry.key),"%s",key);

  keyIdx = ArrayFind(hitArray, &keyEntry, (FuncPtr2)EntryCompare);
  if (keyIdx >= 0) { // �Y���A�h�Ǧ^�Ӱt�諸��Ƥ��� 
    e = (Entry *)hitArray->item[keyIdx];
    return e->data;
  }
  return NULL; // �_�h�Ǧ^ NULL 
}

// �N (key, data) �t���J����� 
void HashTablePut(HashTable *table, char *key, void *data) {
  Entry *e;
  Entry keyEntry;
  int slot;
  Array *hitArray;
  int keyIdx;
  memset(&keyEntry,0,sizeof(keyEntry));
  slot = hash(key, table->size);            // ���o����� (�C��) 

  hitArray = (Array*) table->item[slot]; // ���o�ӦC
  //keyEntry.key = key;
  snprintf(keyEntry.key,sizeof(keyEntry.key),"%s",key);

  keyIdx = ArrayFind(hitArray, &keyEntry, (FuncPtr2)EntryCompare);
  if (keyIdx >= 0) { // �Y���A�h�Ǧ^�Ӱt�諸��Ƥ��� 
    e = (Entry *)hitArray->item[keyIdx];
    e->data = data;
  } else {
    e= EntryNew(key, data);// �إ߰t�� 
    ArrayAdd(hitArray, e); // ��J�������C�� 
  }
}

void HashTableEach(HashTable *table, FuncPtr1 f) {
  int i, j;
  Array *hits;
  Entry *e;
  for (i=0; i<table->count; i++) {
    hits = (Array *)table->item[i];
    for (j=0; j<hits->count; j++) {
      e = (Entry *)hits->item[j];
      f(e->data);
    }
  }
}

Array* HashTableToArray(HashTable *table) { 
  int i, j;
  Entry *e;
  Array *hits;
  Array *array = ArrayNew(table->count);
  for (i=0; i<table->count; i++) {
    hits = (Array *)table->item[i];
    for (j=0; j<hits->count; j++) {
      e = (Entry *)hits->item[j];
      ArrayAdd(array, e->data);
    }
  }
  return array;
}