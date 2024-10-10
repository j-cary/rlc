#include <Windows.h>
#include "common.h"
#include "lex.h"
#include "parse.h"
#include "preprocessor.h"

const char tmp_filename[] = "c:/ti83/rl/test.rls";
lex_c lex;
parse_c parse;
preprocessor_c preproc;

//TODO:
//preprocessing

//FIXME:
//can't use an underscore for whatever reason

int main()
{
	FILE* fin;

	static char program[PROG_MAX_LEN] = {};
	char* line = program;

	llist_c list;

	fopen_s(&fin, tmp_filename, "rb");

	if (!fin)
		Error("Couldn't open input file %s!", tmp_filename);


    while(1)
	{
		//fixme: check for PROG_MAX_LEN here

		fgets(line, PROG_MAX_LEN, fin);

		if (feof(fin))
			break;

		if (preproc.ParseLine(line))
		{
			for (; *line; line++) {}; //find the new end of the string and add this line

		}
		else
		{
			for (char* erase = line; *erase; erase++) 
				*erase = '\0';

		}
	}

	printf("%s\n", program);
	lex.Lex(program, &list);
	parse.Parse(&list);

	return 0;
}


void Warning(const char* msg, ...)
{
	HANDLE cons;
	CONSOLE_SCREEN_BUFFER_INFO csbi;
	va_list args;

	cons = GetStdHandle(STD_OUTPUT_HANDLE);
	GetConsoleScreenBufferInfo(cons, &csbi);
	SetConsoleTextAttribute(cons, BACKGROUND_RED | BACKGROUND_GREEN);

	//printf("\nWarning | Line no. %i col no. %i | ", line_no, col_no);
	va_start(args, msg);
	vprintf(msg, args);
	va_end(args);

	SetConsoleTextAttribute(cons, csbi.wAttributes);
}

void Error(const char* msg, ...)
{
	HANDLE cons;
	CONSOLE_SCREEN_BUFFER_INFO csbi;
	va_list args;

	cons = GetStdHandle(STD_OUTPUT_HANDLE);
	GetConsoleScreenBufferInfo(cons, &csbi);
	SetConsoleTextAttribute(cons, BACKGROUND_RED);

	printf("\nError | Line no. %i col no. %i | ", 1, 2);
	va_start(args, msg);
	vprintf(msg, args);
	va_end(args);

	SetConsoleTextAttribute(cons, csbi.wAttributes);
	exit(0);
}