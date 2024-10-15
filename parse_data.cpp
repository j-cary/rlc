#include "parse.h"

GF_DEF(DATA_TYPE)
{
	kv_c kv;

	if (GETCP(CODE_BYTE) || GETCP(CODE_WORD) || GETCP(CODE_FIXED) || GETCP(CODE_PTR))
	{
		if (!advance)
			return true;

		list->Pop(&kv);
		if (parent)
			parent->InsR("Data", NT_DATA_TYPE)->InsR(&kv);
		return true;
	}

	return false;
}

GF_DEF(ARRAY_DATA_TYPE)
{
	return false;
}