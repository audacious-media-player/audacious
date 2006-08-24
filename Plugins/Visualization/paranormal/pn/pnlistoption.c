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
#include "pnlistoption.h"
#include "pnxml.h"
#include "pnerror.h"

static void         pn_list_option_class_init       (PnListOptionClass *class);
static void         pn_list_option_init             (PnListOption *list_option,
						     PnListOptionClass *class);

/* PnUserObject methods */
static void         pn_list_option_save_thyself     (PnUserObject *user_object,
							xmlNodePtr node);
static void         pn_list_option_load_thyself     (PnUserObject *user_object,
							xmlNodePtr node);

static PnUserObjectClass *parent_class = NULL;

GType
pn_list_option_get_type (void)
{
  static GType list_option_type = 0;

  if (! list_option_type)
    {
      static const GTypeInfo list_option_info =
      {
	sizeof (PnListOptionClass),
	NULL,		/* base_init */
	NULL,		/* base_finalize */
	(GClassInitFunc) pn_list_option_class_init,
	NULL,		/* class_finalize */
	NULL,		/* class_data */
        sizeof (PnListOption),
	0,              /* n_preallocs */
	(GInstanceInitFunc) pn_list_option_init
      };

      /* FIXME: should this be dynamic? */
      list_option_type = g_type_register_static (PN_TYPE_OPTION,
					      "PnListOption",
					      &list_option_info,
					      0);
    }
  return list_option_type;
}

static void
pn_list_option_class_init (PnListOptionClass *class)
{
  PnObjectClass *object_class;
  PnUserObjectClass *user_object_class;
  PnOptionClass *option_class;

  parent_class = g_type_class_peek_parent (class);

  object_class = (PnObjectClass *) class;
  user_object_class = (PnUserObjectClass *) class;
  option_class = (PnOptionClass *) class;

  /* PnUserObject methods */
  user_object_class->save_thyself = pn_list_option_save_thyself;
  user_object_class->load_thyself = pn_list_option_load_thyself;

  /* PnOption methods */
  /* FIXME: this needs to be uncommented when the widget is done */
/*    option_class->widget_type = PN_TYPE_LIST_OPTION_WIDGET; */
}

static void
pn_list_option_init (PnListOption *list_option, PnListOptionClass *class)
{
  list_option->items = g_array_new (FALSE, FALSE, sizeof (const gchar *));
  list_option->index = -1;
}

static void
pn_list_option_save_thyself (PnUserObject *user_object, xmlNodePtr node)
{
  PnListOption *list_option;
  xmlNodePtr value_node;

  g_return_if_fail (user_object != NULL);
  g_return_if_fail (PN_IS_LIST_OPTION (user_object));
  g_return_if_fail (node != NULL);

  list_option = (PnListOption *) user_object;

  value_node = xmlNewChild (node, NULL, (xmlChar *) "Value", NULL);
  xmlNodeSetContent (value_node, (xmlChar *) g_array_index (list_option->items, const gchar *, list_option->index));

  if (parent_class->save_thyself)
    parent_class->save_thyself (user_object, node);
}

static void
pn_list_option_load_thyself (PnUserObject *user_object, const xmlNodePtr node)
{
  PnListOption *list_option;
  xmlNodePtr list_option_node;
  gchar *val_str;
  guint i;

  g_return_if_fail (user_object != NULL);
  g_return_if_fail (PN_IS_LIST_OPTION (user_object));
  g_return_if_fail (node != NULL);

  list_option = (PnListOption *) user_object;

  /* find the node for this class */
  for (list_option_node = node->xmlChildrenNode;
       list_option_node;
       list_option_node = list_option_node->next)
    if (g_strcasecmp (list_option_node->name, "Value") == 0)
      break;
  if (! list_option_node)
    {
      pn_error ("unable to load a PnListOption from xml node \"%s\"", node->name);
      return;
    }

  val_str = xmlNodeGetContent (list_option_node);
  if (! val_str)
    goto done;

  for (i=0; i < list_option->items->len; i++)
    if (g_strcasecmp (val_str, g_array_index (list_option->items, const gchar *, i)) == 0)
      {
	list_option->index = i;
	goto done;
      }

  pn_error ("invalid list option value encountered at xml node \"%s\"", node->name);
  return;

 done:
  if (parent_class->load_thyself)
    parent_class->load_thyself (user_object, node);
}

PnListOption*
pn_list_option_new (const gchar *name, const gchar *desc)
{
  PnListOption *list_option;

  g_return_val_if_fail (name != NULL, NULL);
  g_return_val_if_fail (desc != NULL, NULL);

  list_option = (PnListOption *) g_object_new (PN_TYPE_LIST_OPTION, NULL);

  pn_user_object_set_name (PN_USER_OBJECT (list_option), name);
  pn_user_object_set_description (PN_USER_OBJECT (list_option), desc);

  return list_option;
}

void
pn_list_option_add_item (PnListOption *list_option, const gchar *item)
{
  g_return_if_fail (list_option != NULL);
  g_return_if_fail (PN_IS_LIST_OPTION (list_option));
  g_return_if_fail (item != NULL);

  g_array_append_val (list_option->items, item);

  if (list_option->index < 0)
    list_option->index = 0;
}

void
pn_list_option_set_index (PnListOption *list_option, guint index)
{
  g_return_if_fail (list_option != NULL);
  g_return_if_fail (PN_IS_LIST_OPTION (list_option));
  g_return_if_fail (index < list_option->items->len);

  list_option->index = index;
}

gint
pn_list_option_get_index (PnListOption *list_option)
{
  g_return_val_if_fail (list_option != NULL, FALSE);
  g_return_val_if_fail (PN_IS_LIST_OPTION (list_option), FALSE);

  return list_option->index;
}
