#include "generator.h"

void generator_c::SL_Print(tree_c* n, cfg_c* block, int op_cnt, int* ofs, colori_t* colors)
{//print, op1, op2, ..., opn;
	const char* str;

	//mem or const exprs. - figure out string operands

	printf("");
	
	str =	"\tld\ta, 0\n"
			"\tld\t(curRow), a\n"
			"\tld\t(curCol), a\n"
			"\tld\th, 0\n"
			//"\tld\ta, (_s)\n"
			"\tld\tl, b\n"
			"\tbcall(_DispHL)\n";

	fprintf(f, str);
	
}