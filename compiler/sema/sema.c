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
 * TBOL Compiler - Semantic Analysis Implementation
 */

#include "sema.h"
#include "symtab.h"
#include "../diag/diag.h"
#include "../options.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

/* Current procedure being analyzed (for label scoping) */
static Symbol *current_proc = NULL;

/* Error count for this analysis pass */
static int error_count = 0;

/* Limits derived from production bytecode analysis */
#define MAX_PROGRAM_NAME_LEN  8   /* DOS 8.3 filename constraint */
#define MAX_LITERAL_LEN       97  /* Maximum string/hex literal content length */

/*
 * Verb specifications: operand count and type restrictions
 */
typedef enum {
    OP_NONE         = 0,
    OP_LITERAL_OK   = 1 << 0,   /* Literals allowed */
    OP_MUST_BE_VAR  = 1 << 1,   /* Must be lvalue (variable/register) */
} OpFlags;

typedef struct {
    const char *name;
    int min_ops;
    int max_ops;
    OpFlags flags[8];  /* Flags for each operand position */
} VerbSpec;

/* Verb specifications table */
static const VerbSpec verb_specs[] = {
    /* Arithmetic - all operands can be literals except destination */
    {"ADD",       2, 2, {OP_LITERAL_OK, OP_MUST_BE_VAR}},
    {"SUBTRACT",  2, 2, {OP_LITERAL_OK, OP_MUST_BE_VAR}},
    {"MULTIPLY",  2, 2, {OP_LITERAL_OK, OP_MUST_BE_VAR}},
    {"DIVIDE",    2, 3, {OP_LITERAL_OK, OP_MUST_BE_VAR, OP_MUST_BE_VAR}}, /* divisor must be var */

    /* Bitwise */
    {"AND",       2, 2, {OP_LITERAL_OK, OP_LITERAL_OK}},
    {"OR",        2, 2, {OP_LITERAL_OK, OP_LITERAL_OK}},
    {"XOR",       2, 2, {OP_LITERAL_OK, OP_LITERAL_OK}},
    {"TEST",      2, 2, {OP_LITERAL_OK, OP_LITERAL_OK}},

    /* Data Movement */
    {"MOVE",      2, 2, {OP_LITERAL_OK, OP_MUST_BE_VAR}},
    {"SWAP",      2, 2, {OP_MUST_BE_VAR, OP_MUST_BE_VAR}},
    {"FILL",      3, 3, {OP_MUST_BE_VAR, OP_LITERAL_OK, OP_LITERAL_OK}},
    {"CLEAR",     1, 2, {OP_MUST_BE_VAR, OP_MUST_BE_VAR}},
    {"PUSH",      1, 1, {OP_LITERAL_OK}},
    {"POP",       1, 1, {OP_MUST_BE_VAR}},

    /* String - destination must be variable */
    {"STRING",    2, -1, {OP_MUST_BE_VAR}},  /* First is dest, rest are sources (literal ok) */
    {"SUBSTR",    4, 4, {OP_LITERAL_OK, OP_MUST_BE_VAR, OP_LITERAL_OK, OP_LITERAL_OK}},
    {"INSTR",     3, 3, {OP_LITERAL_OK, OP_LITERAL_OK, OP_MUST_BE_VAR}},
    {"UPPERCASE", 1, 1, {OP_MUST_BE_VAR}},  /* Modifies in place */
    {"LENGTH",    2, 2, {OP_LITERAL_OK, OP_MUST_BE_VAR}},
    {"EDIT",      3, 8, {OP_MUST_BE_VAR, OP_LITERAL_OK, OP_LITERAL_OK, OP_LITERAL_OK, OP_LITERAL_OK, OP_LITERAL_OK, OP_LITERAL_OK, OP_LITERAL_OK}},
    {"FORMAT",    3, 3, {OP_LITERAL_OK, OP_LITERAL_OK, OP_MUST_BE_VAR}},
    {"MAKE_FORMAT", 2, -1, {OP_MUST_BE_VAR}},

    /* Control Flow */
    {"EXIT",      0, 1, {OP_LITERAL_OK}},
    {"RETURN",    0, 1, {OP_LITERAL_OK}},
    {"ERROR",     1, 1, {OP_LITERAL_OK}},
    {"TRIGGER_FUNCTION", 1, 1, {OP_LITERAL_OK}},

    /* Navigation/Objects */
    {"NAVIGATE",  1, 2, {OP_LITERAL_OK, OP_LITERAL_OK}},
    {"FETCH",     1, 2, {OP_LITERAL_OK, OP_MUST_BE_VAR}},
    {"LINK",      1, -1, {OP_LITERAL_OK}},
    {"TRANSFER",  1, -1, {OP_LITERAL_OK}},
    {"OPEN_WINDOW", 1, 1, {OP_LITERAL_OK}},
    {"CLOSE_WINDOW", 0, 1, {OP_LITERAL_OK}},
    {"OPEN_ERROR_WINDOW", 1, 2, {OP_LITERAL_OK, OP_LITERAL_OK}},
    {"KILL",      0, 1, {OP_LITERAL_OK}},
    {"PURGE_CACHE", 0, 0, {0}},

    /* File I/O */
    {"OPEN",      2, 2, {OP_LITERAL_OK, OP_LITERAL_OK}},
    {"CLOSE",     1, 1, {OP_LITERAL_OK}},
    {"READ",      2, 3, {OP_LITERAL_OK, OP_MUST_BE_VAR, OP_LITERAL_OK}},
    {"WRITE",     2, 3, {OP_LITERAL_OK, OP_LITERAL_OK, OP_LITERAL_OK}},
    {"NOTE",      2, 2, {OP_LITERAL_OK, OP_MUST_BE_VAR}},
    {"POINT",     2, 2, {OP_LITERAL_OK, OP_LITERAL_OK}},
    {"DELETE",     1, 1, {OP_LITERAL_OK}},

    /* Communications */
    {"CONNECT",   1, 1, {OP_LITERAL_OK}},
    {"DISCONNECT", 0, 0, {0}},
    {"SEND",      1, 2, {OP_LITERAL_OK, OP_LITERAL_OK}},
    {"RECEIVE",   2, 2, {OP_LITERAL_OK, OP_MUST_BE_VAR}},
    {"CANCEL",    0, 1, {OP_LITERAL_OK}},

    /* State Management */
    {"SAVE",      2, 3, {OP_LITERAL_OK, OP_LITERAL_OK, OP_LITERAL_OK}},
    {"RESTORE",   2, 2, {OP_LITERAL_OK, OP_LITERAL_OK}},
    {"RELEASE",   1, 1, {OP_LITERAL_OK}},

    /* Display/Fields */
    {"REFRESH",   0, 0, {0}},
    {"ERASE",     0, 1, {OP_LITERAL_OK}},
    {"SET_CURSOR", 0, 1, {OP_LITERAL_OK}},
    {"SET_ATTRIBUTE", 2, 2, {OP_LITERAL_OK, OP_LITERAL_OK}},
    {"SET_FUNCTION", 2, 4, {OP_LITERAL_OK, OP_LITERAL_OK, OP_LITERAL_OK, OP_LITERAL_OK}},
    {"SET_KEY",   3, 3, {OP_LITERAL_OK, OP_LITERAL_OK, OP_LITERAL_OK}},
    {"DEFINE_FIELD", 6, 7, {OP_LITERAL_OK, OP_LITERAL_OK, OP_LITERAL_OK, OP_LITERAL_OK, OP_LITERAL_OK, OP_LITERAL_OK, OP_LITERAL_OK}},
    {"SOUND",     0, 2, {OP_LITERAL_OK, OP_LITERAL_OK}},

    /* Data Operations */
    {"LOOKUP",    5, 5, {OP_LITERAL_OK, OP_LITERAL_OK, OP_LITERAL_OK, OP_LITERAL_OK, OP_MUST_BE_VAR}},
    {"SORT",      3, 3, {OP_LITERAL_OK, OP_LITERAL_OK, OP_LITERAL_OK}},
    {"WAIT",      0, 0, {0}},
    {"START",     0, 4, {OP_LITERAL_OK, OP_LITERAL_OK, OP_LITERAL_OK, OP_LITERAL_OK}},
    {"STOP",      0, 1, {OP_LITERAL_OK}},

    {NULL, 0, 0, {0}}  /* Sentinel */
};

/* Find verb specification by name */
static const VerbSpec *find_verb_spec(const char *name) {
    for (int i = 0; verb_specs[i].name; i++) {
        if (strcasecmp(verb_specs[i].name, name) == 0) {
            return &verb_specs[i];
        }
    }
    return NULL;
}

/* Check if an expression is a literal */
static bool is_literal(AstNode *node) {
    if (!node) return false;
    switch (node->kind) {
        case AST_LITERAL_STR:
        case AST_LITERAL_NUM:
        case AST_LITERAL_HEX:
            return true;
        default:
            return false;
    }
}

/* Get operand text for error messages */
static const char *operand_text(AstNode *node) {
    if (!node) return "?";
    switch (node->kind) {
        case AST_LITERAL_STR:
            return node->data.str_lit.value;
        case AST_LITERAL_NUM:
        case AST_LITERAL_HEX:
            return node->data.num_lit.text;
        case AST_IDENT:
            return node->data.ident.name;
        default:
            return "?";
    }
}

/*
 * Recursively collect labels from a procedure's AST (including nested DO blocks)
 */
static void collect_labels_recursive(AstNode *node, Symbol *proc, const char *proc_name) {
    if (!node) return;

    /* If this is a label, register it */
    if (node->kind == AST_LABEL) {
        const char *label_name = node->data.label.name;
        if (symtab_label_exists(proc, label_name)) {
            diag_error(node->range.start,
                "duplicate label '%s' in procedure '%s'",
                label_name, proc_name);
            error_count++;
        } else {
            symtab_define_label(label_name, proc, node->range.start);
        }
    }

    /* Recurse into children */
    for (int i = 0; i < node->child_count; i++) {
        collect_labels_recursive(node->children[i], proc, proc_name);
    }
}

/*
 * Pass 1: Collect declarations
 */
static void collect_declarations(AstNode *node) {
    if (!node) return;

    switch (node->kind) {
        case AST_PROGRAM:
            /* Validate program name length */
            if (node->data.program.name &&
                strlen(node->data.program.name) > MAX_PROGRAM_NAME_LEN) {
                diag_error(node->range.start,
                    "PROGRAM name '%s' exceeds %d characters",
                    node->data.program.name, MAX_PROGRAM_NAME_LEN);
                error_count++;
            }
            /* Verify the first procedure is named 'main' */
            for (int i = 0; i < node->child_count; i++) {
                if (node->children[i]->kind == AST_PROC) {
                    if (strcasecmp(node->children[i]->data.proc.name, "main") != 0) {
                        diag_error(node->children[i]->range.start,
                            "first procedure must be named 'main', got '%s'",
                            node->children[i]->data.proc.name);
                        error_count++;
                    }
                    break;  /* Only check the first proc */
                }
            }
            /* Process all children (data sections and procs) */
            for (int i = 0; i < node->child_count; i++) {
                collect_declarations(node->children[i]);
            }
            break;

        case AST_DATA_SECTION: {
            /* Process variable declarations */
            for (int i = 0; i < node->child_count; i++) {
                collect_declarations(node->children[i]);
            }

            /* Register structure group for SAVE/CLEAR with structure names */
            const char *struct_name = node->data.data_section.name;
            if (struct_name && node->child_count > 0) {
                /* Get starting slot from first variable (already registered above) */
                AstNode *first = node->children[0];
                if (first && first->kind == AST_VAR_DECL) {
                    const char *var_name = first->data.var_decl.name;
                    Symbol *var_sym = symtab_lookup_var(var_name);
                    if (var_sym) {
                        int start_slot = VAR_SLOT(var_sym);

                        /* Calculate total slot count (accounting for arrays) */
                        int total_slots = 0;
                        for (int j = 0; j < node->child_count; j++) {
                            AstNode *child = node->children[j];
                            if (child && child->kind == AST_VAR_DECL) {
                                int size = child->data.var_decl.array_size;
                                total_slots += (size > 0) ? size : 1;
                            }
                        }

                        symtab_define_structure(struct_name, start_slot, total_slots);
                    }
                }
            }
            break;
        }

        case AST_VAR_DECL: {
            const char *name = node->data.var_decl.name;
            int array_size = node->data.var_decl.array_size;

            if (symtab_var_exists(name)) {
                /* Duplicate variable - warning */
                diag_warning(node->range.start,
                    "duplicate variable declaration '%s'", name);
            } else {
                symtab_define_var(name, array_size, node->range.start);
            }
            break;
        }

        case AST_PROC: {
            const char *name = node->data.proc.name;

            if (symtab_proc_exists(name)) {
                diag_error(node->range.start,
                    "duplicate procedure '%s'", name);
                error_count++;
            } else {
                Symbol *proc = symtab_define_proc(name, node, node->range.start);

                /* Collect labels in this procedure (recursively) */
                collect_labels_recursive(node, proc, name);
            }
            break;
        }

        default:
            break;
    }
}

/*
 * Validate an expression (variable references, indexed access)
 */
static void validate_expr(AstNode *node) {
    if (!node) return;

    switch (node->kind) {
        case AST_LITERAL_STR:
            if (node->data.str_lit.length > MAX_LITERAL_LEN) {
                diag_error(node->range.start,
                    "string literal exceeds %d characters (%d)",
                    MAX_LITERAL_LEN, node->data.str_lit.length);
                error_count++;
            }
            break;

        case AST_LITERAL_HEX:
            if (node->data.num_lit.text &&
                strlen(node->data.num_lit.text) > MAX_LITERAL_LEN) {
                diag_error(node->range.start,
                    "hex literal exceeds %d characters",
                    MAX_LITERAL_LEN);
                error_count++;
            }
            break;

        case AST_IDENT: {
            const char *name = node->data.ident.name;
            /* Check for variable or structure group name */
            if (!symtab_var_exists(name) && !symtab_lookup_structure(name)) {
                if (g_options.compat_errors) {
                    diag_error(node->range.start,
                        "Identifier %s not defined", name);
                } else {
                    diag_error(node->range.start,
                        "undefined variable '%s'", name);
                }
                error_count++;
            }
            break;
        }

        case AST_INDEXED: {
            /* Check that base can be indexed */
            AstNode *base = node->children[0];
            AstNode *index = node->children[1];

            if (base) {
                /* Note: PEV/GEV indexing rules are unclear.  The original
                 * compiler may reject indexed PEV/GEV but we have not
                 * verified this, so we accept them for now. */

                /* Validate base expression */
                validate_expr(base);
            }

            /* Validate index expression */
            validate_expr(index);
            break;
        }

        /* Recursively validate children for compound expressions */
        case AST_COMPARE:
        case AST_LOGIC_AND:
        case AST_LOGIC_OR:
            for (int i = 0; i < node->child_count; i++) {
                validate_expr(node->children[i]);
            }
            break;

        default:
            break;
    }
}

/*
 * Validate a verb statement
 */
static void validate_verb(AstNode *node) {
    if (!node || node->kind != AST_VERB_STMT) return;

    const char *verb_name = node->data.call.name;
    const VerbSpec *spec = find_verb_spec(verb_name);

    /* Check operand count */
    int op_count = node->child_count;

    if (spec) {
        if (op_count < spec->min_ops) {
            if (g_options.compat_errors) {
                diag_error(node->range.start,
                    "Number of operands differs from expected for '%s'",
                    verb_name);
            } else {
                diag_error(node->range.start,
                    "%s requires at least %d operand(s), got %d",
                    verb_name, spec->min_ops, op_count);
            }
            error_count++;
            return;
        }

        if (spec->max_ops >= 0 && op_count > spec->max_ops) {
            if (g_options.compat_errors) {
                diag_error(node->range.start,
                    "Number of operands differs from expected for '%s'",
                    verb_name);
            } else {
                diag_error(node->range.start,
                    "%s accepts at most %d operand(s), got %d",
                    verb_name, spec->max_ops, op_count);
            }
            error_count++;
            return;
        }

        /* Check operand type restrictions */
        for (int i = 0; i < op_count && i < 8; i++) {
            AstNode *operand = node->children[i];
            OpFlags flags = spec->flags[i];

            /* Validate the operand expression first */
            validate_expr(operand);

            /* Check type restrictions */
            if ((flags & OP_MUST_BE_VAR) && is_literal(operand)) {
                if (g_options.compat_errors) {
                    diag_error(operand->range.start,
                        "Illegal operand '%s'", operand_text(operand));
                } else {
                    diag_error(operand->range.start,
                        "operand %d of %s must be a variable, not a literal",
                        i + 1, verb_name);
                }
                error_count++;
            }
        }
        /* MOVE-specific: validate array/struct operand compatibility */
        if (strcasecmp(verb_name, "MOVE") == 0 && op_count == 2) {
            AstNode *src = node->children[0];
            AstNode *dst = node->children[1];
            if (src->kind == AST_IDENT && dst->kind == AST_IDENT) {
                Symbol *ssym = symtab_lookup_var(src->data.ident.name);
                Symbol *dsym = symtab_lookup_var(dst->data.ident.name);
                int ssize = ssym ? ssym->data.var.array_size : 0;
                int dsize = dsym ? dsym->data.var.array_size : 0;

                if (ssize > 0 && dsize > 0 && ssize != dsize) {
                    diag_error(node->range.start,
                        "MOVE with arrays of different sizes (%d vs %d)",
                        ssize, dsize);
                    error_count++;
                } else if ((ssize > 0) != (dsize > 0)) {
                    /* One is array, other is scalar */
                    diag_error(node->range.start,
                        "MOVE cannot mix array and scalar operands");
                    error_count++;
                }
            }
        }

        /* SWAP-specific: reject array operands */
        if (strcasecmp(verb_name, "SWAP") == 0 && op_count == 2) {
            for (int i = 0; i < 2; i++) {
                AstNode *op = node->children[i];
                if (op->kind == AST_IDENT) {
                    Symbol *sym = symtab_lookup_var(op->data.ident.name);
                    if (sym && sym->data.var.array_size > 0) {
                        diag_error(op->range.start,
                            "SWAP does not support array operands");
                        error_count++;
                        break;
                    }
                }
            }
        }
    } else {
        /* Unknown verb - just validate operands */
        for (int i = 0; i < op_count; i++) {
            validate_expr(node->children[i]);
        }
    }
}

/*
 * Validate a GOTO statement
 */
static void validate_goto(AstNode *node) {
    if (!node || node->kind != AST_GOTO) return;

    const char *label = node->data.goto_stmt.label;

    if (!current_proc) {
        diag_error(node->range.start, "GOTO outside of procedure");
        error_count++;
        return;
    }

    if (!symtab_label_exists(current_proc, label)) {
        if (g_options.compat_errors) {
            diag_error(node->range.start, "Undefined label: %s", label);
        } else {
            diag_error(node->range.start,
                "undefined label '%s' in procedure '%s'",
                label, current_proc->name);
        }
        error_count++;
    }
}

/*
 * Validate a GOTO_DEPENDING_ON statement
 */
static void validate_goto_depending_on(AstNode *node) {
    if (!node || node->kind != AST_GOTO_DEPENDING_ON) return;

    if (!current_proc) {
        diag_error(node->range.start, "GOTO_DEPENDING_ON outside of procedure");
        error_count++;
        return;
    }

    /* First child is the selector expression */
    if (node->child_count > 0) {
        validate_expr(node->children[0]);
    }

    /* Remaining children are label references */
    for (int i = 1; i < node->child_count; i++) {
        AstNode *label_node = node->children[i];
        if (label_node && label_node->kind == AST_IDENT) {
            const char *label = label_node->data.ident.name;
            if (!symtab_label_exists(current_proc, label)) {
                if (g_options.compat_errors) {
                    diag_error(label_node->range.start, "Undefined label: %s", label);
                } else {
                    diag_error(label_node->range.start,
                        "undefined label '%s' in procedure '%s'",
                        label, current_proc->name);
                }
                error_count++;
            }
        }
    }
}

/*
 * Validate a procedure call
 */
static void validate_proc_call(AstNode *node) {
    if (!node || node->kind != AST_PROC_CALL) return;

    const char *name = node->data.call.name;

    if (!symtab_proc_exists(name)) {
        /* Allow undefined procedures - original compiler accepts them
         * and codegen emits CALL with offset 0 */
        diag_warning(node->range.start,
            "undefined procedure '%s' (will emit CALL with offset 0)", name);
    } else if (current_proc) {
        /* Check that called proc is defined after the calling proc.
         * CALL uses relative forward offsets — backward calls are not valid. */
        Symbol *target = symtab_lookup_proc(name);
        if (target && target->data.proc.order <= current_proc->data.proc.order) {
            diag_error(node->range.start,
                "procedure '%s' must be defined after '%s' (no backward calls)",
                name, current_proc->name);
            error_count++;
        }
    }

    /* Validate arguments */
    for (int i = 0; i < node->child_count; i++) {
        validate_expr(node->children[i]);
    }
}

/*
 * Pass 2: Validate references in a statement
 */
static void validate_statement(AstNode *node) {
    if (!node) return;

    switch (node->kind) {
        case AST_VERB_STMT:
            validate_verb(node);
            break;

        case AST_GOTO:
            validate_goto(node);
            break;

        case AST_GOTO_DEPENDING_ON:
            validate_goto_depending_on(node);
            break;

        case AST_PROC_CALL:
            validate_proc_call(node);
            break;

        case AST_IF_STMT:
        case AST_WHILE_STMT:
            /* Validate condition */
            if (node->child_count > 0) {
                validate_expr(node->children[0]);
            }
            /* Validate body statements */
            for (int i = 1; i < node->child_count; i++) {
                validate_statement(node->children[i]);
            }
            break;

        case AST_DO_BLOCK:
            for (int i = 0; i < node->child_count; i++) {
                validate_statement(node->children[i]);
            }
            break;

        case AST_LABEL:
            /* Labels are already collected in pass 1 */
            break;

        default:
            break;
    }
}

/*
 * Pass 2: Validate references
 */
static void validate_references(AstNode *node) {
    if (!node) return;

    switch (node->kind) {
        case AST_PROGRAM:
            for (int i = 0; i < node->child_count; i++) {
                validate_references(node->children[i]);
            }
            break;

        case AST_PROC: {
            /* Set current procedure for label scoping */
            current_proc = symtab_lookup_proc(node->data.proc.name);

            /* Validate all statements in the procedure */
            for (int i = 0; i < node->child_count; i++) {
                validate_statement(node->children[i]);
            }

            current_proc = NULL;
            break;
        }

        default:
            break;
    }
}

/*
 * Main entry point
 */
int sema_analyze(AstNode *ast) {
    if (!ast) return -1;

    error_count = 0;
    current_proc = NULL;

    /* Clean up any leftover symbol table from a previous parse
     * (LSP may skip cleanup when collect_symbols is set) */
    symtab_cleanup();
    symtab_init();

    /* Pass 1: Collect all declarations */
    collect_declarations(ast);

    /* Pass 2: Validate all references */
    validate_references(ast);

    /* Dump symbol table if verbose */
    if (g_options.verbose) {
        symtab_dump();
    }

    return error_count > 0 ? 1 : 0;
}
