#pragma once
#include "common.h"
#include "semantics.h"

#define STACK_MAX	16
#define DATALUMP_SIZE		1024

#if 0
//regs
#define REG_AF		0
#define REG_BC		1
#define REG_DE		2
#define REG_HL		3
#define REGS_MAX	4

typedef struct register_s
{
	data_t			held[2]; //keep a snapshot of the data so we can tell if it is current later
} register_t;

#else

#define REG_A		0
#define REG_B		1
#define REG_C		2
#define REG_D		3
#define REG_E		4
#define REG_H		5
#define REG_L		6
#define REG_R0		7
#define REG_R255	(REG_R0 + 255) //Maximum of 256 spill regs. Only allocate space for the maximum number of used regs
#define REG_IYH		(REG_R255 + 1) //only used for struct/array indexing
#define REG_IYL		(REG_R255 + 2)
#define REG_IXH		(REG_R255 + 3)
#define REG_IXL		(REG_R255 + 4) 

#define REG_BC		(REG_B + REG_IXL)
#define REG_DE		(REG_D + REG_IXL)
#define REG_HL		(REG_H + REG_IXL)
#define REG_WR0		(REG_R0 + REG_IXL) //r0 and r1
#define REG_WR127	(REG_R255 - 1 + REG_IXL) //r254 and r255
#define REG_IY		(REG_IYH + REG_IXL)
#define REG_IX		(REG_IXH + REG_IXL)

#define REGS_TOTAL	(REG_IXL + 2 + ((REG_IX - REG_BC) / 2)) //400

//0-6 : a-l
//7-262 : r0-r255
//263-266 : iyh-ixl
//267 : bc
//269 : de
//271 : hl
//273 : wr0
//275 : wr1
//...
//527 : wr127
//529 : iy
//531 : ix

typedef struct 
{
	int held; //hold the index of the data that this is holding per block
} register_t;
#endif

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
	data_t*		symtbl = {};
	unsigned	symtbl_top;
	char		dataqueue[DATALUMP_SIZE] = {}; //per-function text lump containing data declarations
	FILE*		f;

	register_t	regs[REGS_TOTAL];

	//
	//code generation functions
	//

	//program control
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
	void CG_Add(tree_c* node, cfg_c* block, int op_cnt, int* ofs, colori_t* colors);
	void CG_Load(tree_c*, cfg_c* block, int op_cnt, int* ofs, colori_t* colors);
	void CG_Call(tree_c* n, cfg_c* block, int op_cnt, int* ofs, colori_t* colors);


	//
	//utility functions
	//

	data_t* SymbolEntry(tree_c* symb);
	void UpdateSymbol(tree_c* symb, int set);

	void PrintSourceLine(tree_c* node);
	void R_PrintSourceLine(tree_c* node);


	
	char* RegToS(regi_t reg); //No more than 32 strings can be requested from this function at once!
	regi_t RegAlloc(colori_t color); //data_ofs is the index of the data value in the block
	void RegFree(regi_t reg);
	void MarkReg(regi_t reg, paralleli_t data);
	bool IsMarked(regi_t reg, paralleli_t data);
	paralleli_t DataOfs(cfg_c* block, tree_c* n);

	inline int Code(tree_c* node);
	inline const char* Str(tree_c* node);

	int Constant_Expression(tree_c* head);
	int Constant_Expression_Helper(tree_c* head, int num_kids, int offset, int type);

	int GetForLabel(char* buf); //returns label id


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

	void ASM_DLoad(cfg_c* block, regi_t reg, paralleli_t data); //Load Data from this block into a reg
	void ASM_RLoad(regi_t dst_reg, regi_t src_reg); //Load from reg to reg
	void ASM_CLoad(regi_t reg, int value); //Load a const

	void ASM_Store(tree_c* var, const char* reg); //var <- reg

	void ASM_Add(regi_t dst_reg, regi_t src_reg);
	void ASM_Djnz(const char* label);

public:
	void Generate(tree_c* _root, cfg_c* _graph, data_t* symbols, unsigned* symbol_top);


	//
	//Standard library
	//

	//Here so stdcalls can access the functions
	void SL_Print(tree_c*n, cfg_c* block, int op_cnt, int* ofs, colori_t* colors);

	generator_c()
	{
		for (int i = 0; i < REGS_TOTAL; i++)
			regs[i].held = -1;

	}
};