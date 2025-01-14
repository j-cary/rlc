#include "generator.h"

void generator_c::CG_ForLoop(tree_c* node, cfg_c* block, cfg_c* body)
{
	char label[32];
	tree_c* child;
	colori_t color;
	regi_t reg;
	paralleli_t data;

	PrintSourceLine(node);
	child = node->Get(3);

	//did the var get initialized?
	if (Code(child) != NT_SINGLE_DATA_DECL)
	{//no init value will replace the decl with the var name
		Error("For loop control variable '%s' not initialized", Str(child));
	}

	//TODO: the control variable should also be initializeable with a mem/arith expression
	//initialize b - the control variable should ALWAYS be assigned to b
	color = body->DataColor(Str(child->Get(0)));
	reg = RegAlloc(color);
	data = DataOfs(body, child->Get(0));
	ASM_CLoad(reg, Constant_Expression(child->Get(2)));
	MarkReg(reg, data);

	//label the start
	GetForLabel(label);
	ASM_Label(label);


	CG_RegBlock(body);
	//CG_ForBlock(body);

	ASM_Djnz(label);
}


void generator_c::CG_ForBlock(cfg_c* block)
{
}
