/***************************************************************************************************
Purpose: Provide expression evaluation for analysis and code generation
***************************************************************************************************/
#pragma once
#include "semantics.h"

/***************************************************************************************************
										Defines/Typedefs
***************************************************************************************************/

class eval_expr_c
{
public:
	typedef struct
	{
		int offset;
		int ref_level;
	} mem_result_t;

private:
	const cfg_c* block, * func, * root;
	const struct_t* cur_struct;
	const structlist_c* slist;
	mem_result_t mem_result;
	bool error_on_invalid;

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

	inline void init(bool invalid_eq_error)
	{
		block = NULL;
		func = NULL;
		root = NULL;
		cur_struct = NULL;
		slist = NULL;
		error_on_invalid = invalid_eq_error;
	}

public:
	/* Evaluates a const expr. Asserts if invalid */
	int Constant(const tree_c* head) const;

	/* Returns the constant offset from the object. Name is the var name of the struct */
	mem_result_t Memory(
		const tree_c* head, const cfg_c* _block, const cfg_c* _func, const cfg_c* root, 
		const structlist_c* sl, 
		tree_c** data);

	eval_expr_c() { init(true); }
	eval_expr_c(bool fail_on_invalid_expr) { init(fail_on_invalid_expr); }
};