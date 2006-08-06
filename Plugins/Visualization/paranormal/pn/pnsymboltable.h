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

#ifndef __PN_SYMBOL_TABLE_H__
#define __PN_SYMBOL_TABLE_H__

#include "pnobject.h"


G_BEGIN_DECLS


#define PN_TYPE_SYMBOL_TABLE              (pn_symbol_table_get_type ())
#define PN_SYMBOL_TABLE(obj)              (G_TYPE_CHECK_INSTANCE_CAST ((obj), PN_TYPE_SYMBOL_TABLE, PnSymbolTable))
#define PN_SYMBOL_TABLE_CLASS(class)      (G_TYPE_CHECK_CLASS_CAST ((class), PN_TYPE_SYMBOL_TABLE, PnSymbolTableClass))
#define PN_IS_SYMBOL_TABLE(obj)           (G_TYPE_CHECK_INSTANCE_TYPE ((obj), PN_TYPE_SYMBOL_TABLE))
#define PN_IS_SYMBOL_TABLE_CLASS(class)   (G_TYPE_CHECK_CLASS_TYPE ((class), PN_TYPE_SYMBOL_TABLE))
#define PN_SYMBOL_TABLE_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS ((obj), PN_TYPE_SYMBOL_TABLE, PnSymbolTableClass))
#define PN_SYMBOL_TABLE_CLASS_TYPE(class) (G_TYPE_FROM_CLASS (class))
#define PN_SYMBOL_TABLE_CLASS_NAME(class) (g_type_name (PN_SYMBOL_TABLE_CLASS_TYPE (class)))

#define PN_VARIABLE(var)                  ((PnVariable *) var)
#define PN_VARIABLE_VALUE(var)            (PN_VARIABLE (var)->value)
#define PN_VARIABLE_NAME(var)             (PN_VARIABLE (var)->name)

typedef struct _PnVariable           PnVariable;
typedef struct _PnSymbolTable        PnSymbolTable;
typedef struct _PnSymbolTableClass   PnSymbolTableClass;

struct _PnVariable
{
  /*< public >*/
  gdouble value; /* read-write */
  gchar *name;   /* read-only */

  /*< private >*/
  guint refs;
};

struct _PnSymbolTable
{
  PnObject parent;

  GList *variables;
};

struct _PnSymbolTableClass
{
  PnObjectClass parent_class;
};

/* Creators */
GType             pn_symbol_table_get_type             (void);
PnSymbolTable    *pn_symbol_table_new                  (void);

/* Accessors */
PnVariable       *pn_symbol_table_ref_variable_by_name (PnSymbolTable *symbol_table,
							const gchar *name);
void              pn_symbol_table_ref_variable         (PnSymbolTable *symbol_table,
							PnVariable *variable);
void              pn_symbol_table_unref_variable       (PnSymbolTable *symbol_table,
							PnVariable *variable);





#endif /* __PN_SYMBOL_TABLE_H__ */
