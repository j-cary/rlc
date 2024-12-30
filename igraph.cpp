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