#include "preprocessor.h"

//make this a keyvalue with function pointers
const char keywords[][8] =
{
	"define",
	"macro",
	"undef",
	"include",
	"set"
};

bool preprocessor_c::ParseLine(char* line)
{
	int start;
	for (start = 0; isspace(line[start]); start++) {}

	if (!line[start])
		return true; //empty line
	else if (line[start] == '@')
	{ //preprocessing line

		if (ParseControlLine(&line[start + 1]))
			return false;
		else
			Error("Invalid control line\n");
	}


	return true; //success
}

bool preprocessor_c::ParseControlLine(char* line)
{
	int command_start, command_end;

	for (command_start = 0; line[command_start] && isspace(line[command_start]); command_start++) {} //skip whitespace

	printf("CTRL-LINE:%s", &line[command_start]);

	for (command_end = command_start; line[command_end] && !isspace(line[command_end]); command_end++)	{} //get the first text const

	if (!strncmp(&line[command_start], "include", command_end - command_start))
	{
		printf("");
	}

	for (int i = 0; i < sizeof(keywords) / 8; i++)
	{
		if (!strncmp(&line[command_start], keywords[i], command_end - command_start))
		{
			printf("");
			//call appropriate function here
			return true;
		}
	}

	return false; //unrecognized control command
}