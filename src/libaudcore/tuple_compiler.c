/*
 * tuple_compiler.c
 * Copyright (c) 2007 Matti 'ccr' Hämäläinen
 * Copyright (c) 2011 John Lindgren
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions, and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions, and the following disclaimer in the documentation
 *    provided with the distribution.
 *
 * This software is provided "as is" and without any warranty, express or
 * implied. In no event shall the authors be liable for any damages arising from
 * the use of this software.
 */

/*
 * TODO:
 * - Unicode/UTF-8 support in format strings. using any non-ASCII
 *   characters in Tuplez format strings WILL cause things go boom
 *   at the moment!
 *
 * - implement definitions (${=foo,"baz"} ${=foo,1234})
 * - implement functions
 * - implement handling of external expressions
 * - evaluation context: how local variables should REALLY work?
 *   currently there is just a single context, is a "global" context needed?
 */

#include <ctype.h>
#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <glib.h>

#include "tuple_compiler.h"

#define MAX_STR		(256)
#define MIN_ALLOC_NODES (8)
#define MIN_ALLOC_BUF	(64)
#define TUPLEZ_MAX_VARS (4)

#define tuple_error(ctx, ...) fprintf (stderr, "Tuple compiler: " __VA_ARGS__)

enum {
    OP_RAW = 0,		/* plain text */
    OP_FIELD,		/* a field/variable */
    OP_EXISTS,
    OP_EQUALS,
    OP_NOT_EQUALS,
    OP_GT,
    OP_GTEQ,
    OP_LT,
    OP_LTEQ,
    OP_IS_EMPTY
};

enum {
    TUPLE_VAR_FIELD = 0,
    TUPLE_VAR_CONST
};

struct _TupleEvalNode {
    int opcode;		/* operator, see OP_ enums */
    int var[TUPLEZ_MAX_VARS];	/* tuple variable references */
    char *text;		/* raw text, if any (OP_RAW) */
    struct _TupleEvalNode *children, *next, *prev; /* children of this struct, and pointer to next node. */
};

typedef struct {
    char *name;
    int type;			/* Type of variable, see VAR_* */
    int defvali;
    TupleValueType ctype;	/* Type of constant/def value */

    int fieldidx;		/* if >= 0: Index # of "pre-defined" Tuple fields */
    bool_t fieldread, fieldvalid;
    char * fieldstr;
} TupleEvalVar;

struct _TupleEvalContext {
    int nvariables;
    TupleEvalVar **variables;
};


static void tuple_evalctx_free_var(TupleEvalVar *var)
{
  g_free(var->name);
  str_unref (var->fieldstr);
  g_free(var);
}


/* Initialize an evaluation context
 */
TupleEvalContext * tuple_evalctx_new(void)
{
  return g_new0(TupleEvalContext, 1);
}


/* "Reset" the evaluation context
 */
void tuple_evalctx_reset(TupleEvalContext *ctx)
{
  int i;

  for (i = 0; i < ctx->nvariables; i++)
    if (ctx->variables[i]) {
      ctx->variables[i]->fieldread = FALSE;
      ctx->variables[i]->fieldvalid = FALSE;
      str_unref (ctx->variables[i]->fieldstr);
      ctx->variables[i]->fieldstr = NULL;
    }
}


/* Free an evaluation context and associated data
 */
void tuple_evalctx_free(TupleEvalContext *ctx)
{
  int i;

  if (!ctx) return;

  /* Deallocate variables */
  for (i = 0; i < ctx->nvariables; i++)
    if (ctx->variables[i])
      tuple_evalctx_free_var(ctx->variables[i]);

  g_free(ctx->variables);
  g_free(ctx);
}


static int tuple_evalctx_add_var (TupleEvalContext * ctx, const char * name,
 const int type, const TupleValueType ctype)
{
  int i;
  TupleEvalVar *tmp = g_new0(TupleEvalVar, 1);

  tmp->name = g_strdup(name);
  tmp->type = type;
  tmp->fieldidx = -1;
  tmp->ctype = ctype;

  /* Find fieldidx, if any */
  switch (type) {
    case TUPLE_VAR_FIELD:
      tmp->fieldidx = tuple_field_by_name (name);
      tmp->ctype = tuple_field_get_type (tmp->fieldidx);
      break;

    case TUPLE_VAR_CONST:
      if (ctype == TUPLE_INT)
        tmp->defvali = atoi(name);
      break;
  }

  /* Find a free slot */
  for (i = 0; i < ctx->nvariables; i++)
  if (!ctx->variables[i]) {
    ctx->variables[i] = tmp;
    return i;
  }

  i = ctx->nvariables;
  ctx->variables = g_renew(TupleEvalVar *, ctx->variables, ctx->nvariables + MIN_ALLOC_NODES);
  memset(&(ctx->variables[ctx->nvariables]), 0, MIN_ALLOC_NODES * sizeof(TupleEvalVar *));
  ctx->nvariables += MIN_ALLOC_NODES;
  ctx->variables[i] = tmp;

  return i;
}


static void tuple_evalnode_insert(TupleEvalNode **nodes, TupleEvalNode *node)
{
  if (*nodes) {
    node->prev = (*nodes)->prev;
    (*nodes)->prev->next = node;
    (*nodes)->prev = node;
    node->next = NULL;
  } else {
    *nodes = node;
    node->prev = node;
    node->next = NULL;
  }
}


static TupleEvalNode *tuple_evalnode_new(void)
{
  return g_new0(TupleEvalNode, 1);
}


void tuple_evalnode_free(TupleEvalNode *expr)
{
  TupleEvalNode *curr = expr, *next;

  while (curr) {
    next = curr->next;

    g_free(curr->text);

    if (curr->children)
      tuple_evalnode_free(curr->children);

    g_free(curr);

    curr = next;
  }
}


static TupleEvalNode *tuple_compiler_pass1(int *level, TupleEvalContext *ctx, const char **expression);


static bool_t tc_get_item(TupleEvalContext *ctx,
    const char **str, char *buf, gssize max,
    char endch, bool_t *literal, char *errstr, const char *item)
{
  gssize i = 0;
  const char *s = *str;
  char tmpendch;

  if (*s == '"') {
    if (*literal == FALSE) {
      tuple_error(ctx, "Literal string value not allowed in '%s'.\n", item);
      return FALSE;
    }
    s++;
    *literal = TRUE;
    tmpendch = '"';
  } else {
    *literal = FALSE;
    tmpendch = endch;
  }

  if (*literal == FALSE) {
    while (*s != '\0' && *s != tmpendch && (isalnum(*s) || *s == '-') && i < (max - 1)) {
      buf[i++] = *(s++);
    }

    if (*s != tmpendch && *s != '}' && !isalnum(*s) && *s != '-') {
      tuple_error(ctx, "Invalid field '%s' in '%s'.\n", *str, item);
      return FALSE;
    } else if (*s != tmpendch) {
      tuple_error(ctx, "Expected '%c' in '%s'.\n", tmpendch, item);
      return FALSE;
    }
  } else {
    while (*s != '\0' && *s != tmpendch && i < (max - 1)) {
      if (*s == '\\') s++;
      buf[i++] = *(s++);
    }
  }
  buf[i] = '\0';

  if (*literal) {
    if (*s == tmpendch)
      s++;
    else {
      tuple_error(ctx, "Expected literal string end ('%c') in '%s'.\n", tmpendch, item);
      return FALSE;
    }
  }

  if (*s != endch) {
    tuple_error(ctx, "Expected '%c' after %s in '%s'\n", endch, errstr, item);
    return FALSE;
  } else {
    *str = s;
    return TRUE;
  }
}


static int tc_get_variable(TupleEvalContext *ctx, char *name, int type)
{
  int i;
  TupleValueType ctype = TUPLE_UNKNOWN;

  if (name == '\0') return -1;

  if (isdigit(name[0])) {
    ctype = TUPLE_INT;
    type = TUPLE_VAR_CONST;
  } else
    ctype = TUPLE_STRING;

  if (type != TUPLE_VAR_CONST) {
    for (i = 0; i < ctx->nvariables; i++)
      if (ctx->variables[i] && !strcmp(ctx->variables[i]->name, name))
        return i;
  }

  return tuple_evalctx_add_var(ctx, name, type, ctype);
}


static bool_t tc_parse_construct(TupleEvalContext *ctx, TupleEvalNode **res,
 const char *item, const char **c, int *level, int opcode)
{
  char tmps1[MAX_STR], tmps2[MAX_STR];
  bool_t literal1 = TRUE, literal2 = TRUE;

  (*c)++;
  if (tc_get_item(ctx, c, tmps1, MAX_STR, ',', &literal1, "tag1", item)) {
    (*c)++;
    if (tc_get_item(ctx, c, tmps2, MAX_STR, ':', &literal2, "tag2", item)) {
      TupleEvalNode *tmp = tuple_evalnode_new();
      (*c)++;

      tmp->opcode = opcode;
      if ((tmp->var[0] = tc_get_variable(ctx, tmps1, literal1 ? TUPLE_VAR_CONST : TUPLE_VAR_FIELD)) < 0) {
        tuple_evalnode_free(tmp);
        tuple_error(ctx, "Invalid variable '%s' in '%s'.\n", tmps1, item);
        return FALSE;
      }
      if ((tmp->var[1] = tc_get_variable(ctx, tmps2, literal2 ? TUPLE_VAR_CONST : TUPLE_VAR_FIELD)) < 0) {
        tuple_evalnode_free(tmp);
        tuple_error(ctx, "Invalid variable '%s' in '%s'.\n", tmps2, item);
        return FALSE;
      }
      tmp->children = tuple_compiler_pass1(level, ctx, c);
      tuple_evalnode_insert(res, tmp);
    } else
      return FALSE;
  } else
    return FALSE;

  return TRUE;
}


/* Compile format expression into TupleEvalNode tree.
 * A "simple" straight compilation is sufficient in first pass, later
 * passes can perform subexpression removal and other optimizations.
 */
static TupleEvalNode *tuple_compiler_pass1(int *level, TupleEvalContext *ctx, const char **expression)
{
  TupleEvalNode *res = NULL, *tmp = NULL;
  const char *c = *expression, *item;
  char tmps1[MAX_STR];
  bool_t literal, end = FALSE;

  (*level)++;

  while (*c != '\0' && !end) {
    tmp = NULL;
    if (*c == '}') {
      c++;
      (*level)--;
      end = TRUE;
    } else if (*c == '$') {
      /* Expression? */
      item = c++;
      if (*c == '{') {
        int opcode;
        const char *expr = ++c;

        switch (*c) {
          case '?': c++;
            /* Exists? */
            literal = FALSE;
            if (tc_get_item(ctx, &c, tmps1, MAX_STR, ':', &literal, "tag", item)) {
              c++;
              tmp = tuple_evalnode_new();
              tmp->opcode = OP_EXISTS;
              if ((tmp->var[0] = tc_get_variable(ctx, tmps1, TUPLE_VAR_FIELD)) < 0) {
                tuple_error(ctx, "Invalid variable '%s' in '%s'.\n", tmps1, expr);
                goto ret_error;
              }
              tmp->children = tuple_compiler_pass1(level, ctx, &c);
              tuple_evalnode_insert(&res, tmp);
            } else
              goto ret_error;
            break;

          case '=': c++;
            if (*c != '=') {
              /* Definition */
              literal = FALSE;
              if (tc_get_item(ctx, &c, tmps1, MAX_STR, ',', &literal, "variable", item)) {
                c++;
                if (*c == '"') {
                  /* String */
                  c++;
                } else if (isdigit(*c)) {
                  /* Integer */
                }

                tuple_error(ctx, "Definitions are not yet supported!\n");
                goto ret_error;
              } else
                goto ret_error;
            } else {
              /* Equals? */
              if (!tc_parse_construct(ctx, &res, item, &c, level, OP_EQUALS))
                goto ret_error;
            }
            break;

          case '!': c++;
            if (*c != '=') goto ext_expression;
            if (!tc_parse_construct(ctx, &res, item, &c, level, OP_NOT_EQUALS))
              goto ret_error;
            break;

          case '<': c++;
            if (*c == '=') {
              opcode = OP_LTEQ;
              c++;
            } else
              opcode = OP_LT;

            if (!tc_parse_construct(ctx, &res, item, &c, level, opcode))
              goto ret_error;
            break;

          case '>': c++;
            if (*c == '=') {
              opcode = OP_GTEQ;
              c++;
            } else
              opcode = OP_GT;

            if (!tc_parse_construct(ctx, &res, item, &c, level, opcode))
              goto ret_error;
            break;

          case '(': c++;
            if (!strncmp(c, "empty)?", 7)) {
              c += 7;
              literal = FALSE;
              if (tc_get_item(ctx, &c, tmps1, MAX_STR, ':', &literal, "tag", item)) {
                c++;
                tmp = tuple_evalnode_new();
                tmp->opcode = OP_IS_EMPTY;
                if ((tmp->var[0] = tc_get_variable(ctx, tmps1, TUPLE_VAR_FIELD)) < 0) {
                  tuple_error(ctx, "Invalid variable '%s' in '%s'.\n", tmps1, expr);
                  goto ret_error;
                }
                tmp->children = tuple_compiler_pass1(level, ctx, &c);
                tuple_evalnode_insert(&res, tmp);
              } else
                goto ret_error;
            } else
              goto ext_expression;
            break;

          default:
          ext_expression:
            /* Get expression content */
            c = expr;
            literal = FALSE;
            if (tc_get_item(ctx, &c, tmps1, MAX_STR, '}', &literal, "field", item)) {
              /* FIXME!! FIX ME! Check for external expressions */

              /* I HAS A FIELD - A field. You has it. */
              tmp = tuple_evalnode_new();
              tmp->opcode = OP_FIELD;
              if ((tmp->var[0] = tc_get_variable(ctx, tmps1, TUPLE_VAR_FIELD)) < 0) {
                tuple_error(ctx, "Invalid variable '%s' in '%s'.\n", tmps1, expr);
                goto ret_error;
              }
              tuple_evalnode_insert(&res, tmp);
              c++;

            } else
              goto ret_error;
        }
      } else {
        tuple_error(ctx, "Expected '{', got '%c' in '%s'.\n", *c, c);
        goto ret_error;
      }

    } else if (*c == '%') {
      /* Function? */
      item = c++;
      if (*c == '{') {
        gssize i = 0;
        c++;

        while (*c != '\0' && (isalnum(*c) || *c == '-') && *c != '}' && *c != ':' && i < (MAX_STR - 1))
          tmps1[i++] = *(c++);
        tmps1[i] = '\0';

        if (*c == ':') {
          c++;
        } else if (*c == '}') {
          c++;
        } else if (*c == '\0') {
          tuple_error(ctx, "Expected '}' or function arguments in '%s'\n", item);
          goto ret_error;
        }
      } else {
        tuple_error(ctx, "Expected '{', got '%c' in '%s'.\n", *c, c);
        goto ret_error;
      }
    } else {
      /* Parse raw/literal text */
      gssize i = 0;
      while (*c != '\0' && *c != '$' && *c != '%' && *c != '}' && i < (MAX_STR - 1)) {
        if (*c == '\\') c++;
        tmps1[i++] = *(c++);
      }
      tmps1[i] = '\0';

      tmp = tuple_evalnode_new();
      tmp->opcode = OP_RAW;
      tmp->text = g_strdup(tmps1);
      tuple_evalnode_insert(&res, tmp);
    }
  }

  if (*level <= 0) {
    tuple_error(ctx, "Syntax error! Uneven/unmatched nesting of elements in '%s'!\n", c);
    goto ret_error;
  }

  *expression = c;
  return res;

ret_error:
  tuple_evalnode_free(tmp);
  tuple_evalnode_free(res);
  return NULL;
}


TupleEvalNode *tuple_formatter_compile(TupleEvalContext *ctx, const char *expr)
{
  int level = 0;
  const char *tmpexpr = expr;
  TupleEvalNode *res1;

  res1 = tuple_compiler_pass1(&level, ctx, &tmpexpr);

  if (level != 1) {
    tuple_error(ctx, "Syntax error! Uneven/unmatched nesting of elements! (%d)\n", level);
    tuple_evalnode_free(res1);
    return NULL;
  }

  return res1;
}


/* Fetch a tuple field value.  Return TRUE if found. */
static bool_t tf_get_fieldval (TupleEvalVar * var, const Tuple * tuple)
{
  if (var->type != TUPLE_VAR_FIELD || var->fieldidx < 0)
    return FALSE;

  if (var->fieldread)
    return var->fieldvalid;

  if (tuple_get_value_type (tuple, var->fieldidx, NULL) != var->ctype) {
    var->fieldread = TRUE;
    var->fieldvalid = FALSE;
    return FALSE;
  }

  if (var->ctype == TUPLE_INT)
    var->defvali = tuple_get_int (tuple, var->fieldidx, NULL);
  else if (var->ctype == TUPLE_STRING)
    var->fieldstr = tuple_get_str (tuple, var->fieldidx, NULL);

  var->fieldread = TRUE;
  var->fieldvalid = TRUE;
  return TRUE;
}


/* Fetch string or int value of given variable, whatever type it might be.
 * Return VAR_* type for the variable.
 */
static TupleValueType tf_get_var (char * * tmps, int * tmpi, TupleEvalVar *
 var, const Tuple * tuple)
{
  TupleValueType type = TUPLE_UNKNOWN;
  *tmps = NULL;
  *tmpi = 0;

  switch (var->type) {
    case TUPLE_VAR_CONST:
      switch (var->ctype) {
        case TUPLE_STRING: *tmps = var->name; break;
        case TUPLE_INT: *tmpi = var->defvali; break;
        default: /* Cannot happen */ break;
      }
      type = var->ctype;
      break;

    case TUPLE_VAR_FIELD:
      if (tf_get_fieldval (var, tuple)) {
        type = var->ctype;
        if (type == TUPLE_INT)
          * tmpi = var->defvali;
        else if (type == TUPLE_STRING)
          * tmps = var->fieldstr;
      }
      break;
  }

  return type;
}


/* Evaluate tuple in given TupleEval expression in given
 * context and return resulting string.
 */
static bool_t tuple_formatter_eval_do (TupleEvalContext * ctx, TupleEvalNode *
 expr, const Tuple * tuple, GString * out)
{
  TupleEvalNode *curr = expr;
  TupleEvalVar *var0, *var1;
  TupleValueType type0, type1;
  int tmpi0, tmpi1;
  char tmps[MAX_STR], *tmps0, *tmps1, *tmps2;
  bool_t result;
  int resulti;

  if (!expr) return FALSE;

  while (curr) {
    const char *str = NULL;

    switch (curr->opcode) {
      case OP_RAW:
        str = curr->text;
        break;

      case OP_FIELD:
        var0 = ctx->variables[curr->var[0]];

        switch (var0->type) {
          case TUPLE_VAR_FIELD:
            if (tf_get_fieldval (var0, tuple)) {
              switch (var0->ctype) {
                case TUPLE_STRING:
                  str = var0->fieldstr;
                  break;

                case TUPLE_INT:
                  g_snprintf (tmps, sizeof (tmps), "%d", var0->defvali);
                  str = tmps;
                  break;

                default:
                  str = NULL;
              }
            }
            break;
        }
        break;

      case OP_EQUALS:
      case OP_NOT_EQUALS:
      case OP_LT: case OP_LTEQ:
      case OP_GT: case OP_GTEQ:
        var0 = ctx->variables[curr->var[0]];
        var1 = ctx->variables[curr->var[1]];

        type0 = tf_get_var(&tmps0, &tmpi0, var0, tuple);
        type1 = tf_get_var(&tmps1, &tmpi1, var1, tuple);
        result = FALSE;

        if (type0 != TUPLE_UNKNOWN && type1 != TUPLE_UNKNOWN) {
          if (type0 == type1) {
            if (type0 == TUPLE_STRING)
              resulti = strcmp(tmps0, tmps1);
            else
              resulti = tmpi0 - tmpi1;
          } else {
            if (type0 == TUPLE_INT)
              resulti = tmpi0 - atoi(tmps1);
            else
              resulti = atoi(tmps0) - tmpi1;
          }

          switch (curr->opcode) {
            case OP_EQUALS:     result = (resulti == 0); break;
            case OP_NOT_EQUALS: result = (resulti != 0); break;
            case OP_LT:         result = (resulti <  0); break;
            case OP_LTEQ:       result = (resulti <= 0); break;
            case OP_GT:         result = (resulti >  0); break;
            case OP_GTEQ:       result = (resulti >= 0); break;
          default:            result = FALSE;
          }
        }

        if (result && ! tuple_formatter_eval_do (ctx, curr->children, tuple, out))
          return FALSE;
        break;

      case OP_EXISTS:
        if (tf_get_fieldval (ctx->variables[curr->var[0]], tuple)) {
          if (! tuple_formatter_eval_do (ctx, curr->children, tuple, out))
            return FALSE;
        }
        break;

      case OP_IS_EMPTY:
        var0 = ctx->variables[curr->var[0]];

        if (tf_get_fieldval (var0, tuple)) {
          switch (var0->ctype) {
          case TUPLE_INT:
            result = (var0->defvali == 0);
            break;

          case TUPLE_STRING:
            result = TRUE;
            tmps2 = var0->fieldstr;

            while (result && tmps2 && *tmps2 != '\0') {
              gunichar uc = g_utf8_get_char(tmps2);
              if (g_unichar_isspace(uc))
                tmps2 = g_utf8_next_char(tmps2);
              else
                result = FALSE;
            }
            break;

          default:
            result = TRUE;
          }
        } else
          result = TRUE;

        if (result && ! tuple_formatter_eval_do (ctx, curr->children, tuple, out))
          return FALSE;
        break;

      default:
        tuple_error(ctx, "Unimplemented opcode %d!\n", curr->opcode);
        return FALSE;
        break;
    }

    if (str)
      g_string_append (out, str);

    curr = curr->next;
  }

  return TRUE;
}

void tuple_formatter_eval (TupleEvalContext * ctx, TupleEvalNode * expr,
 const Tuple * tuple, GString * out)
{
    g_string_truncate (out, 0);
    tuple_formatter_eval_do (ctx, expr, tuple, out);
}
