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

#ifndef __PN_SCOPE_H__
#define __PN_SCOPE_H__

#include "pnactuator.h"
#include "pnscript.h"
#include "pnsymboltable.h"


G_BEGIN_DECLS


enum
{
  PN_SCOPE_OPT_DRAW_METHOD = PN_ACTUATOR_OPT_LAST,
  PN_SCOPE_OPT_INIT_SCRIPT,
  PN_SCOPE_OPT_FRAME_SCRIPT,
  PN_SCOPE_OPT_SAMPLE_SCRIPT,
  PN_SCOPE_OPT_LAST
};

#define PN_TYPE_SCOPE              (pn_scope_get_type ())
#define PN_SCOPE(obj)              (G_TYPE_CHECK_INSTANCE_CAST ((obj), PN_TYPE_SCOPE, PnScope))
#define PN_SCOPE_CLASS(class)      (G_TYPE_CHECK_CLASS_CAST ((class), PN_TYPE_SCOPE, PnScopeClass))
#define PN_IS_SCOPE(obj)           (G_TYPE_CHECK_INSTANCE_TYPE ((obj), PN_TYPE_SCOPE))
#define PN_IS_SCOPE_CLASS(class)   (G_TYPE_CHECK_CLASS_TYPE ((class), PN_TYPE_SCOPE))
#define PN_SCOPE_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS ((obj), PN_TYPE_SCOPE, PnScopeClass))

typedef struct _PnScope        PnScope;
typedef struct _PnScopeClass   PnScopeClass;

struct _PnScope
{
  PnActuator parent;

  /* The script objects */
  PnScript *init_script;
  PnScript *frame_script;
  PnScript *sample_script;
  PnSymbolTable *symbol_table;

  /* The in-script variables */
  PnVariable *samples_var;
  PnVariable *width_var;
  PnVariable *height_var;
  PnVariable *x_var;
  PnVariable *y_var;
  PnVariable *iteration_var;
  PnVariable *value_var;
  PnVariable *red_var;
  PnVariable *green_var;
  PnVariable *blue_var;
  PnVariable *volume_var;
};

struct _PnScopeClass
{
  PnActuatorClass parent_class;
};

/* Creators */
GType                 pn_scope_get_type                (void);
PnScope              *pn_scope_new                     (void);





#endif /* __PN_SCOPE_H__ */
