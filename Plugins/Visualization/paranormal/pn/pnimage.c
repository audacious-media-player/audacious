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
#include "pnimage.h"
#include "pnerror.h"
#include "pncpu.h"

static void         pn_image_class_init       (PnImageClass *class);
static void         pn_image_init             (PnImage *image,
					       PnImageClass *class);

/* GObject signals */
static void         pn_image_finalize         (GObject *gobject);

static PnObjectClass *parent_class = NULL;

const gchar *pn_image_blend_mode_strings[] =
{
  "Ignore",
  "Replace",
  "50/50"
};
const guint pn_image_blend_mode_count = 3;

GType
pn_image_get_type (void)
{
  static GType image_type = 0;

  if (! image_type)
    {
      static const GTypeInfo image_info =
      {
	sizeof (PnImageClass),
	NULL,		/* base_init */
	NULL,		/* base_finalize */
	(GClassInitFunc) pn_image_class_init,
	NULL,		/* class_finalize */
	NULL,		/* class_data */
        sizeof (PnImage),
	0,              /* n_preallocs */
	(GInstanceInitFunc) pn_image_init
      };

      /* FIXME: should this be dynamic? */
      image_type = g_type_register_static (PN_TYPE_OBJECT,
					 "PnImage",
					 &image_info,
					 0);
    }
  return image_type;
}

static void
pn_image_class_init (PnImageClass *class)
{
  GObjectClass *gobject_class;
  PnObjectClass *object_class;

  parent_class = g_type_class_peek_parent (class);

  gobject_class = (GObjectClass *) class;
  object_class = (PnObjectClass *) class;

  /* GObject signals */
  gobject_class->finalize = pn_image_finalize;
}

static void
pn_image_init (PnImage *image, PnImageClass *class)
{
  image->render_mode = PN_BLEND_MODE_REPLACE;
  image->transform_mode = PN_BLEND_MODE_REPLACE;
}

static void
pn_image_finalize (GObject *gobject)
{
  PnImage *image;

  image = (PnImage *) gobject;

  if (image->image_buffer)
    g_free (image->image_buffer);

  if (image->transform_buffer)
    g_free (image->transform_buffer);
}

/**
 * pn_image_new
 *
 * Creates a new #PnImage object
 *
 * Returns: The new #PnImage object
 */
PnImage*
pn_image_new (void)
{
  return (PnImage *) g_object_new (PN_TYPE_IMAGE, NULL);
}

/**
 * pn_image_new_width_size
 * @width: the width of the new image
 * @height: the hight of the new image
 *
 * Creates a new #PnImage object with the given dimensions
 *
 * Returns: The new #PnImage object
 */
PnImage*
pn_image_new_with_size (guint width, guint height)
{
  PnImage *image;

  image = (PnImage *) g_object_new (PN_TYPE_IMAGE, NULL);

  pn_image_set_size (image, width, height);

  return image;
}

/**
 * pn_image_set_size
 * @image: a #PnImage
 * @width: the new width of the image
 * @height: the new height of the image
 *
 * Sets the size of the image contained in a #PnImage object
 */
void
pn_image_set_size (PnImage *image, guint width, guint height)
{
  guint pitch;

  g_return_if_fail (image != NULL);
  g_return_if_fail (PN_IS_IMAGE (image));
  g_return_if_fail (width > 0 && height > 0);

  pitch = width * sizeof (PnColor);

  /* Align each row to 8 bytes */
  if (pitch & 0x00000007)
    pitch = (pitch & 0xfffffff8) + 8;

  if (image->image_buffer)
    g_free (image->image_buffer);

  if (image->transform_buffer)
    g_free (image->transform_buffer);

  image->pitch = pitch;
  image->width = width;
  image->height = height;
  image->image_buffer = g_malloc0 (pitch * height);
  image->transform_buffer = g_malloc0 (pitch * height);
}

/**
 * pn_image_get_width
 * @image: a #PnImage
 *
 * Gets the width of a #PnImage
 *
 * Returns: The width of the image
 */
guint
pn_image_get_width (PnImage *image)
{
  g_return_val_if_fail (image != NULL, 0);
  g_return_val_if_fail (PN_IS_IMAGE (image), 0);

  return image->width;
}

/**
 * pn_image_get_height
 * @image: a #PnImage
 *
 * Gets the height of a #PnImage
 *
 * Returns: The height of the image
 */
guint
pn_image_get_height (PnImage *image)
{
  g_return_val_if_fail (image != NULL, 0);
  g_return_val_if_fail (PN_IS_IMAGE (image), 0);

  return image->height;
}

/**
 * pn_image_get_pitch
 * @image: a #PnImage
 *
 * Gets the pitch (width in bytes) of a #PnImage
 *
 * Returns: The pitch of the image
 */
guint
pn_image_get_pitch (PnImage *image)
{
  g_return_val_if_fail (image != NULL, 0);
  g_return_val_if_fail (PN_IS_IMAGE (image), 0);

  return image->pitch;
}

/**
 * pn_image_set_render_mode
 * @image: a #PnImage
 * @render_mode: the blend mode to use
 *
 * Sets the blend mode to be used by render functions.  The
 * render functions are pn_image_render_pixel(),
 * pn_image_render_pixel_by_offset(), and pn_image_render_line().
 */
void
pn_image_set_render_mode (PnImage *image, PnBlendMode render_mode)
{
  g_return_if_fail (image != NULL);
  g_return_if_fail (PN_IS_IMAGE (image));
  g_return_if_fail (render_mode < PN_BLEND_MODE_LAST);

  image->render_mode = render_mode;
}

/**
 * pn_image_set_transform_mode
 * @image: a #PnImage
 * @transform_mode: the blend mode to use
 *
 * Sets the blend mode to be used by pn_image_apply_transform().
 */
void
pn_image_set_transform_mode (PnImage *image, PnBlendMode transform_mode)
{
  g_return_if_fail (image != NULL);
  g_return_if_fail (PN_IS_IMAGE (image));
  g_return_if_fail (transform_mode < PN_BLEND_MODE_LAST);

  image->transform_mode = transform_mode;
}

/**
 * pn_image_get_image_buffer
 * @image: a #PnImage
 *
 * Retrieves the image buffer (the 'front buffer') of a #PnImage.
 *
 * Returns: A pointer to the image buffer
 */
PnColor*
pn_image_get_image_buffer (PnImage *image)
{
  g_return_val_if_fail (image != NULL, NULL);
  g_return_val_if_fail (PN_IS_IMAGE (image), NULL);

  return image->image_buffer;
}

/**
 * pn_image_get_transform_buffer
 * @image: a #PnImage
 *
 * Retrieves the transform buffer (the 'back buffer') of a #PnImage.
 * The transform buffer should only be used internally by transform
 * actuators.  *EVERY* pixel in the transform buffer *MUST* be set.
 * The transform buffer can be 'copied' to the image buffer using
 * pn_image_apply_transform().
 *
 * Returns: A pointer to the transform buffer
 */
PnColor*
pn_image_get_transform_buffer (PnImage *image)
{
  g_return_val_if_fail (image != NULL, NULL);
  g_return_val_if_fail (PN_IS_IMAGE (image), NULL);

  return image->transform_buffer;
}

/**
 * pn_image_render_pixel
 * @image: a #PnImage
 * @x: the x coordinate (left being 0)
 * @y: the y coordinate (top being 0)
 * @color: the color
 *
 * Renders a pixel to the image buffer of a #PnImage.  pn_image_set_render_mode()
 * can be used to set the blend mode that is used by this function.
 */
void
pn_image_render_pixel (PnImage *image, guint x, guint y, PnColor color)
{
  g_return_if_fail (image != NULL);
  g_return_if_fail (PN_IS_IMAGE (image));
  g_return_if_fail (image->image_buffer != NULL);

  if (x > image->width || y > image->height)
    return;

  switch (image->render_mode)
    {
    case PN_BLEND_MODE_LAST: break;
    case PN_BLEND_MODE_IGNORE:
      break;

    case PN_BLEND_MODE_REPLACE:
      image->image_buffer[(y * (image->pitch>>2)) + x] = color;
      break;

    case PN_BLEND_MODE_5050:
      image->image_buffer[(y * (image->pitch>>2)) + x].red =
	(image->image_buffer[(y * (image->pitch>>2)) + x].red + color.red) >> 1;
      image->image_buffer[(y * (image->pitch>>2)) + x].green =
	(image->image_buffer[(y * (image->pitch>>2)) + x].green + color.green) >> 1;
      image->image_buffer[(y * (image->pitch>>2)) + x].blue =
	(image->image_buffer[(y * (image->pitch>>2)) + x].blue + color.blue) >> 1;
      break;
    }
}

/**
 * pn_image_render_pixel_by_offset
 * @image: a #PnImage
 * @offset: the pixel offset (0 being the top left) NOTE: Use (pitch>>2) rather
 * than width
 * @color: the color
 *
 * Renders a pixel to the image buffer of a #PnImage based on a pixel offset rather than
 * rectangular coordinates.  This function should be used if there is a more optimum way
 * to calculate the offset than multiplying at every pixel.  pn_image_set_render_mode()
 * can be used to set the blend mode that is used by this function.
 */
void
pn_image_render_pixel_by_offset (PnImage *image, guint offset, PnColor color)
{
  g_return_if_fail (image != NULL);
  g_return_if_fail (PN_IS_IMAGE (image));
  g_return_if_fail (image->image_buffer != NULL);

  if (offset > (image->pitch>>2) * image->height)
    return;

  switch (image->render_mode)
    {
    case PN_BLEND_MODE_LAST: break;
    case PN_BLEND_MODE_IGNORE:
      break;

    case PN_BLEND_MODE_REPLACE:
      image->image_buffer[offset] = color;
      break;

    case PN_BLEND_MODE_5050:
      image->image_buffer[offset].red =
	(image->image_buffer[offset].red + color.red) >> 1;
      image->image_buffer[offset].green =
	(image->image_buffer[offset].green + color.green) >> 1;
      image->image_buffer[offset].blue =
	(image->image_buffer[offset].blue + color.blue) >> 1;
      break;
    }
}

/* FIXME: Add clipping to this */
/**
 * pn_image_render_line
 * @image: a #PnImage
 * @x0: the x coordinate of the first point
 * @y0: the y coordinate of the first point
 * @x1: the x coordinate of the second point
 * @y1: the y coordinate of the second point
 * @color: the color
 *
 * Renders a line from (x0,y0) to (x1,y1) to the image buffer of a #PnImage.
 * Currently ***NO CLIPPING IS CURRENTLY DONE!!!***
 */
void
pn_image_render_line (PnImage *image, guint _x0, guint _y0, guint _x1, guint _y1, PnColor color)
{
  gint x0 = _x0;
  gint y0 = _y0;
  gint x1 = _x1;
  gint y1 = _y1;

  gint dy = y1 - y0;
  gint dx = x1 - x0;
  gint stepx, stepy;
  gint fraction;

  g_return_if_fail (image != NULL);
  g_return_if_fail (PN_IS_IMAGE (image));
  g_return_if_fail (image->image_buffer != NULL);

  if (dy < 0)
    {
      dy = -dy;
      stepy = -(image->pitch>>2);
    }
  else
    {
      stepy = image->pitch>>2;
    }
  if (dx < 0)
    {
      dx = -dx;
      stepx = -1;
    }
  else
    {
      stepx = 1;
    }
  dy <<= 1;
  dx <<= 1;

  y0 *= image->pitch>>2;
  y1 *= image->pitch>>2;
  pn_image_render_pixel_by_offset(image, x0+y0, color);
  if (dx > dy)
    {
      fraction = dy - (dx >> 1);
      while (x0 != x1)
	{
	  if (fraction >= 0)
	    {
	      y0 += stepy;
	      fraction -= dx;
	    }
	  x0 += stepx;
	  fraction += dy;
	  pn_image_render_pixel_by_offset (image, x0+y0, color);
	}
    }
  else
    {
      fraction = dx - (dy >> 1);
      while (y0 != y1)
	{
	  if (fraction >= 0)
	    {
	      x0 += stepx;
	      fraction -= dy;
	    }
	  y0 += stepy;
	  fraction += dx;
	  pn_image_render_pixel_by_offset (image, x0+y0, color);
	}
    }
}

static void
pn_image_bufcopy_5050 (PnColor *dest, PnColor *src, guint bufsize)
{
  register guint i;
  register PnColor *d, *s;

  d = dest;
  s = src;

  for (i=0; i<bufsize; i++)
    {
      d->red = (s->red + d->red) >> 1;
      d->green = (s->green + d->green) >> 1;
      d->blue = (s->blue + d->blue) >> 1;
      d++;
      s++;
    }
}

#ifdef PN_USE_MMX
static void
pn_image_bufcopy_5050_mmx (PnColor *dest, PnColor *src, guint bufsize)
{
  __asm__ __volatile__ (
			"pxor %%mm7,%%mm7\n\t"      /* zero mm7 */

			"10:\n\t"                   /* The start of the loop */

			"movq (%0),%%mm0\n\t"       /* Read the pixels */
			"movq (%1),%%mm2\n\t"
			"movq %%mm0,%%mm1\n\t"
			"movq %%mm2,%%mm3\n\t"

			"punpcklbw %%mm7,%%mm0\n\t" /* Unpack the pixels */
			"punpckhbw %%mm7,%%mm1\n\t"
			"punpcklbw %%mm7,%%mm2\n\t"
			"punpckhbw %%mm7,%%mm3\n\t"

			"paddw %%mm2,%%mm0\n\t"     /* Add the pixels */
			"paddw %%mm3,%%mm1\n\t"

			"psrlw $1,%%mm0\n\t"        /* Divide the pixels by 2 */
			"psrlw $1,%%mm1\n\t"

			"packuswb %%mm1,%%mm0\n\t"  /* Pack it up & write it */
			"movq %%mm0,(%0)\n\t"

			"addl $8,%0\n\t"            /* Advance the pointers */
			"addl $8,%1\n\t"

			"decl %2\n\t"               /* See if we're done */
			"jnz 10b\n\t"

			"emms"
			: /* no outputs */
			: "r" (dest), "r" (src), "r" (bufsize >> 1)
			);
}

/* FIXME: Should this be in a separate #define PN_USE_MMXEXT ? */
static void
pn_image_bufcopy_5050_mmxext (PnColor *dest, PnColor *src, guint bufsize)
{
  

  __asm__ __volatile__ (
			"10:\n\t"                   /* The non-unrolled loop */

			"decl %2\n\t"               /* See if we're done */
			"js 20f\n\t"

			"movq (%0),%%mm0\n\t"       /* Read the pixels */
			"movq (%1),%%mm1\n\t"
			"pavgb %%mm1,%%mm0\n\t"     /* Average the pixels */

			"movq %%mm0,(%0)\n\t"       /* Write the pixels */

			"addl $8,%0\n\t"            /* Advance the pointers */
			"addl $8,%1\n\t"

			"20:\n\t"                   /* The unrolled loop */

			"decl %3\n\t"               /* See if we're done */
			"js 30f\n\t"

			/* First 2 pixels */
			"movq (%0),%%mm0\n\t"       /* Read the pixels */
			"movq (%1),%%mm1\n\t"
			"pavgb %%mm1,%%mm0\n\t"     /* Average the pixels */

			/* Second 2 pixels */
			"movq 8(%0),%%mm2\n\t"      /* Read the pixels */
			"movq 8(%1),%%mm3\n\t"
			"pavgb %%mm3,%%mm2\n\t"     /* Average the pixels */

			/* Third 2 pixels */
			"movq 16(%0),%%mm4\n\t"     /* Read the pixels */
			"movq 16(%1),%%mm5\n\t"
			"pavgb %%mm5,%%mm4\n\t"     /* Average the pixels */

			/* Fourth 2 pixels */
			"movq 24(%0),%%mm6\n\t"     /* Read the pixels */
			"movq 24(%1),%%mm7\n\t"
			"pavgb %%mm7,%%mm6\n\t"     /* Average the pixels */

			/* Write them all */
			"movq %%mm0,(%0)\n\t"
			"movq %%mm2,8(%0)\n\t"
			"movq %%mm4,16(%0)\n\t"
			"movq %%mm6,24(%0)\n\t"

			"addl $32,%0\n\t"           /* Advance the pointers */
			"addl $32,%1\n\t"

			"jmp 20b\n\t"               /* And again */

			"30:\n\t"

			"emms"
			: /* no outputs */
			: "r" (dest), "r" (src), "r" ((bufsize >> 1) & 0x3), "r" (bufsize >> 3)
			);
}

#endif /* PN_USE_MMX */

/**
 * pn_image_apply_transform
 * @image: a #PnImage
 *
 * Renders the transform buffer onto the image buffer.
 * pn_image_set_transform_mode() may be used to set the blend mode that is
 * used by this function.
 */
void
pn_image_apply_transform (PnImage *image)
{
  g_return_if_fail (image != NULL);
  g_return_if_fail (PN_IS_IMAGE (image));

  switch (image->transform_mode)
    {
    case PN_BLEND_MODE_LAST:
    case PN_BLEND_MODE_IGNORE:
      return;

    case PN_BLEND_MODE_REPLACE:
      {
	PnColor *tmp = image->image_buffer;
	image->image_buffer = image->transform_buffer;
	image->transform_buffer = tmp;
      }
      break;

    case PN_BLEND_MODE_5050:
#ifdef PN_USE_MMX
      if (pn_cpu_get_caps () & PN_CPU_CAP_MMXEXT)
	pn_image_bufcopy_5050_mmxext (image->image_buffer, image->transform_buffer,
				      image->height * (image->pitch >> 2));
      else if (pn_cpu_get_caps () & PN_CPU_CAP_MMX)
	pn_image_bufcopy_5050_mmx (image->image_buffer, image->transform_buffer,
				   image->height * (image->pitch >> 2));
      else
#endif /* PN_USE_MMX */
	pn_image_bufcopy_5050 (image->image_buffer, image->transform_buffer,
			       image->height * (image->pitch >> 2));
      break;
    }
}

/**
 * pn_image_render_image
 * @image: a #PnImage
 * @src: the source image
 * @blend_mode: the blend mode to use
 *
 * Renders the image buffer of @src onto the image buffer of an image.
 */
void
pn_image_render_image (PnImage *image, PnImage *src, PnBlendMode blend_mode)
{
  g_return_if_fail (image != NULL);
  g_return_if_fail (PN_IS_IMAGE (image));
  g_return_if_fail (src != NULL);
  g_return_if_fail (PN_IS_IMAGE (src));
  g_return_if_fail (blend_mode < PN_BLEND_MODE_LAST);
  g_return_if_fail (image->width == src->width);
  g_return_if_fail (image->height == src->height);

  switch (blend_mode)
    {
    case PN_BLEND_MODE_LAST:
    case PN_BLEND_MODE_IGNORE:
      return;

    case PN_BLEND_MODE_REPLACE:
      memcpy (image->image_buffer, src->image_buffer, image->height * (image->pitch >> 2) * sizeof (PnColor));
      break;

    case PN_BLEND_MODE_5050:
#ifdef PN_USE_MMX
      if (pn_cpu_get_caps () & PN_CPU_CAP_MMXEXT)
	pn_image_bufcopy_5050_mmxext (image->image_buffer, src->image_buffer,
				      image->height * (image->pitch >> 2));
      else if (pn_cpu_get_caps () & PN_CPU_CAP_MMX)
	pn_image_bufcopy_5050_mmx (image->image_buffer, src->image_buffer, image->height * (image->pitch >> 2));
      else
#endif /* PN_USE_MMX */
	pn_image_bufcopy_5050 (image->image_buffer, src->image_buffer, image->height * (image->pitch >> 2));
    }
}
