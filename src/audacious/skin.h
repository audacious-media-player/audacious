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

#ifndef SKIN_H
#define SKIN_H


#include <glib.h>
#include <gdk/gdk.h>
#include <gtk/gtk.h>

#define BMP_DEFAULT_SKIN_PATH \
  DATA_DIR G_DIR_SEPARATOR_S "Skins" G_DIR_SEPARATOR_S "Default"


typedef enum {
    SKIN_MAIN = 0,
    SKIN_CBUTTONS,
    SKIN_TITLEBAR,
    SKIN_SHUFREP,
    SKIN_TEXT,
    SKIN_VOLUME,
    SKIN_BALANCE,
    SKIN_MONOSTEREO,
    SKIN_PLAYPAUSE,
    SKIN_NUMBERS,
    SKIN_POSBAR,
    SKIN_PLEDIT,
    SKIN_EQMAIN,
    SKIN_EQ_EX,
    SKIN_PIXMAP_COUNT
} SkinPixmapId;

typedef enum {
    SKIN_MASK_MAIN = 0,
    SKIN_MASK_MAIN_SHADE,
    SKIN_MASK_EQ,
    SKIN_MASK_EQ_SHADE,
    SKIN_MASK_COUNT
} SkinMaskId;

typedef enum {
    SKIN_PLEDIT_NORMAL = 0,
    SKIN_PLEDIT_CURRENT,
    SKIN_PLEDIT_NORMALBG,
    SKIN_PLEDIT_SELECTEDBG,
    SKIN_TEXTBG,
    SKIN_TEXTFG,
    SKIN_COLOR_COUNT
} SkinColorId;

typedef struct _SkinProperties {
	/* this enables the othertext engine, not it's visibility -nenolod */
	gboolean mainwin_othertext;

	/* Vis properties */
	gint mainwin_vis_x;
	gint mainwin_vis_y;
	gint mainwin_vis_width;
	gboolean mainwin_vis_visible;

	/* Text properties */
	gint mainwin_text_x;
	gint mainwin_text_y;
	gint mainwin_text_width;
	gboolean mainwin_text_visible;

	/* Infobar properties */
	gint mainwin_infobar_x;
	gint mainwin_infobar_y;
	gboolean mainwin_othertext_visible;

	gint mainwin_number_0_x;
	gint mainwin_number_0_y;

	gint mainwin_number_1_x;
	gint mainwin_number_1_y;

	gint mainwin_number_2_x;
	gint mainwin_number_2_y;

	gint mainwin_number_3_x;
	gint mainwin_number_3_y;

	gint mainwin_number_4_x;
	gint mainwin_number_4_y;

	gint mainwin_playstatus_x;
	gint mainwin_playstatus_y;

	gint mainwin_volume_x;
	gint mainwin_volume_y;	

	gint mainwin_balance_x;
	gint mainwin_balance_y;	

	gint mainwin_position_x;
	gint mainwin_position_y;

	gint mainwin_previous_x;
	gint mainwin_previous_y;

	gint mainwin_play_x;
	gint mainwin_play_y;

	gint mainwin_pause_x;
	gint mainwin_pause_y;

	gint mainwin_stop_x;
	gint mainwin_stop_y;

	gint mainwin_next_x;
	gint mainwin_next_y;

	gint mainwin_eject_x;
	gint mainwin_eject_y;

	gint mainwin_eqbutton_x;
	gint mainwin_eqbutton_y;

	gint mainwin_plbutton_x;
	gint mainwin_plbutton_y;

	gint mainwin_shuffle_x;
	gint mainwin_shuffle_y;

	gint mainwin_repeat_x;
	gint mainwin_repeat_y;

	gint mainwin_about_x;
	gint mainwin_about_y;

	gint mainwin_minimize_x;
	gint mainwin_minimize_y;

	gint mainwin_shade_x;
	gint mainwin_shade_y;

	gint mainwin_close_x;
	gint mainwin_close_y;

	gint mainwin_width;
	gint mainwin_height;

	gboolean mainwin_menurow_visible;
	gboolean mainwin_othertext_is_status;

	gint textbox_bitmap_font_width;
	gint textbox_bitmap_font_height;
} SkinProperties;

#define SKIN_PIXMAP(x)  ((SkinPixmap *)(x))
typedef struct _SkinPixmap {
    GdkPixbuf *pixbuf;
    /* GdkPixmap *def_pixmap; */

    /* The real size of the pixmap */
    gint width, height;

    /* The size of the pixmap from the current skin,
       which might be smaller */
    gint current_width, current_height;
} SkinPixmap;


#define SKIN(x)  ((Skin *)(x))
typedef struct _Skin {
    GMutex *lock;
    gchar *path;
    gchar *def_path;
    SkinPixmap pixmaps[SKIN_PIXMAP_COUNT];
    GdkColor textbg[6], def_textbg[6];
    GdkColor textfg[6], def_textfg[6];
    GdkColor *colors[SKIN_COLOR_COUNT];
    guchar vis_color[24][3];
    GdkBitmap *masks[SKIN_MASK_COUNT];
    GdkBitmap *ds_masks[SKIN_MASK_COUNT];
    SkinProperties properties;
} Skin;

extern Skin *bmp_active_skin;

gboolean init_skins(const gchar * path);
void cleanup_skins(void);

gboolean bmp_active_skin_load(const gchar * path);
gboolean bmp_active_skin_reload(void);

Skin *skin_new(void);
gboolean skin_load(Skin * skin, const gchar * path);
gboolean skin_reload_forced(void);
void skin_reload(Skin * skin);
void skin_free(Skin * skin);

GdkBitmap *skin_get_mask(Skin * skin, SkinMaskId mi);
GdkColor *skin_get_color(Skin * skin, SkinColorId color_id);

void skin_get_viscolor(Skin * skin, guchar vis_color[24][3]);
gint skin_get_id(void);
void skin_draw_pixbuf(GtkWidget *widget, Skin * skin, GdkPixbuf * pix,
                 SkinPixmapId pixmap_id,
                 gint xsrc, gint ysrc, gint xdest, gint ydest,
                 gint width, gint height);

void skin_get_eq_spline_colors(Skin * skin, guint32 colors[19]);
void skin_install_skin(const gchar * path);

void skin_draw_playlistwin_shaded(Skin * skin, GdkPixbuf * pix,
                                  gint width, gboolean focus);
void skin_draw_playlistwin_frame(Skin * skin, GdkPixbuf * pix,
                                 gint width, gint height, gboolean focus);

void skin_draw_mainwin_titlebar(Skin * skin, GdkPixbuf * pix,
                                gboolean shaded, gboolean focus);


void skin_parse_hints(Skin * skin, gchar *path_p);


void skin_set_random_skin(void);

#endif
