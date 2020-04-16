#pragma once

#include "types.h"


#define INBUF_SIZE   (1024)
#define OUTBUF_SIZE  (1024 * 1024)
#define MAX_TOKENS   (13)



int emitAdd(u32 argc, const char *const argv[MAX_TOKENS]);
int emitEnd(u32 argc, const char *const argv[MAX_TOKENS]);
int emitFlushp(u32 argc, const char *const argv[MAX_TOKENS]);
int emitGo(u32 argc, const char *const argv[MAX_TOKENS]);
int emitKill(u32 argc, const char *const argv[MAX_TOKENS]);
int emitLd(u32 argc, const char *const argv[MAX_TOKENS]);
int emitLp(u32 argc, const char *const argv[MAX_TOKENS]);
int emitMov(u32 argc, const char *const argv[MAX_TOKENS]);
int emitNop(u32 argc, const char *const argv[MAX_TOKENS]);
int emitMb(u32 argc, const char *const argv[MAX_TOKENS]);
int emitSev(u32 argc, const char *const argv[MAX_TOKENS]);
int emitSt(u32 argc, const char *const argv[MAX_TOKENS]);
int emitWfe(u32 argc, const char *const argv[MAX_TOKENS]);
int emitWfp(u32 argc, const char *const argv[MAX_TOKENS]);

int dma330as(const char *const inFile, const char *const outFile);
