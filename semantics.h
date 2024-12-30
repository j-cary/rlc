#pragma once
#include "common.h"

#define SYMBOLS_MAX	16

//0x4-0x100 are not used by registers
#define DF_NONE	0x0
#define DF_BYTE	0x1
#define DF_WORD	0x2
#define DF_A	0x4
#define DF_B	0x8
#define DF_C	0x10
#define DF_D	0x20
#define DF_E	0x40
#define DF_H	0x80
#define DF_L	0x100
#define DF_USED	0x200 //set once the data is used as an input to an instruction. Before this is set, the data may have its start moved around

#define DF_REGMASK (DF_A | DF_B | DF_C | DF_D | DF_E | DF_H | DF_L)

typedef struct data_s
{
	tree_c*		var; //FIXME: make this a string or kv
	int			val;
	tree_c*		func; //FIXME: make this a graph node
	//cfg_c*	block;
	unsigned	flags;
} data_t;

enum GRAPH_COLORS
{
	COLOR0 = 0, COLOR1 = 1
};

//purpose: simplify the parse tree 
//	check for symbol predefinition
//	generate CFG
//	determine lifetime of every variable

//Interference Graph - one of these per block

#define LINKS_MAX	32

class inode_c
{
private:
	short	num_links;
	int		links[LINKS_MAX];
public:
	int		color;

	bool AddLink(int l); //false if out of space
	inline int LinkCnt() { return num_links; }
	inline int Link(int i) { return links[i]; }

	inode_c()
	{
		for (int i = 0; i < LINKS_MAX; i++)
			links[i] = -1;

		num_links = 0;
		color = -1;
	}
};

//the index of each node is also the index into the cfg's parallel arrays
class igraph_c
{
private:
public:
	inode_c*	nodes;
	int			num_nodes;

	void Clear();
	void Disp();

	inline inode_c* operator[](int index)
	{
		return &nodes[index];
	}

	igraph_c() { nodes = NULL; num_nodes = 0; }
	~igraph_c() { Clear(); }

};


enum BLOCK_TYPE
{
	BLOCK_ROOT = -2, BLOCK_FUNC, BLOCK_REG = 0, BLOCK_ENTRY, BLOCK_EXIT, BLOCK_COND, BLOCK_ELSE, BLOCK_ELSEIF, 
};

//Control flow graph
class cfg_c
{
private:
	std::vector<tree_c*>	statements; //this will be clear()'d on deletion, but the actual nodes will be deleted by the parse tree
	std::vector<cfg_c*>		links;

	//parallel
	std::vector<data_t*>	data;
	std::vector<int>		start; //used for tracking usage/lifetime
	std::vector<int>		end;
	igraph_c				igraph;
	//std::vector<unsigned>	flags; //used for lifetime determination


	char					id[32];
	BLOCK_TYPE				block_type;

	void R_Disp(bool igraph_disp);
public:

	void Set(const char* _id, BLOCK_TYPE _Type);

	void AddStmt(tree_c* s);
	cfg_c* AddLink(const char* _id, BLOCK_TYPE _type);
	cfg_c* AddLink(const char* _id, BLOCK_TYPE _type, cfg_c* sibling); //copy data from sibling instead of from the caller

	tree_c* GetStmt(int ofs);
	cfg_c* GetLink(int ofs);
	int StmtCnt() { return (int)statements.size(); }

	//data
	void AddData(data_t* _d);
	void SetDataStart(const char* name, int _start);
	void SetDataEnd(const char* name, int _end);
	
	void BuildIGraph();

	void Disp(bool igraph_disp);
	~cfg_c();
};

class semantic_c
{
private:
	tree_c* root;
	cfg_c*	graph;
	cfg_c*	cur_func;
	cfg_c*	cur_link;
	data_t*	symtbl;
	unsigned symtbl_top;

	void SimplifyTree(tree_c* node, tree_c* parent); //Pass 1

	void CFG_Start(tree_c* node); //Pass 2
	void CFG_FuncDef(tree_c* node);
	void CFG_Parms(tree_c* node);
	cfg_c* CFG_OpenStatement(tree_c* node, cfg_c* parent, cfg_c* ancestor); //todo: merge these two
	cfg_c* CFG_ClosedStatement(tree_c* node, cfg_c* parent, cfg_c* ancestor); //todo: merge these two
	void CFG_DataDeclaration(tree_c* node, cfg_c* block);
	void CFG_Instruction(tree_c* node, cfg_c* block);

	void CFG_Node(tree_c* node, cfg_c* link, cfg_c* ancestor, int start, int exit_code);

	void BuildIGraphs(cfg_c* block);

	data_t* DataEntry(tree_c* d, cfg_c* block);
public:
	void GenerateAST(tree_c* _root, cfg_c* _graph, data_t* symbols, unsigned* symbols_top);
};