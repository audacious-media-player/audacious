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

#ifndef __PN_IMAGE_H__
#define __PN_IMAGE_H__

#include <glib.h>
#include "pnobject.h"

G_BEGIN_DECLS

typedef enum
{
  PN_BLEND_MODE_IGNORE, /* Ignore the source image */
  PN_BLEND_MODE_REPLACE, /* Replace the destination image with the source */
  PN_BLEND_MODE_5050, /* Use the mean of the source and destination images */
  PN_BLEND_MODE_LAST /* INVALID */
} PnBlendMode;
extern const gchar *pn_image_blend_mode_strings[];
extern const guint pn_image_blend_mode_count;

#define PN_TYPE_IMAGE              (pn_image_get_type ())
#define PN_IMAGE(obj)              (G_TYPE_CHECK_INSTANCE_CAST ((obj), PN_TYPE_IMAGE, PnImage))
#define PN_IMAGE_CLASS(class)      (G_TYPE_CHECK_CLASS_CAST ((class), PN_TYPE_IMAGE, PnImageClass))
#define PN_IS_IMAGE(obj)           (G_TYPE_CHECK_INSTANCE_TYPE ((obj), PN_TYPE_IMAGE))
#define PN_IS_IMAGE_CLASS(class)   (G_TYPE_CHECK_CLASS_TYPE ((class), PN_TYPE_IMAGE))
#define PN_IMAGE_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS ((obj), PN_TYPE_IMAGE, PnImageClass))

/* Member access macros */
#define PN_IMAGE_PITCH(obj)        (PN_IMAGE (obj)->pitch)
#define PN_IMAGE_WIDTH(obj)        (PN_IMAGE (obj)->width)
#define PN_IMAGE_HEIGHT(obj)       (PN_IMAGE (obj)->height)
#define PN_IMAGE_IMAGE_BUF(obj)    (PN_IMAGE (obj)->image_buffer)
#define PN_IMAGE_XFORM_BUF(obj)    (PN_IMAGE (obj)->transform_buffer)

/* This is to help with passing just the PnImage part of a
 * PnImage (e.g. to assembly code)
 */
#define PN_IMAGE_START(obj)        (G_STRUCT_MEMBER_P (obj, G_STRUCT_OFFSET (PnImage, pitch)))

typedef struct _PnColor PnColor;

struct _PnColor
{
  guint8 red;
  guint8 green;
  guint8 blue;
  guint8 unused;
};

typedef struct _PnImage        PnImage;
typedef struct _PnImageClass   PnImageClass;

struct _PnImage
{
  PnObject parent;

  /* The width of the image buffers in bytes */
  guint pitch;

  /* The dimensions of the valid image area in pixels */
  guint width;
  guint height;

  /* The pixel bleding modes to be used when rendering individual
   * pixels and when applying the transform buffer to the
   * image buffer
   */
  PnBlendMode render_mode;
  PnBlendMode transform_mode;

  /* The pixel buffers */
  PnColor *image_buffer;
  PnColor *transform_buffer;
};

struct _PnImageClass
{
  PnObjectClass parent_class;
};

/* Creators */
GType                 pn_image_get_type                (void);
PnImage              *pn_image_new                     (void);
PnImage              *pn_image_new_with_size           (guint width,
							guint height);

/* Accessors */
void                  pn_image_set_size                (PnImage *image,
							guint width,
							guint height);
guint                 pn_image_get_width               (PnImage *image);
guint                 pn_image_get_height              (PnImage *image);
guint                 pn_image_get_pitch               (PnImage *image);
void                  pn_image_set_render_mode         (PnImage *image,
							PnBlendMode render_mode);
void                  pn_image_set_transform_mode      (PnImage *image,
							PnBlendMode transform_mode);
PnColor              *pn_image_get_image_buffer        (PnImage *image);
PnColor              *pn_image_get_transform_buffer    (PnImage *image);

/* Actions */
void                  pn_image_render_pixel            (PnImage *image,
							guint x,
							guint y,
							PnColor color);
void                  pn_image_render_pixel_by_offset  (PnImage *image,
							guint offset,
							PnColor color);
void                  pn_image_render_line             (PnImage *image,
							guint x0,
							guint y0,
							guint x1,
							guint y1,
							PnColor color);
void                  pn_image_apply_transform         (PnImage *image);
void                  pn_image_render_image            (PnImage *image,
							PnImage *src,
							PnBlendMode blend_mode);

#endif /* __PN_IMAGE_H__ */
