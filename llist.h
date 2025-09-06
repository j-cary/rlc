#pragma once
#include "common.h"


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
	void Insert(node_c* prev, const char* key, CODE value, size_t _line_no);
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