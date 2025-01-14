#include "parse.h"

//============================================================================
//Statements
//============================================================================

GF_DEF(COMPOUND_STATEMENT)
{//'{' <statement>* '}'
	node_c* saved = list->Save();
	tree_c* self = NULL;
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
			if (!CL(STATEMENT,  self))
				break;
		}

		if (GETCP(CODE_RBRACKET))
		{// '}'
			list->Pop(&kv);
			if (parent)
				self->InsR(&kv);

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
	tree_c* self = NULL;

	if (parent)
		self = parent->InsR("Statement", NT_STMT);

	if (CL(OPEN_STATEMENT,  self))
	{// <open_statement>
		return true;
	}

	if (CL(CLOSED_STATEMENT,  self))
	{// <closed_statement>

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
	tree_c* self = NULL;
	kv_c kv;

	if (parent)
		self = parent->InsR("Open statement", NT_OPEN_STMT);

	if (CL(SELECTION_CLAUSE,  self))
	{// <selection_clause>

		if (CL(STATEMENT,  self))
		{// <statement

			//lookahead for the 'else' here. reject if so
			if (GETCP(CODE_ELSE))
			{
				//TESTME!!!
				//this is necessary given 'if() ... else if() ...'
				//===
				list->Pop(&kv);
				self->InsR(&kv);
				if (CL(OPEN_STATEMENT, self))
					return true;
				//===

				list->Restore(saved);
				if (parent)
					parent->KillChild(self);

				return false;
			}


			return true;
		}

		if (CL(CLOSED_STATEMENT,  self))
		{// <closed_statement>

			if (GETCP(CODE_ELSE))
			{// 'else'
				list->Pop(&kv);
				if (parent)
					self->InsR(&kv);

				if (CL(OPEN_STATEMENT,  self))
				{// <open_statement>
					return true;
				}
			}
		}
	}
#if 1
	if (GETCP(CODE_WHILE))
	{// <while_clause>
		CL(WHILE_CLAUSE, self);

		if (CL(OPEN_STATEMENT, self))
		{// <open_statement>
			return true;
		}
	}

	if (GETCP(CODE_FOR))
	{// <for_clause>
		CL(FOR_CLAUSE, self);

		if (CL(OPEN_STATEMENT, self))
		{// <open_statement>
			return true;
		}
	}
#endif
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
	tree_c* self = NULL;
	kv_c kv;
	if (parent)
		self = parent->InsR("Closed statement", NT_CLOSED_STMT);

	if (CL(SIMPLE_STATEMENT,  self))
	{// <simple_statement>

		return true;
	}

	if (CL(SELECTION_CLAUSE,  self))
	{// <selection_clause>

		if (CL(CLOSED_STATEMENT,  self))
		{// <closed_statement>

			if (GETCP(CODE_ELSE))
			{// 'else'
				list->Pop(&kv);
				if (parent)
					self->InsR(&kv);

				if (CL(CLOSED_STATEMENT,  self))
				{// <closed_statement>

					return true;
				}
			}
		}
	}
#if 1
	if (GETCP(CODE_WHILE))
	{// <while_clause>
		CL(WHILE_CLAUSE, self);

		if (CL(CLOSED_STATEMENT, self))
		{// <closed_statement>
			return true;
		}
	}

	if (GETCP(CODE_FOR))
	{// <for_clause>
		CL(FOR_CLAUSE, self);

		if (CL(CLOSED_STATEMENT, self))
		{// <closed_statement>
			return true;
		}
	}
#endif
	list->Restore(saved);
	if (parent)
		parent->KillChild(self);

	return false;
}

GF_DEF(SIMPLE_STATEMENT)
{//'repeat' <statement> 'until' '(' <expression> ')' ;	|
//<instruction>		|
//<data_decl>		|
//<label_def>		|
//<compound_statement>
	tree_c* self = NULL;
	node_c* saved = list->Save();
	kv_c kv;

	if(parent)
		self = parent->InsR("Simple statement", NT_SIMPLE_STMT);

	if (GETCP(CODE_REPEAT))
	{// 'repeat'
		list->Pop(&kv);
		if (parent)
			self->InsR(&kv);

		if (CL(STATEMENT,  self))
		{// <statement>

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

					if (CL(LOGICAL_EXPRESSION, self))
					{ // <expression>

						if (GETCP(CODE_RPAREN))
						{// ')'
							list->Pop(&kv);
							if (parent)
								self->InsR(&kv);
							if (GETCP(CODE_SEMICOLON))
							{
								list->Pop(&kv);
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
	else if (CL(INSTRUCTION, self))
	{
		return true;
	}
	else if (CL(DATA_DECL,  self))
	{// <data_decl>

		return true;
	}
	else if (CL(COMPOUND_STATEMENT,  self))
	{// <compound_statement>

		return true;
	}
	else if (CL(LABEL_DEF, self))
	{//<label_def>
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
	tree_c* self = NULL;
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

			if (CL(LOGICAL_EXPRESSION,  self))
			{//<expression>
				if (GETCP(CODE_RPAREN))
				{// ')'
					list->Pop(&kv);
					if (parent)
						self->InsR(&kv);

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

GF_DEF(FOR_CLAUSE)
{ //'for' '(' <data_type>  <single_data_decl> ')'
	tree_c* self = parent->InsR("For clause", NT_FOR_CLAUSE);
	node_c* saved = list->Save();
	kv_c kv;

	if (GETCP(CODE_FOR))
	{// 'for'
		list->Pop(&kv);
		self->InsR(&kv);

		if (GETCP(CODE_LPAREN))
		{// '('
			list->Pop(&kv);
			self->InsR(&kv);

			if (CL(DATA_TYPE, self))
			{// <data_type>
				if (CL(SINGLE_DATA_DECL, self))
				{// <single_data_decl>
					if (GETCP(CODE_RPAREN))
					{//')'
						list->Pop(&kv);
						self->InsR(&kv);
						return true;
					}
				}
			}
		}
	}

	//list->Restore(saved);
	parent->KillChild(self);
	return false;
}

GF_DEF(WHILE_CLAUSE)
{// 'while' '(' <logical_expression> ')'
	node_c* saved = list->Save();
	tree_c* self = parent->InsR("While clause", NT_WHILE_CLAUSE);
	kv_c kv;

	if (GETCP(CODE_WHILE))
	{// 'while'
		list->Pop(&kv);
		self->InsR(&kv);

		if (GETCP(CODE_LPAREN))
		{// '('
			list->Pop(&kv);
			self->InsR(&kv);

			if (CL(LOGICAL_EXPRESSION, self))
			{// <logical_expression>
				if (GETCP(CODE_RPAREN))
				{// ')'
					list->Pop(&kv);
					self->InsR(&kv);
					return true;
				}
			}
		}
	}

	list->Restore(saved);
	parent->KillChild(self);
	return false;
}

GF_DEF(LABEL_DEF)
{//<identifier> ':'
	tree_c* self = parent->InsR("Label definition", NT_LABEL_DEF);
	node_c* saved = list->Save();
	kv_c kv;

	if (CL(IDENTIFIER, self))
	{//<identifier>
		if (GETCP(CODE_COLON))
		{// ':'
			list->Pop(&kv);
			self->InsR(&kv);
			return true;
		}
	}

	list->Restore(saved);
	parent->KillChild(self);
	return false;
}