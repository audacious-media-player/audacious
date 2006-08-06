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

#ifndef __PN_IMAGE_CONTEXT_H__
#define __PN_IMAGE_CONTEXT_H__

#include "pncontainer.h"

G_BEGIN_DECLS

enum
{
  PN_IMAGE_CONTEXT_OPT_INPUT_MODE = PN_CONTAINER_OPT_LAST,
  PN_IMAGE_CONTEXT_OPT_OUTPUT_MODE,
  PN_IMAGE_CONTEXT_OPT_LAST
};

#define PN_TYPE_IMAGE_CONTEXT              (pn_image_context_get_type ())
#define PN_IMAGE_CONTEXT(obj)              (G_TYPE_CHECK_INSTANCE_CAST ((obj), PN_TYPE_IMAGE_CONTEXT, PnImageContext))
#define PN_IMAGE_CONTEXT_CLASS(class)      (G_TYPE_CHECK_CLASS_CAST ((class), PN_TYPE_IMAGE_CONTEXT, PnImageContextClass))
#define PN_IS_IMAGE_CONTEXT(obj)           (G_TYPE_CHECK_INSTANCE_TYPE ((obj), PN_TYPE_IMAGE_CONTEXT))
#define PN_IS_IMAGE_CONTEXT_CLASS(class)   (G_TYPE_CHECK_CLASS_TYPE ((class), PN_TYPE_IMAGE_CONTEXT))
#define PN_IMAGE_CONTEXT_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS ((obj), PN_TYPE_IMAGE_CONTEXT, PnImageContextClass))

#define PN_IMAGE_CONTEXT_ACTUATORS(obj)    (PN_IMAGE_CONTEXT (obj)->actuators)

typedef struct _PnImageContext        PnImageContext;
typedef struct _PnImageContextClass   PnImageContextClass;

struct _PnImageContext
{
  PnContainer parent;

  PnImage *image;
};

struct _PnImageContextClass
{
  PnContainerClass parent_class;
};

/* Creators */
GType                 pn_image_context_get_type                (void);
PnImageContext       *pn_image_context_new                     (void);

#endif /* __PN_IMAGE_CONTEXT_H__ */
