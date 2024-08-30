#pragma once
#include "common.h"

class preprocessor_c
{
private:

public:
	bool ParseLine(char* line);
	bool ParseControlLine(char* line);
};