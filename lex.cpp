#include "lex.h"

ckv_t reservedchars[] =
{
	'(', CODE_LPAREN,
	')', CODE_RPAREN,
	'{', CODE_LBRACKET,
	'}', CODE_RBRACKET,
	'[', CODE_LBRACE,
	']', CODE_RBRACE,

	'@', CODE_AT,
	//'#', CODE_POUND,
	'.', CODE_PERIOD,
	',', CODE_COMMA,
	//'"', CODE_QUOTE_DOUBLE,
	//'\'', CODE_QUOTE_SINGLE,
	';', CODE_SEMICOLON,
	':', CODE_COLON,
	'<', CODE_LARROW,
	'>', CODE_RARROW,
	'!', CODE_EXCLAMATION,
	'&', CODE_AMPERSAND,
	'|', CODE_BAR,

	//arithmetic
	'=', CODE_EQUALS,
	'+', CODE_PLUS,
	'-', CODE_MINUS,
	'*', CODE_STAR,
	'/', CODE_FSLASH,
	//'%', CODE_PERCENT,

	'\0', 0

};

kv_t reservedwords[] =
{
	"inline",	CODE_INLINE,
	"subr",		CODE_SUBR,
	"ans",		CODE_ANS,
	
	//data types
	"db",		CODE_BYTE,		"byte",		CODE_BYTE,
	"dw",		CODE_WORD,		"word",		CODE_WORD,
	"fxd",		CODE_FIXED,		"fixed",	CODE_FIXED,
	"lbl",		CODE_LABEL,		"label",	CODE_LABEL,

	"struct",	CODE_STRUCT,
	"signed",	CODE_SIGNED,
	"static",	CODE_STATIC,	"stack",	CODE_STACK,	"auto", CODE_AUTO,

	//control flow
	"repeat",	CODE_REPEAT,
	"until",	CODE_UNTIL,
	"while",	CODE_WHILE,
	"for",		CODE_FOR,
	"if",		CODE_IF,
	"else",		CODE_ELSE,

	//instructions
	"ld",		CODE_LD,
	"jp",		CODE_JP,
	"call",		CODE_CALL,
	"ret",		CODE_RET,

	"add",		CODE_ADD,
	"sub",		CODE_SUB,
	"mul",		CODE_MUL,
	"div",		CODE_DIV,
	"mod",		CODE_MOD,
	"inc",		CODE_INC,
	"dec",		CODE_DEC,
	"comp",		CODE_COMP,
	"and",		CODE_AND,
	"or",		CODE_OR,
	"xor",		CODE_XOR,
	"neg",		CODE_NEG,

	"rlc",		CODE_RLC,
	"rrc",		CODE_RRC,
	"rl",		CODE_RL,
	"rr",		CODE_RR,
	"sl",		CODE_SL,
	"sr",		CODE_SR,
	"res",		CODE_RES,
	"set",		CODE_SET,
	"flp",		CODE_FLP,

	"in",		CODE_IN,
	"out",		CODE_OUT,
	"im",		CODE_IM,

	"ldm",		CODE_LDM,
	"cpm",		CODE_CPM,
	"inm",		CODE_INM,
	"outm",		CODE_OUTM,

	//preprocessing
	"include",	CODE_PP_INCLUDE,
	"insert",	CODE_PP_INSERT,
	"define",	CODE_PP_DEFINE,
	NULL,		CODE_NONE //null terminator
};

size_t scanner_c::GetTextLump(char text[KEY_MAX_LEN], size_t pi)
{
	//char cur;
	bool hasquote = 0;
	const char* cur = prog + pi;
	const char* const start = prog + pi;

	//Skip any preceding whitespace
	for (; *cur && isspace(*cur); cur++) 
	{
		if (*cur == '\n')
			newlines.push_back(cur - prog);
	}

	//stop when:
	// the program is exhausted OR
	// we're looking at a space NOT found in a string
	for (int ti = 0; (*cur) && (!isspace(*cur) || hasquote); cur++, ti++)
	{
		if (ti >= TEXT_MAX_LEN)
			Error("Text constant too long. Is there an unclosed quote?");

		if (*cur == '\n')
		{
			newlines.push_back(cur - prog);
		}
		else if (*cur == '"')
		{//toggle between accepting spaces in a lexeme.
			hasquote = !hasquote;
		}
		else if (*cur == '#')
		{//skip everything in the comment

			for (cur++; *cur != '#'; cur++)
			{
				if (!*cur)
					Error("Unclosed comment");

				if (*cur == '\n')
					newlines.push_back(cur - prog);
			}
			break;
		}

		text[ti] = *cur;
	}

	return pi + (cur - start);
}

const char* scanner_c::CheckReservedWord(const char* text, size_t prog_index)
{
	for (kv_t* kv = reservedwords; *kv->k; kv++)
	{
		size_t len = strlen(kv->k);
		if (strncmp(text, kv->k, len) == 0)
		{//matched a reserved word
			const char* end = text + len;

			//make sure there ISN'T anymore text after it - ex. sus out 'structname'
			if (end && (isalpha(*end) || *end == '_'))
				break;

			InsertLexeme(kv->k, strlen(kv->k), (CODES)kv->v, prog_index);
			return text + len;
		}
	}

	return text;
}

const char* scanner_c::CheckReservedChar(const char* text, size_t prog_index)
{
	for (ckv_t* kv = reservedchars; kv->k != '\0'; kv++)
	{
		if (*text == kv->k)
		{//matched a reserved char

			InsertLexeme(text, 1, (CODES)kv->v, prog_index);
			return text + 1;
		}
	}

	return text;
}

const char* scanner_c::CheckText(const char* text, size_t prog_index)
{
	const char* cur = text;

	if (!isalpha(*cur) && (*cur != '_')) 
		return text; //Identifiers must start with a-z or '_' 

	for (cur++; *cur; cur++)
	{
		if (!isalpha(*cur) && !isdigit(*cur) && (*cur != '_'))
			break; //Valid ident, more lexemes in this lump
	}

	InsertLexeme(text, cur - text, CODE_TEXT, prog_index);
	return cur; //Valid ident, just encountered a space
}

const char* scanner_c::CheckBinNum(const char* text, size_t prog_index)
{
	const char* cur = text;

	if (*cur != '%')
		return text; //Start with a '%'

	cur++;
	if (*cur != '0' && *cur != '1')
		return text; //Needs at least one number - FIXME: should just '%' be allowed? 

	for (cur++; *cur; cur++)
	{
		if (*cur != '0' && *cur != '1')
			break; //Valid bin#
	}

	InsertLexeme(text, cur - text, CODE_NUM_BIN, prog_index);
	return cur; //Valid bin#, just encountered a space
}

const char* scanner_c::CheckHexNum(const char* text, size_t prog_index)
{
	const char* cur = text;

	if (*cur != '$')
		return text; //Start with a '$'

	cur++;
	if (!isxdigit(*cur))
		return text; //Needs at least one number - FIXME: should just '$' be allowed? 

	for (cur++; *cur; cur++)
	{
		if (!isxdigit(*cur))
			break; //Valid hex#
	}

	InsertLexeme(text, cur - text, CODE_NUM_HEX, prog_index);
	return cur; //Valid hex#, just encountered a space
}

const char* scanner_c::CheckDecNum(const char* text, size_t prog_index)
{
	const char* cur = text;

	if (!isdigit(*cur))
		return text;

	for (cur++; *cur; cur++)
	{
		if (!isdigit(*cur))
			break; //Valid dec#
	}

	InsertLexeme(text, cur - text, CODE_NUM_DEC, prog_index);
	return cur;
}

const char* scanner_c::CheckFxdNum(const char* text, size_t prog_index)
{
	const char* cur = text;

	if (!isdigit(*cur))
		return text; //Must start with a digit

	for (cur++; *cur != '.'; cur++)
	{
		if (!*cur)
			return text; // Never found a decimal point

		if (!isdigit(*cur))
			return text; //Non-digit found before the decimal point
	}

	cur++;
	if (!isdigit(*cur))
		return text; //Can't have '1.'

	for (cur++; *cur; cur++)
	{
		if (!isdigit(*cur))
			break; //Valid fxd#
	}

	InsertLexeme(text, cur - text, CODE_NUM_FXD, prog_index);
	return cur; //Valid, just encountered a space
}

const char* scanner_c::CheckStr(const char* text, size_t prog_index)
{
	const char* cur = text;
	kv_t kv;

	if (*cur != '"')
		return text;

	for (cur++; *cur != '"'; cur++)
		if (!*cur)
			Error("Unclosed string");

	cur++;
	InsertLexeme(text, cur - text, CODE_STRING, prog_index);
	return cur;
}

void scanner_c::Lex(const char* prog_, llist_c* _list, bool _debug)
{
	struct timeb start_time, end_time;
	float time_seconds;
	char text[TEXT_MAX_LEN] = {};

	list = _list;
	debug = _debug;
	prog = prog_;

	ftime(&start_time);
	printf("\nScanning...\n");

	for (size_t pi = 0; prog[pi]; pi++)
	{
		pi = GetTextLump(text, pi);

		if (prog[pi] == '\n')
			newlines.push_back(pi);

		//text is now devoid of whitespace
		//cyclically check and advance the text against all possible lexemes
		//Todo: hex/bin fixed nums?
		for (const char* cur = text, *start = text; *cur; start = cur)
		{
			bool made_progress = false;

			for (int i = 0; i < check_count; i++)
			{
				size_t idx = (pi + (cur - text)) - 1;
				if (start != (cur = (this->*checks[i])(cur, idx)))
					made_progress = true;
			}

			if (!made_progress)
				Error("Bad lexical block '%s'", cur);
		}

		memset(text, 0, TEXT_MAX_LEN);
	}

	ftime(&end_time);
	time_seconds = (1000 * (end_time.time - start_time.time) + (end_time.millitm - start_time.millitm)) / 1000.0f;
	printf("Scanning completed in %.4f second(s)\n", time_seconds);


	list->Insert(NULL, nullkv); //null terminator
	if(debug)
		list->Disp();
}



void scanner_c::InsertLexeme(const char* text, size_t len, CODES code, size_t character_index)
{
	size_t line_no = CalculateLineNo(character_index);
	char key[KEY_MAX_LEN]; 

	strncpy_s(key, text, len); //'text' points to a text lump, we need to give the list only this specific lexeme.
	list->Insert(NULL, key, code, line_no);
}

size_t scanner_c::CalculateLineNo(size_t character_index)
{
	size_t size = newlines.size();

	//atm, char_idx is the index of the LAST element of the lexeme.
	//there SHOULD be sufficient info in newlines to pinpoint the line on which a lexeme lives, as well as its col no.
	//strings / comments can be multi-line. Keep track of start AND stop line?
	//How about defs? '\'?

	for (size_t i = 0; i < size; i++)
	{
		//This is hit if the lexeme directly precedes a newline
		//ex. ';'\n
		if (character_index < newlines[i])
			return i; 
	}

	//all other cases
	//ex. ';'\t\n
	//ex. 'db' 'b1' '=' '1' ';' (space) \n
	return size; 
}

size_t scanner_c::CalculateColNo(const char* text, size_t line_no)
{
	return 0;
}



void scanner_c::AddReservedChar(char c, int code)
{
	kv_t ins;
	ins.k[0] = c;
	ins.v = code;
	list->Insert(NULL, ins);
}

void scanner_c::AddLexeme(char* lexeme)
{
	kv_t ins;

	strcpy_s(ins.k, lexeme);
	ins.v = CODE_TEXT;

	for (int rwi = 0; rwi < sizeof(reservedwords) / sizeof(kv_t); rwi++)
	{//check for reserved words
		if (!strcmp(lexeme, reservedwords[rwi].k))
			ins.v = reservedwords[rwi].v;
	}

	//need to check for text, bin, dec, hex, etc.
	if (lexeme[0] == '$')
	{
		for (int i = 1; lexeme[i]; i++)
		{
			if (!isxdigit(lexeme[i]))
				Error("Invalid hexadecimal number");
		}

		ins.v = CODE_NUM_HEX;
	}
	else if (lexeme[0] == '%')
	{
		for (int i = 1; lexeme[i]; i++)
		{
			if (lexeme[i] != '0' && lexeme[i] != '1')
				Error("Invalid binary number");
		}

		ins.v = CODE_NUM_BIN;
	}
	else if (isdigit(lexeme[0]))
	{
		bool point = false;
		for (int i = 1; lexeme[i]; i++)
		{
			if (lexeme[i] == '.')
			{
				if (point) //there can't be two decimal points in a number
					Error("Invalid fixed number");
				point = true;
			}
			else if (!isdigit(lexeme[i]))
				Error("Invalid decimal number");
		}
		if (point)
			ins.v = CODE_NUM_FXD;
		else
			ins.v = CODE_NUM_DEC;
	}
	else if (isalpha(lexeme[0]))
	{
		for (int i = 1; lexeme[i]; i++)
		{
			if (!isdigit(lexeme[i]) && !isalpha(lexeme[i]) && lexeme[i] != '_')
				Error("Invalid text constant %s", lexeme);
		}
	}
	else if (lexeme[0] == '"')
		ins.v = CODE_STRING;
	else
		Error("Unrecognized starting character in lexeme %s", lexeme);

	list->Insert(NULL, ins);

	memset(lexeme, 0, KEY_MAX_LEN);
}
