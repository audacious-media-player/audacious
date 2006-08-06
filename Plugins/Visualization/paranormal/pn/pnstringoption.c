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
#include "pnstringoption.h"
#include "pnxml.h"
#include "pnerror.h"

static void         pn_string_option_class_init       (PnStringOptionClass *class);
static void         pn_string_option_init             (PnStringOption *string_option,
						       PnStringOptionClass *class);

/* PnUserObject methods */
static void         pn_string_option_save_thyself     (PnUserObject *user_object,
							xmlNodePtr node);
static void         pn_string_option_load_thyself     (PnUserObject *user_object,
							xmlNodePtr node);

static PnUserObjectClass *parent_class = NULL;
static gchar * const empty_string = "";

GType
pn_string_option_get_type (void)
{
  static GType string_option_type = 0;

  if (! string_option_type)
    {
      static const GTypeInfo string_option_info =
      {
	sizeof (PnStringOptionClass),
	NULL,		/* base_init */
	NULL,		/* base_finalize */
	(GClassInitFunc) pn_string_option_class_init,
	NULL,		/* class_finalize */
	NULL,		/* class_data */
        sizeof (PnStringOption),
	0,              /* n_preallocs */
	(GInstanceInitFunc) pn_string_option_init
      };

      /* FIXME: should this be dynamic? */
      string_option_type = g_type_register_static (PN_TYPE_OPTION,
					      "PnStringOption",
					      &string_option_info,
					      0);
    }
  return string_option_type;
}

static void
pn_string_option_class_init (PnStringOptionClass *class)
{
  PnObjectClass *object_class;
  PnUserObjectClass *user_object_class;
  PnOptionClass *option_class;

  parent_class = g_type_class_peek_parent (class);

  object_class = (PnObjectClass *) class;
  user_object_class = (PnUserObjectClass *) class;
  option_class = (PnOptionClass *) class;

  /* PnUserObject methods */
  user_object_class->save_thyself = pn_string_option_save_thyself;
  user_object_class->load_thyself = pn_string_option_load_thyself;

  /* PnOption methods */
  /* FIXME: this needs to be uncommented when the widget is done */
/*    option_class->widget_type = PN_TYPE_STRING_OPTION_WIDGET; */
}

static void
pn_string_option_init (PnStringOption *string_option, PnStringOptionClass *class)
{
  string_option->value = empty_string;
}

static void
pn_string_option_save_thyself (PnUserObject *user_object, xmlNodePtr node)
{
  PnStringOption *string_option;
  xmlNodePtr value_node;

  g_return_if_fail (user_object != NULL);
  g_return_if_fail (PN_IS_STRING_OPTION (user_object));
  g_return_if_fail (node != NULL);

  string_option = (PnStringOption *) user_object;
  value_node = xmlNewChild (node, NULL, "Value", NULL);
  xmlNodeSetContent (value_node, string_option->value);

  if (parent_class->save_thyself)
    parent_class->save_thyself (user_object, node);
}

static void
pn_string_option_load_thyself (PnUserObject *user_object, const xmlNodePtr node)
{
  PnStringOption *string_option;
  xmlNodePtr string_option_node;
  gchar *val_str;

  g_return_if_fail (user_object != NULL);
  g_return_if_fail (PN_IS_STRING_OPTION (user_object));
  g_return_if_fail (node != NULL);

  string_option = (PnStringOption *) user_object;

  for (string_option_node = node->xmlChildrenNode;
       string_option_node;
       string_option_node = string_option_node->next)
    if (g_strcasecmp (string_option_node->name, "Value") == 0)
      break;
  if (! string_option_node)
    {
      pn_error ("unable to load a PnStringOption from xml node \"%s\"", node->name);
      return;
    }

  val_str = xmlNodeGetContent (string_option_node);
  if (val_str)
    pn_string_option_set_value (string_option, val_str);
  else
    pn_string_option_set_value (string_option, empty_string);

  if (parent_class->load_thyself)
    parent_class->load_thyself (user_object, node);
}

PnStringOption*
pn_string_option_new (const gchar *name, const gchar *desc)
{
  PnStringOption *string_option;

  g_return_val_if_fail (name != NULL, NULL);
  g_return_val_if_fail (desc != NULL, NULL);

  string_option =   (PnStringOption *) g_object_new (PN_TYPE_STRING_OPTION, NULL);

  pn_user_object_set_name (PN_USER_OBJECT (string_option), name);
  pn_user_object_set_description (PN_USER_OBJECT (string_option), desc);

  return string_option;
}

void
pn_string_option_set_value (PnStringOption *string_option, const gchar *value)
{
  g_return_if_fail (string_option != NULL);
  g_return_if_fail (PN_IS_STRING_OPTION (string_option));
  g_return_if_fail (value != NULL);

  if (string_option->value && string_option->value != empty_string)
    g_free (string_option->value);

  string_option->value = g_strdup (value);
}

gchar*
pn_string_option_get_value (PnStringOption *string_option)
{
  g_return_val_if_fail (string_option != NULL, FALSE);
  g_return_val_if_fail (PN_IS_STRING_OPTION (string_option), FALSE);

  return string_option->value;
}
