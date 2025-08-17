#include "generator.h"

const char* asmfilename = "C:/ti83/rl/test.z80";
const unsigned short outflags = BACKGROUND_BLUE | FOREGROUND_RED | FOREGROUND_BLUE | FOREGROUND_GREEN | FOREGROUND_INTENSITY;

void generator_c::Generate(tree_c* _root, cfg_c* _graph, tdata_t* _tdata, unsigned* symbol_top)
{
	cfg_c* block;

	printf("Generating code...\n");

	InitFile(asmfilename);

	//symtbl = symbols;
	tdata = _tdata;
	symtbl_top = *symbol_top;
	root = _root;
	graph = _graph;

	for (int i = 0; block = graph->GetLink(i); i++)
	{
		if (block->block_type == BLOCK_FUNC)
			CG_FunctionBlock(block);
		else
			Error("Bad block %s", block->id);
	}

	//allocate global vars
	CG_Global();

	*symbol_top = symtbl_top;

	SetOutFlags(outflags);
	printf("\n");
	PrintFile();
	ResetOutFlags();
	printf("\n");
	fclose(f);
}

//
//program control
//

void generator_c::CG_Global()
{
	tree_c* stmt;

	for (int i = 0; stmt = graph->GetStmt(i); i++)
	{
		if (Code(stmt) != NT_DATA_DECL && Code(stmt) != NT_STRUCT_DECL)
			Error("Unhandled global block '%s'", Str(stmt));

		PrintSourceLine(stmt);

		if (Code(stmt) == NT_DATA_DECL)
			CG_DataDeclaration(stmt, graph);
		else
			CG_StructDeclaration(stmt, graph);
	}

	//dump the dataqueue - no need to zero this
	fprintf(f, "%s", dataqueue);
}

void generator_c::CG_Stack()
{
	if (!stack_allocated)
		return;

	//Allocate space on the stack
	ASM_Push(REG::IXH);
	ASM_CLoad(REG::IXH, 0, 2);
	ASM_RAdd(REG::IXH, REG::SP, 2);

	if (stack_allocated > 4)
	{
		//NOTE: can't just do an 8-bit load; the stack grows downwards so hl needs sign extension regardless
		//27 \ 4
		ASM_CLoad(REG::H, -stack_allocated, 2); //10 cycles \ 3 bytes
		ASM_RAdd(REG::H, REG::SP, 2); // 11 \ 1
		ASM_RLoad(REG::SP, REG::H, 2); // 6 \ 1
	}
	else
	{
		//24 \ 4 for 4
		for (int i = 0; i < stack_allocated; i++)
			ASM_Dec(REG::SP, 2); //6 \ 1
	}

	//Initialize the variables
	fprintf(f, "%s", stack_queue);
}

void generator_c::CG_UnStack()
{
	if (!stack_allocated)
		return;

	ASM_RLoad(REG::SP, REG::IXH, 2);
	ASM_Pop(REG::IXH);
}

void generator_c::CG_FunctionBlock(cfg_c* block)
{
	ASM_Label(block->id);
	subr_name = block->id;
	data_decls_allowed = true;
	stack_allocated = 0;

	for (int i = 0; cfg_c* b = block->GetLink(i); i++)
	{
		switch (b->block_type)
		{
		case BLOCK_ENTRY:
		case BLOCK_REG:
			CG_RegBlock(b);
			break;
		case BLOCK_EXIT:
			CG_UnStack();
			CG_ExitBlock(b);
			break;
		default: Error("Unhandled block '%s'", b->id); break;
		}
	}


	if (*dataqueue)
	{
		fprintf(f, "%s", dataqueue);
		memset(dataqueue, 0, DATALUMP_SIZE);
	}
	subr_name = NULL;
}

void generator_c::CG_RegBlock(cfg_c* block)
{
	tree_c* stmt;
	paralleli_t childblock = 0;

	for (int i = 0; stmt = block->GetStmt(i); i++)
	{
		PrintSourceLine(stmt);
		switch (Code(stmt))
		{
		case NT_DATA_DECL: 
			if (!data_decls_allowed)
				Error("Data declarations are only allowed at the start of blocks");
			CG_DataDeclaration(stmt, block); 
			break;
		case NT_STRUCT_DECL: 
			if (!data_decls_allowed)
				Error("Data declarations are only allowed at the start of blocks");
			CG_StructDeclaration(stmt, block); 
			break;
		case NT_INSTRUCTION: 
			if (data_decls_allowed)
				CG_Stack(); //first instruction; setup the stack before execution proper starts

			CG_Instruction(stmt, block);	
			data_decls_allowed = false;
			break;
		case NT_FOR_CLAUSE: 
			data_decls_allowed = true;
			CG_ForLoop(stmt, block, block->GetLink(childblock)); 
			data_decls_allowed = false;
			childblock++;		
			break;
		default: Error("Unhandled stmt %s", Str(stmt));			break;
		}
	}

	//go through links here
}

void generator_c::CG_ExitBlock(cfg_c* block)
{
	if (!block->GetStmt(0))
	{//Final exit statement
		ASM_Ret("");
		return;
	}
}

void generator_c::CG_StructDeclaration(tree_c* node, cfg_c* block)
{

}

void generator_c::CG_DataDeclaration(tree_c* node, cfg_c* block)
{
	tree_c* child;
	int		i; 
	int		code;
	int		res;

	//Locate the actual varname/decl node
	//there is a data type before the ident at the very least; skip it
	for (i = 1; ; i++)
	{
		child = node->Get(i);
		if (child->Hash()->V() == CODE_TEXT || child->Hash()->V() == NT_SINGLE_DATA_DECL)
			break;
	}

	do
	{
		tdata_t* t;

		child = node->Get(i);
		code = child->Hash()->V();
		i++;
		res = 0;

		if (code == CODE_COMMA)
			continue;
		if (code == CODE_SEMICOLON)
			break;

		if (code == NT_SINGLE_DATA_DECL)
		{//constant expression
			res = Constant_Expression(child->Get(2));
			child = child->Get(0); //get the actual name
		}

		if (!(t = Data(block, child)))
			continue; //unused var

		if (t->si.local_flag)
		{//requires program space
			if (t->size == 1) //FIXME: structs and arrays
				ASM_Data(".db", child, res);
			else
				ASM_Data(".dw", child, res);
		}
		else if (t->si.stack_flag)
		{
			ASM_StackInit(res, stack_allocated, t->size);
			stack_allocated += t->size;
		}

		//Anything allocated on the stack or in registers are initialized later, if required

	} while (1);
}


