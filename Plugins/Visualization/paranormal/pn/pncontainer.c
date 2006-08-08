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

#include <glib.h>
#include "pncontainer.h"
#include "pnactuatorfactory.h"
#include "pnerror.h"
#include "pnxml.h"

static void         pn_container_class_init        (PnContainerClass *class);
static void         pn_container_init              (PnContainer *container,
						    PnContainerClass *class);
/* PnObject methods */
static void         pn_container_destroy           (PnObject *object);

/* PnUserObject methods */
static void         pn_container_save_thyself      (PnUserObject *user_object,
						    xmlNodePtr node);
static void         pn_container_load_thyself      (PnUserObject *user_object,
						    xmlNodePtr node);

/* PnActuator methods */
static void         pn_container_prepare           (PnContainer *container,
						    PnImage *image);

/* PnContainer methods */
static gboolean     pn_container_real_add_actuator (PnContainer *container,
						    PnActuator *actuator,
						    gint position);

static PnActuatorClass *parent_class = NULL;

GType
pn_container_get_type (void)
{
  static GType container_type = 0;

  if (! container_type)
    {
      static const GTypeInfo container_info =
      {
	sizeof (PnContainerClass),
	NULL,		/* base_init */
	NULL,		/* base_finalize */
	(GClassInitFunc) pn_container_class_init,
	NULL,		/* class_finalize */
	NULL,		/* class_data */
        sizeof (PnContainer),
	0,              /* n_preallocs */
	(GInstanceInitFunc) pn_container_init
      };

      /* FIXME: should this be dynamic? */
      container_type = g_type_register_static (PN_TYPE_ACTUATOR,
					      "PnContainer",
					      &container_info,
					      G_TYPE_FLAG_ABSTRACT);
    }
  return container_type;
}

static void
pn_container_class_init (PnContainerClass *class)
{
  PnObjectClass *object_class;
  PnUserObjectClass *user_object_class;
  PnActuatorClass *actuator_class;

  parent_class = g_type_class_peek_parent (class);

  object_class = (PnObjectClass *) class;
  user_object_class = (PnUserObjectClass *) class;
  actuator_class = (PnActuatorClass *) class;

  /* PnObject signals */
  object_class->destroy = pn_container_destroy;

  /* PnUserObject methods */
  user_object_class->save_thyself = pn_container_save_thyself;
  user_object_class->load_thyself = pn_container_load_thyself;

  /* PnActuator methods */
  actuator_class->prepare = (PnActuatorPrepFunc) pn_container_prepare;

  /* PnContainer methods */
  class->add_actuator = pn_container_real_add_actuator;
}

static void
pn_container_init (PnContainer *container, PnContainerClass *class)
{
  container->actuators = g_array_new (FALSE, FALSE, sizeof (PnActuator *));
}

static void
pn_container_destroy (PnObject *object)
{
  PnContainer *container;

  g_return_if_fail (object != NULL);
  g_return_if_fail (PN_IS_CONTAINER (object));

  container = (PnContainer *) object;

  pn_container_remove_all_actuators (container);
}

static void
pn_container_save_thyself (PnUserObject *user_object, xmlNodePtr node)
{
  PnContainer *container;
  PnActuator *actuator;
  xmlNodePtr actuators_node, actuator_node;
  guint i;

  g_return_if_fail (user_object != NULL);
  g_return_if_fail (PN_IS_CONTAINER (user_object));
  g_return_if_fail (node != NULL);

  container = (PnContainer *) user_object;

  actuators_node = xmlNewChild (node, NULL, "Actuators", NULL);

  /* Save the actuators */
  for (i=0; i<container->actuators->len; i++)
    {
      actuator = g_array_index (container->actuators, PnActuator *, i);
      actuator_node = xmlNewChild (actuators_node, NULL, "BUG", NULL);
      pn_user_object_save_thyself (PN_USER_OBJECT (actuator), actuator_node);
    }

  if (PN_USER_OBJECT_CLASS (parent_class)->save_thyself)
    PN_USER_OBJECT_CLASS (parent_class)->save_thyself (user_object, node);
}

static void
pn_container_load_thyself (PnUserObject *user_object, const xmlNodePtr node)
{
  PnContainer *container;
  xmlNodePtr actuators_node, actuator_node;
  PnActuator *actuator;

  g_return_if_fail (user_object != NULL);
  g_return_if_fail (PN_IS_CONTAINER (user_object));
  g_return_if_fail (node != NULL);

  container = (PnContainer *) user_object;

  /* FIXME: should these 'xmlChildrenNode' be 'xmlRootNode'? */

  /* find the 'actuators' node */
  for (actuators_node = node->xmlChildrenNode; actuators_node; actuators_node = actuators_node->next)
    if (g_strcasecmp (actuators_node->name, "Actuators") == 0)
      break;

  /* load each of the actuators */
  if (actuators_node)
    {
      for (actuator_node = actuators_node->xmlChildrenNode; actuator_node; actuator_node = actuator_node->next)
	{
	  if (!g_strcasecmp(actuator_node->name, "text"))
	    continue;

	  actuator = pn_actuator_factory_new_actuator_from_xml (actuator_node);
	  if (actuator)
	    /* FIXME: Should this be pn_container_real_add_actuator? */
	    pn_container_add_actuator (container, actuator, PN_POSITION_TAIL);
	  else
	    pn_error ("unknown actuator ecountered in container \"%s\": %s",
		      node->name,
		      actuator_node->name);
	}
    }

  if (PN_USER_OBJECT_CLASS (parent_class)->load_thyself)
    PN_USER_OBJECT_CLASS (parent_class)->load_thyself (user_object, node);
}

static void
pn_container_prepare (PnContainer *container, PnImage *image)
{
  guint i;

  g_return_if_fail (container != NULL);
  g_return_if_fail (PN_IS_CONTAINER (container));
  g_return_if_fail (image != NULL);
  g_return_if_fail (PN_IS_IMAGE (image));

  for (i=0; i<container->actuators->len; i++)
    pn_actuator_prepare (g_array_index (container->actuators, PnActuator *, i), image);
}

static gboolean
pn_container_real_add_actuator (PnContainer *container, PnActuator *actuator, gint position)
{
  g_return_val_if_fail (container != NULL, FALSE);
  g_return_val_if_fail (PN_IS_CONTAINER (container), FALSE);
  g_return_val_if_fail (actuator != NULL, FALSE);
  g_return_val_if_fail (PN_IS_ACTUATOR (actuator), FALSE);
  g_return_val_if_fail (position >= PN_POSITION_TAIL, FALSE);

  if (position == PN_POSITION_TAIL || position >= container->actuators->len)
    position = container->actuators->len;

  /* Just pass it on to the GArray insert function */
  g_array_insert_val (container->actuators, position, actuator);

  pn_object_ref (PN_OBJECT (actuator));
  pn_object_sink (PN_OBJECT (actuator));

  return TRUE;
}

/**
 * pn_container_add_actuator
 * @container: a #PnContainer
 * @actuator: the #PnActuator to add
 * @position: the position at which to add the actuator
 *
 * Adds @actuator to @container.  @position is the zero-based index
 * in the list of actuators that the newly added actuator should have,
 * or it can be the value %PN_POSITION_HEAD or %PN_POSITION_TAIL.
 *
 * Returns: %TRUE on success; %FALSE on failure
 */
gboolean
pn_container_add_actuator (PnContainer *container, PnActuator *actuator, gint position)
{
  g_return_val_if_fail (container != NULL, FALSE);
  g_return_val_if_fail (PN_IS_CONTAINER (container), FALSE);
  g_return_val_if_fail (actuator != NULL, FALSE);
  g_return_val_if_fail (PN_IS_ACTUATOR (actuator), FALSE);
  g_return_val_if_fail (position >= PN_POSITION_TAIL, FALSE);

  if (PN_CONTAINER_GET_CLASS (container)->add_actuator)
    return PN_CONTAINER_GET_CLASS (container)->add_actuator (container, actuator, position);
  else
    return FALSE;
}

/**
 * pn_container_remove_actuator
 * @container: a #PnContainer
 * @actuator: the #PnActuator to remove
 *
 * Removes the first occurence of @actuator in @container's list
 * of contained actuators.
 */
/* FIXME: what is the most convenient way to remove actuators? */
/* Note: Only removes the first one in the array */
void
pn_container_remove_actuator (PnContainer *container, PnActuator *actuator)
{
  guint i;

  g_return_if_fail (container != NULL);
  g_return_if_fail (PN_IS_CONTAINER (container));
  g_return_if_fail (actuator != NULL);
  g_return_if_fail (PN_IS_ACTUATOR (actuator));

  for (i=0; i<container->actuators->len; i++)
    if (g_array_index (container->actuators, PnActuator *, i) == actuator)
      {
	g_array_remove_index (container->actuators, i);
	pn_object_unref (PN_OBJECT (actuator));
	return;
      }
}

/**
 * pn_container_remove_all_actuators
 * @container: a #PnContainer
 *
 * Removes all actuators from @container's list of contained actuators.
 */
void
pn_container_remove_all_actuators (PnContainer *container)
{
  guint i;

  g_return_if_fail (container != NULL);
  g_return_if_fail (PN_IS_CONTAINER (container));

  for (i=0; i<container->actuators->len; i++)
    pn_object_unref (g_array_index (container->actuators, PnObject *, i));

  g_array_set_size (container->actuators, 0);
}
