/*
 * Audacious 
 * Copyright (c) 2006-2007 Audacious development team.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; under version 3 of the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses>.
 *
 * The Audacious team does not consider modular code linking to
 * Audacious or using our public API to be a derived work.
 */

#include <glib.h>

#include "platform/smartinclude.h"
#include "util.h"

#include <gdk-pixbuf/gdk-pixbuf.h>

static GdkPixbuf *
create_new_pixbuf (GdkPixbuf *src)
{
    g_return_val_if_fail (gdk_pixbuf_get_colorspace (src) == GDK_COLORSPACE_RGB, NULL);
    g_return_val_if_fail ((!gdk_pixbuf_get_has_alpha (src)
                           && gdk_pixbuf_get_n_channels (src) == 3)
                           || (gdk_pixbuf_get_has_alpha (src)
                           && gdk_pixbuf_get_n_channels (src) == 4), NULL);

    return gdk_pixbuf_new (gdk_pixbuf_get_colorspace (src),
                           gdk_pixbuf_get_has_alpha (src),
                           gdk_pixbuf_get_bits_per_sample (src),
                           gdk_pixbuf_get_width (src),
                           gdk_pixbuf_get_height (src));
}

GdkPixbuf *
audacious_create_colorized_pixbuf (GdkPixbuf *src,
                                   int red_value,
                                   int green_value,
                                   int blue_value)
{
    int i, j;
    int width, height, has_alpha, src_row_stride, dst_row_stride;
    guchar *target_pixels;
    guchar *original_pixels;
    guchar *pixsrc;
    guchar *pixdest;
    GdkPixbuf *dest;

    g_return_val_if_fail (gdk_pixbuf_get_colorspace (src) == GDK_COLORSPACE_RGB, NULL);
    g_return_val_if_fail ((!gdk_pixbuf_get_has_alpha (src)
                           && gdk_pixbuf_get_n_channels (src) == 3)
                           || (gdk_pixbuf_get_has_alpha (src)
                           && gdk_pixbuf_get_n_channels (src) == 4), NULL);
    g_return_val_if_fail (gdk_pixbuf_get_bits_per_sample (src) == 8, NULL);

    dest = create_new_pixbuf (src);

    has_alpha = gdk_pixbuf_get_has_alpha (src);
    width = gdk_pixbuf_get_width (src);
    height = gdk_pixbuf_get_height (src);
    src_row_stride = gdk_pixbuf_get_rowstride (src);
    dst_row_stride = gdk_pixbuf_get_rowstride (dest);
    target_pixels = gdk_pixbuf_get_pixels (dest);
    original_pixels = gdk_pixbuf_get_pixels (src);

    for (i = 0; i < height; i++) {
        pixdest = target_pixels + i*dst_row_stride;
        pixsrc = original_pixels + i*src_row_stride;
        for (j = 0; j < width; j++) {
            *pixdest++ = (*pixsrc++ * red_value) >> 8;
            *pixdest++ = (*pixsrc++ * green_value) >> 8;
            *pixdest++ = (*pixsrc++ * blue_value) >> 8;
            if (has_alpha) {
               *pixdest++ = *pixsrc++;
            }
        }
    }
    return dest;
}

