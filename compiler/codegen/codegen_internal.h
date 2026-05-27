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
 * TBOL Compiler - Code Generation Internal Header
 *
 * Shared declarations between codegen.c, codegen_operand.c,
 * codegen_control.c, and codegen_verbs.c.
 */

#ifndef TBOLC_CODEGEN_INTERNAL_H
#define TBOLC_CODEGEN_INTERNAL_H

#include "../../shared/bytecode/opcodes.h"
#include "../../shared/ast.h"
#include "emit.h"
#include "../sema/symtab.h"
#include "../diag/diag.h"
#include "../options.h"

/* Shared state: current procedure name for label resolution */
extern const char *current_proc_name;

/* --- codegen_operand.c --- */
int compute_operand_value(AstNode *node);
int compute_range_end_value(AstNode *node);
bool needs_extended_encoding(AstNode *node);
void emit_operand(AstNode *node, bool use_extended);

/* --- codegen.c --- */
bool check_complex_mode(AstNode **operands, int count);
uint8_t compute_mode_byte(AstNode **operands, int count);
void emit_instruction(uint8_t opcode, AstNode **operands, int count);
void emit_var_instruction(uint8_t opcode, AstNode **operands, int count);
void emit_range_instruction(uint8_t opcode, AstNode **operands, int count);
void emit_struct_clear_save(uint8_t opcode, int start_slot, int count);
void emit_cond_jump(uint8_t opcode, AstNode *left, AstNode *right, const char *target);
void emit_jump(const char *target);
void define_label(const char *name);
int lookup_label(const char *name);
void define_proc(const char *name);
int lookup_proc(const char *name);
void gen_statement(AstNode *node);

/* --- codegen_control.c --- */
void control_reset_counters(void);
void gen_if(AstNode *node);
void gen_while(AstNode *node);
void gen_do_block(AstNode *node);
void gen_goto(AstNode *node);

/* --- codegen_verbs.c --- */
void gen_verb(AstNode *node);
void gen_proc_call(AstNode *node);
void gen_goto_depending_on(AstNode *node);

#endif /* TBOLC_CODEGEN_INTERNAL_H */
