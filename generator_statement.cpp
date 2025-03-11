#include "generator.h"

void generator_c::CG_ForLoop(tree_c* node, cfg_c* block, cfg_c* body)
{
	char label[32];
	tree_c* child;
	regi_t reg;
	tdatai_t data;

	PrintSourceLine(node);
	child = node->Get(3);

	//move this to analysis
	//did the var get initialized?
	if (Code(child) != NT_SINGLE_DATA_DECL)
	{//no init value will replace the decl with the var name
		Error("For loop control variable '%s' not initialized", Str(child));
	}

	//TODO: the control variable should also be initializeable with a mem/arith expression
	//color = DataColor(body, Str(child->Get(0)));
	data = DataOfs(body, child->Get(0)); //even though the preceding block technically uses it, the control var is stored in the actual loop body
	//color = DataColor(data);
	//reg = RegAlloc(color);
	reg = RegAlloc(data);
	ASM_CLoad(reg, Constant_Expression(child->Get(2)));
	MarkReg(reg, data);

	//label the start
	GetForLabel(label);
	ASM_Label(label);


	CG_RegBlock(body);
	//CG_ForBlock(body);

	if(reg == REG_B)
		ASM_Djnz(label);
	else
	{
		ASM_Djnz(label);

	}
}


void generator_c::CG_ForBlock(cfg_c* block)
{
}
