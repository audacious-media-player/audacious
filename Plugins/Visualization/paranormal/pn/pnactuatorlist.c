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
#include "pnactuatorlist.h"

static void         pn_actuator_list_class_init       (PnActuatorListClass *class);
static void         pn_actuator_list_init             (PnActuatorList *actuator_list,
						       PnActuatorListClass *class);
/* PnActuator methods */
static void         pn_actuator_list_execute          (PnActuatorList *actuator_list,
						       PnImage *image,
						       PnAudioData *audio_data);

static PnActuatorClass *parent_class = NULL;

GType
pn_actuator_list_get_type (void)
{
  static GType actuator_list_type = 0;

  if (! actuator_list_type)
    {
      static const GTypeInfo actuator_list_info =
      {
	sizeof (PnActuatorListClass),
	NULL,		/* base_init */
	NULL,		/* base_finalize */
	(GClassInitFunc) pn_actuator_list_class_init,
	NULL,		/* class_finalize */
	NULL,		/* class_data */
        sizeof (PnActuatorList),
	0,              /* n_preallocs */
	(GInstanceInitFunc) pn_actuator_list_init
      };

      /* FIXME: should this be dynamic? */
      actuator_list_type = g_type_register_static (PN_TYPE_CONTAINER,
					      "PnActuatorList",
					      &actuator_list_info,
					      0);
    }
  return actuator_list_type;
}

static void
pn_actuator_list_class_init (PnActuatorListClass *class)
{
  PnObjectClass *object_class;
  PnUserObjectClass *user_object_class;
  PnActuatorClass *actuator_class;

  parent_class = g_type_class_peek_parent (class);

  object_class = (PnObjectClass *) class;
  user_object_class = (PnUserObjectClass *) class;
  actuator_class = (PnActuatorClass *) class;

  /* PnActuator methods */
  actuator_class->execute = (PnActuatorExecFunc) pn_actuator_list_execute;
}

static void
pn_actuator_list_init (PnActuatorList *actuator_list, PnActuatorListClass *class)
{
  /* Set up the name and description */
  pn_user_object_set_name (PN_USER_OBJECT (actuator_list), "Container.Actuator_List");
  pn_user_object_set_description (PN_USER_OBJECT (actuator_list),
				  "A container that executes all its actuators sequentially");
}

static void
pn_actuator_list_execute (PnActuatorList *actuator_list, PnImage *image,
			  PnAudioData *audio_data)
{
  guint i;
  GArray *actuators;

  g_return_if_fail (actuator_list != NULL);
  g_return_if_fail (PN_IS_ACTUATOR_LIST (actuator_list));
  g_return_if_fail (image != NULL);
  g_return_if_fail (PN_IS_IMAGE (image));
  g_return_if_fail (audio_data != NULL);

  actuators = ((PnContainer *) (actuator_list))->actuators;;

  for (i=0; i<actuators->len; i++)
    pn_actuator_execute (g_array_index (actuators, PnActuator *, i), image, audio_data);

  if (PN_ACTUATOR_CLASS (parent_class)->execute)
    PN_ACTUATOR_CLASS (parent_class)->execute(PN_ACTUATOR (actuator_list), image, audio_data);
}

/**
 * pn_actuator_list_new
 *
 * Creates a new #PnActuatorList.
 *
 * Returns: The new #PnActuatorList.
 */
PnActuatorList*
pn_actuator_list_new (void)
{
  return (PnActuatorList *) g_object_new (PN_TYPE_ACTUATOR_LIST, NULL);
}
