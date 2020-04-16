#pragma once



enum
{
	// Generic errors.
	ERR_INV_ARG        = 1u,
	ERR_TOO_FEW_ARGS   = 2u,
	ERR_TOO_MANY_ARGS  = 3u,
	ERR_EXCEPTION      = 4u,
	ERR_UNK_EXCEPTION  = 5u,
	ERR_FILE_OPEN      = 6u,
	ERR_FILE_READ      = 7u,
	ERR_FILE_CREATE    = 8u,
	ERR_FILE_WRITE     = 9u,
	ERR_OUT_OF_MEMORY  = 10u,

	// Parser errors.
	ERR_UNK_INSTRUCTION      = 20u,
	ERR_UNK_REGISTER         = 21u,
	ERR_INV_PARSER_ARGS      = 22u,
	ERR_OUT_OF_RANGE         = 23u,
	ERR_NOT_ENOUGH_LCs       = 24u, // Not enough loop counters.
	ERR_LOOPS_TOO_DEEP       = 25u,
	ERR_LOOP_WITHOUT_START   = 26u,
	ERR_LOOP_WITHOUT_END     = 27u
};


/*
In line 7:
DMAADDH SAR, 0xDEADBEEF
Error: Out of range: "0xDEADBEEF"
*/
