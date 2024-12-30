#pragma once
#include "common.h"
#include "semantics.h"

#define STACK_MAX	16
#define DATALUMP_SIZE		1024

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

//ld a, (nn)/n/(bc)/(de)/(hl)/a/b/c/d/e/h/l
//ld (nn)/(bc)/(de)/(hl)/a/b/c/d/e/h/l, a
//ld x,		 n/			 (hl)/a/b/c/d/e/h/l
//ld				(hl)/a/b/c/d/e/h/l, x

class generator_c
{

	tree_c*		root;
	tree_c*		stack[STACK_MAX];
	unsigned	stack_top = 0;
	data_t*		symtbl = {};
	unsigned	symtbl_top;
	register_t	regs[REGS_MAX] = {};
	char		dataqueue[DATALUMP_SIZE] = {}; //per-function text lump containing data declarations
	FILE*		f;

	register_t* af, * bc, * de, * hl;

	//
	//code generation functions
	//

	//program control
	void VisitNode(tree_c* node);
	void CG_FuncDef(tree_c* node);
	void CG_DataDeclaration(tree_c* node);

	//data types

	//statements
	void CG_CompoundStatement(tree_c* node);

	//instructions
	void CG_Instruction(tree_c* node);
	void CG_Add(tree_c* node);
	void CG_Load(tree_c*);
	void CG_Call(tree_c*);


	//
	//utility functions
	//

	data_t* SymbolEntry(tree_c* symb);
	void UpdateSymbol(tree_c* symb, int set); //fixme: somehow need to keep track of what regs hold what values. problem: load v1, v2;
	const char* RegToS(register_t* r, int which);
	const char* RegToS(int r, int which);
	register_t* SToReg(const char* reg, int* which);
	inline int Code(tree_c* node);
	inline const char* Str(tree_c* node);

	//Data flag conversions
	unsigned RegStrToDF(const char* reg);
	unsigned RegToDF(register_t* reg, unsigned which);

	int Linked(const char* reg, tree_c* var);
	int Linked(register_t* reg, int which, tree_c* var);
	int TreeCmp(tree_c* t1, tree_c* t2);

	int Constant_Expression(tree_c* head);
	int Constant_Expression_Helper(tree_c* head, int num_kids, int offset, int type);


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
	void ASM_Load(const char* reg, tree_c* var);	//reg <- var
	void ASM_Load(const char* reg, int val);		//reg <- var
	void ASM_Load(const char* dst, const char* src);//reg <- reg
	void ASM_Store(tree_c* var, const char* reg); //var <- reg

public:
	void Generate(tree_c* _root, data_t* symbols, unsigned* symbol_top);


	//
	//Standard library
	//

	//Here so stdcalls can access the functions
	void SL_Print(tree_c*);

	generator_c()
	{
		af = &regs[REG_AF];
		bc = &regs[REG_BC];
		de = &regs[REG_DE];
		hl = &regs[REG_HL];
	}
};