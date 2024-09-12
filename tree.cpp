#include "common.h"

tnode_c* tnode_c::_InsL(tnode_c* t)
{
	std::vector<tnode_c*>::iterator it = children.begin();

	if (!t)
		Error("Ran out of memory\n");

	children.insert(it, t);
	leaf = false;

	return t;
}

tnode_c* tnode_c::_InsR(tnode_c* t)
{
	if (!t)
		Error("Ran out of memory\n");

	children.push_back(t);
	leaf = false;

	return t;
}

tnode_c* tnode_c::InsL(kv_c* _kv)
{
	tnode_c* t = new tnode_c;
	t->kv.Copy(_kv);

	return _InsL(t);
}

tnode_c* tnode_c::InsL(const char* str, int code)
{
	tnode_c* t = new tnode_c;
	t->kv.Set(str, code);

	return _InsL(t);
}

tnode_c* tnode_c::InsL(kv_t _kv)
{
	tnode_c* t = new tnode_c(_kv);
	return _InsL(t);
}

tnode_c* tnode_c::InsR(kv_c* _kv)
{
	tnode_c* t = new tnode_c;
	t->kv.Copy(_kv);

	return _InsR(t);
}

tnode_c* tnode_c::InsR(const char* str, int code)
{
	tnode_c* t = new tnode_c;
	t->kv.Set(str, code);
	return _InsR(t);
}

tnode_c* tnode_c::InsR(kv_t _kv)
{
	tnode_c* t = new tnode_c(_kv);
	return _InsR(t);
}

void tnode_c::Ins(kv_t _kv, int idx)
{
	tnode_c* t = new tnode_c(_kv);
	int sz = (int)children.size();
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
	int sz = (int)children.size();

	if (!sz || (idx >= sz))
		return NULL;

	return children.at(idx);
}

char g_tabstr[DEPTH_MAX * 2];
int g_tabs;

void tnode_c::Disp()
{
	//Clear tab string out.
	memset(g_tabstr, '\0', DEPTH_MAX * 2 * sizeof(char));
	g_tabs = 0;

	R_Disp();
}

void tnode_c::R_Disp()
{//preorder traversal of a non-binary tree...
	std::vector<tnode_c*>::iterator it = children.begin();

	//tabs are not working correctly
	//some delete is causing a crash.
	if (g_tabs > (DEPTH_MAX * 2) - 2)
		Error("Parser: Attempted to display too many recursive tree nodes!\n");

	printf("%s%s\n", g_tabstr, kv.K());

	g_tabstr[g_tabs++] = ' ';
	g_tabstr[g_tabs++] = ' ';
	
	if (!leaf)
	{//R_Disp for every child node from left to right
		for (; it != this->children.end(); it++)
		{
			(*it)->R_Disp();
		}
	}

	g_tabstr[--g_tabs] = '\0';
	g_tabstr[--g_tabs] = '\0';
}
