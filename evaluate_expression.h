/***************************************************************************************************
Purpose: Provide expression evaluation for analysis and code generation
***************************************************************************************************/
#pragma once
#include "semantics.h"

class eval_expr_c
{
private:
	int ref_level; // > 0 for references, < 0 for dereferences
	const cfg_c* block, * func, * root;

	enum class mem_op_e
	{
		OP_NONE, OP_REF, OP_DEREF
	};

	enum class const_op_e
	{
		OP_SHIFT, OP_MULT, OP_ADD
	};

	/* Handle the main operators (*, +, <<, etc. */
	int ConstantHelper(const tree_c* head, int num_kids, int offset, const_op_e type) const;

	/* Calculate the offset from the struct/array */
	int MemoryOffset(const tree_c* head, tree_c** data);

	/* Plucks leading '*'s and '&'s off the expr. Referencing can only happen at the beginning. */
	int MemoryPrimary(const tree_c* head, tree_c** data, mem_op_e type);

public:
	/* Evaluates a const expr. Asserts if invalid */
	int Constant(const tree_c* head) const;

	/* Returns the constant offset from the object. Name is the var name of the struct */
	int Memory(const tree_c* head, const cfg_c* _block, const cfg_c* _func, const cfg_c* root, tree_c** data);
};