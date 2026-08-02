/*
 * baseline.h - Flat baseline source emission
 */
#ifndef BASELINE_H
#define BASELINE_H

#include "bytecode/ir.h"
#include "bytecode/decode.h"
#include "bytecode/gev.h"
#include "bytecode/proc.h"
#include "structure.h"
#include <stdio.h>

/* Emit a complete TBOL source file from decoded bytecode.
 * If mt is non-NULL, per-CJ mode overrides are applied during
 * structural emission (for speculative refinement).
 * input_path is the on-disk source of `prog`; if non-NULL it's stamped
 * into the source-header comment block (basename + md5sum). */
void emit_baseline(FILE *out, Program *prog, ProcList *procs, GEVTable *gev,
                   ModeTable *mt, const char *input_path);

#endif
