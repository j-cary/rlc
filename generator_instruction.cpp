#include "generator.h"

void generator_c::CG_Instruction(tree_c* node, cfg_c* block)
{
	const int max_ops = 3;

	int		i = 1;
	int		op_cnt = 0;
	int		op;
	tree_c* op_child = node->Get(1);
	tree_c* child;
	int		op_ofs[max_ops + 1];
	colori_t		color[max_ops + 1];

	//count the operands
	op_ofs[op_cnt] = 0;
	color[op_cnt++] = block->DataColor(Str(op_child->Get(0)));

	//FIXME: what about 1 or 0 ops? 

	while (child = op_child->Get(i))
	{
		int code = child->Hash()->V();
		i++;

		if (code == CODE_COMMA)
		{
			if (op_cnt > max_ops)
				Error("%s has too many ops", Str(node->Get(0)));

			child = op_child->Get(i);
			if (Code(child) == CODE_TEXT)
				color[op_cnt] = block->DataColor(Str(child));
			else
				color[op_cnt] = -1;
			op_ofs[op_cnt] = i;

			op_cnt++;
		}
		else if (code == NT_MEMORY_EXPR)
			Error("Mem exprs not supported\n");
	}

	//determine which data are used per instruction
	//pass (in-order) list of colors corresponding to each data

	switch (node->Get(0)->Hash()->V())
	{
	case CODE_ADD:
	{
		PrintSourceLine(node);
		CG_Add(op_child, block, op_cnt, op_ofs, color);
		break;
	}
	case CODE_LD:
		PrintSourceLine(node);
		CG_Load(op_child, block, op_cnt, op_ofs, color);
		break;

	case CODE_CALL:
		PrintSourceLine(node);
		CG_Call(op_child, block, op_cnt, op_ofs, color);
		break;


	default:
		Error("Unsupported instruction\n"); 
	}
}

void generator_c::CG_Load(tree_c* node, cfg_c* block, int op_cnt, int* ofs, colori_t* colors)
{
	regi_t	reg[3];
	paralleli_t data[3];
	int srcofs = ofs[op_cnt - 1];
	colori_t srccolor = colors[op_cnt - 1];

	//FIXME: need to check this for all dest operands
	if (colors[0] < 0)
	{
		return;
	}

	reg[0] = RegAlloc(colors[0]);
	data[0] = DataOfs(block, node->Get(ofs[0]));

	if (srccolor < 0)
	{//constant 
		ASM_CLoad(reg[0], Constant_Expression(node->Get(ofs[op_cnt - 1])));
		MarkReg(reg[0], data[0]);
		return;
	}
	
	reg[1] = RegAlloc(colors[1]);
	data[1] = DataOfs(block, node->Get(ofs[1]));

	if (!IsMarked(reg[1], data[1]))
	{//initialize the register
		ASM_DLoad(block, REG_A, data[1]);
		ASM_RLoad(reg[1], REG_A);
		MarkReg(reg[1], data[1]);
	}

	ASM_RLoad(reg[0], reg[1]);
}


void generator_c::CG_Add(tree_c* node, cfg_c* block, int op_cnt, int* ofs, colori_t* colors)
{
	regi_t		reg[3];
	paralleli_t data[3]; //direct index into the block's array

	if (op_cnt == 2)
	{//compound assignment

		data[1] = DataOfs(block, node->Get(ofs[1]));
		reg[1] = RegAlloc(colors[1]);
		data[0] = DataOfs(block, node->Get(ofs[0]));
		reg[0] = RegAlloc(colors[0]);

		//init
		if (!IsMarked(reg[1], data[1]))
		{
			ASM_DLoad(block, REG_A, data[1]);
			ASM_RLoad(reg[1], REG_A);
			MarkReg(reg[1], data[1]);
		}

		if (!IsMarked(reg[0], data[0]))
		{
			ASM_DLoad(block, REG_A, data[0]);
			MarkReg(reg[0], data[0]);
		}
		else
			ASM_RLoad(REG_A, reg[0]);

		//add
		ASM_Add(REG_A, reg[1]);
		ASM_RLoad(reg[0], REG_A);

		return;
	}

	data[2] = DataOfs(block, node->Get(ofs[2]));
	reg[2] = RegAlloc(colors[2]);
	if (!IsMarked(reg[2], data[2]))
	{
		ASM_DLoad(block, REG_A, data[2]);
		ASM_RLoad(reg[2], REG_A);

		MarkReg(reg[2], data[2]);
	}

	data[1] = DataOfs(block, node->Get(ofs[1]));
	reg[1] = RegAlloc(colors[1]);
	if (!IsMarked(reg[1], data[1]))
	{
		ASM_DLoad(block, REG_A, data[1]);
		ASM_RLoad(reg[1], REG_A);

		MarkReg(reg[1], data[1]);
	}
	else
		ASM_RLoad(REG_A, reg[1]);

	reg[0] = RegAlloc(colors[0]);
	data[0] = DataOfs(block, node->Get(ofs[0]));

	ASM_Add(REG_A, reg[2]);
	ASM_RLoad(reg[0], REG_A);

	MarkReg(reg[0], data[0]);
}


typedef struct stdcall_s
{
	char	name[16];
	//void	(*cgfunc)();
	void (generator_c::* cgfunc) (tree_c*n, cfg_c* block, int op_cnt, int* ofs, colori_t* colors);
} stdcall_t;

#define SC(n, f) n, &generator_c::f

//this will eventually be replaced with the include file. or maybe make these standard library calls... <- this would be a lot easier
stdcall_t stdcalls[] =
{
	SC("print", SL_Print), //print character code i.e. print, 0x30 - '0'
	//printd - print decimal value i.e. print, 0x30 - "48"
	//printh - print hex value i.e. print, 0x30 - '30'
	"", NULL
};

void generator_c::CG_Call(tree_c* n, cfg_c* block, int op_cnt, int* ofs, colori_t* colors)
{
	const char* funcname;
	const char* str = "";
	int			i;
	stdcall_t*	call;

	funcname = Str(n->Get(0));

	for (i = 0; (call = &stdcalls[i])->name[0]; i++)
	{
		if (!strcmp(funcname, call->name))
		{
			(this->*call->cgfunc)(n, block, op_cnt, ofs, colors);
			return;
		}
	}


	Error("Unknown subroutine %s", funcname);
}

//
//ASSEMBLER FILE FUNCTIONS
//