#pragma once
#include "common.h"
#include "structs.h"

//New register system description:
//Hardware regs: a, b, c, d, e, h, l, af, bc, de, hl, ixh, ixl, iyh, iyl, ix, iy, sp, i, r - 20 total
//Stack sregs: 8 bit signed offset from (ix). i.e., 128 bytes max offset.
//Static sregs: theoretically viable across the entire address space - 65536 total

#define OLD_REG_CODE 0

namespace REG
{
	enum REG : unsigned char //really only 5 bits
	{
		A = 0, B, C, D, E, H, L,
		//AF, BC, DE, HL, 
		IXH, IXL, IYH, IYL, 
		//IX, IY,
		SP, I, R,
		_SIZE
	};

	//Reg to string. Default to 16-bit
	const char* Str(REG reg);

	const char* Str(REG reg, int size);

	// Defines for the bounds of storage areas
	inline constexpr size_t REG_CNT() { return (size_t)REG::_SIZE; }
	inline constexpr REG LAST_GENERAL_REG() { return REG::L; } //Last General purpose register
	inline constexpr int STACK_MIN() { return -127; }
	inline constexpr int STACK_MAX() { return 128; }
	inline constexpr int STACK_CNT() { return STACK_MAX(); }
	inline constexpr unsigned short AUTO_MIN() { return (unsigned short)1; }
	inline constexpr unsigned short AUTO_MAX() { return (unsigned short)65535; }
	inline constexpr unsigned short AUTO_CNT() { return AUTO_MAX(); }
}

#define SYMBOLS_MAX	32

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
	int			size;
	tdatai_t	tdata; //link to respective tdata
} data_t;

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

	bool AddLink(int l); //false if out of space
	inline int LinkCnt() const { return num_links; }
	inline int Link(int i) const { return links[i]; }

	inode_c()
	{
		for (int i = 0; i < LINKS_MAX; i++)
			links[i] = -1;

		num_links = 0;
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

enum class BLOCK
{
	ROOT = -2, FUNC = -1, REG, ENTRY, EXIT, COND, ELSE, ELSEIF, FOR, WHILE, LOOPBACK
};

//temporary convenience struct for global register allocation
typedef struct tdata_s
{
	const kv_c* var;
	int			start, end;
	int			startb, endb;
	int			size;
	dataflags_t	flags;

	union 
	{
		struct
		{
			//Note: reg&stack are indices into the same array
			unsigned int	reg : 5; //0-21 
			signed int		stack : 8; //store the full signed index- -128:127
			unsigned int	local : 16; //static- 0:65535 -- CHECKME: have to be careful towards either end of the address space
			unsigned int	reg_flag : 1;
			unsigned int	stack_flag : 1;
			unsigned int	local_flag : 1;
		};
		unsigned int data;
	} si{};

	const char* ToStr(int size) const
	{
		const size_t maxstrlen = 6;
		static char str[maxstrlen];

		if (si.reg_flag)
		{
			return REG::Str((REG::REG)si.reg, size);
		}
		else if (si.stack_flag)
		{
			signed char actual = si.stack;

			snprintf(str, maxstrlen, "(%i)", actual);

			for (size_t i = strlen(str); i < maxstrlen - 1; i++)
				str[i] = ' ';
			return str;
		}
		else if (si.local_flag)
		{
			snprintf(str, maxstrlen, "%i", si.local);

			for (size_t i = strlen(str); i < maxstrlen - 1; i++)
				str[i] = ' ';
			return str;
		}

		return "REALLYBAD";
	}
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
	std::vector<int>		uses; 

	int TrimVars(cfg_c* parent, int count );

	//igraph building
	void R_TotalLinks();
	void R_GenBlockOfs(cfg_c** offsets);
	void R_BuildTDataList(tdata_t* tdata, cfg_c** offsets);
	bool R_SwapTDataIndices(tdatai_t old, tdatai_t _new);
	void SortTDataList(tdata_t** tdata, int count);
	void FixupStaticInterference(tdata_t** tdata);

	int	StackIndex(tdata_t* var, bool available[], const int size);
	int AutoIndex(tdata_t* var, bool stack_available[], const int stack_size, bool local_available[], const int local_size, bool* local);
	void ColorGraph(int symbol_count, igraph_c* graph, tdata_t* tdata);

	/* Autos and stacks use offsets in colorgraph; remove them here*/
	void FixupStackIndices(int symbol_cnt, tdata_t* tdata);

	int R_CheckGlobalRedef();
	int R_CheckRedef();
	int R_GetScopedData();
	int R_IsStructInstance(const char* name) const;

	void R_Disp( igraph_c* igraph, tdata_t* tdata);
public:
	BLOCK	block_type;
	char	id[KEY_MAX_LEN];

	void Set(const char* _id, BLOCK _Type);

	void AddStmt(tree_c* s);
	cfg_c* AddLink(const char* _id, BLOCK _type);
	cfg_c* AddLink(const char* _id, BLOCK _type, cfg_c* sibling); //copy data from sibling instead of from the caller
	cfg_c* AddLink(const char* _id, BLOCK _type, data_t* init); //add the control variable from loops

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
	data_t* ScopedDataEntry(const char* name, cfg_c* top, cfg_c* root, cfg_c** localblock) const;
	bool IsStructInstance(const char* name, const cfg_c* func, const cfg_c* root) const;

	void Disp(bool igraph_disp, igraph_c* igraph, tdata_t* tdata);
	~cfg_c();
};

class analyzer_c
{
private:
	tree_c* root;
	cfg_c*	graph;
	cfg_c*	cur_func;
	data_t*	symtbl;
	unsigned symtbl_top;

	igraph_c igraph;
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

	void CFG_Node(tree_c* node, cfg_c* link, cfg_c* ancestor, int start, CODE exit_code);

	//Parms:
	CODE EvaluateDataModifiers(tree_c* node, bool struct_def, int* iterator, const char** structname, dataflags_t* flags);
	int EvaluateFirstDataSize(tree_c* node, tree_c* struct_, int* iterator, tree_c** data_name, const char** structname, dataflags_t* flags);

	void BuildIGraphs(cfg_c* block);

	data_t* GetDataEntry(tree_c* d, cfg_c* block, cfg_c** localblock);
	void MakeDataEntry(const kv_c* _var, cfg_c* block, int size, unsigned _flags);
	int MakeStructEntry(const kv_c* _var, cfg_c* _block); //returns new struct index
	//void MakeFunctionEntry(const kv_c* var);

public:
	void GenerateAST(tree_c* _root, cfg_c* _graph, data_t* symbols, unsigned* symbols_top, tdata_t** _tdata, structlist_c* sl);
};