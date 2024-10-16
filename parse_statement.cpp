#include "parse.h"

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

		while (1)
		{// <statement>*
			if (!CL(STATEMENT, true, self))
				break;
		}

		if (GETCP(CODE_RBRACKET))
		{// '}'
			list->Pop(&kv);
			if (parent)
				self->InsR(&kv);
			if (!advance)
				list->Restore(saved);

			return true;
		}

		list->Restore(saved);
		if (parent)
			parent->KillChild(self);
	}

	return false;
}

GF_DEF(STATEMENT)
{//<open_statement> | <closed_statement>
	tnode_c* self = NULL;

	if (parent)
		self = parent->InsR("Statement", NT_STMT);

	if (CL(OPEN_STATEMENT, true, self))
	{// <open_statement>
		

		//if (advance)
		//	CL(OPEN_STATEMENT, true, self);

		return true;
	}

	if (CL(CLOSED_STATEMENT, true, self))
	{// <closed_statement>

		//if (advance)
		//	CL(CLOSED_STATEMENT, true, self);

		return true;
	}

	if (parent)
		parent->KillChild(self);

	return false;
}

GF_DEF(OPEN_STATEMENT)
{//<selection_clause> <statement>								|
//<selection_clause> <closed_statement> else <open_statement>	|
//<while_clause> <open_statement>								|
//<for_clause> <open_statement>
	node_c* saved = list->Save();
	tnode_c* self = NULL;
	kv_c kv;

	if (parent)
		self = parent->InsR("Open statement", NT_OPEN_STMT);

	if (CL(SELECTION_CLAUSE, true, self))
	{// <selection_clause>

		if (CL(STATEMENT, true, self))
		{// <statement

			//lookahead for the 'else' here. reject if so
			if (GETCP(CODE_ELSE))
			{
				list->Restore(saved);
				if (parent)
					parent->KillChild(self);

				return false;
			}

			if (!advance)
				list->Restore(saved);

			return true;
		}

		if (CL(CLOSED_STATEMENT, true, self))
		{// <closed_statement>

			if (GETCP(CODE_ELSE))
			{// 'else'
				list->Pop(&kv);
				if (parent)
					self->InsR(&kv);

				if (CL(OPEN_STATEMENT, true, self))
				{// <open_statement>
					if (!advance)
						list->Restore(saved);
					return true;
				}
			}
		}
	}

	list->Restore(saved);
	if (parent)
		parent->KillChild(self);

	return false;
}

GF_DEF(CLOSED_STATEMENT)
{//<simple_statement>											|
//<selection_clause> <closed_statement> else <closed_statement> |
//<while_clause> <closed_statement>								|
//<for_clause> <closed_statement>
	node_c* saved = list->Save();
	tnode_c* self = NULL;
	kv_c kv;
	if (parent)
		self = parent->InsR("Closed statement", NT_CLOSED_STMT);

	if (CL(SIMPLE_STATEMENT, true, self))
	{// <simple_statement>

		return true;
	}

	if (CL(SELECTION_CLAUSE, true, self))
	{// <selection_clause>

		if (CL(CLOSED_STATEMENT, true, self))
		{// <closed_statement>

			if (GETCP(CODE_ELSE))
			{// 'else'
				list->Pop(&kv);
				if (parent)
					self->InsR(&kv);

				if (CL(CLOSED_STATEMENT, true, self))
				{// <closed_statement>
					if (!advance)
						list->Restore(saved);

					return true;
				}
			}
		}
	}

	list->Restore(saved);
	if (parent)
		parent->KillChild(self);

	return false;
}

GF_DEF(SIMPLE_STATEMENT)
{//'repeat' <statement> 'until' '(' <expression> ')' ;	|
//<instr_statement> |
//<data_decl>		|
//<compound_statement>
	tnode_c* self = NULL;
	node_c* saved = list->Save();
	kv_c kv;

	if(parent)
		self = parent->InsR("Simple statement", NT_SIMPLE_STMT);

	if (GETCP(CODE_REPEAT))
	{// 'repeat'
		list->Pop(&kv);
		if (parent)
			self->InsR(&kv);

		if (CL(STATEMENT, false, NULL))
		{// <statement>
			CL(STATEMENT, true, self);

			if (GETCP(CODE_UNTIL))
			{// 'until'
				list->Pop(&kv);
				if (parent)
					self->InsR(&kv);

				if (GETCP(CODE_LPAREN))
				{// '('
					list->Pop(&kv);
					if (parent)
						self->InsR(&kv);

					//if (CL(EXPRESSION, false, NULL))
					{ // <expression>
						//CL(EXPRESSION, true, self);

						if (GETCP(CODE_RPAREN))
						{// ')'
							list->Pop(&kv);
							if (parent)
								self->InsR(&kv);
							if (GETCP(CODE_SEMICOLON))
							{
								if (advance)
									list->Pop(&kv);
								else
									list->Restore(saved);
								if (parent)
									self->InsR(&kv);

								return true;
							}
						}
					}
				}
			}
		}

		list->Restore(saved);
		if (parent)
			parent->KillChild(self);
	}
	else if (CL(DATA_DECL, true, self))
	{// <data_decl>

		return true;
	}
	else if (CL(COMPOUND_STATEMENT, true, self))
	{// <compound_statement>

		return true;
	}

	list->Restore(saved);
	if (parent)
		parent->KillChild(self);
	return false;
}

GF_DEF(SELECTION_CLAUSE)
{//'if' '(' <expression> ')'
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

			if (CL(LOGICAL_EXPRESSION, true, self))
			{//<expression>
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