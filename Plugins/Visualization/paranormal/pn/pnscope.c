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

#include <math.h>
#include <glib.h>
#include "pnscope.h"
#include "pnlistoption.h"
#include "pnstringoption.h"

enum
{
  PN_SCOPE_DRAW_METHOD_DOTS = 0,
  PN_SCOPE_DRAW_METHOD_LINES
};

static void         pn_scope_class_init       (PnScopeClass *class);
static void         pn_scope_init             (PnScope *scope,
					       PnScopeClass *class);
/* PnObject signals */
static void         pn_scope_destroy          (PnObject *object);

/* PnActuator methods */
static void         pn_scope_prepare          (PnScope *scope,
					       PnImage *image);
static void         pn_scope_execute          (PnScope *scope,
					       PnImage *image,
					       PnAudioData *audio_data);

static PnActuatorClass *parent_class = NULL;

GType
pn_scope_get_type (void)
{
  static GType scope_type = 0;

  if (! scope_type)
    {
      static const GTypeInfo scope_info =
      {
	sizeof (PnScopeClass),
	NULL,		/* base_init */
	NULL,		/* base_finalize */
	(GClassInitFunc) pn_scope_class_init,
	NULL,		/* class_finalize */
	NULL,		/* class_data */
        sizeof (PnScope),
	0,              /* n_preallocs */
	(GInstanceInitFunc) pn_scope_init
      };

      /* FIXME: should this be dynamic? */
      scope_type = g_type_register_static (PN_TYPE_ACTUATOR,
					      "PnScope",
					      &scope_info,
					      0);
    }
  return scope_type;
}

static void
pn_scope_class_init (PnScopeClass *class)
{
  PnObjectClass *object_class;
  PnUserObjectClass *user_object_class;
  PnActuatorClass *actuator_class;

  parent_class = g_type_class_peek_parent (class);

  object_class = (PnObjectClass *) class;
  user_object_class = (PnUserObjectClass *) class;
  actuator_class = (PnActuatorClass *) class;

  /* PnObject signals */
  object_class->destroy = pn_scope_destroy;

  /* PnActuator methods */
  actuator_class->prepare = (PnActuatorPrepFunc) pn_scope_prepare;
  actuator_class->execute = (PnActuatorExecFunc) pn_scope_execute;
}

static void
pn_scope_init (PnScope *scope, PnScopeClass *class)
{
  PnListOption *draw_method_opt;
  PnStringOption *init_script_opt;
  PnStringOption *frame_script_opt;
  PnStringOption *sample_script_opt;

  /* Set up the name and description */
  pn_user_object_set_name (PN_USER_OBJECT (scope), "Render.Scope");
  pn_user_object_set_description (PN_USER_OBJECT (scope),
				  "A test scope actuator");

  /* Set up the options */
  draw_method_opt = pn_list_option_new ("draw_method", "The way the points will be drawn");
  init_script_opt = pn_string_option_new ("init_script", "The initialization script");
  frame_script_opt = pn_string_option_new ("frame_script", "The per-frame script");
  sample_script_opt = pn_string_option_new ("sample_script", "The per-sample script");

  pn_list_option_add_item (draw_method_opt, "Dots");
  pn_list_option_add_item (draw_method_opt, "Lines");

  pn_actuator_add_option (PN_ACTUATOR (scope), PN_OPTION (draw_method_opt));
  pn_actuator_add_option (PN_ACTUATOR (scope), PN_OPTION (init_script_opt));
  pn_actuator_add_option (PN_ACTUATOR (scope), PN_OPTION (frame_script_opt));
  pn_actuator_add_option (PN_ACTUATOR (scope), PN_OPTION (sample_script_opt));

  /* Create the script objects and symbol table */
  scope->init_script = pn_script_new ();
  pn_object_ref (PN_OBJECT (scope->init_script));
  pn_object_sink (PN_OBJECT (scope->init_script));
  scope->frame_script = pn_script_new ();
  pn_object_ref (PN_OBJECT (scope->frame_script));
  pn_object_sink (PN_OBJECT (scope->frame_script));
  scope->sample_script = pn_script_new ();
  pn_object_ref (PN_OBJECT (scope->sample_script));
  pn_object_sink (PN_OBJECT (scope->sample_script));
  scope->symbol_table = pn_symbol_table_new ();
  pn_object_ref (PN_OBJECT (scope->symbol_table));
  pn_object_sink (PN_OBJECT (scope->symbol_table));

  /* Get the variables from the symbol table */
  scope->samples_var = pn_symbol_table_ref_variable_by_name (scope->symbol_table, "samples");
  scope->width_var = pn_symbol_table_ref_variable_by_name (scope->symbol_table, "width");
  scope->height_var = pn_symbol_table_ref_variable_by_name (scope->symbol_table, "height");
  scope->x_var = pn_symbol_table_ref_variable_by_name (scope->symbol_table, "x");
  scope->y_var = pn_symbol_table_ref_variable_by_name (scope->symbol_table, "y");
  scope->iteration_var = pn_symbol_table_ref_variable_by_name (scope->symbol_table, "iteration");
  scope->value_var = pn_symbol_table_ref_variable_by_name (scope->symbol_table, "value");
  scope->red_var = pn_symbol_table_ref_variable_by_name (scope->symbol_table, "red");
  scope->green_var = pn_symbol_table_ref_variable_by_name (scope->symbol_table, "green");
  scope->blue_var = pn_symbol_table_ref_variable_by_name (scope->symbol_table, "blue");
  scope->volume_var = pn_symbol_table_ref_variable_by_name (scope->symbol_table, "volume");

  PN_VARIABLE_VALUE (scope->samples_var) = 0.0;
  PN_VARIABLE_VALUE (scope->red_var) = 1.0;
  PN_VARIABLE_VALUE (scope->green_var) = 1.0;
  PN_VARIABLE_VALUE (scope->blue_var) = 1.0;
}

static void
pn_scope_destroy (PnObject *object)
{
  PnScope *scope;

  scope = (PnScope *) object;

  pn_object_unref (PN_OBJECT (scope->init_script));
  pn_object_unref (PN_OBJECT (scope->frame_script));
  pn_object_unref (PN_OBJECT (scope->sample_script));
  pn_object_unref (PN_OBJECT (scope->symbol_table));
}

static void
pn_scope_prepare (PnScope *scope, PnImage *image)
{
  PnStringOption *init_script_opt;
  PnStringOption *frame_script_opt;
  PnStringOption *sample_script_opt;

  g_return_if_fail (scope != NULL);
  g_return_if_fail (PN_IS_SCOPE (scope));
  g_return_if_fail (image != NULL);
  g_return_if_fail (PN_IS_IMAGE (image));

  /* Parse the script strings */
  init_script_opt = (PnStringOption *) pn_actuator_get_option_by_index (PN_ACTUATOR (scope),
									PN_SCOPE_OPT_INIT_SCRIPT);
  frame_script_opt = (PnStringOption *) pn_actuator_get_option_by_index (PN_ACTUATOR (scope),
									 PN_SCOPE_OPT_FRAME_SCRIPT);
  sample_script_opt = (PnStringOption *) pn_actuator_get_option_by_index (PN_ACTUATOR (scope),
									  PN_SCOPE_OPT_SAMPLE_SCRIPT);

  pn_script_parse_string (scope->init_script,
			  scope->symbol_table,
			  pn_string_option_get_value (init_script_opt));
  pn_script_parse_string (scope->frame_script,
			  scope->symbol_table,
			  pn_string_option_get_value (frame_script_opt));
  pn_script_parse_string (scope->sample_script,
			  scope->symbol_table,
			  pn_string_option_get_value (sample_script_opt));

  /* Set up the width and height in-script variables */
  PN_VARIABLE_VALUE (scope->width_var) = pn_image_get_width (image);
  PN_VARIABLE_VALUE (scope->height_var) = pn_image_get_height (image);

  /* Run the init script */
  pn_script_execute (scope->init_script);

  if (parent_class->prepare)
    parent_class->prepare (PN_ACTUATOR (scope), image);
}

static void
pn_scope_execute (PnScope *scope, PnImage *image, PnAudioData *audio_data)
{
  gdouble i, samples;
  gdouble half_width, neg_half_height, increment = 0.0;
  PnColor color = { 255, 255, 255 };
  guint last_x = 0, last_y = 0, x, y;
  PnListOption *draw_method_opt;
  gboolean use_lines;

  g_return_if_fail (scope != NULL);
  g_return_if_fail (PN_IS_SCOPE (scope));
  g_return_if_fail (image != NULL);
  g_return_if_fail (PN_IS_IMAGE (image));
  g_return_if_fail (audio_data != NULL);
  g_return_if_fail (PN_IS_AUDIO_DATA (audio_data));

  half_width = (((gdouble) pn_image_get_width (image)) - 1.0) * 0.5;
  neg_half_height = (((gdouble) pn_image_get_height (image)) - 1.0) * -0.5;

  draw_method_opt = (PnListOption *) pn_actuator_get_option_by_index (PN_ACTUATOR (scope),
								      PN_SCOPE_OPT_DRAW_METHOD);
  use_lines = pn_list_option_get_index (draw_method_opt) == PN_SCOPE_DRAW_METHOD_LINES;

  /* Set the volume in-script variable */
  PN_VARIABLE_VALUE (scope->volume_var) = pn_audio_data_get_volume (audio_data);

  /* Run the frame script */
  pn_script_execute (scope->frame_script);

  /* Go through and draw each sample-point  */
  samples = PN_VARIABLE_VALUE (scope->samples_var);
  if (samples > 1)
    {
      increment = 1 / (samples - 1);
      PN_VARIABLE_VALUE (scope->iteration_var) = -increment;
    }
  else if (samples > 0)
    {
      increment = 0;
      PN_VARIABLE_VALUE (scope->iteration_var) = 0.0;
    }
  for (i=0; i < samples; i++)
    {
      /* Set up the in-script variables */
      PN_VARIABLE_VALUE (scope->x_var) = 2.0;
      PN_VARIABLE_VALUE (scope->y_var) = 2.0;
      PN_VARIABLE_VALUE (scope->iteration_var) += increment;

      /* FIXME: This should be done in PnAudioData */
      /* Interpolate the sample value */
      {
	gdouble sample, bias;
	guint lsamp, usamp;

	sample = (gdouble)PN_AUDIO_DATA_PCM_SAMPLES(audio_data)
	  * (gdouble)PN_VARIABLE_VALUE (scope->iteration_var);

	lsamp = (guint) sample;
	if (lsamp != sample)
	  {
	    usamp = lsamp + 1;

	    bias = sample - (gdouble) lsamp;

	    PN_VARIABLE_VALUE (scope->value_var) =
	      (PN_AUDIO_DATA_PCM_DATA(audio_data, PN_CHANNEL_LEFT)[lsamp] * (1 - bias))
	      + (PN_AUDIO_DATA_PCM_DATA(audio_data, PN_CHANNEL_LEFT)[lsamp] * bias);
	  }
	else
	  PN_VARIABLE_VALUE (scope->value_var) =
	    PN_AUDIO_DATA_PCM_DATA (audio_data, PN_CHANNEL_LEFT)[lsamp];
      }

      /* Run the sample script */
      pn_script_execute (scope->sample_script);

      /* Get the color */
      color.red = CLAMP (PN_VARIABLE_VALUE (scope->red_var), 0.0, 1.0) * 255.0;
      color.green = CLAMP (PN_VARIABLE_VALUE (scope->green_var), 0.0, 1.0) * 255.0;
      color.blue = CLAMP (PN_VARIABLE_VALUE (scope->blue_var), 0.0, 1.0) * 255.0;

      /* Render the point */
      x = rint ((PN_VARIABLE_VALUE (scope->x_var) + 1.0) * ((gdouble) half_width ));
      y = rint ((PN_VARIABLE_VALUE (scope->y_var) - 1.0) * ((gdouble) neg_half_height));

      if (use_lines)
	{
	  if (i > 0)
	    pn_image_render_line (image, last_x, last_y, x, y, color);

	  last_x = x;
	  last_y = y;
	}
      else
	pn_image_render_pixel (image, x, y, color);
    }

  if (parent_class->execute)
    parent_class->execute (PN_ACTUATOR (scope), image, audio_data);
}

PnScope*
pn_scope_new (void)
{
  return (PnScope *) g_object_new (PN_TYPE_SCOPE, NULL);
}
