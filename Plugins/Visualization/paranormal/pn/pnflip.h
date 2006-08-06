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

#ifndef __PN_FLIP_H__
#define __PN_FLIP_H__

#include "pnactuator.h"
#include "pnscript.h"
#include "pnsymboltable.h"


G_BEGIN_DECLS

enum
{
  PN_FLIP_OPT_BLEND = PN_ACTUATOR_OPT_LAST,
  PN_FLIP_OPT_DIRECTION,
  PN_FLIP_OPT_LAST
};

#define PN_TYPE_FLIP              (pn_flip_get_type ())
#define PN_FLIP(obj)              (G_TYPE_CHECK_INSTANCE_CAST ((obj), PN_TYPE_FLIP, PnFlip))
#define PN_FLIP_CLASS(class)      (G_TYPE_CHECK_CLASS_CAST ((class), PN_TYPE_FLIP, PnFlipClass))
#define PN_IS_FLIP(obj)           (G_TYPE_CHECK_INSTANCE_TYPE ((obj), PN_TYPE_FLIP))
#define PN_IS_FLIP_CLASS(class)   (G_TYPE_CHECK_CLASS_TYPE ((class), PN_TYPE_FLIP))
#define PN_FLIP_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS ((obj), PN_TYPE_FLIP, PnFlipClass))

typedef struct _PnPixelFlip   PnPixelFlip; 
typedef struct _PnFlip        PnFlip;
typedef struct _PnFlipClass   PnFlipClass;

struct _PnFlip
{
  PnActuator parent;
};

struct _PnFlipClass
{
  PnActuatorClass parent_class;
};

/* Creators */
GType                 pn_flip_get_type                (void);
PnFlip               *pn_flip_new                     (void);

#endif /* __PN_FLIP_H__ */
