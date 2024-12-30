#include "generator.h"

void generator_c::CG_CompoundStatement(tree_c* node)
{
	tree_c* child = node->Get(1);
	int code;
	int i = 1;


	while (1)
	{
		child = node->Get(i);
		code = child->Hash()->V();

		if (code == CODE_RBRACKET)
			break;

		switch (code)
		{
		case  NT_DATA_DECL:
			CG_DataDeclaration(child);
			break;
		case NT_INSTRUCTION:
			CG_Instruction(child);
			break;
		default:
			Error("not ready for this in a compound\n");
			break;
		}

		i++;
	}
}