/***************************************************************************************************
Purpose: Perform inline token replacement and conditional compilation based on macros
***************************************************************************************************/
#pragma once
#include "common.h"

class preprocessor_c
{
private:

public:
	bool ParseLine(char* line);
	bool ParseControlLine(char* line);
};