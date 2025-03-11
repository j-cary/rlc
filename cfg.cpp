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

int cfg_c::TrimVars(cfg_c* parent, int count )
{
	int newcount = count;

	//figure out which variables are even used
	for (paralleli_t i = count - 1; i >= 0; i--)
	{
		if (start[i] == end[i])
		{//unused in THIS block
			continue;
			//check if its needed for any childblocks
			

			//check if its needed for any children younger than this

			//Warning("Unused variable %s\n", DataName(i));
			start.erase(start.begin() + i);
			end.erase(end.begin() + i);
			data.erase(data.begin() + i);
			//flags.erase(flags.begin() + pi);
			newcount--;
		}
	}

	return newcount;
}

//NEW VARIABLE/REGISTER SYSTEM:
//each block will contain ONLY its own local variables.
//upon using a local variable, the variable will be updated accordingly
//	USE/DEFINE RULES HERE
//	init: set start/end
//	used: set end
//upon using a parent variable, the parent will update its own var accordingly
//	USE/DEFINE RULES
//	init: store the block (new parallel block array?) , set start/end
//	use: store the block (if necessary), set end. Start should be zeroed if it hasn't already got another value
// 
//OPTIMIZATION:
//	If local var and start == end, remove it
//	If global var, pi.e. used in another block, keep it
//	Should keep track of var usage and sort accordingly. pi.e. frequently used vars are guaranteed to get registers
//	Sort control vars for for loops first - gonna have to make fake for loops for 
// 
//INTERFERENCE:
//	Go through local var list: order of blocks and positions in those blocks determine liveness
//	Pre-order traversal with a static int to keep track of block position in code 
//		
//COLORING:
//	make available[] static. 
//	recursively allocate colors - this will bias earlier vars. Could be remediated by optimization
//no more flag array
//fix up graph functions like color and stuff since blocks no longer keep copies of global vars

static int total_links = 0;
void cfg_c::R_TotalLinks()
{
	total_links++;
	for (int i = 0; i < (int)links.size(); i++)
		links[i]->R_TotalLinks();
}

static int blocki = 0;
void cfg_c::R_GenBlockOfs(cfg_c** offsets)
{
	offsets[blocki] = this;
	blocki++;
	for (int i = 0; i < (int)links.size(); i++)
		links[i]->R_GenBlockOfs(offsets);
}

static int tdatai = 0;
void cfg_c::R_BuildTDataList(tdata_t* tdata, cfg_c** offsets)
{
	for (int ti = 0; ti < (int)data.size(); ti++)
	{
		tdata_t* t = &tdata[tdatai];
		t->var = data[ti]->var;
		t->start = start[ti];
		t->end = end[ti];
		t->flags = data[ti]->flags;
		data[ti]->tdata = tdatai++;

		//generate the block offsets for the start and end blocks of this variable
		int gotone = 0;
		for (int linki = 0; linki < total_links; linki++)
		{
			if (offsets[linki] == startb[ti])
			{
				t->startb = linki;
				if (gotone)
					break;
				gotone++;
			}
			if (offsets[linki] == endb[ti])
			{
				t->endb = linki;
				if (gotone)
					break;
				gotone++;
			}
		}
	}


	for (int i = 0; i < (int)links.size(); i++)
		links[i]->R_BuildTDataList(tdata, offsets);
}

bool cfg_c::R_SwapTDataIndices(tdatai_t old, tdatai_t _new)
{
	for (int i = 0; i < data.size(); i++)
	{
		if (data[i]->tdata == old)
		{//done
			data[i]->tdata = _new;
			return true;
		}
	}

	for (int i = 0; i < links.size(); i++)
	{
		if (links[i]->R_SwapTDataIndices(old, _new))
			return true;
	}

	return false;
}

void cfg_c:: SortTDataList(tdata_t** tdata, int count)
{
	//for loop control vars always come first to hopefully grab b. Everything else is sorted by frequency

	//insertion sort
	for (int i = 1; i < count; i++)
	{
		tdata_t x = (*tdata)[i];
		int j = i;

		if (x.flags & DF_FORCTRL)
		{
			for (; j > 0; j--)
			{
				tdata_t* y, * z;

				if (((*tdata)[j - 1].flags & DF_FORCTRL))
					break;

				//non-control var. Shift it to the right
				y = &(*tdata)[j];
				z = &(*tdata)[j - 1];
				*y = *z;

				if (j == i)
					R_SwapTDataIndices(j - 1, -1); //this can't just be set to j since the source is already j
				else
					R_SwapTDataIndices(j - 1, j);
			}
			(*tdata)[j] = x; //this will be to the right of all previous control vars. Order shouldn't really matter between these
			R_SwapTDataIndices(i, j);
			R_SwapTDataIndices(-1, i);
		}
		else
		{//regular var - sort it by most frequent usage
			//TODO: need to keep track of usage in tdata.
			/*
			for (; j > 0; j--)
			{
				tdata_t* y, * z;

				if (((*tdata)[j - 1].flags & DF_FORCTRL))
					break;

				//var with less usages, shift it to the right
				y = &(*tdata)[j];
				z = &(*tdata)[j - 1];
				*y = *z;
			}
			(*tdata)[j] = x;
			*/
		}
	}
}

int	cfg_c::FirstValidColor(unsigned flags)
{
	int first = REG_B;

	//important: registers are incremented from their initial color value

	if (flags & DF_GLOBAL)
	{
		if (flags & DF_BYTE)
			first = REG_L;
		else
			first = REG_HL;
	}
	else
	{
		if (flags & DF_BYTE)
			first = REG_B;
		else
			first = REG_DE; //save BC
	}

	//first = 0;
	return first;
}

//todo: make this an analyzer function
void cfg_c::BuildIGraph(int symbol_cnt, igraph_c* igraph, tdata_t** tdata)
{
	int		count = (int)data.size();
	cfg_c**	offsets;
	bool*	available;
	int		link_cnt;

	R_TotalLinks(); //set total_links
	offsets = new cfg_c*[total_links];
	*tdata = new tdata_t[symbol_cnt];
	igraph->nodes = new inode_c[symbol_cnt];
	igraph->num_nodes = symbol_cnt;
	R_GenBlockOfs(offsets); //generate block indices
	R_BuildTDataList(*tdata, offsets);//get a plain list of all the data and their respective lifetimes
	SortTDataList(tdata, symbol_cnt);//control vars first, then sort by usage, globals go last 

	//check for interference between vars
	for (int i = 0; i < symbol_cnt; i++)
	{
		tdata_t* t1 = &(*tdata)[i];
		int s1 = t1->start;
		int e1 = t1->end;
		int sb1 = t1->startb;
		int eb1 = t1->endb;

		if (sb1 == eb1 && s1 == e1)
			continue;

		for (int j = 0; j < symbol_cnt; j++)
		{
			tdata_t* t2 = &(*tdata)[j];
			int s2 = t2->start;
			int e2 = t2->end;
			int sb2 = t2->startb;
			int eb2 = t2->endb;

			if (i == j) //don't check for interference with ourselves
				continue;

			//TESTME!!!
			if (sb2 < eb1 && eb2 > sb1)
				(*igraph)[i]->AddLink(j);
			else if (sb2 == eb1 && eb2 == sb1)
			{
				if(s2 <= e1 && e2 >= s1)
					(*igraph)[i]->AddLink(j);
			}
			else if (sb2 == eb1)
			{
				if(s2 <= e1)
					(*igraph)[i]->AddLink(j);
			}
			else if (eb2 == sb1)
			{
				if(e2 >= s1)
					(*igraph)[i]->AddLink(j);
			}
		}
	}

	//color in the graph
	available = new bool[REGS_TOTAL];
	memset(available, true, REGS_TOTAL);

	for (int i = 0; i < symbol_cnt; i++)
	{
		inode_c*	n = (*igraph)[i];
		int			k;
		tdata_t*	x = &(*tdata)[i];
		link_cnt = n->LinkCnt();

		//flag colors used by adjacent vertices as unavailable
		for (int j = 0; j < link_cnt; j++)
		{
			inode_c* l = (*igraph)[n->Link(j)];
			if (l->color != -1)
				available[l->color] = false;
		}

		
		//find first available color - assign it
		int iteratend = 1 + !(x->flags & DF_BYTE); //everything except for bytes are inc'd by 2. FIXME - byte array
		for (k = FirstValidColor(x->flags); k < REGS_TOTAL; k += iteratend)
		{
			//check for interference with aliased registers
			if (x->flags & DF_BYTE)
			{
				int basereg = k;

				if (!(k % 2))
					basereg--;

				if (basereg > -1 && !available[basereg + REG_IXL])
					continue; //check if tehe 16-bit reg is used
			}
			else
			{
				if (k == REG_HL)
					continue; //'hl' is reserved

				if (!available[k - REG_IXL] || !available[k - REG_IXL + 1])
					continue; //check if the hi or low nibble is already used
			}

			if (available[k])
				break; //got one
		}

		n->color = k;

		//clean up available vertices
		for (int j = 0; j < link_cnt; j++)
		{
			inode_c* l = (*igraph)[n->Link(j)];
			if (l->color != -1)
				available[l->color] = true;
		}
	}

	delete[] available;
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
cfg_c* scopeblock;

int cfg_c::R_GetScopedData()
{
	if (this == redef_block)
		return GSD_NODECL; //found the right block

	for (int i = 0; i < (int)links.size(); i++)
	{
		cfg_c* link = links[i];
		int ret = link->R_GetScopedData();

		if (!ret)
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

			printf("%s:%s\t%i - ", g_ctabstr, t1->var->K(), node->color);

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