#pragma once
#include "common.h"

//purpose: turn the parse tree into an AST 
//	check for symbol predefinition
//	check expressions
//	reduce expressions

class semantic_c
{
private:
	tnode_c* root;
public:
	void Check(tnode_c* _root);
};