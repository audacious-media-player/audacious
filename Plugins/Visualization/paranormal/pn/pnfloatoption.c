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
#include "pnfloatoption.h"
#include "pnxml.h"
#include "pnerror.h"

static void         pn_float_option_class_init       (PnFloatOptionClass *class);
static void         pn_float_option_init             (PnFloatOption *float_option,
							PnFloatOptionClass *class);

/* PnUserObject methods */
static void         pn_float_option_save_thyself     (PnUserObject *user_object,
							xmlNodePtr node);
static void         pn_float_option_load_thyself     (PnUserObject *user_object,
							xmlNodePtr node);

static PnUserObjectClass *parent_class = NULL;

GType
pn_float_option_get_type (void)
{
  static GType float_option_type = 0;

  if (! float_option_type)
    {
      static const GTypeInfo float_option_info =
      {
	sizeof (PnFloatOptionClass),
	NULL,		/* base_init */
	NULL,		/* base_finalize */
	(GClassInitFunc) pn_float_option_class_init,
	NULL,		/* class_finalize */
	NULL,		/* class_data */
        sizeof (PnFloatOption),
	0,              /* n_preallocs */
	(GInstanceInitFunc) pn_float_option_init
      };

      /* FIXME: should this be dynamic? */
      float_option_type = g_type_register_static (PN_TYPE_OPTION,
					      "PnFloatOption",
					      &float_option_info,
					      0);
    }
  return float_option_type;
}

static void
pn_float_option_class_init (PnFloatOptionClass *class)
{
  PnObjectClass *object_class;
  PnUserObjectClass *user_object_class;
  PnOptionClass *option_class;

  parent_class = g_type_class_peek_parent (class);

  object_class = (PnObjectClass *) class;
  user_object_class = (PnUserObjectClass *) class;
  option_class = (PnOptionClass *) class;

  /* PnUserObject methods */
  user_object_class->save_thyself = pn_float_option_save_thyself;
  user_object_class->load_thyself = pn_float_option_load_thyself;

  /* PnOption methods */
  /* FIXME: this needs to be uncommented when the widget is done */
/*    option_class->widget_type = PN_TYPE_FLOAT_OPTION_WIDGET; */
}

static void
pn_float_option_init (PnFloatOption *float_option,
			PnFloatOptionClass *class)
{
  float_option->min = INT_MIN;
  float_option->max = INT_MAX;
}

static void
pn_float_option_save_thyself (PnUserObject *user_object, xmlNodePtr node)
{
  PnFloatOption *float_option;
  xmlNodePtr value_node;
  gchar str[16];

  g_return_if_fail (user_object != NULL);
  g_return_if_fail (PN_IS_FLOAT_OPTION (user_object));
  g_return_if_fail (node != NULL);

  float_option = (PnFloatOption *) user_object;

  value_node = xmlNewChild (node, NULL, "Value", NULL);
  sprintf (str, "%f", float_option->value);
  xmlNodeSetContent (value_node, str);

  if (parent_class->save_thyself)
    parent_class->save_thyself (user_object, node);
}

static void
pn_float_option_load_thyself (PnUserObject *user_object, const xmlNodePtr node)
{
  PnFloatOption *float_option;
  xmlNodePtr float_option_node;
  gchar *val_str;

  g_return_if_fail (user_object != NULL);
  g_return_if_fail (PN_IS_FLOAT_OPTION (user_object));
  g_return_if_fail (node != NULL);

  float_option = (PnFloatOption *) user_object;

  /* find the node for this class */
  for (float_option_node = node->xmlChildrenNode;
       float_option_node;
       float_option_node = float_option_node->next)
    if (g_strcasecmp (float_option_node->name, "Value") == 0)
      break;
  if (! float_option_node)
    {
      pn_error ("unable to load a PnFloatOption from xml node \"%s\"", node->name);
      return;
    }

  val_str = xmlNodeGetContent (float_option_node);

  if (val_str)
    pn_float_option_set_value (float_option, strtod (val_str, NULL));
  else
    {
      pn_error ("invalid float option value encountered at xml node \"%s\"", node->name);
      return;
    }

  if (parent_class->load_thyself)
    parent_class->load_thyself (user_object, node);
}

PnFloatOption*
pn_float_option_new (const gchar *name, const gchar *desc)
{
  PnFloatOption *float_option;

  g_return_val_if_fail (name != NULL, NULL);
  g_return_val_if_fail (desc != NULL, NULL);

  float_option =   (PnFloatOption *) g_object_new (PN_TYPE_FLOAT_OPTION, NULL);

  pn_user_object_set_name (PN_USER_OBJECT (float_option), name);
  pn_user_object_set_description (PN_USER_OBJECT (float_option), desc);

  return float_option;
}

void
pn_float_option_set_value (PnFloatOption *float_option, gfloat value)
{
  g_return_if_fail (float_option != NULL);
  g_return_if_fail (PN_IS_FLOAT_OPTION (float_option));

  float_option->value = CLAMP (value, float_option->min, float_option->max);
}

gfloat
pn_float_option_get_value (PnFloatOption *float_option)
{
  g_return_val_if_fail (float_option != NULL, FALSE);
  g_return_val_if_fail (PN_IS_FLOAT_OPTION (float_option), FALSE);

  return float_option->value;
}

void
pn_float_option_set_min (PnFloatOption *float_option, gfloat min)
{
  g_return_if_fail (float_option != NULL);
  g_return_if_fail (PN_IS_FLOAT_OPTION (float_option));

  if (min > float_option->max)
    {
      float_option->min = float_option->max;
      float_option->max = min;
    }
  else
    float_option->min = min;

  pn_float_option_set_value (float_option, float_option->value);
}

gfloat
pn_float_option_get_min (PnFloatOption *float_option)
{
  g_return_val_if_fail (float_option != NULL, FALSE);
  g_return_val_if_fail (PN_IS_FLOAT_OPTION (float_option), FALSE);

  return float_option->min;
}

void
pn_float_option_set_max (PnFloatOption *float_option, gfloat max)
{
  g_return_if_fail (float_option != NULL);
  g_return_if_fail (PN_IS_FLOAT_OPTION (float_option));

  if (max < float_option->min)
    {
      float_option->max = float_option->min;
      float_option->min = max;
    }
  else
    float_option->max = max;

  pn_float_option_set_value (float_option, float_option->value);
}

gfloat
pn_float_option_get_max (PnFloatOption *float_option)
{
  g_return_val_if_fail (float_option != NULL, FALSE);
  g_return_val_if_fail (PN_IS_FLOAT_OPTION (float_option), FALSE);

  return float_option->max;
}
