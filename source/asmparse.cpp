#include <cstdio>
#include <cstring>
#include <string>
#include <memory>
#include <unordered_map>
#include "types.h"
#include "asmparse.h"
#include "instructions.h"
#include "utils.h"
#include "c_header_gen.h"
#include "errors.h"


static const std::unordered_map<std::string, int (*)(u32, const char *const [MAX_TOKENS])> instMap
({
	{"ADDH",   emitAdd},
	{"ADNH",   emitAdd},
	{"END",    emitEnd},
	{"FLUSHP", emitFlushp},
	{"GO",     emitGo},
	{"KILL",   emitKill},
	{"LD",     emitLd},     {"LDS",    emitLd}, {"LDB",    emitLd},
	{"LDP",    emitLd},     {"LDPS",   emitLd}, {"LDPB",   emitLd},
	{"LP",     emitLp},
	{"LPEND",  emitLp},     {"LPENDS", emitLp}, {"LPENDB", emitLp},
	{"LPFE",   emitLp}, // LPEND with special bits
	{"MOV",    emitMov},
	{"NOP",    emitNop},
	{"RMB",    emitMb},
	{"SEV",    emitSev},
	{"ST",     emitSt},     {"STS",    emitSt}, {"STB",    emitSt},
	{"STP",    emitSt},     {"STPS",   emitSt}, {"STPB",   emitSt},
	{"STZ",    emitSt},
	{"WFE",    emitWfe},
	{"WFP",    emitWfp},
	{"WMB",    emitMb}
});

static std::unique_ptr<u8[]> g_progBuf(nullptr);
static u32 g_progPos = 0;
static u32 g_loopDepth = 0;



int emitAdd(u32 argc, const char *const argv[MAX_TOKENS])
{
	if(argc != 3) return ERR_INV_PARSER_ARGS;

	u32 inst;
	if(argv[0][2] == 'D') inst = INST_ADDH;
	else                  inst = INST_ADNH;

	static const char *const regWlist[2] = {"SAR", "DAR"};
	s32 ra;
	if((ra = checkStrList(regWlist, 2, 0, argv[1])) < 0) return ERR_UNK_REGISTER;

	// SAR is 0. Nothing to do.
	if(ra == 1) inst |= INST_BIT_ADD_DAR;

	// TODO: Range check.
	inst |= (strtoul(argv[2], nullptr, 0) & 0xFFFFu)<<INST_ADD_IMM_SHIFT;

	memcpy(&g_progBuf[g_progPos], &inst, 3);
	g_progPos += 3;

	return 0;
}

int emitEnd(u32 argc, const char *const argv[MAX_TOKENS])
{
	if(argc != 1) return ERR_INV_PARSER_ARGS;

	g_progBuf[g_progPos++] = INST_END;

	return 0;
}

int emitFlushp(u32 argc, const char *const argv[MAX_TOKENS])
{
	if(argc != 2) return ERR_INV_PARSER_ARGS;

	// TODO: Range check.
	// TODO: Periphal numbers start with "P".
	u16 inst = INST_FLUSHP;
	inst |= (strtoul(argv[1], nullptr, 0) & INST_PERIPH_MASK)<<INST_PERIPH_SHIFT;

	memcpy(&g_progBuf[g_progPos], &inst, 2);
	g_progPos += 2;

	return 0;
}

int emitGo(u32 argc, const char *const argv[MAX_TOKENS])
{
	if(argc < 3 || argc > 4) return ERR_INV_PARSER_ARGS;

	//fprintf(stderr, "%" PRIu32 ": Warning: Only DMA managers can execute DMAGO.\n", curLine);

	u64 inst = INST_GO;
	if(argc == 4)
	{
		if(strcmp("ns", argv[3]) == 0) inst |= INST_BIT_GO_NON_SEC;
		else return ERR_INV_PARSER_ARGS;
	}

	// TODO: Range check.
	// TODO: Channel numbers start with "C".
	inst |= (strtoul(argv[1], nullptr, 0) & INST_GO_CN_MASK)<<INST_GO_CN_SHIFT;
	inst |= strtoul(argv[2], nullptr, 0)<<INST_GO_IMM_SHIFT;

	memcpy(&g_progBuf[g_progPos], &inst, 6);
	g_progPos += 6;

	return 0;
}

int emitKill(u32 argc, const char *const argv[MAX_TOKENS])
{
	if(argc != 1) return ERR_INV_PARSER_ARGS;

	g_progBuf[g_progPos++] = INST_KILL;

	return 0;
}

int emitLd(u32 argc, const char *const argv[MAX_TOKENS])
{
	if(argc < 1 || argc > 2) return ERR_INV_PARSER_ARGS;

	u16 inst;
	if(strncmp("LDP", argv[0], 3) == 0) inst = INST_LDP;
	else                                inst = INST_LD;

	const char bs = argv[0][strlen(argv[0]) - 1];
	if(bs == 'B')      inst |= INST_BIT_BURST | INST_BIT_COND;
	else if(bs == 'S') inst |= INST_BIT_COND;

	if(inst & 1u<<5) // LDP
	{
		// TODO: Range check.
		// TODO: Periphal numbers start with "P".
		inst |= (strtoul(argv[1], nullptr, 0) & INST_PERIPH_MASK)<<INST_PERIPH_SHIFT;

		memcpy(&g_progBuf[g_progPos], &inst, 2);
		g_progPos += 2;
	}
	else g_progBuf[g_progPos++] = static_cast<u8>(inst);

	return 0;
}

int emitLp(u32 argc, const char *const argv[MAX_TOKENS])
{
	if(argc < 1 || argc > 2) return ERR_INV_PARSER_ARGS;

	// We allow 1 loop forever and 2 counted loops.
	static u8 countedLoops = 0;
	static u8 lTypes[3] = {0};   // 1 = DMALP, 2 = DMALPFE
	static u32 lStarts[3] = {0}; // Each entry contains the start position.

	if(strcmp("LPFE", argv[0]) != 0) // Not DMALPFE.
	{
		u16 inst;
		if(strcmp("LP", argv[0]) == 0) // DMALP
		{
			if(argc != 2) return ERR_INV_PARSER_ARGS;
			if(g_loopDepth == 3) return ERR_LOOPS_TOO_DEEP;
			if(countedLoops == 2) return ERR_NOT_ENOUGH_LCs;

			countedLoops++;
			lTypes[g_loopDepth] = 1;
			lStarts[g_loopDepth++] = g_progPos + 2;

			inst = INST_LP | (countedLoops == 2 ? INST_BIT_LP_LC1 : 0u);
			inst |= (strtoul(argv[1], nullptr, 0) - 1)<<INST_LP_ITER_SHIFT; // TODO: Error and range checking.
		}
		else // DMALPEND
		{
			if(argc != 1) return ERR_INV_PARSER_ARGS;
			if(g_loopDepth == 0) return ERR_LOOP_WITHOUT_START;

			const char bs = argv[0][strlen(argv[0]) - 1];
			inst = INST_LPEND;
			if(lTypes[g_loopDepth - 1] == 1) // DMALP
			{
				if(bs == 'B')      inst |= INST_BIT_BURST | INST_BIT_COND;
				else if(bs == 'S') inst |= INST_BIT_COND;
				inst |= INST_BIT_LPEND_NOT_FOREVER | (countedLoops == 2 ? INST_BIT_LPEND_LC1 : 0u);

				countedLoops--;
			}
			else // DMALPFE
			{
				if(bs == 'B' || bs == 'S')
				{
					fprintf(stderr, "Warning: \"%s\" DMALPFE loops can't be conditional.\n", argv[0]);
					inst |= INST_BIT_LPEND_NOT_FOREVER;
				}
			}

			const u32 back_jmp = g_progPos - lStarts[g_loopDepth - 1];
			if(back_jmp > 255 || back_jmp > ~g_progPos) return ERR_OUT_OF_RANGE;
			inst |= back_jmp<<INST_LPEND_BACK_JMP_SHIFT;

			g_loopDepth--;
		}

		memcpy(&g_progBuf[g_progPos], &inst, 2);
		g_progPos += 2;
	}
	else // Handle DMALPFE pseudo instruction.
	{
		if(argc != 1) return ERR_INV_PARSER_ARGS;
		if(g_loopDepth == 3) return ERR_LOOPS_TOO_DEEP;
		if(lTypes[0] == 2 || lTypes[1] == 2 || lTypes[2] == 2) return ERR_LOOPS_TOO_DEEP;

		lTypes[g_loopDepth] = 2;
		lStarts[g_loopDepth++] = g_progPos;
	}

	return 0;
}

int emitMov(u32 argc, const char *const argv[MAX_TOKENS])
{
	if(argc < 3 || argc > 13) return ERR_INV_PARSER_ARGS;

	static const char *const regWlist[3] = {"SAR", "CCR", "DAR"};
	s32 rd;
	if((rd = checkStrList(regWlist, 3, 0, argv[1])) < 0) return ERR_UNK_REGISTER;
	if(rd != 1 && argc > 3) return ERR_INV_PARSER_ARGS;

	// Defaults.
	u64 inst = INST_MOV;

	// SAR is 0. Nothing to do.
	//if(rd == 1)      inst |= 1u<<INST_MOV_RD_SHIFT; // CCR
	//else if(rd == 2) inst |= 2u<<INST_MOV_RD_SHIFT; // DAR
	inst |= static_cast<u32>(rd)<<INST_MOV_RD_SHIFT;

	if(argc == 3) inst |= strtoul(argv[2], nullptr, 0)<<INST_MOV_IMM_SHIFT; // TODO: Error checking.
	else
	{
		inst |= CCR_DEFAULT_VAL<<INST_MOV_IMM_SHIFT;

		for(u32 i = 2; i < argc; i++)
		{
			const char *const arg = argv[i];

			static const char *const ccrWlist[11] = {"SA", "SB", "SS", "SP", "SC", "DA", "DB", "DS", "DP", "DC", "ES"};
			s32 ccrArg;
			if((ccrArg = checkStrList(ccrWlist, 11, 2, arg)) < 0) return ERR_INV_PARSER_ARGS;

			static const struct
			{
				u8 mask;
				u8 shift;
				u8 type;       // 0 = Value, 1 = Bits, 2 = 'I' and 'F'
				u8 rangeStart;
				u8 rangeEnd;
			} ccrLut[11] =
			{
				// SA[I|F]           SB[1-16]             SS[8|16|32|64|128]   SP[0-7]
				// SC[0-15]          DA[I|F]              SB[1-16]             DS[8|16|32|64|128]
				// DP[0-7]           DC[0-15]             ES[8|16|32|64|128]
				{0x1, 0, 2, 0, 0},   {0xF, 4, 0, 1, 16},  {0x7, 1, 1, 8, 128}, {0x7, 8, 0, 0, 7},
				{0x7, 11, 0, 0, 15}, {0x1, 14, 2, 0, 0},  {0xF, 18, 0, 1, 16}, {0x7, 15, 1, 8, 128},
				{0x7, 22, 0, 0, 7},  {0x7, 25, 0, 0, 15}, {0x7, 28, 1, 8, 128}
			};

			const u8 mask  = ccrLut[ccrArg].mask;
			const u8 shift = ccrLut[ccrArg].shift;
			inst &= ~(static_cast<u64>(mask)<<(INST_MOV_IMM_SHIFT + shift)); // TODO: Do we need the casts?

			const u8 type       = ccrLut[ccrArg].type;
			u32 val;
			const u8 rangeStart = ccrLut[ccrArg].rangeStart;
			const u8 rangeEnd   = ccrLut[ccrArg].rangeEnd;
			if(type < 2)
			{
				val = strtoul(&arg[2], nullptr, 0); // TODO: Error checks.
				if(val < rangeStart || val > rangeEnd) return ERR_OUT_OF_RANGE;
			}

			int res = 0;
			if(ccrArg != 4 && ccrArg != 9)
			{
				switch(type)
				{
					case 0: // Value
						if(rangeStart == 1) val--; // Workaround for SB and DB.
						inst |= static_cast<u64>(val)<<(INST_MOV_IMM_SHIFT + shift);
						break;
					case 1: // Bits
						if((val & (val - 1)) == 0) // Power of 2.
						{
							inst |= static_cast<u64>(__builtin_ctzl(val / 8))<<(INST_MOV_IMM_SHIFT + shift);
						}
						else res = ERR_OUT_OF_RANGE;
						break;
					case 2: // 'I' and 'F'
						if(arg[2] == 'I') inst |= 1ull<<(INST_MOV_IMM_SHIFT + shift);
						else if(arg[2] == 'F') ;
						else res = ERR_INV_PARSER_ARGS;
						break;
				}
			}
			else // SC and DC need special handling.
			{
				if(ccrArg == 4) // SC
				{
					if(val & 1u<<3) fprintf(stderr, "Warning: \"%s\" bit 3 can't be 1.\n", arg);
					inst |= static_cast<u64>(val & 7u)<<(INST_MOV_IMM_SHIFT + shift);
				}
				else // DC
				{
					if(val & 1u<<2) fprintf(stderr, "Warning: \"%s\" bit 2 can't be 1.\n", arg);
					val = (val & 3u) | (val>>1 & 1u<<2);
					inst |= static_cast<u64>(val)<<(INST_MOV_IMM_SHIFT + shift);
				}
			}

			if(res != 0) return res;
		}
	}

	memcpy(&g_progBuf[g_progPos], &inst, 6);
	g_progPos += 6;

	return 0;
}

int emitNop(u32 argc, const char *const argv[MAX_TOKENS])
{
	if(argc != 1) return ERR_INV_PARSER_ARGS;

	g_progBuf[g_progPos++] = INST_NOP;

	return 0;
}

int emitMb(u32 argc, const char *const argv[MAX_TOKENS])
{
	if(argc != 1) return ERR_INV_PARSER_ARGS;

	g_progBuf[g_progPos++] = (argv[0][0] == 'R' ? INST_RMB : INST_WMB);

	return 0;
}

int emitSev(u32 argc, const char *const argv[MAX_TOKENS])
{
	if(argc != 2) return ERR_INV_PARSER_ARGS;

	// TODO: Range check.
	// TODO: Event numbers start with "E"?
	u16 inst = INST_SEV;
	inst |= (strtoul(argv[1], nullptr, 0) & INST_EVENT_MASK)<<INST_EVENT_SHIFT;

	memcpy(&g_progBuf[g_progPos], &inst, 2);
	g_progPos += 2;

	return 0;
}

int emitSt(u32 argc, const char *const argv[MAX_TOKENS])
{
	if(argc < 1 || argc > 2) return ERR_INV_PARSER_ARGS;

	u16 inst;
	if(strncmp("STP", argv[0], 3) == 0)      inst = INST_STP;
	else if(strncmp("STZ", argv[0], 3) == 0) inst = INST_STZ;
	else                                     inst = INST_ST;

	if(inst != INST_STZ)
	{
		const char bs = argv[0][strlen(argv[0]) - 1];
		if(bs == 'B')      inst |= INST_BIT_BURST | INST_BIT_COND;
		else if(bs == 'S') inst |= INST_BIT_COND;
	}

	if(inst & 1u<<5) // STP
	{
		// TODO: Range check.
		// TODO: Periphal numbers start with "P".
		inst |= (strtoul(argv[1], nullptr, 0) & INST_PERIPH_MASK)<<INST_PERIPH_SHIFT;

		memcpy(&g_progBuf[g_progPos], &inst, 2);
		g_progPos += 2;
	}
	else g_progBuf[g_progPos++] = static_cast<u8>(inst);

	return 0;
}

int emitWfe(u32 argc, const char *const argv[MAX_TOKENS])
{
	if(argc < 2 || argc > 3) return ERR_INV_PARSER_ARGS;

	u16 inst = INST_WFE;
	if(argc == 3)
	{
		if(strcmp("invalid", argv[2]) == 0) inst |= INST_BIT_WFE_INVAL;
		else return ERR_INV_PARSER_ARGS;
	}

	// TODO: Range check.
	// TODO: Event numbers start with "E"?
	inst |= (strtoul(argv[1], nullptr, 0) & INST_EVENT_MASK)<<INST_EVENT_SHIFT;

	memcpy(&g_progBuf[g_progPos], &inst, 2);
	g_progPos += 2;

	return 0;
}

int emitWfp(u32 argc, const char *const argv[MAX_TOKENS])
{
	if(argc != 3) return ERR_INV_PARSER_ARGS;

	static const char *const trigWlist[3] = {"single", "burst", "periph"};
	s32 p_bs;
	if((p_bs = checkStrList(trigWlist, 3, 0, argv[2])) < 0) return ERR_INV_PARSER_ARGS;

	u16 inst = INST_WFP;
	if(p_bs == 1)      inst |= INST_BIT_BURST;
	else if(p_bs == 2) inst |= INST_BIT_WFP_PERIPH;

	inst |= (strtoul(argv[1], nullptr, 0) & INST_PERIPH_MASK)<<INST_PERIPH_SHIFT;

	memcpy(&g_progBuf[g_progPos], &inst, 2);
	g_progPos += 2;

	return 0;
}

// Assumes at least 1 token.
static u32 tokenize(char *const line, const char *tokens[MAX_TOKENS])
{
	memset(tokens, 0, sizeof(char*) * MAX_TOKENS);
	tokens[0] = strtok(line, " ,\t");
	u32 num = 1;
	for(u32 i = 1; i < MAX_TOKENS; i++)
	{
		if((tokens[i] = strtok(nullptr, " ,\t")) == nullptr) break;
		num++;
	}

	return num;
}

int dma330as(const char *const inFile, const char *const outFile)
{
	FILE *asmFh = fopen(inFile, "r");
	if(!asmFh)
	{
		fprintf(stderr, "Failed to open '%s'.\n", inFile);
		return 1;
	}

	const std::unique_ptr<char[]> inBuf(new(std::nothrow) char[INBUF_SIZE]);
	if(!inBuf)
	{
		fclose(asmFh);
		return ERR_OUT_OF_MEMORY;
	}

	g_progBuf = std::unique_ptr<u8[]>(new(std::nothrow) u8[OUTBUF_SIZE]);
	if(!g_progBuf)
	{
		fclose(asmFh);
		return ERR_OUT_OF_MEMORY;
	}

	u32 curLine = 0;
	int res = 0;
	while(fgets(inBuf.get(), INBUF_SIZE, asmFh))
	{
		curLine++;

		char *line = const_cast<char*>(findChar(inBuf.get()));
		if(line == nullptr || *line == '#') continue;
		strtok(line, "#\n"); // Remove comments and newlines.
		if(strncmp("DMA", line, 3) == 0) line += 3;
printf("Line %u: %s\n", curLine, line);

		const char *tokens[MAX_TOKENS];
		const u32 num = tokenize(line, tokens);

		if((res = instMap.at(tokens[0])(num, tokens)) != 0) break;
	}
printf("Parser res: %d\n\n", res);
printf("Bytecode: l%u ", g_progPos);
for(u32 i = 0; i < g_progPos; i++)
{
	printf(" %X", g_progBuf[i]);
}
puts("");
	fclose(asmFh);

	if(g_loopDepth != 0)
	{
		fprintf(stderr, "Error: Reached program end before loop end.\n");
		return ERR_LOOP_WITHOUT_END;
	}
	// TODO: Check if last instruction is DMAEND.

	/*FILE *bcodeFh = fopen(outFile, "wb");
	if(bcodeFh)
	{
		if(fwrite(g_progBuf.get(), 1, g_progPos, bcodeFh) != g_progPos)
		{
			fprintf(stderr, "Failed to write to file.\n");
			res = 2;
		}
		fclose(bcodeFh);
	}
	else
	{
		fprintf(stderr, "Failed to open '%s'.\n", outFile);
		res = 1;
	}*/
	res = makeCHeader(g_progBuf.get(), g_progPos, outFile);

	return res;
}
