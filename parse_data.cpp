#include "parse.h"

GF_DEF(DATA_TYPE)
{// 'db' | 'dw' | 'fxd' | 'ptr'
	kv_c kv;

	if (GETCP(CODE_BYTE) || GETCP(CODE_WORD) || GETCP(CODE_FIXED) || GETCP(CODE_PTR))
	{
		list->Pop(&kv);
		parent->InsR("Data", NT_DATA_TYPE)->InsR(&kv);
		return true;
	}

	return false;
}

GF_DEF(ARRAY_DATA_TYPE)
{//'dfa' | 'dba' | 'dwa' | 'dpa' | 'fixedarray' | 'bytearray' | 'wordarray' | 'ptrarray'
	kv_c kv;

	if (GETCP(CODE_BYTEARRAY) || GETCP(CODE_WORDARRAY) || GETCP(CODE_FXDARRAY) || GETCP(CODE_PTRARRAY))
	{
		list->Pop(&kv);
		parent->InsR("Data", NT_DATA_TYPE)->InsR(&kv);
		return true;
	}

	return false;
}