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
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include "textbox.h"

#include <glib.h>
#include <gtk/gtk.h>
#include <gdk/gdk.h>
#include <gdk/gdkprivate.h>
#include <string.h>
#include <ctype.h>

#include "main.h"
#include "util.h"

static void textbox_generate_pixmap(TextBox * tb);

static void
textbox_draw(Widget * w)
{
    TextBox *tb = TEXT_BOX(w);
    gint cw;
    GdkPixmap *obj;
    GdkPixmap *src;

    g_return_if_fail(tb != NULL);

    if (tb->tb_text &&
        (!tb->tb_pixmap_text || strcmp(tb->tb_text, tb->tb_pixmap_text)))
        textbox_generate_pixmap(tb);

    if (tb->tb_pixmap) {
        if (skin_get_id() != tb->tb_skin_id) {
            tb->tb_skin_id = skin_get_id();
            textbox_generate_pixmap(tb);
        }
        obj = tb->tb_widget.parent;
        src = tb->tb_pixmap;

        cw = tb->tb_pixmap_width - tb->tb_offset;
        if (cw > tb->tb_widget.width)
            cw = tb->tb_widget.width;
        gdk_draw_pixmap(obj, tb->tb_widget.gc, src, tb->tb_offset, 0,
                        tb->tb_widget.x, tb->tb_widget.y, cw,
                        tb->tb_widget.height);
        if (cw < tb->tb_widget.width)
            gdk_draw_pixmap(obj, tb->tb_widget.gc, src, 0, 0,
                            tb->tb_widget.x + cw, tb->tb_widget.y,
                            tb->tb_widget.width - cw, tb->tb_widget.height);
    }
}

static gboolean
textbox_scroll(gpointer data)
{
    TextBox *tb = TEXT_BOX(data);

    if (!tb->tb_is_dragging) {
        tb->tb_offset++;
        if (tb->tb_offset >= tb->tb_pixmap_width)
            tb->tb_offset -= tb->tb_pixmap_width;
        widget_draw(WIDGET(tb));
    }

    return TRUE;
}

static void
textbox_button_press(GtkWidget * w, GdkEventButton * event, gpointer data)
{
    TextBox *tb = TEXT_BOX(data);

    if (event->button != 1)
        return;
    if (widget_contains(&tb->tb_widget, event->x, event->y) &&
        tb->tb_scroll_allowed &&
        tb->tb_pixmap_width > tb->tb_widget.width && tb->tb_is_scrollable) {
        tb->tb_is_dragging = TRUE;
        tb->tb_drag_off = tb->tb_offset;
        tb->tb_drag_x = event->x;
    }
}

static void
textbox_motion(GtkWidget * w, GdkEventMotion * event, gpointer data)
{
    TextBox *tb = TEXT_BOX(data);

    if (tb->tb_is_dragging) {
        if (tb->tb_scroll_allowed &&
            tb->tb_pixmap_width > tb->tb_widget.width) {
            tb->tb_offset = tb->tb_drag_off - (event->x - tb->tb_drag_x);

            while (tb->tb_offset < 0)
                tb->tb_offset += tb->tb_pixmap_width;

            while (tb->tb_offset > tb->tb_pixmap_width)
                tb->tb_offset -= tb->tb_pixmap_width;

            widget_draw(WIDGET(tb));
        }
    }
}

static void
textbox_button_release(GtkWidget * w, GdkEventButton * event, gpointer data)
{
    TextBox *tb = TEXT_BOX(data);

    if (event->button == 1)
        tb->tb_is_dragging = FALSE;
}

static gboolean
textbox_should_scroll(TextBox * tb)
{
    g_return_val_if_fail(tb != NULL, FALSE);

    if (!tb->tb_scroll_allowed)
        return FALSE;

    if (tb->tb_font) {
        gint width;

        text_get_extents(tb->tb_fontname, tb->tb_text, &width, NULL, NULL,
                         NULL);

        if (width <= tb->tb_widget.width)
            return FALSE;
        else
            return TRUE;
    }

    if (g_utf8_strlen(tb->tb_text, -1) * 5 > tb->tb_widget.width)
        return TRUE;

    return FALSE;
}

void
textbox_set_text(TextBox * tb, const gchar * text)
{
    g_return_if_fail(tb != NULL);
    g_return_if_fail(text != NULL);

    widget_lock(WIDGET(tb));

    if (tb->tb_text) {
        if (!strcmp(text, tb->tb_text)) {
            widget_unlock(WIDGET(tb));
            return;
        }
        g_free(tb->tb_text);
    }

    tb->tb_text = str_to_utf8(text);

    widget_unlock(WIDGET(tb));
    widget_draw(WIDGET(tb));
}

static void
textbox_generate_xfont_pixmap(TextBox * tb, const gchar * pixmaptext)
{
    gint length, i;
    GdkGC *gc, *maskgc;
    GdkColor *c, pattern;
    GdkBitmap *mask;
    PangoLayout *layout;
    gint width;

    g_return_if_fail(tb != NULL);
    g_return_if_fail(pixmaptext != NULL);

    length = g_utf8_strlen(pixmaptext, -1);

    text_get_extents(tb->tb_fontname, pixmaptext, &width, NULL, NULL, NULL);

    tb->tb_pixmap_width = MAX(width, tb->tb_widget.width);
    tb->tb_pixmap = gdk_pixmap_new(mainwin->window, tb->tb_pixmap_width,
                                   tb->tb_widget.height,
                                   gdk_rgb_get_visual()->depth);
    gc = tb->tb_widget.gc;
    c = skin_get_color(bmp_active_skin, SKIN_TEXTBG);
    for (i = 0; i < tb->tb_widget.height; i++) {
        gdk_gc_set_foreground(gc, &c[6 * i / tb->tb_widget.height]);
        gdk_draw_line(tb->tb_pixmap, gc, 0, i, tb->tb_pixmap_width, i);
    }

    mask = gdk_pixmap_new(mainwin->window, tb->tb_pixmap_width,
                          tb->tb_widget.height, 1);
    maskgc = gdk_gc_new(mask);
    pattern.pixel = 0;
    gdk_gc_set_foreground(maskgc, &pattern);

    gdk_draw_rectangle(mask, maskgc, TRUE, 0, 0,
                       tb->tb_pixmap_width, tb->tb_widget.height);
    pattern.pixel = 1;
    gdk_gc_set_foreground(maskgc, &pattern);

    gdk_gc_set_foreground(gc, skin_get_color(bmp_active_skin, SKIN_TEXTFG));

    layout = gtk_widget_create_pango_layout(mainwin, pixmaptext);
    pango_layout_set_font_description(layout, tb->tb_font);

    gdk_draw_layout(tb->tb_pixmap, gc, 0, (tb->tb_font_descent / 2), layout);
    g_object_unref(layout);

    g_object_unref(maskgc);

    gdk_gc_set_clip_mask(gc, mask);
    c = skin_get_color(bmp_active_skin, SKIN_TEXTFG);
    for (i = 0; i < tb->tb_widget.height; i++) {
        gdk_gc_set_foreground(gc, &c[6 * i / tb->tb_widget.height]);
        gdk_draw_line(tb->tb_pixmap, gc, 0, i, tb->tb_pixmap_width, i);
    }
    g_object_unref(mask);
    gdk_gc_set_clip_mask(gc, NULL);
}

static void
textbox_handle_special_char(gchar c, gint * x, gint * y)
{
    switch (c) {
    case '"':
        *x = 130;
        *y = 0;
        break;
    case ':':
        *x = 60;
        *y = 6;
        break;
    case '(':
        *x = 65;
        *y = 6;
        break;
    case ')':
        *x = 70;
        *y = 6;
        break;
    case '-':
        *x = 75;
        *y = 6;
        break;
    case '`':
    case '\'':
        *x = 80;
        *y = 6;
        break;
    case '!':
        *x = 85;
        *y = 6;
        break;
    case '_':
        *x = 90;
        *y = 6;
        break;
    case '+':
        *x = 95;
        *y = 6;
        break;
    case '\\':
        *x = 100;
        *y = 6;
        break;
    case '/':
        *x = 105;
        *y = 6;
        break;
    case '[':
        *x = 110;
        *y = 6;
        break;
    case ']':
        *x = 115;
        *y = 6;
        break;
    case '^':
        *x = 120;
        *y = 6;
        break;
    case '&':
        *x = 125;
        *y = 6;
        break;
    case '%':
        *x = 130;
        *y = 6;
        break;
    case '.':
    case ',':
        *x = 135;
        *y = 6;
        break;
    case '=':
        *x = 140;
        *y = 6;
        break;
    case '$':
        *x = 145;
        *y = 6;
        break;
    case '#':
        *x = 150;
        *y = 6;
        break;
    case 'å':
    case 'Å':
        *x = 0;
        *y = 12;
        break;
    case 'ö':
    case 'Ö':
        *x = 5;
        *y = 12;
        break;
    case 'ä':
    case 'Ä':
        *x = 10;
        *y = 12;
        break;
    case 'ü':
    case 'Ü':
        *x = 100;
        *y = 0;
        break;
    case '?':
        *x = 15;
        *y = 12;
        break;
    case '*':
        *x = 20;
        *y = 12;
        break;
    default:
        *x = 145;
        *y = 0;
        break;
    }
}

static void
textbox_generate_pixmap(TextBox * tb)
{
    gint length, i, x, y, wl;
    gchar *pixmaptext;
    GdkGC *gc;

    g_return_if_fail(tb != NULL);

    if (tb->tb_pixmap) {
        g_object_unref(tb->tb_pixmap);
        tb->tb_pixmap = NULL;
    }

    /*
     * Don't reset the offset if only text after the last '(' has
     * changed.  This is a hack to avoid visual noice on vbr files
     * where we guess the length.
     */
    if (!(tb->tb_pixmap_text && strrchr(tb->tb_text, '(') &&
          !strncmp(tb->tb_pixmap_text, tb->tb_text,
                   strrchr(tb->tb_text, '(') - tb->tb_text)))
        tb->tb_offset = 0;

    g_free(tb->tb_pixmap_text);
    tb->tb_pixmap_text = g_strdup(tb->tb_text);

    /*
     * wl is the number of (partial) letters visible. Only makes
     * sense when using skinned font.
     */

    wl = tb->tb_widget.width / 5;
    if (wl * 5 != tb->tb_widget.width)
        wl++;

    length = g_utf8_strlen(tb->tb_text, -1);

    tb->tb_is_scrollable = FALSE;

    if (textbox_should_scroll(tb)) {
        tb->tb_is_scrollable = TRUE;
        pixmaptext = g_strconcat(tb->tb_pixmap_text, "  ***  ", NULL);
        length += 7;
    }
    else if (!tb->tb_font && length <= wl) {
        gint pad = wl - length;
        gchar *padchars = g_strnfill(pad, ' ');

        pixmaptext = g_strconcat(tb->tb_pixmap_text, padchars, NULL);
        g_free(padchars);
        length += pad;
    }
    else
        pixmaptext = g_strdup(tb->tb_pixmap_text);


    if (tb->tb_is_scrollable) {
        if (tb->tb_scroll_enabled && !tb->tb_timeout_tag) {
            gint tag;
            tag = TEXTBOX_SCROLL_SMOOTH_TIMEOUT;
            tb->tb_timeout_tag = gtk_timeout_add(tag, textbox_scroll, tb);
        }
    }
    else {
        if (tb->tb_timeout_tag) {
            gtk_timeout_remove(tb->tb_timeout_tag);
            tb->tb_timeout_tag = 0;
        }
        tb->tb_offset = 0;
    }

    if (tb->tb_font) {
        textbox_generate_xfont_pixmap(tb, pixmaptext);
        g_free(pixmaptext);
        return;
    }

    tb->tb_pixmap_width = length * 5;
    tb->tb_pixmap = gdk_pixmap_new(mainwin->window,
                                   tb->tb_pixmap_width, 6,
                                   gdk_rgb_get_visual()->depth);
    gc = tb->tb_widget.gc;

    for (i = 0; i < length; i++) {
        gchar c;
        x = y = -1;
        c = toupper(pixmaptext[i]);
        if (c >= 'A' && c <= 'Z') {
            x = 5 * (c - 'A');
            y = 0;
        }
        else if (c >= '0' && c <= '9') {
            x = 5 * (c - '0');
            y = 6;
        }
        else
            textbox_handle_special_char(c, &x, &y);

        skin_draw_pixmap(bmp_active_skin,
                         tb->tb_pixmap, gc, tb->tb_skin_index,
                         x, y, i * 5, 0, 5, 6);
    }
    g_free(pixmaptext);
}

void
textbox_set_scroll(TextBox * tb, gboolean s)
{
    g_return_if_fail(tb != NULL);

    tb->tb_scroll_enabled = s;
    if (tb->tb_scroll_enabled && tb->tb_is_scrollable
        && tb->tb_scroll_allowed) {
        gint tag;
        tag = TEXTBOX_SCROLL_SMOOTH_TIMEOUT;
        tb->tb_timeout_tag = gtk_timeout_add(tag, textbox_scroll, tb);
    }
    else {
        if (tb->tb_timeout_tag) {
            gtk_timeout_remove(tb->tb_timeout_tag);
            tb->tb_timeout_tag = 0;
        }
        tb->tb_offset = 0;
        widget_draw(WIDGET(tb));
    }

}

void
textbox_set_xfont(TextBox * tb, gboolean use_xfont, const gchar * fontname)
{
    gint ascent, descent;

    g_return_if_fail(tb != NULL);

    if (tb->tb_font) {
        pango_font_description_free(tb->tb_font);
        tb->tb_font = NULL;
    }

    tb->tb_widget.y = tb->tb_nominal_y;
    tb->tb_widget.height = tb->tb_nominal_height;

    /* Make sure the pixmap is regenerated */
    if (tb->tb_pixmap_text) {
        g_free(tb->tb_pixmap_text);
        tb->tb_pixmap_text = NULL;
    }

    if (!use_xfont || strlen(fontname) == 0)
        return;

    tb->tb_font = pango_font_description_from_string(fontname);
    tb->tb_fontname = g_strdup(fontname);

    text_get_extents(fontname,
                     "AaBbCcDdEeFfGgHhIiJjKkLlMmNnOoPpQqRrSsTtUuVvWwXxYyZz ",
                     NULL, NULL, &ascent, &descent);
    tb->tb_font_ascent = ascent;
    tb->tb_font_descent = descent;


    if (tb->tb_font == NULL)
        return;

    tb->tb_widget.height = tb->tb_font_ascent;
    if (tb->tb_widget.height > tb->tb_nominal_height)
        tb->tb_widget.y -= (tb->tb_widget.height - tb->tb_nominal_height) / 2;
    else
        tb->tb_widget.height = tb->tb_nominal_height;
}

TextBox *
create_textbox(GList ** wlist, GdkPixmap * parent, GdkGC * gc,
               gint x, gint y, gint w, gboolean allow_scroll, SkinPixmapId si)
{
    TextBox *tb;

    tb = g_new0(TextBox, 1);
    widget_init(&tb->tb_widget, parent, gc, x, y, w, 6, 1);
    tb->tb_widget.button_press_cb = textbox_button_press;
    tb->tb_widget.button_release_cb = textbox_button_release;
    tb->tb_widget.motion_cb = textbox_motion;
    tb->tb_widget.draw = textbox_draw;
    tb->tb_scroll_allowed = allow_scroll;
    tb->tb_scroll_enabled = TRUE;
    tb->tb_skin_index = si;
    tb->tb_nominal_y = y;
    tb->tb_nominal_height = tb->tb_widget.height;
    widget_list_add(wlist, WIDGET(tb));
    return tb;
}

void
textbox_free(TextBox * tb)
{
    g_return_if_fail(tb != NULL);

    if (tb->tb_pixmap)
        g_object_unref(tb->tb_pixmap);
    g_free(tb->tb_text);
    g_free(tb);
}
