#include "generator.h"

void generator_c::SL_Print(tree_c* n)
{//print, op1, op2, ..., opn;
	//mem or const exprs. - figure out string operands
	tree_c*	child;
	int			i = 1; //skip the 'print'
	const int	max_operands = 2 * (REGS_MAX * 2 - 1);
	const int	max_easy_ops = REGS_MAX * 2 - 1;
	int			op_ofs[max_operands];
	int			op_cnt = 0;
	int			src_code;
	const char* str;

	//count the operands
	while (child = n->Get(i))
	{
		int code = child->Hash()->V();
		i++;

		if (code == CODE_COMMA)
		{
			op_ofs[op_cnt] = i;
			op_cnt++;
		}
		else if (code == NT_MEMORY_EXPR) //this check won't work in all cases ex. 1 + v.m 
			Error("Mem exprs not supported");
	}

	for (i = 0; i < op_cnt; i++)
	{
		child = n->Get(op_ofs[i]);
		src_code = Code(child);

		if (src_code == CODE_TEXT)
		{
			ASM_Load("a", child);
			fprintf(f, "bcall(_PutC)\n");
		}
		else
			Error("Unsupported print");

	}
	/*
	str = "push af\n"
		"add a, $30\n"
		"bcall(_PutC)\n"
		"pop af\n";

	fprintf(f, str);
	*/
}