#include "parse.h"

#define PEEKCP(x)	(list->Peek()->V() == x)
#define GETCP(x)	(list->Get()->V() == x)
//#define CL(x, y)	(Call(&parse_c::x, y, NULL))
#define CL(x,y,z)	(Call(&parse_c::x, y, z))

//GUIDELINES:
//The list should only really be incremented if the function succeeds AND advance is set. Otherwise, return it like it was
//Tree stuff:	A function should only pass a node when advancing.
//				A function should pass a newly inserted node related to itself for sub-functions.
//				Upon failure, a function will kill its associated node (and all its children). This is done from the passed parent node

void parse_c::Parse(llist_c* _list)
{
	list = _list;
#if NEW

	bool success = CL(TRANSLATION_UNIT, true, &root);

	if (success)
	{
		printf("\n\n");
		root.Disp();
		printf("\n================\n\nValid translation unit\n\n================\n");
	}
	else
		printf("\n================\n\nInvalid translation unit\n\n================\n");


#else
	//UNIT(false, &root);
	//root.Disp();
#endif

}

#if NEW

//============================================================================
//Main prog control
//============================================================================

GF_DEF(TRANSLATION_UNIT)
{
	parent->Set("Translation unit", NT_UNIT);
	while (CL(EXTERNAL_DECL, false, NULL))	{ CL(EXTERNAL_DECL, true, parent); }

	if (GETCP(CODE_NONE))
		return 1;//nothing left

	return 0;
}

GF_DEF(EXTERNAL_DECL)
{
	if (CL(FUNC, false, NULL))
	{
		if(advance)
			CL(FUNC, true, parent->InsR("External decl", NT_EXTERNAL_DECL));
		return true;
	}

	if (CL(DATA_DECL, false, NULL))
	{
		if (advance)
			CL(DATA_DECL, true, parent->InsR("External decl", NT_DATA_DECL));
		return true;
	}

	return 0;
}

GF_DEF(FUNC)
{ //'subr' <identifier> '(' <parameter_list>+ ')' ';'
  //'subr' <identifier> '(' <parameter_list>+ ')' <compound_statement>
  //cheat here. Combine a decl and a def into one function due to their similarities.
	tnode_c* self = NULL;
	kv_c kv;
	node_c* saved = list->Save();

	if (GETCP(CODE_SUBR))
	{
		list->Pop(&kv);
		if (advance)
		{
			self = parent->InsR("Func decl", NT_FUNC_DECL); //could still be a def, though
			self->InsR(&kv); // 'subr'
		}

		if (CL(IDENTIFIER, false, NULL))
		{
			CL(IDENTIFIER, true, self); // <identifier>

			if (GETCP(CODE_LPAREN))
			{
				list->Pop(&kv);
				if(advance)
					self->InsR(&kv); // '('

				if (CL(PARAMETER_LIST, false, NULL)) //there can be no parameters, too
					CL(PARAMETER_LIST, true, self); // <parameter_list>

				if (GETCP(CODE_RPAREN))
				{
					list->Pop(&kv);
					if (advance)
						self->InsR(&kv); // ')'

					if (GETCP(CODE_SEMICOLON))
					{
						if (advance)
						{
							list->Pop(&kv);
							self->InsR(&kv); // ';'
						}
						else
							list->Restore(saved);

						return true;
					}
					else if CL(COMPOUND_STATEMENT, false, NULL)
					{//function def
						if (advance)
							CL(COMPOUND_STATEMENT, true, self);
						else
							list->Restore(saved);
						return true;
					}
				}
			}
		}

		if(parent)
			parent->KillChild(self);
		list->Restore(saved);
	}

	return false;
}

GF_DEF(DATA_DECL)
{//<data_type> <single_data_decl> { ',' <single_data_decl> }* ';' |
//<array_data_type> <identifier> '=' '{' <initializer_list> '}' ';' |
//<array_data_type> <identifier> '[' <constant_expression> ']' { '=' '{' <initializer_list> '}' } + ';'
	node_c* saved = list->Save();
	node_c* comma_saved;
	tnode_c* self = NULL;
	kv_c kv;

	if (CL(DATA_TYPE, false, NULL))
	{//<data_type>
		if (parent)
			self = parent->InsR("Data declaration", NT_DATA_DECL);
		CL(DATA_TYPE, true, self);

		if (CL(SINGLE_DATA_DECL, false, NULL))
		{// <single_data_decl>
			CL(SINGLE_DATA_DECL, true, self);

			while (1)
			{//{ ',' <single_data_decl> }*
				if (!GETCP(CODE_COMMA))
					break;
				comma_saved = list->Save();
				list->Pop(&kv);

				if (CL(SINGLE_DATA_DECL, false, NULL))
				{
					if(parent)
						self->InsR(&kv);
					CL(SINGLE_DATA_DECL, true, self);
				}
				else
				{//replace that comma here
					list->Restore(comma_saved);
					break;
				}
			}

			if (GETCP(CODE_SEMICOLON))
			{
				if (advance)
				{
					list->Pop(&kv);
					self->InsR(&kv);
				}
				else
					list->Restore(saved);
				return true;
			}
		}

		list->Restore(saved);
		if (parent)
			parent->KillChild(self);
	}
	//else if (CL(ARRAY_DATA_TYPE, false, NULL))

	return  false;
}

GF_DEF(SINGLE_DATA_DECL)
{//<identifier> { '=' <initializer> }+
	node_c* saved = list->Save();
	tnode_c* self = NULL;
	kv_c kv;

	if (CL(IDENTIFIER, false, NULL))
	{// <identifier>
		if (parent)
			self = parent->InsR("Single data declaration", NT_SINGLE_DATA_DECL);
		CL(IDENTIFIER, true, self);

		//if (GETCP(CODE_EQUALS))
		{// '='

		}

		if (!advance)
			list->Restore(saved);
		
		return true;
	}

	return false;
}

//============================================================================
//Data
//============================================================================

GF_DEF(DATA_TYPE)
{
	kv_c kv;

	if (GETCP(CODE_BYTE))
	{
		if (!advance)
			return true;

		list->Pop(&kv);
		if (parent)
			parent->InsR("Data", NT_DATA_TYPE)->InsR(&kv);
		return true;
	}

	return false;
}

//============================================================================
//Statements
//============================================================================

GF_DEF(COMPOUND_STATEMENT)
{//'{' <statement>* '}'
	node_c* saved = list->Save();
	tnode_c* self = NULL;
	kv_c kv;

	if (GETCP(CODE_LBRACKET))
	{// '{'
		list->Pop(&kv);
		if (parent)
		{
			self = parent->InsR("Compound statement", NT_COMPOUND_STMT);
			self->InsR(&kv);
		}

		//if (CL(STATEMENT, false, NULL))
		{
			//CL(STATEMENT, true, self);
			if (GETCP(CODE_RBRACKET))
			{
				list->Pop(&kv);
				if (parent)
					self->InsR(&kv);
				if (!advance)
					list->Restore(saved);

				return true;
			}
		}
		list->Restore(saved);
		if (parent)
			parent->KillChild(self);
	}

	return false;
}

GF_DEF(STATEMENT)
{
	return false;
}

GF_DEF(OPEN_STATEMENT)
{//<selection_clause> <statement>								|
//<selection_clause> <closed_statement> else <open_statement>	|
//<while_clause> <open_statement>								|
//<for_clause> <open_statement>
	

	return false;
}

GF_DEF(CLOSED_STATEMENT)
{//<simple_statement>											|
//<selection_clause> <closed_statement> else <closed_statement> |
//<while_clause> <closed_statement>								|
//<for_clause> <closed_statement>

	return false;
}

GF_DEF(SIMPLE_STATEMENT)
{//repeat <statement> until ( <expression> ) ;	|
//<instr_statement> |
//	<data_decl>		|
//{ <simple_statement>* } ? ? ? ? ? ? ? ?

	return false;
}

GF_DEF(SELECTION_CLAUSE)
{//'if' ( <expression> )
	node_c* saved = list->Save();
	tnode_c* self = NULL;
	kv_c kv;

	if (GETCP(CODE_IF))
	{// 'if'
		list->Pop(&kv);
		if (parent)
		{
			self = parent->InsR("Selection clause", NT_SELECTION_CLAUSE);
			self->InsR(&kv);
		}

		if (GETCP(CODE_LPAREN))
		{// '('
			list->Pop(&kv);
			if (parent)
				self->InsR(&kv);

			//if (CL(EXPRESSION, false, NULL))
			{//<expression>
				//CL(EXPRESSION, true, self);

				if (GETCP(CODE_RPAREN))
				{// ')'
					list->Pop(&kv);
					if (parent)
						self->InsR(&kv);
					if (!advance)
						list->Restore(saved);

					return true;
				}
			}
		}

		list->Restore(saved);
		if (parent)
			parent->KillChild(self);
	}

	return false;
}



//============================================================================
//Misc
//============================================================================

GF_DEF(PARAMETER)
{ // <data_type> <identifier>
	node_c* saved = list->Save();
	tnode_c* self = NULL;

	if (CL(DATA_TYPE, false, NULL))
	{
		if (parent)
			self = parent->InsR("Parameter", NT_PARAMETER);

		CL(DATA_TYPE, true, self); // <data_type>

		if (CL(IDENTIFIER, false, NULL))
		{
			if (advance)
				CL(IDENTIFIER, true, self); // <identifier>
			else
				list->Restore(saved);

			return true;
		}

		list->Restore(saved);
	}

	return false;
}

GF_DEF(PARAMETER_LIST)
{ //<parameter> { ',' <parameter> }*
	node_c* saved = list->Save();
	node_c* comma_saved;
	tnode_c* self = NULL;
	kv_c kv;

	if (CL(PARAMETER, false, NULL))
	{
		if (parent)
			self = parent->InsR("Parameter list", NT_PARAMETER_LIST);
		CL(PARAMETER, true, self); // <parameter>

		while (1)
		{
			if (!GETCP(CODE_COMMA))
				break;
			comma_saved = list->Save();
			list->Pop(&kv);

			if (CL(PARAMETER, false, NULL))
			{
				if(parent)
					self->InsR(&kv);
				CL(PARAMETER, true, self);
			}
			else
			{//replace that comma here
				list->Restore(comma_saved);
				break;
			}
		}

		if (!advance)
			list->Restore(saved);

		return true;
	}
	
	return false;
}

GF_DEF(IDENTIFIER)
{
	kv_c kv;
	if (GETCP(CODE_TEXT))
	{
		if (advance)
		{
			list->Pop(&kv);
			if(parent)
				parent->InsR(&kv);
		}
		return true;
	}
	return false;
}

#else
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
	tnode_c* funcdef = NULL;
	kv_c kv;

	saved = list->Save();
	if(subroot)
		funcdef = subroot->InsR("FUNC_DEF", NT_FUNC_DEF);

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
		CL(DECLARATION_SPECIFIER, true, funcdef);
	}

	if (CL(IDENTIFIER, false, NULL) == RC_FAIL)
	{
		list->Restore(saved);
		if (subroot)
			subroot->KillAllChildren();
		return RC_FAIL;
	}
	CL(IDENTIFIER, true, funcdef);// <identifier>

	if (!GETCP(CODE_LPAREN))
	{
		list->Restore(saved);
		if (subroot)
			subroot->KillAllChildren();
		return RC_FAIL;
	}
	//list->Pop(NULL); // (
	list->Pop(&kv);
	if(funcdef)
		funcdef->InsR(&kv);

	if (!GETCP(CODE_RPAREN))
	{
		list->Restore(saved);
		if (subroot)
			subroot->KillAllChildren();
		return RC_FAIL;
	}
	//list->Pop(NULL); // )
	list->Pop(&kv);
	if(funcdef)
		funcdef->InsR(&kv);

	if (CL(COMPOUND_STATEMENT, false, NULL) == RC_FAIL)
	{
		list->Restore(saved);
		if (subroot)
			subroot->KillAllChildren();
		return RC_FAIL;
	}

	if (advance)
		CL(COMPOUND_STATEMENT, true, funcdef); // <compound_statement>
	else
	{
		if (subroot)
			subroot->KillAllChildren();
		list->Restore(saved);
	}

	return RC_PASS;
#endif
}



parse_c::rcode_t parse_c::DECLARATION_SPECIFIER(GF_ARGS)
{//<storage_specifier> | <type_specifier>
	tnode_c* decl_spec = NULL;

	if (subroot)
		decl_spec = subroot->InsR("DECL_SPEC", NT_DECL_SPEC);

	if (CL(STORAGE_SPECIFIER, false, NULL) == RC_PASS)
	{
		if(advance)
			CL(STORAGE_SPECIFIER, true, decl_spec);
		return RC_PASS;
	}

	if (CL(TYPE_SPECIFIER, false, NULL) == RC_PASS)
	{
		if(advance)
			CL(TYPE_SPECIFIER, true, decl_spec);
		return RC_PASS;
	}

	if(subroot)
		subroot->KillAllChildren();

	return RC_FAIL;
}

parse_c::rcode_t parse_c::STORAGE_SPECIFIER(GF_ARGS)
{//stack | heap
	kv_c kv;
	tnode_c* storage_spec = NULL;

	if (subroot)
		storage_spec = subroot->InsR("STORAGE_SPEC", NT_STORAGE_SPEC);

	if (GETCP(CODE_STACK))
	{
		if (advance)
		{
			list->Pop(&kv);
			if (subroot)
				storage_spec->InsR(&kv);
		}
		return RC_PASS;
	}

	if (GETCP(CODE_HEAP))
	{
		if (advance)
		{
			list->Pop(&kv);
			if (subroot)
				storage_spec->InsR(&kv);
		}

		return RC_PASS;
	}

	if (subroot)
		subroot->KillAllChildren();
	return RC_FAIL;
}

parse_c::rcode_t parse_c::TYPE_SPECIFIER(GF_ARGS)
{//byte | word | label | db | dbp | dba | dw | dwp | dwa | <struct_specifier> | <typedef_name>
	tnode_c* type_spec = NULL;
	kv_c kv;

	if (subroot)
		type_spec = subroot->InsR("TYPE_SPEC", NT_TYPE_SPEC);

	if (GETCP(CODE_BYTE))
	{
		if(advance)
		{
			list->Pop(&kv);
			if (subroot)
				type_spec->InsR(&kv);
		}

		return RC_PASS;
	}

	if (GETCP(CODE_WORD))
	{
		if (advance)
		{
			list->Pop(&kv);
			if (subroot)
				type_spec->InsR(&kv);
		}

		return RC_PASS;
	}

	if (subroot)
		subroot->KillAllChildren();

	return RC_FAIL;
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
	tnode_c* compound_stmt = NULL;
	kv_c kv;

	saved = list->Save();

	if (!GETCP(CODE_LBRACKET))
		return RC_FAIL;

	list->Pop(&kv);
	if (subroot)
	{
		compound_stmt = subroot->InsR("COMPOUND_STMT", NT_COMPOUND_STMT);
		compound_stmt->InsR(&kv);
	}

	while (1)
	{//{ <declaration> }*
		if (CL(DECLARATION, false, NULL) == RC_FAIL)
			break;
		CL(DECLARATION, true, compound_stmt);
	}

	while (1)
	{//{ <statement> }*
		if (CL(STATEMENT, false, NULL) == RC_FAIL)
			break;
		CL(STATEMENT, true, compound_stmt);
	}

	if (!GETCP(CODE_RBRACKET))
	{
		list->Restore(saved);
		if (subroot)
			subroot->KillAllChildren();
		return RC_FAIL;
	}

	if (!advance)
	{
		list->Restore(saved);
		if (subroot)
			subroot->KillAllChildren();
	}
	else
	{
		list->Pop(&kv); //get the last bracket
		if (subroot)
			compound_stmt->InsR(&kv);
	}

	return RC_PASS;
}

#define PARANOID_TEST	1

GF_DEF(STATEMENT)
{// <open_statement> | <closed_statement>
	tnode_c* stmt = NULL;

	if (subroot) //just split this into two in the conditionals below. Fix remove though
		stmt = subroot->InsR("STMT", NT_STMT);

	if (CL(OPEN_STATEMENT, false, NULL) == RC_PASS)
	{
		CL(OPEN_STATEMENT, true, stmt);
		return RC_PASS;
	}

	if (CL(CLOSED_STATEMENT, false, NULL) == RC_PASS)
	{
		CL(CLOSED_STATEMENT, true, stmt);
		return RC_PASS;
	}

	if (subroot)
		subroot->KillChild(stmt);

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
	tnode_c* ident = NULL;
	kv_c kv;

	if (!GETCP(CODE_TEXT))
		return RC_FAIL;

	if (advance)
	{
		list->Pop(&kv);
		if (subroot)
		{
			ident = subroot->InsR("IDENT", NT_IDENT);
			ident->InsR(&kv);
		}
	}

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

#endif

parse_c::rcode_t parse_c::Call(gfunc_t func, GF_ARGS)
{
	rcode_t rc;
	char funcname[DBG_STR_MAX] = "Unknown";

	if (!advance && parent)
		exit(124);

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

	rc = (this->*func)(advance, parent);

	tabstr[--tabs] = '\0';
	tabstr[--tabs] = '\0';

	return rc;
}


#undef PEEKCP
#undef GETCP
#undef CL