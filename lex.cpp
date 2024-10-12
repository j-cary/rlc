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
	'>', CODE_RARROW,

	//arithmetic
	'=', CODE_EQUALS,
	'+', CODE_PLUS,
	'-', CODE_MINUS,
	'*', CODE_STAR,
	'/', CODE_FSLASH,

};

kv_t reservedwords[] =
{
	"start",	CODE_START,
	"inline",	CODE_INLINE,
	"subr",		CODE_SUBR,
	"ans",		CODE_ANS,

	"byte",		CODE_BYTE,
	"word",		CODE_WORD,
	"ptr",		CODE_PTR,
	"array",	CODE_ARRAY,
	"struct",	CODE_STRUCT,
	"type",		CODE_TYPE,

	"stack",	CODE_STACK,
	"heap",		CODE_HEAP,

	"repeat",	CODE_REPEAT,
	"until",	CODE_UNTIL,
	"while",	CODE_WHILE,
	"for",		CODE_FOR,

	"if",		CODE_IF,
	"else",		CODE_ELSE,
	"eq",		CODE_EQ,
	"neq",		CODE_NEQ,
	"lt",		CODE_LT,
	"gt",		CODE_GT,
	"le",		CODE_LE,
	"ge",		CODE_GE,

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
	"expr",		CODE_EXPR,

	"rlc",		CODE_RLC,
	"rrc",		CODE_RRC,
	"rl",		CODE_RL,
	"rr",		CODE_RR,
	"sla",		CODE_SLA,
	"sra",		CODE_SRA,
	"sll",		CODE_SLL,
	"srl",		CODE_SRL,
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

void lex_c::Lex(const char* prog, llist_c* _list)
{
	char text[TEXT_MAX_LEN] = {};

	list = _list;

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
						Error("Unclosed double quote!"); //should be superflous

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
					if (*lexeme)
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
		
		if (*lexeme)
			AddLexeme(lexeme);

		memset(text, 0, TEXT_MAX_LEN);
	}

	list->Insert(NULL, nullkv); //null terminator
	list->Disp();

	printf("");
}

void lex_c::AddReservedChar(char c, int code)
{
	kv_t ins;
	ins.k[0] = c;
	ins.v = code;
	list->Insert(NULL, ins);
}

void lex_c::AddLexeme(char* lexeme)
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
		for (int i = 1; lexeme[i]; i++)
		{
			if (!isdigit(lexeme[i]))
				Error("Invalid decimal number");
		}

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