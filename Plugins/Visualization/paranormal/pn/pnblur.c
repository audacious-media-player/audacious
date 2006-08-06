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
#include "pnblur.h"
#include "pncpu.h"

static void         pn_blur_class_init       (PnBlurClass *class);
static void         pn_blur_init             (PnBlur *blur,
					      PnBlurClass *class);

/* PnActuator methods */
static void         pn_blur_execute          (PnBlur *blur,
					      PnImage *image,
					      PnAudioData *audio_data);

static PnActuatorClass *parent_class = NULL;

GType
pn_blur_get_type (void)
{
  static GType blur_type = 0;

  if (! blur_type)
    {
      static const GTypeInfo blur_info =
      {
	sizeof (PnBlurClass),
	NULL,		/* base_init */
	NULL,		/* base_finalize */
	(GClassInitFunc) pn_blur_class_init,
	NULL,		/* class_finalize */
	NULL,		/* class_data */
        sizeof (PnBlur),
	0,              /* n_preallocs */
	(GInstanceInitFunc) pn_blur_init
      };

      /* FIXME: should this be dynamic? */
      blur_type = g_type_register_static (PN_TYPE_ACTUATOR,
					      "PnBlur",
					      &blur_info,
					      0);
    }
  return blur_type;
}

static void
pn_blur_class_init (PnBlurClass *class)
{
  PnObjectClass *object_class;
  PnUserObjectClass *user_object_class;
  PnActuatorClass *actuator_class;

  parent_class = g_type_class_peek_parent (class);

  object_class = (PnObjectClass *) class;
  user_object_class = (PnUserObjectClass *) class;
  actuator_class = (PnActuatorClass *) class;

  /* PnActuator methods */
  actuator_class->execute = (PnActuatorExecFunc) pn_blur_execute;
}

static void
pn_blur_init (PnBlur *blur, PnBlurClass *class)
{
  /* Set up the name and description */
  pn_user_object_set_name (PN_USER_OBJECT (blur), "Transform.Blur");
  pn_user_object_set_description (PN_USER_OBJECT (blur),
				  "Blur the image");
}

static inline void
pn_blur_execute_medium (PnImage *image)
{
  register PnColor *src_left;
  PnColor *src_top, *src_bot, *src_right;
  PnColor *dest, *dest_last;
  guint width, width_m1, total_width, height, height_m1;
  register guint red_sum, green_sum, blue_sum;
  guint i = 0;

  width = pn_image_get_width (image);
  width_m1 = width - 1;
  total_width = pn_image_get_pitch (image)>>2;
  height = pn_image_get_height (image);
  height_m1 = height - 1;

  dest = pn_image_get_transform_buffer (image);
  src_left = pn_image_get_image_buffer (image) - 1;
  src_right = src_left + 2;
  src_top = (src_left - total_width) + 1;
  src_bot = (src_left + total_width) + 1;

  src_top += total_width;
  src_bot += total_width;
  src_left += total_width;
  src_right += total_width;
  dest += total_width;

  dest_last = pn_image_get_transform_buffer (image) + (total_width * (height-1));
  while (dest < dest_last)
    {
      for (i=0; i < width; i++)
	{
	  /* Top pixel */
	  red_sum = src_top->red;
	  green_sum = src_top->green;
	  blue_sum = src_top->blue;

	  /* Bottom pixel */
	  red_sum += src_bot->red;
	  green_sum += src_bot->green;
	  blue_sum += src_bot->blue;

	  /* Left pixel */
	  if (i != 0)
	    {
	      red_sum += src_left->red;
	      green_sum += src_left->green;
	      blue_sum += src_left->blue;
	    }
	  else
	    {
	      red_sum += (src_left+1)->red;
	      green_sum += (src_left+1)->green;
	      blue_sum += (src_left+1)->blue;
	    }

	  /* Right pixel */
	  if (i != width_m1)
	    {
	      red_sum += src_right->red;
	      green_sum += src_right->green;
	      blue_sum += src_right->blue;
	    }
	  else
	    {
	      red_sum += (src_left+1)->red;
	      green_sum += (src_left+1)->green;
	      blue_sum += (src_left+1)->blue;
	    }

	  src_left++;
	  src_right++;
	  src_top++;
	  src_bot++;

	  /*  	  red_sum *= 3; */
	  /*  	  green_sum *= 3; */
	  /*  	  blue_sum *= 3; */

	  /* Center pixel */
	  red_sum += src_left->red << 2;
	  green_sum += src_left->green << 2;
	  blue_sum += src_left->blue << 2;

	  dest->red = (guchar)(red_sum >> 3);
	  dest->green = (guchar)(green_sum >> 3);
	  dest->blue = (guchar)(blue_sum >> 3);
	  dest++;
	}
      src_left += total_width - width;
      src_right += total_width - width;
      src_bot += total_width - width;
      src_top += total_width - width;
      dest += total_width - width;
    }
}

#ifdef PN_USE_MMX
static void
pn_blur_execute_medium_mmx (PnImage *image)
{
  guint width, pitch;
  PnColor *src, *dest, *dest_last;

  width = pn_image_get_width (image);
  pitch = pn_image_get_pitch (image);

  src = pn_image_get_image_buffer (image) - 1;
  dest = pn_image_get_transform_buffer (image) + (pitch>>2);
  dest_last = dest + ((pitch>>2) * (pn_image_get_height (image) - 2));

  /*
   * ecx = x counter
   */

  /*
   * src  X    X    X
   * X    X    X    X
   * X    X    X    X
   */

  /*
   * X    X    X    X
   * X    dest X    X
   * X    X    X    X
   */

  /*
   * %0 = src
   * %1 = dest
   * %2 = pitch
   * %3 = width
   * %4 = dest_last
   */
  
  /*
   * mm0 = left dest pixel sum
   * mm1 = right dest pixel sum
   * mm6 = unpacked leftmost source pixel
   * mm7 = 0
   */

  __asm__ __volatile__ (
			"movl %3,%%ecx\n\t"                 /* start with x = 0 */

			"pxor %%mm7,%%mm7\n\t"                 /* set up the zero mmx register used
								* in unpacking
								*/

			"pxor %%mm6,%%mm6\n\t"                  /* start with the leftmost pixel = zero */
								 

			"10:\n\t"                              /* begin the inner loop */

			"movq 4(%0),%%mm0\n\t"                 /* load and unpack the top and bottom */
			"movq 4(%0,%2,2),%%mm2\n\t"            /* row of pixels	*/
			"movq %%mm0,%%mm1\n\t"
			"movq %%mm2,%%mm3\n\t"
			"punpcklbw %%mm7,%%mm0\n\t"            /* tops */
			"punpckhbw %%mm7,%%mm1\n\t"
			"punpcklbw %%mm7,%%mm2\n\t"            /* bottoms */
			"punpckhbw %%mm7,%%mm3\n\t"

			"paddw %%mm2,%%mm0\n\t"                /* add the two top & bottom pixels */
			"paddw %%mm3,%%mm1\n\t"

			"paddw %%mm6,%%mm0\n\t"                /* add the left pixel */

			"movq 4(%0,%2),%%mm2\n\t"              /* load and unpack the two center pixels */
			"movq %%mm2,%%mm3\n\t"
			"punpcklbw %%mm7,%%mm2\n\t"            /* used as quadruple-weighted center pixels */
			"punpckhbw %%mm7,%%mm3\n\t"

			"movq %%mm3,%%mm6\n\t"                 /* save the unpacked right-center pixel
								* for the next iteration's leftmost pixel
								*/

			"cmpl $1,%%ecx\n\t"                    /* make sure the right-center pixel is on
								* the image
								*/
			"je 20f\n\t"

			"movd 12(%0,%2),%%mm5\n\t"             /* load and unpack the right pixel */
			"punpcklbw %%mm7,%%mm5\n\t"
			"paddw %%mm5,%%mm1\n\t"                /* add the right pixel */

			"paddw %%mm3,%%mm0\n\t"                /* add the center pixels as adjecent pixels */
			"paddw %%mm2,%%mm1\n\t"
			"20:\n\t"

			"psllw $2,%%mm2\n\t"                   /* multiply the center pixels by 4 */
			"psllw $2,%%mm3\n\t"

			/* multiply the sums (mm0 and mm1) by 3 for the heavy blur here */

			"paddw %%mm2,%%mm0\n\t"                /* add the center pixels as center pixels */
			"paddw %%mm3,%%mm1\n\t"

			"psrlw $3,%%mm0\n\t"                   /* normalize the values - should be 4 for */
			"psrlw $3,%%mm1\n\t"                   /* heavy blur */

			"packuswb %%mm1,%%mm0\n\t"             /* repack it all and write it */
			"movq %%mm0,(%1)\n\t"

			"addl $8,%0\n\t"                       /* advance the pointers */
			"addl $8,%1\n\t"

			"subl $2,%%ecx\n\t"                    /* decrement inverse x counter */
			"jg 10b\n\t"

			"movl %3,%%ecx\n\t"                    /* start a new line */
			"pxor %%mm6,%%mm6\n\t"

			"cmpl %1,%4\n\t"                       /* if we're all done then stop */
			"jg 10b\n\t"

			"emms"                                 /* all done! */
			: /* no outputs */
			: "r" (src), "r" (dest), "r" (pitch), "r" (width), "m" (dest_last)
			: "ecx"
			);
}
#endif /* USE_MMX */

static void
pn_blur_execute (PnBlur *blur, PnImage *image, PnAudioData *audio_data)
{
  g_return_if_fail (blur != NULL);
  g_return_if_fail (PN_IS_BLUR (blur));
  g_return_if_fail (image != NULL);
  g_return_if_fail (PN_IS_IMAGE (image));
  g_return_if_fail (audio_data != NULL);
  g_return_if_fail (PN_IS_AUDIO_DATA (audio_data));

#ifdef PN_USE_MMX
  if (pn_cpu_get_caps () & PN_CPU_CAP_MMX)
    pn_blur_execute_medium_mmx (image);
  else
#endif /* USE_MMX */
    pn_blur_execute_medium (image);

  pn_image_apply_transform (image);
}

PnBlur*
pn_blur_new (void)
{
  return (PnBlur *) g_object_new (PN_TYPE_BLUR, NULL);
}
