#include "generator.h"

const char* asmheader = ".nolist\n"
"#include \"../inc/ti83p.inc\"\n"
".list\n"
".org\tuserMem - 2\n"
".db\tt2ByteTok, tAsmCmp\n\n";

//
//ASSEMBLER FILE FUNCTIONS
//
void generator_c::ASM_Label(const char* name)
{
	fprintf(f, name);
	fprintf(f, ":\n");
}

void generator_c::ASM_Ret(const char* parm)
{
	if (parm && *parm)
		fprintf(f, "\tret\t%s\n", parm);
	else
		fprintf(f, "\tret\n"); //passed NULL or ""
}

void generator_c::ASM_Data(const char* type, tree_c* var, const char* init)
{
	char lblname[KEY_MAX_LEN + 2] = {};
	data_t* data = &symtbl[symtbl_top];

	//Do we need to make a data entry?
	 //graph->DataName

	sprintf_s(lblname, "_%s:", var->Hash()->K());

	//queue this up for writing after this function's ret statement
	strcat_s(dataqueue, lblname);
	strcat_s(dataqueue, "\n");
	strcat_s(dataqueue, type);
	strcat_s(dataqueue, " ");
	strcat_s(dataqueue, init);
	strcat_s(dataqueue, "\n");
}

void generator_c::ASM_Data(const char* type, tree_c* var, int init)
{
	char buf[64];
	_itoa_s(init, buf, 10);
	ASM_Data(type, var, buf);
}

void generator_c::ASM_DLoad(cfg_c* block, regi_t reg, paralleli_t data)
{
	//mark the reg as holding this var
	//MarkReg(reg, data);
	fprintf(f, "\tld\t%s, (_%s)\n", RegToS(reg), block->DataName(data));
}

void generator_c::ASM_RLoad(regi_t dst, regi_t src)
{
	fprintf(f, "\tld\t%s, %s\n", RegToS(dst), RegToS(src));
}

void generator_c::ASM_CLoad(regi_t reg, int value)
{
	fprintf(f, "\tld\t%s, %i\n", RegToS(reg), value);
}


void generator_c::ASM_Store(tree_c* var, const char* reg)
{
	data_t*		data;
	register_t* r;
	int			which;

	if (!reg || !*reg)
		return;

	if (Code(var) != CODE_TEXT)
		Error("Tried loading into non-sensical location %s", Str(var));

	if (!(data = SymbolEntry(var)))
		Error("%s is undeclared or inaccessible", Str(var));

	fprintf(f, "ld\t(_%s), %s\n", var->Hash()->K(), reg);

	/*
	data->flags &= ~DF_REGMASK; //clear out any registers this symbol has flagged
	data->flags |= RegStrToDF(reg);
	//have the register hold this flag?

	r = SToReg(reg, &which);
	r->held[0] = *data;
	r->held[0].var = var;
	r->held[0].val = 0;
	r->held[0].func = 0;
	r->held[0].flags = DF_BYTE;
	*/
}

void generator_c::ASM_Add(regi_t dst_reg, regi_t src_reg)
{
	fprintf(f, "\tadd\t%s, %s\n", RegToS(dst_reg), RegToS(src_reg));
}

void generator_c::ASM_Djnz(const char* label)
{
	fprintf(f, "\tdjnz\t%s\n", label);
}




void generator_c::InitFile(const char* filename)
{
	fopen_s(&f, filename, "w+");

	if (!f)
		Error("Couldn't open assembler file %s\n", filename);

	fprintf(f, asmheader);
}

void generator_c::PrintFile()
{
	char line[1024] = {};

	rewind(f);
	while (fgets(line, sizeof(line), f))
		printf("%s", line);
}