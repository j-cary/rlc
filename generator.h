/***************************************************************************************************
Purpose: Handle high-level parsing of the CFG into ASM code. 
Input: 
Output: 
***************************************************************************************************/
#pragma once
#include "common.h"
#include "semantics.h"
#include "asm.h"
#include "evaluate_expression.h"

/***************************************************************************************************
										Defines/Typedefs
***************************************************************************************************/

// Operand information passed into the instruction modules
typedef struct
{
	const tdata_t* data;
	const tree_c* node;
	eval_expr_c::mem_result_t mem;
	//int offset;
} op_info_t;

#define INSTR_GEN_FUNC_ARGS	const op_info_t info[], int cnt 

// Def of a instr_gen_func
#define INSTR_GEN_FUNC(fn)	void fn					(INSTR_GEN_FUNC_ARGS)

// Type of an INSTR_GEN_FUNC
typedef						void(*instr_gen_func_t)	(INSTR_GEN_FUNC_ARGS);

//Table of functions for dispatching based on operands
typedef instr_gen_func_t instr_gen_table_t[5][4];

// Dispatches the reduced parse tree to instruction modules
class generator_c
{
	asm_c assembler;
	tree_c*		root;
	cfg_c*		graph; //The root of the control flow graph
	tdata_t*	tdata;
	const structlist_c *slist;


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

	CODE Code(tree_c* node);
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