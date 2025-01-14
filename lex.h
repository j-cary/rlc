#pragma once
#include <functional>
#include "common.h"

class scanner_c
{
private:

	llist_c* list;
	bool debug;
	int line;
	void AddReservedChar(char c, int code);
	void AddLexeme(char* lexeme);
	int isspaceEX(int _c);
public:
	void Lex(const char* prog, llist_c* list, bool _debug);
};