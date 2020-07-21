#define MAXSIZE 256

enum type 
	{
		filename,
		scan_status, // it is virus name if the file is infected.
		repair_status,
		end,
	};

struct object_info
{
	type obj_type;
	char* value;
};

struct stack /* Structure definition for stack */
{
	object_info stk[MAXSIZE];
	int top;
};

typedef struct stack STACK;

/* Function declaration/Prototype*/
object_info setobj(type obj_tpye, char* value);
void initstack_obj(void);
void unitstack_obj(void);
void clearstack_obj(void);
bool push_obj (object_info obj);
bool pop_obj(object_info &obj);
bool display_obj (void);