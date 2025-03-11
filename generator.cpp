#include "generator.h"

const char* asmfilename = "C:/ti83/rl/test.z80";
const unsigned short outflags = BACKGROUND_BLUE | FOREGROUND_RED | FOREGROUND_BLUE | FOREGROUND_GREEN | FOREGROUND_INTENSITY;

void generator_c::Generate(tree_c* _root, cfg_c* _graph, tdata_t* _tdata, unsigned* symbol_top, igraph_c* _igraph)
{
	cfg_c* block;

	printf("Generating code...\n");

	InitFile(asmfilename);

	//symtbl = symbols;
	tdata = _tdata;
	symtbl_top = *symbol_top;
	root = _root;
	graph = _graph;
	igraph = _igraph;

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
		if (Code(stmt) != NT_DATA_DECL)
			Error("Unhandled global block '%s'", Str(stmt));

		PrintSourceLine(stmt);
		CG_DataDeclaration(stmt, graph);
	}

	//dump the dataqueue - no need to zero this
	fprintf(f, "%s", dataqueue);
}

void generator_c::CG_FunctionBlock(cfg_c* block)
{
	cfg_c* b;
	

	stack[stack_top++] = block->id;
	ASM_Label(block->id);

	for (int i = 0; b = block->GetLink(i); i++)
	{
		switch (b->block_type)
		{
		case BLOCK_ENTRY:
		case BLOCK_REG:
			CG_RegBlock(b);
			break;
		case BLOCK_EXIT:
			CG_ExitBlock(b);
			break;
		default: Error("Unhandled block '%s'", b->id); break;
		}
	}

	stack_top--;

	if (*dataqueue)
	{
		fprintf(f, "%s", dataqueue);
		memset(dataqueue, 0, DATALUMP_SIZE);
	}
}

void generator_c::CG_RegBlock(cfg_c* block)
{
	tree_c* stmt;
	paralleli_t childblock = 0;

	for (int i = 0; stmt = block->GetStmt(i); i++)
	{
		switch (Code(stmt))
		{
		case NT_DATA_DECL:	
			PrintSourceLine(stmt);
			CG_DataDeclaration(stmt, block);
			break;
		case NT_INSTRUCTION:	CG_Instruction(stmt, block);	break;
		case NT_FOR_CLAUSE:		
			CG_ForLoop(stmt, block, block->GetLink(childblock)); 
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

void generator_c::CG_DataDeclaration(tree_c* node, cfg_c* block)
{
	tree_c* child;
	int		i = 1;
	int		code;
	int		res;
	regi_t	reg;
	tdatai_t	dataofs;

	do
	{
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

		if ((dataofs = DataOfs(block, child)) < 0)
			continue; //unused var

		reg = RegAlloc(DataOfs(block, child));
		if (!IsSReg(reg))
		{//non-global hardware reg
			if (code == NT_SINGLE_DATA_DECL)
			{//initialize it
				ASM_CLoad(reg, res);
				MarkReg(reg, dataofs);
			}
			//otherwise, this variable gets initialized later in the program
			continue;
		}

		//an actual data entry is used iff - the data is used && (it is held in a software reg || it is global)
		if (node->Get(0)->Hash()->V() == CODE_BYTE)
			ASM_Data(".db", child, res);
		else
			ASM_Data(".dw", child, res);
		MarkReg(reg, dataofs);
	} while (1);
}


