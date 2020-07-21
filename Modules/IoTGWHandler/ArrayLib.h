/****************************************************************************/
/* Copyright(C) : Advantech Technologies, Inc.(��ج�ުѥ��������q)				 */
/* Create Date  : 2013/05/20 by Eric Liang															     */
/* Modified Date: 2013/05/20 by Eric Liang															 */
/* Abstract       :  Array Library Def          			    										             */
/* Reference    : None																									 */
/****************************************************************************/
#ifndef  __ARRAY_LIB_H__
#define __ARRAY_LIB_H__

#include "BaseLib.h"

typedef struct {
  int size;    // �}�C�ثe���W�� 
  int count;   // �}�C�ثe�������Ӽ� 
  void **item; // �C�Ӱ}�C����������
} Array;       // �ʺA�}�C����Ƶ��c 

typedef enum { KEEP_SPLITER, REMOVE_SPLITER } SplitMode;

extern void ArrayTest();

extern Array* ArrayNew(int size);// �إ߷s�}�C 
extern void ArrayFree(Array *array, FuncPtr1 freeFuncPtr); // ����Ӱ}�C 
extern void ArrayAdd(Array *array, void *item); // �s�W�@�Ӥ��� 
extern void ArrayPush(Array *array,void *item); // (�������|) ���J�@�Ӥ��� 
extern void* ArrayPop(Array *array);  //(�������|) �u�X�@�Ӥ���  
extern void* ArrayPeek(Array *array); //(�������|) ���o�̤W�������� 
extern void* ArrayLast(Array *array); // ���o�̫�@�Ӥ��� 
extern void ArrayEach(Array *array, FuncPtr1 f); //��C�Ӥ��������� f ��� 
extern int ArrayFind(Array *array, void *data, FuncPtr2 fcmp);
extern Array* split(char *str, char *spliter, SplitMode mode);

#endif // __ARRAY_LIB_H__