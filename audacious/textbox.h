/*  BMP - Cross-platform multimedia player
 *  Copyright (C) 2003-2004  BMP development team.
 *
 *  Based on XMMS:
 *  Copyright (C) 1998-2003  XMMS development team.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02111-1307, USA.
 */

#ifndef TEXTBOX_H
#define	TEXTBOX_H

#include <glib.h>
#include <gdk/gdk.h>
#include <pango/pango.h>

#include "skin.h"
#include "widget.h"

#define	TEXTBOX_SCROLL_TIMEOUT	       200
#define TEXTBOX_SCROLL_SMOOTH_TIMEOUT  30

#define TEXT_BOX(x)  ((TextBox *)(x))
struct _TextBox {
    Widget tb_widget;
    GdkPixmap *tb_pixmap;
    gchar *tb_text, *tb_pixmap_text;
    gint tb_pixmap_width;
    gint tb_offset;
    gboolean tb_scroll_allowed, tb_scroll_enabled;
    gboolean tb_is_scrollable, tb_is_dragging;
    gint tb_timeout_tag, tb_drag_x, tb_drag_off;
    gint tb_nominal_y, tb_nominal_height;
    gint tb_skin_id;
    SkinPixmapId tb_skin_index;
    PangoFontDescription *tb_font;
    gint tb_font_ascent, tb_font_descent;
    gchar *tb_fontname;
};

typedef struct _TextBox TextBox;

void textbox_set_text(TextBox * tb, const gchar * text);
void textbox_set_scroll(TextBox * tb, gboolean s);
TextBox *create_textbox(GList ** wlist, GdkPixmap * parent, GdkGC * gc,
                        gint x, gint y, gint w, gboolean allow_scroll,
                        SkinPixmapId si);
void textbox_set_xfont(TextBox * tb, gboolean use_xfont,
                       const gchar * fontname);
void textbox_free(TextBox * tb);

#endif
