#pragma once
#include "common.h"
#include "semantics.h"

#define DATALUMP_SIZE		1024

typedef struct 
{
	tdatai_t held; //hold the index of the data that this is holding per block
} register_t;

// Operand information passed into the instruction modules
typedef struct
{
	const tdata_t* data;
	const tree_c* node;
	int offset;
} op_info_t;

//ld a, (nn)/n/(bc)/(de)/(hl)/a/b/c/d/e/h/l
//ld (nn)/(bc)/(de)/(hl)/a/b/c/d/e/h/l, a
//ld x,		 n/			 (hl)/a/b/c/d/e/h/l
//ld				(hl)/a/b/c/d/e/h/l, x

#define INSTR_GEN_FUNC_ARGS	const op_info_t info[], int cnt 
#define INSTR_GEN_FUNC(fn)	void fn					(INSTR_GEN_FUNC_ARGS)
typedef						void(*instr_gen_func_t)	(INSTR_GEN_FUNC_ARGS);
typedef instr_gen_func_t instr_gen_table_t[5][4];

class generator_c;

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
	void Xor(REG::REG reg, bool is_hl=false);
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

/* Dispatches the reduced parse tree to instruction modules */
class generator_c
{
	asm_c assembler;
	tree_c*		root;
	cfg_c*		graph; //The root of the control flow graph
	tdata_t*	tdata;
	const structlist_c *sl;


	bool	data_decls_allowed;
	const cfg_c* func; //The current function

	unsigned	symtbl_top; //rename - sizeof tdata

	//
	//code generation functions
	//

	//program control
	void CG_Global();
#if !OLD_REG_CODE
	void CG_Stack();
	void CG_UnStack();
#endif
	void CG_FunctionBlock(cfg_c* block);
	void CG_RegBlock(cfg_c* block);
	void CG_ExitBlock(cfg_c* block);
	void CG_ForBlock(cfg_c* block);

	void CG_StructDeclaration(tree_c* node, cfg_c* block);
	void CG_DataDeclaration(tree_c* node, cfg_c* block);

	//data types

	//statements
	void CG_ForLoop(tree_c* node, cfg_c* block, cfg_c* body);

	//instructions
	void CG_Instruction(tree_c* node, cfg_c* block);
	void CG_Add (tree_c* node, int op_cnt, int* ofs, tdatai_t* data, regi_t* reg);
#if OLD_REG_CODE
	void CG_Load(tree_c* node, int op_cnt, int* ofs, tdatai_t* data, regi_t* reg);
#else
	void CG_Load(INSTR_GEN_FUNC_ARGS);
#endif
	void CG_Call(tree_c* node, int op_cnt, int* ofs, tdatai_t* data, regi_t* reg);


	//
	//utility functions
	//

	data_t* SymbolEntry(tree_c* symb);
	void UpdateSymbol(tree_c* symb, int set);


	char* RegToS(regi_t reg); //No more than 32 strings can be requested from this function at once!
	regi_t RegAlloc(tdatai_t index);
	regi_t HiByte(regi_t r);
	regi_t LoByte(regi_t r);
	//regi_t AliasReg(regi_t r);
	void RegFree(regi_t reg);
	void MarkReg(regi_t reg, tdatai_t data);
	bool IsMarked(regi_t reg, paralleli_t data);

	tdata_t* Data(cfg_c* block, tree_c* n);
	tdatai_t DataOfs(cfg_c* block, tree_c* n);
	const char* DataName(tdatai_t data);

	int Code(tree_c* node);
	const char* Str(tree_c* node);

	bool IsRegActive(REG::REG reg);

	int GetForLabel(char* buf); //returns label id

public:
	generator_c() : assembler("C:/ti83/rl/test.z80", this) { }

	void Generate(tree_c* _root, cfg_c* _graph, tdata_t* _tdata, unsigned* symbol_top, const structlist_c* _sl);


	//
	//Standard library
	//

	//Here so stdcalls can access the functions
	void SL_Print(tree_c* node, int op_cnt, int* ofs, tdatai_t* data, regi_t* reg);
};