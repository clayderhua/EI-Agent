#include <stdio.h>
#include <conio.h>
#include "stdafx.h"
#include "stack_objinfo.h"

STACK s;
bool init_ok = false;

object_info setobj(type obj_tpye, char* value)
{
	object_info tmp;
	tmp.obj_type = obj_tpye;
	tmp.value = (char*)malloc(strlen(value)+1);
	strcpy(tmp.value, value);

	return tmp;
}

void initstack_obj()
{
	s.top = -1;
	init_ok = true;
}

void unitstack_obj()
{
	init_ok = false;
}

void clearstack_obj()
{
	object_info obj;
	
	while (s.top != - 1)
	{
		pop_obj(obj);
		free(obj.value);
		obj.value = NULL;
	}
}

bool push_obj(object_info obj)
{
	if (!init_ok) return false;
	if (s.top == (MAXSIZE - 1))
	{
		// Stack is Full
		return false;
	}
	else
	{
		s.top = s.top + 1;
		s.stk[s.top] = obj;
	}
	return true;
}

/*Function to delete an element from the stack*/
bool pop_obj(object_info &obj)
{
	if (!init_ok) return false;
	if (s.top == - 1)
	{
		// Stack is Empty
		return false;
	}
	else
	{
		obj = s.stk[s.top];
		s.top = s.top - 1;
	}

	return true;
}

/*Function to display the status of the stack*/
bool display_obj ()
{
	int i;
	if (!init_ok) return false;
	if (s.top == -1)
	{
		// Stack is empty
		return false;
	}
	else
	{
		printf ("\nThe status of the stack is\n");
		for (i = s.top; i >= 0; i--)
		{
			printf ("%d, %s\n", s.stk[i].obj_type, s.stk[i].value);
		}
	}
	printf ("\n");
}