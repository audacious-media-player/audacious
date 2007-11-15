/*  Audacious
 *  Copyright (C) 2005-2007  Audacious development team.
 *
 *  BMP - Cross-platform multimedia player
 *  Copyright (C) 2003-2004  BMP development team.
 *
 *  Based on XMMS:
 *  Copyright (C) 1998-2003  XMMS development team.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; under version 3 of the License.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses>.
 *
 *  The Audacious team does not consider modular code linking to
 *  Audacious or using our public API to be a derived work.
 */

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

/* TODO: enforce default sizes! */

#include <glib.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "skin.h"
#include "ui_equalizer.h"
#include "main.h"
#include "ui_playlist.h"
#include "ui_skinselector.h"
#include "util.h"

#include "debug.h"

#include "platform/smartinclude.h"
#include "vfs.h"

#include "ui_skinned_window.h"
#include "ui_skinned_button.h"
#include "ui_skinned_number.h"
#include "ui_skinned_horizontal_slider.h"
#include "ui_skinned_playstatus.h"

#define EXTENSION_TARGETS 7

static gchar *ext_targets[EXTENSION_TARGETS] =
{ "bmp", "xpm", "png", "svg", "gif", "jpg", "jpeg" };

struct _SkinPixmapIdMapping {
    SkinPixmapId id;
    const gchar *name;
    const gchar *alt_name;
    gint width, height;
};

struct _SkinMaskInfo {
    gint width, height;
    gchar *inistr;
};

typedef struct _SkinPixmapIdMapping SkinPixmapIdMapping;
typedef struct _SkinMaskInfo SkinMaskInfo;


Skin *bmp_active_skin = NULL;

static gint skin_current_num;

static SkinMaskInfo skin_mask_info[] = {
    {275, 116, "Normal"},
    {275, 16,  "WindowShade"},
    {275, 116, "Equalizer"},
    {275, 16,  "EqualizerWS"}
};

static SkinPixmapIdMapping skin_pixmap_id_map[] = {
    {SKIN_MAIN, "main", NULL, 0, 0},
    {SKIN_CBUTTONS, "cbuttons", NULL, 0, 0},
    {SKIN_SHUFREP, "shufrep", NULL, 0, 0},
    {SKIN_TEXT, "text", NULL, 0, 0},
    {SKIN_TITLEBAR, "titlebar", NULL, 0, 0},
    {SKIN_VOLUME, "volume", NULL, 0, 0},
    {SKIN_BALANCE, "balance", "volume", 0, 0},
    {SKIN_MONOSTEREO, "monoster", NULL, 0, 0},
    {SKIN_PLAYPAUSE, "playpaus", NULL, 0, 0},
    {SKIN_NUMBERS, "nums_ex", "numbers", 0, 0},
    {SKIN_POSBAR, "posbar", NULL, 0, 0},
    {SKIN_EQMAIN, "eqmain", NULL, 0, 0},
    {SKIN_PLEDIT, "pledit", NULL, 0, 0},
    {SKIN_EQ_EX, "eq_ex", NULL, 0, 0}
};

static guint skin_pixmap_id_map_size = G_N_ELEMENTS(skin_pixmap_id_map);

static const guchar skin_default_viscolor[24][3] = {
    {9, 34, 53},
    {10, 18, 26},
    {0, 54, 108},
    {0, 58, 116},
    {0, 62, 124},
    {0, 66, 132},
    {0, 70, 140},
    {0, 74, 148},
    {0, 78, 156},
    {0, 82, 164},
    {0, 86, 172},
    {0, 92, 184},
    {0, 98, 196},
    {0, 104, 208},
    {0, 110, 220},
    {0, 116, 232},
    {0, 122, 244},
    {0, 128, 255},
    {0, 128, 255},
    {0, 104, 208},
    {0, 80, 160},
    {0, 56, 112},
    {0, 32, 64},
    {200, 200, 200}
};

static gchar *original_gtk_theme = NULL;

static GdkBitmap *skin_create_transparent_mask(const gchar *,
                                               const gchar *,
                                               const gchar *,
                                               GdkWindow *,
                                               gint, gint, gboolean);

static void skin_setup_masks(Skin * skin);

static void skin_set_default_vis_color(Skin * skin);

void
skin_lock(Skin * skin)
{
    g_mutex_lock(skin->lock);
}

void
skin_unlock(Skin * skin)
{
    g_mutex_unlock(skin->lock);
}

gboolean
bmp_active_skin_reload(void) 
{
    return bmp_active_skin_load(bmp_active_skin->path); 
}

gboolean
bmp_active_skin_load(const gchar * path)
{
    g_return_val_if_fail(bmp_active_skin != NULL, FALSE);

    memset(&bmp_active_skin->properties, 0, sizeof(SkinProperties));

    if (!skin_load(bmp_active_skin, path))
        return FALSE;

    skin_setup_masks(bmp_active_skin);

    ui_skinned_window_draw_all(mainwin);
    ui_skinned_window_draw_all(equalizerwin);
    ui_skinned_window_draw_all(playlistwin);

    playlistwin_update_list(playlist_get_active());

    SkinPixmap *pixmap;
    pixmap = &bmp_active_skin->pixmaps[SKIN_POSBAR];
    /* last 59 pixels of SKIN_POSBAR are knobs (normal and selected) */
    gtk_widget_set_size_request(mainwin_position, pixmap->width - 59, pixmap->height);

    return TRUE;
}

void
skin_pixmap_free(SkinPixmap * p)
{
    g_return_if_fail(p != NULL);
    g_return_if_fail(p->pixmap != NULL);

    g_object_unref(p->pixmap);
    p->pixmap = NULL;
}

Skin *
skin_new(void)
{
    Skin *skin;
    skin = g_new0(Skin, 1);
    skin->lock = g_mutex_new();
    return skin;
}

void
skin_free(Skin * skin)
{
    gint i;

    g_return_if_fail(skin != NULL);

    skin_lock(skin);

    for (i = 0; i < SKIN_PIXMAP_COUNT; i++)
        skin_pixmap_free(&skin->pixmaps[i]);

    for (i = 0; i < SKIN_PIXMAP_COUNT; i++) {
        if (skin->masks[i])
            g_object_unref(skin->masks[i]);
        if (skin->ds_masks[i])
            g_object_unref(skin->ds_masks[i]);

        skin->masks[i] = NULL;
        skin->ds_masks[i] = NULL;
    }

    skin_set_default_vis_color(skin);
    skin_unlock(skin);
}

void
skin_destroy(Skin * skin)
{
    g_return_if_fail(skin != NULL);
    skin_free(skin);
    g_mutex_free(skin->lock);
    g_free(skin);
}

const SkinPixmapIdMapping *
skin_pixmap_id_lookup(guint id)
{
    guint i;

    for (i = 0; i < skin_pixmap_id_map_size; i++) {
        if (id == skin_pixmap_id_map[i].id) {
            return &skin_pixmap_id_map[i];
        }
    }

    return NULL;
}

const gchar *
skin_pixmap_id_to_name(SkinPixmapId id)
{
    guint i;

    for (i = 0; i < skin_pixmap_id_map_size; i++) {
        if (id == skin_pixmap_id_map[i].id)
            return skin_pixmap_id_map[i].name;
    }
    return NULL;
}

static void
skin_set_default_vis_color(Skin * skin)
{
    memcpy(skin->vis_color, skin_default_viscolor,
           sizeof(skin_default_viscolor));
}

/*
 * I have rewritten this to take an array of possible targets,
 * once we find a matching target we now return, instead of loop
 * recursively. This allows for us to support many possible format
 * targets for our skinning engine than just the original winamp 
 * formats.
 *
 *    -- nenolod, 16 January 2006
 */
gchar *
skin_pixmap_locate(const gchar * dirname, gchar ** basenames)
{
    gchar *filename;
    gint i;

    for (i = 0; basenames[i]; i++)
    if (!(filename = find_path_recursively(dirname, basenames[i]))) 
        g_free(filename);
    else
        return filename;

    /* can't find any targets -- sorry */
    return NULL;
}

/* FIXME: this function is temporary. It will be removed when the skinning system
   uses GdkPixbuf in place of GdkPixmap */

static GdkPixmap *
pixmap_new_from_file(const gchar * filename)
{
    GdkPixbuf *pixbuf, *pixbuf2;
    GdkPixmap *pixmap;
    gint width, height;

    if (!(pixbuf = gdk_pixbuf_new_from_file(filename, NULL)))
        return NULL;

    width = gdk_pixbuf_get_width(pixbuf);
    height = gdk_pixbuf_get_height(pixbuf);

    if (!(pixmap = gdk_pixmap_new(mainwin->window, width, height,
                                  gdk_rgb_get_visual()->depth))) {
        g_object_unref(pixbuf);
        return NULL;
    }

    pixbuf2 = audacious_create_colorized_pixbuf(pixbuf, cfg.colorize_r, cfg.colorize_g, cfg.colorize_b);
    g_object_unref(pixbuf);

    GdkGC *gc;
    gc = gdk_gc_new(pixmap);
    gdk_draw_pixbuf(pixmap, gc, pixbuf2, 0, 0, 0, 0, width, height, GDK_RGB_DITHER_MAX, 0, 0);
    g_object_unref(gc);
    g_object_unref(pixbuf2);

    return pixmap;
}

static gboolean
skin_load_pixmap_id(Skin * skin, SkinPixmapId id, const gchar * path_p)
{
    const gchar *path;
    gchar *filename;
    gint width, height;
    const SkinPixmapIdMapping *pixmap_id_mapping;
    GdkPixmap *gpm;
    SkinPixmap *pm = NULL;
    gchar *basenames[EXTENSION_TARGETS * 2 + 1]; /* alternate basenames */
    gint i, y;

    g_return_val_if_fail(skin != NULL, FALSE);
    g_return_val_if_fail(id < SKIN_PIXMAP_COUNT, FALSE);

    pixmap_id_mapping = skin_pixmap_id_lookup(id);
    g_return_val_if_fail(pixmap_id_mapping != NULL, FALSE);

    memset(&basenames, 0, sizeof(basenames));

    for (i = 0, y = 0; i < EXTENSION_TARGETS; i++, y++)
    {
        basenames[y] =
            g_strdup_printf("%s.%s", pixmap_id_mapping->name, ext_targets[i]);

        if (pixmap_id_mapping->alt_name)
            basenames[++y] =
                g_strdup_printf("%s.%s", pixmap_id_mapping->alt_name,
                                ext_targets[i]);
    }

    path = path_p ? path_p : skin->path;
    filename = skin_pixmap_locate(path, basenames);

    for (i = 0; basenames[i] != NULL; i++)
    {
         g_free(basenames[i]);
         basenames[i] = NULL;
    }

    if (filename == NULL)
        return FALSE;

    if (!(gpm = pixmap_new_from_file(filename))) {
        g_warning("loading of %s failed", filename);
        g_free(filename);
        return FALSE;
    }

    g_free(filename);

    gdk_drawable_get_size(GDK_DRAWABLE(gpm), &width, &height);
    pm = &skin->pixmaps[id];
    pm->pixmap = gpm;
    pm->width = width;
    pm->height = height;
    pm->current_width = width;
    pm->current_height = height;

    return TRUE;
}

void
skin_mask_create(Skin * skin,
                 const gchar * path,
                 gint id,
                 GdkWindow * window)
{
    skin->masks[id] =
        skin_create_transparent_mask(path, "region.txt",
                                     skin_mask_info[id].inistr, window,
                                     skin_mask_info[id].width,
                                     skin_mask_info[id].height, FALSE);

    skin->ds_masks[id] =
        skin_create_transparent_mask(path, "region.txt",
                                     skin_mask_info[id].inistr, window,
                                     skin_mask_info[id].width * 2,
                                     skin_mask_info[id].height * 2, TRUE);
}

static void
skin_setup_masks(Skin * skin)
{
    GdkBitmap *mask;

    if (cfg.show_wm_decorations)
        return;

    if (cfg.player_visible) {
        mask = skin_get_mask(skin, SKIN_MASK_MAIN + cfg.player_shaded);
        gtk_widget_shape_combine_mask(mainwin, mask, 0, 0);
    }

    mask = skin_get_mask(skin, SKIN_MASK_EQ + cfg.equalizer_shaded);
    gtk_widget_shape_combine_mask(equalizerwin, mask, 0, 0);
}

static GdkBitmap *
create_default_mask(GdkWindow * parent, gint w, gint h)
{
    GdkBitmap *ret;
    GdkGC *gc;
    GdkColor pattern;

    ret = gdk_pixmap_new(parent, w, h, 1);
    gc = gdk_gc_new(ret);
    pattern.pixel = 1;
    gdk_gc_set_foreground(gc, &pattern);
    gdk_draw_rectangle(ret, gc, TRUE, 0, 0, w, h);
    g_object_unref(gc);

    return ret;
}

static void
skin_query_color(GdkColormap * cm, GdkColor * c)
{
#ifdef GDK_WINDOWING_X11
    XColor xc = { 0,0,0,0,0,0 };

    xc.pixel = c->pixel;
    XQueryColor(GDK_COLORMAP_XDISPLAY(cm), GDK_COLORMAP_XCOLORMAP(cm), &xc);
    c->red = xc.red;
    c->green = xc.green;
    c->blue = xc.blue;
#else
    /* do nothing. see what breaks? */
#endif
}

static glong
skin_calc_luminance(GdkColor * c)
{
    return (0.212671 * c->red + 0.715160 * c->green + 0.072169 * c->blue);
}

static void
skin_get_textcolors(GdkPixmap * text, GdkColor * bgc, GdkColor * fgc)
{
    /*
     * Try to extract reasonable background and foreground colors
     * from the font pixmap
     */

    GdkImage *gi;
    GdkColormap *cm;
    gint i;

    g_return_if_fail(text != NULL);
    g_return_if_fail(GDK_IS_WINDOW(playlistwin->window));

    /* Get the first line of text */
    gi = gdk_drawable_get_image(text, 0, 0, 152, 6);
    cm = gdk_drawable_get_colormap(playlistwin->window);

    for (i = 0; i < 6; i++) {
        GdkColor c;
        gint x;
        glong d, max_d;

        /* Get a pixel from the middle of the space character */
        bgc[i].pixel = gdk_image_get_pixel(gi, 151, i);
        skin_query_color(cm, &bgc[i]);

        max_d = 0;
        for (x = 1; x < 150; x++) {
            c.pixel = gdk_image_get_pixel(gi, x, i);
            skin_query_color(cm, &c);

            d = labs(skin_calc_luminance(&c) - skin_calc_luminance(&bgc[i]));
            if (d > max_d) {
                memcpy(&fgc[i], &c, sizeof(GdkColor));
                max_d = d;
            }
        }
    }
    g_object_unref(gi);
}

gboolean
init_skins(const gchar * path)
{
    bmp_active_skin = skin_new();

    skin_parse_hints(bmp_active_skin, NULL);

    /* create the windows if they haven't been created yet, needed for bootstrapping */
    if (mainwin == NULL)
    {
        mainwin_create();
        equalizerwin_create();
        playlistwin_create();
    }

    if (!bmp_active_skin_load(path)) {
        if (path != NULL)
            g_message("Unable to load skin (%s), trying default...", path);
        else
            g_message("Skin not defined: trying default...");

        /* can't load configured skin, retry with default */
        if (!bmp_active_skin_load(BMP_DEFAULT_SKIN_PATH)) {
            g_message("Unable to load default skin (%s)! Giving up.",
                      BMP_DEFAULT_SKIN_PATH);
            return FALSE;
        }
    }

    if (cfg.random_skin_on_play)
        skinlist_update();

    return TRUE;
}

/*
 * Opens and parses a skin's hints file.
 * Hints files are somewhat like "scripts" in Winamp3/5.
 * We'll probably add scripts to it next.
 */
void
skin_parse_hints(Skin * skin, gchar *path_p)
{
    gchar *filename, *tmp;
    INIFile *inifile;

    path_p = path_p ? path_p : skin->path;

    skin->properties.mainwin_othertext = FALSE;
    skin->properties.mainwin_vis_x = 24;
    skin->properties.mainwin_vis_y = 43;
    skin->properties.mainwin_vis_width = 76;
    skin->properties.mainwin_text_x = 112;
    skin->properties.mainwin_text_y = 27;
    skin->properties.mainwin_text_width = 153;
    skin->properties.mainwin_infobar_x = 112;
    skin->properties.mainwin_infobar_y = 43;
    skin->properties.mainwin_number_0_x = 36;
    skin->properties.mainwin_number_0_y = 26;
    skin->properties.mainwin_number_1_x = 48;
    skin->properties.mainwin_number_1_y = 26;
    skin->properties.mainwin_number_2_x = 60;
    skin->properties.mainwin_number_2_y = 26;
    skin->properties.mainwin_number_3_x = 78;
    skin->properties.mainwin_number_3_y = 26;
    skin->properties.mainwin_number_4_x = 90;
    skin->properties.mainwin_number_4_y = 26;
    skin->properties.mainwin_playstatus_x = 24;
    skin->properties.mainwin_playstatus_y = 28;
    skin->properties.mainwin_menurow_visible = TRUE;
    skin->properties.mainwin_volume_x = 107;
    skin->properties.mainwin_volume_y = 57;
    skin->properties.mainwin_balance_x = 177;
    skin->properties.mainwin_balance_y = 57;
    skin->properties.mainwin_position_x = 16;
    skin->properties.mainwin_position_y = 72;
    skin->properties.mainwin_othertext_is_status = FALSE;
    skin->properties.mainwin_othertext_visible = skin->properties.mainwin_othertext;
    skin->properties.mainwin_text_visible = TRUE;
    skin->properties.mainwin_vis_visible = TRUE;
    skin->properties.mainwin_previous_x = 16;
    skin->properties.mainwin_previous_y = 88;
    skin->properties.mainwin_play_x = 39;
    skin->properties.mainwin_play_y = 88;
    skin->properties.mainwin_pause_x = 62;
    skin->properties.mainwin_pause_y = 88;
    skin->properties.mainwin_stop_x = 85;
    skin->properties.mainwin_stop_y = 88;
    skin->properties.mainwin_next_x = 108;
    skin->properties.mainwin_next_y = 88;
    skin->properties.mainwin_eject_x = 136;
    skin->properties.mainwin_eject_y = 89;
    skin->properties.mainwin_width = 275;
    skin_mask_info[0].width = skin->properties.mainwin_width;
    skin->properties.mainwin_height = 116;
    skin_mask_info[0].height = skin->properties.mainwin_height;
    skin->properties.mainwin_about_x = 247;
    skin->properties.mainwin_about_y = 83;
    skin->properties.mainwin_shuffle_x = 164;
    skin->properties.mainwin_shuffle_y = 89;
    skin->properties.mainwin_repeat_x = 210;
    skin->properties.mainwin_repeat_y = 89;
    skin->properties.mainwin_eqbutton_x = 219;
    skin->properties.mainwin_eqbutton_y = 58;
    skin->properties.mainwin_plbutton_x = 242;
    skin->properties.mainwin_plbutton_y = 58;
    skin->properties.textbox_bitmap_font_width = 5;
    skin->properties.textbox_bitmap_font_height = 6;
    skin->properties.mainwin_minimize_x = 244;
    skin->properties.mainwin_minimize_y = 3;
    skin->properties.mainwin_shade_x = 254;
    skin->properties.mainwin_shade_y = 3;
    skin->properties.mainwin_close_x = 264;
    skin->properties.mainwin_close_y = 3;

    if (path_p == NULL)
        return;

    filename = find_file_recursively(path_p, "skin.hints");

    if (filename == NULL)
        return;

    inifile = open_ini_file(filename);

    tmp = read_ini_string(inifile, "skin", "mainwinOthertext");

    if (tmp != NULL)
    {
        skin->properties.mainwin_othertext = atoi(tmp);
        g_free(tmp);
    }

    tmp = read_ini_string(inifile, "skin", "mainwinVisX");

    if (tmp != NULL)
    {
        skin->properties.mainwin_vis_x = atoi(tmp);
        g_free(tmp);
    }

    tmp = read_ini_string(inifile, "skin", "mainwinVisY");

    if (tmp != NULL)
    {
        skin->properties.mainwin_vis_y = atoi(tmp);
        g_free(tmp);
    }

    tmp = read_ini_string(inifile, "skin", "mainwinVisWidth");

    if (tmp != NULL)
    {
        skin->properties.mainwin_vis_width = atoi(tmp);
        g_free(tmp);
    }

    tmp = read_ini_string(inifile, "skin", "mainwinTextX");

    if (tmp != NULL)
    {
        skin->properties.mainwin_text_x = atoi(tmp);
        g_free(tmp);
    }

    tmp = read_ini_string(inifile, "skin", "mainwinTextY");

    if (tmp != NULL)
    {
        skin->properties.mainwin_text_y = atoi(tmp);
        g_free(tmp);
    }

    tmp = read_ini_string(inifile, "skin", "mainwinTextWidth");

    if (tmp != NULL)
    {
        skin->properties.mainwin_text_width = atoi(tmp);
        g_free(tmp);
    }

    tmp = read_ini_string(inifile, "skin", "mainwinInfoBarX");

    if (tmp != NULL)
    {
        skin->properties.mainwin_infobar_x = atoi(tmp);
        g_free(tmp);
    }

    tmp = read_ini_string(inifile, "skin", "mainwinInfoBarY");

    if (tmp != NULL)
    {
        skin->properties.mainwin_infobar_y = atoi(tmp);
        g_free(tmp);
    }

    tmp = read_ini_string(inifile, "skin", "mainwinNumber0X");

    if (tmp != NULL)
    {
        skin->properties.mainwin_number_0_x = atoi(tmp);
        g_free(tmp);
    }

    tmp = read_ini_string(inifile, "skin", "mainwinNumber0Y");

    if (tmp != NULL)
    {
        skin->properties.mainwin_number_0_y = atoi(tmp);
        g_free(tmp);
    }

    tmp = read_ini_string(inifile, "skin", "mainwinNumber1X");

    if (tmp != NULL)
    {
        skin->properties.mainwin_number_1_x = atoi(tmp);
        g_free(tmp);
    }

    tmp = read_ini_string(inifile, "skin", "mainwinNumber1Y");

    if (tmp != NULL)
    {
        skin->properties.mainwin_number_1_y = atoi(tmp);
        g_free(tmp);
    }

    tmp = read_ini_string(inifile, "skin", "mainwinNumber2X");

    if (tmp != NULL)
    {
        skin->properties.mainwin_number_2_x = atoi(tmp);
        g_free(tmp);
    }

    tmp = read_ini_string(inifile, "skin", "mainwinNumber2Y");

    if (tmp != NULL)
    {
        skin->properties.mainwin_number_2_y = atoi(tmp);
        g_free(tmp);
    }

    tmp = read_ini_string(inifile, "skin", "mainwinNumber3X");

    if (tmp != NULL)
    {
        skin->properties.mainwin_number_3_x = atoi(tmp);
        g_free(tmp);
    }

    tmp = read_ini_string(inifile, "skin", "mainwinNumber3Y");

    if (tmp != NULL)
    {
        skin->properties.mainwin_number_3_y = atoi(tmp);
        g_free(tmp);
    }

    tmp = read_ini_string(inifile, "skin", "mainwinNumber4X");

    if (tmp != NULL)
    {
        skin->properties.mainwin_number_4_x = atoi(tmp);
        g_free(tmp);
    }

    tmp = read_ini_string(inifile, "skin", "mainwinNumber4Y");

    if (tmp != NULL)
    {
        skin->properties.mainwin_number_4_y = atoi(tmp);
        g_free(tmp);
    }

    tmp = read_ini_string(inifile, "skin", "mainwinPlayStatusX");

    if (tmp != NULL)
    {
        skin->properties.mainwin_playstatus_x = atoi(tmp);
        g_free(tmp);
    }

    tmp = read_ini_string(inifile, "skin", "mainwinPlayStatusY");

    if (tmp != NULL)
    {
        skin->properties.mainwin_playstatus_y = atoi(tmp);
        g_free(tmp);
    }

    tmp = read_ini_string(inifile, "skin", "mainwinMenurowVisible");

    if (tmp != NULL)
    {
        skin->properties.mainwin_menurow_visible = atoi(tmp);
        g_free(tmp);
    }

    tmp = read_ini_string(inifile, "skin", "mainwinVolumeX");

    if (tmp != NULL)
    {
        skin->properties.mainwin_volume_x = atoi(tmp);
        g_free(tmp);
    }

    tmp = read_ini_string(inifile, "skin", "mainwinVolumeY");

    if (tmp != NULL)
    {
        skin->properties.mainwin_volume_y = atoi(tmp);
        g_free(tmp);
    }

    tmp = read_ini_string(inifile, "skin", "mainwinBalanceX");

    if (tmp != NULL)
    {
        skin->properties.mainwin_balance_x = atoi(tmp);
        g_free(tmp);
    }

    tmp = read_ini_string(inifile, "skin", "mainwinBalanceY");

    if (tmp != NULL)
    {
        skin->properties.mainwin_balance_y = atoi(tmp);
        g_free(tmp);
    }

    tmp = read_ini_string(inifile, "skin", "mainwinPositionX");

    if (tmp != NULL)
    {
        skin->properties.mainwin_position_x = atoi(tmp);
        g_free(tmp);
    }

    tmp = read_ini_string(inifile, "skin", "mainwinPositionY");

    if (tmp != NULL)
    {
        skin->properties.mainwin_position_y = atoi(tmp);
        g_free(tmp);
    }

    tmp = read_ini_string(inifile, "skin", "mainwinOthertextIsStatus");

    if (tmp != NULL)
    {
        skin->properties.mainwin_othertext_is_status = atoi(tmp);
        g_free(tmp);
    }

    tmp = read_ini_string(inifile, "skin", "mainwinOthertextVisible");

    if (tmp != NULL)
    {
        skin->properties.mainwin_othertext_visible = atoi(tmp);
        g_free(tmp);
    }

    tmp = read_ini_string(inifile, "skin", "mainwinTextVisible");

    if (tmp != NULL)
    {
        skin->properties.mainwin_text_visible = atoi(tmp);
        g_free(tmp);
    }

    tmp = read_ini_string(inifile, "skin", "mainwinVisVisible");

    if (tmp != NULL)
    {
        skin->properties.mainwin_vis_visible = atoi(tmp);
        g_free(tmp);
    }

    tmp = read_ini_string(inifile, "skin", "mainwinPreviousX");

    if (tmp != NULL)
    {
        skin->properties.mainwin_previous_x = atoi(tmp);
        g_free(tmp);
    }

    tmp = read_ini_string(inifile, "skin", "mainwinPreviousY");

    if (tmp != NULL)
    {
        skin->properties.mainwin_previous_y = atoi(tmp);
        g_free(tmp);
    }

    tmp = read_ini_string(inifile, "skin", "mainwinPlayX");

    if (tmp != NULL)
    {
        skin->properties.mainwin_play_x = atoi(tmp);
        g_free(tmp);
    }

    tmp = read_ini_string(inifile, "skin", "mainwinPlayY");

    if (tmp != NULL)
    {
        skin->properties.mainwin_play_y = atoi(tmp);
        g_free(tmp);
    }

    tmp = read_ini_string(inifile, "skin", "mainwinPauseX");

    if (tmp != NULL)
    {
        skin->properties.mainwin_pause_x = atoi(tmp);
        g_free(tmp);
    }

    tmp = read_ini_string(inifile, "skin", "mainwinPauseY");

    if (tmp != NULL)
    {
        skin->properties.mainwin_pause_y = atoi(tmp);
        g_free(tmp);
    }

    tmp = read_ini_string(inifile, "skin", "mainwinStopX");

    if (tmp != NULL)
    {
        skin->properties.mainwin_stop_x = atoi(tmp);
        g_free(tmp);
    }

    tmp = read_ini_string(inifile, "skin", "mainwinStopY");

    if (tmp != NULL)
    {
        skin->properties.mainwin_stop_y = atoi(tmp);
        g_free(tmp);
    }

    tmp = read_ini_string(inifile, "skin", "mainwinNextX");

    if (tmp != NULL)
    {
        skin->properties.mainwin_next_x = atoi(tmp);
        g_free(tmp);
    }

    tmp = read_ini_string(inifile, "skin", "mainwinNextY");

    if (tmp != NULL)
    {
        skin->properties.mainwin_next_y = atoi(tmp);
        g_free(tmp);
    }

    tmp = read_ini_string(inifile, "skin", "mainwinEjectX");

    if (tmp != NULL)
    {
        skin->properties.mainwin_eject_x = atoi(tmp);
        g_free(tmp);
    }

    tmp = read_ini_string(inifile, "skin", "mainwinEjectY");

    if (tmp != NULL)
    {
        skin->properties.mainwin_eject_y = atoi(tmp);
        g_free(tmp);
    }

    tmp = read_ini_string(inifile, "skin", "mainwinWidth");

    if (tmp != NULL)
    {
        skin->properties.mainwin_width = atoi(tmp);
        g_free(tmp);
    }

    skin_mask_info[0].width = skin->properties.mainwin_width;

    tmp = read_ini_string(inifile, "skin", "mainwinHeight");

    if (tmp != NULL)
    {
        skin->properties.mainwin_height = atoi(tmp);
        g_free(tmp);
    }

    skin_mask_info[0].height = skin->properties.mainwin_height;

    tmp = read_ini_string(inifile, "skin", "mainwinAboutX");

    if (tmp != NULL)
    {
        skin->properties.mainwin_about_x = atoi(tmp);
        g_free(tmp);
    }

    tmp = read_ini_string(inifile, "skin", "mainwinAboutY");

    if (tmp != NULL)
    {
        skin->properties.mainwin_about_y = atoi(tmp);
        g_free(tmp);
    }

    tmp = read_ini_string(inifile, "skin", "mainwinShuffleX");

    if (tmp != NULL)
    {
        skin->properties.mainwin_shuffle_x = atoi(tmp);
        g_free(tmp);
    }

    tmp = read_ini_string(inifile, "skin", "mainwinShuffleY");

    if (tmp != NULL)
    {
        skin->properties.mainwin_shuffle_y = atoi(tmp);
        g_free(tmp);
    }

    tmp = read_ini_string(inifile, "skin", "mainwinRepeatX");

    if (tmp != NULL)
    {
        skin->properties.mainwin_repeat_x = atoi(tmp);
        g_free(tmp);
    }

    tmp = read_ini_string(inifile, "skin", "mainwinRepeatY");

    if (tmp != NULL)
    {
        skin->properties.mainwin_repeat_y = atoi(tmp);
        g_free(tmp);
    }

    tmp = read_ini_string(inifile, "skin", "mainwinEQButtonX");

    if (tmp != NULL)
    {
        skin->properties.mainwin_eqbutton_x = atoi(tmp);
        g_free(tmp);
    }

    tmp = read_ini_string(inifile, "skin", "mainwinEQButtonY");

    if (tmp != NULL)
    {
        skin->properties.mainwin_eqbutton_y = atoi(tmp);
        g_free(tmp);
    }

    tmp = read_ini_string(inifile, "skin", "mainwinPLButtonX");

    if (tmp != NULL)
    {
        skin->properties.mainwin_plbutton_x = atoi(tmp);
        g_free(tmp);
    }

    tmp = read_ini_string(inifile, "skin", "mainwinPLButtonY");

    if (tmp != NULL)
    {
        skin->properties.mainwin_plbutton_y = atoi(tmp);
        g_free(tmp);
    }

    tmp = read_ini_string(inifile, "skin", "textboxBitmapFontWidth");

    if (tmp != NULL)
    {
        skin->properties.textbox_bitmap_font_width = atoi(tmp);
        g_free(tmp);
    }

    tmp = read_ini_string(inifile, "skin", "textboxBitmapFontHeight");

    if (tmp != NULL)
    {
        skin->properties.textbox_bitmap_font_height = atoi(tmp);
        g_free(tmp);
    }

    tmp = read_ini_string(inifile, "skin", "mainwinMinimizeX");

    if (tmp != NULL)
    {
        skin->properties.mainwin_minimize_x = atoi(tmp);
        g_free(tmp);
    }

    tmp = read_ini_string(inifile, "skin", "mainwinMinimizeY");

    if (tmp != NULL)
    {
        skin->properties.mainwin_minimize_y = atoi(tmp);
        g_free(tmp);
    }

    tmp = read_ini_string(inifile, "skin", "mainwinShadeX");

    if (tmp != NULL)
    {
        skin->properties.mainwin_shade_x = atoi(tmp);
        g_free(tmp);
    }

    tmp = read_ini_string(inifile, "skin", "mainwinShadeY");

    if (tmp != NULL)
    {
        skin->properties.mainwin_shade_y = atoi(tmp);
        g_free(tmp);
    }

    tmp = read_ini_string(inifile, "skin", "mainwinCloseX");

    if (tmp != NULL)
    {
        skin->properties.mainwin_close_x = atoi(tmp);
        g_free(tmp);
    }

    tmp = read_ini_string(inifile, "skin", "mainwinCloseY");

    if (tmp != NULL)
    {
        skin->properties.mainwin_close_y = atoi(tmp);
        g_free(tmp);
    }

    if (filename != NULL)
        g_free(filename);

    close_ini_file(inifile);
}

static guint
hex_chars_to_int(gchar hi, gchar lo)
{
    /*
     * Converts a value in the range 0x00-0xFF
     * to a integer in the range 0-65535
     */
    gchar str[3];

    str[0] = hi;
    str[1] = lo;
    str[2] = 0;

    return (CLAMP(strtol(str, NULL, 16), 0, 0xFF) << 8);
}

static GdkColor *
skin_load_color(INIFile *inifile,
                const gchar * section, const gchar * key,
                gchar * default_hex)
{
    gchar *value;
    GdkColor *color = NULL;

    if (inifile || default_hex) {
        if (inifile) {
            value = g_strdup(read_ini_string(inifile, section, key));
            if (value == NULL) {
                value = g_strdup(default_hex);
            }
        } else {
            value = g_strdup(default_hex);
        }
        if (value) {
            gchar *ptr = value;
            gint len;

            color = g_new0(GdkColor, 1);
            g_strstrip(value);

            if (value[0] == '#')
                ptr++;
            len = strlen(ptr);
            /*
             * The handling of incomplete values is done this way
             * to maximize winamp compatibility
             */
            if (len >= 6) {
                color->red = hex_chars_to_int(*ptr, *(ptr + 1));
                ptr += 2;
            }
            if (len >= 4) {
                color->green = hex_chars_to_int(*ptr, *(ptr + 1));
                ptr += 2;
            }
            if (len >= 2)
                color->blue = hex_chars_to_int(*ptr, *(ptr + 1));

            gdk_colormap_alloc_color(gdk_drawable_get_colormap(playlistwin->window),
                            color, TRUE, TRUE);
            g_free(value);
        }
    }
    return color;
}



GdkBitmap *
skin_create_transparent_mask(const gchar * path,
                             const gchar * file,
                             const gchar * section,
                             GdkWindow * window,
                             gint width,
                             gint height, gboolean doublesize)
{
    GdkBitmap *mask = NULL;
    GdkGC *gc = NULL;
    GdkColor pattern;
    GdkPoint *gpoints;

    gchar *filename = NULL;
    INIFile *inifile = NULL;
    gboolean created_mask = FALSE;
    GArray *num, *point;
    guint i, j;
    gint k;

    if (path)
        filename = find_file_recursively(path, file);

    /* filename will be null if path wasn't set */
    if (!filename)
        return create_default_mask(window, width, height);

    inifile = open_ini_file(filename);

    if ((num = read_ini_array(inifile, section, "NumPoints")) == NULL) {
        g_free(filename);
        close_ini_file(inifile);
        return NULL;
    }

    if ((point = read_ini_array(inifile, section, "PointList")) == NULL) {
        g_array_free(num, TRUE);
        g_free(filename);
        close_ini_file(inifile);
        return NULL;
    }

    close_ini_file(inifile);

    mask = gdk_pixmap_new(window, width, height, 1);
    gc = gdk_gc_new(mask);

    pattern.pixel = 0;
    gdk_gc_set_foreground(gc, &pattern);
    gdk_draw_rectangle(mask, gc, TRUE, 0, 0, width, height);
    pattern.pixel = 1;
    gdk_gc_set_foreground(gc, &pattern);

    j = 0;
    for (i = 0; i < num->len; i++) {
        if ((int)(point->len - j) >= (g_array_index(num, gint, i) * 2)) {
            created_mask = TRUE;
            gpoints = g_new(GdkPoint, g_array_index(num, gint, i));
            for (k = 0; k < g_array_index(num, gint, i); k++) {
                gpoints[k].x =
                    g_array_index(point, gint, j + k * 2) * (1 + doublesize);
                gpoints[k].y =
                    g_array_index(point, gint,
                                  j + k * 2 + 1) * (1 + doublesize);
            }
            j += k * 2;
            gdk_draw_polygon(mask, gc, TRUE, gpoints,
                             g_array_index(num, gint, i));
            g_free(gpoints);
        }
    }
    g_array_free(num, TRUE);
    g_array_free(point, TRUE);
    g_free(filename);

    if (!created_mask)
        gdk_draw_rectangle(mask, gc, TRUE, 0, 0, width, height);

    g_object_unref(gc);

    return mask;
}

void
skin_load_viscolor(Skin * skin, const gchar * path, const gchar * basename)
{
    VFSFile *file;
    gint i, c;
    gchar line[256], *filename;
    GArray *a;

    g_return_if_fail(skin != NULL);
    g_return_if_fail(path != NULL);
    g_return_if_fail(basename != NULL);

    skin_set_default_vis_color(skin);

    filename = find_file_recursively(path, basename);
    if (!filename)
        return;

    if (!(file = vfs_fopen(filename, "r"))) {
        g_free(filename);
        return;
    }

    g_free(filename);

    for (i = 0; i < 24; i++) {
        if (vfs_fgets(line, 255, file)) {
            a = string_to_garray(line);
            if (a->len > 2) {
                for (c = 0; c < 3; c++)
                    skin->vis_color[i][c] = g_array_index(a, gint, c);
            }
            g_array_free(a, TRUE);
        }
        else
            break;
    }

    vfs_fclose(file);
}

static void
skin_numbers_generate_dash(Skin * skin)
{
    GdkGC *gc;
    GdkPixmap *pixmap;
    SkinPixmap *numbers;

    g_return_if_fail(skin != NULL);

    numbers = &skin->pixmaps[SKIN_NUMBERS];
    if (!numbers->pixmap || numbers->current_width < 99)
        return;

    pixmap = gdk_pixmap_new(NULL, 108,
                            numbers->current_height,
                            gdk_rgb_get_visual()->depth);
    gc = gdk_gc_new(pixmap);

    skin_draw_pixmap(NULL, skin, pixmap, gc, SKIN_NUMBERS, 0, 0, 0, 0, 99, numbers->current_height);
    skin_draw_pixmap(NULL, skin, pixmap, gc, SKIN_NUMBERS, 90, 0, 99, 0, 9, numbers->current_height);
    skin_draw_pixmap(NULL, skin, pixmap, gc, SKIN_NUMBERS, 20, 6, 101, 6, 5, 1);

    g_object_unref(numbers->pixmap);
    g_object_unref(gc);

    numbers->pixmap = pixmap;
    numbers->current_width = 108;
    numbers->width = 108;
}

static void
skin_load_cursor(Skin * skin, const gchar * dirname)
{
    const gchar * basename = "normal.cur";
    gchar * filename = NULL;
    GdkPixbuf * cursor_pixbuf = NULL;
    GdkPixbufAnimation * cursor_animated = NULL;
    GdkCursor * cursor_gdk = NULL;
    GError * error = NULL;
 
    filename = find_file_recursively(dirname, basename);

    if (filename && cfg.custom_cursors)
        cursor_animated = gdk_pixbuf_animation_new_from_file(filename, &error);

    if (cursor_animated) {
        cursor_pixbuf = gdk_pixbuf_animation_get_static_image(cursor_animated);
        cursor_gdk = gdk_cursor_new_from_pixbuf(gdk_display_get_default(),
                                                cursor_pixbuf, 0, 0);
    }
    else
        cursor_gdk = gdk_cursor_new(GDK_LEFT_PTR);

    if (mainwin && playlistwin && equalizerwin)
    {
        gdk_window_set_cursor(mainwin->window, cursor_gdk);
        gdk_window_set_cursor(playlistwin->window, cursor_gdk);
        gdk_window_set_cursor(equalizerwin->window, cursor_gdk);
    }

    gdk_cursor_unref(cursor_gdk);
}

static gboolean
skin_load_pixmaps(Skin * skin, const gchar * path)
{
    GdkPixmap *text_pm;
    guint i;
    gchar *filename;
    INIFile *inifile;

    for (i = 0; i < SKIN_PIXMAP_COUNT; i++)
        skin_load_pixmap_id(skin, i, path);

    text_pm = skin->pixmaps[SKIN_TEXT].pixmap;

    if (text_pm)
        skin_get_textcolors(text_pm, skin->textbg, skin->textfg);

    if (skin->pixmaps[SKIN_NUMBERS].pixmap &&
        skin->pixmaps[SKIN_NUMBERS].width < 108 )
        skin_numbers_generate_dash(skin);

    filename = find_file_recursively(path, "pledit.txt");
    inifile = open_ini_file(filename);

    if (!inifile)
        return FALSE;

    skin->colors[SKIN_PLEDIT_NORMAL] =
        skin_load_color(inifile, "Text", "Normal", "#2499ff");
    skin->colors[SKIN_PLEDIT_CURRENT] =
        skin_load_color(inifile, "Text", "Current", "#ffeeff");
    skin->colors[SKIN_PLEDIT_NORMALBG] =
        skin_load_color(inifile, "Text", "NormalBG", "#0a120a");
    skin->colors[SKIN_PLEDIT_SELECTEDBG] =
        skin_load_color(inifile, "Text", "SelectedBG", "#0a124a");

    if (filename)
        g_free(filename);
    close_ini_file(inifile);

    skin_mask_create(skin, path, SKIN_MASK_MAIN, mainwin->window);
    skin_mask_create(skin, path, SKIN_MASK_MAIN_SHADE, mainwin->window);

    skin_mask_create(skin, path, SKIN_MASK_EQ, equalizerwin->window);
    skin_mask_create(skin, path, SKIN_MASK_EQ_SHADE, equalizerwin->window);

    skin_load_viscolor(skin, path, "viscolor.txt");

    return TRUE;
}

static void
skin_set_gtk_theme(GtkSettings * settings, Skin * skin)
{
    if (original_gtk_theme == NULL)
         g_object_get(settings, "gtk-theme-name", &original_gtk_theme, NULL);

    /* the way GTK does things can be very broken. --nenolod */

    gchar *tmp = g_strdup_printf("%s/.themes/aud-%s", g_get_home_dir(),
                                 basename(skin->path));

    gchar *troot = g_strdup_printf("%s/.themes", g_get_home_dir());
    g_mkdir_with_parents(troot, 0755);
    g_free(troot);

    symlink(skin->path, tmp);
    gtk_settings_set_string_property(settings, "gtk-theme-name",
                                     basename(tmp), "audacious");
    g_free(tmp);
}

static gboolean
skin_load_nolock(Skin * skin, const gchar * path, gboolean force)
{
    GtkSettings *settings;
    gchar *cpath, *gtkrcpath;

    g_return_val_if_fail(skin != NULL, FALSE);
    g_return_val_if_fail(path != NULL, FALSE);
    REQUIRE_LOCK(skin->lock);

    if (!g_file_test(path, G_FILE_TEST_IS_REGULAR | G_FILE_TEST_IS_DIR))
        return FALSE;
   
    if (!force && skin->path && !strcmp(skin->path, path))
    return FALSE;
      
    skin_current_num++;

    skin->path = g_strdup(path);


    settings = gtk_settings_get_default();
    
    if (original_gtk_theme != NULL)
    {
        gtk_settings_set_string_property(settings, "gtk-theme-name",
                                         original_gtk_theme, "audacious");
        g_free(original_gtk_theme);
        original_gtk_theme = NULL;
    }


    if (!file_is_archive(path)) {
        /* Parse the hints for this skin. */
        skin_parse_hints(skin, NULL);

        if (!skin_load_pixmaps(skin, path))
            return FALSE;

        skin_load_cursor(skin, path);

#ifndef _WIN32
        gtkrcpath = find_path_recursively(skin->path, "gtkrc");
        if (gtkrcpath != NULL)
            skin_set_gtk_theme(settings, skin);
        g_free(gtkrcpath);
#endif

        return TRUE;
    }

    if (!(cpath = archive_decompress(path))) {
        g_message("Unable to extract skin archive (%s)", path);
        return FALSE;
    }

    /* Parse the hints for this skin. */
    skin_parse_hints(skin, cpath);

    if (!skin_load_pixmaps(skin, cpath))
    {
        del_directory(cpath);
        g_free(cpath);
        return FALSE;
    }

    skin_load_cursor(skin, cpath);

#ifndef _WIN32
    gtkrcpath = find_path_recursively(skin->path, "gtkrc");
    if (gtkrcpath != NULL)
        skin_set_gtk_theme(settings, skin);
    g_free(gtkrcpath);
#endif

    del_directory(cpath);
    g_free(cpath);

    return TRUE;
}

void
skin_install_skin(const gchar * path)
{
    gchar *command;

    g_return_if_fail(path != NULL);

    command = g_strdup_printf("cp %s %s",
                              path, bmp_paths[BMP_PATH_USER_SKIN_DIR]);
    if (system(command)) {
        g_message("Unable to install skin (%s) into user directory (%s)",
                  path, bmp_paths[BMP_PATH_USER_SKIN_DIR]);
    }
    g_free(command);
}

static SkinPixmap *
skin_get_pixmap(Skin * skin, SkinPixmapId map_id)
{
    g_return_val_if_fail(skin != NULL, NULL);
    g_return_val_if_fail(map_id < SKIN_PIXMAP_COUNT, NULL);

    return &skin->pixmaps[map_id];
}

gboolean
skin_load(Skin * skin, const gchar * path)
{
    gboolean error;

    g_return_val_if_fail(skin != NULL, FALSE);

    if (!path)
        return FALSE;

    skin_lock(skin);
    error = skin_load_nolock(skin, path, FALSE);
    skin_unlock(skin);

    SkinPixmap *pixmap = NULL;
    pixmap = skin_get_pixmap(skin, SKIN_NUMBERS);
    if (pixmap) {
        ui_skinned_number_set_size(mainwin_minus_num, 9, pixmap->height);
        ui_skinned_number_set_size(mainwin_10min_num, 9, pixmap->height);
        ui_skinned_number_set_size(mainwin_min_num, 9, pixmap->height);
        ui_skinned_number_set_size(mainwin_10sec_num, 9, pixmap->height);
        ui_skinned_number_set_size(mainwin_sec_num, 9, pixmap->height);
    }

    pixmap = skin_get_pixmap(skin, SKIN_MAIN);
    if (pixmap && skin->properties.mainwin_height > pixmap->height)
        skin->properties.mainwin_height = pixmap->height;

    pixmap = skin_get_pixmap(skin, SKIN_PLAYPAUSE);
    if (pixmap)
        ui_skinned_playstatus_set_size(mainwin_playstatus, 11, pixmap->height);

    pixmap = skin_get_pixmap(skin, SKIN_EQMAIN);
    if (pixmap->height >= 313)
        gtk_widget_show(equalizerwin_graph);

    return error;
}

gboolean
skin_reload_forced(void) 
{
   gboolean error;

   skin_lock(bmp_active_skin);
   error = skin_load_nolock(bmp_active_skin, bmp_active_skin->path, TRUE);
   skin_unlock(bmp_active_skin);

   return error;
}

void
skin_reload(Skin * skin)
{
    g_return_if_fail(skin != NULL);
    skin_load_nolock(skin, skin->path, TRUE);
}

GdkBitmap *
skin_get_mask(Skin * skin, SkinMaskId mi)
{
    GdkBitmap **masks;

    g_return_val_if_fail(skin != NULL, NULL);
    g_return_val_if_fail(mi < SKIN_PIXMAP_COUNT, NULL);

    masks = cfg.doublesize ? skin->ds_masks : skin->masks;
    return masks[mi];
}

GdkColor *
skin_get_color(Skin * skin, SkinColorId color_id)
{
    GdkColor *ret = NULL;

    g_return_val_if_fail(skin != NULL, NULL);

    switch (color_id) {
    case SKIN_TEXTBG:
        if (skin->pixmaps[SKIN_TEXT].pixmap)
            ret = skin->textbg;
        else
            ret = skin->def_textbg;
        break;
    case SKIN_TEXTFG:
        if (skin->pixmaps[SKIN_TEXT].pixmap)
            ret = skin->textfg;
        else
            ret = skin->def_textfg;
        break;
    default:
        if (color_id < SKIN_COLOR_COUNT)
            ret = skin->colors[color_id];
        break;
    }
    return ret;
}

void
skin_get_viscolor(Skin * skin, guchar vis_color[24][3])
{
    gint i;

    g_return_if_fail(skin != NULL);

    for (i = 0; i < 24; i++) {
        vis_color[i][0] = skin->vis_color[i][0];
        vis_color[i][1] = skin->vis_color[i][1];
        vis_color[i][2] = skin->vis_color[i][2];
    }
}

gint
skin_get_id(void)
{
    return skin_current_num;
}

void
skin_draw_pixmap(GtkWidget *widget, Skin * skin, GdkDrawable * drawable, GdkGC * gc,
                 SkinPixmapId pixmap_id,
                 gint xsrc, gint ysrc, gint xdest, gint ydest,
                 gint width, gint height)
{
    SkinPixmap *pixmap;

    g_return_if_fail(skin != NULL);

    pixmap = skin_get_pixmap(skin, pixmap_id);
    g_return_if_fail(pixmap != NULL);
    g_return_if_fail(pixmap->pixmap != NULL);

    /* FIXME: instead of copying stuff from SKIN_MAIN, we should use transparency or resize widget */
    if (xsrc+width > pixmap->width || ysrc+height > pixmap->height) {
        if (pixmap_id == SKIN_EQMAIN) {
            /* there are skins which EQMAIN doesn't include pixmap for equalizer graph */
            if (pixmap->height != 313) /* skins with EQMAIN which is 313 in height seems to display ok */
                gtk_widget_hide(equalizerwin_graph);
        } else if (widget) {
            /* it's better to hide widget using SKIN_PLAYPAUSE/SKIN_POSBAR than display mess */
            if ((pixmap_id == SKIN_PLAYPAUSE && pixmap->width != 42) || pixmap_id == SKIN_POSBAR) {
                gtk_widget_hide(widget);
                return;
            }
            gint x, y;
            x = -1;
            y = -1;

            if (gtk_widget_get_parent(widget) == SKINNED_WINDOW(mainwin)->fixed) {
                GList *iter;
                for (iter = GTK_FIXED (SKINNED_WINDOW(mainwin)->fixed)->children; iter; iter = g_list_next (iter)) {
                     GtkFixedChild *child_data = (GtkFixedChild *) iter->data;
                     if (child_data->widget == widget) {
                         x = child_data->x;
                         y = child_data->y;
                         break;
                     }
                }

                if (x != -1 && y != -1) {
                    /* Some skins include SKIN_VOLUME and/or SKIN_BALANCE
                       without knobs */
                    if (pixmap_id == SKIN_VOLUME || pixmap_id == SKIN_BALANCE) {
                        if (ysrc+height > 421 && xsrc+width <= pixmap->width)
                            return;
                    }
                    /* let's copy what's under widget */
                    gdk_draw_drawable(drawable, gc, skin_get_pixmap(bmp_active_skin, SKIN_MAIN)->pixmap,
                                      x, y, xdest, ydest, width, height);

                    /* some winamp skins have too strait SKIN_SHUFREP */
                    if (pixmap_id == SKIN_SHUFREP)
                        width = pixmap->width - xsrc;

                    if (pixmap_id == SKIN_VOLUME)
                        width = pixmap->width;

                    /* XMMS skins seems to have SKIN_MONOSTEREO with size 58x20 instead of 58x24 */
                    if (pixmap_id == SKIN_MONOSTEREO)
                        height = pixmap->height/2;
                }
            } else if (gtk_widget_get_parent(widget) == SKINNED_WINDOW(equalizerwin)->fixed) {
                   /* TODO */
            } else if (gtk_widget_get_parent(widget) == SKINNED_WINDOW(playlistwin)->fixed) {
                   /* TODO */
            }
        } else
            return;
    }

    width = MIN(width, pixmap->width - xsrc);
    height = MIN(height, pixmap->height - ysrc);
    gdk_draw_drawable(drawable, gc, pixmap->pixmap, xsrc, ysrc,
                      xdest, ydest, width, height);
}

void
skin_get_eq_spline_colors(Skin * skin, guint32 colors[19])
{
    gint i;
    GdkPixmap *pixmap;
    GdkImage *img;
    SkinPixmap *eqmainpm;

    g_return_if_fail(skin != NULL);

    eqmainpm = &skin->pixmaps[SKIN_EQMAIN];
    if (eqmainpm->pixmap &&
        eqmainpm->current_width >= 116 && eqmainpm->current_height >= 313)
        pixmap = eqmainpm->pixmap;
    else
        return;

    if (!GDK_IS_DRAWABLE(pixmap))
        return;

    if (!(img = gdk_drawable_get_image(pixmap, 115, 294, 1, 19)))
        return;

    for (i = 0; i < 19; i++)
        colors[i] = gdk_image_get_pixel(img, 0, i);

    g_object_unref(img);
}


static void
skin_draw_playlistwin_frame_top(Skin * skin,
                                GdkDrawable * drawable,
                                GdkGC * gc,
                                gint width, gint height, gboolean focus)
{
    /* The title bar skin consists of 2 sets of 4 images, 1 set
     * for focused state and the other for unfocused. The 4 images
     * are: 
     *
     * a. right corner (25,20)
     * b. left corner  (25,20)
     * c. tiler        (25,20)
     * d. title        (100,20)
     * 
     * min allowed width = 100+25+25 = 150
     */

    gint i, y, c;

    /* get y offset of the pixmap set to use */
    if (focus)
        y = 0;
    else
        y = 21;

    /* left corner */
    skin_draw_pixmap(NULL, skin, drawable, gc, SKIN_PLEDIT, 0, y, 0, 0, 25, 20);

    /* titlebar title */
    skin_draw_pixmap(NULL, skin, drawable, gc, SKIN_PLEDIT, 26, y,
                     (width - 100) / 2, 0, 100, 20);

    /* titlebar right corner  */
    skin_draw_pixmap(NULL, skin, drawable, gc, SKIN_PLEDIT, 153, y,
                     width - 25, 0, 25, 20);

    /* tile draw the remaining frame */

    /* compute tile count */
    c = (width - (100 + 25 + 25)) / 25;

    for (i = 0; i < c / 2; i++) {
        /* left of title */
        skin_draw_pixmap(NULL, skin, drawable, gc, SKIN_PLEDIT, 127, y,
                         25 + i * 25, 0, 25, 20);

        /* right of title */
        skin_draw_pixmap(NULL, skin, drawable, gc, SKIN_PLEDIT, 127, y,
                         (width + 100) / 2 + i * 25, 0, 25, 20);
    }

    if (c & 1) {
        /* Odd tile count, so one remaining to draw. Here we split
         * it into two and draw half on either side of the title */
        skin_draw_pixmap(NULL, skin, drawable, gc, SKIN_PLEDIT, 127, y,
                         ((c / 2) * 25) + 25, 0, 12, 20);
        skin_draw_pixmap(NULL, skin, drawable, gc, SKIN_PLEDIT, 127, y,
                         (width / 2) + ((c / 2) * 25) + 50, 0, 13, 20);
    }
}

static void
skin_draw_playlistwin_frame_bottom(Skin * skin,
                                   GdkDrawable * drawable,
                                   GdkGC * gc,
                                   gint width, gint height, gboolean focus)
{
    /* The bottom frame skin consists of 1 set of 4 images. The 4
     * images are:
     *
     * a. left corner with menu buttons (125,38)
     * b. visualization window (75,38)
     * c. right corner with play buttons (150,38)
     * d. frame tile (25,38)
     * 
     * (min allowed width = 125+150+25=300
     */

    gint i, c;

    /* bottom left corner (menu buttons) */
    skin_draw_pixmap(NULL, skin, drawable, gc, SKIN_PLEDIT, 0, 72,
                     0, height - 38, 125, 38);

    c = (width - 275) / 25;

    /* draw visualization window, if width allows */
    if (c >= 3) {
        c -= 3;
        skin_draw_pixmap(NULL, skin, drawable, gc, SKIN_PLEDIT, 205, 0,
                         width - (150 + 75), height - 38, 75, 38);
    }

    /* Bottom right corner (playbuttons etc) */
    skin_draw_pixmap(NULL, skin, drawable, gc, SKIN_PLEDIT,
                     126, 72, width - 150, height - 38, 150, 38);

    /* Tile draw the remaining undrawn portions */
    for (i = 0; i < c; i++)
        skin_draw_pixmap(NULL, skin, drawable, gc, SKIN_PLEDIT, 179, 0,
                         125 + i * 25, height - 38, 25, 38);
}

static void
skin_draw_playlistwin_frame_sides(Skin * skin,
                                  GdkDrawable * drawable,
                                  GdkGC * gc,
                                  gint width, gint height, gboolean focus)
{
    /* The side frames consist of 2 tile images. 1 for the left, 1 for
     * the right. 
     * a. left  (12,29)
     * b. right (19,29)
     */

    gint i;

    /* frame sides */
    for (i = 0; i < (height - (20 + 38)) / 29; i++) {
        /* left */
        skin_draw_pixmap(NULL, skin, drawable, gc, SKIN_PLEDIT, 0, 42,
                         0, 20 + i * 29, 12, 29);

        /* right */
        skin_draw_pixmap(NULL, skin, drawable, gc, SKIN_PLEDIT, 32, 42,
                         width - 19, 20 + i * 29, 19, 29);
    }
}


void
skin_draw_playlistwin_frame(Skin * skin,
                            GdkDrawable * drawable, GdkGC * gc,
                            gint width, gint height, gboolean focus)
{
    skin_draw_playlistwin_frame_top(skin, drawable, gc, width, height, focus);
    skin_draw_playlistwin_frame_bottom(skin, drawable, gc, width, height,
                                       focus);
    skin_draw_playlistwin_frame_sides(skin, drawable, gc, width, height,
                                      focus);
}


void
skin_draw_playlistwin_shaded(Skin * skin,
                             GdkDrawable * drawable, GdkGC * gc,
                             gint width, gboolean focus)
{
    /* The shade mode titlebar skin consists of 4 images:
     * a) left corner               offset (72,42) size (25,14)
     * b) right corner, focused     offset (99,57) size (50,14)
     * c) right corner, unfocused   offset (99,42) size (50,14)
     * d) bar tile                  offset (72,57) size (25,14)
     */

    gint i;

    /* left corner */
    skin_draw_pixmap(NULL, skin, drawable, gc, SKIN_PLEDIT, 72, 42, 0, 0, 25, 14);

    /* bar tile */
    for (i = 0; i < (width - 75) / 25; i++)
        skin_draw_pixmap(NULL, skin, drawable, gc, SKIN_PLEDIT, 72, 57,
                         (i * 25) + 25, 0, 25, 14);

    /* right corner */
    skin_draw_pixmap(NULL, skin, drawable, gc, SKIN_PLEDIT, 99, focus ? 42 : 57,
                     width - 50, 0, 50, 14);
}


void
skin_draw_mainwin_titlebar(Skin * skin,
                           GdkDrawable * drawable, GdkGC * gc,
                           gboolean shaded, gboolean focus)
{
    /* The titlebar skin consists of 2 sets of 2 images, one for for
     * shaded and the other for unshaded mode, giving a total of 4.
     * The images are exactly 275x14 pixels, aligned and arranged
     * vertically on each other in the pixmap in the following order:
     * 
     * a) unshaded, focused      offset (27, 0)
     * b) unshaded, unfocused    offset (27, 15)
     * c) shaded, focused        offset (27, 29)
     * d) shaded, unfocused      offset (27, 42)
     */

    gint y_offset;

    if (shaded) {
        if (focus)
            y_offset = 29;
        else
            y_offset = 42;
    }
    else {
        if (focus)
            y_offset = 0;
        else
            y_offset = 15;
    }

    skin_draw_pixmap(NULL, skin, drawable, gc, SKIN_TITLEBAR, 27, y_offset,
                     0, 0, bmp_active_skin->properties.mainwin_width, MAINWIN_TITLEBAR_HEIGHT);
}


void
skin_set_random_skin(void)
{
    SkinNode *node;
    guint32 randval;

    /* Get a random value to select the skin to use */
    randval = g_random_int_range(0, g_list_length(skinlist));
    node = g_list_nth(skinlist, randval)->data;
    bmp_active_skin_load(node->path);
}
