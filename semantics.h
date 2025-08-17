#pragma once
#include "common.h"

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

	//Reg to string. Specify size in bytes.
	//Set bad_check if checks for validity are desired. Also pads the string
	const char* Str(REG reg, int size);
}
#define SI_REG_COUNT	((size_t)REG::_SIZE)
#define SI_LAST_GENERAL	(REG::L) //the last general purpose register
#define SI_STACK_MIN	((signed char)(-128))
#define SI_STACK_MAX	((signed char)(+127))
#define SI_STACK_COUNT	SI_STACK_MAX //1-127
#define SI_LOCAL_MIN	((unsigned short)(1))
#define SI_LOCAL_MAX	((unsigned short)(65535))
#define SI_LOCAL_COUNT	((size_t)(65536))

typedef union storageinfo_u
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
} storageinfo_t;

#define SYMBOLS_MAX	32
#define STRUCTURES_MAX	32

#define DF_NONE		0x0000
#define DF_BYTE		0x0001
#define DF_WORD		0x0002
#define DF_LABEL	0x0004
#define DF_FXD		0x0008
#define DF_PTR		0x0010
#define DF_SIGNED	0x0020
#define DF_STRUCT	0x0040
#define DF_ARRAY	0x0080
#define DF_STATIC	0x0100 //mutually exclusive with auto & stack
#define DF_STACK	0x0200 //if neither stack or static are set, it's an auto
#define DF_USED		0x0400 //set once the data is used as an input to an instruction. Before this is set, the data may have its start moved around
#define DF_FORCTRL	0x0800 //control variable for a for loop - try to store this in 'b'
#define DF_GLOBAL	0x1000 //visible from absolutely everywhere in the program

#define DF_OTHER_MASK (DF_LABEL | DF_FXD | DF_STRUCT | DF_ARRAY)

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
	int			size;
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
	//regi_t		color; //removeme
	//storageinfo_t si;

	bool AddLink(int l); //false if out of space
	inline int LinkCnt() { return num_links; }
	inline int Link(int i) { return links[i]; }

	inode_c()
	{
		for (int i = 0; i < LINKS_MAX; i++)
			links[i] = -1;

		num_links = 0;
		//color = -1;
		//si.data = 0;
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
	int			size;
	dataflags_t	flags;
	storageinfo_t si{};

	const char* ToStr(int size)
	{
		const size_t maxstrlen = 6;
		static char str[maxstrlen];

		if (si.reg_flag)
		{
			return REG::Str((REG::REG)si.reg, size);
		}
		else if (si.stack_flag)
		{
			signed char actual = si.stack - SI_LAST_GENERAL;

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
	void FixupStaticInterference(tdata_t** tdata);
	//int	 FirstValidColor(unsigned flags);
	//int	 Iteratend(unsigned flags);
	int	StackIndex(tdata_t* var, bool available[], const int size);
	int AutoIndex(tdata_t* var, bool stack_available[], const int stack_size, bool local_available[], const int local_size, bool* local);
	unsigned short DataSize() { return 1U; } //FIXME:
	void ColorGraph(int symbol_count, igraph_c* graph, tdata_t* tdata);

	int R_CheckGlobalRedef();
	int R_CheckRedef();
	int R_GetScopedData();
	int R_IsStructInstance(const char* name);

	void R_Disp( igraph_c* igraph, tdata_t* tdata);
public:
	BLOCK_TYPE				block_type;
	char					id[KEY_MAX_LEN];

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

	void CFG_Node(tree_c* node, cfg_c* link, cfg_c* ancestor, int start, int exit_code);

	//Parms:
	CODES EvaluateDataModifiers(tree_c* node, bool struct_def, int* iterator, const char** structname, dataflags_t* flags);
	int EvaluateFirstDataSize(tree_c* node, tree_c* struct_, int* iterator, tree_c** data_name, const char** structname, dataflags_t* flags);

	void BuildIGraphs(cfg_c* block);

	data_t* GetDataEntry(tree_c* d, cfg_c* block, cfg_c** localblock);
	void MakeDataEntry(const kv_c* _var, cfg_c* block, int size, unsigned _flags);
	int MakeStructEntry(const kv_c* _var, cfg_c* _block); //returns new struct index
	//void MakeFunctionEntry(const kv_c* var);

public:
	void GenerateAST(tree_c* _root, cfg_c* _graph, data_t* symbols, unsigned* symbols_top, tdata_t** _tdata, structlist_c* sl);
};

//generator_util
int Constant_Expression(tree_c* head);