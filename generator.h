#pragma once
#include "common.h"

class generator_c
{
	tnode_c* root;
	FILE* f;

	void InitFile(const char* filename);
	void VisitNode(tnode_c* node);
	void VisitFuncDef(tnode_c* node);

	void Label(const char* name);

public:
	void Generate(tnode_c* _root);
};