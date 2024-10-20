#include "parse.h"

//GUIDELINES:
//The list should only really be incremented if the function succeeds AND advance is set. Otherwise, return it like it was
//Tree stuff:	A function should only pass a node when advancing.
//				A function should pass a newly inserted node related to itself for sub-functions.
//				Upon failure, a function will kill its associated node (and all its children). This is done from the passed parent node


//upon rejection of the stream from a rule, the list will be UNCHANGED!
//the tree will also be UNCHANGED!


//TODO:
//Double check expressions
//Double check main - data decls 
//Double check statements
//Change over data
//Get rid of 'advance' argument
//get rid of 'parent' checks everywhere

static int maxtab = 0;
static int calls = 0;

void parse_c::Parse(llist_c* _list)
{
	struct timeb start, end;
	int elapsed_time;
	bool result;

	printf("Parsing...\n");
	ftime(&start);

	list = _list;

	root.Set("Translation unit", NT_UNIT);
	result = CL(TRANSLATION_UNIT, &root);

	ftime(&end);
	elapsed_time = (int)(1000.0 * (end.time - start.time)+ (end.millitm - start.millitm));
	printf("Parsing completed in %u millisecond(s)\n", elapsed_time);
	printf("Max tabs: %i calls: %i\n", maxtab, calls);


	if (result)
	{
		printf("\n\n");
		root.Disp();
		printf("\n================\n\nValid translation unit\n\n================\n");
	}
	else
		printf("\n================\n\nInvalid translation unit\n\n================\n");



}


//============================================================================
//Main prog control
//============================================================================

GF_DEF(TRANSLATION_UNIT)
{// <external_decl>*
	while (CL(EXTERNAL_DECL, parent)) {}

	if (GETCP(CODE_NONE))
		return 1;//nothing left

	return 0;
}

GF_DEF(EXTERNAL_DECL)
{//<function_decl> | <function_def> | <data_decl> | <type_def>
	tnode_c* self = parent->InsR("External decl", NT_EXTERNAL_DECL);

	if (GETCP(CODE_SUBR))
	{
		if (!CL(FUNC, self))
			return false;
		return true;
	}
	
	if (CL(DATA_DECL, self))
	{
		return true;
	}
	
	if (GETCP(CODE_TYPE))
	{
		if (!CL(TYPE_DEF, self))
			return false;
		return true;
	}
	
	parent->KillChild(self);

	return false;
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
		self = parent->InsR("Func decl", NT_FUNC_DECL); //could still be a def, though
		self->InsR(&kv); // 'subr'
		

		if (CL(IDENTIFIER, self))
		{// <identifier>

			if (GETCP(CODE_LPAREN))
			{
				list->Pop(&kv);
				self->InsR(&kv); // '('

				//there can be no parameters, too
				CL(PARAMETER_LIST, self); // <parameter_list>

				if (GETCP(CODE_RPAREN))
				{
					list->Pop(&kv);
					self->InsR(&kv); // ')'

					if (GETCP(CODE_SEMICOLON))
					{
						list->Pop(&kv);
						self->InsR(&kv); // ';'

						return true;
					}
					else if (CL(COMPOUND_STATEMENT, self))
					{//function def
						return true;
					}
				}
			}
		}

		parent->KillChild(self);

		list->Restore(saved);
	}


	return false;
}

GF_DEF(DATA_DECL)
{//<data_type> <single_data_decl> { ',' <single_data_decl> }* ';' |
//<array_data_type> <identifier> '=' '{' <initializer_list> '}' ';' |
//<array_data_type> <identifier> '[' <constant_expression> ']' { '=' '{' <initializer_list> '}' } + ';' |
//<unitializeable_data_type> <identifier> ';'
	//<type> <ident> = {<expr>, <expr>, ... <expr>};
	//<type> <ident> [<expr>];
	//<type> <ident> [<expr>] = {<expr>, ... <expr>};
	node_c* saved = list->Save();
	node_c* comma_saved;
	node_c* equals_saved;
	tnode_c* self = parent->InsR("Data declaration", NT_DATA_DECL);
	kv_c kv;

	if (CL(DATA_TYPE, self))
	{// <data_type>
		if (CL(SINGLE_DATA_DECL, self))
		{// <single_data_decl>
			while (1)
			{
				if (!GETCP(CODE_COMMA))
					break;
				comma_saved = list->Save();
				list->Pop(&kv);// ','
				self->InsR(&kv);

				if (!CL(SINGLE_DATA_DECL, self))
				{
					list->Restore(comma_saved);
					parent->KillChild(self);
					break;
				}
				//<single_data_decl>
			}

			if (GETCP(CODE_SEMICOLON))
			{// ';'
				list->Pop(&kv);
				self->InsR(&kv);
				return true;
			}
		}
	}
	else if (CL(ARRAY_DATA_TYPE, self))
	{// <array_data_type>
		if (CL(IDENTIFIER, self))
		{// <identifier>
			if (GETCP(CODE_EQUALS))
			{// '='
				list->Pop(&kv);
				self->InsR(&kv);

				if (GETCP(CODE_LBRACKET))
				{// '{'
					list->Pop(&kv);
					self->InsR(&kv);

					if (CL(INITIALIZER_LIST, self))
					{// <initializer_list>

						if (GETCP(CODE_RBRACKET))
						{// '}'
							list->Pop(&kv);
							self->InsR(&kv);

							if (GETCP(CODE_SEMICOLON))
							{// ';'
								list->Pop(&kv);
								self->InsR(&kv);
								return true;
							}
						}
					}
				}
			}
			else if (GETCP(CODE_LBRACE))
			{// '['
				list->Pop(&kv);
				self->InsR(&kv);

				if (CL(CONSTANT_EXPRESSION, self))
				{// <constant_expression>
					if (GETCP(CODE_RBRACE))
					{// ']'
						list->Pop(&kv);
						self->InsR(&kv);

						if (GETCP(CODE_SEMICOLON))
						{// ';'
							list->Pop(&kv);
							self->InsR(&kv);

							return true;
						}
						else if (GETCP(CODE_EQUALS)) //optional
						{// '='
							//Reeeeaaallly should save/restore all this stuff in case the rest fails. oh well.
							list->Pop(&kv);
							self->InsR(&kv);

							if (GETCP(CODE_LBRACKET))
							{//'{'
								list->Pop(&kv);
								self->InsR(&kv);

								if (CL(INITIALIZER_LIST, self))
								{// <initializer_list>
									if (GETCP(CODE_RBRACKET))
									{//'}'
										list->Pop(&kv);
										self->InsR(&kv);
										if (GETCP(CODE_SEMICOLON))
										{// just handle this separately than the above
											list->Pop(&kv);
											self->InsR(&kv);
											return true;
										}
									}
								}
							}
						}
					}
				}
			}
		}
	}
	else if (GETCP(CODE_LABEL))
	{// <unitializeable_data_type>
		list->Pop(&kv);
		self->InsR(&kv);

		if (CL(IDENTIFIER, self))
		{// <identifier>
			if (GETCP(CODE_SEMICOLON))
			{
				list->Pop(&kv);
				self->InsR(&kv);
				return true;
			}
		}
	}

	parent->KillChild(self);
	list->Restore(saved);

	return  false;
}

GF_DEF(SINGLE_DATA_DECL)
{//<identifier> { '=' <constant_expression> }+
	node_c* saved = list->Save();
	node_c* saved_equals;
	tnode_c* self = parent->InsR("Single data decl", NT_SINGLE_DATA_DECL);
	tnode_c* equals_sign = NULL;
	kv_c kv;

	if (CL(IDENTIFIER, self))
	{//<identifier>
		if (GETCP(CODE_EQUALS))
		{//'='
			saved_equals = list->Save();
			list->Pop(&kv);
			equals_sign = self->InsR(&kv);

			if (!CL(CONSTANT_EXPRESSION, self))
			{//no expression, restore it anyways
				list->Restore(saved_equals);
				self->KillChild(equals_sign);
			}
			//<constant_expression>
		}

		return true;
	}

	parent->KillChild(self);

	return false;
}

GF_DEF(TYPE_DEF)
{// 'type' '{' <data_decl>+ '}' <identifier> ';'
	kv_c kv;
	node_c* saved = list->Save();
	tnode_c* self = parent->InsR("Type def", NT_TYPE_DEF);

	if (GETCP(CODE_TYPE))
	{// 'type'
		list->Pop(&kv);
		self->InsR(&kv);

		if (GETCP(CODE_LBRACKET))
		{// '{'
			list->Pop(&kv);
			self->InsR(&kv);

			if(CL(DATA_DECL, self))
			{// <data_decl>
				while (CL(DATA_DECL, self)){} // <data_decl>*

				if (GETCP(CODE_RBRACKET))
				{// '}'
					list->Pop(&kv);
					self->InsR(&kv);

					if (CL(IDENTIFIER, self))
					{// <identifier>
						if (GETCP(CODE_SEMICOLON))
						{// ';'
							list->Pop(&kv);
							self->InsR(&kv);
							return true;
						}
					}
				}
			}

		}
	}

	list->Restore(saved);
	parent->KillChild(self);

	return false;
}

//============================================================================
//Misc
//============================================================================

GF_DEF(INITIALIZER_LIST)
{// <constant_expression> { ',' <constant_expression> }*
	node_c* saved = list->Save();
	node_c* comma_saved;
	tnode_c* self;
	tnode_c* comma_op;
	kv_c kv;

	self = parent->InsR("Initializer list", NT_INITIALIZER_LIST);

	if (CL(CONSTANT_EXPRESSION, self))
	{// <constant_expression>

		while (1)
		{
			if (!GETCP(CODE_COMMA))
				break;

			comma_saved = list->Save();
			list->Pop(&kv);// ','
			comma_op = self->InsR(&kv);

			if (!CL(CONSTANT_EXPRESSION, self))
			{
				list->Restore(comma_saved);
				self->KillChild(comma_op);
				break;
			}
			//<constant_expression>
		}

		return true;
	}
	list->Restore(saved);
	parent->KillChild(self);

	return false;
}

GF_DEF(PARAMETER)
{ // <data_type> <identifier>
	node_c* saved = list->Save();
	tnode_c* self = parent->InsR("Parameter", NT_PARAMETER);

	if (CL(DATA_TYPE, self))
	{// <data_type>

		if (CL(IDENTIFIER, self))
		{//<identifier>
			return true;
		}

	}

	parent->KillChild(self);
	list->Restore(saved);
	return false;
}

GF_DEF(PARAMETER_LIST)
{ //<parameter> { ',' <parameter> }*
	node_c* saved = list->Save();
	node_c* comma_saved;
	tnode_c* self = NULL;
	tnode_c* comma_op;
	kv_c kv;

	self = parent->InsR("Parameter list", NT_PARAMETER_LIST);

	if (CL(PARAMETER, self))
	{// <parameter>

		while (1)
		{
			if (!GETCP(CODE_COMMA))
				break;
			comma_saved = list->Save();
			list->Pop(&kv);// ','
			comma_op = self->InsR(&kv);

			if (!CL(PARAMETER, self))
			{
				list->Restore(comma_saved);
				self->KillChild(comma_op);
				break;
			}
			// <parameter>
		}

		return true;
	}

	if (parent)
		parent->KillChild(self);
	list->Restore(saved);
	
	return false;
}

GF_DEF(IDENTIFIER)
{
	kv_c kv;
	if (GETCP(CODE_TEXT))
	{
		list->Pop(&kv);
		parent->InsR(&kv);
		return true;
	}
	return false;
}


parse_c::rcode_t parse_c::Call(gfunc_t func, GF_ARGS)
{
	rcode_t rc;
	char funcname[DBG_STR_MAX] = "Unknown";

	if (tabs > (DEPTH_MAX * 2) - 2)
		exit(123);

	tabstr[tabs++] = ' ';
	tabstr[tabs++] = ' ';

	if (tabs > maxtab)
		maxtab = tabs;

	calls++;

	for (int i = 0; i < sizeof(fs) / sizeof(fstrans_t); i++)
	{
		if (func == fs[i].f)
		{
			strcpy_s(funcname, fs[i].s);
			break;
		}
	}

	if (!parent)
		printf("\n====%s has no passed root!====\n\n", funcname);


	
#if 0
	printf("%s%s", tabstr, funcname);
	printf("\n");
#endif
	
	

	rc = (this->*func)(parent);

	tabstr[--tabs] = '\0';
	tabstr[--tabs] = '\0';

	return rc;
}


#undef PEEKCP
#undef GETCP
#undef CL