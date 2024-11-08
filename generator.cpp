#include "generator.h"

const char* asmfilename = "C:/ti83/rl/test.z80";
const char* asmheader = ".nolist\n#include \"../inc/ti83p.inc\"\n.list\n.org\tuserMem - 2\n.db\t\tt2ByteTok, tAsmCmp\n\n";

void generator_c::Generate(tnode_c* _root)
{
	printf("Generating code...\n");

	InitFile(asmfilename);

	root = _root;
	VisitNode(root);

	fclose(f);
}

void generator_c::VisitNode(tnode_c* node)
{
	tnode_c* child;
	const kv_c* kv = node->Hash();

	//printf("%s\n", kv->K());
	switch (kv->V())
	{
	case NT_FUNC_DEF:
		VisitFuncDef(node);
		break;
	default:
		for (int i = 0; child = node->Get(i); i++)
		{
			VisitNode(child);
		}
	}
	

}

void generator_c::VisitFuncDef(tnode_c* node)
{
	tnode_c* child;
	int i;

	//should maybe get rid of func def, then pass the root as the parent and use and offset to get the name and the rest of the stuff

	child = node->Get(1); //Func name
	Label(child->Hash()->K());

	for (i = 0; child = node->Get(i); i++)
	{
		if (child->Hash()->V() == CODE_LBRACKET)
		{//skip the parms and the bracket
			i++;
			break;
		}
	}

	/*
	child = node->Get(i); //closed statement
	child = child->Get(0); 

	switch (child->Hash()->V())
	{
	case CODE_ADD:
		//child = child->Get(1);
		printf("");
		break;
	default:
		Error("Compilation for %s has not been implemented yet\n", child->Hash()->K());
	}
	*/
}


//
//ASSEMBLER FILE FUNCTIONS
//
void generator_c::Label(const char* name)
{
	fprintf(f, name);
	fprintf(f, ":\n");
}




void generator_c::InitFile(const char* filename)
{
	fopen_s(&f, filename, "w+");

	if (!f)
		Error("Couldn't open assembler file %s\n", filename);

	fprintf(f, asmheader);
}