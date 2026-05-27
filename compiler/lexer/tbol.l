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
 * TBOL Compiler - Lexical Analyzer
 */

/* Suppress warnings in flex-generated code */
#ifdef __clang__
#pragma clang diagnostic ignored "-Wsign-compare"
#endif

#include <string.h>
#include <stdlib.h>
#include "tokens.h"
#include "preproc.h"
#include "preproc_event.h"
#include "../parser/yyltype.h"  /* Must come before tbol.tab.h for custom YYLTYPE */
#include "../parser/tbol.tab.h"
#include "../diag/diag.h"

/* Location tracking - exported for preproc.c */
static const char *current_filename = NULL;
int current_line = 1;
int current_column = 1;
static char line_buffer[4096];
static int line_buffer_pos = 0;

/* Filename string pool for YYLTYPE - prevents leaking on every token */
#define FILENAME_POOL_SIZE 32
static const char *filename_pool[FILENAME_POOL_SIZE];
static int filename_pool_count = 0;

/* Intern a filename string - returns pooled pointer or adds to pool */
static const char *intern_filename(const char *filename) {
    if (!filename) return NULL;

    /* Check if already in pool */
    for (int i = 0; i < filename_pool_count; i++) {
        if (strcmp(filename_pool[i], filename) == 0) {
            return filename_pool[i];
        }
    }

    /* Add to pool if room */
    if (filename_pool_count < FILENAME_POOL_SIZE) {
        filename_pool[filename_pool_count] = strdup(filename);
        return filename_pool[filename_pool_count++];
    }

    /* Pool full - just strdup (shouldn't happen with typical include depths) */
    return strdup(filename);
}

/* Clear filename pool - call at lexer reset/cleanup */
void lexer_clear_filename_pool(void) {
    for (int i = 0; i < filename_pool_count; i++) {
        free((void *)filename_pool[i]);
        filename_pool[i] = NULL;
    }
    filename_pool_count = 0;
}

#define UPDATE_LOC() do { \
    yylloc.first_line = yylloc.last_line = current_line; \
    yylloc.first_column = current_column; \
    yylloc.last_column = current_column + yyleng - 1; \
    yylloc.filename = intern_filename(current_filename); \
    current_column += yyleng; \
} while(0)

#define NEWLINE() do { \
    if (line_buffer_pos > 0) { \
        line_buffer[line_buffer_pos] = '\0'; \
        diag_set_source_line(current_filename, current_line, line_buffer); \
        line_buffer_pos = 0; \
    } \
    current_line++; \
    current_column = 1; \
} while(0)

#define SAVE_TEXT() do { \
    for (int i = 0; yytext[i] && i < (int)(sizeof(line_buffer) - line_buffer_pos - 1); i++) { \
        if (yytext[i] == '\n') break; \
        line_buffer[line_buffer_pos++] = yytext[i]; \
    } \
} while(0)

static char *strupper(char *s) {
    for (char *p = s; *p; p++) {
        if (*p >= 'a' && *p <= 'z') *p -= 32;
    }
    return s;
}

/* Track last processed string length (for embedded nulls) */
static int last_string_length = 0;

int lexer_get_last_string_length(void) {
    return last_string_length;
}

static char *process_string(const char *s) {
    size_t len = strlen(s);
    if (len < 2) {
        last_string_length = 0;
        return strdup("");
    }
    char *result = malloc(len - 1);
    if (!result) return NULL;
    char *dst = result;
    const char *src = s + 1;
    const char *end = s + len - 1;
    while (src < end) {
        if (*src == '\\' && src + 1 < end) {
            /* Backslash escape */
            src++;
            if (*src == '\'') {
                *dst++ = '\'';
            } else if (*src == '\\') {
                *dst++ = '\\';
            } else if (*src == 'x' && src + 2 < end) {
                /* Hex escape \xNN */
                char hex[3] = {src[1], src[2], '\0'};
                char *endp;
                long val = strtol(hex, &endp, 16);
                if (endp == hex + 2) {
                    *dst++ = (char)val;
                    src += 2;  /* Skip the two hex digits (x already skipped) */
                } else {
                    /* Invalid hex - keep as-is */
                    *dst++ = '\\';
                    *dst++ = *src;
                }
            } else if (*src == '0') {
                /* Null escape \0 */
                *dst++ = '\0';
            } else {
                /* Unknown escape - keep as-is */
                *dst++ = '\\';
                *dst++ = *src;
            }
            src++;
        } else {
            *dst++ = *src++;
        }
    }
    *dst = '\0';
    last_string_length = (int)(dst - result);
    return result;
}

void lexer_set_filename(const char *filename) {
    free((void *)current_filename);
    current_filename = filename ? strdup(filename) : NULL;
    current_line = 1;
    current_column = 1;
    line_buffer_pos = 0;
}

/* Set filename without resetting line/column - for COPY transparency */
void lexer_set_filename_only(const char *filename) {
    free((void *)current_filename);
    current_filename = filename ? strdup(filename) : NULL;
}

const char *lexer_get_filename(void) {
    return current_filename;
}

/* Buffer stack for COPY file handling - use flex's built-in stack */
#define MAX_INCLUDE_DEPTH 32
static FILE *file_stack[MAX_INCLUDE_DEPTH];
static int file_stack_ptr = 0;

/* Temporary storage for COPY filename while waiting for semicolon */
static char *copy_filename = NULL;

/* COPY keyword location (saved when COPY is seen, used when event is recorded) */
static int copy_start_line = 0;
static int copy_start_col = 0;

/* Forward declarations - defined in flex-generated code */
extern void yy_delete_buffer(YY_BUFFER_STATE b);
extern void yypop_buffer_state(void);

/* Full lexer cleanup - frees stacked flex buffers and resets state.
 * Must be called between successive parses to prevent buffer leaks.
 * Does NOT call yylex_destroy (which breaks subsequent yyrestart). */
void lexer_cleanup(void) {
    lexer_clear_filename_pool();
    free((void *)current_filename);
    current_filename = NULL;
    free(copy_filename);
    copy_filename = NULL;

    /* Pop any leftover COPY file buffers from the stack */
    while (file_stack_ptr > 0) {
        file_stack_ptr--;
        /* Close file handles left on the stack */
        if (file_stack[file_stack_ptr]) {
            fclose(file_stack[file_stack_ptr]);
            file_stack[file_stack_ptr] = NULL;
        }
        yypop_buffer_state();
    }
}

void lexer_push_buffer(FILE *fp) {
    if (file_stack_ptr >= MAX_INCLUDE_DEPTH) {
        /* Should not happen - preproc checks depth */
        return;
    }
    /* Save current file handle */
    file_stack[file_stack_ptr++] = yyin;

    /* Push current buffer and switch to new one */
    yypush_buffer_state(yy_create_buffer(fp, YY_BUF_SIZE));
    yyin = fp;
}

void lexer_pop_buffer(void) {
    if (file_stack_ptr <= 0) {
        return;
    }
    /* Close current file */
    if (yyin) {
        fclose(yyin);
    }

    /* Pop buffer (this deletes current and switches to previous) */
    yypop_buffer_state();

    /* Restore file handle */
    yyin = file_stack[--file_stack_ptr];
}

/*
 * Handle an identifier token: DEFINE expansion, keyword check, or plain IDENT.
 * Called from both {ALPHA}{ALNUM}* and {DIGIT}+{ALPHA}{ALNUM}* rules.
 *
 * If a DEFINE match is found, stores the expansion text in *expand_out
 * and returns 0. The caller (flex rule action) must unput() the expansion.
 * Otherwise returns the token type (IDENT, KW_FIRST, etc.).
 */
static const char *pending_expansion = NULL;

static int handle_identifier(void) {
    if (yyleng > 99) {
        SourceLoc loc = {current_filename, yylloc.first_line, yylloc.first_column};
        diag_error(loc, "Token buffer overflow");
    }

    char *upper = strupper(strdup(yytext));
    const char *def_value = preproc_lookup_define(upper);

    if (def_value) {
        if (preproc_is_expanding(upper)) {
            SourceLoc loc = {current_filename, yylloc.first_line, yylloc.first_column};
            diag_error(loc, "Circular DEFINE expansion: %s", upper);
            yylval.sval = upper;
            return IDENT;
        }

        if (g_preproc_events) {
            PreprocDefineInfo dinfo;
            if (preproc_lookup_define_info(upper, &dinfo)) {
                preproc_event_add_define_ref(g_preproc_events,
                    current_filename, yylloc.first_line, yylloc.first_column,
                    yylloc.first_column + yyleng - 1,
                    upper, dinfo.value, dinfo.filename, dinfo.line, dinfo.column);
            }
        }

        preproc_push_expansion(upper);
        free(upper);

        /* Signal caller to unput the expansion text */
        pending_expansion = def_value;
        return 0;
    } else {
        if (strcasecmp(upper, "FIRST") == 0) { free(upper); return KW_FIRST; }
        if (strcasecmp(upper, "NEXT") == 0)  { free(upper); return KW_NEXT; }
        if (strcasecmp(upper, "BACK") == 0)  { free(upper); return KW_BACK; }
        if (strcasecmp(upper, "LAST") == 0)  { free(upper); return KW_LAST; }
        yylval.ident_pair.canonical = upper;
        yylval.ident_pair.original = strdup(yytext);
        return IDENT;
    }
}

/*
 * Macro to call handle_identifier() and perform unput() if DEFINE expansion
 * is needed. Must be used inside a flex rule action (where unput is available).
 */
#define HANDLE_IDENT() do { \
    int tok = handle_identifier(); \
    if (tok) return tok; \
    /* DEFINE expansion: push text back into input */ \
    const char *exp = pending_expansion; \
    pending_expansion = NULL; \
    size_t len = strlen(exp); \
    for (int _i = (int)len - 1; _i >= 0; _i--) \
        unput(exp[_i]); \
} while(0)

/* Override default yylex declaration to create a wrapper */
#define YY_DECL int yylex_impl(void)

%}

%option case-insensitive
%option noyywrap
%option noinput

%x COPY_STATE
%x COPY_SEMI
%x DEFINE_NAME

DIGIT       [0-9]
HEXDIGIT    [0-9A-Fa-f]
ALPHA       [A-Za-z_]
ALNUM       [A-Za-z0-9_]
WS          [ \t\r]

%%

"PROGRAM"       { UPDATE_LOC(); SAVE_TEXT(); return KW_PROGRAM; }
"DATA"          { UPDATE_LOC(); SAVE_TEXT(); return KW_DATA; }
"DEFINE"        { UPDATE_LOC(); SAVE_TEXT(); BEGIN(DEFINE_NAME); return KW_DEFINE; }
"PROC"          { UPDATE_LOC(); SAVE_TEXT(); return KW_PROC; }
"END_PROC"      { UPDATE_LOC(); SAVE_TEXT(); return KW_END_PROC; }
"COPY"          { UPDATE_LOC(); SAVE_TEXT(); copy_start_line = yylloc.first_line; copy_start_col = yylloc.first_column; BEGIN(COPY_STATE); }
"IF"            { UPDATE_LOC(); SAVE_TEXT(); return KW_IF; }
"THEN"          { UPDATE_LOC(); SAVE_TEXT(); return KW_THEN; }
"ELSE"          { UPDATE_LOC(); SAVE_TEXT(); return KW_ELSE; }
"DO"            { UPDATE_LOC(); SAVE_TEXT(); return KW_DO; }
"END"           { UPDATE_LOC(); SAVE_TEXT(); return KW_END; }
"WHILE"         { UPDATE_LOC(); SAVE_TEXT(); return KW_WHILE; }
"GOTO"          { UPDATE_LOC(); SAVE_TEXT(); return KW_GOTO; }
"GOTO_DEPENDING_ON" { UPDATE_LOC(); SAVE_TEXT(); return KW_GOTO_DEPENDING_ON; }
"EXIT"          { UPDATE_LOC(); SAVE_TEXT(); return KW_EXIT; }
"RETURN"        { UPDATE_LOC(); SAVE_TEXT(); return KW_RETURN; }
"MOVE"          { UPDATE_LOC(); SAVE_TEXT(); return KW_MOVE; }
"ABS"           { UPDATE_LOC(); SAVE_TEXT(); return KW_ABS; }
"SWAP"          { UPDATE_LOC(); SAVE_TEXT(); return KW_SWAP; }
"FILL"          { UPDATE_LOC(); SAVE_TEXT(); return KW_FILL; }
"CLEAR"         { UPDATE_LOC(); SAVE_TEXT(); return KW_CLEAR; }
"PUSH"          { UPDATE_LOC(); SAVE_TEXT(); return KW_PUSH; }
"POP"           { UPDATE_LOC(); SAVE_TEXT(); return KW_POP; }
"STRING"        { UPDATE_LOC(); SAVE_TEXT(); return KW_STRING; }
"SUBSTR"        { UPDATE_LOC(); SAVE_TEXT(); return KW_SUBSTR; }
"INSTR"         { UPDATE_LOC(); SAVE_TEXT(); return KW_INSTR; }
"UPPERCASE"     { UPDATE_LOC(); SAVE_TEXT(); return KW_UPPERCASE; }
"LENGTH"        { UPDATE_LOC(); SAVE_TEXT(); return KW_LENGTH; }
"EDIT"          { UPDATE_LOC(); SAVE_TEXT(); return KW_EDIT; }
"ADD"           { UPDATE_LOC(); SAVE_TEXT(); return KW_ADD; }
"SUBTRACT"      { UPDATE_LOC(); SAVE_TEXT(); return KW_SUBTRACT; }
"MULTIPLY"      { UPDATE_LOC(); SAVE_TEXT(); return KW_MULTIPLY; }
"DIVIDE"        { UPDATE_LOC(); SAVE_TEXT(); return KW_DIVIDE; }
"AND"           { UPDATE_LOC(); SAVE_TEXT(); return KW_AND; }
"OR"            { UPDATE_LOC(); SAVE_TEXT(); return KW_OR; }
"XOR"           { UPDATE_LOC(); SAVE_TEXT(); return KW_XOR; }
"TEST"          { UPDATE_LOC(); SAVE_TEXT(); return KW_TEST; }
"NAVIGATE"      { UPDATE_LOC(); SAVE_TEXT(); return KW_NAVIGATE; }
"FETCH"         { UPDATE_LOC(); SAVE_TEXT(); return KW_FETCH; }
"OPEN_WINDOW"   { UPDATE_LOC(); SAVE_TEXT(); return KW_OPEN_WINDOW; }
"CLOSE_WINDOW"  { UPDATE_LOC(); SAVE_TEXT(); return KW_CLOSE_WINDOW; }
"OPEN_ERROR_WINDOW" { UPDATE_LOC(); SAVE_TEXT(); return KW_OPEN_ERROR_WINDOW; }
"KILL"          { UPDATE_LOC(); SAVE_TEXT(); return KW_KILL; }
"LINK"          { UPDATE_LOC(); SAVE_TEXT(); return KW_LINK; }
"TRANSFER"      { UPDATE_LOC(); SAVE_TEXT(); return KW_TRANSFER; }
"PURGE_CACHE"   { UPDATE_LOC(); SAVE_TEXT(); return KW_PURGE_CACHE; }
"OPEN"          { UPDATE_LOC(); SAVE_TEXT(); return KW_OPEN; }
"CLOSE"         { UPDATE_LOC(); SAVE_TEXT(); return KW_CLOSE; }
"READ"          { UPDATE_LOC(); SAVE_TEXT(); return KW_READ; }
"WRITE"         { UPDATE_LOC(); SAVE_TEXT(); return KW_WRITE; }
"NOTE"          { UPDATE_LOC(); SAVE_TEXT(); return KW_NOTE; }
"POINT"         { UPDATE_LOC(); SAVE_TEXT(); return KW_POINT; }
"DELETE"        { UPDATE_LOC(); SAVE_TEXT(); return KW_DELETE; }
"CONNECT"       { UPDATE_LOC(); SAVE_TEXT(); return KW_CONNECT; }
"DISCONNECT"    { UPDATE_LOC(); SAVE_TEXT(); return KW_DISCONNECT; }
"SEND"          { UPDATE_LOC(); SAVE_TEXT(); return KW_SEND; }
"TIMEOUT"       { UPDATE_LOC(); SAVE_TEXT(); return KW_TIMEOUT; }
"PRIORITY"      { UPDATE_LOC(); SAVE_TEXT(); return KW_PRIORITY; }
"OPT_HDRS"      { UPDATE_LOC(); SAVE_TEXT(); return KW_OPT_HDRS; }
"RECEIVE"       { UPDATE_LOC(); SAVE_TEXT(); return KW_RECEIVE; }
"CANCEL"        { UPDATE_LOC(); SAVE_TEXT(); return KW_CANCEL; }
"SAVE"          { UPDATE_LOC(); SAVE_TEXT(); return KW_SAVE; }
"RESTORE"       { UPDATE_LOC(); SAVE_TEXT(); return KW_RESTORE; }
"RELEASE"       { UPDATE_LOC(); SAVE_TEXT(); return KW_RELEASE; }
"WAIT"          { UPDATE_LOC(); SAVE_TEXT(); return KW_WAIT; }
"START"         { UPDATE_LOC(); SAVE_TEXT(); return KW_START; }
"STOP"          { UPDATE_LOC(); SAVE_TEXT(); return KW_STOP; }
"REFRESH"       { UPDATE_LOC(); SAVE_TEXT(); return KW_REFRESH; }
"ERASE"         { UPDATE_LOC(); SAVE_TEXT(); return KW_ERASE; }
"SET_CURSOR"    { UPDATE_LOC(); SAVE_TEXT(); return KW_SET_CURSOR; }
"SOUND"         { UPDATE_LOC(); SAVE_TEXT(); return KW_SOUND; }
"DEFINE_FIELD"  { UPDATE_LOC(); SAVE_TEXT(); return KW_DEFINE_FIELD; }
"SET_ATTRIBUTE" { UPDATE_LOC(); SAVE_TEXT(); return KW_SET_ATTRIBUTE; }
"SET_FUNCTION"  { UPDATE_LOC(); SAVE_TEXT(); return KW_SET_FUNCTION; }
"SET_KEY"       { UPDATE_LOC(); SAVE_TEXT(); return KW_SET_KEY; }
"FORMAT"        { UPDATE_LOC(); SAVE_TEXT(); return KW_FORMAT; }
"MAKE_FORMAT"   { UPDATE_LOC(); SAVE_TEXT(); return KW_MAKE_FORMAT; }
"LOOKUP"        { UPDATE_LOC(); SAVE_TEXT(); return KW_LOOKUP; }
"SORT"          { UPDATE_LOC(); SAVE_TEXT(); return KW_SORT; }
"ERROR"         { UPDATE_LOC(); SAVE_TEXT(); return KW_ERROR; }
"TRIGGER_FUNCTION" { UPDATE_LOC(); SAVE_TEXT(); return KW_TRIGGER_FUNCTION; }

    /* COPY_STATE: handle filename after COPY keyword */
<COPY_STATE>{WS}+     { current_column += yyleng; }
<COPY_STATE>\n        { NEWLINE(); }
<COPY_STATE>"{"[^}]*"}" {
    /* Comment in COPY state - skip */
    for (int i = 0; yytext[i]; i++) {
        if (yytext[i] == '\n') NEWLINE();
        else current_column++;
    }
}
<COPY_STATE>{ALPHA}{ALNUM}* |
<COPY_STATE>{DIGIT}+{ALPHA}{ALNUM}* {
    UPDATE_LOC();
    /* Save filename, then wait for semicolon before pushing file */
    copy_filename = strupper(strdup(yytext));
    BEGIN(COPY_SEMI);
}
<COPY_STATE>. {
    UPDATE_LOC();
    SourceLoc loc = {current_filename, yylloc.first_line, yylloc.first_column};
    diag_error(loc, "expected filename after COPY");
    BEGIN(INITIAL);
    yyless(0);
}

    /* COPY_SEMI: consume semicolon, then push file */
<COPY_SEMI>{WS}+      { current_column += yyleng; }
<COPY_SEMI>\n         { NEWLINE(); }
<COPY_SEMI>";" {
    UPDATE_LOC();
    /* Save current filename before push changes it to the COPY file */
    char *saved_filename = current_filename ? strdup(current_filename) : NULL;
    /* Now push the file - semicolon consumed from current buffer */
    if (preproc_push_file(copy_filename) == 0) {
        /* Record event for LSP */
        if (g_preproc_events) {
            int end_col = copy_start_col + 4 + 1 + (int)strlen(copy_filename);
            preproc_event_add_copy(g_preproc_events,
                saved_filename, copy_start_line, copy_start_col, end_col,
                copy_filename, preproc_get_last_resolved_path());
        }
    }
    free(saved_filename);
    free(copy_filename);
    copy_filename = NULL;
    BEGIN(INITIAL);
}
<COPY_SEMI>. {
    UPDATE_LOC();
    SourceLoc loc = {current_filename, yylloc.first_line, yylloc.first_column};
    diag_error(loc, "expected ';' after COPY filename");
    free(copy_filename);
    copy_filename = NULL;
    BEGIN(INITIAL);
    yyless(0);
}

    /* DEFINE_NAME: parse identifier without DEFINE expansion */
<DEFINE_NAME>{WS}+     { current_column += yyleng; SAVE_TEXT(); }
<DEFINE_NAME>\n        { NEWLINE(); }
<DEFINE_NAME>"{"[^}]*"}" {
    /* Comment in DEFINE_NAME state - skip */
    for (int i = 0; yytext[i]; i++) {
        if (yytext[i] == '\n') NEWLINE();
        else current_column++;
    }
}
<DEFINE_NAME>{ALPHA}{ALNUM}* |
<DEFINE_NAME>{DIGIT}+{ALPHA}{ALNUM}* {
    UPDATE_LOC(); SAVE_TEXT();
    /* Return as plain IDENT without DEFINE expansion */
    yylval.ident_pair.canonical = strupper(strdup(yytext));
    yylval.ident_pair.original = strdup(yytext);
    BEGIN(INITIAL);
    return IDENT;
}
<DEFINE_NAME>. {
    UPDATE_LOC();
    SourceLoc loc = {current_filename, yylloc.first_line, yylloc.first_column};
    diag_error(loc, "expected identifier after DEFINE");
    BEGIN(INITIAL);
    yyless(0);
}

"<>"            { UPDATE_LOC(); SAVE_TEXT(); return OP_NE; }
"<="            { UPDATE_LOC(); SAVE_TEXT(); return OP_LE; }
">="            { UPDATE_LOC(); SAVE_TEXT(); return OP_GE; }
"="             { UPDATE_LOC(); SAVE_TEXT(); return OP_EQ; }
"<"             { UPDATE_LOC(); SAVE_TEXT(); return OP_LT; }
">"             { UPDATE_LOC(); SAVE_TEXT(); return OP_GT; }

"I"[1-8]        { UPDATE_LOC(); SAVE_TEXT(); yylval.ival = yytext[1] - '0'; return REG_I; }
"D"[1-8]        { UPDATE_LOC(); SAVE_TEXT(); yylval.ival = yytext[1] - '0'; return REG_D; }
"P"[0-8]        { UPDATE_LOC(); SAVE_TEXT(); yylval.ival = yytext[1] - '0'; return REG_P; }

"&"{DIGIT}+     { UPDATE_LOC(); SAVE_TEXT(); yylval.ival = atoi(yytext + 1); return PEV; }
"#"{DIGIT}+     { UPDATE_LOC(); SAVE_TEXT(); yylval.ival = atoi(yytext + 1); return GEV; }

"RDA_FIRST"     { UPDATE_LOC(); SAVE_TEXT(); yylval.ival = 0; return RDA_SLOT; }
"RDA_LAST"      { UPDATE_LOC(); SAVE_TEXT(); yylval.ival = 221; return RDA_SLOT; }

"0x"{HEXDIGIT}+ { UPDATE_LOC(); SAVE_TEXT(); yylval.sval = strdup(yytext); return LIT_HEX; }
"-"{DIGIT}+     { UPDATE_LOC(); SAVE_TEXT(); yylval.sval = strdup(yytext); return LIT_NUM; }
{DIGIT}+"."{DIGIT}+ { UPDATE_LOC(); SAVE_TEXT(); yylval.sval = strdup(yytext); return LIT_NUM; }
"-"{DIGIT}+"."{DIGIT}+ { UPDATE_LOC(); SAVE_TEXT(); yylval.sval = strdup(yytext); return LIT_NUM; }
{DIGIT}+{ALPHA}{ALNUM}* {
    /* Digit-prefixed identifier (e.g., 6100DATA, 6100INIT) */
    UPDATE_LOC(); SAVE_TEXT();
    HANDLE_IDENT();
}
{DIGIT}+        { UPDATE_LOC(); SAVE_TEXT(); yylval.sval = strdup(yytext); return LIT_NUM; }

\'([^'\n\\]|\'\'|\\[\\\'0x]|\\x[0-9a-fA-F][0-9a-fA-F])*\' { UPDATE_LOC(); SAVE_TEXT(); yylval.sval = process_string(yytext); return LIT_STR; }

\'[^'\n]*\n     {
    UPDATE_LOC();
    SourceLoc loc = {current_filename, yylloc.first_line, yylloc.first_column};
    diag_error(loc, "unterminated string literal");
    NEWLINE();
    yylval.sval = strdup("");
    return LIT_STR;
}

{ALPHA}{ALNUM}* {
    UPDATE_LOC(); SAVE_TEXT();
    HANDLE_IDENT();
}

";"             { UPDATE_LOC(); SAVE_TEXT(); return ';'; }
","             { UPDATE_LOC(); SAVE_TEXT(); return ','; }
"("             { UPDATE_LOC(); SAVE_TEXT(); return '('; }
")"             { UPDATE_LOC(); SAVE_TEXT(); return ')'; }
":"             { UPDATE_LOC(); SAVE_TEXT(); return ':'; }

"{"[^}]*"}"     {
    for (int i = 0; yytext[i]; i++) {
        if (yytext[i] == '\n') {
            NEWLINE();
        } else {
            current_column++;
        }
    }
}

"{"[^}]*        {
    UPDATE_LOC();
    SourceLoc loc = {current_filename, yylloc.first_line, yylloc.first_column};
    diag_error(loc, "unterminated comment");
}

{WS}+           { current_column += yyleng; SAVE_TEXT(); }
\n              { NEWLINE(); }

.               {
    UPDATE_LOC();
    SourceLoc loc = {current_filename, yylloc.first_line, yylloc.first_column};
    diag_error(loc, "invalid character '%c' (0x%02x)", yytext[0], (unsigned char)yytext[0]);
}

<<EOF>>         {
    /* Try to pop include stack */
    if (preproc_pop_file()) {
        /* Switched to previous file - continue lexing from there */
        /* With yypop_buffer_state(), flex will continue from previous buffer */
    } else {
        /* No more files, end of input */
        yyterminate();
    }
}

%%

/*
 * Wrapper for yylex that clears DEFINE expansion tracking after each token.
 * This ensures cycle detection works correctly within a single token,
 * but resets between tokens.
 */
int yylex(void) {
    int token = yylex_impl();
    preproc_clear_expansions();
    return token;
}
