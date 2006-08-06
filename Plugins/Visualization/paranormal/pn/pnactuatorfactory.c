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
#include "pnactuatorfactory.h"

static gboolean    pn_actuator_factory_initialized = FALSE;
static GHashTable *actuator_hash;

void
pn_actuator_factory_init (void)
{
  if (pn_actuator_factory_initialized == TRUE)
    return;

  /* FIXME: This should be done this way, but to avoid mixing glib version calls
   * when linked to both versions, we need to use functions that are in both
   * versions.
   *
   * uncomment this when glib-1.2 is no longer linked to by things like XMMS
   */
/*    actuator_hash = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, g_free); */
  actuator_hash = g_hash_table_new (g_str_hash, g_str_equal);

  pn_actuator_factory_initialized = TRUE;
}

void
pn_actuator_factory_register_actuator (const gchar *name, GType type)
{
  gchar *dup_name;
  GType *dup_type;

  g_return_if_fail (pn_actuator_factory_initialized == TRUE);
  g_return_if_fail (name != NULL);
  g_return_if_fail (g_type_is_a (type, PN_TYPE_ACTUATOR));

/*    if (pn_actuator_factory_is_registered (name)) */
/*      return; */

  dup_name = g_strdup (name);
  dup_type = g_new (GType, 1);
  *dup_type = type;

  g_hash_table_insert (actuator_hash, dup_name, dup_type);
}

void
pn_actuator_factory_unregister_actuator (const gchar *name)
{
  g_return_if_fail (pn_actuator_factory_initialized == TRUE);
  g_return_if_fail (name != NULL);

  g_hash_table_remove (actuator_hash, name);
}

gboolean
pn_actuator_factory_is_registered (const gchar *name)
{
  g_return_val_if_fail (pn_actuator_factory_initialized == TRUE, FALSE);
  g_return_val_if_fail (name != NULL, FALSE);

  return (g_hash_table_lookup (actuator_hash, name) != NULL);
}

PnActuator*
pn_actuator_factory_new_actuator (const gchar *name)
{
  GType *type;
  g_return_val_if_fail (pn_actuator_factory_initialized == TRUE, NULL);
  g_return_val_if_fail (name != NULL, NULL);

  type = (GType *) g_hash_table_lookup (actuator_hash, name);
  if (! type)
    return NULL;

  return (PnActuator *) g_object_new (*type, NULL);
}

PnActuator*
pn_actuator_factory_new_actuator_from_xml (xmlNodePtr node)
{
  PnActuator *actuator;

  actuator = pn_actuator_factory_new_actuator (node->name);
  if (! actuator)
    return NULL;

  pn_user_object_load_thyself (PN_USER_OBJECT (actuator), node);

  return actuator;
}
