#include "semantics.h"

//<identifier> - need to be forward declared. Thinking a stack which pops stuff off after completing a compound statement

void analyzer_c::GenerateAST(tree_c* _root, cfg_c* _graph, data_t* symbols, unsigned* symbols_top)
{
	struct timeb start, end;
	float time_seconds;

	ftime(&start);
	printf("\nGenerating AST...\n");

	root = _root;
	graph = _graph;
	symtbl = symbols;
	symtbl_top = *symbols_top;
	graph->Set("ROOT", BLOCK_ROOT);

	SimplifyTree(root, NULL);//Pass 1
	printf("==\tDEBUG: Reduced parse tree\t==\n");
	root->Disp();
	
	CFG_Start(root);//Pass2
	BuildIGraphs(graph);
	printf("==\tDEBUG: Control flow graph\t==\n");
	graph->Disp(true, &igraph, tdata);
	

	*symbols_top = symtbl_top;

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
	default:
		Error("semantics: %s is unhandled", kv->K());
		break;
	}

	for (int i = 0; child = node->Get(i); i++)
		CFG_Start(child);


}

void analyzer_c::CFG_FuncDef(tree_c* node)
{
	tree_c*		child, * parms, * stmts;
	const kv_c* tmp = root->Hash();
	int			i;
	cfg_c*		block;

	parms = node->Get(3);

	if (parms->Hash()->V() == NT_PARAMETER_LIST || parms->Hash()->V() == NT_PARAMETER)
		i = 5;
	else
		i = 4;

	stmts = node->Get(i);

	//CFG_FuncBody(stmts);
	block = cur_func->AddLink("ENTRY", BLOCK_ENTRY);
	//cur_link = block;
	CFG_Node(stmts, block, cur_func, 1, CODE_RBRACKET);

	cur_func->AddLink("EXIT", BLOCK_EXIT);
}

cfg_c* analyzer_c::CFG_Statement(tree_c* node, cfg_c* parent, cfg_c* ancestor)
{
	tree_c* child;
	tree_c* subnode;
	int		code, exitcode;
	cfg_c*	link, * else_link, * if_link;
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
	int		code;
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
		data_t* init = &symtbl[symtbl_top];
		child = child->Get(3)->Get(0);//get the data name

		//TODO: check for re-definition
		//TODO: make a more general purpose function for this and  datadecls
		
		symtbl_top++;

		//if_link = parent->AddLink("CLOSED_FOR", BLOCK_FOR, init);
		if_link = parent->AddLink("CLOSED_FOR", BLOCK_FOR);

		init->flags = DF_BYTE;
		init->var = child->Hash();
		init->block = if_link;

		if_link->AddData(init);
		if_link->SetDataStart(child->Hash()->K(), 0);

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
	int		code;
	data_t* symbol;

	for (int i = 1; child = node->Get(i); i += 2)
	{
		code = child->Hash()->V();
		symbol = &symtbl[symtbl_top];

		if (code == NT_SINGLE_DATA_DECL)
			child = child->Get(0); //initialized - get the actual text

		//check for re-definition
		//FIXME: do this function specific
		for (int i = 0; i < symtbl_top; i++)
		{
			if (!strcmp(symtbl[i].var->K(), child->Hash()->K()))
				Error("Symbol re-declaration: %s", child->Hash()->K());
		}

		symbol->flags = DF_BYTE;
		symbol->var = child->Hash();
		symbol->block = (void*)block;
		symtbl_top++;

		//add the symbol to the block
		block->AddData(symbol);
	}

	block->AddStmt(node);
}

void analyzer_c::CFG_Instruction(tree_c* node, cfg_c* block)
{//instr <oplist> ; or instr op ;
	tree_c* child, * op, * firstop;
	int		code;
	data_t* symbol;
	int		opcnt;
	bool	initializer = true; //set if the instruction is NOT reliant on the first operand's value

	//modify from/to accordingly
	//TODO: check for undeclared identifiers

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
			block->SetDataStart(op->Hash()->K(), block->StmtCnt());
			block->SetDataEnd(op->Hash()->K(), block->StmtCnt());
		}
		else
		{
			//localblock->SetDataStart(op->Hash()->K(), block->StmtCnt());
			localblock->SetDataEndBlock(op->Hash()->K(), block);
			localblock->SetDataEnd(op->Hash()->K(), block->StmtCnt());//
		}
	}
	else if (!local)
	{//update the end block regardless
		localblock->SetDataEndBlock(op->Hash()->K(), block);
		localblock->SetDataEnd(op->Hash()->K(), block->StmtCnt());//
	}
	else
	{
		block->SetDataEnd(op->Hash()->K(), block->StmtCnt());

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

		if(symbol) 
			symbol->flags |= DF_USED;
	}
	

	block->AddStmt(node);
}


void analyzer_c::BuildIGraphs(cfg_c* block)
{
	block->BuildIGraph(symtbl_top, &igraph, &tdata);
}



data_t* analyzer_c::DataEntry(tree_c* d, cfg_c* block, cfg_c** localblock)
{
	if (d->Hash()->V() == CODE_NUM_DEC)
		return NULL;

	for (int i = 0; i < symtbl_top; i++)
	{
		if (!strcmp(symtbl[i].var->K(), d->Hash()->K()))
		{
			*localblock = (cfg_c*)symtbl[i].block;

			return &symtbl[i];
		}
	}

	//check for reserved words - TMP
	//maybe add these to the symbol table...
	if(!strcmp(d->Hash()->K(), "print"))
		return NULL;


	Error("Undeclared identifier %s", d->Hash()->K());
	return NULL;
}