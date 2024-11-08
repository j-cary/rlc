#include "parse.h"

GF_DEF(INSTRUCTION)
{
	tnode_c* self = parent->InsR("Instruction", NT_INSTRUCTION);
	node_c* saved = list->Save();
	kv_c kv;
	gfunc_t func = NULL;

	switch (list->Get()->V())
	{
	case CODE_JP:
	case CODE_INC:
	case CODE_DEC:
	case CODE_IM:
	case CODE_NEG:	func = &parse_c::OPERANDS_ONE;			break;

	case CODE_RES:
	case CODE_SET:
	case CODE_FLP:
	case CODE_IN:
	case CODE_OUT:	func = &parse_c::OPERANDS_TWO;			break;

	case CODE_LDM:
	case CODE_OUTM:
	case CODE_INM:	func = &parse_c::OPERANDS_THREE;		break;

	case CODE_RR:
	case CODE_RL:	func = &parse_c::OPERANDS_ONE_TO_TWO;	break;

	case CODE_SL:
	case CODE_SR:	func = &parse_c::OPERANDS_ONE_TO_THREE;	break;

	case CODE_LD:
	case CODE_ADD:
	case CODE_SUB:
	case CODE_MUL:
	case CODE_DIV:
	case CODE_MOD:
	case CODE_AND:
	case CODE_OR:
	case CODE_XOR:	func = &parse_c::OPERANDS_TWO_TO_INF;	break;

	case CODE_COMP:	func = &parse_c::OPERANDS_COMP;			break;

	case CODE_CPM:	func = &parse_c::OPERANDS_CPM;			break;

	case CODE_RET:	func = &parse_c::OPERANDS_RET;			break;

	case CODE_CALL:	func = &parse_c::OPERANDS_CALL;			break;

	default:	parent->KillChild(self);	return false;	break;
	}


	list->Pop(&kv);
	self->InsR(&kv);

	if(Call(func, self))
	{// <operand(s)>
		if (GETCP(CODE_SEMICOLON))
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
	tnode_c* self = parent->InsR("1 op", NT_OPERANDS_ONE);

	if (CL(MEMORY_EXPRESSION, self))
	{// <memory_expression>
		return true;
	}

	parent->KillChild(self);
	return false;
}

GF_DEF(OPERANDS_TWO)
{//<memory_expression> ',' <mem_or_const_expression>
	tnode_c* self = parent->InsR("2 ops", NT_OPERANDS_TWO);
	node_c* saved = list->Save();
	kv_c kv;

	if (CL(MEMORY_EXPRESSION, self))
	{// <memory_expression>
		if (GETCP(CODE_COMMA))
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
	tnode_c* self = parent->InsR("3 ops", NT_OPERANDS_THREE);
	node_c* saved = list->Save();
	kv_c kv;

	if (CL(MEMORY_EXPRESSION, self))
	{// <memory_expression>
		if (GETCP(CODE_COMMA))
		{// ','
			list->Pop(&kv);
			self->InsR(&kv);

			if (CL(MEMORY_EXPRESSION, self))
			{// <memory_expression>
				if (GETCP(CODE_COMMA))
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
	tnode_c* self = parent->InsR("1 or 2 ops", NT_OPERANDS_ONE_TO_TWO);
	node_c* saved = list->Save();
	kv_c kv;

	if (CL(MEMORY_EXPRESSION, self))
	{// <memory_expression>
		if (GETCP(CODE_COMMA))
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
	tnode_c* self = parent->InsR("1 to 3 ops", NT_OPERANDS_ONE_TO_THREE);
	node_c* saved = list->Save();
	kv_c kv;

	if (CL(MEMORY_EXPRESSION, self))
	{// <memory_expression>
		if (GETCP(CODE_COMMA))
		{// ','
			list->Pop(&kv);
			self->InsR(&kv);

			if (CL(CONSTANT, self)) //Semantically, 0 or 1.
			{// <constant>
				if (GETCP(CODE_COMMA))
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
	tnode_c* self = parent->InsR("2 to inf ops", NT_OPERANDS_TWO_TO_INF);
	tnode_c* comma_child;
	node_c* saved = list->Save();
	node_c* comma_saved;

	kv_c kv;

	//FIXME: LD needs its own version of this function! Only the rightmost operand should be a const, but whatever...

	if (CL(MEMORY_EXPRESSION, self))
	{// <memory_expression>
		if (GETCP(CODE_COMMA))
		{// ','
			list->Pop(&kv);
			self->InsR(&kv);

			if (CL(MEM_OR_CONST_EXPRESSION, self))
			{// <mem_or_const_expression>

				while (1)
				{
					if (!GETCP(CODE_COMMA))
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
	tnode_c* self = parent->InsR("2 ops", NT_OPERANDS_COMP);
	node_c* saved = list->Save();
	kv_c kv;

	if (CL(MEMORY_EXPRESSION, self))
	{// <memory_expression>
		if (GETCP(CODE_COMMA))
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
	tnode_c* self = parent->InsR("4 ops", NT_OPERANDS_FOUR);
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
		if (!GETCP(CODE_COMMA))
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

	tnode_c* self = parent->InsR("1 op", NT_OPERANDS_RET);

	if (!CL(LOGICAL_EXPRESSION, self))
		parent->KillChild(self);

	return true;
}

GF_DEF(OPERANDS_CALL)
{//<memory_expression>  { ',' <mem_or_const_expression> }*
	tnode_c* self = parent->InsR("1 to inf ops", NT_OPERANDS_CALL);
	tnode_c* comma_child;
	node_c* saved = list->Save();
	node_c* comma_saved;
	kv_c kv;


	if (CL(MEMORY_EXPRESSION, self))
	{// <memory_expression>

		while (1)
		{
			if (!GETCP(CODE_COMMA))
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
