#include "generator.h"

const char* asmfilename = "C:/ti83/rl/test.z80";
const unsigned short outflags = BACKGROUND_BLUE | FOREGROUND_RED | FOREGROUND_BLUE | FOREGROUND_GREEN | FOREGROUND_INTENSITY;

void generator_c::Generate(tree_c* _root, data_t* symbols, unsigned* symbol_top)
{
	printf("Generating code...\n");

	InitFile(asmfilename);

	symtbl = symbols;
	symtbl_top = *symbol_top;
	root = _root;
	VisitNode(root);
	*symbol_top = symtbl_top;

	SetOutFlags(outflags);
	printf("\n");
	PrintFile();
	ResetOutFlags();
	printf("\n");
	fclose(f);
}

//program control
void generator_c::VisitNode(tree_c* node)
{
	tree_c* child;
	const kv_c* kv = node->Hash();

	//printf("%s\n", kv->K());
	switch (kv->V())
	{
	case NT_FUNC_DEF:
		CG_FuncDef(node);
		break;
	default:
		for (int i = 0; child = node->Get(i); i++)
		{
			VisitNode(child);
		}
	}
	

}

void generator_c::CG_FuncDef(tree_c* node)
{//'subr' <identifier> '(' <parameter_list>* ')' <compound_statement>
	tree_c* child;
	int i = 4;

	//should maybe get rid of func def, then pass the root as the parent and use and offset to get the name and the rest of the stuff

	child = node->Get(1); //Func name
	stack[stack_top++] = child;
	ASM_Label(child->Hash()->K());

	child = node->Get(3);
	if (child->Hash()->V() != CODE_RPAREN)
		Error("parameter lists are unsupported\n"); //i needs to be changed here

	child = node->Get(i);
	CG_CompoundStatement(child);

	ASM_Ret(NULL);
}

void generator_c::CG_DataDeclaration(tree_c* node)
{
	tree_c* child;
	int i = 1;
	int code;

	switch (node->Get(0)->Hash()->V())
	{
	case CODE_BYTE:
	{
		do
		{
			child = node->Get(i);
			code = child->Hash()->V();
			i++;

			if (code == CODE_COMMA)
				continue;
			if (code == CODE_SEMICOLON)
				break;

			if (code == NT_SINGLE_DATA_DECL)
			{//constant expression
				int res = Constant_Expression(child->Get(2));

				ASM_Data(".db", child->Get(0), res);
			}
			else
				ASM_Data(".db", child, "0");

		} while (1);

	}
		break;
	default: break;
	}

}


