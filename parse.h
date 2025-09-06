#pragma once
#include "common.h"
#include "llist.h"

#define GF_ARGS	tree_c* parent
#define GF_DECL(x)	rcode_t x(GF_ARGS)
#define GF_DEF(x)	parser_c::rcode_t parser_c::x(GF_ARGS)
#define FS_ENTRY(func, string)	&parser_c::func,string
#define DBG_STR_MAX	32

class parser_c
{
private:
	typedef uint64_t rcode_t;
	typedef rcode_t (parser_c::*gfunc_t) (GF_ARGS);
	typedef struct fstrans_s
	{
		gfunc_t f;
		char s[DBG_STR_MAX];
	} fstrans_t;

	//Grammar functions

	//
	//Main program control
	//
	GF_DECL(TRANSLATION_UNIT);
	GF_DECL(EXTERNAL_DECL);

	GF_DECL(FUNC);
	GF_DECL(DATA_DECL);
	GF_DECL(SINGLE_DATA_DECL);
	GF_DECL(TYPE_DEF);

	//
	//ASM_Data types
	//
	GF_DECL(STORAGE_SPECIFIER);
	GF_DECL(DATA_MODIFIER);
	GF_DECL(DATA_TYPE);
	GF_DECL(STRUCT_DECL);

	//
	//Statements
	//
	GF_DECL(COMPOUND_STATEMENT);
	GF_DECL(STATEMENT);
	GF_DECL(OPEN_STATEMENT);
	GF_DECL(CLOSED_STATEMENT);
	GF_DECL(SIMPLE_STATEMENT);

	GF_DECL(SELECTION_CLAUSE);
	GF_DECL(FOR_CLAUSE);
	GF_DECL(WHILE_CLAUSE);
	GF_DECL(LABEL_DEF);

	//
	//Instructions
	//
	GF_DECL(INSTRUCTION);
	GF_DECL(OPERANDS_ONE);
	GF_DECL(OPERANDS_TWO);
	GF_DECL(OPERANDS_THREE);
	GF_DECL(OPERANDS_ONE_TO_TWO);
	GF_DECL(OPERANDS_ONE_TO_THREE);
	GF_DECL(OPERANDS_TWO_TO_INF);
	GF_DECL(OPERANDS_COMP);
	GF_DECL(OPERANDS_CPM);
	GF_DECL(OPERANDS_RET);
	GF_DECL(OPERANDS_CALL);


	//
	//Misc
	//
	GF_DECL(INITIALIZER_LIST);
	GF_DECL(PARAMETER);
	GF_DECL(PARAMETER_LIST);
	GF_DECL(CONSTANT);

	GF_DECL(IDENTIFIER);

	//
	//Expressions
	//

	//Logical expressions
	GF_DECL(LOGICAL_EXPRESSION);

	GF_DECL(OR_EXPRESSION);
	GF_DECL(AND_EXPRESSION);
	GF_DECL(EQUALITY_EXPRESSION);
	GF_DECL(RELATIONAL_EXPRESSION);
	GF_DECL(LOGICAL_POSTFIX_EXPRESSION); //these last two could be shared, but the '(' <logical/arithmetic_expression> ')' production wouldn't work
	GF_DECL(LOGICAL_PRIMARY_EXPRESSION);
	//Arithmetic expressions
	GF_DECL(ARITHMETIC_EXPRESSION);
	GF_DECL(CONSTANT_EXPRESSION);

	GF_DECL(SHIFT_EXPRESSION);
	GF_DECL(ADDITIVE_EXPRESSION);
	GF_DECL(MULTIPLICATIVE_EXPRESSION);
	GF_DECL(ARITHMETIC_POSTFIX_EXPRESSION);
	GF_DECL(ARITHMETIC_PRIMARY_EXPRESSION);
	//Memory expressions
	GF_DECL(MEMORY_EXPRESSION); 
	GF_DECL(MEM_OR_CONST_EXPRESSION);
	GF_DECL(MEMORY_PRIMARY_EXPRESSION);


	rcode_t Call(gfunc_t func, GF_ARGS);

	llist_c* list;
	//tree_c root;
	tree_c* root;
	int debug;

	char tabstr[DEPTH_MAX * 2] = {};
	int tabs = 0;

	const fstrans_t fs[53] =
	{
		//main prog
		FS_ENTRY(TRANSLATION_UNIT, "Translation unit"),
		FS_ENTRY(EXTERNAL_DECL, "External declaration"),

		FS_ENTRY(FUNC, "Function"),
		FS_ENTRY(DATA_DECL, "Data declaration"),
		FS_ENTRY(SINGLE_DATA_DECL, "Single data declaration"),
		FS_ENTRY(TYPE_DEF, "Type definition"),

		//data
		FS_ENTRY(STORAGE_SPECIFIER, "Storage specifier"),
		FS_ENTRY(DATA_MODIFIER, "Data modifier"),
		FS_ENTRY(DATA_TYPE, "Data"),
		FS_ENTRY(STRUCT_DECL, "Struct decl"),

		//statements
		FS_ENTRY(COMPOUND_STATEMENT, "Compound statement"),
		FS_ENTRY(STATEMENT, "Statement"),
		FS_ENTRY(OPEN_STATEMENT, "Open statement"),
		FS_ENTRY(CLOSED_STATEMENT, "Closed statement"),
		FS_ENTRY(SIMPLE_STATEMENT, "Simple statement"),

		FS_ENTRY(SELECTION_CLAUSE, "Selection clause"),
		FS_ENTRY(FOR_CLAUSE, "For clause"),
		FS_ENTRY(WHILE_CLAUSE, "While clause"),
		FS_ENTRY(LABEL_DEF, "Label definition"),

		//Instructions
		FS_ENTRY(INSTRUCTION, "Instruction"),
		FS_ENTRY(OPERANDS_ONE, "One op"),
		FS_ENTRY(OPERANDS_TWO, "Two ops"),
		FS_ENTRY(OPERANDS_THREE, "Three ops"),
		FS_ENTRY(OPERANDS_ONE_TO_TWO, "1/2 op(s)"),
		FS_ENTRY(OPERANDS_ONE_TO_THREE, "1-3 op(s)"),
		FS_ENTRY(OPERANDS_TWO_TO_INF, "2+ ops"),
		FS_ENTRY(OPERANDS_COMP, "COMP op"),
		FS_ENTRY(OPERANDS_CPM, "CPM ops"),
		FS_ENTRY(OPERANDS_RET, "RET ops"),
		FS_ENTRY(OPERANDS_CALL, "CALL ops"),

		//misc
		FS_ENTRY(INITIALIZER_LIST, "Initializer list"),
		FS_ENTRY(PARAMETER, "Parameter"),
		FS_ENTRY(PARAMETER_LIST, "Parameter list"),

		FS_ENTRY(IDENTIFIER, "Ident"),
		FS_ENTRY(CONSTANT, "Const"),

		//Logical expressions

		FS_ENTRY(LOGICAL_EXPRESSION, "Logical expression"),

		FS_ENTRY(OR_EXPRESSION, "Or expression"),
		FS_ENTRY(AND_EXPRESSION, "And expression"),
		FS_ENTRY(EQUALITY_EXPRESSION, "Equality expression"),
		FS_ENTRY(RELATIONAL_EXPRESSION, "Relational expression"),
		FS_ENTRY(LOGICAL_POSTFIX_EXPRESSION, "LPostfix expression"),
		FS_ENTRY(LOGICAL_PRIMARY_EXPRESSION, "LPrimary expression"),

		//Arithmetic expressions

		FS_ENTRY(ARITHMETIC_EXPRESSION, "Arithmetic expression"),
		FS_ENTRY(CONSTANT_EXPRESSION, "Constant expression"),

		FS_ENTRY(SHIFT_EXPRESSION, "Shift expression"),
		FS_ENTRY(ADDITIVE_EXPRESSION, "Additive expression"),
		FS_ENTRY(MULTIPLICATIVE_EXPRESSION, "Multiplicative expression"),
		FS_ENTRY(ARITHMETIC_POSTFIX_EXPRESSION, "APostfix expression"),
		FS_ENTRY(ARITHMETIC_PRIMARY_EXPRESSION, "APrimary expression"),
		
		//Memory expressions

		FS_ENTRY(MEMORY_EXPRESSION, "Memory expression"),
		FS_ENTRY(MEM_OR_CONST_EXPRESSION, "Mem or const expression"),
		FS_ENTRY(MEMORY_PRIMARY_EXPRESSION, "MPrimary expression"),

	};

public:
	void Parse(llist_c* _list, tree_c* _root, int debug);
	parser_c()
	{
		list = NULL;
		//root.tree_c::tree_c();
	}
};

//#define PEEKCP(x)	(list->Peek()->Code() == x)
#define PEEKCP(x)	(list->Peek() ? list->Peek()->Code() == x : false)
#define GETCP(x)	(list->Get()->Code() == x)
//#define CL(x, y)	(Call(&parser_c::x, y, NULL))
//#define CL(x,y,z)	(Call(&parser_c::x, y, z))
#define CL(x, y)	(Call(&parser_c::x, y))