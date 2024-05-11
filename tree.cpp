#include "common.h"

void tnode_c::InsL(kv_t _kv)
{
	tnode_c* t = new tnode_c(_kv);
	std::vector<tnode_c*>::iterator it = children.begin();

	if (!t)
		Error("Ran out of memory\n");

	children.insert(it, t);
}

void tnode_c::InsR(kv_t _kv)
{
	tnode_c* t = new tnode_c(_kv);

	if (!t)
		Error("Ran out of memory\n");

	children.push_back(t);
	leaf = false;
}

void tnode_c::Ins(kv_t _kv, int idx)
{
	tnode_c* t = new tnode_c(_kv);
	int sz = children.size();
	std::vector<tnode_c*>::iterator it = children.begin();

	if (!t)
		Error("Ran out of memory\n");

	if (!sz || (idx > sz))
		return;

	for (int i = 0; i < idx; i++, it++) {}

	children.insert(it, t);
}

tnode_c* tnode_c::GetL()
{
	if(!children.size())
		return NULL;
	return children.front();
}

tnode_c* tnode_c::GetR()
{
	if (!children.size())
		return NULL;
	return children.back();
}

tnode_c* tnode_c::Get(int idx)
{
	int sz = children.size();

	if (!sz || (idx >= sz))
		return NULL;

	return children.at(idx);
}
