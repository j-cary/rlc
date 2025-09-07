/***************************************************************************************************
Purpose: Generate the z80 assembly code 
Inputs: Filename, generator reference, data indicating operators, operands, and respective info
Outputs: An ascii assembly file ready for assembling
***************************************************************************************************/
#pragma once
#include "common.h"
#include "generator.h"

#define DATALUMP_SIZE		1024

class generator_c; // The generator and this cross-reference each other

/* Creates the assembler file */
class asm_c
{
private:
	FILE* f = NULL;
	const generator_c* gen;

	char	dataqueue[DATALUMP_SIZE];
	char	stack_queue[DATALUMP_SIZE];
	int		stack_bytes_alloced = 0;

	void R_PrintSourceLine(tree_c*);
public:
	void StackFrame();
	void UnStackFrame();
	inline int StackAlloc() const { return stack_bytes_alloced; }
	inline void ResetStack() { stack_bytes_alloced = 0; }

	void UnQueueData();

	void PrintSourceLine(tree_c* line);


	void Ret(const char* parm);
	void Label(const char* name);
	void Data(const char* type, tree_c* var, const char* init);
	void Data(const char* type, tree_c* var, int init);

	void DLoad(regi_t reg, tdatai_t data); //Load Data from this block into a reg
	void ALoad(regi_t reg, tdatai_t data); //load an addr
	void Store(int ofs, REG::REG reg); //auto <- reg

	void CAdd(regi_t dst_reg, int value);
	void Djnz(const char* label);


	//
	//loads
	//

	void RLoad(REG::REG dst, REG::REG src, int width);
	//Just for ix/iy loads/stores
	void RLoad(REG::REG dst, REG::REG src, int width, int ofs);
	void CLoad(REG::REG reg, int value, int width);

	//
	//adds
	//
	void RAdd(REG::REG dst_reg, REG::REG src_reg, int width);

	//
	//inc/dec
	//

	void Dec(REG::REG reg, int width);

	//
	//bitwise
	//

	//Xor A against an reg8 or (hl)
	void Xor(REG::REG reg, bool is_hl = false);
	//Xor A against an imm8
	void Xor(int val);
	//Xor A against an index reg
	void Xor(REG::REG idx_reg, int ofs);

	//
	//stack
	//

	void StackInit(int value, int width);
	void Push(REG::REG reg);
	void Pop(REG::REG reg);

	void Print();

	asm_c(const char* filename, const generator_c* gen);
	~asm_c()
	{
		if (f) fclose(f);
	}
};
