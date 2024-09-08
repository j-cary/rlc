#pragma once
#include "common.h"


enum RETURNCODES
{
	RC_NULL	= 0, RC_PASS, RC_FAIL, RC_WARNING
};

#define GF_ARGS	bool advance
#define GF_DECL(x)	rcode_t x(GF_ARGS)
#define GF_DEF(x)	parse_c::rcode_t parse_c::x(GF_ARGS)
#define DEPTH_MAX	128 //FIXME!!! this needs to be dynamic. Used for the tab string
#define DBG_STR_MAX	32

class parse_c
{
private:
	typedef uint64_t rcode_t;
	typedef rcode_t (parse_c::*gfunc_t) (GF_ARGS);
	typedef struct fstrans_s
	{
		gfunc_t f;
		char s[DBG_STR_MAX];
	} fstrans_t;

	//Grammar functions

	rcode_t UNIT(GF_ARGS);
	rcode_t EXTERNAL_DECLARATION(GF_ARGS);
	rcode_t FUNCTION_DEFINITION(GF_ARGS);

	rcode_t DECLARATION_SPECIFIER(GF_ARGS);
	rcode_t STORAGE_SPECIFIER(GF_ARGS);
	rcode_t TYPE_SPECIFIER(GF_ARGS);

	GF_DECL(STRUCT_SPECIFIER);
	GF_DECL(STRUCT_DECLARATION);
	GF_DECL(STRUCT_DECLARATOR_LIST);
	GF_DECL(STRUCT_DECLARATOR);
	rcode_t DECLARATOR(GF_ARGS);
	rcode_t DIRECT_DECLARATOR(GF_ARGS);

	//expressions
	GF_DECL(CONSTANT_EXPRESSION); //is this useful?
	GF_DECL(CONDITIONAL_EXPRESSION);
	GF_DECL(LOGICAL_OR_EXPRESSION);
	GF_DECL(LOGICAL_AND_EXPRESSION);
	GF_DECL(OR_EXPRESSION);
	GF_DECL(AND_EXPRESSION);
	GF_DECL(EQUALITY_EXPRESSION);
	GF_DECL(RELATIONAL_EXPRESSION);
	GF_DECL(SHIFT_EXPRESSION);
	GF_DECL(ADDITIVE_EXPRESSION);
	GF_DECL(MULTIPLICATIVE_EXPRESSION);
	GF_DECL(UNARY_EXPRESSION);
	GF_DECL(POSTFIX_EXPRESSION);
	GF_DECL(PRIMARY_EXPRESSION);
	GF_DECL(EXPRESSION);
	GF_DECL(ASSIGNMENT_EXPRESSION); //I think this is getting replaced by the EXPR pseudo-instruction

	rcode_t DECLARATION(GF_ARGS);
	rcode_t INIT_DECLARATOR(GF_ARGS);
	rcode_t INITIALIZER(GF_ARGS);
	rcode_t INITIALIZER_LIST(GF_ARGS);

	//statements
	rcode_t COMPOUND_STATEMENT(GF_ARGS);
	/*
	rcode_t STATEMENT(GF_ARGS);
	rcode_t INSTRUCTION_STATEMENT(GF_ARGS);
	rcode_t SELECTION_STATEMENT(GF_ARGS);
	rcode_t ITERATION_STATEMENT(GF_ARGS);
	*/
	GF_DECL(STATEMENT);
	GF_DECL(OPEN_STATEMENT);
	GF_DECL(CLOSED_STATEMENT);
	GF_DECL(SIMPLE_STATEMENT);
	GF_DECL(INSTR_STATEMENT);
	GF_DECL(SELECTION_CLAUSE);
	GF_DECL(FOR_CLAUSE);
	GF_DECL(WHILE_CLAUSE);

	//instructions & pseudo-instructions
	GF_DECL(INSTRUCTION_LD);
	GF_DECL(INSTRUCTION_JP);
	GF_DECL(INSTRUCTION_CALL);
	GF_DECL(INSTRUCTION_RET);

	rcode_t INSTRUCTION_ADD(GF_ARGS);
	GF_DECL(INSTRUCTION_SUB);
	GF_DECL(INSTRUCTION_MUL);
	GF_DECL(INSTRUCTION_DIV);
	GF_DECL(INSTRUCTION_MOD);
	GF_DECL(INSTRUCTION_INC);
	GF_DECL(INSTRUCTION_DEC);
	GF_DECL(INSTRUCTION_EXPR);

	GF_DECL(INSTRUCTION_OR);
	GF_DECL(INSTRUCTION_AND);
	GF_DECL(INSTRUCTION_XOR);

	GF_DECL(INSTRUCTION_RLC);
	GF_DECL(INSTRUCTION_RRC);
	GF_DECL(INSTRUCTION_RL);
	GF_DECL(INSTRUCTION_RR);
	GF_DECL(INSTRUCTION_SLA);
	GF_DECL(INSTRUCTION_SRA);
	GF_DECL(INSTRUCTION_SLL);
	GF_DECL(INSTRUCTION_SRL);
	GF_DECL(INSTRUCTION_SET);
	GF_DECL(INSTRUCTION_RES);
	GF_DECL(INSTRUCTION_FLP);

	GF_DECL(INSTRUCTION_IN);
	GF_DECL(INSTRUCTION_OUT);

	GF_DECL(INSTRUCTION_IM);

	GF_DECL(INSTRUCTION_LDM);
	GF_DECL(INSTRUCTION_CPM);
	GF_DECL(INSTRUCTION_INM);
	GF_DECL(INSTRUCTION_OUTM);

	GF_DECL(CONSTANT);
	rcode_t IDENTIFIER(GF_ARGS);
	GF_DECL(RVALUE);
	GF_DECL(RVALUE_LIST);

	rcode_t Call(gfunc_t func, GF_ARGS);
	//

	llist_c* list;

	char tabstr[DEPTH_MAX * 2] = {};
	int tabs = 0;

	fstrans_t fs[26] =
	{

		& parse_c::UNIT,					"Unit",
		& parse_c::EXTERNAL_DECLARATION,	"External decl",
		& parse_c::FUNCTION_DEFINITION,		"Function def",
		
		& parse_c::DECLARATION_SPECIFIER,	"Decl specifier",
		& parse_c::STORAGE_SPECIFIER,		"Storage specifier",
		& parse_c::TYPE_SPECIFIER,			"Type specifier",
		
		& parse_c::DECLARATOR,				"Declarator",
		& parse_c::DIRECT_DECLARATOR,		"Direct declarator",

		& parse_c::EXPRESSION,				"Expression",
		
		& parse_c::DECLARATION,				"Declaration",
		& parse_c::INIT_DECLARATOR,			"Init declarator",
		& parse_c::INITIALIZER,				"Initializer",
		& parse_c::INITIALIZER_LIST,		"Initializer list",

		& parse_c::COMPOUND_STATEMENT,		"Compound statement",
		& parse_c::STATEMENT,				"Statement",
		/*
		& parse_c::INSTRUCTION_STATEMENT,	"Instruction statement",
		& parse_c::SELECTION_STATEMENT,		"Selection statement",
		& parse_c::ITERATION_STATEMENT,		"Iteration statement",
		*/
		& parse_c::OPEN_STATEMENT,			"Open statement",
		& parse_c::CLOSED_STATEMENT,		"Closed statement",
		& parse_c::SIMPLE_STATEMENT,		"Simple statement",
		& parse_c::INSTR_STATEMENT,			"Instruction statement",
		& parse_c::SELECTION_CLAUSE,		"Selection clause",
		& parse_c::FOR_CLAUSE,				"For clause",
		& parse_c::WHILE_CLAUSE,			"While clause",

		& parse_c::INSTRUCTION_ADD,			"Add Instruction",


		& parse_c::IDENTIFIER,				"Identifier",
		& parse_c::RVALUE,					"R-value",
		& parse_c::RVALUE_LIST,				"R-value list",

	};

public:
	void Parse(llist_c* _list);
};