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
 * TBOL Abstract Syntax Tree Implementation
 * Shared AST representation for TBOL tools
 */

#include "ast.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#define INITIAL_CHILDREN_CAPACITY 4

/* Create a new AST node */
AstNode *ast_new(AstNodeKind kind, SourceLoc loc) {
    AstNode *node = calloc(1, sizeof(AstNode));
    if (!node) return NULL;

    node->kind = kind;
    /* Copy SourceLoc but own the filename string */
    node->range.start.filename = loc.filename ? strdup(loc.filename) : NULL;
    node->range.start.line = loc.line;
    node->range.start.column = loc.column;
    node->range.end = node->range.start;  /* Initially same as start */
    node->children = NULL;
    node->child_count = 0;
    node->child_capacity = 0;

    return node;
}

/* Free an AST node and all children */
void ast_free(AstNode *node) {
    if (!node) return;

    /* Free children first */
    for (int i = 0; i < node->child_count; i++) {
        ast_free(node->children[i]);
    }
    free(node->children);

    /* Free node-specific data */
    switch (node->kind) {
        case AST_PROGRAM:
            free(node->data.program.name);
            break;
        case AST_DATA_SECTION:
            free(node->data.data_section.name);
            break;
        case AST_VAR_DECL:
            free(node->data.var_decl.name);
            free(node->data.var_decl.original_text);
            break;
        case AST_DEFINE:
            free(node->data.define.name);
            free(node->data.define.original_text);
            free(node->data.define.value);
            break;
        case AST_COPY:
            free(node->data.copy.filename);
            break;
        case AST_PROC:
            free(node->data.proc.name);
            free(node->data.proc.original_text);
            break;
        case AST_LABEL:
            free(node->data.label.name);
            free(node->data.label.original_text);
            break;
        case AST_VERB_STMT:
        case AST_PROC_CALL:
            free(node->data.call.name);
            break;
        case AST_GOTO:
            free(node->data.goto_stmt.label);
            break;
        case AST_LITERAL_STR:
            free(node->data.str_lit.value);
            break;
        case AST_LITERAL_NUM:
        case AST_LITERAL_HEX:
            free(node->data.num_lit.text);
            break;
        case AST_IDENT:
            free(node->data.ident.name);
            break;
        default:
            break;
    }

    /* Free SourceLoc filenames - they're strdup'd in ast_new/ast_set_end */
    free((void *)node->range.start.filename);
    if (node->range.end.filename != node->range.start.filename) {
        free((void *)node->range.end.filename);
    }

    free(node);
}

/* Add a child to a node.  Returns 0 on success, -1 on allocation failure. */
int ast_add_child(AstNode *parent, AstNode *child) {
    if (!parent || !child) return -1;

    if (parent->child_count >= parent->child_capacity) {
        int new_cap = parent->child_capacity == 0
            ? INITIAL_CHILDREN_CAPACITY
            : parent->child_capacity * 2;
        AstNode **new_children = realloc(parent->children,
                                         new_cap * sizeof(AstNode *));
        if (!new_children) {
            fprintf(stderr, "ast_add_child: out of memory\n");
            return -1;
        }
        parent->children = new_children;
        parent->child_capacity = new_cap;
    }

    parent->children[parent->child_count++] = child;
    return 0;
}

/* Set the end location of a node */
void ast_set_end(AstNode *node, SourceLoc end) {
    if (node) {
        /* Free old end filename if it was separately allocated (not shared with start) */
        if (node->range.end.filename != node->range.start.filename) {
            free((void *)node->range.end.filename);
        }
        /* Copy end location with owned filename */
        node->range.end.filename = end.filename ? strdup(end.filename) : NULL;
        node->range.end.line = end.line;
        node->range.end.column = end.column;
    }
}

/* Helper to duplicate a string */
static char *str_dup(const char *s) {
    if (!s) return NULL;
    return strdup(s);
}

/* Helper to duplicate a string with explicit length (handles embedded nulls) */
static char *str_dup_n(const char *s, int len) {
    if (!s || len <= 0) return NULL;
    char *copy = malloc(len + 1);
    if (copy) {
        memcpy(copy, s, len);
        copy[len] = '\0';
    }
    return copy;
}

/* Convenience constructors */

AstNode *ast_program(const char *name, SourceLoc loc) {
    AstNode *node = ast_new(AST_PROGRAM, loc);
    if (node) {
        node->data.program.name = str_dup(name);
    }
    return node;
}

AstNode *ast_data_section(const char *name, SourceLoc loc) {
    AstNode *node = ast_new(AST_DATA_SECTION, loc);
    if (node) {
        node->data.data_section.name = str_dup(name);
    }
    return node;
}

AstNode *ast_var_decl(const char *name, int array_size, SourceLoc loc) {
    AstNode *node = ast_new(AST_VAR_DECL, loc);
    if (node) {
        node->data.var_decl.name = str_dup(name);
        node->data.var_decl.array_size = array_size;
    }
    return node;
}

AstNode *ast_define(const char *name, const char *value, SourceLoc loc) {
    AstNode *node = ast_new(AST_DEFINE, loc);
    if (node) {
        node->data.define.name = str_dup(name);
        node->data.define.value = str_dup(value);
    }
    return node;
}

AstNode *ast_copy(const char *filename, SourceLoc loc) {
    AstNode *node = ast_new(AST_COPY, loc);
    if (node) {
        node->data.copy.filename = str_dup(filename);
    }
    return node;
}

AstNode *ast_proc(const char *name, SourceLoc loc) {
    AstNode *node = ast_new(AST_PROC, loc);
    if (node) {
        node->data.proc.name = str_dup(name);
    }
    return node;
}

AstNode *ast_label(const char *name, SourceLoc loc) {
    AstNode *node = ast_new(AST_LABEL, loc);
    if (node) {
        node->data.label.name = str_dup(name);
    }
    return node;
}

AstNode *ast_verb(const char *name, SourceLoc loc) {
    AstNode *node = ast_new(AST_VERB_STMT, loc);
    if (node) {
        node->data.call.name = str_dup(name);
    }
    return node;
}

AstNode *ast_proc_call(const char *name, SourceLoc loc) {
    AstNode *node = ast_new(AST_PROC_CALL, loc);
    if (node) {
        node->data.call.name = str_dup(name);
    }
    return node;
}

AstNode *ast_goto(const char *label, SourceLoc loc) {
    AstNode *node = ast_new(AST_GOTO, loc);
    if (node) {
        node->data.goto_stmt.label = str_dup(label);
    }
    return node;
}

AstNode *ast_if(SourceLoc loc) {
    return ast_new(AST_IF_STMT, loc);
}

AstNode *ast_while(SourceLoc loc) {
    return ast_new(AST_WHILE_STMT, loc);
}

AstNode *ast_do_block(SourceLoc loc) {
    return ast_new(AST_DO_BLOCK, loc);
}

AstNode *ast_goto_depending_on(SourceLoc loc) {
    return ast_new(AST_GOTO_DEPENDING_ON, loc);
}

AstNode *ast_literal_str(const char *value, int length, SourceLoc loc) {
    AstNode *node = ast_new(AST_LITERAL_STR, loc);
    if (node) {
        node->data.str_lit.value = str_dup_n(value, length);
        node->data.str_lit.length = length;
    }
    return node;
}

AstNode *ast_literal_num(int64_t value, const char *text, SourceLoc loc) {
    AstNode *node = ast_new(AST_LITERAL_NUM, loc);
    if (node) {
        node->data.num_lit.value = value;
        node->data.num_lit.text = str_dup(text);
    }
    return node;
}

AstNode *ast_literal_hex(int64_t value, const char *text, SourceLoc loc) {
    AstNode *node = ast_new(AST_LITERAL_HEX, loc);
    if (node) {
        node->data.num_lit.value = value;
        node->data.num_lit.text = str_dup(text);
    }
    return node;
}

AstNode *ast_ident(const char *name, SourceLoc loc) {
    AstNode *node = ast_new(AST_IDENT, loc);
    if (node) {
        node->data.ident.name = str_dup(name);
    }
    return node;
}

AstNode *ast_reg_i(int num, SourceLoc loc) {
    AstNode *node = ast_new(AST_REG_I, loc);
    if (node) {
        node->data.reg.number = num;
    }
    return node;
}

AstNode *ast_reg_d(int num, SourceLoc loc) {
    AstNode *node = ast_new(AST_REG_D, loc);
    if (node) {
        node->data.reg.number = num;
    }
    return node;
}

AstNode *ast_reg_p(int num, SourceLoc loc) {
    AstNode *node = ast_new(AST_REG_P, loc);
    if (node) {
        node->data.reg.number = num;
    }
    return node;
}

AstNode *ast_pev(int num, SourceLoc loc) {
    AstNode *node = ast_new(AST_PEV, loc);
    if (node) {
        node->data.ext_var.number = num;
    }
    return node;
}

AstNode *ast_gev(int num, SourceLoc loc) {
    AstNode *node = ast_new(AST_GEV, loc);
    if (node) {
        node->data.ext_var.number = num;
    }
    return node;
}

AstNode *ast_rda_slot(int slot, SourceLoc loc) {
    AstNode *node = ast_new(AST_RDA_SLOT, loc);
    if (node) {
        node->data.ext_var.number = slot;
    }
    return node;
}

AstNode *ast_indexed(AstNode *base, AstNode *index, SourceLoc loc) {
    AstNode *node = ast_new(AST_INDEXED, loc);
    if (node) {
        ast_add_child(node, base);
        ast_add_child(node, index);
    }
    return node;
}

AstNode *ast_compare(CmpOp op, AstNode *left, AstNode *right, SourceLoc loc) {
    AstNode *node = ast_new(AST_COMPARE, loc);
    if (node) {
        node->data.compare.op = op;
        ast_add_child(node, left);
        ast_add_child(node, right);
    }
    return node;
}

AstNode *ast_logic_and(AstNode *left, AstNode *right, SourceLoc loc) {
    AstNode *node = ast_new(AST_LOGIC_AND, loc);
    if (node) {
        ast_add_child(node, left);
        ast_add_child(node, right);
    }
    return node;
}

AstNode *ast_logic_or(AstNode *left, AstNode *right, SourceLoc loc) {
    AstNode *node = ast_new(AST_LOGIC_OR, loc);
    if (node) {
        ast_add_child(node, left);
        ast_add_child(node, right);
    }
    return node;
}

AstNode *ast_format_spec(AstNode *field, int fix_len, int imbed_len, SourceLoc loc) {
    AstNode *node = ast_new(AST_FORMAT_SPEC, loc);
    if (node) {
        node->data.format_spec.fix_len = fix_len;
        node->data.format_spec.imbed_len = imbed_len;
        if (field) {
            ast_add_child(node, field);
        }
    }
    return node;
}

AstNode *ast_error(SourceLoc loc) {
    return ast_new(AST_ERROR, loc);
}

/* Node kind names for debugging */
const char *ast_kind_name(AstNodeKind kind) {
    static const char *names[] = {
        [AST_PROGRAM] = "PROGRAM",
        [AST_DATA_SECTION] = "DATA_SECTION",
        [AST_VAR_DECL] = "VAR_DECL",
        [AST_DEFINE] = "DEFINE",
        [AST_COPY] = "COPY",
        [AST_PROC] = "PROC",
        [AST_LABEL] = "LABEL",
        [AST_VERB_STMT] = "VERB_STMT",
        [AST_PROC_CALL] = "PROC_CALL",
        [AST_IF_STMT] = "IF_STMT",
        [AST_WHILE_STMT] = "WHILE_STMT",
        [AST_DO_BLOCK] = "DO_BLOCK",
        [AST_GOTO] = "GOTO",
        [AST_GOTO_DEPENDING_ON] = "GOTO_DEPENDING_ON",
        [AST_LITERAL_STR] = "LITERAL_STR",
        [AST_LITERAL_NUM] = "LITERAL_NUM",
        [AST_LITERAL_HEX] = "LITERAL_HEX",
        [AST_IDENT] = "IDENT",
        [AST_REG_I] = "REG_I",
        [AST_REG_D] = "REG_D",
        [AST_REG_P] = "REG_P",
        [AST_PEV] = "PEV",
        [AST_GEV] = "GEV",
        [AST_INDEXED] = "INDEXED",
        [AST_COMPARE] = "COMPARE",
        [AST_LOGIC_AND] = "LOGIC_AND",
        [AST_LOGIC_OR] = "LOGIC_OR",
        [AST_RDA_SLOT] = "RDA_SLOT",
        [AST_FORMAT_SPEC] = "FORMAT_SPEC",
        [AST_ERROR] = "ERROR",
    };
    if (kind >= 0 && kind < sizeof(names)/sizeof(names[0]) && names[kind]) {
        return names[kind];
    }
    return "UNKNOWN";
}

/* Dump AST for debugging */
void ast_dump(AstNode *node, int indent) {
    if (!node) {
        printf("%*s(null)\n", indent, "");
        return;
    }

    printf("%*s%s", indent, "", ast_kind_name(node->kind));

    /* Print node-specific info */
    switch (node->kind) {
        case AST_PROGRAM:
            printf(" name=\"%s\"", node->data.program.name);
            break;
        case AST_DATA_SECTION:
            printf(" name=\"%s\"", node->data.data_section.name);
            break;
        case AST_VAR_DECL:
            printf(" name=\"%s\"", node->data.var_decl.name);
            if (node->data.var_decl.array_size > 0) {
                printf(" size=%d", node->data.var_decl.array_size);
            }
            break;
        case AST_DEFINE:
            printf(" name=\"%s\" value=\"%s\"",
                   node->data.define.name, node->data.define.value);
            break;
        case AST_COPY:
            printf(" file=\"%s\"", node->data.copy.filename);
            break;
        case AST_PROC:
            printf(" name=\"%s\"", node->data.proc.name);
            break;
        case AST_LABEL:
            printf(" name=\"%s\"", node->data.label.name);
            break;
        case AST_VERB_STMT:
        case AST_PROC_CALL:
            printf(" name=\"%s\"", node->data.call.name);
            break;
        case AST_GOTO:
            printf(" label=\"%s\"", node->data.goto_stmt.label);
            break;
        case AST_LITERAL_STR:
            printf(" value=\"%s\"", node->data.str_lit.value);
            break;
        case AST_LITERAL_NUM:
        case AST_LITERAL_HEX:
            printf(" value=%ld text=\"%s\"",
                   (long)node->data.num_lit.value, node->data.num_lit.text);
            break;
        case AST_IDENT:
            printf(" name=\"%s\"", node->data.ident.name);
            break;
        case AST_REG_I:
            printf(" I%d", node->data.reg.number);
            break;
        case AST_REG_D:
            printf(" D%d", node->data.reg.number);
            break;
        case AST_REG_P:
            printf(" P%d", node->data.reg.number);
            break;
        case AST_PEV:
            printf(" &%d", node->data.ext_var.number);
            break;
        case AST_GEV:
            printf(" #%d", node->data.ext_var.number);
            break;
        case AST_RDA_SLOT:
            printf(" slot=%d", node->data.ext_var.number);
            break;
        case AST_COMPARE:
            {
                const char *ops[] = {"=", "<>", "<", ">", "<=", ">="};
                printf(" op=\"%s\"", ops[node->data.compare.op]);
            }
            break;
        case AST_FORMAT_SPEC:
            printf(" fix=%d imbed=%d",
                   node->data.format_spec.fix_len,
                   node->data.format_spec.imbed_len);
            break;
        default:
            break;
    }

    printf(" [%s:%d:%d]\n",
           node->range.start.filename ? node->range.start.filename : "<unknown>",
           node->range.start.line,
           node->range.start.column);

    /* Dump children */
    for (int i = 0; i < node->child_count; i++) {
        ast_dump(node->children[i], indent + 2);
    }
}
