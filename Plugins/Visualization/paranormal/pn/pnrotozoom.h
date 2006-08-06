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

#ifndef __PN_ROTO_ZOOM_H__
#define __PN_ROTO_ZOOM_H__

#include "pnactuator.h"
#include "pnscript.h"
#include "pnsymboltable.h"

G_BEGIN_DECLS


enum
{
  PN_ROTO_ZOOM_OPT_INIT_SCRIPT = PN_ACTUATOR_OPT_LAST,
  PN_ROTO_ZOOM_OPT_FRAME_SCRIPT,
  PN_ROTO_ZOOM_OPT_LAST
};

#define PN_TYPE_ROTO_ZOOM              (pn_roto_zoom_get_type ())
#define PN_ROTO_ZOOM(obj)              (G_TYPE_CHECK_INSTANCE_CAST ((obj), PN_TYPE_ROTO_ZOOM, PnRotoZoom))
#define PN_ROTO_ZOOM_CLASS(class)      (G_TYPE_CHECK_CLASS_CAST ((class), PN_TYPE_ROTO_ZOOM, PnRotoZoomClass))
#define PN_IS_ROTO_ZOOM(obj)           (G_TYPE_CHECK_INSTANCE_TYPE ((obj), PN_TYPE_ROTO_ZOOM))
#define PN_IS_ROTO_ZOOM_CLASS(class)   (G_TYPE_CHECK_CLASS_TYPE ((class), PN_TYPE_ROTO_ZOOM))
#define PN_ROTO_ZOOM_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS ((obj), PN_TYPE_ROTO_ZOOM, PnRotoZoomClass))

typedef struct _PnRotoZoom        PnRotoZoom;
typedef struct _PnRotoZoomClass   PnRotoZoomClass;

struct _PnRotoZoom
{
  PnActuator parent;

  /* The script objects */
  PnScript *init_script;
  PnScript *frame_script;
  PnSymbolTable *symbol_table;

  /* The in-script variables */
  PnVariable *zoom_var;
  PnVariable *theta_var;
  PnVariable *volume_var;
};

struct _PnRotoZoomClass
{
  PnActuatorClass parent_class;
};

/* Creators */
GType                     pn_roto_zoom_get_type                (void);
PnRotoZoom               *pn_roto_zoom_new                     (void);

#endif /* __PN_ROTO_ZOOM_H__ */
