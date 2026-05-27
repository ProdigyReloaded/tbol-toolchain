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
 * TBOL Compiler - Token Definitions
 *
 * This file is included by both the lexer and parser.
 * Token values are defined by bison in tbol.tab.h
 */

#ifndef TBOLC_TOKENS_H
#define TBOLC_TOKENS_H

#include "../../shared/ast.h"

/* Token value union - matches %union in tbol.y */
typedef union {
    int ival;
    int64_t i64val;
    char *sval;
    AstNode *node;
    CmpOp cmp_op;
} TokenValue;

/* Token with location */
typedef struct {
    int type;
    TokenValue value;
    SourceLoc loc;
} Token;

/* Verb ID enum for fast lookup */
typedef enum {
    VERB_UNKNOWN = 0,

    /* Arithmetic */
    VERB_ADD,
    VERB_SUBTRACT,
    VERB_MULTIPLY,
    VERB_DIVIDE,

    /* Bitwise */
    VERB_AND,
    VERB_OR,
    VERB_XOR,
    VERB_TEST,

    /* Data Movement */
    VERB_MOVE,
    VERB_SWAP,
    VERB_FILL,
    VERB_CLEAR,
    VERB_PUSH,
    VERB_POP,

    /* String */
    VERB_STRING,
    VERB_SUBSTR,
    VERB_INSTR,
    VERB_UPPERCASE,
    VERB_LENGTH,
    VERB_EDIT,
    VERB_FORMAT,
    VERB_MAKE_FORMAT,

    /* Control Flow */
    VERB_GOTO,
    VERB_GOTO_DEPENDING_ON,
    VERB_EXIT,
    VERB_RETURN,
    VERB_ERROR,
    VERB_TRIGGER_FUNCTION,

    /* Navigation/Objects */
    VERB_NAVIGATE,
    VERB_FETCH,
    VERB_LINK,
    VERB_TRANSFER,
    VERB_OPEN_WINDOW,
    VERB_CLOSE_WINDOW,
    VERB_OPEN_ERROR_WINDOW,
    VERB_KILL,
    VERB_PURGE_CACHE,

    /* File I/O */
    VERB_OPEN,
    VERB_CLOSE,
    VERB_READ,
    VERB_WRITE,
    VERB_NOTE,
    VERB_POINT,
    VERB_DELETEFILE,

    /* Communications */
    VERB_CONNECT,
    VERB_DISCONNECT,
    VERB_SEND,
    VERB_RECEIVE,
    VERB_CANCEL,

    /* State Management */
    VERB_SAVE,
    VERB_SYNC_SAVE,
    VERB_RESTORE,
    VERB_RELEASE,
    VERB_SYNC_RELEASE,

    /* Display/Fields */
    VERB_REFRESH,
    VERB_ERASE,
    VERB_SET_CURSOR,
    VERB_SET_ATTRIBUTE,
    VERB_SET_FUNCTION,
    VERB_SET_KEY,
    VERB_DEFINE_FIELD,
    VERB_SOUND,

    /* Data Operations */
    VERB_LOOKUP,
    VERB_SORT,
    VERB_ACCESS,

    /* Timer/Process */
    VERB_TIMER_ON,
    VERB_TIMER_OFF,
    VERB_WAIT,
    VERB_START,
    VERB_STOP,

    /* Tracking */
    VERB_TRACK,
    VERB_LOG,

    /* Screen */
    VERB_FILE_SCREEN,
    VERB_SHOW_SCREEN,
    VERB_UPLOAD,
    VERB_DOWNLOAD,
    VERB_SET_BACKGROUND,

    VERB_COUNT  /* Total count */
} VerbId;

#endif /* TBOLC_TOKENS_H */
