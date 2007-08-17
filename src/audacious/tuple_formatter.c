/*
 * Audacious
 * Copyright (c) 2007 William Pitcock
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

#include <glib.h>
#include <mowgli.h>

#include "config.h"
#include "tuple.h"
#include "tuple_formatter.h"

#define _DEBUG

#ifdef _DEBUG
# define _TRACE(fmt, ...) g_print("[tuple-fmt] %s(%d) " fmt "\n", __FILE__, __LINE__, __VA_ARGS__);
#else
# define _TRACE(fmt, ...)
#endif

/*
 * the tuple formatter:
 *
 * this is a data-driven meta-language which eventually hopes to be
 * turing complete.
 *
 * language constructs follow the following basic rules:
 *   - begin with ${
 *   - end with }
 *
 * language constructs:
 *   - ${field}: prints a field
 *   - ${?field:expr}: evaluates expr if field exists
 *   - ${=field,"value"}: defines field in the currently iterated
 *                        tuple as string value of "value"
 *   - ${=field,value}: defines field in the currently iterated
 *                      tuple as integer value of "value"
 *   - ${==field,field:expr}: evaluates expr if both fields are the same
 *   - ${!=field,field:expr}: evaluates expr if both fields are not the same
 *   - ${(empty)?field:expr}: evaluates expr if field is empty or does not exist
 *   - %{function:args,arg2,...}: runs function and inserts the result.
 *
 * everything else is treated as raw text.
 * additionally, plugins can add additional instructions and functions!
 */

typedef struct {
    Tuple *tuple;
    GString *str;
} TupleFormatterContext;

/* processes a construct, e.g. "${?artist:artist is defined}" would
   return "artist is defined" if artist is defined. */
gchar *
tuple_formatter_process_construct(Tuple *tuple, const gchar *string)
{
    TupleFormatterContext *ctx;
    const gchar *iter;
    gchar *out;
    gint level = 0;

    g_return_val_if_fail(tuple != NULL, NULL);
    g_return_val_if_fail(string != NULL, NULL);

    ctx = g_new0(TupleFormatterContext, 1);
    ctx->str = g_string_new("");

    _TRACE("parsing <%s>", string);

    /* parsers are ugly */
    for (iter = string; *iter != '\0'; iter++)
    {
        /* if it's raw text, just copy the byte */
        if (*iter != '$' && *iter != '%' && *iter != '}' )
        {
            g_string_append_c(ctx->str, *iter);
        }
        else if (*iter == '}' && level > 0)
        {
            level--;
        }
        else if (g_str_has_prefix(iter, "${") == TRUE)
        {
            GString *expression = g_string_new("");
            GString *argument = g_string_new("");
            GString *sel = expression;
            gchar *result;
            level++;

            for (iter += 2; *iter != '\0'; iter++)
            {
                if (*iter == ':')
                {
                    if (sel != argument)
                    {
                        sel = argument;
                        continue;
                    }
                    else
                        g_string_append_c(sel, *iter);
                    continue;
                }

                if (g_str_has_prefix(iter, "${") == TRUE || g_str_has_prefix(iter, "%{") == TRUE)
                {
                    if (sel == argument)
                    {
                        g_string_append_c(sel, *iter);
                        level++;
                    }
                }
                else if (*iter == '}' && (sel == argument))
                {
                    level--;
                    if (level == 0)
                      break;
                    else
                      g_string_append_c(sel, *iter);
                }
                else if (*iter == '}' && ((sel != argument)))
                    break;
                else
                    g_string_append_c(sel, *iter);
            }

            if (expression->len == 0)
            {
                g_string_free(expression, TRUE);
                g_string_free(argument, TRUE);
                continue;
            }

            result = tuple_formatter_process_expr(tuple, expression->str, argument->len ? argument->str : NULL);
            if (result != NULL)
            {
                g_string_append(ctx->str, result);
                g_free(result);
            }

            g_string_free(expression, TRUE);
            g_string_free(argument, TRUE);

            if (*iter == '\0')
                break;
        }
        else if (g_str_has_prefix(iter, "%{") == TRUE)
        {
            GString *expression = g_string_new("");
            GString *argument = g_string_new("");
            GString *sel = expression;
            gchar *result;
            level++;

            for (iter += 2; *iter != '\0'; iter++)
            {
                if (*iter == ':')
                {
                    if (sel != argument)
                    {
                        sel = argument;
                        continue;
                    }
                    else
                        g_string_append_c(sel, *iter);
                    continue;
                }

                if (g_str_has_prefix(iter, "${") == TRUE || g_str_has_prefix(iter, "%{") == TRUE)
                {
                    if (sel == argument)
                    {
                        g_string_append_c(sel, *iter);
                        level++;
                    }
                }
                else if (*iter == '}' && (sel == argument))
                {
                    level--;
                    if (level == 0)
                      break;
                    else
                    g_string_append_c(sel, *iter);
                }
                else if (*iter == '}' && ((sel != argument)))
                    break;
                else
                    g_string_append_c(sel, *iter);
            }

            if (expression->len == 0)
            {
                g_string_free(expression, TRUE);
                g_string_free(argument, TRUE);
                continue;
            }

            result = tuple_formatter_process_function(tuple, expression->str, argument->len ? argument->str : NULL);
            if (result != NULL)
            {
                g_string_append(ctx->str, result);
                g_free(result);
            }

            g_string_free(expression, TRUE);
            g_string_free(argument, TRUE);

            if (*iter == '\0')
                break;
        }
    }

    out = g_strdup(ctx->str->str);
    g_string_free(ctx->str, TRUE);
    g_free(ctx);

    _TRACE("parsed <%s> as <%s>", string, out);

    return out;
}

static GList *tuple_formatter_expr_list = NULL;

typedef struct {
    const gchar *name;
    gboolean (*func)(Tuple *tuple, const gchar *expression);
} TupleFormatterExpression;

/* processes an expression and optional argument pair. */
gchar *
tuple_formatter_process_expr(Tuple *tuple, const gchar *expression, 
    const gchar *argument)
{
    TupleFormatterExpression *expr = NULL;
    GList *iter;

    g_return_val_if_fail(tuple != NULL, NULL);
    g_return_val_if_fail(expression != NULL, NULL);

    for (iter = tuple_formatter_expr_list; iter != NULL; iter = iter->next)
    {
        TupleFormatterExpression *tmp = (TupleFormatterExpression *) iter->data;

        if (g_str_has_prefix(expression, tmp->name) == TRUE)
        {
            expr = tmp;
            expression += strlen(tmp->name);
        }
    }

    /* ${artist} */
    if (expr == NULL && argument == NULL)
    {
        TupleValueType type = tuple_get_value_type(tuple, expression);

        switch(type)
        {
        case TUPLE_STRING:
             return g_strdup(tuple_get_string(tuple, expression));
             break;
        case TUPLE_INT:
             return g_strdup_printf("%d", tuple_get_int(tuple, expression));
             break;
        case TUPLE_UNKNOWN:
        default:
             return NULL;
        }
    }
    else if (expr != NULL)
    {
        if (expr->func(tuple, expression) == TRUE && argument != NULL)
            return tuple_formatter_process_construct(tuple, argument);
    }

    return NULL;
}

static GList *tuple_formatter_func_list = NULL;

typedef struct {
    const gchar *name;
    gchar *(*func)(Tuple *tuple, gchar **args);
} TupleFormatterFunction;

/* processes a function */
gchar *
tuple_formatter_process_function(Tuple *tuple, const gchar *expression, 
    const gchar *argument)
{
    TupleFormatterFunction *expr = NULL;
    GList *iter;

    g_return_val_if_fail(tuple != NULL, NULL);
    g_return_val_if_fail(expression != NULL, NULL);

    for (iter = tuple_formatter_func_list; iter != NULL; iter = iter->next)
    {
        TupleFormatterFunction *tmp = (TupleFormatterFunction *) iter->data;

        if (g_str_has_prefix(expression, tmp->name) == TRUE)
        {
            expr = tmp;
            expression += strlen(tmp->name);
        }
    }

    if (expr != NULL)
    {
        gchar **args;
        gchar *ret;

        if (argument)
            args = g_strsplit(argument, ",", 10);
        else
            args = NULL;

        ret = expr->func(tuple, args);

        if (args)
            g_strfreev(args);

        return ret;
    }

    return NULL;
}

/* registers a formatter */
void
tuple_formatter_register_expression(const gchar *keyword,
	gboolean (*func)(Tuple *tuple, const gchar *argument))
{
    TupleFormatterExpression *expr;

    g_return_if_fail(keyword != NULL);
    g_return_if_fail(func != NULL);

    expr = g_new0(TupleFormatterExpression, 1);
    expr->name = keyword;
    expr->func = func;

    tuple_formatter_expr_list = g_list_append(tuple_formatter_expr_list, expr);
}

/* registers a function */
void
tuple_formatter_register_function(const gchar *keyword,
	gchar *(*func)(Tuple *tuple, gchar **argument))
{
    TupleFormatterFunction *expr;

    g_return_if_fail(keyword != NULL);
    g_return_if_fail(func != NULL);

    expr = g_new0(TupleFormatterFunction, 1);
    expr->name = keyword;
    expr->func = func;

    tuple_formatter_func_list = g_list_append(tuple_formatter_func_list, expr);
}

/* builtin-keyword: ${?arg}, returns TRUE if <arg> exists. */
static gboolean
tuple_formatter_expression_exists(Tuple *tuple, const gchar *expression)
{
    return (tuple_get_value_type(tuple, expression) != TUPLE_UNKNOWN) ? TRUE : FALSE;
}

/* builtin-keyword: ${==arg1,arg2}, returns TRUE if <arg1> and <arg2> match.
   <arg1> and <arg2> can also be raw text, which should be enclosed in "double quotes". */
static gboolean
tuple_formatter_expression_match(Tuple *tuple, const gchar *expression)
{
    gchar **args = g_strsplit(expression, ",", 2);
    gchar *arg1 = NULL, *arg2 = NULL;
    gint ret;

    if (args[0][0] == '\"') /* check if arg1 is "raw text" */
    {
        if ( strlen(args[0]) > 1 )
        {
            args[0][strlen(args[0]) - 1] = '\0';
            arg1 = g_strdup(&(args[0][1]));
            args[0][strlen(args[0]) - 1] = '\"';
        }
        else /* bad formatted arg */
            return FALSE;
    }
    else if (tuple_get_value_type(tuple, args[0]) == TUPLE_UNKNOWN)
    {
        g_strfreev(args);
        return FALSE;
    }
    
    if (args[1][0] == '\"') /* check if arg2 is "raw text" */
    {
        if ( strlen(args[1]) > 1 )
        {
            args[1][strlen(args[1]) - 1] = '\0';
            arg2 = g_strdup(&(args[1][1]));
            args[1][strlen(args[1]) - 1] = '\"';
        }
        else /* bad formatted arg */
            return FALSE;
    }
    else if (tuple_get_value_type(tuple, args[1]) == TUPLE_UNKNOWN)
    {
        g_strfreev(args);
        return FALSE;
    }

    if (!arg1) /* if arg1 is not "raw text", get the tuple value */
    {
        if (tuple_get_value_type(tuple, args[0]) == TUPLE_STRING)
            arg1 = g_strdup(tuple_get_string(tuple, args[0]));
        else
            arg1 = g_strdup_printf("%d", tuple_get_int(tuple, args[0]));
    }

    if (!arg2) /* if arg2 is not "raw text", get the tuple value */
    {
        if (tuple_get_value_type(tuple, args[1]) == TUPLE_STRING)
            arg2 = g_strdup(tuple_get_string(tuple, args[1]));
        else
            arg2 = g_strdup_printf("%d", tuple_get_int(tuple, args[1]));
    }

    ret = g_ascii_strcasecmp(arg1, arg2);
    g_free(arg1);
    g_free(arg2);
    g_strfreev(args);

    return ret ? FALSE : TRUE;
}

/* builtin-keyword: ${!=arg1,arg2}. returns TRUE if <arg1> and <arg2> don't match.
   <arg1> and <arg2> can also be raw text, which should be enclosed in "double quotes". */
static gboolean
tuple_formatter_expression_nonmatch(Tuple *tuple, const gchar *expression)
{
    return tuple_formatter_expression_match(tuple, expression) ^ 1;
}

/* builtin-keyword: ${empty?}. returns TRUE if <arg> is empty. */
static gboolean
tuple_formatter_expression_empty(Tuple *tuple, const gchar *expression)
{
    gboolean ret = TRUE;
    const gchar *iter;
    TupleValueType type = tuple_get_value_type(tuple, expression);

    if (type == TUPLE_UNKNOWN)
        return TRUE;

    if (type == TUPLE_INT && tuple_get_int(tuple, expression) != 0)
        return FALSE;

    iter = tuple_get_string(tuple, expression);

    while (ret && *iter != '\0')
    {
        if (*iter == ' ')
            iter++;
        else
            ret = FALSE;
    }

    return ret;
}

/* builtin function: %{audacious-version} */
static gchar *
tuple_formatter_function_version(Tuple *tuple, gchar **args)
{
    return g_strdup(PACKAGE_NAME " " PACKAGE_VERSION);
}

/* processes a string containing instructions. does initialization phases
   if not already done */
gchar *
tuple_formatter_process_string(Tuple *tuple, const gchar *string)
{
    static gboolean initialized = FALSE;

    if (initialized == FALSE)
    {
        tuple_formatter_register_expression("?", tuple_formatter_expression_exists);
        tuple_formatter_register_expression("==", tuple_formatter_expression_match);
        tuple_formatter_register_expression("!=", tuple_formatter_expression_nonmatch);
        tuple_formatter_register_expression("(empty)?", tuple_formatter_expression_empty);

        tuple_formatter_register_function("audacious-version", tuple_formatter_function_version);
        initialized = TRUE;
    }

    return tuple_formatter_process_construct(tuple, string);
}

/* wrapper function for making title string. it falls back to filename
 * if process_string returns NULL or a blank string. */
gchar *
tuple_formatter_make_title_string(Tuple *tuple, const gchar *string)
{
    gchar *rv;

    g_return_val_if_fail(tuple != NULL, NULL);

    rv = tuple_formatter_process_string(tuple, string);

    if(!rv || !strcmp(rv, "")) {
        g_free(rv);
        rv = g_strdup(tuple_get_string(tuple, "file-name"));
    }

    return rv;
}
