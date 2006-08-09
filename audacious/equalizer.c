/*  Audacious - Cross-platform multimedia player
 *  Copyright (C) 2005-2006  Audacious development team.
 *
 *  BMP - Cross-platform multimedia player
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
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include "equalizer.h"

#include <glib.h>
#include <glib/gi18n.h>
#include <gtk/gtk.h>
#include <gdk/gdk.h>
#include <gdk/gdkkeysyms.h>
#include <stdio.h>
#include <math.h>
#include <string.h>

#include <gdk/gdkx.h>
#include <X11/Xlib.h>

#include "dock.h"
#include "widgets/eq_graph.h"
#include "widgets/eq_slider.h"
#include "hints.h"
#include "widgets/hslider.h"
#include "input.h"
#include "main.h"
#include "widgets/pbutton.h"
#include "playlist.h"
#include "ui_playlist.h"
#include "widgets/skin.h"
#include "widgets/tbutton.h"
#include "util.h"
#include "output.h"

#include "libaudacious/rcfile.h"
#include "libaudacious/vfs.h"

#include "images/audacious_eq.xpm"

enum PresetViewCols {
    PRESET_VIEW_COL_NAME,
    PRESET_VIEW_N_COLS
};

enum {
    EQUALIZER_PRESETS_LOAD_PRESET,
    EQUALIZER_PRESETS_LOAD_AUTOPRESET,
    EQUALIZER_PRESETS_LOAD_DEFAULT,
    EQUALIZER_PRESETS_LOAD_ZERO,
    EQUALIZER_PRESETS_LOAD_FROM_FILE,
    EQUALIZER_PRESETS_LOAD_FROM_WINAMPFILE,
    EQUALIZER_PRESETS_IMPORT_WINAMPFILE,
    EQUALIZER_PRESETS_SAVE_PRESET,
    EQUALIZER_PRESETS_SAVE_AUTOPRESET,
    EQUALIZER_PRESETS_SAVE_DEFAULT,
    EQUALIZER_PRESETS_SAVE_TO_FILE,
    EQUALIZER_PRESETS_SAVE_TO_WINAMPFILE,
    EQUALIZER_PRESETS_DELETE_PRESET,
    EQUALIZER_PRESETS_DELETE_AUTOPRESET
};


struct _EqualizerPreset {
    gchar *name;
    gfloat preamp, bands[10];
};

typedef struct _EqualizerPreset EqualizerPreset;


GtkWidget *equalizerwin;

static GtkWidget *equalizerwin_load_window = NULL;
static GtkWidget *equalizerwin_load_auto_window = NULL;
static GtkWidget *equalizerwin_save_window = NULL;
static GtkWidget *equalizerwin_save_entry;
static GtkWidget *equalizerwin_save_auto_window = NULL;
static GtkWidget *equalizerwin_save_auto_entry;
static GtkWidget *equalizerwin_delete_window = NULL;
static GtkWidget *equalizerwin_delete_auto_window = NULL;

static GdkPixmap *equalizerwin_bg;
static GdkGC *equalizerwin_gc;

static GList *equalizerwin_wlist = NULL;

static GtkAccelGroup *equalizerwin_accel;

static TButton *equalizerwin_on, *equalizerwin_auto;

static PButton *equalizerwin_presets, *equalizerwin_shade;
PButton *equalizerwin_close;
static EqGraph *equalizerwin_graph;
static EqSlider *equalizerwin_preamp, *equalizerwin_bands[10];
static HSlider *equalizerwin_volume, *equalizerwin_balance;

static GtkItemFactory *equalizerwin_presets_menu;

static GList *equalizer_presets = NULL, *equalizer_auto_presets = NULL;


static void equalizerwin_presets_menu_cb(gpointer cb_data, guint action,
                                         GtkWidget * w);

static GtkItemFactoryEntry equalizerwin_presets_menu_entries[] = {
    {N_("/Load"), NULL, NULL, 0, "<Branch>", NULL},
    {N_("/Load/Preset"), NULL, equalizerwin_presets_menu_cb,
     EQUALIZER_PRESETS_LOAD_PRESET, "<Item>", NULL},
    {N_("/Load/Auto-load preset"), NULL, equalizerwin_presets_menu_cb,
     EQUALIZER_PRESETS_LOAD_AUTOPRESET, "<Item>", NULL},
    {N_("/Load/Default"), NULL, equalizerwin_presets_menu_cb,
     EQUALIZER_PRESETS_LOAD_DEFAULT, "<Item>", NULL},
    {"/Load/-", NULL, NULL, 0, "<Separator>", NULL},
    {N_("/Load/Zero"), NULL, equalizerwin_presets_menu_cb,
     EQUALIZER_PRESETS_LOAD_ZERO, "<Item>", NULL},
    {"/Load/-", NULL, NULL, 0, "<Separator>", NULL},
    {N_("/Load/From file"), NULL, equalizerwin_presets_menu_cb,
     EQUALIZER_PRESETS_LOAD_FROM_FILE, "<Item>", NULL},
    {N_("/Load/From WinAMP EQF file"), NULL, equalizerwin_presets_menu_cb,
     EQUALIZER_PRESETS_LOAD_FROM_WINAMPFILE, "<Item>", NULL},
    {N_("/Import"), NULL, NULL, 0, "<Branch>", NULL},
    {N_("/Import/WinAMP Presets"), NULL, equalizerwin_presets_menu_cb,
     EQUALIZER_PRESETS_IMPORT_WINAMPFILE, "<Item>", NULL},
    {N_("/Save"), NULL, NULL, 0, "<Branch>", NULL},
    {N_("/Save/Preset"), NULL, equalizerwin_presets_menu_cb,
     EQUALIZER_PRESETS_SAVE_PRESET, "<Item>", NULL},
    {N_("/Save/Auto-load preset"), NULL, equalizerwin_presets_menu_cb,
     EQUALIZER_PRESETS_SAVE_AUTOPRESET, "<Item>", NULL},
    {N_("/Save/Default"), NULL, equalizerwin_presets_menu_cb,
     EQUALIZER_PRESETS_SAVE_DEFAULT, "<Item>", NULL},
    {"/Save/-", NULL, NULL, 0, "<Separator>", NULL},
    {N_("/Save/To file"), NULL, equalizerwin_presets_menu_cb,
     EQUALIZER_PRESETS_SAVE_TO_FILE, "<Item>", NULL},
    {N_("/Save/To WinAMP EQF file"), NULL, equalizerwin_presets_menu_cb,
     EQUALIZER_PRESETS_SAVE_TO_WINAMPFILE, "<Item>", NULL},
    {N_("/Delete"), NULL, NULL, 0, "<Branch>", NULL},
    {N_("/Delete/Preset"), NULL, equalizerwin_presets_menu_cb,
     EQUALIZER_PRESETS_DELETE_PRESET, "<Item>", NULL},
    {N_("/Delete/Auto-load preset"), NULL, equalizerwin_presets_menu_cb,
     EQUALIZER_PRESETS_DELETE_AUTOPRESET, "<Item>", NULL},
};

static gint equalizerwin_presets_menu_entries_num =
    G_N_ELEMENTS(equalizerwin_presets_menu_entries);


EqualizerPreset *
equalizer_preset_new(const gchar * name)
{
    EqualizerPreset *preset = g_new0(EqualizerPreset, 1);
    preset->name = g_strdup(name);
    return preset;
}

void
equalizer_preset_free(EqualizerPreset * preset)
{
    if (!preset)
        return;

    g_free(preset->name);
    g_free(preset);
}


static void
equalizerwin_set_shape_mask(void)
{
    GdkBitmap *mask;

    if (cfg.show_wm_decorations)
        return;

    mask = skin_get_mask(bmp_active_skin, SKIN_MASK_EQ + cfg.equalizer_shaded);
    gtk_widget_shape_combine_mask(equalizerwin, mask, 0, 0);
}


void
equalizerwin_set_shade_menu_cb(gboolean shaded)
{
    cfg.equalizer_shaded = shaded;

    equalizerwin_set_shape_mask();

    if (shaded) {
        dock_shade(dock_window_list, GTK_WINDOW(equalizerwin), 14);
        pbutton_set_button_data(equalizerwin_shade, -1, 3, -1, 47);
        pbutton_set_skin_index1(equalizerwin_shade, SKIN_EQ_EX);
        pbutton_set_button_data(equalizerwin_close, 11, 38, 11, 47);
        pbutton_set_skin_index(equalizerwin_close, SKIN_EQ_EX);
        widget_show(WIDGET(equalizerwin_volume));
        widget_show(WIDGET(equalizerwin_balance));
    }
    else {
        dock_shade(dock_window_list, GTK_WINDOW(equalizerwin), 116);
        pbutton_set_button_data(equalizerwin_shade, -1, 137, -1, 38);
        pbutton_set_skin_index1(equalizerwin_shade, SKIN_EQMAIN);
        pbutton_set_button_data(equalizerwin_close, 0, 116, 0, 125);
        pbutton_set_skin_index(equalizerwin_close, SKIN_EQMAIN);
        widget_hide(WIDGET(equalizerwin_volume));
        widget_hide(WIDGET(equalizerwin_balance));
    }

    draw_equalizer_window(TRUE);
}

static void
equalizerwin_set_shade(gboolean shaded)
{
    GtkWidget *widget;
    widget = gtk_item_factory_get_widget(mainwin_view_menu,
                                         "/Roll up Equalizer");
    gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(widget), shaded);
}

static void
equalizerwin_shade_toggle(void)
{
    equalizerwin_set_shade(!cfg.equalizer_shaded);
}

static void
equalizerwin_raise(void)
{
    if (cfg.equalizer_visible)
        gtk_window_present(GTK_WINDOW(equalizerwin));
}

void
equalizerwin_eq_changed(void)
{
    gint i;

    cfg.equalizer_preamp = eqslider_get_position(equalizerwin_preamp);
    for (i = 0; i < 10; i++)
        cfg.equalizer_bands[i] = eqslider_get_position(equalizerwin_bands[i]);
    /* um .. i think we need both of these for xmms compatibility ..
       not sure. -larne */
    input_set_eq(cfg.equalizer_active, cfg.equalizer_preamp,
                 cfg.equalizer_bands);
    output_set_eq(cfg.equalizer_active, cfg.equalizer_preamp,
                  cfg.equalizer_bands);

    widget_draw(WIDGET(equalizerwin_graph));
}

static void
equalizerwin_on_pushed(gboolean toggled)
{
    cfg.equalizer_active = toggled;
    equalizerwin_eq_changed();
}

static void
equalizerwin_presets_pushed(void)
{
    GdkModifierType modmask;
    gint x, y;

    gdk_window_get_pointer(NULL, &x, &y, &modmask);
    util_item_factory_popup(equalizerwin_presets_menu, x, y, 1,
                            GDK_CURRENT_TIME);
}

static void
equalizerwin_auto_pushed(gboolean toggled)
{
    cfg.equalizer_autoload = toggled;
}

void
draw_equalizer_window(gboolean force)
{
    gboolean redraw;

    widget_list_lock(equalizerwin_wlist);

    if (force) {
        skin_draw_pixmap(bmp_active_skin, equalizerwin_bg, equalizerwin_gc,
                         SKIN_EQMAIN, 0, 0, 0, 0, 275, 116);
        if (gtk_window_has_toplevel_focus(GTK_WINDOW(equalizerwin)) ||
            !cfg.dim_titlebar) {
            if (!cfg.equalizer_shaded)
                skin_draw_pixmap(bmp_active_skin, equalizerwin_bg,
                                 equalizerwin_gc, SKIN_EQMAIN, 0, 134, 0,
                                 0, 275, 14);
            else
                skin_draw_pixmap(bmp_active_skin, equalizerwin_bg,
                                 equalizerwin_gc, SKIN_EQ_EX, 0, 0, 0, 0,
                                 275, 14);
        }
        else {
            if (!cfg.equalizer_shaded)
                skin_draw_pixmap(bmp_active_skin, equalizerwin_bg,
                                 equalizerwin_gc, SKIN_EQMAIN, 0, 149, 0,
                                 0, 275, 14);
            else
                skin_draw_pixmap(bmp_active_skin, equalizerwin_bg,
                                 equalizerwin_gc, SKIN_EQ_EX, 0, 15, 0, 0,
                                 275, 14);

        }
    }

    widget_list_draw(equalizerwin_wlist, &redraw, force);

    if (force || redraw) {
        widget_list_clear_redraw(equalizerwin_wlist);
        gdk_window_clear(equalizerwin->window);
        gdk_flush();
    }

    widget_list_unlock(equalizerwin_wlist);
}

static gboolean
inside_sensitive_widgets(gint x, gint y)
{
    return (widget_contains(WIDGET(equalizerwin_on), x, y) ||
            widget_contains(WIDGET(equalizerwin_auto), x, y) ||
            widget_contains(WIDGET(equalizerwin_presets), x, y) ||
            widget_contains(WIDGET(equalizerwin_close), x, y) ||
            widget_contains(WIDGET(equalizerwin_shade), x, y) ||
            widget_contains(WIDGET(equalizerwin_preamp), x, y) ||
            widget_contains(WIDGET(equalizerwin_bands[0]), x, y) ||
            widget_contains(WIDGET(equalizerwin_bands[1]), x, y) ||
            widget_contains(WIDGET(equalizerwin_bands[2]), x, y) ||
            widget_contains(WIDGET(equalizerwin_bands[3]), x, y) ||
            widget_contains(WIDGET(equalizerwin_bands[4]), x, y) ||
            widget_contains(WIDGET(equalizerwin_bands[5]), x, y) ||
            widget_contains(WIDGET(equalizerwin_bands[6]), x, y) ||
            widget_contains(WIDGET(equalizerwin_bands[7]), x, y) ||
            widget_contains(WIDGET(equalizerwin_bands[8]), x, y) ||
            widget_contains(WIDGET(equalizerwin_bands[9]), x, y) ||
            widget_contains(WIDGET(equalizerwin_volume), x, y) ||
            widget_contains(WIDGET(equalizerwin_balance), x, y));
}

gboolean
equalizerwin_press(GtkWidget * widget, GdkEventButton * event,
                   gpointer callback_data)
{
    gint mx, my;
    gboolean grab = TRUE;

    mx = event->x;
    my = event->y;

    if (event->button == 1 && event->type == GDK_BUTTON_PRESS &&
        ((cfg.equalizer_shaded || event->y < 14) &&
         !inside_sensitive_widgets(event->x, event->y))) {
        if (0 && hint_move_resize_available()) {
            hint_move_resize(equalizerwin, event->x_root,
                             event->y_root, TRUE);
            grab = FALSE;
        }
        else {
            equalizerwin_raise();
            dock_move_press(dock_window_list, GTK_WINDOW(equalizerwin), event,
                            FALSE);
        }
    }
    else if (event->button == 1 && event->type == GDK_2BUTTON_PRESS
             && event->y < 14) {
        equalizerwin_set_shade(!cfg.equalizer_shaded);
        if (dock_is_moving(GTK_WINDOW(equalizerwin)))
            dock_move_release(GTK_WINDOW(equalizerwin));
    }
    else if (event->button == 3 &&
             !(widget_contains(WIDGET(equalizerwin_on), event->x, event->y) ||
               widget_contains(WIDGET(equalizerwin_auto), event->x, event->y))) {
        /*
         * Pop up the main menu a few pixels down to avoid
         * anything to be selected initially.
         */
        util_item_factory_popup(mainwin_general_menu, event->x_root,
                                event->y_root + 2, 3, event->time);
        grab = FALSE;
    }
    else {
        handle_press_cb(equalizerwin_wlist, widget, event);
        draw_equalizer_window(FALSE);
    }
    if (grab)
        gdk_pointer_grab(GDK_WINDOW(equalizerwin->window), FALSE,
                         GDK_BUTTON_MOTION_MASK | GDK_BUTTON_RELEASE_MASK,
                         NULL, NULL, GDK_CURRENT_TIME);

    return FALSE;
}

static void
equalizerwin_scroll(GtkWidget * widget, GdkEventScroll * event, gpointer data)
{
    handle_scroll_cb(equalizerwin_wlist, widget, event);
    draw_equalizer_window(FALSE);
}

static gboolean
equalizerwin_motion(GtkWidget * widget,
                    GdkEventMotion * event, gpointer callback_data)
{
    GdkEvent *gevent;

    if (dock_is_moving(GTK_WINDOW(equalizerwin)))
    {
        dock_move_motion(GTK_WINDOW(equalizerwin), event);
    }
    else 
    {
        handle_motion_cb(equalizerwin_wlist, widget, event);
        draw_equalizer_window(FALSE);
    }

    gdk_flush();

    while ((gevent = gdk_event_get()) != NULL) gdk_event_free(gevent);

    return FALSE;
}

static gboolean
equalizerwin_release(GtkWidget * widget,
                     GdkEventButton * event, gpointer callback_data)
{
    gdk_pointer_ungrab(GDK_CURRENT_TIME);
    gdk_flush();

    if (dock_is_moving(GTK_WINDOW(equalizerwin))) {
        dock_move_release(GTK_WINDOW(equalizerwin));
    }
    else {
        handle_release_cb(equalizerwin_wlist, widget, event);
        draw_equalizer_window(FALSE);
    }

    return FALSE;
}

static gboolean
equalizerwin_focus_in(GtkWidget * widget,
                      GdkEvent * event,
                      gpointer data)
{
    equalizerwin_close->pb_allow_draw = TRUE;
    equalizerwin_shade->pb_allow_draw = TRUE;
    draw_equalizer_window(TRUE);
    return TRUE;
}

static gboolean
equalizerwin_focus_out(GtkWidget * widget,
                       GdkEventButton * event,
                       gpointer data)
{
    equalizerwin_close->pb_allow_draw = FALSE;
    equalizerwin_shade->pb_allow_draw = FALSE;
    draw_equalizer_window(TRUE);
    return TRUE;
}

static gboolean
equalizerwin_keypress(GtkWidget * widget,
                      GdkEventKey * event,
                      gpointer data)
{
    if (!cfg.equalizer_shaded) {
        gtk_widget_event(mainwin, (GdkEvent *) event);
        return TRUE;
    }

    switch (event->keyval) {
    case GDK_Left:
    case GDK_KP_Left:
        mainwin_set_balance_diff(-4);
        break;
    case GDK_Right:
    case GDK_KP_Right:
        mainwin_set_balance_diff(4);
        break;
    default:
        gtk_widget_event(mainwin, (GdkEvent *) event);
        break;
    }

    return FALSE;
}

static gboolean
equalizerwin_configure(GtkWidget * window,
                       GdkEventConfigure * event,
                       gpointer data)
{
    if (!GTK_WIDGET_VISIBLE(window))
        return FALSE;

    cfg.equalizer_x = event->x;
    cfg.equalizer_y = event->y;
    return FALSE;
}

static void
equalizerwin_set_back_pixmap(void)
{
    gdk_window_set_back_pixmap(equalizerwin->window, equalizerwin_bg, 0);
    gdk_window_clear(equalizerwin->window);
}

static void
equalizerwin_close_cb(void)
{
    equalizerwin_show(FALSE);
}

static gboolean
equalizerwin_delete(GtkWidget * widget,
                    gpointer data)
{
    equalizerwin_show(FALSE);
    return TRUE;
}

static GList *
equalizerwin_read_presets(const gchar * basename)
{
    gchar *filename, *name;
    RcFile *rcfile;
    GList *list = NULL;
    gint i, p = 0;
    EqualizerPreset *preset;

    filename = g_build_filename(bmp_paths[BMP_PATH_USER_DIR], basename, NULL);

    if ((rcfile = bmp_rcfile_open(filename)) == NULL) {
        g_free(filename);
        return NULL;
    }

    g_free(filename);

    for (;;) {
        gchar section[21];

        g_snprintf(section, sizeof(section), "Preset%d", p++);
        if (bmp_rcfile_read_string(rcfile, "Presets", section, &name)) {
            preset = g_new0(EqualizerPreset, 1);
            preset->name = name;
            bmp_rcfile_read_float(rcfile, name, "Preamp", &preset->preamp);
            for (i = 0; i < 10; i++) {
                gchar band[7];
                g_snprintf(band, sizeof(band), "Band%d", i);
                bmp_rcfile_read_float(rcfile, name, band, &preset->bands[i]);
            }
            list = g_list_prepend(list, preset);
        }
        else
            break;
    }
    list = g_list_reverse(list);
    bmp_rcfile_free(rcfile);
    return list;
}

gint
equalizerwin_volume_frame_cb(gint pos)
{
    if (equalizerwin_volume) {
        if (pos < 32)
            equalizerwin_volume->hs_knob_nx =
                equalizerwin_volume->hs_knob_px = 1;
        else if (pos < 63)
            equalizerwin_volume->hs_knob_nx =
                equalizerwin_volume->hs_knob_px = 4;
        else
            equalizerwin_volume->hs_knob_nx =
                equalizerwin_volume->hs_knob_px = 7;
    }
    return 1;
}

static void
equalizerwin_volume_motion_cb(gint pos)
{
    gint v = (gint) rint(pos * 100 / 94.0);
    mainwin_adjust_volume_motion(v);
    mainwin_set_volume_slider(v);
}

static void
equalizerwin_volume_release_cb(gint pos)
{
    mainwin_adjust_volume_release();
}

static gint
equalizerwin_balance_frame_cb(gint pos)
{
    if (equalizerwin_balance) {
        if (pos < 13)
            equalizerwin_balance->hs_knob_nx =
                equalizerwin_balance->hs_knob_px = 11;
        else if (pos < 26)
            equalizerwin_balance->hs_knob_nx =
                equalizerwin_balance->hs_knob_px = 14;
        else
            equalizerwin_balance->hs_knob_nx =
                equalizerwin_balance->hs_knob_px = 17;
    }

    return 1;
}

static void
equalizerwin_balance_motion_cb(gint pos)
{
    gint b;
    pos = MIN(pos, 38);         /* The skin uses a even number of pixels
                                   for the balance-slider *sigh* */
    b = (gint) rint((pos - 19) * 100 / 19.0);
    mainwin_adjust_balance_motion(b);
    mainwin_set_balance_slider(b);
}

static void
equalizerwin_balance_release_cb(gint pos)
{
    mainwin_adjust_balance_release();
}

void
equalizerwin_set_balance_slider(gint percent)
{
    hslider_set_position(equalizerwin_balance,
                         (gint) rint((percent * 19 / 100.0) + 19));
}

void
equalizerwin_set_volume_slider(gint percent)
{
    hslider_set_position(equalizerwin_volume,
                         (gint) rint(percent * 94 / 100.0));
}

static void
equalizerwin_create_widgets(void)
{
    gint i;

    equalizerwin_on =
        create_tbutton(&equalizerwin_wlist, equalizerwin_bg,
                       equalizerwin_gc, 14, 18, 25, 12, 10, 119, 128, 119,
                       69, 119, 187, 119, equalizerwin_on_pushed,
                       SKIN_EQMAIN);
    tbutton_set_toggled(equalizerwin_on, cfg.equalizer_active);
    equalizerwin_auto =
        create_tbutton(&equalizerwin_wlist, equalizerwin_bg,
                       equalizerwin_gc, 39, 18, 33, 12, 35, 119, 153, 119,
                       94, 119, 212, 119, equalizerwin_auto_pushed,
                       SKIN_EQMAIN);
    tbutton_set_toggled(equalizerwin_auto, cfg.equalizer_autoload);
    equalizerwin_presets =
        create_pbutton(&equalizerwin_wlist, equalizerwin_bg,
                       equalizerwin_gc, 217, 18, 44, 12, 224, 164, 224,
                       176, equalizerwin_presets_pushed, SKIN_EQMAIN);
    equalizerwin_close =
        create_pbutton(&equalizerwin_wlist, equalizerwin_bg,
                       equalizerwin_gc, 264, 3, 9, 9, 0, 116, 0, 125,
                       equalizerwin_close_cb, SKIN_EQMAIN);
    equalizerwin_close->pb_allow_draw = FALSE;

    equalizerwin_shade =
        create_pbutton_ex(&equalizerwin_wlist, equalizerwin_bg,
                          equalizerwin_gc, 254, 3, 9, 9, 254, 137, 1, 38,
                          equalizerwin_shade_toggle, NULL, SKIN_EQMAIN, SKIN_EQ_EX);
    equalizerwin_shade->pb_allow_draw = FALSE;

    equalizerwin_graph =
        create_eqgraph(&equalizerwin_wlist, equalizerwin_bg,
                       equalizerwin_gc, 86, 17);
    equalizerwin_preamp =
        create_eqslider(&equalizerwin_wlist, equalizerwin_bg,
                        equalizerwin_gc, 21, 38);
    eqslider_set_position(equalizerwin_preamp, cfg.equalizer_preamp);
    for (i = 0; i < 10; i++) {
        equalizerwin_bands[i] =
            create_eqslider(&equalizerwin_wlist, equalizerwin_bg,
                            equalizerwin_gc, 78 + (i * 18), 38);
        eqslider_set_position(equalizerwin_bands[i], cfg.equalizer_bands[i]);
    }

    equalizerwin_volume =
        create_hslider(&equalizerwin_wlist, equalizerwin_bg,
                       equalizerwin_gc, 61, 4, 97, 8, 1, 30, 1, 30, 3, 7,
                       4, 61, 0, 94, equalizerwin_volume_frame_cb,
                       equalizerwin_volume_motion_cb,
                       equalizerwin_volume_release_cb, SKIN_EQ_EX);
    equalizerwin_balance =
        create_hslider(&equalizerwin_wlist, equalizerwin_bg,
                       equalizerwin_gc, 164, 4, 42, 8, 11, 30, 11, 30, 3,
                       7, 4, 164, 0, 39, equalizerwin_balance_frame_cb,
                       equalizerwin_balance_motion_cb,
                       equalizerwin_balance_release_cb, SKIN_EQ_EX);

    if (!cfg.equalizer_shaded) {
        widget_hide(WIDGET(equalizerwin_volume));
        widget_hide(WIDGET(equalizerwin_balance));
    }
    else {
        pbutton_set_button_data(equalizerwin_shade, -1, 3, -1, 47);
        pbutton_set_skin_index1(equalizerwin_shade, SKIN_EQ_EX);
        pbutton_set_button_data(equalizerwin_close, 11, 38, 11, 47);
        pbutton_set_skin_index(equalizerwin_close, SKIN_EQ_EX);
    }
}


static void
equalizerwin_create_window(void)
{
    GdkPixbuf *icon;
    gint width, height;

    equalizerwin = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(equalizerwin), _("Audacious Equalizer"));
    gtk_window_set_wmclass(GTK_WINDOW(equalizerwin), "equalizer", "Audacious");
    gtk_window_set_role(GTK_WINDOW(equalizerwin), "equalizer");

    width = 275;
    height = cfg.equalizer_shaded ? 14 : 116;

    gtk_window_set_default_size(GTK_WINDOW(equalizerwin), width, height);
    gtk_window_set_resizable(GTK_WINDOW(equalizerwin), FALSE);

    dock_window_list = dock_window_set_decorated(dock_window_list,
                                                 GTK_WINDOW(equalizerwin),
                                                 cfg.show_wm_decorations);

    gtk_window_set_transient_for(GTK_WINDOW(equalizerwin),
                                 GTK_WINDOW(mainwin));
    gtk_window_set_skip_taskbar_hint(GTK_WINDOW(equalizerwin), TRUE);

    icon = gdk_pixbuf_new_from_xpm_data((const gchar **) bmp_eq_icon);
    gtk_window_set_icon(GTK_WINDOW(equalizerwin), icon);
    g_object_unref(icon);

    gtk_widget_set_app_paintable(equalizerwin, TRUE);

    if (cfg.equalizer_x != -1 && cfg.save_window_position)
        gtk_window_move(GTK_WINDOW(equalizerwin),
                        cfg.equalizer_x, cfg.equalizer_y);

    gtk_widget_set_events(equalizerwin,
                          GDK_FOCUS_CHANGE_MASK | GDK_BUTTON_MOTION_MASK |
                          GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK |
                          GDK_VISIBILITY_NOTIFY_MASK);
    gtk_widget_realize(equalizerwin);

    util_set_cursor(equalizerwin);

    g_signal_connect(equalizerwin, "delete_event",
                     G_CALLBACK(equalizerwin_delete), NULL);
    g_signal_connect(equalizerwin, "button_press_event",
                     G_CALLBACK(equalizerwin_press), NULL);
    g_signal_connect(equalizerwin, "button_release_event",
                     G_CALLBACK(equalizerwin_release), NULL);
    g_signal_connect(equalizerwin, "motion_notify_event",
                     G_CALLBACK(equalizerwin_motion), NULL);
    g_signal_connect_after(equalizerwin, "focus_in_event",
                           G_CALLBACK(equalizerwin_focus_in), NULL);
    g_signal_connect_after(equalizerwin, "focus_out_event",
                           G_CALLBACK(equalizerwin_focus_out), NULL);
    g_signal_connect(equalizerwin, "configure_event",
                     G_CALLBACK(equalizerwin_configure), NULL);
    g_signal_connect(equalizerwin, "style_set",
                     G_CALLBACK(equalizerwin_set_back_pixmap), NULL);
    g_signal_connect(equalizerwin, "key_press_event",
                     G_CALLBACK(equalizerwin_keypress), NULL);
    g_signal_connect(equalizerwin, "scroll_event",
                     G_CALLBACK(equalizerwin_scroll), NULL);
}

void
equalizerwin_create_popup_menus(void)
{
    equalizerwin_accel = gtk_accel_group_new();
    gtk_window_add_accel_group(GTK_WINDOW(equalizerwin), equalizerwin_accel);

    equalizerwin_presets_menu = create_menu(equalizerwin_presets_menu_entries,
                                            equalizerwin_presets_menu_entries_num,
                                            NULL);
}

void
equalizerwin_create(void)
{
    equalizer_presets = equalizerwin_read_presets("eq.preset");
    equalizer_auto_presets = equalizerwin_read_presets("eq.auto_preset");

    equalizerwin_create_window();
    equalizerwin_create_popup_menus();

    equalizerwin_gc = gdk_gc_new(equalizerwin->window);
    equalizerwin_bg = gdk_pixmap_new(equalizerwin->window, 275, 116, -1);

    equalizerwin_create_widgets();

    equalizerwin_set_back_pixmap();
    gdk_window_set_back_pixmap(equalizerwin->window, equalizerwin_bg, 0);
}


void
equalizerwin_show(gboolean show)
{
    GtkWidget *item = gtk_item_factory_get_widget(mainwin_view_menu,
                                                  "/Show Equalizer");
    gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(item), show);
}

void
equalizerwin_real_show(void)
{
    /*
     * This function should only be called from the
     * main menu signal handler
     */

    gint x, y;

    gtk_window_get_position(GTK_WINDOW(equalizerwin), &x, &y);
    gtk_window_move(GTK_WINDOW(equalizerwin), x, y);
    gtk_widget_set_size_request(equalizerwin, 275,
                                (cfg.equalizer_shaded ? 14 : 116));
    gdk_flush();
    draw_equalizer_window(TRUE);
    cfg.equalizer_visible = TRUE;
    tbutton_set_toggled(mainwin_eq, TRUE);

    gtk_widget_show(equalizerwin);
}

void
equalizerwin_real_hide(void)
{
    /*
     * This function should only be called from the
     * main menu signal handler
     */
    gtk_widget_hide(equalizerwin);
    cfg.equalizer_visible = FALSE;
    tbutton_set_toggled(mainwin_eq, FALSE);
}

static EqualizerPreset *
equalizerwin_find_preset(GList * list, const gchar * name)
{
    GList *node = list;
    EqualizerPreset *preset;

    while (node) {
        preset = node->data;
        if (!strcasecmp(preset->name, name))
            return preset;
        node = g_list_next(node);
    }
    return NULL;
}

static void
equalizerwin_write_preset_file(GList * list, const gchar * basename)
{
    gchar *filename, *tmp;
    gint i, p;
    EqualizerPreset *preset;
    RcFile *rcfile;
    GList *node;

    rcfile = bmp_rcfile_new();
    p = 0;
    for (node = list; node; node = g_list_next(node)) {
        preset = node->data;
        tmp = g_strdup_printf("Preset%d", p++);
        bmp_rcfile_write_string(rcfile, "Presets", tmp, preset->name);
        g_free(tmp);
        bmp_rcfile_write_float(rcfile, preset->name, "Preamp",
                               preset->preamp);
        for (i = 0; i < 10; i++) {
            tmp = g_strdup_printf("Band%d\n", i);
            bmp_rcfile_write_float(rcfile, preset->name, tmp,
                                   preset->bands[i]);
            g_free(tmp);
        }
    }

    filename = g_build_filename(bmp_paths[BMP_PATH_USER_DIR], basename, NULL);
    bmp_rcfile_write(rcfile, filename);
    bmp_rcfile_free(rcfile);
    g_free(filename);
}

static gboolean
equalizerwin_load_preset(GList * list, const gchar * name)
{
    EqualizerPreset *preset;
    gint i;

    if ((preset = equalizerwin_find_preset(list, name)) != NULL) {
        eqslider_set_position(equalizerwin_preamp, preset->preamp);
        for (i = 0; i < 10; i++)
            eqslider_set_position(equalizerwin_bands[i], preset->bands[i]);
        equalizerwin_eq_changed();
        return TRUE;
    }
    return FALSE;
}

static GList *
equalizerwin_save_preset(GList * list, const gchar * name,
                         const gchar * filename)
{
    gint i;
    EqualizerPreset *preset;

    if (!(preset = equalizerwin_find_preset(list, name))) {
        preset = g_new0(EqualizerPreset, 1);
        preset->name = g_strdup(name);
        list = g_list_append(list, preset);
    }

    preset->preamp = eqslider_get_position(equalizerwin_preamp);
    for (i = 0; i < 10; i++)
        preset->bands[i] = eqslider_get_position(equalizerwin_bands[i]);

    equalizerwin_write_preset_file(list, filename);

    return list;
}

static GList *
equalizerwin_delete_preset(GList * list, gchar * name, gchar * filename)
{
    EqualizerPreset *preset;
    GList *node;

    if (!(preset = equalizerwin_find_preset(list, name)))
        return list;

    if (!(node = g_list_find(list, preset)))
        return list;

    list = g_list_remove_link(list, node);
    equalizer_preset_free(preset);
    g_list_free_1(node);

    equalizerwin_write_preset_file(list, filename);

    return list;
}


static GList *
import_winamp_eqf(VFSFile * file)
{
    gchar header[31];
    gchar name[257];
    gchar bands[11];
    gint i = 0;
    GList *list = NULL;
    EqualizerPreset *preset;

    vfs_fread(header, 1, 31, file);
    if (!strncmp(header, "Winamp EQ library file v1.1", 27)) {
        while (vfs_fread(name, 1, 257, file)) {
            preset = equalizer_preset_new(name);
            preset->preamp = 20.0 - ((bands[10] * 40.0) / 64);

            vfs_fread(bands, 1, 11, file);

            for (i = 0; i < 10; i++)
                preset->bands[i] = 20.0 - ((bands[i] * 40.0) / 64);

            list = g_list_prepend(list, preset);
        }
    }

    list = g_list_reverse(list);
    return list;
}

static void
equalizerwin_read_winamp_eqf(VFSFile * file)
{
    gchar header[31];
    guchar bands[11];
    gint i;

    vfs_fread(header, 1, 31, file);

    if (!strncmp(header, "Winamp EQ library file v1.1", 27)) {
        /* Skip name */
        if (vfs_fseek(file, 257, SEEK_CUR) == -1)
            return;

        if (vfs_fread(bands, 1, 11, file) != 11)
            return;

        eqslider_set_position(equalizerwin_preamp,
                              20.0 - ((bands[10] * 40.0) / 63.0));

        for (i = 0; i < 10; i++)
            eqslider_set_position(equalizerwin_bands[i],
                                  20.0 - ((bands[i] * 40.0) / 64.0));
    }

    equalizerwin_eq_changed();
}

static void
equalizerwin_read_bmp_preset(RcFile * rcfile)
{
    gfloat val;
    gint i;

    if (bmp_rcfile_read_float(rcfile, "Equalizer preset", "Preamp", &val))
        eqslider_set_position(equalizerwin_preamp, val);
    for (i = 0; i < 10; i++) {
        gchar tmp[7];
        g_snprintf(tmp, sizeof(tmp), "Band%d", i);
        if (bmp_rcfile_read_float(rcfile, "Equalizer preset", tmp, &val))
            eqslider_set_position(equalizerwin_bands[i], val);
    }
    equalizerwin_eq_changed();
}

static void
equalizerwin_save_ok(GtkWidget * widget, gpointer data)
{
    const gchar *text;

    text = gtk_entry_get_text(GTK_ENTRY(equalizerwin_save_entry));
    if (strlen(text) != 0)
        equalizer_presets =
            equalizerwin_save_preset(equalizer_presets, text, "eq.preset");
    gtk_widget_destroy(equalizerwin_save_window);
}

static void
equalizerwin_save_select(GtkCList * clist, gint row,
                         gint column, GdkEventButton * event, gpointer data)
{
    gchar *text;

    gtk_clist_get_text(clist, row, 0, &text);

    gtk_entry_set_text(GTK_ENTRY(equalizerwin_save_entry), text);
    if (event->type == GDK_2BUTTON_PRESS)
        equalizerwin_save_ok(NULL, NULL);

}

static void
equalizerwin_load_ok(GtkWidget * widget, gpointer data)
{
    gchar *text;
    GtkCList *clist = GTK_CLIST(data);

    if (clist && clist->selection) {
        gtk_clist_get_text(clist, GPOINTER_TO_INT(clist->selection->data),
                           0, &text);
        equalizerwin_load_preset(equalizer_presets, text);
    }
    gtk_widget_destroy(equalizerwin_load_window);
}

static void
equalizerwin_load_apply(GtkWidget * widget, gpointer data)
{
    gchar *text;
    GtkCList *clist = GTK_CLIST(data);

    if (clist && clist->selection) {
        gtk_clist_get_text(clist, GPOINTER_TO_INT(clist->selection->data),
                           0, &text);
        equalizerwin_load_preset(equalizer_presets, text);
    }
}


static void
equalizerwin_load_select(GtkCList * widget, gint row,
                         gint column, GdkEventButton * event, gpointer data)
{
    if (event->type == GDK_2BUTTON_PRESS)
        equalizerwin_load_ok(NULL, widget);
}

static void
equalizerwin_delete_delete(GtkWidget * widget, gpointer data)
{
    gchar *text;
    GList *list, *next;
    GtkCList *clist = GTK_CLIST(data);

    g_return_if_fail(clist != NULL);

    list = clist->selection;
    gtk_clist_freeze(clist);
    while (list) {
        next = g_list_next(list);
        gtk_clist_get_text(clist, GPOINTER_TO_INT(list->data), 0, &text);
        equalizer_auto_presets =
            equalizerwin_delete_preset(equalizer_presets, text, "eq.preset");
        gtk_clist_remove(clist, GPOINTER_TO_INT(list->data));
        list = next;
    }
    gtk_clist_thaw(clist);
}

static void
equalizerwin_save_auto_ok(GtkWidget * widget, gpointer data)
{
    const gchar *text;

    text = gtk_entry_get_text(GTK_ENTRY(equalizerwin_save_auto_entry));
    if (strlen(text) != 0)
        equalizer_auto_presets =
            equalizerwin_save_preset(equalizer_auto_presets, text,
                                     "eq.auto_preset");
    gtk_widget_destroy(equalizerwin_save_auto_window);
}

static void
equalizerwin_save_auto_select(GtkCList * clist, gint row,
                              gint column,
                              GdkEventButton * event, gpointer data)
{
    gchar *text;

    gtk_clist_get_text(clist, row, 0, &text);

    gtk_entry_set_text(GTK_ENTRY(equalizerwin_save_auto_entry), text);
    if (event->type == GDK_2BUTTON_PRESS)
        equalizerwin_save_auto_ok(NULL, NULL);

}

static void
equalizerwin_load_auto_ok(GtkWidget * widget, gpointer data)
{
    gchar *text;
    GtkCList *clist = GTK_CLIST(data);

    if (clist && clist->selection) {
        gtk_clist_get_text(clist, GPOINTER_TO_INT(clist->selection->data),
                           0, &text);
        equalizerwin_load_preset(equalizer_auto_presets, text);
    }
    gtk_widget_destroy(equalizerwin_load_auto_window);
}

static void
equalizerwin_load_auto_select(GtkWidget * widget, gint row,
                              gint column,
                              GdkEventButton * event, gpointer data)
{
    if (event->type == GDK_2BUTTON_PRESS)
        equalizerwin_load_auto_ok(NULL, widget);
}

static void
equalizerwin_delete_auto_delete(GtkWidget * widget, gpointer data)
{
    gchar *text;
    GList *list, *next;
    GtkCList *clist = GTK_CLIST(data);

    g_return_if_fail(clist != NULL);

    list = clist->selection;
    gtk_clist_freeze(clist);
    while (list) {
        next = g_list_next(list);
        gtk_clist_get_text(clist, GPOINTER_TO_INT(list->data), 0, &text);
        equalizer_auto_presets =
            equalizerwin_delete_preset(equalizer_auto_presets, text,
                                       "eq.auto_preset");
        gtk_clist_remove(clist, GPOINTER_TO_INT(list->data));
        list = next;
    }
    gtk_clist_thaw(clist);
}


typedef void (*ResponseHandler)(const gchar *filename);

static void
equalizerwin_file_chooser_on_response(GtkWidget * dialog,
                                      gint response,
                                      gpointer data)
{
    GtkFileChooser *file_chooser = GTK_FILE_CHOOSER(dialog);
    ResponseHandler handler = (ResponseHandler) data;
    gchar *filename;

    gtk_widget_hide(dialog);

    switch (response)
    {
    case GTK_RESPONSE_ACCEPT:
        filename = gtk_file_chooser_get_filename(file_chooser);
        handler(filename);
        g_free(filename);
        break;

    case GTK_RESPONSE_REJECT:
        break;
    }

    gtk_widget_destroy(dialog);
}
                                     


static void
load_preset_file(const gchar *filename)
{
    RcFile *rcfile;

    if ((rcfile = bmp_rcfile_open(filename)) != NULL) {
        equalizerwin_read_bmp_preset(rcfile);
        bmp_rcfile_free(rcfile);
    }
}

static void
load_winamp_file(const gchar * filename)
{
    VFSFile *file;
    gchar *tmp;

    if (!(file = vfs_fopen(filename, "rb"))) {
        tmp = g_strconcat("Failed to load WinAmp file: ",filename,"\n",NULL);
        report_error(tmp);
        g_free(tmp);
        return;
    }

    equalizerwin_read_winamp_eqf(file);
    vfs_fclose(file);
}

static void
import_winamp_file(const gchar * filename)
{
    VFSFile *file;
    gchar *tmp;

    if (!(file = vfs_fopen(filename, "rb"))) {
        tmp = g_strconcat("Failed to import WinAmp file: ",filename,"\n",NULL);
        report_error(tmp);
        g_free(tmp);
        return;
    }

    equalizer_presets = g_list_concat(equalizer_presets,
                                      import_winamp_eqf(file));
    equalizerwin_write_preset_file(equalizer_presets, "eq.preset");

    vfs_fclose(file);
}

static void
save_preset_file(const gchar * filename)
{
    RcFile *rcfile;
    gint i;

    rcfile = bmp_rcfile_new();
    bmp_rcfile_write_float(rcfile, "Equalizer preset", "Preamp",
                           eqslider_get_position(equalizerwin_preamp));

    for (i = 0; i < 10; i++) {
        gchar tmp[7];
        g_snprintf(tmp, sizeof(tmp), "Band%d", i);
        bmp_rcfile_write_float(rcfile, "Equalizer preset", tmp,
                               eqslider_get_position(equalizerwin_bands[i]));
    }

    bmp_rcfile_write(rcfile, filename);
    bmp_rcfile_free(rcfile);
}

static void
save_winamp_file(const gchar * filename)
{
    VFSFile *file;

    gchar name[257];
    gint i;
    guchar bands[11];
    gchar *tmp;

    if (!(file = vfs_fopen(filename, "wb"))) {
        tmp = g_strconcat("Failed to save WinAmp file: ",filename,"\n",NULL);
        report_error(tmp);
        g_free(tmp);
        return;
    }

    vfs_fwrite("Winamp EQ library file v1.1\x1a!--", 1, 31, file);

    memset(name, 0, 257);
    strcpy(name, "Entry1");
    vfs_fwrite(name, 1, 257, file);

    for (i = 0; i < 10; i++)
        bands[i] = 63 - (((eqslider_get_position(equalizerwin_bands[i]) + 20) * 63) / 40);
    bands[10] = 63 - (((eqslider_get_position(equalizerwin_preamp) + 20) * 63) / 40);
    vfs_fwrite(bands, 1, 11, file);

    vfs_fclose(file);
}

static gint
equalizerwin_list_sort_func(GtkCList * clist,
                            gconstpointer ptr1, gconstpointer ptr2)
{
    GtkCListRow *row1 = (GtkCListRow *) ptr1;
    GtkCListRow *row2 = (GtkCListRow *) ptr2;

    return strcasecmp(GTK_CELL_TEXT(row1->cell[clist->sort_column])->text,
                      GTK_CELL_TEXT(row2->cell[clist->sort_column])->text);
}


static GtkListStore *
preset_list_store_new(GList * preset)
{
    GtkListStore *store;
    GtkTreeIter iter;
    GList *node;

    store = gtk_list_store_new(PRESET_VIEW_N_COLS, G_TYPE_STRING);

    for (node = preset; node; node = g_list_next(node)) {
        gtk_list_store_append(store, &iter);
        gtk_list_store_set(store, &iter,
                           PRESET_VIEW_COL_NAME,
                           &((EqualizerPreset *) node->data)->name, -1);
    }

    return store;
}


GtkWidget *
preset_view_new(GList * preset)
{
    GtkWidget *treeview;
    GtkTreeModel *model;
    GtkTreeViewColumn *column;
    GtkCellRenderer *renderer;
    GtkListStore *store;

    store = preset_list_store_new(preset);

    model = gtk_tree_model_sort_new_with_model(GTK_TREE_MODEL(store));
    gtk_tree_sortable_set_sort_column_id(GTK_TREE_SORTABLE(model),
                                         PRESET_VIEW_COL_NAME,
                                         GTK_SORT_ASCENDING);

    treeview = gtk_tree_view_new_with_model(model);
    gtk_tree_view_set_headers_clickable(GTK_TREE_VIEW(treeview), TRUE);
    gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(treeview), TRUE);

    renderer = gtk_cell_renderer_text_new();
    column = gtk_tree_view_column_new_with_attributes(_("Preset"), renderer,
                                                      "text",
                                                      PRESET_VIEW_COL_NAME,
                                                      NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(treeview), column);

    return treeview;
}


static GtkWidget *
equalizerwin_create_list_window(GList * preset_list,
                                const gchar * title,
                                GtkWidget ** window,
                                GtkSelectionMode sel_mode,
                                GtkWidget ** entry,
                                const gchar * btn2_stock_name,
				const gchar * btn3_stock_name,
                                const gchar * btn1_stock_name,
                                GCallback btn2_func,
				GCallback btn3_func,
                                GCallback select_row_func)
{
    GtkWidget *vbox, *scrolled_window, *bbox, *btn1, *btn2, *btn3, *clist;
    gchar *preset_text[1];
    GList *node;

    *window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(*window), title);
    gtk_window_set_type_hint(GTK_WINDOW(*window), GDK_WINDOW_TYPE_HINT_DIALOG);
    gtk_window_set_default_size(GTK_WINDOW(*window), 350, 300);
    gtk_window_set_position(GTK_WINDOW(*window), GTK_WIN_POS_CENTER);
    gtk_container_set_border_width(GTK_CONTAINER(*window), 10);
    gtk_window_set_transient_for(GTK_WINDOW(*window),
                                 GTK_WINDOW(equalizerwin));
    g_signal_connect(*window, "destroy",
                     G_CALLBACK(gtk_widget_destroyed), window);

    vbox = gtk_vbox_new(FALSE, 10);
    gtk_container_add(GTK_CONTAINER(*window), vbox);

    scrolled_window = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled_window),
                                   GTK_POLICY_AUTOMATIC, GTK_POLICY_ALWAYS);

    preset_text[0] = _("Presets");
    clist = gtk_clist_new_with_titles(1, preset_text);
    if (select_row_func)
        g_signal_connect(clist, "select_row",
                         G_CALLBACK(select_row_func), NULL);
    gtk_clist_column_titles_passive(GTK_CLIST(clist));
    gtk_clist_set_selection_mode(GTK_CLIST(clist), sel_mode);

    for (node = preset_list; node; node = g_list_next(node)) {
        gtk_clist_append(GTK_CLIST(clist),
                         &((EqualizerPreset *) node->data)->name);
    }
    gtk_clist_set_compare_func(GTK_CLIST(clist), equalizerwin_list_sort_func);
    gtk_clist_sort(GTK_CLIST(clist));

    gtk_container_add(GTK_CONTAINER(scrolled_window), clist);
    gtk_box_pack_start(GTK_BOX(vbox), scrolled_window, TRUE, TRUE, 0);

    if (entry) {
        *entry = gtk_entry_new();
        g_signal_connect(*entry, "activate", btn2_func, NULL);
        gtk_box_pack_start(GTK_BOX(vbox), *entry, FALSE, FALSE, 0);
    }

    bbox = gtk_hbutton_box_new();
    gtk_button_box_set_layout(GTK_BUTTON_BOX(bbox), GTK_BUTTONBOX_END);
    gtk_button_box_set_spacing(GTK_BUTTON_BOX(bbox), 5);
    gtk_box_pack_start(GTK_BOX(vbox), bbox, FALSE, FALSE, 0);

    btn1 = gtk_button_new_from_stock(btn1_stock_name);
    g_signal_connect_swapped(btn1, "clicked",
                             G_CALLBACK(gtk_widget_destroy),
                             GTK_OBJECT(*window));
    gtk_box_pack_start(GTK_BOX(bbox), btn1, TRUE, TRUE, 0);

    if (btn3_stock_name) {
        btn3 = gtk_button_new_from_stock(btn3_stock_name);
        g_signal_connect(btn3, "clicked", G_CALLBACK(btn3_func), clist);
        gtk_box_pack_start(GTK_BOX(bbox), btn3, TRUE, TRUE, 0);
    }

    btn2 = gtk_button_new_from_stock(btn2_stock_name);
    g_signal_connect(btn2, "clicked", G_CALLBACK(btn2_func), clist);
    GTK_WIDGET_SET_FLAGS(btn2, GTK_CAN_DEFAULT);
        
    gtk_box_pack_start(GTK_BOX(bbox), btn2, TRUE, TRUE, 0);

    gtk_widget_grab_default(btn2);


    gtk_widget_show_all(*window);

    return *window;
}

void
equalizerwin_presets_menu_cb(gpointer cb_data, guint action, GtkWidget * w)
{
    GtkWidget *dialog;

    switch (action) {
    case EQUALIZER_PRESETS_LOAD_PRESET:
        if (!equalizerwin_load_window)
            equalizerwin_create_list_window(equalizer_presets,
                                            _("Load preset"),
                                            &equalizerwin_load_window,
                                            GTK_SELECTION_SINGLE, NULL,
                                            GTK_STOCK_OK, GTK_STOCK_APPLY, GTK_STOCK_CANCEL,
                                            G_CALLBACK
                                            (equalizerwin_load_ok),
					    G_CALLBACK
					    (equalizerwin_load_apply),
                                            G_CALLBACK
                                            (equalizerwin_load_select));
        else
            gtk_window_present(GTK_WINDOW(equalizerwin_load_window));
        break;
    case EQUALIZER_PRESETS_LOAD_AUTOPRESET:
        if (!equalizerwin_load_auto_window)
            equalizerwin_create_list_window(equalizer_auto_presets,
                                            _("Load auto-preset"),
                                            &equalizerwin_load_auto_window,
                                            GTK_SELECTION_SINGLE, NULL,
                                            GTK_STOCK_OK, NULL, GTK_STOCK_CANCEL,
                                            G_CALLBACK
                                            (equalizerwin_load_auto_ok),
					    NULL,
                                            G_CALLBACK
                                            (equalizerwin_load_auto_select));
        else
            gtk_window_present(GTK_WINDOW(equalizerwin_load_auto_window));
        break;
    case EQUALIZER_PRESETS_LOAD_DEFAULT:
        equalizerwin_load_preset(equalizer_presets, "Default");
        break;
    case EQUALIZER_PRESETS_LOAD_ZERO:
        {
            gint i;

            eqslider_set_position(equalizerwin_preamp, 0);
            for (i = 0; i < 10; i++)
                eqslider_set_position(equalizerwin_bands[i], 0);
            equalizerwin_eq_changed();
            break;
        }
    case EQUALIZER_PRESETS_LOAD_FROM_FILE:
        dialog = make_filebrowser(_("Load equalizer preset"), FALSE);
        g_signal_connect(dialog , "response",
                         G_CALLBACK(equalizerwin_file_chooser_on_response),
                         load_preset_file);
        break;
    case EQUALIZER_PRESETS_LOAD_FROM_WINAMPFILE:
        dialog = make_filebrowser(_("Load equalizer preset"), FALSE);
        g_signal_connect(dialog, "response",
                         G_CALLBACK(equalizerwin_file_chooser_on_response),
                         load_winamp_file);
        break;

    case EQUALIZER_PRESETS_IMPORT_WINAMPFILE:
        dialog = make_filebrowser(_("Load equalizer preset"), FALSE);
        g_signal_connect(dialog, "response",
                         G_CALLBACK(equalizerwin_file_chooser_on_response),
                         import_winamp_file);
        break;

    case EQUALIZER_PRESETS_SAVE_PRESET:
        if (!equalizerwin_save_window)
            equalizerwin_create_list_window(equalizer_presets,
                                            _("Save preset"),
                                            &equalizerwin_save_window,
                                            GTK_SELECTION_SINGLE,
                                            &equalizerwin_save_entry,
                                            GTK_STOCK_OK, NULL, GTK_STOCK_CANCEL, 
                                            G_CALLBACK
                                            (equalizerwin_save_ok),
					    NULL,
                                            G_CALLBACK
                                            (equalizerwin_save_select));
        else
            gtk_window_present(GTK_WINDOW(equalizerwin_save_window));
        break;

    case EQUALIZER_PRESETS_SAVE_AUTOPRESET:
        {
            gchar *name;

            if (!equalizerwin_save_auto_window)
                equalizerwin_create_list_window(equalizer_auto_presets,
                                                _("Save auto-preset"),
                                                &equalizerwin_save_auto_window,
                                                GTK_SELECTION_SINGLE,
                                                &equalizerwin_save_auto_entry,
                                                GTK_STOCK_OK,
						NULL,
                                                GTK_STOCK_CANCEL,
                                                G_CALLBACK
                                                (equalizerwin_save_auto_ok),
						NULL,
                                                G_CALLBACK
                                                (equalizerwin_save_auto_select));
            else
                gtk_window_present(GTK_WINDOW(equalizerwin_save_auto_window));

            name = playlist_get_filename(playlist_get_position());

            if (name) {
                gtk_entry_set_text(GTK_ENTRY(equalizerwin_save_auto_entry),
                                   g_basename(name));
                g_free(name);
            }
            break;
        }

    case EQUALIZER_PRESETS_SAVE_DEFAULT:
        equalizer_presets =
            equalizerwin_save_preset(equalizer_presets, "Default",
                                     "eq.preset");
        break;

    case EQUALIZER_PRESETS_SAVE_TO_FILE:
        {
            gchar *songname;

            dialog = make_filebrowser(_("Save equalizer preset"), TRUE);
            g_signal_connect(dialog, "response",
                             G_CALLBACK(equalizerwin_file_chooser_on_response),
                             save_preset_file);

            songname = playlist_get_filename(playlist_get_position());
            if (songname) {
                gchar *eqname = g_strdup_printf("%s.%s", songname,
                                                cfg.eqpreset_extension);
                g_free(songname);
                gtk_file_chooser_set_filename(GTK_FILE_CHOOSER(dialog),
                                              eqname);
                g_free(eqname);
            }

            break;
        }

    case EQUALIZER_PRESETS_SAVE_TO_WINAMPFILE:
        dialog = make_filebrowser(_("Save equalizer preset"), TRUE);
        g_signal_connect(dialog, "response",
                         G_CALLBACK(equalizerwin_file_chooser_on_response),
                         save_winamp_file);
        break;

    case EQUALIZER_PRESETS_DELETE_PRESET:
        if (!equalizerwin_delete_window)
            equalizerwin_create_list_window(equalizer_presets,
                                            _("Delete preset"),
                                            &equalizerwin_delete_window,
                                            GTK_SELECTION_EXTENDED, NULL,
                                            GTK_STOCK_DELETE,
					    NULL,
                                            GTK_STOCK_CLOSE,
                                            G_CALLBACK
                                            (equalizerwin_delete_delete),
					    NULL,
                                            NULL);
        else
            gtk_window_present(GTK_WINDOW(equalizerwin_delete_window));

        break;

    case EQUALIZER_PRESETS_DELETE_AUTOPRESET:
        if (!equalizerwin_delete_auto_window)
            equalizerwin_create_list_window(equalizer_auto_presets,
                                            _("Delete auto-preset"),
                                            &equalizerwin_delete_auto_window,
                                            GTK_SELECTION_EXTENDED, NULL,
                                            GTK_STOCK_DELETE,
					    NULL,
                                            GTK_STOCK_CLOSE,
                                            G_CALLBACK
                                            (equalizerwin_delete_auto_delete),
					    NULL,
                                            NULL);
        else
            gtk_window_present(GTK_WINDOW(equalizerwin_delete_auto_window));
        
        break;
    }
}

void
equalizerwin_load_auto_preset(const gchar * filename)
{
    gchar *presetfilename, *directory;
    RcFile *rcfile;

    g_return_if_fail(filename != NULL);

    if (!cfg.equalizer_autoload)
        return;

    presetfilename = g_strconcat(filename, ".", cfg.eqpreset_extension, NULL);

    /* First try to find a per file preset file */
    if (strlen(cfg.eqpreset_extension) > 0 &&
        (rcfile = bmp_rcfile_open(presetfilename)) != NULL) {
        g_free(presetfilename);
        equalizerwin_read_bmp_preset(rcfile);
        bmp_rcfile_free(rcfile);
        return;
    }

    g_free(presetfilename);

    directory = g_path_get_dirname(filename);
    presetfilename = g_build_filename(directory, cfg.eqpreset_default_file,
                                      NULL);
    g_free(directory);

    /* Try to find a per directory preset file */
    if (strlen(cfg.eqpreset_default_file) > 0 &&
        (rcfile = bmp_rcfile_open(presetfilename)) != NULL) {
        equalizerwin_read_bmp_preset(rcfile);
        bmp_rcfile_free(rcfile);
    }
    else if (!equalizerwin_load_preset
             (equalizer_auto_presets, g_basename(filename))) {
        /* Fall back to the oldstyle auto presets */
        equalizerwin_load_preset(equalizer_presets, "Default");
    }

    g_free(presetfilename);
}

void
equalizerwin_set_preamp(gfloat preamp)
{
    eqslider_set_position(equalizerwin_preamp, preamp);
    equalizerwin_eq_changed();
}

void
equalizerwin_set_band(gint band, gfloat value)
{
    g_return_if_fail(band >= 0 && band < 10);
    eqslider_set_position(equalizerwin_bands[band], value);
}

gfloat
equalizerwin_get_preamp(void)
{
    return eqslider_get_position(equalizerwin_preamp);
}

gfloat
equalizerwin_get_band(gint band)
{
    g_return_val_if_fail(band >= 0 && band < 10, 0);
    return eqslider_get_position(equalizerwin_bands[band]);
}
