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
	'%', CODE_PERCENT,

};

kv_t reservedwords[] =
{
	"inline",	CODE_INLINE,
	"subr",		CODE_SUBR,
	"ans",		CODE_ANS,
	
	//data types
	"db",		CODE_BYTE,		"byte",		CODE_BYTE,
	"dw",		CODE_WORD,		"word",		CODE_WORD,
	"ptr",		CODE_PTR,		"pointer",	CODE_PTR,
	"fxd",		CODE_FIXED,		"fixed",	CODE_FIXED,
	"lbl",		CODE_LABEL,		"label",	CODE_LABEL,
	"dba",		CODE_BYTEARRAY,	"bytearray",CODE_BYTEARRAY,
	"dwa",		CODE_WORDARRAY,	"wordarray",CODE_WORDARRAY,
	"dpa",		CODE_PTRARRAY,	"ptrarray",	CODE_PTRARRAY,
	"dfa",		CODE_FXDARRAY,	"fxdarray",	CODE_FXDARRAY,

	"type",		CODE_TYPE,

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
};

void scanner_c::Lex(const char* prog, llist_c* _list, bool _debug)
{
	struct timeb start, end;
	float time_seconds;
	char text[TEXT_MAX_LEN] = {};

	list = _list;
	debug = _debug;

	ftime(&start);
	printf("\nScanning...\n");

	for (int pi = 0; prog[pi]; pi++)
	{
		int ti;
		bool hasquote = 0;

		for (; prog[pi] && isspace(prog[pi]); pi++) {}
		for (ti = 0; prog[pi] && (!isspace(prog[pi]) || hasquote); pi++, ti++)
		{
			if (ti >= TEXT_MAX_LEN)
				Error("Text constant too long. Is there an unclosed quote?");

			if (prog[pi] == '"')
			{//this is here so strings don't skip over whitespace
				if (hasquote)	
					hasquote = 0;
				else
					hasquote = 1;
			}
			else if (prog[pi] == '#')
			{//skip everything in the comment
				
				for (pi++; prog[pi] != '#'; pi++)
				{
					if (!prog[pi])
						Error("Unclosed comment");

				}
				break;
			}


			text[ti] = prog[pi];
		}

		if (!*text) //null string
			continue;

		//text is now devoid of whitespace
		char lexeme[KEY_MAX_LEN] = {};
		int li = 0;

		for (int ti = 0; text[ti]; ti++)
		{
			kv_t ins;

			//string
			if (text[ti] == '"')
			{
				if (*lexeme)
					AddLexeme(lexeme);
				li = 0;

				lexeme[li++] = '"';

				for (ti++; text[ti] != '"'; ti++)
				{
					if (!text[ti])
						Error("Unclosed double quote!"); //should be superfluous

					lexeme[li++] = text[ti];
				}

				lexeme[li++] = '"'; //catch the closing quote
				ti++; //don't run into this quote again

				if (*lexeme)
					AddLexeme(lexeme);
				li = 0;
			}

			//check for reserved chars
			for (int ri = 0; ri < sizeof(reservedchars) / sizeof(ckv_t); ri++)
			{
				if (text[ti] == reservedchars[ri].k)
				{
					if (li && reservedchars[ri].k == '.' && isdigit(lexeme[li - 1]) && isdigit(text[ti + 1]))
					{//if lexeme has been started, the char is a period, and numbers surround the period - assume a fixed number
						lexeme[li++] = text[ti];
						for (ti++; text[ti]; ti++)
						{
							lexeme[li++] = text[ti];
						}
						goto copy;
					}
					else if (*lexeme)
					{
						AddLexeme(lexeme);
						li = 0;
					}

					AddReservedChar(text[ti], reservedchars[ri].v);
					goto skipcopy;
				}

			}

			lexeme[li++] = text[ti];
		skipcopy:;

		}
	copy:
		if (*lexeme)
			AddLexeme(lexeme);

		memset(text, 0, TEXT_MAX_LEN);
	}

	ftime(&end);
	time_seconds = (1000 * (end.time - start.time) + (end.millitm - start.millitm)) / 1000.0f;
	printf("Scanning completed in %.4f second(s)\n", time_seconds);


	list->Insert(NULL, nullkv); //null terminator
	if(debug)
		list->Disp();
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