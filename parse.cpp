#include "parse.h"

#define PEEKCP(x)	(list->Peek()->V() == x)
#define GETCP(x)	(list->Get()->V() == x)
//#define CL(x, y)	(Call(&parse_c::x, y, NULL))
#define CL(x,y,z)	(Call(&parse_c::x, y, z))

//GUIDELINES:
//The list should only really be incremented if the function succeeds AND advance is set. Otherwise, return it like it was
//Tree stuff:	A function should only pass a node when advancing.
//				A function should pass a newly inserted node related to itself for sub-functions.
//				FUNCTION_DEFINITION looks tough... insl or insr? how do you add the identifier? probably need a save/restore system for the tree
//				ex. DECLARATION_SPECIFIER should create a new leaf identifying itself to STORAGE_SPECIFIER iff there is certainly a STORAGE_SPECIFIER

//Should this whole system be re-written?
//Ex. Run a function, returns yes or no and also the index in the list, try the next func, so on.

void parse_c::Parse(llist_c* _list)
{
	list = _list;

	UNIT(false, &root);

	root.Disp();

#if 0
	kv_c kv("TEST", CODE_NONE);
	kv_t kvt, kvtr;
	tnode_c* tn;

	root.Set(&kv);

	strcpy_s(kvt.k, "TESTL");
	kvt.v = CODE_NONE;
	strcpy_s(kvtr.k, "TESTR");
	kvtr.v = CODE_NONE;

	root.InsL(kvt);
	root.InsR(kvtr);

	strcpy_s(kvt.k, "TESTLL");
	tn = root.GetL();
	tn->InsL(kvt);

	strcpy_s(kvt.k, "TESTLR");
	tn->InsR(kvt);

	printf("");

	root.Disp();
#endif
}

parse_c::rcode_t parse_c::UNIT(GF_ARGS)
{//{ <external_declaration> }*


	if (GETCP(CODE_NONE))
	{
		printf("\n================\n\nEmpty translation unit\n\n================\n");
		return RC_FAIL;
	}

	subroot->Set("UNIT", NT_UNIT);

	//fix the logic for this loop
	while (CL(EXTERNAL_DECLARATION, true, subroot) == RC_PASS){}

	if (!GETCP(CODE_NONE))
	{//no more external declarations, but the program list is not empty
		printf("\n================\n\nInvalid translation unit\n\n================\n");
		return RC_FAIL;
	}

	printf("\n================\n\nValid translation unit\n\n================\n");
	return RC_PASS;
}

parse_c::rcode_t parse_c::EXTERNAL_DECLARATION(GF_ARGS)
{//<function_definition> | <declaration>
	rcode_t rcode = RC_FAIL;

	if (CL(FUNCTION_DEFINITION, false, NULL) == RC_PASS)
	{
		CL(FUNCTION_DEFINITION, true, subroot->InsL("EXTERNAL_DECL", NT_EXTERNAL_DECL));
		return RC_PASS;
	}
	//if (DECLARATION() == RC_SUCCESS)
	//	return RC_SUCCESS;

	return rcode;
}

parse_c::rcode_t parse_c::FUNCTION_DEFINITION(GF_ARGS)
{// { <declaration_specifier> }* <identifier> ( ) <compound_statement>
	node_c* saved;

	saved = list->Save();
#if 0
	while (1) 
	{
		if (CL(DECLARATION_SPECIFIER ,false, NULL) == RC_FAILURE) //{ <declaration_specifier> }*
			break;
		CL(DECLARATION_SPECIFIER, true, NULL);
	}

	if (CL(DECLARATOR, false, NULL) == RC_FAILURE)
	{//<declarator>
		list->Restore(saved);
		return RC_FAILURE;
	}
	CL(DECLARATOR, true, NULL);

	while (1)
	{
		if (CL(DECLARATION, false, NULL) == RC_FAILURE) //{ <declaration> }*
			break;
		DECLARATION(true, NULL);
	}

	//<compound_statement>
	if (CL(COMPOUND_STATEMENT, false, NULL) == RC_FAILURE)
	{
		list->Restore(saved);
		return RC_FAILURE;
	}

	if (advance)
		CL(COMPOUND_STATEMENT, true, NULL);
	else
		list->Restore(saved);

	return RC_SUCCESS;
#else
	while (1)
	{
		if (CL(DECLARATION_SPECIFIER, false, NULL) == RC_FAIL) //{ <declaration_specifier> }*
			break;
		CL(DECLARATION_SPECIFIER, true, NULL);
	}

	if (CL(IDENTIFIER, false, NULL) == RC_FAIL)
	{
		list->Restore(saved);
		return RC_FAIL;
	}
	CL(IDENTIFIER, true, NULL);// <identifier>

	if (!GETCP(CODE_LPAREN))
	{
		list->Restore(saved);
		return RC_FAIL;
	}
	list->Pop(NULL); // (

	if (!GETCP(CODE_RPAREN))
	{
		list->Restore(saved);
		return RC_FAIL;
	}
	list->Pop(NULL); // )

	if (CL(COMPOUND_STATEMENT, false, NULL) == RC_FAIL)
	{
		list->Restore(saved);
		return RC_FAIL;
	}

	if (advance)
		CL(COMPOUND_STATEMENT, true, NULL); // <compound_statement>
	else
		list->Restore(saved);

	return RC_PASS;
#endif
}



parse_c::rcode_t parse_c::DECLARATION_SPECIFIER(GF_ARGS)
{//<storage_specifier> | <type_specifier>
	rcode_t rcode = RC_FAIL;

	if (CL(STORAGE_SPECIFIER, false, NULL) == RC_PASS)
	{
		if(advance)
			CL(STORAGE_SPECIFIER, true, NULL);
		return RC_PASS;
	}

	if (CL(TYPE_SPECIFIER, false, NULL) == RC_PASS)
	{
		if(advance)
			CL(TYPE_SPECIFIER, true, NULL);
		return RC_PASS;
	}

	return rcode;
}

parse_c::rcode_t parse_c::STORAGE_SPECIFIER(GF_ARGS)
{//stack | heap
	rcode_t rcode = RC_FAIL;

	if (GETCP(CODE_STACK))
	{
		if(advance)
			list->Pop(NULL);

		return RC_PASS;
	}

	if (GETCP(CODE_HEAP))
	{
		if (advance)
			list->Pop(NULL);

		return RC_PASS;
	}

	return rcode;
}

parse_c::rcode_t parse_c::TYPE_SPECIFIER(GF_ARGS)
{//byte | word | label | db | dbp | dba | dw | dwp | dwa | <struct_specifier> | <typedef_name>
	rcode_t rcode = RC_FAIL;

	if (GETCP(CODE_BYTE))
	{
		if (advance)
			list->Pop(NULL);

		return RC_PASS;
	}

	if (GETCP(CODE_WORD))
	{
		if (advance)
			list->Pop(NULL);

		return RC_PASS;
	}

	return rcode;
}

parse_c::rcode_t parse_c::DECLARATOR(GF_ARGS)
{//<direct_declarator>

	return CL(DIRECT_DECLARATOR, advance, NULL);
}

parse_c::rcode_t parse_c::DIRECT_DECLARATOR(GF_ARGS)
{//<identifier> | (<declarator>)
	//| <direct_declarator> [{<constant_expression>} ? ]
	//| <direct_declarator> (<parameter_type_list>)
	//| <direct_declarator> ({ <identifier> }*)

	kv_c save1,  save2;
	node_c* saved;

	saved = list->Save();

	if (GETCP(CODE_TEXT))
	{//<identifier>
		if (advance)
			list->Pop(NULL);

		return RC_PASS;
	}

	if (GETCP(CODE_LPAREN))
	{//(<declarator>)
		
		list->Pop(NULL);

		if (CL(DECLARATOR, false, NULL) != RC_PASS)
		{
			list->Restore(saved);
			return RC_FAIL;
		}

		CL(DECLARATOR, true, NULL);

		if (!GETCP(CODE_RPAREN))
		{
			list->Restore(saved);
			return RC_FAIL;
		}

		if (advance)
			list->Restore(saved);

		return RC_PASS;
	}


	if (CL(DIRECT_DECLARATOR, false, NULL) == RC_FAIL)
		return RC_FAIL; //<direct_declarator>
	CL(DIRECT_DECLARATOR, true, NULL);

	//[{<constant_expression>} ? ]

	if (GETCP(CODE_LPAREN))
	{
		list->Pop(NULL);


		if (GETCP(CODE_LBRACKET))
		{//({ <identifier> })
			list->Pop(NULL);

			//if (!IDENTIFIER())
			//	return RC_FAILURE;

			if (!PEEKCP(CODE_RBRACKET))
			{
				list->Restore(saved);
				return RC_FAIL;
			}

			list->Pop(NULL);

			if (!GETCP(CODE_RPAREN))
			{
				list->Restore(saved);
				return RC_FAIL;
			}

			if (advance)
				list->Pop(NULL);
			else
				list->Restore(saved);

			return RC_PASS;
		}
		else
		{//(<parameter_type_list>)

		}

		list->Restore(saved);
		return RC_FAIL;
	}




	return 0; //fixme
}

GF_DEF(EXPRESSION)
{//TMPTMPTMP <identifier> = = <identifier>
	node_c* saved = list->Save();

	if (CL(IDENTIFIER, false, NULL) == RC_PASS)
	{
		CL(IDENTIFIER, true, NULL);

		if (GETCP(CODE_EQUALS))
		{
			list->Pop(NULL);

			if (GETCP(CODE_EQUALS))
			{
				list->Pop(NULL);

				if (CL(IDENTIFIER, false, NULL) == RC_PASS)
				{
					if (advance)
						CL(IDENTIFIER, true, NULL);
					else
						list->Restore(saved);

					return RC_PASS;
				}
			}
		}

		list->Restore(saved);
	}

	return RC_FAIL;
}


parse_c::rcode_t parse_c::DECLARATION(GF_ARGS)
{//{<declaration_specifier>}+ { <init_declarator> }* ;
	node_c* saved;

	saved = list->Save();

	//{<declaration_specifier>}
	if (CL(DECLARATION_SPECIFIER, false, NULL) == RC_FAIL)
		return RC_FAIL;
	CL(DECLARATION_SPECIFIER, true, NULL);

	while (1)
	{//+
		if (CL(DECLARATION_SPECIFIER, false, NULL) == RC_FAIL)
			break;
		CL(DECLARATION_SPECIFIER, true, NULL);
	}

	while (1)
	{//{ <init_declarator> }*
		if (CL(INIT_DECLARATOR, false, NULL) == RC_FAIL)
			break;
		CL(INIT_DECLARATOR, true, NULL);
	}

	if(!advance)
		list->Restore(saved);

	return RC_PASS;
}

parse_c::rcode_t parse_c::INIT_DECLARATOR(GF_ARGS)
{// <declarator> | <declarator> = <initializer>
	return 0; //fixme 
}

parse_c::rcode_t parse_c::INITIALIZER(GF_ARGS)
{//<assignment_expression> | { <initializer_list> } | { <initializer_list> , }
	return 0; //fixme
}

parse_c::rcode_t parse_c::INITIALIZER_LIST(GF_ARGS)
{//<initializer> | <initializer_list> , <initializer>
	return 0; //fixme
}

//STATEMENTS

parse_c::rcode_t parse_c::COMPOUND_STATEMENT(GF_ARGS)
{//{ { <declaration> }* { <statement> }* }
	node_c* saved;

	saved = list->Save();

	if (!GETCP(CODE_LBRACKET))
		return RC_FAIL;
	list->Pop(NULL);

	while (1)
	{//{ <declaration> }*
		if (CL(DECLARATION, false, NULL) == RC_FAIL)
			break;
		CL(DECLARATION, true, NULL);
	}

	while (1)
	{//{ <statement> }*
		if (CL(STATEMENT, false, NULL) == RC_FAIL)
			break;
		CL(STATEMENT, true, NULL);
	}

	if (!GETCP(CODE_RBRACKET))
	{
		list->Restore(saved);
		return RC_FAIL;
	}

	if (!advance)
		list->Restore(saved);
	else
		list->Pop(NULL); //get the last bracket

	return RC_PASS;
}

#if 0
parse_c::rcode_t parse_c::STATEMENT(GF_ARGS)
{// { <instr_statement> | <compound_statement> | <selection_statement> | <iteration_statement> }
	if (CL(INSTRUCTION_STATEMENT, false, NULL) == RC_SUCCESS)
	{// { <instr_statement>
		if (advance)
			CL(INSTRUCTION_STATEMENT, true, NULL);
		return RC_SUCCESS;
	}
	else if (CL(COMPOUND_STATEMENT, false, NULL) == RC_SUCCESS)
	{// <compound_statement>
		if (advance)
			CL(COMPOUND_STATEMENT, true, NULL);
		return RC_SUCCESS;
	}
	else if (CL(SELECTION_STATEMENT, false, NULL) == RC_SUCCESS)
	{
		if (advance)
			CL(SELECTION_STATEMENT, true, NULL);
		return RC_SUCCESS;
	}
	else if (CL(ITERATION_STATEMENT, false, NULL) == RC_SUCCESS)
	{
		if (advance)
			CL(ITERATION_STATEMENT, true, NULL);
		return RC_SUCCESS;
	}

	return RC_FAILURE;
}

parse_c::rcode_t parse_c::INSTRUCTION_STATEMENT(GF_ARGS)
{ //{ <instr_add> | <instr_ld> | <instr_jp> };
	node_c* saved = list->Save();

	if (CL(INSTRUCTION_ADD, false, NULL) == RC_SUCCESS)
	{// <instr_add>
		//if (advance)
		CL(INSTRUCTION_ADD, true, NULL);

		if (!GETCP(CODE_SEMICOLON))
		{
			list->Restore(saved);
			return RC_FAILURE;
		}
		list->Pop(NULL);
	}
	else
		return RC_FAILURE;

	// <instr_ld>
	// <instr_jp>

	if (advance)
		list->Restore(saved);
	else
		delete saved;

	return RC_SUCCESS;
}

parse_c::rcode_t parse_c::SELECTION_STATEMENT(GF_ARGS)
{// if ( <expression> ) <statement> | if ( <expression> ) <statement> else <statement>
	node_c* saved = list->Save();

	if (!GETCP(CODE_IF)) //if
		return RC_FAILURE;
	list->Pop(NULL);

	if (!GETCP(CODE_LPAREN))
	{// (
		list->Restore(saved);
		return RC_FAILURE;
	}
	list->Pop(NULL);

	//if (CL(EXPRESSION, false, NULL) == RC_FAILURE)
	{// <expression>
		list->Restore(saved);
		return RC_FAILURE;
	}
	//CL(EXPRESSION, false, NULL);

	if (!GETCP(CODE_RPAREN))
	{// )
		list->Restore(saved);
		return RC_FAILURE;
	}
	list->Pop(NULL);


	return RC_FAILURE;
}

parse_c::rcode_t parse_c::ITERATION_STATEMENT(GF_ARGS)
{

	return RC_FAILURE;
}
#endif

#define PARANOID_TEST	1

GF_DEF(STATEMENT)
{// <open_statement> | <closed_statement>
	if (CL(OPEN_STATEMENT, false, NULL) == RC_PASS)
	{
		CL(OPEN_STATEMENT, true, NULL);
		return RC_PASS;
	}

	if (CL(CLOSED_STATEMENT, false, NULL) == RC_PASS)
	{
		CL(CLOSED_STATEMENT, true, NULL);
		return RC_PASS;
	}

	

	return RC_FAIL;
}

GF_DEF(OPEN_STATEMENT)
{// <selection_clause> <statement> |
//	<selection_clause> <closed_statement> else <open_statement> |
//	<while_clause> <open_statement> |
//	<for_clause> <open_statement>

	node_c* saved = list->Save();
	node_c* saved2;

	if (CL(SELECTION_CLAUSE, false, NULL) == RC_PASS)
	{// <selection_clause>
		CL(SELECTION_CLAUSE, true, NULL);
		saved2 = list->Save(); //save the rest of the list without the above clause for the second possible production

 		if (CL(CLOSED_STATEMENT, false, NULL) == RC_PASS)
		{// <closed_statement>
			CL(CLOSED_STATEMENT, true, NULL);
			if (GETCP(CODE_ELSE))
			{// else
				list->Pop(NULL);
				if (CL(OPEN_STATEMENT, false, NULL) == RC_PASS)
				{// <open_statement>
					if (advance)
						CL(OPEN_STATEMENT, true, NULL);
					else
						list->Restore(saved);
					return RC_PASS;
				}
			}

			list->Restore(saved2);
			//saved = list->Save();
		}

		if (CL(STATEMENT, false, NULL) == RC_PASS)
		{// <statement>
			CL(STATEMENT, true, NULL);

			if (GETCP(CODE_ELSE))
			{
				/* Given the following if else block
					if (cond)
						instr;
					else
						instr;
					
					if(cond) instr will be seen as an open statement and then parsing will fail at the else. 
					This lookahead prevents that
				*/
				list->Restore(saved);
				return RC_FAIL;
			}

			if (!advance)
				list->Restore(saved);

			return RC_PASS;
		}

		list->Restore(saved);
	}
#if !PARANOID_TEST
	if (CL(WHILE_CLAUSE, false, NULL) == RC_PASS)
	{// <while_clause>
		CL(WHILE_CLAUSE, true, NULL);
		if (CL(OPEN_STATEMENT, false, NULL), RC_PASS)
		{// <open_statement>
			if (advance)
				CL(OPEN_STATEMENT, true, NULL);
			else
				list->Restore(saved);
			return RC_PASS;
		}

		list->Restore(saved);
		saved = list->Save();
	}
	
	if (CL(FOR_CLAUSE, false, NULL) == RC_PASS)
	{// <for_clause>
		CL(FOR_CLAUSE, true, NULL);
		if (CL(OPEN_STATEMENT, false, NULL), RC_PASS)
		{// <open_statement>
			if (advance)
				CL(OPEN_STATEMENT, true, NULL);
			else
				list->Restore(saved);
			return RC_PASS;
		}

		list->Restore(saved);
		saved = list->Save();
	}
#endif
	return RC_FAIL;
}

GF_DEF(CLOSED_STATEMENT)
{// <simple_statement> |
//	<selection_clause> <closed_statement> else <closed_statement> |
//	<while_clause> <closed_statement> |
//	<for_clause> <closed_statement>

	node_c* saved = list->Save();

	if (CL(SIMPLE_STATEMENT, false, NULL) == RC_PASS)
	{// <simple_statement>
		if (advance)
			CL(SIMPLE_STATEMENT, true, NULL);
		else
			list->Restore(saved);
		return RC_PASS;
	}

	if (CL(SELECTION_CLAUSE, false, NULL) == RC_PASS)
	{// <selection_clause>
		CL(SELECTION_CLAUSE, true, NULL);

		if (CL(CLOSED_STATEMENT, false, NULL) == RC_PASS)
		{// <closed_statement>
			CL(CLOSED_STATEMENT, true, NULL);
			if (GETCP(CODE_ELSE))
			{// else
				list->Pop(NULL);
				if (CL(CLOSED_STATEMENT, false, NULL) == RC_PASS)
				{// <closed_statement>
					if (advance)
						CL(CLOSED_STATEMENT, true, NULL);
					else
						list->Restore(saved);
					return RC_PASS;
				}
			}
		}

		list->Restore(saved);
		return RC_FAIL;
	}
#if !PARANOID_TEST
	if (CL(WHILE_CLAUSE, false, NULL) == RC_PASS)
	{// <while_clause>
		CL(WHILE_CLAUSE, true, NULL);
		if (CL(CLOSED_STATEMENT, false, NULL) == RC_PASS)
		{
			if (advance)
			{
				CL(CLOSED_STATEMENT, true, NULL);
				delete saved;
			}
			else
				list->Restore(saved);
			return RC_PASS;
		}
	}

	if (CL(FOR_CLAUSE, false, NULL) == RC_PASS)
	{// <for_clause>
		CL(FOR_CLAUSE, true, NULL);
		if (CL(CLOSED_STATEMENT, false, NULL) == RC_PASS)
		{
			if (advance)
			{
				CL(CLOSED_STATEMENT, true, NULL);
				delete saved;
			}
			else
				list->Restore(saved);
			return RC_PASS;
		}
	}
#endif
	return RC_FAIL;
}

GF_DEF(SIMPLE_STATEMENT)
{// repeat <statement> until ( <expression> ) ; |
//	<instr_statement>

	node_c* saved = list->Save();

	if (GETCP(CODE_REPEAT))
	{// repeat
		list->Pop(NULL);
		if (CL(STATEMENT, false, NULL) == RC_PASS)
		{// <statement>
			CL(STATEMENT, true, NULL);

			if (GETCP(CODE_UNTIL))
			{// until
				list->Pop(NULL);

				if (GETCP(CODE_LPAREN))
				{// (
					list->Pop(NULL);

					if (CL(EXPRESSION, false, NULL) == RC_PASS)
					{// <expression>
						CL(EXPRESSION, true, NULL);

						if (GETCP(CODE_RPAREN))
						{// )
							list->Pop(NULL);

							if (GETCP(CODE_SEMICOLON))
							{// ;

								if (advance)
									list->Pop(NULL);
								else
									list->Restore(saved);
								return RC_PASS;
							}
						}
					}
				}
			}
		}

		list->Restore(saved);
		saved = list->Save();
	}

	if (CL(INSTR_STATEMENT, false, NULL) == RC_PASS)
	{
		if (advance)
			CL(INSTR_STATEMENT, true, NULL);
		else
			list->Restore(saved);

		return RC_PASS;
	}

	return RC_FAIL;
}

GF_DEF(INSTR_STATEMENT)
{//{ <instr_add> | <instr_ld> | <instr_jp> };
	node_c* saved = list->Save();
	//!!!MEMLEAK
	if (CL(INSTRUCTION_ADD, false, NULL) == RC_PASS)
	{// <instr_add>
		//if (advance)
		CL(INSTRUCTION_ADD, true, NULL);

		if (!GETCP(CODE_SEMICOLON))
		{
			list->Restore(saved);
			return RC_FAIL;
		}
		list->Pop(NULL);
	}
	else
		return RC_FAIL;

	// <instr_ld>
	// <instr_jp>

	if (advance)
		list->Restore(saved);

	return RC_PASS;
}

GF_DEF(SELECTION_CLAUSE)
{// if ( <expression> )
	node_c* saved = list->Save();

	if (GETCP(CODE_IF))
	{// if
		list->Pop(NULL);
		if (GETCP(CODE_LPAREN))
		{// (
			list->Pop(NULL);

			if (CL(EXPRESSION, false, NULL) == RC_PASS)
			{// <expression>
				CL(EXPRESSION, true, NULL);

				if (GETCP(CODE_RPAREN))
				{// )
					if (advance)
						list->Pop(NULL);
					else
						list->Restore(saved);

					return RC_PASS;
				}
			}
		}

		list->Restore(saved); //Popped at least one thing off
	}

	return RC_FAIL;
}

GF_DEF(FOR_CLAUSE)
{
	return RC_FAIL;
}

GF_DEF(WHILE_CLAUSE)
{
	return RC_FAIL;
}

//INSTRUCTIONS

parse_c::rcode_t parse_c::INSTRUCTION_ADD(GF_ARGS)
{// add <identifier> , <rvalue> { , <rvalue> }*
	node_c* saved = list->Save();
	kv_c* saved_comma = NULL;

	if (!GETCP(CODE_ADD))
	{// add
		return RC_FAIL;
	}
	list->Pop(NULL);

	if (CL(IDENTIFIER, false, NULL) == RC_FAIL)
	{// <identifier>
		list->Restore(saved);
		return RC_FAIL;
	}
	CL(IDENTIFIER, true, NULL);

	if (!GETCP(CODE_COMMA))
	{// ,
		list->Restore(saved);
		return RC_FAIL;
	}
	list->Pop(NULL);

	if (CL(RVALUE, false, NULL) == RC_FAIL)
	{// <rvalue>
		list->Restore(saved);
		return RC_FAIL;
	}
	CL(RVALUE, true, NULL);


	if (CL(RVALUE_LIST, false, NULL) == RC_PASS)
	{// <rvalue_list>
		CL(RVALUE_LIST, true, NULL);
	}

	if (advance)
	{
		if(saved_comma) //restore any lone commas
			list->Push(saved_comma);
	}
	else
		list->Restore(saved);

	return RC_PASS;
}



parse_c::rcode_t parse_c::IDENTIFIER(GF_ARGS)
{
	if (!GETCP(CODE_TEXT))
		return RC_FAIL;

	if (advance)
		list->Pop(NULL);

	return RC_PASS;
}

parse_c::rcode_t parse_c::RVALUE(GF_ARGS)
{// <identifier> | <number>
	//NOT DONE. Subject to possible name change

	if (CL(IDENTIFIER, false, NULL) == RC_PASS)
	{
		if (advance)
			CL(IDENTIFIER, true, NULL);
		return RC_PASS;
	}

	if (GETCP(CODE_NUM_BIN) || GETCP(CODE_NUM_DEC) || GETCP(CODE_NUM_HEX))
	{
		if (advance)
			list->Pop(NULL);
		return RC_PASS;
	}

	return RC_FAIL;
}

GF_DEF(RVALUE_LIST)
{// {, <rvalue> }*
	node_c* saved = list->Save();
	kv_c* saved_comma = NULL;

	//FIXME: Does saved_comma have to be deleted in any of these cases?

	while (1)
	{// {, <rvalue> }*
		if (!GETCP(CODE_COMMA))
			break;
		list->Pop(saved_comma); //could just set saved_comma manually but this is easier

		if (CL(RVALUE, false, NULL) == RC_FAIL)
		{//comma without following rval
			list->Restore(saved);
			return RC_FAIL;
		}
		CL(RVALUE, true, NULL);
	}

	return RC_FAIL;
}



parse_c::rcode_t parse_c::Call(gfunc_t func, GF_ARGS)
{
	rcode_t rc;
	char funcname[DBG_STR_MAX] = "Unknown";

	if (tabs > (DEPTH_MAX * 2) - 2)
		exit(123);

	tabstr[tabs++] = ' ';
	tabstr[tabs++] = ' ';

	for (int i = 0; i < sizeof(fs) / sizeof(fstrans_t); i++)
	{
		if (func == fs[i].f)
		{
			strcpy_s(funcname, fs[i].s);
			break;
		}
	}

	printf("%s%s", tabstr, funcname);

	if (advance)
		printf(" ADV");
	printf("\n");

	rc = (this->*func)(advance, subroot);

	tabstr[--tabs] = '\0';
	tabstr[--tabs] = '\0';

	return rc;
}


#undef PEEKCP
#undef GETCP
#undef CL