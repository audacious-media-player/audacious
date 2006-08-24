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

#include "pnuserobject.h"

/* Initialization */
static void         pn_user_object_class_init        (PnUserObjectClass *class);

/* PnUserObject methods */
static void         pn_user_object_real_save_thyself (PnUserObject *user_object,
						      xmlNodePtr node);

static GObjectClass *parent_class = NULL;

GType
pn_user_object_get_type (void)
{
  static GType user_object_type = 0;

  if (! user_object_type)
    {
      static const GTypeInfo user_object_info =
      {
	sizeof (PnUserObjectClass),
	NULL,		/* base_init */
	NULL,		/* base_finalize */
	(GClassInitFunc) pn_user_object_class_init,
	NULL,		/* class_finalize */
	NULL,		/* class_data */
        sizeof (PnUserObject),
	0,              /* n_preallocs */
	NULL            /* instance_init */
      };

      /* FIXME: should this be dynamic? */
      user_object_type = g_type_register_static (PN_TYPE_OBJECT,
					    "PnUserObject",
					    &user_object_info,
					    G_TYPE_FLAG_ABSTRACT);
    }
  return user_object_type;
}

static void
pn_user_object_class_init (PnUserObjectClass *class)
{
  GObjectClass *gobject_class;
  PnObjectClass *object_class;

  gobject_class = (GObjectClass *) class;
  object_class = (PnObjectClass *) class;

  /* PnUserObject methods */
  class->save_thyself = pn_user_object_real_save_thyself;

  parent_class = g_type_class_peek_parent (class);
}

static void
pn_user_object_real_save_thyself (PnUserObject *user_object, xmlNodePtr node)
{
  g_return_if_fail (user_object != NULL);
  g_return_if_fail (PN_IS_USER_OBJECT (user_object));
  g_return_if_fail (node != NULL);

  xmlNodeSetName (node, (xmlChar *) user_object->name);
}

void
pn_user_object_set_name (PnUserObject *user_object, const gchar *name)
{
  g_return_if_fail (user_object != NULL);
  g_return_if_fail (PN_IS_USER_OBJECT (user_object));
  g_return_if_fail (name != NULL);

  if (user_object->name)
    g_free (user_object->name);

  user_object->name = g_strdup (name);
}

gchar*
pn_user_object_get_name (PnUserObject *user_object)
{
  g_return_val_if_fail (user_object != NULL, NULL);
  g_return_val_if_fail (PN_IS_USER_OBJECT (user_object), NULL);

  return user_object->name;
}

void
pn_user_object_set_description (PnUserObject *user_object, const gchar *desc)
{
  g_return_if_fail (user_object != NULL);
  g_return_if_fail (PN_IS_USER_OBJECT (user_object));
  g_return_if_fail (desc != NULL);

  if (user_object->desc)
    g_free (user_object->desc);

  user_object->desc = g_strdup (desc);
}

gchar*
pn_user_object_get_description (PnUserObject *user_object)
{
  g_return_val_if_fail (user_object != NULL, NULL);
  g_return_val_if_fail (PN_IS_USER_OBJECT (user_object), NULL);

  return user_object->desc;
}

void
pn_user_object_set_owner (PnUserObject *user_object, PnUserObject *owner)
{
  g_return_if_fail (user_object != NULL);
  g_return_if_fail (PN_IS_USER_OBJECT (user_object));
  g_return_if_fail (owner != NULL);
  g_return_if_fail (PN_IS_USER_OBJECT (owner));

  user_object->owner = owner;
}

PnUserObject*
pn_user_object_get_owner (PnUserObject *user_object)
{
  g_return_val_if_fail (user_object != NULL, NULL);
  g_return_val_if_fail (PN_IS_USER_OBJECT (user_object), NULL);

  return user_object->owner;
}

void
pn_user_object_save_thyself (PnUserObject *user_object, xmlNodePtr node)
{
  PnUserObjectClass *class;

  g_return_if_fail (user_object != NULL);
  g_return_if_fail (PN_IS_USER_OBJECT (user_object));
  g_return_if_fail (node != NULL);

  class = PN_USER_OBJECT_GET_CLASS (user_object);

  if (class->save_thyself)
    class->save_thyself (user_object, node);
}

void
pn_user_object_load_thyself (PnUserObject *user_object, xmlNodePtr node)
{
  PnUserObjectClass *class;

  g_return_if_fail (user_object != NULL);
  g_return_if_fail (PN_IS_USER_OBJECT (user_object));
  g_return_if_fail (node != NULL);

  class = PN_USER_OBJECT_GET_CLASS (user_object);

  if (class->load_thyself)
    class->load_thyself (user_object, node);
}
