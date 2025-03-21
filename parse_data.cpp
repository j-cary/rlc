#include "parse.h"

GF_DEF(DATA_TYPE)
{// 'byte' | 'word' | 'dword' | 'fixed' | 'dfixed'
	kv_c kv;
	tree_c* self;
	node_c* saved = list->Save();

	if (GETCP(CODE_BYTE) || GETCP(CODE_WORD) || GETCP(CODE_FIXED))
	{
		list->Pop(&kv);
		self = parent->InsR("Data", NT_DATA_TYPE);
		self->InsR(&kv);

		return true;
	}

	list->Restore(saved);
	return false;
}