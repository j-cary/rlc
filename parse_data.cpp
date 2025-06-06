#include "parse.h"

GF_DEF(STORAGE_SPECIFIER)
{//'static' | 'auto' | 'stack'
	kv_c kv;
	tree_c* self = parent->InsR("Storage specifier", NT_STORAGE_SPEC);

	if (GETCP(CODE_STATIC) || GETCP(CODE_AUTO) || GETCP(CODE_STACK))
	{
		list->Pop(&kv);
		self->InsR(&kv);
		return true;
	}

	parent->KillChild(self);
	return false;
}

GF_DEF(DATA_MODIFIER)
{//<storage_specifier> | 'signed' | <storage_specifier> 'signed' | 'signed' <storage_specifier>
	kv_c kv;
	tree_c* self = parent->InsR("Data modifier", NT_DATA_MODIFIER);

	if (GETCP(CODE_SIGNED))
	{//'signed'
		list->Pop(&kv);
		self->InsR(&kv);

		CL(STORAGE_SPECIFIER, self); //<storage_specifier>

		return true;
	}
	else if (CL(STORAGE_SPECIFIER, self))
	{//<storage_specifier>

		if (GETCP(CODE_SIGNED))
		{//'signed'
			list->Pop(&kv);
			self->InsR(&kv);
		}

		return true;
	}

	parent->KillChild(self);
	return false;
}

GF_DEF(DATA_TYPE)
{// 'byte' | 'word' | 'dword' | 'fixed' | 'dfixed'
	kv_c kv;
	tree_c* self;
	node_c* saved = list->Save();

	if (GETCP(CODE_BYTE) || GETCP(CODE_WORD) || GETCP(CODE_FIXED))
	{
		list->Pop(&kv);
		self = parent->InsR("Data", NT_DATA_TYPE);
		self->InsR(&kv);

		return true;
	}

	list->Restore(saved);
	return false;
}

GF_DEF(DATA_DECL)
{//<data_decl> ::= <data_modifier>? <data_type> '*'? <single_data_decl> { ',' <single_data_decl> }* ';' |
//<data_modifier>? <data_type> '*'? <identifier> '[' <const_expression> ']' { '{' <initializer_list> '}' }? ';' |
//<struct_decl> | 
//'label' <identifier> ';'

	node_c* saved = list->Save();
	node_c* comma_saved;
	tree_c* self = parent->InsR("Data declaration", NT_DATA_DECL);
	kv_c kv;
#if 0

	if (GETCP(CODE_LABEL))
	{//'label'
		list->Pop(&kv);
		self->InsR(&kv);

		if (!CL(IDENTIFIER, self))
			Error("Expected an identifier after label declaration");

		if (!GETCP(CODE_SEMICOLON))
			Error("Missing ';' after label declaration");

		list->Pop(&kv);
		self->InsR(&kv);

		return true;
	}
	else if (GETCP(CODE_STRUCT))
	{//'struct'
		list->Pop(&kv);
		self->InsR(&kv);

		if (CL(IDENTIFIER, self))
		{//<identifier>
			if (GETCP(CODE_STAR))
			{//'*'
				list->Pop(&kv);
				self->InsR(&kv);
			}

			if (!CL(IDENTIFIER, self)) //<identifier>
				Error("Expected identifier in struct instance declaration\n");

			if (GETCP(CODE_LBRACE))
			{//'['
				list->Pop(&kv);
				self->InsR(&kv);

				if (!CL(CONSTANT_EXPRESSION, self)) //<const_expression>
					Error("Expected an expression in struct array\n");

				if (!GETCP(CODE_RBRACE)) //']'
					Error("Unclosed brace in structure array declaration\n");

				list->Pop(&kv);
				self->InsR(&kv);
			}
		}
		else
		{//this could still be a type declaration
			parent->KillChild(self);
			list->Restore(saved);
			return false;
		}

		if (!GETCP(CODE_SEMICOLON))
			Error("Missing ';' after data declaration");
		list->Pop(&kv);
		self->InsR(&kv);

		return true;
	}
	else
	{
		if (GETCP(CODE_SIGNED))
		{//'signed'
			list->Pop(&kv);
			self->InsR(&kv);

			if (!CL(DATA_TYPE, self)) //already found a 'signed' so just quit out
				Error("Expected data type after type specifier");
		}
		else
		{
			if (!CL(DATA_TYPE, self))
			{//nothing matches at all - restore the list to check for others
				parent->KillChild(self);
				list->Restore(saved);
				return false;
			}
		}

		if (GETCP(CODE_STAR))
		{//'*'?
			list->Pop(&kv);
			self->InsR(&kv);
		}

		if (CL(SINGLE_DATA_DECL, self))
		{//<single_data_decl>
			while (1)
			{
				if (!GETCP(CODE_COMMA))
					break;

				list->Pop(&kv);// ','
				self->InsR(&kv);

				if (!CL(SINGLE_DATA_DECL, self))
					Error("Expected a data declaration after comma");
				//<single_data_decl>
			}
		}
		else if (CL(IDENTIFIER, self))
		{//<identifier>
			if (!GETCP(CODE_LBRACE))
				Error("Expected a '[' after identifier");
			list->Pop(&kv);// '['
			self->InsR(&kv);

			if (!CL(CONSTANT_EXPRESSION, self)) //<constant_expression>
				Error("Expected an expression after '['");

			if (!GETCP(CODE_RBRACE))
				Error("Expected a ']' after expression");
			list->Pop(&kv);// ']'
			self->InsR(&kv);

			if (GETCP(CODE_EQUALS))
			{//'='
				list->Pop(&kv);
				self->InsR(&kv);

				if (!GETCP(CODE_LBRACKET))
					Error("Expected a '{' after '='");

				list->Pop(&kv);//'{'
				self->InsR(&kv);

				if (!CL(INITIALIZER_LIST, self))//<initializer_list>
					Error("Expected an initializer in array declaration");

				if (!GETCP(CODE_RBRACKET))
					Error("Expeceted a '}' after initializer list");

				list->Pop(&kv);//'}'
				self->InsR(&kv);
			}
		}
		else
			Error("Expected an identifier after data type\n");

		if (!GETCP(CODE_SEMICOLON))
			Error("Expected a ';' after data decl");

		list->Pop(&kv);
		self->InsR(&kv);

		return true;
	}

	parent->KillChild(self);
	list->Restore(saved);

#else
	if (GETCP(CODE_LABEL))
	{//'label'
		list->Pop(&kv);
		self->InsR(&kv);

		if (!CL(IDENTIFIER, self))
			Error("Expected an identifier after label declaration");

		if (!GETCP(CODE_SEMICOLON))
			Error("Missing ';' after label declaration");

		list->Pop(&kv);
		self->InsR(&kv);

		return true;
	}
	else if (GETCP(CODE_STRUCT) || PEEKCP(CODE_STRUCT))
	{
		if (CL(STRUCT_DECL, self))
			return true;
	}
	else
	{
		if (CL(DATA_MODIFIER, self))
		{//<data_modifier>?
			if (!CL(DATA_TYPE, self)) 
				Error("Expected data type after data modifier"); //already found a modifier so just quit out
		}
		else
		{
			if (!CL(DATA_TYPE, self))
			{//nothing matches at all - restore the list to check for others
				parent->KillChild(self);
				list->Restore(saved);
				return false;
			}
		}

		if (GETCP(CODE_STAR))
		{//'*'?
			list->Pop(&kv);
			self->InsR(&kv);
		}

		if (CL(SINGLE_DATA_DECL, self))
		{//<single_data_decl>
			while (1)
			{
				if (!GETCP(CODE_COMMA))
					break;

				list->Pop(&kv);// ','
				self->InsR(&kv);

				if (!CL(SINGLE_DATA_DECL, self))
					Error("Expected a data declaration after comma");
				//<single_data_decl>
			}
		}
		else if (CL(IDENTIFIER, self))
		{//<identifier>
			if (!GETCP(CODE_LBRACE))
				Error("Expected a '[' after identifier");
			list->Pop(&kv);// '['
			self->InsR(&kv);

			if (!CL(CONSTANT_EXPRESSION, self)) //<constant_expression>
				Error("Expected an expression after '['");

			if (!GETCP(CODE_RBRACE))
				Error("Expected a ']' after expression");
			list->Pop(&kv);// ']'
			self->InsR(&kv);

			if (GETCP(CODE_EQUALS))
			{//'='
				list->Pop(&kv);
				self->InsR(&kv);

				if (!GETCP(CODE_LBRACKET))
					Error("Expected a '{' after '='");

				list->Pop(&kv);//'{'
				self->InsR(&kv);

				if (!CL(INITIALIZER_LIST, self))//<initializer_list>
					Error("Expected an initializer in array declaration");

				if (!GETCP(CODE_RBRACKET))
					Error("Expeceted a '}' after initializer list");

				list->Pop(&kv);//'}'
				self->InsR(&kv);
			}
		}
		else
			Error("Expected an identifier after data type\n");

		if (!GETCP(CODE_SEMICOLON))
			Error("Expected a ';' after data decl");

		list->Pop(&kv);
		self->InsR(&kv);

		return true;
	}

	parent->KillChild(self);
	list->Restore(saved);
#endif
	return  false;
}

GF_DEF(STRUCT_DECL)
{//<storage_specifier>? 'struct' <identifier> '*'? <identifier>  { '[' <const_expression> ']' }? ';' |
	kv_c kv;
	node_c* saved = list->Save();
	tree_c* self = parent->InsR("Struct decl", NT_STRUCT_DECL);

	CL(STORAGE_SPECIFIER, self); //<storage_specifier>?

	if (GETCP(CODE_STRUCT))
	{//'struct'
		list->Pop(&kv);
		self->InsR(&kv);

		if (CL(IDENTIFIER, self))
		{//<identifier>
			if (GETCP(CODE_STAR))
			{//'*'
				list->Pop(&kv);
				self->InsR(&kv);
			}

			if (!CL(IDENTIFIER, self)) //<identifier>
				Error("Expected identifier in struct instance declaration\n");

			if (GETCP(CODE_LBRACE))
			{//'['
				list->Pop(&kv);
				self->InsR(&kv);

				if (!CL(CONSTANT_EXPRESSION, self)) //<const_expression>
					Error("Expected an expression in struct array\n");

				if (!GETCP(CODE_RBRACE)) //']'
					Error("Unclosed brace in structure array declaration\n");

				list->Pop(&kv);
				self->InsR(&kv);
			}
		}
		else
		{//this could still be a type declaration
			parent->KillChild(self);
			list->Restore(saved);
			return false;
		}

		if (!GETCP(CODE_SEMICOLON))
			Error("Missing ';' after data declaration");
		list->Pop(&kv);
		self->InsR(&kv);

		return true;
	}

	parent->KillChild(self);
	list->Restore(saved);
	return false;
}

GF_DEF(SINGLE_DATA_DECL)
{//<identifier> { '=' <arithmetic_expression> }+
	node_c* saved = list->Save();
	node_c* saved_equals;
	tree_c* self = parent->InsR("Single data decl", NT_SINGLE_DATA_DECL);
	tree_c* equals_sign = NULL;
	kv_c kv;

	if (CL(IDENTIFIER, self))
	{//<identifier>

		if (GETCP(CODE_LBRACE))
		{//'[' - look ahead so array decls don't return true here

			list->Restore(saved);
			parent->KillChild(self);
			return false;
		}

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
{// 'flags' '{' <data_decl>+ '}' <identifier> ';'
	kv_c kv;
	node_c* saved = list->Save();
	tree_c* self = parent->InsR("Type def", NT_TYPE_DEF);

	if (GETCP(CODE_STRUCT))
	{// 'struct'
		list->Pop(&kv);
		self->InsR(&kv);

		if (GETCP(CODE_LBRACKET))
		{// '{'
			list->Pop(&kv);
			self->InsR(&kv);

			if (CL(DATA_DECL, self))
			{// <data_decl>
				while (CL(DATA_DECL, self)) {} // <data_decl>*

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