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
#include "pnflip.h"
#include "pnbooleanoption.h"
#include "pnlistoption.h"
#include "pncpu.h"

enum
{
  PN_FLIP_DIRECTION_HORIZONTAL,
  PN_FLIP_DIRECTION_VERTICAL
};

static void         pn_flip_class_init       (PnFlipClass *class);
static void         pn_flip_init             (PnFlip *flip,
					      PnFlipClass *class);
/* PnActuator methods */
static void         pn_flip_execute          (PnFlip *flip,
					      PnImage *image,
					      PnAudioData *audio_data);

static PnActuatorClass *parent_class = NULL;

GType
pn_flip_get_type (void)
{
  static GType flip_type = 0;

  if (! flip_type)
    {
      static const GTypeInfo flip_info =
      {
	sizeof (PnFlipClass),
	NULL,		/* base_init */
	NULL,		/* base_finalize */
	(GClassInitFunc) pn_flip_class_init,
	NULL,		/* class_finalize */
	NULL,		/* class_data */
        sizeof (PnFlip),
	0,              /* n_preallocs */
	(GInstanceInitFunc) pn_flip_init
      };

      /* FIXME: should this be dynamic? */
      flip_type = g_type_register_static (PN_TYPE_ACTUATOR,
						 "PnFlip",
						 &flip_info,
						 0);
    }
  return flip_type;
}

static void
pn_flip_class_init (PnFlipClass *class)
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

  /* PnActuator methods */
  actuator_class->execute = (PnActuatorExecFunc) pn_flip_execute;
}

static void
pn_flip_init (PnFlip *flip, PnFlipClass *class)
{
  PnBooleanOption *blend_opt;
  PnListOption *direction_opt;

  /* Set up the name and description */
  pn_user_object_set_name (PN_USER_OBJECT (flip), "Transform.Flip");
  pn_user_object_set_description (PN_USER_OBJECT (flip),
				  "Flip the image");

  /* Set up the options */
  blend_opt = pn_boolean_option_new ("blend", "Blend the flipped image with the original");
  direction_opt = pn_list_option_new ("direction", "The direction in which the image will be flipped");

  pn_list_option_add_item (direction_opt, "Horizontal");
  pn_list_option_add_item (direction_opt, "Vertical");

  pn_actuator_add_option (PN_ACTUATOR (flip), PN_OPTION (blend_opt));
  pn_actuator_add_option (PN_ACTUATOR (flip), PN_OPTION (direction_opt));
}

static void
pn_flip_execute_horizontal (PnImage *image)
{
  register guint i, j, tmp;
  register guint32 *src, *dest;
  guint width, height, stride;

  width = pn_image_get_width (image);
  height = pn_image_get_height (image);
  stride = pn_image_get_pitch (image) >> 2;

  src = (guint32 *) pn_image_get_image_buffer (image);
  dest = (guint32 *) pn_image_get_transform_buffer (image);

  for (j=0; j<height; j++)
    {
      for (i=0; i<width >> 1; i++)
	{
	  tmp = src[j * stride + i];
	  dest[j * stride + i] = src[j * stride + (width - (i + 1))];
	  dest[j * stride + (width - (i + 1))] = tmp;
	}
      if (width & 0x1)
	dest[j * stride + i] = src [j * stride + i];
    }
}

static void
pn_flip_execute_horizontal_blend (PnImage *image)
{
  register guint i, j;
  register PnColor *src, *dest;
  guint width, height, stride;
  guint r, g, b;

  width = pn_image_get_width (image);
  height = pn_image_get_height (image);
  stride = pn_image_get_pitch (image) >> 2;

  src = pn_image_get_image_buffer (image);
  dest = pn_image_get_transform_buffer (image);

  for (j=0; j<height; j++)
    {
      for (i=0; i<width >> 1; i++)
	{
	  r = src[j * stride + i].red + src[j * stride + (width - (i + 1))].red;
	  g = src[j * stride + i].green + src[j * stride + (width - (i + 1))].green;
	  b = src[j * stride + i].blue + src[j * stride + (width - (i + 1))].blue;

	  r >>= 1;
	  g >>= 1;
	  b >>= 1;

	  dest[j * stride + i].red = dest[j * stride + (width - (i + 1))].red = r;
	  dest[j * stride + i].green = dest[j * stride + (width - (i + 1))].green = g;
	  dest[j * stride + i].blue = dest[j * stride + (width - (i + 1))].blue = b;
	}
      if (width & 0x1)
	((guint32 *) dest)[j * stride + i] =
	  ((guint32 *) src)[j * stride + i];
    }
}

#ifdef PN_USE_MMX
static void
pn_flip_execute_horizontal_blend_mmx (PnImage *image)
{
  /*
   * 0: src
   * 1: dest
   * 2: left index
   * 3: right index
   * 4: inverse line counter
   * 5; half width
   * 6: width
   * 7: pitch
   */

  __asm__ __volatile__ (
			"pxor %%mm7,%%mm7\n\t"              /* Zero mm7 */

			"10:\n\t"

			"movd (%0, %2, 4), %%mm0\n\t"       /* Load the two pixels */
			"movd (%0, %3, 4), %%mm1\n\t"

			"punpcklbw %%mm7, %%mm0\n\t"        /* Unpack the pixels */
			"punpcklbw %%mm7, %%mm1\n\t"

			"paddw %%mm1, %%mm0\n\t"            /* Average the pixels */
			"psrlw $1, %%mm0\n\t"

			"packuswb %%mm7, %%mm0\n\t"         /* Pack 'er up & write it*/
			"movd %%mm0, (%1, %2, 4)\n\t"
			"movd %%mm0, (%1, %3, 4)\n\t"

			"incl %3\n\t"                       /* Advance the indexes & see if we're */
			"decl %2\n\t"                       /* done with this line */
			"jge 10b\n\t"

			"movl %5, %2\n\t"                   /* Reset the indexes */
			"movl %5, %3\n\t"
			"decl %2\n\t"

			"testl $0x1, %6\n\t"                 /* Do the stuff for an odd width */
			"jz 20f\n\t"
			"movd (%0, %3, 4), %%mm0\n\t"
			"movd %%mm0, (%1, %3, 4)\n\t"
			"incl %3\n\t"

			"20:\n\t"

			"addl %7, %0\n\t"                   /* Advance the line pointers */
			"addl %7, %1\n\t"

			"decl %4\n\t"                       /* See if we're done */
			"jnz 10b\n\t"

			"emms"
			: /* no outputs */
			: "r" (pn_image_get_image_buffer (image)),
			"r" (pn_image_get_transform_buffer (image)),
			"r" ((pn_image_get_width (image) >> 1) - 1),
			"r" ((pn_image_get_width (image) >> 1) + (pn_image_get_width (image) & 0x1 ? 1 : 0)),
			"rm" (pn_image_get_height (image)),
			"rm" (pn_image_get_width (image) >> 1),
			"rm" (pn_image_get_width (image)),
			"rm" (pn_image_get_pitch (image))
			);
}
#endif /* PN_USE_MMX */

static void
pn_flip_execute_vertical (PnImage *image)
{
  register guint i, j, tmp;
  register guint32 *src, *dest;
  guint width, height, stride;

  width = pn_image_get_width (image);
  height = pn_image_get_height (image);
  stride = pn_image_get_pitch (image) >> 2;

  src = (guint32 *) pn_image_get_image_buffer (image);
  dest = (guint32 *) pn_image_get_transform_buffer (image);

  for (j=0; j<height >> 1; j++)
    {
      for (i=0; i<width; i++)
	{
	  tmp = src[j * stride + i];
	  dest[j * stride + i] = src[(height - (j + 1)) * stride + i];
	  dest[(height - (j + 1)) * stride + i] = tmp;
	}
      if (height & 0x1)
	dest[j * stride + i] = src [j * stride + i];
    }
}

static void
pn_flip_execute_vertical_blend (PnImage *image)
{
  register guint i, j;
  register PnColor *src, *dest;
  guint width, height, stride;
  guint r, g, b;

  width = pn_image_get_width (image);
  height = pn_image_get_height (image);
  stride = pn_image_get_pitch (image) >> 2;

  src = pn_image_get_image_buffer (image);
  dest = pn_image_get_transform_buffer (image);

  for (j=0; j<height >> 1; j++)
    {
      for (i=0; i<width; i++)
	{
	  r = src[j * stride + i].red + src[(height - (j + 1)) * stride + i].red;
	  g = src[j * stride + i].green + src[(height - (j + 1)) * stride + i].green;
	  b = src[j * stride + i].blue + src[(height - (j + 1)) * stride + i].blue;

	  r >>= 1;
	  g >>= 1;
	  b >>= 1;

	  dest[j * stride + i].red = dest[(height - (j + 1)) * stride + i].red = r;
	  dest[j * stride + i].green = dest[(height - (j + 1)) * stride + i].green = g;
	  dest[j * stride + i].blue = dest[(height - (j + 1)) * stride + i].blue = b;
	}
      if (height & 0x1)
	((guint32 *) dest)[j * stride + i] =
	  ((guint32 *) src)[j * stride + i];
    }
}

#ifdef PN_USE_MMX
static void
pn_flip_execute_vertical_blend_mmx (PnImage *image)
{
  PnColor *src, *dest;
  guint pitch, width, height;

  src = pn_image_get_image_buffer (image);
  dest = pn_image_get_transform_buffer (image);
  pitch = pn_image_get_pitch (image);
  width = pn_image_get_width (image);
  height = pn_image_get_height (image);

  /*
   * 0: tsrc
   * 1: bsrc
   * 2: tdest
   * 3: bdest
   * 4: i
   * 5: (half stride) - 1
   * 6: pitch
   * 7: src
   */

  __asm__ __volatile__ (
			"pxor %%mm7,%%mm7\n\t"          /* Zero mm7 */

			"movl %5,%4\n\t"                /* Do this so gcc doesn't make %4 and
							 * %5 the same register
							 */

			"10:\n\t"

			"movq (%0, %4, 8),%%mm0\n\t"    /* Read the pixels */
			"movq (%1, %4, 8),%%mm2\n\t"
			"movq %%mm0,%%mm1\n\t"
			"movq %%mm2,%%mm3\n\t"

			"punpcklbw %%mm7,%%mm0\n\t"     /* Unpack the pixels */
			"punpckhbw %%mm7,%%mm1\n\t"
			"punpcklbw %%mm7,%%mm2\n\t"
			"punpckhbw %%mm7,%%mm3\n\t"

			"paddw %%mm2,%%mm0\n\t"         /* Average the pixels */
			"paddw %%mm3,%%mm1\n\t"
			"psrlw $1,%%mm0\n\t"
			"psrlw $1,%%mm1\n\t"

			"packuswb %%mm1,%%mm0\n\t"      /* Pack & write the pixels */
			"movq %%mm0,(%2,%4,8)\n\t"
			"movq %%mm0,(%3,%4,8)\n\t"

			"decl %4\n\t"                   /* See if we're done */
			"jns 10b\n\t"

			"movl %5,%4\n\t"                /* Reset the x-counter */

			"subl %6,%0\n\t"                /* Advance the pointers */
			"addl %6,%1\n\t"
			"subl %6,%2\n\t"
			"addl %6,%3\n\t"

			"cmpl %7,%0\n\t"                /* See if we're done */
			"jge 10b\n\t"

			"emms"
			: /* no outputs */
			: "r" (src + (pitch >> 2) * ((height >> 1) - 1)),
			"r" (src + (pitch >> 2) * ((height >> 1) + (height & 0x1 ? 1 : 0))),
			"r" (dest + (pitch >> 2) * ((height >> 1) - 1)),
			"r" (dest + (pitch >> 2) * ((height >> 1) + (height & 0x1 ? 1 : 0))),
			"r" (0),
			"rm" ((pitch >> 3) - 1),
			"rm" (pitch),
			"rm" (src)
			);
}
#endif /* PN_USE_MMX */

static void
pn_flip_execute (PnFlip *flip, PnImage *image, PnAudioData *audio_data)
{
  PnBooleanOption *blend_opt;
  PnListOption *direction_opt;

  g_return_if_fail (flip != NULL);
  g_return_if_fail (PN_IS_FLIP (flip));
  g_return_if_fail (image != NULL);
  g_return_if_fail (PN_IS_IMAGE (image));
  g_return_if_fail (audio_data != NULL);
  g_return_if_fail (PN_IS_AUDIO_DATA (audio_data));

  blend_opt = (PnBooleanOption *) pn_actuator_get_option_by_index (PN_ACTUATOR (flip), PN_FLIP_OPT_BLEND);
  direction_opt = (PnListOption *) pn_actuator_get_option_by_index (PN_ACTUATOR (flip), PN_FLIP_OPT_DIRECTION);

  switch (pn_list_option_get_index (direction_opt))
    {
    case PN_FLIP_DIRECTION_HORIZONTAL:
      if (pn_boolean_option_get_value (blend_opt))
#ifdef PN_USE_MMX
	if (pn_cpu_get_caps () & PN_CPU_CAP_MMX)
	  pn_flip_execute_horizontal_blend_mmx (image);
	else
#endif /* PN_USE_MMX */
	  pn_flip_execute_horizontal_blend (image);
      else
	pn_flip_execute_horizontal (image);
      break;
    case PN_FLIP_DIRECTION_VERTICAL:
      if (pn_boolean_option_get_value (blend_opt))
#ifdef PN_USE_MMX
	if (pn_cpu_get_caps () & PN_CPU_CAP_MMX)
	  pn_flip_execute_vertical_blend_mmx (image);
      else
#endif /* PN_USE_MMX */
	pn_flip_execute_vertical_blend (image);
      else
	pn_flip_execute_vertical (image);
      break;
    }

  pn_image_apply_transform (image);
}

PnFlip*
pn_flip_new (void)
{
  return (PnFlip *) g_object_new (PN_TYPE_FLIP, NULL);
}
