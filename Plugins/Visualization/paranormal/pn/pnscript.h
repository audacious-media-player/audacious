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

#ifndef __PN_SCRIPT_H__
#define __PN_SCRIPT_H__

#include "pnobject.h"
#include "pnsymboltable.h"


G_BEGIN_DECLS


/* Opcodes */
enum
{
  PN_OP_NOP,
  PN_OP_END,
  PN_OP_PUSHV, /* Push a variable */
  PN_OP_PUSHC, /* Push a constant */
  PN_OP_POP,
  PN_OP_SET,
  /* Arithmetic operators */
  PN_OP_ADD,
  PN_OP_SUB,
  PN_OP_MUL,
  PN_OP_DIV,
  PN_OP_NEG,
  PN_OP_POW,
  /* Functions */
  PN_OP_ABS,
  PN_OP_MAX,
  PN_OP_MIN,
  PN_OP_SIN,
  PN_OP_COS,
  PN_OP_TAN,
  PN_OP_ASIN,
  PN_OP_ACOS,
  PN_OP_ATAN,
  PN_OP_LAST
};

#define PN_TYPE_SCRIPT              (pn_script_get_type ())
#define PN_SCRIPT(obj)              (G_TYPE_CHECK_INSTANCE_CAST ((obj), PN_TYPE_SCRIPT, PnScript))
#define PN_SCRIPT_CLASS(class)      (G_TYPE_CHECK_CLASS_CAST ((class), PN_TYPE_SCRIPT, PnScriptClass))
#define PN_IS_SCRIPT(obj)           (G_TYPE_CHECK_INSTANCE_TYPE ((obj), PN_TYPE_SCRIPT))
#define PN_IS_SCRIPT_CLASS(class)   (G_TYPE_CHECK_CLASS_TYPE ((class), PN_TYPE_SCRIPT))
#define PN_SCRIPT_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS ((obj), PN_TYPE_SCRIPT, PnScriptClass))
#define PN_SCRIPT_CLASS_TYPE(class) (G_TYPE_FROM_CLASS (class))
#define PN_SCRIPT_CLASS_NAME(class) (g_type_name (PN_SCRIPT_CLASS_TYPE (class)))

typedef struct _PnScript        PnScript;
typedef struct _PnScriptClass   PnScriptClass;

struct _PnScript
{
  PnObject parent;

  /* The symbol table that is being used by the current code */
  PnSymbolTable *symbol_table;

  /* The table of constants used by the script */
  gdouble *constant_table;

  /* The byte-compiled code */
  guint32 *code;

  /* The script stack used during script execution */
  gdouble *stack;
};

struct _PnScriptClass
{
  PnObjectClass parent_class;
};

/* Creators */
GType             pn_script_get_type             (void);
PnScript         *pn_script_new                  (void);

/* Actions */
gboolean          pn_script_parse_string         (PnScript *script,
						  PnSymbolTable *symbol_table,
						  const gchar *string);
void              pn_script_execute              (PnScript *script);

#endif /* __PN_SCRIPT_H__ */
