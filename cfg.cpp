#include "semantics.h"
#include "evaluate_expression.h"

/***************************************************************************************************
                                          Link Section
***************************************************************************************************/

void cfg_c::Set(const char* _id, BLOCK _type)
{
	strncpy_s(id, _id, 16); //change to strcpy once id is dynamic
	block_type = _type;
}

void cfg_c::AddStmt(tree_c* s)
{
	statements.push_back(s);
}

cfg_c* cfg_c::AddLink(const char* _id, BLOCK _type)
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

cfg_c* cfg_c::AddLink(const char* _id, BLOCK _type, cfg_c* sibling)
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

cfg_c* cfg_c::AddLink(const char* _id, BLOCK _type, data_t* init)
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


tree_c* cfg_c::GetStmt(int ofs) const
{
	int sz = (int)statements.size();

	if (!sz || (ofs >= sz))
		return NULL;

	return statements.at(ofs);
}

cfg_c* cfg_c::GetLink(int ofs) const
{
	int sz = (int)links.size();

	if (!sz || (ofs >= sz))
		return NULL;

	return links.at(ofs);
}

/***************************************************************************************************
                                          Data Section
***************************************************************************************************/

void cfg_c::AddData(data_t* _d)
{
	int pos = StmtCnt();

	_d->start_line = pos; //start.push_back(pos);
	_d->end_line = pos; //end.push_back(pos);
	startb.push_back(this);
	endb.push_back(this);
	uses.push_back(0);

	data.push_back(_d);
}

bool cfg_c::SetDataStart(const char* name, int _start)
{
	for (int i = 0; i < data.size(); i++)
	{
		if (!strcmp(name, data[i]->var->Str()))
		{
			data[i]->start_line = _start; // start[i] = _start;
			return true;
		}
	}

	/* Const exprs may be piped into these; it's ok to just let them fail */
	return false;
}

bool cfg_c::SetDataEnd(const char* name, int _end)
{
	for (int i = 0; i < data.size(); i++)
	{
		if (!strcmp(name, data[i]->var->Str()))
		{
			data[i]->end_line = _end; // end[i] = _end;
			return true;
		}
	}

	return false;
}

void cfg_c::SetDataEndBlock(const char* name, cfg_c* block)
{
	for (int i = 0; i < data.size(); i++)
	{
		if (!strcmp(name, data[i]->var->Str()))
		{
			endb[i] = block;
			return;
		}
	}
}

void cfg_c::IncDataUses(const char* name)
{
	for (int i = 0; i < data.size(); i++)
	{
		if (!strcmp(name, data[i]->var->Str()))
		{
			uses[i]++;
			return;
		}
	}
}

/***************************************************************************************************
                                      Redefinition Checking
***************************************************************************************************/

//To check for redefs, recurse down the tree until the block holding the variable in question is found.
//From there, go back up the tree and check each node, its younger siblings, but NOT and of their children, has defined the symbol

//no need to pass these as parameters every time
const char* redef_name;
const cfg_c*redef_block;

//set above vars before calling
cfg_c::CRD cfg_c::R_CheckRedef()
{
	if (this == redef_block)
		return CRD::NOREDEF; //found the right block

	for (int i = 0; i < (int)links.size(); i++)
	{
		cfg_c* link = links[i];
		CRD ret = link->R_CheckRedef();

		if (ret != CRD::DEADEND) //CHECKME
			continue;

		if (ret == CRD::REDEF)
			return CRD::REDEF;

		//just found the block, check it and any of its previous siblings for the symbol
		for (int li = 0; li <= i; li++)
		{
			cfg_c* sibling = links[li];

			for (int di = 0; di < (int)sibling->data.size(); di++)
			{
				if (!strcmp(sibling->data[di]->var->Str(), redef_name))
				{//redefinition
					return CRD::REDEF;
				}
			}

			if (sibling == link)
				return CRD::NOREDEF;
		}
	}

	return CRD::DEADEND; //this should never be the final return value from this function
}

cfg_c::CRD cfg_c::R_CheckGlobalRedef()
{
	cfg_c* c;
	for (int i = 0; i < links.size(); i++)
	{
		c = links[i];
		for (int j = 0; j < c->data.size(); j++)
		{
			if (!strcmp(redef_name, c->data[j]->var->Str()))
				return CRD::REDEF;
		}

		if (c->R_CheckGlobalRedef() == CRD::REDEF)
			return CRD::REDEF;
	}

	return CRD::NOREDEF;
}

bool cfg_c::CheckRedef(const char* name, cfg_c* top, cfg_c* root, dataflags_t flags)
{
	redef_name = name;
	redef_block = this;
	//redef_flags = flags;

	if (flags & DF_GLOBAL) //also top == NULL
	{//these are visible from everywhere in the program, and can only be defined in ROOT
		if(root->R_CheckGlobalRedef() == CRD::REDEF)
			return 0;
	}
	else
	{
		if (top->R_CheckRedef() == CRD::REDEF)
			return 0; //found it in the local scope
	}


	//might still be a global
	for (int i = 0; i < root->data.size(); i++)
	{
		if (!strcmp(name, root->data[i]->var->Str()))
			return 0;
	}

	return 1; //not in our function or global scope
}

/***************************************************************************************************
                                      Scoped-Data Aquisition
***************************************************************************************************/

//these are set by the function
static const data_t*	scopedata;
static const cfg_c*		scopeblock;

cfg_c::GSD cfg_c::R_GetScopedData()
{
	if (this == redef_block)
		return GSD::NODECL; //found the right block

	for (int i = 0; i < (int)links.size(); i++)
	{
		cfg_c* link = links[i];
		GSD ret = link->R_GetScopedData();

		if (ret == GSD::DEADEND)
			continue;

		if (ret == GSD::DECL)
			return GSD::DECL;

		//just found the block, check it and any of its previous siblings for the symbol
		for (int li = 0; li <= i; li++)
		{
			cfg_c* sibling = links[li];

			for (int di = 0; di < (int)sibling->data.size(); di++)
			{
				if (!strcmp(sibling->data[di]->var->Str(), redef_name))
				{//found the symbol
					scopedata = sibling->data[di];
					scopeblock = sibling;
					return GSD::DECL;
				}
			}

			if (sibling == link)
				return GSD::NODECL;
		}
	}

	return GSD::DEADEND; //this should never be the final return value from this function
}

data_t* cfg_c::ScopedDataEntry(const char* name, cfg_c* top, cfg_c* root, cfg_c** localblock) const
{
	redef_name = name;
	redef_block = this;

	if (top->R_GetScopedData() != GSD::DECL)
	{//could still be a global var
		for (int i = 0; i < root->data.size(); i++)
		{
			if (!strcmp(name, root->data[i]->var->Str()))
			{
				scopedata = root->data[i];
				*localblock = root;
				return (data_t*)scopedata;
			}
		}
		return NULL;
	}

	// These are just const because they aren't modified in the function
	*localblock = (cfg_c*)scopeblock;
	return (data_t*)scopedata;
}

#if 0
/***************************************************************************************************
                                     Struct Instance Checking
***************************************************************************************************/
//recurse until the source block is found
//then search from the bottom up for the declaration
static const cfg_c* instanceblock; //the block that the instance is being used in

cfg_c::ISI cfg_c::R_IsStructInstance(const char* name) const
{
	if (instanceblock == this)
		return ISI::NOTSTRUCT;

	for (int i = 0; i < (int)links.size(); i++)
	{
		cfg_c* link = links[i];
		ISI ret = link->R_IsStructInstance(name);

		if (ret == ISI::NOTFOUND)
			continue;

		if (ret == ISI::ISSTRUCT)
			return ISI::ISSTRUCT; 

		//just found the block, check it and any of its previous siblings for the symbol
		for (int li = 0; li <= i; li++)
		{
			cfg_c* sibling = links[li];

			for (int di = 0; di < (int)sibling->data.size(); di++)
			{
				if (!strcmp(sibling->data[di]->var->Str(), name))
				{//found it, see if its actually a struct
					if(sibling->data[di]->flags & DF_STRUCT)
						return ISI::ISSTRUCT;
					else 
						return ISI::NOTSTRUCT;
				}
			}

			if (sibling == link)
				return ISI::NOTFOUND;
		}
	}

	return ISI::NOTFOUND;
}

bool cfg_c::IsStructInstance(const char* name, const cfg_c* func, const cfg_c* root) const
{
	ISI ret;

	instanceblock = this;
	ret = func->R_IsStructInstance(name);

	if (ret == ISI::ISSTRUCT)
		return true;
	else if (ret == ISI::NOTSTRUCT)
		return false;

	//didn't find it in the function - check if its a global
	for (int i = 0; i < root->data.size(); i++)
	{
		if (!strcmp(root->data[i]->var->Str(), name))
		{//found it
			return root->data[i]->flags & DF_STRUCT;
		}
	}

	Error("IsStructInstance: Couldn't find '%s' anywhere", name); //should never reach this
	return false;
}
#endif

cfg_c::~cfg_c()
{
	for (std::vector<cfg_c*>::iterator it = links.begin(); it != this->links.end(); it++)
		delete* it;

	links.clear();//Hopefully clear the list...
	statements.clear();
	data.clear();
	//start.clear();
	//end.clear();
	startb.clear();
	endb.clear();
	uses.clear();
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

			//printf("%s:%s\t%i - ", g_ctabstr, t1->var->Str(), node->color);
			printf("%s:%s\t%s - ", g_ctabstr, t1->var->Str(), t1->ToStr(t1->size));

			for (int j = 0; j < node->LinkCnt(); j++)
			{
				int l = node->Link(j);
				tdata_t* t2 = &tdata[l];

				printf("%s ", t2->var->Str());
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
		Error("Parser: Attempted to display too many recursive tree nodes!");

	printf("%s[[%s]] %i", g_ctabstr, id, blocki);

	if (data.size())
		printf("\n%s", g_ctabstr);

	for (paralleli_t i = 0; i < data.size(); i++)
	{
		//printf(":%s [[%s]] %pi-%pi ", data[pi]->var->Str(), endb[pi]->id, start[pi], end[pi]);
		printf(":%s ", data[i]->var->Str());
		if (startb[i] != endb[i])
			printf("[[%s]] ", endb[i]->id);
		printf("%ix %i-%i", uses[i], data[i]->start_line, data[i]->end_line);
		//printf("%ix %i-%i #%i#", uses[i], start[i], end[i], data[i]->tdata);

	}
	printf("\n");


	for (; s_it != this->statements.end(); s_it++)
		printf("%s%s\n", g_ctabstr, (*s_it)->Hash()->Str());

	g_ctabstr[g_ctabs++] = ' ';
	g_ctabstr[g_ctabs++] = ' ';

	blocki++;
	for (; it != this->links.end(); it++)
		(*it)->R_Disp( igraph, tdata);

	g_ctabstr[--g_ctabs] = '\0';
	g_ctabstr[--g_ctabs] = '\0';
}