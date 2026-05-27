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
 * TBOL Abstract Syntax Tree
 * Shared AST representation for TBOL tools
 */

#ifndef TBOL_AST_H
#define TBOL_AST_H

#include <stdint.h>
#include <stdbool.h>

/* Source location tracking */
typedef struct {
    const char *filename;
    int line;
    int column;
} SourceLoc;

typedef struct {
    SourceLoc start;
    SourceLoc end;
} SourceRange;

/* AST Node kinds */
typedef enum {
    /* Program structure */
    AST_PROGRAM,
    AST_DATA_SECTION,
    AST_VAR_DECL,
    AST_DEFINE,
    AST_COPY,
    AST_PROC,

    /* Statements */
    AST_LABEL,
    AST_VERB_STMT,
    AST_PROC_CALL,
    AST_IF_STMT,
    AST_WHILE_STMT,
    AST_DO_BLOCK,
    AST_GOTO,
    AST_GOTO_DEPENDING_ON,

    /* Expressions/Operands */
    AST_LITERAL_STR,
    AST_LITERAL_NUM,
    AST_LITERAL_HEX,
    AST_IDENT,
    AST_REG_I,
    AST_REG_D,
    AST_REG_P,
    AST_PEV,
    AST_GEV,
    AST_RDA_SLOT,       /* RDA_FIRST, RDA_LAST - direct slot reference */
    AST_INDEXED,        /* base(index) */

    /* Conditions */
    AST_COMPARE,
    AST_LOGIC_AND,
    AST_LOGIC_OR,

    /* Special */
    AST_FORMAT_SPEC,    /* field:length for MAKE_FORMAT */
    AST_ERROR,          /* Error recovery placeholder */
} AstNodeKind;

/* Comparison operators */
typedef enum {
    CMP_EQ,     /* = */
    CMP_NE,     /* <> */
    CMP_LT,     /* < */
    CMP_GT,     /* > */
    CMP_LE,     /* <= */
    CMP_GE,     /* >= */
} CmpOp;

/* Forward declaration */
typedef struct AstNode AstNode;

/* AST Node structure */
struct AstNode {
    AstNodeKind kind;
    SourceRange range;

    /* Child nodes (flexible array) */
    AstNode **children;
    int child_count;
    int child_capacity;

    /* Node-specific data */
    union {
        /* AST_PROGRAM */
        struct {
            char *name;
        } program;

        /* AST_DATA_SECTION */
        struct {
            char *name;
        } data_section;

        /* AST_VAR_DECL */
        struct {
            char *name;
            char *original_text;
            int array_size;     /* 0 if not array */
            bool unused;        /* true if variable not referenced */
        } var_decl;

        /* AST_DEFINE */
        struct {
            char *name;
            char *original_text;
            char *value;        /* Raw value text */
        } define;

        /* AST_COPY */
        struct {
            char *filename;
        } copy;

        /* AST_PROC */
        struct {
            char *name;
            char *original_text;
        } proc;

        /* AST_LABEL */
        struct {
            char *name;
            char *original_text;
        } label;

        /* AST_VERB_STMT, AST_PROC_CALL */
        struct {
            char *name;         /* Verb or procedure name */
            /* Operands are in children[] */
            int16_t send_timeout;   /* For SEND: timeout in seconds (default 0) */
            uint8_t send_flags;     /* For SEND: PRIORITY=0x04, OPT_HDRS=0x02 */
        } call;

        /* AST_GOTO */
        struct {
            char *label;
        } goto_stmt;

        /* AST_LITERAL_STR */
        struct {
            char *value;
            int length;         /* Actual length (handles embedded nulls) */
        } str_lit;

        /* AST_LITERAL_NUM, AST_LITERAL_HEX */
        struct {
            int64_t value;
            char *text;         /* Original text for bytecode */
        } num_lit;

        /* AST_IDENT */
        struct {
            char *name;
        } ident;

        /* AST_REG_I, AST_REG_D, AST_REG_P */
        struct {
            int number;         /* 0-8 depending on type */
        } reg;

        /* AST_PEV, AST_GEV, AST_RDA_SLOT */
        struct {
            int number;         /* PEV: 1-256, GEV: 1-32000, RDA_SLOT: 0-221 */
        } ext_var;

        /* AST_INDEXED - children[0]=base, children[1]=index */

        /* AST_COMPARE */
        struct {
            CmpOp op;
            /* children[0]=left, children[1]=right */
        } compare;

        /* AST_FORMAT_SPEC */
        struct {
            int fix_len;
            int imbed_len;      /* -1 if not specified */
            /* children[0]=field (may be NULL for anonymous) */
        } format_spec;
    } data;
};

/* AST construction functions */
AstNode *ast_new(AstNodeKind kind, SourceLoc loc);
void ast_free(AstNode *node);
int ast_add_child(AstNode *parent, AstNode *child);
void ast_set_end(AstNode *node, SourceLoc end);

/* Convenience constructors */
AstNode *ast_program(const char *name, SourceLoc loc);
AstNode *ast_data_section(const char *name, SourceLoc loc);
AstNode *ast_var_decl(const char *name, int array_size, SourceLoc loc);
AstNode *ast_define(const char *name, const char *value, SourceLoc loc);
AstNode *ast_copy(const char *filename, SourceLoc loc);
AstNode *ast_proc(const char *name, SourceLoc loc);
AstNode *ast_label(const char *name, SourceLoc loc);
AstNode *ast_verb(const char *name, SourceLoc loc);
AstNode *ast_proc_call(const char *name, SourceLoc loc);
AstNode *ast_goto(const char *label, SourceLoc loc);
AstNode *ast_if(SourceLoc loc);
AstNode *ast_while(SourceLoc loc);
AstNode *ast_do_block(SourceLoc loc);
AstNode *ast_goto_depending_on(SourceLoc loc);

AstNode *ast_literal_str(const char *value, int length, SourceLoc loc);
AstNode *ast_literal_num(int64_t value, const char *text, SourceLoc loc);
AstNode *ast_literal_hex(int64_t value, const char *text, SourceLoc loc);
AstNode *ast_ident(const char *name, SourceLoc loc);
AstNode *ast_reg_i(int num, SourceLoc loc);
AstNode *ast_reg_d(int num, SourceLoc loc);
AstNode *ast_reg_p(int num, SourceLoc loc);
AstNode *ast_pev(int num, SourceLoc loc);
AstNode *ast_gev(int num, SourceLoc loc);
AstNode *ast_rda_slot(int slot, SourceLoc loc);
AstNode *ast_indexed(AstNode *base, AstNode *index, SourceLoc loc);

AstNode *ast_compare(CmpOp op, AstNode *left, AstNode *right, SourceLoc loc);
AstNode *ast_logic_and(AstNode *left, AstNode *right, SourceLoc loc);
AstNode *ast_logic_or(AstNode *left, AstNode *right, SourceLoc loc);

AstNode *ast_format_spec(AstNode *field, int fix_len, int imbed_len, SourceLoc loc);
AstNode *ast_error(SourceLoc loc);

/* Debug/dump */
void ast_dump(AstNode *node, int indent);
const char *ast_kind_name(AstNodeKind kind);

#endif /* TBOL_AST_H */
