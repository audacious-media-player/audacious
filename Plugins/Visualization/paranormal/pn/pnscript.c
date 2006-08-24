/* Paranormal - A highly customizable audio visualization library
 * Copyright (C) 2001  Jamie Gennis <jgennis@mindspring.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the Free
 * Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <config.h>

#include <math.h>
#include "pnscript.h"

/* Initialization */
static void         pn_script_class_init             (PnScriptClass *class);

/* Internal parser */
gboolean            pn_script_internal_parse_string  (PnScript *script,
						      const gchar *string);

static GObjectClass *parent_class = NULL;

GType
pn_script_get_type (void)
{
  static GType script_type = 0;

  if (! script_type)
    {
      static const GTypeInfo script_info =
      {
	sizeof (PnScriptClass),
	NULL,		/* base_init */
	NULL,		/* base_finalize */
	(GClassInitFunc) pn_script_class_init,
	NULL,		/* class_finalize */
	NULL,		/* class_data */
        sizeof (PnScript),
	0,              /* n_preallocs */
	NULL            /* instance_init */
      };

      /* FIXME: should this be dynamic? */
      script_type = g_type_register_static (PN_TYPE_OBJECT,
					    "PnScript",
					    &script_info,
					    0);
    }
  return script_type;
}

static void
pn_script_class_init (PnScriptClass *class)
{
  GObjectClass *gobject_class;
  PnObjectClass *object_class;

  gobject_class = (GObjectClass *) class;
  object_class = (PnObjectClass *) class;

  parent_class = g_type_class_peek_parent (class);
}

static void
pn_script_unref_variables (PnScript *script)
{
  g_return_if_fail (script != NULL);
  g_return_if_fail (PN_IS_SCRIPT (script));

  if (script->code && script->symbol_table)
    {
      guint *op;

      for (op = script->code; *op != PN_OP_END; op++)
	switch (*op)
	  {
	  case PN_OP_PUSHC:
	    op++;
	    break;

	  case PN_OP_PUSHV:
	    pn_symbol_table_unref_variable (script->symbol_table, PN_VARIABLE (*(++op)));
	    break;

	  case PN_OP_SET:
	    pn_symbol_table_unref_variable (script->symbol_table, PN_VARIABLE (*(++op)));
	    break;
	  }
    }
}

/**
 * pn_script_new
 *
 * Creates a new #PnScript object.
 *
 * Returns: The newly created #PnScript object
 */
PnScript*
pn_script_new (void)
{
  return (PnScript *) g_object_new (PN_TYPE_SCRIPT, NULL);
}

/**
 * pn_script_parse_string
 * @script: A #PnScript
 * @symbol_table: the #PnSymbolTable to associate with the script
 * @string: a string containing the script
 *
 * Parses a script, compiling it to a bytecode that is stored within the script object.
 * All in-script variables within the script are added to the specified symbol table (if
 * they are not already in it).  If errors are encountered while parsing the script,
 * they are output using pn_error().
 *
 * Returns: %TRUE on success; %FALSE otherwise
 */
gboolean
pn_script_parse_string (PnScript *script, PnSymbolTable *symbol_table, const gchar *string)
{
  g_return_val_if_fail (script != NULL, FALSE);
  g_return_val_if_fail (PN_IS_SCRIPT (script), FALSE);
  g_return_val_if_fail (symbol_table != NULL, FALSE);
  g_return_val_if_fail (PN_IS_SYMBOL_TABLE (symbol_table), FALSE);
  g_return_val_if_fail (string != NULL, FALSE);

  /* Make sure if it's the same symbol table, we don't destroy it */
  pn_object_ref (PN_OBJECT (symbol_table));
  pn_object_sink (PN_OBJECT (symbol_table));

  /* get rid of the old script */
  if (script->symbol_table)
    {
      pn_script_unref_variables (script);
      pn_object_unref (PN_OBJECT (script->symbol_table));
    }
  if (script->stack)
    {
      g_free (script->stack);
      script->stack = NULL;
    }
  if (script->code)
    {
      g_free (script->code);
      script->code = NULL;
    }
  if (script->constant_table)
    {
      g_free (script->constant_table);
      script->constant_table = NULL;
    }
 
  /* Set our new symbol table */
  script->symbol_table = symbol_table;

  return pn_script_internal_parse_string (script, string);
}

/**
 * pn_script_execute
 * @script: a #PnScript
 *
 * Executes a script, updating all variabes in the associated symbol
 * table as the script dictates.
 */
void
pn_script_execute (PnScript *script)
{
  guint *op;
  gdouble stack[64];
  guint stack_top = 0;
  guint temp;

  g_return_if_fail (script != NULL);
  g_return_if_fail (PN_IS_SCRIPT (script));

  if (! script->code)
    return;

  for (op = script->code; *op != PN_OP_END; op++)
    switch (*op)
      {
#define PUSH(f) stack[stack_top++] = f;
#define POP stack_top--;
#define POPV (stack[--stack_top])
#define PEEK (stack[stack_top-1])
#define PEEKN(n) (stack[stack_top-n])
      case PN_OP_PUSHC:
      case PN_OP_PUSHV:
	PUSH (* (gdouble *)(*(++op)));
	break;

      case PN_OP_POP:
	POP;
	break;

      case PN_OP_SET:
	*(gdouble *)(*(++op)) = PEEK;
	break;

      case PN_OP_ADD:
	/* PEEKN (2) += POPV; */
	temp = POPV;
	PEEKN(2) += temp;
	break;

      case PN_OP_SUB:
	/* PEEKN (2) -= POPV; */
	temp = POPV;
	PEEKN(2) -= temp;
	break;

      case PN_OP_MUL:
	/* PEEKN (2) *= POPV; */
	temp = POPV;
	PEEKN(2) *= temp;
	break;

      case PN_OP_DIV:
	if (PEEK != 0)
	  PEEKN (2) /= PEEK;
	else
	  PEEK = 0;
	POP;
	break;

      case PN_OP_NEG:
	PEEK = -PEEK;
	break;

      case PN_OP_POW:
	PEEKN (2) = pow (PEEKN (2), PEEK);
	POP;
	break;

      case PN_OP_ABS:
	PEEK = ABS (PEEK);
	break;

      case PN_OP_MAX:
	PEEKN (2) = MAX (PEEK, PEEKN (2));
	POP;
	break;

      case PN_OP_MIN:
	PEEKN (2) = MIN (PEEK, PEEKN (2));
	POP;
	break;

      case PN_OP_SIN:
	PEEK = sin (PEEK);
	break;

      case PN_OP_COS:
	PEEK = cos (PEEK);
	break;

      case PN_OP_TAN:
	PEEK = tan (PEEK);
	break;

      case PN_OP_ASIN:
	PEEK = asin (PEEK);
	break;

      case PN_OP_ACOS:
	PEEK = acos (PEEK);
	break;

      case PN_OP_ATAN:
	PEEK = atan (PEEK);
	break;
      }
}

