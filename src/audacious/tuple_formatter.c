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

#include "tuple.h"
#include "tuple_formatter.h"

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
 *
 * everything else is treated as raw text.
 * additionally, plugins can add additional instructions!
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

    g_return_val_if_fail(tuple != NULL, NULL);
    g_return_val_if_fail(string != NULL, NULL);

    ctx = g_new0(TupleFormatterContext, 1);
    ctx->str = g_string_new("");

    /* parsers are ugly */
    for (iter = string; *iter != '\0'; iter++)
    {
        /* if it's raw text, just copy the byte */
        if (*iter != '$')
            g_string_append_c(ctx->str, *iter);
        else if (*(iter + 1) == '{')
        {
            GString *expression = g_string_new("");
            GString *argument = g_string_new("");
            GString *sel = expression;
            gchar *result;
            gint level = 0;

            for (iter += 2; *iter != '\0'; iter++)
            {
                if (*iter == ':')
                {
                    sel = argument;
                    continue;
                }

                if (g_str_has_prefix(iter, "${") == TRUE)
                {
                    if (sel == argument)
                    {
                        g_string_append_c(sel, *iter);
                        level++;
                    }
                }
                else if (*iter == '}' && (sel == argument && --level != 0))
                    g_string_append_c(sel, *iter);
                else if (*iter == '}' && ((sel != argument) || (sel == argument && level == 0)))
                {
                    if (sel == argument)
                        iter++;
                    break;
                }
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
    }

    out = g_strdup(ctx->str->str);
    g_string_free(ctx->str, TRUE);
    g_free(ctx);

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

/* builtin-keyword: ${?arg}, returns TRUE if <arg> exists. */
static gboolean
tuple_formatter_expression_exists(Tuple *tuple, const gchar *expression)
{
    return (tuple_get_value_type(tuple, expression) != TUPLE_UNKNOWN) ? TRUE : FALSE;
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
        initialized = TRUE;
    }

    return tuple_formatter_process_construct(tuple, string);
}
