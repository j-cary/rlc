#include "common.h"

void llist_c::InsertHead(const kv_t _kv)
{
	node_c* _next = head;
	head = new node_c;
	if (!head)
		Error("Ran out of memory!");

	len++;
	head->next = _next;
	head->kv = _kv;
}

void llist_c::RemoveHead()
{
	node_c* remove;

	if (!head)
		return;

	len--;
	remove = head;
	head = head->next;
	delete remove;
}

void llist_c::Insert(node_c* prev, kv_t _kv)
{
	node_c* ins;
	ins = new node_c;

	if (!ins)
		Error("Ran out of memory!");

	if (!head)
	{
		InsertHead(_kv);
		return;
	}

	if (prev)
	{
		ins->next = prev->next;
		prev->next = ins;	
	}
	else //place it at the end
	{
		node_c* curs = head;
		while (curs->next)
			curs = curs->next;

		curs->next = ins;
		ins->next = NULL;
	}

	ins->kv = _kv;
	len++;
}

void llist_c::Insert(node_c* prev, const kv_c* _kv)
{
	kv_t kv = {};

	if (!_kv)
		return;

	kv.v = _kv->V();

	for (int i = 0; _kv->K()[i] && i < KEY_MAX_LEN - 1; i++)
		kv.k[i] = _kv->K()[i];

	Insert(prev, kv);
}

void llist_c::Remove(node_c* prev)
{
	node_c* remove;

	if (prev)
	{
		remove = prev->next;

		if (!remove)
			return;
			//Error("Tried to remove NULL node!");

		prev->next = remove->next;
	}
	else
	{
		node_c* _prev = NULL;
		remove = head;

		if (!remove)
			return;

		if (!remove->next)
		{
			RemoveHead();
			return;
		}

		while (remove->next)
		{
			_prev = remove;
			remove = remove->next;
		}

		_prev->next = NULL;
	}

	len--;
	delete remove;
}

void llist_c::Disp()
{
	node_c* curs = head;

	if (!curs)
	{
		printf("Empty list\n");
		return;
	}

	for (; curs; curs = curs->next)
	{
		printf("%c%s\t%c%i\t%c\n", 0xB2, curs->kv.K(), 0xB1, curs->kv.V(), 0xB0);
	}
	printf("\n");
}

node_c* llist_c::Search(const char* key)
{
	node_c* curs = head;

	if (!key)
		return NULL;

	while (curs)
	{
		if (!strcmp(curs->kv.K(), key))
			return curs;
		curs = curs->next;
	}

	return NULL;
}

node_c* llist_c::Offset(int ofs)
{
	node_c* curs = head;

	for (int i = 0; i < ofs; i++, curs = curs->next)
	{
		if (!curs)
			return NULL;

	}

	return curs;
}

void llist_c::KillAllChildren()
{
	while (head)
		RemoveHead();
}

const kv_c* llist_c::Peek()
{
	node_c* curs = head;
	curs = curs->next;

	if (!curs)
		return NULL;
	return curs->KV();
}
#if 0
//deletes nodes as they are popped
kv_c* llist_c::Pop(kv_c* kv)
{
	const kv_c* tmp;

	if (kv)
	{
		tmp = head->KV();
		kv->Copy(tmp);
	}

	RemoveHead();

	return kv;
}
#endif

kv_c* llist_c::Pop(kv_c* kv)
{
	const kv_c* tmp;

	if (kv)
	{
		tmp = head->KV();
		kv->Copy(tmp);
	}

	if (!head)
		exit(124);
	head = head->next;

	return kv;
}


const kv_c* llist_c::Get()
{
	if (!head)
		return NULL;

	return head->KV();
}

void llist_c::Push(kv_t kv)
{
	InsertHead(kv);
}

void llist_c::Push(const kv_c kv)
{
	Push(&kv);
}

void llist_c::Push(const kv_c* _kv)
{
	kv_t kv = {};

	if (!_kv)
		return;

	kv.v = _kv->V();

	for (int i = 0; _kv->K()[i] && i < KEY_MAX_LEN - 1; i++)
		kv.k[i] = _kv->K()[i];

	InsertHead(kv);
}

node_c* llist_c::Save()
{
	return head;
}

void llist_c::Restore(node_c* save)
{
	head = save;
}

//kv_c
kv_c& kv_c::operator=(const kv_t& kv)
{
	int cnt;
	int thiscnt;

	if (!*kv.k) //don't bother with null keyvalues
		return *this;

	for (cnt = 0; kv.k[cnt]; cnt++) {}
	cnt++;

	if (!k)
	{
		k = new char[cnt];
		memset(k, 0, cnt * sizeof(char));
	}
	else
	{
		for (thiscnt = 0; k[thiscnt]; thiscnt++) {}
		thiscnt++;

		if (thiscnt >= cnt)
		{
			memset(k, 0, thiscnt * sizeof(char));
		}
		else
		{
			delete[] k;
			k = new char[cnt];
			memset(k, 0, cnt * sizeof(char));
		}
	}

	strcpy_s(k, cnt, kv.k);
	v = kv.v;

	return *this;
}

kv_c& kv_c::Copy(const kv_c* src)
{
	if (this == src || !src)
		return *this; //fixme: return src?

	if (!*(src->k)) //don't bother with null keyvalues
		return *this;

	Set(src->k, src->v);

	return *this;
}

void kv_c::Set(const char* _k, int _v)
{
	int cnt;
	int thiscnt;

	for (cnt = 0; _k[cnt]; cnt++) {}
	cnt++;

	if (!k)
	{
		k = new char[cnt];
		memset(k, 0, cnt * sizeof(char));
	}
	else
	{
		for (thiscnt = 0; k[thiscnt]; thiscnt++) {}
		thiscnt++;

		if (thiscnt >= cnt)
		{
			memset(k, 0, thiscnt * sizeof(char));
		}
		else
		{
			delete[] k;
			k = new char[cnt];
			memset(k, 0, cnt * sizeof(char));
		}
	}

	strcpy_s(k, cnt, _k);
	v = _v;
}
