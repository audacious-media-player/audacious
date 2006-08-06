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
#include "pnimagecontext.h"
#include "pnlistoption.h"

static void         pn_image_context_class_init       (PnImageContextClass *class);
static void         pn_image_context_init             (PnImageContext *image_context,
						       PnImageContextClass *class);
/* GObject methods */
static void         pn_image_context_finalize         (GObject *gobject);

/* PnActuator methods */
static void         pn_image_context_prepare          (PnImageContext *image_context,
						       PnImage *image);
static void         pn_image_context_execute          (PnImageContext *image_context,
						       PnImage *image,
						       PnAudioData *audio_data);

static PnActuatorClass *parent_class = NULL;

GType
pn_image_context_get_type (void)
{
  static GType image_context_type = 0;

  if (! image_context_type)
    {
      static const GTypeInfo image_context_info =
      {
	sizeof (PnImageContextClass),
	NULL,		/* base_init */
	NULL,		/* base_finalize */
	(GClassInitFunc) pn_image_context_class_init,
	NULL,		/* class_finalize */
	NULL,		/* class_data */
        sizeof (PnImageContext),
	0,              /* n_preallocs */
	(GInstanceInitFunc) pn_image_context_init
      };

      /* FIXME: should this be dynamic? */
      image_context_type = g_type_register_static (PN_TYPE_CONTAINER,
					      "PnImageContext",
					      &image_context_info,
					      0);
    }
  return image_context_type;
}

static void
pn_image_context_class_init (PnImageContextClass *class)
{
  GObjectClass *gobject_class;
  PnObjectClass *object_class;
  PnUserObjectClass *user_object_class;
  PnActuatorClass *actuator_class;

  parent_class = g_type_class_peek_parent (class);

  gobject_class = (GObjectClass *) class;
  object_class = (PnObjectClass *) class;
  user_object_class = (PnUserObjectClass *) class;
  actuator_class = (PnActuatorClass *) class;

  /* GObject methods */
  gobject_class->finalize = pn_image_context_finalize;

  /* PnActuator methods */
  actuator_class->prepare = (PnActuatorPrepFunc) pn_image_context_prepare;
  actuator_class->execute = (PnActuatorExecFunc) pn_image_context_execute;
}

static void
pn_image_context_finalize (GObject *gobject)
{
  PnImageContext *image_context;

  image_context = (PnImageContext *) gobject;

  if (image_context->image)
    pn_object_unref (PN_OBJECT (image_context->image));
}

static void
pn_image_context_init (PnImageContext *image_context, PnImageContextClass *class)
{
  PnListOption *input_mode_opt, *output_mode_opt;
  guint i;

  /* Set up the name and description */
  pn_user_object_set_name (PN_USER_OBJECT (image_context), "Container.Image_Context");
  pn_user_object_set_description (PN_USER_OBJECT (image_context),
				  "A container that executes all its actuators sequentially");

  /* Set up the options */
  input_mode_opt = pn_list_option_new ("input_mode", "The blend mode to be used when getting the image "
				       "from the parent context");
  output_mode_opt = pn_list_option_new ("output_mode", "The blend mode to be used when writing the image "
					"to the parent context");

  for (i=0; i<pn_image_blend_mode_count; i++)
    {
      pn_list_option_add_item (input_mode_opt, pn_image_blend_mode_strings[i]);
      pn_list_option_add_item (output_mode_opt, pn_image_blend_mode_strings[i]);
    }

  pn_list_option_set_index (input_mode_opt, 0); /* default mode: Ignore */
  pn_list_option_set_index (output_mode_opt, 1); /* default mode: Replace */

  pn_actuator_add_option (PN_ACTUATOR (image_context), PN_OPTION (input_mode_opt));
  pn_actuator_add_option (PN_ACTUATOR (image_context), PN_OPTION (output_mode_opt));

  /* Create the new image */
  image_context->image = pn_image_new ();
  pn_object_ref (PN_OBJECT (image_context->image));
  pn_object_sink (PN_OBJECT (image_context->image));
}

static void
pn_image_context_prepare (PnImageContext *image_context, PnImage *image)
{
  g_return_if_fail (image_context != NULL);
  g_return_if_fail (PN_IS_IMAGE_CONTEXT (image_context));
  g_return_if_fail (image != NULL);
  g_return_if_fail (PN_IS_IMAGE (image));

  pn_image_set_size (image_context->image, pn_image_get_width (image), pn_image_get_height (image));

  if (PN_ACTUATOR_CLASS (parent_class)->prepare)
    PN_ACTUATOR_CLASS (parent_class)->prepare (PN_ACTUATOR (image_context), image);
}

static void
pn_image_context_execute (PnImageContext *image_context, PnImage *image,
			  PnAudioData *audio_data)
{
  guint i;
  GArray *actuators;
  PnListOption *input_mode_opt, *output_mode_opt;

  g_return_if_fail (image_context != NULL);
  g_return_if_fail (PN_IS_IMAGE_CONTEXT (image_context));
  g_return_if_fail (image != NULL);
  g_return_if_fail (PN_IS_IMAGE (image));
  g_return_if_fail (audio_data != NULL);

  actuators = ((PnContainer *) (image_context))->actuators;;

  input_mode_opt = (PnListOption *) pn_actuator_get_option_by_index (PN_ACTUATOR (image_context),
								     PN_IMAGE_CONTEXT_OPT_INPUT_MODE);
  output_mode_opt = (PnListOption *) pn_actuator_get_option_by_index (PN_ACTUATOR (image_context),
								      PN_IMAGE_CONTEXT_OPT_OUTPUT_MODE);

  /* Get the image from the parent context */
  pn_image_render_image (image_context->image, image, (PnBlendMode) pn_list_option_get_index (input_mode_opt));
  
  for (i=0; i<actuators->len; i++)
    pn_actuator_execute (g_array_index (actuators, PnActuator *, i), image_context->image, audio_data);

  /* Write the image to the parent context */
  pn_image_render_image (image, image_context->image, (PnBlendMode) pn_list_option_get_index (output_mode_opt));

  if (PN_ACTUATOR_CLASS (parent_class)->execute)
    PN_ACTUATOR_CLASS (parent_class)->execute(PN_ACTUATOR (image_context), image, audio_data);
}

PnImageContext*
pn_image_context_new (void)
{
  return (PnImageContext *) g_object_new (PN_TYPE_IMAGE_CONTEXT, NULL);
}
