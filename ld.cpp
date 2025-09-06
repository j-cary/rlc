#include "generator.h"
#include "evaluate_expression.h"

static asm_c* sm;
static eval_expr_c eval_expr;

#pragma region hl_src

/* 
*  src=HL
*/

static INSTR_GEN_FUNC(hl2hl)
{

}

static INSTR_GEN_FUNC(hl2reg)
{

}

static INSTR_GEN_FUNC(hl2stack)
{

}

static INSTR_GEN_FUNC(hl2auto)
{

}

#pragma endregion hl_src

#pragma region reg_src

/*
*  src=reg
*/


static INSTR_GEN_FUNC(reg2hl)
{

}

static INSTR_GEN_FUNC(reg2reg)
{
	const tdata_t* const src = info[cnt - 1].data;
	const tdata_t* const dst = info[cnt - 2].data;

	ASSERT_WARN(dst->size >= src->size, "Truncation required in load %s <- %s", dst->var->Str(), src->var->Str());

	if (dst->size == 1 && src->size == 1)
	{
		sm->RLoad((REG::REG)dst->si.reg, (REG::REG)src->si.reg, 1);
	}
	else if (dst->size == 1 && src->size == 2)
	{
		//Load the low byte
		sm->RLoad((REG::REG)dst->si.reg, (REG::REG)(src->si.reg + 1), 1); //Should check that this is valid
	}
	else if (dst->size == 2 && src->size == 1)
	{
		//Load the low byte
		sm->RLoad((REG::REG)(dst->si.reg + 1), (REG::REG)src->si.reg, 1); //Should check that this is valid

		//FIXME: how about handling signed stuff here? what if the ops have different sign types?
		sm->Xor(REG::A);
		sm->RLoad((REG::REG)dst->si.reg, REG::A, 1);
		
	}
	else if (dst->size == 2 && src->size == 2)
	{
		sm->RLoad((REG::REG)dst->si.reg, (REG::REG)src->si.reg, 1);
		sm->RLoad((REG::REG)(dst->si.reg + 1), (REG::REG)(src->si.reg + 1), 1); //Should check that this is valid
	}
	else
	{
		INTERNAL_ASSERT(0, "Bogus register sizes in load %s <- %s");
	}

}

static INSTR_GEN_FUNC(reg2stack)
{

}

static INSTR_GEN_FUNC(reg2auto)
{

}

#pragma endregion reg_src

#pragma region stack_src

/*
*  src=stack
*/


static INSTR_GEN_FUNC(stack2hl)
{

}

static INSTR_GEN_FUNC(stack2reg)
{

}

static INSTR_GEN_FUNC(stack2stack)
{

}

static INSTR_GEN_FUNC(stack2auto)
{

}

#pragma endregion stack_src

#pragma region auto_src

/*
*  src=auto
*/


static INSTR_GEN_FUNC(auto2hl)
{

}

static INSTR_GEN_FUNC(auto2reg)
{
	
}

static INSTR_GEN_FUNC(auto2stack)
{

}

static INSTR_GEN_FUNC(auto2auto)
{

}

#pragma endregion auto_src

#pragma region const_src

/*
*  src=const
*/

static INSTR_GEN_FUNC(const2hl)
{
	const tree_c* src = info[cnt - 1].node;
	const tdata_t* dst = info[cnt - 2].data;
	const int min = ((dst->flags | DF_SIGNED) ? INT16_MIN : 0);
	const int max = ((dst->flags | DF_SIGNED) ? INT16_MAX : UINT16_MAX);
	//const int const_val = Constant_Expression(src);
	const int const_val = eval_expr.Constant(src);

	ASSERT_WARN(const_val >= min && const_val <= max, "Truncation required from %i", const_val);

	sm->CLoad((REG::REG)dst->si.reg, const_val, dst->size);
}

static INSTR_GEN_FUNC(const2reg)
{
	const tree_c* src = info[cnt - 1].node;
	const tdata_t* dst = info[cnt - 2].data;
	const int const_val = eval_expr.Constant(src);
	int min;
	int max;

	if (dst->size == 1)
	{
		min = ((dst->flags | DF_SIGNED) ? INT8_MIN : 0);
		max = ((dst->flags | DF_SIGNED) ? INT8_MAX : UINT8_MAX);
	}
	else
	{
		min = ((dst->flags | DF_SIGNED) ? INT16_MIN : 0);
		max = ((dst->flags | DF_SIGNED) ? INT16_MAX : UINT16_MAX);
	}

	ASSERT_WARN(const_val >= min && const_val <= max, "Truncation required from %i while loading to %s", const_val, dst->var->Str());

	sm->CLoad((REG::REG)dst->si.reg, eval_expr.Constant(src), dst->size);
}

static INSTR_GEN_FUNC(const2stack)
{
	const tdata_t* dst = info[cnt - 2].data;
	int const_val = eval_expr.Constant(info[cnt - 1].node);

	//FIXME: check for truncation here

	//Load into the stack byte by byte
	for (int i = 0; i < dst->size; ++i)
	{
		sm->CLoad(REG::A, (const_val >> (i*8)) & 0xFF, 1);
		sm->RLoad(REG::IXH, REG::A, 1, (dst->si.stack) + i);
	}
}

static INSTR_GEN_FUNC(const2auto)
{
	const tdata_t* dst = info[cnt - 2].data;
	int const_val = eval_expr.Constant(info[cnt - 1].node);

	//Store byte by byte
	for (int i = 0; i < dst->size; ++i)
	{
		sm->CLoad(REG::A, (const_val >> (i*8)) & 0xFF, 1);
		sm->Store(dst->si.local + i, REG::A);
	}
}

#pragma endregion const_src

#pragma region entry

//FIXME: this should guarantee stuff that can't be null isn't
static instr_gen_table_t ld_table =
{//			dst =>
/*src ||  */{hl2hl,		hl2reg,		hl2stack,	hl2auto},
/*	  \/  */{reg2hl,	reg2reg,	reg2stack,	reg2auto},
			{stack2hl,	stack2reg,	stack2stack,stack2auto},
			{auto2hl,	auto2reg,	auto2stack, auto2auto},
			{const2hl,	const2reg,	const2stack,const2auto},
};

void generator_c::CG_Load(INSTR_GEN_FUNC_ARGS)
{
	enum { BAD = -1, HL = 0, REG = 1, STACK = 2, AUTO = 3, CONST = 4 } 
	src_tbl = BAD, dst_tbl = BAD;
	instr_gen_func_t func = NULL;
	const tdata_t* src = info[cnt - 1].data;

	sm = &assembler;

	//Determine the source index
	if (!src)
		src_tbl = CONST;
	else if (src->si.reg_flag)
		src_tbl = ((src->si.reg == REG::H && src->size == 2) ? HL : REG);
	else if (src->si.stack_flag)
		src_tbl = STACK;
	else if (src->si.local_flag)
		src_tbl = AUTO;

	INTERNAL_ASSERT(src_tbl != BAD, "Failed to determine storage of %s", src->var->Str());

	//TMP: just do 2 ops for right now
	for (int i = cnt - 2; i > cnt - 3 /*0*/; --i)
	{
		const tdata_t* dst = info[i].data;

		INTERNAL_ASSERT(dst, "Failed to determine storage dest operand");
		if (dst->si.reg_flag)
			dst_tbl = ((dst->si.reg == REG::H && dst->size == 2) ? HL : REG);
		else if (dst->si.stack_flag)
			dst_tbl = STACK;
		else if (dst->si.local_flag)
			dst_tbl = AUTO;

		INTERNAL_ASSERT(dst_tbl != BAD, "Failed to determine storage of %s", dst->var->Str());

		(*ld_table[src_tbl][dst_tbl]) (info, cnt);
	}

}

#pragma endregion entry