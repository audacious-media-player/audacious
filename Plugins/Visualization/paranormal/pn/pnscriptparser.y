%{
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
%}

%union {
  gdouble     *constant;
  PnVariable *variable;
}

%token <constant> CONSTANT
%token <variable> VARIABLE
/* Functions */
%token ABS_FUNC
%token MAX_FUNC
%token MIN_FUNC
%token SIN_FUNC
%token COS_FUNC
%token TAN_FUNC
%token ASIN_FUNC
%token ACOS_FUNC
%token ATAN_FUNC


%right '='
%left '-' '+'
%left '*' '/'
%left NEG
%right '^'

%%

script
	: /* empty */
	| script statement
	;

statement
	: equation ';'
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
		}
	;

equation
	: VARIABLE '=' expression
		{
		  if (size_pass)
		    code_size += sizeof (guint) + sizeof (gdouble *);
		  else
		    {
		      *code_ptr++ = PN_OP_SET;
		      *code_ptr++ = (guint) $1;

#ifdef PN_PRINT_OPS
		      g_print ("SET %s\n", $1->name);
#endif /* PN_PRINT_OPS */
		    }
		}

expression
	: CONSTANT
		{
		  if (size_pass)
		    code_size += sizeof (guint) + sizeof (gdouble *);
		  else
		    {
		      *code_ptr++ = PN_OP_PUSHC;
		      *code_ptr++ = GPOINTER_TO_UINT ($1);

#ifdef PN_PRINT_OPS
		      g_print ("PUSHC %f\n", *$1);
#endif /* PN_PRINT_OPS */
		    }
		}
	| VARIABLE
		{
		  if (size_pass)
		    code_size += sizeof (guint) + sizeof (gdouble *);
		  else
		    {
		      *code_ptr++ = PN_OP_PUSHV;
		      *code_ptr++ = (guint) $1;

#ifdef PN_PRINT_OPS
		      g_print ("PUSHV %s\n", $1->name);
#endif /* PN_PRINT_OPS */
		    }
		}
	| equation
	| ABS_FUNC '(' expression ')'
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
		}
	| MAX_FUNC '(' expression ',' expression  ')'
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
		}
	| MIN_FUNC '(' expression ',' expression ')'
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

		}
	| SIN_FUNC '(' expression ')'
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

		}
	| COS_FUNC '(' expression ')'
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

		}
	| TAN_FUNC '(' expression ')'
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

		}
	| ASIN_FUNC '(' expression ')'
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

		}
	| ACOS_FUNC '(' expression ')'
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

		}
	| ATAN_FUNC '(' expression ')'
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

		}
	| expression '+' expression
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
		}
	| expression '-' expression
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
		}
	| expression '*' expression
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
		}
	| expression '/' expression
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
		}
	| '-' expression %prec NEG
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
		}
	| expression '^' expression
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
		}
	| '(' expression ')'
	;

%%

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
