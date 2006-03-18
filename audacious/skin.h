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

#ifndef SKIN_H
#define SKIN_H


#include <glib.h>
#include <gdk/gdk.h>


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
	gboolean mainwin_othertext;
} SkinProperties;

#define SKIN_PIXMAP(x)  ((SkinPixmap *)(x))
typedef struct _SkinPixmap {
    GdkPixmap *pixmap;
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
    SkinProperties properties;
} Skin;

extern Skin *bmp_active_skin;

gboolean init_skins(const gchar * path);
void cleanup_skins(void);

gboolean bmp_active_skin_load(const gchar * path);
gboolean bmp_active_skin_reload(void);

Skin *skin_new(void);
gboolean skin_load(Skin * skin, const gchar * path);
void skin_reload(Skin * skin);
void skin_free(Skin * skin);

GdkBitmap *skin_get_mask(Skin * skin, SkinMaskId mi);
GdkColor *skin_get_color(Skin * skin, SkinColorId color_id);

void skin_get_viscolor(Skin * skin, guchar vis_color[24][3]);
gint skin_get_id(void);
void skin_draw_pixmap(Skin * skin, GdkDrawable * drawable, GdkGC * gc,
                      SkinPixmapId pixmap_id,
                      gint xsrc, gint ysrc, gint xdest, gint ydest,
                      gint width, gint height);
void skin_get_eq_spline_colors(Skin * skin, guint32 colors[19]);
void skin_install_skin(const gchar * path);

void skin_draw_playlistwin_shaded(Skin * skin,
                                  GdkDrawable * drawable, GdkGC * gc,
                                  gint width, gboolean focus);
void skin_draw_playlistwin_frame(Skin * skin,
                                 GdkDrawable * drawable, GdkGC * gc,
                                 gint width, gint height, gboolean focus);

void skin_draw_mainwin_titlebar(Skin * skin,
                                GdkDrawable * drawable, GdkGC * gc,
                                gboolean shaded, gboolean focus);


void skin_parse_hints(Skin * skin, gchar *path_p);


gboolean
skin_reload_forced(void);

#endif
