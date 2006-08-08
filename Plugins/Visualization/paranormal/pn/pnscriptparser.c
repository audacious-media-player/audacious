/* A Bison parser, made by GNU Bison 2.1.  */

/* Skeleton parser for Yacc-like parsing with Bison,
   Copyright (C) 1984, 1989, 1990, 2000, 2001, 2002, 2003, 2004, 2005 Free Software Foundation, Inc.

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

/* As a special exception, when this file is copied by Bison into a
   Bison output file, you may use that output file without restriction.
   This special exception was added by the Free Software Foundation
   in version 1.24 of Bison.  */

/* Written by Richard Stallman by simplifying the original so called
   ``semantic'' parser.  */

/* All symbols defined below should begin with yy or YY, to avoid
   infringing on user name space.  This should be done even for local
   variables, as they might otherwise be expanded by user macros.
   There are some unavoidable exceptions within include files to
   define necessary library symbols; they are noted "INFRINGES ON
   USER NAME SPACE" below.  */

/* Identify Bison output.  */
#define YYBISON 1

/* Bison version.  */
#define YYBISON_VERSION "2.1"

/* Skeleton name.  */
#define YYSKELETON_NAME "yacc.c"

/* Pure parsers.  */
#define YYPURE 0

/* Using locations.  */
#define YYLSP_NEEDED 0



/* Tokens.  */
#ifndef YYTOKENTYPE
# define YYTOKENTYPE
   /* Put the tokens into the symbol table, so that GDB and other debuggers
      know about them.  */
   enum yytokentype {
     CONSTANT = 258,
     VARIABLE = 259,
     ABS_FUNC = 260,
     MAX_FUNC = 261,
     MIN_FUNC = 262,
     SIN_FUNC = 263,
     COS_FUNC = 264,
     TAN_FUNC = 265,
     ASIN_FUNC = 266,
     ACOS_FUNC = 267,
     ATAN_FUNC = 268,
     NEG = 269
   };
#endif
/* Tokens.  */
#define CONSTANT 258
#define VARIABLE 259
#define ABS_FUNC 260
#define MAX_FUNC 261
#define MIN_FUNC 262
#define SIN_FUNC 263
#define COS_FUNC 264
#define TAN_FUNC 265
#define ASIN_FUNC 266
#define ACOS_FUNC 267
#define ATAN_FUNC 268
#define NEG 269




/* Copy the first part of user declarations.  */
#line 1 "pnscriptparser.y"

#define    yymaxdepth pn_script_parser_maxdepth
#define    yyparse    pn_script_parser_parse
#define    yylex      pn_script_parser_lex
#define    yyerror    pn_script_parser_error
#define    yylval     pn_script_parser_lval
#define    yychar     pn_script_parser_char
#define    yydebug    pn_script_parser_debug
#define    yypact     pn_script_parser_pact
#define    yyr1       pn_script_parser_r1
#define    yyr2       pn_script_parser_r2
#define    yydef      pn_script_parser_def
#define    yychk      pn_script_parser_chk
#define    yypgo      pn_script_parser_pgo
#define    yyact      pn_script_parser_act
#define    yyexca     pn_script_parser_exca
#define    yyerrflag  pn_script_parser_errflag
#define    ynerrs     pn_script_parser_nerrs
#define    yyps       pn_script_parser_ps
#define    yypv       pn_script_parser_pv
#define    yys        pn_script_parser_s
#define    yy_yys     pn_script_parser_yys
#define    yystate    pn_script_parser_state
#define    yytmp      pn_script_parser_tmp
#define    yyv        pn_script_parser_v
#define    yy_yyv     pn_script_parser_yyv
#define    yyval      pn_script_parser_val
#define    yylloc     pn_script_parser_lloc
#define    yyreds     pn_script_parser_reds
#define    yytoks     pn_script_parser_toks
#define    yylhs      pn_script_parser_yylhs
#define    yylen      pn_script_parser_yylen
#define    yydefred   pn_script_parser_yydefred
#define    yydgoto    pn_script_parser_yydgoto
#define    yysindex   pn_script_parser_yysindex
#define    yyrindex   pn_script_parser_yyrindex
#define    yygindex   pn_script_parser_yygindex
#define    yytable    pn_script_parser_yytable
#define    yycheck    pn_script_parser_yycheck
#define    yyname     pn_script_parser_yyname
#define    yyrule     pn_script_parser_yyrule

#include <ctype.h>
#include <stdlib.h>
#include <glib.h>
#include <pn/pnscript.h>

/* define this to dump the parser output to stdout */
/*  #define PN_PRINT_OPS 1 */

int yyerror (char *s);
int yylex (void); 

static gboolean parse_failed;

/* Are we on the size-determining pass? */
static gboolean size_pass;

/* Used during the size pass to determine the size of the constant table */
static GArray *temp_constant_table = NULL; 

/* The code size */
static guint code_size;

/* The current code byte */
static guint *code_ptr;
 
/* Input variables used during parsing  */ 
static PnScript *parse_script;
static const gchar *parse_string;


/* Enabling traces.  */
#ifndef YYDEBUG
# define YYDEBUG 0
#endif

/* Enabling verbose error messages.  */
#ifdef YYERROR_VERBOSE
# undef YYERROR_VERBOSE
# define YYERROR_VERBOSE 1
#else
# define YYERROR_VERBOSE 0
#endif

/* Enabling the token table.  */
#ifndef YYTOKEN_TABLE
# define YYTOKEN_TABLE 0
#endif

#if ! defined (YYSTYPE) && ! defined (YYSTYPE_IS_DECLARED)
#line 73 "pnscriptparser.y"
typedef union YYSTYPE {
  gdouble     *constant;
  PnVariable *variable;
} YYSTYPE;
/* Line 196 of yacc.c.  */
#line 190 "pnscriptparser.tab.c"
# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
# define YYSTYPE_IS_TRIVIAL 1
#endif



/* Copy the second part of user declarations.  */


/* Line 219 of yacc.c.  */
#line 202 "pnscriptparser.tab.c"

#if ! defined (YYSIZE_T) && defined (__SIZE_TYPE__)
# define YYSIZE_T __SIZE_TYPE__
#endif
#if ! defined (YYSIZE_T) && defined (size_t)
# define YYSIZE_T size_t
#endif
#if ! defined (YYSIZE_T) && (defined (__STDC__) || defined (__cplusplus))
# include <stddef.h> /* INFRINGES ON USER NAME SPACE */
# define YYSIZE_T size_t
#endif
#if ! defined (YYSIZE_T)
# define YYSIZE_T unsigned int
#endif

#ifndef YY_
# if YYENABLE_NLS
#  if ENABLE_NLS
#   include <libintl.h> /* INFRINGES ON USER NAME SPACE */
#   define YY_(msgid) dgettext ("bison-runtime", msgid)
#  endif
# endif
# ifndef YY_
#  define YY_(msgid) msgid
# endif
#endif

#if ! defined (yyoverflow) || YYERROR_VERBOSE

/* The parser invokes alloca or malloc; define the necessary symbols.  */

# ifdef YYSTACK_USE_ALLOCA
#  if YYSTACK_USE_ALLOCA
#   ifdef __GNUC__
#    define YYSTACK_ALLOC __builtin_alloca
#   else
#    define YYSTACK_ALLOC alloca
#    if defined (__STDC__) || defined (__cplusplus)
#     include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
#     define YYINCLUDED_STDLIB_H
#    endif
#   endif
#  endif
# endif

# ifdef YYSTACK_ALLOC
   /* Pacify GCC's `empty if-body' warning. */
#  define YYSTACK_FREE(Ptr) do { /* empty */; } while (0)
#  ifndef YYSTACK_ALLOC_MAXIMUM
    /* The OS might guarantee only one guard page at the bottom of the stack,
       and a page size can be as small as 4096 bytes.  So we cannot safely
       invoke alloca (N) if N exceeds 4096.  Use a slightly smaller number
       to allow for a few compiler-allocated temporary stack slots.  */
#   define YYSTACK_ALLOC_MAXIMUM 4032 /* reasonable circa 2005 */
#  endif
# else
#  define YYSTACK_ALLOC YYMALLOC
#  define YYSTACK_FREE YYFREE
#  ifndef YYSTACK_ALLOC_MAXIMUM
#   define YYSTACK_ALLOC_MAXIMUM ((YYSIZE_T) -1)
#  endif
#  ifdef __cplusplus
extern "C" {
#  endif
#  ifndef YYMALLOC
#   define YYMALLOC malloc
#   if (! defined (malloc) && ! defined (YYINCLUDED_STDLIB_H) \
	&& (defined (__STDC__) || defined (__cplusplus)))
void *malloc (YYSIZE_T); /* INFRINGES ON USER NAME SPACE */
#   endif
#  endif
#  ifndef YYFREE
#   define YYFREE free
#   if (! defined (free) && ! defined (YYINCLUDED_STDLIB_H) \
	&& (defined (__STDC__) || defined (__cplusplus)))
void free (void *); /* INFRINGES ON USER NAME SPACE */
#   endif
#  endif
#  ifdef __cplusplus
}
#  endif
# endif
#endif /* ! defined (yyoverflow) || YYERROR_VERBOSE */


#if (! defined (yyoverflow) \
     && (! defined (__cplusplus) \
	 || (defined (YYSTYPE_IS_TRIVIAL) && YYSTYPE_IS_TRIVIAL)))

/* A type that is properly aligned for any stack member.  */
union yyalloc
{
  short int yyss;
  YYSTYPE yyvs;
  };

/* The size of the maximum gap between one aligned stack and the next.  */
# define YYSTACK_GAP_MAXIMUM (sizeof (union yyalloc) - 1)

/* The size of an array large to enough to hold all stacks, each with
   N elements.  */
# define YYSTACK_BYTES(N) \
     ((N) * (sizeof (short int) + sizeof (YYSTYPE))			\
      + YYSTACK_GAP_MAXIMUM)

/* Copy COUNT objects from FROM to TO.  The source and destination do
   not overlap.  */
# ifndef YYCOPY
#  if defined (__GNUC__) && 1 < __GNUC__
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
      while (0)
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
    while (0)

#endif

#if defined (__STDC__) || defined (__cplusplus)
   typedef signed char yysigned_char;
#else
   typedef short int yysigned_char;
#endif

/* YYFINAL -- State number of the termination state. */
#define YYFINAL  2
/* YYLAST -- Last index in YYTABLE.  */
#define YYLAST   159

/* YYNTOKENS -- Number of terminals. */
#define YYNTOKENS  25
/* YYNNTS -- Number of nonterminals. */
#define YYNNTS  5
/* YYNRULES -- Number of rules. */
#define YYNRULES  24
/* YYNRULES -- Number of states. */
#define YYNSTATES  67

/* YYTRANSLATE(YYLEX) -- Bison symbol number corresponding to YYLEX.  */
#define YYUNDEFTOK  2
#define YYMAXUTOK   269

#define YYTRANSLATE(YYX)						\
  ((unsigned int) (YYX) <= YYMAXUTOK ? yytranslate[YYX] : YYUNDEFTOK)

/* YYTRANSLATE[YYLEX] -- Bison symbol number corresponding to YYLEX.  */
static const unsigned char yytranslate[] =
{
       0,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
      22,    23,    17,    16,    24,    15,     2,    18,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,    21,
       2,    14,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,    20,     2,     2,     2,     2,     2,
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
       5,     6,     7,     8,     9,    10,    11,    12,    13,    19
};

#if YYDEBUG
/* YYPRHS[YYN] -- Index of the first RHS symbol of rule number YYN in
   YYRHS.  */
static const unsigned char yyprhs[] =
{
       0,     0,     3,     4,     7,    10,    14,    16,    18,    20,
      25,    32,    39,    44,    49,    54,    59,    64,    69,    73,
      77,    81,    85,    88,    92
};

/* YYRHS -- A `-1'-separated list of the rules' RHS. */
static const yysigned_char yyrhs[] =
{
      26,     0,    -1,    -1,    26,    27,    -1,    28,    21,    -1,
       4,    14,    29,    -1,     3,    -1,     4,    -1,    28,    -1,
       5,    22,    29,    23,    -1,     6,    22,    29,    24,    29,
      23,    -1,     7,    22,    29,    24,    29,    23,    -1,     8,
      22,    29,    23,    -1,     9,    22,    29,    23,    -1,    10,
      22,    29,    23,    -1,    11,    22,    29,    23,    -1,    12,
      22,    29,    23,    -1,    13,    22,    29,    23,    -1,    29,
      16,    29,    -1,    29,    15,    29,    -1,    29,    17,    29,
      -1,    29,    18,    29,    -1,    15,    29,    -1,    29,    20,
      29,    -1,    22,    29,    23,    -1
};

/* YYRLINE[YYN] -- source line where rule number YYN was defined.  */
static const unsigned short int yyrline[] =
{
       0,   100,   100,   102,   106,   122,   138,   152,   166,   167,
     179,   191,   205,   219,   233,   247,   261,   275,   289,   302,
     314,   327,   340,   353,   366
};
#endif

#if YYDEBUG || YYERROR_VERBOSE || YYTOKEN_TABLE
/* YYTNAME[SYMBOL-NUM] -- String name of the symbol SYMBOL-NUM.
   First, the terminals, then, starting at YYNTOKENS, nonterminals. */
static const char *const yytname[] =
{
  "$end", "error", "$undefined", "CONSTANT", "VARIABLE", "ABS_FUNC",
  "MAX_FUNC", "MIN_FUNC", "SIN_FUNC", "COS_FUNC", "TAN_FUNC", "ASIN_FUNC",
  "ACOS_FUNC", "ATAN_FUNC", "'='", "'-'", "'+'", "'*'", "'/'", "NEG",
  "'^'", "';'", "'('", "')'", "','", "$accept", "script", "statement",
  "equation", "expression", 0
};
#endif

# ifdef YYPRINT
/* YYTOKNUM[YYLEX-NUM] -- Internal token number corresponding to
   token YYLEX-NUM.  */
static const unsigned short int yytoknum[] =
{
       0,   256,   257,   258,   259,   260,   261,   262,   263,   264,
     265,   266,   267,   268,    61,    45,    43,    42,    47,   269,
      94,    59,    40,    41,    44
};
# endif

/* YYR1[YYN] -- Symbol number of symbol that rule YYN derives.  */
static const unsigned char yyr1[] =
{
       0,    25,    26,    26,    27,    28,    29,    29,    29,    29,
      29,    29,    29,    29,    29,    29,    29,    29,    29,    29,
      29,    29,    29,    29,    29
};

/* YYR2[YYN] -- Number of symbols composing right hand side of rule YYN.  */
static const unsigned char yyr2[] =
{
       0,     2,     0,     2,     2,     3,     1,     1,     1,     4,
       6,     6,     4,     4,     4,     4,     4,     4,     3,     3,
       3,     3,     2,     3,     3
};

/* YYDEFACT[STATE-NAME] -- Default rule to reduce with in state
   STATE-NUM when YYTABLE doesn't specify something else to do.  Zero
   means the default is an error.  */
static const unsigned char yydefact[] =
{
       2,     0,     1,     0,     3,     0,     0,     4,     6,     7,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     8,     5,     0,     0,     0,     0,     0,     0,     0,
       0,     0,    22,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,    24,    19,
      18,    20,    21,    23,     9,     0,     0,    12,    13,    14,
      15,    16,    17,     0,     0,    10,    11
};

/* YYDEFGOTO[NTERM-NUM]. */
static const yysigned_char yydefgoto[] =
{
      -1,     1,     4,    21,    22
};

/* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
   STATE-NUM.  */
#define YYPACT_NINF -20
static const short int yypact[] =
{
     -20,    31,   -20,   -12,   -20,   -18,    17,   -20,   -20,   -12,
      -9,    -8,    11,    12,    16,    22,    24,    26,    32,    17,
      17,   -20,   135,    17,    17,    17,    17,    17,    17,    17,
      17,    17,    27,    45,    17,    17,    17,    17,    17,    54,
      25,    35,    63,    72,    81,    90,    99,   108,   -20,   139,
     139,    27,    27,    27,   -20,    17,    17,   -20,   -20,   -20,
     -20,   -20,   -20,   117,   126,   -20,   -20
};

/* YYPGOTO[NTERM-NUM].  */
static const yysigned_char yypgoto[] =
{
     -20,   -20,   -20,    55,   -19
};

/* YYTABLE[YYPACT[STATE-NUM]].  What to do in state STATE-NUM.  If
   positive, shift that token.  If negative, reduce the rule which
   number is the opposite.  If zero, do what YYDEFACT says.
   If YYTABLE_NINF, syntax error.  */
#define YYTABLE_NINF -1
static const unsigned char yytable[] =
{
      32,    33,     6,     7,    39,    40,    41,    42,    43,    44,
      45,    46,    47,    23,    24,    49,    50,    51,    52,    53,
       8,     9,    10,    11,    12,    13,    14,    15,    16,    17,
      18,     2,    19,    25,    26,     3,    63,    64,    27,    20,
      34,    35,    36,    37,    28,    38,    29,    38,    30,    55,
      34,    35,    36,    37,    31,    38,     5,     0,     0,    56,
      34,    35,    36,    37,     0,    38,     0,     0,    48,    34,
      35,    36,    37,     0,    38,     0,     0,    54,    34,    35,
      36,    37,     0,    38,     0,     0,    57,    34,    35,    36,
      37,     0,    38,     0,     0,    58,    34,    35,    36,    37,
       0,    38,     0,     0,    59,    34,    35,    36,    37,     0,
      38,     0,     0,    60,    34,    35,    36,    37,     0,    38,
       0,     0,    61,    34,    35,    36,    37,     0,    38,     0,
       0,    62,    34,    35,    36,    37,     0,    38,     0,     0,
      65,    34,    35,    36,    37,     0,    38,     0,     0,    66,
      34,    35,    36,    37,     0,    38,    36,    37,     0,    38
};

static const yysigned_char yycheck[] =
{
      19,    20,    14,    21,    23,    24,    25,    26,    27,    28,
      29,    30,    31,    22,    22,    34,    35,    36,    37,    38,
       3,     4,     5,     6,     7,     8,     9,    10,    11,    12,
      13,     0,    15,    22,    22,     4,    55,    56,    22,    22,
      15,    16,    17,    18,    22,    20,    22,    20,    22,    24,
      15,    16,    17,    18,    22,    20,     1,    -1,    -1,    24,
      15,    16,    17,    18,    -1,    20,    -1,    -1,    23,    15,
      16,    17,    18,    -1,    20,    -1,    -1,    23,    15,    16,
      17,    18,    -1,    20,    -1,    -1,    23,    15,    16,    17,
      18,    -1,    20,    -1,    -1,    23,    15,    16,    17,    18,
      -1,    20,    -1,    -1,    23,    15,    16,    17,    18,    -1,
      20,    -1,    -1,    23,    15,    16,    17,    18,    -1,    20,
      -1,    -1,    23,    15,    16,    17,    18,    -1,    20,    -1,
      -1,    23,    15,    16,    17,    18,    -1,    20,    -1,    -1,
      23,    15,    16,    17,    18,    -1,    20,    -1,    -1,    23,
      15,    16,    17,    18,    -1,    20,    17,    18,    -1,    20
};

/* YYSTOS[STATE-NUM] -- The (internal number of the) accessing
   symbol of state STATE-NUM.  */
static const unsigned char yystos[] =
{
       0,    26,     0,     4,    27,    28,    14,    21,     3,     4,
       5,     6,     7,     8,     9,    10,    11,    12,    13,    15,
      22,    28,    29,    22,    22,    22,    22,    22,    22,    22,
      22,    22,    29,    29,    15,    16,    17,    18,    20,    29,
      29,    29,    29,    29,    29,    29,    29,    29,    23,    29,
      29,    29,    29,    29,    23,    24,    24,    23,    23,    23,
      23,    23,    23,    29,    29,    23,    23
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
      YYPOPSTACK;						\
      goto yybackup;						\
    }								\
  else								\
    {								\
      yyerror (YY_("syntax error: cannot back up")); \
      YYERROR;							\
    }								\
while (0)


#define YYTERROR	1
#define YYERRCODE	256


/* YYLLOC_DEFAULT -- Set CURRENT to span from RHS[1] to RHS[N].
   If N is 0, then set CURRENT to the empty location which ends
   the previous symbol: RHS[0] (always defined).  */

#define YYRHSLOC(Rhs, K) ((Rhs)[K])
#ifndef YYLLOC_DEFAULT
# define YYLLOC_DEFAULT(Current, Rhs, N)				\
    do									\
      if (N)								\
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
    while (0)
#endif


/* YY_LOCATION_PRINT -- Print the location on the stream.
   This macro was not mandated originally: define only if we know
   we won't break user code: when these are the locations we know.  */

#ifndef YY_LOCATION_PRINT
# if YYLTYPE_IS_TRIVIAL
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
} while (0)

# define YY_SYMBOL_PRINT(Title, Type, Value, Location)		\
do {								\
  if (yydebug)							\
    {								\
      YYFPRINTF (stderr, "%s ", Title);				\
      yysymprint (stderr,					\
                  Type, Value);	\
      YYFPRINTF (stderr, "\n");					\
    }								\
} while (0)

/*------------------------------------------------------------------.
| yy_stack_print -- Print the state stack from its BOTTOM up to its |
| TOP (included).                                                   |
`------------------------------------------------------------------*/

#if defined (__STDC__) || defined (__cplusplus)
static void
yy_stack_print (short int *bottom, short int *top)
#else
static void
yy_stack_print (bottom, top)
    short int *bottom;
    short int *top;
#endif
{
  YYFPRINTF (stderr, "Stack now");
  for (/* Nothing. */; bottom <= top; ++bottom)
    YYFPRINTF (stderr, " %d", *bottom);
  YYFPRINTF (stderr, "\n");
}

# define YY_STACK_PRINT(Bottom, Top)				\
do {								\
  if (yydebug)							\
    yy_stack_print ((Bottom), (Top));				\
} while (0)


/*------------------------------------------------.
| Report that the YYRULE is going to be reduced.  |
`------------------------------------------------*/

#if defined (__STDC__) || defined (__cplusplus)
static void
yy_reduce_print (int yyrule)
#else
static void
yy_reduce_print (yyrule)
    int yyrule;
#endif
{
  int yyi;
  unsigned long int yylno = yyrline[yyrule];
  YYFPRINTF (stderr, "Reducing stack by rule %d (line %lu), ",
             yyrule - 1, yylno);
  /* Print the symbols being reduced, and their result.  */
  for (yyi = yyprhs[yyrule]; 0 <= yyrhs[yyi]; yyi++)
    YYFPRINTF (stderr, "%s ", yytname[yyrhs[yyi]]);
  YYFPRINTF (stderr, "-> %s\n", yytname[yyr1[yyrule]]);
}

# define YY_REDUCE_PRINT(Rule)		\
do {					\
  if (yydebug)				\
    yy_reduce_print (Rule);		\
} while (0)

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
#  if defined (__GLIBC__) && defined (_STRING_H)
#   define yystrlen strlen
#  else
/* Return the length of YYSTR.  */
static YYSIZE_T
#   if defined (__STDC__) || defined (__cplusplus)
yystrlen (const char *yystr)
#   else
yystrlen (yystr)
     const char *yystr;
#   endif
{
  const char *yys = yystr;

  while (*yys++ != '\0')
    continue;

  return yys - yystr - 1;
}
#  endif
# endif

# ifndef yystpcpy
#  if defined (__GLIBC__) && defined (_STRING_H) && defined (_GNU_SOURCE)
#   define yystpcpy stpcpy
#  else
/* Copy YYSRC to YYDEST, returning the address of the terminating '\0' in
   YYDEST.  */
static char *
#   if defined (__STDC__) || defined (__cplusplus)
yystpcpy (char *yydest, const char *yysrc)
#   else
yystpcpy (yydest, yysrc)
     char *yydest;
     const char *yysrc;
#   endif
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
      size_t yyn = 0;
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

#endif /* YYERROR_VERBOSE */



#if YYDEBUG
/*--------------------------------.
| Print this symbol on YYOUTPUT.  |
`--------------------------------*/

#if defined (__STDC__) || defined (__cplusplus)
static void
yysymprint (FILE *yyoutput, int yytype, YYSTYPE *yyvaluep)
#else
static void
yysymprint (yyoutput, yytype, yyvaluep)
    FILE *yyoutput;
    int yytype;
    YYSTYPE *yyvaluep;
#endif
{
  /* Pacify ``unused variable'' warnings.  */
  (void) yyvaluep;

  if (yytype < YYNTOKENS)
    YYFPRINTF (yyoutput, "token %s (", yytname[yytype]);
  else
    YYFPRINTF (yyoutput, "nterm %s (", yytname[yytype]);


# ifdef YYPRINT
  if (yytype < YYNTOKENS)
    YYPRINT (yyoutput, yytoknum[yytype], *yyvaluep);
# endif
  switch (yytype)
    {
      default:
        break;
    }
  YYFPRINTF (yyoutput, ")");
}

#endif /* ! YYDEBUG */
/*-----------------------------------------------.
| Release the memory associated to this symbol.  |
`-----------------------------------------------*/

#if defined (__STDC__) || defined (__cplusplus)
static void
yydestruct (const char *yymsg, int yytype, YYSTYPE *yyvaluep)
#else
static void
yydestruct (yymsg, yytype, yyvaluep)
    const char *yymsg;
    int yytype;
    YYSTYPE *yyvaluep;
#endif
{
  /* Pacify ``unused variable'' warnings.  */
  (void) yyvaluep;

  if (!yymsg)
    yymsg = "Deleting";
  YY_SYMBOL_PRINT (yymsg, yytype, yyvaluep, yylocationp);

  switch (yytype)
    {

      default:
        break;
    }
}


/* Prevent warnings from -Wmissing-prototypes.  */

#ifdef YYPARSE_PARAM
# if defined (__STDC__) || defined (__cplusplus)
int yyparse (void *YYPARSE_PARAM);
# else
int yyparse ();
# endif
#else /* ! YYPARSE_PARAM */
#if defined (__STDC__) || defined (__cplusplus)
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



/*----------.
| yyparse.  |
`----------*/

#ifdef YYPARSE_PARAM
# if defined (__STDC__) || defined (__cplusplus)
int yyparse (void *YYPARSE_PARAM)
# else
int yyparse (YYPARSE_PARAM)
  void *YYPARSE_PARAM;
# endif
#else /* ! YYPARSE_PARAM */
#if defined (__STDC__) || defined (__cplusplus)
int
yyparse (void)
#else
int
yyparse ()
    ;
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

  /* Three stacks and their tools:
     `yyss': related to states,
     `yyvs': related to semantic values,
     `yyls': related to locations.

     Refer to the stacks thru separate pointers, to allow yyoverflow
     to reallocate them elsewhere.  */

  /* The state stack.  */
  short int yyssa[YYINITDEPTH];
  short int *yyss = yyssa;
  short int *yyssp;

  /* The semantic value stack.  */
  YYSTYPE yyvsa[YYINITDEPTH];
  YYSTYPE *yyvs = yyvsa;
  YYSTYPE *yyvsp;



#define YYPOPSTACK   (yyvsp--, yyssp--)

  YYSIZE_T yystacksize = YYINITDEPTH;

  /* The variables used to return semantic value and location from the
     action routines.  */
  YYSTYPE yyval;


  /* When reducing, the number of symbols on the RHS of the reduced
     rule.  */
  int yylen;

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

  goto yysetstate;

/*------------------------------------------------------------.
| yynewstate -- Push a new state, which is found in yystate.  |
`------------------------------------------------------------*/
 yynewstate:
  /* In all cases, when you get here, the value and location stacks
     have just been pushed. so pushing a state here evens the stacks.
     */
  yyssp++;

 yysetstate:
  *yyssp = yystate;

  if (yyss + yystacksize - 1 <= yyssp)
    {
      /* Get the current used size of the three stacks, in elements.  */
      YYSIZE_T yysize = yyssp - yyss + 1;

#ifdef yyoverflow
      {
	/* Give user a chance to reallocate the stack. Use copies of
	   these so that the &'s don't force the real ones into
	   memory.  */
	YYSTYPE *yyvs1 = yyvs;
	short int *yyss1 = yyss;


	/* Each stack pointer address is followed by the size of the
	   data in use in that stack, in bytes.  This used to be a
	   conditional around just the two extra args, but that might
	   be undefined if yyoverflow is a macro.  */
	yyoverflow (YY_("memory exhausted"),
		    &yyss1, yysize * sizeof (*yyssp),
		    &yyvs1, yysize * sizeof (*yyvsp),

		    &yystacksize);

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
	short int *yyss1 = yyss;
	union yyalloc *yyptr =
	  (union yyalloc *) YYSTACK_ALLOC (YYSTACK_BYTES (yystacksize));
	if (! yyptr)
	  goto yyexhaustedlab;
	YYSTACK_RELOCATE (yyss);
	YYSTACK_RELOCATE (yyvs);

#  undef YYSTACK_RELOCATE
	if (yyss1 != yyssa)
	  YYSTACK_FREE (yyss1);
      }
# endif
#endif /* no yyoverflow */

      yyssp = yyss + yysize - 1;
      yyvsp = yyvs + yysize - 1;


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

/* Do appropriate processing given the current state.  */
/* Read a look-ahead token if we need one and don't already have one.  */
/* yyresume: */

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

  /* Shift the look-ahead token.  */
  YY_SYMBOL_PRINT ("Shifting", yytoken, &yylval, &yylloc);

  /* Discard the token being shifted unless it is eof.  */
  if (yychar != YYEOF)
    yychar = YYEMPTY;

  *++yyvsp = yylval;


  /* Count tokens shifted since error; after three, turn off error
     status.  */
  if (yyerrstatus)
    yyerrstatus--;

  yystate = yyn;
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


  YY_REDUCE_PRINT (yyn);
  switch (yyn)
    {
        case 4:
#line 107 "pnscriptparser.y"
    {
		  if (size_pass)
		    code_size += sizeof (guint);
		  else
		    {
		      *code_ptr++ = PN_OP_POP;

#ifdef PN_PRINT_OPS
		      g_print ("POP\n");
#endif /* PN_PRINT_OPS */
		    }
		;}
    break;

  case 5:
#line 123 "pnscriptparser.y"
    {
		  if (size_pass)
		    code_size += sizeof (guint) + sizeof (gdouble *);
		  else
		    {
		      *code_ptr++ = PN_OP_SET;
		      *code_ptr++ = (guint) (yyvsp[-2].variable);

#ifdef PN_PRINT_OPS
		      g_print ("SET %s\n", (yyvsp[-2].variable)->name);
#endif /* PN_PRINT_OPS */
		    }
		;}
    break;

  case 6:
#line 139 "pnscriptparser.y"
    {
		  if (size_pass)
		    code_size += sizeof (guint) + sizeof (gdouble *);
		  else
		    {
		      *code_ptr++ = PN_OP_PUSHC;
		      *code_ptr++ = GPOINTER_TO_UINT ((yyvsp[0].constant));

#ifdef PN_PRINT_OPS
		      g_print ("PUSHC %f\n", *(yyvsp[0].constant));
#endif /* PN_PRINT_OPS */
		    }
		;}
    break;

  case 7:
#line 153 "pnscriptparser.y"
    {
		  if (size_pass)
		    code_size += sizeof (guint) + sizeof (gdouble *);
		  else
		    {
		      *code_ptr++ = PN_OP_PUSHV;
		      *code_ptr++ = (guint) (yyvsp[0].variable);

#ifdef PN_PRINT_OPS
		      g_print ("PUSHV %s\n", (yyvsp[0].variable)->name);
#endif /* PN_PRINT_OPS */
		    }
		;}
    break;

  case 9:
#line 168 "pnscriptparser.y"
    {
		  if (size_pass)
		    code_size += sizeof (guint);
		  else
		    {
		      *code_ptr++ = PN_OP_ABS;
#ifdef PN_PRINT_OPS
		      g_print ("ABS\n");
#endif /* PN_PRINT_OPS */
		    }
		;}
    break;

  case 10:
#line 180 "pnscriptparser.y"
    {
		  if (size_pass)
		    code_size += sizeof (guint);
		  else
		    {
		      *code_ptr++ = PN_OP_MAX;
#ifdef PN_PRINT_OPS
		      g_print ("MAX\n");
#endif /* PN_PRINT_OPS */
		    }
		;}
    break;

  case 11:
#line 192 "pnscriptparser.y"
    {
		  if (size_pass)
		    code_size += sizeof (guint);
		  else
		    {
		      *code_ptr++ = PN_OP_MIN;

#ifdef PN_PRINT_OPS
		      g_print ("MIN\n");
#endif /* PN_PRINT_OPS */
		    }

		;}
    break;

  case 12:
#line 206 "pnscriptparser.y"
    {
		  if (size_pass)
		    code_size += sizeof (guint);
		  else
		    {
		      *code_ptr++ = PN_OP_SIN;

#ifdef PN_PRINT_OPS
		      g_print ("SIN\n");
#endif /* PN_PRINT_OPS */
		    }

		;}
    break;

  case 13:
#line 220 "pnscriptparser.y"
    {
		  if (size_pass)
		    code_size += sizeof (guint);
		  else
		    {
		      *code_ptr++ = PN_OP_COS;

#ifdef PN_PRINT_OPS
		      g_print ("COS\n");
#endif /* PN_PRINT_OPS */
		    }

		;}
    break;

  case 14:
#line 234 "pnscriptparser.y"
    {
		  if (size_pass)
		    code_size += sizeof (guint);
		  else
		    {
		      *code_ptr++ = PN_OP_TAN;

#ifdef PN_PRINT_OPS
		      g_print ("TAN\n");
#endif /* PN_PRINT_OPS */
		    }

		;}
    break;

  case 15:
#line 248 "pnscriptparser.y"
    {
		  if (size_pass)
		    code_size += sizeof (guint);
		  else
		    {
		      *code_ptr++ = PN_OP_ASIN;

#ifdef PN_PRINT_OPS
		      g_print ("ASIN\n");
#endif /* PN_PRINT_OPS */
		    }

		;}
    break;

  case 16:
#line 262 "pnscriptparser.y"
    {
		  if (size_pass)
		    code_size += sizeof (guint);
		  else
		    {
		      *code_ptr++ = PN_OP_ACOS;

#ifdef PN_PRINT_OPS
		      g_print ("ACOS\n");
#endif /* PN_PRINT_OPS */
		    }

		;}
    break;

  case 17:
#line 276 "pnscriptparser.y"
    {
		  if (size_pass)
		    code_size += sizeof (guint);
		  else
		    {
		      *code_ptr++ = PN_OP_ATAN;

#ifdef PN_PRINT_OPS
		      g_print ("ATAN\n");
#endif /* PN_PRINT_OPS */
		    }

		;}
    break;

  case 18:
#line 290 "pnscriptparser.y"
    {
		  if (size_pass)
		    code_size += sizeof (guint);
		  else
		    {
		      *code_ptr++ = PN_OP_ADD;

#ifdef PN_PRINT_OPS
		      g_print ("ADD\n");
#endif /* PN_PRINT_OPS */
		    }
		;}
    break;

  case 19:
#line 303 "pnscriptparser.y"
    {
		  if (size_pass)
		    code_size += sizeof (guint);
		  else
		    {
		      *code_ptr++ = PN_OP_SUB;
#ifdef PN_PRINT_OPS
		      g_print ("SUB\n");
#endif /* PN_PRINT_OPS */
		    }
		;}
    break;

  case 20:
#line 315 "pnscriptparser.y"
    {
		  if (size_pass)
		    code_size += sizeof (guint);
		  else
		    {
		      *code_ptr++ = PN_OP_MUL;

#ifdef PN_PRINT_OPS
		      g_print ("MUL\n");
#endif /* PN_PRINT_OPS */
		    }		      
		;}
    break;

  case 21:
#line 328 "pnscriptparser.y"
    {
		  if (size_pass)
		    code_size += sizeof (guint);
		  else
		    {
		      *code_ptr++ = PN_OP_DIV;

#ifdef PN_PRINT_OPS
		      g_print ("DIV\n");
#endif /* PN_PRINT_OPS */
		    }
		;}
    break;

  case 22:
#line 341 "pnscriptparser.y"
    {
		  if (size_pass)
		    code_size += sizeof (guint);
		  else
		    {
		      *code_ptr++ = PN_OP_NEG;

#ifdef PN_PRINT_OPS
		      g_print ("NEG\n");
#endif /* PN_PRINT_OPS */
		    }
		;}
    break;

  case 23:
#line 354 "pnscriptparser.y"
    {
		  if (size_pass)
		    code_size += sizeof (guint);
		  else
		    {
		      *code_ptr++ = PN_OP_POW;

#ifdef PN_PRINT_OPS
		      g_print ("POW\n");
#endif /* PN_PRINT_OPS  */
		    }
		;}
    break;


      default: break;
    }

/* Line 1126 of yacc.c.  */
#line 1557 "pnscriptparser.tab.c"

  yyvsp -= yylen;
  yyssp -= yylen;


  YY_STACK_PRINT (yyss, yyssp);

  *++yyvsp = yyval;


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
#if YYERROR_VERBOSE
      yyn = yypact[yystate];

      if (YYPACT_NINF < yyn && yyn < YYLAST)
	{
	  int yytype = YYTRANSLATE (yychar);
	  YYSIZE_T yysize0 = yytnamerr (0, yytname[yytype]);
	  YYSIZE_T yysize = yysize0;
	  YYSIZE_T yysize1;
	  int yysize_overflow = 0;
	  char *yymsg = 0;
#	  define YYERROR_VERBOSE_ARGS_MAXIMUM 5
	  char const *yyarg[YYERROR_VERBOSE_ARGS_MAXIMUM];
	  int yyx;

#if 0
	  /* This is so xgettext sees the translatable formats that are
	     constructed on the fly.  */
	  YY_("syntax error, unexpected %s");
	  YY_("syntax error, unexpected %s, expecting %s");
	  YY_("syntax error, unexpected %s, expecting %s or %s");
	  YY_("syntax error, unexpected %s, expecting %s or %s or %s");
	  YY_("syntax error, unexpected %s, expecting %s or %s or %s or %s");
#endif
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
	  int yychecklim = YYLAST - yyn;
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
		yysize_overflow |= yysize1 < yysize;
		yysize = yysize1;
		yyfmt = yystpcpy (yyfmt, yyprefix);
		yyprefix = yyor;
	      }

	  yyf = YY_(yyformat);
	  yysize1 = yysize + yystrlen (yyf);
	  yysize_overflow |= yysize1 < yysize;
	  yysize = yysize1;

	  if (!yysize_overflow && yysize <= YYSTACK_ALLOC_MAXIMUM)
	    yymsg = (char *) YYSTACK_ALLOC (yysize);
	  if (yymsg)
	    {
	      /* Avoid sprintf, as that infringes on the user's name space.
		 Don't have undefined behavior even if the translation
		 produced a string with the wrong number of "%s"s.  */
	      char *yyp = yymsg;
	      int yyi = 0;
	      while ((*yyp = *yyf))
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
	      yyerror (yymsg);
	      YYSTACK_FREE (yymsg);
	    }
	  else
	    {
	      yyerror (YY_("syntax error"));
	      goto yyexhaustedlab;
	    }
	}
      else
#endif /* YYERROR_VERBOSE */
	yyerror (YY_("syntax error"));
    }



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
	  yydestruct ("Error: discarding", yytoken, &yylval);
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
  if (0)
     goto yyerrorlab;

yyvsp -= yylen;
  yyssp -= yylen;
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


      yydestruct ("Error: popping", yystos[yystate], yyvsp);
      YYPOPSTACK;
      yystate = *yyssp;
      YY_STACK_PRINT (yyss, yyssp);
    }

  if (yyn == YYFINAL)
    YYACCEPT;

  *++yyvsp = yylval;


  /* Shift the error token. */
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
		 yytoken, &yylval);
  while (yyssp != yyss)
    {
      yydestruct ("Cleanup: popping",
		  yystos[*yyssp], yyvsp);
      YYPOPSTACK;
    }
#ifndef yyoverflow
  if (yyss != yyssa)
    YYSTACK_FREE (yyss);
#endif
  return yyresult;
}


#line 369 "pnscriptparser.y"


static gdouble
get_named_constant_value (const gchar *name)
{
  if (g_strcasecmp (name, "pi") == 0)
    return G_PI;

  /* This is a failure, so don't make a "zero" :) */
  return 0.0;
}

static guint
get_function_token_type (const gchar *name)
{
  if (g_strcasecmp (name, "abs") == 0)
    return ABS_FUNC;
  if (g_strcasecmp (name, "max") == 0)
    return MAX_FUNC;
  if (g_strcasecmp (name, "min") == 0)
    return MIN_FUNC;
  if (g_strcasecmp (name, "sin") == 0)
    return SIN_FUNC;
  if (g_strcasecmp (name, "cos") == 0)
    return COS_FUNC;
  if (g_strcasecmp (name, "tan") == 0)
    return TAN_FUNC;
  if (g_strcasecmp (name, "asin") == 0)
    return ASIN_FUNC;
  if (g_strcasecmp (name, "acos") == 0)
    return ACOS_FUNC;
  if (g_strcasecmp (name, "atan") == 0)
    return ATAN_FUNC;

  return 0;
}

static gdouble*
get_constant_ptr (gdouble value)
{
  guint i;

  if (size_pass)
    {
      for (i=0; i<temp_constant_table->len; i++)
	if (g_array_index (temp_constant_table, gdouble, i) == value)
	  return (gdouble *) TRUE;

      /* Add a constant */
      g_array_append_val (temp_constant_table, value);
      return (gdouble *) TRUE;
    }
  else
    {
      for (i=0; i<temp_constant_table->len; i++)
	if (parse_script->constant_table[i] == value)
	  return &parse_script->constant_table[i];

      return NULL; /* This should never be reached */
    }
}

int
yylex (void)
{
  /* Skip whitespaces */
  while (isspace (*parse_string)) parse_string++;

  /* Handle the end of the string */
  if (*parse_string == '\0')
    return 0;

  /* Handle unnamed (numeric) constants */
  if (*parse_string == '.' || isdigit (*parse_string))
    {
      gdouble value;

      value = strtod (parse_string, (char **) &parse_string);
      yylval.constant = get_constant_ptr (value);

      return CONSTANT;
    }

  /* Handle alphanumeric symbols */
  if (isalpha (*parse_string))
    {
      const gchar *symbol_start = parse_string;
      guint function_token;
      gchar *symbol_name;

      while (isalnum (*parse_string) || *parse_string == '_') parse_string++;

      symbol_name = g_strndup (symbol_start, parse_string - symbol_start);

      /* Handle a named constant (e.g. 'pi') */
      if (get_named_constant_value (symbol_name))
	{
	  yylval.constant = get_constant_ptr (get_named_constant_value (symbol_name));

	  g_free (symbol_name);

	  return CONSTANT;
	}

      /* Handle a function (e.g. 'max') */
      if ((function_token = get_function_token_type (symbol_name)))
	{
	  g_free (symbol_name);

	  return function_token;
	}

      /* Handle a variable */
      if (! size_pass)
	yylval.variable = pn_symbol_table_ref_variable_by_name (parse_script->symbol_table,
								symbol_name);

      g_free (symbol_name);

      return VARIABLE;
    }

  /* Handle a single-character symbol (or invalid tokens) */
  return *parse_string++;
}

int
yyerror (char *s)
{
  parse_failed = TRUE;

  return 0;
}

gboolean
pn_script_internal_parse_string (PnScript *script, const gchar *string)
{
  guint i;

  g_return_val_if_fail (script != NULL, FALSE);
  g_return_val_if_fail (PN_IS_SCRIPT (script), FALSE);
  g_return_val_if_fail (string != NULL, FALSE);

  /* Make a new temp constant table if needed */
  if (! temp_constant_table)
    temp_constant_table = g_array_new (FALSE, FALSE, sizeof (gdouble));

  parse_failed = FALSE;

  parse_script = script;
  parse_string = string;

  /* First determine the code size */
  size_pass = TRUE;
  code_size = 0;
  yyparse ();

  if (parse_failed)
    return FALSE;

  if (code_size == 0)
    return TRUE;

  /* Now generate the real code */
  size_pass = FALSE;
  parse_string = string;
  script->code = g_malloc (code_size);
  script->constant_table = g_malloc (temp_constant_table->len * sizeof (gdouble));
  for (i=0; i<temp_constant_table->len; i++)
    script->constant_table[i] = g_array_index (temp_constant_table, gdouble, i);
  code_ptr = script->code;
  yyparse ();
  g_array_set_size (temp_constant_table, 0);

  /* Terminate the script, replacing the last POP with an END  */
  *(code_ptr-1) = PN_OP_END;

#ifdef PN_PRINT_OPS
  g_print ("END\n");
#endif /* PN_PRINT_OPS */

  return TRUE;
}


