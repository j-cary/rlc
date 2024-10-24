#include <Windows.h>
#include "common.h"
#include "lex.h"
#include "parse.h"
#include "preprocessor.h"
#include "semantics.h"

const char tmp_filename[] = "c:/ti83/rl/test.rls";
lex_c lex;
preprocessor_c preproc;
parse_c parse;
semantic_c semantic;

//TODO:
//Call stuff. thinking call oscallname, parm1, parm2, etc.;
//Typedef stuff - declaring variables of a type, how are arrays going to work? 
//preprocessing
//symbol table - make this part of IDENTIFIER?


int main()
{
	FILE* fin;

	struct timeb start, end;
	int elapsed_time;

	static char program[PROG_MAX_LEN] = {};
	char* line = program;

	llist_c list;
	tnode_c tree;

	ftime(&start);

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

	//printf("%s\n", program);
	lex.Lex(program, &list, false);
	//actually do the preprocessing here
	parse.Parse(&list, &tree, false);
	semantic.Check(&tree);

	ftime(&end);
	elapsed_time = (int)(1000.0 * (end.time - start.time) + (end.millitm - start.millitm));
	printf("Compilation finished in %u millisecond(s)\n", elapsed_time);

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