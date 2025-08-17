#pragma once
#include "common.h"
#include "semantics.h"

#define DATALUMP_SIZE		1024



typedef struct 
{
	tdatai_t held; //hold the index of the data that this is holding per block
} register_t;

//ld a, (nn)/n/(bc)/(de)/(hl)/a/b/c/d/e/h/l
//ld (nn)/(bc)/(de)/(hl)/a/b/c/d/e/h/l, a
//ld x,		 n/			 (hl)/a/b/c/d/e/h/l
//ld				(hl)/a/b/c/d/e/h/l, x

#define INSTR_GEN_FUNC_ARGS	const tdata_t* data[], const tree_c* nodes[], int cnt
#define INSTR_GEN_FUNC(fn)	void fn					(INSTR_GEN_FUNC_ARGS)
typedef						void(*instr_gen_func_t)	(INSTR_GEN_FUNC_ARGS);
typedef instr_gen_func_t instr_gen_table_t[4][4];

class generator_c
{
	tree_c*		root;
	cfg_c*		graph;
	tdata_t*	tdata;

	bool	data_decls_allowed;
	int		stack_allocated; //FIXME:this is going to have to be a stack; push per block
	char	stack_queue[DATALUMP_SIZE] = {};
	const char* subr_name = NULL;

	unsigned	symtbl_top; //rename - sizeof tdata
	char		dataqueue[DATALUMP_SIZE] = {}; //per-function text lump containing data declarations
	FILE*		f;

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

	void PrintSourceLine(tree_c* node);
	void R_PrintSourceLine(tree_c* node);


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

	//
	//assembler file functions
	//

	void InitFile(const char* filename);
	void PrintFile(); //only call this after generation
	
	void ASM_Ret(const char* parm);
	void ASM_Label(const char* name);
	void ASM_Data(const char* type, tree_c* var, const char* init);
	void ASM_Data(const char* type, tree_c* var, int init);

	void ASM_DLoad(regi_t reg, tdatai_t data); //Load Data from this block into a reg
#if OLD_REG_CODE
	void ASM_RLoad(regi_t dst_reg, regi_t src_reg); //Load from reg to reg
	void ASM_CLoad(regi_t reg, int value); //Load a const
#endif
	void ASM_ALoad(regi_t reg, tdatai_t data); //load an addr
	void ASM_Store(tdatai_t data, regi_t reg); //var <- reg

#if OLD_REG_CODE
	void ASM_RAdd(regi_t dst_reg, regi_t src_reg);
#endif
	void ASM_CAdd(regi_t dst_reg, int value);
	void ASM_Djnz(const char* label);

#if OLD_REG_CODE
	void ASM_Push(regi_t reg);
	void ASM_Pop(regi_t reg);
#else

	//
	//loads
	//

	void ASM_RLoad(REG::REG dst, REG::REG src, int width);
	//Just for ix/iy loads/stores
	void ASM_RLoad(REG::REG dst, REG::REG src, int width, int ofs);
	void ASM_CLoad(REG::REG reg, int value, int width);

	//
	//adds
	//
	void ASM_RAdd(REG::REG dst_reg, REG::REG src_reg, int width);

	//
	//inc/dec
	//

	void ASM_Dec(REG::REG reg, int width);

	//
	//stack
	//

	void ASM_StackInit(int value, int ofs, int width);
	void ASM_Push(REG::REG reg);
	void ASM_Pop(REG::REG reg);
#endif

public:
	void Generate(tree_c* _root, cfg_c* _graph, tdata_t* _tdata, unsigned* symbol_top);


	//
	//Standard library
	//

	//Here so stdcalls can access the functions
	void SL_Print(tree_c* node, int op_cnt, int* ofs, tdatai_t* data, regi_t* reg);
};