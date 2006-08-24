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
#include "pnactuator.h"
#include "pnerror.h"

static void         pn_actuator_class_init       (PnActuatorClass *class);
static void         pn_actuator_init             (PnActuator *actuator,
						  PnActuatorClass *class);

/* GObject signals */
static void         pn_actuator_finalize         (GObject *gobject);

/* PnObject signals */
static void         pn_actuator_destroy          (PnObject *object);

/* PnUserObject methods */
static void         pn_actuator_save_thyself     (PnUserObject *user_object,
						  xmlNodePtr node);
static void         pn_actuator_load_thyself     (PnUserObject *user_object,
						  xmlNodePtr node);

static PnUserObjectClass *parent_class = NULL;

GType
pn_actuator_get_type (void)
{
  static GType actuator_type = 0;

  if (! actuator_type)
    {
      static const GTypeInfo actuator_info =
      {
	sizeof (PnActuatorClass),
	NULL,		/* base_init */
	NULL,		/* base_finalize */
	(GClassInitFunc) pn_actuator_class_init,
	NULL,		/* class_finalize */
	NULL,		/* class_data */
        sizeof (PnActuator),
	0,              /* n_preallocs */
	(GInstanceInitFunc) pn_actuator_init
      };

      /* FIXME: should this be dynamic? */
      actuator_type = g_type_register_static (PN_TYPE_USER_OBJECT,
					      "PnActuator",
					      &actuator_info,
					      G_TYPE_FLAG_ABSTRACT);
    }
  return actuator_type;
}

static void
pn_actuator_class_init (PnActuatorClass *class)
{
  GObjectClass *gobject_class;
  PnObjectClass *object_class;
  PnUserObjectClass *user_object_class;
  

  parent_class = g_type_class_peek_parent (class);

  gobject_class = (GObjectClass *) class;
  object_class = (PnObjectClass *) class;
  user_object_class = (PnUserObjectClass *) class;

  /* GObject signals */
  gobject_class->finalize = pn_actuator_finalize;

  /* PnObject signals */
  object_class->destroy = pn_actuator_destroy;

  /* PnUserObject methods */
  user_object_class->save_thyself = pn_actuator_save_thyself;
  user_object_class->load_thyself = pn_actuator_load_thyself;
}

static void
pn_actuator_init (PnActuator *actuator, PnActuatorClass *class)
{
  actuator->options = g_array_new (FALSE, FALSE, sizeof (PnOption *));
}

static void
pn_actuator_destroy (PnObject *object)
{
  PnActuator *actuator = (PnActuator *) object;
  gint i;

  for (i=0; i < actuator->options->len; i++)
    pn_object_unref (g_array_index (actuator->options, PnObject *, i));
}

static void
pn_actuator_finalize (GObject *gobject)
{
  PnActuator *actuator = (PnActuator *) gobject;

  g_array_free (actuator->options, FALSE);

  if (G_OBJECT_CLASS (parent_class)->finalize)
    G_OBJECT_CLASS (parent_class)->finalize (gobject);
}

static void
pn_actuator_save_thyself (PnUserObject *user_object, xmlNodePtr node)
{
  PnActuator *actuator;
  gint i;
  xmlNodePtr options_node, option_node;
  PnOption *option;

  g_return_if_fail (user_object != NULL);
  g_return_if_fail (PN_IS_ACTUATOR (user_object));
  g_return_if_fail (node != NULL);

  actuator = (PnActuator *) user_object;

  options_node = xmlNewChild (node, NULL, (xmlChar *) "Options", NULL);

  /* Save all the options  */
  for (i=0; i < actuator->options->len; i++)
    {
      option = g_array_index (actuator->options, PnOption *, i);
      option_node = xmlNewChild (options_node, NULL, (xmlChar *) "BUG",  NULL);
      pn_user_object_save_thyself (PN_USER_OBJECT (option), option_node);
    }

  if (parent_class->save_thyself)
    parent_class->save_thyself (user_object, node);
}

static void
pn_actuator_load_thyself (PnUserObject *user_object, const xmlNodePtr node)
{
  PnActuator *actuator;
  xmlNodePtr options_node, option_node;
  PnOption *option;

  g_return_if_fail (user_object != NULL);
  g_return_if_fail (PN_IS_ACTUATOR (user_object));
  g_return_if_fail (node != NULL);

  actuator = (PnActuator *) user_object;

  /* FIXME: should these 'xmlChildrenNode' be 'xmlRootNode'? */

  /* find the 'options' node */
  for (options_node = node->xmlChildrenNode; options_node; options_node = options_node->next)
    if (g_strcasecmp (options_node->name, "Options") == 0)
      break;

  /* load each of the options */
  if (options_node)
    {
      for (option_node = options_node->xmlChildrenNode; option_node; option_node = option_node->next)
	{
	  if (!g_strcasecmp(option_node->name, "text"))
            continue;

	  option = pn_actuator_get_option_by_name (actuator, option_node->name);
	  if (option)
	    pn_user_object_load_thyself (PN_USER_OBJECT (option), option_node);
	  else
	    pn_error ("unknown actuator option ecountered in actuator \"%s\": %s",
		      node->name,
		      option_node->name);
	}
    }

  if (parent_class->load_thyself)
    parent_class->load_thyself (user_object, node);
}

/**
 * pn_actuator_add_option
 * @actuator: a #PnActuator
 * @option: the #PnOption to add
 *
 * Adds a #PnOption to the actuator.
 */
void
pn_actuator_add_option (PnActuator *actuator, PnOption *option)
{
  g_return_if_fail (actuator != NULL);
  g_return_if_fail (PN_IS_ACTUATOR (actuator));
  g_return_if_fail (option != NULL);
  g_return_if_fail (PN_IS_OPTION (option));
  g_return_if_fail (! pn_actuator_get_option_by_name (actuator, PN_USER_OBJECT_NAME (option)));

  g_array_append_val (actuator->options, option);
  pn_object_ref (PN_OBJECT (option));
  pn_object_sink (PN_OBJECT (option));
  pn_user_object_set_owner (PN_USER_OBJECT (option), PN_USER_OBJECT (actuator));
}

/**
 * pn_actuator_get_option_by_index
 * @actuator: a #PnActuator
 * @index: the index of the option to retrieve
 *
 * Retrieves a #PnOption associated with the actuator based on
 * the index of the option.  Indices start at 0 and are determined
 * by the order in which the options were added.
 *
 * Returns: The #PnOption at index @index
 */
PnOption*
pn_actuator_get_option_by_index (PnActuator *actuator, guint index)
{
  g_return_val_if_fail (actuator != NULL, NULL);
  g_return_val_if_fail (PN_IS_ACTUATOR (actuator), NULL);
  g_return_val_if_fail (index < actuator->options->len, NULL);

  return g_array_index (actuator->options, PnOption *, index);
}

/**
 * pn_actuator_get_option_by_name
 * @actuator: a #PnActuator
 * @name: the name of the option to retrieve
 *
 * Retrieves the first #PnOption associated with the actuator 
 * based on the name of the option.
 *
 * Returns: The first #PnOption with the name @name
 */
PnOption*
pn_actuator_get_option_by_name (PnActuator *actuator, const gchar *name)
{
  gint i;
  PnOption *option;

  g_return_val_if_fail (actuator != NULL, NULL);
  g_return_val_if_fail (PN_IS_ACTUATOR (actuator), NULL);
  g_return_val_if_fail (name != NULL, NULL);

  for (i=0; i < actuator->options->len; i++)
    {
      option = g_array_index (actuator->options, PnOption *, i);
      if (g_strcasecmp (pn_user_object_get_name (PN_USER_OBJECT (option)), name) == 0)
	return option;
    }

  return NULL;
}

/**
 * pn_actuator_prepare
 * @actuator: a #PnActuator
 * @image: the #PnImage for which the actuator should prepare
 *
 * Prepares an actuator to act upon a given #PnImage.
 */
void
pn_actuator_prepare (PnActuator *actuator, PnImage *image)
{
  PnActuatorClass *class;

  g_return_if_fail (actuator != NULL);
  g_return_if_fail (PN_IS_ACTUATOR (actuator));
  g_return_if_fail (image != NULL);
  g_return_if_fail (PN_IS_IMAGE (image));

  class = PN_ACTUATOR_GET_CLASS (actuator);

  if (class->prepare)
    class->prepare (actuator, image);
}

/**
 * pn_actuator_execute
 * @actuator: a #PnActuator
 * @image: the #PnImage to act upon
 * @audio_data: the #PnAudioData to use when basing effects on sound
 *
 * Causes the actuator to perform its function on @image.  The pn_actuator_prepare()
 * *MUST* have previously been called with the same @actuator and @image arguments.
 */
void
pn_actuator_execute (PnActuator *actuator, PnImage *image, PnAudioData *audio_data)
{
  PnActuatorClass *class;

  g_return_if_fail (actuator != NULL);
  g_return_if_fail (PN_IS_ACTUATOR (actuator));
  g_return_if_fail (image != NULL);
  g_return_if_fail (PN_IS_IMAGE (image));
  g_return_if_fail (audio_data != NULL);
/*    g_return_if_fail (PN_IS_AUDIO_DATA (audio_data)); */

  class = PN_ACTUATOR_GET_CLASS (actuator);

  if (class->execute)
    class->execute (actuator, image, audio_data);
}
