/*
 * Copyright 2025-2026, Phillip Heller
 *
 * This file is part of Prodigy Reloaded.
 *
 * Prodigy Reloaded is free software: you can redistribute it and/or modify it under the terms of the GNU General
 * Public License as published by the Free Software Foundation, either version 3 of the License, or (at your
 * option) any later version.
 *
 * Prodigy Reloaded is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even
 * the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with Prodigy Reloaded. If not,
 * see <https://www.gnu.org/licenses/>.
 */
/*
 * TBOL Compiler - Code Generation
 */

#ifndef TBOLC_CODEGEN_H
#define TBOLC_CODEGEN_H

#include "../../shared/ast.h"

/*
 * Generate bytecode from the AST.
 * Returns 0 on success, non-zero on error.
 *
 * output_dir: Directory to write .COD file (NULL = current dir)
 * base_name: Base name for output file (without extension)
 */
int codegen_generate(AstNode *ast, const char *output_dir, const char *base_name);

#endif /* TBOLC_CODEGEN_H */
