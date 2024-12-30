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
	for (int i = 0; i < data.size(); i++)
	{
		data_t* d = data[i];
		l->AddData(d);
	}

	return l;
}

cfg_c* cfg_c::AddLink(const char* _id, BLOCK_TYPE _type, cfg_c* sibling)
{
	cfg_c* l = new cfg_c;
	l->Set(_id, _type);

	links.push_back(l);

	//add all the parent's data to this link as well
	for (int i = 0; i < sibling->data.size(); i++)
	{
		data_t* d = sibling->data[i];
		l->AddData(d);
	}

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
}

void cfg_c::SetDataStart(const char* name, int _start)
{
	for (int i = 0; i < data.size(); i++)
	{
		if (!strcmp(name, data[i]->var->Hash()->K()))
		{
			start[i] = _start;
			return;
		}
	}
	Warning("%s is not a recorded variable\n", name);
}

void cfg_c::SetDataEnd(const char* name, int _end)
{
	for (int i = 0; i < data.size(); i++)
	{
		if (!strcmp(name, data[i]->var->Hash()->K()))
		{
			end[i] = _end;
			return;
		}
	}
	Warning("%s is not a recorded variable\n", name);
}


int FirstAvailableColor(int* colors)
{
	int	color = 0;

	for (; colors[color] >= 0; color++);

	return colors[color]; //???
}


void cfg_c::BuildIGraph()
{
	int		count = (int)data.size();
	bool*	available;
	int		link_cnt;

	if (block_type == BLOCK_ROOT || block_type == BLOCK_FUNC || !count)
	{
		for (int i = 0; i < links.size(); i++)
			links[i]->BuildIGraph();
		return;
	}

	igraph.nodes = new inode_c[count]; //deleted in the igraph deconstructor
	igraph.num_nodes = count;

	//build the interference graph
	for (int i = 0; i < count; i++)
	{
		int s1 = start[i];
		int e1 = end[i];
		for (int j = 0; j < count; j++)
		{
			int s2 = start[j];
			int e2 = end[j];

			if (i == j)
				continue;

			if (s2 > e1 || e2 < s1)
				continue; //these data are not alive at the same time

			igraph[i]->AddLink(j);
			//really could do the reciprocal link, but this is trivial to do
		}
	}

	//color in the graph
	available = new bool[count];
	memset(available, false, count);

	//!!!TESTME!!!
	igraph[0]->color =  0;
	for (int i = 1; i < count; i++)
	{
		inode_c*	n = igraph[i];
		int			k;
		link_cnt = n->LinkCnt();
		
		//flag colors used by adjacent vertices as unavailable
		for (int j = 0; j < link_cnt; j++)
		{
			inode_c* l = igraph[n->Link(j)];
			if (l->color != -1)
				available[l->color] = true;
		}

		//find first available color - assign it
		for (k = 0; k < count; k++)
		{
			if (!available[k])
				break;
		}

		n->color = k;

		//clean up available vertices
		for (int j = 0; j < link_cnt; j++)
		{
			inode_c* l = igraph[n->Link(j)];
			if (l->color != -1)
				available[l->color] = false;
		}

		//color - lowest numbered color not used on any previously colored adjacent vertices
		//If all previously used colors appear on adjacent vertices, assigne a new color
	}

	delete[] available;

	//Do this for all child blocks
	for (int i = 0; i < links.size(); i++)
		links[i]->BuildIGraph();
}




cfg_c::~cfg_c()
{
	//FIXME: this needs to get reworked for loops

	for (std::vector<cfg_c*>::iterator it = links.begin(); it != this->links.end(); it++)
		delete* it;

	links.clear();//Hopefully clear the list...
	statements.clear();
	data.clear();
	start.clear();
	end.clear();
}

char g_ctabstr[DEPTH_MAX * 2];
int	 g_ctabs;

void cfg_c::Disp(bool igraph_disp)
{
	memset(g_ctabstr, '\0', DEPTH_MAX * 2 * sizeof(char));
	g_ctabs = 0;

	R_Disp(igraph_disp);
}


void cfg_c::R_Disp(bool igraph_disp)
{
	std::vector<cfg_c*>::iterator	it = links.begin();
	std::vector<tree_c*>::iterator	s_it = statements.begin();
	int link_cnt;

	if (g_ctabs > (DEPTH_MAX * 2) - 2)
		Error("Parser: Attempted to display too many recursive tree nodes!\n");

	printf("%s[[%s]]", g_ctabstr, id);

	if (data.size())
		printf("\n%s", g_ctabstr);

	for (int i = 0; i < data.size(); i++)
		printf(":%s %i-%i ", data[i]->var->Hash()->K(), start[i], end[i]);
	printf("\n");

	if (igraph_disp)
	{
		//num_nodes = sizeof data
		for (int i = 0; i < igraph.num_nodes; i++)
		{
			inode_c*	node = &igraph.nodes[i];
			data_t*		d1 = data[i];

			printf("%s:%s\t%i - ", g_ctabstr, d1->var->Hash()->K(), node->color);

			link_cnt = node->LinkCnt();
			for (int j = 0; j < link_cnt; j++)
			{
				int		l = node->Link(j);
				data_t* d2 = data[l];

				printf("%s ", d2->var->Hash()->K());

			}
			printf("\n");
		}
	}


	for (; s_it != this->statements.end(); s_it++)
		printf("%s%s\n", g_ctabstr, (*s_it)->Hash()->K());

	g_ctabstr[g_ctabs++] = ' ';
	g_ctabstr[g_ctabs++] = ' ';

	for (; it != this->links.end(); it++)
		(*it)->R_Disp(igraph_disp);

	g_ctabstr[--g_ctabs] = '\0';
	g_ctabstr[--g_ctabs] = '\0';
}