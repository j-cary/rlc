#include "generator.h"

void asm_c::StackFrame()
{
	fprintf(f, "%s", stack_queue);
}

void asm_c::UnStackFrame()
{

}

void asm_c::UnQueueData()
{
	if (*dataqueue)
	{
		fprintf(f, "%s", dataqueue);
		memset(dataqueue, 0, DATALUMP_SIZE);
	}
}

//
//ASSEMBLER FILE FUNCTIONS
//
void asm_c::Label(const char* name)
{
	fprintf(f, name);
	fprintf(f, ":\n");
}

void asm_c::Ret(const char* parm)
{
	if (parm && *parm)
		fprintf(f, "\tret\t%s\n", parm);
	else
		fprintf(f, "\tret\n"); //passed NULL or ""
}

void asm_c::Data(const char* type, tree_c* var, const char* init)
{



}

void asm_c::Data(const char* type, tree_c* var, int init)
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
		ASSERT_WARN(digits_used <= 2, "Truncation required in byte decl");
		digits_max = 2;
	}
	//else if (type[2] == 'd') {// double word} //fixme: also fixed
	else
	{//word
		ASSERT_WARN(digits_used <= 4, "Truncation required in word decl");
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

void asm_c::DLoad(regi_t reg, tdatai_t data)
{
	//fprintf(f, "\tld\t%s, (_%s)\n", RegToS(reg), DataName(data));
}

void asm_c::ALoad(regi_t reg, tdatai_t data)
{
	//fprintf(f, "\tld\t%s, _%s\n", RegToS(reg), DataName(data));
}


void asm_c::Store(int ofs, REG::REG reg)
{
	INTERNAL_ASSERT(REG::H == reg || REG::B == reg || REG::D == reg || 
					REG::A == reg || REG::IXH == reg || REG::IYH == reg, 
					"Tried to store bad register %s", REG::Str(reg, 1));

	int size = (reg == REG::A) ? 1 : 2;

	fprintf(f, "\tld\t(data_start+%i), %s\n", ofs, REG::Str(reg, size));
}

void asm_c::CAdd(regi_t dst_reg, int value)
{
	//fprintf(f, "\tadd\t%s, %i\n", RegToS(dst_reg), value);
}

void asm_c::Djnz(const char* label)
{
	fprintf(f, "\tdjnz\t%s\n", label);
}

//
//loads
//

void asm_c::RLoad(REG::REG dst, REG::REG src, int width)
{
	fprintf(f, "\tld\t%s, %s\n", REG::Str(dst, width), REG::Str(src, width));
}


void asm_c::RLoad(REG::REG dst, REG::REG src, int width, int ofs)
{
	INTERNAL_ASSERT(width == 1, "Tried loading %s into %s with a bad width", REG::Str(src, 2), REG::Str(dst, 1));

	if (dst == REG::IXH || dst == REG::IYH)
	{
		fprintf(f, "\tld\t(%s+%i), %s\n", REG::Str(dst, 2), ofs, REG::Str(src, width));
	}
	else if (src == REG::IXH || src == REG::IYH)
	{
		fprintf(f, "\tld\t%s, (%s+%i)\n", REG::Str(dst, 2), REG::Str(src, 2), ofs);
	}
	else
		INTERNAL_ASSERT(0, "Bad index register %s rload", REG::Str(src, 1));
}

void asm_c::CLoad(REG::REG reg, int value, int width)
{
	if (width == 1)
	{//a, b, c, d, e, h, l, ixh, ixl, iyh, iyl
	}
	else if (width == 2)
	{//bc, de, hl, sp, ix, iy - no af
		INTERNAL_ASSERT(reg != REG::A, "af cannot be loaded into");
	}
	else
		INTERNAL_ASSERT(0, "a constant load of width '%i' is not allowed", width);


	fprintf(f, "\tld\t%s, %i\n", REG::Str(reg, width), value);
}

//
//adds
//

void asm_c::RAdd(REG::REG dst_reg, REG::REG src_reg, int width)
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
		if (dst_reg != REG::H && dst_reg != REG::IXH && dst_reg != REG::IYL)
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

void asm_c::Dec(REG::REG reg, int width)
{
	fprintf(f, "\tdec\t%s\n", REG::Str(reg, width));
}

//
//bitwise
//

void asm_c::Xor(REG::REG reg, bool is_hl)
{
	INTERNAL_ASSERT(reg >= REG::A && reg <= REG::L, "Tried to XOR reg %s", REG::Str(reg, 1));
	INTERNAL_ASSERT(is_hl == (reg == REG::H), "Nonsene args in XOR");

	if (is_hl)
	{
		fprintf(f, "\txor\t(hl)\n");
		return;
	}

	fprintf(f, "\txor\t%s\n", REG::Str(reg, 1));
}

void asm_c::Xor(int val)
{
	INTERNAL_ASSERT(val >= 0 && val <= UINT8_MAX, "Tried to XOR with val %i", val);
	fprintf(f, "\txor\t%i\n", val);
}

void asm_c::Xor(REG::REG idx_reg, int ofs)
{
	INTERNAL_ASSERT(ofs >= INT8_MIN && ofs <= INT8_MAX, "Invalid index offset %i", ofs);
	INTERNAL_ASSERT(idx_reg == REG::IXH || idx_reg == REG::IYH, "Tried an XOR w/offset with reg %s", REG::Str(idx_reg, 1));

	fprintf(f, "\txor\t(%s+%i)\n", REG::Str(idx_reg, 2), ofs);
}

//
// Stack
//

//FIXME: handle struct init info
//TODO: the catting really should be replaced with copying
//Handles initialization of bytes/words on the stack
void asm_c::StackInit(int value, int width)
{
	char buf[256]; //ought to be big enough

	for (int i = 0; i < width; i++)
	{
		int v = (value >> (i * 8)) & 0xFF;
		int len = sprintf_s(buf, "\tld\t(ix+%i), %i\n", stack_bytes_alloced + i, v);
		strcat_s(stack_queue, buf);
	}

	stack_bytes_alloced += width;
}

void asm_c::Push(REG::REG reg)
{
	fprintf(f, "\tpush\t%s\n", REG::Str(reg, 2));
}

void asm_c::Pop(REG::REG reg)
{
	fprintf(f, "\tpop\t%s\n", REG::Str(reg, 2));
}


const char* asmheader = ".nolist\n"
"#include \"../inc/ti83p.inc\"\n"
".list\n"
".org\tuserMem - 2\n"
".db\tt2ByteTok, tAsmCmp\n\n";

const unsigned short outflags = BACKGROUND_BLUE | FOREGROUND_RED | FOREGROUND_BLUE | FOREGROUND_GREEN | FOREGROUND_INTENSITY;

void asm_c::Print()
{
	if (!f)
	{
		printf("No file data\n");
		return;
	}
	char line[1024] = {};

	SetOutFlags(outflags);
	printf("\n");

	rewind(f);
	while (fgets(line, sizeof(line), f))
		printf("%s", line);

	ResetOutFlags();
	printf("\n");
}

asm_c::asm_c(const char* filename, const generator_c* gen_)
{
	fopen_s(&f, filename, "w+");
	INTERNAL_ASSERT(f, "Couldn't open assembler file %s", filename);
	fprintf(f, asmheader);

	gen = gen_;
}