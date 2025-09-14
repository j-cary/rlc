/***************************************************************************************************
Purpose: Maintain a collection of user-defined structures
***************************************************************************************************/
#pragma once
#include "common.h"
#include "semantics.h"

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

// Flag type to be used with the DF_* defines
typedef unsigned dataflags_t;

typedef struct member_s
{
	const char* name;
	const char* struct_name; //NULL unless this is a struct 
	dataflags_t flags;
	int			length;
	int			offset; //Technically redundant
	tree_c*		val; //node so that arrays can be initialized
	member_s*	next = NULL;
} member_t;

typedef struct struct_s
{
	const char* name;
	int			length;
	member_t*	first_member;

	const member_t* GetMemberInfo(const char* name) const
	{
		for (const member_t* m = first_member; m; m = m->next)
			if (!strcmp(name, m->name))
				return m;
		return NULL;
	}

} struct_t;

class structlist_c
{
private:
	struct_t	structs[STRUCTURES_MAX];
	int			struct_cnt = 0;
public:
	int AddStruct(const char* name); //returns index of the new struct
	void AddMemberVar(int struct_idx, const char* name, dataflags_t flags, int length, tree_c* init_val, const char* structname);

	int GetStruct(const char* name) const; // < 0 if invalid
	int StructLen(int struct_idx) const { return structs[struct_idx].length; }
	void SetLen(int struct_idx, int len) { structs[struct_idx].length = len; }

	const struct_t* StructInfo(int idx) const;
	const struct_t* StructInfo(const char* struct_name) const;

	~structlist_c(); //delete all member vars
};
