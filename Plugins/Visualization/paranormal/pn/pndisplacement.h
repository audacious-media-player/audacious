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

#ifndef __PN_DISPLACEMENT_H__
#define __PN_DISPLACEMENT_H__

#include "pnactuator.h"
#include "pnscript.h"
#include "pnsymboltable.h"


G_BEGIN_DECLS


enum
{
  PN_DISPLACEMENT_OPT_INIT_SCRIPT = PN_ACTUATOR_OPT_LAST,
  PN_DISPLACEMENT_OPT_FRAME_SCRIPT,
  PN_DISPLACEMENT_OPT_LAST
};

#define PN_TYPE_DISPLACEMENT              (pn_displacement_get_type ())
#define PN_DISPLACEMENT(obj)              (G_TYPE_CHECK_INSTANCE_CAST ((obj), PN_TYPE_DISPLACEMENT, PnDisplacement))
#define PN_DISPLACEMENT_CLASS(class)      (G_TYPE_CHECK_CLASS_CAST ((class), PN_TYPE_DISPLACEMENT, PnDisplacementClass))
#define PN_IS_DISPLACEMENT(obj)           (G_TYPE_CHECK_INSTANCE_TYPE ((obj), PN_TYPE_DISPLACEMENT))
#define PN_IS_DISPLACEMENT_CLASS(class)   (G_TYPE_CHECK_CLASS_TYPE ((class), PN_TYPE_DISPLACEMENT))
#define PN_DISPLACEMENT_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS ((obj), PN_TYPE_DISPLACEMENT, PnDisplacementClass))

typedef struct _PnDisplacement        PnDisplacement;
typedef struct _PnDisplacementClass   PnDisplacementClass;

struct _PnDisplacement
{
  PnActuator parent;

  /* The script objects */
  PnScript *init_script;
  PnScript *frame_script;
  PnSymbolTable *symbol_table;

  /* The in-script variables */
  PnVariable *x_var;
  PnVariable *y_var;
  PnVariable *volume_var;
};

struct _PnDisplacementClass
{
  PnActuatorClass parent_class;
};

/* Creators */
GType                  pn_displacement_get_type                (void);
PnDisplacement        *pn_displacement_new                     (void);

#endif /* __PN_DISPLACEMENT_H__ */
