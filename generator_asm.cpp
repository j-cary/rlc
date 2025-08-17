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
	char real_hex[17]; //0 - FFFFFFFFFFFFFFFF
	char final_hex[18]; //XXXXh
	size_t digits_used;
	size_t digits_max;
	char lblname[KEY_MAX_LEN + 2] = {};

	_itoa_s(init, real_hex, 16); //determine the real hex string
	digits_used = strlen(real_hex);

	if (type[2] == 'b')
	{//byte
		if (digits_used > 2)
			Error("Truncation required in byte decl");

		digits_max = 2;
	}
	//else if (type[2] == 'd') {// double word} //fixme: also fixed
	else
	{//word
		if (digits_used > 4)
			Error("Truncation required in word decl");
		digits_max = 4;
	}

	for (int i = 0; i < digits_max - digits_used; i++)
		final_hex[i] = '0'; //prefix the real number with the appropriate # of 0's. Necessary since .dw 00 only allocates one byte (it must at least be .dw 000h)
	
#pragma warning(suppress:4996) //sprintf_s only accepts an array as an arg, not a pointer
	sprintf(&final_hex[digits_max - digits_used], "%sh", real_hex); //append the real number with 'h'

	sprintf_s(lblname, "_%s:", var->Hash()->K());

	//queue this up for writing after this function's ret statement
	strcat_s(dataqueue, lblname);
	strcat_s(dataqueue, "\n");
	strcat_s(dataqueue, type);
	strcat_s(dataqueue, " ");
	strcat_s(dataqueue, final_hex);
	strcat_s(dataqueue, "\n");
}

void generator_c::ASM_DLoad(regi_t reg, tdatai_t data)
{
	fprintf(f, "\tld\t%s, (_%s)\n", RegToS(reg), DataName(data));
}

#if OLD_REG_CODE
void generator_c::ASM_RLoad(regi_t dst, regi_t src)
{
	fprintf(f, "\tld\t%s, %s\n", RegToS(dst), RegToS(src));
}

void generator_c::ASM_CLoad(regi_t reg, int value)
{
	fprintf(f, "\tld\t%s, %i\n", RegToS(reg), value);
}
#endif

void generator_c::ASM_ALoad(regi_t reg, tdatai_t data)
{
	fprintf(f, "\tld\t%s, _%s\n", RegToS(reg), DataName(data));
}


void generator_c::ASM_Store(tdatai_t var, regi_t reg)
{
	//fprintf(f, "ld\t(_%s), %s\n", var->Hash()->K(), reg);
	fprintf(f, "\tld\t(_%s), %s\n", DataName(var), RegToS(reg));
}

#if OLD_REG_CODE
void generator_c::ASM_RAdd(regi_t dst_reg, regi_t src_reg)
{
	if (dst_reg == REG_A && src_reg == REG_HL)
	{//indirect add
		fprintf(f, "\tadd\ta, (hl)\n");

		return;
	}

	fprintf(f, "\tadd\t%s, %s\n", RegToS(dst_reg), RegToS(src_reg));
}
#endif

void generator_c::ASM_CAdd(regi_t dst_reg, int value)
{
	fprintf(f, "\tadd\t%s, %i\n", RegToS(dst_reg), value);
}

void generator_c::ASM_Djnz(const char* label)
{
	fprintf(f, "\tdjnz\t%s\n", label);
}

#if OLD_REG_CODE
void generator_c::ASM_Push(regi_t reg)
{
	fprintf(f, "\tpush\t%s\n", RegToS(reg));
}

void generator_c::ASM_Pop(regi_t reg)
{
	fprintf(f, "\tpop\t%s\n", RegToS(reg));
}

#else

//
//loads
//

void generator_c::ASM_RLoad(REG::REG dst, REG::REG src, int width)
{
	fprintf(f, "\tld\t%s, %s\n", REG::Str(dst, width), REG::Str(src, width));
}


void generator_c::ASM_RLoad(REG::REG dst, REG::REG src, int width, int ofs)
{
	if (dst == REG::IXH || dst == REG::IYH)
	{
		fprintf(f, "\tld\t(%s+%i), %s\n", REG::Str(dst, 2), ofs, REG::Str(src, 2));
	}
	else if (src == REG::IXH || src == REG::IYH)
	{
		fprintf(f, "\tld\t%s, (%s+%i)\n", REG::Str(dst, 2), REG::Str(src, 2), ofs);
	}
	else
		Error("PROGRAMMER ERROR: bad index register rload");
}

void generator_c::ASM_CLoad(REG::REG reg, int value, int width)
{
	if (width == 1)
	{//a, b, c, d, e, h, l, ixh, ixl, iyh, iyl
	}
	else if (width == 2)
	{//bc, de, hl, sp, ix, iy - no af
		if (reg == REG::A)
			Error("PROGRAMMER ERROR: af cannot be loaded into");
	}
	else
		Error("PROGRAMMER ERROR: a constant load of width '%i' is not allowed", width);


	fprintf(f, "\tld\t%s, %i\n", REG::Str(reg, width), value);
}

//
//adds
//

void generator_c::ASM_RAdd(REG::REG dst_reg, REG::REG src_reg, int width)
{
	if (width == 1)
	{//a
		if (dst_reg != REG::A)
			Error("PROGRAMMER ERROR: a byte register add destination must be 'a'");
		//a,b,c,d,e,h,l,(hl),ixl,ixh,iyl,iyh,(ix+d),(iy+d)
		//TODO check this
	}
	else if (width == 2)
	{
		//bc, de, self, sp
		if(dst_reg != REG::H && dst_reg != REG::IXH && dst_reg != REG::IYL)
			Error("PROGRAMMER ERROR: a word register add destination must be hl, ix, or iy");

		//hl, ix, iy
		if (src_reg != REG::B && src_reg != REG::D && src_reg != REG::IYL && src_reg != src_reg && src_reg != REG::SP)
			Error("PROGRAMMER ERROR: a word register add source must be bc, de, 'self', or sp");
	}
	else
		Error("PROGRAMMER ERROR: a register add of width '%i' is not allowed", width);

	fprintf(f, "\tadd\t%s, %s\n", REG::Str(dst_reg, width), REG::Str(src_reg, width));
}

//
//inc/dec
//

void generator_c::ASM_Dec(REG::REG reg, int width)
{
	fprintf(f, "\tdec\t%s\n", REG::Str(reg, width));
}

//
// Stack
//

//FIXME: handle struct init info
//TODO: the catting really should be replaced with copying
//Handles initialization of bytes/words on the stack
void generator_c::ASM_StackInit(int value, int ofs, int width)
{
	char buf[256]; //ought to be big enough

	for (int i = 0; i < width; i++)
	{
		int v = (value >> (i * 8)) & 0xFF;
		int len = sprintf_s(buf, "\tld\t(ix+%i), %i\n", ofs + i, v);
		strcat_s(stack_queue, buf); 
	}
	
}

void generator_c::ASM_Push(REG::REG reg)
{
	fprintf(f, "\tpush\t%s\n", REG::Str(reg, 2));
}

void generator_c::ASM_Pop(REG::REG reg)
{
	fprintf(f, "\tpop\t%s\n", REG::Str(reg, 2));
}

#endif





void generator_c::InitFile(const char* filename)
{
	fopen_s(&f, filename, "w+");

	if (!f)
		Error("Couldn't open assembler file %s", filename);

	fprintf(f, asmheader);
}

void generator_c::PrintFile()
{
	char line[1024] = {};

	rewind(f);
	while (fgets(line, sizeof(line), f))
		printf("%s", line);
}