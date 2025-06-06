#include <Windows.h>
#include "common.h"
#include "lex.h"
#include "parse.h"
#include "preprocessor.h"
#include "semantics.h"
#include "generator.h"

const char		tmp_filename[] = "c:/ti83/rl/test.rls";
scanner_c		lex;
preprocessor_c	preproc;
parser_c		parse;
analyzer_c		semantic;
generator_c		generator;

CONSOLE_SCREEN_BUFFER_INFO	base_csbi;
HANDLE console;

//TODO:
//Parsing
//	Reject storage specifiers in struct members in analysis
//Register allocation
//	don't allocate registers for data used 0-0 or 7-7
//	^need to skip data decls and instructions with only un allocated-vars
//Semantic analysis
//	in generator-check out dataofs with redefs
//	also check dload
//	manage checking inside expressions in general:
//	handle expression operands in instructions - check for def, update usage
//	don't allow [][] in mem exprs
//	check that the operators make sense ex. 1.member is a valid logical expression
//Mark 'a' with the last loaded sreg
//symbol declarations in expressions
//Fix memory leaks when using Error()
//structs
//	in semantics check that only valid member variables are used for an object
//	structlist args
//preprocessing
//labels
//	only one assignment in a function
//figure out how 'program' is big enough for a whole source file
//identifiers directly copied from the program should start with '_'. Anything the compiler generates should not be prefixed with this.


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
	tdata_t* tdata; //dynamically allocated by the analyzer
	igraph_c igraph;
	structlist_c sl;

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

	printf("%s\n", program);
	lex.Lex(program, &list, 0);
	//actually do the preprocessing here
	parse.Parse(&list, &tree, true);
	semantic.GenerateAST(&tree, &graph, symtbl, &symtbl_top, &tdata, &igraph, &sl);
	//generator.Generate(&tree, &graph, tdata, &symtbl_top, &igraph);

	ftime(&end);
	time_seconds = (1000 * (end.time - start.time) + (end.millitm - start.millitm)) / 1000.0f;
	printf("Compilation finished in %.4f second(s)\n", time_seconds);

	//delete[] tdata; //uncomment this when generateast gets uncommented

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

[[noreturn]] void Error(const char* msg, ...)
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