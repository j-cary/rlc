#include "parse.h"

//https://stackoverflow.com/questions/2245962/writing-a-parser-like-flex-bison-that-is-usable-on-8-bit-embedded-systems/2336769#2336769
//https://www.engr.mun.ca/~theo/Misc/exp_parsing.htm#bib


GF_DEF(LOGICAL_EXPRESSION)
{//<or_expression>
	tnode_c* self = NULL;

	self = parent->InsR("Logical expression", NT_LOGICAL_EXPR);

	if (CL(OR_EXPRESSION, self))
		return true; //<or_expression>

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

	self = parent->InsR("Or expression", NT_OR_EXPR);

	if (CL(AND_EXPRESSION, self))
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
			logical_or = self->InsR("||", T_LOGICAL_OR); //combine the two

			if (!CL(AND_EXPRESSION, self)) //<and_expression>
			{
				list->Restore(bar_saved); //restore list to before the first bar was taken off
				self->KillChild(logical_or);
				break;
			}
		}

		return true;
	}

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

	self = parent->InsR("And expression", NT_OR_EXPR);

	if (CL(EQUALITY_EXPRESSION, self))
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
			logical_and = self->InsR("&&", T_LOGICAL_AND); //combine the two

			if (!CL(EQUALITY_EXPRESSION, self)) //<equality_expression>
			{
				list->Restore(ampersand_saved); //restore list to before the first bar was taken off
				self->KillChild(logical_and);
				break;
			}
		}

		return true;
	}

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

	self = parent->InsR("Equality expression", NT_OR_EXPR);

	if (CL(RELATIONAL_EXPRESSION,  self))
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
			if(kv.V() == CODE_EXCLAMATION)
				equals_sign = self->InsR("!=", T_NON_EQUIVALENCE); 
			else
				equals_sign = self->InsR("==", T_EQUIVALENCE); 
			

			if (!CL(RELATIONAL_EXPRESSION,  self)) // <relational_expression>
			{
				list->Restore(operator_saved); //restore list to before the first bar was taken off
				self->KillChild(equals_sign);
				break;
			}
		}

		return true;
	}

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

	self = parent->InsR("Relational expression", NT_RELATIONAL_EXPR);

	if (CL(LOGICAL_POSTFIX_EXPRESSION,  self))
	{// <equality_expression>

		while (1)
		{
			if (!GETCP(CODE_LARROW) && !GETCP(CODE_RARROW))
				break;

			operator_saved = list->Save();

			list->Pop(&arrow); //'<' | '>'

			if (GETCP(CODE_EQUALS))
				list->Pop(&equal); //'='

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

			if (!CL(LOGICAL_POSTFIX_EXPRESSION,  self))//<logical_postfix_expression>
			{
				list->Restore(operator_saved); //restore list to before the first bar was taken off
				self->KillChild(relational_sign);
				break;
			}
		}

		return true;
	}

	list->Restore(saved); //is this necessary?
	parent->KillChild(self);

	return false;
}


GF_DEF(LOGICAL_POSTFIX_EXPRESSION)
{//<logical_primary_expression> 
//	{ { { '.' | '..' } <logical_primary_expression> } | '[' <constant_expression> ']' }*
	//basically an identifier followed by any number of .<ident> or [<expr>]
	node_c* saved = list->Save();
	node_c* saved_op;
	tnode_c* child = NULL;
	tnode_c* self = NULL;
	kv_c kv;

	self = parent->InsR("Postfix expression", NT_LOGICAL_POSTFIX_EXPRESSION);

	if (CL(LOGICAL_PRIMARY_EXPRESSION,  self))
	{//<logical_primary_expression>
		while (1)
		{
			if (GETCP(CODE_PERIOD))
			{// '.'
				saved_op = list->Save();
				list->Pop(&kv);

				if (GETCP(CODE_PERIOD))
				{
					list->Pop(NULL);// '.'
					child = self->InsR("..", T_DEREF_MEMBER);
				}
				else
					child = self->InsR(&kv);

				if (!CL(LOGICAL_PRIMARY_EXPRESSION,  self))
				{
					list->Restore(saved_op);
					self->KillChild(child);
					break;
				}
			}
			else if (GETCP(CODE_LBRACE))
			{// '['
				saved_op = list->Save();
				list->Pop(&kv);// '['
				child = self->InsR(&kv);

				if (!CL(CONSTANT_EXPRESSION,  self))
				{
					list->Restore(saved_op);
					self->KillChild(child);
					break;
				}
				//<constant_expression>

				if (!GETCP(CODE_RBRACE))
				{
					list->Restore(saved_op);
					self->KillChild(child);
					self->KillChild(self->GetR()); //kill the const_expr
					break;
				}

				list->Pop(&kv); //']'
				self->InsR(&kv);
			}
			else
				break;// not a '.' or a '['
		}

		return true;
	}

	list->Restore(saved);
	parent->KillChild(self);

	return false;
}

GF_DEF(LOGICAL_PRIMARY_EXPRESSION)
{// <identifier> | <constant> | <string> | '(' <logical_expression> ')' | { '&' | '*' | '!' } <logical_expression>
	node_c* saved = list->Save();
	tnode_c* self = NULL;
	kv_c kv;

	self = parent->InsR("Primary expression", NT_AND_EXPR);

	//<constant>

	//<string>

	if (CL(IDENTIFIER,  self))
	{// <identifier>

		return true;
	}
	else if (GETCP(CODE_LPAREN))
	{// '('
		list->Pop(&kv);

		self->InsR(&kv);

		if (CL(LOGICAL_EXPRESSION,  self))
		{// <logical_expression>

			if (GETCP(CODE_RPAREN))
			{// ')'
				list->Pop(&kv);

				self->InsR(&kv);
				return true;
			}
		}

		list->Restore(saved);
		parent->KillChild(self);

		return false;
	}
	else if (GETCP(CODE_AMPERSAND) || GETCP(CODE_STAR) || GETCP(CODE_EXCLAMATION))
	{// '&' | '*' | '!'
		list->Pop(&kv);
		self->InsR(&kv);

		if (CL(LOGICAL_EXPRESSION,  self))
		{//<logical_expression>

			return true;
		}
	}

	list->Restore(saved);
	parent->KillChild(self);

	return false;
}

//
//Arithmetic expressions
//

GF_DEF(ARITHMETIC_EXPRESSION)
{// <shift_expression>
	tnode_c* self = parent->InsR("Arithmetic expression", NT_ARITHMETIC_EXPR);

	if (CL(SHIFT_EXPRESSION,  self))
		return true; // <shift_expression>

	parent->KillChild(self);
	return false;
}

GF_DEF(CONSTANT_EXPRESSION)
{// <shift_expression>
	tnode_c* self = parent->InsR("Constant expression", NT_CONSTANT_EXPR);

	if (CL(SHIFT_EXPRESSION,  self))
		return true; // <shift_expression>

	parent->KillChild(self);
	return false;
}

GF_DEF(SHIFT_EXPRESSION)
{// <additive_expression> { { '<<' | '>>' } <additive_expression> }*
	tnode_c* self = parent->InsR("Shift expression", NT_SHIFT_EXPRESSION);
	tnode_c* shift_op = NULL;
	node_c* operator_saved = NULL;
	kv_c first, second;

	if (CL(ADDITIVE_EXPRESSION,  self))
	{// <equality_expression>

		while (1)
		{
			if (!GETCP(CODE_LARROW) && !GETCP(CODE_RARROW))
				break;

			operator_saved = list->Save();

			list->Pop(&first); //'<' | '>'

			if (!GETCP(CODE_LARROW) && !GETCP(CODE_RARROW))
			{
				list->Restore(operator_saved); //restore list to before the first bar was taken off
				break;
			}

			list->Pop(&second); //'<' | '>'
			if (second.V() == first.V())
			{
				if(first.V() == CODE_LARROW)
					shift_op = self->InsR("<<", T_LEFT_SHIFT);
				else
					shift_op = self->InsR(">>", T_RIGHT_SHIFT);

			}
			else
			{//something like '<>' or '><'
				list->Restore(operator_saved); //restore list to before the first arrow was taken off
				self->KillChild(shift_op);
				break;
			}


			if (!CL(ADDITIVE_EXPRESSION,  self)) // <relational_expression>
			{
				list->Restore(operator_saved); //restore list to before the first arrow was taken off
				self->KillChild(shift_op);
				break;
			}
		}

		return true;
	}

	parent->KillChild(self);
	return false;
}

GF_DEF(ADDITIVE_EXPRESSION)
{// <multiplicative_expression> { { '+' | '-' } <multiplicative_expression> }*
	tnode_c* self = parent->InsR("Additive expression", NT_ADDITIVE_EXPR);
	kv_c kv;
	node_c* operator_saved = NULL;
	tnode_c* additive_op = NULL;

	
	if (CL(MULTIPLICATIVE_EXPRESSION,  self))
	{//<multiplicative_expression>

		while (1)
		{
			if (!GETCP(CODE_PLUS) && !GETCP(CODE_MINUS))
				break;
			operator_saved = list->Save();
			list->Pop(&kv); // '+' | '-'
			additive_op = self->InsR(&kv);

			if (!CL(MULTIPLICATIVE_EXPRESSION,  self))
			{
				list->Restore(operator_saved);
				self->KillChild(additive_op);
				break;
			}
			//<multiplicative_expression>
		}

		return true;
	}
	
	parent->KillChild(self);

	return false;
}

GF_DEF(MULTIPLICATIVE_EXPRESSION)
{// <arithmetic_postfix_expression> { { '*' | '/' | '%' } <arithmetic_postfix_expression> }*
	tnode_c* self = parent->InsR("Multiplicative expression", NT_ADDITIVE_EXPR);
	kv_c kv;
	node_c* operator_saved = NULL;
	tnode_c* multiplicative_op = NULL;


	if (CL(ARITHMETIC_POSTFIX_EXPRESSION,  self))
	{//<arithmetic_postfix_expression>

		while (1)
		{
			if (!GETCP(CODE_STAR) && !GETCP(CODE_FSLASH) && !GETCP(CODE_PERCENT))
				break;
			operator_saved = list->Save();
			list->Pop(&kv); // '*' | '/' | '%'
			multiplicative_op = self->InsR(&kv);

			if (!CL(ARITHMETIC_POSTFIX_EXPRESSION,  self))
			{
				list->Restore(operator_saved);
				self->KillChild(multiplicative_op);
				break;
			}
			//<arithmetic_postfix_expression>
		}

		return true;
	}

	parent->KillChild(self);
	return false;
}

GF_DEF(ARITHMETIC_POSTFIX_EXPRESSION)
{//<arithmetic_primary_expression> 
//	{ { { '.' | '..' } <arithmetic_primary_expression> } | '[' <arithmetic_expression> ']' }*
	//basically an identifier followed by any number of .<ident> or [<expr>]
	node_c* saved = list->Save();
	node_c* saved_op;
	tnode_c* child = NULL;
	tnode_c* self = NULL;
	kv_c kv;

	self = parent->InsR("Postfix expression", NT_ARITHMETIC_POSTFIX_EXPR);

	if (CL(ARITHMETIC_PRIMARY_EXPRESSION,  self))
	{//<logical_primary_expression>
		while (1)
		{
			if (GETCP(CODE_PERIOD))
			{// '.'
				saved_op = list->Save();
				list->Pop(&kv);

				if (GETCP(CODE_PERIOD))
				{
					list->Pop(NULL);// '.'
					child = self->InsR("..", T_DEREF_MEMBER);
				}
				else
					child = self->InsR(&kv);

				if (!CL(ARITHMETIC_PRIMARY_EXPRESSION,  self))
				{
					list->Restore(saved_op);
					self->KillChild(child);
					break;
				}
			}
			else if (GETCP(CODE_LBRACE))
			{// '['
				saved_op = list->Save();
				list->Pop(&kv);// '['
				child = self->InsR(&kv);

				if (!CL(ARITHMETIC_EXPRESSION,  self))
				{
					list->Restore(saved_op);
					self->KillChild(child);
					break;
				}
				//<constant_expression>

				if (!GETCP(CODE_RBRACE))
				{
					list->Restore(saved_op);
					self->KillChild(child);
					self->KillChild(self->GetR()); //kill the const_expr
					break;
				}

				list->Pop(&kv); //']'
				self->InsR(&kv);
			}
			else
				break;// not a '.' or a '['
		}

		return true;
	}

	list->Restore(saved);
	parent->KillChild(self);

	return false;
}

GF_DEF(ARITHMETIC_PRIMARY_EXPRESSION)
{// <identifier> | <constant> | '(' <arithmetic_expression> ')' | { '&' | '*' | '-' } <arithmetic_expression>
	node_c* saved = list->Save();
	tnode_c* self = NULL;
	kv_c kv;

	self = parent->InsR("Primary expression", NT_AND_EXPR);

	//<constant>

	//<string>

	if (CL(IDENTIFIER,  self))
	{// <identifier>
		return true;
	}
	else if (GETCP(CODE_LPAREN))
	{// '('
		list->Pop(&kv);

		self->InsR(&kv);

		if (CL(LOGICAL_EXPRESSION,  self))
		{// <logical_expression>

			if (GETCP(CODE_RPAREN))
			{// ')'
				list->Pop(&kv);

				self->InsR(&kv);
				return true;
			}
		}

		list->Restore(saved);
		parent->KillChild(self);

		return false;
	}//FIXME: check out this versus the additive expr
	else if (GETCP(CODE_AMPERSAND) || GETCP(CODE_STAR) || GETCP(CODE_MINUS)) 
	{// '&' | '*' | '-'
		list->Pop(&kv);
		self->InsR(&kv);

		if (CL(ARITHMETIC_EXPRESSION,  self))
		{//<arithmetic_expression>
			return true;
		}
	}

	list->Restore(saved);
	parent->KillChild(self);

	return false;
}
