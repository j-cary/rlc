#include "semantics.h"

int structlist_c::AddStruct(const char* name)
{
	struct_t* s = &structs[struct_cnt];

	//struct names aren't stored in the data list so we have to check for redefs here, too
	for (int i = 0; i < struct_cnt; i++)
	{
		if (!strcmp(structs[i].name, name))
			Error("Struct '%s' is already defined", name);
	}

	s->name = name;
	s->length = 0;
	s->first_member = NULL;

	return struct_cnt++; //check me
}

void structlist_c::AddMemberVar(int struct_idx, const char* name, dataflags_t flags, int length, tree_c* init_val, const char* structname)
{
	struct_t* s = &structs[struct_idx];
	member_t* m;
	int offset = 0;

	if (!(m = s->first_member))
	{
		m = new member_t;
		m->name = name;
		m->flags = flags;
		m->length = length;
		m->val = init_val;
		m->offset = 0;
		m->struct_name = structname;
		s->first_member = m;
		return;
	}

	do
	{
		offset += m->length;

		if (!strcmp(name, m->name))
			Error("Structure '%s' cannot have two members with the name '%s'", s->name, name);

		if (!m->next)
		{//found the last one
			m->next = new member_t;
			m = m->next;
			m->name = name;
			m->flags = flags;
			m->length = length;
			m->val = init_val;
			m->offset = offset;
			m->struct_name = structname;
			return;
		}
		m = m->next;
	} while (1);
}

int structlist_c::GetStruct(const char* name) const
{
	for (int i = 0; i < struct_cnt; i++)
	{
		if (!strcmp(structs[i].name, name))
			return i;
	}

	return -1;
}

const struct_t* structlist_c::StructInfo(int idx)
{
	if (idx < 0 || idx >= struct_cnt)
		return NULL;
	return &structs[idx];
}

structlist_c::~structlist_c()
{//delete all member vars
	for (int i = 0; i < struct_cnt; i++)
	{
		struct_t* s = &structs[i];
		member_t* m, * del;

		if (s->first_member) //this should only be false if there was some semantic error while describing the struct
		{
			del = s->first_member;
			m = del->next;
			delete del;

			while (m)
			{
				del = m;
				m = m->next;
				delete del;
			}
		}
	}
}