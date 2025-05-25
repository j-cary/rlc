#include "semantics.h"

void cfg_c::Set(const char* _id, BLOCK_TYPE _type)
{
	strncpy_s(id, _id, 16); //change to strcpy once id is dynamic
	block_type = _type;
}

void cfg_c::AddStmt(tree_c* s)
{
	statements.push_back(s);
}

cfg_c* cfg_c::AddLink(const char* _id, BLOCK_TYPE _type)
{
	cfg_c* l = new cfg_c;
	l->Set(_id, _type);

	links.push_back(l);

	//add all the parent's data to this link as well
	/*
	for (int pi = 0; pi < data.size(); pi++)
	{
		data_t* d = data[pi];
		l->AddData(d);
	}
	*/

	return l;
}

cfg_c* cfg_c::AddLink(const char* _id, BLOCK_TYPE _type, cfg_c* sibling)
{
	cfg_c* l = new cfg_c;
	l->Set(_id, _type);

	links.push_back(l);

	//add all the parent's data to this link as well
	/*
	for (int pi = 0; pi < sibling->data.size(); pi++)
	{
		data_t* d = sibling->data[pi];
		l->AddData(d);
	}
	*/

	return l;
}

cfg_c* cfg_c::AddLink(const char* _id, BLOCK_TYPE _type, data_t* init)
{
	cfg_c* l = new cfg_c;
	l->Set(_id, _type);

	links.push_back(l);
	l->AddData(init); //add this first so it gets b assigned to it

	//add all the parent's data to this link as well
	/*
	for (int pi = 0; pi < data.size(); pi++)
	{
		data_t* d = data[pi];
		l->AddData(d);
	}
	*/

	return l;
}


tree_c* cfg_c::GetStmt(int ofs)
{
	int sz = (int)statements.size();

	if (!sz || (ofs >= sz))
		return NULL;

	return statements.at(ofs);
}

cfg_c* cfg_c::GetLink(int ofs)
{
	int sz = (int)links.size();

	if (!sz || (ofs >= sz))
		return NULL;

	return links.at(ofs);
}

//
//DATA
//

void cfg_c::AddData(data_t* _d)
{
	int pos = StmtCnt();

	data.push_back(_d);
	start.push_back(pos);
	end.push_back(pos);
	startb.push_back(this);
	endb.push_back(this);
	uses.push_back(0);
	//flags.push_back(_flags);
}

bool cfg_c::SetDataStart(const char* name, int _start)
{
	for (int i = 0; i < data.size(); i++)
	{
		if (!strcmp(name, data[i]->var->K()))
		{
			start[i] = _start;
			return true;
		}
	}
	Warning("%s is not a recorded variable\n", name);
	return false;
}

bool cfg_c::SetDataEnd(const char* name, int _end)
{
	for (int i = 0; i < data.size(); i++)
	{
		if (!strcmp(name, data[i]->var->K()))
		{
			end[i] = _end;
			return true;
		}
	}
	Warning("%s is not a recorded variable\n", name);
	return false;
}

void cfg_c::SetDataEndBlock(const char* name, cfg_c* block)
{
	for (int i = 0; i < data.size(); i++)
	{
		if (!strcmp(name, data[i]->var->K()))
		{
			endb[i] = block;
			return;
		}
	}
	Warning("%s is not a recorded variable\n", name);
}

void cfg_c::IncDataUses(const char* name)
{
	for (int i = 0; i < data.size(); i++)
	{
		if (!strcmp(name, data[i]->var->K()))
		{
			uses[i]++;
			return;
		}
	}
	Warning("%s is not a recorded variable\n", name);
}



//To check for redefs, recurse down the tree until the block holding the variable in question is found.
//From there, go back up the tree and check each node, its younger siblings, but NOT and of their children, has defined the symbol

#define CRD_DEADEND 0
#define CRD_NOREDEF	1 //first set after finding the block the data in question is in
#define CRD_REDEF	2

//no need to pass these as parameters every time
const char* redef_name;
cfg_c*		redef_block;
//dataflags_t redef_flags;

//set above vars before calling
int cfg_c::R_CheckRedef()
{
	if (this == redef_block)
		return CRD_NOREDEF; //found the right block

	for (int i = 0; i < (int)links.size(); i++)
	{
		cfg_c* link = links[i];
		int ret = link->R_CheckRedef();

		if (!ret)
			continue;

		if (ret == CRD_REDEF)
			return CRD_REDEF;

		//just found the block, check it and any of its previous siblings for the symbol
		for (int li = 0; li <= i; li++)
		{
			cfg_c* sibling = links[li];

			for (int di = 0; di < (int)sibling->data.size(); di++)
			{
				if (!strcmp(sibling->data[di]->var->K(), redef_name))
				{//redefinition
					return CRD_REDEF;
				}
			}

			if (sibling == link)
				return CRD_NOREDEF;
		}
	}

	return CRD_DEADEND; //this should never be the final return value from this function
}

int cfg_c::R_CheckGlobalRedef()
{
	cfg_c* c;
	for (int i = 0; i < links.size(); i++)
	{
		c = links[i];
		for (int j = 0; j < c->data.size(); j++)
		{
			if (!strcmp(redef_name, c->data[j]->var->K()))
				return CRD_REDEF;
		}

		if (c->R_CheckGlobalRedef() == CRD_REDEF)
			return CRD_REDEF;
	}

	return CRD_NOREDEF;
}

bool cfg_c::CheckRedef(const char* name, cfg_c* top, cfg_c* root, dataflags_t flags)
{
	redef_name = name;
	redef_block = this;
	//redef_flags = flags;

	if (flags & DF_GLOBAL) //also top == NULL
	{//these are visible from everywhere in the program, and can only be defined in ROOT
		if(root->R_CheckGlobalRedef() == CRD_REDEF)
			return 0;
	}
	else
	{
		if (top->R_CheckRedef() == CRD_REDEF)
			return 0; //found it in the local scope
	}


	//might still be a global
	for (int i = 0; i < root->data.size(); i++)
	{
		if (!strcmp(name, root->data[i]->var->K()))
			return 0;
	}

	return 1; //not in our function or global scope
}

#define GSD_DEADEND	0
#define GSD_NODECL	1
#define GSD_DECL	2

//these are set by the function
data_t* scopedata;
cfg_c*	scopeblock;

int cfg_c::R_GetScopedData()
{
	if (this == redef_block)
		return GSD_NODECL; //found the right block

	for (int i = 0; i < (int)links.size(); i++)
	{
		cfg_c* link = links[i];
		int ret = link->R_GetScopedData();

		if (ret == GSD_DEADEND)
			continue;

		if (ret == GSD_DECL)
			return GSD_DECL;

		//just found the block, check it and any of its previous siblings for the symbol
		for (int li = 0; li <= i; li++)
		{
			cfg_c* sibling = links[li];

			for (int di = 0; di < (int)sibling->data.size(); di++)
			{
				if (!strcmp(sibling->data[di]->var->K(), redef_name))
				{//found the symbol
					scopedata = sibling->data[di];
					scopeblock = sibling;
					return GSD_DECL;
				}
			}

			if (sibling == link)
				return GSD_NODECL;
		}
	}

	return CRD_DEADEND; //this should never be the final return value from this function
}

data_t* cfg_c::ScopedDataEntry(const char* name, cfg_c* top, cfg_c* root, cfg_c** localblock)
{
	redef_name = name;
	redef_block = this;

	if (top->R_GetScopedData() != GSD_DECL)
	{//could still be a global var
		for (int i = 0; i < root->data.size(); i++)
		{
			if (!strcmp(name, root->data[i]->var->K()))
			{
				scopedata = root->data[i];
				*localblock = root;
				return scopedata;
			}
		}
		return NULL;
	}

	*localblock = scopeblock;
	return scopedata;
}

#undef CRD_DEADEND
#undef CRD_NOREDEF
#undef CRD_REDEF
#undef GSD_DEADEND
#undef GSD_NODECL
#undef GSD_DECL

#define ISI_NOTSTRUCT	0
#define ISI_ISSTRUCT	1
#define ISI_NOTFOUND	2

//recurse until the source block is found
//then search from the bottom up for the declaration
cfg_c* instanceblock; //the block that the instance is being used in

int cfg_c::R_IsStructInstance(const char* name)
{
	if (instanceblock == this)
		return ISI_NOTSTRUCT;

	for (int i = 0; i < (int)links.size(); i++)
	{
		cfg_c* link = links[i];
		int ret = link->R_IsStructInstance(name);

		if (ret == ISI_NOTFOUND)
			continue;

		if (ret == ISI_ISSTRUCT)
			return ISI_ISSTRUCT; 

		//just found the block, check it and any of its previous siblings for the symbol
		for (int li = 0; li <= i; li++)
		{
			cfg_c* sibling = links[li];

			for (int di = 0; di < (int)sibling->data.size(); di++)
			{
				if (!strcmp(sibling->data[di]->var->K(), name))
				{//found it, see if its actually a struct
					if(sibling->data[di]->flags & DF_STRUCT)
						return ISI_ISSTRUCT;
					else 
						return ISI_NOTSTRUCT;
				}
			}

			if (sibling == link)
				return ISI_NOTFOUND;
		}
	}

	return ISI_NOTFOUND;
}

bool cfg_c::IsStructInstance(const char* name, cfg_c* func, cfg_c* root)
{
	int ret;

	instanceblock = this;
	ret = func->R_IsStructInstance(name);

	if (ret == ISI_ISSTRUCT)
		return true;
	else if (ret == ISI_NOTSTRUCT)
		return false;

	//didn't find it in the function - check if its a global
	for (int i = 0; i < root->data.size(); i++)
	{
		if (!strcmp(root->data[i]->var->K(), name))
		{//found it
			if (root->data[i]->flags & DF_STRUCT)
				return true;
			else
				return false;
		}
	}

	Error("IsStructInstance: Couldn't find '%s' anywhere", name); //should never reach this
	return false;
}

#undef ISI_NOTSTRUCT
#undef ISI_ISSTRUCT
#undef ISI_NOTFOUND


cfg_c::~cfg_c()
{
	for (std::vector<cfg_c*>::iterator it = links.begin(); it != this->links.end(); it++)
		delete* it;

	links.clear();//Hopefully clear the list...
	statements.clear();
	data.clear();
	start.clear();
	end.clear();
	startb.clear();
	endb.clear();
	uses.clear();
	//flags.clear();
}

char g_ctabstr[DEPTH_MAX * 2];
int	 g_ctabs;

static int blocki = 0;
void cfg_c::Disp(bool igraph_disp, igraph_c* igraph, tdata_t* tdata)
{
	memset(g_ctabstr, '\0', DEPTH_MAX * 2 * sizeof(char));
	g_ctabs = 0;

	//display the tdata array

	//display the interference graph
	if (igraph_disp)
	{
		//num_nodes = sizeof tdata
		for (int i = 0; i < igraph->num_nodes; i++)
		{
			inode_c* node = &igraph->nodes[i];
			tdata_t* t1 = &tdata[i];

			//printf("%s:%s\t%i - ", g_ctabstr, t1->var->K(), node->color);
			printf("%s:%s\t%s - ", g_ctabstr, t1->var->K(), node->ToStr());

			for (int j = 0; j < node->LinkCnt(); j++)
			{
				int l = node->Link(j);
				tdata_t* t2 = &tdata[l];

				printf("%s ", t2->var->K());
			}
			printf("\n");
		}
	}

	blocki = 0;
	R_Disp( igraph, tdata);
}


void cfg_c::R_Disp( igraph_c* igraph, tdata_t* tdata)
{
	std::vector<cfg_c*>::iterator	it = links.begin();
	std::vector<tree_c*>::iterator	s_it = statements.begin();

	if (g_ctabs > (DEPTH_MAX * 2) - 2)
		Error("Parser: Attempted to display too many recursive tree nodes!\n");

	printf("%s[[%s]] %i", g_ctabstr, id, blocki);

	if (data.size())
		printf("\n%s", g_ctabstr);

	for (paralleli_t i = 0; i < data.size(); i++)
	{
		//printf(":%s [[%s]] %pi-%pi ", data[pi]->var->K(), endb[pi]->id, start[pi], end[pi]);
		printf(":%s ", data[i]->var->K());
		if (startb[i] != endb[i])
			printf("[[%s]] ", endb[i]->id);
		printf("%ix %i-%i #%i#", uses[i], start[i], end[i], data[i]->tdata);

	}
	printf("\n");


	for (; s_it != this->statements.end(); s_it++)
		printf("%s%s\n", g_ctabstr, (*s_it)->Hash()->K());

	g_ctabstr[g_ctabs++] = ' ';
	g_ctabstr[g_ctabs++] = ' ';

	blocki++;
	for (; it != this->links.end(); it++)
		(*it)->R_Disp( igraph, tdata);

	g_ctabstr[--g_ctabs] = '\0';
	g_ctabstr[--g_ctabs] = '\0';
}