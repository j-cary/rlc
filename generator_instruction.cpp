#include "generator.h"
#include "evaluate_expression.h"

void generator_c::CG_Instruction(tree_c* node, cfg_c* block)
{
	constexpr int ops_max = 8; //tmp
	tree_c* operand_list = node->Get(1);
	int		op_cnt = 0;
	op_info_t op_info[ops_max + 1];
	eval_expr_c eval_expr;


	//Count the operands; get their indices and links to their data
	for (int i  = 0; tree_c* op = operand_list->Get(i); i++)
	{
		int code = op->Hash()->V();

		if (code == CODE_COMMA)
			continue;

		if (op_cnt > ops_max)
			Error("TMP: %s has too many ops", Str(node->Get(0)));

		if (code == NT_MEMORY_EXPR || code == NT_MEMORY_PRIMARY_EXPR || code == CODE_TEXT)
		{
			tree_c* data; //The actual struct/array instance

			//op_info[op_cnt].offset = Memory_Expression(op, block, func, graph, &data);
			op_info[op_cnt].offset = eval_expr.Memory(op, block, func, graph, &data);
			op_info[op_cnt].data = Data(block, data);
		}
		else
		{
			op_info[op_cnt].offset = 0;
			op_info[op_cnt].data = NULL;
		}

		op_info[op_cnt++].node = op;
	}


	switch (node->Get(0)->Hash()->V())
	{
	case CODE_LD:
		//CG_Load(op_data, op_list, op_cnt);
		CG_Load(op_info, op_cnt);
		break;
	case CODE_ADD: break;
	case CODE_RET: break;
	default:
		Error("Unsupported instruction %s", node->Get(0)->Hash()->K());
	}

}

#if OLD_REG_CODE
//TODO: mark the a reg - loading into multiple sregs in succession
void generator_c::CG_Load(tree_c* node, int op_cnt, int* ofs, tdatai_t* data, regi_t* reg)
{
	bool	sreg = false;
	bool	word = false;
	bool	byte = false;
	bool	aloaded = false;
	int		srcofs = op_cnt - 1;
	
#if 1
	for (int i = 0; i < op_cnt - 1; i++)
	{
		if (!sreg && IsSReg(reg[i]))
			sreg = true;

		if (!word && IsWord(reg[i]))
			word = true;

		if (!byte && !IsWord(reg[i]))
			byte = true;
	}

	if (reg[srcofs] < 0)
	{//const load
		regi_t	src;
		int i;

		if (sreg)
		{
			i = 0;
			if (word)
				src = REG_HL;
			else
				src = REG_A;
		}
		else
		{
			i = 1;
			src = reg[0];
			MarkReg(reg[0], data[0]);
		}
		

		ASM_CLoad(src, Constant_Expression(node->Get(ofs[srcofs])));
		for (; i < op_cnt - 1; i++)
		{
			bool isword = IsWord(reg[i]);
			if (IsSReg(reg[i]))
			{//src will only be hl or a here
				if (src == REG_HL)
				{
					if(isword)
						ASM_Store(data[i], src);
					else
					{
						if (!aloaded)
							ASM_RLoad(REG_A, LoByte(REG_HL));
						aloaded = true;
						ASM_Store(data[i], REG_A);
					}
				}
				else
				{
					if (isword)
					{
						
					}
				}

			}
			else
			{
				if (IsWord(src))
				{

				}
				ASM_RLoad(reg[i], src);
			}
			MarkReg(reg[i], data[i]);
		}

		return;
	}


	if (IsSReg(reg[srcofs]))
	{//sreg load
		ASM_DLoad(REG_A, data[srcofs]);
	}
	else
	{//hreg load
		//load 0 here if not marked
		if (!IsMarked(reg[srcofs], data[srcofs]))
			ASM_CLoad(reg[srcofs], 0);

		if(!sreg)
		{//only hreg destinations
			for (int i = 0; i < op_cnt - 1; i++)
			{
				ASM_RLoad(reg[i], reg[srcofs]);
				MarkReg(reg[i], data[i]);
			}
			return;
		}
		ASM_RLoad(REG_A, reg[srcofs]); //load into a and copy from there
	}

	for (int i = 0; i < op_cnt - 1; i++)
	{
		if (IsSReg(reg[i]))
			ASM_Store(data[i], REG_A);
		else
			ASM_RLoad(reg[i], REG_A);
		MarkReg(reg[i], data[i]);
	}
#else
	for (int i = 0; i < op_cnt - 1; i++)
	{
		if (!sreg && IsSReg(reg[i]))
			sreg = true;

		if (!word && IsWord(reg[i]))
			word = true;
	}

	if (reg[srcofs] < 0)
	{//const load
		
		if (sreg)
		{//software register somewhere. a has to be loaded regardless so just copy from it to any s or hreg
			ASM_CLoad(REG_A, Constant_Expression(node->Get(ofs[srcofs])));
			for (int i = 0; i < op_cnt - 1; i++)
			{
				if (IsSReg(reg[i]))
					ASM_Store(data[i], REG_A);
				else
					ASM_RLoad(reg[i], REG_A);
				MarkReg(reg[i], data[i]);
			}
		}
		else
		{//no sregs here
			ASM_CLoad(reg[0], Constant_Expression(node->Get(ofs[srcofs])));
			MarkReg(reg[0], data[0]);
			for (int i = 1; i < op_cnt - 1; i++)
			{
				ASM_RLoad(reg[i], reg[0]);
				MarkReg(reg[i], data[i]);
			}
		}

		return;
	}


	if (IsSReg(reg[op_cnt - 1]))
	{//sreg load
		ASM_DLoad(REG_A, data[op_cnt - 1]);

		for (int i = 0; i < op_cnt - 1; i++)
		{
			if (IsSReg(reg[i]))
				ASM_Store(data[i], REG_A);
			else
				ASM_RLoad(reg[i], REG_A);
			MarkReg(reg[i], data[i]);
		}
	}
	else
	{//hreg load
		//load 0 here if not marked
		if (!IsMarked(reg[op_cnt - 1], data[op_cnt - 1]))
			ASM_CLoad(reg[op_cnt - 1], 0);

		if (sreg)
		{
			ASM_RLoad(REG_A, reg[op_cnt - 1]); //load into a and copy from there
			for (int i = 0; i < op_cnt - 1; i++)
			{
				if (IsSReg(reg[i]))
					ASM_Store(data[i], REG_A);
				else
					ASM_RLoad(reg[i], REG_A);
				MarkReg(reg[i], data[i]);
			}
		}
		else
		{//only hreg destinations
			for (int i = 0; i < op_cnt - 1; i++)
			{
				ASM_RLoad(reg[i], reg[op_cnt - 1]);
				MarkReg(reg[i], data[i]);
			}
		}
	}
#endif
}
#endif


void generator_c::CG_Add(tree_c* node, int op_cnt, int* ofs, tdatai_t* data, regi_t* reg)
{
#if OLD_REG_CODE
	if (op_cnt == 2)
	{//compound assignment
		//init
		if (reg[1] > REG_BAD && !IsMarked(reg[1], data[1]))
		{
			ASM_CLoad(reg[1], 0);
			MarkReg(reg[1], data[1]);
		}

		//preload
		if (!IsMarked(reg[0], data[0]))
		{
			ASM_CLoad(reg[0], 0);
			MarkReg(reg[0], data[0]);
		}

		if (IsSReg(reg[0]))
			ASM_DLoad(REG_A, data[0]);
		else
			ASM_RLoad(REG_A, reg[0]);

		//add
		if (IsSReg(reg[1]))
		{
			ASM_Push(REG_HL);
			ASM_ALoad(REG_HL, data[1]);
			ASM_RAdd(REG_A, REG_HL);
			ASM_Pop(REG_HL);
		}
		else if (reg[1] == REG_BAD) //const add
			ASM_CAdd(REG_A, Constant_Expression(node->Get(ofs[1])));
		else
			ASM_RAdd(REG_A, reg[1]);

		//store
		if (IsSReg(reg[0]))
			ASM_Store(data[0], REG_A);
		else
			ASM_RLoad(reg[0], REG_A);

		return;
	}


	//INIT SECTION
	regi_t first_inited = -1;
	for (int i = op_cnt - 1; i > 1; i--)
	{//initialize any of the source operands if need be

		if (reg[i] > REG_BAD && !IsMarked(reg[i], data[i]))
		{//initialize it - only hregs since sregs are guaranteed to be initialized

			if (first_inited > 0)
				ASM_RLoad(reg[i], first_inited);
			else
			{
				ASM_CLoad(reg[i], 0);
				first_inited = reg[i];
			}

			MarkReg(reg[i], data[i]);
		}
	}

	//PRELOAD SECTION
	//handle the first source operand separately. If it doesnt get initialized, it has to be loaded into a
	if (IsSReg(reg[1]))
	{
		ASM_DLoad(REG_A, data[1]);
	} 
	else if (reg[1] == REG_BAD)
	{
		ASM_CLoad(REG_A, Constant_Expression(node->Get(ofs[1])));
	}
	else
	{//hreg
		if(IsMarked(reg[1], data[1]))
			ASM_RLoad(REG_A, reg[1]);
		else
		{
			ASM_CLoad(REG_A, 0);
			ASM_RLoad(reg[1], REG_A);
			MarkReg(reg[1], data[1]);
		}
	}

	//ADD SECTION
	bool hl_pushed = false;
	for (int i = 2; i < op_cnt; i++)
	//for (int i = op_cnt - 1; i > 1; i--)
	{
		if (IsSReg(reg[i]))
		{
			if(!hl_pushed)
				ASM_Push(REG_HL);
			ASM_ALoad(REG_HL, data[i]);
			ASM_RAdd(REG_A, REG_HL); //indirect add
			hl_pushed = true;
		}
		else if (reg[i] == REG_BAD)
		{
			ASM_CAdd(REG_A, Constant_Expression(node->Get(ofs[i])));
		}
		else
		{
			if ((reg[i] == REG_L || reg[i] == REG_H) && hl_pushed)
			{
				ASM_Pop(REG_HL);
				hl_pushed = false;
			}

			ASM_RAdd(REG_A, reg[i]);
		}
	}

	if (hl_pushed)
		ASM_Pop(REG_HL);

	//STORE SECTION
	ASM_RLoad(reg[0], REG_A);//load into op1
	MarkReg(reg[0], data[0]);
#endif
}


typedef struct stdcall_s
{
	char	name[16];
	//void	(*cgfunc)();
	void (generator_c::* cgfunc) (tree_c* node, int op_cnt, int* ofs, tdatai_t* data, regi_t* reg);
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

void generator_c::CG_Call(tree_c* node, int op_cnt, int* ofs, tdatai_t* data, regi_t* reg)
{
	const char* funcname;
	const char* str = "";
	int			i;
	stdcall_t*	call;

	funcname = Str(node->Get(0));

	for (i = 0; (call = &stdcalls[i])->name[0]; i++)
	{
		if (!strcmp(funcname, call->name))
		{
			(this->*call->cgfunc)(node, op_cnt, ofs, data, reg);
			return;
		}
	}


	Error("Undeclared subroutine %s", funcname);
}