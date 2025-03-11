#include "semantics.h"

//<identifier> - need to be forward declared. Thinking a stack which pops stuff off after completing a compound statement

void analyzer_c::GenerateAST(tree_c* _root, cfg_c* _graph, data_t* symbols, unsigned* symbols_top, tdata_t** _tdata, igraph_c* _igraph, structlist_c* sl)
{
	struct timeb start, end;
	float time_seconds;

	ftime(&start);
	printf("\nGenerating AST...\n");

	root = _root;
	graph = _graph;
	symtbl = symbols;
	symtbl_top = *symbols_top;
	tdata = *_tdata;
	igraph = _igraph;
	slist = sl;
	graph->Set("ROOT", BLOCK_ROOT);

	SimplifyTree(root, NULL);//Pass 1
	printf("==\tDEBUG: Reduced parse tree\t==\n");
	root->Disp();
	
	/*
	*/
	CFG_Start(root);//Pass2
	BuildIGraphs(graph);
	printf("==\tDEBUG: Control flow graph\t==\n");
	graph->Disp(true, igraph, tdata);
	
	printf("==\tDEBUG: User structure dump\t==\n");
	const struct_t* s;
	for (int i = 0; s = slist->StructInfo(i); i++)
	{
		printf("%s is %i byte(s) wide\n", s->name, s->length);
		if (s->first_member)
		{
			for (member_t* m = s->first_member; m; m = m->next)
			{
				printf("  %s\t%3i %3i", m->name, m->offset, m->length);

				if (m->flags & DF_SIGNED)
					printf(" signed");

				if (m->flags & DF_BYTE)
					printf(" byte");
				else if (m->flags & DF_WORD)
				{
					if (m->flags & DF_DOUBLE)
						printf(" dword");
					else
						printf(" word");
				}
				else if (m->flags & DF_FXD)
				{
					if (m->flags & DF_DOUBLE)
						printf(" dfixed");
					else
						printf(" fixed");
				}
				else if (m->flags & DF_STRUCT)
					printf(" %s", m->struct_name);

				if (m->flags & DF_ARRAY)
					printf(" array");


				printf("\n");
			}
		}

	}
	

	*symbols_top = symtbl_top;
	*_tdata = tdata;

	ftime(&end);
	time_seconds = (1000 * (end.time - start.time) + (end.millitm - start.millitm)) / 1000.0f;
	printf("Analyzed parse tree in %.4f second(s)\n", time_seconds);
}

void analyzer_c::SimplifyTree(tree_c* node, tree_c* parent)
{
	tree_c* child;
	const kv_c* kv = node->Hash();

	for (int i = 0; child = node->Get(i); i++)
		SimplifyTree(child, node);


	if (parent && node->Get(0) && !node->Get(1))
	{//just one operand
		parent->Collapse(node);
	}

	
}

void analyzer_c::CFG_Start(tree_c* node)
{
	tree_c* child;
	const kv_c* kv = node->Hash();
	const kv_c* kv_1;

	switch (kv->V())
	{
	case NT_FUNC_DEF:
		kv_1 = node->Get(1)->Hash(); //the function name
		cur_func = graph->AddLink(kv_1->K(), BLOCK_FUNC);
		CFG_FuncDef(node);
		cur_func = NULL;
		return;
		break;
	case NT_UNIT: break;
	case NT_DATA_DECL:
		CFG_DataDeclaration(node, graph);
		return;
		break;
	case NT_TYPE_DEF:
		CFG_TypeDef(node, graph);
		return;
		break;
	default:
		Error("semantics: %s is unhandled", kv->K());
		break;
	}

	for (int i = 0; child = node->Get(i); i++)
		CFG_Start(child);


}

void analyzer_c::CFG_FuncDef(tree_c* node)
{
	tree_c*		parms, * stmts;
	const kv_c* tmp = root->Hash();
	int			i;
	cfg_c*		block;

	parms = node->Get(3);

	if (parms->Hash()->V() == NT_PARAMETER_LIST || parms->Hash()->V() == NT_PARAMETER)
		i = 5;
	else
		i = 4;

	stmts = node->Get(i);

	
	block = cur_func->AddLink("ENTRY", BLOCK_ENTRY);
	
	CFG_Node(stmts, block, cur_func, 1, CODE_RBRACKET);

	cur_func->AddLink("EXIT", BLOCK_EXIT);
}

cfg_c* analyzer_c::CFG_Statement(tree_c* node, cfg_c* parent, cfg_c* ancestor)
{
	tree_c* child;
	tree_c* subnode;
	int		exitcode;
	cfg_c*	else_link, * if_link;
	int		i;

	//add the conditional to the current block
	child = node->Get(0);
	parent->AddStmt(child);

	//branch block
	if_link = parent->AddLink("OPEN_COND", BLOCK_COND);

	if (node->Get(1)->Hash()->V() == NT_COMPOUND_STMT)
	{
		i = 1;
		subnode = node->Get(1);
		exitcode = CODE_RBRACKET;
	}
	else
	{
		i = 1;
		subnode = node;
		exitcode = CODE_ELSE; //RBRACKET in openstatement
	}

	CFG_Node(subnode, if_link, parent, i, exitcode);

	else_link = ancestor->AddLink("ELSE", BLOCK_ELSE, parent);
	else_link->AddStmt(node->Get(2)); //else

	return NULL;
}

cfg_c* analyzer_c:: CFG_OpenStatement(tree_c* node, cfg_c* parent, cfg_c* ancestor)
{//if, if else, while, for
	tree_c* child;
	tree_c* subnode;
	cfg_c*	link, * else_link, * if_link;
	int		i;

	//TODO: add loop handling

	//add the conditional to the current block
	child = node->Get(0);
	parent->AddStmt(child);

	//branch block
	if_link = parent->AddLink("OPEN_COND", BLOCK_COND);

	if (node->Get(1)->Hash()->V() == NT_COMPOUND_STMT)
	{
		i = 1;
		subnode = node->Get(1);
	}
	else
	{//no { } pair
		i = 1;
		subnode = node;
	}

	//take the branch
	CFG_Node(subnode, if_link, parent, i, CODE_RBRACKET);

	if (child = node->Get(2))
	{//'else'
		else_link = ancestor->AddLink("ELSE", BLOCK_ELSE, parent);
		else_link->AddStmt(child);

		subnode = node->Get(3);

		if (subnode->Get(0)->Hash()->V() == NT_SELECTION_CLAUSE)
		{//'else if'
			else_link->AddStmt(subnode->Get(0));
			if (subnode->Get(1)->Hash()->V() == NT_COMPOUND_STMT)
			{
				subnode = subnode->Get(1); //get the comp stmt
				i = 1; //skip the '{'
			}
			else
				i = 1; //single statement, skip the clause

			else_link->Set("ELSE_IF", BLOCK_ELSEIF);
		}
		else
		{//does this ever even get reached?
			if (subnode->Hash()->V() == NT_COMPOUND_STMT)
				i = 1;
		}

		link = else_link->AddLink("OPEN_ELSE_COND", BLOCK_COND);

		CFG_Node(subnode, link, parent, i, CODE_RBRACKET);
	}


	link = ancestor->AddLink("REG", BLOCK_REG, parent);
	return link;
}

cfg_c* analyzer_c::CFG_ClosedStatement(tree_c* node, cfg_c* parent, cfg_c* ancestor)
{//if else, while, for
	tree_c* child;
	tree_c* subnode;
	int		code, exitcode;
	cfg_c*	link, *if_link, * else_link;
	int		i;

	child = node->Get(0);
	code = child->Hash()->V();
	parent->AddStmt(child);
	
	//add the conditional/loop statement to the current block
	if (code == NT_FOR_CLAUSE)
	{
		unsigned flags = DF_FORCTRL;
		int dt_code = node->Get(0)->Get(2)->Hash()->V();

		child = child->Get(3)->Get(0);//get the data name
		if (!child)
			Error("For loop control variable '%s' not initialized", node->Get(0)->Get(3)->Hash()->K());

		if_link = parent->AddLink("CLOSED_FOR", BLOCK_FOR);

		if (dt_code == CODE_BYTE)
			flags |= DF_BYTE;
		else if (dt_code == CODE_WORD)
			flags |= DF_WORD;

		MakeDataEntry(child->Hash(), if_link, flags);
	}
	else if (code == NT_WHILE_CLAUSE)
		if_link = parent->AddLink("CLOSED_WHILE", BLOCK_WHILE);
	else
		if_link = parent->AddLink("CLOSED_COND", BLOCK_COND);

	//branch/loop block
	if (node->Get(1)->Hash()->V() == NT_COMPOUND_STMT)
	{
		i = 1;
		subnode = node->Get(1);
		exitcode = CODE_RBRACKET;
	}
	else
	{
		i = 1;
		subnode = node;
		exitcode = CODE_ELSE;
	}

	CFG_Node(subnode, if_link, parent, i, exitcode);

	if (code != NT_FOR_CLAUSE && code != NT_WHILE_CLAUSE)
	{//if - handle the else
		else_link = ancestor->AddLink("ELSE", BLOCK_ELSE, parent);
		else_link->AddStmt(node->Get(2)); //else

		if (node->Get(3)->Hash()->V() == NT_COMPOUND_STMT)
		{
			i = 1;
			subnode = node->Get(3);
		}
		else
		{
			i = 3;
			subnode = node;
		}

		link = else_link->AddLink("CLOSED_ELSE_COND", BLOCK_COND);
		CFG_Node(subnode, link, parent, i, CODE_RBRACKET);
	}
	else
	{//some kind of loop
		parent->AddLink("LOOPBACK", BLOCK_LOOPBACK);
		if_link->SetDataEnd(child->Hash()->K(), if_link->StmtCnt()); //the control var is used for the whole block
	}


	link = ancestor->AddLink("REG", BLOCK_REG, parent);

	return link;
}

void analyzer_c::CFG_Node(tree_c* node, cfg_c* link, cfg_c* ancestor, int start, int exit_code)
{
	tree_c* child;
	int		code;

	for (int i = start; child = node->Get(i); i++)
	{
		code = child->Hash()->V();

		if (code == exit_code)
			break;

		switch (code)
		{
		case NT_DATA_DECL: CFG_DataDeclaration(child, link); break;
		case NT_INSTRUCTION: CFG_Instruction(child, link); break;

		case NT_OPEN_STMT: link = CFG_OpenStatement(child, link, ancestor); break;
		case NT_CLOSED_STMT: link = CFG_ClosedStatement(child, link, ancestor); break;

		default: Error("Unhandled block type %s", child->Hash()->K()); break;
		}
	}
}

void analyzer_c::CFG_DataDeclaration(tree_c* node, cfg_c* block)
{
	tree_c* child;
	unsigned flags = DF_NONE;
	int		code = node->Get(0)->Hash()->V();
	int		start = 1;

	if (code == CODE_SIGNED)
	{
		flags = DF_SIGNED;
		code = node->Get(1)->Hash()->V();
		start = 2; //skip the data type in the loop below
	}

	switch (code)
	{
	case CODE_BYTE:		flags |= DF_BYTE; break;
	case CODE_WORD:		flags |= DF_WORD; break;
	case CODE_LABEL:	flags |= DF_LABEL; break;
	case CODE_DWORD:	flags |= DF_WORD | DF_DOUBLE; break;
	case CODE_DFIXED:	flags |= DF_WORD | DF_FXD; break;
	case CODE_STRUCT:	flags |= DF_STRUCT; start = 2; break;
	default: break;
	}

	//is this an array decl?
	if (child = node->Get(start + 1))
	{
		if (child->Hash()->V() == CODE_LBRACE)
		{//it is
			child = node->Get(start);

			if (block == graph)
				flags |= DF_GLOBAL;
			flags |= DF_ARRAY;

			MakeDataEntry(child->Hash(), block, flags);
			block->AddStmt(node);
			return;
		}
	}

	for (int i = start; child = node->Get(i); i += 2)
	{
		if (child->Hash()->V() == NT_SINGLE_DATA_DECL)
			child = child->Get(0); //initialized - get the actual text

		if (block == graph)
			flags |= DF_GLOBAL;

		MakeDataEntry(child->Hash(), block, flags);
	}

	block->AddStmt(node);
}

void analyzer_c::CFG_Instruction(tree_c* node, cfg_c* block)
{//instr <oplist> ; or instr op ;
	tree_c* child, * op, * firstop;
	int		code;
	data_t* symbol;
	const char* name;
	int		opcnt;
	bool	initializer = true; //set if the instruction is NOT reliant on the first operand's value

	//modify from/to accordingly

	child = node->Get(1); //operand/oplist
	code = child->Hash()->V();

	if (code == NT_OPERANDS_TWO ||
		code == NT_OPERANDS_THREE ||
		code == NT_OPERANDS_ONE_TO_TWO ||
		code == NT_OPERANDS_ONE_TO_THREE ||
		code == NT_OPERANDS_TWO_TO_INF ||
		code == NT_OPERANDS_COMP ||
		code == NT_OPERANDS_FOUR ||
		code == NT_OPERANDS_RET ||
		code == NT_OPERANDS_CALL)
	{
		
	}
	else if (code == CODE_SEMICOLON)
	{//ret with no condition i.e. no operands
		block->AddStmt(node);
		return;
	}
	else
	{//one operand. i.e. inc a or inc x.y
		block->AddStmt(node);
		return;
	}

	firstop = child->Get(0);

	for (opcnt = 2; op = child->Get(opcnt); opcnt += 2)
	{
		if (!strcmp(firstop->Hash()->K(), op->Hash()->K()))
			initializer = false; //the first operand is used as a source somewhere
	}
	opcnt /= 2;

	if (initializer)
	{
		switch (node->Get(0)->Hash()->V())
		{
		case CODE_LD: //FIXME!!!: LD is a special case - ld dst1, dst2, ..., dstn, src
			initializer = true; break;

		case CODE_ADD:
		case CODE_SUB:
		case CODE_MUL:
		case CODE_DIV:
		case CODE_MOD:
		case CODE_AND:
		case CODE_OR:
		case CODE_XOR:
		case CODE_COMP:
		case CODE_CPM:

			if (opcnt == 2) //add x, y; -> x = y + x
				initializer = false;
			else //add x, y, z; -> x = z + y
				initializer = true;
			break;

		default: initializer = false; break;
		}
	}

	bool local;
	cfg_c* localblock = NULL;
	op = child->Get(0);
	symbol = DataEntry(op, block, &localblock);
	name = symbol->var->K();

	//check if the symbol is local
	local = block == localblock;
	
#if 0
	if (symbol && initializer && !(symbol->flags & DF_USED))
	{//not reliant on previous value

		block->SetDataStart(op->Hash()->K(), block->StmtCnt());
		//this^ won't work. maybe call from the function and go through all the child blocks
	}
#endif

	//FIXME: check for scope here
	//FIXME: this logic might just be plain wrong in a lot of cases

	if (symbol && initializer && !(symbol->flags & DF_USED))
	{//not reliant on previous value
		if (local)
		{
			block->SetDataStart(name, block->StmtCnt());
			block->SetDataEnd(name, block->StmtCnt());
		}
		else
		{
			//localblock->SetDataStart(op->Hash()->K(), block->StmtCnt());
			localblock->SetDataEndBlock(name, block);
			localblock->SetDataEnd(name, block->StmtCnt());
			localblock->IncDataUses(name);
		}
	}
	else if (!local)
	{//update the end block regardless
		localblock->SetDataEndBlock(name, block);
		localblock->SetDataEnd(name, block->StmtCnt());
		localblock->IncDataUses(name);
	}
	else
	{
		block->SetDataEnd(name, block->StmtCnt());
		block->IncDataUses(name);
	}
		

	//handle the rest of the operands
	for (int i = 2; op = child->Get(i); i += 2)
	{
		symbol = DataEntry(op, block, &localblock);
		if (block == localblock)
			block->SetDataEnd(op->Hash()->K(), block->StmtCnt());
		else
		{
			localblock->SetDataEnd(op->Hash()->K(), block->StmtCnt());
			localblock->SetDataEndBlock(op->Hash()->K(), block);
		}

		if (symbol)
		{
			symbol->flags |= DF_USED;
			localblock->IncDataUses(op->Hash()->K());
		}
	}
	

	block->AddStmt(node);
}

void analyzer_c::CFG_TypeDef(tree_c* node, cfg_c* block)
{
	tree_c* name, * child;
	int i;
	int s;
	int total_length = 0;

	//check for redefinition
	for ( i = 0; name = node->Get(i); i++) {}
	name = node->Get(i - 2);
	//MakeDataEntry(name->Hash(), block, DF_STRUCT | DF_GLOBAL);
	//s = slist->AddStruct(name->Hash()->K());
	s = MakeStructEntry(name->Hash(), block);

	//add the member vars to the struct
	for (int j = 2; j < i - 3; j++)
	{
		int k = 0;
		dataflags_t flags = DF_NONE;
		tree_c* subchild, * dname, * init = NULL;
		const char* structname = NULL;
		int dtype;
		int length = 0;
		int arraylength = 1;
		child = node->Get(j);

		subchild = child->Get(k);

		switch (subchild->Hash()->V())
		{
		case CODE_STRUCT:
			structname = child->Get(++k)->Hash()->K();
			dtype = subchild->Hash()->V();
			break;
		case CODE_SIGNED:
			flags |= DF_SIGNED;
			subchild = child->Get(++k);
		default:
			dtype = subchild->Hash()->V();
			break;
		}

		switch (dtype)
		{
		case CODE_LABEL:		Error("Structures cannot contain labels"); break;
		case CODE_BYTE:			flags |= DF_BYTE; break;
		case CODE_WORD:			flags |= DF_WORD; break;
		case CODE_DWORD:		flags |= DF_WORD | DF_DOUBLE; break;
		case CODE_FIXED:		flags |= DF_FXD; break;
		case CODE_DFIXED:		flags |= DF_FXD | DF_DOUBLE; break;
		case CODE_STRUCT:		flags |= DF_STRUCT; break;
		default: break;
		}

		dname = child->Get(++k); //the name or a data decl

		/*
		if (flags & DF_ARRAY && !(flags & DF_STRUCT))
		{//built in array, different syntax
			if (child->Get(++k)->Hash()->V() == CODE_LBRACE)
			{//explicit size
				arraylength = Constant_Expression(child->Get(++k));
				init = child->Get(k + 4); //get the initializer list, if there is one
			}
			else
			{//implicit size
				arraylength = 0;
				init = child->Get(k += 2);
				if (init->Hash()->V() == NT_INITIALIZER_LIST)
				{//more than just one const
					for (int l = 0; subchild = init->Get(l); l++)
					{
						if (subchild->Hash()->V() == CODE_COMMA)
							arraylength++;
					}
				}
				arraylength++;
			}

			if(arraylength < 2)
				Error("Array '%s' must have a length greater than 1", dname->Hash()->K());
		}
		else if (flags & DF_ARRAY && flags & DF_STRUCT)
		{//fixme: rewrite syntax for arrays of structs

		}
		*/

		if (dname->Hash()->V() == NT_SINGLE_DATA_DECL)
		{//figure out any initial value for simple decls
			init = dname->Get(2);
			dname = dname->Get(0);
		}
		else
		{//definitely not a simple decl
			tree_c* lbrace = child->Get(k + 1); //don't advance k since this might still be a simple decl

			if (lbrace->Hash()->V() == CODE_LBRACE)
			{//array
				k++;
				arraylength = Constant_Expression(child->Get(++k));
				init = child->Get(k + 4); //get the initializer list if there is one
				flags |= DF_ARRAY;
			}
		}

		//calculate the length of the data
		if (flags & DF_WORD || flags & DF_FXD)
			length = 2 * arraylength;
		else if (flags & DF_BYTE)
			length = arraylength;
		else if (flags & DF_STRUCT)
		{
			int s1 = slist->GetStruct(structname);
			if (s1 < 0)
				Error("Undeclared struct '%s'", structname);
			if (!strcmp(structname, name->Hash()->K()))
				Error("Structure '%s' cannot contain itself", structname);
			length = slist->StructLen(s1) * arraylength;
		}

		if (flags & DF_DOUBLE)
			length *= 2;

		slist->AddMemberVar(s, dname->Hash()->K(), flags, length, init, structname);
		total_length += length;

		if (flags & (DF_ARRAY | DF_STRUCT))
			continue; //arrays and structs can only have one decl per statement

		for (k++; (subchild = child->Get(k))->Hash()->V() != CODE_SEMICOLON; k++)
		{
			init = NULL;
			structname = NULL;
			if (subchild->Hash()->V() == CODE_COMMA)
				continue;
			if (subchild->Hash()->V() == NT_SINGLE_DATA_DECL)
			{
				init = subchild->Get(2);
				subchild = subchild->Get(0);
			}



			slist->AddMemberVar(s, subchild->Hash()->K(), flags, length, init, structname);
			total_length += length;
		}
	}

	slist->SetLen(s, total_length);
}


void analyzer_c::BuildIGraphs(cfg_c* block)
{
	block->BuildIGraph(symtbl_top, igraph, &tdata);
}

#define DECL_NA(v)	v == CODE_NUM_DEC || v == CODE_NUM_HEX || v == CODE_NUM_BIN || \
					v == NT_SHIFT_EXPR || v == NT_ADDITIVE_EXPR || v == NT_MULTIPLICATIVE_EXPR || v == NT_ARITHMETIC_POSTFIX_EXPR || v == NT_ARITHMETIC_PRIMARY_EXPR

data_t* analyzer_c::DataEntry(tree_c* d, cfg_c* block, cfg_c** localblock)
{
	data_t* data;
	const char* name = d->Hash()->K();

	//FIXME: variables inside these expressions need to be checked for.
	//problem for arithmetic and mem expressions - this will do for now.
	if (DECL_NA(d->Hash()->V()))
		return NULL;

	if (d->Hash()->V() == NT_MEMORY_EXPR)
	{//get the name of the object
		name = d->Get(0)->Hash()->K();
		//also have to check out if the members are valid
	}
	else if (d->Hash()->V() == NT_MEMORY_PRIMARY_EXPR)
	{//checkme
		name = d->Get(1)->Hash()->K();
	}

	//check if the data here is in scope
	if (!(data = block->ScopedDataEntry(name, cur_func, graph, localblock)))
		Error("Symbol '%s' is undeclared", name);

	return data;

	//check for reserved words - TMP
	//maybe add these to the symbol table...
	if(!strcmp(name, "print"))
		return NULL;


	//Error("Undeclared identifier %s", d->Hash()->K());
	return NULL;
}

void analyzer_c::MakeDataEntry(const kv_c* _var, cfg_c* _block, unsigned _flags)
{
	data_t* data;

	//check for re-definition
	if (!_block->CheckRedef(_var->K(), cur_func, graph, _flags))
		Error("Symbol '%s' already defined", _var->K());

	//check if a struct is using this name
	if (slist->GetStruct(_var->K()) != -1)
		Error("Symbol '%s' is already a structure name", _var->K());

	data = &symtbl[symtbl_top++];
	data->var = _var;
	data->block = (void*)_block;
	data->flags = _flags;
	_block->AddData(data);
	//_block->SetDataStart(_var->K(), 0);
}

int analyzer_c::MakeStructEntry(const kv_c* _var, cfg_c* _block)
{//_block might be a needless operand...
	dataflags_t flags = DF_STRUCT | DF_GLOBAL;
	int ret;

	//check for re-definition
	if (!_block->CheckRedef(_var->K(), cur_func, graph, flags))
		Error("Symbol '%s' already defined", _var->K());

	//check if a struct is using this name
	if (slist->GetStruct(_var->K()) != -1)
		Error("Symbol '%s' is already a structure name", _var->K());

	ret = slist->AddStruct(_var->K());
	if (ret < 0)
		Error("No more struct space");
	return ret;
}
