#pragma once
#include <iostream>
#include <vector>
#include <stdarg.h>

#define PROG_MAX_LEN	128
#define KEY_MAX_LEN		64
#define TEXT_MAX_LEN	64
#define LINES_MAX_CNT	1024

enum CODES
{
	CODE_NONE = 0, 
	CODE_LPAREN, CODE_RPAREN, CODE_LBRACE, CODE_RBRACE, CODE_LBRACKET, CODE_RBRACKET,
	CODE_AT, CODE_POUND, CODE_PERIOD, CODE_COMMA, /*CODE_QUOTE_DOUBLE, CODE_QUOTE_SINGLE,*/ CODE_SEMICOLON,
	CODE_COLON, CODE_RARROW,
	CODE_EQUALS, CODE_PLUS, CODE_MINUS, CODE_STAR, CODE_FSLASH, //arithmetic

	//words
	CODE_START, CODE_INLINE, CODE_FUNC, CODE_ANS,
	//preprocessing directives
	CODE_PP_INCLUDE, CODE_PP_INSERT, CODE_PP_DEFINE,

	CODE_TEXT, CODE_NUM_DEC, CODE_NUM_BIN, CODE_NUM_HEX, CODE_STRING,


	//NON-TERMINALS
	NT_STATEMENT
};

typedef struct kv_s
{
	char k[KEY_MAX_LEN] = {};
	int v;
} kv_t;

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

	inline const char* K() { return k; };
	inline int V() { return v; };

	kv_c()
	{
		k = NULL;
		v = CODE_NONE;
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
	node_c* next;
	union
	{
		//kv_t kv;
		kv_c kv;
		int i;
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
	void InsertHead(kv_t _kv);
	void RemoveHead();
	void Insert(node_c* prev, kv_t _kv);
	void Remove(node_c* prev);

	void Disp();
	int Len() { return len; }

	node_c* Search(const char* key);
	node_c* Offset(int ofs);

	void Clear(); //make this a destructor, too

	llist_c()
	{
		head = NULL;
		len = 0;
	}
	~llist_c()
	{
		Clear();
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


class tnode_c
{
private:
	bool leaf;
	kv_c kv;
	std::vector<tnode_c*> children;
public:

	void InsR(kv_t _kv);
	void InsL(kv_t _kv);
	void Ins(kv_t _kv, int idx);

	tnode_c* GetL();
	tnode_c* GetR();
	tnode_c* Get(int idx);

	bool IsLeaf() { return leaf; }

	tnode_c()
	{
		kv.kv_c::kv_c();
		leaf = true;
	}
	tnode_c(kv_t _kv)
	{
		kv = _kv;
		leaf = true;
	}
	~tnode_c()
	{
		kv.kv_c::~kv_c();

		if (leaf) //this should also be caught in the condition below
			return;

		for (int i = 0, cnt = children.size(); i < cnt; i++)
		{
			//printf("Deleting %s %i\n", children[i]->kv.K(), children[i]->kv.V());
			delete children[i];
		}
	}
};

class tree_c
{
private:
	tnode_c* head;
public:
	void InsertLNode();
	void InsertRNode();
	void InsertNode();

	void DispInorder();

	~tree_c();
};

void Warning(const char* msg, ...);
void Error(const char* msg, ...);