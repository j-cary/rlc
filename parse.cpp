#include "parse.h"

//GUIDELINES:
//The list should only really be incremented if the function succeeds AND advance is set. Otherwise, return it like it was
//Tree stuff:	A function should only pass a node when advancing.
//				A function should pass a newly inserted node related to itself for sub-functions.
//				Upon failure, a function will kill its associated node (and all its children). This is done from the passed parent node


//upon rejection of the stream from a rule, the list will be UNCHANGED!
//the tree will also be UNCHANGED!

static int maxtab = 0;
static int calls = 0;

void parse_c::Parse(llist_c* _list)
{
	list = _list;

	root.Set("Translation unit", NT_UNIT);
	bool success = CL(TRANSLATION_UNIT, true, &root);

	printf("Max tabs: %i calls: %i\n", maxtab, calls);

	if (success)
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
{
	while (CL(EXTERNAL_DECL, true, parent)) {}

	if (GETCP(CODE_NONE))
		return 1;//nothing left

	return 0;
}

GF_DEF(EXTERNAL_DECL)
{
	tnode_c* self = parent->InsR("External decl", NT_EXTERNAL_DECL);

	if (GETCP(CODE_SUBR))
	{
		if (!CL(FUNC, true, self))
			return false;
		return true;
	}

	if (CL(DATA_DECL, false, NULL))
	{
		if (advance)
			CL(DATA_DECL, true, self);
		return true;
	}

	//TMPTMPTMP!!!
	if (CL(LOGICAL_EXPRESSION, true, self))
	{
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
		if (advance)
		{
			self = parent->InsR("Func decl", NT_FUNC_DECL); //could still be a def, though
			self->InsR(&kv); // 'subr'
		}

		if (CL(IDENTIFIER, true, self))
		{// <identifier>

			if (GETCP(CODE_LPAREN))
			{
				list->Pop(&kv);
				if(advance)
					self->InsR(&kv); // '('

				//there can be no parameters, too
				CL(PARAMETER_LIST, true, self); // <parameter_list>

				if (GETCP(CODE_RPAREN))
				{
					list->Pop(&kv);
					if (advance)
						self->InsR(&kv); // ')'

					if (GETCP(CODE_SEMICOLON))
					{
						list->Pop(&kv);
						self->InsR(&kv); // ';'

						return true;
					}
					else if (CL(COMPOUND_STATEMENT, true, self))
					{//function def
						return true;
					}
				}
			}
		}
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
					if(parent)
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
	else if (CL(ARRAY_DATA_TYPE, false, NULL))
	{// <array_data_type>

		return false; //TMPTMPTMP!!!

		if (parent)
			self = parent->InsR("Data declaration", NT_DATA_DECL);
		CL(ARRAY_DATA_TYPE, true, self);

		if (CL(IDENTIFIER, false, NULL))
		{// <identifier>
			CL(IDENTIFIER, true, self);

			if (GETCP(CODE_EQUALS))
			{// '='

			}
			else if (GETCP(CODE_LBRACKET))
			{// '['

			}
		}

		list->Restore(saved);
		if (parent)
			parent->KillChild(self);
	}

	return  false;
}

GF_DEF(SINGLE_DATA_DECL)
{//<identifier> { '=' <constant_expression> }+
	node_c* saved = list->Save();
	tnode_c* self = NULL;
	kv_c kv;

	if (CL(IDENTIFIER, false, NULL))
	{// <identifier>
		if (parent)
			self = parent->InsR("Single data declaration", NT_SINGLE_DATA_DECL);
		CL(IDENTIFIER, true, self);

		if (GETCP(CODE_EQUALS))
		{// '='
			list->Pop(&kv);
			if (parent)
				self->InsR(&kv);
			/*
			if (CL(CONSTANT_EXPRESSION, false, NULL))
			{// <constant_expression>
				if (advance)
					CL(CONSTANT_EXPRESSION, true, self);
				else
					list->Restore(saved);

				return true;
			}
			*/

			list->Restore(saved);
			if (parent)
				parent->KillChild(self);

			return false;
		}

		if (!advance)
			list->Restore(saved);
		
		return true;
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

	if (parent)
		self = parent->InsR("Parameter list", NT_PARAMETER_LIST);

	if (CL(PARAMETER, true, self))
	{// <parameter>

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

	
#if 1
	printf("%s%s", tabstr, funcname);
	if (advance)
		printf(" ADV");
	printf("\n");
#endif
	
	

	rc = (this->*func)(advance, parent);

	tabstr[--tabs] = '\0';
	tabstr[--tabs] = '\0';

	return rc;
}


#undef PEEKCP
#undef GETCP
#undef CL