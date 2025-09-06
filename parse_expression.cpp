#include "parse.h"

//https://stackoverflow.com/questions/2245962/writing-a-parser-like-flex-bison-that-is-usable-on-8-bit-embedded-systems/2336769#2336769
//https://www.engr.mun.ca/~theo/Misc/exp_parsing.htm#bib


GF_DEF(LOGICAL_EXPRESSION)
{//<or_expression>
	tree_c* self = NULL;

	self = parent->InsR("Logical expression", CODE::NT_LOGICAL_EXPR);

	if (CL(OR_EXPRESSION, self))
		return true; //<or_expression>

	parent->KillChild(self);

	return false;
}

GF_DEF(OR_EXPRESSION)
{//<and_expression> { '||' <and_expression> }*

	node_c* saved = list->Save();
	node_c* bar_saved;
	tree_c* self = NULL;
	tree_c* logical_or = NULL; //unnecessary definitions to shut the compiler up
	kv_c kv;

	self = parent->InsR("Or expression", CODE::NT_OR_EXPR);

	if (CL(AND_EXPRESSION, self))
	{// <and_expression>
		
		while (1)
		{
			if (!GETCP(CODE::BAR))
				break;

			bar_saved = list->Save();

			list->Pop(NULL); //'|'

			if (!GETCP(CODE::BAR))
			{
				list->Restore(bar_saved); //restore list to before the first bar was taken off
				break;
			}

			list->Pop(&kv); //'|'
			logical_or = self->InsR("||", CODE::T_LOGICAL_OR); //combine the two

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
	tree_c* self = NULL;
	tree_c* logical_and = NULL; //unnecessary definitions to shut the compiler up
	kv_c kv;

	self = parent->InsR("And expression", CODE::NT_AND_EXPR);

	if (CL(EQUALITY_EXPRESSION, self))
	{// <equality_expression>

		while (1)
		{
			if (!GETCP(CODE::AMPERSAND))
				break;

			ampersand_saved = list->Save();

			list->Pop(NULL); //'&'

			if (!GETCP(CODE::AMPERSAND))
			{
				list->Restore(ampersand_saved); //restore list to before the first bar was taken off
				break;
			}

			list->Pop(&kv); //'&'
			logical_and = self->InsR("&&", CODE::T_LOGICAL_AND); //combine the two

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
	tree_c* self = NULL;
	tree_c* equals_sign = NULL; //unnecessary definitions to shut the compiler up
	kv_c kv;

	self = parent->InsR("Equality expression", CODE::NT_EQUALITY_EXPR);

	if (CL(RELATIONAL_EXPRESSION,  self))
	{// <equality_expression>
		
		while (1)
		{
			if (!GETCP(CODE::EQUALS) && !GETCP(CODE::EXCLAMATION))
				break;

			operator_saved = list->Save();

			list->Pop(&kv); //'=' | '!'

			if (!GETCP(CODE::EQUALS))
			{
				list->Restore(operator_saved); //restore list to before the first bar was taken off
				break;
			}

			list->Pop(NULL); //'='
			if(kv.Code() == CODE::EXCLAMATION)
				equals_sign = self->InsR("!=", CODE::T_NON_EQUIVALENCE);
			else
				equals_sign = self->InsR("==", CODE::T_EQUIVALENCE);
			

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
	tree_c* self = NULL;
	tree_c* relational_sign = NULL; //unnecessary definitions to shut the compiler up
	kv_c arrow, equal("", CODE::NONE);

	self = parent->InsR("Relational expression", CODE::NT_RELATIONAL_EXPR);

	if (CL(LOGICAL_POSTFIX_EXPRESSION,  self))
	{// <equality_expression>

		while (1)
		{
			if (!GETCP(CODE::LARROW) && !GETCP(CODE::RARROW))
				break;

			operator_saved = list->Save();

			list->Pop(&arrow); //'<' | '>'

			if (GETCP(CODE::EQUALS))
				list->Pop(&equal); //'='

			if (equal.Code() != CODE::NONE)
			{
				if (arrow.Code() == CODE::LARROW)
					relational_sign = self->InsR("<=", CODE::T_LESS_OR_EQUAL);
				else
					relational_sign = self->InsR(">=", CODE::T_GREATER_OR_EQUAL);
			}
			else
			{
				if (arrow.Code() == CODE::LARROW)
					relational_sign = self->InsR("<", CODE::LARROW);
				else
					relational_sign = self->InsR(">", CODE::RARROW);
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
//	{ { { '.' | '..' } <identifier> } | '[' <constant_expression> ']' }*
	//basically an identifier followed by any number of .<ident> or [<expr>]
	node_c* saved = list->Save();
	node_c* saved_op;
	tree_c* child = NULL;
	tree_c* self = NULL;
	kv_c kv;

	self = parent->InsR("LPostfix expression", CODE::NT_LOGICAL_POSTFIX_EXPR);

	if (CL(LOGICAL_PRIMARY_EXPRESSION,  self))
	{//<logical_primary_expression>
		while (1)
		{
			if (GETCP(CODE::PERIOD))
			{// '.'
				saved_op = list->Save();
				list->Pop(&kv);

				if (GETCP(CODE::PERIOD))
				{
					list->Pop(NULL);// '.'
					child = self->InsR("..", CODE::T_DEREF_MEMBER);
				}
				else
					child = self->InsR(&kv);

				if (!CL(IDENTIFIER,  self))
				{
					list->Restore(saved_op);
					self->KillChild(child);
					break;
				}
			}
			else if (GETCP(CODE::LBRACE))
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

				if (!GETCP(CODE::RBRACE))
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
	tree_c* self = NULL;
	kv_c kv;

	self = parent->InsR("LPrimary expression", CODE::NT_LOGICAL_PRIMARY_EXPR);

	if (CL(IDENTIFIER,  self))
	{// <identifier>
		return true;
	}
	else if (CL(CONSTANT, self))
	{// <constant>
		return true;
	}
	else if (GETCP(CODE::STRING))
	{// <string>
		list->Pop(&kv);
		self->InsR(&kv);
		return true;
	}
	else if (GETCP(CODE::LPAREN))
	{// '('
		list->Pop(&kv);

		self->InsR(&kv);

		if (CL(LOGICAL_EXPRESSION,  self))
		{// <logical_expression>

			if (GETCP(CODE::RPAREN))
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
	else if (GETCP(CODE::AMPERSAND) || GETCP(CODE::STAR) || GETCP(CODE::EXCLAMATION))
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
	tree_c* self = parent->InsR("Arithmetic expression", CODE::NT_ARITHMETIC_EXPR);

	if (CL(SHIFT_EXPRESSION,  self))
		return true; // <shift_expression>

	parent->KillChild(self);
	return false;
}

GF_DEF(CONSTANT_EXPRESSION)
{// <shift_expression>
	tree_c* self = parent->InsR("Constant expression", CODE::NT_CONSTANT_EXPR);

	if (CL(SHIFT_EXPRESSION,  self))
		return true; // <shift_expression>

	parent->KillChild(self);
	return false;
}

GF_DEF(SHIFT_EXPRESSION)
{// <additive_expression> { { '<<' | '>>' } <additive_expression> }*
	tree_c* self = parent->InsR("Shift expression", CODE::NT_SHIFT_EXPR);
	tree_c* shift_op = NULL;
	node_c* operator_saved = NULL;
	kv_c first, second;

	if (CL(ADDITIVE_EXPRESSION,  self))
	{// <equality_expression>

		while (1)
		{
			if (!GETCP(CODE::LARROW) && !GETCP(CODE::RARROW))
				break;

			operator_saved = list->Save();

			list->Pop(&first); //'<' | '>'

			if (!GETCP(CODE::LARROW) && !GETCP(CODE::RARROW))
			{
				list->Restore(operator_saved); //restore list to before the first bar was taken off
				break;
			}

			list->Pop(&second); //'<' | '>'
			if (second.Code() == first.Code())
			{
				if(first.Code() == CODE::LARROW)
					shift_op = self->InsR("<<", CODE::T_LEFT_SHIFT);
				else
					shift_op = self->InsR(">>", CODE::T_RIGHT_SHIFT);

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
	tree_c* self = parent->InsR("Additive expression", CODE::NT_ADDITIVE_EXPR);
	kv_c kv;
	node_c* operator_saved = NULL;
	tree_c* additive_op = NULL;

	
	if (CL(MULTIPLICATIVE_EXPRESSION,  self))
	{//<multiplicative_expression>

		while (1)
		{
			if (!GETCP(CODE::PLUS) && !GETCP(CODE::MINUS))
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
	tree_c* self = parent->InsR("Multiplicative expression", CODE::NT_MULTIPLICATIVE_EXPR);
	kv_c kv;
	node_c* operator_saved = NULL;
	tree_c* multiplicative_op = NULL;


	if (CL(ARITHMETIC_POSTFIX_EXPRESSION,  self))
	{//<arithmetic_postfix_expression>

		while (1)
		{
			if (!GETCP(CODE::STAR) && !GETCP(CODE::FSLASH) && !GETCP(CODE::PERCENT))
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
//	{ { { '.' | '..' } <identifier> } | '[' <arithmetic_expression> ']' }*
	//basically an identifier followed by any number of .<ident> or [<expr>]
	node_c* saved = list->Save();
	node_c* saved_op;
	tree_c* child = NULL;
	tree_c* self = NULL;
	kv_c kv;

	self = parent->InsR("APostfix expression", CODE::NT_ARITHMETIC_POSTFIX_EXPR);

	if (CL(ARITHMETIC_PRIMARY_EXPRESSION,  self))
	{//<logical_primary_expression>
		while (1)
		{
			if (GETCP(CODE::PERIOD))
			{// '.'
				saved_op = list->Save();
				list->Pop(&kv);

				if (GETCP(CODE::PERIOD))
				{
					list->Pop(NULL);// '.'
					child = self->InsR("..", CODE::T_DEREF_MEMBER);
				}
				else
					child = self->InsR(&kv);

				if (!CL(IDENTIFIER,  self))
				{
					list->Restore(saved_op);
					self->KillChild(child);
					break;
				}
			}
			else if (GETCP(CODE::LBRACE))
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

				if (!GETCP(CODE::RBRACE))
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
	tree_c* self = NULL;
	kv_c kv;

	self = parent->InsR("APrimary expression", CODE::NT_ARITHMETIC_PRIMARY_EXPR);

	//<constant>

	//<string>

	if (CL(IDENTIFIER,  self))
	{// <identifier>
		return true;
	}
	else if (CL(CONSTANT, self))
	{// <identifier>
		return true;
	}
	else if (GETCP(CODE::LPAREN))
	{// '('
		list->Pop(&kv);

		self->InsR(&kv);

		if (CL(ARITHMETIC_EXPRESSION,  self))
		{// <arithmetic_expression>

			if (GETCP(CODE::RPAREN))
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
	else if (GETCP(CODE::AMPERSAND) || GETCP(CODE::STAR) || GETCP(CODE::MINUS)) 
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

//
//Memory expressions
//

GF_DEF(MEMORY_EXPRESSION)
{//<memory_primary_expression> { { { '.' | '..' } <identifier> } | '[' <const_expression> ']' }*
	node_c* saved = list->Save();
	node_c* saved_op;
	tree_c* child = NULL;
	tree_c* self = NULL;
	kv_c kv;

	self = parent->InsR("Memory expression", CODE::NT_MEMORY_EXPR);

	if (CL(MEMORY_PRIMARY_EXPRESSION, self))
	{//<logical_primary_expression>
		while (1)
		{
			if (GETCP(CODE::PERIOD))
			{// '.'
				saved_op = list->Save();
				list->Pop(&kv);

				if (GETCP(CODE::PERIOD))
				{
					list->Pop(NULL);// '.'
					child = self->InsR("..", CODE::T_DEREF_MEMBER);
				}
				else
					child = self->InsR(&kv);

				if (!CL(IDENTIFIER, self))
				{
					list->Restore(saved_op);
					self->KillChild(child);
					break;
				}
			}
			else if (GETCP(CODE::LBRACE))
			{// '['
				saved_op = list->Save();
				list->Pop(&kv);// '['
				child = self->InsR(&kv);

				if (!CL(CONSTANT_EXPRESSION, self))
				{
					list->Restore(saved_op);
					self->KillChild(child);
					Error("Expected a constant expression after '['");
					break;
				}
				//<constant_expression>

				if (!GETCP(CODE::RBRACE))
				{
					list->Restore(saved_op);
					self->KillChild(child);
					self->KillChild(self->GetR()); //kill the expr
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

GF_DEF(MEM_OR_CONST_EXPRESSION)
{//<memory_expression> | <constant_expression>
	tree_c* self = parent->InsR("Mem or const expression", CODE::NT_MEM_OR_CONST_EXPR);
	node_c* saved = list->Save();

	//note: identifiers are not checked for in syntax parsing of constant_expression.
	//also, this code should only match a constant expression that could not otherwise be matched to a memory expression

	if (CL(MEMORY_EXPRESSION, self))
	{
		if(GETCP(CODE::SEMICOLON) || GETCP(CODE::COMMA))
			return true;

		list->Restore(saved);
		//self->KillAllChildren(); //should work, not confident this is cleaning up everything yet. just needs testing
		self->KillChild(self->GetR());
	}

	//either its not a memory expression or there is some trailing stuff besides a comma
	if (CL(CONSTANT_EXPRESSION, self))
		return true;

	parent->KillChild(self);
	return false;
}

GF_DEF(MEMORY_PRIMARY_EXPRESSION)
{//<identifier> | { '&' | '*' } <memory_expression>
	node_c* saved = list->Save();
	tree_c* self = NULL;
	kv_c kv;

	self = parent->InsR("MPrimary expression", CODE::NT_MEMORY_PRIMARY_EXPR);

	if (CL(IDENTIFIER, self))
	{// <identifier>
		return true;
	}
	else if (GETCP(CODE::AMPERSAND) || GETCP(CODE::STAR))
	{// '&' | '*' 
		list->Pop(&kv);
		self->InsR(&kv);

		if (CL(MEMORY_EXPRESSION, self))
		{//<arithmetic_expression>
			return true;
		}
	}

	list->Restore(saved);
	parent->KillChild(self);

	return false;
}