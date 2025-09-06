#include "parse.h"

GF_DEF(INSTRUCTION)
{
	tree_c* self = parent->InsR("Instruction", CODE::NT_INSTRUCTION);
	node_c* saved = list->Save();
	kv_c kv;
	gfunc_t func = NULL;

	switch (list->Get()->Code())
	{
	case CODE::JP:
	case CODE::INC:
	case CODE::DEC:
	case CODE::IM:
	case CODE::NEG:	func = &parser_c::OPERANDS_ONE;			break;

	case CODE::RES:
	case CODE::SET:
	case CODE::FLP:
	case CODE::INN:
	case CODE::OT:	func = &parser_c::OPERANDS_TWO;			break;

	case CODE::LDM:
	case CODE::OUTM:
	case CODE::INM:	func = &parser_c::OPERANDS_THREE;		break;

	case CODE::RR:
	case CODE::RL:	func = &parser_c::OPERANDS_ONE_TO_TWO;	break;

	case CODE::SL:
	case CODE::SR:	func = &parser_c::OPERANDS_ONE_TO_THREE;	break;

	case CODE::LD:
	case CODE::ADD:
	case CODE::SUB:
	case CODE::MUL:
	case CODE::DIV:
	case CODE::MOD:
	case CODE::AND:
	case CODE::OR:
	case CODE::XOR:	func = &parser_c::OPERANDS_TWO_TO_INF;	break;

	case CODE::COMP:	func = &parser_c::OPERANDS_COMP;			break;

	case CODE::CPM:	func = &parser_c::OPERANDS_CPM;			break;

	case CODE::RET:	func = &parser_c::OPERANDS_RET;			break;

	case CODE::CALL:	func = &parser_c::OPERANDS_CALL;			break;

	default:	parent->KillChild(self);	return false;	break;
	}


	list->Pop(&kv);
	self->InsR(&kv);

	if(Call(func, self))
	{// <operand(s)>
		if (GETCP(CODE::SEMICOLON))
		{// ';'
			list->Pop(&kv);
			self->InsR(&kv);
			return true;
		}
	}

	
	list->Restore(saved);
	parent->KillChild(self);
	return false;
}


GF_DEF(OPERANDS_ONE)
{// <memory_expression>
	tree_c* self = parent->InsR("1 op", CODE::NT_OPERANDS_ONE);

	if (CL(MEMORY_EXPRESSION, self))
	{// <memory_expression>
		return true;
	}

	parent->KillChild(self);
	return false;
}

GF_DEF(OPERANDS_TWO)
{//<memory_expression> ',' <mem_or_const_expression>
	tree_c* self = parent->InsR("2 ops", CODE::NT_OPERANDS_TWO);
	node_c* saved = list->Save();
	kv_c kv;

	if (CL(MEMORY_EXPRESSION, self))
	{// <memory_expression>
		if (GETCP(CODE::COMMA))
		{// ','
			list->Pop(&kv);
			self->InsR(&kv);

			if (CL(MEM_OR_CONST_EXPRESSION, self))
				return true;
		}
	}

	list->Restore(saved);
	parent->KillChild(self);
	return false;
}

GF_DEF(OPERANDS_THREE)
{//<memory_expression> ',' <memory_expression> ',' <mem_or_const_expression>
	tree_c* self = parent->InsR("3 ops", CODE::NT_OPERANDS_THREE);
	node_c* saved = list->Save();
	kv_c kv;

	if (CL(MEMORY_EXPRESSION, self))
	{// <memory_expression>
		if (GETCP(CODE::COMMA))
		{// ','
			list->Pop(&kv);
			self->InsR(&kv);

			if (CL(MEMORY_EXPRESSION, self))
			{// <memory_expression>
				if (GETCP(CODE::COMMA))
				{// ','
					list->Pop(&kv);
					self->InsR(&kv);

					if (CL(MEM_OR_CONST_EXPRESSION, self))
						return true;
				}
			}
		}
	}

	list->Restore(saved);
	parent->KillChild(self);
	return false;
}

GF_DEF(OPERANDS_ONE_TO_TWO)
{//<memory_expression> { ',' <mem_or_const_expression> }?
	tree_c* self = parent->InsR("1 or 2 ops", CODE::NT_OPERANDS_ONE_TO_TWO);
	node_c* saved = list->Save();
	kv_c kv;

	if (CL(MEMORY_EXPRESSION, self))
	{// <memory_expression>
		if (GETCP(CODE::COMMA))
		{// ','
			list->Pop(&kv);
			self->InsR(&kv);

			if (CL(MEM_OR_CONST_EXPRESSION, self))
				return true;
		}
		else
			return true;
		
	}

	list->Restore(saved);
	parent->KillChild(self);
	return false;
}

GF_DEF(OPERANDS_ONE_TO_THREE)
{// <memory_expression>  { ',' <constant>  { ',' <mem_or_const_expression> }? }?
	tree_c* self = parent->InsR("1 to 3 ops", CODE::NT_OPERANDS_ONE_TO_THREE);
	node_c* saved = list->Save();
	kv_c kv;

	if (CL(MEMORY_EXPRESSION, self))
	{// <memory_expression>
		if (GETCP(CODE::COMMA))
		{// ','
			list->Pop(&kv);
			self->InsR(&kv);

			if (CL(CONSTANT, self)) //Semantically, 0 or 1.
			{// <constant>
				if (GETCP(CODE::COMMA))
				{// ','
					list->Pop(&kv);
					self->InsR(&kv);

					if (CL(MEM_OR_CONST_EXPRESSION, self))
						return true; //all 3 ops
				}
				else //no third operand
					return true;
			}
		}
		else //no second operand
			return true;

	}

	list->Restore(saved);
	parent->KillChild(self);
	return false;
}

GF_DEF(OPERANDS_TWO_TO_INF)
{//<memory_expression> { ',' <mem_or_const_expression> }+
	tree_c* self = parent->InsR("2 to inf ops", CODE::NT_OPERANDS_TWO_TO_INF);
	tree_c* comma_child;
	node_c* saved = list->Save();
	node_c* comma_saved;

	kv_c kv;

	//FIXME: LD needs its own version of this function! Only the rightmost operand should be a const, but whatever...

	if (CL(MEMORY_EXPRESSION, self))
	{// <memory_expression>
		if (GETCP(CODE::COMMA))
		{// ','
			list->Pop(&kv);
			self->InsR(&kv);

			if (CL(MEM_OR_CONST_EXPRESSION, self))
			{// <mem_or_const_expression>

				while (1)
				{
					if (!GETCP(CODE::COMMA))
						break;
					comma_saved = list->Save();
					list->Pop(&kv);
					comma_child = self->InsR(&kv);

					if (!CL(MEM_OR_CONST_EXPRESSION, self))
					{
						list->Restore(comma_saved);
						self->KillChild(comma_child);
						break;
					}
					//<mem_or_const_expression>
				}

				return true;
			}
		}
	}

	list->Restore(saved);
	parent->KillChild(self);
	return false;

	return false;
}

GF_DEF(OPERANDS_COMP)
{//<memory_expression> ',' <arithmetic_expression>
	tree_c* self = parent->InsR("2 ops", CODE::NT_OPERANDS_COMP);
	node_c* saved = list->Save();
	kv_c kv;

	if (CL(MEMORY_EXPRESSION, self))
	{// <memory_expression>
		if (GETCP(CODE::COMMA))
		{// ','
			list->Pop(&kv);
			self->InsR(&kv);

			if (CL(ARITHMETIC_EXPRESSION, self))
				return true;
		}
	}

	list->Restore(saved);
	parent->KillChild(self);
	return false;
}

GF_DEF(OPERANDS_CPM)
{//<memory_expression> ',' <memory_expression> ',' <memory_expression> ',' <mem_or_const_expression>
	tree_c* self = parent->InsR("4 ops", CODE::NT_OPERANDS_FOUR);
	node_c* saved = list->Save();
	bool success = true;
	kv_c kv;

	for (int i = 0; i < 3; i++)
	{
		if (!CL(MEMORY_EXPRESSION, self))
		{
			success = false;
			break;
		}

		//<memory_expression>
		if (!GETCP(CODE::COMMA))
		{
			success = false;
			break;
		}

		// ','
		list->Pop(&kv);
		self->InsR(&kv);
	}

	
	if(success && CL(MEM_OR_CONST_EXPRESSION, self))
		return true;
	

	list->Restore(saved);
	parent->KillChild(self);
	return false;
}

GF_DEF(OPERANDS_RET)
{// { <logical_expression> }+

	tree_c* self = parent->InsR("1 op", CODE::NT_OPERANDS_RET);

	if (!CL(LOGICAL_EXPRESSION, self))
		parent->KillChild(self);

	return true;
}

GF_DEF(OPERANDS_CALL)
{//<memory_expression>  { ',' <mem_or_const_expression> }*
	tree_c* self = parent->InsR("1 to inf ops", CODE::NT_OPERANDS_CALL);
	tree_c* comma_child;
	node_c* saved = list->Save();
	node_c* comma_saved;
	kv_c kv;


	if (CL(MEMORY_EXPRESSION, self))
	{// <memory_expression>

		while (1)
		{
			if (!GETCP(CODE::COMMA))
				break;
			comma_saved = list->Save();
			list->Pop(&kv); //','
			comma_child = self->InsR(&kv);

			if (!CL(MEM_OR_CONST_EXPRESSION, self))
			{
				list->Restore(comma_saved);
				self->KillChild(comma_child);
				break;
			}
			//<mem_or_const_expression>
		}

		return true;
	}

	list->Restore(saved);
	parent->KillChild(self);
	return false;
}
