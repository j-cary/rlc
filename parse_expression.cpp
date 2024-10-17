#include "parse.h"

//https://stackoverflow.com/questions/2245962/writing-a-parser-like-flex-bison-that-is-usable-on-8-bit-embedded-systems/2336769#2336769
//https://www.engr.mun.ca/~theo/Misc/exp_parsing.htm#bib


#define OPERANDS_MAX	32
int operand_queue[OPERANDS_MAX];
int operator_stack[OPERANDS_MAX];

//680 - var1 && var2

GF_DEF(LOGICAL_EXPRESSION)
{//<or_expression>
	tnode_c* self = NULL;

	if (parent)
		self = parent->InsR("Logical expression", NT_LOGICAL_EXPR);

	if (CL(OR_EXPRESSION, true, self))
	{//<or_expression>
		return true;
	}

	if (parent)
		parent->KillChild(self);

	return false;
}

GF_DEF(OR_EXPRESSION)
{//<and_expression> { '||' <and_expression> }*

	node_c* saved = list->Save();
	node_c* bar_saved;
	tnode_c* self = NULL;
	tnode_c* logical_or = NULL; //unnecessary definitions to shut the compiler up
	kv_c kv;

	if (parent)
		self = parent->InsR("Or expression", NT_OR_EXPR);

	if (CL(AND_EXPRESSION, true, self))
	{// <and_expression>
		
		while (1)
		{
			if (!GETCP(CODE_BAR))
				break;

			bar_saved = list->Save();

			list->Pop(NULL); //'|'

			if (!GETCP(CODE_BAR))
			{
				list->Restore(bar_saved); //restore list to before the first bar was taken off
				break;
			}

			list->Pop(&kv); //'|'
			if (parent)
				logical_or = self->InsR("||", T_LOGICAL_OR); //combine the two

			if (!CL(AND_EXPRESSION, true, self)) //<and_expression>
			{
				list->Restore(bar_saved); //restore list to before the first bar was taken off
				if (parent)
					self->KillChild(logical_or);
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
GF_DEF(AND_EXPRESSION)
{//<equality_expression> { '&&' <equality_expression> }*
	node_c* saved = list->Save();
	node_c* ampersand_saved;
	tnode_c* self = NULL;
	tnode_c* logical_and = NULL; //unnecessary definitions to shut the compiler up
	kv_c kv;

	if (parent)
		self = parent->InsR("And expression", NT_OR_EXPR);

	if (CL(EQUALITY_EXPRESSION, true, self))
	{// <equality_expression>

		while (1)
		{
			if (!GETCP(CODE_AMPERSAND))
				break;

			ampersand_saved = list->Save();

			list->Pop(NULL); //'&'

			if (!GETCP(CODE_AMPERSAND))
			{
				list->Restore(ampersand_saved); //restore list to before the first bar was taken off
				break;
			}

			list->Pop(&kv); //'&'
			if (parent)
				logical_and = self->InsR("&&", T_LOGICAL_AND); //combine the two

			if (!CL(EQUALITY_EXPRESSION, true, self)) //<equality_expression>
			{
				list->Restore(ampersand_saved); //restore list to before the first bar was taken off
				if (parent)
					self->KillChild(logical_and);
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


GF_DEF(EQUALITY_EXPRESSION)
{//<relational_expression> { { '==' | '!=' } <relational_expression> }*
	node_c* saved = list->Save();
	node_c* operator_saved;
	tnode_c* self = NULL;
	tnode_c* equals_sign = NULL; //unnecessary definitions to shut the compiler up
	kv_c kv;

	if (parent)
		self = parent->InsR("Equality expression", NT_OR_EXPR);

	if (CL(RELATIONAL_EXPRESSION, true, self))
	{// <equality_expression>
		
		while (1)
		{
			if (!GETCP(CODE_EQUALS) && !GETCP(CODE_EXCLAMATION))
				break;

			operator_saved = list->Save();

			list->Pop(&kv); //'=' | '!'

			if (!GETCP(CODE_EQUALS))
			{
				list->Restore(operator_saved); //restore list to before the first bar was taken off
				break;
			}

			list->Pop(NULL); //'='
			if (parent)
			{
				if(kv.V() == CODE_EXCLAMATION)
					equals_sign = self->InsR("!=", T_NON_EQUIVALENCE); 
				else
					equals_sign = self->InsR("==", T_EQUIVALENCE); 
			}

			if (!CL(RELATIONAL_EXPRESSION, true, self)) // <relational_expression>
			{
				list->Restore(operator_saved); //restore list to before the first bar was taken off
				if (parent)
					self->KillChild(equals_sign);
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


GF_DEF(RELATIONAL_EXPRESSION)
{//<logical_unary_expression> { {'<' | '>' | '<=' | '>=' } <logical_unary_expression> }*
	node_c* saved = list->Save();
	node_c* operator_saved;
	tnode_c* self = NULL;
	tnode_c* relational_sign = NULL; //unnecessary definitions to shut the compiler up
	kv_c arrow, equal("", CODE_NONE);

	if (parent)
		self = parent->InsR("Relational expression", NT_RELATIONAL_EXPR);

	if (CL(LOGICAL_POSTFIX_EXPRESSION, true, self))
	{// <equality_expression>

		while (1)
		{
			if (!GETCP(CODE_LARROW) && !GETCP(CODE_RARROW))
				break;

			operator_saved = list->Save();

			list->Pop(&arrow); //'<' | '>'

			if (GETCP(CODE_EQUALS))
				list->Pop(&equal); //'='

			if (parent)
			{
				if (equal.V() != CODE_NONE)
				{
					if (arrow.V() == CODE_LARROW)
						relational_sign = self->InsR("<=", T_LESS_OR_EQUAL);
					else
						relational_sign = self->InsR(">=", T_GREATER_OR_EQUAL);
				}
				else
				{
					if (arrow.V() == CODE_LARROW)
						relational_sign = self->InsR("<", CODE_LARROW);
					else
						relational_sign = self->InsR(">", CODE_RARROW);
				}
				
			}

			if (!CL(LOGICAL_POSTFIX_EXPRESSION, true, self))//<and_expression>
			{
				list->Restore(operator_saved); //restore list to before the first bar was taken off
				if (parent)
					self->KillChild(relational_sign);
				break;
			}
		}

		if (!advance)
			list->Restore(saved);
		return true;
	}

	list->Restore(saved); //is this necessary?
	if (parent)
		parent->KillChild(self);

	return false;
}

#if 0
GF_DEF(LOGICAL_UNARY_EXPRESSION)
{//<logical_postfix_expression> { { '&' | '*' | '!' } <logical_unary_expression> }?
	node_c* saved = list->Save();
	tnode_c* self = NULL;
	kv_c kv;

	if (CL(LOGICAL_POSTFIX_EXPRESSION, false, NULL))
	{// <logical_postfix_expression>
		if (parent)
			self = parent->InsR("Unary expression", NT_OR_EXPR);
		CL(LOGICAL_POSTFIX_EXPRESSION, true, self);

		if (GETCP(CODE_AMPERSAND) || GETCP(CODE_STAR) || GETCP(CODE_EXCLAMATION))
		{// '&' | '*' | '!'
			list->Pop(&kv);
			if (parent)
				self->InsR(&kv);

			if (CL(LOGICAL_UNARY_EXPRESSION, false, NULL))
			{//<logical_unary_expression>
				if (advance)
					CL(LOGICAL_UNARY_EXPRESSION, true, self);
				else
					list->Restore(saved);

				return true;
			}
		}

		if (!advance)
			list->Restore(saved);
		return true;
	}

	return false;
}
#endif

GF_DEF(LOGICAL_POSTFIX_EXPRESSION)
{//<logical_primary_expression> { { '.' | '..' } <identifier> }?	|
// <logical_primary_expression> { '[' <constant_expression> ']' }?
	node_c* saved = list->Save();
	tnode_c* self = NULL;
	kv_c kv;

	if (parent)
		self = parent->InsR("Postfix expression", NT_LOGICAL_POSTFIX_EXPRESSION);

	if (CL(LOGICAL_PRIMARY_EXPRESSION, true, self))
	{//<logical_primary_expression>
		
		if (GETCP(CODE_PERIOD))
		{
			list->Pop(&kv);

			if (GETCP(CODE_PERIOD))
			{// '..'
				list->Pop(NULL);
				if (parent)
					self->InsR("..", T_DEREF_MEMBER);
			}
			else
			{// '.'
				if (parent)
					self->InsR(&kv);
			}
			
			if (CL(IDENTIFIER, true, self))
			{// <identifier>
				if (!advance)
					list->Restore(saved);

				return true;
			}

			list->Restore(saved);
			if (parent)
				parent->KillChild(self);

		}
		else if (GETCP(CODE_LBRACE))
		{// '['
			//<expression>
				//']'
		}

		if (!advance)
			list->Restore(saved);
		return true;
	}

	list->Restore(saved);
	if (parent)
		parent->KillChild(self);

	return false;
}

GF_DEF(LOGICAL_PRIMARY_EXPRESSION)
{// <identifier> | <constant> | <string> | '(' <logical_expression> ')' | { '&' | '*' | '!' } <logical_expression>
	node_c* saved = list->Save();
	tnode_c* self = NULL;
	kv_c kv;

	if (CL(IDENTIFIER, false, NULL))
	{// <identifier> 
		if (parent)
			self = parent->InsR("Primary expression", NT_AND_EXPR);
		if (advance)
			CL(IDENTIFIER, true, self);

		return true;
	}

	//<constant>

	//<string>

	if (GETCP(CODE_LPAREN))
	{// '('
		list->Pop(&kv);

		if (parent)
		{
			self = parent->InsR("Primary expression", NT_AND_EXPR);
			self->InsR(&kv);
		}

		if (CL(LOGICAL_EXPRESSION, false, NULL))
		{// <logical_expression>
			CL(LOGICAL_EXPRESSION, true, self);

			if (GETCP(CODE_RPAREN))
			{// ')'
				if (advance)
					list->Pop(&kv);
				else
					list->Restore(saved);

				if (parent)
					self->InsR(&kv);
				return true;
			}
		}

		list->Restore(saved);
		if (parent)
			parent->KillChild(self);

		return false;
	}
	else if (GETCP(CODE_AMPERSAND) || GETCP(CODE_STAR) || GETCP(CODE_EXCLAMATION))
	{// '&' | '*' | '!'
		list->Pop(&kv);
		if (parent)
		{
			self = parent->InsR("Primary expression", NT_AND_EXPR);
			self->InsR(&kv);
		}

		if (CL(LOGICAL_EXPRESSION, false, NULL))
		{//<logical_expression>
			if (advance)
				CL(LOGICAL_EXPRESSION, true, self);
			else
				list->Restore(saved);

			return true;
		}

		list->Restore(saved);
		if (parent)
			parent->KillChild(self);
	}

	return false;
}

//
//Arithmetic expressions
//


//
//Shared
//


