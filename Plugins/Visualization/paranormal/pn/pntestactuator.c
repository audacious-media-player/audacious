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
#include "pntestactuator.h"
#include "pnbooleanoption.h"
#include "pnintegeroption.h"
#include "pnfloatoption.h"
#include "pnstringoption.h"
#include "pnerror.h"

static void         pn_test_actuator_class_init       (PnTestActuatorClass *class);
static void         pn_test_actuator_init             (PnTestActuator *test_actuator,
						       PnTestActuatorClass *class);
/* PnActuator methods */
static void         pn_test_actuator_prepare          (PnTestActuator *test_actuator,
						       PnImage *image);
static void         pn_test_actuator_execute          (PnTestActuator *test_actuator,
						       PnImage *image,
						       PnAudioData *audio_data);

static PnActuatorClass *parent_class = NULL;

GType
pn_test_actuator_get_type (void)
{
  static GType test_actuator_type = 0;

  if (! test_actuator_type)
    {
      static const GTypeInfo test_actuator_info =
      {
	sizeof (PnTestActuatorClass),
	NULL,		/* base_init */
	NULL,		/* base_finalize */
	(GClassInitFunc) pn_test_actuator_class_init,
	NULL,		/* class_finalize */
	NULL,		/* class_data */
        sizeof (PnTestActuator),
	0,              /* n_preallocs */
	(GInstanceInitFunc) pn_test_actuator_init
      };

      /* FIXME: should this be dynamic? */
      test_actuator_type = g_type_register_static (PN_TYPE_ACTUATOR,
					      "PnTestActuator",
					      &test_actuator_info,
					      0);
    }
  return test_actuator_type;
}

static void
pn_test_actuator_class_init (PnTestActuatorClass *class)
{
  PnObjectClass *object_class;
  PnUserObjectClass *user_object_class;
  PnActuatorClass *actuator_class;

  parent_class = g_type_class_peek_parent (class);

  object_class = (PnObjectClass *) class;
  user_object_class = (PnUserObjectClass *) class;
  actuator_class = (PnActuatorClass *) class;

  /* PnActuator methods */
  actuator_class->prepare = (PnActuatorPrepFunc) pn_test_actuator_prepare;
  actuator_class->execute = (PnActuatorExecFunc) pn_test_actuator_execute;
}

static void
pn_test_actuator_init (PnTestActuator *test_actuator, PnTestActuatorClass *class)
{
  PnBooleanOption *test_bool_opt;
  PnIntegerOption *test_int_opt;
  PnFloatOption *test_float_opt;
  PnStringOption *test_str_opt;

  /* Set up the name and description */
  pn_user_object_set_name (PN_USER_OBJECT (test_actuator), "Test.Test");
  pn_user_object_set_description (PN_USER_OBJECT (test_actuator),
				  "An actuator to test the functionality of the PnActuator base class");

  /* Set up the options */
  test_bool_opt = pn_boolean_option_new ("test_boolean_option", "A boolean test option");
  pn_boolean_option_set_value (test_bool_opt, TRUE);

  test_int_opt = pn_integer_option_new ("test_integer_option", "An integer test option");
  pn_integer_option_set_value (test_int_opt, 32);
  pn_integer_option_set_min (test_int_opt, 16);
  pn_integer_option_set_max (test_int_opt, 64);

  test_float_opt = pn_float_option_new ("test_float_option", "A float test option");
  pn_float_option_set_value (test_float_opt, 0.7);
  pn_float_option_set_min (test_float_opt, 0.0);
  pn_float_option_set_max (test_float_opt, 1.0);

  test_str_opt = pn_string_option_new ("test_string_option", "A string test option");

  pn_actuator_add_option (PN_ACTUATOR (test_actuator), PN_OPTION (test_bool_opt));
  pn_actuator_add_option (PN_ACTUATOR (test_actuator), PN_OPTION (test_int_opt));
  pn_actuator_add_option (PN_ACTUATOR (test_actuator), PN_OPTION (test_float_opt));
  pn_actuator_add_option (PN_ACTUATOR (test_actuator), PN_OPTION (test_str_opt));
}

static void
pn_test_actuator_prepare (PnTestActuator *test_actuator, PnImage *image)
{
  g_return_if_fail (test_actuator != NULL);
  g_return_if_fail (PN_IS_TEST_ACTUATOR (test_actuator));
  g_return_if_fail (image != NULL);
  g_return_if_fail (PN_IS_IMAGE (image));

  pn_error ("pn_test_actuator_prepare called");
}

static void
pn_test_actuator_execute (PnTestActuator *test_actuator, PnImage *image,
			  PnAudioData *audio_data)
{
  PnOption *test_bool_opt, *test_int_opt, *test_float_opt, *test_str_opt;
  PnColor color = { 0, 0, 0, 0 };
  guint i;

  g_return_if_fail (test_actuator != NULL);
  g_return_if_fail (PN_IS_TEST_ACTUATOR (test_actuator));
  g_return_if_fail (image != NULL);
  g_return_if_fail (PN_IS_IMAGE (image));
  g_return_if_fail (audio_data != NULL);
/*    g_return_if_fail (PN_IS_AUDIO_DATA (audio_data)); */

  test_bool_opt = pn_actuator_get_option_by_index (PN_ACTUATOR (test_actuator),
						   PN_TEST_ACTUATOR_OPT_TEST_BOOL);
  test_int_opt = pn_actuator_get_option_by_index (PN_ACTUATOR (test_actuator),
						  PN_TEST_ACTUATOR_OPT_TEST_INT);
  test_float_opt = pn_actuator_get_option_by_index (PN_ACTUATOR (test_actuator),
						    PN_TEST_ACTUATOR_OPT_TEST_FLOAT);
  test_str_opt = pn_actuator_get_option_by_index (PN_ACTUATOR (test_actuator),
						  PN_TEST_ACTUATOR_OPT_TEST_STR);

/*    printf ("pn_test_actuator_execute: %s = %d\n", */
/*  	  pn_user_object_get_name (PN_USER_OBJECT (test_bool_opt)), */
/*  	  pn_boolean_option_get_value (PN_BOOLEAN_OPTION (test_bool_opt))); */
/*    printf ("pn_test_actuator_execute: %s = %d\n", */
/*  	  pn_user_object_get_name (PN_USER_OBJECT (test_int_opt)), */
/*  	  pn_integer_option_get_value (PN_INTEGER_OPTION (test_int_opt))); */
/*    printf ("pn_test_actuator_execute: %s = %f\n", */
/*  	  pn_user_object_get_name (PN_USER_OBJECT (test_float_opt)), */
/*  	  pn_float_option_get_value (PN_FLOAT_OPTION (test_float_opt))); */
/*    printf ("pn_test_actuator_execute: %s = \"%s\"\n", */
/*  	  pn_user_object_get_name (PN_USER_OBJECT (test_str_opt)), */
/*  	  pn_string_option_get_value (PN_STRING_OPTION (test_str_opt))); */

  for (i=0; i<pn_image_get_width (image) *  64; i++)
    {
      color.red = i % 256;
      pn_image_render_pixel_by_offset (image, i, color);
    }
}

PnTestActuator*
pn_test_actuator_new (void)
{
  return (PnTestActuator *) g_object_new (PN_TYPE_TEST_ACTUATOR, NULL);
}
