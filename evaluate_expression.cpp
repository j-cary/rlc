#include "evaluate_expression.h"

/*
* TODO: Implement the whole 'don't error on invalid expr' thing
* Will need a wrapper for constant first of all
*/

/***************************************************************************************************
										Defines/Typedefs
***************************************************************************************************/

#define FAIL(cond, internal_flag, error_string) \
	do { \
		if(!(cond)) \
		{ \
			if(!error_on_invalid) \
				throw 0; \
			else if (internal_flag) \
				INTERNAL_ASSERT(0, error_string); \
			else \
				ASSERT(0, error_string); \
		} \
	} while (0)


/***************************************************************************************************
											Constant
***************************************************************************************************/

int eval_expr_c::ConstantHelper(const tree_c* head, int num_kids, int offset, const_op_e type) const
{
	int	r1, r2, ret = 0;
	CODE	op_code;

	r1 = Constant(head->Get(offset));
	op_code = head->Get(offset + 1)->Hash()->Code();
	r2 = Constant(head->Get(offset + 2));
	
	if (type == const_op_e::OP_SHIFT)
	{
		switch (op_code)
		{
		case CODE::T_LEFT_SHIFT: ret = r1 << r2; break;
		case CODE::T_RIGHT_SHIFT: ret = r1 >> r2; break;
		//default: Error("Bad shift operator"); break;
		default: FAIL(0, false, "Bad shift operator"); break;
		}
	}
	else if (type == const_op_e::OP_ADD)
	{
		switch (op_code)
		{
		case CODE::PLUS: ret = r1 + r2; break;
		case CODE::MINUS: ret = r1 - r2; break;
		//default: Error("Bad additive operator"); break;
		default: FAIL(0, false, "Bad additive operator"); break;
		}
	}
	else if (type == const_op_e::OP_MULT)
	{
		switch (op_code)
		{
		case CODE::STAR: ret = r1 * r2; break;
		case CODE::FSLASH: ret = r1 / r2; break;
		case CODE::PERCENT: ret = r1 % r2; break;
		//default: Error("Bad multiplicative operator"); break;
		default: FAIL(0, false, "Bad multiplicative operator"); break;
		}
	}



	for (int i = offset + 3; i < num_kids; i += 2)
	{
		op_code = head->Get(i)->Hash()->Code();
		r1 = Constant(head->Get(i + 1));

		if (type == const_op_e::OP_SHIFT)
		{
			switch (op_code)
			{
			case CODE::T_LEFT_SHIFT: ret <<= r1; break;
			case CODE::T_RIGHT_SHIFT: ret >>= r1; break;
			//default: Error("Bad shift operator"); break;
			default: FAIL(0, false, "Bad shift operator"); break;
			}
		}
		else if (type == const_op_e::OP_ADD)
		{
			switch (op_code)
			{
			case CODE::PLUS: ret += r1; break;
			case CODE::MINUS: ret -= r1; break;
			//default: Error("Bad additive operator"); break;
			default: FAIL(0, false, "Bad additive operator"); break;
			}
		}
		else if (type == const_op_e::OP_MULT)
		{
			switch (op_code)
			{
			case CODE::STAR: ret *= r1; break;
			case CODE::FSLASH: ret /= r1; break;
			case CODE::PERCENT: ret %= r1; break;
			//default: Error("Bad multiplicative operator"); break;
			default: FAIL(0, false, "Bad multiplicative operator"); break;
			}
		}
	}

	return ret;
}

int eval_expr_c::Constant(const tree_c* head) const
{
	int		kids;
	int		ret = 0;
	CODE		op_code;

	for (kids = 0; head->Get(kids); kids++);

	switch (head->Hash()->Code())
	{
	case CODE::NUM_DEC:				ret = atoi(head->Hash()->Str()); break;
	case CODE::NUM_BIN:				ret = strtol(&head->Hash()->Str()[1], NULL, 2); break;
	case CODE::NUM_HEX:				ret = strtol(&head->Hash()->Str()[1], NULL, 16); break;
	case CODE::NT_SHIFT_EXPR:		ret = ConstantHelper(head, kids, 0, const_op_e::OP_SHIFT); break;
	case CODE::NT_ADDITIVE_EXPR:	ret = ConstantHelper(head, kids, 0, const_op_e::OP_ADD); break;
	case CODE::NT_MULTIPLICATIVE_EXPR:ret = ConstantHelper(head, kids, 0, const_op_e::OP_MULT); break;
	case CODE::NT_ARITHMETIC_PRIMARY_EXPR:

		ret = Constant(head->Get(1));//skip over the (, *, &, etc.
		op_code = head->Get(0)->Hash()->Code();

		FAIL(op_code != CODE::STAR && op_code != CODE::AMPERSAND, false, 
			"referencing/de-referencing is not allowed in a constant expression");

		if (op_code == CODE::MINUS)
			ret = -ret;
		//else if (op_code == CODE::STAR || op_code == CODE::AMPERSAND)
		//	Error("referencing/de-referencing is not allowed in a constant expression");

		break;

	case CODE::NT_ARITHMETIC_POSTFIX_EXPR:
		//Error("Postfix operators are not allowed in constant expressions");
		FAIL(0, false, "Postfix operators are not allowed in constant expressions");
		break;
	case CODE::TEXT:
		//Error("Text value found in const. expr");
		FAIL(0, false, "Text value found in const. expr");
		break;
	default:
		//INTERNAL_ASSERT(0, "Bad const expression");
		FAIL(0, true, "Bad const expression");
	}

	return ret;
}

/***************************************************************************************************
											Memory
***************************************************************************************************/

int eval_expr_c::MemoryOffset(const tree_c* head, tree_c** data)
{
	CODE code;
	int offset = 0;
	int	kids;
	const tree_c* expr = head;
	const struct_t* strct = NULL;

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
	for (kids = 0; expr->Get(kids); ++kids);

	code = expr->Hash()->Code();

	if (CODE::NT_MEMORY_PRIMARY_EXPR == code)
	{
		const CODE unary_op = head->Get(0)->Hash()->Code();
		mem_op_e next_type;

		switch (unary_op)
		{
		case CODE::STAR: next_type = mem_op_e::OP_DEREF; break;
		case CODE::AMPERSAND: next_type = mem_op_e::OP_REF; break;
		default: next_type = mem_op_e::OP_NONE;
		}

		// Parse any leading refs/derefs
		return MemoryPrimary(head->Get(1), data, next_type);
	}
	else if (CODE::NT_MEMORY_EXPR == code)
	{
		*data = expr->Get(0);
	}
	else if (CODE::TEXT == code)
	{
		*data = (tree_c*)expr; // TODO: re-evaluate const status of stuff program-wide
	}
	else
	{
		INTERNAL_ASSERT(0, "Invalid memory expression");
	}

	cfg_c* local_block = NULL; //FIXME: remove casts here
	data_t* data_entry = block->ScopedDataEntry((*data)->Hash()->Str(), (cfg_c*)func, (cfg_c*)root, &local_block);

	ASSERT(data_entry, "Undeclared variable %s", (*data)->Str());

	dataflags_t flags = data_entry->flags;
	strct = slist->StructInfo(data_entry->struct_name);

	//Base data types should be allowed array offsetting (for untyped pointers stored in a dw)
	//Structs should be allowed member offsets; base data types can do the above as well

	// Calculate the offset - this only runs once per Memory call
	// Just a flat list at this point; skip over the ident and check out the members/offsets
	for (int i = 1; i < kids; ++i)
	{
		const tree_c* const op = expr->Get(i);

		switch (op->Hash()->Code())
		{
		case CODE::PERIOD: //Member access
		{
			ASSERT(strct,
				"Member access operator used on non-struct object");

			ASSERT_FALSE(flags & DF_PTR,
				"Direct member access operator used instead of indirect operator");

			const member_t* const member = strct->GetMemberInfo(expr->Get(++i)->Str());
			
			ASSERT(member, "%s is not a member of %s", expr->Get(i), strct->name);

			offset += member->offset; // Update the offset
			// Rethink what to do based on type-information
			strct = slist->StructInfo(member->struct_name);
			flags = member->flags;

			break;
		}
		case CODE::T_DEREF_MEMBER: //Indirect member access
		{

			ASSERT(strct,
				"Member access operator used on non-struct object");

			ASSERT(flags & DF_PTR,
				"Direct member access operator used instead of indirect operator");

			const member_t* const member = strct->GetMemberInfo(expr->Get(++i)->Str());

			ASSERT(member, "%s is not a member of %s", expr->Get(i), strct->name);

			offset += member->offset; // Update the offset
			// Rethink what to do based on type-information
			strct = slist->StructInfo(member->struct_name);
			flags = member->flags;

			break;
		}
		case CODE::LBRACE: //Array indexing
		{
			ASSERT(flags & (DF_PTR | DF_ARRAY | DF_WORD | DF_BYTE),
				"Braces used on non-indexable object");

			const tree_c* const index = expr->Get(++i);
			const eval_expr_c eval_expr;

			offset += eval_expr.Constant(index); // Update the offset

			++i; // Skip past the closing bracket
			break;
		}
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
	case mem_op_e::OP_REF: ++mem_result.ref_level; break;
	case mem_op_e::OP_DEREF: --mem_result.ref_level; break;
	}

	if (CODE::NT_MEMORY_EXPR == head->Code() || CODE::TEXT == head->Code())
	{ // Found the first non ref token. This'll be the struct/array instance
		return MemoryOffset(head, data);
	}

	switch (head->Get(0)->Hash()->Code())
	{
	case CODE::STAR: next_type = mem_op_e::OP_DEREF; break;
	case CODE::AMPERSAND: next_type = mem_op_e::OP_REF; break;
	default: next_type = mem_op_e::OP_NONE;
	}

	return MemoryPrimary(next_expr, data, next_type);
}

/* Returns the constant offset from the object. Name is the var name of the struct */
eval_expr_c::mem_result_t eval_expr_c::Memory(const tree_c* head, const cfg_c* _block, const cfg_c* _func, const cfg_c* _root,
	const structlist_c* sl, tree_c** data)
{
	mem_result.offset = 0;
	mem_result.ref_level = 0;

	block = _block;
	func = _func;
	root = _root;
	slist = sl;

	mem_result.offset = MemoryOffset(head, data);
	return mem_result;

	//int ret = MemoryOffset(head, data);
	//return ret;
}