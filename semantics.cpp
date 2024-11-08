#include "semantics.h"

//<identifier> - need to be forward declared. Thinking a stack which pops stuff off after completing a compound statement
//<constant_expression> - needs to get expressions computed. Also needs to check for <identifier>'s

void semantic_c::GenerateAST(tnode_c* _root)
{
	struct timeb start, end;
	int elapsed_time;
	float time_seconds;

	ftime(&start);
	printf("\nGenerating AST...\n");

	root = _root;

	VisitNode(root, NULL);

	ftime(&end);
	time_seconds = (1000 * (end.time - start.time) + (end.millitm - start.millitm)) / 1000.0;
	printf("Generated AST in %.4f second(s)\n", time_seconds);

	root->Disp();
}

void semantic_c::VisitNode(tnode_c* node, tnode_c* parent)
{
	tnode_c* child;
	const kv_c* kv = node->Hash();

	for (int i = 0; child = node->Get(i); i++)
	{
		VisitNode(child, node);
	}
#if 1

	if (parent && node->Get(0) && !node->Get(1))
	{//just one operand
		parent->Collapse(node);
	}

#else
	switch (kv->V())
	{
	case NT_MEMORY_EXPR:
	case NT_MEMORY_PRIMARY_EXPR:
	case NT_EXTERNAL_DECL:
	//case NT_FUNC_DEF:
	case NT_DATA_DECL:
	case NT_COMPOUND_STMT:
	case NT_STMT:
	case NT_SIMPLE_STMT:
	case NT_INSTRUCTION:
	case NT_MEM_OR_CONST_EXPR:

		parent->Collapse(node);
		break;

	default:
		break;
	}
#endif

	//printf("%s\n", kv->K());
}