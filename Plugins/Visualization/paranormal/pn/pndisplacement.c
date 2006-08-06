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
#include "pndisplacement.h"
#include "pnbooleanoption.h"
#include "pnstringoption.h"
#include "pncpu.h"

enum
{
  PN_PIXEL_DISPLACEMENT_NO_PIXEL = 0xffffffff
};

static void         pn_displacement_class_init       (PnDisplacementClass *class);
static void         pn_displacement_init             (PnDisplacement *displacement,
						     PnDisplacementClass *class);
/* PnObject signals */
static void         pn_displacement_destroy          (PnObject *object);

/* PnActuator methods */
static void         pn_displacement_prepare          (PnDisplacement *displacement,
						     PnImage *image);
static void         pn_displacement_execute          (PnDisplacement *displacement,
						     PnImage *image,
						     PnAudioData *audio_data);

static PnActuatorClass *parent_class = NULL;

GType
pn_displacement_get_type (void)
{
  static GType displacement_type = 0;

  if (! displacement_type)
    {
      static const GTypeInfo displacement_info =
      {
	sizeof (PnDisplacementClass),
	NULL,		/* base_init */
	NULL,		/* base_finalize */
	(GClassInitFunc) pn_displacement_class_init,
	NULL,		/* class_finalize */
	NULL,		/* class_data */
        sizeof (PnDisplacement),
	0,              /* n_preallocs */
	(GInstanceInitFunc) pn_displacement_init
      };

      /* FIXME: should this be dynamic? */
      displacement_type = g_type_register_static (PN_TYPE_ACTUATOR,
						 "PnDisplacement",
						 &displacement_info,
						 0);
    }
  return displacement_type;
}

static void
pn_displacement_class_init (PnDisplacementClass *class)
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

  /* PnObject signals */
  object_class->destroy = pn_displacement_destroy;

  /* PnActuator methods */
  actuator_class->prepare = (PnActuatorPrepFunc) pn_displacement_prepare;
  actuator_class->execute = (PnActuatorExecFunc) pn_displacement_execute;
}

static void
pn_displacement_init (PnDisplacement *displacement, PnDisplacementClass *class)
{
  PnStringOption *init_script_opt, *frame_script_opt;

  /* Set up the name and description */
  pn_user_object_set_name (PN_USER_OBJECT (displacement), "Transform.Displacement");
  pn_user_object_set_description (PN_USER_OBJECT (displacement),
				  "Displaces the image");

  /* Set up the options */
  init_script_opt = pn_string_option_new ("init_script", "The initialization script");
  frame_script_opt = pn_string_option_new ("frame_script", "The per-frame script");

  pn_actuator_add_option (PN_ACTUATOR (displacement), PN_OPTION (init_script_opt));
  pn_actuator_add_option (PN_ACTUATOR (displacement), PN_OPTION (frame_script_opt));

  /* Create the script objects and symbol table */
  displacement->init_script = pn_script_new ();
  pn_object_ref (PN_OBJECT (displacement->init_script));
  pn_object_sink (PN_OBJECT (displacement->init_script));
  displacement->frame_script = pn_script_new ();
  pn_object_ref (PN_OBJECT (displacement->frame_script));
  pn_object_sink (PN_OBJECT (displacement->frame_script));
  displacement->symbol_table = pn_symbol_table_new ();
  pn_object_ref (PN_OBJECT (displacement->symbol_table));
  pn_object_sink (PN_OBJECT (displacement->symbol_table));

  /* Get the variables from the symbol table */
  displacement->x_var = pn_symbol_table_ref_variable_by_name (displacement->symbol_table, "x");
  displacement->y_var = pn_symbol_table_ref_variable_by_name (displacement->symbol_table, "y");
  displacement->volume_var = pn_symbol_table_ref_variable_by_name (displacement->symbol_table, "volume");
}

static void
pn_displacement_destroy (PnObject *object)
{
  PnDisplacement *displacement;

  displacement = (PnDisplacement *) object;

  pn_object_unref (PN_OBJECT (displacement->init_script));
  pn_object_unref (PN_OBJECT (displacement->frame_script));
  pn_object_unref (PN_OBJECT (displacement->symbol_table));
}

/* FIXME: optimize this */
static void
pn_displacement_prepare (PnDisplacement *displacement, PnImage *image)
{
  PnStringOption *init_script_opt, *frame_script_opt;

  g_return_if_fail (displacement != NULL);
  g_return_if_fail (PN_IS_DISPLACEMENT (displacement));
  g_return_if_fail (image != NULL);
  g_return_if_fail (PN_IS_IMAGE (image));

  /* Parse the script strings */
  init_script_opt =
    (PnStringOption *) pn_actuator_get_option_by_index (PN_ACTUATOR (displacement),
							PN_DISPLACEMENT_OPT_INIT_SCRIPT);
  frame_script_opt =
    (PnStringOption *) pn_actuator_get_option_by_index (PN_ACTUATOR (displacement),
							PN_DISPLACEMENT_OPT_FRAME_SCRIPT);

  pn_script_parse_string (displacement->init_script, displacement->symbol_table,
			  pn_string_option_get_value (init_script_opt));
  pn_script_parse_string (displacement->frame_script, displacement->symbol_table,
			  pn_string_option_get_value (frame_script_opt));

  /* Set up 0 displacement */
  PN_VARIABLE_VALUE (displacement->x_var) = 0.0;
  PN_VARIABLE_VALUE (displacement->y_var) = 0.0;

  /* Run the init script */
  pn_script_execute (displacement->init_script);

  if (PN_ACTUATOR_CLASS (parent_class)->prepare)
    PN_ACTUATOR_CLASS (parent_class)->prepare (PN_ACTUATOR (displacement), image);
}

static void
pn_displacement_exec (PnImage *image, gint offset, guint nw, guint ne, guint sw, guint se,
		      gint startx, gint starty, gint endx, gint endy)
{
  gint i, j, width, height, stride;
  PnColor *src, *dest, zero = {0,0,0,0};
  guint r, g, b;

  width = pn_image_get_width (image);
  height = pn_image_get_height (image);
  stride = pn_image_get_pitch (image) >> 2;

  src = pn_image_get_image_buffer (image);
  dest = pn_image_get_transform_buffer (image);

  for (j=0; j<height; j++)
    for (i=0; i<width; i++)
      {
	if (i < startx || i >= endx
	    || j < starty || j >= endy)
	  dest[j * stride + i] = zero;
	else
	  {
	    r = src[(j * stride + i) + offset].red * nw;
	    g = src[(j * stride + i) + offset].green * nw;
	    b = src[(j * stride + i) + offset].blue * nw;

	    r += src[(j * stride + (i + 1)) + offset].red * ne;
	    g += src[(j * stride + (i + 1)) + offset].green * ne;
	    b += src[(j * stride + (i + 1)) + offset].blue * ne;

	    r += src[((j + 1) * stride + i) + offset].red * sw;
	    g += src[((j + 1) * stride + i) + offset].green * sw;
	    b += src[((j + 1) * stride + i) + offset].blue * sw;

	    r += src[((j + 1) * stride + (i + 1)) + offset].red * se;
	    g += src[((j + 1) * stride + (i + 1)) + offset].green * se;
	    b += src[((j + 1) * stride + (i + 1)) + offset].blue * se;

	    dest[j * stride + i].red = r >> 7;
	    dest[j * stride + i].green = g >> 7;
	    dest[j * stride + i].blue = b >> 7;
	  }
      }
}

#ifdef PN_USE_MMX
static void
pn_displacement_exec_mmx (PnImage *image, gint offset, guint nw, guint ne, guint sw, guint se,
			  gint startx, gint starty, gint endx, gint endy)
{
  PnColor *src, *dest, *endptr;
  gint width, height, stride, i;
  guint64 upnw; /* a place to store the unpacked se weight */

  width = pn_image_get_width (image);
  height = pn_image_get_height (image);
  stride = pn_image_get_pitch (image) >> 2;

  src = pn_image_get_image_buffer (image);
  dest = pn_image_get_transform_buffer (image);

  memset (dest, 0, pn_image_get_pitch (image) * starty);
  for (i=MAX(starty, 0); i<endy; i++)
    {
      memset (dest + i * stride, 0, startx * sizeof (PnColor));
      memset (dest + i * stride + endx, 0, (width - endx) * sizeof (PnColor));
    }
  memset (dest + endy * stride, 0, (height - endy) * pn_image_get_pitch (image));

  src += (starty * stride) + offset;
  endptr = src + endx;
  src += startx;
  dest += (starty * stride) + startx;

  __asm__ __volatile__ (
			/* Unpack the weights */
			"pxor %%mm7,%%mm7\n\t"          /* zero mm7 for unpacking */
			"movd %8,%%mm4\n\t"             /* load the weights */
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
			"movq %%mm7,%4\n\t"             /* save the SE weight so we can use mm7 as 0 */
			"pxor %%mm7,%%mm7\n\t"          /* re-zero mm7 */

			"10:\n\t"

			/* zero mm7 for unpacking */
			"pxor %%mm7,%%mm7\n\t"

			/* load & unpack the pixels */
			"movd (%0),%%mm0\n\t"
			"punpcklbw %%mm7,%%mm0\n\t"
			"movd 4(%0),%%mm1\n\t"
			"punpcklbw %%mm7,%%mm1\n\t"
			"movd (%0,%5),%%mm2\n\t"
			"punpcklbw %%mm7,%%mm2\n\t"
			"movd 4(%0,%5),%%mm3\n\t"
			"punpcklbw %%mm7,%%mm3\n\t"

			/* reload the unpacked se weight */
			"movq %4,%%mm7\n\t"

			/* weight each pixel */
			"pmullw %%mm4,%%mm0\n\t"
			"pmullw %%mm5,%%mm1\n\t"
			"pmullw %%mm6,%%mm2\n\t"
			"pmullw %%mm7,%%mm3\n\t"

			/* add up the weighted pixels */
			"paddw %%mm1,%%mm0\n\t"
			"paddw %%mm2,%%mm0\n\t"
			"paddw %%mm3,%%mm0\n\t"

			/* normalize the resulting pixel, pack it, and write it */
			"psrlw $7,%%mm0\n\t"
			"packuswb %%mm0,%%mm0\n\t"
			"movd %%mm0,(%1)\n\t"

			/* advance the pointers */
			"addl $4,%0\n\t"
			"addl $4,%1\n\t"

			/* see if we're done with this line */
			"cmpl %3,%0\n\t"
			"jl 10b\n\t"

			/* advance the pointers */
			"addl %7,%0\n\t"
			"addl %7,%1\n\t"
			"addl %5,%3\n\t"

			/* see if we're done */
			"incl %2\n\t"
			"cmpl %6,%2\n\t"
			"jl 10b\n\t"

			"emms\n\t"

			: /* no outputs */
			: "r" (src), /* 0 */
			"r" (dest), /* 1 */
			"r" (starty), /* 2 */
			"m" (endptr), /* 3 */
			"m" (upnw), /* 4 */
			"r" (pn_image_get_pitch (image)),  /* 5 */
			"r" (endy), /* 6 */
			"rm" (((stride - endx) + startx) * sizeof (PnColor)), /* 7 */
			"rm" ((guint32)((se << 24) | (sw << 16) | (ne << 8) | nw ))); /* 8 */
}
#endif /* PN_USE_MMX */

static void
pn_displacement_execute (PnDisplacement *displacement, PnImage *image, PnAudioData *audio_data)
{
  g_return_if_fail (displacement != NULL);
  g_return_if_fail (PN_IS_DISPLACEMENT (displacement));
  g_return_if_fail (image != NULL);
  g_return_if_fail (PN_IS_IMAGE (image));
  g_return_if_fail (audio_data != NULL);
  g_return_if_fail (PN_IS_AUDIO_DATA (audio_data));

  /* set up the volume in-script variable */
  PN_VARIABLE_VALUE (displacement->volume_var) = pn_audio_data_get_volume (audio_data);

  /* run the frame scipt */
  pn_script_execute (displacement->frame_script);

  if (fabs (PN_VARIABLE_VALUE (displacement->x_var)) <= 2.0
      && fabs (PN_VARIABLE_VALUE (displacement->y_var)) <= 2.0)
    {
      guint nw, ne, sw, se;
      gint startx, starty, endx, endy;
      gint offset;
      gdouble x, y, xfrac, yfrac;

      x = -PN_VARIABLE_VALUE (displacement->x_var) * (pn_image_get_width (image) >> 1);
      y = PN_VARIABLE_VALUE (displacement->y_var) * (pn_image_get_height (image) >> 1);

      xfrac = modf (x, &x);
      yfrac = modf (y, &y);

      if (x < 0.0 || xfrac < 0.0)
	{
	  x--;
	  xfrac++;
	}
      if (y < 0.0 || yfrac < 0.0)
	{
	  y--;
	  yfrac++;
	}

      se = xfrac * yfrac * 128.0;
      sw = (1.0 - xfrac) * yfrac * 128.0;
      ne = xfrac * (1.0 - yfrac) * 128.0;
      nw = 128 - (se + sw + ne);

      offset = (y * (pn_image_get_pitch (image) >> 2)) + x;

      startx = -x;
      starty = -y;
      endx = pn_image_get_width (image) - (x+1);
      endy = pn_image_get_height (image) - (y+1);

      startx = MAX (startx, 0);
      starty = MAX (starty, 0);
      endx = MIN (endx, pn_image_get_width (image));
      endy = MIN (endy, pn_image_get_height (image));

      /* I'm too lazy to special case the rightmost & bottommost pixels */
      if (endx == pn_image_get_width (image))
	endx--;
      if (endy == pn_image_get_height (image))
	endy--;
      
#ifdef PN_USE_MMX
      if (pn_cpu_get_caps () & PN_CPU_CAP_MMX)
	pn_displacement_exec_mmx (image, offset, nw, ne, sw, se, startx, starty, endx, endy);
      else
#endif /* PN_USE_MMX */
	pn_displacement_exec (image, offset, nw, ne, sw, se, startx, starty, endx, endy);
    }
  else
    memset (pn_image_get_transform_buffer (image), 0,
	    pn_image_get_pitch (image) * pn_image_get_height (image));

  pn_image_apply_transform (image);

  if (PN_ACTUATOR_CLASS (parent_class)->execute)
    PN_ACTUATOR_CLASS (parent_class)->execute (PN_ACTUATOR (displacement), image, audio_data);
}

PnDisplacement*
pn_displacement_new (void)
{
  return (PnDisplacement *) g_object_new (PN_TYPE_DISPLACEMENT, NULL);
}
