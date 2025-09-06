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

void parser_c::Parse(llist_c* _list, tree_c* _root, int _debug)
{
	struct timeb start, end;
	float time_seconds;
	bool result;

	printf("Parsing...\n");
	ftime(&start);

	list = _list;
	debug = _debug;
	root = _root;

	root->Set("Translation unit", CODE::NT_UNIT);
	result = CL(TRANSLATION_UNIT, root);

	ftime(&end);
	time_seconds = (1000 * (end.time - start.time) + (end.millitm - start.millitm)) / 1000.0f;
	printf("Parsing completed in %.4f second(s)\n", time_seconds);

	//printf("Max tabs: %i calls: %i\n", maxtab, calls);


	if (result)
	{
		//printf("\n\n");
		if(debug)
			root->Disp();
		//printf("\n================\n\nValid translation unit\n\n================\n");
	}
	else
	{
		//printf("\n================\n\nInvalid translation unit\n\n================\n");
		Error("Invalid translation unit");
	}

}


//============================================================================
//Main prog control
//============================================================================

GF_DEF(TRANSLATION_UNIT)
{// <external_decl>*
	while (CL(EXTERNAL_DECL, parent)) {}

	if (GETCP(CODE::NONE))
		return 1;//nothing left

	return 0;
}

GF_DEF(EXTERNAL_DECL)
{//<function_decl> | <function_def> | <data_decl> | <type_def>
	tree_c* self = parent->InsR("External decl", CODE::NT_EXTERNAL_DECL);

	if (GETCP(CODE::SUBR))
	{
		if (!CL(FUNC, self))
			return false;
		return true;
	}
	
	if (CL(DATA_DECL, self))
	{
		return true;
	}
	
	if (GETCP(CODE::STRUCT))
	{
		if (!CL(TYPE_DEF, self))
			return false;
		return true;
	}
	
	parent->KillChild(self);

	return false;
}

GF_DEF(FUNC)
{ //'subr' <identifier> '(' <parameter_list>* ')' ';'
  //'subr' <identifier> '(' <parameter_list>* ')' <compound_statement>
  //cheat here. Combine a decl and a def into one function due to their similarities.
	tree_c* self = NULL;
	kv_c kv;
	node_c* saved = list->Save();

	if (GETCP(CODE::SUBR))
	{
		list->Pop(&kv);
		self = parent->InsR("Func decl", CODE::NT_FUNC_DECL); //could still be a def, though
		self->InsR(&kv); // 'subr'
		

		if (CL(IDENTIFIER, self))
		{// <identifier>

			if (GETCP(CODE::LPAREN))
			{
				list->Pop(&kv);
				self->InsR(&kv); // '('

				//there can be no parameters, too
				CL(PARAMETER_LIST, self); // <parameter_list>

				if (GETCP(CODE::RPAREN))
				{
					list->Pop(&kv);
					self->InsR(&kv); // ')'

					if (GETCP(CODE::SEMICOLON))
					{
						list->Pop(&kv);
						self->InsR(&kv); // ';'

						return true;
					}
					else if (CL(COMPOUND_STATEMENT, self))
					{//function def
						self->Set("Func def", CODE::NT_FUNC_DEF);
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



//============================================================================
//Misc
//============================================================================

GF_DEF(INITIALIZER_LIST)
{// <constant_expression> { ',' <constant_expression> }*
	node_c* saved = list->Save();
	node_c* comma_saved;
	tree_c* self;
	tree_c* comma_op;
	kv_c kv;

	self = parent->InsR("Initializer list", CODE::NT_INITIALIZER_LIST);

	if (CL(CONSTANT_EXPRESSION, self))
	{// <constant_expression>

		while (1)
		{
			if (!GETCP(CODE::COMMA))
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
	tree_c* self = parent->InsR("Parameter", CODE::NT_PARAMETER);

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
	tree_c* self = NULL;
	tree_c* comma_op;
	kv_c kv;

	self = parent->InsR("Parameter list", CODE::NT_PARAMETER_LIST);

	if (CL(PARAMETER, self))
	{// <parameter>

		while (1)
		{
			if (!GETCP(CODE::COMMA))
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
	if (GETCP(CODE::TEXT))
	{
		list->Pop(&kv);
		parent->InsR(&kv);
		return true;
	}
	return false;
}

GF_DEF(CONSTANT)
{
	kv_c kv;
	if (GETCP(CODE::NUM_BIN) | GETCP(CODE::NUM_DEC) | GETCP(CODE::NUM_HEX) | GETCP(CODE::NUM_FXD))
	{
		list->Pop(&kv);
		parent->InsR(&kv);
		return true;
	}
	return false;
}


parser_c::rcode_t parser_c::Call(gfunc_t func, GF_ARGS)
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


	
	if (debug > 1)
	{
		printf("%s%s", tabstr, funcname);
		printf("\n");
	}

	
	

	rc = (this->*func)(parent);

	tabstr[--tabs] = '\0';
	tabstr[--tabs] = '\0';

	return rc;
}


#undef PEEKCP
#undef GETCP
#undef CL