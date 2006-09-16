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

#include <ctype.h>
#include <glib.h>
#include "pnbooleanoption.h"
#include "pnxml.h"
#include "pnerror.h"

static void         pn_boolean_option_class_init       (PnBooleanOptionClass *class);

/* PnUserObject methods */
static void         pn_boolean_option_save_thyself     (PnUserObject *user_object,
							xmlNodePtr node);
static void         pn_boolean_option_load_thyself     (PnUserObject *user_object,
							xmlNodePtr node);

static PnUserObjectClass *parent_class = NULL;

GType
pn_boolean_option_get_type (void)
{
  static GType boolean_option_type = 0;

  if (! boolean_option_type)
    {
      static const GTypeInfo boolean_option_info =
      {
	sizeof (PnBooleanOptionClass),
	NULL,		/* base_init */
	NULL,		/* base_finalize */
	(GClassInitFunc) pn_boolean_option_class_init,
	NULL,		/* class_finalize */
	NULL,		/* class_data */
        sizeof (PnBooleanOption),
	0,              /* n_preallocs */
	NULL            /* instance_init */
      };

      /* FIXME: should this be dynamic? */
      boolean_option_type = g_type_register_static (PN_TYPE_OPTION,
					      "PnBooleanOption",
					      &boolean_option_info,
					      0);
    }
  return boolean_option_type;
}

static void
pn_boolean_option_class_init (PnBooleanOptionClass *class)
{
  PnObjectClass *object_class;
  PnUserObjectClass *user_object_class;
  PnOptionClass *option_class;

  parent_class = g_type_class_peek_parent (class);

  object_class = (PnObjectClass *) class;
  user_object_class = (PnUserObjectClass *) class;
  option_class = (PnOptionClass *) class;

  /* PnUserObject methods */
  user_object_class->save_thyself = pn_boolean_option_save_thyself;
  user_object_class->load_thyself = pn_boolean_option_load_thyself;

  /* PnOption methods */
  /* FIXME: this needs to be uncommented when the widget is done */
/*    option_class->widget_type = PN_TYPE_BOOLEAN_OPTION_WIDGET; */
}

static void
pn_boolean_option_save_thyself (PnUserObject *user_object, xmlNodePtr node)
{
  PnBooleanOption *boolean_option;
  xmlNodePtr value_node;

  g_return_if_fail (user_object != NULL);
  g_return_if_fail (PN_IS_BOOLEAN_OPTION (user_object));
  g_return_if_fail (node != NULL);

  boolean_option = (PnBooleanOption *) user_object;

  value_node = xmlNewChild (node, NULL, (xmlChar *) "Value", NULL);

  if (boolean_option->value)
    xmlNodeSetContent (value_node, (xmlChar *) "True");
  else
    xmlNodeSetContent (value_node, (xmlChar *) "False");

  if (parent_class->save_thyself)
    parent_class->save_thyself (user_object, node);
}

static void
pn_boolean_option_load_thyself (PnUserObject *user_object, const xmlNodePtr node)
{
  PnBooleanOption *boolean_option;
  xmlNodePtr boolean_option_node;
  gchar *val_str;

  g_return_if_fail (user_object != NULL);
  g_return_if_fail (PN_IS_BOOLEAN_OPTION (user_object));
  g_return_if_fail (node != NULL);

  boolean_option = (PnBooleanOption *) user_object;

  /* find the node for this class */
  for (boolean_option_node = node->xmlChildrenNode;
       boolean_option_node;
       boolean_option_node = boolean_option_node->next)
    if (g_strcasecmp ((gchar *) boolean_option_node->name, "Value") == 0)
      break;
  if (! boolean_option_node)
    {
      pn_error ("unable to load a PnBooleanOption from xml node \"%s\"", node->name);
      return;
    }

  val_str = (gchar *)xmlNodeGetContent (boolean_option_node);
  if (! val_str)
    goto done;

  while (isspace ((int) *val_str))
    val_str++;

  if (g_strncasecmp (val_str, "True", 4) == 0)
    boolean_option->value = TRUE;
  else if (g_strncasecmp (val_str, "False", 5) == 0)
    boolean_option->value = FALSE;
  else
    {
      pn_error ("invalid boolean option value encountered at xml node \"%s\"", node->name);
      return;
    }

 done:
  if (parent_class->load_thyself)
    parent_class->load_thyself (user_object, node);
}

PnBooleanOption*
pn_boolean_option_new (const gchar *name, const gchar *desc)
{
  PnBooleanOption *boolean_option;

  g_return_val_if_fail (name != NULL, NULL);
  g_return_val_if_fail (desc != NULL, NULL);

  boolean_option =   (PnBooleanOption *) g_object_new (PN_TYPE_BOOLEAN_OPTION, NULL);

  pn_user_object_set_name (PN_USER_OBJECT (boolean_option), name);
  pn_user_object_set_description (PN_USER_OBJECT (boolean_option), desc);

  return boolean_option;
}

void
pn_boolean_option_set_value (PnBooleanOption *boolean_option, gboolean value)
{
  g_return_if_fail (boolean_option != NULL);
  g_return_if_fail (PN_IS_BOOLEAN_OPTION (boolean_option));

  boolean_option->value = value;
}

gboolean
pn_boolean_option_get_value (PnBooleanOption *boolean_option)
{
  g_return_val_if_fail (boolean_option != NULL, FALSE);
  g_return_val_if_fail (PN_IS_BOOLEAN_OPTION (boolean_option), FALSE);

  return boolean_option->value;
}
