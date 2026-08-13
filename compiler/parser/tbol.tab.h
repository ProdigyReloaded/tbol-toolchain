/* A Bison parser, made by GNU Bison 2.3.  */

/* Skeleton interface for Bison's Yacc-like parsers in C

   Copyright (C) 1984, 1989, 1990, 2000, 2001, 2002, 2003, 2004, 2005, 2006
   Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.  */

/* As a special exception, you may create a larger work that contains
   part or all of the Bison parser skeleton and distribute that work
   under terms of your choice, so long as that work isn't itself a
   parser generator using the skeleton or a modified version thereof
   as a parser skeleton.  Alternatively, if you modify or redistribute
   the parser skeleton itself, you may (at your option) remove this
   special exception, which will cause the skeleton and the resulting
   Bison output files to be licensed under the GNU General Public
   License without this special exception.

   This special exception was added by the Free Software Foundation in
   version 2.2 of Bison.  */

/* Tokens.  */
#ifndef YYTOKENTYPE
# define YYTOKENTYPE
   /* Put the tokens into the symbol table, so that GDB and other debuggers
      know about them.  */
   enum yytokentype {
     KW_PROGRAM = 258,
     KW_DATA = 259,
     KW_DEFINE = 260,
     KW_PROC = 261,
     KW_END_PROC = 262,
     KW_IF = 263,
     KW_THEN = 264,
     KW_ELSE = 265,
     KW_DO = 266,
     KW_END = 267,
     KW_WHILE = 268,
     KW_GOTO = 269,
     KW_GOTO_DEPENDING_ON = 270,
     KW_EXIT = 271,
     KW_RETURN = 272,
     KW_MOVE = 273,
     KW_ABS = 274,
     KW_SWAP = 275,
     KW_FILL = 276,
     KW_CLEAR = 277,
     KW_PUSH = 278,
     KW_POP = 279,
     KW_STRING = 280,
     KW_SUBSTR = 281,
     KW_INSTR = 282,
     KW_UPPERCASE = 283,
     KW_LENGTH = 284,
     KW_EDIT = 285,
     KW_ADD = 286,
     KW_SUBTRACT = 287,
     KW_MULTIPLY = 288,
     KW_DIVIDE = 289,
     KW_AND = 290,
     KW_OR = 291,
     KW_XOR = 292,
     KW_TEST = 293,
     KW_NAVIGATE = 294,
     KW_FIRST = 295,
     KW_NEXT = 296,
     KW_BACK = 297,
     KW_LAST = 298,
     KW_FETCH = 299,
     KW_OPEN_WINDOW = 300,
     KW_CLOSE_WINDOW = 301,
     KW_OPEN_ERROR_WINDOW = 302,
     KW_KILL = 303,
     KW_LINK = 304,
     KW_TRANSFER = 305,
     KW_PURGE_CACHE = 306,
     KW_OPEN = 307,
     KW_CLOSE = 308,
     KW_READ = 309,
     KW_WRITE = 310,
     KW_NOTE = 311,
     KW_POINT = 312,
     KW_DELETE = 313,
     KW_CONNECT = 314,
     KW_DISCONNECT = 315,
     KW_SEND = 316,
     KW_RECEIVE = 317,
     KW_CANCEL = 318,
     KW_TIMEOUT = 319,
     KW_PRIORITY = 320,
     KW_OPT_HDRS = 321,
     KW_SAVE = 322,
     KW_RESTORE = 323,
     KW_RELEASE = 324,
     KW_WAIT = 325,
     KW_START = 326,
     KW_STOP = 327,
     KW_REFRESH = 328,
     KW_ERASE = 329,
     KW_SET_CURSOR = 330,
     KW_SOUND = 331,
     KW_DEFINE_FIELD = 332,
     KW_SET_ATTRIBUTE = 333,
     KW_SET_FUNCTION = 334,
     KW_SET_KEY = 335,
     KW_FORMAT = 336,
     KW_MAKE_FORMAT = 337,
     KW_LOOKUP = 338,
     KW_SORT = 339,
     KW_ERROR = 340,
     KW_TRIGGER_FUNCTION = 341,
     OP_EQ = 342,
     OP_NE = 343,
     OP_LT = 344,
     OP_GT = 345,
     OP_LE = 346,
     OP_GE = 347,
     REG_I = 348,
     REG_D = 349,
     REG_P = 350,
     PEV = 351,
     GEV = 352,
     RDA_SLOT = 353,
     LIT_STR = 354,
     LIT_NUM = 355,
     LIT_HEX = 356,
     IDENT = 357
   };
#endif
/* Tokens.  */
#define KW_PROGRAM 258
#define KW_DATA 259
#define KW_DEFINE 260
#define KW_PROC 261
#define KW_END_PROC 262
#define KW_IF 263
#define KW_THEN 264
#define KW_ELSE 265
#define KW_DO 266
#define KW_END 267
#define KW_WHILE 268
#define KW_GOTO 269
#define KW_GOTO_DEPENDING_ON 270
#define KW_EXIT 271
#define KW_RETURN 272
#define KW_MOVE 273
#define KW_ABS 274
#define KW_SWAP 275
#define KW_FILL 276
#define KW_CLEAR 277
#define KW_PUSH 278
#define KW_POP 279
#define KW_STRING 280
#define KW_SUBSTR 281
#define KW_INSTR 282
#define KW_UPPERCASE 283
#define KW_LENGTH 284
#define KW_EDIT 285
#define KW_ADD 286
#define KW_SUBTRACT 287
#define KW_MULTIPLY 288
#define KW_DIVIDE 289
#define KW_AND 290
#define KW_OR 291
#define KW_XOR 292
#define KW_TEST 293
#define KW_NAVIGATE 294
#define KW_FIRST 295
#define KW_NEXT 296
#define KW_BACK 297
#define KW_LAST 298
#define KW_FETCH 299
#define KW_OPEN_WINDOW 300
#define KW_CLOSE_WINDOW 301
#define KW_OPEN_ERROR_WINDOW 302
#define KW_KILL 303
#define KW_LINK 304
#define KW_TRANSFER 305
#define KW_PURGE_CACHE 306
#define KW_OPEN 307
#define KW_CLOSE 308
#define KW_READ 309
#define KW_WRITE 310
#define KW_NOTE 311
#define KW_POINT 312
#define KW_DELETE 313
#define KW_CONNECT 314
#define KW_DISCONNECT 315
#define KW_SEND 316
#define KW_RECEIVE 317
#define KW_CANCEL 318
#define KW_TIMEOUT 319
#define KW_PRIORITY 320
#define KW_OPT_HDRS 321
#define KW_SAVE 322
#define KW_RESTORE 323
#define KW_RELEASE 324
#define KW_WAIT 325
#define KW_START 326
#define KW_STOP 327
#define KW_REFRESH 328
#define KW_ERASE 329
#define KW_SET_CURSOR 330
#define KW_SOUND 331
#define KW_DEFINE_FIELD 332
#define KW_SET_ATTRIBUTE 333
#define KW_SET_FUNCTION 334
#define KW_SET_KEY 335
#define KW_FORMAT 336
#define KW_MAKE_FORMAT 337
#define KW_LOOKUP 338
#define KW_SORT 339
#define KW_ERROR 340
#define KW_TRIGGER_FUNCTION 341
#define OP_EQ 342
#define OP_NE 343
#define OP_LT 344
#define OP_GT 345
#define OP_LE 346
#define OP_GE 347
#define REG_I 348
#define REG_D 349
#define REG_P 350
#define PEV 351
#define GEV 352
#define RDA_SLOT 353
#define LIT_STR 354
#define LIT_NUM 355
#define LIT_HEX 356
#define IDENT 357




#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
typedef union YYSTYPE
#line 70 "compiler/parser/tbol.y"
{
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
/* Line 1529 of yacc.c.  */
#line 265 "compiler/parser/tbol.tab.h"
	YYSTYPE;
# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
# define YYSTYPE_IS_TRIVIAL 1
#endif

extern YYSTYPE yylval;

#if ! defined YYLTYPE && ! defined YYLTYPE_IS_DECLARED
typedef struct YYLTYPE
{
  int first_line;
  int first_column;
  int last_line;
  int last_column;
} YYLTYPE;
# define yyltype YYLTYPE /* obsolescent; will be withdrawn */
# define YYLTYPE_IS_DECLARED 1
# define YYLTYPE_IS_TRIVIAL 1
#endif

extern YYLTYPE yylloc;
