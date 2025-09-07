/***************************************************************************************************
Purpose:
***************************************************************************************************/
#include "lex.h"

/***************************************************************************************************
										Private Variables
***************************************************************************************************/

static struct {
	char reserved;
	CODE code;
} reservedchars[] =
{
	'(', CODE::LPAREN,
	')', CODE::RPAREN,
	'{', CODE::LBRACKET,
	'}', CODE::RBRACKET,
	'[', CODE::LBRACE,
	']', CODE::RBRACE,

	'@', CODE::AT,
	//'#', CODE::POUND,
	'.', CODE::PERIOD,
	',', CODE::COMMA,
	//'"', CODE::QUOTE_DOUBLE,
	//'\'', CODE::QUOTE_SINGLE,
	';', CODE::SEMICOLON,
	':', CODE::COLON,
	'<', CODE::LARROW,
	'>', CODE::RARROW,
	'!', CODE::EXCLAMATION,
	'&', CODE::AMPERSAND,
	'|', CODE::BAR,

	//arithmetic
	'=', CODE::EQUALS,
	'+', CODE::PLUS,
	'-', CODE::MINUS,
	'*', CODE::STAR,
	'/', CODE::FSLASH,
	//'%', CODE::PERCENT,

	'\0', CODE::NONE

};

static kv_t reservedwords[] =
{
	"inline",	CODE::INLINE,
	"subr",		CODE::SUBR,
	"ans",		CODE::ANS,
	
	//data types
	"db",		CODE::BYTE,		"byte",		CODE::BYTE,
	"dw",		CODE::WORD,		"word",		CODE::WORD,
	"fxd",		CODE::FIXED,	"fixed",	CODE::FIXED,
	"lbl",		CODE::LABEL,	"label",	CODE::LABEL,

	"struct",	CODE::STRUCT,
	"signed",	CODE::SIGNED,
	"static",	CODE::STATIC,	"stack",	CODE::STACK,	"auto", CODE::AUTO,

	//control flow
	"repeat",	CODE::REPEAT,
	"until",	CODE::UNTIL,
	"while",	CODE::WHILE,
	"for",		CODE::FOR,
	"if",		CODE::IF,
	"else",		CODE::ELSE,

	//instructions
	"ld",		CODE::LD,
	"jp",		CODE::JP,
	"call",		CODE::CALL,
	"ret",		CODE::RET,

	"add",		CODE::ADD,
	"sub",		CODE::SUB,
	"mul",		CODE::MUL,
	"div",		CODE::DIV,
	"mod",		CODE::MOD,
	"inc",		CODE::INC,
	"dec",		CODE::DEC,
	"comp",		CODE::COMP,
	"and",		CODE::AND,
	"or",		CODE::OR,
	"xor",		CODE::XOR,
	"neg",		CODE::NEG,

	"rlc",		CODE::RLC,
	"rrc",		CODE::RRC,
	"rl",		CODE::RL,
	"rr",		CODE::RR,
	"sl",		CODE::SL,
	"sr",		CODE::SR,
	"res",		CODE::RES,
	"set",		CODE::SET,
	"flp",		CODE::FLP,

	"in",		CODE::INN,
	"out",		CODE::OT,
	"im",		CODE::IM,

	"ldm",		CODE::LDM,
	"cpm",		CODE::CPM,
	"inm",		CODE::INM,
	"outm",		CODE::OUTM,

	//preprocessing
	"include",	CODE::PP_INCLUDE,
	"insert",	CODE::PP_INSERT,
	"define",	CODE::PP_DEFINE,
	NULL,		0 //null terminator
};

/***************************************************************************************************
										Private Functions
***************************************************************************************************/

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
		ASSERT(ti < TEXT_MAX_LEN, "Text constant too long. Is there an unclosed quote?");

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
				ASSERT(*cur, "Unclosed comment");

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

			InsertLexeme(kv->k, strlen(kv->k), (CODE)kv->v, prog_index);
			return text + len;
		}
	}

	return text;
}

const char* scanner_c::CheckReservedChar(const char* text, size_t prog_index)
{
	for (int i = 0; reservedchars[i].reserved != '\0'; ++i)
	{
		if (*text == reservedchars[i].reserved)
		{//matched a reserved char

			InsertLexeme(text, 1, reservedchars[i].code, prog_index);
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

	InsertLexeme(text, cur - text, CODE::TEXT, prog_index);
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

	InsertLexeme(text, cur - text, CODE::NUM_BIN, prog_index);
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

	InsertLexeme(text, cur - text, CODE::NUM_HEX, prog_index);
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

	InsertLexeme(text, cur - text, CODE::NUM_DEC, prog_index);
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

	InsertLexeme(text, cur - text, CODE::NUM_FXD, prog_index);
	return cur; //Valid, just encountered a space
}

const char* scanner_c::CheckStr(const char* text, size_t prog_index)
{
	const char* cur = text;
	kv_t kv;

	if (*cur != '"')
		return text;

	for (cur++; *cur != '"'; cur++)
		ASSERT(*cur, "Unclosed string");

	cur++;
	InsertLexeme(text, cur - text, CODE::STRING, prog_index);
	return cur;
}





void scanner_c::InsertLexeme(const char* text, size_t len, CODE code, size_t character_index)
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



void scanner_c::AddReservedChar(char c, CODE code)
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
	ins.v = CODE::TEXT;

	for (int rwi = 0; rwi < sizeof(reservedwords) / sizeof(kv_t); rwi++)
	{//check for reserved words
		if (!strcmp(lexeme, reservedwords[rwi].k))
			ins.v = reservedwords[rwi].v;
	}

	//need to check for text, bin, dec, hex, etc.
	if (lexeme[0] == '$')
	{
		for (int i = 1; lexeme[i]; i++)
			ASSERT(isxdigit(lexeme[i]), "Invalid hexadecimal constant");

		ins.v = CODE::NUM_HEX;
	}
	else if (lexeme[0] == '%')
	{
		for (int i = 1; lexeme[i]; i++)
			ASSERT(lexeme[i] == '0' || lexeme[i] == '1', "Invalid binary constant");

		ins.v = CODE::NUM_BIN;
	}
	else if (isdigit(lexeme[0]))
	{
		bool point = false;
		for (int i = 1; lexeme[i]; i++)
		{
			if (lexeme[i] == '.')
			{
				ASSERT(!point, "Unexpected '.' in fixed constant");
				point = true;
			}
			else if (!isdigit(lexeme[i]))
				ASSERT(0, "Invalid decimal constant");
		}

		ins.v = point ? CODE::NUM_FXD : CODE::NUM_DEC;
	}
	else if (isalpha(lexeme[0]))
	{
		for (int i = 1; lexeme[i]; i++)
		{
			ASSERT(isdigit(lexeme[i]) || isalpha(lexeme[i]) || lexeme[i] == '_',
				"Invalid text constant %s", lexeme);
		}
	}
	else if (lexeme[0] == '"')
	{
		ins.v = CODE::STRING;
	}
	else
		ASSERT(0, "Unrecognized primary character in lexeme %s", lexeme);

	list->Insert(NULL, ins);

	memset(lexeme, 0, KEY_MAX_LEN);
}

/***************************************************************************************************
									   Interface Functions
***************************************************************************************************/

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
