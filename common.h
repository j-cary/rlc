#pragma once
#include <iostream>
#include <vector>
#include <stdarg.h> //SYS printing
#include <sys/timeb.h> //Timing

#define PROG_MAX_LEN	128
#define KEY_MAX_LEN		64
#define TEXT_MAX_LEN	64
#define LINES_MAX_CNT	1024

#define DEPTH_MAX	1024 //FIXME!!! this needs to be dynamic. Used for the tab string


enum CODES
{
	CODE_NONE = 0, 
	CODE_LPAREN, CODE_RPAREN, CODE_LBRACE, CODE_RBRACE, CODE_LBRACKET, CODE_RBRACKET,
	CODE_AT, CODE_POUND, CODE_PERIOD, CODE_COMMA, /*CODE_QUOTE_DOUBLE, CODE_QUOTE_SINGLE,*/ CODE_SEMICOLON,
	CODE_COLON, CODE_LARROW, CODE_RARROW, CODE_EXCLAMATION, CODE_AMPERSAND, CODE_BAR,
	CODE_EQUALS, CODE_PLUS, CODE_MINUS, CODE_STAR, CODE_FSLASH, CODE_PERCENT,//arithmetic

	//words
	CODE_INLINE, CODE_SUBR, CODE_ANS,
	//data types
	CODE_BYTE, CODE_WORD, CODE_FIXED, CODE_LABEL, 
	CODE_STRUCT, CODE_SIGNED, CODE_STATIC, CODE_STACK, CODE_AUTO,
	//control flow
	CODE_REPEAT, CODE_UNTIL, CODE_WHILE, CODE_FOR,
	CODE_IF, CODE_ELSE, 
	//instructions
	CODE_LD, CODE_JP, CODE_CALL, CODE_RET, 
	CODE_ADD, CODE_SUB, CODE_MUL, CODE_DIV, CODE_MOD, CODE_INC, CODE_DEC, CODE_COMP,
	CODE_AND, CODE_OR, CODE_XOR, CODE_NEG,
	CODE_RLC, CODE_RRC, CODE_RL, CODE_RR, CODE_SL, CODE_SR, CODE_RES, CODE_SET, CODE_FLP,
	CODE_IN, CODE_OUT, CODE_IM,
	CODE_LDM, CODE_CPM, CODE_INM, CODE_OUTM,
	//preprocessing directives
	CODE_PP_INCLUDE, CODE_PP_INSERT, CODE_PP_DEFINE,

	CODE_TEXT, CODE_NUM_DEC, CODE_NUM_BIN, CODE_NUM_HEX, CODE_NUM_FXD, CODE_STRING,

	//GENERATED DURING PARSING
	
	//TERMINALS
	//these symbols are combinations of reserved chars not caught in lexing. ex. || or even | | will be recognized and turned into a single node in the AST
	//this is due to lazyness when designing the scanner
	T_LOGICAL_OR, T_LOGICAL_AND, T_EQUIVALENCE, T_NON_EQUIVALENCE, T_LESS_OR_EQUAL, T_GREATER_OR_EQUAL, T_DEREF_MEMBER, T_LEFT_SHIFT, T_RIGHT_SHIFT,


	//NON-TERMINALS
	NT_UNIT, NT_EXTERNAL_DECL, NT_FUNC_DEF, NT_FUNC_DECL,
	NT_DECL_SPEC, NT_STORAGE_SPEC, NT_TYPE_SPEC,
	NT_DATA_DECL, NT_SINGLE_DATA_DECL, NT_STRUCT_DECL,
	NT_TYPE_DEF,

	//data
	NT_DATA_TYPE, NT_DATA_MODIFIER,

	//statements
	NT_COMPOUND_STMT, NT_STMT, NT_OPEN_STMT, NT_CLOSED_STMT, NT_SIMPLE_STMT, 
	NT_SELECTION_CLAUSE, NT_WHILE_CLAUSE, NT_FOR_CLAUSE, NT_LABEL_DEF,

	//Instructions
	NT_INSTRUCTION, 
	NT_OPERANDS_ONE, NT_OPERANDS_TWO, NT_OPERANDS_THREE, NT_OPERANDS_ONE_TO_TWO, NT_OPERANDS_ONE_TO_THREE, NT_OPERANDS_TWO_TO_INF, NT_OPERANDS_COMP, NT_OPERANDS_FOUR, NT_OPERANDS_RET, NT_OPERANDS_CALL,

	//misc
	NT_INITIALIZER_LIST,
	NT_PARAMETER, NT_PARAMETER_LIST,
	NT_IDENT,

	//expressions
	//Logical
	NT_LOGICAL_EXPR, 
	NT_OR_EXPR, NT_AND_EXPR, NT_EQUALITY_EXPR, NT_RELATIONAL_EXPR, NT_LOGICAL_POSTFIX_EXPR, NT_LOGICAL_PRIMARY_EXPR,
	//Arithmetic
	NT_ARITHMETIC_EXPR, NT_CONSTANT_EXPR, 
	NT_SHIFT_EXPR, NT_ADDITIVE_EXPR, NT_MULTIPLICATIVE_EXPR, NT_ARITHMETIC_POSTFIX_EXPR, NT_ARITHMETIC_PRIMARY_EXPR, 
	//Memory
	NT_MEMORY_EXPR, NT_MEM_OR_CONST_EXPR, NT_MEMORY_PRIMARY_EXPR

};

typedef struct kv_s
{
	char k[KEY_MAX_LEN] = {};
	int v;
} kv_t;

const kv_t nullkv = { "NULL", CODE_NONE };

//more compact version for characters
typedef struct ckv_s
{
	char k;
	int v;
} ckv_t;


class kv_c
{
private:
	char* k = NULL;
	int v;
public:
	kv_c& operator=(const kv_t& kv);

	inline const char* K() const { return k; };
	inline int V() const { return v; };
	inline const kv_c* KV() { return this; };

	kv_c& Copy(const kv_c* src);
	void Set(const char* _k, int _v);

	kv_c()
	{
		k = NULL;
		v = CODE_NONE;
	}

	kv_c(const char* _k, int _v)
	{
		int cnt;

		for (cnt = 0; _k[cnt]; cnt++) {}
		cnt++;

		k = new char[cnt];
		memset(k, 0, cnt * sizeof(char)); //to make sure strcpy works

		strcpy_s(k, cnt, _k);
		v = _v;
	}

	~kv_c()
	{
		if (k)
			delete[] k;
		k = NULL;
	}
};

class node_c
{
private:
public:
	size_t line_no;
	node_c* next;
	union 
	{
		//kv_t kv;
		kv_c kv;
		int i;
	};

	const kv_c* KV() 
	{ 
		//if (!next)
		//	return NULL;
		return &kv;
	};

	node_c()
	{
		kv.kv_c::kv_c();
	}

	node_c(kv_t _kv, node_c* _next)
	{
		kv.kv_c::kv_c();
		kv = _kv;
		next = _next;
	}

	node_c(int _i, node_c* _next)
	{
		i = _i;
		next = _next;
	}
	~node_c()
	{
		kv.~kv_c();
	}
};

class llist_c
{
private:
	node_c* head;
	int len;
public:

	void InsertHead(const kv_t _kv);
	void RemoveHead();
	void Insert(node_c* prev, kv_t _kv, size_t _line_no);
	void Insert(node_c* prev, kv_t _kv);
	void Insert(node_c* prev, const char* key, CODES value, size_t _line_no);
	void Remove(node_c* prev);

	void Disp();
	int Len() { return len; }

	node_c* Search(const char* key);
	node_c* Offset(int ofs);

	void KillAllChildren();

	//For parsing
	const kv_c* Peek();
	kv_c* Pop(kv_c* kv);
	const kv_c* Get();
	void Push(const kv_c* _kv);
	void Push(kv_t _kv);
	void Push(const kv_c _kv);

	node_c* Save();
	void Restore(node_c* save);


	llist_c()
	{
		head = NULL;
		len = 0;
	}
	~llist_c()
	{
		KillAllChildren();
	}

#if 0
	llist_c(kv_t _kv)
	{
		head = (node_c*)malloc(sizeof(node_c));
		if (!head)
			Error("Ran out of memory!");

		head->kv = _kv;
		head->next = NULL;
	}

	llist_c(int _i)
	{
		head = (node_c*)malloc(sizeof(node_c));
		if (!head)
			Error("Ran out of memory!");

		head->i = _i;
		head->next = NULL;
	}
#endif
};


class tree_c
{
private:
	bool leaf;
	kv_c kv;
	std::vector<tree_c*> children;

	void R_Disp();
	tree_c* _InsL(tree_c* t);
	tree_c* _InsR(tree_c* t);
public:

	//need to have copies of these for a kv_c* and a const char/int pair
	tree_c* InsR(kv_c* _kv);
	tree_c* InsR(const char* str, int code);
	tree_c* InsR(kv_t _kv);
	tree_c* InsL(kv_c* _kv);
	tree_c* InsL(const char* str, int code);
	tree_c* InsL(kv_t _kv);
	void Ins(kv_t _kv, int idx); //needs work. Currently will only add a child if the list is not empty
	tree_c* Ins(const char* str, int code, int idx);
	tree_c* Ins(tree_c* t, int idx);

	tree_c* Save(); //make copy of current sub-tree
	void Restore(tree_c* saved);

	void Delete(); //Delete this sub-tree, including the root
	void KillAllChildren(); //Delete this sub-tree, minus the root
	bool KillChild(tree_c* removee); //Delete a single node. Must be called from an ancestor node
	void DetachChild(tree_c* removee); //Get a node out of the list, but don't delete it

	//Getting children
	tree_c* GetL();
	tree_c* GetR();
	tree_c* Get(int idx); //rename this

	int GetIndex(tree_c* child); //-1 if non-existant

	void Set(const kv_c* _kv) { kv.Copy(_kv); }
	void Set(const char* str, int code) { kv.Set(str, code); }
	const kv_c* Hash()		{	return &kv;	}//rename this
	 
	bool IsLeaf() { return leaf; }

	void Disp();

	void Collapse(tree_c* child);

	tree_c()
	{
		kv.kv_c::kv_c();
		leaf = true;
	}
	tree_c(kv_t _kv)
	{
		kv = _kv;
		leaf = true;
	}
	~tree_c()
	{
		kv.~kv_c();
		KillAllChildren();
	}
};

void Warning(const char* msg, ...);
void Error(const char* msg, ...);
void SetOutFlags(unsigned short flags); //same as WORD
void ResetOutFlags();

//same as in consoleapi2.h - don't want to include this in every file
#ifndef FOREGROUND_BLUE
#define FOREGROUND_BLUE      0x0001 // text color contains blue.
#endif
#ifndef FOREGROUND_GREEN
#define FOREGROUND_GREEN     0x0002 // text color contains green.
#endif
#ifndef FOREGROUND_RED
#define FOREGROUND_RED       0x0004 // text color contains red.
#endif
#ifndef FOREGROUND_INTENSITY
#define FOREGROUND_INTENSITY 0x0008 // text color is intensified.
#endif
#ifndef BACKGROUND_BLUE
#define BACKGROUND_BLUE      0x0010 // background color contains blue.
#endif
#ifndef BACKGROUND_GREEN
#define BACKGROUND_GREEN     0x0020 // background color contains green.
#endif
#ifndef BACKGROUND_RED
#define BACKGROUND_RED       0x0040 // background color contains red.
#endif
#ifndef BACKGROUND_INTENSITY
#define BACKGROUND_INTENSITY 0x0080 // background color is intensified.
#endif