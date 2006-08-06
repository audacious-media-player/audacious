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

#include "pnsymboltable.h"

/* Initialization */
static void         pn_symbol_table_class_init        (PnSymbolTableClass *class);

static GObjectClass *parent_class = NULL;



GType
pn_symbol_table_get_type (void)
{
  static GType symbol_table_type = 0;

  if (! symbol_table_type)
    {
      static const GTypeInfo symbol_table_info =
      {
	sizeof (PnSymbolTableClass),
	NULL,		/* base_init */
	NULL,		/* base_finalize */
	(GClassInitFunc) pn_symbol_table_class_init,
	NULL,		/* class_finalize */
	NULL,		/* class_data */
        sizeof (PnSymbolTable),
	0,              /* n_preallocs */
	NULL            /* instance_init */
      };

      /* FIXME: should this be dynamic? */
      symbol_table_type = g_type_register_static (PN_TYPE_OBJECT,
					    "PnSymbolTable",
					    &symbol_table_info,
					    0);
    }
  return symbol_table_type;
}

static void
pn_symbol_table_class_init (PnSymbolTableClass *class)
{
  GObjectClass *gobject_class;
  PnObjectClass *object_class;

  gobject_class = (GObjectClass *) class;
  object_class = (PnObjectClass *) class;

  parent_class = g_type_class_peek_parent (class);
}

/**
 * pn_symbol_table_new
 *
 * Creates a new #PnSymbolTable object.
 *
 * Returns: The newly created #PnSymbolTable object
 */
PnSymbolTable*
pn_symbol_table_new (void)
{
  return (PnSymbolTable *) g_object_new (PN_TYPE_SYMBOL_TABLE, NULL);
}

/**
 * pn_symbol_table_ref_variable_by_name
 * @symbol_table: a #PnSymbolTable
 * @name: the name a an in-script variable
 *
 * Retrieves an in-script variable contained within a symbol table and increments the
 * variable's reference count. If the variable is not yet in the symbol table, it is
 * added (with a value of 0.0).
 *
 * Returns: A pointer to the variable object
 */
PnVariable*
pn_symbol_table_ref_variable_by_name (PnSymbolTable *symbol_table, const gchar *name)
{
  GList *cur;
  PnVariable *variable;
  
  g_return_val_if_fail (symbol_table != NULL, NULL);
  g_return_val_if_fail (PN_IS_SYMBOL_TABLE (symbol_table), NULL);
  g_return_val_if_fail (name != NULL, NULL);

  /* If the variable already exists, return it */
  for (cur = symbol_table->variables; cur; cur = cur->next)
    if (g_strcasecmp (PN_VARIABLE (cur->data)->name, name) == 0)
      {
	PN_VARIABLE (cur->data)->refs++;
	return PN_VARIABLE (cur->data);
      }

  /* We need to make a new variable */
  variable = g_new (PnVariable, 1);
  variable->value = 0.0;
  variable->name = g_strdup (name);
  variable->refs = 1;

  /* Add the variable to the list */
  symbol_table->variables = g_list_prepend (symbol_table->variables, variable);

  return variable;
}

/**
 * pn_symbol_table_ref_variable
 * @symbol_table: a #PnSymbolTable
 * @variable: an in-script variable belonging to @symbol_table
 *
 * Increments the reference count of an in-script variable contained within
 * a symbol table.
 */
void
pn_symbol_table_ref_variable (PnSymbolTable *symbol_table, PnVariable *variable)
{
  g_return_if_fail (symbol_table != NULL);
  g_return_if_fail (PN_IS_SYMBOL_TABLE (symbol_table));
  g_return_if_fail (variable != NULL);

  /* FIXME: Check to ensure the variable belongs to the symtab */

  variable->refs++;
}

/**
 * pn_symbol_table_unref_variable
 * @symbol_table: a #PnSymbolTable
 * @variable: an in-script variable belonging to @symbol_table
 *
 * Decrements the reference count of an in-script variable.  If the reference
 * count becomes 0, the variable is freed from memory.
 */ 
void
pn_symbol_table_unref_variable (PnSymbolTable *symbol_table, PnVariable *variable)
{
  g_return_if_fail (symbol_table != NULL);
  g_return_if_fail (PN_IS_SYMBOL_TABLE (symbol_table));
  g_return_if_fail (variable != NULL);

  variable->refs--;

  if (variable->refs <= 0)
    {
      g_free (variable->name);
      g_free (variable);
      symbol_table->variables = g_list_remove (symbol_table->variables, variable);
    }
}
