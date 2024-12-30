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
	stack_top--;

	if (parm && *parm)
		fprintf(f, "ret %s\n", parm);
	else
		fprintf(f, "ret\n"); //passed NULL or ""

	//spit out all the queued up data.
	if (*dataqueue)
	{
		fprintf(f, "%s", dataqueue);
		memset(dataqueue, 0, DATALUMP_SIZE);
	}
}

void generator_c::ASM_Data(const char* type, tree_c* var, const char* init)
{
	char lblname[KEY_MAX_LEN + 2] = {};
	data_t* data = &symtbl[symtbl_top];

	sprintf_s(lblname, "_%s:", var->Hash()->K());
	//fprintf(f, "%s\n%s %s\n", lblname, flags, init);

	//queue this up for writing after this function's ret statement
	strcat_s(dataqueue, lblname);
	strcat_s(dataqueue, "\n");
	strcat_s(dataqueue, type);
	strcat_s(dataqueue, " ");
	strcat_s(dataqueue, init);
	strcat_s(dataqueue, "\n");

	return;
	//add the variable
	data->var = var;
	data->func = stack[stack_top - 1];
	data->flags = DF_BYTE;
	data->val = atoi(init);
	symtbl_top++;
}

void generator_c::ASM_Data(const char* type, tree_c* var, int init)
{
	char buf[64];
	_itoa_s(init, buf, 10);
	ASM_Data(type, var, buf);
}

void generator_c::ASM_Load(const char* reg, tree_c* var)
{
	data_t*		data;
	data_t*		regdata;
	register_t* r;
	int			which;

	//if (data->flags == DT_WORD) //need to use bc, de, hl here
	//	Error("can't handle words yet");

	if (!reg || !*reg)
		Error("Tried loading into a bad register");

	r = SToReg(reg, &which);

	if (which == 2)
		Error("16-bit loads not supported");

	regdata = &r->held[which];

	//check if this register already holds this variable
	if (Linked(reg, var))
		return;

	//note: because of implicit initialization to 0 there are no initialization checks
	if (var->Hash()->V() == CODE_TEXT)
	{
		data = SymbolEntry(var);

		if (!data)
			Error("%s is undeclared or inaccessible", var->Hash()->K());


		fprintf(f, "ld %s, (_%s)\n", reg, var->Hash()->K());
		*regdata = *data; //snapshot the symbol entry
		
		//let the symbol remember that this reg holds it right now
		data->flags |= RegStrToDF(reg);
	}
	else if (var->Hash()->V() == CODE_NUM_DEC)
	{//constant
		fprintf(f, "ld %s, %s\n", reg, Str(var));

		regdata->func = NULL; //this value can be used anywhere
		regdata->var = NULL;
		regdata->val = atoi(Str(var));
		regdata->flags = DF_BYTE;
	}

}

void generator_c::ASM_Load(const char* reg, int val)
{
	register_t* r = NULL;
	int			which;
	data_t*		held;

	if (reg && !*reg)
		Error("Tried loading into a bad register");

	fprintf(f, "ld %s, %i\n", reg, val);

	r = SToReg(reg, &which);

	if (which == 2)
		Error("16-bit loads not supported");

	held = &r->held[which];
	held->func = NULL; //this value can be used anywhere
	held->var = NULL;
	held->val = val;
	held->flags = DF_BYTE;
}

//register to register loading
void generator_c::ASM_Load(const char* dst, const char* src)
{
	data_t*		sheld = NULL, * data;
	tree_c*	var = NULL;
	unsigned	df;

	if (dst[1] || src[1])
		Error("16-bit loading not yet supported");

	fprintf(f, "ld %s, %s\n", dst, src);

	switch (*src)
	{
	case 'a': sheld = &af->held[0]; break;
	case 'b': sheld = &bc->held[0];	break;
	case 'c': sheld = &bc->held[1];	break;
	case 'd': sheld = &de->held[0];	break;
	case 'e': sheld = &de->held[1];	break;
	case 'h': sheld = &hl->held[0];	break;
	case 'l': sheld = &hl->held[1];	break;
	default: Error("Tried loading from bad register '%s'", src); break;
	}

	var = sheld->var;

	switch (*dst)
	{
	case 'a': af->held[0] = *sheld; df = DF_A; break;
	case 'b': bc->held[0] = *sheld; df = DF_B; break;
	case 'c': bc->held[1] = *sheld; df = DF_C; break;
	case 'd': de->held[0] = *sheld; df = DF_D; break;
	case 'e': de->held[1] = *sheld; df = DF_E; break;
	case 'h': hl->held[0] = *sheld; df = DF_H; break;
	case 'l': hl->held[1] = *sheld; df = DF_L; break;
	default: Error("Tried loading into bad register '%s'", dst); break;
	}

	if (var)
	{//the source register had a variable in mem. Let the corresponding symbol know the dest reg has it now, too
		data = SymbolEntry(var);
		data->flags |= df;
	}
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

	fprintf(f, "ld (_%s), %s\n", var->Hash()->K(), reg);

	data->flags &= ~DF_REGMASK; //clear out any registers this symbol has flagged
	data->flags |= RegStrToDF(reg);
	//have the register hold this flag?

	r = SToReg(reg, &which);
	r->held[0] = *data;
	/*
	r->held[0].var = var;
	r->held[0].val = 0;
	r->held[0].func = 0;
	r->held[0].flags = DF_BYTE;
	*/
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