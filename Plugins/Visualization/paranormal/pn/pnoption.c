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

#include <glib.h>
#include "pnoption.h"

static void         pn_option_class_init       (PnOptionClass *class);
static void         pn_option_init             (PnOption *option,
						PnOptionClass *class);

/* GObject signals */
static void         pn_option_finalize         (GObject *gobject);

static PnUserObjectClass *parent_class = NULL;

GType
pn_option_get_type (void)
{
  static GType option_type = 0;

  if (! option_type)
    {
      static const GTypeInfo option_info =
      {
	sizeof (PnOptionClass),
	NULL,		/* base_init */
	NULL,		/* base_finalize */
	(GClassInitFunc) pn_option_class_init,
	NULL,		/* class_finalize */
	NULL,		/* class_data */
        sizeof (PnOption),
	0,              /* n_preallocs */
	(GInstanceInitFunc) pn_option_init
      };

      /* FIXME: should this be dynamic? */
      option_type = g_type_register_static (PN_TYPE_USER_OBJECT,
					      "PnOption",
					      &option_info,
					      G_TYPE_FLAG_ABSTRACT);
    }
  return option_type;
}

static void
pn_option_class_init (PnOptionClass *class)
{
  GObjectClass *gobject_class;
  PnObjectClass *object_class;
  PnUserObjectClass *user_object_class;
  

  parent_class = g_type_class_peek_parent (class);

  gobject_class = (GObjectClass *) class;
  object_class = (PnObjectClass *) class;
  user_object_class = (PnUserObjectClass *) class;

  /* GObject signals */
  gobject_class->finalize = pn_option_finalize;
}

static void
pn_option_init (PnOption *option, PnOptionClass *class)
{
}

static void
pn_option_finalize (GObject *gobject)
{
  PnOption *option;

  option = (PnOption *) gobject;

  if (G_OBJECT_CLASS (parent_class)->finalize)
    (* G_OBJECT_CLASS (parent_class)->finalize) (gobject);
}

PnOptionWidget*
pn_option_new_widget (PnOption *option)
{
  PnOptionClass *class;

  g_return_val_if_fail (option != NULL, NULL);
  g_return_val_if_fail (PN_IS_OPTION (option), NULL);

  class = PN_OPTION_GET_CLASS (option);

#ifndef PN_NO_GTK
  if (class->widget_type)
    return PN_OPTION_WIDGET (g_object_new (class->widget_type, NULL));
#endif /* PN_NO_GTK */

  return NULL;
}
