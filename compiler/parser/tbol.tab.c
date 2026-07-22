/* A Bison parser, made by GNU Bison 2.3.  */

/* Skeleton implementation for Bison's Yacc-like parsers in C

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

/* C LALR(1) parser skeleton written by Richard Stallman, by
   simplifying the original so-called "semantic" parser.  */

/* All symbols defined below should begin with yy or YY, to avoid
   infringing on user name space.  This should be done even for local
   variables, as they might otherwise be expanded by user macros.
   There are some unavoidable exceptions within include files to
   define necessary library symbols; they are noted "INFRINGES ON
   USER NAME SPACE" below.  */

/* Identify Bison output.  */
#define YYBISON 1

/* Bison version.  */
#define YYBISON_VERSION "2.3"

/* Skeleton name.  */
#define YYSKELETON_NAME "yacc.c"

/* Pure parsers.  */
#define YYPURE 0

/* Using locations.  */
#define YYLSP_NEEDED 1



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




/* Copy the first part of user declarations.  */
#line 17 "compiler/parser/tbol.y"

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



/* Enabling traces.  */
#ifndef YYDEBUG
# define YYDEBUG 0
#endif

/* Enabling verbose error messages.  */
#ifdef YYERROR_VERBOSE
# undef YYERROR_VERBOSE
# define YYERROR_VERBOSE 1
#else
# define YYERROR_VERBOSE 1
#endif

/* Enabling the token table.  */
#ifndef YYTOKEN_TABLE
# define YYTOKEN_TABLE 0
#endif

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
/* Line 193 of yacc.c.  */
#line 360 "compiler/parser/tbol.tab.c"
	YYSTYPE;
# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
# define YYSTYPE_IS_TRIVIAL 1
#endif

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


/* Copy the second part of user declarations.  */


/* Line 216 of yacc.c.  */
#line 385 "compiler/parser/tbol.tab.c"

#ifdef short
# undef short
#endif

#ifdef YYTYPE_UINT8
typedef YYTYPE_UINT8 yytype_uint8;
#else
typedef unsigned char yytype_uint8;
#endif

#ifdef YYTYPE_INT8
typedef YYTYPE_INT8 yytype_int8;
#elif (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
typedef signed char yytype_int8;
#else
typedef short int yytype_int8;
#endif

#ifdef YYTYPE_UINT16
typedef YYTYPE_UINT16 yytype_uint16;
#else
typedef unsigned short int yytype_uint16;
#endif

#ifdef YYTYPE_INT16
typedef YYTYPE_INT16 yytype_int16;
#else
typedef short int yytype_int16;
#endif

#ifndef YYSIZE_T
# ifdef __SIZE_TYPE__
#  define YYSIZE_T __SIZE_TYPE__
# elif defined size_t
#  define YYSIZE_T size_t
# elif ! defined YYSIZE_T && (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
#  include <stddef.h> /* INFRINGES ON USER NAME SPACE */
#  define YYSIZE_T size_t
# else
#  define YYSIZE_T unsigned int
# endif
#endif

#define YYSIZE_MAXIMUM ((YYSIZE_T) -1)

#ifndef YY_
# if defined YYENABLE_NLS && YYENABLE_NLS
#  if ENABLE_NLS
#   include <libintl.h> /* INFRINGES ON USER NAME SPACE */
#   define YY_(msgid) dgettext ("bison-runtime", msgid)
#  endif
# endif
# ifndef YY_
#  define YY_(msgid) msgid
# endif
#endif

/* Suppress unused-variable warnings by "using" E.  */
#if ! defined lint || defined __GNUC__
# define YYUSE(e) ((void) (e))
#else
# define YYUSE(e) /* empty */
#endif

/* Identity function, used to suppress warnings about constant conditions.  */
#ifndef lint
# define YYID(n) (n)
#else
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static int
YYID (int i)
#else
static int
YYID (i)
    int i;
#endif
{
  return i;
}
#endif

#if ! defined yyoverflow || YYERROR_VERBOSE

/* The parser invokes alloca or malloc; define the necessary symbols.  */

# ifdef YYSTACK_USE_ALLOCA
#  if YYSTACK_USE_ALLOCA
#   ifdef __GNUC__
#    define YYSTACK_ALLOC __builtin_alloca
#   elif defined __BUILTIN_VA_ARG_INCR
#    include <alloca.h> /* INFRINGES ON USER NAME SPACE */
#   elif defined _AIX
#    define YYSTACK_ALLOC __alloca
#   elif defined _MSC_VER
#    include <malloc.h> /* INFRINGES ON USER NAME SPACE */
#    define alloca _alloca
#   else
#    define YYSTACK_ALLOC alloca
#    if ! defined _ALLOCA_H && ! defined _STDLIB_H && (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
#     include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
#     ifndef _STDLIB_H
#      define _STDLIB_H 1
#     endif
#    endif
#   endif
#  endif
# endif

# ifdef YYSTACK_ALLOC
   /* Pacify GCC's `empty if-body' warning.  */
#  define YYSTACK_FREE(Ptr) do { /* empty */; } while (YYID (0))
#  ifndef YYSTACK_ALLOC_MAXIMUM
    /* The OS might guarantee only one guard page at the bottom of the stack,
       and a page size can be as small as 4096 bytes.  So we cannot safely
       invoke alloca (N) if N exceeds 4096.  Use a slightly smaller number
       to allow for a few compiler-allocated temporary stack slots.  */
#   define YYSTACK_ALLOC_MAXIMUM 4032 /* reasonable circa 2006 */
#  endif
# else
#  define YYSTACK_ALLOC YYMALLOC
#  define YYSTACK_FREE YYFREE
#  ifndef YYSTACK_ALLOC_MAXIMUM
#   define YYSTACK_ALLOC_MAXIMUM YYSIZE_MAXIMUM
#  endif
#  if (defined __cplusplus && ! defined _STDLIB_H \
       && ! ((defined YYMALLOC || defined malloc) \
	     && (defined YYFREE || defined free)))
#   include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
#   ifndef _STDLIB_H
#    define _STDLIB_H 1
#   endif
#  endif
#  ifndef YYMALLOC
#   define YYMALLOC malloc
#   if ! defined malloc && ! defined _STDLIB_H && (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
void *malloc (YYSIZE_T); /* INFRINGES ON USER NAME SPACE */
#   endif
#  endif
#  ifndef YYFREE
#   define YYFREE free
#   if ! defined free && ! defined _STDLIB_H && (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
void free (void *); /* INFRINGES ON USER NAME SPACE */
#   endif
#  endif
# endif
#endif /* ! defined yyoverflow || YYERROR_VERBOSE */


#if (! defined yyoverflow \
     && (! defined __cplusplus \
	 || (defined YYLTYPE_IS_TRIVIAL && YYLTYPE_IS_TRIVIAL \
	     && defined YYSTYPE_IS_TRIVIAL && YYSTYPE_IS_TRIVIAL)))

/* A type that is properly aligned for any stack member.  */
union yyalloc
{
  yytype_int16 yyss;
  YYSTYPE yyvs;
    YYLTYPE yyls;
};

/* The size of the maximum gap between one aligned stack and the next.  */
# define YYSTACK_GAP_MAXIMUM (sizeof (union yyalloc) - 1)

/* The size of an array large to enough to hold all stacks, each with
   N elements.  */
# define YYSTACK_BYTES(N) \
     ((N) * (sizeof (yytype_int16) + sizeof (YYSTYPE) + sizeof (YYLTYPE)) \
      + 2 * YYSTACK_GAP_MAXIMUM)

/* Copy COUNT objects from FROM to TO.  The source and destination do
   not overlap.  */
# ifndef YYCOPY
#  if defined __GNUC__ && 1 < __GNUC__
#   define YYCOPY(To, From, Count) \
      __builtin_memcpy (To, From, (Count) * sizeof (*(From)))
#  else
#   define YYCOPY(To, From, Count)		\
      do					\
	{					\
	  YYSIZE_T yyi;				\
	  for (yyi = 0; yyi < (Count); yyi++)	\
	    (To)[yyi] = (From)[yyi];		\
	}					\
      while (YYID (0))
#  endif
# endif

/* Relocate STACK from its old location to the new one.  The
   local variables YYSIZE and YYSTACKSIZE give the old and new number of
   elements in the stack, and YYPTR gives the new location of the
   stack.  Advance YYPTR to a properly aligned location for the next
   stack.  */
# define YYSTACK_RELOCATE(Stack)					\
    do									\
      {									\
	YYSIZE_T yynewbytes;						\
	YYCOPY (&yyptr->Stack, Stack, yysize);				\
	Stack = &yyptr->Stack;						\
	yynewbytes = yystacksize * sizeof (*Stack) + YYSTACK_GAP_MAXIMUM; \
	yyptr += yynewbytes / sizeof (*yyptr);				\
      }									\
    while (YYID (0))

#endif

/* YYFINAL -- State number of the termination state.  */
#define YYFINAL  6
/* YYLAST -- Last index in YYTABLE.  */
#define YYLAST   1109

/* YYNTOKENS -- Number of terminals.  */
#define YYNTOKENS  108
/* YYNNTS -- Number of nonterminals.  */
#define YYNNTS  36
/* YYNRULES -- Number of rules.  */
#define YYNRULES  242
/* YYNRULES -- Number of states.  */
#define YYNSTATES  531

/* YYTRANSLATE(YYLEX) -- Bison symbol number corresponding to YYLEX.  */
#define YYUNDEFTOK  2
#define YYMAXUTOK   357

#define YYTRANSLATE(YYX)						\
  ((unsigned int) (YYX) <= YYMAXUTOK ? yytranslate[YYX] : YYUNDEFTOK)

/* YYTRANSLATE[YYLEX] -- Bison symbol number corresponding to YYLEX.  */
static const yytype_uint8 yytranslate[] =
{
       0,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     105,   106,     2,     2,   104,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,   107,   103,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     1,     2,     3,     4,
       5,     6,     7,     8,     9,    10,    11,    12,    13,    14,
      15,    16,    17,    18,    19,    20,    21,    22,    23,    24,
      25,    26,    27,    28,    29,    30,    31,    32,    33,    34,
      35,    36,    37,    38,    39,    40,    41,    42,    43,    44,
      45,    46,    47,    48,    49,    50,    51,    52,    53,    54,
      55,    56,    57,    58,    59,    60,    61,    62,    63,    64,
      65,    66,    67,    68,    69,    70,    71,    72,    73,    74,
      75,    76,    77,    78,    79,    80,    81,    82,    83,    84,
      85,    86,    87,    88,    89,    90,    91,    92,    93,    94,
      95,    96,    97,    98,    99,   100,   101,   102
};

#if YYDEBUG
/* YYPRHS[YYN] -- Index of the first RHS symbol of rule number YYN in
   YYRHS.  */
static const yytype_uint16 yyprhs[] =
{
       0,     0,     3,     9,    15,    17,    18,    21,    23,    25,
      27,    30,    35,    41,    45,    47,    51,    53,    58,    64,
      66,    69,    73,    79,    85,    88,    91,    94,    95,    98,
     101,   103,   105,   107,   109,   111,   113,   115,   117,   119,
     121,   123,   125,   127,   129,   131,   133,   135,   137,   139,
     141,   143,   145,   147,   149,   151,   153,   155,   157,   159,
     161,   163,   165,   167,   169,   171,   173,   175,   177,   179,
     181,   183,   185,   187,   189,   191,   193,   195,   197,   199,
     201,   203,   205,   207,   209,   211,   213,   215,   217,   219,
     221,   223,   225,   228,   230,   232,   234,   236,   239,   245,
     247,   249,   251,   253,   258,   265,   270,   275,   280,   282,
     286,   288,   292,   296,   300,   302,   304,   306,   308,   310,
     312,   314,   316,   318,   320,   322,   325,   327,   330,   332,
     335,   337,   340,   345,   347,   350,   352,   355,   357,   359,
     362,   364,   367,   369,   372,   374,   377,   380,   383,   386,
     389,   394,   397,   400,   403,   406,   409,   412,   415,   418,
     421,   424,   427,   430,   435,   438,   443,   448,   453,   458,
     463,   470,   475,   480,   485,   490,   495,   500,   507,   512,
     515,   520,   525,   532,   537,   542,   547,   552,   557,   562,
     569,   574,   581,   586,   593,   598,   603,   610,   617,   624,
     631,   638,   645,   654,   659,   666,   675,   686,   699,   714,
     717,   722,   725,   728,   731,   736,   738,   742,   744,   748,
     750,   754,   758,   763,   769,   771,   774,   776,   778,   780,
     782,   784,   786,   788,   790,   792,   794,   796,   801,   803,
     807,   812,   814
};

/* YYRHS -- A `-1'-separated list of the rules' RHS.  */
static const yytype_int16 yyrhs[] =
{
     109,     0,    -1,     3,   102,   103,   110,   117,    -1,     3,
       1,   103,   110,   117,    -1,     1,    -1,    -1,   110,   111,
      -1,   113,    -1,   112,    -1,   116,    -1,     1,   103,    -1,
     102,    87,   114,   103,    -1,     4,   102,    87,   114,   103,
      -1,     4,     1,   103,    -1,   115,    -1,   114,   104,   115,
      -1,   102,    -1,   102,   105,   100,   106,    -1,     5,   102,
     104,   141,   103,    -1,   118,    -1,   117,   118,    -1,   117,
       1,   103,    -1,     6,   102,    87,   119,     7,    -1,     6,
     102,    87,     1,     7,    -1,   120,   123,    -1,   119,   123,
      -1,   119,   121,    -1,    -1,   120,   121,    -1,   122,   107,
      -1,   102,    -1,    16,    -1,    17,    -1,    18,    -1,    19,
      -1,    20,    -1,    21,    -1,    22,    -1,    23,    -1,    24,
      -1,    25,    -1,    26,    -1,    27,    -1,    28,    -1,    29,
      -1,    30,    -1,    31,    -1,    32,    -1,    33,    -1,    34,
      -1,    37,    -1,    38,    -1,    39,    -1,    44,    -1,    45,
      -1,    46,    -1,    47,    -1,    48,    -1,    49,    -1,    50,
      -1,    51,    -1,    52,    -1,    53,    -1,    54,    -1,    55,
      -1,    56,    -1,    57,    -1,    58,    -1,    59,    -1,    60,
      -1,    61,    -1,    62,    -1,    63,    -1,    67,    -1,    68,
      -1,    69,    -1,    70,    -1,    71,    -1,    72,    -1,    73,
      -1,    74,    -1,    75,    -1,    76,    -1,    77,    -1,    78,
      -1,    79,    -1,    80,    -1,    81,    -1,    83,    -1,    84,
      -1,    85,    -1,    86,    -1,   125,   103,    -1,   127,    -1,
     128,    -1,   124,    -1,   103,    -1,     1,   103,    -1,     5,
     102,   104,   141,   103,    -1,   134,    -1,   139,    -1,   123,
      -1,   129,    -1,     8,   130,     9,   126,    -1,     8,   130,
       9,   126,    10,   126,    -1,    13,   130,     9,   126,    -1,
      11,   119,    12,   103,    -1,    11,     1,    12,   103,    -1,
     131,    -1,   130,    36,   131,    -1,   132,    -1,   131,    35,
     132,    -1,   141,   133,   141,    -1,   105,   130,   106,    -1,
      87,    -1,    88,    -1,    89,    -1,    90,    -1,    91,    -1,
      92,    -1,    60,    -1,    51,    -1,    73,    -1,    70,    -1,
      17,    -1,    17,   141,    -1,    16,    -1,    16,   141,    -1,
      46,    -1,    46,   141,    -1,    47,    -1,    47,   141,    -1,
      47,   141,   104,   141,    -1,    74,    -1,    74,   141,    -1,
      75,    -1,    75,   141,    -1,    76,    -1,    48,    -1,    48,
     141,    -1,    63,    -1,    63,   141,    -1,    71,    -1,    71,
     141,    -1,    72,    -1,    72,   141,    -1,    22,   141,    -1,
      53,   141,    -1,    59,   141,    -1,    39,   141,    -1,    39,
     141,   104,   141,    -1,    39,    40,    -1,    39,    41,    -1,
      39,    42,    -1,    39,    43,    -1,    45,   141,    -1,    24,
     141,    -1,    23,   141,    -1,    69,   141,    -1,    28,   141,
      -1,    85,   141,    -1,    86,   141,    -1,    58,   141,    -1,
      22,   141,   104,   141,    -1,    44,   141,    -1,    44,   141,
     104,   141,    -1,    31,   141,   104,   141,    -1,    35,   141,
     104,   141,    -1,    29,   141,   104,   141,    -1,    18,   141,
     104,   141,    -1,    18,   141,   104,   141,   104,    19,    -1,
      33,   141,   104,   141,    -1,    56,   141,   104,   141,    -1,
      52,   141,   104,   141,    -1,    36,   141,   104,   141,    -1,
      57,   141,   104,   141,    -1,    54,   141,   104,   141,    -1,
      54,   141,   104,   141,   104,   141,    -1,    62,   141,   104,
     141,    -1,    61,   141,    -1,    61,   141,   104,   141,    -1,
      61,   141,   104,   142,    -1,    61,   141,   104,   141,   104,
     142,    -1,    78,   141,   104,   141,    -1,    76,   141,   104,
     141,    -1,    32,   141,   104,   141,    -1,    20,   141,   104,
     141,    -1,    38,   141,   104,   141,    -1,    55,   141,   104,
     141,    -1,    55,   141,   104,   141,   104,   141,    -1,    37,
     141,   104,   141,    -1,    21,   141,   104,   141,   104,   141,
      -1,    67,   141,   104,   141,    -1,    67,   141,   104,   141,
     104,   141,    -1,    68,   141,   104,   141,    -1,    34,   141,
     104,   141,    -1,    34,   141,   104,   141,   104,   141,    -1,
      27,   141,   104,   141,   104,   141,    -1,    81,   141,   104,
     141,   104,   141,    -1,    84,   141,   104,   141,   104,   141,
      -1,    80,   141,   104,   141,   104,   141,    -1,    30,   141,
     104,   141,   104,   136,    -1,    26,   141,   104,   141,   104,
     141,   104,   141,    -1,    79,   141,   104,   141,    -1,    79,
     141,   104,   141,   104,   141,    -1,    79,   141,   104,   141,
     104,   141,   104,   141,    -1,    83,   141,   104,   141,   104,
     141,   104,   141,   104,   141,    -1,    77,   141,   104,   141,
     104,   141,   104,   141,   104,   141,   104,   141,    -1,    77,
     141,   104,   141,   104,   141,   104,   141,   104,   141,   104,
     141,   104,   141,    -1,    14,   122,    -1,    15,   141,   104,
     135,    -1,    25,   136,    -1,    49,   136,    -1,    50,   136,
      -1,    82,   141,   104,   137,    -1,   122,    -1,   135,   104,
     122,    -1,   141,    -1,   136,   104,   141,    -1,   138,    -1,
     137,   104,   138,    -1,   141,   107,   100,    -1,   141,   107,
     107,   100,    -1,   141,   107,   100,   107,   100,    -1,   102,
      -1,   102,   136,    -1,   102,    -1,    93,    -1,    94,    -1,
      95,    -1,    98,    -1,    96,    -1,    97,    -1,    99,    -1,
     100,    -1,   101,    -1,   140,    -1,   140,   105,   141,   106,
      -1,   143,    -1,   142,   104,   143,    -1,    64,   105,   100,
     106,    -1,    65,    -1,    66,    -1
};

/* YYRLINE[YYN] -- source line where rule number YYN was defined.  */
static const yytype_uint16 yyrline[] =
{
       0,   176,   176,   198,   218,   228,   229,   240,   241,   242,
     243,   251,   267,   280,   287,   291,   298,   304,   320,   379,
     383,   387,   395,   408,   422,   427,   431,   439,   440,   451,
     463,   465,   466,   468,   469,   470,   471,   472,   473,   474,
     476,   477,   478,   479,   480,   481,   483,   484,   485,   486,
     487,   488,   490,   491,   492,   493,   494,   495,   496,   497,
     498,   500,   501,   502,   503,   504,   505,   506,   508,   509,
     510,   511,   512,   514,   515,   516,   518,   519,   520,   522,
     523,   524,   525,   527,   528,   529,   530,   532,   533,   534,
     535,   536,   540,   547,   550,   553,   556,   560,   568,   621,
     622,   629,   630,   634,   640,   650,   659,   670,   679,   680,
     687,   688,   695,   699,   705,   706,   707,   708,   709,   710,
     716,   717,   718,   719,   722,   725,   729,   732,   736,   739,
     743,   746,   750,   755,   758,   762,   765,   769,   772,   775,
     779,   782,   786,   789,   793,   796,   802,   806,   810,   814,
     818,   823,   826,   829,   832,   835,   839,   843,   847,   851,
     855,   859,   863,   869,   874,   878,   885,   890,   895,   900,
     905,   910,   915,   920,   925,   930,   935,   940,   946,   951,
     958,   966,   973,   981,   986,   991,   996,  1001,  1006,  1011,
    1017,  1022,  1030,  1035,  1041,  1046,  1051,  1059,  1065,  1071,
    1077,  1083,  1098,  1105,  1110,  1116,  1125,  1135,  1144,  1156,
    1161,  1175,  1185,  1195,  1207,  1222,  1227,  1235,  1239,  1246,
    1250,  1257,  1263,  1269,  1280,  1285,  1301,  1306,  1307,  1308,
    1309,  1310,  1311,  1315,  1320,  1325,  1330,  1333,  1342,  1345,
    1352,  1357,  1361
};
#endif

#if YYDEBUG || YYERROR_VERBOSE || YYTOKEN_TABLE
/* YYTNAME[SYMBOL-NUM] -- String name of the symbol SYMBOL-NUM.
   First, the terminals, then, starting at YYNTOKENS, nonterminals.  */
static const char *const yytname[] =
{
  "$end", "error", "$undefined", "KW_PROGRAM", "KW_DATA", "KW_DEFINE",
  "KW_PROC", "KW_END_PROC", "KW_IF", "KW_THEN", "KW_ELSE", "KW_DO",
  "KW_END", "KW_WHILE", "KW_GOTO", "KW_GOTO_DEPENDING_ON", "KW_EXIT",
  "KW_RETURN", "KW_MOVE", "KW_ABS", "KW_SWAP", "KW_FILL", "KW_CLEAR",
  "KW_PUSH", "KW_POP", "KW_STRING", "KW_SUBSTR", "KW_INSTR",
  "KW_UPPERCASE", "KW_LENGTH", "KW_EDIT", "KW_ADD", "KW_SUBTRACT",
  "KW_MULTIPLY", "KW_DIVIDE", "KW_AND", "KW_OR", "KW_XOR", "KW_TEST",
  "KW_NAVIGATE", "KW_FIRST", "KW_NEXT", "KW_BACK", "KW_LAST", "KW_FETCH",
  "KW_OPEN_WINDOW", "KW_CLOSE_WINDOW", "KW_OPEN_ERROR_WINDOW", "KW_KILL",
  "KW_LINK", "KW_TRANSFER", "KW_PURGE_CACHE", "KW_OPEN", "KW_CLOSE",
  "KW_READ", "KW_WRITE", "KW_NOTE", "KW_POINT", "KW_DELETE", "KW_CONNECT",
  "KW_DISCONNECT", "KW_SEND", "KW_RECEIVE", "KW_CANCEL", "KW_TIMEOUT",
  "KW_PRIORITY", "KW_OPT_HDRS", "KW_SAVE", "KW_RESTORE", "KW_RELEASE",
  "KW_WAIT", "KW_START", "KW_STOP", "KW_REFRESH", "KW_ERASE",
  "KW_SET_CURSOR", "KW_SOUND", "KW_DEFINE_FIELD", "KW_SET_ATTRIBUTE",
  "KW_SET_FUNCTION", "KW_SET_KEY", "KW_FORMAT", "KW_MAKE_FORMAT",
  "KW_LOOKUP", "KW_SORT", "KW_ERROR", "KW_TRIGGER_FUNCTION", "OP_EQ",
  "OP_NE", "OP_LT", "OP_GT", "OP_LE", "OP_GE", "REG_I", "REG_D", "REG_P",
  "PEV", "GEV", "RDA_SLOT", "LIT_STR", "LIT_NUM", "LIT_HEX", "IDENT",
  "';'", "','", "'('", "')'", "':'", "$accept", "program", "definitions",
  "definition", "data_group", "data_section", "var_decl_list", "var_decl",
  "define_stmt", "proc_list", "proc", "stmt_list", "label_prefix", "label",
  "label_name", "statement", "define_in_proc", "simple_stmt", "then_body",
  "if_stmt", "while_stmt", "do_block", "condition", "condition_term",
  "condition_factor", "cmp_op", "verb_stmt", "label_list", "expr_list",
  "format_spec_list", "format_spec", "proc_call", "indexable", "expr",
  "send_modifiers", "send_modifier", 0
};
#endif

# ifdef YYPRINT
/* YYTOKNUM[YYLEX-NUM] -- Internal token number corresponding to
   token YYLEX-NUM.  */
static const yytype_uint16 yytoknum[] =
{
       0,   256,   257,   258,   259,   260,   261,   262,   263,   264,
     265,   266,   267,   268,   269,   270,   271,   272,   273,   274,
     275,   276,   277,   278,   279,   280,   281,   282,   283,   284,
     285,   286,   287,   288,   289,   290,   291,   292,   293,   294,
     295,   296,   297,   298,   299,   300,   301,   302,   303,   304,
     305,   306,   307,   308,   309,   310,   311,   312,   313,   314,
     315,   316,   317,   318,   319,   320,   321,   322,   323,   324,
     325,   326,   327,   328,   329,   330,   331,   332,   333,   334,
     335,   336,   337,   338,   339,   340,   341,   342,   343,   344,
     345,   346,   347,   348,   349,   350,   351,   352,   353,   354,
     355,   356,   357,    59,    44,    40,    41,    58
};
# endif

/* YYR1[YYN] -- Symbol number of symbol that rule YYN derives.  */
static const yytype_uint8 yyr1[] =
{
       0,   108,   109,   109,   109,   110,   110,   111,   111,   111,
     111,   112,   113,   113,   114,   114,   115,   115,   116,   117,
     117,   117,   118,   118,   119,   119,   119,   120,   120,   121,
     122,   122,   122,   122,   122,   122,   122,   122,   122,   122,
     122,   122,   122,   122,   122,   122,   122,   122,   122,   122,
     122,   122,   122,   122,   122,   122,   122,   122,   122,   122,
     122,   122,   122,   122,   122,   122,   122,   122,   122,   122,
     122,   122,   122,   122,   122,   122,   122,   122,   122,   122,
     122,   122,   122,   122,   122,   122,   122,   122,   122,   122,
     122,   122,   123,   123,   123,   123,   123,   123,   124,   125,
     125,   126,   126,   127,   127,   128,   129,   129,   130,   130,
     131,   131,   132,   132,   133,   133,   133,   133,   133,   133,
     134,   134,   134,   134,   134,   134,   134,   134,   134,   134,
     134,   134,   134,   134,   134,   134,   134,   134,   134,   134,
     134,   134,   134,   134,   134,   134,   134,   134,   134,   134,
     134,   134,   134,   134,   134,   134,   134,   134,   134,   134,
     134,   134,   134,   134,   134,   134,   134,   134,   134,   134,
     134,   134,   134,   134,   134,   134,   134,   134,   134,   134,
     134,   134,   134,   134,   134,   134,   134,   134,   134,   134,
     134,   134,   134,   134,   134,   134,   134,   134,   134,   134,
     134,   134,   134,   134,   134,   134,   134,   134,   134,   134,
     134,   134,   134,   134,   134,   135,   135,   136,   136,   137,
     137,   138,   138,   138,   139,   139,   140,   140,   140,   140,
     140,   140,   140,   141,   141,   141,   141,   141,   142,   142,
     143,   143,   143
};

/* YYR2[YYN] -- Number of symbols composing right hand side of rule YYN.  */
static const yytype_uint8 yyr2[] =
{
       0,     2,     5,     5,     1,     0,     2,     1,     1,     1,
       2,     4,     5,     3,     1,     3,     1,     4,     5,     1,
       2,     3,     5,     5,     2,     2,     2,     0,     2,     2,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     2,     1,     1,     1,     1,     2,     5,     1,
       1,     1,     1,     4,     6,     4,     4,     4,     1,     3,
       1,     3,     3,     3,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     2,     1,     2,     1,     2,
       1,     2,     4,     1,     2,     1,     2,     1,     1,     2,
       1,     2,     1,     2,     1,     2,     2,     2,     2,     2,
       4,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     4,     2,     4,     4,     4,     4,     4,
       6,     4,     4,     4,     4,     4,     4,     6,     4,     2,
       4,     4,     6,     4,     4,     4,     4,     4,     4,     6,
       4,     6,     4,     6,     4,     4,     6,     6,     6,     6,
       6,     6,     8,     4,     6,     8,    10,    12,    14,     2,
       4,     2,     2,     2,     4,     1,     3,     1,     3,     1,
       3,     3,     4,     5,     1,     2,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     4,     1,     3,
       4,     1,     1
};

/* YYDEFACT[STATE-NAME] -- Default rule to reduce with in state
   STATE-NUM when YYTABLE doesn't specify something else to do.  Zero
   means the default is an error.  */
static const yytype_uint8 yydefact[] =
{
       0,     4,     0,     0,     0,     0,     1,     5,     5,     0,
       0,     0,     0,     0,     0,     0,     6,     8,     7,     9,
       0,    19,     0,    10,     0,     0,     0,     0,     0,     0,
      20,    13,     0,     0,     0,    16,     0,    14,    21,     0,
     227,   228,   229,   231,   232,   230,   233,   234,   235,   226,
     236,     0,     0,     0,     0,     0,    11,     0,    12,     0,
      18,    23,     0,     0,    22,     0,     0,     0,     0,    31,
      32,    33,    34,    35,    36,    37,    38,    39,    40,    41,
      42,    43,    44,    45,    46,    47,    48,    49,     0,     0,
      50,    51,    52,    53,    54,    55,    56,    57,    58,    59,
      60,    61,    62,    63,    64,    65,    66,    67,    68,    69,
      70,    71,    72,    73,    74,    75,    76,    77,    78,    79,
      80,    81,    82,    83,    84,    85,    86,    87,     0,    88,
      89,    90,    91,    30,    96,    26,     0,    25,    95,     0,
      93,    94,    99,   100,    28,    24,     0,    15,     0,    97,
       0,     0,     0,   108,   110,     0,     0,    31,    32,    33,
      35,    36,    37,    38,    39,    40,    41,    42,    43,    44,
      45,    46,    47,    48,    49,    50,    51,    52,    53,    54,
      55,    56,    57,    58,    59,    60,    61,    62,    63,    64,
      65,    66,    67,    68,    69,    70,    71,    72,    73,    74,
      75,    76,    77,    78,    79,    80,    81,    82,    83,    84,
      85,    86,    87,    88,    89,    90,    91,    30,   209,     0,
     127,   125,     0,     0,     0,   146,   157,   156,   211,   217,
       0,     0,   159,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   151,   152,   153,   154,   149,   164,   155,
     129,   131,   139,   212,   213,     0,   147,     0,     0,     0,
       0,   162,   148,   179,     0,   141,     0,     0,   158,   143,
     145,   134,   136,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   160,   161,   225,    29,    92,    17,   237,     0,
       0,     0,     0,     0,   114,   115,   116,   117,   118,   119,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   113,     0,   126,   124,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   128,   130,   138,
       0,     0,   121,     0,     0,     0,     0,     0,     0,     0,
       0,   120,     0,     0,   140,     0,     0,     0,   123,   142,
     144,   122,   133,   135,   137,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   224,   101,   103,   102,   109,   111,
     112,   105,   215,   210,   169,   186,     0,   163,   218,     0,
       0,   168,     0,   166,   185,   171,   195,   167,   174,   190,
     187,   150,   165,   132,   173,   176,   188,   172,   175,     0,
     241,   242,   180,   181,   238,   178,   192,   194,   184,     0,
     183,   203,     0,     0,   214,   219,     0,     0,     0,    98,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   104,   216,   170,   191,
       0,   197,   201,   196,   177,   189,     0,   182,   239,   193,
       0,   204,   200,   198,   220,   221,     0,     0,   199,   107,
     106,     0,   240,     0,     0,     0,   222,     0,   202,     0,
     205,   223,     0,     0,     0,     0,   206,     0,   207,     0,
     208
};

/* YYDEFGOTO[NTERM-NUM].  */
static const yytype_int16 yydefgoto[] =
{
      -1,     3,     9,    16,    17,    18,    36,    37,    19,    20,
      21,    53,    54,   135,   136,   405,   138,   139,   406,   140,
     141,   407,   152,   153,   154,   300,   142,   413,   228,   454,
     455,   143,    50,   229,   443,   444
};

/* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
   STATE-NUM.  */
#define YYPACT_NINF -298
static const yytype_int16 yypact[] =
{
      64,  -298,     5,    11,   -79,   -76,  -298,  -298,  -298,     4,
       4,   -69,     6,   -36,   -26,    -1,  -298,  -298,  -298,  -298,
      19,  -298,    22,  -298,     1,    32,    16,    34,     7,    20,
    -298,  -298,     7,   135,   578,    17,   -88,  -298,  -298,    13,
    -298,  -298,  -298,  -298,  -298,  -298,  -298,  -298,  -298,  -298,
      21,    24,   118,   123,   669,    29,  -298,     7,  -298,   135,
    -298,  -298,    30,    61,  -298,   122,   122,   941,   135,   281,
     292,   135,  -298,   135,   135,   135,   135,   135,   135,   135,
     135,   135,   135,   135,   135,   135,   135,   135,   135,   135,
     135,   135,   360,   135,   135,   317,   328,   847,   135,   135,
      31,   135,   135,   135,   135,   135,   135,   135,   135,    63,
     135,   135,   935,   135,   135,   135,    84,   951,   962,    85,
     973,   984,   995,   135,   135,   135,   135,   135,   135,   135,
     135,   135,   135,  1006,  -298,  -298,    25,  -298,  -298,    86,
    -298,  -298,  -298,  -298,  -298,  -298,    58,  -298,   107,  -298,
     110,   122,     3,   214,  -298,    23,     9,  -298,  -298,  -298,
    -298,  -298,  -298,  -298,  -298,  -298,  -298,  -298,  -298,  -298,
    -298,  -298,  -298,  -298,  -298,  -298,  -298,  -298,  -298,  -298,
    -298,  -298,  -298,  -298,  -298,  -298,  -298,  -298,  -298,  -298,
    -298,  -298,  -298,  -298,  -298,  -298,  -298,  -298,  -298,  -298,
    -298,  -298,  -298,  -298,  -298,  -298,  -298,  -298,  -298,  -298,
    -298,  -298,  -298,  -298,  -298,  -298,  -298,  -298,  -298,   147,
    -298,  -298,   148,   149,   150,   151,  -298,  -298,   153,  -298,
     154,   157,  -298,   158,   159,   160,   161,   162,   164,   165,
     204,   205,   206,  -298,  -298,  -298,  -298,   215,   233,  -298,
    -298,   234,  -298,   153,   153,   235,  -298,   244,   251,   254,
     267,  -298,  -298,   293,   294,  -298,   300,   301,  -298,  -298,
    -298,  -298,  -298,   303,   304,   331,   335,   336,   337,   344,
     347,   348,  -298,  -298,   153,  -298,  -298,  -298,  -298,   135,
     -23,   762,   122,   122,  -298,  -298,  -298,  -298,  -298,  -298,
     135,   762,   941,   135,   135,   135,   135,   135,   135,   135,
     135,   135,   135,   135,   135,   135,   135,   135,   135,   135,
     135,   135,   135,   135,   135,   135,   135,   135,   146,   135,
     135,   135,   135,   135,   135,   135,   135,   135,   135,   135,
     135,   361,  -298,   853,   135,   135,   135,   135,   135,   135,
     135,   135,   135,   135,   135,   135,   135,   135,   135,   135,
     135,   135,   135,   135,   360,   135,   135,   135,   135,   135,
     135,   135,  -298,   135,   135,   135,   135,   135,   135,   135,
     135,  -298,   135,   135,   135,   135,   135,   135,  -298,   135,
     135,  -298,   135,   135,   135,   135,   135,   135,   135,   135,
     135,   135,   135,   135,   135,  -298,   240,  -298,   214,  -298,
    -298,  -298,  -298,   359,   362,  -298,   363,  -298,  -298,   364,
     365,  -298,   367,  -298,  -298,  -298,   368,  -298,  -298,  -298,
    -298,  -298,  -298,  -298,  -298,   369,   370,  -298,  -298,   291,
    -298,  -298,   371,   372,  -298,  -298,   373,  -298,  -298,   375,
    -298,   378,   379,   381,   382,  -298,   358,   383,   385,  -298,
     481,   487,   762,   941,   478,   135,   135,   135,   135,   135,
     135,   135,   398,   -35,   -35,   135,   135,   135,   135,   135,
     135,   -86,   135,   135,   424,   425,  -298,  -298,  -298,  -298,
     426,  -298,   153,  -298,  -298,  -298,   423,   372,  -298,  -298,
     447,   448,  -298,  -298,  -298,   446,   474,   471,  -298,  -298,
    -298,   135,  -298,   135,   135,   476,  -298,   135,  -298,   473,
    -298,  -298,   477,   135,   135,   480,  -298,   135,   483,   135,
    -298
};

/* YYPGOTO[NTERM-NUM].  */
static const yytype_int16 yypgoto[] =
{
    -298,  -298,   570,  -298,  -298,  -298,   548,   525,  -298,   575,
      83,   245,  -298,   564,   -64,   -52,  -298,  -298,  -297,  -298,
    -298,  -298,   -49,   327,   349,  -298,  -298,  -298,     2,  -298,
     140,  -298,  -298,   -33,   170,   191
};

/* YYTABLE[YYPACT[STATE-NUM]].  What to do in state STATE-NUM.  If
   positive, shift that token.  If negative, reduce the rule which
   number is the opposite.  If zero, do what YYDEFACT says.
   If YYTABLE_NINF, syntax error.  */
#define YYTABLE_NINF -225
static const yytype_int16 yytable[] =
{
      51,   137,   145,   218,   411,    11,     4,    24,    12,    13,
      14,     6,   291,   292,   505,    56,    57,   156,   301,    -3,
      29,   506,    -2,    29,     7,    14,   148,     8,    14,   439,
     440,   441,   155,   155,    23,   219,   220,   221,   222,   292,
     223,   224,   225,   226,   227,   292,   230,   231,   232,   233,
     234,   235,   236,   237,   238,   239,   240,   241,   242,   247,
     248,   249,   250,   251,   252,     1,    26,     2,   255,   256,
     257,   258,   259,   260,   261,   262,    27,   263,   264,   265,
     266,   267,   268,   342,   269,   270,    28,   271,   272,   273,
     274,   275,   276,   277,   278,   279,   280,   281,   282,   283,
     253,   254,   290,    30,    31,    30,    15,     5,    25,    35,
     294,   295,   296,   297,   298,   299,    58,    57,   155,    32,
      33,    34,    55,    38,    62,    61,    59,    60,    63,   146,
      64,    65,   285,   149,  -121,   284,    66,    67,    68,    69,
      70,    71,    72,    73,    74,    75,    76,    77,    78,    79,
      80,    81,    82,    83,    84,    85,    86,    87,    88,    89,
      90,    91,    92,   150,   287,   486,  -120,    93,    94,    95,
      96,    97,    98,    99,   100,   101,   102,   103,   104,   105,
     106,   107,   108,   109,   110,   111,   112,  -123,  -122,   286,
     113,   114,   115,   116,   117,   118,   119,   120,   121,   122,
     123,   124,   125,   126,   127,   128,   129,   130,   131,   132,
     439,   440,   441,   288,   289,    40,    41,    42,    43,    44,
      45,    46,    47,    48,    49,   133,   134,   151,    40,    41,
      42,    43,    44,    45,    46,    47,    48,    49,   412,    40,
      41,    42,    43,    44,    45,    46,    47,    48,    49,   293,
     462,   302,   303,   304,   305,   306,   341,   307,   308,   155,
     155,   309,   310,   311,   312,   313,   314,   410,   315,   316,
     414,   415,   416,   417,   418,   419,   420,   421,   422,   423,
     424,   425,   426,   427,   428,   429,   430,   431,   432,   433,
     434,   435,   436,   437,   438,   442,   445,   446,   447,   448,
     449,   450,   451,   452,   453,   456,   457,   458,   317,   318,
     319,   220,   221,   222,   223,   224,   225,   226,   227,   320,
     230,   231,   232,   233,   234,   235,   236,   237,   238,   241,
     242,   247,   248,   249,   250,   251,   252,   321,   322,   323,
     255,   256,   257,   258,   259,   260,   261,   262,   324,   263,
     264,   265,   266,   267,   268,   325,   269,   270,   326,   271,
     272,   273,   274,   275,   276,   277,   278,   280,   281,   282,
     283,   327,   253,   254,    40,    41,    42,    43,    44,    45,
      46,    47,    48,    49,  -126,    40,    41,    42,    43,    44,
      45,    46,    47,    48,    49,  -124,   472,   328,   329,   487,
     243,   244,   245,   246,   330,   331,   284,   332,   333,   137,
      40,    41,    42,    43,    44,    45,    46,    47,    48,    49,
    -128,    40,    41,    42,    43,    44,    45,    46,    47,    48,
      49,  -130,   489,   490,   491,   334,   493,   494,   495,   335,
     336,   337,   499,   500,   501,   502,   503,   456,   338,   507,
     508,   339,   340,    40,    41,    42,    43,    44,    45,    46,
      47,    48,    49,   463,   459,   481,   464,   465,   466,   467,
     492,   468,   469,   470,   471,   473,   474,   475,   518,   476,
     519,   520,   477,   478,   522,   479,   480,   482,    62,   483,
     525,   526,    63,   484,   528,    65,   530,   488,   496,   485,
      66,    67,    68,    69,    70,    71,    72,    73,    74,    75,
      76,    77,    78,    79,    80,    81,    82,    83,    84,    85,
      86,    87,    88,    89,    90,    91,    92,   509,   510,   512,
     511,    93,    94,    95,    96,    97,    98,    99,   100,   101,
     102,   103,   104,   105,   106,   107,   108,   109,   110,   111,
     112,   513,   514,   515,   113,   114,   115,   116,   117,   118,
     119,   120,   121,   122,   123,   124,   125,   126,   127,   128,
     129,   130,   131,   132,   516,   517,   521,   523,    10,    52,
      39,   524,   147,   -27,   527,    22,   -27,   529,   461,   133,
     134,   -27,   -27,   -27,   -27,   -27,   -27,   -27,   -27,   -27,
     -27,   -27,   -27,   -27,   -27,   -27,   -27,   -27,   -27,   -27,
     -27,   -27,   -27,   -27,   -27,   -27,   -27,   -27,   144,   408,
     504,     0,   -27,   -27,   -27,   -27,   -27,   -27,   -27,   -27,
     -27,   -27,   -27,   -27,   -27,   -27,   -27,   -27,   -27,   -27,
     -27,   -27,   409,   497,     0,   -27,   -27,   -27,   -27,   -27,
     -27,   -27,   -27,   -27,   -27,   -27,   -27,   -27,   -27,   -27,
     -27,   -27,   -27,   -27,   -27,   498,     0,     0,     0,     0,
      62,     0,     0,     0,    63,     0,     0,    65,     0,     0,
     -27,   -27,    66,    67,    68,    69,    70,    71,    72,    73,
      74,    75,    76,    77,    78,    79,    80,    81,    82,    83,
      84,    85,    86,    87,    88,    89,    90,    91,    92,     0,
       0,     0,     0,    93,    94,    95,    96,    97,    98,    99,
     100,   101,   102,   103,   104,   105,   106,   107,   108,   109,
     110,   111,   112,     0,     0,     0,   113,   114,   115,   116,
     117,   118,   119,   120,   121,   122,   123,   124,   125,   126,
     127,   128,   129,   130,   131,   132,     0,     0,     0,     0,
       0,     0,     0,    62,     0,     0,     0,    63,     0,     0,
      65,   133,   134,   343,     0,    66,    67,    68,   344,   345,
     346,     0,   347,   348,   349,   350,   351,   352,   353,   354,
     355,   356,   357,   358,   359,   360,   361,    88,    89,   362,
     363,   364,     0,     0,     0,     0,   365,   366,   367,   368,
     369,   370,   371,   372,   373,   374,   375,   376,   377,   378,
     379,   380,   381,   382,   383,   384,     0,     0,     0,   385,
     386,   387,   388,   389,   390,   391,   392,   393,   394,   395,
     396,   397,   398,   399,   128,   400,   401,   402,   403,     0,
       0,     0,     0,     0,   460,     0,     0,     0,   -27,     0,
       0,   -27,     0,     0,   404,   134,   -27,   -27,   -27,   -27,
     -27,   -27,   -27,   -27,   -27,   -27,   -27,   -27,   -27,   -27,
     -27,   -27,   -27,   -27,   -27,   -27,   -27,   -27,   -27,   -27,
     -27,   -27,   -27,     0,     0,     0,     0,   -27,   -27,   -27,
     -27,   -27,   -27,   -27,   -27,   -27,   -27,   -27,   -27,   -27,
     -27,   -27,   -27,   -27,   -27,   -27,   -27,     0,     0,     0,
     -27,   -27,   -27,   -27,   -27,   -27,   -27,   -27,   -27,   -27,
     -27,   -27,   -27,   -27,   -27,   -27,   -27,   -27,   -27,   -27,
      40,    41,    42,    43,    44,    45,    46,    47,    48,    49,
    -138,     0,     0,     0,     0,   -27,   -27,   157,   158,   159,
      72,   160,   161,   162,   163,   164,   165,   166,   167,   168,
     169,   170,   171,   172,   173,   174,     0,     0,   175,   176,
     177,     0,     0,     0,     0,   178,   179,   180,   181,   182,
     183,   184,   185,   186,   187,   188,   189,   190,   191,   192,
     193,   194,   195,   196,   197,     0,     0,     0,   198,   199,
     200,   201,   202,   203,   204,   205,   206,   207,   208,   209,
     210,   211,   212,     0,   213,   214,   215,   216,    40,    41,
      42,    43,    44,    45,    46,    47,    48,    49,  -140,     0,
       0,     0,     0,   217,    40,    41,    42,    43,    44,    45,
      46,    47,    48,    49,  -142,    40,    41,    42,    43,    44,
      45,    46,    47,    48,    49,  -144,    40,    41,    42,    43,
      44,    45,    46,    47,    48,    49,  -133,    40,    41,    42,
      43,    44,    45,    46,    47,    48,    49,  -135,    40,    41,
      42,    43,    44,    45,    46,    47,    48,    49,  -137,    40,
      41,    42,    43,    44,    45,    46,    47,    48,    49,  -224
};

static const yytype_int16 yycheck[] =
{
      33,    53,    54,    67,   301,     1,     1,     1,     4,     5,
       6,     0,     9,    36,   100,   103,   104,    66,     9,     0,
       1,   107,     0,     1,   103,     6,    59,   103,     6,    64,
      65,    66,    65,    66,   103,    68,    69,    70,    71,    36,
      73,    74,    75,    76,    77,    36,    79,    80,    81,    82,
      83,    84,    85,    86,    87,    88,    89,    90,    91,    92,
      93,    94,    95,    96,    97,     1,   102,     3,   101,   102,
     103,   104,   105,   106,   107,   108,   102,   110,   111,   112,
     113,   114,   115,   106,   117,   118,    87,   120,   121,   122,
     123,   124,   125,   126,   127,   128,   129,   130,   131,   132,
      98,    99,   151,    20,   103,    22,   102,   102,   102,   102,
      87,    88,    89,    90,    91,    92,   103,   104,   151,    87,
     104,    87,   105,   103,     1,     7,   105,   103,     5,   100,
       7,     8,   107,   103,   103,   133,    13,    14,    15,    16,
      17,    18,    19,    20,    21,    22,    23,    24,    25,    26,
      27,    28,    29,    30,    31,    32,    33,    34,    35,    36,
      37,    38,    39,   102,   106,   462,   103,    44,    45,    46,
      47,    48,    49,    50,    51,    52,    53,    54,    55,    56,
      57,    58,    59,    60,    61,    62,    63,   103,   103,   103,
      67,    68,    69,    70,    71,    72,    73,    74,    75,    76,
      77,    78,    79,    80,    81,    82,    83,    84,    85,    86,
      64,    65,    66,   106,   104,    93,    94,    95,    96,    97,
      98,    99,   100,   101,   102,   102,   103,   105,    93,    94,
      95,    96,    97,    98,    99,   100,   101,   102,   302,    93,
      94,    95,    96,    97,    98,    99,   100,   101,   102,    35,
      10,   104,   104,   104,   104,   104,   289,   104,   104,   292,
     293,   104,   104,   104,   104,   104,   104,   300,   104,   104,
     303,   304,   305,   306,   307,   308,   309,   310,   311,   312,
     313,   314,   315,   316,   317,   318,   319,   320,   321,   322,
     323,   324,   325,   326,   327,   328,   329,   330,   331,   332,
     333,   334,   335,   336,   337,   338,   339,   340,   104,   104,
     104,   344,   345,   346,   347,   348,   349,   350,   351,   104,
     353,   354,   355,   356,   357,   358,   359,   360,   361,   362,
     363,   364,   365,   366,   367,   368,   369,   104,   104,   104,
     373,   374,   375,   376,   377,   378,   379,   380,   104,   382,
     383,   384,   385,   386,   387,   104,   389,   390,   104,   392,
     393,   394,   395,   396,   397,   398,   399,   400,   401,   402,
     403,   104,   370,   371,    93,    94,    95,    96,    97,    98,
      99,   100,   101,   102,   103,    93,    94,    95,    96,    97,
      98,    99,   100,   101,   102,   103,   105,   104,   104,   463,
      40,    41,    42,    43,   104,   104,   404,   104,   104,   461,
      93,    94,    95,    96,    97,    98,    99,   100,   101,   102,
     103,    93,    94,    95,    96,    97,    98,    99,   100,   101,
     102,   103,   465,   466,   467,   104,   469,   470,   471,   104,
     104,   104,   475,   476,   477,   478,   479,   480,   104,   482,
     483,   104,   104,    93,    94,    95,    96,    97,    98,    99,
     100,   101,   102,   104,   103,   107,   104,   104,   104,   104,
     468,   104,   104,   104,   104,   104,   104,   104,   511,   104,
     513,   514,   104,   104,   517,   104,   104,   104,     1,   104,
     523,   524,     5,    12,   527,     8,   529,    19,   100,    12,
      13,    14,    15,    16,    17,    18,    19,    20,    21,    22,
      23,    24,    25,    26,    27,    28,    29,    30,    31,    32,
      33,    34,    35,    36,    37,    38,    39,   103,   103,   106,
     104,    44,    45,    46,    47,    48,    49,    50,    51,    52,
      53,    54,    55,    56,    57,    58,    59,    60,    61,    62,
      63,   104,   104,   107,    67,    68,    69,    70,    71,    72,
      73,    74,    75,    76,    77,    78,    79,    80,    81,    82,
      83,    84,    85,    86,   100,   104,   100,   104,     8,     1,
      32,   104,    57,     5,   104,    10,     8,   104,   343,   102,
     103,    13,    14,    15,    16,    17,    18,    19,    20,    21,
      22,    23,    24,    25,    26,    27,    28,    29,    30,    31,
      32,    33,    34,    35,    36,    37,    38,    39,    54,   292,
     480,    -1,    44,    45,    46,    47,    48,    49,    50,    51,
      52,    53,    54,    55,    56,    57,    58,    59,    60,    61,
      62,    63,   293,   473,    -1,    67,    68,    69,    70,    71,
      72,    73,    74,    75,    76,    77,    78,    79,    80,    81,
      82,    83,    84,    85,    86,   474,    -1,    -1,    -1,    -1,
       1,    -1,    -1,    -1,     5,    -1,    -1,     8,    -1,    -1,
     102,   103,    13,    14,    15,    16,    17,    18,    19,    20,
      21,    22,    23,    24,    25,    26,    27,    28,    29,    30,
      31,    32,    33,    34,    35,    36,    37,    38,    39,    -1,
      -1,    -1,    -1,    44,    45,    46,    47,    48,    49,    50,
      51,    52,    53,    54,    55,    56,    57,    58,    59,    60,
      61,    62,    63,    -1,    -1,    -1,    67,    68,    69,    70,
      71,    72,    73,    74,    75,    76,    77,    78,    79,    80,
      81,    82,    83,    84,    85,    86,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,     1,    -1,    -1,    -1,     5,    -1,    -1,
       8,   102,   103,    11,    -1,    13,    14,    15,    16,    17,
      18,    -1,    20,    21,    22,    23,    24,    25,    26,    27,
      28,    29,    30,    31,    32,    33,    34,    35,    36,    37,
      38,    39,    -1,    -1,    -1,    -1,    44,    45,    46,    47,
      48,    49,    50,    51,    52,    53,    54,    55,    56,    57,
      58,    59,    60,    61,    62,    63,    -1,    -1,    -1,    67,
      68,    69,    70,    71,    72,    73,    74,    75,    76,    77,
      78,    79,    80,    81,    82,    83,    84,    85,    86,    -1,
      -1,    -1,    -1,    -1,     1,    -1,    -1,    -1,     5,    -1,
      -1,     8,    -1,    -1,   102,   103,    13,    14,    15,    16,
      17,    18,    19,    20,    21,    22,    23,    24,    25,    26,
      27,    28,    29,    30,    31,    32,    33,    34,    35,    36,
      37,    38,    39,    -1,    -1,    -1,    -1,    44,    45,    46,
      47,    48,    49,    50,    51,    52,    53,    54,    55,    56,
      57,    58,    59,    60,    61,    62,    63,    -1,    -1,    -1,
      67,    68,    69,    70,    71,    72,    73,    74,    75,    76,
      77,    78,    79,    80,    81,    82,    83,    84,    85,    86,
      93,    94,    95,    96,    97,    98,    99,   100,   101,   102,
     103,    -1,    -1,    -1,    -1,   102,   103,    16,    17,    18,
      19,    20,    21,    22,    23,    24,    25,    26,    27,    28,
      29,    30,    31,    32,    33,    34,    -1,    -1,    37,    38,
      39,    -1,    -1,    -1,    -1,    44,    45,    46,    47,    48,
      49,    50,    51,    52,    53,    54,    55,    56,    57,    58,
      59,    60,    61,    62,    63,    -1,    -1,    -1,    67,    68,
      69,    70,    71,    72,    73,    74,    75,    76,    77,    78,
      79,    80,    81,    -1,    83,    84,    85,    86,    93,    94,
      95,    96,    97,    98,    99,   100,   101,   102,   103,    -1,
      -1,    -1,    -1,   102,    93,    94,    95,    96,    97,    98,
      99,   100,   101,   102,   103,    93,    94,    95,    96,    97,
      98,    99,   100,   101,   102,   103,    93,    94,    95,    96,
      97,    98,    99,   100,   101,   102,   103,    93,    94,    95,
      96,    97,    98,    99,   100,   101,   102,   103,    93,    94,
      95,    96,    97,    98,    99,   100,   101,   102,   103,    93,
      94,    95,    96,    97,    98,    99,   100,   101,   102,   103
};

/* YYSTOS[STATE-NUM] -- The (internal number of the) accessing
   symbol of state STATE-NUM.  */
static const yytype_uint8 yystos[] =
{
       0,     1,     3,   109,     1,   102,     0,   103,   103,   110,
     110,     1,     4,     5,     6,   102,   111,   112,   113,   116,
     117,   118,   117,   103,     1,   102,   102,   102,    87,     1,
     118,   103,    87,   104,    87,   102,   114,   115,   103,   114,
      93,    94,    95,    96,    97,    98,    99,   100,   101,   102,
     140,   141,     1,   119,   120,   105,   103,   104,   103,   105,
     103,     7,     1,     5,     7,     8,    13,    14,    15,    16,
      17,    18,    19,    20,    21,    22,    23,    24,    25,    26,
      27,    28,    29,    30,    31,    32,    33,    34,    35,    36,
      37,    38,    39,    44,    45,    46,    47,    48,    49,    50,
      51,    52,    53,    54,    55,    56,    57,    58,    59,    60,
      61,    62,    63,    67,    68,    69,    70,    71,    72,    73,
      74,    75,    76,    77,    78,    79,    80,    81,    82,    83,
      84,    85,    86,   102,   103,   121,   122,   123,   124,   125,
     127,   128,   134,   139,   121,   123,   100,   115,   141,   103,
     102,   105,   130,   131,   132,   141,   130,    16,    17,    18,
      20,    21,    22,    23,    24,    25,    26,    27,    28,    29,
      30,    31,    32,    33,    34,    37,    38,    39,    44,    45,
      46,    47,    48,    49,    50,    51,    52,    53,    54,    55,
      56,    57,    58,    59,    60,    61,    62,    63,    67,    68,
      69,    70,    71,    72,    73,    74,    75,    76,    77,    78,
      79,    80,    81,    83,    84,    85,    86,   102,   122,   141,
     141,   141,   141,   141,   141,   141,   141,   141,   136,   141,
     141,   141,   141,   141,   141,   141,   141,   141,   141,   141,
     141,   141,   141,    40,    41,    42,    43,   141,   141,   141,
     141,   141,   141,   136,   136,   141,   141,   141,   141,   141,
     141,   141,   141,   141,   141,   141,   141,   141,   141,   141,
     141,   141,   141,   141,   141,   141,   141,   141,   141,   141,
     141,   141,   141,   141,   136,   107,   103,   106,   106,   104,
     130,     9,    36,    35,    87,    88,    89,    90,    91,    92,
     133,     9,   104,   104,   104,   104,   104,   104,   104,   104,
     104,   104,   104,   104,   104,   104,   104,   104,   104,   104,
     104,   104,   104,   104,   104,   104,   104,   104,   104,   104,
     104,   104,   104,   104,   104,   104,   104,   104,   104,   104,
     104,   141,   106,    11,    16,    17,    18,    20,    21,    22,
      23,    24,    25,    26,    27,    28,    29,    30,    31,    32,
      33,    34,    37,    38,    39,    44,    45,    46,    47,    48,
      49,    50,    51,    52,    53,    54,    55,    56,    57,    58,
      59,    60,    61,    62,    63,    67,    68,    69,    70,    71,
      72,    73,    74,    75,    76,    77,    78,    79,    80,    81,
      83,    84,    85,    86,   102,   123,   126,   129,   131,   132,
     141,   126,   122,   135,   141,   141,   141,   141,   141,   141,
     141,   141,   141,   141,   141,   141,   141,   141,   141,   141,
     141,   141,   141,   141,   141,   141,   141,   141,   141,    64,
      65,    66,   141,   142,   143,   141,   141,   141,   141,   141,
     141,   141,   141,   141,   137,   138,   141,   141,   141,   103,
       1,   119,    10,   104,   104,   104,   104,   104,   104,   104,
     104,   104,   105,   104,   104,   104,   104,   104,   104,   104,
     104,   107,   104,   104,    12,    12,   126,   122,    19,   141,
     141,   141,   136,   141,   141,   141,   100,   142,   143,   141,
     141,   141,   141,   141,   138,   100,   107,   141,   141,   103,
     103,   104,   106,   104,   104,   107,   100,   104,   141,   141,
     141,   100,   141,   104,   104,   141,   141,   104,   141,   104,
     141
};

#define yyerrok		(yyerrstatus = 0)
#define yyclearin	(yychar = YYEMPTY)
#define YYEMPTY		(-2)
#define YYEOF		0

#define YYACCEPT	goto yyacceptlab
#define YYABORT		goto yyabortlab
#define YYERROR		goto yyerrorlab


/* Like YYERROR except do call yyerror.  This remains here temporarily
   to ease the transition to the new meaning of YYERROR, for GCC.
   Once GCC version 2 has supplanted version 1, this can go.  */

#define YYFAIL		goto yyerrlab

#define YYRECOVERING()  (!!yyerrstatus)

#define YYBACKUP(Token, Value)					\
do								\
  if (yychar == YYEMPTY && yylen == 1)				\
    {								\
      yychar = (Token);						\
      yylval = (Value);						\
      yytoken = YYTRANSLATE (yychar);				\
      YYPOPSTACK (1);						\
      goto yybackup;						\
    }								\
  else								\
    {								\
      yyerror (YY_("syntax error: cannot back up")); \
      YYERROR;							\
    }								\
while (YYID (0))


#define YYTERROR	1
#define YYERRCODE	256


/* YYLLOC_DEFAULT -- Set CURRENT to span from RHS[1] to RHS[N].
   If N is 0, then set CURRENT to the empty location which ends
   the previous symbol: RHS[0] (always defined).  */

#define YYRHSLOC(Rhs, K) ((Rhs)[K])
#ifndef YYLLOC_DEFAULT
# define YYLLOC_DEFAULT(Current, Rhs, N)				\
    do									\
      if (YYID (N))                                                    \
	{								\
	  (Current).first_line   = YYRHSLOC (Rhs, 1).first_line;	\
	  (Current).first_column = YYRHSLOC (Rhs, 1).first_column;	\
	  (Current).last_line    = YYRHSLOC (Rhs, N).last_line;		\
	  (Current).last_column  = YYRHSLOC (Rhs, N).last_column;	\
	}								\
      else								\
	{								\
	  (Current).first_line   = (Current).last_line   =		\
	    YYRHSLOC (Rhs, 0).last_line;				\
	  (Current).first_column = (Current).last_column =		\
	    YYRHSLOC (Rhs, 0).last_column;				\
	}								\
    while (YYID (0))
#endif


/* YY_LOCATION_PRINT -- Print the location on the stream.
   This macro was not mandated originally: define only if we know
   we won't break user code: when these are the locations we know.  */

#ifndef YY_LOCATION_PRINT
# if defined YYLTYPE_IS_TRIVIAL && YYLTYPE_IS_TRIVIAL
#  define YY_LOCATION_PRINT(File, Loc)			\
     fprintf (File, "%d.%d-%d.%d",			\
	      (Loc).first_line, (Loc).first_column,	\
	      (Loc).last_line,  (Loc).last_column)
# else
#  define YY_LOCATION_PRINT(File, Loc) ((void) 0)
# endif
#endif


/* YYLEX -- calling `yylex' with the right arguments.  */

#ifdef YYLEX_PARAM
# define YYLEX yylex (YYLEX_PARAM)
#else
# define YYLEX yylex ()
#endif

/* Enable debugging if requested.  */
#if YYDEBUG

# ifndef YYFPRINTF
#  include <stdio.h> /* INFRINGES ON USER NAME SPACE */
#  define YYFPRINTF fprintf
# endif

# define YYDPRINTF(Args)			\
do {						\
  if (yydebug)					\
    YYFPRINTF Args;				\
} while (YYID (0))

# define YY_SYMBOL_PRINT(Title, Type, Value, Location)			  \
do {									  \
  if (yydebug)								  \
    {									  \
      YYFPRINTF (stderr, "%s ", Title);					  \
      yy_symbol_print (stderr,						  \
		  Type, Value, Location); \
      YYFPRINTF (stderr, "\n");						  \
    }									  \
} while (YYID (0))


/*--------------------------------.
| Print this symbol on YYOUTPUT.  |
`--------------------------------*/

/*ARGSUSED*/
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yy_symbol_value_print (FILE *yyoutput, int yytype, YYSTYPE const * const yyvaluep, YYLTYPE const * const yylocationp)
#else
static void
yy_symbol_value_print (yyoutput, yytype, yyvaluep, yylocationp)
    FILE *yyoutput;
    int yytype;
    YYSTYPE const * const yyvaluep;
    YYLTYPE const * const yylocationp;
#endif
{
  if (!yyvaluep)
    return;
  YYUSE (yylocationp);
# ifdef YYPRINT
  if (yytype < YYNTOKENS)
    YYPRINT (yyoutput, yytoknum[yytype], *yyvaluep);
# else
  YYUSE (yyoutput);
# endif
  switch (yytype)
    {
      default:
	break;
    }
}


/*--------------------------------.
| Print this symbol on YYOUTPUT.  |
`--------------------------------*/

#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yy_symbol_print (FILE *yyoutput, int yytype, YYSTYPE const * const yyvaluep, YYLTYPE const * const yylocationp)
#else
static void
yy_symbol_print (yyoutput, yytype, yyvaluep, yylocationp)
    FILE *yyoutput;
    int yytype;
    YYSTYPE const * const yyvaluep;
    YYLTYPE const * const yylocationp;
#endif
{
  if (yytype < YYNTOKENS)
    YYFPRINTF (yyoutput, "token %s (", yytname[yytype]);
  else
    YYFPRINTF (yyoutput, "nterm %s (", yytname[yytype]);

  YY_LOCATION_PRINT (yyoutput, *yylocationp);
  YYFPRINTF (yyoutput, ": ");
  yy_symbol_value_print (yyoutput, yytype, yyvaluep, yylocationp);
  YYFPRINTF (yyoutput, ")");
}

/*------------------------------------------------------------------.
| yy_stack_print -- Print the state stack from its BOTTOM up to its |
| TOP (included).                                                   |
`------------------------------------------------------------------*/

#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yy_stack_print (yytype_int16 *bottom, yytype_int16 *top)
#else
static void
yy_stack_print (bottom, top)
    yytype_int16 *bottom;
    yytype_int16 *top;
#endif
{
  YYFPRINTF (stderr, "Stack now");
  for (; bottom <= top; ++bottom)
    YYFPRINTF (stderr, " %d", *bottom);
  YYFPRINTF (stderr, "\n");
}

# define YY_STACK_PRINT(Bottom, Top)				\
do {								\
  if (yydebug)							\
    yy_stack_print ((Bottom), (Top));				\
} while (YYID (0))


/*------------------------------------------------.
| Report that the YYRULE is going to be reduced.  |
`------------------------------------------------*/

#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yy_reduce_print (YYSTYPE *yyvsp, YYLTYPE *yylsp, int yyrule)
#else
static void
yy_reduce_print (yyvsp, yylsp, yyrule)
    YYSTYPE *yyvsp;
    YYLTYPE *yylsp;
    int yyrule;
#endif
{
  int yynrhs = yyr2[yyrule];
  int yyi;
  unsigned long int yylno = yyrline[yyrule];
  YYFPRINTF (stderr, "Reducing stack by rule %d (line %lu):\n",
	     yyrule - 1, yylno);
  /* The symbols being reduced.  */
  for (yyi = 0; yyi < yynrhs; yyi++)
    {
      fprintf (stderr, "   $%d = ", yyi + 1);
      yy_symbol_print (stderr, yyrhs[yyprhs[yyrule] + yyi],
		       &(yyvsp[(yyi + 1) - (yynrhs)])
		       , &(yylsp[(yyi + 1) - (yynrhs)])		       );
      fprintf (stderr, "\n");
    }
}

# define YY_REDUCE_PRINT(Rule)		\
do {					\
  if (yydebug)				\
    yy_reduce_print (yyvsp, yylsp, Rule); \
} while (YYID (0))

/* Nonzero means print parse trace.  It is left uninitialized so that
   multiple parsers can coexist.  */
int yydebug;
#else /* !YYDEBUG */
# define YYDPRINTF(Args)
# define YY_SYMBOL_PRINT(Title, Type, Value, Location)
# define YY_STACK_PRINT(Bottom, Top)
# define YY_REDUCE_PRINT(Rule)
#endif /* !YYDEBUG */


/* YYINITDEPTH -- initial size of the parser's stacks.  */
#ifndef	YYINITDEPTH
# define YYINITDEPTH 200
#endif

/* YYMAXDEPTH -- maximum size the stacks can grow to (effective only
   if the built-in stack extension method is used).

   Do not make this value too large; the results are undefined if
   YYSTACK_ALLOC_MAXIMUM < YYSTACK_BYTES (YYMAXDEPTH)
   evaluated with infinite-precision integer arithmetic.  */

#ifndef YYMAXDEPTH
# define YYMAXDEPTH 10000
#endif



#if YYERROR_VERBOSE

# ifndef yystrlen
#  if defined __GLIBC__ && defined _STRING_H
#   define yystrlen strlen
#  else
/* Return the length of YYSTR.  */
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static YYSIZE_T
yystrlen (const char *yystr)
#else
static YYSIZE_T
yystrlen (yystr)
    const char *yystr;
#endif
{
  YYSIZE_T yylen;
  for (yylen = 0; yystr[yylen]; yylen++)
    continue;
  return yylen;
}
#  endif
# endif

# ifndef yystpcpy
#  if defined __GLIBC__ && defined _STRING_H && defined _GNU_SOURCE
#   define yystpcpy stpcpy
#  else
/* Copy YYSRC to YYDEST, returning the address of the terminating '\0' in
   YYDEST.  */
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static char *
yystpcpy (char *yydest, const char *yysrc)
#else
static char *
yystpcpy (yydest, yysrc)
    char *yydest;
    const char *yysrc;
#endif
{
  char *yyd = yydest;
  const char *yys = yysrc;

  while ((*yyd++ = *yys++) != '\0')
    continue;

  return yyd - 1;
}
#  endif
# endif

# ifndef yytnamerr
/* Copy to YYRES the contents of YYSTR after stripping away unnecessary
   quotes and backslashes, so that it's suitable for yyerror.  The
   heuristic is that double-quoting is unnecessary unless the string
   contains an apostrophe, a comma, or backslash (other than
   backslash-backslash).  YYSTR is taken from yytname.  If YYRES is
   null, do not copy; instead, return the length of what the result
   would have been.  */
static YYSIZE_T
yytnamerr (char *yyres, const char *yystr)
{
  if (*yystr == '"')
    {
      YYSIZE_T yyn = 0;
      char const *yyp = yystr;

      for (;;)
	switch (*++yyp)
	  {
	  case '\'':
	  case ',':
	    goto do_not_strip_quotes;

	  case '\\':
	    if (*++yyp != '\\')
	      goto do_not_strip_quotes;
	    /* Fall through.  */
	  default:
	    if (yyres)
	      yyres[yyn] = *yyp;
	    yyn++;
	    break;

	  case '"':
	    if (yyres)
	      yyres[yyn] = '\0';
	    return yyn;
	  }
    do_not_strip_quotes: ;
    }

  if (! yyres)
    return yystrlen (yystr);

  return yystpcpy (yyres, yystr) - yyres;
}
# endif

/* Copy into YYRESULT an error message about the unexpected token
   YYCHAR while in state YYSTATE.  Return the number of bytes copied,
   including the terminating null byte.  If YYRESULT is null, do not
   copy anything; just return the number of bytes that would be
   copied.  As a special case, return 0 if an ordinary "syntax error"
   message will do.  Return YYSIZE_MAXIMUM if overflow occurs during
   size calculation.  */
static YYSIZE_T
yysyntax_error (char *yyresult, int yystate, int yychar)
{
  int yyn = yypact[yystate];

  if (! (YYPACT_NINF < yyn && yyn <= YYLAST))
    return 0;
  else
    {
      int yytype = YYTRANSLATE (yychar);
      YYSIZE_T yysize0 = yytnamerr (0, yytname[yytype]);
      YYSIZE_T yysize = yysize0;
      YYSIZE_T yysize1;
      int yysize_overflow = 0;
      enum { YYERROR_VERBOSE_ARGS_MAXIMUM = 5 };
      char const *yyarg[YYERROR_VERBOSE_ARGS_MAXIMUM];
      int yyx;

# if 0
      /* This is so xgettext sees the translatable formats that are
	 constructed on the fly.  */
      YY_("syntax error, unexpected %s");
      YY_("syntax error, unexpected %s, expecting %s");
      YY_("syntax error, unexpected %s, expecting %s or %s");
      YY_("syntax error, unexpected %s, expecting %s or %s or %s");
      YY_("syntax error, unexpected %s, expecting %s or %s or %s or %s");
# endif
      char *yyfmt;
      char const *yyf;
      static char const yyunexpected[] = "syntax error, unexpected %s";
      static char const yyexpecting[] = ", expecting %s";
      static char const yyor[] = " or %s";
      char yyformat[sizeof yyunexpected
		    + sizeof yyexpecting - 1
		    + ((YYERROR_VERBOSE_ARGS_MAXIMUM - 2)
		       * (sizeof yyor - 1))];
      char const *yyprefix = yyexpecting;

      /* Start YYX at -YYN if negative to avoid negative indexes in
	 YYCHECK.  */
      int yyxbegin = yyn < 0 ? -yyn : 0;

      /* Stay within bounds of both yycheck and yytname.  */
      int yychecklim = YYLAST - yyn + 1;
      int yyxend = yychecklim < YYNTOKENS ? yychecklim : YYNTOKENS;
      int yycount = 1;

      yyarg[0] = yytname[yytype];
      yyfmt = yystpcpy (yyformat, yyunexpected);

      for (yyx = yyxbegin; yyx < yyxend; ++yyx)
	if (yycheck[yyx + yyn] == yyx && yyx != YYTERROR)
	  {
	    if (yycount == YYERROR_VERBOSE_ARGS_MAXIMUM)
	      {
		yycount = 1;
		yysize = yysize0;
		yyformat[sizeof yyunexpected - 1] = '\0';
		break;
	      }
	    yyarg[yycount++] = yytname[yyx];
	    yysize1 = yysize + yytnamerr (0, yytname[yyx]);
	    yysize_overflow |= (yysize1 < yysize);
	    yysize = yysize1;
	    yyfmt = yystpcpy (yyfmt, yyprefix);
	    yyprefix = yyor;
	  }

      yyf = YY_(yyformat);
      yysize1 = yysize + yystrlen (yyf);
      yysize_overflow |= (yysize1 < yysize);
      yysize = yysize1;

      if (yysize_overflow)
	return YYSIZE_MAXIMUM;

      if (yyresult)
	{
	  /* Avoid sprintf, as that infringes on the user's name space.
	     Don't have undefined behavior even if the translation
	     produced a string with the wrong number of "%s"s.  */
	  char *yyp = yyresult;
	  int yyi = 0;
	  while ((*yyp = *yyf) != '\0')
	    {
	      if (*yyp == '%' && yyf[1] == 's' && yyi < yycount)
		{
		  yyp += yytnamerr (yyp, yyarg[yyi++]);
		  yyf += 2;
		}
	      else
		{
		  yyp++;
		  yyf++;
		}
	    }
	}
      return yysize;
    }
}
#endif /* YYERROR_VERBOSE */


/*-----------------------------------------------.
| Release the memory associated to this symbol.  |
`-----------------------------------------------*/

/*ARGSUSED*/
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yydestruct (const char *yymsg, int yytype, YYSTYPE *yyvaluep, YYLTYPE *yylocationp)
#else
static void
yydestruct (yymsg, yytype, yyvaluep, yylocationp)
    const char *yymsg;
    int yytype;
    YYSTYPE *yyvaluep;
    YYLTYPE *yylocationp;
#endif
{
  YYUSE (yyvaluep);
  YYUSE (yylocationp);

  if (!yymsg)
    yymsg = "Deleting";
  YY_SYMBOL_PRINT (yymsg, yytype, yyvaluep, yylocationp);

  switch (yytype)
    {
      case 99: /* "LIT_STR" */
#line 97 "compiler/parser/tbol.y"
	{ free((yyvaluep->sval)); };
#line 1876 "compiler/parser/tbol.tab.c"
	break;
      case 100: /* "LIT_NUM" */
#line 97 "compiler/parser/tbol.y"
	{ free((yyvaluep->sval)); };
#line 1881 "compiler/parser/tbol.tab.c"
	break;
      case 101: /* "LIT_HEX" */
#line 97 "compiler/parser/tbol.y"
	{ free((yyvaluep->sval)); };
#line 1886 "compiler/parser/tbol.tab.c"
	break;
      case 102: /* "IDENT" */
#line 98 "compiler/parser/tbol.y"
	{ free((yyvaluep->ident_pair).canonical); free((yyvaluep->ident_pair).original); };
#line 1891 "compiler/parser/tbol.tab.c"
	break;
      case 110: /* "definitions" */
#line 99 "compiler/parser/tbol.y"
	{ ast_free((yyvaluep->node)); };
#line 1896 "compiler/parser/tbol.tab.c"
	break;
      case 111: /* "definition" */
#line 99 "compiler/parser/tbol.y"
	{ ast_free((yyvaluep->node)); };
#line 1901 "compiler/parser/tbol.tab.c"
	break;
      case 112: /* "data_group" */
#line 99 "compiler/parser/tbol.y"
	{ ast_free((yyvaluep->node)); };
#line 1906 "compiler/parser/tbol.tab.c"
	break;
      case 113: /* "data_section" */
#line 99 "compiler/parser/tbol.y"
	{ ast_free((yyvaluep->node)); };
#line 1911 "compiler/parser/tbol.tab.c"
	break;
      case 114: /* "var_decl_list" */
#line 99 "compiler/parser/tbol.y"
	{ ast_free((yyvaluep->node)); };
#line 1916 "compiler/parser/tbol.tab.c"
	break;
      case 115: /* "var_decl" */
#line 99 "compiler/parser/tbol.y"
	{ ast_free((yyvaluep->node)); };
#line 1921 "compiler/parser/tbol.tab.c"
	break;
      case 116: /* "define_stmt" */
#line 99 "compiler/parser/tbol.y"
	{ ast_free((yyvaluep->node)); };
#line 1926 "compiler/parser/tbol.tab.c"
	break;
      case 117: /* "proc_list" */
#line 99 "compiler/parser/tbol.y"
	{ ast_free((yyvaluep->node)); };
#line 1931 "compiler/parser/tbol.tab.c"
	break;
      case 118: /* "proc" */
#line 99 "compiler/parser/tbol.y"
	{ ast_free((yyvaluep->node)); };
#line 1936 "compiler/parser/tbol.tab.c"
	break;
      case 119: /* "stmt_list" */
#line 99 "compiler/parser/tbol.y"
	{ ast_free((yyvaluep->node)); };
#line 1941 "compiler/parser/tbol.tab.c"
	break;
      case 120: /* "label_prefix" */
#line 99 "compiler/parser/tbol.y"
	{ ast_free((yyvaluep->node)); };
#line 1946 "compiler/parser/tbol.tab.c"
	break;
      case 121: /* "label" */
#line 99 "compiler/parser/tbol.y"
	{ ast_free((yyvaluep->node)); };
#line 1951 "compiler/parser/tbol.tab.c"
	break;
      case 122: /* "label_name" */
#line 97 "compiler/parser/tbol.y"
	{ free((yyvaluep->sval)); };
#line 1956 "compiler/parser/tbol.tab.c"
	break;
      case 123: /* "statement" */
#line 99 "compiler/parser/tbol.y"
	{ ast_free((yyvaluep->node)); };
#line 1961 "compiler/parser/tbol.tab.c"
	break;
      case 124: /* "define_in_proc" */
#line 99 "compiler/parser/tbol.y"
	{ ast_free((yyvaluep->node)); };
#line 1966 "compiler/parser/tbol.tab.c"
	break;
      case 125: /* "simple_stmt" */
#line 99 "compiler/parser/tbol.y"
	{ ast_free((yyvaluep->node)); };
#line 1971 "compiler/parser/tbol.tab.c"
	break;
      case 126: /* "then_body" */
#line 99 "compiler/parser/tbol.y"
	{ ast_free((yyvaluep->node)); };
#line 1976 "compiler/parser/tbol.tab.c"
	break;
      case 127: /* "if_stmt" */
#line 99 "compiler/parser/tbol.y"
	{ ast_free((yyvaluep->node)); };
#line 1981 "compiler/parser/tbol.tab.c"
	break;
      case 128: /* "while_stmt" */
#line 99 "compiler/parser/tbol.y"
	{ ast_free((yyvaluep->node)); };
#line 1986 "compiler/parser/tbol.tab.c"
	break;
      case 129: /* "do_block" */
#line 99 "compiler/parser/tbol.y"
	{ ast_free((yyvaluep->node)); };
#line 1991 "compiler/parser/tbol.tab.c"
	break;
      case 130: /* "condition" */
#line 99 "compiler/parser/tbol.y"
	{ ast_free((yyvaluep->node)); };
#line 1996 "compiler/parser/tbol.tab.c"
	break;
      case 131: /* "condition_term" */
#line 99 "compiler/parser/tbol.y"
	{ ast_free((yyvaluep->node)); };
#line 2001 "compiler/parser/tbol.tab.c"
	break;
      case 132: /* "condition_factor" */
#line 99 "compiler/parser/tbol.y"
	{ ast_free((yyvaluep->node)); };
#line 2006 "compiler/parser/tbol.tab.c"
	break;
      case 134: /* "verb_stmt" */
#line 99 "compiler/parser/tbol.y"
	{ ast_free((yyvaluep->node)); };
#line 2011 "compiler/parser/tbol.tab.c"
	break;
      case 135: /* "label_list" */
#line 99 "compiler/parser/tbol.y"
	{ ast_free((yyvaluep->node)); };
#line 2016 "compiler/parser/tbol.tab.c"
	break;
      case 136: /* "expr_list" */
#line 99 "compiler/parser/tbol.y"
	{ ast_free((yyvaluep->node)); };
#line 2021 "compiler/parser/tbol.tab.c"
	break;
      case 137: /* "format_spec_list" */
#line 99 "compiler/parser/tbol.y"
	{ ast_free((yyvaluep->node)); };
#line 2026 "compiler/parser/tbol.tab.c"
	break;
      case 138: /* "format_spec" */
#line 99 "compiler/parser/tbol.y"
	{ ast_free((yyvaluep->node)); };
#line 2031 "compiler/parser/tbol.tab.c"
	break;
      case 139: /* "proc_call" */
#line 99 "compiler/parser/tbol.y"
	{ ast_free((yyvaluep->node)); };
#line 2036 "compiler/parser/tbol.tab.c"
	break;
      case 140: /* "indexable" */
#line 99 "compiler/parser/tbol.y"
	{ ast_free((yyvaluep->node)); };
#line 2041 "compiler/parser/tbol.tab.c"
	break;
      case 141: /* "expr" */
#line 99 "compiler/parser/tbol.y"
	{ ast_free((yyvaluep->node)); };
#line 2046 "compiler/parser/tbol.tab.c"
	break;

      default:
	break;
    }
}


/* Prevent warnings from -Wmissing-prototypes.  */

#ifdef YYPARSE_PARAM
#if defined __STDC__ || defined __cplusplus
int yyparse (void *YYPARSE_PARAM);
#else
int yyparse ();
#endif
#else /* ! YYPARSE_PARAM */
#if defined __STDC__ || defined __cplusplus
int yyparse (void);
#else
int yyparse ();
#endif
#endif /* ! YYPARSE_PARAM */



/* The look-ahead symbol.  */
int yychar;

/* The semantic value of the look-ahead symbol.  */
YYSTYPE yylval;

/* Number of syntax errors so far.  */
int yynerrs;
/* Location data for the look-ahead symbol.  */
YYLTYPE yylloc;



/*----------.
| yyparse.  |
`----------*/

#ifdef YYPARSE_PARAM
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
int
yyparse (void *YYPARSE_PARAM)
#else
int
yyparse (YYPARSE_PARAM)
    void *YYPARSE_PARAM;
#endif
#else /* ! YYPARSE_PARAM */
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
int
yyparse (void)
#else
int
yyparse ()

#endif
#endif
{
  
  int yystate;
  int yyn;
  int yyresult;
  /* Number of tokens to shift before error messages enabled.  */
  int yyerrstatus;
  /* Look-ahead token as an internal (translated) token number.  */
  int yytoken = 0;
#if YYERROR_VERBOSE
  /* Buffer for error messages, and its allocated size.  */
  char yymsgbuf[128];
  char *yymsg = yymsgbuf;
  YYSIZE_T yymsg_alloc = sizeof yymsgbuf;
#endif

  /* Three stacks and their tools:
     `yyss': related to states,
     `yyvs': related to semantic values,
     `yyls': related to locations.

     Refer to the stacks thru separate pointers, to allow yyoverflow
     to reallocate them elsewhere.  */

  /* The state stack.  */
  yytype_int16 yyssa[YYINITDEPTH];
  yytype_int16 *yyss = yyssa;
  yytype_int16 *yyssp;

  /* The semantic value stack.  */
  YYSTYPE yyvsa[YYINITDEPTH];
  YYSTYPE *yyvs = yyvsa;
  YYSTYPE *yyvsp;

  /* The location stack.  */
  YYLTYPE yylsa[YYINITDEPTH];
  YYLTYPE *yyls = yylsa;
  YYLTYPE *yylsp;
  /* The locations where the error started and ended.  */
  YYLTYPE yyerror_range[2];

#define YYPOPSTACK(N)   (yyvsp -= (N), yyssp -= (N), yylsp -= (N))

  YYSIZE_T yystacksize = YYINITDEPTH;

  /* The variables used to return semantic value and location from the
     action routines.  */
  YYSTYPE yyval;
  YYLTYPE yyloc;

  /* The number of symbols on the RHS of the reduced rule.
     Keep to zero when no symbol should be popped.  */
  int yylen = 0;

  YYDPRINTF ((stderr, "Starting parse\n"));

  yystate = 0;
  yyerrstatus = 0;
  yynerrs = 0;
  yychar = YYEMPTY;		/* Cause a token to be read.  */

  /* Initialize stack pointers.
     Waste one element of value and location stack
     so that they stay on the same level as the state stack.
     The wasted elements are never initialized.  */

  yyssp = yyss;
  yyvsp = yyvs;
  yylsp = yyls;
#if defined YYLTYPE_IS_TRIVIAL && YYLTYPE_IS_TRIVIAL
  /* Initialize the default location before parsing starts.  */
  yylloc.first_line   = yylloc.last_line   = 1;
  yylloc.first_column = yylloc.last_column = 0;
#endif

  goto yysetstate;

/*------------------------------------------------------------.
| yynewstate -- Push a new state, which is found in yystate.  |
`------------------------------------------------------------*/
 yynewstate:
  /* In all cases, when you get here, the value and location stacks
     have just been pushed.  So pushing a state here evens the stacks.  */
  yyssp++;

 yysetstate:
  *yyssp = yystate;

  if (yyss + yystacksize - 1 <= yyssp)
    {
      /* Get the current used size of the three stacks, in elements.  */
      YYSIZE_T yysize = yyssp - yyss + 1;

#ifdef yyoverflow
      {
	/* Give user a chance to reallocate the stack.  Use copies of
	   these so that the &'s don't force the real ones into
	   memory.  */
	YYSTYPE *yyvs1 = yyvs;
	yytype_int16 *yyss1 = yyss;
	YYLTYPE *yyls1 = yyls;

	/* Each stack pointer address is followed by the size of the
	   data in use in that stack, in bytes.  This used to be a
	   conditional around just the two extra args, but that might
	   be undefined if yyoverflow is a macro.  */
	yyoverflow (YY_("memory exhausted"),
		    &yyss1, yysize * sizeof (*yyssp),
		    &yyvs1, yysize * sizeof (*yyvsp),
		    &yyls1, yysize * sizeof (*yylsp),
		    &yystacksize);
	yyls = yyls1;
	yyss = yyss1;
	yyvs = yyvs1;
      }
#else /* no yyoverflow */
# ifndef YYSTACK_RELOCATE
      goto yyexhaustedlab;
# else
      /* Extend the stack our own way.  */
      if (YYMAXDEPTH <= yystacksize)
	goto yyexhaustedlab;
      yystacksize *= 2;
      if (YYMAXDEPTH < yystacksize)
	yystacksize = YYMAXDEPTH;

      {
	yytype_int16 *yyss1 = yyss;
	union yyalloc *yyptr =
	  (union yyalloc *) YYSTACK_ALLOC (YYSTACK_BYTES (yystacksize));
	if (! yyptr)
	  goto yyexhaustedlab;
	YYSTACK_RELOCATE (yyss);
	YYSTACK_RELOCATE (yyvs);
	YYSTACK_RELOCATE (yyls);
#  undef YYSTACK_RELOCATE
	if (yyss1 != yyssa)
	  YYSTACK_FREE (yyss1);
      }
# endif
#endif /* no yyoverflow */

      yyssp = yyss + yysize - 1;
      yyvsp = yyvs + yysize - 1;
      yylsp = yyls + yysize - 1;

      YYDPRINTF ((stderr, "Stack size increased to %lu\n",
		  (unsigned long int) yystacksize));

      if (yyss + yystacksize - 1 <= yyssp)
	YYABORT;
    }

  YYDPRINTF ((stderr, "Entering state %d\n", yystate));

  goto yybackup;

/*-----------.
| yybackup.  |
`-----------*/
yybackup:

  /* Do appropriate processing given the current state.  Read a
     look-ahead token if we need one and don't already have one.  */

  /* First try to decide what to do without reference to look-ahead token.  */
  yyn = yypact[yystate];
  if (yyn == YYPACT_NINF)
    goto yydefault;

  /* Not known => get a look-ahead token if don't already have one.  */

  /* YYCHAR is either YYEMPTY or YYEOF or a valid look-ahead symbol.  */
  if (yychar == YYEMPTY)
    {
      YYDPRINTF ((stderr, "Reading a token: "));
      yychar = YYLEX;
    }

  if (yychar <= YYEOF)
    {
      yychar = yytoken = YYEOF;
      YYDPRINTF ((stderr, "Now at end of input.\n"));
    }
  else
    {
      yytoken = YYTRANSLATE (yychar);
      YY_SYMBOL_PRINT ("Next token is", yytoken, &yylval, &yylloc);
    }

  /* If the proper action on seeing token YYTOKEN is to reduce or to
     detect an error, take that action.  */
  yyn += yytoken;
  if (yyn < 0 || YYLAST < yyn || yycheck[yyn] != yytoken)
    goto yydefault;
  yyn = yytable[yyn];
  if (yyn <= 0)
    {
      if (yyn == 0 || yyn == YYTABLE_NINF)
	goto yyerrlab;
      yyn = -yyn;
      goto yyreduce;
    }

  if (yyn == YYFINAL)
    YYACCEPT;

  /* Count tokens shifted since error; after three, turn off error
     status.  */
  if (yyerrstatus)
    yyerrstatus--;

  /* Shift the look-ahead token.  */
  YY_SYMBOL_PRINT ("Shifting", yytoken, &yylval, &yylloc);

  /* Discard the shifted token unless it is eof.  */
  if (yychar != YYEOF)
    yychar = YYEMPTY;

  yystate = yyn;
  *++yyvsp = yylval;
  *++yylsp = yylloc;
  goto yynewstate;


/*-----------------------------------------------------------.
| yydefault -- do the default action for the current state.  |
`-----------------------------------------------------------*/
yydefault:
  yyn = yydefact[yystate];
  if (yyn == 0)
    goto yyerrlab;
  goto yyreduce;


/*-----------------------------.
| yyreduce -- Do a reduction.  |
`-----------------------------*/
yyreduce:
  /* yyn is the number of a rule to reduce with.  */
  yylen = yyr2[yyn];

  /* If YYLEN is nonzero, implement the default value of the action:
     `$$ = $1'.

     Otherwise, the following line sets YYVAL to garbage.
     This behavior is undocumented and Bison
     users should not rely upon it.  Assigning to YYVAL
     unconditionally makes the parser a bit smaller, and it avoids a
     GCC warning that YYVAL may be used uninitialized.  */
  yyval = yyvsp[1-yylen];

  /* Default location.  */
  YYLLOC_DEFAULT (yyloc, (yylsp - yylen), yylen);
  YY_REDUCE_PRINT (yyn);
  switch (yyn)
    {
        case 2:
#line 176 "compiler/parser/tbol.y"
    {
        (yyval.node) = ast_program((yyvsp[(2) - (5)].ident_pair).canonical, MAKE_LOC((yylsp[(1) - (5)])));
        free((yyvsp[(2) - (5)].ident_pair).canonical); free((yyvsp[(2) - (5)].ident_pair).original);
        /* Add definitions as children */
        if ((yyvsp[(4) - (5)].node)) {
            for (int i = 0; i < (yyvsp[(4) - (5)].node)->child_count; i++) {
                ast_add_child((yyval.node), (yyvsp[(4) - (5)].node)->children[i]);
            }
            (yyvsp[(4) - (5)].node)->child_count = 0;  /* Prevent double-free */
            ast_free((yyvsp[(4) - (5)].node)); (yyvsp[(4) - (5)].node) = NULL;
        }
        /* Add procedures as children */
        if ((yyvsp[(5) - (5)].node)) {
            for (int i = 0; i < (yyvsp[(5) - (5)].node)->child_count; i++) {
                ast_add_child((yyval.node), (yyvsp[(5) - (5)].node)->children[i]);
            }
            (yyvsp[(5) - (5)].node)->child_count = 0;
            ast_free((yyvsp[(5) - (5)].node)); (yyvsp[(5) - (5)].node) = NULL;
        }
        ast_set_end((yyval.node), MAKE_LOC((yylsp[(5) - (5)])));
        ast_root = (yyval.node);
    ;}
    break;

  case 3:
#line 198 "compiler/parser/tbol.y"
    {
        /* Error in program name - recover and continue */
        yyerrok;
        (yyval.node) = ast_program("<error>", MAKE_LOC((yylsp[(1) - (5)])));
        if ((yyvsp[(4) - (5)].node)) {
            for (int i = 0; i < (yyvsp[(4) - (5)].node)->child_count; i++) {
                ast_add_child((yyval.node), (yyvsp[(4) - (5)].node)->children[i]);
            }
            (yyvsp[(4) - (5)].node)->child_count = 0;
            ast_free((yyvsp[(4) - (5)].node)); (yyvsp[(4) - (5)].node) = NULL;
        }
        if ((yyvsp[(5) - (5)].node)) {
            for (int i = 0; i < (yyvsp[(5) - (5)].node)->child_count; i++) {
                ast_add_child((yyval.node), (yyvsp[(5) - (5)].node)->children[i]);
            }
            (yyvsp[(5) - (5)].node)->child_count = 0;
            ast_free((yyvsp[(5) - (5)].node)); (yyvsp[(5) - (5)].node) = NULL;
        }
        ast_root = (yyval.node);
    ;}
    break;

  case 4:
#line 218 "compiler/parser/tbol.y"
    {
        /* Complete parse failure - no PROGRAM statement found.
         * YYABORT immediately to avoid infinite error recovery loop. */
        (yyval.node) = NULL;
        ast_root = NULL;
        YYABORT;
    ;}
    break;

  case 5:
#line 228 "compiler/parser/tbol.y"
    { (yyval.node) = NULL; ;}
    break;

  case 6:
#line 229 "compiler/parser/tbol.y"
    {
        if (!(yyvsp[(1) - (2)].node)) {
            (yyval.node) = ast_new(AST_PROGRAM, MAKE_LOC((yylsp[(2) - (2)])));  /* Temporary container */
        } else {
            (yyval.node) = (yyvsp[(1) - (2)].node); (yyvsp[(1) - (2)].node) = NULL;
        }
        if ((yyvsp[(2) - (2)].node)) { ast_add_child((yyval.node), (yyvsp[(2) - (2)].node)); (yyvsp[(2) - (2)].node) = NULL; }
    ;}
    break;

  case 7:
#line 240 "compiler/parser/tbol.y"
    { (yyval.node) = (yyvsp[(1) - (1)].node); (yyvsp[(1) - (1)].node) = NULL; ;}
    break;

  case 8:
#line 241 "compiler/parser/tbol.y"
    { (yyval.node) = (yyvsp[(1) - (1)].node); (yyvsp[(1) - (1)].node) = NULL; ;}
    break;

  case 9:
#line 242 "compiler/parser/tbol.y"
    { (yyval.node) = (yyvsp[(1) - (1)].node); (yyvsp[(1) - (1)].node) = NULL; ;}
    break;

  case 10:
#line 243 "compiler/parser/tbol.y"
    {
        yyerrok;
        (yyval.node) = ast_error(MAKE_LOC((yylsp[(1) - (2)])));
    ;}
    break;

  case 11:
#line 251 "compiler/parser/tbol.y"
    {
        (yyval.node) = ast_data_section((yyvsp[(1) - (4)].ident_pair).canonical, MAKE_LOC((yylsp[(1) - (4)])));
        free((yyvsp[(1) - (4)].ident_pair).canonical); free((yyvsp[(1) - (4)].ident_pair).original);
        if ((yyvsp[(3) - (4)].node)) {
            for (int i = 0; i < (yyvsp[(3) - (4)].node)->child_count; i++) {
                ast_add_child((yyval.node), (yyvsp[(3) - (4)].node)->children[i]);
            }
            (yyvsp[(3) - (4)].node)->child_count = 0;
            ast_free((yyvsp[(3) - (4)].node)); (yyvsp[(3) - (4)].node) = NULL;
        }
        ast_set_end((yyval.node), MAKE_LOC((yylsp[(4) - (4)])));
    ;}
    break;

  case 12:
#line 267 "compiler/parser/tbol.y"
    {
        (yyval.node) = ast_data_section((yyvsp[(2) - (5)].ident_pair).canonical, MAKE_LOC((yylsp[(1) - (5)])));
        free((yyvsp[(2) - (5)].ident_pair).canonical); free((yyvsp[(2) - (5)].ident_pair).original);
        /* Move children from var_decl_list */
        if ((yyvsp[(4) - (5)].node)) {
            for (int i = 0; i < (yyvsp[(4) - (5)].node)->child_count; i++) {
                ast_add_child((yyval.node), (yyvsp[(4) - (5)].node)->children[i]);
            }
            (yyvsp[(4) - (5)].node)->child_count = 0;
            ast_free((yyvsp[(4) - (5)].node)); (yyvsp[(4) - (5)].node) = NULL;
        }
        ast_set_end((yyval.node), MAKE_LOC((yylsp[(5) - (5)])));
    ;}
    break;

  case 13:
#line 280 "compiler/parser/tbol.y"
    {
        yyerrok;
        (yyval.node) = ast_error(MAKE_LOC((yylsp[(1) - (3)])));
    ;}
    break;

  case 14:
#line 287 "compiler/parser/tbol.y"
    {
        (yyval.node) = ast_new(AST_DATA_SECTION, MAKE_LOC((yylsp[(1) - (1)])));  /* Container */
        ast_add_child((yyval.node), (yyvsp[(1) - (1)].node)); (yyvsp[(1) - (1)].node) = NULL;
    ;}
    break;

  case 15:
#line 291 "compiler/parser/tbol.y"
    {
        (yyval.node) = (yyvsp[(1) - (3)].node); (yyvsp[(1) - (3)].node) = NULL;
        ast_add_child((yyval.node), (yyvsp[(3) - (3)].node)); (yyvsp[(3) - (3)].node) = NULL;
    ;}
    break;

  case 16:
#line 298 "compiler/parser/tbol.y"
    {
        (yyval.node) = ast_var_decl((yyvsp[(1) - (1)].ident_pair).canonical, 0, MAKE_LOC((yylsp[(1) - (1)])));
        if ((yyvsp[(1) - (1)].ident_pair).original) (yyval.node)->data.var_decl.original_text = (yyvsp[(1) - (1)].ident_pair).original;
        ast_set_end((yyval.node), MAKE_END_LOC((yylsp[(1) - (1)])));
        free((yyvsp[(1) - (1)].ident_pair).canonical);
    ;}
    break;

  case 17:
#line 304 "compiler/parser/tbol.y"
    {
        int asize = atoi((yyvsp[(3) - (4)].sval));
        if (asize <= 0) {
            yyerror("array size must be greater than zero");
            YYERROR;
        }
        (yyval.node) = ast_var_decl((yyvsp[(1) - (4)].ident_pair).canonical, asize, MAKE_LOC((yylsp[(1) - (4)])));
        if ((yyvsp[(1) - (4)].ident_pair).original) (yyval.node)->data.var_decl.original_text = (yyvsp[(1) - (4)].ident_pair).original;
        ast_set_end((yyval.node), MAKE_END_LOC((yylsp[(4) - (4)])));
        free((yyvsp[(1) - (4)].ident_pair).canonical);
        free((yyvsp[(3) - (4)].sval)); (yyvsp[(3) - (4)].sval) = NULL;
    ;}
    break;

  case 18:
#line 320 "compiler/parser/tbol.y"
    {
        /* Convert expr to string for DEFINE value */
        char *val = NULL;
        char *preproc_val = NULL;  /* Value for preprocessor (may differ) */
        switch ((yyvsp[(4) - (5)].node)->kind) {
            case AST_LITERAL_STR:
                val = strdup((yyvsp[(4) - (5)].node)->data.str_lit.value);
                /* Preprocessor needs quoted form for string detection */
                asprintf(&preproc_val, "'%s'", (yyvsp[(4) - (5)].node)->data.str_lit.value);
                break;
            case AST_LITERAL_NUM:
            case AST_LITERAL_HEX:
                val = strdup((yyvsp[(4) - (5)].node)->data.num_lit.text);
                preproc_val = strdup((yyvsp[(4) - (5)].node)->data.num_lit.text);
                break;
            case AST_PEV:
                asprintf(&val, "&%d", (yyvsp[(4) - (5)].node)->data.ext_var.number);
                preproc_val = strdup(val);
                break;
            case AST_GEV:
                asprintf(&val, "#%d", (yyvsp[(4) - (5)].node)->data.ext_var.number);
                preproc_val = strdup(val);
                break;
            case AST_IDENT:
                val = strdup((yyvsp[(4) - (5)].node)->data.ident.name);
                preproc_val = strdup(val);
                break;
            case AST_REG_I:
                asprintf(&val, "I%d", (yyvsp[(4) - (5)].node)->data.reg.number);
                preproc_val = strdup(val);
                break;
            case AST_REG_D:
                asprintf(&val, "D%d", (yyvsp[(4) - (5)].node)->data.reg.number);
                preproc_val = strdup(val);
                break;
            case AST_REG_P:
                asprintf(&val, "P%d", (yyvsp[(4) - (5)].node)->data.reg.number);
                preproc_val = strdup(val);
                break;
            default:
                val = strdup("?");
                preproc_val = strdup("?");
        }

        /* Register with preprocessor for substitution */
        preproc_add_define((yyvsp[(2) - (5)].ident_pair).canonical, preproc_val, (yylsp[(1) - (5)]).first_line, (yylsp[(1) - (5)]).first_column);
        free(preproc_val);

        (yyval.node) = ast_define((yyvsp[(2) - (5)].ident_pair).canonical, val, MAKE_LOC((yylsp[(1) - (5)])));
        if ((yyvsp[(2) - (5)].ident_pair).original) (yyval.node)->data.define.original_text = (yyvsp[(2) - (5)].ident_pair).original;
        ast_set_end((yyval.node), MAKE_LOC((yylsp[(5) - (5)])));
        free((yyvsp[(2) - (5)].ident_pair).canonical);
        free(val);
        ast_free((yyvsp[(4) - (5)].node)); (yyvsp[(4) - (5)].node) = NULL;
    ;}
    break;

  case 19:
#line 379 "compiler/parser/tbol.y"
    {
        (yyval.node) = ast_new(AST_PROGRAM, MAKE_LOC((yylsp[(1) - (1)])));  /* Container */
        ast_add_child((yyval.node), (yyvsp[(1) - (1)].node)); (yyvsp[(1) - (1)].node) = NULL;
    ;}
    break;

  case 20:
#line 383 "compiler/parser/tbol.y"
    {
        (yyval.node) = (yyvsp[(1) - (2)].node); (yyvsp[(1) - (2)].node) = NULL;
        ast_add_child((yyval.node), (yyvsp[(2) - (2)].node)); (yyvsp[(2) - (2)].node) = NULL;
    ;}
    break;

  case 21:
#line 387 "compiler/parser/tbol.y"
    {
        /* Skip garbage lines between procs */
        yyerrok;
        (yyval.node) = (yyvsp[(1) - (3)].node); (yyvsp[(1) - (3)].node) = NULL;
    ;}
    break;

  case 22:
#line 395 "compiler/parser/tbol.y"
    {
        (yyval.node) = ast_proc((yyvsp[(2) - (5)].ident_pair).canonical, MAKE_LOC((yylsp[(1) - (5)])));
        if ((yyvsp[(2) - (5)].ident_pair).original) (yyval.node)->data.proc.original_text = (yyvsp[(2) - (5)].ident_pair).original;
        free((yyvsp[(2) - (5)].ident_pair).canonical);
        if ((yyvsp[(4) - (5)].node)) {
            for (int i = 0; i < (yyvsp[(4) - (5)].node)->child_count; i++) {
                ast_add_child((yyval.node), (yyvsp[(4) - (5)].node)->children[i]);
            }
            (yyvsp[(4) - (5)].node)->child_count = 0;
            ast_free((yyvsp[(4) - (5)].node)); (yyvsp[(4) - (5)].node) = NULL;
        }
        ast_set_end((yyval.node), MAKE_LOC((yylsp[(5) - (5)])));
    ;}
    break;

  case 23:
#line 408 "compiler/parser/tbol.y"
    {
        yyerrok;
        (yyval.node) = ast_proc((yyvsp[(2) - (5)].ident_pair).canonical, MAKE_LOC((yylsp[(1) - (5)])));
        if ((yyvsp[(2) - (5)].ident_pair).original) (yyval.node)->data.proc.original_text = (yyvsp[(2) - (5)].ident_pair).original;
        free((yyvsp[(2) - (5)].ident_pair).canonical);
        ast_add_child((yyval.node), ast_error(MAKE_LOC((yylsp[(4) - (5)]))));
    ;}
    break;

  case 24:
#line 422 "compiler/parser/tbol.y"
    {
        (yyval.node) = (yyvsp[(1) - (2)].node) ? (yyvsp[(1) - (2)].node) : ast_new(AST_DO_BLOCK, MAKE_LOC((yylsp[(2) - (2)])));
        if ((yyvsp[(1) - (2)].node)) { (yyvsp[(1) - (2)].node) = NULL; }
        if ((yyvsp[(2) - (2)].node)) { ast_add_child((yyval.node), (yyvsp[(2) - (2)].node)); (yyvsp[(2) - (2)].node) = NULL; }
    ;}
    break;

  case 25:
#line 427 "compiler/parser/tbol.y"
    {
        (yyval.node) = (yyvsp[(1) - (2)].node); (yyvsp[(1) - (2)].node) = NULL;
        if ((yyvsp[(2) - (2)].node)) { ast_add_child((yyval.node), (yyvsp[(2) - (2)].node)); (yyvsp[(2) - (2)].node) = NULL; }
    ;}
    break;

  case 26:
#line 431 "compiler/parser/tbol.y"
    {
        (yyval.node) = (yyvsp[(1) - (2)].node); (yyvsp[(1) - (2)].node) = NULL;
        if ((yyvsp[(2) - (2)].node)) { ast_add_child((yyval.node), (yyvsp[(2) - (2)].node)); (yyvsp[(2) - (2)].node) = NULL; }
    ;}
    break;

  case 27:
#line 439 "compiler/parser/tbol.y"
    { (yyval.node) = NULL; ;}
    break;

  case 28:
#line 440 "compiler/parser/tbol.y"
    {
        if (!(yyvsp[(1) - (2)].node)) {
            (yyval.node) = ast_new(AST_DO_BLOCK, MAKE_LOC((yylsp[(2) - (2)])));
        } else {
            (yyval.node) = (yyvsp[(1) - (2)].node); (yyvsp[(1) - (2)].node) = NULL;
        }
        if ((yyvsp[(2) - (2)].node)) { ast_add_child((yyval.node), (yyvsp[(2) - (2)].node)); (yyvsp[(2) - (2)].node) = NULL; }
    ;}
    break;

  case 29:
#line 451 "compiler/parser/tbol.y"
    {
        (yyval.node) = ast_label((yyvsp[(1) - (2)].sval), MAKE_LOC((yylsp[(1) - (2)])));
        ast_set_end((yyval.node), MAKE_END_LOC((yylsp[(2) - (2)])));
        free((yyvsp[(1) - (2)].sval)); (yyvsp[(1) - (2)].sval) = NULL;
    ;}
    break;

  case 30:
#line 463 "compiler/parser/tbol.y"
    { (yyval.sval) = (yyvsp[(1) - (1)].ident_pair).canonical; free((yyvsp[(1) - (1)].ident_pair).original); ;}
    break;

  case 31:
#line 465 "compiler/parser/tbol.y"
    { (yyval.sval) = strdup("EXIT"); ;}
    break;

  case 32:
#line 466 "compiler/parser/tbol.y"
    { (yyval.sval) = strdup("RETURN"); ;}
    break;

  case 33:
#line 468 "compiler/parser/tbol.y"
    { (yyval.sval) = strdup("MOVE"); ;}
    break;

  case 34:
#line 469 "compiler/parser/tbol.y"
    { (yyval.sval) = strdup("ABS"); ;}
    break;

  case 35:
#line 470 "compiler/parser/tbol.y"
    { (yyval.sval) = strdup("SWAP"); ;}
    break;

  case 36:
#line 471 "compiler/parser/tbol.y"
    { (yyval.sval) = strdup("FILL"); ;}
    break;

  case 37:
#line 472 "compiler/parser/tbol.y"
    { (yyval.sval) = strdup("CLEAR"); ;}
    break;

  case 38:
#line 473 "compiler/parser/tbol.y"
    { (yyval.sval) = strdup("PUSH"); ;}
    break;

  case 39:
#line 474 "compiler/parser/tbol.y"
    { (yyval.sval) = strdup("POP"); ;}
    break;

  case 40:
#line 476 "compiler/parser/tbol.y"
    { (yyval.sval) = strdup("STRING"); ;}
    break;

  case 41:
#line 477 "compiler/parser/tbol.y"
    { (yyval.sval) = strdup("SUBSTR"); ;}
    break;

  case 42:
#line 478 "compiler/parser/tbol.y"
    { (yyval.sval) = strdup("INSTR"); ;}
    break;

  case 43:
#line 479 "compiler/parser/tbol.y"
    { (yyval.sval) = strdup("UPPERCASE"); ;}
    break;

  case 44:
#line 480 "compiler/parser/tbol.y"
    { (yyval.sval) = strdup("LENGTH"); ;}
    break;

  case 45:
#line 481 "compiler/parser/tbol.y"
    { (yyval.sval) = strdup("EDIT"); ;}
    break;

  case 46:
#line 483 "compiler/parser/tbol.y"
    { (yyval.sval) = strdup("ADD"); ;}
    break;

  case 47:
#line 484 "compiler/parser/tbol.y"
    { (yyval.sval) = strdup("SUBTRACT"); ;}
    break;

  case 48:
#line 485 "compiler/parser/tbol.y"
    { (yyval.sval) = strdup("MULTIPLY"); ;}
    break;

  case 49:
#line 486 "compiler/parser/tbol.y"
    { (yyval.sval) = strdup("DIVIDE"); ;}
    break;

  case 50:
#line 487 "compiler/parser/tbol.y"
    { (yyval.sval) = strdup("XOR"); ;}
    break;

  case 51:
#line 488 "compiler/parser/tbol.y"
    { (yyval.sval) = strdup("TEST"); ;}
    break;

  case 52:
#line 490 "compiler/parser/tbol.y"
    { (yyval.sval) = strdup("NAVIGATE"); ;}
    break;

  case 53:
#line 491 "compiler/parser/tbol.y"
    { (yyval.sval) = strdup("FETCH"); ;}
    break;

  case 54:
#line 492 "compiler/parser/tbol.y"
    { (yyval.sval) = strdup("OPEN_WINDOW"); ;}
    break;

  case 55:
#line 493 "compiler/parser/tbol.y"
    { (yyval.sval) = strdup("CLOSE_WINDOW"); ;}
    break;

  case 56:
#line 494 "compiler/parser/tbol.y"
    { (yyval.sval) = strdup("OPEN_ERROR_WINDOW"); ;}
    break;

  case 57:
#line 495 "compiler/parser/tbol.y"
    { (yyval.sval) = strdup("KILL"); ;}
    break;

  case 58:
#line 496 "compiler/parser/tbol.y"
    { (yyval.sval) = strdup("LINK"); ;}
    break;

  case 59:
#line 497 "compiler/parser/tbol.y"
    { (yyval.sval) = strdup("TRANSFER"); ;}
    break;

  case 60:
#line 498 "compiler/parser/tbol.y"
    { (yyval.sval) = strdup("PURGE_CACHE"); ;}
    break;

  case 61:
#line 500 "compiler/parser/tbol.y"
    { (yyval.sval) = strdup("OPEN"); ;}
    break;

  case 62:
#line 501 "compiler/parser/tbol.y"
    { (yyval.sval) = strdup("CLOSE"); ;}
    break;

  case 63:
#line 502 "compiler/parser/tbol.y"
    { (yyval.sval) = strdup("READ"); ;}
    break;

  case 64:
#line 503 "compiler/parser/tbol.y"
    { (yyval.sval) = strdup("WRITE"); ;}
    break;

  case 65:
#line 504 "compiler/parser/tbol.y"
    { (yyval.sval) = strdup("NOTE"); ;}
    break;

  case 66:
#line 505 "compiler/parser/tbol.y"
    { (yyval.sval) = strdup("POINT"); ;}
    break;

  case 67:
#line 506 "compiler/parser/tbol.y"
    { (yyval.sval) = strdup("DELETE"); ;}
    break;

  case 68:
#line 508 "compiler/parser/tbol.y"
    { (yyval.sval) = strdup("CONNECT"); ;}
    break;

  case 69:
#line 509 "compiler/parser/tbol.y"
    { (yyval.sval) = strdup("DISCONNECT"); ;}
    break;

  case 70:
#line 510 "compiler/parser/tbol.y"
    { (yyval.sval) = strdup("SEND"); ;}
    break;

  case 71:
#line 511 "compiler/parser/tbol.y"
    { (yyval.sval) = strdup("RECEIVE"); ;}
    break;

  case 72:
#line 512 "compiler/parser/tbol.y"
    { (yyval.sval) = strdup("CANCEL"); ;}
    break;

  case 73:
#line 514 "compiler/parser/tbol.y"
    { (yyval.sval) = strdup("SAVE"); ;}
    break;

  case 74:
#line 515 "compiler/parser/tbol.y"
    { (yyval.sval) = strdup("RESTORE"); ;}
    break;

  case 75:
#line 516 "compiler/parser/tbol.y"
    { (yyval.sval) = strdup("RELEASE"); ;}
    break;

  case 76:
#line 518 "compiler/parser/tbol.y"
    { (yyval.sval) = strdup("WAIT"); ;}
    break;

  case 77:
#line 519 "compiler/parser/tbol.y"
    { (yyval.sval) = strdup("START"); ;}
    break;

  case 78:
#line 520 "compiler/parser/tbol.y"
    { (yyval.sval) = strdup("STOP"); ;}
    break;

  case 79:
#line 522 "compiler/parser/tbol.y"
    { (yyval.sval) = strdup("REFRESH"); ;}
    break;

  case 80:
#line 523 "compiler/parser/tbol.y"
    { (yyval.sval) = strdup("ERASE"); ;}
    break;

  case 81:
#line 524 "compiler/parser/tbol.y"
    { (yyval.sval) = strdup("SET_CURSOR"); ;}
    break;

  case 82:
#line 525 "compiler/parser/tbol.y"
    { (yyval.sval) = strdup("SOUND"); ;}
    break;

  case 83:
#line 527 "compiler/parser/tbol.y"
    { (yyval.sval) = strdup("DEFINE_FIELD"); ;}
    break;

  case 84:
#line 528 "compiler/parser/tbol.y"
    { (yyval.sval) = strdup("SET_ATTRIBUTE"); ;}
    break;

  case 85:
#line 529 "compiler/parser/tbol.y"
    { (yyval.sval) = strdup("SET_FUNCTION"); ;}
    break;

  case 86:
#line 530 "compiler/parser/tbol.y"
    { (yyval.sval) = strdup("SET_KEY"); ;}
    break;

  case 87:
#line 532 "compiler/parser/tbol.y"
    { (yyval.sval) = strdup("FORMAT"); ;}
    break;

  case 88:
#line 533 "compiler/parser/tbol.y"
    { (yyval.sval) = strdup("LOOKUP"); ;}
    break;

  case 89:
#line 534 "compiler/parser/tbol.y"
    { (yyval.sval) = strdup("SORT"); ;}
    break;

  case 90:
#line 535 "compiler/parser/tbol.y"
    { (yyval.sval) = strdup("ERROR"); ;}
    break;

  case 91:
#line 536 "compiler/parser/tbol.y"
    { (yyval.sval) = strdup("TRIGGER_FUNCTION"); ;}
    break;

  case 92:
#line 540 "compiler/parser/tbol.y"
    {
        (yyval.node) = (yyvsp[(1) - (2)].node); (yyvsp[(1) - (2)].node) = NULL;
        /* Verb statements and proc calls set only their start location; give
         * them a true end span (through the last operand) for .sdb column
         * info. @1 spans the whole simple_stmt reduction. */
        if ((yyval.node)) ast_set_end((yyval.node), MAKE_END_LOC((yylsp[(1) - (2)])));
    ;}
    break;

  case 93:
#line 547 "compiler/parser/tbol.y"
    {
        (yyval.node) = (yyvsp[(1) - (1)].node); (yyvsp[(1) - (1)].node) = NULL;
    ;}
    break;

  case 94:
#line 550 "compiler/parser/tbol.y"
    {
        (yyval.node) = (yyvsp[(1) - (1)].node); (yyvsp[(1) - (1)].node) = NULL;
    ;}
    break;

  case 95:
#line 553 "compiler/parser/tbol.y"
    {
        (yyval.node) = (yyvsp[(1) - (1)].node); (yyvsp[(1) - (1)].node) = NULL;
    ;}
    break;

  case 96:
#line 556 "compiler/parser/tbol.y"
    {
        /* Empty statement - can occur after COPY expansion */
        (yyval.node) = NULL;
    ;}
    break;

  case 97:
#line 560 "compiler/parser/tbol.y"
    {
        yyerrok;
        (yyval.node) = ast_error(MAKE_LOC((yylsp[(1) - (2)])));
    ;}
    break;

  case 98:
#line 568 "compiler/parser/tbol.y"
    {
        char *val = NULL;
        char *preproc_val = NULL;
        switch ((yyvsp[(4) - (5)].node)->kind) {
            case AST_LITERAL_STR:
                val = strdup((yyvsp[(4) - (5)].node)->data.str_lit.value);
                asprintf(&preproc_val, "'%s'", (yyvsp[(4) - (5)].node)->data.str_lit.value);
                break;
            case AST_LITERAL_NUM:
            case AST_LITERAL_HEX:
                val = strdup((yyvsp[(4) - (5)].node)->data.num_lit.text);
                preproc_val = strdup((yyvsp[(4) - (5)].node)->data.num_lit.text);
                break;
            case AST_PEV:
                asprintf(&val, "&%d", (yyvsp[(4) - (5)].node)->data.ext_var.number);
                preproc_val = strdup(val);
                break;
            case AST_GEV:
                asprintf(&val, "#%d", (yyvsp[(4) - (5)].node)->data.ext_var.number);
                preproc_val = strdup(val);
                break;
            case AST_IDENT:
                val = strdup((yyvsp[(4) - (5)].node)->data.ident.name);
                preproc_val = strdup(val);
                break;
            case AST_REG_I:
                asprintf(&val, "I%d", (yyvsp[(4) - (5)].node)->data.reg.number);
                preproc_val = strdup(val);
                break;
            case AST_REG_D:
                asprintf(&val, "D%d", (yyvsp[(4) - (5)].node)->data.reg.number);
                preproc_val = strdup(val);
                break;
            case AST_REG_P:
                asprintf(&val, "P%d", (yyvsp[(4) - (5)].node)->data.reg.number);
                preproc_val = strdup(val);
                break;
            default:
                val = strdup("?");
                preproc_val = strdup("?");
        }
        preproc_add_define((yyvsp[(2) - (5)].ident_pair).canonical, preproc_val, (yylsp[(1) - (5)]).first_line, (yylsp[(1) - (5)]).first_column);
        free(preproc_val);
        (yyval.node) = ast_define((yyvsp[(2) - (5)].ident_pair).canonical, val, MAKE_LOC((yylsp[(1) - (5)])));
        if ((yyvsp[(2) - (5)].ident_pair).original) (yyval.node)->data.define.original_text = (yyvsp[(2) - (5)].ident_pair).original;
        ast_set_end((yyval.node), MAKE_LOC((yylsp[(5) - (5)])));
        free((yyvsp[(2) - (5)].ident_pair).canonical);
        free(val);
        ast_free((yyvsp[(4) - (5)].node)); (yyvsp[(4) - (5)].node) = NULL;
    ;}
    break;

  case 99:
#line 621 "compiler/parser/tbol.y"
    { (yyval.node) = (yyvsp[(1) - (1)].node); (yyvsp[(1) - (1)].node) = NULL; ;}
    break;

  case 100:
#line 622 "compiler/parser/tbol.y"
    { (yyval.node) = (yyvsp[(1) - (1)].node); (yyvsp[(1) - (1)].node) = NULL; ;}
    break;

  case 101:
#line 629 "compiler/parser/tbol.y"
    { (yyval.node) = (yyvsp[(1) - (1)].node); (yyvsp[(1) - (1)].node) = NULL; ;}
    break;

  case 102:
#line 630 "compiler/parser/tbol.y"
    { (yyval.node) = (yyvsp[(1) - (1)].node); (yyvsp[(1) - (1)].node) = NULL; ;}
    break;

  case 103:
#line 634 "compiler/parser/tbol.y"
    {
        (yyval.node) = ast_if(MAKE_LOC((yylsp[(1) - (4)])));
        ast_add_child((yyval.node), (yyvsp[(2) - (4)].node)); (yyvsp[(2) - (4)].node) = NULL;
        ast_add_child((yyval.node), (yyvsp[(4) - (4)].node)); (yyvsp[(4) - (4)].node) = NULL;
        ast_set_end((yyval.node), MAKE_LOC((yylsp[(4) - (4)])));
    ;}
    break;

  case 104:
#line 640 "compiler/parser/tbol.y"
    {
        (yyval.node) = ast_if(MAKE_LOC((yylsp[(1) - (6)])));
        ast_add_child((yyval.node), (yyvsp[(2) - (6)].node)); (yyvsp[(2) - (6)].node) = NULL;
        ast_add_child((yyval.node), (yyvsp[(4) - (6)].node)); (yyvsp[(4) - (6)].node) = NULL;
        ast_add_child((yyval.node), (yyvsp[(6) - (6)].node)); (yyvsp[(6) - (6)].node) = NULL;
        ast_set_end((yyval.node), MAKE_LOC((yylsp[(6) - (6)])));
    ;}
    break;

  case 105:
#line 650 "compiler/parser/tbol.y"
    {
        (yyval.node) = ast_while(MAKE_LOC((yylsp[(1) - (4)])));
        ast_add_child((yyval.node), (yyvsp[(2) - (4)].node)); (yyvsp[(2) - (4)].node) = NULL;
        ast_add_child((yyval.node), (yyvsp[(4) - (4)].node)); (yyvsp[(4) - (4)].node) = NULL;
        ast_set_end((yyval.node), MAKE_LOC((yylsp[(4) - (4)])));
    ;}
    break;

  case 106:
#line 659 "compiler/parser/tbol.y"
    {
        (yyval.node) = ast_do_block(MAKE_LOC((yylsp[(1) - (4)])));
        if ((yyvsp[(2) - (4)].node)) {
            for (int i = 0; i < (yyvsp[(2) - (4)].node)->child_count; i++) {
                ast_add_child((yyval.node), (yyvsp[(2) - (4)].node)->children[i]);
            }
            (yyvsp[(2) - (4)].node)->child_count = 0;
            ast_free((yyvsp[(2) - (4)].node)); (yyvsp[(2) - (4)].node) = NULL;
        }
        ast_set_end((yyval.node), MAKE_LOC((yylsp[(4) - (4)])));
    ;}
    break;

  case 107:
#line 670 "compiler/parser/tbol.y"
    {
        yyerrok;
        (yyval.node) = ast_do_block(MAKE_LOC((yylsp[(1) - (4)])));
        ast_add_child((yyval.node), ast_error(MAKE_LOC((yylsp[(2) - (4)]))));
    ;}
    break;

  case 108:
#line 679 "compiler/parser/tbol.y"
    { (yyval.node) = (yyvsp[(1) - (1)].node); (yyvsp[(1) - (1)].node) = NULL; ;}
    break;

  case 109:
#line 680 "compiler/parser/tbol.y"
    {
        (yyval.node) = ast_logic_or((yyvsp[(1) - (3)].node), (yyvsp[(3) - (3)].node), MAKE_LOC((yylsp[(2) - (3)])));
        (yyvsp[(1) - (3)].node) = NULL; (yyvsp[(3) - (3)].node) = NULL;
    ;}
    break;

  case 110:
#line 687 "compiler/parser/tbol.y"
    { (yyval.node) = (yyvsp[(1) - (1)].node); (yyvsp[(1) - (1)].node) = NULL; ;}
    break;

  case 111:
#line 688 "compiler/parser/tbol.y"
    {
        (yyval.node) = ast_logic_and((yyvsp[(1) - (3)].node), (yyvsp[(3) - (3)].node), MAKE_LOC((yylsp[(2) - (3)])));
        (yyvsp[(1) - (3)].node) = NULL; (yyvsp[(3) - (3)].node) = NULL;
    ;}
    break;

  case 112:
#line 695 "compiler/parser/tbol.y"
    {
        (yyval.node) = ast_compare((yyvsp[(2) - (3)].cmp_op), (yyvsp[(1) - (3)].node), (yyvsp[(3) - (3)].node), MAKE_LOC((yylsp[(2) - (3)])));
        (yyvsp[(1) - (3)].node) = NULL; (yyvsp[(3) - (3)].node) = NULL;
    ;}
    break;

  case 113:
#line 699 "compiler/parser/tbol.y"
    {
        (yyval.node) = (yyvsp[(2) - (3)].node); (yyvsp[(2) - (3)].node) = NULL;
    ;}
    break;

  case 114:
#line 705 "compiler/parser/tbol.y"
    { (yyval.cmp_op) = CMP_EQ; ;}
    break;

  case 115:
#line 706 "compiler/parser/tbol.y"
    { (yyval.cmp_op) = CMP_NE; ;}
    break;

  case 116:
#line 707 "compiler/parser/tbol.y"
    { (yyval.cmp_op) = CMP_LT; ;}
    break;

  case 117:
#line 708 "compiler/parser/tbol.y"
    { (yyval.cmp_op) = CMP_GT; ;}
    break;

  case 118:
#line 709 "compiler/parser/tbol.y"
    { (yyval.cmp_op) = CMP_LE; ;}
    break;

  case 119:
#line 710 "compiler/parser/tbol.y"
    { (yyval.cmp_op) = CMP_GE; ;}
    break;

  case 120:
#line 716 "compiler/parser/tbol.y"
    { (yyval.node) = ast_verb("DISCONNECT", MAKE_LOC((yylsp[(1) - (1)]))); ;}
    break;

  case 121:
#line 717 "compiler/parser/tbol.y"
    { (yyval.node) = ast_verb("PURGE_CACHE", MAKE_LOC((yylsp[(1) - (1)]))); ;}
    break;

  case 122:
#line 718 "compiler/parser/tbol.y"
    { (yyval.node) = ast_verb("REFRESH", MAKE_LOC((yylsp[(1) - (1)]))); ;}
    break;

  case 123:
#line 719 "compiler/parser/tbol.y"
    { (yyval.node) = ast_verb("WAIT", MAKE_LOC((yylsp[(1) - (1)]))); ;}
    break;

  case 124:
#line 722 "compiler/parser/tbol.y"
    {
        (yyval.node) = ast_verb("RETURN", MAKE_LOC((yylsp[(1) - (1)])));
    ;}
    break;

  case 125:
#line 725 "compiler/parser/tbol.y"
    {
        (yyval.node) = ast_verb("RETURN", MAKE_LOC((yylsp[(1) - (2)])));
        ast_add_child((yyval.node), (yyvsp[(2) - (2)].node)); (yyvsp[(2) - (2)].node) = NULL;
    ;}
    break;

  case 126:
#line 729 "compiler/parser/tbol.y"
    {
        (yyval.node) = ast_verb("EXIT", MAKE_LOC((yylsp[(1) - (1)])));
    ;}
    break;

  case 127:
#line 732 "compiler/parser/tbol.y"
    {
        (yyval.node) = ast_verb("EXIT", MAKE_LOC((yylsp[(1) - (2)])));
        ast_add_child((yyval.node), (yyvsp[(2) - (2)].node)); (yyvsp[(2) - (2)].node) = NULL;
    ;}
    break;

  case 128:
#line 736 "compiler/parser/tbol.y"
    {
        (yyval.node) = ast_verb("CLOSE_WINDOW", MAKE_LOC((yylsp[(1) - (1)])));
    ;}
    break;

  case 129:
#line 739 "compiler/parser/tbol.y"
    {
        (yyval.node) = ast_verb("CLOSE_WINDOW", MAKE_LOC((yylsp[(1) - (2)])));
        ast_add_child((yyval.node), (yyvsp[(2) - (2)].node)); (yyvsp[(2) - (2)].node) = NULL;
    ;}
    break;

  case 130:
#line 743 "compiler/parser/tbol.y"
    {
        (yyval.node) = ast_verb("OPEN_ERROR_WINDOW", MAKE_LOC((yylsp[(1) - (1)])));
    ;}
    break;

  case 131:
#line 746 "compiler/parser/tbol.y"
    {
        (yyval.node) = ast_verb("OPEN_ERROR_WINDOW", MAKE_LOC((yylsp[(1) - (2)])));
        ast_add_child((yyval.node), (yyvsp[(2) - (2)].node)); (yyvsp[(2) - (2)].node) = NULL;
    ;}
    break;

  case 132:
#line 750 "compiler/parser/tbol.y"
    {
        (yyval.node) = ast_verb("OPEN_ERROR_WINDOW", MAKE_LOC((yylsp[(1) - (4)])));
        ast_add_child((yyval.node), (yyvsp[(2) - (4)].node)); (yyvsp[(2) - (4)].node) = NULL;
        ast_add_child((yyval.node), (yyvsp[(4) - (4)].node)); (yyvsp[(4) - (4)].node) = NULL;
    ;}
    break;

  case 133:
#line 755 "compiler/parser/tbol.y"
    {
        (yyval.node) = ast_verb("ERASE", MAKE_LOC((yylsp[(1) - (1)])));
    ;}
    break;

  case 134:
#line 758 "compiler/parser/tbol.y"
    {
        (yyval.node) = ast_verb("ERASE", MAKE_LOC((yylsp[(1) - (2)])));
        ast_add_child((yyval.node), (yyvsp[(2) - (2)].node)); (yyvsp[(2) - (2)].node) = NULL;
    ;}
    break;

  case 135:
#line 762 "compiler/parser/tbol.y"
    {
        (yyval.node) = ast_verb("SET_CURSOR", MAKE_LOC((yylsp[(1) - (1)])));
    ;}
    break;

  case 136:
#line 765 "compiler/parser/tbol.y"
    {
        (yyval.node) = ast_verb("SET_CURSOR", MAKE_LOC((yylsp[(1) - (2)])));
        ast_add_child((yyval.node), (yyvsp[(2) - (2)].node)); (yyvsp[(2) - (2)].node) = NULL;
    ;}
    break;

  case 137:
#line 769 "compiler/parser/tbol.y"
    {
        (yyval.node) = ast_verb("SOUND", MAKE_LOC((yylsp[(1) - (1)])));
    ;}
    break;

  case 138:
#line 772 "compiler/parser/tbol.y"
    {
        (yyval.node) = ast_verb("KILL", MAKE_LOC((yylsp[(1) - (1)])));
    ;}
    break;

  case 139:
#line 775 "compiler/parser/tbol.y"
    {
        (yyval.node) = ast_verb("KILL", MAKE_LOC((yylsp[(1) - (2)])));
        ast_add_child((yyval.node), (yyvsp[(2) - (2)].node)); (yyvsp[(2) - (2)].node) = NULL;
    ;}
    break;

  case 140:
#line 779 "compiler/parser/tbol.y"
    {
        (yyval.node) = ast_verb("CANCEL", MAKE_LOC((yylsp[(1) - (1)])));
    ;}
    break;

  case 141:
#line 782 "compiler/parser/tbol.y"
    {
        (yyval.node) = ast_verb("CANCEL", MAKE_LOC((yylsp[(1) - (2)])));
        ast_add_child((yyval.node), (yyvsp[(2) - (2)].node)); (yyvsp[(2) - (2)].node) = NULL;
    ;}
    break;

  case 142:
#line 786 "compiler/parser/tbol.y"
    {
        (yyval.node) = ast_verb("START", MAKE_LOC((yylsp[(1) - (1)])));
    ;}
    break;

  case 143:
#line 789 "compiler/parser/tbol.y"
    {
        (yyval.node) = ast_verb("START", MAKE_LOC((yylsp[(1) - (2)])));
        ast_add_child((yyval.node), (yyvsp[(2) - (2)].node)); (yyvsp[(2) - (2)].node) = NULL;
    ;}
    break;

  case 144:
#line 793 "compiler/parser/tbol.y"
    {
        (yyval.node) = ast_verb("STOP", MAKE_LOC((yylsp[(1) - (1)])));
    ;}
    break;

  case 145:
#line 796 "compiler/parser/tbol.y"
    {
        (yyval.node) = ast_verb("STOP", MAKE_LOC((yylsp[(1) - (2)])));
        ast_add_child((yyval.node), (yyvsp[(2) - (2)].node)); (yyvsp[(2) - (2)].node) = NULL;
    ;}
    break;

  case 146:
#line 802 "compiler/parser/tbol.y"
    {
        (yyval.node) = ast_verb("CLEAR", MAKE_LOC((yylsp[(1) - (2)])));
        ast_add_child((yyval.node), (yyvsp[(2) - (2)].node)); (yyvsp[(2) - (2)].node) = NULL;
    ;}
    break;

  case 147:
#line 806 "compiler/parser/tbol.y"
    {
        (yyval.node) = ast_verb("CLOSE", MAKE_LOC((yylsp[(1) - (2)])));
        ast_add_child((yyval.node), (yyvsp[(2) - (2)].node)); (yyvsp[(2) - (2)].node) = NULL;
    ;}
    break;

  case 148:
#line 810 "compiler/parser/tbol.y"
    {
        (yyval.node) = ast_verb("CONNECT", MAKE_LOC((yylsp[(1) - (2)])));
        ast_add_child((yyval.node), (yyvsp[(2) - (2)].node)); (yyvsp[(2) - (2)].node) = NULL;
    ;}
    break;

  case 149:
#line 814 "compiler/parser/tbol.y"
    {
        (yyval.node) = ast_verb("NAVIGATE", MAKE_LOC((yylsp[(1) - (2)])));
        ast_add_child((yyval.node), (yyvsp[(2) - (2)].node)); (yyvsp[(2) - (2)].node) = NULL;
    ;}
    break;

  case 150:
#line 818 "compiler/parser/tbol.y"
    {
        (yyval.node) = ast_verb("NAVIGATE", MAKE_LOC((yylsp[(1) - (4)])));
        ast_add_child((yyval.node), (yyvsp[(2) - (4)].node)); (yyvsp[(2) - (4)].node) = NULL;
        ast_add_child((yyval.node), (yyvsp[(4) - (4)].node)); (yyvsp[(4) - (4)].node) = NULL;
    ;}
    break;

  case 151:
#line 823 "compiler/parser/tbol.y"
    {
        (yyval.node) = ast_verb("NAVIGATE_FIRST", MAKE_LOC((yylsp[(1) - (2)])));
    ;}
    break;

  case 152:
#line 826 "compiler/parser/tbol.y"
    {
        (yyval.node) = ast_verb("NAVIGATE_NEXT", MAKE_LOC((yylsp[(1) - (2)])));
    ;}
    break;

  case 153:
#line 829 "compiler/parser/tbol.y"
    {
        (yyval.node) = ast_verb("NAVIGATE_BACK", MAKE_LOC((yylsp[(1) - (2)])));
    ;}
    break;

  case 154:
#line 832 "compiler/parser/tbol.y"
    {
        (yyval.node) = ast_verb("NAVIGATE_LAST", MAKE_LOC((yylsp[(1) - (2)])));
    ;}
    break;

  case 155:
#line 835 "compiler/parser/tbol.y"
    {
        (yyval.node) = ast_verb("OPEN_WINDOW", MAKE_LOC((yylsp[(1) - (2)])));
        ast_add_child((yyval.node), (yyvsp[(2) - (2)].node)); (yyvsp[(2) - (2)].node) = NULL;
    ;}
    break;

  case 156:
#line 839 "compiler/parser/tbol.y"
    {
        (yyval.node) = ast_verb("POP", MAKE_LOC((yylsp[(1) - (2)])));
        ast_add_child((yyval.node), (yyvsp[(2) - (2)].node)); (yyvsp[(2) - (2)].node) = NULL;
    ;}
    break;

  case 157:
#line 843 "compiler/parser/tbol.y"
    {
        (yyval.node) = ast_verb("PUSH", MAKE_LOC((yylsp[(1) - (2)])));
        ast_add_child((yyval.node), (yyvsp[(2) - (2)].node)); (yyvsp[(2) - (2)].node) = NULL;
    ;}
    break;

  case 158:
#line 847 "compiler/parser/tbol.y"
    {
        (yyval.node) = ast_verb("RELEASE", MAKE_LOC((yylsp[(1) - (2)])));
        ast_add_child((yyval.node), (yyvsp[(2) - (2)].node)); (yyvsp[(2) - (2)].node) = NULL;
    ;}
    break;

  case 159:
#line 851 "compiler/parser/tbol.y"
    {
        (yyval.node) = ast_verb("UPPERCASE", MAKE_LOC((yylsp[(1) - (2)])));
        ast_add_child((yyval.node), (yyvsp[(2) - (2)].node)); (yyvsp[(2) - (2)].node) = NULL;
    ;}
    break;

  case 160:
#line 855 "compiler/parser/tbol.y"
    {
        (yyval.node) = ast_verb("ERROR", MAKE_LOC((yylsp[(1) - (2)])));
        ast_add_child((yyval.node), (yyvsp[(2) - (2)].node)); (yyvsp[(2) - (2)].node) = NULL;
    ;}
    break;

  case 161:
#line 859 "compiler/parser/tbol.y"
    {
        (yyval.node) = ast_verb("TRIGGER_FUNCTION", MAKE_LOC((yylsp[(1) - (2)])));
        ast_add_child((yyval.node), (yyvsp[(2) - (2)].node)); (yyvsp[(2) - (2)].node) = NULL;
    ;}
    break;

  case 162:
#line 863 "compiler/parser/tbol.y"
    {
        (yyval.node) = ast_verb("DELETE", MAKE_LOC((yylsp[(1) - (2)])));
        ast_add_child((yyval.node), (yyvsp[(2) - (2)].node)); (yyvsp[(2) - (2)].node) = NULL;
    ;}
    break;

  case 163:
#line 869 "compiler/parser/tbol.y"
    {
        (yyval.node) = ast_verb("CLEAR", MAKE_LOC((yylsp[(1) - (4)])));
        ast_add_child((yyval.node), (yyvsp[(2) - (4)].node)); (yyvsp[(2) - (4)].node) = NULL;
        ast_add_child((yyval.node), (yyvsp[(4) - (4)].node)); (yyvsp[(4) - (4)].node) = NULL;
    ;}
    break;

  case 164:
#line 874 "compiler/parser/tbol.y"
    {
        (yyval.node) = ast_verb("FETCH", MAKE_LOC((yylsp[(1) - (2)])));
        ast_add_child((yyval.node), (yyvsp[(2) - (2)].node)); (yyvsp[(2) - (2)].node) = NULL;
    ;}
    break;

  case 165:
#line 878 "compiler/parser/tbol.y"
    {
        (yyval.node) = ast_verb("FETCH", MAKE_LOC((yylsp[(1) - (4)])));
        ast_add_child((yyval.node), (yyvsp[(2) - (4)].node)); (yyvsp[(2) - (4)].node) = NULL;
        ast_add_child((yyval.node), (yyvsp[(4) - (4)].node)); (yyvsp[(4) - (4)].node) = NULL;
    ;}
    break;

  case 166:
#line 885 "compiler/parser/tbol.y"
    {
        (yyval.node) = ast_verb("ADD", MAKE_LOC((yylsp[(1) - (4)])));
        ast_add_child((yyval.node), (yyvsp[(2) - (4)].node)); (yyvsp[(2) - (4)].node) = NULL;
        ast_add_child((yyval.node), (yyvsp[(4) - (4)].node)); (yyvsp[(4) - (4)].node) = NULL;
    ;}
    break;

  case 167:
#line 890 "compiler/parser/tbol.y"
    {
        (yyval.node) = ast_verb("AND", MAKE_LOC((yylsp[(1) - (4)])));
        ast_add_child((yyval.node), (yyvsp[(2) - (4)].node)); (yyvsp[(2) - (4)].node) = NULL;
        ast_add_child((yyval.node), (yyvsp[(4) - (4)].node)); (yyvsp[(4) - (4)].node) = NULL;
    ;}
    break;

  case 168:
#line 895 "compiler/parser/tbol.y"
    {
        (yyval.node) = ast_verb("LENGTH", MAKE_LOC((yylsp[(1) - (4)])));
        ast_add_child((yyval.node), (yyvsp[(2) - (4)].node)); (yyvsp[(2) - (4)].node) = NULL;
        ast_add_child((yyval.node), (yyvsp[(4) - (4)].node)); (yyvsp[(4) - (4)].node) = NULL;
    ;}
    break;

  case 169:
#line 900 "compiler/parser/tbol.y"
    {
        (yyval.node) = ast_verb("MOVE", MAKE_LOC((yylsp[(1) - (4)])));
        ast_add_child((yyval.node), (yyvsp[(2) - (4)].node)); (yyvsp[(2) - (4)].node) = NULL;
        ast_add_child((yyval.node), (yyvsp[(4) - (4)].node)); (yyvsp[(4) - (4)].node) = NULL;
    ;}
    break;

  case 170:
#line 905 "compiler/parser/tbol.y"
    {
        (yyval.node) = ast_verb("MOVE_ABS", MAKE_LOC((yylsp[(1) - (6)])));
        ast_add_child((yyval.node), (yyvsp[(2) - (6)].node)); (yyvsp[(2) - (6)].node) = NULL;
        ast_add_child((yyval.node), (yyvsp[(4) - (6)].node)); (yyvsp[(4) - (6)].node) = NULL;
    ;}
    break;

  case 171:
#line 910 "compiler/parser/tbol.y"
    {
        (yyval.node) = ast_verb("MULTIPLY", MAKE_LOC((yylsp[(1) - (4)])));
        ast_add_child((yyval.node), (yyvsp[(2) - (4)].node)); (yyvsp[(2) - (4)].node) = NULL;
        ast_add_child((yyval.node), (yyvsp[(4) - (4)].node)); (yyvsp[(4) - (4)].node) = NULL;
    ;}
    break;

  case 172:
#line 915 "compiler/parser/tbol.y"
    {
        (yyval.node) = ast_verb("NOTE", MAKE_LOC((yylsp[(1) - (4)])));
        ast_add_child((yyval.node), (yyvsp[(2) - (4)].node)); (yyvsp[(2) - (4)].node) = NULL;
        ast_add_child((yyval.node), (yyvsp[(4) - (4)].node)); (yyvsp[(4) - (4)].node) = NULL;
    ;}
    break;

  case 173:
#line 920 "compiler/parser/tbol.y"
    {
        (yyval.node) = ast_verb("OPEN", MAKE_LOC((yylsp[(1) - (4)])));
        ast_add_child((yyval.node), (yyvsp[(2) - (4)].node)); (yyvsp[(2) - (4)].node) = NULL;
        ast_add_child((yyval.node), (yyvsp[(4) - (4)].node)); (yyvsp[(4) - (4)].node) = NULL;
    ;}
    break;

  case 174:
#line 925 "compiler/parser/tbol.y"
    {
        (yyval.node) = ast_verb("OR", MAKE_LOC((yylsp[(1) - (4)])));
        ast_add_child((yyval.node), (yyvsp[(2) - (4)].node)); (yyvsp[(2) - (4)].node) = NULL;
        ast_add_child((yyval.node), (yyvsp[(4) - (4)].node)); (yyvsp[(4) - (4)].node) = NULL;
    ;}
    break;

  case 175:
#line 930 "compiler/parser/tbol.y"
    {
        (yyval.node) = ast_verb("POINT", MAKE_LOC((yylsp[(1) - (4)])));
        ast_add_child((yyval.node), (yyvsp[(2) - (4)].node)); (yyvsp[(2) - (4)].node) = NULL;
        ast_add_child((yyval.node), (yyvsp[(4) - (4)].node)); (yyvsp[(4) - (4)].node) = NULL;
    ;}
    break;

  case 176:
#line 935 "compiler/parser/tbol.y"
    {
        (yyval.node) = ast_verb("READ", MAKE_LOC((yylsp[(1) - (4)])));
        ast_add_child((yyval.node), (yyvsp[(2) - (4)].node)); (yyvsp[(2) - (4)].node) = NULL;
        ast_add_child((yyval.node), (yyvsp[(4) - (4)].node)); (yyvsp[(4) - (4)].node) = NULL;
    ;}
    break;

  case 177:
#line 940 "compiler/parser/tbol.y"
    {
        (yyval.node) = ast_verb("READ", MAKE_LOC((yylsp[(1) - (6)])));
        ast_add_child((yyval.node), (yyvsp[(2) - (6)].node)); (yyvsp[(2) - (6)].node) = NULL;
        ast_add_child((yyval.node), (yyvsp[(4) - (6)].node)); (yyvsp[(4) - (6)].node) = NULL;
        ast_add_child((yyval.node), (yyvsp[(6) - (6)].node)); (yyvsp[(6) - (6)].node) = NULL;
    ;}
    break;

  case 178:
#line 946 "compiler/parser/tbol.y"
    {
        (yyval.node) = ast_verb("RECEIVE", MAKE_LOC((yylsp[(1) - (4)])));
        ast_add_child((yyval.node), (yyvsp[(2) - (4)].node)); (yyvsp[(2) - (4)].node) = NULL;
        ast_add_child((yyval.node), (yyvsp[(4) - (4)].node)); (yyvsp[(4) - (4)].node) = NULL;
    ;}
    break;

  case 179:
#line 951 "compiler/parser/tbol.y"
    {
        /* SEND request; */
        (yyval.node) = ast_verb("SEND", MAKE_LOC((yylsp[(1) - (2)])));
        ast_add_child((yyval.node), (yyvsp[(2) - (2)].node)); (yyvsp[(2) - (2)].node) = NULL;
        (yyval.node)->data.call.send_timeout = 0;
        (yyval.node)->data.call.send_flags = 0;
    ;}
    break;

  case 180:
#line 958 "compiler/parser/tbol.y"
    {
        /* SEND request, msg_id; */
        (yyval.node) = ast_verb("SEND", MAKE_LOC((yylsp[(1) - (4)])));
        ast_add_child((yyval.node), (yyvsp[(2) - (4)].node)); (yyvsp[(2) - (4)].node) = NULL;
        ast_add_child((yyval.node), (yyvsp[(4) - (4)].node)); (yyvsp[(4) - (4)].node) = NULL;
        (yyval.node)->data.call.send_timeout = 0;
        (yyval.node)->data.call.send_flags = 0;
    ;}
    break;

  case 181:
#line 966 "compiler/parser/tbol.y"
    {
        /* SEND request, modifiers; */
        (yyval.node) = ast_verb("SEND", MAKE_LOC((yylsp[(1) - (4)])));
        ast_add_child((yyval.node), (yyvsp[(2) - (4)].node)); (yyvsp[(2) - (4)].node) = NULL;
        (yyval.node)->data.call.send_timeout = (yyvsp[(4) - (4)].send_mods).timeout;
        (yyval.node)->data.call.send_flags = (yyvsp[(4) - (4)].send_mods).flags;
    ;}
    break;

  case 182:
#line 973 "compiler/parser/tbol.y"
    {
        /* SEND request, msg_id, modifiers; */
        (yyval.node) = ast_verb("SEND", MAKE_LOC((yylsp[(1) - (6)])));
        ast_add_child((yyval.node), (yyvsp[(2) - (6)].node)); (yyvsp[(2) - (6)].node) = NULL;
        ast_add_child((yyval.node), (yyvsp[(4) - (6)].node)); (yyvsp[(4) - (6)].node) = NULL;
        (yyval.node)->data.call.send_timeout = (yyvsp[(6) - (6)].send_mods).timeout;
        (yyval.node)->data.call.send_flags = (yyvsp[(6) - (6)].send_mods).flags;
    ;}
    break;

  case 183:
#line 981 "compiler/parser/tbol.y"
    {
        (yyval.node) = ast_verb("SET_ATTRIBUTE", MAKE_LOC((yylsp[(1) - (4)])));
        ast_add_child((yyval.node), (yyvsp[(2) - (4)].node)); (yyvsp[(2) - (4)].node) = NULL;
        ast_add_child((yyval.node), (yyvsp[(4) - (4)].node)); (yyvsp[(4) - (4)].node) = NULL;
    ;}
    break;

  case 184:
#line 986 "compiler/parser/tbol.y"
    {
        (yyval.node) = ast_verb("SOUND", MAKE_LOC((yylsp[(1) - (4)])));
        ast_add_child((yyval.node), (yyvsp[(2) - (4)].node)); (yyvsp[(2) - (4)].node) = NULL;
        ast_add_child((yyval.node), (yyvsp[(4) - (4)].node)); (yyvsp[(4) - (4)].node) = NULL;
    ;}
    break;

  case 185:
#line 991 "compiler/parser/tbol.y"
    {
        (yyval.node) = ast_verb("SUBTRACT", MAKE_LOC((yylsp[(1) - (4)])));
        ast_add_child((yyval.node), (yyvsp[(2) - (4)].node)); (yyvsp[(2) - (4)].node) = NULL;
        ast_add_child((yyval.node), (yyvsp[(4) - (4)].node)); (yyvsp[(4) - (4)].node) = NULL;
    ;}
    break;

  case 186:
#line 996 "compiler/parser/tbol.y"
    {
        (yyval.node) = ast_verb("SWAP", MAKE_LOC((yylsp[(1) - (4)])));
        ast_add_child((yyval.node), (yyvsp[(2) - (4)].node)); (yyvsp[(2) - (4)].node) = NULL;
        ast_add_child((yyval.node), (yyvsp[(4) - (4)].node)); (yyvsp[(4) - (4)].node) = NULL;
    ;}
    break;

  case 187:
#line 1001 "compiler/parser/tbol.y"
    {
        (yyval.node) = ast_verb("TEST", MAKE_LOC((yylsp[(1) - (4)])));
        ast_add_child((yyval.node), (yyvsp[(2) - (4)].node)); (yyvsp[(2) - (4)].node) = NULL;
        ast_add_child((yyval.node), (yyvsp[(4) - (4)].node)); (yyvsp[(4) - (4)].node) = NULL;
    ;}
    break;

  case 188:
#line 1006 "compiler/parser/tbol.y"
    {
        (yyval.node) = ast_verb("WRITE", MAKE_LOC((yylsp[(1) - (4)])));
        ast_add_child((yyval.node), (yyvsp[(2) - (4)].node)); (yyvsp[(2) - (4)].node) = NULL;
        ast_add_child((yyval.node), (yyvsp[(4) - (4)].node)); (yyvsp[(4) - (4)].node) = NULL;
    ;}
    break;

  case 189:
#line 1011 "compiler/parser/tbol.y"
    {
        (yyval.node) = ast_verb("WRITE", MAKE_LOC((yylsp[(1) - (6)])));
        ast_add_child((yyval.node), (yyvsp[(2) - (6)].node)); (yyvsp[(2) - (6)].node) = NULL;
        ast_add_child((yyval.node), (yyvsp[(4) - (6)].node)); (yyvsp[(4) - (6)].node) = NULL;
        ast_add_child((yyval.node), (yyvsp[(6) - (6)].node)); (yyvsp[(6) - (6)].node) = NULL;
    ;}
    break;

  case 190:
#line 1017 "compiler/parser/tbol.y"
    {
        (yyval.node) = ast_verb("XOR", MAKE_LOC((yylsp[(1) - (4)])));
        ast_add_child((yyval.node), (yyvsp[(2) - (4)].node)); (yyvsp[(2) - (4)].node) = NULL;
        ast_add_child((yyval.node), (yyvsp[(4) - (4)].node)); (yyvsp[(4) - (4)].node) = NULL;
    ;}
    break;

  case 191:
#line 1022 "compiler/parser/tbol.y"
    {
        (yyval.node) = ast_verb("FILL", MAKE_LOC((yylsp[(1) - (6)])));
        ast_add_child((yyval.node), (yyvsp[(2) - (6)].node)); (yyvsp[(2) - (6)].node) = NULL;
        ast_add_child((yyval.node), (yyvsp[(4) - (6)].node)); (yyvsp[(4) - (6)].node) = NULL;
        ast_add_child((yyval.node), (yyvsp[(6) - (6)].node)); (yyvsp[(6) - (6)].node) = NULL;
    ;}
    break;

  case 192:
#line 1030 "compiler/parser/tbol.y"
    {
        (yyval.node) = ast_verb("SAVE", MAKE_LOC((yylsp[(1) - (4)])));
        ast_add_child((yyval.node), (yyvsp[(2) - (4)].node)); (yyvsp[(2) - (4)].node) = NULL;
        ast_add_child((yyval.node), (yyvsp[(4) - (4)].node)); (yyvsp[(4) - (4)].node) = NULL;
    ;}
    break;

  case 193:
#line 1035 "compiler/parser/tbol.y"
    {
        (yyval.node) = ast_verb("SAVE", MAKE_LOC((yylsp[(1) - (6)])));
        ast_add_child((yyval.node), (yyvsp[(2) - (6)].node)); (yyvsp[(2) - (6)].node) = NULL;
        ast_add_child((yyval.node), (yyvsp[(4) - (6)].node)); (yyvsp[(4) - (6)].node) = NULL;
        ast_add_child((yyval.node), (yyvsp[(6) - (6)].node)); (yyvsp[(6) - (6)].node) = NULL;
    ;}
    break;

  case 194:
#line 1041 "compiler/parser/tbol.y"
    {
        (yyval.node) = ast_verb("RESTORE", MAKE_LOC((yylsp[(1) - (4)])));
        ast_add_child((yyval.node), (yyvsp[(2) - (4)].node)); (yyvsp[(2) - (4)].node) = NULL;
        ast_add_child((yyval.node), (yyvsp[(4) - (4)].node)); (yyvsp[(4) - (4)].node) = NULL;
    ;}
    break;

  case 195:
#line 1046 "compiler/parser/tbol.y"
    {
        (yyval.node) = ast_verb("DIVIDE", MAKE_LOC((yylsp[(1) - (4)])));
        ast_add_child((yyval.node), (yyvsp[(2) - (4)].node)); (yyvsp[(2) - (4)].node) = NULL;
        ast_add_child((yyval.node), (yyvsp[(4) - (4)].node)); (yyvsp[(4) - (4)].node) = NULL;
    ;}
    break;

  case 196:
#line 1051 "compiler/parser/tbol.y"
    {
        (yyval.node) = ast_verb("DIVIDE", MAKE_LOC((yylsp[(1) - (6)])));
        ast_add_child((yyval.node), (yyvsp[(2) - (6)].node)); (yyvsp[(2) - (6)].node) = NULL;
        ast_add_child((yyval.node), (yyvsp[(4) - (6)].node)); (yyvsp[(4) - (6)].node) = NULL;
        ast_add_child((yyval.node), (yyvsp[(6) - (6)].node)); (yyvsp[(6) - (6)].node) = NULL;
    ;}
    break;

  case 197:
#line 1059 "compiler/parser/tbol.y"
    {
        (yyval.node) = ast_verb("INSTR", MAKE_LOC((yylsp[(1) - (6)])));
        ast_add_child((yyval.node), (yyvsp[(2) - (6)].node)); (yyvsp[(2) - (6)].node) = NULL;
        ast_add_child((yyval.node), (yyvsp[(4) - (6)].node)); (yyvsp[(4) - (6)].node) = NULL;
        ast_add_child((yyval.node), (yyvsp[(6) - (6)].node)); (yyvsp[(6) - (6)].node) = NULL;
    ;}
    break;

  case 198:
#line 1065 "compiler/parser/tbol.y"
    {
        (yyval.node) = ast_verb("FORMAT", MAKE_LOC((yylsp[(1) - (6)])));
        ast_add_child((yyval.node), (yyvsp[(2) - (6)].node)); (yyvsp[(2) - (6)].node) = NULL;
        ast_add_child((yyval.node), (yyvsp[(4) - (6)].node)); (yyvsp[(4) - (6)].node) = NULL;
        ast_add_child((yyval.node), (yyvsp[(6) - (6)].node)); (yyvsp[(6) - (6)].node) = NULL;
    ;}
    break;

  case 199:
#line 1071 "compiler/parser/tbol.y"
    {
        (yyval.node) = ast_verb("SORT", MAKE_LOC((yylsp[(1) - (6)])));
        ast_add_child((yyval.node), (yyvsp[(2) - (6)].node)); (yyvsp[(2) - (6)].node) = NULL;
        ast_add_child((yyval.node), (yyvsp[(4) - (6)].node)); (yyvsp[(4) - (6)].node) = NULL;
        ast_add_child((yyval.node), (yyvsp[(6) - (6)].node)); (yyvsp[(6) - (6)].node) = NULL;
    ;}
    break;

  case 200:
#line 1077 "compiler/parser/tbol.y"
    {
        (yyval.node) = ast_verb("SET_KEY", MAKE_LOC((yylsp[(1) - (6)])));
        ast_add_child((yyval.node), (yyvsp[(2) - (6)].node)); (yyvsp[(2) - (6)].node) = NULL;
        ast_add_child((yyval.node), (yyvsp[(4) - (6)].node)); (yyvsp[(4) - (6)].node) = NULL;
        ast_add_child((yyval.node), (yyvsp[(6) - (6)].node)); (yyvsp[(6) - (6)].node) = NULL;
    ;}
    break;

  case 201:
#line 1083 "compiler/parser/tbol.y"
    {
        /* EDIT dest, format, args... (3+ operands) */
        (yyval.node) = ast_verb("EDIT", MAKE_LOC((yylsp[(1) - (6)])));
        ast_add_child((yyval.node), (yyvsp[(2) - (6)].node)); (yyvsp[(2) - (6)].node) = NULL;  /* dest */
        ast_add_child((yyval.node), (yyvsp[(4) - (6)].node)); (yyvsp[(4) - (6)].node) = NULL;  /* format */
        if ((yyvsp[(6) - (6)].node)) {
            for (int i = 0; i < (yyvsp[(6) - (6)].node)->child_count; i++) {
                ast_add_child((yyval.node), (yyvsp[(6) - (6)].node)->children[i]);
            }
            (yyvsp[(6) - (6)].node)->child_count = 0;
            ast_free((yyvsp[(6) - (6)].node)); (yyvsp[(6) - (6)].node) = NULL;
        }
    ;}
    break;

  case 202:
#line 1098 "compiler/parser/tbol.y"
    {
        (yyval.node) = ast_verb("SUBSTR", MAKE_LOC((yylsp[(1) - (8)])));
        ast_add_child((yyval.node), (yyvsp[(2) - (8)].node)); (yyvsp[(2) - (8)].node) = NULL;
        ast_add_child((yyval.node), (yyvsp[(4) - (8)].node)); (yyvsp[(4) - (8)].node) = NULL;
        ast_add_child((yyval.node), (yyvsp[(6) - (8)].node)); (yyvsp[(6) - (8)].node) = NULL;
        ast_add_child((yyval.node), (yyvsp[(8) - (8)].node)); (yyvsp[(8) - (8)].node) = NULL;
    ;}
    break;

  case 203:
#line 1105 "compiler/parser/tbol.y"
    {
        (yyval.node) = ast_verb("SET_FUNCTION", MAKE_LOC((yylsp[(1) - (4)])));
        ast_add_child((yyval.node), (yyvsp[(2) - (4)].node)); (yyvsp[(2) - (4)].node) = NULL;
        ast_add_child((yyval.node), (yyvsp[(4) - (4)].node)); (yyvsp[(4) - (4)].node) = NULL;
    ;}
    break;

  case 204:
#line 1110 "compiler/parser/tbol.y"
    {
        (yyval.node) = ast_verb("SET_FUNCTION", MAKE_LOC((yylsp[(1) - (6)])));
        ast_add_child((yyval.node), (yyvsp[(2) - (6)].node)); (yyvsp[(2) - (6)].node) = NULL;
        ast_add_child((yyval.node), (yyvsp[(4) - (6)].node)); (yyvsp[(4) - (6)].node) = NULL;
        ast_add_child((yyval.node), (yyvsp[(6) - (6)].node)); (yyvsp[(6) - (6)].node) = NULL;
    ;}
    break;

  case 205:
#line 1116 "compiler/parser/tbol.y"
    {
        (yyval.node) = ast_verb("SET_FUNCTION", MAKE_LOC((yylsp[(1) - (8)])));
        ast_add_child((yyval.node), (yyvsp[(2) - (8)].node)); (yyvsp[(2) - (8)].node) = NULL;
        ast_add_child((yyval.node), (yyvsp[(4) - (8)].node)); (yyvsp[(4) - (8)].node) = NULL;
        ast_add_child((yyval.node), (yyvsp[(6) - (8)].node)); (yyvsp[(6) - (8)].node) = NULL;
        ast_add_child((yyval.node), (yyvsp[(8) - (8)].node)); (yyvsp[(8) - (8)].node) = NULL;
    ;}
    break;

  case 206:
#line 1125 "compiler/parser/tbol.y"
    {
        (yyval.node) = ast_verb("LOOKUP", MAKE_LOC((yylsp[(1) - (10)])));
        ast_add_child((yyval.node), (yyvsp[(2) - (10)].node)); (yyvsp[(2) - (10)].node) = NULL;
        ast_add_child((yyval.node), (yyvsp[(4) - (10)].node)); (yyvsp[(4) - (10)].node) = NULL;
        ast_add_child((yyval.node), (yyvsp[(6) - (10)].node)); (yyvsp[(6) - (10)].node) = NULL;
        ast_add_child((yyval.node), (yyvsp[(8) - (10)].node)); (yyvsp[(8) - (10)].node) = NULL;
        ast_add_child((yyval.node), (yyvsp[(10) - (10)].node)); (yyvsp[(10) - (10)].node) = NULL;
    ;}
    break;

  case 207:
#line 1135 "compiler/parser/tbol.y"
    {
        (yyval.node) = ast_verb("DEFINE_FIELD", MAKE_LOC((yylsp[(1) - (12)])));
        ast_add_child((yyval.node), (yyvsp[(2) - (12)].node)); (yyvsp[(2) - (12)].node) = NULL;
        ast_add_child((yyval.node), (yyvsp[(4) - (12)].node)); (yyvsp[(4) - (12)].node) = NULL;
        ast_add_child((yyval.node), (yyvsp[(6) - (12)].node)); (yyvsp[(6) - (12)].node) = NULL;
        ast_add_child((yyval.node), (yyvsp[(8) - (12)].node)); (yyvsp[(8) - (12)].node) = NULL;
        ast_add_child((yyval.node), (yyvsp[(10) - (12)].node)); (yyvsp[(10) - (12)].node) = NULL;
        ast_add_child((yyval.node), (yyvsp[(12) - (12)].node)); (yyvsp[(12) - (12)].node) = NULL;
    ;}
    break;

  case 208:
#line 1144 "compiler/parser/tbol.y"
    {
        (yyval.node) = ast_verb("DEFINE_FIELD", MAKE_LOC((yylsp[(1) - (14)])));
        ast_add_child((yyval.node), (yyvsp[(2) - (14)].node)); (yyvsp[(2) - (14)].node) = NULL;
        ast_add_child((yyval.node), (yyvsp[(4) - (14)].node)); (yyvsp[(4) - (14)].node) = NULL;
        ast_add_child((yyval.node), (yyvsp[(6) - (14)].node)); (yyvsp[(6) - (14)].node) = NULL;
        ast_add_child((yyval.node), (yyvsp[(8) - (14)].node)); (yyvsp[(8) - (14)].node) = NULL;
        ast_add_child((yyval.node), (yyvsp[(10) - (14)].node)); (yyvsp[(10) - (14)].node) = NULL;
        ast_add_child((yyval.node), (yyvsp[(12) - (14)].node)); (yyvsp[(12) - (14)].node) = NULL;
        ast_add_child((yyval.node), (yyvsp[(14) - (14)].node)); (yyvsp[(14) - (14)].node) = NULL;
    ;}
    break;

  case 209:
#line 1156 "compiler/parser/tbol.y"
    {
        (yyval.node) = ast_goto((yyvsp[(2) - (2)].sval), MAKE_LOC((yylsp[(1) - (2)])));
        ast_set_end((yyval.node), MAKE_END_LOC((yylsp[(2) - (2)])));
        free((yyvsp[(2) - (2)].sval)); (yyvsp[(2) - (2)].sval) = NULL;
    ;}
    break;

  case 210:
#line 1161 "compiler/parser/tbol.y"
    {
        (yyval.node) = ast_goto_depending_on(MAKE_LOC((yylsp[(1) - (4)])));
        ast_add_child((yyval.node), (yyvsp[(2) - (4)].node)); (yyvsp[(2) - (4)].node) = NULL;
        /* Add labels from label_list */
        if ((yyvsp[(4) - (4)].node)) {
            for (int i = 0; i < (yyvsp[(4) - (4)].node)->child_count; i++) {
                ast_add_child((yyval.node), (yyvsp[(4) - (4)].node)->children[i]);
            }
            (yyvsp[(4) - (4)].node)->child_count = 0;
            ast_free((yyvsp[(4) - (4)].node)); (yyvsp[(4) - (4)].node) = NULL;
        }
    ;}
    break;

  case 211:
#line 1175 "compiler/parser/tbol.y"
    {
        (yyval.node) = ast_verb("STRING", MAKE_LOC((yylsp[(1) - (2)])));
        if ((yyvsp[(2) - (2)].node)) {
            for (int i = 0; i < (yyvsp[(2) - (2)].node)->child_count; i++) {
                ast_add_child((yyval.node), (yyvsp[(2) - (2)].node)->children[i]);
            }
            (yyvsp[(2) - (2)].node)->child_count = 0;
            ast_free((yyvsp[(2) - (2)].node)); (yyvsp[(2) - (2)].node) = NULL;
        }
    ;}
    break;

  case 212:
#line 1185 "compiler/parser/tbol.y"
    {
        (yyval.node) = ast_verb("LINK", MAKE_LOC((yylsp[(1) - (2)])));
        if ((yyvsp[(2) - (2)].node)) {
            for (int i = 0; i < (yyvsp[(2) - (2)].node)->child_count; i++) {
                ast_add_child((yyval.node), (yyvsp[(2) - (2)].node)->children[i]);
            }
            (yyvsp[(2) - (2)].node)->child_count = 0;
            ast_free((yyvsp[(2) - (2)].node)); (yyvsp[(2) - (2)].node) = NULL;
        }
    ;}
    break;

  case 213:
#line 1195 "compiler/parser/tbol.y"
    {
        (yyval.node) = ast_verb("TRANSFER", MAKE_LOC((yylsp[(1) - (2)])));
        if ((yyvsp[(2) - (2)].node)) {
            for (int i = 0; i < (yyvsp[(2) - (2)].node)->child_count; i++) {
                ast_add_child((yyval.node), (yyvsp[(2) - (2)].node)->children[i]);
            }
            (yyvsp[(2) - (2)].node)->child_count = 0;
            ast_free((yyvsp[(2) - (2)].node)); (yyvsp[(2) - (2)].node) = NULL;
        }
    ;}
    break;

  case 214:
#line 1207 "compiler/parser/tbol.y"
    {
        (yyval.node) = ast_verb("MAKE_FORMAT", MAKE_LOC((yylsp[(1) - (4)])));
        ast_add_child((yyval.node), (yyvsp[(2) - (4)].node)); (yyvsp[(2) - (4)].node) = NULL;
        if ((yyvsp[(4) - (4)].node)) {
            for (int i = 0; i < (yyvsp[(4) - (4)].node)->child_count; i++) {
                ast_add_child((yyval.node), (yyvsp[(4) - (4)].node)->children[i]);
            }
            (yyvsp[(4) - (4)].node)->child_count = 0;
            ast_free((yyvsp[(4) - (4)].node)); (yyvsp[(4) - (4)].node) = NULL;
        }
    ;}
    break;

  case 215:
#line 1222 "compiler/parser/tbol.y"
    {
        (yyval.node) = ast_new(AST_DO_BLOCK, MAKE_LOC((yylsp[(1) - (1)])));  /* Container */
        ast_add_child((yyval.node), ast_ident((yyvsp[(1) - (1)].sval), MAKE_LOC((yylsp[(1) - (1)]))));
        free((yyvsp[(1) - (1)].sval)); (yyvsp[(1) - (1)].sval) = NULL;
    ;}
    break;

  case 216:
#line 1227 "compiler/parser/tbol.y"
    {
        (yyval.node) = (yyvsp[(1) - (3)].node); (yyvsp[(1) - (3)].node) = NULL;
        ast_add_child((yyval.node), ast_ident((yyvsp[(3) - (3)].sval), MAKE_LOC((yylsp[(3) - (3)]))));
        free((yyvsp[(3) - (3)].sval)); (yyvsp[(3) - (3)].sval) = NULL;
    ;}
    break;

  case 217:
#line 1235 "compiler/parser/tbol.y"
    {
        (yyval.node) = ast_new(AST_DO_BLOCK, MAKE_LOC((yylsp[(1) - (1)])));  /* Container */
        ast_add_child((yyval.node), (yyvsp[(1) - (1)].node)); (yyvsp[(1) - (1)].node) = NULL;
    ;}
    break;

  case 218:
#line 1239 "compiler/parser/tbol.y"
    {
        (yyval.node) = (yyvsp[(1) - (3)].node); (yyvsp[(1) - (3)].node) = NULL;
        ast_add_child((yyval.node), (yyvsp[(3) - (3)].node)); (yyvsp[(3) - (3)].node) = NULL;
    ;}
    break;

  case 219:
#line 1246 "compiler/parser/tbol.y"
    {
        (yyval.node) = ast_new(AST_DO_BLOCK, MAKE_LOC((yylsp[(1) - (1)])));  /* Container */
        ast_add_child((yyval.node), (yyvsp[(1) - (1)].node)); (yyvsp[(1) - (1)].node) = NULL;
    ;}
    break;

  case 220:
#line 1250 "compiler/parser/tbol.y"
    {
        (yyval.node) = (yyvsp[(1) - (3)].node); (yyvsp[(1) - (3)].node) = NULL;
        ast_add_child((yyval.node), (yyvsp[(3) - (3)].node)); (yyvsp[(3) - (3)].node) = NULL;
    ;}
    break;

  case 221:
#line 1257 "compiler/parser/tbol.y"
    {
        /* target:fixed_length - fixed width field */
        (yyval.node) = ast_format_spec((yyvsp[(1) - (3)].node), atoi((yyvsp[(3) - (3)].sval)), -1, MAKE_LOC((yylsp[(1) - (3)])));
        (yyvsp[(1) - (3)].node) = NULL;
        free((yyvsp[(3) - (3)].sval)); (yyvsp[(3) - (3)].sval) = NULL;
    ;}
    break;

  case 222:
#line 1263 "compiler/parser/tbol.y"
    {
        /* target::embedded_length - length-prefixed field (1 or 2 byte prefix) */
        (yyval.node) = ast_format_spec((yyvsp[(1) - (4)].node), -1, atoi((yyvsp[(4) - (4)].sval)), MAKE_LOC((yylsp[(1) - (4)])));
        (yyvsp[(1) - (4)].node) = NULL;
        free((yyvsp[(4) - (4)].sval)); (yyvsp[(4) - (4)].sval) = NULL;
    ;}
    break;

  case 223:
#line 1269 "compiler/parser/tbol.y"
    {
        /* target:fixed_length:embedded_length - length-prefixed with padding/truncation */
        (yyval.node) = ast_format_spec((yyvsp[(1) - (5)].node), atoi((yyvsp[(3) - (5)].sval)), atoi((yyvsp[(5) - (5)].sval)), MAKE_LOC((yylsp[(1) - (5)])));
        (yyvsp[(1) - (5)].node) = NULL;
        free((yyvsp[(3) - (5)].sval)); (yyvsp[(3) - (5)].sval) = NULL;
        free((yyvsp[(5) - (5)].sval)); (yyvsp[(5) - (5)].sval) = NULL;
    ;}
    break;

  case 224:
#line 1280 "compiler/parser/tbol.y"
    {
        (yyval.node) = ast_proc_call((yyvsp[(1) - (1)].ident_pair).canonical, MAKE_LOC((yylsp[(1) - (1)])));
        ast_set_end((yyval.node), MAKE_END_LOC((yylsp[(1) - (1)])));
        free((yyvsp[(1) - (1)].ident_pair).canonical); free((yyvsp[(1) - (1)].ident_pair).original);
    ;}
    break;

  case 225:
#line 1285 "compiler/parser/tbol.y"
    {
        (yyval.node) = ast_proc_call((yyvsp[(1) - (2)].ident_pair).canonical, MAKE_LOC((yylsp[(1) - (2)])));
        free((yyvsp[(1) - (2)].ident_pair).canonical); free((yyvsp[(1) - (2)].ident_pair).original);
        if ((yyvsp[(2) - (2)].node)) {
            for (int i = 0; i < (yyvsp[(2) - (2)].node)->child_count; i++) {
                ast_add_child((yyval.node), (yyvsp[(2) - (2)].node)->children[i]);
            }
            (yyvsp[(2) - (2)].node)->child_count = 0;
            ast_free((yyvsp[(2) - (2)].node)); (yyvsp[(2) - (2)].node) = NULL;
        }
        ast_set_end((yyval.node), MAKE_END_LOC((yylsp[(2) - (2)])));
    ;}
    break;

  case 226:
#line 1301 "compiler/parser/tbol.y"
    {
        (yyval.node) = ast_ident((yyvsp[(1) - (1)].ident_pair).canonical, MAKE_LOC((yylsp[(1) - (1)])));
        ast_set_end((yyval.node), MAKE_END_LOC((yylsp[(1) - (1)])));
        free((yyvsp[(1) - (1)].ident_pair).canonical); free((yyvsp[(1) - (1)].ident_pair).original);
    ;}
    break;

  case 227:
#line 1306 "compiler/parser/tbol.y"
    { (yyval.node) = ast_reg_i((yyvsp[(1) - (1)].ival), MAKE_LOC((yylsp[(1) - (1)]))); ast_set_end((yyval.node), MAKE_END_LOC((yylsp[(1) - (1)]))); ;}
    break;

  case 228:
#line 1307 "compiler/parser/tbol.y"
    { (yyval.node) = ast_reg_d((yyvsp[(1) - (1)].ival), MAKE_LOC((yylsp[(1) - (1)]))); ast_set_end((yyval.node), MAKE_END_LOC((yylsp[(1) - (1)]))); ;}
    break;

  case 229:
#line 1308 "compiler/parser/tbol.y"
    { (yyval.node) = ast_reg_p((yyvsp[(1) - (1)].ival), MAKE_LOC((yylsp[(1) - (1)]))); ast_set_end((yyval.node), MAKE_END_LOC((yylsp[(1) - (1)]))); ;}
    break;

  case 230:
#line 1309 "compiler/parser/tbol.y"
    { (yyval.node) = ast_rda_slot((yyvsp[(1) - (1)].ival), MAKE_LOC((yylsp[(1) - (1)]))); ast_set_end((yyval.node), MAKE_END_LOC((yylsp[(1) - (1)]))); ;}
    break;

  case 231:
#line 1310 "compiler/parser/tbol.y"
    { (yyval.node) = ast_pev((yyvsp[(1) - (1)].ival), MAKE_LOC((yylsp[(1) - (1)]))); ast_set_end((yyval.node), MAKE_END_LOC((yylsp[(1) - (1)]))); ;}
    break;

  case 232:
#line 1311 "compiler/parser/tbol.y"
    { (yyval.node) = ast_gev((yyvsp[(1) - (1)].ival), MAKE_LOC((yylsp[(1) - (1)]))); ast_set_end((yyval.node), MAKE_END_LOC((yylsp[(1) - (1)]))); ;}
    break;

  case 233:
#line 1315 "compiler/parser/tbol.y"
    {
        (yyval.node) = ast_literal_str((yyvsp[(1) - (1)].sval), lexer_get_last_string_length(), MAKE_LOC((yylsp[(1) - (1)])));
        ast_set_end((yyval.node), MAKE_END_LOC((yylsp[(1) - (1)])));
        free((yyvsp[(1) - (1)].sval)); (yyvsp[(1) - (1)].sval) = NULL;
    ;}
    break;

  case 234:
#line 1320 "compiler/parser/tbol.y"
    {
        (yyval.node) = ast_literal_num(atoll((yyvsp[(1) - (1)].sval)), (yyvsp[(1) - (1)].sval), MAKE_LOC((yylsp[(1) - (1)])));
        ast_set_end((yyval.node), MAKE_END_LOC((yylsp[(1) - (1)])));
        free((yyvsp[(1) - (1)].sval)); (yyvsp[(1) - (1)].sval) = NULL;
    ;}
    break;

  case 235:
#line 1325 "compiler/parser/tbol.y"
    {
        (yyval.node) = ast_literal_hex(strtoll((yyvsp[(1) - (1)].sval) + 2, NULL, 16), (yyvsp[(1) - (1)].sval), MAKE_LOC((yylsp[(1) - (1)])));
        ast_set_end((yyval.node), MAKE_END_LOC((yylsp[(1) - (1)])));
        free((yyvsp[(1) - (1)].sval)); (yyvsp[(1) - (1)].sval) = NULL;
    ;}
    break;

  case 236:
#line 1330 "compiler/parser/tbol.y"
    {
        (yyval.node) = (yyvsp[(1) - (1)].node); (yyvsp[(1) - (1)].node) = NULL;
    ;}
    break;

  case 237:
#line 1333 "compiler/parser/tbol.y"
    {
        (yyval.node) = ast_indexed((yyvsp[(1) - (4)].node), (yyvsp[(3) - (4)].node), MAKE_LOC((yylsp[(1) - (4)])));
        (yyvsp[(1) - (4)].node) = NULL; (yyvsp[(3) - (4)].node) = NULL;
        ast_set_end((yyval.node), MAKE_END_LOC((yylsp[(4) - (4)])));
    ;}
    break;

  case 238:
#line 1342 "compiler/parser/tbol.y"
    {
        (yyval.send_mods) = (yyvsp[(1) - (1)].send_mods);
    ;}
    break;

  case 239:
#line 1345 "compiler/parser/tbol.y"
    {
        (yyval.send_mods).timeout = ((yyvsp[(1) - (3)].send_mods).timeout != 0) ? (yyvsp[(1) - (3)].send_mods).timeout : (yyvsp[(3) - (3)].send_mods).timeout;
        (yyval.send_mods).flags = (yyvsp[(1) - (3)].send_mods).flags | (yyvsp[(3) - (3)].send_mods).flags;
    ;}
    break;

  case 240:
#line 1352 "compiler/parser/tbol.y"
    {
        (yyval.send_mods).timeout = (int16_t)atoi((yyvsp[(3) - (4)].sval));
        (yyval.send_mods).flags = 0;
        free((yyvsp[(3) - (4)].sval)); (yyvsp[(3) - (4)].sval) = NULL;
    ;}
    break;

  case 241:
#line 1357 "compiler/parser/tbol.y"
    {
        (yyval.send_mods).timeout = 0;
        (yyval.send_mods).flags = 0x04;
    ;}
    break;

  case 242:
#line 1361 "compiler/parser/tbol.y"
    {
        (yyval.send_mods).timeout = 0;
        (yyval.send_mods).flags = 0x02;
    ;}
    break;


/* Line 1267 of yacc.c.  */
#line 4361 "compiler/parser/tbol.tab.c"
      default: break;
    }
  YY_SYMBOL_PRINT ("-> $$ =", yyr1[yyn], &yyval, &yyloc);

  YYPOPSTACK (yylen);
  yylen = 0;
  YY_STACK_PRINT (yyss, yyssp);

  *++yyvsp = yyval;
  *++yylsp = yyloc;

  /* Now `shift' the result of the reduction.  Determine what state
     that goes to, based on the state we popped back to and the rule
     number reduced by.  */

  yyn = yyr1[yyn];

  yystate = yypgoto[yyn - YYNTOKENS] + *yyssp;
  if (0 <= yystate && yystate <= YYLAST && yycheck[yystate] == *yyssp)
    yystate = yytable[yystate];
  else
    yystate = yydefgoto[yyn - YYNTOKENS];

  goto yynewstate;


/*------------------------------------.
| yyerrlab -- here on detecting error |
`------------------------------------*/
yyerrlab:
  /* If not already recovering from an error, report this error.  */
  if (!yyerrstatus)
    {
      ++yynerrs;
#if ! YYERROR_VERBOSE
      yyerror (YY_("syntax error"));
#else
      {
	YYSIZE_T yysize = yysyntax_error (0, yystate, yychar);
	if (yymsg_alloc < yysize && yymsg_alloc < YYSTACK_ALLOC_MAXIMUM)
	  {
	    YYSIZE_T yyalloc = 2 * yysize;
	    if (! (yysize <= yyalloc && yyalloc <= YYSTACK_ALLOC_MAXIMUM))
	      yyalloc = YYSTACK_ALLOC_MAXIMUM;
	    if (yymsg != yymsgbuf)
	      YYSTACK_FREE (yymsg);
	    yymsg = (char *) YYSTACK_ALLOC (yyalloc);
	    if (yymsg)
	      yymsg_alloc = yyalloc;
	    else
	      {
		yymsg = yymsgbuf;
		yymsg_alloc = sizeof yymsgbuf;
	      }
	  }

	if (0 < yysize && yysize <= yymsg_alloc)
	  {
	    (void) yysyntax_error (yymsg, yystate, yychar);
	    yyerror (yymsg);
	  }
	else
	  {
	    yyerror (YY_("syntax error"));
	    if (yysize != 0)
	      goto yyexhaustedlab;
	  }
      }
#endif
    }

  yyerror_range[0] = yylloc;

  if (yyerrstatus == 3)
    {
      /* If just tried and failed to reuse look-ahead token after an
	 error, discard it.  */

      if (yychar <= YYEOF)
	{
	  /* Return failure if at end of input.  */
	  if (yychar == YYEOF)
	    YYABORT;
	}
      else
	{
	  yydestruct ("Error: discarding",
		      yytoken, &yylval, &yylloc);
	  yychar = YYEMPTY;
	}
    }

  /* Else will try to reuse look-ahead token after shifting the error
     token.  */
  goto yyerrlab1;


/*---------------------------------------------------.
| yyerrorlab -- error raised explicitly by YYERROR.  |
`---------------------------------------------------*/
yyerrorlab:

  /* Pacify compilers like GCC when the user code never invokes
     YYERROR and the label yyerrorlab therefore never appears in user
     code.  */
  if (/*CONSTCOND*/ 0)
     goto yyerrorlab;

  yyerror_range[0] = yylsp[1-yylen];
  /* Do not reclaim the symbols of the rule which action triggered
     this YYERROR.  */
  YYPOPSTACK (yylen);
  yylen = 0;
  YY_STACK_PRINT (yyss, yyssp);
  yystate = *yyssp;
  goto yyerrlab1;


/*-------------------------------------------------------------.
| yyerrlab1 -- common code for both syntax error and YYERROR.  |
`-------------------------------------------------------------*/
yyerrlab1:
  yyerrstatus = 3;	/* Each real token shifted decrements this.  */

  for (;;)
    {
      yyn = yypact[yystate];
      if (yyn != YYPACT_NINF)
	{
	  yyn += YYTERROR;
	  if (0 <= yyn && yyn <= YYLAST && yycheck[yyn] == YYTERROR)
	    {
	      yyn = yytable[yyn];
	      if (0 < yyn)
		break;
	    }
	}

      /* Pop the current state because it cannot handle the error token.  */
      if (yyssp == yyss)
	YYABORT;

      yyerror_range[0] = *yylsp;
      yydestruct ("Error: popping",
		  yystos[yystate], yyvsp, yylsp);
      YYPOPSTACK (1);
      yystate = *yyssp;
      YY_STACK_PRINT (yyss, yyssp);
    }

  if (yyn == YYFINAL)
    YYACCEPT;

  *++yyvsp = yylval;

  yyerror_range[1] = yylloc;
  /* Using YYLLOC is tempting, but would change the location of
     the look-ahead.  YYLOC is available though.  */
  YYLLOC_DEFAULT (yyloc, (yyerror_range - 1), 2);
  *++yylsp = yyloc;

  /* Shift the error token.  */
  YY_SYMBOL_PRINT ("Shifting", yystos[yyn], yyvsp, yylsp);

  yystate = yyn;
  goto yynewstate;


/*-------------------------------------.
| yyacceptlab -- YYACCEPT comes here.  |
`-------------------------------------*/
yyacceptlab:
  yyresult = 0;
  goto yyreturn;

/*-----------------------------------.
| yyabortlab -- YYABORT comes here.  |
`-----------------------------------*/
yyabortlab:
  yyresult = 1;
  goto yyreturn;

#ifndef yyoverflow
/*-------------------------------------------------.
| yyexhaustedlab -- memory exhaustion comes here.  |
`-------------------------------------------------*/
yyexhaustedlab:
  yyerror (YY_("memory exhausted"));
  yyresult = 2;
  /* Fall through.  */
#endif

yyreturn:
  if (yychar != YYEOF && yychar != YYEMPTY)
     yydestruct ("Cleanup: discarding lookahead",
		 yytoken, &yylval, &yylloc);
  /* Do not reclaim the symbols of the rule which action triggered
     this YYABORT or YYACCEPT.  */
  YYPOPSTACK (yylen);
  YY_STACK_PRINT (yyss, yyssp);
  while (yyssp != yyss)
    {
      yydestruct ("Cleanup: popping",
		  yystos[*yyssp], yyvsp, yylsp);
      YYPOPSTACK (1);
    }
#ifndef yyoverflow
  if (yyss != yyssa)
    YYSTACK_FREE (yyss);
#endif
#if YYERROR_VERBOSE
  if (yymsg != yymsgbuf)
    YYSTACK_FREE (yymsg);
#endif
  /* Make sure YYID is used.  */
  return YYID (yyresult);
}


#line 1367 "compiler/parser/tbol.y"


void yyerror(const char *s) {
    SourceLoc loc = {lexer_get_filename(), yylloc.first_line, yylloc.first_column};
    diag_error(loc, "%s", s);
}

