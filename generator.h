#pragma once
#include "common.h"
#include "semantics.h"

#define STACK_MAX	16
#define DATALUMP_SIZE		1024



typedef struct 
{
	tdatai_t held; //hold the index of the data that this is holding per block
} register_t;

//ld a, (nn)/n/(bc)/(de)/(hl)/a/b/c/d/e/h/l
//ld (nn)/(bc)/(de)/(hl)/a/b/c/d/e/h/l, a
//ld x,		 n/			 (hl)/a/b/c/d/e/h/l
//ld				(hl)/a/b/c/d/e/h/l, x

class generator_c
{
	tree_c*		root;
	cfg_c*		graph;
	char*		stack[STACK_MAX];
	unsigned	stack_top = 0;
	//data_t*		symtbl = {};
	tdata_t*	tdata;
	unsigned	symtbl_top; //rename - sizeof tdata
	char		dataqueue[DATALUMP_SIZE] = {}; //per-function text lump containing data declarations
	FILE*		f;
	igraph_c*	igraph;

	register_t	regs[REGS_TOTAL];

	//
	//code generation functions
	//

	//program control
	void CG_Global();
	void CG_FunctionBlock(cfg_c* block);
	void CG_RegBlock(cfg_c* block);
	void CG_ExitBlock(cfg_c* block);
	void CG_ForBlock(cfg_c* block);

	void CG_DataDeclaration(tree_c* node, cfg_c* block);

	//data types

	//statements
	void CG_ForLoop(tree_c* node, cfg_c* block, cfg_c* body);

	//instructions
	void CG_Instruction(tree_c* node, cfg_c* block);
	void CG_Add (tree_c* node, int op_cnt, int* ofs, tdatai_t* data, regi_t* reg);
	void CG_Load(tree_c* node, int op_cnt, int* ofs, tdatai_t* data, regi_t* reg);
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

	tdatai_t DataOfs(cfg_c* block, tree_c* n);
	const char* DataName(tdatai_t data);

	 int Code(tree_c* node);
	 const char* Str(tree_c* node);

	//int Constant_Expression(tree_c* head);
	//int Constant_Expression_Helper(tree_c* head, int num_kids, int offset, int type);

	int GetForLabel(char* buf); //returns label id

	bool IsSReg(regi_t r);
	bool IsWord(regi_t r);


	//
	//assembler file functions
	//

	void InitFile(const char* filename);
	void PrintFile(); //only call this after generation
	//void WriteF();
	void ASM_Ret(const char* parm);
	void ASM_Label(const char* name);
	void ASM_Data(const char* type, tree_c* var, const char* init);
	void ASM_Data(const char* type, tree_c* var, int init);

	void ASM_DLoad(regi_t reg, tdatai_t data); //Load Data from this block into a reg
	void ASM_RLoad(regi_t dst_reg, regi_t src_reg); //Load from reg to reg
	void ASM_CLoad(regi_t reg, int value); //Load a const
	void ASM_ALoad(regi_t reg, tdatai_t data); //load an addr
	void ASM_Store(tdatai_t data, regi_t reg); //var <- reg

	void ASM_RAdd(regi_t dst_reg, regi_t src_reg);
	void ASM_CAdd(regi_t dst_reg, int value);
	void ASM_Djnz(const char* label);

	void ASM_Push(regi_t reg);
	void ASM_Pop(regi_t reg);

public:
	void Generate(tree_c* _root, cfg_c* _graph, tdata_t* _tdata, unsigned* symbol_top, igraph_c* igraph);


	//
	//Standard library
	//

	//Here so stdcalls can access the functions
	void SL_Print(tree_c* node, int op_cnt, int* ofs, tdatai_t* data, regi_t* reg);

	generator_c()
	{
		for (int i = 0; i < REGS_TOTAL; i++)
			regs[i].held = REG_BAD;

	}
};