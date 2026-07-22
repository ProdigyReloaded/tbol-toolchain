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
 * TBOL Compiler - Code Generation Implementation
 */

#include "codegen.h"
#include "codegen_internal.h"
#include "sdb.h"
#include "../sema/symtab.h"
#include "../lexer/preproc.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <ctype.h>
#include <time.h>

/* Current procedure name for label resolution */
const char *current_proc_name = NULL;

/* Label tracking for current procedure */
#define MAX_LABELS 256
typedef struct {
    const char *name;
    int offset;
} LabelDef;
static LabelDef labels[MAX_LABELS];
static int label_count = 0;

/* Procedure offsets */
#define MAX_PROCS 128
typedef struct {
    const char *name;
    int offset;
} ProcDef;
static ProcDef proc_defs[MAX_PROCS];
static int proc_count = 0;

/*
 * Define a label at current position and fix up any forward references
 */
void define_label(const char *name) {
    int offset = emit_get_offset();

    /* Record the label (strdup to avoid dangling pointers from stack) */
    if (label_count >= MAX_LABELS) {
        SourceLoc loc = {NULL, 0, 0};
        diag_error(loc, "too many labels (max %d)", MAX_LABELS);
        return;
    }
    labels[label_count].name = strdup(name);
    labels[label_count].offset = offset;
    label_count++;

    /* Fix up any forward references to this label */
    ForwardRef *refs = emit_get_forward_refs();
    int ref_count = emit_get_forward_ref_count();

    for (int i = 0; i < ref_count; i++) {
        /* Match label name and procedure scope */
        if (refs[i].label && strcasecmp(refs[i].label, name) == 0) {
            /* Check if this is a local label (proc matches current_proc_name) */
            if ((refs[i].proc == NULL && current_proc_name == NULL) ||
                (refs[i].proc && current_proc_name && strcasecmp(refs[i].proc, current_proc_name) == 0)) {
                /* Calculate relative offset from end of jump instruction */
                int rel = offset - (refs[i].patch_offset + 2);
                emit_patch_word_be(refs[i].patch_offset, (int16_t)rel);
                /* Mark as resolved by freeing and clearing the label */
                free((char *)refs[i].label);
                refs[i].label = NULL;
            }
        }
    }
}

/*
 * Look up a label offset
 */
int lookup_label(const char *name) {
    for (int i = 0; i < label_count; i++) {
        if (strcasecmp(labels[i].name, name) == 0) {
            return labels[i].offset;
        }
    }
    return -1;
}

/*
 * Define a procedure at current position
 */
void define_proc(const char *name) {
    if (proc_count >= MAX_PROCS) {
        SourceLoc loc = {NULL, 0, 0};
        diag_error(loc, "too many procedures (max %d)", MAX_PROCS);
        return;
    }
    proc_defs[proc_count].name = name;
    proc_defs[proc_count].offset = emit_get_offset();
    proc_count++;
}

/*
 * Look up a procedure offset
 */
int lookup_proc(const char *name) {
    for (int i = 0; i < proc_count; i++) {
        if (strcasecmp(proc_defs[i].name, name) == 0) {
            return proc_defs[i].offset;
        }
    }
    return -1;
}

/*
 * Check if any operands need complex mode
 */
bool check_complex_mode(AstNode **operands, int count) {
    for (int i = 0; i < count; i++) {
        if (needs_extended_encoding(operands[i])) {
            return true;
        }
    }
    return false;
}

/*
 * Compute mode byte for complex instructions
 * Mode byte uses high-bit-first ordering: bit 7 for operand 0, bit 6 for operand 1, etc.
 */
uint8_t compute_mode_byte(AstNode **operands, int count) {
    uint8_t mode = 0;
    for (int i = 0; i < count && i < 8; i++) {
        if (needs_extended_encoding(operands[i])) {
            mode |= (1 << (7 - i));
        }
    }
    return mode;
}

/*
 * Emit a simple instruction with fixed operands
 */
void emit_instruction(uint8_t opcode, AstNode **operands, int count) {
    bool complex = check_complex_mode(operands, count);

    if (complex) {
        emit_byte(opcode | OP_COMPLEX);
        emit_byte(compute_mode_byte(operands, count));
    } else {
        emit_byte(opcode);
    }

    for (int i = 0; i < count; i++) {
        bool extended = needs_extended_encoding(operands[i]);
        emit_operand(operands[i], extended);
    }
}

/*
 * Emit a range instruction (SAVE_RANGE, CLEAR_RANGE) where the last
 * operand uses array-end resolution.  For array variables, the last
 * operand encodes as base_slot + array_size - 1 (the last element).
 */
void emit_range_instruction(uint8_t opcode, AstNode **operands, int count) {
    /* For range instructions (SAVE_RANGE, CLEAR_RANGE), the last operand
     * may be an indexed expression like &1(P5).  Use the same extended
     * encoding logic as regular operands - don't special-case it.
     *
     * For non-indexed last operands, compute_range_end_value resolves
     * array/structure names to their end slot value. */
    AstNode *last = operands[count - 1];
    bool last_is_indexed = (last && last->kind == AST_INDEXED);

    bool complex = check_complex_mode(operands, count);

    /* If the last operand isn't indexed, use range-end resolution */
    int end_val = 0;
    bool end_extended = false;
    if (!last_is_indexed) {
        end_val = compute_range_end_value(last);
        end_extended = (end_val >= 256);
        if (end_extended) complex = true;
    }

    if (complex) {
        emit_byte(opcode | OP_COMPLEX);
        uint8_t mode = 0;
        for (int i = 0; i < count && i < 8; i++) {
            if (i == count - 1 && !last_is_indexed) {
                if (end_extended)
                    mode |= (1 << (7 - i));
            } else {
                if (needs_extended_encoding(operands[i]))
                    mode |= (1 << (7 - i));
            }
        }
        emit_byte(mode);
    } else {
        emit_byte(opcode);
    }

    /* Emit all operands except the last normally */
    for (int i = 0; i < count - 1; i++) {
        emit_operand(operands[i], needs_extended_encoding(operands[i]));
    }

    /* Emit the last operand */
    if (last_is_indexed) {
        /* Indexed operand - use standard emit_operand */
        emit_operand(last, needs_extended_encoding(last));
    } else if (end_extended) {
        emit_byte((uint8_t)(end_val >> 8));
        emit_byte((uint8_t)(end_val & 0xFF));
    } else {
        emit_byte((uint8_t)end_val);
    }
}

/*
 * Emit CLEAR or SAVE with structure name (slot, count)
 * Handles extended encoding for RDA slots 158-221
 */
void emit_struct_clear_save(uint8_t opcode, int start_slot, int count) {
    bool extended = (start_slot > 157);

    if (extended) {
        emit_byte(opcode | OP_COMPLEX);
        emit_byte(0x80);  /* Mode byte: first operand needs extended encoding */
    } else {
        emit_byte(opcode);
    }

    emit_byte((uint8_t)count);

    if (extended) {
        /* Extended RDA encoding: 256 + (slot - 158) = 0x100 + (slot - 158) */
        uint16_t ext_val = 256 + (start_slot - 158);
        emit_byte((ext_val >> 8) & 0xFF);  /* High byte first (big-endian) */
        emit_byte(ext_val & 0xFF);
    } else {
        /* Simple RDA encoding: 34 + slot */
        emit_byte(0x22 + start_slot);
    }
}

/*
 * Emit a variable-operand instruction
 */
void emit_var_instruction(uint8_t opcode, AstNode **operands, int count) {
    bool complex = check_complex_mode(operands, count);

    if (complex) {
        emit_byte(opcode | OP_COMPLEX);
        /* For var-arg instructions, mode bytes may need multiple bytes */
        int mode_bytes = (count + 7) / 8;
        for (int b = 0; b < mode_bytes; b++) {
            uint8_t mode = 0;
            for (int i = 0; i < 8 && (b * 8 + i) < count; i++) {
                if (needs_extended_encoding(operands[b * 8 + i])) {
                    /* High-bit-first ordering: bit 7 for first operand in byte */
                    mode |= (1 << (7 - i));
                }
            }
            emit_byte(mode);
        }
    } else {
        emit_byte(opcode);
    }

    emit_byte(count);  /* Operand count */

    for (int i = 0; i < count; i++) {
        bool extended = needs_extended_encoding(operands[i]);
        emit_operand(operands[i], extended);
    }
}

/*
 * Emit a conditional jump
 */
void emit_cond_jump(uint8_t opcode, AstNode *left, AstNode *right, const char *target) {
    AstNode *operands[2] = {left, right};

    /* Column-level span for this comparison: from the left operand's start to
     * the right operand's end. Lets a composite condition (A > B OR C = D) map
     * each comparison instruction to its own source columns. */
    if (g_options.emit_sdb && left && right)
        sdb_mark(emit_get_offset(),
                 (SourceRange){ left->range.start, right->range.end });

    bool complex = check_complex_mode(operands, 2);

    if (complex) {
        emit_byte(opcode | OP_COMPLEX);
        emit_byte(compute_mode_byte(operands, 2));
    } else {
        emit_byte(opcode);
    }

    emit_operand(left, needs_extended_encoding(left));
    emit_operand(right, needs_extended_encoding(right));

    /* Emit placeholder offset, record for fixup */
    int patch_pos = emit_get_offset();
    emit_word_be(0);

    /* Check if label is already defined */
    int target_offset = lookup_label(target);
    if (target_offset >= 0) {
        /* Calculate relative offset from end of instruction */
        int rel = target_offset - (patch_pos + 2);
        emit_patch_word_be(patch_pos, (int16_t)rel);
    } else {
        /* Forward reference */
        emit_add_forward_ref(patch_pos, target, current_proc_name);
    }
}

/*
 * Emit an unconditional jump
 */
void emit_jump(const char *target) {
    emit_byte(OP_JUMP);

    int patch_pos = emit_get_offset();
    emit_word_be(0);

    int target_offset = lookup_label(target);
    if (target_offset >= 0) {
        int rel = target_offset - (patch_pos + 2);
        emit_patch_word_be(patch_pos, (int16_t)rel);
    } else {
        emit_add_forward_ref(patch_pos, target, current_proc_name);
    }
}

/*
 * Generate code for a statement
 */
void gen_statement(AstNode *node) {
    if (!node) return;

    /* Line-table entry: the next instruction emitted for this statement begins
     * at the current offset. Labels and DEFINEs emit no code, so a following
     * statement overwrites the entry at the same address (sdb_mark handles it). */
    if (g_options.emit_sdb)
        sdb_mark(emit_get_offset(), node->range);

    switch (node->kind) {
        case AST_LABEL:
            define_label(node->data.label.name);
            break;

        case AST_VERB_STMT:
            gen_verb(node);
            break;

        case AST_PROC_CALL:
            gen_proc_call(node);
            break;

        case AST_GOTO:
            gen_goto(node);
            break;

        case AST_GOTO_DEPENDING_ON:
            gen_goto_depending_on(node);
            break;

        case AST_IF_STMT:
            gen_if(node);
            break;

        case AST_WHILE_STMT:
            gen_while(node);
            break;

        case AST_DO_BLOCK:
            gen_do_block(node);
            break;

        case AST_DEFINE:
            /* DEFINEs are handled at compile time, no code needed */
            break;

        default:
            break;
    }
}

/*
 * Generate code for a procedure
 */
static void gen_proc(AstNode *node) {
    const char *name = node->data.proc.name;

    /* Record procedure position */
    define_proc(name);

    /* Set current procedure for label scoping */
    current_proc_name = name;
    /* Free strdup'd label names from previous procedure */
    for (int i = 0; i < label_count; i++) {
        free((char *)labels[i].name);
    }
    label_count = 0;  /* Clear labels for new procedure */

    /* Generate code for statements */
    for (int i = 0; i < node->child_count; i++) {
        gen_statement(node->children[i]);
    }

    /* Implicit RETURN at end of procedure */
    emit_byte(OP_RETURN);

    /* Fix up forward references within this procedure */
    ForwardRef *refs = emit_get_forward_refs();
    int ref_count = emit_get_forward_ref_count();

    for (int i = 0; i < ref_count; i++) {
        /* Skip already-resolved references (label set to NULL) */
        if (!refs[i].label) continue;

        if (refs[i].proc && strcasecmp(refs[i].proc, name) == 0) {
            /* This is a label within this procedure */
            int target_offset = lookup_label(refs[i].label);
            if (target_offset >= 0) {
                int rel = target_offset - (refs[i].patch_offset + 2);
                emit_patch_word_be(refs[i].patch_offset, (int16_t)rel);
            }
        }
    }

    current_proc_name = NULL;
}

/*
 * Emit file header - placeholder bytes, fixed up later
 */
static int header_file_size_offset;
static int header_code_start_offset;

static void emit_header(const char *program_name) {
    time_t now = time(NULL);
    struct tm *tm = localtime(&now);

    char date_str[32];
    snprintf(date_str, sizeof(date_str), "%02d/%02d/%02d %02d:%02d",
             tm->tm_mon + 1, tm->tm_mday, tm->tm_year % 100,
             tm->tm_hour, tm->tm_min);

    int name_len = strlen(program_name);
    int date_len = strlen(date_str);
    /* Header: 6 control bytes + name + date + version(5) + marker(1) */
    int code_start = 6 + name_len + date_len + 5 + 1;

    /* Header format: file_size(2), code_start(2), name_len(2), name, date, version, marker */
    header_file_size_offset = emit_get_offset();
    emit_word_be(0x0000);                    /* Placeholder: file size (16-bit BE) */
    header_code_start_offset = emit_get_offset();
    emit_word_be((uint16_t)code_start);      /* Code start offset (16-bit BE) */
    emit_word_be((uint16_t)name_len);        /* Name length (16-bit BE) */

    emit_bytes((const uint8_t *)program_name, name_len);
    emit_bytes((const uint8_t *)date_str, date_len);
    emit_bytes((const uint8_t *)"04.21", 5);
    emit_byte(0x30);                         /* Marker */
}

static void fixup_header_file_size(int file_size) {
    /* Patch 16-bit big-endian file size */
    emit_patch_byte(header_file_size_offset, (uint8_t)(file_size >> 8));
    emit_patch_byte(header_file_size_offset + 1, (uint8_t)(file_size & 0xFF));
}

/*
 * Symbol-table export for .sdb: every DATA variable becomes an RDA slot symbol.
 */
static void sdb_collect_var(const Symbol *sym, void *ctx) {
    (void)ctx;
    int len = sym->data.var.array_size > 0 ? sym->data.var.array_size : -1;
    sdb_add_symbol(sym->name, "RDA", VAR_SLOT(sym), len);
}

/* Export DEFINEs that name a global/partition variable (value #N -> GEV N,
 * &N -> PEV N) so the debugger can show e.g. SYS_TTX_PHONE as GEV 19. */
static void sdb_collect_define(const char *name, const char *value, void *ctx) {
    (void)ctx;
    while (*value == ' ' || *value == '\t') value++;
    if ((value[0] == '#' || value[0] == '&') && isdigit((unsigned char)value[1]))
        sdb_add_symbol(name, value[0] == '#' ? "GEV" : "PEV",
                       atoi(value + 1), -1);
}

/* Write "<program>.sdb" beside the .cod, deriving its path from the .cod path. */
static void write_sdb(const char *cod_path, const char *program_name) {
    char sdb_path[4096];
    snprintf(sdb_path, sizeof(sdb_path), "%s", cod_path);
    char *dot = strrchr(sdb_path, '.');
    if (dot) strcpy(dot, ".sdb");
    else     strncat(sdb_path, ".sdb", sizeof(sdb_path) - strlen(sdb_path) - 1);

    const char *cod_name = strrchr(cod_path, '/');
    cod_name = cod_name ? cod_name + 1 : cod_path;

    symtab_foreach_var(sdb_collect_var, NULL);
    preproc_foreach_define(sdb_collect_define, NULL);

    if (sdb_write(sdb_path, program_name, cod_name) != 0) {
        diag_error((SourceLoc){NULL, 0, 0}, "error writing .sdb file '%s'", sdb_path);
    } else if (g_options.verbose) {
        fprintf(stderr, "Wrote %s\n", sdb_path);
    }
    sdb_cleanup();
}

/*
 * Main code generation entry point
 */
int codegen_generate(AstNode *ast, const char *output_dir, const char *base_name) {
    if (!ast || ast->kind != AST_PROGRAM) {
        return -1;
    }

    emit_init();
    label_count = 0;
    proc_count = 0;
    control_reset_counters();

    if (g_options.emit_sdb) {
        sdb_reset();
    }

    /* Emit header */
    emit_header(ast->data.program.name);

    /* Code section starts immediately after the header; line-table addresses
     * are relative to it (matching how a loader addresses the code section). */
    if (g_options.emit_sdb) {
        sdb_set_code_base(emit_get_offset());
    }

    /* Generate code for each procedure */
    for (int i = 0; i < ast->child_count; i++) {
        AstNode *child = ast->children[i];
        if (child && child->kind == AST_PROC) {
            gen_proc(child);
        }
    }

    /* Fix up any remaining forward references (procedure calls) */
    ForwardRef *refs = emit_get_forward_refs();
    int ref_count = emit_get_forward_ref_count();

    for (int i = 0; i < ref_count; i++) {
        /* Skip already-resolved references */
        if (!refs[i].label) continue;

        if (!refs[i].proc) {
            /* Procedure call forward reference -- relative offset */
            int target_offset = lookup_proc(refs[i].label);
            if (target_offset >= 0) {
                int rel = target_offset - (refs[i].patch_offset + 2);
                emit_patch_word_be(refs[i].patch_offset, (int16_t)rel);
            }
        }
    }

    /* code_start points to the first procedure (main), which is always
     * emitted first, immediately after the header. emit_header() already
     * sets code_start correctly. */

    /* Fix up header with final file size */
    int file_size;
    emit_get_buffer(&file_size);
    fixup_header_file_size(file_size);

    /* Write output file */
    char output_path[4096];
    if (output_dir && output_dir[0]) {
        snprintf(output_path, sizeof(output_path), "%s/%s.cod", output_dir, base_name);
    } else {
        snprintf(output_path, sizeof(output_path), "%s.cod", base_name);
    }

    /* Convert filename to lowercase */
    char *slash = strrchr(output_path, '/');
    for (char *p = slash ? slash + 1 : output_path; *p; p++)
        *p = tolower((unsigned char)*p);

    FILE *fp = fopen(output_path, "wb");
    if (!fp) {
        diag_error((SourceLoc){NULL, 0, 0}, "cannot create output file '%s'", output_path);
        emit_cleanup();
        return -1;
    }

    int result = emit_write_to_file(fp);
    fclose(fp);

    if (result != 0) {
        diag_error((SourceLoc){NULL, 0, 0}, "error writing output file");
    } else if (g_options.verbose) {
        int size;
        emit_get_buffer(&size);
        fprintf(stderr, "Wrote %s (%d bytes)\n", output_path, size);
    }

    /* Emit source-debug info (.sdb) alongside the .cod. Symbols are read from
     * the still-live symbol table (the caller cleans it up after we return). */
    if (result == 0 && g_options.emit_sdb) {
        write_sdb(output_path, ast->data.program.name);
    }

    /* Free remaining strdup'd label names */
    for (int i = 0; i < label_count; i++) {
        free((char *)labels[i].name);
    }
    label_count = 0;

    emit_cleanup();
    return result;
}
