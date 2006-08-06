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
#include "pndistortion.h"
#include "pnbooleanoption.h"
#include "pnstringoption.h"
#include "pncpu.h"

enum
{
  PN_PIXEL_DISTORTION_NO_PIXEL = 0xffffffff
};

static void         pn_distortion_class_init       (PnDistortionClass *class);
static void         pn_distortion_init             (PnDistortion *distortion,
						     PnDistortionClass *class);
/* GObject methods */
static void         pn_distortion_finalize         (GObject *gobject);

/* PnObject signals */
static void         pn_distortion_destroy          (PnObject *object);

/* PnActuator methods */
static void         pn_distortion_prepare          (PnDistortion *distortion,
						     PnImage *image);
static void         pn_distortion_execute          (PnDistortion *distortion,
						     PnImage *image,
						     PnAudioData *audio_data);

static PnActuatorClass *parent_class = NULL;

GType
pn_distortion_get_type (void)
{
  static GType distortion_type = 0;

  if (! distortion_type)
    {
      static const GTypeInfo distortion_info =
      {
	sizeof (PnDistortionClass),
	NULL,		/* base_init */
	NULL,		/* base_finalize */
	(GClassInitFunc) pn_distortion_class_init,
	NULL,		/* class_finalize */
	NULL,		/* class_data */
        sizeof (PnDistortion),
	0,              /* n_preallocs */
	(GInstanceInitFunc) pn_distortion_init
      };

      /* FIXME: should this be dynamic? */
      distortion_type = g_type_register_static (PN_TYPE_ACTUATOR,
						 "PnDistortion",
						 &distortion_info,
						 0);
    }
  return distortion_type;
}

static void
pn_distortion_class_init (PnDistortionClass *class)
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
  gobject_class->finalize = pn_distortion_finalize;

  /* PnObject signals */
  object_class->destroy = pn_distortion_destroy;

  /* PnActuator methods */
  actuator_class->prepare = (PnActuatorPrepFunc) pn_distortion_prepare;
  actuator_class->execute = (PnActuatorExecFunc) pn_distortion_execute;
}

static void
pn_distortion_init (PnDistortion *distortion, PnDistortionClass *class)
{
  PnBooleanOption *polar_coords_opt;
  PnStringOption *distortion_script_opt;

  /* Set up the name and description */
  pn_user_object_set_name (PN_USER_OBJECT (distortion), "Transform.Distortion");
  pn_user_object_set_description (PN_USER_OBJECT (distortion),
				  "Perform a distortion on all the points in the image");

  /* Set up the options */
  polar_coords_opt = pn_boolean_option_new ("polar_coords", "Whether or not to use polar coordinates "
					    "in the distortion script");
  distortion_script_opt = pn_string_option_new ("distortion_script", "A script specifying the "
						 "distortion to be done at each point");

  pn_actuator_add_option (PN_ACTUATOR (distortion), PN_OPTION (polar_coords_opt));
  pn_actuator_add_option (PN_ACTUATOR (distortion), PN_OPTION (distortion_script_opt));

  /* Create the script object and symbol table */
  distortion->distortion_script = pn_script_new ();
  pn_object_ref (PN_OBJECT (distortion->distortion_script));
  pn_object_sink (PN_OBJECT (distortion->distortion_script));
  distortion->symbol_table = pn_symbol_table_new ();
  pn_object_ref (PN_OBJECT (distortion->symbol_table));
  pn_object_sink (PN_OBJECT (distortion->symbol_table));

  /* Get the variables from the symbol table */
  distortion->x_var = pn_symbol_table_ref_variable_by_name (distortion->symbol_table, "x");
  distortion->y_var = pn_symbol_table_ref_variable_by_name (distortion->symbol_table, "y");
  distortion->r_var = pn_symbol_table_ref_variable_by_name (distortion->symbol_table, "r");
  distortion->theta_var = pn_symbol_table_ref_variable_by_name (distortion->symbol_table, "theta");
  distortion->intensity_var = pn_symbol_table_ref_variable_by_name (distortion->symbol_table, "intensity");
}

static void
pn_distortion_finalize (GObject *gobject)
{
  PnDistortion *distortion;

  distortion = (PnDistortion *) gobject;

  if (distortion->map)
    g_free (distortion->map);

  if (G_OBJECT_CLASS (parent_class)->finalize)
    G_OBJECT_CLASS (parent_class)->finalize (gobject);
}

static void
pn_distortion_destroy (PnObject *object)
{
  PnDistortion *distortion;

  distortion = (PnDistortion *) object;

  pn_object_unref (PN_OBJECT (distortion->distortion_script));
  pn_object_unref (PN_OBJECT (distortion->symbol_table));
}  

/* FIXME: optimize this */
static void
pn_distortion_prepare (PnDistortion *distortion, PnImage *image)
{
  PnBooleanOption *polar_coords_opt;
  PnStringOption *distortion_script_opt;
  gboolean polar;
  guint width, stride, height;
  guint i, j;
  gdouble half_width, neg_half_height;
  gdouble xfrac, yfrac;
  gdouble x, y;

  g_return_if_fail (distortion != NULL);
  g_return_if_fail (PN_IS_DISTORTION (distortion));
  g_return_if_fail (image != NULL);
  g_return_if_fail (PN_IS_IMAGE (image));

  /* Parse the script strings */
  distortion_script_opt =
    (PnStringOption *) pn_actuator_get_option_by_index (PN_ACTUATOR (distortion),
							PN_DISTORTION_OPT_DISTORTION_SCRIPT);

  pn_script_parse_string (distortion->distortion_script,
			  distortion->symbol_table,
			  pn_string_option_get_value (distortion_script_opt));

  /* Set up the new pixel distortion map */
  if (distortion->map)
    g_free (distortion->map);

  polar_coords_opt =
    (PnBooleanOption *) pn_actuator_get_option_by_index (PN_ACTUATOR (distortion),
							 PN_DISTORTION_OPT_POLAR_COORDS);
  polar = pn_boolean_option_get_value (polar_coords_opt);
  width = pn_image_get_width (image);
  stride = pn_image_get_pitch (image) >> 2;
  height = pn_image_get_height (image);

  half_width = (((gdouble) width) - 1.0) * 0.5;
  neg_half_height = (((gdouble) height) - 1.0) * -0.5;

  distortion->map = g_new (PnPixelDistortion, width * height);

  for (j=0; j < height; j++)
    for (i=0; i < width; i++)
      {
	PN_VARIABLE_VALUE (distortion->x_var) = (((gdouble) i) / half_width) - 1.0;
	PN_VARIABLE_VALUE (distortion->y_var) = (((gdouble) j) / neg_half_height) + 1.0;
	PN_VARIABLE_VALUE (distortion->r_var) =
	  sqrt ((PN_VARIABLE_VALUE (distortion->x_var) * PN_VARIABLE_VALUE (distortion->x_var))
		+ (PN_VARIABLE_VALUE (distortion->y_var) * PN_VARIABLE_VALUE (distortion->y_var)));
	if (PN_VARIABLE_VALUE (distortion->r_var) != 0)
	  {
	    PN_VARIABLE_VALUE (distortion->theta_var) = acos (PN_VARIABLE_VALUE (distortion->x_var)
							       / PN_VARIABLE_VALUE (distortion->r_var));
	    if (PN_VARIABLE_VALUE (distortion->y_var) < 0)
	      PN_VARIABLE_VALUE (distortion->theta_var) = 2.0 * G_PI
		- PN_VARIABLE_VALUE (distortion->theta_var);
	  }
	else
	  PN_VARIABLE_VALUE (distortion->theta_var) = 0.0;
	PN_VARIABLE_VALUE (distortion->intensity_var) = 1.0;

	pn_script_execute (distortion->distortion_script);

	if (polar)
	  {
	    PN_VARIABLE_VALUE (distortion->x_var) = PN_VARIABLE_VALUE (distortion->r_var)
	      * cos (PN_VARIABLE_VALUE (distortion->theta_var));
	    PN_VARIABLE_VALUE (distortion->y_var) = PN_VARIABLE_VALUE (distortion->r_var)
	      * sin (PN_VARIABLE_VALUE (distortion->theta_var));
	  }

	if (PN_VARIABLE_VALUE (distortion->x_var) <= 1.0
	    && PN_VARIABLE_VALUE (distortion->x_var) >= -1.0
	    && PN_VARIABLE_VALUE (distortion->y_var) <= 1.0
	    && PN_VARIABLE_VALUE (distortion->y_var) >= -1.0)
	  {
	    x = (PN_VARIABLE_VALUE (distortion->x_var) + 1.0) * half_width;
	    xfrac = modf (x, &x);
	    yfrac = modf ((PN_VARIABLE_VALUE (distortion->y_var) - 1.0) * neg_half_height, &y);

	    distortion->map[i + (j * width)].offset =  ((guint) x) + (((guint) y) * stride);

	    PN_VARIABLE_VALUE (distortion->intensity_var) =
	      CLAMP (PN_VARIABLE_VALUE (distortion->intensity_var), 0.0, 1.0);

	    distortion->map[i + (j * width)].se = xfrac * yfrac * 128.0 *
	      PN_VARIABLE_VALUE (distortion->intensity_var);
	    distortion->map[i + (j * width)].sw = (1.0 - xfrac) * yfrac * 128.0 *
	      PN_VARIABLE_VALUE (distortion->intensity_var);
	    distortion->map[i + (j * width)].ne = xfrac * (1.0 - yfrac) * 128.0 *
	      PN_VARIABLE_VALUE (distortion->intensity_var);
	    distortion->map[i + (j * width)].nw = (128.0 * PN_VARIABLE_VALUE (distortion->intensity_var))
	      - (distortion->map[i + (j * width)].se
		 + distortion->map[i + (j * width)].sw
		 + distortion->map[i + (j * width)].ne);
	  }
	else
	  distortion->map[i + (j * stride)].offset = PN_PIXEL_DISTORTION_NO_PIXEL;
      }

  if (parent_class->prepare)
    parent_class->prepare (PN_ACTUATOR (distortion), image);
}

static inline void
pn_distortion_translate (PnDistortion *distortion, PnImage *image)
{
  register PnPixelDistortion *pixtrans;
  register PnColor *src;
  PnColor *dest, *src_start;
  guint i, j;
  guint width, stride, height;
  register guint red_sum, green_sum, blue_sum;

  pixtrans = distortion->map;
  src_start = pn_image_get_image_buffer (image);
  dest = pn_image_get_transform_buffer (image);

  width = pn_image_get_width (image);
  stride = pn_image_get_pitch (image) >> 2;
  height = pn_image_get_height (image);

  for (j=0; j < height; j++)
    {
      for (i=0; i < width; i++)
	{
	  if (pixtrans->offset != PN_PIXEL_DISTORTION_NO_PIXEL)
	    {
	      src = src_start + pixtrans->offset;

	      red_sum = src->red * pixtrans->nw;
	      green_sum = src->green * pixtrans->nw;
	      blue_sum = src->blue * pixtrans->nw;

	      src++;

	      red_sum += src->red * pixtrans->ne;
	      green_sum += src->green * pixtrans->ne;
	      blue_sum += src->blue * pixtrans->ne;

	      src += stride;

	      red_sum += src->red * pixtrans->se;
	      green_sum += src->green * pixtrans->se;
	      blue_sum += src->blue * pixtrans->se;

	      src--;

	      red_sum += src->red * pixtrans->sw;
	      green_sum += src->green * pixtrans->sw;
	      blue_sum += src->blue * pixtrans->sw;

	      dest->red = (guchar) (red_sum >> 7);
	      dest->green = (guchar) (green_sum >> 7);
	      dest->blue = (guchar) (blue_sum >> 7);
	    }
	  else
	    *((gulong *)dest) = 0;

	  pixtrans++;
	  dest++;
	}
      dest += stride - width;
    }
}

#ifdef PN_USE_MMX
static inline void
pn_distortion_translate_mmx (PnDistortion *distortion, PnImage *image)
{
  __asm__ __volatile__ (
			".align 16\n"                   /* The start of the loop */
			"10:\n\t"

			"pxor %%mm7,%%mm7\n\t"          /* zero the reg for unpacking */

			"movd (%0,%2,4),%%mm0\n\t"      /* load the NW pixel */
			"movd 4(%0,%2,4),%%mm1\n\t"     /* load the NE pixel */
			"addl %6,%2\n\t"                /* advance the offset to the next line */
			"movd (%0,%2,4),%%mm2\n\t"      /* load the SW pixel */
			"movd 4(%0,%2,4),%%mm3\n\t"     /* load the SE pixel */

			"punpcklbw %%mm7,%%mm0\n\t"     /* unpack the NW pixel */
			"punpcklbw %%mm7,%%mm1\n\t"     /* unpack the NE pixel */
			"punpcklbw %%mm7,%%mm2\n\t"     /* unpack the SW pixel */
			"punpcklbw %%mm7,%%mm3\n\t"     /* unpack the SE pixel */

			"movd 4(%4),%%mm4\n\t"          /* load the weights */
			"punpcklbw %%mm7,%%mm4\n\t"     /* unpack the weights to words */
			"movq %%mm4,%%mm6\n\t"          /* copy the weights for separate expansion */
			"punpcklwd %%mm4,%%mm4\n\t"     /* expand the top weights */
			"punpckhwd %%mm6,%%mm6\n\t"     /* expand the bottom weights */
			"movq %%mm4,%%mm5\n\t"          /* copy the top pixels for separate expansion */
			"movq %%mm6,%%mm7\n\t"          /* copy the bottom pixels for separate expansion */
			"punpckldq %%mm4,%%mm4\n\t"     /* expand the NW weight */
			"punpckhdq %%mm5,%%mm5\n\t"     /* expand the NE weight */
			"punpckldq %%mm6,%%mm6\n\t"     /* expand the SW weight */
			"punpckhdq %%mm7,%%mm7\n\t"     /* expand the SE weight */

			"pmullw %%mm4,%%mm0\n\t"        /* weight the NW pixel */
			"pmullw %%mm5,%%mm1\n\t"        /* weight the NE pixel */
			"pmullw %%mm6,%%mm2\n\t"        /* weight the SW pixel */
			"pmullw %%mm7,%%mm3\n\t"        /* weight the SE pixel */

			"paddusw %%mm1,%%mm0\n\t"       /* sum up the weighted pixels */
			"paddusw %%mm2,%%mm0\n\t"
			"paddusw %%mm3,%%mm0\n\t"

			"addl $4,%1\n\t"                /* advance the dest pointer */

			"psrlw $7,%%mm0\n\t"            /* divide the sums by 128 */
			"packuswb %%mm0,%%mm0\n\t"      /* pack up the resulting pixel */
			"movd %%mm0,(%1)\n\t"           /* write the pixel */
			

			"addl $8,%4\n\t"                /* advance the map pointer */
			"movl (%4),%2\n\t"              /* load the next offset */

			"dec %3\n\t"                    /* advance the inverse column counter */
			"jg 10b\n\t"

			"shll $2,%5\n\t"                /* get the width/stride in bytes */
			"shll $2,%6\n\t"
			"addl %6,%1\n\t"                /* add (stride - width) to the dest */
			"subl %5,%1\n\t"
			"shrl $2,%5\n\t"                /* revert width/stride to pixels */
			"shrl $2,%6\n\t"
			"movl %5,%3\n\t"                /* reset the inverse column counter */

			"cmpl %7,%1\n\t"                /* see if we're done */
			"jl 10b\n\t"

			"emms\n\t"
			:
			:
			"r" (pn_image_get_image_buffer (image)),
			"r" (pn_image_get_transform_buffer (image) - 1),
			"r" (distortion->map->offset),
			"r" (pn_image_get_width (image)),
			"r" (distortion->map),
			"m" (pn_image_get_width (image)),
			"m" (pn_image_get_pitch (image) >> 2),
			"m" (pn_image_get_transform_buffer (image)
			     + ((pn_image_get_pitch (image) >> 2) * (pn_image_get_height (image) - 1))));
}
#endif /* PN_USE_MMX */

static void
pn_distortion_execute (PnDistortion *distortion, PnImage *image, PnAudioData *audio_data)
{
  g_return_if_fail (distortion != NULL);
  g_return_if_fail (PN_IS_DISTORTION (distortion));
  g_return_if_fail (image != NULL);
  g_return_if_fail (PN_IS_IMAGE (image));
  g_return_if_fail (audio_data != NULL);
  g_return_if_fail (PN_IS_AUDIO_DATA (audio_data));

#ifdef PN_USE_MMX
  if (pn_cpu_get_caps () & PN_CPU_CAP_MMX)
    pn_distortion_translate_mmx (distortion, image);
  else
#endif /* PN_USE_MMX */
    pn_distortion_translate (distortion, image);

  pn_image_apply_transform (image);
}

PnDistortion*
pn_distortion_new (void)
{
  return (PnDistortion *) g_object_new (PN_TYPE_DISTORTION, NULL);
}

