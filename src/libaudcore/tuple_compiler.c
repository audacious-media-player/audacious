/*
 * Audacious - Tuplez compiler
 * Copyright (c) 2007 Matti 'ccr' Hämäläinen
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; under version 3 of the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses>.
 *
 * The Audacious team does not consider modular code linking to
 * Audacious or using our public API to be a derived work.
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

#include "config.h"
#include <assert.h>
#include <stdarg.h>
#include "tuple_compiler.h"

#define MAX_STR		(256)
#define MIN_ALLOC_NODES (8)
#define MIN_ALLOC_BUF	(64)


void tuple_error(TupleEvalContext *ctx, const gchar *fmt, ...)
{
  va_list ap;
  g_free(ctx->errmsg);
  va_start(ap, fmt);
  ctx->errmsg = g_strdup_vprintf(fmt, ap);
  va_end(ap);
  ctx->iserror = TRUE;
}


static void tuple_evalctx_free_var(TupleEvalVar *var)
{
  assert(var != NULL);
  var->fieldidx = -1;
  g_free(var->defvals);
  g_free(var->name);
  g_free(var);
}


static void tuple_evalctx_free_function(TupleEvalFunc *func)
{
  assert(func != NULL);

  g_free(func->name);
  g_free(func);
}


/* Initialize an evaluation context
 */
TupleEvalContext * tuple_evalctx_new(void)
{
  return g_new0(TupleEvalContext, 1);
}


/* "Reset" the evaluation context, clean up temporary variables.
 */
void tuple_evalctx_reset(TupleEvalContext *ctx)
{
  gint i;

  for (i = 0; i < ctx->nvariables; i++)
    if (ctx->variables[i]) {
      ctx->variables[i]->fieldref = NULL;

      if (ctx->variables[i]->istemp)
        tuple_evalctx_free_var(ctx->variables[i]);
    }

  ctx->iserror = FALSE;
}


/* Free an evaluation context and associated data
 */
void tuple_evalctx_free(TupleEvalContext *ctx)
{
  gint i;

  if (!ctx) return;

  /* Deallocate variables */
  for (i = 0; i < ctx->nvariables; i++)
    if (ctx->variables[i])
      tuple_evalctx_free_var(ctx->variables[i]);

  g_free(ctx->variables);

  /* Deallocate functions */
  for (i = 0; i < ctx->nfunctions; i++)
    if (ctx->functions[i])
      tuple_evalctx_free_function(ctx->functions[i]);

  g_free(ctx->functions);
  g_free(ctx);
}


gint tuple_evalctx_add_var(TupleEvalContext *ctx, const gchar *name, const gboolean istemp, const gint type, const TupleValueType ctype)
{
  gint i, ref = -1;
  TupleEvalVar *tmp = g_new0(TupleEvalVar, 1);
  assert(tmp != NULL);

  tmp->name = g_strdup(name);
  tmp->istemp = istemp;
  tmp->type = type;
  tmp->fieldidx = ref;
  tmp->ctype = ctype;

  /* Find fieldidx, if any */
  switch (type) {
    case TUPLE_VAR_FIELD:
      for (i = 0; i < FIELD_LAST && ref < 0; i++)
        if (strcmp(tuple_fields[i].name, name) == 0) ref = i;

        tmp->fieldidx = ref;
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


gint tuple_evalctx_add_function(TupleEvalContext *ctx, gchar *name)
{
  assert(ctx != NULL);
  assert(name != NULL);

  return -1;
}


static void tuple_evalnode_insert(TupleEvalNode **nodes, TupleEvalNode *node)
{
  assert(nodes != NULL);
  assert(node != NULL);

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


static TupleEvalNode *tuple_compiler_pass1(gint *level, TupleEvalContext *ctx, gchar **expression);


static gboolean tc_get_item(TupleEvalContext *ctx,
    gchar **str, gchar *buf, gssize max,
    gchar endch, gboolean *literal, gchar *errstr, gchar *item)
{
  gssize i = 0;
  gchar *s = *str, tmpendch;
  assert(str != NULL);
  assert(buf != NULL);

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


static gint tc_get_variable(TupleEvalContext *ctx, gchar *name, gint type)
{
  gint i;
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

  return tuple_evalctx_add_var(ctx, name, FALSE, type, ctype);
}


static gboolean tc_parse_construct(TupleEvalContext *ctx, TupleEvalNode **res, gchar *item, gchar **c, gint *level, gint opcode)
{
  gchar tmps1[MAX_STR], tmps2[MAX_STR];
  gboolean literal1 = TRUE, literal2 = TRUE;

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
static TupleEvalNode *tuple_compiler_pass1(gint *level, TupleEvalContext *ctx, gchar **expression)
{
  TupleEvalNode *res = NULL, *tmp = NULL;
  gchar *c = *expression, *item, tmps1[MAX_STR];
  gboolean literal, end = FALSE;
  assert(ctx != NULL);
  assert(expression != NULL);

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
        gint opcode;
        gchar *expr = ++c;

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


static TupleEvalNode *tuple_compiler_pass2(gboolean *changed, TupleEvalContext *ctx, TupleEvalNode *expr)
{
  /* TupleEvalNode *curr = expr; */
  TupleEvalNode *res = NULL;
  assert(ctx != NULL);

  return res;
}


TupleEvalNode *tuple_formatter_compile(TupleEvalContext *ctx, gchar *expr)
{
  gint level = 0;
  gboolean changed = FALSE;
  gchar *tmpexpr = expr;
  TupleEvalNode *res1, *res2;

  res1 = tuple_compiler_pass1(&level, ctx, &tmpexpr);

  if (level != 1) {
    tuple_error(ctx, "Syntax error! Uneven/unmatched nesting of elements! (%d)\n", level);
    tuple_evalnode_free(res1);
    return NULL;
  }

  res2 = tuple_compiler_pass2(&changed, ctx, res1);

  if (changed) {
    tuple_evalnode_free(res1);
    return res2;
  } else {
    tuple_evalnode_free(res2);
    return res1;
  }
}


/* Fetch a reference to a tuple field for given variable by fieldidx or dict.
 * Return pointer to field, NULL if not available.
 */
static TupleValue * tf_get_fieldref (TupleEvalVar * var, const Tuple * tuple)
{
  if (var->type == TUPLE_VAR_FIELD && var->fieldref == NULL) {
    if (var->fieldidx < 0)
      var->fieldref = mowgli_dictionary_retrieve(tuple->dict, var->name);
    else
      var->fieldref = tuple->values[var->fieldidx];
  }

  return var->fieldref;
}


/* Fetch string or int value of given variable, whatever type it might be.
 * Return VAR_* type for the variable.
 */
static TupleValueType tf_get_var (gchar * * tmps, gint * tmpi, TupleEvalVar *
 var, const Tuple * tuple)
{
  TupleValueType type = TUPLE_UNKNOWN;
  *tmps = NULL;
  *tmpi = 0;

  switch (var->type) {
    case TUPLE_VAR_DEF:
      switch (var->ctype) {
        case TUPLE_STRING: *tmps = var->defvals; break;
        case TUPLE_INT: *tmpi = var->defvali; break;
        default: /* Possible, but should be prevented elsewhere */ break;
      }
      type = var->ctype;
      break;

    case TUPLE_VAR_CONST:
      switch (var->ctype) {
        case TUPLE_STRING: *tmps = var->name; break;
        case TUPLE_INT: *tmpi = var->defvali; break;
        default: /* Cannot happen */ break;
      }
      type = var->ctype;
      break;

    case TUPLE_VAR_FIELD:
      if (tf_get_fieldref(var, tuple)) {
        if (var->fieldref->type == TUPLE_STRING)
          *tmps = var->fieldref->value.string;
        else
          *tmpi = var->fieldref->value.integer;
        type = var->fieldref->type;
      }
      break;
  }

  return type;
}


/* Evaluate tuple in given TupleEval expression in given
 * context and return resulting string.
 */
static gboolean tuple_formatter_eval_do (TupleEvalContext * ctx, TupleEvalNode *
 expr, const Tuple * tuple, gchar * * res, gssize * resmax, gssize * reslen)
{
  TupleEvalNode *curr = expr;
  TupleEvalVar *var0, *var1;
  TupleValueType type0, type1;
  gint tmpi0, tmpi1;
  gchar tmps[MAX_STR], *tmps0, *tmps1, *tmps2;
  gboolean result;
  gint resulti;

  if (!expr) return FALSE;

  while (curr) {
    const gchar *str = NULL;

    switch (curr->opcode) {
      case OP_RAW:
        str = curr->text;
        break;

      case OP_FIELD:
        var0 = ctx->variables[curr->var[0]];

        switch (var0->type) {
          case TUPLE_VAR_DEF:
            switch (var0->ctype) {
              case TUPLE_STRING:
                str = var0->defvals;
                break;

              case TUPLE_INT:
                g_snprintf(tmps, sizeof(tmps), "%d", var0->defvali);
                str = tmps;
                break;

              default:
                break;
            }
            break;

          case TUPLE_VAR_FIELD:
            if (tf_get_fieldref(var0, tuple)) {
              switch (var0->fieldref->type) {
                case TUPLE_STRING:
                  str = var0->fieldref->value.string;
                  break;

                case TUPLE_INT:
                  g_snprintf(tmps, sizeof(tmps), "%d", var0->fieldref->value.integer);
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

        if (result && !tuple_formatter_eval_do(ctx, curr->children, tuple, res, resmax, reslen))
          return FALSE;
        break;

      case OP_EXISTS:
#ifdef NO_EXISTS_HACK
        if (tf_get_fieldref(ctx->variables[curr->var[0]], tuple)) {
          if (!tuple_formatter_eval_do(ctx, curr->children, tuple, res, resmax, reslen))
            return FALSE;
        }
        break;
#endif

      case OP_IS_EMPTY:
        var0 = ctx->variables[curr->var[0]];

        if (tf_get_fieldref(var0, tuple)) {

          switch (var0->fieldref->type) {
          case TUPLE_INT:
            result = (var0->fieldref->value.integer == 0);
            break;

          case TUPLE_STRING:
            result = TRUE;
            tmps2 = var0->fieldref->value.string;

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

#ifdef NO_EXISTS_HACK
        if (result && !tuple_formatter_eval_do(ctx, curr->children, tuple, res, resmax, reslen))
          return FALSE;
#else
        if ((curr->opcode == OP_EXISTS && !result) || (curr->opcode == OP_IS_EMPTY && result)) {
          if (!tuple_formatter_eval_do(ctx, curr->children, tuple, res, resmax, reslen))
            return FALSE;
        }
#endif
        break;

      default:
        tuple_error(ctx, "Unimplemented opcode %d!\n", curr->opcode);
        return FALSE;
        break;
    }

    if (str) {
      /* (Re)allocate res for more space, if needed. */
      *reslen += strlen(str);
      if (*res) {
        if (*reslen >= *resmax) {
          *resmax += *reslen + MIN_ALLOC_BUF;
          *res = g_realloc(*res, *resmax);
        }

        strcat(*res, str);
      } else {
        *resmax = *reslen + MIN_ALLOC_BUF;
        *res = g_malloc(*resmax);

        g_strlcpy(*res, str, *resmax);
      }
    }

    curr = curr->next;
  }

  return TRUE;
}


gchar * tuple_formatter_eval (TupleEvalContext * ctx, TupleEvalNode * expr,
 const Tuple * tuple)
{
  gchar *res = g_strdup("");
  gssize resmax = 0, reslen = 0;
  assert(ctx != NULL);
  assert(tuple != NULL);

  if (!expr) return res;

  tuple_formatter_eval_do(ctx, expr, tuple, &res, &resmax, &reslen);

  return res;
}


static void print_vars(FILE *f, TupleEvalContext *ctx, TupleEvalNode *node, gint start, gint end)
{
  gint i;

  for (i = start; i <= end; i++) {
    TupleEvalVar *v = NULL;
    gchar *s = NULL;
    gint n = node->var[i];

    if (n >= 0) {
      v = ctx->variables[n];
      if (v) {
        s = v->name;

        if (v->type == TUPLE_VAR_CONST)
          fprintf(f, "(const)");
        else if (v->type == TUPLE_VAR_DEF)
          fprintf(f, "(def)");
      }
    }

    fprintf(f, "var[%d]=(%d),\"%s\" ", i, n, s);
  }
}


gint tuple_formatter_print(FILE *f, gint *level, TupleEvalContext *ctx, TupleEvalNode *expr)
{
  TupleEvalNode *curr = expr;

  if (!expr) return -1;

  (*level)++;

  while (curr) {
    gint i;
    for (i = 0; i < *level; i++)
      fprintf(f, "  ");

    switch (curr->opcode) {
      case OP_RAW:
        fprintf(f, "OP_RAW text=\"%s\"\n", curr->text);
        break;

      case OP_FIELD:
        fprintf(f, "OP_FIELD ");
        print_vars(f, ctx, curr, 0, 0);
        fprintf(f, "\n");
        break;

      case OP_EXISTS:
        fprintf(f, "OP_EXISTS ");
        print_vars(f, ctx, curr, 0, 0);
        fprintf(f, "\n");
        tuple_formatter_print(f, level, ctx, curr->children);
        break;

      case OP_DEF_STRING:
        fprintf(f, "OP_DEF_STRING ");
        fprintf(f, "\n");
        break;

      case OP_DEF_INT:
        fprintf(f, "OP_DEF_INT ");
        fprintf(f, "\n");
        break;

      case OP_EQUALS:
        fprintf(f, "OP_EQUALS ");
        print_vars(f, ctx, curr, 0, 1);
        fprintf(f, "\n");
        tuple_formatter_print(f, level, ctx, curr->children);
        break;

      case OP_NOT_EQUALS:
        fprintf(f, "OP_NOT_EQUALS ");
        print_vars(f, ctx, curr, 0, 1);
        fprintf(f, "\n");
        tuple_formatter_print(f, level, ctx, curr->children);
        break;

      case OP_IS_EMPTY:
        fprintf(f, "OP_IS_EMPTY ");
        print_vars(f, ctx, curr, 0, 0);
        fprintf(f, "\n");
        tuple_formatter_print(f, level, ctx, curr->children);
        break;

      default:
        fprintf(f, "Unimplemented opcode %d!\n", curr->opcode);
        break;
    }

    curr = curr->next;
  }

  (*level)--;

  return 0;
}
