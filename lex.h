/***************************************************************************************************
Purpose: Tokenize the input program
Input: The program text
Output: A linked list of tokens representing the program. All tokens will retain the text used to 
create them
***************************************************************************************************/
#pragma once
#include "common.h"
#include "llist.h"

#define OLD_LEX_CODE 1

class scanner_c
{
private:

	llist_c*	list;
	bool		debug;
	int			line;
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


	void AddReservedChar(char c, CODE code);
	void AddLexeme(char* lexeme);

	//Generates a text lump surrounded by spaces and not containing spaces
	//Returns the index directly following the lump
	size_t GetTextLump(char text[KEY_MAX_LEN], size_t pi);

	// Check if text, in its entirety, is a reserved word
	const char* CheckReservedWord(const char* text, size_t prog_index);

	// Check if the first char of text is a reserved character
	const char* CheckReservedChar(const char* text, size_t prog_index);

	// Check if text starts off with a proper piece of text. Eats the longest string possible
	const char* CheckText	(const char* text, size_t prog_index);

	// Check if text is a binary number - at least '%' and a bit
	const char* CheckBinNum	(const char* text, size_t prog_index);

	// Check if text is a hex number - at least '$' and a hexit
	const char* CheckHexNum	(const char* text, size_t prog_index);

	// Check if text is a dec number - at least one digit
	const char* CheckDecNum	(const char* text, size_t prog_index);

	// Check if text is a fixed number - at least a digit, '.', and a digit
	const char* CheckFxdNum	(const char* text, size_t prog_index);

	//FIXME: fold fixed num into dec; do the same for bin and hex to allow for bin and hex fixeds

	// Check if text is a quote-enclosed string
	const char* CheckStr	(const char* text, size_t prog_index);

	//Comment TODO
	// '//' style comments
	// '||', '&&', '..', etc
	// '\'
	// Macros/defines

	// Put a lexeme at the end of the list
	void InsertLexeme(const char* text, size_t len, CODE code, size_t character_index);
	size_t CalculateLineNo(size_t character_index);
	size_t CalculateColNo(const char* text, size_t line_no);

public:
	void Lex(const char* prog, llist_c* list, bool _debug);
};