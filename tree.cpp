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

tnode_c* tnode_c::Ins(const char* str, int code, int idx)
{
	tnode_c* t = new tnode_c;
	int sz = (int)children.size();
	std::vector<tnode_c*>::iterator it = children.begin();

	if (!t)
		Error("Ran out of memory\n");

	t->kv.Set(str, code);

	if (!sz)
		return _InsR(t);

	for (int i = 0; i < idx; i++, it++);

	children.insert(it, t);
	return t;
}

tnode_c* tnode_c::Ins(tnode_c* t, int idx)
{
	int sz = (int)children.size();
	std::vector<tnode_c*>::iterator it = children.begin();

	//if (!t)
	//	Error("Ran out of memory\n");

	if (!sz)
		return _InsR(t);

	for (int i = 0; i < idx; i++, it++);

	children.insert(it, t);
	return t;
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

int tnode_c::GetIndex(tnode_c* child)
{
	int index = -1;
	int sz = (int)children.size();

	for (int i = 0; i < sz; i++)
	{
		if (children.at(i) == child)
		{
			index = i;
			break;
		}
	}

	return index;
}

void tnode_c::Delete()
{
	//call clear and then remove
}

void tnode_c::KillAllChildren()
{//clear out the list below, but not including, the root
	if (leaf) //this should also be caught in the condition below
		return;

	for (std::vector<tnode_c*>::iterator it = children.begin(); it != this->children.end(); it++)
		delete* it;

	children.clear();//Hopefully clear the list...
}

bool tnode_c::KillChild(tnode_c* removee)
{
	//recursively look through all the child nodes, delete removee
	std::vector<tnode_c*>::iterator it = children.begin();

	if (this == removee)
	{
		delete this;
		return true;
	}

	for (; it != this->children.end(); it++)
	{
		if ((*it)->KillChild(removee))
		{//deleted it
			children.erase(it);
			return false;
		}
	}

	return false;
}

void tnode_c::DetachChild(tnode_c* removee)
{
	//loop through, find the child, detach it 
	int i = GetIndex(removee);

	if (i != -1)
		children.erase(children.begin() + i);
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

void tnode_c::Collapse(tnode_c* child)
{
	int start = GetIndex(child);
	tnode_c* grandchild;

	for (int i = 0; grandchild = child->Get(i); i++)
		Ins(grandchild, start + i);

	//remove just this node
	DetachChild(child);
	child->kv.~kv();
}