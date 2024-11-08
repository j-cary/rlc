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
	llist_c symbols;

	void VisitNode(tnode_c* node, tnode_c* parent);
public:
	void GenerateAST(tnode_c* _root);
};