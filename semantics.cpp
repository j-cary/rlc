#include "semantics.h"

//<identifier> - need to be forward declared. Thinking a stack which pops stuff off after completing a compound statement
//<constant_expression> - needs to get expressions computed. Also needs to check for <identifier>'s

void semantic_c::CheckParseTree(tnode_c* _root)
{
	root = _root;

	VisitNode(root);
}

void semantic_c::VisitNode(tnode_c* node)
{
	tnode_c* child;
	for (int i = 0; child = node->Get(i); i++)
	{
		VisitNode(child);
	}
}