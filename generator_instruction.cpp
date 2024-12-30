#include "generator.h"

void generator_c::CG_Instruction(tree_c* node)
{
	switch (node->Get(0)->Hash()->V())
	{
	case CODE_ADD:
	{
		CG_Add(node->Get(1));
		break;
	}
	case CODE_LD:
		CG_Load(node->Get(1));
		break;

	case CODE_CALL:
		CG_Call(node->Get(1));
		break;


	default:
		Error("Unsupported instruction\n");
	}
}

void generator_c::CG_Load(tree_c* node)
{
	tree_c*	child;
	int			i = 1; //skip the first operand. It is always at location 0
	const int	max_operands = 2 * (REGS_MAX * 2 - 1);
	const int	max_easy_ops = REGS_MAX * 2 - 1;
	int			op_ofs[max_operands];
	int			op_cnt = 0;
	int			src_code;
	tree_c*	src;

	op_ofs[op_cnt++] = 0;

	//count the operands
	while (child = node->Get(i))
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

	//load the last one into a
	src = node->Get(op_ofs[op_cnt - 1]);
	src_code = Code(src);

	//see if a already has the value loaded
	//FIXME: handle held labels
	if(src_code == CODE_TEXT)
		ASM_Load("a", src);
	else
	{//number or expression
		int ret = Constant_Expression(src);

		if(af->held[0].var || ret != af->held[0].val)
			ASM_Load("a", ret); //holding a label or a different value. Load the new value into a

	}
	
	//load a into all the other operands
	for (i = 0; i < op_cnt - 1; i++)
	{
		child = node->Get(op_ofs[i]);
		ASM_Store(child, "a");
	}
}


void generator_c::CG_Add(tree_c* node)
{
	const int	max_operands = 2 * (REGS_MAX * 2 - 1);
	const int	max_easy_ops = REGS_MAX * 2 - 1;
	int			i = 1; //skip the destination operand
	int			op_cnt = 0;
	int			op;
	tree_c*		child;
	register_t* reg;
	int			op_ofs[max_operands];

	//count the operands
	while (child = node->Get(i))
	{
		int code = child->Hash()->V();
		i++;

		if (code == CODE_COMMA)
		{
			op_ofs[op_cnt] = i;
			op_cnt++;
		}
		else if (code == NT_MEMORY_EXPR)
			Error("Mem exprs not supported\n");
	}

	if (op_cnt > max_operands)
		Error("Add only supports up to 14 operands");

#if 0

	reg = &regs[REG_AF];
	for (int i = 0; i < op_cnt; i++)
	{
		child = node->Get(op_ofs[i]);
		if (Linked(reg, 0, child))
		{//a holds this, go ahead and load it into the appropriate register(s)

		}
	}

	//check if any register already has something we need
	for (op = 0; op < max_easy_ops - 1; op++)
	{
		int which = op % 2;
		reg = &regs[REG_BC + (op / 2)];

		for (int i = 0; i < op_cnt; i++)
		{
			child = node->Get(op_ofs[i]);
			if (Linked(reg, which, child))
				printf("");
		}
	}


	

#else
	for (op = 0;; op++)
	{
		const char* regname;

		
		if (op >= op_cnt - 1)
		{
			child = node->Get(op_ofs[op_cnt - 1]); //correct order of operations for sub
			ASM_Load("a", child);
			break; //normal exit condition
		}

		if (op > 5)
		{
			child = node->Get(op_ofs[op_cnt - 1]);
			ASM_Load("a", child);
			break; //used bc,de,hl
		}

		child = node->Get(op_ofs[op_cnt - op - 2]);
		reg = &regs[REG_BC + (op / 2)];
		regname = RegToS(reg, op % 2);

		if (child->Hash()->V() != CODE_NUM_DEC) //FIXME: need to check for constant exprs!
		{
			ASM_Load("a", child);
			ASM_Load(regname, "a");
		}
		else
			ASM_Load(regname, child);
	}

	//do the add(s)
	if (op_cnt == 1)
	{//i.e. add y, x. basically y += x
		ASM_Load("b", "a");
		ASM_Load("a", node->Get(0));
		fprintf(f, "add a, b\n");
	}
	else
	{
		for (op = 0; op < op_cnt - 1; op++)
		{
			const char* regname;
			regname = RegToS(REG_BC + op / 2, op % 2);

			fprintf(f, "add a, %s\n", regname);
		}
		/*
		if (op_cnt > max_easy_ops)
		{//more than a,b,c,d,e,h,l can hold
			int reg_ofs = 0;

			fprintf(f, "push af\n"); //these pushes/pops are unnecessary if we are just dealing with constants.

			for (op = 0; op < op_cnt - max_easy_ops; op++, reg_ofs++)
			{
				const char* regname;
				child = node->Get(op_ofs[op_cnt - max_easy_ops - 1 - op]);

				reg = &regs[REG_BC + (reg_ofs / 2)];
				regname = RegToS(reg, reg_ofs % 2);

				if (child->Hash()->V() != CODE_NUM_DEC)
				{
					ASM_Load("a", child);
					ASM_Load(regname, "a");
				}
				else
					ASM_Load(regname, child);
			}

			fprintf(f, "pop af\n");

			for (op = 0; op < op_cnt - 7; op++)
			{
				const char* regname;
				regname = RegToS(REG_BC + op / 2, op % 2);

				fprintf(f, "add a, %s\n", regname);
			}
		}
		*/
	}
#endif
	ASM_Store(node->Get(0), "a");
}


typedef struct stdcall_s
{
	char	name[16];
	//void	(*cgfunc)();
	void (generator_c::* cgfunc) (tree_c*);
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

void generator_c::CG_Call(tree_c* n)
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
			(this->*call->cgfunc)(n);
			return;
		}
	}


	Error("Unknown subroutine %s", funcname);
}

//
//ASSEMBLER FILE FUNCTIONS
//