#pragma once
#include <functional>
#include "common.h"

class lex_c
{
private:

	llist_c* list;
	bool debug;
	void AddReservedChar(char c, int code);
	void AddLexeme(char* lexeme);
public:
	void Lex(const char* line, llist_c* list, bool _debug);
};