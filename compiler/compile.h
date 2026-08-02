/*
 * compile.h - TBOL compiler library interface
 *
 * Provides a callable compilation function for use by tools
 * like the decompiler's round-trip verification.
 */
#ifndef TBOLC_COMPILE_H
#define TBOLC_COMPILE_H

/*
 * Compile a TBOL source file to a .cod bytecode file.
 *
 * src_path:       path to the .src input file
 * out_dir:        directory for the output .cod file (NULL = same dir as source)
 * include_paths:  array of -I include directories
 * include_count:  number of include paths
 *
 * Returns 0 on success, non-zero on error.
 * Error messages are suppressed (not printed to stderr).
 */
int tbolc_compile_file(const char *src_path, const char *out_dir,
                        const char **include_paths, int include_count);

/*
 * Parse a TBOL source file and return the AST (no codegen).
 * Returns the root AST node on success, NULL on parse error.
 * Caller must call ast_free() on the returned node.
 *
 * include_paths: -I directories for COPY resolution
 */
struct AstNode *tbolc_parse_file(const char *src_path,
                                  const char **include_paths, int include_count);

#endif
