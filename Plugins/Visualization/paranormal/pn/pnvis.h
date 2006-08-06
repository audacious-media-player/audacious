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

#ifndef __PN_VIS_H__
#define __PN_VIS_H__

#include <glib.h>
#include "pnuserobject.h"
#include "pnactuator.h"
#include "pnimage.h"
#include "pnaudiodata.h"


G_BEGIN_DECLS


#define PN_TYPE_VIS              (pn_vis_get_type ())
#define PN_VIS(obj)              (G_TYPE_CHECK_INSTANCE_CAST ((obj), PN_TYPE_VIS, PnVis))
#define PN_VIS_CLASS(class)      (G_TYPE_CHECK_CLASS_CAST ((class), PN_TYPE_VIS, PnVisClass))
#define PN_IS_VIS(obj)           (G_TYPE_CHECK_INSTANCE_TYPE ((obj), PN_TYPE_VIS))
#define PN_IS_VIS_CLASS(class)   (G_TYPE_CHECK_CLASS_TYPE ((class), PN_TYPE_VIS))
#define PN_VIS_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS ((obj), PN_TYPE_VIS, PnVisClass))

typedef struct _PnVis        PnVis;
typedef struct _PnVisClass   PnVisClass;

struct _PnVis
{
  PnUserObject parent;

  PnActuator *root_actuator;
  PnImage *root_image;
};

struct _PnVisClass
{
  PnUserObjectClass parent_class;
};

/* Creators */
GType                 pn_vis_get_type                (void);
PnVis                *pn_vis_new                     (guint image_width,
						      guint image_height);

/* Accessors */
void                  pn_vis_set_root_actuator       (PnVis *vis,
						      PnActuator *actuator);
PnActuator           *pn_vis_get_root_actuator       (PnVis *vis);
void                  pn_vis_set_image_size          (PnVis *vis,
						      guint width,
						      guint height);

/* Actions */
gboolean              pn_vis_save_to_file            (PnVis *vis,
						      const gchar *fname);
gboolean              pn_vis_load_from_file          (PnVis *vis,
						      const gchar *fname);
PnImage              *pn_vis_render                  (PnVis *vis,
						      PnAudioData *audio_data);





#endif /* __PN_VIS_H__ */
