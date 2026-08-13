/*
 * verify.h - Speculative round-trip verification
 *
 * Iteratively refines decompiler output by compiling it with the
 * statically-linked tbolc compiler, comparing bytecodes, and ratcheting
 * down pattern expressiveness for CJ instructions that don't round-trip.
 */
#ifndef VERIFY_H
#define VERIFY_H

#include "bytecode/ir.h"
#include "bytecode/proc.h"
#include "bytecode/gev.h"
#include "operand_fmt.h"
#include "structure.h"
#include <stdio.h>

/*
 * Emit source with speculative round-trip verification.
 *
 * Uses the statically-linked compiler to iteratively refine the output
 * until it round-trips to identical bytecode.
 *
 * include_paths/count: -I paths for the compiler
 * original_cod: path to the original .cod file (for comparison)
 * Returns 0 on success (round-trip achieved), 1 if converged without match.
 */
int emit_verified(FILE *out, Program *prog, ProcList *procs, GEVTable *gev,
                  const char **include_paths, int include_path_count,
                  const char *original_cod, int max_iter,
                  const char *input_path);

/*
 * Compile a source file and compare its bytecode to the original.
 * Reports IDENTICAL or MISMATCH to stderr.
 * Returns 0 if identical, 1 if different or error.
 */
int verify_roundtrip(const char *src_path, const char *original_cod,
                     const char **include_paths, int include_path_count);

/*
 * Directory for scratch temp files, without a trailing separator. On
 * Windows this is the OS temp path (GetTempPath); elsewhere $TMPDIR if
 * set, otherwise /tmp. The returned pointer is owned by the callee.
 */
const char *tbol_tmp_dir(void);

#endif
