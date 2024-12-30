#include "generator.h"

//
//UTILITY FUNCTIONS
//

data_t* generator_c::SymbolEntry(tree_c* symb)
{
	//TODO: check for function here. i.e. if this symbol is inaccessible

	for (unsigned i = 0; i < symtbl_top; i++)
	{
		if (!strcmp(symb->Hash()->K(), symtbl[i].var->Hash()->K()))
			return &symtbl[i];
	}

	return NULL;
}

void generator_c::UpdateSymbol(tree_c* symb, int set)
{
	for (unsigned i = 0; i < symtbl_top; i++)
	{
		if (!strcmp(symb->Hash()->K(), symtbl[i].var->Hash()->K()))
		{
			symtbl[i].val = set;
		}
	}
}

//which: 0 - hi, 1 - low, 2 - full
const char* generator_c::RegToS(register_t* r, int which)
{
	if (r == af)
	{
		switch (which)
		{
		case 0: return "a"; break;
		case 1: Error("Attempted to access f"); break;
		case 2: return "af"; break;
		default: Error("Attempted to access non-existent register slot"); break;
		}
	}
	else if (r == bc)
	{
		switch (which)
		{
		case 0: return "b"; break;
		case 1: return "c"; break;
		case 2: return "bc"; break;
		default: Error("Attempted to access non-existent register slot"); break;
		}
	}
	else if (r == de)
	{
		switch (which)
		{
		case 0: return "d"; break;
		case 1: return "e"; break;
		case 2: return "de"; break;
		default: Error("Attempted to access non-existent register slot"); break;
		}
	}
	else if (r = hl)
	{
		switch (which)
		{
		case 0: return "h"; break;
		case 1: return "l"; break;
		case 2: return "hl"; break;
		default: Error("Attempted to access non-existent register slot"); break;
		}
	}

	return NULL;
}

const char* generator_c::RegToS(int r, int which)
{
	return RegToS(&regs[r], which);
}

register_t* generator_c::SToReg(const char* reg, int* w)
{
	register_t* r = NULL;
	int			which;

	switch (reg[0])
	{
	case 'a':
		r = af;
		if (reg[1] == 'f')
			which = 2;
		else
			which = 0;
		break;
	case 'f': Error("Tried loading into f"); break;
	case 'b':
		r = bc;
		if (reg[1] == 'c')
			which = 2;
		else
			which = 0;
		break;
	case 'c': r = bc; which = 1; break;
	case 'd':
		r = de;
		if (reg[1] == 'e')
			which = 2;
		else
			which = 0;
		break;
	case 'e': r = de; which = 1; break;
	case 'h':
		r = hl;
		if (reg[1] == 'j')
			which = 2;
		else
			which = 0;
		break;
	case 'l': r = hl; which = 1; break;
	default: Error("Tried loading into a bad register"); break;
	}

	*w = which;
	return r;
}

inline int generator_c::Code(tree_c* node)
{
	return node->Hash()->V();
}

inline const char* generator_c::Str(tree_c* node)
{
	if (!node)
		return "";
	return node->Hash()->K();
}

int generator_c::Linked(const char* reg, tree_c* var)
{
	register_t* r;
	int			which;
	data_t*		regdata, * data;

	r = SToReg(reg, &which);
	
	return Linked(r, which, var);
}

int generator_c::Linked(register_t* reg, int which, tree_c* var)
{
	data_t* regdata, * data;

	if (which == 2)
		Error("16-bit loads not supported");

	regdata = &reg->held[which];

	if (!TreeCmp(regdata->var, var))
	{
		unsigned bits;
		data = SymbolEntry(var);
		bits = RegToDF(reg, which);

		//does the variable remember this register?
		if (data->flags & bits)
			return true;
	}

	return false;
}

int generator_c::TreeCmp(tree_c* t1, tree_c* t2)
{
	if (!t1 || !t2)
		return 1;

	return strcmp(Str(t1), Str(t2));
}

int generator_c::Constant_Expression(tree_c* head)
{
	int		kids;
	int		ret = 0;
	int		op_code;

	for (kids = 0; head->Get(kids); kids++);

	switch (Code(head))
	{
	case CODE_NUM_DEC:				ret = atoi(Str(head)); break;
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
	case CODE_NUM_HEX:
		Error("Binary and hex numbers aren't supported in const. exprs. yet");
		break;
	case CODE_TEXT:
		Error("Text value found in const. expr");
	default:
		Error("Bad expression");
		break;
	}

	return ret;
}

int generator_c::Constant_Expression_Helper(tree_c* head, int num_kids,  int offset, int type)
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




unsigned generator_c::RegStrToDF(const char* reg)
{
	unsigned flag = 0;

	switch (reg[0])
	{
	case 'a': flag |= DF_A; break;
	case 'b': flag |= DF_B; break;
	case 'c': flag |= DF_C; break;
	case 'd': flag |= DF_D; break;
	case 'e': flag |= DF_E; break;
	case 'h': flag |= DF_H; break;
	case 'l': flag |= DF_L; break;
	default: break;
	}

	switch (reg[1])
	{
	case 'a': flag |= DF_A; break;
	case 'b': flag |= DF_B; break;
	case 'c': flag |= DF_C; break;
	case 'd': flag |= DF_D; break;
	case 'e': flag |= DF_E; break;
	case 'h': flag |= DF_H; break;
	case 'l': flag |= DF_L; break;
	default: break;
	}

	return flag;
}

unsigned generator_c::RegToDF(register_t* reg, unsigned which)
{
	unsigned flag = 0;
	if (which == 0)
	{//hi
		if (reg == af)
			flag |= DF_A;
		else if (reg == bc)
			flag |= DF_B;
		else if (reg == de)
			flag |= DF_D;
		else if (reg == hl)
			flag |= DF_H;
	}
	else if (which == 1)
	{//low
		if (reg == af)
			Error("Can't use f");
		else if (reg == bc)
			flag |= DF_C;
		else if (reg == de)
			flag |= DF_E;
		else if (reg == hl)
			flag |= DF_L;
	}
	else
		Error("No 16-bit loads");

	return flag;
}