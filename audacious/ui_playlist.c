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

#include "ui_playlist.h"

#include <glib.h>
#include <glib/gi18n.h>
#include <gdk/gdk.h>
#include <gdk/gdkkeysyms.h>
#include <gtk/gtk.h>
#include <string.h>

#include "platform/smartinclude.h"

#include <unistd.h>
#include <errno.h>

#include "libaudacious/util.h"

#include "dnd.h"
#include "dock.h"
#include "equalizer.h"
#include "hints.h"
#include "input.h"
#include "main.h"
#include "mainwin.h"
#include "playback.h"
#include "playlist.h"
#include "playlist_container.h"
#include "util.h"

#include "pixmaps.h"
#include "images/audacious_playlist.xpm"


#define ITEM_SEPARATOR {"/-", NULL, NULL, 0, "<Separator>", NULL}


enum {
    ADD_URL, ADD_DIR, ADD_FILES,
    SUB_MISC, SUB_ALL, SUB_CROP, SUB_SELECTED,
    SUB_DUPLICATE_BYTITLE, SUB_DUPLICATE_BYFILENAME, SUB_DUPLICATE_BYPATH,
    SEL_INV, SEL_ZERO, SEL_ALL,
    MISC_SORT, MISC_FILEINFO, MISC_MISCOPTS,
    PLIST_NEW, PLIST_SAVE_AS, PLIST_LOAD,
    SEL_LOOKUP, CLOSE_PL_WINDOW, MOVE_UP, PLIST_SAVE,
    MISC_QUEUE, PLIST_CQUEUE, PLIST_JTF, PLIST_JTT,
    PLAYLISTWIN_REMOVE_DEAD_FILES,
    PLAYLISTWIN_REFRESH, PLIST_DEFAULTSAVE, MISC_FILEPOPUP
};

enum {
    PLAYLISTWIN_SORT_BYTITLE, PLAYLISTWIN_SORT_BYFILENAME,
    PLAYLISTWIN_SORT_BYPATH, PLAYLISTWIN_SORT_BYDATE,
    PLAYLISTWIN_SORT_BYARTIST, PLAYLISTWIN_SORT_SEL_BYARTIST,
    PLAYLISTWIN_SORT_SEL_BYTITLE, PLAYLISTWIN_SORT_SEL_BYFILENAME,
    PLAYLISTWIN_SORT_SEL_BYPATH, PLAYLISTWIN_SORT_SEL_BYDATE,
    PLAYLISTWIN_SORT_RANDOMIZE, PLAYLISTWIN_SORT_REVERSE,
    PLAYLISTWIN_SORT_BYTRACK, PLAYLISTWIN_SORT_SEL_BYTRACK,
    PLAYLISTWIN_SORT_BYPLAYLIST, PLAYLISTWIN_SORT_SEL_BYPLAYLIST
};

GtkWidget *playlistwin;

PlayList_List *playlistwin_list = NULL;
PButton *playlistwin_shade, *playlistwin_close;

static gboolean playlistwin_resizing = FALSE;

static GtkItemFactory *playlistwin_popup_menu;
static GtkItemFactory *pladd_menu, *pldel_menu;
static GtkItemFactory *plsel_menu, *plsort_menu;
static GtkItemFactory *pllist_menu;

static GdkPixmap *playlistwin_bg;
static GdkBitmap *playlistwin_mask = NULL;
static GdkGC *playlistwin_gc;

static GtkAccelGroup *playlistwin_accel;

static gboolean playlistwin_hint_flag = FALSE;

static PlaylistSlider *playlistwin_slider = NULL;
static TextBox *playlistwin_time_min, *playlistwin_time_sec;
static TextBox *playlistwin_info, *playlistwin_sinfo;
static SButton *playlistwin_srew, *playlistwin_splay;
static SButton *playlistwin_spause, *playlistwin_sstop;
static SButton *playlistwin_sfwd, *playlistwin_seject;
static SButton *playlistwin_sscroll_up, *playlistwin_sscroll_down;

static GList *playlistwin_wlist = NULL;

static void plsort_menu_callback(gpointer cb_data, guint action,
                                           GtkWidget * w);
static void playlistwin_sub_menu_callback(gpointer cb_data, guint action,
                                          GtkWidget * w);
static void playlistwin_popup_menu_callback(gpointer cb_data, guint action,
                                            GtkWidget * w);

static GtkItemFactoryEntry playlistwin_popup_menu_entries[] = {
    {N_("/View Track Details"), NULL,
     playlistwin_popup_menu_callback,
     MISC_FILEINFO, "<ImageItem>", my_pixbuf},

    {N_("/Show Popup Info"), NULL,
     playlistwin_popup_menu_callback,
     MISC_FILEPOPUP, "<ToggleItem>", NULL},

    ITEM_SEPARATOR,

    {N_("/Remove Selected"), "Delete",
     playlistwin_sub_menu_callback,
     SUB_SELECTED, "<StockItem>", GTK_STOCK_REMOVE},

    {N_("/Remove Unselected"), NULL,
     playlistwin_sub_menu_callback,
     SUB_CROP, "<StockItem>", GTK_STOCK_REMOVE},

    {N_("/Remove All"), NULL,
     playlistwin_sub_menu_callback,
     SUB_ALL, "<StockItem>", GTK_STOCK_CLEAR},

    ITEM_SEPARATOR,

    {N_("/Queue Toggle"), "q",
     playlistwin_popup_menu_callback,
     MISC_QUEUE, "<ImageItem>", queuetoggle_pixbuf},
};

static GtkItemFactoryEntry pladd_menu_entries[] = {
    {N_("/Add CD..."), "<shift>c",
     mainwin_general_menu_callback,
     MAINWIN_GENERAL_ADDCD, "<StockItem>", GTK_STOCK_CDROM},

    {N_("/Add Internet Address..."), "<control>h",
     mainwin_general_menu_callback,
     MAINWIN_GENERAL_PLAYLOCATION, "<StockItem>", GTK_STOCK_NETWORK},

    {N_("/Add Files..."), "f",
     mainwin_general_menu_callback,
     MAINWIN_GENERAL_PLAYFILE, "<StockItem>", GTK_STOCK_ADD},
};

static GtkItemFactoryEntry pldel_menu_entries[] = {
    {N_("/Clear Queue"), "<shift>Q",
     playlistwin_popup_menu_callback,
     PLIST_CQUEUE, "<StockItem>", GTK_STOCK_CANCEL},

    ITEM_SEPARATOR,

    {N_("/Remove Unavailable Files"), NULL,
     playlistwin_sub_menu_callback,
     PLAYLISTWIN_REMOVE_DEAD_FILES, "<ImageItem>", removeunavail_pixbuf},

    {N_("/Remove Duplicates"), NULL, NULL, 0, "<Branch>", NULL},
    {N_("/Remove Duplicates/By Title"), NULL,
     playlistwin_sub_menu_callback,
     SUB_DUPLICATE_BYTITLE, "<ImageItem>", removedups_pixbuf},
    {N_("/Remove Duplicates/By Filename"), NULL,
     playlistwin_sub_menu_callback,
     SUB_DUPLICATE_BYFILENAME, "<ImageItem>", removedups_pixbuf},
    {N_("/Remove Duplicates/By Path + Filename"), NULL,
     playlistwin_sub_menu_callback,
     SUB_DUPLICATE_BYPATH, "<ImageItem>", removedups_pixbuf},

    ITEM_SEPARATOR,

    {N_("/Remove All"), NULL,
     playlistwin_sub_menu_callback,
     SUB_ALL, "<StockItem>", GTK_STOCK_CLEAR},

    {N_("/Remove Unselected"), NULL,
     playlistwin_sub_menu_callback,
     SUB_CROP, "<StockItem>", GTK_STOCK_REMOVE},

    {N_("/Remove Selected"), "Delete",
     playlistwin_sub_menu_callback,
     SUB_SELECTED, "<StockItem>", GTK_STOCK_REMOVE}
};

static GtkItemFactoryEntry pllist_menu_entries[] = {
    {N_("/New List"), "<shift>N",
     playlistwin_sub_menu_callback,
     PLIST_NEW, "<StockItem>", GTK_STOCK_NEW},

    ITEM_SEPARATOR,

    {N_("/Load List"), "o",
     playlistwin_sub_menu_callback,
     PLIST_LOAD, "<StockItem>", GTK_STOCK_OPEN},

    {N_("/Save List"), "<shift>S",
     playlistwin_sub_menu_callback,
     PLIST_SAVE, "<StockItem>", GTK_STOCK_SAVE},

    {N_("/Save Default List"), "<alt>S",
     playlistwin_sub_menu_callback,
     PLIST_DEFAULTSAVE, "<StockItem>", GTK_STOCK_SAVE},

    ITEM_SEPARATOR,

    {N_("/Update View"), "F5",
     playlistwin_sub_menu_callback,
     PLAYLISTWIN_REFRESH, "<StockItem>", GTK_STOCK_REFRESH}
};

static GtkItemFactoryEntry plsel_menu_entries[] = {
    {N_("/Invert Selection"), NULL,
     playlistwin_sub_menu_callback,
     SEL_INV, "<ImageItem>", selectinvert_pixbuf},
    
    ITEM_SEPARATOR,

    {N_("/Select None"),"<Ctrl><Shift>A",
     playlistwin_sub_menu_callback,
     SEL_ZERO, "<ImageItem>", selectnone_pixbuf},

    {N_("/Select All"), "<Ctrl>A",
     playlistwin_sub_menu_callback,
     SEL_ALL, "<ImageItem>", selectall_pixbuf},
};

static GtkItemFactoryEntry plsort_menu_entries[] = {
    {N_("/Randomize List"), NULL, plsort_menu_callback,
     PLAYLISTWIN_SORT_RANDOMIZE, "<ImageItem>", randomizepl_pixbuf},
    {N_("/Reverse List"), NULL, plsort_menu_callback,
     PLAYLISTWIN_SORT_REVERSE, "<ImageItem>", invertpl_pixbuf},
    ITEM_SEPARATOR,
    {N_("/Sort List"), NULL, NULL, 0, "<Branch>", NULL},
    {N_("/Sort List/By Title"), NULL, plsort_menu_callback,
     PLAYLISTWIN_SORT_BYTITLE, "<ImageItem>", sortbytitle_pixbuf},
    {N_("/Sort List/By Artist"), NULL, plsort_menu_callback,
     PLAYLISTWIN_SORT_BYARTIST, "<ImageItem>", sortbytitle_pixbuf},
    {N_("/Sort List/By Filename"), NULL, plsort_menu_callback,
     PLAYLISTWIN_SORT_BYFILENAME, "<ImageItem>", sortbyfilename_pixbuf},
    {N_("/Sort List/By Path + Filename"), NULL, plsort_menu_callback,
     PLAYLISTWIN_SORT_BYPATH, "<ImageItem>", sortbypathfile_pixbuf},
    {N_("/Sort List/By Date"), NULL, plsort_menu_callback,
     PLAYLISTWIN_SORT_BYDATE, "<ImageItem>", sortbydate_pixbuf},
    {N_("/Sort List/By Track Number"), NULL, plsort_menu_callback,
     PLAYLISTWIN_SORT_BYTRACK, "<ImageItem>", sortbytitle_pixbuf},
    {N_("/Sort List/By Playlist Entry"), NULL, plsort_menu_callback,
     PLAYLISTWIN_SORT_BYPLAYLIST, "<ImageItem>", sortbytitle_pixbuf},
    {N_("/Sort Selection"), NULL, NULL, 0, "<Branch>", NULL},
    {N_("/Sort Selection/By Title"), NULL, plsort_menu_callback,
     PLAYLISTWIN_SORT_SEL_BYTITLE, "<ImageItem>", sortbytitle_pixbuf},
    {N_("/Sort Selection/By Artist"), NULL, plsort_menu_callback,
     PLAYLISTWIN_SORT_SEL_BYARTIST, "<ImageItem>", sortbytitle_pixbuf},
    {N_("/Sort Selection/By Filename"), NULL, plsort_menu_callback,
     PLAYLISTWIN_SORT_SEL_BYFILENAME, "<ImageItem>", sortbyfilename_pixbuf},
    {N_("/Sort Selection/By Path + Filename"), NULL, plsort_menu_callback,
     PLAYLISTWIN_SORT_SEL_BYPATH, "<ImageItem>", sortbypathfile_pixbuf},
    {N_("/Sort Selection/By Date"), NULL, plsort_menu_callback,
     PLAYLISTWIN_SORT_SEL_BYDATE, "<ImageItem>", sortbydate_pixbuf},
    {N_("/Sort Selection/By Track Number"), NULL, plsort_menu_callback,
     PLAYLISTWIN_SORT_SEL_BYTRACK, "<ImageItem>", sortbytitle_pixbuf},
    {N_("/Sort Selection/By Playlist Entry"), NULL, plsort_menu_callback,
     PLAYLISTWIN_SORT_SEL_BYPLAYLIST, "<ImageItem>", sortbytitle_pixbuf}
};


static void playlistwin_draw_frame(void);


gboolean
playlistwin_is_shaded(void)
{
    return cfg.playlist_shaded;
}

gint
playlistwin_get_width(void)
{
    cfg.playlist_width /= PLAYLISTWIN_WIDTH_SNAP;
    cfg.playlist_width *= PLAYLISTWIN_WIDTH_SNAP;
    return cfg.playlist_width;
}

gint
playlistwin_get_height_unshaded(void)
{
    cfg.playlist_height /= PLAYLISTWIN_HEIGHT_SNAP;
    cfg.playlist_height *= PLAYLISTWIN_HEIGHT_SNAP;
    gint height = cfg.playlist_height;
    return height;
}

gint
playlistwin_get_height_shaded(void)
{
    return PLAYLISTWIN_SHADED_HEIGHT;
}

gint
playlistwin_get_height(void)
{
    if (playlistwin_is_shaded())
        return playlistwin_get_height_shaded();
    else
        return playlistwin_get_height_unshaded();
}

void
playlistwin_get_size(gint * width, gint * height)
{
    if (width)
        *width = playlistwin_get_width();

    if (height)
        *height = playlistwin_get_height();
}

static void
playlistwin_update_info(void)
{
    gchar *text, *sel_text, *tot_text;
    gulong selection, total;
    gboolean selection_more, total_more;

    playlist_get_total_time(&total, &selection, &total_more, &selection_more);

    if (selection > 0 || (selection == 0 && !selection_more)) {
        if (selection > 3600)
            sel_text =
                g_strdup_printf("%lu:%-2.2lu:%-2.2lu%s", selection / 3600,
                                (selection / 60) % 60, selection % 60,
                                (selection_more ? "+" : ""));
        else
            sel_text =
                g_strdup_printf("%lu:%-2.2lu%s", selection / 60,
                                selection % 60, (selection_more ? "+" : ""));
    }
    else
        sel_text = g_strdup("?");
    if (total > 0 || (total == 0 && !total_more)) {
        if (total > 3600)
            tot_text =
                g_strdup_printf("%lu:%-2.2lu:%-2.2lu%s", total / 3600,
                                (total / 60) % 60, total % 60,
                                total_more ? "+" : "");
        else
            tot_text =
                g_strdup_printf("%lu:%-2.2lu%s", total / 60, total % 60,
                                total_more ? "+" : "");
    }
    else
        tot_text = g_strdup("?");
    text = g_strconcat(sel_text, "/", tot_text, NULL);
    textbox_set_text(playlistwin_info, text ? text : "");
    g_free(text);
    g_free(tot_text);
    g_free(sel_text);
}

static void
playlistwin_update_sinfo(void)
{
    gchar *posstr, *timestr, *title, *info;
    gint pos, time;

    pos = playlist_get_position();
    title = playlist_get_songtitle(pos);

    if (!title) {
        textbox_set_text(playlistwin_sinfo, "");
        return;
    }

    convert_title_text(title);

    time = playlist_get_songtime(pos);

    if (cfg.show_numbers_in_pl)
        posstr = g_strdup_printf("%d. ", pos + 1);
    else
        posstr = g_strdup("");

    if (time != -1) {
        timestr = g_strdup_printf(" (%d:%-2.2d)", time / 60000,
                                      (time / 1000) % 60);
    }
    else
        timestr = g_strdup("");

    info = g_strdup_printf("%s%s%s", posstr, title, timestr);

    g_free(posstr);
    g_free(title);
    g_free(timestr);

    textbox_set_text(playlistwin_sinfo, info ? info : "");
    g_free(info);
}

gboolean
playlistwin_item_visible(gint index)
{
    if (index >= playlistwin_list->pl_first
        && index <
        (playlistwin_list->pl_first + playlistwin_list->pl_num_visible))
        return TRUE;
    return FALSE;
}

gint
playlistwin_get_toprow(void)
{
    if (playlistwin_list)
        return (playlistwin_list->pl_first);
    return (-1);
}

void
playlistwin_set_toprow(gint toprow)
{
    if (playlistwin_list)
        playlistwin_list->pl_first = toprow;
    playlistwin_update_list();
}

void
playlistwin_update_list(void)
{
    g_return_if_fail(playlistwin_list != NULL);

    widget_draw(WIDGET(playlistwin_list));
    widget_draw(WIDGET(playlistwin_slider));
    playlistwin_update_info();
    playlistwin_update_sinfo();
    /*     mainwin_update_jtf(); */
}

#if 0
static void
playlistwin_redraw_list(void)
{
    g_return_if_fail(playlistwin_list != NULL);

    draw_widget(playlistwin_list);
    draw_widget(playlistwin_slider);
}
#endif

static void
playlistwin_set_mask(void)
{
    GdkGC *gc;
    GdkColor pattern;

    if (playlistwin_mask)
        g_object_unref(playlistwin_mask);

    playlistwin_mask =
        gdk_pixmap_new(playlistwin->window, playlistwin_get_width(),
                       playlistwin_get_height(), 1);
    gc = gdk_gc_new(playlistwin_mask);
    pattern.pixel = 1;
    gdk_gc_set_foreground(gc, &pattern);
    gdk_draw_rectangle(playlistwin_mask, gc, TRUE, 0, 0,
                       playlistwin_get_width(), playlistwin_get_height());
    gdk_gc_destroy(gc);

    gtk_widget_shape_combine_mask(playlistwin, playlistwin_mask, 0, 0);
}

static void
playlistwin_set_geometry_hints(gboolean shaded)
{
    GdkGeometry geometry;
    GdkWindowHints mask;

    geometry.min_width = PLAYLISTWIN_MIN_WIDTH;
    geometry.max_width = G_MAXUINT16;

    geometry.width_inc = PLAYLISTWIN_WIDTH_SNAP;
    geometry.height_inc = PLAYLISTWIN_HEIGHT_SNAP;

    if (shaded) {
        geometry.min_height = PLAYLISTWIN_SHADED_HEIGHT;
        geometry.max_height = PLAYLISTWIN_SHADED_HEIGHT;
        geometry.base_height = PLAYLISTWIN_SHADED_HEIGHT;
    }
    else {
        geometry.min_height = PLAYLISTWIN_MIN_HEIGHT;
        geometry.max_height = G_MAXUINT16;
    }

    mask = GDK_HINT_MIN_SIZE | GDK_HINT_MAX_SIZE | GDK_HINT_RESIZE_INC;

    gtk_window_set_geometry_hints(GTK_WINDOW(playlistwin),
                                  playlistwin, &geometry, mask);
}

void
playlistwin_set_sinfo_font(gchar *font)
{
    gchar *tmp = NULL, *tmp2 = NULL;

    if(!font)
        return;

    tmp = g_strdup(font);
    if(!tmp)
        return;

    *strrchr(tmp, ' ') = '\0';
    tmp2 = g_strdup_printf("%s 8", tmp);
    if(!tmp2)
        return;
    textbox_set_xfont(playlistwin_sinfo, cfg.mainwin_use_xfont, tmp2);

    g_free(tmp); g_free(tmp2);
}

void
playlistwin_set_sinfo_scroll(gboolean scroll)
{
    GtkWidget *item;

    if(playlistwin_is_shaded())
        textbox_set_scroll(playlistwin_sinfo, cfg.autoscroll);
    else
        textbox_set_scroll(playlistwin_sinfo, FALSE);
}

void
playlistwin_set_shade(gboolean shaded)
{
    cfg.playlist_shaded = shaded;

    if (shaded) {
        playlistwin_set_sinfo_font(cfg.playlist_font);
        playlistwin_set_sinfo_scroll(cfg.autoscroll);
        widget_show(WIDGET(playlistwin_sinfo));
        playlistwin_shade->pb_nx = 128;
        playlistwin_shade->pb_ny = 45;
        playlistwin_shade->pb_px = 150;
        playlistwin_shade->pb_py = 42;
        playlistwin_close->pb_nx = 138;
        playlistwin_close->pb_ny = 45;
    }
    else {
        widget_hide(WIDGET(playlistwin_sinfo));
        playlistwin_set_sinfo_scroll(FALSE);
        playlistwin_shade->pb_nx = 157;
        playlistwin_shade->pb_ny = 3;
        playlistwin_shade->pb_px = 62;
        playlistwin_shade->pb_py = 42;
        playlistwin_close->pb_nx = 167;
        playlistwin_close->pb_ny = 3;
    }

    dock_shade(dock_window_list, GTK_WINDOW(playlistwin),
               playlistwin_get_height());

    playlistwin_set_geometry_hints(cfg.playlist_shaded);

    gtk_window_resize(GTK_WINDOW(playlistwin),
                      playlistwin_get_width(),
                      playlistwin_get_height());

    playlistwin_set_mask();

    widget_draw(WIDGET(playlistwin_list));
    widget_draw(WIDGET(playlistwin_slider));

    draw_playlist_window(TRUE);
}

static void
playlistwin_set_shade_menu(gboolean shaded)
{
    GtkWidget *item;

    item = gtk_item_factory_get_widget(mainwin_view_menu,
                                       "/Roll up Playlist Editor");
    gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(item), shaded);

    playlistwin_set_shade(shaded);
    playlistwin_update_list();
}

void
playlistwin_shade_toggle(void)
{
    playlistwin_set_shade_menu(!cfg.playlist_shaded);
}

static void
playlistwin_release(GtkWidget * widget,
                    GdkEventButton * event,
                    gpointer callback_data)
{
    if (event->button == 3)
        return;

    gdk_pointer_ungrab(GDK_CURRENT_TIME);
    playlistwin_resizing = FALSE;
    gdk_flush();
 
    if (dock_is_moving(GTK_WINDOW(playlistwin)))
    {
       dock_move_release(GTK_WINDOW(playlistwin));

       if (cfg.playlist_transparent)
           playlistwin_update_list();
    }
    else
    {
       handle_release_cb(playlistwin_wlist, widget, event);
       draw_playlist_window(FALSE);
    }
}

void
playlistwin_scroll(gint num)
{
    playlistwin_list->pl_first += num;
    playlistwin_update_list();
}

void
playlistwin_scroll_up_pushed(void)
{
    playlistwin_list->pl_first -= 3;
    playlistwin_update_list();
}

void
playlistwin_scroll_down_pushed(void)
{
    playlistwin_list->pl_first += 3;
    playlistwin_update_list();
}

static void
playlistwin_select_all(void)
{
    playlist_select_all(TRUE);
    playlistwin_list->pl_prev_selected = 0;
    playlistwin_list->pl_prev_min = 0;
    playlistwin_list->pl_prev_max = playlist_get_length() - 1;
    playlistwin_update_list();
}

static void
playlistwin_select_none(void)
{
    playlist_select_all(FALSE);
    playlistwin_list->pl_prev_selected = -1;
    playlistwin_list->pl_prev_min = -1;
    playlistwin_update_list();
}

static void
playlistwin_inverse_selection(void)
{
    playlist_select_invert_all();
    playlistwin_list->pl_prev_selected = -1;
    playlistwin_list->pl_prev_min = -1;
    playlistwin_update_list();
}

static void
playlistwin_resize(gint width, gint height)
{
    gboolean redraw;

    g_return_if_fail(width > 0 && height > 0);

    cfg.playlist_width = width;

    if (!cfg.playlist_shaded)
        cfg.playlist_height = height;
    else
        height = cfg.playlist_height;

    /* FIXME: why the fsck are we doing this manually? */
    /* adjust widget positions and sizes */

    widget_resize(WIDGET(playlistwin_list), width - 31, height - 58);

    widget_move(WIDGET(playlistwin_slider), width - 15, 20);
    widget_resize(WIDGET(playlistwin_slider), 8, height - 58);

    widget_resize(WIDGET(playlistwin_sinfo), width - 35, 14);
    playlistwin_update_sinfo();

    widget_move(WIDGET(playlistwin_shade), width - 21, 3);
    widget_move(WIDGET(playlistwin_close), width - 11, 3);
    widget_move(WIDGET(playlistwin_time_min), width - 82, height - 15);
    widget_move(WIDGET(playlistwin_time_sec), width - 64, height - 15);
    widget_move(WIDGET(playlistwin_info), width - 143, height - 28);
    widget_move(WIDGET(playlistwin_srew), width - 144, height - 16);
    widget_move(WIDGET(playlistwin_splay), width - 138, height - 16);
    widget_move(WIDGET(playlistwin_spause), width - 128, height - 16);
    widget_move(WIDGET(playlistwin_sstop), width - 118, height - 16);
    widget_move(WIDGET(playlistwin_sfwd), width - 109, height - 16);
    widget_move(WIDGET(playlistwin_seject), width - 100, height - 16);
    widget_move(WIDGET(playlistwin_sscroll_up), width - 14, height - 35);
    widget_move(WIDGET(playlistwin_sscroll_down), width - 14, height - 30);

    g_object_unref(playlistwin_bg);
    playlistwin_bg = gdk_pixmap_new(playlistwin->window, width, height, -1);
    playlistwin_set_mask();

    widget_list_lock(playlistwin_wlist);

    widget_list_change_pixmap(playlistwin_wlist, playlistwin_bg);
    playlistwin_draw_frame();
    widget_list_draw(playlistwin_wlist, &redraw, TRUE);
    widget_list_clear_redraw(playlistwin_wlist);

    widget_list_unlock(playlistwin_wlist);

    gdk_window_set_back_pixmap(playlistwin->window, playlistwin_bg, 0);
    gdk_window_clear(playlistwin->window);
}



static void
playlistwin_motion(GtkWidget * widget,
                   GdkEventMotion * event,
                   gpointer callback_data)
{
    GdkEvent *gevent;

    if (dock_is_moving(GTK_WINDOW(playlistwin))) {
        dock_move_motion(GTK_WINDOW(playlistwin), event);
    }
    else {
        handle_motion_cb(playlistwin_wlist, widget, event);
        draw_playlist_window(FALSE);
    }
    gdk_flush();

    while ((gevent = gdk_event_get()) != NULL) gdk_event_free(gevent);
}

static void
playlistwin_enter(GtkWidget * widget,
                   GdkEventMotion * event,
                   gpointer callback_data)
{
    playlistwin_list->pl_tooltips = TRUE;
}

static void
playlistwin_leave(GtkWidget * widget,
                   GdkEventMotion * event,
                   gpointer callback_data)
{
    playlistwin_list->pl_tooltips = FALSE;
}

static void
playlistwin_show_filebrowser(void)
{
    util_run_filebrowser(NO_PLAY_BUTTON);
}

#if 0
static void
playlistwin_add_dir_handler(const gchar * dir)
{
    g_free(cfg.filesel_path);
    cfg.filesel_path = g_strdup(dir);
    playlist_add_dir(dir);
}
#endif

static void
playlistwin_fileinfo(void)
{
    /* Show the first selected file, or the current file if nothing is
     * selected */
    GList *list = playlist_get_selected();
    if (list) {
        playlist_fileinfo(GPOINTER_TO_INT(list->data));
        g_list_free(list);
    }
    else
        playlist_fileinfo_current();
}

static void
menu_set_item_sensitive(GtkItemFactory * item_factory,
                        const gchar * path,
                        gboolean sensitive)
{
    GtkWidget *item = gtk_item_factory_get_widget(item_factory, path);
    gtk_widget_set_sensitive(item, sensitive);
}

/* FIXME: broken */
static void
playlistwin_set_sensitive_sortmenu(void)
{
    gboolean set = playlist_get_num_selected() > 1;
    menu_set_item_sensitive(plsort_menu, "/Sort Selection/By Title", set);
    menu_set_item_sensitive(plsort_menu, "/Sort Selection/By Filename", set);
    menu_set_item_sensitive(plsort_menu, "/Sort Selection/By Path + Filename", set);
    menu_set_item_sensitive(plsort_menu, "/Sort Selection/By Date", set);
}

static void
show_playlist_save_error(GtkWindow * parent,
                         const gchar * filename)
{
    GtkWidget *dialog;
    
    g_return_if_fail(GTK_IS_WINDOW(parent));
    g_return_if_fail(filename != NULL);

    dialog = gtk_message_dialog_new(GTK_WINDOW(parent),
                                    GTK_DIALOG_DESTROY_WITH_PARENT,
                                    GTK_MESSAGE_ERROR,
                                    GTK_BUTTONS_OK,
                                    _("Error writing playlist \"%s\": %s"),
                                    filename, strerror(errno));

    gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);
}

static gboolean
show_playlist_overwrite_prompt(GtkWindow * parent,
                               const gchar * filename)
{
    GtkWidget *dialog;
    gint result;

    g_return_val_if_fail(GTK_IS_WINDOW(parent), FALSE);
    g_return_val_if_fail(filename != NULL, FALSE);

    dialog = gtk_message_dialog_new(GTK_WINDOW(parent),
                                    GTK_DIALOG_DESTROY_WITH_PARENT,
                                    GTK_MESSAGE_QUESTION,
                                    GTK_BUTTONS_YES_NO,
                                    _("%s already exist. Continue?"),
                                    filename);

    result = gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);

    return (result == GTK_RESPONSE_YES);
}

static void
show_playlist_save_format_error(GtkWindow * parent,
                                const gchar * filename)
{
    const gchar *markup = 
        N_("<b><big>Unable to save playlist.</big></b>\n\n"
           "Unknown file type for '%s'.\n");

    GtkWidget *dialog;

    g_return_if_fail(GTK_IS_WINDOW(parent));
    g_return_if_fail(filename != NULL);

    dialog =
        gtk_message_dialog_new_with_markup(GTK_WINDOW(parent),
                                           GTK_DIALOG_DESTROY_WITH_PARENT,
                                           GTK_MESSAGE_ERROR,
                                           GTK_BUTTONS_OK,
                                           _(markup),
                                           filename);
    gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);
}

static void
playlistwin_save_playlist(const gchar * filename)
{
    PlaylistContainer *plc;
    gchar *ext = strrchr(filename, '.') + 1;

    plc = playlist_container_find(ext);
    if (plc == NULL) {
        show_playlist_save_format_error(GTK_WINDOW(playlistwin), filename);
        return;
    }

    str_replace_in(&cfg.playlist_path, g_path_get_dirname(filename));

    if (g_file_test(filename, G_FILE_TEST_IS_REGULAR))
        if (!show_playlist_overwrite_prompt(GTK_WINDOW(playlistwin), filename))
            return;

    if (!playlist_save(filename))
        show_playlist_save_error(GTK_WINDOW(playlistwin), filename);
}

#if 0
static void
playlistwin_save_current(void)
{
    const gchar *filename;

    if (!(filename = playlist_get_current_name()))
        return;

    playlistwin_save_playlist(filename);
}
#endif

static void
playlistwin_load_playlist(const gchar * filename)
{
    g_return_if_fail(filename != NULL);

    str_replace_in(&cfg.playlist_path, g_strdup(filename));

    playlist_clear();
    mainwin_clear_song_info();
    mainwin_set_info_text();

    playlist_load(filename);
    playlist_set_current_name(filename);
}

static gchar *
playlist_file_selection_load(const gchar * title,
                        const gchar * default_filename)
{
    static GtkWidget *dialog = NULL;
    GtkWidget *button;
    gchar *filename;

    g_return_val_if_fail(title != NULL, NULL);

    if(!dialog) {
        dialog = gtk_file_chooser_dialog_new(title, GTK_WINDOW(mainwin),
                                             GTK_FILE_CHOOSER_ACTION_OPEN, NULL, NULL);

        if (default_filename)
            gtk_file_chooser_set_filename(GTK_FILE_CHOOSER(dialog),
                                          default_filename);

        button = gtk_dialog_add_button(GTK_DIALOG(dialog), GTK_STOCK_CANCEL,
                                       GTK_RESPONSE_REJECT);
        gtk_button_set_use_stock(GTK_BUTTON(button), TRUE);
        GTK_WIDGET_SET_FLAGS(button, GTK_CAN_DEFAULT);

        button = gtk_dialog_add_button(GTK_DIALOG(dialog),
                                       GTK_STOCK_OPEN,
                                       GTK_RESPONSE_ACCEPT);
        gtk_button_set_use_stock(GTK_BUTTON(button), TRUE);
        gtk_dialog_set_default_response(GTK_DIALOG(dialog), GTK_RESPONSE_ACCEPT);
    }

    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT)
        filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
    else
        filename = NULL;

    gtk_widget_hide(dialog);
    return filename;
}

static gchar *
playlist_file_selection_save(const gchar * title,
                        const gchar * default_filename)
{
    static GtkWidget *dialog = NULL;
    GtkWidget *button;
    gchar *filename;

    g_return_val_if_fail(title != NULL, NULL);

    if(!dialog) {
        dialog = gtk_file_chooser_dialog_new(title, GTK_WINDOW(mainwin),
                                             GTK_FILE_CHOOSER_ACTION_SAVE, NULL, NULL);

        if (default_filename)
            gtk_file_chooser_set_filename(GTK_FILE_CHOOSER(dialog),
                                          default_filename);

        button = gtk_dialog_add_button(GTK_DIALOG(dialog), GTK_STOCK_CANCEL,
                                       GTK_RESPONSE_REJECT);
        gtk_button_set_use_stock(GTK_BUTTON(button), TRUE);
        GTK_WIDGET_SET_FLAGS(button, GTK_CAN_DEFAULT);

        button = gtk_dialog_add_button(GTK_DIALOG(dialog),
                                       GTK_STOCK_SAVE,
                                       GTK_RESPONSE_ACCEPT);
        gtk_button_set_use_stock(GTK_BUTTON(button), TRUE);
        gtk_dialog_set_default_response(GTK_DIALOG(dialog), GTK_RESPONSE_ACCEPT);
    }

    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT)
        filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
    else
        filename = NULL;

    gtk_widget_hide(dialog);
    return filename;
}

void
playlistwin_select_playlist_to_load(const gchar * default_filename)
{
    gchar *filename =
        playlist_file_selection_load(_("Load Playlist"), default_filename);

    if (filename) {
        playlistwin_load_playlist(filename);
        g_free(filename);
    }
}

static void
playlistwin_select_playlist_to_save(const gchar * default_filename)
{
    gchar *dot = NULL, *basename = NULL;
    gchar *filename =
        playlist_file_selection_save(_("Save Playlist"), default_filename);

    if (filename) {
        /* Default to xspf if no filename has extension */
        basename = g_path_get_basename(filename);
        dot = strrchr(basename, '.');
        if( dot == NULL || dot == basename) {
            gchar *oldname = filename;
            filename = g_strconcat(oldname, ".xspf", NULL);
            g_free(oldname);
        }
        g_free(basename);

        playlistwin_save_playlist(filename);
        g_free(filename);
    }
}

static gboolean
inside_sensitive_widgets(gint x, gint y)
{
    return (widget_contains(WIDGET(playlistwin_list), x, y) ||
            widget_contains(WIDGET(playlistwin_slider), x, y) ||
            widget_contains(WIDGET(playlistwin_close), x, y) ||
            widget_contains(WIDGET(playlistwin_shade), x, y) ||
            widget_contains(WIDGET(playlistwin_time_min), x, y) ||
            widget_contains(WIDGET(playlistwin_time_sec), x, y) ||
            widget_contains(WIDGET(playlistwin_info), x, y) ||
            widget_contains(WIDGET(playlistwin_srew), x, y) ||
            widget_contains(WIDGET(playlistwin_splay), x, y) ||
            widget_contains(WIDGET(playlistwin_spause), x, y) ||
            widget_contains(WIDGET(playlistwin_sstop), x, y) ||
            widget_contains(WIDGET(playlistwin_sfwd), x, y) ||
            widget_contains(WIDGET(playlistwin_seject), x, y) ||
            widget_contains(WIDGET(playlistwin_sscroll_up), x, y) ||
            widget_contains(WIDGET(playlistwin_sscroll_down), x, y));
}

#define REGION_L(x1,x2,y1,y2)                   \
    (event->x >= (x1) && event->x < (x2) &&     \
     event->y >= cfg.playlist_height - (y1) &&  \
     event->y < cfg.playlist_height - (y2))

#define REGION_R(x1,x2,y1,y2)                      \
    (event->x >= playlistwin_get_width() - (x1) && \
     event->x < playlistwin_get_width() - (x2) &&  \
     event->y >= cfg.playlist_height - (y1) &&     \
     event->y < cfg.playlist_height - (y2))

static void
playlistwin_scrolled(GtkWidget * widget,
                     GdkEventScroll * event,
                     gpointer callback_data)
{

    if (event->direction == GDK_SCROLL_DOWN)
        playlistwin_scroll(cfg.scroll_pl_by);

    if (event->direction == GDK_SCROLL_UP)
        playlistwin_scroll(-cfg.scroll_pl_by);

}




static gboolean
playlistwin_press(GtkWidget * widget,
                  GdkEventButton * event,
                  gpointer callback_data)
{
    gboolean grab = TRUE;
    gint xpos, ypos;
    GtkWidget *_menu;
    GtkRequisition req;

    gtk_window_get_position(GTK_WINDOW(playlistwin), &xpos, &ypos);

    if (event->button == 1 && !cfg.show_wm_decorations &&
        ((!cfg.playlist_shaded &&
          event->x > playlistwin_get_width() - 20 &&
          event->y > cfg.playlist_height - 20) ||
         (cfg.playlist_shaded &&
          event->x >= playlistwin_get_width() - 31 &&
          event->x < playlistwin_get_width() - 22))) {

        /* NOTE: Workaround for bug #214 */
        if (event->type != GDK_2BUTTON_PRESS && 
            event->type != GDK_3BUTTON_PRESS) {
            /* resize area */
            playlistwin_resizing = TRUE;
            gtk_window_begin_resize_drag(GTK_WINDOW(widget),
                                         GDK_WINDOW_EDGE_SOUTH_EAST,
                                         event->button,
                                         event->x + xpos, event->y + ypos,
                                         event->time);
        }
        grab = FALSE;
    }
    else if (event->button == 1 && REGION_L(12, 37, 29, 11)) {
        /* ADD button menu */

        _menu = GTK_WIDGET(pladd_menu->widget);
        if (!GTK_WIDGET_REALIZED(_menu)) gtk_widget_realize(_menu);
        gtk_widget_size_request(_menu, &req);
        gtk_item_factory_popup_with_data(pladd_menu,
                                         NULL, NULL,
                                         xpos+12,
                                         (ypos + playlistwin_get_height()) - 8 - req.height, 1, event->time);
        grab = FALSE;
    }
    else if (event->button == 1 && REGION_L(41, 66, 29, 11)) {
        /* SUB button menu */
        _menu = GTK_WIDGET(pldel_menu->widget);
        if (!GTK_WIDGET_REALIZED(_menu)) gtk_widget_realize(_menu);
        gtk_widget_size_request(_menu, &req);
        gtk_item_factory_popup_with_data(pldel_menu,
                                         NULL, NULL,
                                         xpos+40,
                                         (ypos + playlistwin_get_height()) - 8 - req.height, 1, event->time);
        grab = FALSE;
    }
    else if (event->button == 1 && REGION_L(70, 95, 29, 11)) {
        /* SEL button menu */
        _menu = GTK_WIDGET(plsel_menu->widget);
        if (!GTK_WIDGET_REALIZED(_menu)) gtk_widget_realize(_menu);
        gtk_widget_size_request(_menu, &req);
        gtk_item_factory_popup_with_data(plsel_menu,
                                         NULL, NULL,
                                         xpos+68,
                                         (ypos + playlistwin_get_height()) - 8 - req.height, 1, event->time);

        grab = FALSE;
    }
    else if (event->button == 1 && REGION_L(99, 124, 29, 11)) {
        /* MISC button menu */
        _menu = GTK_WIDGET(plsort_menu->widget);
        if (!GTK_WIDGET_REALIZED(_menu)) gtk_widget_realize(_menu);
        gtk_widget_size_request(_menu, &req);
        gtk_item_factory_popup_with_data(plsort_menu,
                                         NULL, NULL,
                                         xpos+100,
                                         (ypos + playlistwin_get_height()) - 8 - req.height, 1, event->time);
        grab = FALSE;
    }
    else if (event->button == 1 && REGION_R(46, 23, 29, 11)) {
        /* LIST button menu */
        _menu = GTK_WIDGET(pllist_menu->widget);
        if (!GTK_WIDGET_REALIZED(_menu)) gtk_widget_realize(_menu);
        gtk_widget_size_request(_menu, &req);
        gtk_item_factory_popup_with_data(pllist_menu,
                                         NULL, NULL,
                                         xpos + playlistwin_get_width() - req.width - 12,
                                         (ypos + playlistwin_get_height()) - 8 - req.height, 1, event->time);
        grab = FALSE;
    }
    else if (event->button == 1 && REGION_R(82, 54, 15, 9)) {
        if (cfg.timer_mode == TIMER_ELAPSED)
            cfg.timer_mode = TIMER_REMAINING;
        else
            cfg.timer_mode = TIMER_ELAPSED;
    }
    else if (event->button == 2 && (event->type == GDK_BUTTON_PRESS) &&
             widget_contains(WIDGET(playlistwin_list), event->x, event->y)) {
        gtk_selection_convert(widget, GDK_SELECTION_PRIMARY,
                              GDK_TARGET_STRING, event->time);
    }
    else if (event->button == 1 && event->type == GDK_BUTTON_PRESS &&
             !inside_sensitive_widgets(event->x, event->y) && event->y < 14)
    {
        dock_move_press(dock_window_list, GTK_WINDOW(playlistwin), event,
                        FALSE);
        gtk_window_present(GTK_WINDOW(playlistwin));
    }
    else if (event->button == 1 && event->type == GDK_2BUTTON_PRESS &&
             !inside_sensitive_widgets(event->x, event->y)
             && event->y < 14) {
        /* double click on title bar */
        playlistwin_shade_toggle();
        if (dock_is_moving(GTK_WINDOW(playlistwin)))
            dock_move_release(GTK_WINDOW(playlistwin));
        return TRUE;
    }
    else if (event->button == 3 &&
             !(widget_contains(WIDGET(playlistwin_list), event->x, event->y) ||
               (event->y >= cfg.playlist_height - 29 &&
                event->y < cfg.playlist_height - 11 &&
                ((event->x >= 12 && event->x < 37) ||
                 (event->x >= 41 && event->x < 66) ||
                 (event->x >= 70 && event->x < 95) ||
                 (event->x >= 99 && event->x < 124) ||
                 (event->x >= playlistwin_get_width() - 46 &&
                  event->x < playlistwin_get_width() - 23))))) {
        /*
         * Pop up the main menu a few pixels down to avoid
         * anything to be selected initially.
         */
        util_item_factory_popup(mainwin_general_menu, event->x_root,
                                event->y_root + 2, 3, event->time);
        grab = FALSE;
    }
    else if (event->button == 3 &&
             widget_contains(WIDGET(playlistwin_list), event->x, event->y)) {
        /* popup menu */
        playlistwin_set_sensitive_sortmenu();
        {
            GtkWidget *item = gtk_item_factory_get_widget(playlistwin_popup_menu, "/Show Popup Info");
            gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(item), cfg.show_filepopup_for_tuple);
        }
        gtk_item_factory_popup(playlistwin_popup_menu,
                               event->x_root, event->y_root + 5,
                               3, event->time);
        grab = FALSE;
    }
    else {
        handle_press_cb(playlistwin_wlist, widget, event);
        draw_playlist_window(FALSE);
    }

    if (grab)
        gdk_pointer_grab(playlistwin->window, FALSE,
                         GDK_BUTTON_MOTION_MASK | GDK_BUTTON_RELEASE_MASK |
                         GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK |
                         GDK_BUTTON1_MOTION_MASK, NULL, NULL,
                         GDK_CURRENT_TIME);

    return FALSE;
}

static gboolean
playlistwin_focus_in(GtkWidget * widget, GdkEvent * event, gpointer data)
{
    playlistwin_close->pb_allow_draw = TRUE;
    playlistwin_shade->pb_allow_draw = TRUE;
    draw_playlist_window(TRUE);
    return FALSE;
}

static gboolean
playlistwin_focus_out(GtkWidget * widget,
                      GdkEventButton * event, gpointer data)
{
    playlistwin_close->pb_allow_draw = FALSE;
    playlistwin_shade->pb_allow_draw = FALSE;
    draw_playlist_window(TRUE);
    return FALSE;
}

static gboolean
playlistwin_configure(GtkWidget * window,
                      GdkEventConfigure * event, gpointer data)
{
    if (!GTK_WIDGET_VISIBLE(window))
        return FALSE;

    cfg.playlist_x = event->x;
    cfg.playlist_y = event->y;

    if (playlistwin_resizing) {
        if (event->width != playlistwin_get_width() ||
            event->height != playlistwin_get_height())
            playlistwin_resize(event->width, event->height);
    }
    return TRUE;
}

void
playlistwin_set_back_pixmap(void)
{
    gdk_window_set_back_pixmap(playlistwin->window, playlistwin_bg, 0);
    gdk_window_clear(playlistwin->window);
}

static gboolean
playlistwin_delete(GtkWidget * w, gpointer data)
{
    playlistwin_hide();
    return TRUE;
}

static void
playlistwin_keypress_up_down_handler(PlayList_List * pl,
                                     gboolean up, guint state)
{
    if ((state & GDK_MOD1_MASK) && (state & GDK_SHIFT_MASK))
        return;
    if (!(state & GDK_MOD1_MASK))
        playlist_select_all(FALSE);

    if (pl->pl_prev_selected == -1 ||
        (!playlistwin_item_visible(pl->pl_prev_selected) &&
         !(state & GDK_SHIFT_MASK && pl->pl_prev_min != -1))) {
        pl->pl_prev_selected = pl->pl_first;
    }
    else if (state & GDK_SHIFT_MASK) {
        if (pl->pl_prev_min == -1) {
            pl->pl_prev_max = pl->pl_prev_selected;
            pl->pl_prev_min = pl->pl_prev_selected;
        }
        pl->pl_prev_max += (up ? -1 : 1);
        pl->pl_prev_max =
            CLAMP(pl->pl_prev_max, 0, playlist_get_length() - 1);

        pl->pl_first = MIN(pl->pl_first, pl->pl_prev_max);
        pl->pl_first = MAX(pl->pl_first, pl->pl_prev_max -
                           pl->pl_num_visible + 1);
        playlist_select_range(pl->pl_prev_min, pl->pl_prev_max, TRUE);
        return;
    }
    else if (state & GDK_MOD1_MASK) {
        if (up)
            playlist_list_move_up(pl);
        else
            playlist_list_move_down(pl);
        if (pl->pl_prev_min < pl->pl_first)
            pl->pl_first = pl->pl_prev_min;
        else if (pl->pl_prev_max >= (pl->pl_first + pl->pl_num_visible))
            pl->pl_first = pl->pl_prev_max - pl->pl_num_visible + 1;
        return;
    }
    else if (up)
        pl->pl_prev_selected--;
    else
        pl->pl_prev_selected++;

    pl->pl_prev_selected =
        CLAMP(pl->pl_prev_selected, 0, playlist_get_length() - 1);

    if (pl->pl_prev_selected < pl->pl_first)
        pl->pl_first--;
    else if (pl->pl_prev_selected >= (pl->pl_first + pl->pl_num_visible))
        pl->pl_first++;

    playlist_select_range(pl->pl_prev_selected, pl->pl_prev_selected, TRUE);
    pl->pl_prev_min = -1;

}

/* FIXME: Handle the keys through menu */

static gboolean
playlistwin_keypress(GtkWidget * w, GdkEventKey * event, gpointer data)
{
    guint keyval;
    gboolean refresh = FALSE;

    if (cfg.playlist_shaded)
        return FALSE;

    switch (keyval = event->keyval) {
    case GDK_KP_Up:
    case GDK_KP_Down:
    case GDK_Up:
    case GDK_Down:
        playlistwin_keypress_up_down_handler(playlistwin_list,
                                             keyval == GDK_Up
                                             || keyval == GDK_KP_Up,
                                             event->state);
        refresh = TRUE;
        break;
    case GDK_Page_Up:
        playlistwin_scroll(-playlistwin_list->pl_num_visible);
        refresh = TRUE;
        break;
    case GDK_Page_Down:
        playlistwin_scroll(playlistwin_list->pl_num_visible);
        refresh = TRUE;
        break;
    case GDK_Home:
        playlistwin_list->pl_first = 0;
        refresh = TRUE;
        break;
    case GDK_End:
        playlistwin_list->pl_first =
            playlist_get_length() - playlistwin_list->pl_num_visible;
        refresh = TRUE;
        break;
    case GDK_Return:
        if (playlistwin_list->pl_prev_selected > -1
            && playlistwin_item_visible(playlistwin_list->pl_prev_selected)) {
            playlist_set_position(playlistwin_list->pl_prev_selected);
            if (!bmp_playback_get_playing())
                bmp_playback_initiate();
        }
        break;
    case GDK_3:
        if (event->state & GDK_CONTROL_MASK)
            playlistwin_fileinfo();
        break;
    case GDK_Delete:
        if (event->state & GDK_CONTROL_MASK)
            playlist_delete(TRUE);
        else
            playlist_delete(FALSE);
        break;
    case GDK_Insert:
        if (event->state & GDK_MOD1_MASK)
            mainwin_show_add_url_window();
        else
            playlistwin_show_filebrowser();
        break;
    case GDK_Left:
    case GDK_KP_Left:
    case GDK_KP_7:
        if (playlist_get_current_length() != -1)
            bmp_playback_seek(CLAMP
                              (bmp_playback_get_time() - 1000, 0,
                              playlist_get_current_length()) / 1000);
        break;
    case GDK_Right:
    case GDK_KP_Right:
    case GDK_KP_9:
        if (playlist_get_current_length() != -1)
            bmp_playback_seek(CLAMP
                              (bmp_playback_get_time() + 1000, 0,
                              playlist_get_current_length()) / 1000);
        break;

    case GDK_Escape:
        mainwin_minimize_cb();
        break;
    default:
        return FALSE;
    }

    if (refresh)
        playlistwin_update_list();

    return TRUE;
}

static void
playlistwin_draw_frame(void)
{
    gboolean focus =
        gtk_window_has_toplevel_focus(GTK_WINDOW(playlistwin)) ||
        !cfg.dim_titlebar;

    if (cfg.playlist_shaded) {
        skin_draw_playlistwin_shaded(bmp_active_skin,
                                     playlistwin_bg, playlistwin_gc,
                                     playlistwin_get_width(), focus);
    }
    else {
        skin_draw_playlistwin_frame(bmp_active_skin,
                                    playlistwin_bg, playlistwin_gc,
                                    playlistwin_get_width(),
                                    cfg.playlist_height, focus);
    }
}

void
draw_playlist_window(gboolean force)
{
    gboolean redraw;
    GList *wl;
    Widget *w;

    if (force)
        playlistwin_draw_frame();

    widget_list_lock(playlistwin_wlist);
    widget_list_draw(playlistwin_wlist, &redraw, force);

    if (redraw || force) {
        if (force) {
            gdk_window_clear(playlistwin->window);
        }
        else {
            for (wl = playlistwin_wlist; wl; wl = g_list_next(wl)) {
                w = WIDGET(wl->data);
                if (w->redraw && w->visible) {
                    gdk_window_clear_area(playlistwin->window, w->x, w->y,
                                          w->width, w->height);
                    w->redraw = FALSE;
                }
            }
        }

        gdk_flush();
    }

    widget_list_unlock(playlistwin_wlist);
}


void
playlistwin_hide_timer(void)
{
    textbox_set_text(playlistwin_time_min, "   ");
    textbox_set_text(playlistwin_time_sec, "  ");
}

void
playlistwin_set_time(gint time, gint length, TimerMode mode)
{
    gchar *text, sign;

    if (mode == TIMER_REMAINING && length != -1) {
        time = length - time;
        sign = '-';
    }
    else
        sign = ' ';

    time /= 1000;

    if (time < 0)
        time = 0;
    if (time > 99 * 60)
        time /= 60;

    text = g_strdup_printf("%c%-2.2d", sign, time / 60);
    textbox_set_text(playlistwin_time_min, text);
    g_free(text);

    text = g_strdup_printf("%-2.2d", time % 60);
    textbox_set_text(playlistwin_time_sec, text);
    g_free(text);
}

static void
playlistwin_drag_motion(GtkWidget * widget,
                        GdkDragContext * context,
                        gint x, gint y,
                        GtkSelectionData * selection_data,
                        guint info, guint time, gpointer user_data)
{
    playlistwin_list->pl_drag_motion = TRUE;
    playlistwin_list->drag_motion_x = x;
    playlistwin_list->drag_motion_y = y;
    playlistwin_update_list();
    playlistwin_hint_flag = TRUE;
}

static void
playlistwin_drag_end(GtkWidget * widget,
                     GdkDragContext * context, gpointer user_data)
{
    playlistwin_list->pl_drag_motion = FALSE;
    playlistwin_hint_flag = FALSE;
    playlistwin_update_list();
}

static void
playlistwin_drag_data_received(GtkWidget * widget,
                               GdkDragContext * context,
                               gint x, gint y,
                               GtkSelectionData *
                               selection_data, guint info,
                               guint time, gpointer user_data)
{
    gint pos;

    g_return_if_fail(selection_data != NULL);

    if (!selection_data->data) {
        g_message("Received no DND data!");
        return;
    }

    if (widget_contains(WIDGET(playlistwin_list), x, y)) {
        pos = (y - WIDGET(playlistwin_list)->y) /
            playlistwin_list->pl_fheight + playlistwin_list->pl_first;

        pos = MIN(pos, playlist_get_length());
        playlist_ins_url((gchar *) selection_data->data, pos);
    }
    else
        playlist_add_url((gchar *) selection_data->data);
}

static void
playlistwin_create_widgets(void)
{
    gchar *font = NULL, *tmp = NULL;
    /* This function creates the custom widgets used by the playlist editor */

    /* text box for displaying song title in shaded mode */
    playlistwin_sinfo =
        create_textbox(&playlistwin_wlist, playlistwin_bg, playlistwin_gc,
                       4, 4, playlistwin_get_width() - 35, TRUE, SKIN_TEXT);

    playlistwin_set_sinfo_font(cfg.playlist_font);

    if (!cfg.playlist_shaded)
        widget_hide(WIDGET(playlistwin_sinfo));

    /* shade/unshade window push button */
    if (cfg.playlist_shaded)
        playlistwin_shade =
            create_pbutton(&playlistwin_wlist, playlistwin_bg,
                           playlistwin_gc, playlistwin_get_width() - 21, 3,
                           9, 9, 128, 45, 150, 42,
                           playlistwin_shade_toggle, SKIN_PLEDIT);
    else
        playlistwin_shade =
            create_pbutton(&playlistwin_wlist, playlistwin_bg,
                           playlistwin_gc, playlistwin_get_width() - 21, 3,
                           9, 9, 157, 3, 62, 42, playlistwin_shade_toggle,
                           SKIN_PLEDIT);

    playlistwin_shade->pb_allow_draw = FALSE;

    /* close window push button */
    playlistwin_close =
        create_pbutton(&playlistwin_wlist, playlistwin_bg, playlistwin_gc,
                       playlistwin_get_width() - 11, 3, 9, 9,
                       cfg.playlist_shaded ? 138 : 167,
                       cfg.playlist_shaded ? 45 : 3, 52, 42,
                       playlistwin_hide, SKIN_PLEDIT);
    playlistwin_close->pb_allow_draw = FALSE;

    /* playlist list box */
    playlistwin_list =
        create_playlist_list(&playlistwin_wlist, playlistwin_bg,
                             playlistwin_gc, 12, 20,
                             playlistwin_get_width() - 31,
                             cfg.playlist_height - 58);
    playlist_list_set_font(cfg.playlist_font);

    /* playlist list box slider */
    playlistwin_slider =
        create_playlistslider(&playlistwin_wlist, playlistwin_bg,
                              playlistwin_gc, playlistwin_get_width() - 15,
                              20, cfg.playlist_height - 58, playlistwin_list);
    /* track time (minute) */
    playlistwin_time_min =
        create_textbox(&playlistwin_wlist, playlistwin_bg, playlistwin_gc,
                       playlistwin_get_width() - 82,
                       cfg.playlist_height - 15, 15, FALSE, SKIN_TEXT);

    /* track time (second) */
    playlistwin_time_sec =
        create_textbox(&playlistwin_wlist, playlistwin_bg, playlistwin_gc,
                       playlistwin_get_width() - 64,
                       cfg.playlist_height - 15, 10, FALSE, SKIN_TEXT);

    /* playlist information (current track length / total track length) */
    playlistwin_info =
        create_textbox(&playlistwin_wlist, playlistwin_bg, playlistwin_gc,
                       playlistwin_get_width() - 143,
                       cfg.playlist_height - 28, 90, FALSE, SKIN_TEXT);

    /* mini play control buttons at right bottom corner */

    /* rewind button */
    playlistwin_srew =
        create_sbutton(&playlistwin_wlist, playlistwin_bg, playlistwin_gc,
                       playlistwin_get_width() - 144,
                       cfg.playlist_height - 16, 8, 7, playlist_prev);

    /* play button */
    playlistwin_splay =
        create_sbutton(&playlistwin_wlist, playlistwin_bg, playlistwin_gc,
                       playlistwin_get_width() - 138,
                       cfg.playlist_height - 16, 10, 7, mainwin_play_pushed);

    /* pause button */
    playlistwin_spause =
        create_sbutton(&playlistwin_wlist, playlistwin_bg, playlistwin_gc,
                       playlistwin_get_width() - 128,
                       cfg.playlist_height - 16, 10, 7, bmp_playback_pause);

    /* stop button */
    playlistwin_sstop =
        create_sbutton(&playlistwin_wlist, playlistwin_bg, playlistwin_gc,
                       playlistwin_get_width() - 118,
                       cfg.playlist_height - 16, 9, 7, mainwin_stop_pushed);

    /* forward button */
    playlistwin_sfwd =
        create_sbutton(&playlistwin_wlist, playlistwin_bg, playlistwin_gc,
                       playlistwin_get_width() - 109,
                       cfg.playlist_height - 16, 8, 7, playlist_next);

    /* eject button */
    playlistwin_seject =
        create_sbutton(&playlistwin_wlist, playlistwin_bg, playlistwin_gc,
                       playlistwin_get_width() - 100,
                       cfg.playlist_height - 16, 9, 7, mainwin_eject_pushed);


    playlistwin_sscroll_up =
        create_sbutton(&playlistwin_wlist, playlistwin_bg, playlistwin_gc,
                       playlistwin_get_width() - 14,
                       cfg.playlist_height - 35, 8, 5,
                       playlistwin_scroll_up_pushed);
    playlistwin_sscroll_down =
        create_sbutton(&playlistwin_wlist, playlistwin_bg, playlistwin_gc,
                       playlistwin_get_width() - 14,
                       cfg.playlist_height - 30, 8, 5,
                       playlistwin_scroll_down_pushed);

}

static void
selection_received(GtkWidget * widget,
                   GtkSelectionData * selection_data, gpointer data)
{
    if (selection_data->type == GDK_SELECTION_TYPE_STRING &&
        selection_data->length > 0)
        playlist_add_url((gchar *) selection_data->data);
}

static void
playlistwin_create_window(void)
{
    GdkPixbuf *icon;

    playlistwin = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(playlistwin), _("Audacious Playlist Editor"));
    gtk_window_set_wmclass(GTK_WINDOW(playlistwin), "playlist", "Audacious");
    gtk_window_set_role(GTK_WINDOW(playlistwin), "playlist");
    gtk_window_set_default_size(GTK_WINDOW(playlistwin),
                                playlistwin_get_width(),
                                playlistwin_get_height());
    gtk_window_set_resizable(GTK_WINDOW(playlistwin), TRUE);
    playlistwin_set_geometry_hints(cfg.playlist_shaded);
    dock_window_list = dock_window_set_decorated(dock_window_list,
                                                 GTK_WINDOW(playlistwin),
                                                 cfg.show_wm_decorations);

    gtk_window_set_transient_for(GTK_WINDOW(playlistwin),
                                 GTK_WINDOW(mainwin));
    gtk_window_set_skip_taskbar_hint(GTK_WINDOW(playlistwin), TRUE);

    icon = gdk_pixbuf_new_from_xpm_data((const gchar **) bmp_playlist_icon);
    gtk_window_set_icon(GTK_WINDOW(playlistwin), icon);
    g_object_unref(icon);

    gtk_widget_set_app_paintable(playlistwin, TRUE);

    if (cfg.playlist_x != -1 && cfg.save_window_position)
        gtk_window_move(GTK_WINDOW(playlistwin),
                        cfg.playlist_x, cfg.playlist_y);

    gtk_widget_add_events(playlistwin, GDK_POINTER_MOTION_MASK |
                          GDK_FOCUS_CHANGE_MASK | GDK_BUTTON_MOTION_MASK |
                          GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK |
                          GDK_SCROLL_MASK | GDK_VISIBILITY_NOTIFY_MASK);
    gtk_widget_realize(playlistwin);

    util_set_cursor(playlistwin);

    g_signal_connect(playlistwin, "delete_event",
                     G_CALLBACK(playlistwin_delete), NULL);
    g_signal_connect(playlistwin, "button_press_event",
                     G_CALLBACK(playlistwin_press), NULL);
    g_signal_connect(playlistwin, "button_release_event",
                     G_CALLBACK(playlistwin_release), NULL);
    g_signal_connect(playlistwin, "scroll_event",
                     G_CALLBACK(playlistwin_scrolled), NULL);
    g_signal_connect(playlistwin, "motion_notify_event",
                     G_CALLBACK(playlistwin_motion), NULL);
    g_signal_connect(playlistwin, "enter_notify_event",
                     G_CALLBACK(playlistwin_enter), NULL);
    g_signal_connect(playlistwin, "leave_notify_event",
                     G_CALLBACK(playlistwin_leave), NULL);
    g_signal_connect_after(playlistwin, "focus_in_event",
                           G_CALLBACK(playlistwin_focus_in), NULL);
    g_signal_connect_after(playlistwin, "focus_out_event",
                           G_CALLBACK(playlistwin_focus_out), NULL);
    g_signal_connect(playlistwin, "configure_event",
                     G_CALLBACK(playlistwin_configure), NULL);
    g_signal_connect(playlistwin, "style_set",
                     G_CALLBACK(playlistwin_set_back_pixmap), NULL);

    bmp_drag_dest_set(playlistwin);

    /* DnD stuff */
    g_signal_connect(playlistwin, "drag-leave",
                     G_CALLBACK(playlistwin_drag_end), NULL);
    g_signal_connect(playlistwin, "drag-data-delete",
                     G_CALLBACK(playlistwin_drag_end), NULL);
    g_signal_connect(playlistwin, "drag-end",
                     G_CALLBACK(playlistwin_drag_end), NULL);
    g_signal_connect(playlistwin, "drag-drop",
                     G_CALLBACK(playlistwin_drag_end), NULL);
    g_signal_connect(playlistwin, "drag-data-received",
                     G_CALLBACK(playlistwin_drag_data_received), NULL);
    g_signal_connect(playlistwin, "drag-motion",
                     G_CALLBACK(playlistwin_drag_motion), NULL);

    g_signal_connect(playlistwin, "key_press_event",
                     G_CALLBACK(playlistwin_keypress), NULL);
    g_signal_connect(playlistwin, "selection_received",
                     G_CALLBACK(selection_received), NULL);

    playlistwin_set_mask();
}

void
playlistwin_create_popup_menus(void)
{
    playlistwin_accel = gtk_accel_group_new();

    /* playlist window popup menu */
    playlistwin_popup_menu = create_menu(playlistwin_popup_menu_entries,
                                         G_N_ELEMENTS(playlistwin_popup_menu_entries),
                                         playlistwin_accel);

    pladd_menu  = create_menu(pladd_menu_entries, G_N_ELEMENTS(pladd_menu_entries),
                              playlistwin_accel);
    pldel_menu  = create_menu(pldel_menu_entries, G_N_ELEMENTS(pldel_menu_entries),
                              playlistwin_accel);
    plsel_menu  = create_menu(plsel_menu_entries, G_N_ELEMENTS(plsel_menu_entries),
                              playlistwin_accel);
    plsort_menu = create_menu(plsort_menu_entries,
                              G_N_ELEMENTS(plsort_menu_entries),
                              playlistwin_accel);
    pllist_menu = create_menu(pllist_menu_entries, G_N_ELEMENTS(pllist_menu_entries),
                              playlistwin_accel);

#if 0
    make_submenu(playlistwin_popup_menu, "/Playlist",
                 playlistwin_playlist_menu);
    make_submenu(playlistwin_popup_menu, "/Playback",
                 playlistwin_playback_menu);
    make_submenu(playlistwin_popup_menu, "/Add",
                 pladd_menu);
#endif
}

void
playlistwin_create(void)
{
    playlistwin_create_window();
    playlistwin_create_popup_menus();

    /* create GC and back pixmap for custom widget to draw on */
    playlistwin_gc = gdk_gc_new(playlistwin->window);
    playlistwin_bg = gdk_pixmap_new(playlistwin->window,
                                    playlistwin_get_width(),
                                    playlistwin_get_height_unshaded(), -1);
    gdk_window_set_back_pixmap(playlistwin->window, playlistwin_bg, 0);

    playlistwin_create_widgets();
    playlistwin_update_info();

    gtk_window_add_accel_group(GTK_WINDOW(playlistwin), playlistwin_accel);
    gtk_window_add_accel_group(GTK_WINDOW(playlistwin), mainwin_accel);
}


void
playlistwin_show(void)
{
    GtkWidget *item;

    item = gtk_item_factory_get_widget(mainwin_view_menu,
                                       "/Show Playlist Editor");
    gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(item), TRUE);

    tbutton_set_toggled(mainwin_pl, TRUE);
    cfg.playlist_visible = TRUE;

    playlistwin_set_toprow(0);
    playlist_check_pos_current();

    gtk_widget_show(playlistwin);
}

void
playlistwin_hide(void)
{
    GtkWidget *item;

    item = gtk_item_factory_get_widget(mainwin_view_menu,
                                       "/Show Playlist Editor");
    gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(item), FALSE);

    gtk_widget_hide(playlistwin);
    tbutton_set_toggled(mainwin_pl, FALSE);
    cfg.playlist_visible = FALSE;

    gtk_window_present(GTK_WINDOW(mainwin));
    gtk_widget_grab_focus(mainwin);
}


static void
plsort_menu_callback(gpointer data,
                     guint action,
                     GtkWidget * widget)
{
    switch (action) {
    case PLAYLISTWIN_SORT_BYPLAYLIST:
        playlist_sort(PLAYLIST_SORT_PLAYLIST);
        playlistwin_update_list();
        break;
    case PLAYLISTWIN_SORT_BYTRACK:
        playlist_sort(PLAYLIST_SORT_TRACK);
        playlistwin_update_list();
        break;
    case PLAYLISTWIN_SORT_BYTITLE:
        playlist_sort(PLAYLIST_SORT_TITLE);
        playlistwin_update_list();
        break;
    case PLAYLISTWIN_SORT_BYARTIST:
        playlist_sort(PLAYLIST_SORT_ARTIST);
        playlistwin_update_list();
        break;
    case PLAYLISTWIN_SORT_BYPATH:
        playlist_sort(PLAYLIST_SORT_PATH);
        playlistwin_update_list();
        break;
    case PLAYLISTWIN_SORT_BYDATE:
        playlist_sort(PLAYLIST_SORT_DATE);
        playlistwin_update_list();
        break;
    case PLAYLISTWIN_SORT_BYFILENAME:
        playlist_sort(PLAYLIST_SORT_FILENAME);
        playlistwin_update_list();
        break;
    case PLAYLISTWIN_SORT_SEL_BYPLAYLIST:
        playlist_sort_selected(PLAYLIST_SORT_PLAYLIST);
        playlistwin_update_list();
        break;
    case PLAYLISTWIN_SORT_SEL_BYTRACK:
        playlist_sort_selected(PLAYLIST_SORT_TRACK);
        playlistwin_update_list();
        break;
    case PLAYLISTWIN_SORT_SEL_BYTITLE:
        playlist_sort_selected(PLAYLIST_SORT_TITLE);
        playlistwin_update_list();
        break;
    case PLAYLISTWIN_SORT_SEL_BYARTIST:
        playlist_sort_selected(PLAYLIST_SORT_ARTIST);
        playlistwin_update_list();
        break;
    case PLAYLISTWIN_SORT_SEL_BYFILENAME:
        playlist_sort_selected(PLAYLIST_SORT_FILENAME);
        playlistwin_update_list();
        break;
    case PLAYLISTWIN_SORT_SEL_BYPATH:
        playlist_sort_selected(PLAYLIST_SORT_PATH);
        playlistwin_update_list();
        break;
    case PLAYLISTWIN_SORT_SEL_BYDATE:
        playlist_sort_selected(PLAYLIST_SORT_DATE);
        playlistwin_update_list();
        break;
    case PLAYLISTWIN_SORT_REVERSE:
        playlist_reverse();
        playlistwin_update_list();
        break;
    case PLAYLISTWIN_SORT_RANDOMIZE:
        playlist_random();
        playlistwin_update_list();
        break;
    }
}

static void
playlistwin_sub_menu_callback(gpointer data,
                              guint action,
                              GtkWidget * widget)
{
    switch (action) {
    case PLIST_NEW:
        playlist_set_current_name(NULL);
        playlist_clear();
        mainwin_clear_song_info();
        mainwin_set_info_text();
        break;
    case PLIST_SAVE:
        playlistwin_select_playlist_to_save(playlist_get_current_name());
        break;
    case PLIST_DEFAULTSAVE:
        playlist_save(bmp_paths[BMP_PATH_PLAYLIST_FILE]);
        break;
    case PLIST_SAVE_AS:
        playlistwin_select_playlist_to_save(playlist_get_current_name());
        break;
    case PLIST_LOAD:
        playlistwin_select_playlist_to_load(playlist_get_current_name());
        break;
    case SEL_INV:
        playlistwin_inverse_selection();
        break;
    case SEL_ZERO:
        playlistwin_select_none();
        break;
    case SEL_ALL:
        playlistwin_select_all();
        break;
    case SUB_ALL:
        playlist_clear();
        mainwin_clear_song_info();
        mainwin_set_info_text();
        break;
    case SUB_CROP:
        playlist_delete(TRUE);
        break;
    case SUB_SELECTED:
        playlist_delete(FALSE);
        break;
    case SUB_DUPLICATE_BYTITLE:
        playlist_remove_duplicates(PLAYLIST_DUPS_TITLE);
        break;
    case SUB_DUPLICATE_BYFILENAME:
        playlist_remove_duplicates(PLAYLIST_DUPS_FILENAME);
        break;
    case SUB_DUPLICATE_BYPATH:
        playlist_remove_duplicates(PLAYLIST_DUPS_PATH);
        break;
    case PLAYLISTWIN_REMOVE_DEAD_FILES:
        playlist_remove_dead_files();
        break;
    case PLAYLISTWIN_REFRESH:
        playlist_read_info_selection();
        playlistwin_update_list();
        break;
    }
}

static void
playlistwin_popup_menu_callback(gpointer data,
                                guint action,
                                GtkWidget * widget)
{
    extern GtkWidget *filepopupbutton;

    switch (action) {
    case ADD_FILES:
        playlistwin_show_filebrowser();
        break;
    case CLOSE_PL_WINDOW:
        playlistwin_hide();
        break;
    case MISC_FILEINFO:
        playlistwin_fileinfo();
        break;
    case SEL_LOOKUP:
        playlist_read_info_selection();
        break;
    case MISC_QUEUE:
        playlist_queue();
        break;
    case PLIST_CQUEUE:
        playlist_clear_queue();
        break;
    case PLIST_JTT:
        mainwin_jump_to_time();
        break;
    case PLIST_JTF:
        mainwin_jump_to_file();
        break;
    case MISC_FILEPOPUP:
        cfg.show_filepopup_for_tuple = gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(widget));
        if(filepopupbutton != NULL){
            gtk_signal_emit_by_name(GTK_OBJECT(filepopupbutton), "realize");
        }
        break;

    }
}
