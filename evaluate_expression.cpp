#include "evaluate_expression.h"

/***************************************************************************************************
											Constant
***************************************************************************************************/

int eval_expr_c::ConstantHelper(const tree_c* head, int num_kids, int offset, const_op_e type) const
{
	int	r1, r2, ret = 0;
	int	op_code;

	r1 = Constant(head->Get(offset));
	op_code = head->Get(offset + 1)->Hash()->V();
	r2 = Constant(head->Get(offset + 2));

	if (type == const_op_e::OP_SHIFT)
	{
		switch (op_code)
		{
		case T_LEFT_SHIFT: ret = r1 << r2; break;
		case T_RIGHT_SHIFT: ret = r1 >> r2; break;
		default: Error("Bad shift operator"); break;
		}
	}
	else if (type == const_op_e::OP_ADD)
	{
		switch (op_code)
		{
		case CODE_PLUS: ret = r1 + r2; break;
		case CODE_MINUS: ret = r1 - r2; break;
		default: Error("Bad additive operator"); break;
		}
	}
	else if (type == const_op_e::OP_MULT)
	{
		switch (op_code)
		{
		case CODE_STAR: ret = r1 * r2; break;
		case CODE_FSLASH: ret = r1 / r2; break;
		case CODE_PERCENT: ret = r1 % r2; break;
		default: Error("Bad multiplicative operator"); break;
		}
	}



	for (int i = offset + 3; i < num_kids; i += 2)
	{
		op_code = head->Get(i)->Hash()->V();
		r1 = Constant(head->Get(i + 1));

		if (type == const_op_e::OP_SHIFT)
		{
			switch (op_code)
			{
			case T_LEFT_SHIFT: ret <<= r1; break;
			case T_RIGHT_SHIFT: ret >>= r1; break;
			default: Error("Bad shift operator"); break;
			}
		}
		else if (type == const_op_e::OP_ADD)
		{
			switch (op_code)
			{
			case CODE_PLUS: ret += r1; break;
			case CODE_MINUS: ret -= r1; break;
			default: Error("Bad additive operator"); break;
			}
		}
		else if (type == const_op_e::OP_MULT)
		{
			switch (op_code)
			{
			case CODE_STAR: ret *= r1; break;
			case CODE_FSLASH: ret /= r1; break;
			case CODE_PERCENT: ret %= r1; break;
			default: Error("Bad multiplicative operator"); break;
			}
		}
	}

	return ret;
}

int eval_expr_c::Constant(const tree_c* head) const
{
	int		kids;
	int		ret = 0;
	int		op_code;

	for (kids = 0; head->Get(kids); kids++);

	switch (head->Hash()->V())
	{
	case CODE_NUM_DEC:				ret = atoi(head->Hash()->K()); break;
	case CODE_NUM_BIN:				ret = strtol(&head->Hash()->K()[1], NULL, 2); break;
	case CODE_NUM_HEX:				ret = strtol(&head->Hash()->K()[1], NULL, 16); break;
	case NT_SHIFT_EXPR:				ret = ConstantHelper(head, kids, 0, const_op_e::OP_SHIFT); break;
	case NT_ADDITIVE_EXPR:			ret = ConstantHelper(head, kids, 0, const_op_e::OP_ADD); break;
	case NT_MULTIPLICATIVE_EXPR:	ret = ConstantHelper(head, kids, 0, const_op_e::OP_MULT); break;
	case NT_ARITHMETIC_PRIMARY_EXPR:

		ret = Constant(head->Get(1));//skip over the (, *, &, etc.
		op_code = head->Get(0)->Hash()->V();

		if (op_code == CODE_MINUS)
			ret = -ret;
		else if (op_code == CODE_STAR || op_code == CODE_AMPERSAND)
			Error("referencing/de-referencing is not allowed in a constant expression");

		break;

	case NT_ARITHMETIC_POSTFIX_EXPR:
		Error("Postfix operators are not allowed in constant expressions");
	case CODE_TEXT:
		Error("Text value found in const. expr");
	default:
		INTERNAL_ASSERT(0, "Bad const expression");
	}

	return ret;
}

/***************************************************************************************************
											Memory
***************************************************************************************************/

int eval_expr_c::MemoryOffset(const tree_c* head, tree_c** data)
{
	int code;
	int offset = 0;
	int	kids;
	const tree_c* expr = head;
	bool is_struct;

	/*
	* Thoughts on what this needs to return:
	*	What struct (or array) is being accessed (for semantics, reg alloc, CG)
	*	The offset from the base (for CG)
	*	Ref/de-ref levels (for CG)
	*/

	/*
	* Checks:
	*	Valid struct member access
	*	Arrays?
	*/

	//Count the operands
	for (kids = 0; expr->Get(kids); kids++);

	code = expr->Hash()->V();

	if (NT_MEMORY_PRIMARY_EXPR == code)
	{
		const int unary_op = head->Get(0)->Hash()->V();
		mem_op_e next_type;

		switch (unary_op)
		{
		case CODE_STAR: next_type = mem_op_e::OP_DEREF; break;
		case CODE_AMPERSAND: next_type = mem_op_e::OP_REF;
		default: next_type = mem_op_e::OP_NONE;
		}

		// Parse any leading refs/derefs
		return MemoryPrimary(head->Get(1), data, next_type);
	}
	else if (NT_MEMORY_EXPR == code)
	{
		*data = expr->Get(0);
	}
	else if (CODE_TEXT == code)
	{
		*data = (tree_c*)expr; // TODO: re-evaluate const status of stuff program-wide
	}
	else
	{
		INTERNAL_ASSERT(0, "Invalid memory expression");
	}

	//Note: Struct member validity checking happens twice - in semantics and in CG. Oh well
	//TODO: This'll become useful once it returns the actual struct entry
	is_struct = block->IsStructInstance((*data)->Hash()->K(), func, root);

	cfg_c* local_block = NULL; //FIXME: remove casts here
	data_t* data_entry = block->ScopedDataEntry((*data)->Hash()->K(), (cfg_c*)func, (cfg_c*)root, &local_block);

	//Base data types should be allowed array offsetting (for untyped pointers stored in a dw)
	//Structs should be allowed member offsets; base data types can do the above as well

	/*
	* Data plan:
	*	Semantics SHOULD check semantic validity (i.e. structs can't reference non-existant
	*	members)
	*	Need struct offset info - CG - Can be passed into generator then here
	*	Need type info - semantics/CG - can be retrieved with mods to isstructinst
	*/

	// Calculate the offset - this only runs once per Memory call
	// Just a flat list at this point; skip over the ident and check out the members/offsets
	for (int i = 1; i < kids; ++i)
	{
		const tree_c* const op = expr->Get(i);

		switch (op->Hash()->V())
		{
		case CODE_PERIOD: //Member access

			ASSERT(data_entry->flags & DF_STRUCT,
				"Member access operator used on non-struct object");

			ASSERT_FALSE(data_entry->flags & DF_PTR,
				"Direct member access operator used instead of indirect operator");

			//Does this member exist?

			//Get the member, set it to the current 'working' struct/data

			break;

		case T_DEREF_MEMBER: //Indirect member access
			ASSERT(data_entry->flags & DF_STRUCT,
				"Member access operator used on non-struct object");

			ASSERT(data_entry->flags & DF_PTR,
				"Direct member access operator used instead of indirect operator");

			//Does this member exist?

			//Get the member, set it to the current 'working' struct/data

			break;

		case CODE_LBRACE: //Array indexing
			ASSERT(data_entry->flags & (DF_PTR | DF_ARRAY | DF_WORD),
				"Braces used on non-indexable object");

			// Get the offset, swap types I think...

			break;
		default: INTERNAL_ASSERT(0, "Invalid memory expression"); break;
		}
	}


	return offset;
}

int eval_expr_c::MemoryPrimary(const tree_c* head, tree_c** data, mem_op_e type)
{
	const tree_c* const next_expr = head->Get(1);
	mem_op_e next_type;

	// Adjust the number of references/de-references
	switch (type)
	{
	case mem_op_e::OP_REF: ++ref_level; break;
	case mem_op_e::OP_DEREF: --ref_level; break;
	}

	if (NT_MEMORY_EXPR == head->Hash()->V())
	{ // Found the first non ref token. This'll be the struct/array instance
		return MemoryOffset(head, data);
	}

	switch (head->Get(0)->Hash()->V())
	{
	case CODE_STAR: next_type = mem_op_e::OP_DEREF; break;
	case CODE_AMPERSAND: next_type = mem_op_e::OP_REF;
	default: next_type = mem_op_e::OP_NONE;
	}

	return MemoryPrimary(next_expr, data, next_type);
}

/* Returns the constant offset from the object. Name is the var name of the struct */
int eval_expr_c::Memory(const tree_c* head, const cfg_c* _block, const cfg_c* _func, const cfg_c* _root, tree_c** data)
{
	ref_level = 0; // No de-referencing yet
	block = _block;
	func = _func;
	root = _root;

	int ret = MemoryOffset(head, data);
	return ret;
}