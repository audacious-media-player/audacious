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

#include <stdlib.h>
#include <limits.h>
#include <glib.h>
#include "pnintegeroption.h"
#include "pnxml.h"
#include "pnerror.h"

static void         pn_integer_option_class_init       (PnIntegerOptionClass *class);
static void         pn_integer_option_init             (PnIntegerOption *integer_option,
							PnIntegerOptionClass *class);

/* PnUserObject methods */
static void         pn_integer_option_save_thyself     (PnUserObject *user_object,
							xmlNodePtr node);
static void         pn_integer_option_load_thyself     (PnUserObject *user_object,
							xmlNodePtr node);

static PnUserObjectClass *parent_class = NULL;

GType
pn_integer_option_get_type (void)
{
  static GType integer_option_type = 0;

  if (! integer_option_type)
    {
      static const GTypeInfo integer_option_info =
      {
	sizeof (PnIntegerOptionClass),
	NULL,		/* base_init */
	NULL,		/* base_finalize */
	(GClassInitFunc) pn_integer_option_class_init,
	NULL,		/* class_finalize */
	NULL,		/* class_data */
        sizeof (PnIntegerOption),
	0,              /* n_preallocs */
	(GInstanceInitFunc) pn_integer_option_init
      };

      /* FIXME: should this be dynamic? */
      integer_option_type = g_type_register_static (PN_TYPE_OPTION,
					      "PnIntegerOption",
					      &integer_option_info,
					      0);
    }
  return integer_option_type;
}

static void
pn_integer_option_class_init (PnIntegerOptionClass *class)
{
  PnObjectClass *object_class;
  PnUserObjectClass *user_object_class;
  PnOptionClass *option_class;

  parent_class = g_type_class_peek_parent (class);

  object_class = (PnObjectClass *) class;
  user_object_class = (PnUserObjectClass *) class;
  option_class = (PnOptionClass *) class;

  /* PnUserObject methods */
  user_object_class->save_thyself = pn_integer_option_save_thyself;
  user_object_class->load_thyself = pn_integer_option_load_thyself;

  /* PnOption methods */
  /* FIXME: this needs to be uncommented when the widget is done */
/*    option_class->widget_type = PN_TYPE_INTEGER_OPTION_WIDGET; */
}

static void
pn_integer_option_init (PnIntegerOption *integer_option,
			PnIntegerOptionClass *class)
{
  integer_option->min = INT_MIN;
  integer_option->max = INT_MAX;
}

static void
pn_integer_option_save_thyself (PnUserObject *user_object, xmlNodePtr node)
{
  PnIntegerOption *integer_option;
  xmlNodePtr value_node;
  gchar str[16];

  g_return_if_fail (user_object != NULL);
  g_return_if_fail (PN_IS_INTEGER_OPTION (user_object));
  g_return_if_fail (node != NULL);

  integer_option = (PnIntegerOption *) user_object;

  value_node = xmlNewChild (node, NULL, (xmlChar *) "Value", NULL);
  sprintf (str, "%i", integer_option->value);
  xmlNodeSetContent (value_node, (xmlChar *) str);

  if (parent_class->save_thyself)
    parent_class->save_thyself (user_object, node);
}

static void
pn_integer_option_load_thyself (PnUserObject *user_object, const xmlNodePtr node)
{
  PnIntegerOption *integer_option;
  xmlNodePtr integer_option_node;
  gchar *val_str;

  g_return_if_fail (user_object != NULL);
  g_return_if_fail (PN_IS_INTEGER_OPTION (user_object));
  g_return_if_fail (node != NULL);

  integer_option = (PnIntegerOption *) user_object;

  /* find the node for this class */
  for (integer_option_node = node->xmlChildrenNode;
       integer_option_node;
       integer_option_node = integer_option_node->next)
    if (g_strcasecmp (integer_option_node->name, "Value") == 0)
      break;
  if (! integer_option_node)
    {
      pn_error ("unable to load a PnIntegerOption from xml node \"%s\"", node->name);
      return;
    }

  val_str = xmlNodeGetContent (integer_option_node);

  if (val_str)
    pn_integer_option_set_value (integer_option, strtol (val_str, NULL, 0));
  else
    {
      pn_error ("invalid integer option value encountered at xml node \"%s\"", node->name);
      return;
    }

  if (parent_class->load_thyself)
    parent_class->load_thyself (user_object, node);
}

PnIntegerOption*
pn_integer_option_new (const gchar *name, const gchar *desc)
{
  PnIntegerOption *integer_option;

  g_return_val_if_fail (name != NULL, NULL);
  g_return_val_if_fail (desc != NULL, NULL);

  integer_option =   (PnIntegerOption *) g_object_new (PN_TYPE_INTEGER_OPTION, NULL);

  pn_user_object_set_name (PN_USER_OBJECT (integer_option), name);
  pn_user_object_set_description (PN_USER_OBJECT (integer_option), desc);

  return integer_option;
}

void
pn_integer_option_set_value (PnIntegerOption *integer_option, gint value)
{
  g_return_if_fail (integer_option != NULL);
  g_return_if_fail (PN_IS_INTEGER_OPTION (integer_option));

  integer_option->value = CLAMP (value, integer_option->min, integer_option->max);
}

gint
pn_integer_option_get_value (PnIntegerOption *integer_option)
{
  g_return_val_if_fail (integer_option != NULL, FALSE);
  g_return_val_if_fail (PN_IS_INTEGER_OPTION (integer_option), FALSE);

  return integer_option->value;
}

void
pn_integer_option_set_min (PnIntegerOption *integer_option, gint min)
{
  g_return_if_fail (integer_option != NULL);
  g_return_if_fail (PN_IS_INTEGER_OPTION (integer_option));

  if (min > integer_option->max)
    {
      integer_option->min = integer_option->max;
      integer_option->max = min;
    }
  else
    integer_option->min = min;

  pn_integer_option_set_value (integer_option, integer_option->value);
}

gint
pn_integer_option_get_min (PnIntegerOption *integer_option)
{
  g_return_val_if_fail (integer_option != NULL, FALSE);
  g_return_val_if_fail (PN_IS_INTEGER_OPTION (integer_option), FALSE);

  return integer_option->min;
}

void
pn_integer_option_set_max (PnIntegerOption *integer_option, gint max)
{
  g_return_if_fail (integer_option != NULL);
  g_return_if_fail (PN_IS_INTEGER_OPTION (integer_option));

  if (max < integer_option->min)
    {
      integer_option->max = integer_option->min;
      integer_option->min = max;
    }
  else
    integer_option->max = max;

  pn_integer_option_set_value (integer_option, integer_option->value);
}

gint
pn_integer_option_get_max (PnIntegerOption *integer_option)
{
  g_return_val_if_fail (integer_option != NULL, FALSE);
  g_return_val_if_fail (PN_IS_INTEGER_OPTION (integer_option), FALSE);

  return integer_option->max;
}
