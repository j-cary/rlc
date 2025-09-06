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
	const data_t* data = block->ScopedDataEntry(Str(node), graph, graph, &localblock); //recurse through the graph, find the ONLY matching data in scope.

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

int generator_c::GetForLabel(char* buf)
{
	static int count = 0;
	const char* base = "for%i";
	int len = (int)strlen(base);

	sprintf_s(buf, len, base, count);

	return count++;
}