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
	const kv_c*	var;
	int			val;
	void*		block; //FIXME: pointer to cfg_c*
	unsigned	flags;
} data_t;

//for type checking
typedef int colori_t; //register coloring. [0 - a lot] -1 is reserved for constants, std vars, unused variables
typedef int regi_t; //index into generator_c's regs array [1 - a lot] 0 is unused
typedef int paralleli_t; //index into the parallel arrays in cfg_c as well as the igraph member [0 - inf] -1 for unused vars

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

	inline inode_c* operator[](paralleli_t index)
	{
		return &nodes[index];
	}

	igraph_c() { nodes = NULL; num_nodes = 0; }
	~igraph_c() { Clear(); }

};


enum BLOCK_TYPE
{
	BLOCK_ROOT = -2, BLOCK_FUNC, BLOCK_REG = 0, BLOCK_ENTRY, BLOCK_EXIT, BLOCK_COND, BLOCK_ELSE, BLOCK_ELSEIF, BLOCK_FOR, BLOCK_WHILE, BLOCK_LOOPBACK
};

//temporary convenience struct for global register allocation
typedef struct
{
	const kv_c* var;
	int		start, end;
	int		startb, endb;
} tdata_t;

//Control flow graph
class cfg_c
{
private:
	

	std::vector<tree_c*>	statements; //this will be clear()'d on deletion, but the actual nodes will be deleted by the parse tree
	std::vector<cfg_c*>		links;

	//parallel
	std::vector<data_t*>	data;
	std::vector<int>		start; //statement number in the start block. this block is ALWAYS the block in which the data is declared
	std::vector<int>		end; //statement number in the end block
	std::vector<cfg_c*>		startb;
	std::vector<cfg_c*>		endb;
	//igraph_c				igraph; //should only need one of these now.
	//tdata_t*				tdata;

	//todo: pass igraph from analyzer to generator. use symbol table instead of cfg data array. 
	// Note: all scoping should be checked in the analyzer
	// what should be done about potential index mismatch between tdata and symtbl?
	//dataname - move to generator. regular int parm. 
	//datacolor - move to generator. go through symbol table
	//dataofs - remove cfg* parm, change return to int, search symbol table

	int TrimVars(cfg_c* parent, int count );
	bool IsUsedInTree( const char* dataname);

	void R_TotalLinks();
	void R_GenBlockOfs(cfg_c** offsets);
	void R_BuildTDataList(tdata_t* tdata, cfg_c** offsets);

	void R_Disp( igraph_c* igraph, tdata_t* tdata);
public:
	BLOCK_TYPE				block_type;
	char					id[32];

	void Set(const char* _id, BLOCK_TYPE _Type);

	void AddStmt(tree_c* s);
	cfg_c* AddLink(const char* _id, BLOCK_TYPE _type);
	cfg_c* AddLink(const char* _id, BLOCK_TYPE _type, cfg_c* sibling); //copy data from sibling instead of from the caller
	cfg_c* AddLink(const char* _id, BLOCK_TYPE _type, data_t* init); //add the control variable from loops

	tree_c* GetStmt(int ofs);
	cfg_c* GetLink(int ofs);
	int StmtCnt() { return (int)statements.size(); }

	//data
	void AddData(data_t* _d);
	bool SetDataStart(const char* name, int _start); //returns true if the data was local to this block
	bool SetDataEnd(const char* name, int _end); //ditto above
	void SetDataEndBlock(const char* name, cfg_c* block);
	colori_t DataColor(const char* name);
	const char* DataName(paralleli_t ofs);
	
	void BuildIGraph(int symbol_cnt, igraph_c* igraph, tdata_t** tdata);

	void Disp(bool igraph_disp, igraph_c* igraph, tdata_t* tdata);
	~cfg_c();
};

class analyzer_c
{
private:
	tree_c* root;
	cfg_c*	graph;
	cfg_c*	cur_func;
	cfg_c*	cur_link;
	data_t*	symtbl;
	unsigned symtbl_top;

	igraph_c igraph;
	tdata_t* tdata;

	void SimplifyTree(tree_c* node, tree_c* parent); //Pass 1

	void CFG_Start(tree_c* node); //Pass 2
	void CFG_DataDecl(tree_c* node);
	void CFG_FuncDef(tree_c* node);
	void CFG_Parms(tree_c* node);
	cfg_c* CFG_Statement(tree_c* node, cfg_c* parent, cfg_c* ancestor);
	cfg_c* CFG_OpenStatement(tree_c* node, cfg_c* parent, cfg_c* ancestor); //todo: merge these two
	cfg_c* CFG_ClosedStatement(tree_c* node, cfg_c* parent, cfg_c* ancestor); //todo: merge these two
	void CFG_DataDeclaration(tree_c* node, cfg_c* block);
	void CFG_Instruction(tree_c* node, cfg_c* block);

	void CFG_Node(tree_c* node, cfg_c* link, cfg_c* ancestor, int start, int exit_code);

	void BuildIGraphs(cfg_c* block);

	data_t* DataEntry(tree_c* d, cfg_c* block, cfg_c** localblock);
public:
	void GenerateAST(tree_c* _root, cfg_c* _graph, data_t* symbols, unsigned* symbols_top);
};