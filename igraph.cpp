#include "semantics.h"


//
//Interference Node
//


bool inode_c::AddLink(int l)
{
	if (num_links >= (LINKS_MAX))
		return false; //too bad!

	links[num_links] = l;
	num_links++;
	return true;
}


//
//Interference Graph
//


void igraph_c::Clear()
{
	if (!num_nodes)
		return;

	delete[] nodes;
	num_nodes = 0;
}

void igraph_c::Disp()
{
	
}

//
//REGISTER ALLOCATION
//

int cfg_c::TrimVars(cfg_c* parent, int count)
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

			//Warning("Unused variable %s\n", DataName(symbol));
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
		t->size = data[ti]->size;
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

void cfg_c::SortTDataList(tdata_t** tdata, int count)
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

#if OLD_REG_CODE
int	cfg_c::FirstValidColor(unsigned flags)
{
	int first = REG_B;

	//important: registers are incremented from their initial color value

	if (flags & DF_GLOBAL)
	{//don't use hregs for globals
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
			first = REG_BC; //save BC
	}

	//first = 0;
	return first;
}

int	cfg_c::Iteratend(unsigned flags)
{
	int iteratend;
	iteratend = 1 + !(flags & DF_BYTE);
	return iteratend;
}
#endif

void cfg_c::ColorGraph(int symbol_count, igraph_c* graph, tdata_t* tdata)
{
	const int max_stack = SI_LAST_GENERAL + SI_STACK_COUNT;
	bool available[max_stack];
	int local_count = SI_LOCAL_MIN;

	memset(available, true, sizeof(bool) * max_stack);

	for (int symbol = 0; symbol < symbol_count; symbol++)
	{
		inode_c* n = (*graph)[symbol];
		tdata_t* x = &(tdata)[symbol];
		int link_cnt = n->LinkCnt();

		if (x->flags & DF_LABEL)
		{//pseudo data type
			n->si.data = 0; //n->color = -1;
			continue;
		}
		else if (x->flags & DF_STATIC)
		{//static - always live so this function is N/A
			//FIXME: cut these out of the below function as well
			n->si.local_flag = 1;
			n->si.local = local_count;
			local_count += x->size; 
			continue;
		}

		//flag colors used by adjacent vertices as unavailable
		for (int j = 0; j < link_cnt; j++)
		{
			inode_c* l = (*graph)[n->Link(j)];
			tdata_t* t = &(tdata)[j];

			if (l->si.reg_flag)
			{
				for(int i = 0; i < t->size; i++) //mark successive bytes as marked, too
					available[l->si.reg + i] = false;
			}
			else if (l->si.stack_flag)
			{
				for (int i = 0; i < t->size; i++)
					available[l->si.stack + i] = false;
			}
		}

		//Find first available reg/stack index
		int index;
		if (x->flags & DF_OTHER_MASK) //structs, arrays, etc. cannot be held in regs
			index = SI_LAST_GENERAL + 1;
		else
			index = static_cast<int>(REG::B);

		for (; index < max_stack; index++)
		{
			if (available[index])
			{
				if (x->flags & DF_OTHER_MASK)
				{//TESTME: structs/arrays don't have their lifecycles determined yet
					for (int offset = 1; offset < x->size; offset++)
					{
						if (!available[index + offset])
							goto invalidstoragelocation; //try the next stack block
					}
					goto foundstoragelocation;
				}
				else if (x->flags & (DF_WORD | DF_PTR))
				{
					if ((index <= SI_LAST_GENERAL) && !(index % 2)) //can't have a word in 'e' or 'c' - 
						continue; //('c', 'e' and 'l' are all even)

					if (((index + 1) < max_stack) && available[index + 1])
						goto foundstoragelocation;
					index++;
				}
				else
				{//Byte - no need to check bytes beside this one
					goto foundstoragelocation;
				}
			}
		invalidstoragelocation:;
		}

		Error("More than 127 bytes of stack space used!");

	foundstoragelocation:

		if (index > SI_LAST_GENERAL)
		{//stack
			n->si.stack_flag = 1;
			n->si.stack = index;
		}
		else
		{//register
			n->si.reg_flag = 1;
			n->si.reg = index;
		}

		//clean up available vertices
		//Technically only need to set the indices that have been reset
		memset(available, true, sizeof(available));
	}
}


//todo: make this an analyzer function
void cfg_c::BuildIGraph(int symbol_cnt, igraph_c* igraph, tdata_t** tdata)
{
	int		count = (int)data.size();
	cfg_c** offsets;
#if OLD_REG_CODE
	bool* available;
#endif
	int		link_cnt;

	R_TotalLinks(); //set total_links
	offsets = new cfg_c * [total_links];
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
			continue; //totally unused

		if (t1->flags & DF_LABEL)
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

			if (t2->flags & DF_LABEL)
				continue;

			//TESTME!!!
			if (sb2 < eb1 && eb2 > sb1)
				(*igraph)[i]->AddLink(j);
			else if (sb2 == eb1 && eb2 == sb1)
			{
				if (s2 <= e1 && e2 >= s1)
					(*igraph)[i]->AddLink(j);
			}
			else if (sb2 == eb1)
			{
				if (s2 <= e1)
					(*igraph)[i]->AddLink(j);
			}
			else if (eb2 == sb1)
			{
				if (e2 >= s1)
					(*igraph)[i]->AddLink(j);
			}
		}
	}
#if OLD_REG_CODE
	//color in the graph
	available = new bool[REGS_TOTAL];
	memset(available, true, REGS_TOTAL);

	for (int i = 0; i < symbol_cnt; i++)
	{
		inode_c* n = (*igraph)[i];
		tdata_t* x = &(*tdata)[i];
		link_cnt = n->LinkCnt();

		if (x->flags & DF_LABEL)
		{//pseudo data type
			n->color = -1;
			continue;
		}

		//flag colors used by adjacent vertices as unavailable
		for (int j = 0; j < link_cnt; j++)
		{
			inode_c* l = (*igraph)[n->Link(j)];
			if (l->color != -1)
				available[l->color] = false;
		}


		//find first available color - assign it
		int iteratend = Iteratend(x->flags); //everything except for bytes are inc'd by 2. FIXME - byte array
		for (int k = FirstValidColor(x->flags); k < REGS_TOTAL; k += iteratend)
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
				//if (k == REG_HL)
				//	continue; //'hl' is reserved

				if (!available[k - REG_IXL] || !available[k - REG_IXL + 1])
					continue; //check if the hi or low nibble is already used
			}

			if (available[k])
			{
				n->color = k;
				break; //got one
			}
		}

		//clean up available vertices
		for (int j = 0; j < link_cnt; j++)
		{
			inode_c* l = (*igraph)[n->Link(j)];
			if (l->color != -1)
				available[l->color] = true;
		}
	}

	delete[] available;
#endif
	ColorGraph(symbol_cnt, igraph, *tdata);

}