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
%{
/*
 * TBOL Compiler - Parser with AST Construction
 */

/* Include custom YYLTYPE BEFORE anything else */
#include "yyltype.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../../shared/ast.h"
#include "../diag/diag.h"
#include "../options.h"
#include "../lexer/preproc.h"

/* External declarations */
extern int yylex(void);
extern const char *lexer_get_filename(void);
extern int lexer_get_last_string_length(void);

void yyerror(const char *s);

/* AST root */
AstNode *ast_root = NULL;

/* Default location merging - keep first location's filename */
#define YYLLOC_DEFAULT(Current, Rhs, N) \
    do { \
        if (N) { \
            (Current).first_line   = YYRHSLOC(Rhs, 1).first_line; \
            (Current).first_column = YYRHSLOC(Rhs, 1).first_column; \
            (Current).last_line    = YYRHSLOC(Rhs, N).last_line; \
            (Current).last_column  = YYRHSLOC(Rhs, N).last_column; \
            (Current).filename     = YYRHSLOC(Rhs, 1).filename; \
        } else { \
            (Current).first_line   = (Current).last_line   = YYRHSLOC(Rhs, 0).last_line; \
            (Current).first_column = (Current).last_column = YYRHSLOC(Rhs, 0).last_column; \
            (Current).filename     = YYRHSLOC(Rhs, 0).filename; \
        } \
    } while (0)

/* Helper macro to create SourceLoc from YYLTYPE - now uses stored filename */
#define MAKE_LOC(loc) ((SourceLoc){(loc).filename, (loc).first_line, (loc).first_column})
#define MAKE_END_LOC(loc) ((SourceLoc){(loc).filename, (loc).last_line, (loc).last_column})

%}

/* Enable location tracking */
%locations
%error-verbose

/* Value types */
%union {
    int ival;
    char *sval;
    struct { char *canonical; char *original; } ident_pair;
    AstNode *node;
    CmpOp cmp_op;
    struct {
        int16_t timeout;
        uint8_t flags;
    } send_mods;
}

/* Destructors for error-recovery cleanup.
 * Note: 'program' is excluded because bison 2.3 calls destructors on the
 * start symbol during stack cleanup after a successful parse. Since the
 * program node is saved to ast_root, we manage its lifetime manually. */
/* 3 expected shift/reduce conflicts:
 * - States 34, 385: error recovery in PROC/DO body vs empty label_prefix.
 *   Bison correctly shifts (prefers error recovery).
 * - State 461: dangling else (IF THEN body . ELSE).
 *   Bison correctly shifts (ELSE binds to nearest IF).
 * 4 expected reduce/reduce conflicts:
 * - States 251-254: NAVIGATE KW_FIRST/KW_NEXT/KW_BACK/KW_LAST can reduce as
 *   dedicated verb_stmt (opcode) or as expr (identifier argument).
 *   Bison correctly picks verb_stmt (first rule) for ';', expr for ','. */
%expect 3

%destructor { free($$); } LIT_STR LIT_NUM LIT_HEX label_name
%destructor { free($$.canonical); free($$.original); } IDENT
%destructor { ast_free($$); } definitions definition data_section data_group var_decl var_decl_list define_stmt define_in_proc proc_list proc stmt_list label_prefix label statement simple_stmt verb_stmt proc_call if_stmt while_stmt then_body do_block condition condition_term condition_factor expr indexable expr_list label_list format_spec format_spec_list

/* Token declarations - Program Structure */
/* Note: COPY is handled entirely in the lexer - no KW_COPY token */
%token KW_PROGRAM KW_DATA KW_DEFINE KW_PROC KW_END_PROC

/* Token declarations - Control Flow */
%token KW_IF KW_THEN KW_ELSE KW_DO KW_END KW_WHILE
%token KW_GOTO KW_GOTO_DEPENDING_ON KW_EXIT KW_RETURN

/* Token declarations - Data Movement */
%token KW_MOVE KW_ABS KW_SWAP KW_FILL KW_CLEAR KW_PUSH KW_POP

/* Token declarations - String Operations */
%token KW_STRING KW_SUBSTR KW_INSTR KW_UPPERCASE KW_LENGTH KW_EDIT

/* Token declarations - Arithmetic */
%token KW_ADD KW_SUBTRACT KW_MULTIPLY KW_DIVIDE

/* Token declarations - Bitwise */
%token KW_AND KW_OR KW_XOR KW_TEST

/* Token declarations - Navigation/Objects */
%token KW_NAVIGATE KW_FIRST KW_NEXT KW_BACK KW_LAST
%token KW_FETCH KW_OPEN_WINDOW KW_CLOSE_WINDOW
%token KW_OPEN_ERROR_WINDOW KW_KILL KW_LINK KW_TRANSFER KW_PURGE_CACHE

/* Token declarations - File I/O */
%token KW_OPEN KW_CLOSE KW_READ KW_WRITE
%token KW_NOTE KW_POINT KW_DELETE

/* Token declarations - Communications */
%token KW_CONNECT KW_DISCONNECT KW_SEND KW_RECEIVE KW_CANCEL
%token KW_TIMEOUT KW_PRIORITY KW_OPT_HDRS

/* Token declarations - State Management */
%token KW_SAVE KW_RESTORE KW_RELEASE

/* Token declarations - Timer/Process */
%token KW_WAIT KW_START KW_STOP

/* Token declarations - Display/Fields */
%token KW_REFRESH KW_ERASE KW_SET_CURSOR KW_SOUND
%token KW_DEFINE_FIELD KW_SET_ATTRIBUTE KW_SET_FUNCTION KW_SET_KEY
/* Token declarations - Data Operations */
%token KW_FORMAT KW_MAKE_FORMAT KW_LOOKUP KW_SORT

/* Token declarations - Other Verbs */
%token KW_ERROR KW_TRIGGER_FUNCTION

/* Operators */
%token OP_EQ OP_NE OP_LT OP_GT OP_LE OP_GE

/* Typed tokens */
%token <ival> REG_I REG_D REG_P PEV GEV RDA_SLOT
%token <sval> LIT_STR LIT_NUM LIT_HEX
%token <ident_pair> IDENT

/* Non-terminal types */
%type <node> program definitions definition
%type <node> data_section data_group var_decl var_decl_list
%type <node> define_stmt define_in_proc
%type <node> proc_list proc stmt_list label_prefix label statement
%type <node> simple_stmt verb_stmt proc_call
%type <node> if_stmt while_stmt then_body do_block
%type <node> condition condition_term condition_factor
%type <node> expr indexable expr_list label_list
%type <node> format_spec format_spec_list
%type <cmp_op> cmp_op
%type <send_mods> send_modifiers send_modifier
%type <sval> label_name

%start program

%%

program
    : KW_PROGRAM IDENT ';' definitions proc_list {
        $$ = ast_program($2.canonical, MAKE_LOC(@1));
        free($2.canonical); free($2.original);
        /* Add definitions as children */
        if ($4) {
            for (int i = 0; i < $4->child_count; i++) {
                ast_add_child($$, $4->children[i]);
            }
            $4->child_count = 0;  /* Prevent double-free */
            ast_free($4); $4 = NULL;
        }
        /* Add procedures as children */
        if ($5) {
            for (int i = 0; i < $5->child_count; i++) {
                ast_add_child($$, $5->children[i]);
            }
            $5->child_count = 0;
            ast_free($5); $5 = NULL;
        }
        ast_set_end($$, MAKE_LOC(@5));
        ast_root = $$;
    }
    | KW_PROGRAM error ';' definitions proc_list {
        /* Error in program name - recover and continue */
        yyerrok;
        $$ = ast_program("<error>", MAKE_LOC(@1));
        if ($4) {
            for (int i = 0; i < $4->child_count; i++) {
                ast_add_child($$, $4->children[i]);
            }
            $4->child_count = 0;
            ast_free($4); $4 = NULL;
        }
        if ($5) {
            for (int i = 0; i < $5->child_count; i++) {
                ast_add_child($$, $5->children[i]);
            }
            $5->child_count = 0;
            ast_free($5); $5 = NULL;
        }
        ast_root = $$;
    }
    | error {
        /* Complete parse failure - no PROGRAM statement found.
         * YYABORT immediately to avoid infinite error recovery loop. */
        $$ = NULL;
        ast_root = NULL;
        YYABORT;
    }
    ;

definitions
    : /* empty */ { $$ = NULL; }
    | definitions definition {
        if (!$1) {
            $$ = ast_new(AST_PROGRAM, MAKE_LOC(@2));  /* Temporary container */
        } else {
            $$ = $1; $1 = NULL;
        }
        if ($2) { ast_add_child($$, $2); $2 = NULL; }
    }
    ;

definition
    : data_section { $$ = $1; $1 = NULL; }
    | data_group { $$ = $1; $1 = NULL; }
    | define_stmt { $$ = $1; $1 = NULL; }
    | error ';' {
        yyerrok;
        $$ = ast_error(MAKE_LOC(@1));
    }
    ;

/* DATA group - continuation of DATA section without DATA keyword */
data_group
    : IDENT OP_EQ var_decl_list ';' {
        $$ = ast_data_section($1.canonical, MAKE_LOC(@1));
        free($1.canonical); free($1.original);
        if ($3) {
            for (int i = 0; i < $3->child_count; i++) {
                ast_add_child($$, $3->children[i]);
            }
            $3->child_count = 0;
            ast_free($3); $3 = NULL;
        }
        ast_set_end($$, MAKE_LOC(@4));
    }
    ;

/* DATA section */
data_section
    : KW_DATA IDENT OP_EQ var_decl_list ';' {
        $$ = ast_data_section($2.canonical, MAKE_LOC(@1));
        free($2.canonical); free($2.original);
        /* Move children from var_decl_list */
        if ($4) {
            for (int i = 0; i < $4->child_count; i++) {
                ast_add_child($$, $4->children[i]);
            }
            $4->child_count = 0;
            ast_free($4); $4 = NULL;
        }
        ast_set_end($$, MAKE_LOC(@5));
    }
    | KW_DATA error ';' {
        yyerrok;
        $$ = ast_error(MAKE_LOC(@1));
    }
    ;

var_decl_list
    : var_decl {
        $$ = ast_new(AST_DATA_SECTION, MAKE_LOC(@1));  /* Container */
        ast_add_child($$, $1); $1 = NULL;
    }
    | var_decl_list ',' var_decl {
        $$ = $1; $1 = NULL;
        ast_add_child($$, $3); $3 = NULL;
    }
    ;

var_decl
    : IDENT {
        $$ = ast_var_decl($1.canonical, 0, MAKE_LOC(@1));
        if ($1.original) $$->data.var_decl.original_text = $1.original;
        ast_set_end($$, MAKE_END_LOC(@1));
        free($1.canonical);
    }
    | IDENT '(' LIT_NUM ')' {
        int asize = atoi($3);
        if (asize <= 0) {
            yyerror("array size must be greater than zero");
            YYERROR;
        }
        $$ = ast_var_decl($1.canonical, asize, MAKE_LOC(@1));
        if ($1.original) $$->data.var_decl.original_text = $1.original;
        ast_set_end($$, MAKE_END_LOC(@4));
        free($1.canonical);
        free($3); $3 = NULL;
    }
    ;

/* DEFINE statement */
define_stmt
    : KW_DEFINE IDENT ',' expr ';' {
        /* Convert expr to string for DEFINE value */
        char *val = NULL;
        char *preproc_val = NULL;  /* Value for preprocessor (may differ) */
        switch ($4->kind) {
            case AST_LITERAL_STR:
                val = strdup($4->data.str_lit.value);
                /* Preprocessor needs quoted form for string detection */
                asprintf(&preproc_val, "'%s'", $4->data.str_lit.value);
                break;
            case AST_LITERAL_NUM:
            case AST_LITERAL_HEX:
                val = strdup($4->data.num_lit.text);
                preproc_val = strdup($4->data.num_lit.text);
                break;
            case AST_PEV:
                asprintf(&val, "&%d", $4->data.ext_var.number);
                preproc_val = strdup(val);
                break;
            case AST_GEV:
                asprintf(&val, "#%d", $4->data.ext_var.number);
                preproc_val = strdup(val);
                break;
            case AST_IDENT:
                val = strdup($4->data.ident.name);
                preproc_val = strdup(val);
                break;
            case AST_REG_I:
                asprintf(&val, "I%d", $4->data.reg.number);
                preproc_val = strdup(val);
                break;
            case AST_REG_D:
                asprintf(&val, "D%d", $4->data.reg.number);
                preproc_val = strdup(val);
                break;
            case AST_REG_P:
                asprintf(&val, "P%d", $4->data.reg.number);
                preproc_val = strdup(val);
                break;
            default:
                val = strdup("?");
                preproc_val = strdup("?");
        }

        /* Register with preprocessor for substitution */
        preproc_add_define($2.canonical, preproc_val, @1.first_line, @1.first_column);
        free(preproc_val);

        $$ = ast_define($2.canonical, val, MAKE_LOC(@1));
        if ($2.original) $$->data.define.original_text = $2.original;
        ast_set_end($$, MAKE_LOC(@5));
        free($2.canonical);
        free(val);
        ast_free($4); $4 = NULL;
    }
    ;

/* Procedures */
proc_list
    : proc {
        $$ = ast_new(AST_PROGRAM, MAKE_LOC(@1));  /* Container */
        ast_add_child($$, $1); $1 = NULL;
    }
    | proc_list proc {
        $$ = $1; $1 = NULL;
        ast_add_child($$, $2); $2 = NULL;
    }
    | proc_list error ';' {
        /* Skip garbage lines between procs */
        yyerrok;
        $$ = $1; $1 = NULL;
    }
    ;

proc
    : KW_PROC IDENT OP_EQ stmt_list KW_END_PROC {
        $$ = ast_proc($2.canonical, MAKE_LOC(@1));
        if ($2.original) $$->data.proc.original_text = $2.original;
        free($2.canonical);
        if ($4) {
            for (int i = 0; i < $4->child_count; i++) {
                ast_add_child($$, $4->children[i]);
            }
            $4->child_count = 0;
            ast_free($4); $4 = NULL;
        }
        ast_set_end($$, MAKE_LOC(@5));
    }
    | KW_PROC IDENT OP_EQ error KW_END_PROC {
        yyerrok;
        $$ = ast_proc($2.canonical, MAKE_LOC(@1));
        if ($2.original) $$->data.proc.original_text = $2.original;
        free($2.canonical);
        ast_add_child($$, ast_error(MAKE_LOC(@4)));
    }
    ;

/* stmt_list requires at least one non-label statement.
 * Labels may appear before or between statements but a proc/DO block
 * containing only labels is rejected (matches original compiler behavior).
 * Structure: optional_labels statement rest_of_stmts */
stmt_list
    : label_prefix statement {
        $$ = $1 ? $1 : ast_new(AST_DO_BLOCK, MAKE_LOC(@2));
        if ($1) { $1 = NULL; }
        if ($2) { ast_add_child($$, $2); $2 = NULL; }
    }
    | stmt_list statement {
        $$ = $1; $1 = NULL;
        if ($2) { ast_add_child($$, $2); $2 = NULL; }
    }
    | stmt_list label {
        $$ = $1; $1 = NULL;
        if ($2) { ast_add_child($$, $2); $2 = NULL; }
    }
    ;

/* Zero or more labels before the first statement */
label_prefix
    : /* empty */ { $$ = NULL; }
    | label_prefix label {
        if (!$1) {
            $$ = ast_new(AST_DO_BLOCK, MAKE_LOC(@2));
        } else {
            $$ = $1; $1 = NULL;
        }
        if ($2) { ast_add_child($$, $2); $2 = NULL; }
    }
    ;

label
    : label_name ':' {
        $$ = ast_label($1, MAKE_LOC(@1));
        ast_set_end($$, MAKE_END_LOC(@2));
        free($1); $1 = NULL;
    }
    ;

/* Label names can be identifiers or most verb keywords.
 * The original compiler rejects only: PROGRAM, DATA, DEFINE, PROC, END_PROC,
 * IF, THEN, ELSE, DO, END, WHILE, GOTO, AND, OR, MAKE_FORMAT
 */
label_name
    : IDENT { $$ = $1.canonical; free($1.original); }
    /* Control flow (exit/return ok, goto not ok) */
    | KW_EXIT { $$ = strdup("EXIT"); }
    | KW_RETURN { $$ = strdup("RETURN"); }
    /* Data operations */
    | KW_MOVE { $$ = strdup("MOVE"); }
    | KW_ABS { $$ = strdup("ABS"); }
    | KW_SWAP { $$ = strdup("SWAP"); }
    | KW_FILL { $$ = strdup("FILL"); }
    | KW_CLEAR { $$ = strdup("CLEAR"); }
    | KW_PUSH { $$ = strdup("PUSH"); }
    | KW_POP { $$ = strdup("POP"); }
    /* String operations */
    | KW_STRING { $$ = strdup("STRING"); }
    | KW_SUBSTR { $$ = strdup("SUBSTR"); }
    | KW_INSTR { $$ = strdup("INSTR"); }
    | KW_UPPERCASE { $$ = strdup("UPPERCASE"); }
    | KW_LENGTH { $$ = strdup("LENGTH"); }
    | KW_EDIT { $$ = strdup("EDIT"); }
    /* Math (and/or not ok) */
    | KW_ADD { $$ = strdup("ADD"); }
    | KW_SUBTRACT { $$ = strdup("SUBTRACT"); }
    | KW_MULTIPLY { $$ = strdup("MULTIPLY"); }
    | KW_DIVIDE { $$ = strdup("DIVIDE"); }
    | KW_XOR { $$ = strdup("XOR"); }
    | KW_TEST { $$ = strdup("TEST"); }
    /* Navigation/objects */
    | KW_NAVIGATE { $$ = strdup("NAVIGATE"); }
    | KW_FETCH { $$ = strdup("FETCH"); }
    | KW_OPEN_WINDOW { $$ = strdup("OPEN_WINDOW"); }
    | KW_CLOSE_WINDOW { $$ = strdup("CLOSE_WINDOW"); }
    | KW_OPEN_ERROR_WINDOW { $$ = strdup("OPEN_ERROR_WINDOW"); }
    | KW_KILL { $$ = strdup("KILL"); }
    | KW_LINK { $$ = strdup("LINK"); }
    | KW_TRANSFER { $$ = strdup("TRANSFER"); }
    | KW_PURGE_CACHE { $$ = strdup("PURGE_CACHE"); }
    /* File I/O */
    | KW_OPEN { $$ = strdup("OPEN"); }
    | KW_CLOSE { $$ = strdup("CLOSE"); }
    | KW_READ { $$ = strdup("READ"); }
    | KW_WRITE { $$ = strdup("WRITE"); }
    | KW_NOTE { $$ = strdup("NOTE"); }
    | KW_POINT { $$ = strdup("POINT"); }
    | KW_DELETE { $$ = strdup("DELETE"); }
    /* Network/messaging */
    | KW_CONNECT { $$ = strdup("CONNECT"); }
    | KW_DISCONNECT { $$ = strdup("DISCONNECT"); }
    | KW_SEND { $$ = strdup("SEND"); }
    | KW_RECEIVE { $$ = strdup("RECEIVE"); }
    | KW_CANCEL { $$ = strdup("CANCEL"); }
    /* Persistence */
    | KW_SAVE { $$ = strdup("SAVE"); }
    | KW_RESTORE { $$ = strdup("RESTORE"); }
    | KW_RELEASE { $$ = strdup("RELEASE"); }
    /* Timer/control */
    | KW_WAIT { $$ = strdup("WAIT"); }
    | KW_START { $$ = strdup("START"); }
    | KW_STOP { $$ = strdup("STOP"); }
    /* Display */
    | KW_REFRESH { $$ = strdup("REFRESH"); }
    | KW_ERASE { $$ = strdup("ERASE"); }
    | KW_SET_CURSOR { $$ = strdup("SET_CURSOR"); }
    | KW_SOUND { $$ = strdup("SOUND"); }
    /* Field/attribute operations */
    | KW_DEFINE_FIELD { $$ = strdup("DEFINE_FIELD"); }
    | KW_SET_ATTRIBUTE { $$ = strdup("SET_ATTRIBUTE"); }
    | KW_SET_FUNCTION { $$ = strdup("SET_FUNCTION"); }
    | KW_SET_KEY { $$ = strdup("SET_KEY"); }
    /* Format/misc */
    | KW_FORMAT { $$ = strdup("FORMAT"); }
    | KW_LOOKUP { $$ = strdup("LOOKUP"); }
    | KW_SORT { $$ = strdup("SORT"); }
    | KW_ERROR { $$ = strdup("ERROR"); }
    | KW_TRIGGER_FUNCTION { $$ = strdup("TRIGGER_FUNCTION"); }
    ;

statement
    : simple_stmt ';' {
        $$ = $1; $1 = NULL;
        /* Verb statements and proc calls set only their start location; give
         * them a true end span (through the last operand) for .sdb column
         * info. @1 spans the whole simple_stmt reduction. */
        if ($$) ast_set_end($$, MAKE_END_LOC(@1));
    }
    | if_stmt {
        $$ = $1; $1 = NULL;
    }
    | while_stmt {
        $$ = $1; $1 = NULL;
    }
    | define_in_proc {
        $$ = $1; $1 = NULL;
    }
    | ';' {
        /* Empty statement - can occur after COPY expansion */
        $$ = NULL;
    }
    | error ';' {
        yyerrok;
        $$ = ast_error(MAKE_LOC(@1));
    }
    ;

/* DEFINE inside procedure */
define_in_proc
    : KW_DEFINE IDENT ',' expr ';' {
        char *val = NULL;
        char *preproc_val = NULL;
        switch ($4->kind) {
            case AST_LITERAL_STR:
                val = strdup($4->data.str_lit.value);
                asprintf(&preproc_val, "'%s'", $4->data.str_lit.value);
                break;
            case AST_LITERAL_NUM:
            case AST_LITERAL_HEX:
                val = strdup($4->data.num_lit.text);
                preproc_val = strdup($4->data.num_lit.text);
                break;
            case AST_PEV:
                asprintf(&val, "&%d", $4->data.ext_var.number);
                preproc_val = strdup(val);
                break;
            case AST_GEV:
                asprintf(&val, "#%d", $4->data.ext_var.number);
                preproc_val = strdup(val);
                break;
            case AST_IDENT:
                val = strdup($4->data.ident.name);
                preproc_val = strdup(val);
                break;
            case AST_REG_I:
                asprintf(&val, "I%d", $4->data.reg.number);
                preproc_val = strdup(val);
                break;
            case AST_REG_D:
                asprintf(&val, "D%d", $4->data.reg.number);
                preproc_val = strdup(val);
                break;
            case AST_REG_P:
                asprintf(&val, "P%d", $4->data.reg.number);
                preproc_val = strdup(val);
                break;
            default:
                val = strdup("?");
                preproc_val = strdup("?");
        }
        preproc_add_define($2.canonical, preproc_val, @1.first_line, @1.first_column);
        free(preproc_val);
        $$ = ast_define($2.canonical, val, MAKE_LOC(@1));
        if ($2.original) $$->data.define.original_text = $2.original;
        ast_set_end($$, MAKE_LOC(@5));
        free($2.canonical);
        free(val);
        ast_free($4); $4 = NULL;
    }
    ;

simple_stmt
    : verb_stmt { $$ = $1; $1 = NULL; }
    | proc_call { $$ = $1; $1 = NULL; }
    ;

/* then_body: a statement or a DO/END block.
 * DO/END blocks are only valid after THEN or ELSE - not as standalone
 * statements (matches original compiler behavior). */
then_body
    : statement { $$ = $1; $1 = NULL; }
    | do_block  { $$ = $1; $1 = NULL; }
    ;

if_stmt
    : KW_IF condition KW_THEN then_body {
        $$ = ast_if(MAKE_LOC(@1));
        ast_add_child($$, $2); $2 = NULL;
        ast_add_child($$, $4); $4 = NULL;
        ast_set_end($$, MAKE_LOC(@4));
    }
    | KW_IF condition KW_THEN then_body KW_ELSE then_body {
        $$ = ast_if(MAKE_LOC(@1));
        ast_add_child($$, $2); $2 = NULL;
        ast_add_child($$, $4); $4 = NULL;
        ast_add_child($$, $6); $6 = NULL;
        ast_set_end($$, MAKE_LOC(@6));
    }
    ;

while_stmt
    : KW_WHILE condition KW_THEN then_body {
        $$ = ast_while(MAKE_LOC(@1));
        ast_add_child($$, $2); $2 = NULL;
        ast_add_child($$, $4); $4 = NULL;
        ast_set_end($$, MAKE_LOC(@4));
    }
    ;

do_block
    : KW_DO stmt_list KW_END ';' {
        $$ = ast_do_block(MAKE_LOC(@1));
        if ($2) {
            for (int i = 0; i < $2->child_count; i++) {
                ast_add_child($$, $2->children[i]);
            }
            $2->child_count = 0;
            ast_free($2); $2 = NULL;
        }
        ast_set_end($$, MAKE_LOC(@4));
    }
    | KW_DO error KW_END ';' {
        yyerrok;
        $$ = ast_do_block(MAKE_LOC(@1));
        ast_add_child($$, ast_error(MAKE_LOC(@2)));
    }
    ;

/* Conditions */
condition
    : condition_term { $$ = $1; $1 = NULL; }
    | condition KW_OR condition_term {
        $$ = ast_logic_or($1, $3, MAKE_LOC(@2));
        $1 = NULL; $3 = NULL;
    }
    ;

condition_term
    : condition_factor { $$ = $1; $1 = NULL; }
    | condition_term KW_AND condition_factor {
        $$ = ast_logic_and($1, $3, MAKE_LOC(@2));
        $1 = NULL; $3 = NULL;
    }
    ;

condition_factor
    : expr cmp_op expr {
        $$ = ast_compare($2, $1, $3, MAKE_LOC(@2));
        $1 = NULL; $3 = NULL;
    }
    | '(' condition ')' {
        $$ = $2; $2 = NULL;
    }
    ;

cmp_op
    : OP_EQ { $$ = CMP_EQ; }
    | OP_NE { $$ = CMP_NE; }
    | OP_LT { $$ = CMP_LT; }
    | OP_GT { $$ = CMP_GT; }
    | OP_LE { $$ = CMP_LE; }
    | OP_GE { $$ = CMP_GE; }
    ;

/* Verb statements */
verb_stmt
    /* 0 operands */
    : KW_DISCONNECT { $$ = ast_verb("DISCONNECT", MAKE_LOC(@1)); }
    | KW_PURGE_CACHE { $$ = ast_verb("PURGE_CACHE", MAKE_LOC(@1)); }
    | KW_REFRESH { $$ = ast_verb("REFRESH", MAKE_LOC(@1)); }
    | KW_WAIT { $$ = ast_verb("WAIT", MAKE_LOC(@1)); }

    /* 0-1 operands */
    | KW_RETURN {
        $$ = ast_verb("RETURN", MAKE_LOC(@1));
    }
    | KW_RETURN expr {
        $$ = ast_verb("RETURN", MAKE_LOC(@1));
        ast_add_child($$, $2); $2 = NULL;
    }
    | KW_EXIT {
        $$ = ast_verb("EXIT", MAKE_LOC(@1));
    }
    | KW_EXIT expr {
        $$ = ast_verb("EXIT", MAKE_LOC(@1));
        ast_add_child($$, $2); $2 = NULL;
    }
    | KW_CLOSE_WINDOW {
        $$ = ast_verb("CLOSE_WINDOW", MAKE_LOC(@1));
    }
    | KW_CLOSE_WINDOW expr {
        $$ = ast_verb("CLOSE_WINDOW", MAKE_LOC(@1));
        ast_add_child($$, $2); $2 = NULL;
    }
    | KW_OPEN_ERROR_WINDOW {
        $$ = ast_verb("OPEN_ERROR_WINDOW", MAKE_LOC(@1));
    }
    | KW_OPEN_ERROR_WINDOW expr {
        $$ = ast_verb("OPEN_ERROR_WINDOW", MAKE_LOC(@1));
        ast_add_child($$, $2); $2 = NULL;
    }
    | KW_OPEN_ERROR_WINDOW expr ',' expr {
        $$ = ast_verb("OPEN_ERROR_WINDOW", MAKE_LOC(@1));
        ast_add_child($$, $2); $2 = NULL;
        ast_add_child($$, $4); $4 = NULL;
    }
    | KW_ERASE {
        $$ = ast_verb("ERASE", MAKE_LOC(@1));
    }
    | KW_ERASE expr {
        $$ = ast_verb("ERASE", MAKE_LOC(@1));
        ast_add_child($$, $2); $2 = NULL;
    }
    | KW_SET_CURSOR {
        $$ = ast_verb("SET_CURSOR", MAKE_LOC(@1));
    }
    | KW_SET_CURSOR expr {
        $$ = ast_verb("SET_CURSOR", MAKE_LOC(@1));
        ast_add_child($$, $2); $2 = NULL;
    }
    | KW_SOUND {
        $$ = ast_verb("SOUND", MAKE_LOC(@1));
    }
    | KW_KILL {
        $$ = ast_verb("KILL", MAKE_LOC(@1));
    }
    | KW_KILL expr {
        $$ = ast_verb("KILL", MAKE_LOC(@1));
        ast_add_child($$, $2); $2 = NULL;
    }
    | KW_CANCEL {
        $$ = ast_verb("CANCEL", MAKE_LOC(@1));
    }
    | KW_CANCEL expr {
        $$ = ast_verb("CANCEL", MAKE_LOC(@1));
        ast_add_child($$, $2); $2 = NULL;
    }
    | KW_START {
        $$ = ast_verb("START", MAKE_LOC(@1));
    }
    | KW_START expr {
        $$ = ast_verb("START", MAKE_LOC(@1));
        ast_add_child($$, $2); $2 = NULL;
    }
    | KW_STOP {
        $$ = ast_verb("STOP", MAKE_LOC(@1));
    }
    | KW_STOP expr {
        $$ = ast_verb("STOP", MAKE_LOC(@1));
        ast_add_child($$, $2); $2 = NULL;
    }

    /* 1 operand */
    | KW_CLEAR expr {
        $$ = ast_verb("CLEAR", MAKE_LOC(@1));
        ast_add_child($$, $2); $2 = NULL;
    }
    | KW_CLOSE expr {
        $$ = ast_verb("CLOSE", MAKE_LOC(@1));
        ast_add_child($$, $2); $2 = NULL;
    }
    | KW_CONNECT expr {
        $$ = ast_verb("CONNECT", MAKE_LOC(@1));
        ast_add_child($$, $2); $2 = NULL;
    }
    | KW_NAVIGATE expr {
        $$ = ast_verb("NAVIGATE", MAKE_LOC(@1));
        ast_add_child($$, $2); $2 = NULL;
    }
    | KW_NAVIGATE expr ',' expr {
        $$ = ast_verb("NAVIGATE", MAKE_LOC(@1));
        ast_add_child($$, $2); $2 = NULL;
        ast_add_child($$, $4); $4 = NULL;
    }
    | KW_NAVIGATE KW_FIRST {
        $$ = ast_verb("NAVIGATE_FIRST", MAKE_LOC(@1));
    }
    | KW_NAVIGATE KW_NEXT {
        $$ = ast_verb("NAVIGATE_NEXT", MAKE_LOC(@1));
    }
    | KW_NAVIGATE KW_BACK {
        $$ = ast_verb("NAVIGATE_BACK", MAKE_LOC(@1));
    }
    | KW_NAVIGATE KW_LAST {
        $$ = ast_verb("NAVIGATE_LAST", MAKE_LOC(@1));
    }
    | KW_OPEN_WINDOW expr {
        $$ = ast_verb("OPEN_WINDOW", MAKE_LOC(@1));
        ast_add_child($$, $2); $2 = NULL;
    }
    | KW_POP expr {
        $$ = ast_verb("POP", MAKE_LOC(@1));
        ast_add_child($$, $2); $2 = NULL;
    }
    | KW_PUSH expr {
        $$ = ast_verb("PUSH", MAKE_LOC(@1));
        ast_add_child($$, $2); $2 = NULL;
    }
    | KW_RELEASE expr {
        $$ = ast_verb("RELEASE", MAKE_LOC(@1));
        ast_add_child($$, $2); $2 = NULL;
    }
    | KW_UPPERCASE expr {
        $$ = ast_verb("UPPERCASE", MAKE_LOC(@1));
        ast_add_child($$, $2); $2 = NULL;
    }
    | KW_ERROR expr {
        $$ = ast_verb("ERROR", MAKE_LOC(@1));
        ast_add_child($$, $2); $2 = NULL;
    }
    | KW_TRIGGER_FUNCTION expr {
        $$ = ast_verb("TRIGGER_FUNCTION", MAKE_LOC(@1));
        ast_add_child($$, $2); $2 = NULL;
    }
    | KW_DELETE expr {
        $$ = ast_verb("DELETE", MAKE_LOC(@1));
        ast_add_child($$, $2); $2 = NULL;
    }

    /* 1-2 operands */
    | KW_CLEAR expr ',' expr {
        $$ = ast_verb("CLEAR", MAKE_LOC(@1));
        ast_add_child($$, $2); $2 = NULL;
        ast_add_child($$, $4); $4 = NULL;
    }
    | KW_FETCH expr {
        $$ = ast_verb("FETCH", MAKE_LOC(@1));
        ast_add_child($$, $2); $2 = NULL;
    }
    | KW_FETCH expr ',' expr {
        $$ = ast_verb("FETCH", MAKE_LOC(@1));
        ast_add_child($$, $2); $2 = NULL;
        ast_add_child($$, $4); $4 = NULL;
    }

    /* 2 operands */
    | KW_ADD expr ',' expr {
        $$ = ast_verb("ADD", MAKE_LOC(@1));
        ast_add_child($$, $2); $2 = NULL;
        ast_add_child($$, $4); $4 = NULL;
    }
    | KW_AND expr ',' expr {
        $$ = ast_verb("AND", MAKE_LOC(@1));
        ast_add_child($$, $2); $2 = NULL;
        ast_add_child($$, $4); $4 = NULL;
    }
    | KW_LENGTH expr ',' expr {
        $$ = ast_verb("LENGTH", MAKE_LOC(@1));
        ast_add_child($$, $2); $2 = NULL;
        ast_add_child($$, $4); $4 = NULL;
    }
    | KW_MOVE expr ',' expr {
        $$ = ast_verb("MOVE", MAKE_LOC(@1));
        ast_add_child($$, $2); $2 = NULL;
        ast_add_child($$, $4); $4 = NULL;
    }
    | KW_MOVE expr ',' expr ',' KW_ABS {
        $$ = ast_verb("MOVE_ABS", MAKE_LOC(@1));
        ast_add_child($$, $2); $2 = NULL;
        ast_add_child($$, $4); $4 = NULL;
    }
    | KW_MULTIPLY expr ',' expr {
        $$ = ast_verb("MULTIPLY", MAKE_LOC(@1));
        ast_add_child($$, $2); $2 = NULL;
        ast_add_child($$, $4); $4 = NULL;
    }
    | KW_NOTE expr ',' expr {
        $$ = ast_verb("NOTE", MAKE_LOC(@1));
        ast_add_child($$, $2); $2 = NULL;
        ast_add_child($$, $4); $4 = NULL;
    }
    | KW_OPEN expr ',' expr {
        $$ = ast_verb("OPEN", MAKE_LOC(@1));
        ast_add_child($$, $2); $2 = NULL;
        ast_add_child($$, $4); $4 = NULL;
    }
    | KW_OR expr ',' expr {
        $$ = ast_verb("OR", MAKE_LOC(@1));
        ast_add_child($$, $2); $2 = NULL;
        ast_add_child($$, $4); $4 = NULL;
    }
    | KW_POINT expr ',' expr {
        $$ = ast_verb("POINT", MAKE_LOC(@1));
        ast_add_child($$, $2); $2 = NULL;
        ast_add_child($$, $4); $4 = NULL;
    }
    | KW_READ expr ',' expr {
        $$ = ast_verb("READ", MAKE_LOC(@1));
        ast_add_child($$, $2); $2 = NULL;
        ast_add_child($$, $4); $4 = NULL;
    }
    | KW_READ expr ',' expr ',' expr {
        $$ = ast_verb("READ", MAKE_LOC(@1));
        ast_add_child($$, $2); $2 = NULL;
        ast_add_child($$, $4); $4 = NULL;
        ast_add_child($$, $6); $6 = NULL;
    }
    | KW_RECEIVE expr ',' expr {
        $$ = ast_verb("RECEIVE", MAKE_LOC(@1));
        ast_add_child($$, $2); $2 = NULL;
        ast_add_child($$, $4); $4 = NULL;
    }
    | KW_SEND expr {
        /* SEND request; */
        $$ = ast_verb("SEND", MAKE_LOC(@1));
        ast_add_child($$, $2); $2 = NULL;
        $$->data.call.send_timeout = 0;
        $$->data.call.send_flags = 0;
    }
    | KW_SEND expr ',' expr {
        /* SEND request, msg_id; */
        $$ = ast_verb("SEND", MAKE_LOC(@1));
        ast_add_child($$, $2); $2 = NULL;
        ast_add_child($$, $4); $4 = NULL;
        $$->data.call.send_timeout = 0;
        $$->data.call.send_flags = 0;
    }
    | KW_SEND expr ',' send_modifiers {
        /* SEND request, modifiers; */
        $$ = ast_verb("SEND", MAKE_LOC(@1));
        ast_add_child($$, $2); $2 = NULL;
        $$->data.call.send_timeout = $4.timeout;
        $$->data.call.send_flags = $4.flags;
    }
    | KW_SEND expr ',' expr ',' send_modifiers {
        /* SEND request, msg_id, modifiers; */
        $$ = ast_verb("SEND", MAKE_LOC(@1));
        ast_add_child($$, $2); $2 = NULL;
        ast_add_child($$, $4); $4 = NULL;
        $$->data.call.send_timeout = $6.timeout;
        $$->data.call.send_flags = $6.flags;
    }
    | KW_SET_ATTRIBUTE expr ',' expr {
        $$ = ast_verb("SET_ATTRIBUTE", MAKE_LOC(@1));
        ast_add_child($$, $2); $2 = NULL;
        ast_add_child($$, $4); $4 = NULL;
    }
    | KW_SOUND expr ',' expr {
        $$ = ast_verb("SOUND", MAKE_LOC(@1));
        ast_add_child($$, $2); $2 = NULL;
        ast_add_child($$, $4); $4 = NULL;
    }
    | KW_SUBTRACT expr ',' expr {
        $$ = ast_verb("SUBTRACT", MAKE_LOC(@1));
        ast_add_child($$, $2); $2 = NULL;
        ast_add_child($$, $4); $4 = NULL;
    }
    | KW_SWAP expr ',' expr {
        $$ = ast_verb("SWAP", MAKE_LOC(@1));
        ast_add_child($$, $2); $2 = NULL;
        ast_add_child($$, $4); $4 = NULL;
    }
    | KW_TEST expr ',' expr {
        $$ = ast_verb("TEST", MAKE_LOC(@1));
        ast_add_child($$, $2); $2 = NULL;
        ast_add_child($$, $4); $4 = NULL;
    }
    | KW_WRITE expr ',' expr {
        $$ = ast_verb("WRITE", MAKE_LOC(@1));
        ast_add_child($$, $2); $2 = NULL;
        ast_add_child($$, $4); $4 = NULL;
    }
    | KW_WRITE expr ',' expr ',' expr {
        $$ = ast_verb("WRITE", MAKE_LOC(@1));
        ast_add_child($$, $2); $2 = NULL;
        ast_add_child($$, $4); $4 = NULL;
        ast_add_child($$, $6); $6 = NULL;
    }
    | KW_XOR expr ',' expr {
        $$ = ast_verb("XOR", MAKE_LOC(@1));
        ast_add_child($$, $2); $2 = NULL;
        ast_add_child($$, $4); $4 = NULL;
    }
    | KW_FILL expr ',' expr ',' expr {
        $$ = ast_verb("FILL", MAKE_LOC(@1));
        ast_add_child($$, $2); $2 = NULL;
        ast_add_child($$, $4); $4 = NULL;
        ast_add_child($$, $6); $6 = NULL;
    }

    /* 2-3 operands */
    | KW_SAVE expr ',' expr {
        $$ = ast_verb("SAVE", MAKE_LOC(@1));
        ast_add_child($$, $2); $2 = NULL;
        ast_add_child($$, $4); $4 = NULL;
    }
    | KW_SAVE expr ',' expr ',' expr {
        $$ = ast_verb("SAVE", MAKE_LOC(@1));
        ast_add_child($$, $2); $2 = NULL;
        ast_add_child($$, $4); $4 = NULL;
        ast_add_child($$, $6); $6 = NULL;
    }
    | KW_RESTORE expr ',' expr {
        $$ = ast_verb("RESTORE", MAKE_LOC(@1));
        ast_add_child($$, $2); $2 = NULL;
        ast_add_child($$, $4); $4 = NULL;
    }
    | KW_DIVIDE expr ',' expr {
        $$ = ast_verb("DIVIDE", MAKE_LOC(@1));
        ast_add_child($$, $2); $2 = NULL;
        ast_add_child($$, $4); $4 = NULL;
    }
    | KW_DIVIDE expr ',' expr ',' expr {
        $$ = ast_verb("DIVIDE", MAKE_LOC(@1));
        ast_add_child($$, $2); $2 = NULL;
        ast_add_child($$, $4); $4 = NULL;
        ast_add_child($$, $6); $6 = NULL;
    }

    /* 3 operands */
    | KW_INSTR expr ',' expr ',' expr {
        $$ = ast_verb("INSTR", MAKE_LOC(@1));
        ast_add_child($$, $2); $2 = NULL;
        ast_add_child($$, $4); $4 = NULL;
        ast_add_child($$, $6); $6 = NULL;
    }
    | KW_FORMAT expr ',' expr ',' expr {
        $$ = ast_verb("FORMAT", MAKE_LOC(@1));
        ast_add_child($$, $2); $2 = NULL;
        ast_add_child($$, $4); $4 = NULL;
        ast_add_child($$, $6); $6 = NULL;
    }
    | KW_SORT expr ',' expr ',' expr {
        $$ = ast_verb("SORT", MAKE_LOC(@1));
        ast_add_child($$, $2); $2 = NULL;
        ast_add_child($$, $4); $4 = NULL;
        ast_add_child($$, $6); $6 = NULL;
    }
    | KW_SET_KEY expr ',' expr ',' expr {
        $$ = ast_verb("SET_KEY", MAKE_LOC(@1));
        ast_add_child($$, $2); $2 = NULL;
        ast_add_child($$, $4); $4 = NULL;
        ast_add_child($$, $6); $6 = NULL;
    }
    | KW_EDIT expr ',' expr ',' expr_list {
        /* EDIT dest, format, args... (3+ operands) */
        $$ = ast_verb("EDIT", MAKE_LOC(@1));
        ast_add_child($$, $2); $2 = NULL;  /* dest */
        ast_add_child($$, $4); $4 = NULL;  /* format */
        if ($6) {
            for (int i = 0; i < $6->child_count; i++) {
                ast_add_child($$, $6->children[i]);
            }
            $6->child_count = 0;
            ast_free($6); $6 = NULL;
        }
    }

    /* 4 operands */
    | KW_SUBSTR expr ',' expr ',' expr ',' expr {
        $$ = ast_verb("SUBSTR", MAKE_LOC(@1));
        ast_add_child($$, $2); $2 = NULL;
        ast_add_child($$, $4); $4 = NULL;
        ast_add_child($$, $6); $6 = NULL;
        ast_add_child($$, $8); $8 = NULL;
    }
    | KW_SET_FUNCTION expr ',' expr {
        $$ = ast_verb("SET_FUNCTION", MAKE_LOC(@1));
        ast_add_child($$, $2); $2 = NULL;
        ast_add_child($$, $4); $4 = NULL;
    }
    | KW_SET_FUNCTION expr ',' expr ',' expr {
        $$ = ast_verb("SET_FUNCTION", MAKE_LOC(@1));
        ast_add_child($$, $2); $2 = NULL;
        ast_add_child($$, $4); $4 = NULL;
        ast_add_child($$, $6); $6 = NULL;
    }
    | KW_SET_FUNCTION expr ',' expr ',' expr ',' expr {
        $$ = ast_verb("SET_FUNCTION", MAKE_LOC(@1));
        ast_add_child($$, $2); $2 = NULL;
        ast_add_child($$, $4); $4 = NULL;
        ast_add_child($$, $6); $6 = NULL;
        ast_add_child($$, $8); $8 = NULL;
    }

    /* 5 operands */
    | KW_LOOKUP expr ',' expr ',' expr ',' expr ',' expr {
        $$ = ast_verb("LOOKUP", MAKE_LOC(@1));
        ast_add_child($$, $2); $2 = NULL;
        ast_add_child($$, $4); $4 = NULL;
        ast_add_child($$, $6); $6 = NULL;
        ast_add_child($$, $8); $8 = NULL;
        ast_add_child($$, $10); $10 = NULL;
    }

    /* 6-7 operands */
    | KW_DEFINE_FIELD expr ',' expr ',' expr ',' expr ',' expr ',' expr {
        $$ = ast_verb("DEFINE_FIELD", MAKE_LOC(@1));
        ast_add_child($$, $2); $2 = NULL;
        ast_add_child($$, $4); $4 = NULL;
        ast_add_child($$, $6); $6 = NULL;
        ast_add_child($$, $8); $8 = NULL;
        ast_add_child($$, $10); $10 = NULL;
        ast_add_child($$, $12); $12 = NULL;
    }
    | KW_DEFINE_FIELD expr ',' expr ',' expr ',' expr ',' expr ',' expr ',' expr {
        $$ = ast_verb("DEFINE_FIELD", MAKE_LOC(@1));
        ast_add_child($$, $2); $2 = NULL;
        ast_add_child($$, $4); $4 = NULL;
        ast_add_child($$, $6); $6 = NULL;
        ast_add_child($$, $8); $8 = NULL;
        ast_add_child($$, $10); $10 = NULL;
        ast_add_child($$, $12); $12 = NULL;
        ast_add_child($$, $14); $14 = NULL;
    }

    /* Variable operands - GOTO */
    | KW_GOTO label_name {
        $$ = ast_goto($2, MAKE_LOC(@1));
        ast_set_end($$, MAKE_END_LOC(@2));
        free($2); $2 = NULL;
    }
    | KW_GOTO_DEPENDING_ON expr ',' label_list {
        $$ = ast_goto_depending_on(MAKE_LOC(@1));
        ast_add_child($$, $2); $2 = NULL;
        /* Add labels from label_list */
        if ($4) {
            for (int i = 0; i < $4->child_count; i++) {
                ast_add_child($$, $4->children[i]);
            }
            $4->child_count = 0;
            ast_free($4); $4 = NULL;
        }
    }

    /* Variable operands - variadic verbs */
    | KW_STRING expr_list {
        $$ = ast_verb("STRING", MAKE_LOC(@1));
        if ($2) {
            for (int i = 0; i < $2->child_count; i++) {
                ast_add_child($$, $2->children[i]);
            }
            $2->child_count = 0;
            ast_free($2); $2 = NULL;
        }
    }
    | KW_LINK expr_list {
        $$ = ast_verb("LINK", MAKE_LOC(@1));
        if ($2) {
            for (int i = 0; i < $2->child_count; i++) {
                ast_add_child($$, $2->children[i]);
            }
            $2->child_count = 0;
            ast_free($2); $2 = NULL;
        }
    }
    | KW_TRANSFER expr_list {
        $$ = ast_verb("TRANSFER", MAKE_LOC(@1));
        if ($2) {
            for (int i = 0; i < $2->child_count; i++) {
                ast_add_child($$, $2->children[i]);
            }
            $2->child_count = 0;
            ast_free($2); $2 = NULL;
        }
    }

    /* MAKE_FORMAT - special syntax */
    | KW_MAKE_FORMAT expr ',' format_spec_list {
        $$ = ast_verb("MAKE_FORMAT", MAKE_LOC(@1));
        ast_add_child($$, $2); $2 = NULL;
        if ($4) {
            for (int i = 0; i < $4->child_count; i++) {
                ast_add_child($$, $4->children[i]);
            }
            $4->child_count = 0;
            ast_free($4); $4 = NULL;
        }
    }
    ;

/* Lists */
label_list
    : label_name {
        $$ = ast_new(AST_DO_BLOCK, MAKE_LOC(@1));  /* Container */
        ast_add_child($$, ast_ident($1, MAKE_LOC(@1)));
        free($1); $1 = NULL;
    }
    | label_list ',' label_name {
        $$ = $1; $1 = NULL;
        ast_add_child($$, ast_ident($3, MAKE_LOC(@3)));
        free($3); $3 = NULL;
    }
    ;

expr_list
    : expr {
        $$ = ast_new(AST_DO_BLOCK, MAKE_LOC(@1));  /* Container */
        ast_add_child($$, $1); $1 = NULL;
    }
    | expr_list ',' expr {
        $$ = $1; $1 = NULL;
        ast_add_child($$, $3); $3 = NULL;
    }
    ;

format_spec_list
    : format_spec {
        $$ = ast_new(AST_DO_BLOCK, MAKE_LOC(@1));  /* Container */
        ast_add_child($$, $1); $1 = NULL;
    }
    | format_spec_list ',' format_spec {
        $$ = $1; $1 = NULL;
        ast_add_child($$, $3); $3 = NULL;
    }
    ;

format_spec
    : expr ':' LIT_NUM {
        /* target:fixed_length - fixed width field */
        $$ = ast_format_spec($1, atoi($3), -1, MAKE_LOC(@1));
        $1 = NULL;
        free($3); $3 = NULL;
    }
    | expr ':' ':' LIT_NUM {
        /* target::embedded_length - length-prefixed field (1 or 2 byte prefix) */
        $$ = ast_format_spec($1, -1, atoi($4), MAKE_LOC(@1));
        $1 = NULL;
        free($4); $4 = NULL;
    }
    | expr ':' LIT_NUM ':' LIT_NUM {
        /* target:fixed_length:embedded_length - length-prefixed with padding/truncation */
        $$ = ast_format_spec($1, atoi($3), atoi($5), MAKE_LOC(@1));
        $1 = NULL;
        free($3); $3 = NULL;
        free($5); $5 = NULL;
    }
    ;

/* Procedure call */
proc_call
    : IDENT {
        $$ = ast_proc_call($1.canonical, MAKE_LOC(@1));
        ast_set_end($$, MAKE_END_LOC(@1));
        free($1.canonical); free($1.original);
    }
    | IDENT expr_list {
        $$ = ast_proc_call($1.canonical, MAKE_LOC(@1));
        free($1.canonical); free($1.original);
        if ($2) {
            for (int i = 0; i < $2->child_count; i++) {
                ast_add_child($$, $2->children[i]);
            }
            $2->child_count = 0;
            ast_free($2); $2 = NULL;
        }
        ast_set_end($$, MAKE_END_LOC(@2));
    }
    ;

/* Expressions - indexable types can have (expr) suffix for array access */
indexable
    : IDENT {
        $$ = ast_ident($1.canonical, MAKE_LOC(@1));
        ast_set_end($$, MAKE_END_LOC(@1));
        free($1.canonical); free($1.original);
    }
    | REG_I { $$ = ast_reg_i($1, MAKE_LOC(@1)); ast_set_end($$, MAKE_END_LOC(@1)); }
    | REG_D { $$ = ast_reg_d($1, MAKE_LOC(@1)); ast_set_end($$, MAKE_END_LOC(@1)); }
    | REG_P { $$ = ast_reg_p($1, MAKE_LOC(@1)); ast_set_end($$, MAKE_END_LOC(@1)); }
    | RDA_SLOT { $$ = ast_rda_slot($1, MAKE_LOC(@1)); ast_set_end($$, MAKE_END_LOC(@1)); }
    | PEV { $$ = ast_pev($1, MAKE_LOC(@1)); ast_set_end($$, MAKE_END_LOC(@1)); }
    | GEV { $$ = ast_gev($1, MAKE_LOC(@1)); ast_set_end($$, MAKE_END_LOC(@1)); }
    ;

expr
    : LIT_STR {
        $$ = ast_literal_str($1, lexer_get_last_string_length(), MAKE_LOC(@1));
        ast_set_end($$, MAKE_END_LOC(@1));
        free($1); $1 = NULL;
    }
    | LIT_NUM {
        $$ = ast_literal_num(atoll($1), $1, MAKE_LOC(@1));
        ast_set_end($$, MAKE_END_LOC(@1));
        free($1); $1 = NULL;
    }
    | LIT_HEX {
        $$ = ast_literal_hex(strtoll($1 + 2, NULL, 16), $1, MAKE_LOC(@1));
        ast_set_end($$, MAKE_END_LOC(@1));
        free($1); $1 = NULL;
    }
    | indexable {
        $$ = $1; $1 = NULL;
    }
    | indexable '(' expr ')' {
        $$ = ast_indexed($1, $3, MAKE_LOC(@1));
        $1 = NULL; $3 = NULL;
        ast_set_end($$, MAKE_END_LOC(@4));
    }
    ;

/* SEND modifiers: TIMEOUT(n), PRIORITY, OPT_HDRS in any order */
send_modifiers
    : send_modifier {
        $$ = $1;
    }
    | send_modifiers ',' send_modifier {
        $$.timeout = ($1.timeout != 0) ? $1.timeout : $3.timeout;
        $$.flags = $1.flags | $3.flags;
    }
    ;

send_modifier
    : KW_TIMEOUT '(' LIT_NUM ')' {
        $$.timeout = (int16_t)atoi($3);
        $$.flags = 0;
        free($3); $3 = NULL;
    }
    | KW_PRIORITY {
        $$.timeout = 0;
        $$.flags = 0x04;
    }
    | KW_OPT_HDRS {
        $$.timeout = 0;
        $$.flags = 0x02;
    }
    ;

%%

void yyerror(const char *s) {
    SourceLoc loc = {lexer_get_filename(), yylloc.first_line, yylloc.first_column};
    diag_error(loc, "%s", s);
}
