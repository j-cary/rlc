#include "semantics.h"


//
//Interference Node
//


bool inode_c::AddLink(int l)
{
	if (num_links >= (LINKS_MAX))
		Error("Too many variable interferences!"); //return false; //too bad!

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

//ABSTRACT:
// Allocate space for stack, auto, and static vars.
// Output: 
//	Tdata: sorted array of data
//	Igraph: structure parallel to tdata. Contains interference information between tdata indicies. FIXME: move si over to tdata
// Note: ALL variables are allocated at once. 
// SYSTEM:
//	Count the # of cfg links there are. Including ROOT!
//	Generate an in-order list of blocks e.x. [ENTRY] 1 [IF] 2 [ELSE] 3 [REG] 4
//	Use the list to determine variable lifetime
//	Sort Tdata. First globals/static, then loop ctrl, then by usage
//		Note: Static vars MUST go first! More on this later.
//	Determine interference between vars. Each var has an antry in the igraph listing all its interfer-ers
//		Note: Static vars have NO explicit interfer-ers. All vars are implicitly understood to interfere with them.
//	Color in the graph - modified Dijkstra's algorithm:
//	Notes about static var allocation: 
//		Static vars are allocated first and their space in the local array is NEVER de-allocated
//		Static vars should be allocated first so that autos have no chance of overriding their space



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
//	make stack_available[] static. 
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

//Priority list: static vars, loop control vars, usage count
void cfg_c::SortTDataList(tdata_t** tdata, int count)
{
	//for loop control vars always come first to hopefully grab b. Everything else is sorted by frequency

	//insertion sort
	for (int i = 1; i < count; i++)
	{
		tdata_t x = (*tdata)[i];
		int j = i;
		/*
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
		*/
		if (x.flags & DF_STATIC)
		{
			for (; j > 0; j--)
			{
				tdata_t* y, * z;

				if (((*tdata)[j - 1].flags & DF_STATIC))
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

int	cfg_c::StackIndex(tdata_t* var, bool available[], const int size)
{
	int index;
	int last_general = (int)REG::LAST_GENERAL_REG();

	if (var->flags & DF_OTHER_MASK) //structs, arrays, etc. cannot be held in regs
		index = last_general + 1;
	else
		index = REG::B;

	for (; index < size; index++)
	{
		if (available[index])
		{
			if (var->flags & DF_OTHER_MASK)
			{//TESTME: structs/arrays don't have their lifecycles determined yet
				for (int offset = 1; offset < var->size; offset++)
				{
					if (!available[index + offset])
						goto invalidstoragelocation; //try the next stack block
				}
				return index;
			}
			else if (var->flags & (DF_WORD | DF_PTR))
			{
				if ((index <= last_general) && !(index % 2)) //can't have a word in 'e' or 'c' - 
					continue;									//('c', 'e' and 'l' are all even)

				if (((index + 1) < size) && available[index + 1])
					return index;
				index++;
			}
			else
			{//Byte - no need to check bytes beside this one
				return index;
			}
		}
	invalidstoragelocation:;
	}

	Error("More than 127 bytes of stack space used!");
	return 0;
}

//Autos can be stored in registers or in '.db's. They can overlap in memory. Statics cannot
int cfg_c::AutoIndex(tdata_t* var, bool stack_available[], const int stack_size, bool local_available[], const int local_size, bool* local)
{
	*local = true;

	if (!(var->flags & DF_STATIC) && (var->flags & (DF_BYTE | DF_WORD | DF_PTR)))
	{//auto byte/word/ptr

		if (var->flags & DF_BYTE)
		{
			for (int index = REG::AUTO_MIN(); index <= REG::LAST_GENERAL_REG(); index++)
				if (stack_available[index])
				{
					*local = false;
					return index; //Found a register 
				}
		}
		else
		{
			for (int index = REG::AUTO_MIN(); index <= REG::LAST_GENERAL_REG(); index += 2)
				if (stack_available[index] && stack_available[index + 1])
				{
					*local = false;
					return index; //Found a register 
				}
		}
	}

	for (int index = 0; index < local_size; index++)
	{
		if (local_available[index])
		{
			if (var->flags & DF_OTHER_MASK)
			{//TESTME: structs/arrays don't have their lifecycles determined yet
				for (int offset = 1; offset < var->size; offset++)
				{
					if (!local_available[index + offset])
						goto invalidstoragelocation; //try the next stack block
				}
				return index;
			}
			else if (var->flags & (DF_WORD | DF_PTR))
			{
				if (((index + 1) < local_size) && local_available[index + 1])
					return index;
				index++;
			}
			else
			{//Byte - no need to check bytes beside this one
				return index;
			}
		}
	invalidstoragelocation:;
	}

	Error("More than 65535 bytes of data space used!");
	return 0;
}

void cfg_c::ColorGraph(int symbol_count, igraph_c* graph, tdata_t* tdata)
{
	constexpr int max_stack = REG::LAST_GENERAL_REG() + REG::STACK_MAX();
	bool stack_available[max_stack]; //Reg vars, both stack & auto, go in here
	static bool local_available[REG::AUTO_CNT()];
	int local_count = REG::AUTO_MIN();

	memset(stack_available, true, sizeof(stack_available));
	memset(local_available, true, sizeof(local_available));

	for (int symbol = 0; symbol < symbol_count; symbol++)
	{
		inode_c* n = (*graph)[symbol];
		tdata_t* x = &(tdata)[symbol];
		int link_cnt = n->LinkCnt();
		int index;

		if (x->flags & DF_LABEL)
		{//pseudo data type
			x->si.data = 0;//n->si.data = 0;
			continue;
		} 

		//flag colors used by adjacent vertices as unavailable
		//Note: static vars only have to worry about other static vars; they will never enter this loop
		for (int j = 0; j < link_cnt; j++)
		{
			inode_c* l = (*graph)[n->Link(j)];
			tdata_t* t = &(tdata)[n->Link(j)];

			if (t->si.reg_flag)
			{
				for(int i = 0; i < t->size; i++) //mark successive bytes as marked, too
					stack_available[t->si.reg + i] = false;
			}
			else if (t->si.stack_flag)
			{
				for (int i = 0; i < t->size; i++)
					stack_available[t->si.stack + i] = false;
			}
			else if (t->si.local_flag)
			{
				for (int i = 0; i < t->size; i++)
					local_available[t->si.local + i] = false;
			}
		}

		if (x->flags & DF_STACK)
		{
			index = StackIndex(x, stack_available, max_stack);

			if (index > REG::LAST_GENERAL_REG())
			{//stack
				x->si.stack_flag = 1;
				x->si.stack = index;
			}
			else
			{//register
				x->si.reg_flag = 1;
				x->si.reg = index;
			}
		}
		else
		{
			bool local;
			index = AutoIndex(x, stack_available, max_stack, local_available, REG::AUTO_CNT(), &local);

			if (local)
			{//saved in a '.db'
				x->si.local_flag = 1;
				x->si.local = index;

				if (x->flags & DF_STATIC) //permanently make space for these. These indices are NEVER cleared in the cleanup step.
					for (int offset = 0; offset < x->size; offset++)
						local_available[index + offset] = false;
			}
			else
			{//saved in a reg
				x->si.reg_flag = 1;
				x->si.reg = index;
			}
		}

		
		//clean up vertices
		for (int j = 0; j < link_cnt; j++)
		{
			inode_c* l = (*graph)[n->Link(j)];
			tdata_t* t = &(tdata)[n->Link(j)];

			if (t->si.reg_flag)
			{
				for (int i = 0; i < t->size; i++) //mark successive bytes as marked, too
					stack_available[t->si.reg + i] = true;
			}
			else if (t->si.stack_flag)
			{
				for (int i = 0; i < t->size; i++)
					stack_available[t->si.stack + i] = true;
			}
			else if (t->si.local_flag)
			{
				if(!(t->flags & DF_STATIC))
					for (int i = 0; i < t->size; i++) //Only clear out auto's. Static vars, once allocated, do not get cleared 
						local_available[t->si.local + i] = true;
			}
		}

	}
}

void cfg_c::FixupStackIndices(int symbol_cnt, tdata_t* tdata)
{
	for (int symbol = 0; symbol < symbol_cnt; symbol++)
	{
		if (tdata[symbol].si.stack_flag)
			tdata[symbol].si.stack -= REG::LAST_GENERAL_REG();
	}
}

//todo: make this an analyzer function
void cfg_c::BuildIGraph(int symbol_cnt, igraph_c* igraph, tdata_t** tdata)
{
	int		count = (int)data.size();
	cfg_c** offsets;
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

		if (t1->flags & DF_STATIC)
			continue; //Static vars implicitly interfere with all other vars

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

			if (t2->flags & DF_STATIC)
				continue; //Static vars implicitly interfere with all other vars

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

	ColorGraph(symbol_cnt, igraph, *tdata);
	FixupStackIndices(symbol_cnt, *tdata);

	delete[] offsets;
}