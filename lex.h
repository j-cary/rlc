#pragma once
#include <functional>
#include "common.h"

#define OLD_LEX_CODE 1

class scanner_c
{
private:

	llist_c* list;
	bool debug;
	int line;
	const char* prog;
	std::vector<size_t> newlines; //Newline indices

	static constexpr int check_count = 8;

	const char* (scanner_c::*checks[check_count])(const char*, size_t) =
	{
		&scanner_c::CheckReservedChar,
		&scanner_c::CheckReservedWord, //Must come before text
		&scanner_c::CheckText,
		&scanner_c::CheckFxdNum, //Must come before dec#
		&scanner_c::CheckBinNum,
		&scanner_c::CheckHexNum,
		&scanner_c::CheckDecNum,
		&scanner_c::CheckStr,
	};


	void AddReservedChar(char c, int code);
	void AddLexeme(char* lexeme);

	//Generates a text lump surrounded by spaces and not containing spaces
	//Returns the index directly following the lump
	size_t GetTextLump(char text[KEY_MAX_LEN], size_t pi);

	const char* CheckReservedWord(const char* text, size_t prog_index);
	const char* CheckReservedChar(const char* text, size_t prog_index);
	const char* CheckText(const char* text, size_t prog_index);
	const char* CheckBinNum(const char* text, size_t prog_index);
	const char* CheckHexNum(const char* text, size_t prog_index);
	const char* CheckDecNum(const char* text, size_t prog_index);
	const char* CheckFxdNum(const char* text, size_t prog_index);
	const char* CheckStr(const char* text, size_t prog_index);
	//Comment
	// '//' style comments
	// '||', '&&', '..', etc
	// '\'
	// Macros/defines

	void InsertLexeme(const char* text, size_t len, CODES code, size_t character_index);
	size_t CalculateLineNo(size_t character_index);
	size_t CalculateColNo(const char* text, size_t line_no);

public:
	void Lex(const char* prog, llist_c* list, bool _debug);
};