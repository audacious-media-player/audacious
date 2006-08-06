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

#ifndef __PN_DISTORTION_H__
#define __PN_DISTORTION_H__

#include "pnactuator.h"
#include "pnscript.h"
#include "pnsymboltable.h"


G_BEGIN_DECLS


enum
{
  PN_DISTORTION_OPT_POLAR_COORDS = PN_ACTUATOR_OPT_LAST,
  PN_DISTORTION_OPT_DISTORTION_SCRIPT,
  PN_DISTORTION_OPT_LAST
};

#define PN_TYPE_DISTORTION              (pn_distortion_get_type ())
#define PN_DISTORTION(obj)              (G_TYPE_CHECK_INSTANCE_CAST ((obj), PN_TYPE_DISTORTION, PnDistortion))
#define PN_DISTORTION_CLASS(class)      (G_TYPE_CHECK_CLASS_CAST ((class), PN_TYPE_DISTORTION, PnDistortionClass))
#define PN_IS_DISTORTION(obj)           (G_TYPE_CHECK_INSTANCE_TYPE ((obj), PN_TYPE_DISTORTION))
#define PN_IS_DISTORTION_CLASS(class)   (G_TYPE_CHECK_CLASS_TYPE ((class), PN_TYPE_DISTORTION))
#define PN_DISTORTION_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS ((obj), PN_TYPE_DISTORTION, PnDistortionClass))

typedef struct _PnPixelDistortion   PnPixelDistortion; 
typedef struct _PnDistortion        PnDistortion;
typedef struct _PnDistortionClass   PnDistortionClass;

struct _PnPixelDistortion
{
  guint offset;
  guchar nw; /* northwest weight */
  guchar ne; /* northeast weight */
  guchar sw; /* southwest weight */
  guchar se; /* southeast weight */
};

struct _PnDistortion
{
  PnActuator parent;

  /* The script objects */
  PnScript *distortion_script;
  PnSymbolTable *symbol_table;

  /* The in-script variables */
  PnVariable *x_var;
  PnVariable *y_var;
  PnVariable *r_var;
  PnVariable *theta_var;
  PnVariable *intensity_var;

  /* Distortion map */
  PnPixelDistortion *map;
};

struct _PnDistortionClass
{
  PnActuatorClass parent_class;
};

/* Creators */
GType                 pn_distortion_get_type                (void);
PnDistortion        *pn_distortion_new                     (void);

#endif /* __PN_DISTORTION_H__ */
