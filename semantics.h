#pragma once
#include "common.h"

#define REG_BAD		(-1)
#define REG_A		0
#define REG_B		1
#define REG_C		2
#define REG_D		3
#define REG_E		4
#define REG_H		5
#define REG_L		6
#define REG_R0		7
#define REG_R255	(REG_R0 + 255) //Maximum of 256 spill regs. Only allocate space for the maximum number of used regs
#define REG_IYH		(REG_R255 + 1)//unused
#define REG_IYL		(REG_R255 + 2)//unused
#define REG_IXH		(REG_R255 + 3)
#define REG_IXL		(REG_R255 + 4) 

#define REG_BC		(REG_B + REG_IXL)
#define REG_DE		(REG_D + REG_IXL)
#define REG_HL		(REG_H + REG_IXL)
#define REG_WR0		(REG_R0 + REG_IXL) //r0 and r1
#define REG_WR127	(REG_R255 - 1 + REG_IXL) //r254 and r255
#define REG_IY		(REG_IYH + REG_IXL)//unused
#define REG_IX		(REG_IXH + REG_IXL)//only used for struct/array indexing

#define REGS_TOTAL	(REG_IXL + 2 + ((REG_IX - REG_BC) / 2)) //400

//0-6 : a-l
//7-262 : r0-r255
//263-266 : iyh-ixl
//267 : bc
//269 : de
//271 : hl
//273 : wr0
//275 : wr1
//...
//527 : wr127
//529 : iy
//531 : ix

//New register system description:
//Hardware regs: a, b, c, d, e, f, h, l, af, bc, de, hl, ixh, ixl, iyh, iyl, ix, iy, sp, i, r - 21 total
//Stack sregs: 8 bit signed offset from (ix). i.e., 128 bytes max offset.
//Static sregs: theoretically viable across the entire address space - 65536 total

typedef struct registerinfo_s
{
	unsigned int reg : 5; 
	unsigned int stack : 8; //store the full signed index- -128:127
	unsigned int local : 16; //static- 0:65535 -- CHECKME: have to be careful towards either end of the address space
	unsigned int padding : 3;
} registerinfo_t;

#define SYMBOLS_MAX	32
#define STRUCTURES_MAX	32

#define DF_NONE		0x0
#define DF_BYTE		0x1
#define DF_WORD		0x2
#define DF_LABEL	0x4
#define DF_FXD		0x8
#define DF_PTR		0x10
#define DF_SIGNED	0x20
#define DF_STRUCT	0x40
#define DF_ARRAY	0x80

#define DF_USED		0x200 //set once the data is used as an input to an instruction. Before this is set, the data may have its start moved around
#define DF_FORCTRL	0x400 //control variable for a for loop - try to store this in 'b'
#define DF_GLOBAL	0x800 //visible from absolutely everywhere in the program

typedef unsigned dataflags_t;

//for type checking
typedef int regi_t; //index into generator_c's regs array [1 - a lot] 0 is unused
typedef int paralleli_t; //index into the parallel arrays in cfg_c
typedef int tdatai_t; //tdata/igrpah index

typedef struct data_s
{
	const kv_c*	var;
	int			val;
	void*		block; //FIXME: unnecessary
	dataflags_t	flags;
	tdatai_t	tdata; //link to respective tdata
} data_t;

typedef struct member_s
{
	const char* name;
	const char* struct_name; //NULL unless this is a struct 
	dataflags_t flags;
	int			length;
	int			offset; //redundant
	tree_c*		val; //node so that arrays can be initialized
	member_s*	next = NULL;
} member_t;

typedef struct struct_s
{
	const char* name;
	int			length;
	member_t*	first_member;
} struct_t;

class structlist_c
{
private:
	struct_t structs[STRUCTURES_MAX];
	int struct_cnt = 0;
public:
	int AddStruct(const char* name); //returns index of the new struct
	void AddMemberVar(int struct_idx, const char* name, dataflags_t flags, int length, tree_c* init_val, const char* structname);

	int GetStruct(const char* name); // < 0 if invalid
	int StructLen(int struct_idx) { return structs[struct_idx].length; }
	void SetLen(int struct_idx, int len) { structs[struct_idx].length = len; }

	const struct_t* StructInfo(int idx);

	~structlist_c(); //delete all member vars
};


//purpose: simplify the parse tree 
//	check for symbol predefinition
//	generate CFG
//	determine lifetime of every variable

//Interference Graph

#define LINKS_MAX	32

class inode_c
{
private:
	short	num_links;
	int		links[LINKS_MAX];
public:
	regi_t		color;

	bool AddLink(int l); //false if out of space
	inline int LinkCnt() { return num_links; }
	inline int Link(int i) { return links[i]; }

	const char* ToStr()
	{
		const size_t maxstrlen = 6;
		switch (color)
		{
		case REG_BAD:	return "BAD  ";
		case REG_A:		return "a    ";
		case REG_B:		return "b    ";
		case REG_C:		return "c    ";
		case REG_D:		return "d    ";
		case REG_E:		return "e    ";
		case REG_H:		return "h    ";
		case REG_L:		return "l    ";
		case REG_BC:	return "bc   ";
		case REG_DE:	return "de   ";
		case REG_HL:	return "hl   ";
		case REG_IXH:	return "ixh  ";
		case REG_IXL:	return "ixl  ";
		case REG_IYH:	return "iyh  ";
		case REG_IYL:	return "iyl  ";
		}

		if (color >= REG_R0 && color <= REG_R255)
		{
			static char r[maxstrlen];
			size_t len;

			snprintf(r, maxstrlen, "%c%i", 'r', color - REG_R0); //snprintf just in case
			len = strlen(r);

			for (size_t i = len; i < maxstrlen - 1; i++)
				r[i] = ' ';
			return r;
		}

		if (color >= REG_WR0 && color <= REG_WR127)
		{
			static char wr[maxstrlen];
			size_t len;

			snprintf(wr, 6, "%s%i", "wr", (color - REG_WR0) / 2);
			len = strlen(wr);

			for (size_t i = len; i < maxstrlen - 1; i++)
				wr[i] = ' ';
			return wr;
		}

		return "REALLYBAD";
	}

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
typedef struct tdata_s
{
	const kv_c* var;
	int			start, end;
	int			startb, endb;
	dataflags_t	flags;
} tdata_t;

//Control flow graph
class cfg_c
{
private:
	std::vector<tree_c*>	statements; //this will be clear()'d on deletion, but the actual nodes will be deleted by the parse tree
	std::vector<cfg_c*>		links;
	std::vector<struct_t>	structs; //only ROOT really needs this

	//parallel
	std::vector<data_t*>	data;
	std::vector<int>		start; //statement number in the start block. this block is ALWAYS the block in which the data is declared
	std::vector<int>		end; //statement number in the end block
	std::vector<cfg_c*>		startb;
	std::vector<cfg_c*>		endb;
	std::vector<int>		uses; 

	int TrimVars(cfg_c* parent, int count );

	//igraph building
	void R_TotalLinks();
	void R_GenBlockOfs(cfg_c** offsets);
	void R_BuildTDataList(tdata_t* tdata, cfg_c** offsets);
	bool R_SwapTDataIndices(tdatai_t old, tdatai_t _new);
	void SortTDataList(tdata_t** tdata, int count);
	int	 FirstValidColor(unsigned flags);
	int	 Iteratend(unsigned flags);

	int R_CheckGlobalRedef();
	int R_CheckRedef();
	int R_GetScopedData();
	int R_IsStructInstance(const char* name);

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
	void IncDataUses(const char* name);


	
	void BuildIGraph(int symbol_cnt, igraph_c* igraph, tdata_t** tdata);

	bool CheckRedef(const char* name, cfg_c* top, cfg_c* root, dataflags_t flags);
	data_t* ScopedDataEntry(const char* name, cfg_c* top, cfg_c* root, cfg_c** localblock);
	bool IsStructInstance(const char* name, cfg_c* func, cfg_c* root); 

	void Disp(bool igraph_disp, igraph_c* igraph, tdata_t* tdata);
	~cfg_c();
};

#define BLOCKSTACK_MAX	128

class analyzer_c
{
private:
	tree_c* root;
	cfg_c*	graph;
	cfg_c*	cur_func;
	data_t*	symtbl;
	unsigned symtbl_top;

	igraph_c* igraph;
	tdata_t* tdata;

	structlist_c* slist;

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
	void CFG_TypeDef(tree_c* node, cfg_c* block);

	void CFG_Node(tree_c* node, cfg_c* link, cfg_c* ancestor, int start, int exit_code);

	void BuildIGraphs(cfg_c* block);

	data_t* DataEntry(tree_c* d, cfg_c* block, cfg_c** localblock);
	void MakeDataEntry(const kv_c* _var, cfg_c* block, unsigned _flags);
	int MakeStructEntry(const kv_c* _var, cfg_c* _block); //returns new struct index
	//void MakeFunctionEntry(const kv_c* var);

public:
	void GenerateAST(tree_c* _root, cfg_c* _graph, data_t* symbols, unsigned* symbols_top, tdata_t** _tdata, igraph_c* igraph, structlist_c* sl);
};

//generator_util
int Constant_Expression(tree_c* head);