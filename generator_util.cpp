#include "generator.h"

//
//UTILITY FUNCTIONS
//

void asm_c::PrintSourceLine(tree_c* node)
{
	tree_c* child;

	fprintf(f, "; ");

	for (int i = 0; child = node->Get(i); i++)
		R_PrintSourceLine(child);
	fprintf(f, "\n");
}

void asm_c::R_PrintSourceLine(tree_c* node)
{
	tree_c* child;

	if (!node->Get(0))
	{//leaf
		fprintf(f, "%s ", node->Hash()->K());
	}


	for (int i = 0; child = node->Get(i); i++)
		R_PrintSourceLine(child);
}

char* generator_c::RegToS(regi_t reg)
{
	const char* name = "";
	const int	max_strs = 32;
	const int	longest = 6; //longest name is wr127\0

	static char str[max_strs][longest];
	static int	strcnt = 0;

	strcnt = strcnt % max_strs;

#if OLD_REG_CODE
	switch (reg)
	{
	case REG_A: name = "a"; break;
	case REG_B: name = "b"; break;
	case REG_C: name = "c"; break;
	case REG_D: name = "d"; break;
	case REG_E: name = "e"; break;
	case REG_H: name = "h"; break;
	case REG_L: name = "l"; break;

	case REG_IYH: name = "iyh"; break;
	case REG_IYL: name = "iyl"; break;
	case REG_IXH: name = "ixh"; break;
	case REG_IXL: name = "ixl"; break;

	case REG_BC: 
	case REG_BC + 1:
		name = "bc"; break;
	case REG_DE: 
	case REG_DE + 1: 
		name = "de"; break;
	case REG_HL: 
	case REG_HL + 1: 
		name = "hl"; break;

	case REG_IX: name = "ix"; break;
	case REG_IY: name = "iy"; break;

	default: break;
	}

	if (reg >= REG_R0 && reg <= REG_R255)
	{
		regi_t i = reg - REG_R0;
		snprintf(str[strcnt], longest, "r%i", i);
	}
	else if (reg >= REG_WR0 && reg <= REG_WR127)
	{
		regi_t i = (reg - REG_WR0) / 2; //also allow 1 + reg
		snprintf(str[strcnt], longest, "wr%i", i);
	}
	else if (*name)
		strncpy_s(str[strcnt], name, longest);
	else
		Error("Bad reg");

	return str[strcnt++];

#else



#endif
	return NULL;
}

regi_t generator_c::RegAlloc(tdatai_t index)
{
#if OLD_REG_CODE
	regi_t color = (*igraph)[index]->color;
	return color ;
#endif
	return 0;
}

regi_t generator_c::HiByte(regi_t r)
{
#if OLD_REG_CODE
	return r - REG_IXL;
#endif
	return 0;
}

regi_t generator_c::LoByte(regi_t r)
{
	return HiByte(r) + 1;
}

void generator_c::RegFree(regi_t r)
{
#if OLD_REG_CODE
	if (r <= REG_IXL)
	{//direct index
		regs[r].held = -1;
	}
	else
	{
		int i = ((r - REG_BC) / 2) + REG_BC;
		regs[i].held = -1;

		//also held the corresponding 8-bit regs
		regs[r - REG_BC + 1].held = -1;
		regs[r - REG_BC + 2].held = -1;
	}
#endif
}

void generator_c::MarkReg(regi_t reg, tdatai_t data)
{
#if OLD_REG_CODE
	if (reg <= REG_IXL)
	{//direct index
		regs[reg].held = data;
	}
	else
	{
		int i = ((reg - REG_BC) / 2) + REG_BC;
		regs[i].held = data;

		//also held the corresponding 8-bit regs
		regs[reg - REG_BC + 1].held = data;
		regs[reg - REG_BC + 2].held = data;
	}
#endif
}

bool generator_c::IsMarked(regi_t reg, tdatai_t data)
{
#if OLD_REG_CODE
	if (reg <= REG_IXL)
	{//direct index
		if (regs[reg].held == data)
			return true;
	}
	else
	{
		/*
		int i = ((reg - REG_BC) / 2) + REG_BC;
		regs[i].held = data;

		//also held the corresponding 8-bit regs
		regs[reg - REG_BC + 1].held = data;
		regs[reg - REG_BC + 2].held = data;
		*/
	}
#endif
	return false;
}

//Returns NULL if the var is unused
tdata_t* generator_c::Data(cfg_c* block, tree_c* n)
{
	return &tdata[DataOfs(block, n)];
}

tdatai_t generator_c::DataOfs(cfg_c* block, tree_c* node)
{
	cfg_c* localblock = NULL;
	data_t* data = block->ScopedDataEntry(Str(node), graph, graph, &localblock); //recurse through the graph, find the ONLY matching data in scope.

	//fixme: first graph parm should be the current function

	return data->tdata;

	//plan: 
	// tdata only needs to be in dataname - the indices will suffice elsewhere
	//fix asm_dload
}
/*
colori_t generator_c::DataColor(tdatai_t index)
{
	return (*igraph)[index]->color;
}
*/

const char* generator_c::DataName(tdatai_t data)
{
	if (data < 0 || data >= (tdatai_t)symtbl_top)
		Error("Bad data index %i", data);

	return tdata[data].var->K();
}



int generator_c::Code(tree_c* node)
{
	return node->Hash()->V();
}

const char* generator_c::Str(tree_c* node)
{
	if (!node)
		return "BADNODE";
	return node->Hash()->K();
}



static int Constant_Expression_Helper(const tree_c* head, int num_kids, int offset, int type)
{
	int	r1, r2, ret = 0;
	int	op_code;

	r1 = Constant_Expression(head->Get(offset));
	op_code = head->Get(offset + 1)->Hash()->V();
	r2 = Constant_Expression(head->Get(offset + 2));

	if (type == 0)
	{
		switch (op_code)
		{
		case T_LEFT_SHIFT: ret = r1 << r2; break;
		case T_RIGHT_SHIFT: ret = r1 >> r2; break;
		default: Error("Bad shift operator"); break;
		}
	}
	else if (type == 1)
	{
		switch (op_code)
		{
		case CODE_PLUS: ret = r1 + r2; break;
		case CODE_MINUS: ret = r1 - r2; break;
		default: Error("Bad additive operator"); break;
		}
	}
	else if (type == 2)
	{
		switch (op_code)
		{
		case CODE_STAR: ret = r1 * r2; break;
		case CODE_FSLASH: ret = r1 / r2; break;
		case CODE_PERCENT: ret = r1 % r2; break;
		default: Error("Bad additive operator"); break;
		}
	}



	for (int i = offset + 3; i < num_kids; i += 2)
	{
		op_code = head->Get(i)->Hash()->V();
		r1 = Constant_Expression(head->Get(i + 1));

		if (type == 0)
		{
			switch (op_code)
			{
			case T_LEFT_SHIFT: ret <<= r1; break;
			case T_RIGHT_SHIFT: ret >>= r1; break;
			default: Error("Bad shift operator"); break;
			}
		}
		else if (type == 1)
		{
			switch (op_code)
			{
			case CODE_PLUS: ret += r1; break;
			case CODE_MINUS: ret -= r1; break;
			default: Error("Bad additive operator"); break;
			}
		}
		else if (type == 2)
		{
			switch (op_code)
			{
			case CODE_STAR: ret *= r1; break;
			case CODE_FSLASH: ret /= r1; break;
			case CODE_PERCENT: ret %= r1; break;
			default: Error("Bad additive operator"); break;
			}
		}
	}

	return ret;
}

int Constant_Expression(const tree_c* head)
{
	int		kids;
	int		ret = 0;
	int		op_code;

	for (kids = 0; head->Get(kids); kids++);

	switch (head->Hash()->V())
	{
	case CODE_NUM_DEC:				ret = atoi(head->Hash()->K()); break;
	case NT_SHIFT_EXPR:				ret = Constant_Expression_Helper(head, kids, 0, 0);break;
	case NT_ADDITIVE_EXPR:			ret = Constant_Expression_Helper(head, kids, 0, 1);break;
	case NT_MULTIPLICATIVE_EXPR:	ret = Constant_Expression_Helper(head, kids, 0, 2);break;
	case NT_ARITHMETIC_PRIMARY_EXPR:
		
		ret = Constant_Expression(head->Get(1));//skip over the (, *, &, etc.
		op_code = head->Get(0)->Hash()->V();

		if (op_code == CODE_MINUS)
			ret = -ret;
		else if (op_code == CODE_STAR || op_code == CODE_AMPERSAND)
			Error("referencing/de-referencing is not allowed in a constant expression");

		break;

	case NT_ARITHMETIC_POSTFIX_EXPR:
		Error("Postfix operators are not allowed in constant expressions");
		break;
	case CODE_NUM_BIN:
		ret = strtol(head->Hash()->K() + 1, NULL, 2);
		break;
	case CODE_NUM_HEX:
		ret = strtol(head->Hash()->K() + 1, NULL, 16);

		//Error("Binary and hex numbers aren't supported in const. exprs. yet");
		break;
	case CODE_TEXT:
		Error("Text value found in const. expr");
	default:
		Error("Bad expression");
		break;
	}

	return ret;
}



int generator_c::GetForLabel(char* buf)
{
	static int count = 0;
	const char* base = "for%i";
	int len = (int)strlen(base);

	sprintf_s(buf, len, base, count);

	return count++;
}