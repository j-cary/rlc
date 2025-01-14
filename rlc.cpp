#include <Windows.h>
#include "common.h"
#include "lex.h"
#include "parse.h"
#include "preprocessor.h"
#include "semantics.h"
#include "generator.h"

const char tmp_filename[] = "c:/ti83/rl/test.rls";
scanner_c		lex;
preprocessor_c	preproc;
parser_c		parse;
analyzer_c		semantic;
generator_c		generator;

CONSOLE_SCREEN_BUFFER_INFO	base_csbi;
HANDLE console;

//TODO:
//Register allocation
//	Reserve b in for loops 
//	only make data entries for values using software registers 
//	^don't initialize a data entry if a register is holding it. Just load the initial value 
//	don't allocate registers for data used 0-0 or 7-7
//	^need to skip data decls and instructions with only un allocated-vars
//Fix memory leaks when using Error()
//Typedef stuff - declaring variables of a typedef, how are arrays going to work? 
//preprocessing
//symbol table - make this part of IDENTIFIER?


int main()
{
	FILE* fin;

	struct timeb start, end;
	float time_seconds;

	static char program[PROG_MAX_LEN] = {};
	char* line = program;

	llist_c list;
	tree_c	tree;
	cfg_c	graph;
	data_t	symtbl[SYMBOLS_MAX];
	unsigned symtbl_top = 0;

	ftime(&start);

	console = GetStdHandle(STD_OUTPUT_HANDLE);
	GetConsoleScreenBufferInfo(console, &base_csbi);

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
	lex.Lex(program, &list, 0);
	//actually do the preprocessing here
	parse.Parse(&list, &tree, 0);
	semantic.GenerateAST(&tree, &graph, symtbl, &symtbl_top);
	//generator.Generate(&tree, &graph, symtbl, &symtbl_top);

	ftime(&end);
	time_seconds = (1000 * (end.time - start.time) + (end.millitm - start.millitm)) / 1000.0f;
	printf("Compilation finished in %.4f second(s)\n", time_seconds);

	return 0;
}


void Warning(const char* msg, ...)
{
	HANDLE cons = console;
	CONSOLE_SCREEN_BUFFER_INFO csbi;
	va_list args;

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
	HANDLE cons = console;
	va_list args;

	SetConsoleTextAttribute(cons, BACKGROUND_RED | BACKGROUND_INTENSITY);

	printf("\nError | Line no. %i col no. %i | ", 1, 2);
	va_start(args, msg);
	vprintf(msg, args);
	va_end(args);

	SetConsoleTextAttribute(cons, base_csbi.wAttributes);
	exit(0);
}

void SetOutFlags(unsigned short flags)
{
	SetConsoleTextAttribute(console, flags);
}

void ResetOutFlags()
{
	SetConsoleTextAttribute(console, base_csbi.wAttributes);
}