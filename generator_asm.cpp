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
	

	
}

void generator_c::ASM_Data(const char* type, tree_c* var, int init)
{
	char buf[64];
	char lblname[KEY_MAX_LEN + 2] = {};


	_itoa_s(init, buf, 10);

	sprintf_s(lblname, "_%s:", var->Hash()->K());

	//queue this up for writing after this function's ret statement
	strcat_s(dataqueue, lblname);
	strcat_s(dataqueue, "\n");
	strcat_s(dataqueue, type);
	strcat_s(dataqueue, " ");
	strcat_s(dataqueue, buf);
	if(type[2] == 'w' && init <= 255)
		strcat_s(dataqueue, ", 0"); //CHECKME
	strcat_s(dataqueue, "\n");
}

void generator_c::ASM_DLoad(regi_t reg, tdatai_t data)
{
	fprintf(f, "\tld\t%s, (_%s)\n", RegToS(reg), DataName(data));
}

void generator_c::ASM_RLoad(regi_t dst, regi_t src)
{
	fprintf(f, "\tld\t%s, %s\n", RegToS(dst), RegToS(src));
}

void generator_c::ASM_CLoad(regi_t reg, int value)
{
	fprintf(f, "\tld\t%s, %i\n", RegToS(reg), value);
}

void generator_c::ASM_ALoad(regi_t reg, tdatai_t data)
{
	fprintf(f, "\tld\t%s, _%s\n", RegToS(reg), DataName(data));
}


void generator_c::ASM_Store(tdatai_t var, regi_t reg)
{
	//fprintf(f, "ld\t(_%s), %s\n", var->Hash()->K(), reg);
	fprintf(f, "\tld\t(_%s), %s\n", DataName(var), RegToS(reg));
}

void generator_c::ASM_RAdd(regi_t dst_reg, regi_t src_reg)
{
	if (dst_reg == REG_A && src_reg == REG_HL)
	{//indirect add
		fprintf(f, "\tadd\ta, (hl)\n");

		return;
	}

	fprintf(f, "\tadd\t%s, %s\n", RegToS(dst_reg), RegToS(src_reg));
}

void generator_c::ASM_CAdd(regi_t dst_reg, int value)
{
	fprintf(f, "\tadd\t%s, %i\n", RegToS(dst_reg), value);
}

void generator_c::ASM_Djnz(const char* label)
{
	fprintf(f, "\tdjnz\t%s\n", label);
}

void generator_c::ASM_Push(regi_t reg)
{
	fprintf(f, "\tpush\t%s\n", RegToS(reg));
}

void generator_c::ASM_Pop(regi_t reg)
{
	fprintf(f, "\tpop\t%s\n", RegToS(reg));
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