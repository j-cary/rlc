#include "generator.h"

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

/*
*  src=reg
*/


static INSTR_GEN_FUNC(reg2hl)
{

}

static INSTR_GEN_FUNC(reg2reg)
{

}

static INSTR_GEN_FUNC(reg2stack)
{

}

static INSTR_GEN_FUNC(reg2auto)
{

}

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

static instr_gen_table_t ld_table =
{//			dst =>
/*src ||  */hl2hl,		hl2reg,		hl2stack,	hl2auto,
/*	  \/  */reg2hl,		reg2reg,	reg2stack,	reg2auto,
			stack2hl,	stack2reg,	stack2stack,stack2auto,
			auto2hl,	auto2reg,	auto2stack, auto2auto
};

void generator_c::CG_Load(INSTR_GEN_FUNC_ARGS)
{
	enum { BAD = -1, HL = 0, REG = 1, STACK = 2, AUTO = 3 } 
	src_tbl = BAD, dst_tbl = BAD;
	instr_gen_func_t func = NULL;
	const tdata_t* src = data[0];

	//Determine the source index
	if (src->si.reg_flag)
		src_tbl = ((src->si.reg == REG::H && src->size == 2) ? HL : REG);
	else if (src->si.stack_flag)
		src_tbl = STACK;
	else if (src->si.local_flag)
		src_tbl = AUTO;



	//tmp: just do 2 ops for right now
	for (int i = cnt - 1; i > cnt - 2 /*0*/; --i)
	{
		const tdata_t* dst = data[i];

		if (dst->si.reg_flag)
			dst_tbl = ((dst->si.reg == REG::H && dst->size == 2) ? HL : REG);
		else if (dst->si.stack_flag)
			dst_tbl = STACK;
		else if (dst->si.local_flag)
			dst_tbl = AUTO;


	}

}