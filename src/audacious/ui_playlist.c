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

#include "actions-playlist.h"
#include "dnd.h"
#include "dock.h"
#include "hints.h"
#include "input.h"
#include "main.h"
#include "playback.h"
#include "playlist.h"
#include "playlist_container.h"
#include "ui_playlist_manager.h"
#include "strings.h"
#include "ui_equalizer.h"
#include "ui_fileopener.h"
#include "ui_main.h"
#include "ui_manager.h"
#include "util.h"

#include "ui_skinned_window.h"
#include "ui_skinned_button.h"
#include "ui_skinned_textbox.h"
#include "ui_skinned_playlist_slider.h"
#include "ui_skinned_playlist.h"

#include "icons-stock.h"
#include "images/audacious_playlist.xpm"

GtkWidget *playlistwin;

static GMutex *resize_mutex = NULL;

GtkWidget *playlistwin_list = NULL;
GtkWidget *playlistwin_shade, *playlistwin_close;

static GdkBitmap *playlistwin_mask = NULL;

static gboolean playlistwin_hint_flag = FALSE;

static GtkWidget *playlistwin_slider = NULL;
static GtkWidget *playlistwin_time_min, *playlistwin_time_sec;
static GtkWidget *playlistwin_info, *playlistwin_sinfo;
static GtkWidget *playlistwin_srew, *playlistwin_splay;
static GtkWidget *playlistwin_spause, *playlistwin_sstop;
static GtkWidget *playlistwin_sfwd, *playlistwin_seject;
static GtkWidget *playlistwin_sscroll_up, *playlistwin_sscroll_down;

void playlistwin_select_search_cbt_cb( GtkWidget *called_cbt ,
                                              gpointer other_cbt );
static gboolean playlistwin_select_search_kp_cb( GtkWidget *entry , GdkEventKey *event ,
                                                 gpointer searchdlg_win );

static gboolean playlistwin_resizing = FALSE;
static gint playlistwin_resize_x, playlistwin_resize_y;

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
    gint height;
    cfg.playlist_height /= PLAYLISTWIN_HEIGHT_SNAP;
    cfg.playlist_height *= PLAYLISTWIN_HEIGHT_SNAP;
    height = cfg.playlist_height;
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
playlistwin_update_info(Playlist *playlist)
{
    gchar *text, *sel_text, *tot_text;
    gulong selection, total;
    gboolean selection_more, total_more;

    playlist_get_total_time(playlist, &total, &selection, &total_more, &selection_more);

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
    ui_skinned_textbox_set_text(playlistwin_info, text ? text : "");
    g_free(text);
    g_free(tot_text);
    g_free(sel_text);
}

static void
playlistwin_update_sinfo(Playlist *playlist)
{
    gchar *posstr, *timestr, *title, *info;
    gint pos, time;

    pos = playlist_get_position(playlist);
    title = playlist_get_songtitle(playlist, pos);

    if (!title) {
        ui_skinned_textbox_set_text(playlistwin_sinfo, "");
        return;
    }

    convert_title_text(title);

    time = playlist_get_songtime(playlist, pos);

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

    ui_skinned_textbox_set_text(playlistwin_sinfo, info ? info : "");
    g_free(info);
}

gboolean
playlistwin_item_visible(gint index)
{
    if (index >= UI_SKINNED_PLAYLIST(playlistwin_list)->first
        && index <
        (UI_SKINNED_PLAYLIST(playlistwin_list)->first + UI_SKINNED_PLAYLIST(playlistwin_list)->num_visible))
        return TRUE;
    return FALSE;
}

gint
playlistwin_list_get_visible_count(void)
{
    if (playlistwin_list)
    return UI_SKINNED_PLAYLIST(playlistwin_list)->num_visible;
    return (-1);
}

gint
playlistwin_list_get_first(void)
{
    if (playlistwin_list)
    return UI_SKINNED_PLAYLIST(playlistwin_list)->first;
    return (-1);
}

gint
playlistwin_get_toprow(void)
{
    if (playlistwin_list)
        return (UI_SKINNED_PLAYLIST(playlistwin_list)->first);
    return (-1);
}

void
playlistwin_set_toprow(gint toprow)
{
    if (playlistwin_list)
        UI_SKINNED_PLAYLIST(playlistwin_list)->first = toprow;
    playlistwin_update_list(playlist_get_active());
}

void
playlistwin_update_list(Playlist *playlist)
{
    /* this can happen early on. just bail gracefully. */
    g_return_if_fail(playlistwin_list);

    playlistwin_update_info(playlist);
    playlistwin_update_sinfo(playlist);
    gtk_widget_queue_draw(playlistwin_list);
    gtk_widget_queue_draw(playlistwin_slider);
}

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
    g_object_unref(gc);

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

    g_return_if_fail(font);

    tmp = g_strdup(font);
    g_return_if_fail(tmp);

    *strrchr(tmp, ' ') = '\0';
    tmp2 = g_strdup_printf("%s 8", tmp);
    g_return_if_fail(tmp2);

    ui_skinned_textbox_set_xfont(playlistwin_sinfo, cfg.mainwin_use_xfont, tmp2);

    g_free(tmp);
    g_free(tmp2);
}

void
playlistwin_set_sinfo_scroll(gboolean scroll)
{
    if(playlistwin_is_shaded())
        ui_skinned_textbox_set_scroll(playlistwin_sinfo, cfg.autoscroll);
    else
        ui_skinned_textbox_set_scroll(playlistwin_sinfo, FALSE);
}

void
playlistwin_set_shade(gboolean shaded)
{
    cfg.playlist_shaded = shaded;

    if (shaded) {
        playlistwin_set_sinfo_font(cfg.playlist_font);
        playlistwin_set_sinfo_scroll(cfg.autoscroll);
        gtk_widget_show(playlistwin_sinfo);
        ui_skinned_set_push_button_data(playlistwin_shade, 128, 45, 150, 42);
        ui_skinned_set_push_button_data(playlistwin_close, 138, 45, -1, -1);
    }
    else {
        gtk_widget_hide(playlistwin_sinfo);
        playlistwin_set_sinfo_scroll(FALSE);
        ui_skinned_set_push_button_data(playlistwin_shade, 157, 3, 62, 42);
        ui_skinned_set_push_button_data(playlistwin_close, 167, 3, -1, -1);
    }

    dock_shade(dock_window_list, GTK_WINDOW(playlistwin),
               playlistwin_get_height());

    playlistwin_set_geometry_hints(cfg.playlist_shaded);

    gtk_window_resize(GTK_WINDOW(playlistwin),
                      playlistwin_get_width(),
                      playlistwin_get_height());

    playlistwin_set_mask();
}

static void
playlistwin_set_shade_menu(gboolean shaded)
{
    GtkAction *action = gtk_action_group_get_action(
      toggleaction_group_others , "roll up playlist editor" );
    gtk_toggle_action_set_active( GTK_TOGGLE_ACTION(action) , shaded );

    playlistwin_set_shade(shaded);
    playlistwin_update_list(playlist_get_active());
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

    playlistwin_resizing = FALSE;

    if (dock_is_moving(GTK_WINDOW(playlistwin)))
       dock_move_release(GTK_WINDOW(playlistwin));
}

void
playlistwin_scroll(gint num)
{
    UI_SKINNED_PLAYLIST(playlistwin_list)->first += num;
    playlistwin_update_list(playlist_get_active());
}

void
playlistwin_scroll_up_pushed(void)
{
    playlistwin_scroll(-3);
}

void
playlistwin_scroll_down_pushed(void)
{
    playlistwin_scroll(3);
}

static void
playlistwin_select_all(void)
{
    Playlist *playlist = playlist_get_active();

    playlist_select_all(playlist, TRUE);
    UI_SKINNED_PLAYLIST(playlistwin_list)->prev_selected = 0;
    UI_SKINNED_PLAYLIST(playlistwin_list)->prev_min = 0;
    UI_SKINNED_PLAYLIST(playlistwin_list)->prev_max = playlist_get_length(playlist) - 1;
    playlistwin_update_list(playlist);
}

static void
playlistwin_select_none(void)
{
    playlist_select_all(playlist_get_active(), FALSE);
    UI_SKINNED_PLAYLIST(playlistwin_list)->prev_selected = -1;
    UI_SKINNED_PLAYLIST(playlistwin_list)->prev_min = -1;
    playlistwin_update_list(playlist_get_active());
}

static void
playlistwin_select_search(void)
{
    Playlist *playlist = playlist_get_active();
    GtkWidget *searchdlg_win, *searchdlg_table;
    GtkWidget *searchdlg_hbox, *searchdlg_logo, *searchdlg_helptext;
    GtkWidget *searchdlg_entry_title, *searchdlg_label_title;
    GtkWidget *searchdlg_entry_album, *searchdlg_label_album;
    GtkWidget *searchdlg_entry_file_name, *searchdlg_label_file_name;
    GtkWidget *searchdlg_entry_performer, *searchdlg_label_performer;
    GtkWidget *searchdlg_checkbt_clearprevsel;
    GtkWidget *searchdlg_checkbt_newplaylist;
    GtkWidget *searchdlg_checkbt_autoenqueue;
    gint result;

    /* create dialog */
    searchdlg_win = gtk_dialog_new_with_buttons(
      _("Search entries in active playlist") , GTK_WINDOW(mainwin) ,
      GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT ,
      GTK_STOCK_CANCEL , GTK_RESPONSE_REJECT , GTK_STOCK_OK , GTK_RESPONSE_ACCEPT , NULL );
    gtk_window_set_position(GTK_WINDOW(searchdlg_win), GTK_WIN_POS_CENTER);

    /* help text and logo */
    searchdlg_hbox = gtk_hbox_new( FALSE , 4 );
    searchdlg_logo = gtk_image_new_from_stock( GTK_STOCK_FIND , GTK_ICON_SIZE_DIALOG );
    searchdlg_helptext = gtk_label_new( _("Select entries in playlist by filling one or more "
      "fields. Fields use regular expressions syntax, case-insensitive. If you don't know how "
      "regular expressions work, simply insert a literal portion of what you're searching for.") );
    gtk_label_set_line_wrap( GTK_LABEL(searchdlg_helptext) , TRUE );
    gtk_box_pack_start( GTK_BOX(searchdlg_hbox) , searchdlg_logo , FALSE , FALSE , 0 );
    gtk_box_pack_start( GTK_BOX(searchdlg_hbox) , searchdlg_helptext , FALSE , FALSE , 0 );

    /* title */
    searchdlg_label_title = gtk_label_new( _("Title: ") );
    searchdlg_entry_title = gtk_entry_new();
    gtk_misc_set_alignment( GTK_MISC(searchdlg_label_title) , 0 , 0.5 );
    g_signal_connect( G_OBJECT(searchdlg_entry_title) , "key-press-event" ,
      G_CALLBACK(playlistwin_select_search_kp_cb) , searchdlg_win );

    /* album */
    searchdlg_label_album= gtk_label_new( _("Album: ") );
    searchdlg_entry_album= gtk_entry_new();
    gtk_misc_set_alignment( GTK_MISC(searchdlg_label_album) , 0 , 0.5 );
    g_signal_connect( G_OBJECT(searchdlg_entry_album) , "key-press-event" ,
      G_CALLBACK(playlistwin_select_search_kp_cb) , searchdlg_win );

    /* artist */
    searchdlg_label_performer = gtk_label_new( _("Artist: ") );
    searchdlg_entry_performer = gtk_entry_new();
    gtk_misc_set_alignment( GTK_MISC(searchdlg_label_performer) , 0 , 0.5 );
    g_signal_connect( G_OBJECT(searchdlg_entry_performer) , "key-press-event" ,
      G_CALLBACK(playlistwin_select_search_kp_cb) , searchdlg_win );

    /* file name */
    searchdlg_label_file_name = gtk_label_new( _("Filename: ") );
    searchdlg_entry_file_name = gtk_entry_new();
    gtk_misc_set_alignment( GTK_MISC(searchdlg_label_file_name) , 0 , 0.5 );
    g_signal_connect( G_OBJECT(searchdlg_entry_file_name) , "key-press-event" ,
      G_CALLBACK(playlistwin_select_search_kp_cb) , searchdlg_win );

    /* some options that control behaviour */
    searchdlg_checkbt_clearprevsel = gtk_check_button_new_with_label(
      _("Clear previous selection before searching") );
    gtk_toggle_button_set_active( GTK_TOGGLE_BUTTON(searchdlg_checkbt_clearprevsel) , TRUE );
    searchdlg_checkbt_autoenqueue = gtk_check_button_new_with_label(
      _("Automatically toggle queue for matching entries") );
    gtk_toggle_button_set_active( GTK_TOGGLE_BUTTON(searchdlg_checkbt_autoenqueue) , FALSE );
    searchdlg_checkbt_newplaylist = gtk_check_button_new_with_label(
      _("Create a new playlist with matching entries") );
    gtk_toggle_button_set_active( GTK_TOGGLE_BUTTON(searchdlg_checkbt_newplaylist) , FALSE );
    g_signal_connect( G_OBJECT(searchdlg_checkbt_autoenqueue) , "clicked" ,
      G_CALLBACK(playlistwin_select_search_cbt_cb) , searchdlg_checkbt_newplaylist );
    g_signal_connect( G_OBJECT(searchdlg_checkbt_newplaylist) , "clicked" ,
      G_CALLBACK(playlistwin_select_search_cbt_cb) , searchdlg_checkbt_autoenqueue );

    /* place fields in searchdlg_table */
    searchdlg_table = gtk_table_new( 8 , 2 , FALSE );
    gtk_table_set_row_spacing( GTK_TABLE(searchdlg_table) , 0 , 8 );
    gtk_table_set_row_spacing( GTK_TABLE(searchdlg_table) , 4 , 8 );
    gtk_table_attach( GTK_TABLE(searchdlg_table) , searchdlg_hbox ,
      0 , 2 , 0 , 1 , GTK_FILL | GTK_EXPAND , GTK_FILL | GTK_EXPAND , 0 , 2 );
    gtk_table_attach( GTK_TABLE(searchdlg_table) , searchdlg_label_title ,
      0 , 1 , 1 , 2 , GTK_FILL , GTK_FILL | GTK_EXPAND , 0 , 2 );
    gtk_table_attach( GTK_TABLE(searchdlg_table) , searchdlg_entry_title ,
      1 , 2 , 1 , 2 , GTK_FILL | GTK_EXPAND , GTK_FILL | GTK_EXPAND , 0 , 2 );
    gtk_table_attach( GTK_TABLE(searchdlg_table) , searchdlg_label_album,
      0 , 1 , 2 , 3 , GTK_FILL , GTK_FILL | GTK_EXPAND , 0 , 2 );
    gtk_table_attach( GTK_TABLE(searchdlg_table) , searchdlg_entry_album,
      1 , 2 , 2 , 3 , GTK_FILL | GTK_EXPAND , GTK_FILL | GTK_EXPAND , 0 , 2 );
    gtk_table_attach( GTK_TABLE(searchdlg_table) , searchdlg_label_performer ,
      0 , 1 , 3 , 4 , GTK_FILL , GTK_FILL | GTK_EXPAND , 0 , 2 );
    gtk_table_attach( GTK_TABLE(searchdlg_table) , searchdlg_entry_performer ,
      1 , 2 , 3 , 4 , GTK_FILL | GTK_EXPAND , GTK_FILL | GTK_EXPAND , 0 , 2 );
    gtk_table_attach( GTK_TABLE(searchdlg_table) , searchdlg_label_file_name ,
      0 , 1 , 4 , 5 , GTK_FILL , GTK_FILL | GTK_EXPAND , 0 , 2 );
    gtk_table_attach( GTK_TABLE(searchdlg_table) , searchdlg_entry_file_name ,
      1 , 2 , 4 , 5 , GTK_FILL | GTK_EXPAND , GTK_FILL | GTK_EXPAND , 0 , 2 );
    gtk_table_attach( GTK_TABLE(searchdlg_table) , searchdlg_checkbt_clearprevsel ,
      0 , 2 , 5 , 6 , GTK_FILL | GTK_EXPAND , GTK_FILL | GTK_EXPAND , 0 , 1 );
    gtk_table_attach( GTK_TABLE(searchdlg_table) , searchdlg_checkbt_autoenqueue ,
      0 , 2 , 6 , 7 , GTK_FILL | GTK_EXPAND , GTK_FILL | GTK_EXPAND , 0 , 1 );
    gtk_table_attach( GTK_TABLE(searchdlg_table) , searchdlg_checkbt_newplaylist ,
      0 , 2 , 7 , 8 , GTK_FILL | GTK_EXPAND , GTK_FILL | GTK_EXPAND , 0 , 1 );

    gtk_container_set_border_width( GTK_CONTAINER(searchdlg_table) , 5 );
    gtk_container_add( GTK_CONTAINER(GTK_DIALOG(searchdlg_win)->vbox) , searchdlg_table );
    gtk_widget_show_all( searchdlg_win );
    result = gtk_dialog_run( GTK_DIALOG(searchdlg_win) );
    switch(result)
    {
      case GTK_RESPONSE_ACCEPT:
      {
         gint matched_entries_num = 0;
         /* create a TitleInput tuple with user search data */
         Tuple *tuple = tuple_new();
         gchar *searchdata = NULL;
         searchdata = (gchar*)gtk_entry_get_text( GTK_ENTRY(searchdlg_entry_title) );
         tuple_associate_string(tuple, "title", searchdata);
         searchdata = (gchar*)gtk_entry_get_text( GTK_ENTRY(searchdlg_entry_album) );
         tuple_associate_string(tuple, "album", searchdata);
         searchdata = (gchar*)gtk_entry_get_text( GTK_ENTRY(searchdlg_entry_performer) );
         tuple_associate_string(tuple, "artist", searchdata);
         searchdata = (gchar*)gtk_entry_get_text( GTK_ENTRY(searchdlg_entry_file_name) );
         tuple_associate_string(tuple, "file-name", searchdata);
         /* check if previous selection should be cleared before searching */
         if ( gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(searchdlg_checkbt_clearprevsel)) == TRUE )
             playlistwin_select_none();
         /* now send this tuple to the real search function */
         matched_entries_num = playlist_select_search( playlist , tuple , 0 );
         /* we do not need the tuple and its data anymore */
         mowgli_object_unref(tuple);
         playlistwin_update_list(playlist_get_active());
         /* check if a new playlist should be created after searching */
         if ( gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(searchdlg_checkbt_newplaylist)) == TRUE )
             playlist_new_from_selected();
         /* check if matched entries should be queued */
         else if ( gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(searchdlg_checkbt_autoenqueue)) == TRUE )
             playlist_queue(playlist_get_active());
         break;
      }
      default:
         break;
    }
    /* done here :) */
    gtk_widget_destroy( searchdlg_win );
}

static void
playlistwin_inverse_selection(void)
{
    playlist_select_invert_all(playlist_get_active());
    UI_SKINNED_PLAYLIST(playlistwin_list)->prev_selected = -1;
    UI_SKINNED_PLAYLIST(playlistwin_list)->prev_min = -1;
    playlistwin_update_list(playlist_get_active());
}

static void
playlistwin_resize(gint width, gint height)
{
    gint tx, ty;
    gint dx, dy;

    g_return_if_fail(width > 0 && height > 0);

    tx = (width - PLAYLISTWIN_MIN_WIDTH) / PLAYLISTWIN_WIDTH_SNAP;
    tx = (tx * PLAYLISTWIN_WIDTH_SNAP) + PLAYLISTWIN_MIN_WIDTH;
    if (tx < PLAYLISTWIN_MIN_WIDTH)
        tx = PLAYLISTWIN_MIN_WIDTH;

    if (!cfg.playlist_shaded)
    {
        ty = (height - PLAYLISTWIN_MIN_HEIGHT) / PLAYLISTWIN_HEIGHT_SNAP;
        ty = (ty * PLAYLISTWIN_HEIGHT_SNAP) + PLAYLISTWIN_MIN_HEIGHT;
        if (ty < PLAYLISTWIN_MIN_HEIGHT)
            ty = PLAYLISTWIN_MIN_HEIGHT;
    }
    else
        ty = cfg.playlist_height;

    if (tx == cfg.playlist_width && ty == cfg.playlist_height)
        return;

    /* difference between previous size and new size */
    dx = tx - cfg.playlist_width;
    dy = ty - cfg.playlist_height;

    cfg.playlist_width = width = tx;
    cfg.playlist_height = height = ty;

    g_mutex_lock(resize_mutex);
    ui_skinned_playlist_resize_relative(playlistwin_list, dx, dy);

    ui_skinned_playlist_slider_move_relative(playlistwin_slider, dx);
    ui_skinned_playlist_slider_resize_relative(playlistwin_slider, dy);

    playlistwin_update_sinfo(playlist_get_active());

    ui_skinned_button_move_relative(playlistwin_shade, dx, 0);
    ui_skinned_button_move_relative(playlistwin_close, dx, 0);
    ui_skinned_textbox_move_relative(playlistwin_time_min, dx, dy);
    ui_skinned_textbox_move_relative(playlistwin_time_sec, dx, dy);
    ui_skinned_textbox_move_relative(playlistwin_info, dx, dy);
    ui_skinned_button_move_relative(playlistwin_srew, dx, dy);
    ui_skinned_button_move_relative(playlistwin_splay, dx, dy);
    ui_skinned_button_move_relative(playlistwin_spause, dx, dy);
    ui_skinned_button_move_relative(playlistwin_sstop, dx, dy);
    ui_skinned_button_move_relative(playlistwin_sfwd, dx, dy);
    ui_skinned_button_move_relative(playlistwin_seject, dx, dy);
    ui_skinned_button_move_relative(playlistwin_sscroll_up, dx, dy);
    ui_skinned_button_move_relative(playlistwin_sscroll_down, dx, dy);

    playlistwin_set_mask();

    gtk_widget_set_size_request(playlistwin_sinfo, playlistwin_get_width() - 35,
                                bmp_active_skin->properties.textbox_bitmap_font_height);
    GList *iter;
    for (iter = GTK_FIXED (SKINNED_WINDOW(playlistwin)->fixed)->children; iter; iter = g_list_next (iter)) {
         GtkFixedChild *child_data = (GtkFixedChild *) iter->data;
         GtkWidget *child = child_data->widget;
         g_signal_emit_by_name(child, "redraw");
    }
    g_mutex_unlock(resize_mutex);
}

static void
playlistwin_motion(GtkWidget * widget,
                   GdkEventMotion * event,
                   gpointer callback_data)
{
    GdkEvent *gevent;

    /*
     * GDK2's resize is broken and doesn't really play nice, so we have
     * to do all of this stuff by hand.
     */
    if (playlistwin_resizing == TRUE)
    {
        if (event->x + playlistwin_resize_x != playlistwin_get_width() ||
            event->y + playlistwin_resize_y != playlistwin_get_height())
        {
            playlistwin_resize(event->x + playlistwin_resize_x,
                               event->y + playlistwin_resize_y);
        }
        gdk_window_resize(playlistwin->window,
                          cfg.playlist_width, cfg.playlist_height);
    }
    else if (dock_is_moving(GTK_WINDOW(playlistwin)))
        dock_move_motion(GTK_WINDOW(playlistwin), event);
    gdk_flush();

    while ((gevent = gdk_event_get()) != NULL) gdk_event_free(gevent);
}

static void
playlistwin_show_filebrowser(void)
{
    run_filebrowser(NO_PLAY_BUTTON);
}

static void
playlistwin_fileinfo(void)
{
    Playlist *playlist = playlist_get_active();

    /* Show the first selected file, or the current file if nothing is
     * selected */
    GList *list = playlist_get_selected(playlist);
    if (list) {
        playlist_fileinfo(playlist, GPOINTER_TO_INT(list->data));
        g_list_free(list);
    }
    else
        playlist_fileinfo_current(playlist);
}

static void
show_playlist_save_error(GtkWindow *parent,
                         const gchar *filename)
{
    GtkWidget *dialog;

    g_return_if_fail(GTK_IS_WINDOW(parent));
    g_return_if_fail(filename);

    dialog = gtk_message_dialog_new(GTK_WINDOW(parent),
                                    GTK_DIALOG_DESTROY_WITH_PARENT,
                                    GTK_MESSAGE_ERROR,
                                    GTK_BUTTONS_OK,
                                    _("Error writing playlist \"%s\": %s"),
                                    filename, strerror(errno));

    gtk_window_set_position(GTK_WINDOW(dialog), GTK_WIN_POS_CENTER); /* centering */
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

    gtk_window_set_position(GTK_WINDOW(dialog), GTK_WIN_POS_CENTER); /* centering */
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

    gtk_window_set_position(GTK_WINDOW(dialog), GTK_WIN_POS_CENTER); /* centering */
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

    if (!playlist_save(playlist_get_active(), filename))
        show_playlist_save_error(GTK_WINDOW(playlistwin), filename);
}

static void
playlistwin_load_playlist(const gchar * filename)
{
    Playlist *playlist = playlist_get_active();

    g_return_if_fail(filename != NULL);

    str_replace_in(&cfg.playlist_path, g_path_get_dirname(filename));

    playlist_clear(playlist);
    mainwin_clear_song_info();

    playlist_load(playlist, filename);
    playlist_set_current_name(playlist, filename);
}

static gchar *
playlist_file_selection_load(const gchar * title,
                        const gchar * default_filename)
{
    GtkWidget *dialog;
    gchar *filename;

    g_return_val_if_fail(title != NULL, NULL);

    dialog = make_filebrowser(title, FALSE);
    gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(dialog), cfg.playlist_path);
    if (default_filename)
        gtk_file_chooser_set_filename(GTK_FILE_CHOOSER(dialog), default_filename);
    gtk_window_set_position(GTK_WINDOW(dialog), GTK_WIN_POS_CENTER); /* centering */

    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT)
        filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
    else
        filename = NULL;

    gtk_widget_destroy(dialog);
    return filename;
}

static void
on_static_toggle(GtkToggleButton *button, gpointer data)
{
    Playlist *playlist = playlist_get_active();

    playlist->attribute =
        gtk_toggle_button_get_active(button) ?
        playlist->attribute | PLAYLIST_STATIC :
        playlist->attribute & ~PLAYLIST_STATIC;
}

static void
on_relative_toggle(GtkToggleButton *button, gpointer data)
{
    Playlist *playlist = playlist_get_active();

    playlist->attribute =
        gtk_toggle_button_get_active(button) ?
        playlist->attribute | PLAYLIST_USE_RELATIVE :
        playlist->attribute & ~PLAYLIST_USE_RELATIVE;
}

static gchar *
playlist_file_selection_save(const gchar * title,
                        const gchar * default_filename)
{
    GtkWidget *dialog;
    gchar *filename;
    GtkWidget *hbox;
    GtkWidget *toggle, *toggle2;

    g_return_val_if_fail(title != NULL, NULL);

    dialog = make_filebrowser(title, TRUE);
    gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(dialog), cfg.playlist_path);
    gtk_file_chooser_set_filename(GTK_FILE_CHOOSER(dialog), default_filename);

    hbox = gtk_hbox_new(FALSE, 5);

    /* static playlist */
    toggle = gtk_check_button_new_with_label(_("Save as Static Playlist"));
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(toggle),
                                 (playlist_get_active()->attribute & PLAYLIST_STATIC) ? TRUE : FALSE);
    g_signal_connect(G_OBJECT(toggle), "toggled", G_CALLBACK(on_static_toggle), dialog);
    gtk_box_pack_start(GTK_BOX(hbox), toggle, FALSE, FALSE, 0);

    /* use relative path */
    toggle2 = gtk_check_button_new_with_label(_("Use Relative Path"));
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(toggle2),
                                 (playlist_get_active()->attribute & PLAYLIST_USE_RELATIVE) ? TRUE : FALSE);
    g_signal_connect(G_OBJECT(toggle2), "toggled", G_CALLBACK(on_relative_toggle), dialog);
    gtk_box_pack_start(GTK_BOX(hbox), toggle2, FALSE, FALSE, 0);

    gtk_widget_show_all(hbox);
    gtk_file_chooser_set_extra_widget(GTK_FILE_CHOOSER(dialog), hbox);

    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT)
        filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
    else
        filename = NULL;

    gtk_widget_destroy(dialog);
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

    // deactivating this fixed a gui freeze when scrolling. -- mf0102
    //g_cond_signal(cond_scan);

}

static gboolean
playlistwin_press(GtkWidget * widget,
                  GdkEventButton * event,
                  gpointer callback_data)
{
    gint xpos, ypos;
    GtkRequisition req;

    gtk_window_get_position(GTK_WINDOW(playlistwin), &xpos, &ypos);

    if (event->button == 1 && !cfg.show_wm_decorations &&
        ((!cfg.playlist_shaded &&
          event->x > playlistwin_get_width() - 20 &&
          event->y > cfg.playlist_height - 20) ||
         (cfg.playlist_shaded &&
          event->x >= playlistwin_get_width() - 31 &&
          event->x < playlistwin_get_width() - 22))) {

        if (event->type != GDK_2BUTTON_PRESS &&
            event->type != GDK_3BUTTON_PRESS) {
            playlistwin_resizing = TRUE;
            playlistwin_resize_x = cfg.playlist_width - event->x;
            playlistwin_resize_y = cfg.playlist_height - event->y;
        }
    }
    else if (event->button == 1 && REGION_L(12, 37, 29, 11)) {
        /* ADD button menu */
        gtk_widget_size_request(playlistwin_pladd_menu, &req);
        ui_manager_popup_menu_show(GTK_MENU(playlistwin_pladd_menu),
                   xpos + 12,
                   (ypos + playlistwin_get_height()) - 8 - req.height,
                   event->button,
                   event->time);
    }
    else if (event->button == 1 && REGION_L(41, 66, 29, 11)) {
        /* SUB button menu */
        gtk_widget_size_request(playlistwin_pldel_menu, &req);
        ui_manager_popup_menu_show(GTK_MENU(playlistwin_pldel_menu),
                   xpos + 40,
                   (ypos + playlistwin_get_height()) - 8 - req.height,
                   event->button,
                   event->time);
    }
    else if (event->button == 1 && REGION_L(70, 95, 29, 11)) {
        /* SEL button menu */
        gtk_widget_size_request(playlistwin_plsel_menu, &req);
        ui_manager_popup_menu_show(GTK_MENU(playlistwin_plsel_menu),
                   xpos + 68,
                   (ypos + playlistwin_get_height()) - 8 - req.height,
                   event->button,
                   event->time);
    }
    else if (event->button == 1 && REGION_L(99, 124, 29, 11)) {
        /* MISC button menu */
        gtk_widget_size_request(playlistwin_plsort_menu, &req);
        ui_manager_popup_menu_show(GTK_MENU(playlistwin_plsort_menu),
                   xpos + 100,
                   (ypos + playlistwin_get_height()) - 8 - req.height,
                   event->button,
                   event->time);
    }
    else if (event->button == 1 && REGION_R(46, 23, 29, 11)) {
        /* LIST button menu */
        gtk_widget_size_request(playlistwin_pllist_menu, &req);
        ui_manager_popup_menu_show(GTK_MENU(playlistwin_pllist_menu),
                   xpos + playlistwin_get_width() - req.width - 12,
                   (ypos + playlistwin_get_height()) - 8 - req.height,
                   event->button,
                   event->time);
    }
    else if (event->button == 1 && event->type == GDK_BUTTON_PRESS &&
             (cfg.easy_move || event->y < 14))
    {
        dock_move_press(dock_window_list, GTK_WINDOW(playlistwin), event,
                        FALSE);
        gtk_window_present(GTK_WINDOW(playlistwin));
    }
    else if (event->button == 1 && event->type == GDK_2BUTTON_PRESS
             && event->y < 14) {
        /* double click on title bar */
        playlistwin_shade_toggle();
        if (dock_is_moving(GTK_WINDOW(playlistwin)))
            dock_move_release(GTK_WINDOW(playlistwin));
        return TRUE;
    }
    else if (event->button == 3) {
        /*
         * Pop up the main menu a few pixels down to avoid
         * anything to be selected initially.
         */
        ui_manager_popup_menu_show(GTK_MENU(mainwin_general_menu), event->x_root,
                                event->y_root + 2, 3, event->time);
    }

    return FALSE;
}

static gboolean
playlistwin_delete(GtkWidget * w, gpointer data)
{
    playlistwin_hide();
    return TRUE;
}

static gboolean
playlistwin_keypress_up_down_handler(UiSkinnedPlaylist * pl,
                                     gboolean up, guint state)
{
    Playlist *playlist = playlist_get_active();
    if ((!(pl->prev_selected || pl->first) && up) ||
       ((pl->prev_selected >= playlist_get_length(playlist) - 1) && !up))
         return FALSE;

    if ((state & GDK_MOD1_MASK) && (state & GDK_SHIFT_MASK))
        return FALSE;
    if (!(state & GDK_MOD1_MASK))
        playlist_select_all(playlist, FALSE);

    if (pl->prev_selected == -1 ||
        (!playlistwin_item_visible(pl->prev_selected) &&
         !(state & GDK_SHIFT_MASK && pl->prev_min != -1))) {
        pl->prev_selected = pl->first;
    }
    else if (state & GDK_SHIFT_MASK) {
        if (pl->prev_min == -1) {
            pl->prev_max = pl->prev_selected;
            pl->prev_min = pl->prev_selected;
        }
        pl->prev_max += (up ? -1 : 1);
        pl->prev_max =
            CLAMP(pl->prev_max, 0, playlist_get_length(playlist) - 1);

        pl->first = MIN(pl->first, pl->prev_max);
        pl->first = MAX(pl->first, pl->prev_max -
                           pl->num_visible + 1);
        playlist_select_range(playlist, pl->prev_min, pl->prev_max, TRUE);
        return TRUE;
    }
    else if (state & GDK_MOD1_MASK) {
        if (up)
            ui_skinned_playlist_move_up(pl);
        else
            ui_skinned_playlist_move_down(pl);
        if (pl->prev_min < pl->first)
            pl->first = pl->prev_min;
        else if (pl->prev_max >= (pl->first + pl->num_visible))
            pl->first = pl->prev_max - pl->num_visible + 1;
        return TRUE;
    }
    else if (up)
        pl->prev_selected--;
    else
        pl->prev_selected++;

    pl->prev_selected =
        CLAMP(pl->prev_selected, 0, playlist_get_length(playlist) - 1);

    if (pl->prev_selected < pl->first)
        pl->first--;
    else if (pl->prev_selected >= (pl->first + pl->num_visible))
        pl->first++;

    playlist_select_range(playlist, pl->prev_selected, pl->prev_selected, TRUE);
    pl->prev_min = -1;

    return TRUE;
}

/* FIXME: Handle the keys through menu */

static gboolean
playlistwin_keypress(GtkWidget * w, GdkEventKey * event, gpointer data)
{
    Playlist *playlist = playlist_get_active();

    guint keyval;
    gboolean refresh = FALSE;

    if (cfg.playlist_shaded)
        return FALSE;

    switch (keyval = event->keyval) {
    case GDK_KP_Up:
    case GDK_KP_Down:
    case GDK_Up:
    case GDK_Down:
        refresh = playlistwin_keypress_up_down_handler(UI_SKINNED_PLAYLIST(playlistwin_list),
                                                       keyval == GDK_Up
                                                       || keyval == GDK_KP_Up,
                                                       event->state);
        break;
    case GDK_Page_Up:
        playlistwin_scroll(-UI_SKINNED_PLAYLIST(playlistwin_list)->num_visible);
        refresh = TRUE;
        break;
    case GDK_Page_Down:
        playlistwin_scroll(UI_SKINNED_PLAYLIST(playlistwin_list)->num_visible);
        refresh = TRUE;
        break;
    case GDK_Home:
        UI_SKINNED_PLAYLIST(playlistwin_list)->first = 0;
        refresh = TRUE;
        break;
    case GDK_End:
        UI_SKINNED_PLAYLIST(playlistwin_list)->first =
            playlist_get_length(playlist) - UI_SKINNED_PLAYLIST(playlistwin_list)->num_visible;
        refresh = TRUE;
        break;
    case GDK_Return:
        if (UI_SKINNED_PLAYLIST(playlistwin_list)->prev_selected > -1
            && playlistwin_item_visible(UI_SKINNED_PLAYLIST(playlistwin_list)->prev_selected)) {
            playlist_set_position(playlist, UI_SKINNED_PLAYLIST(playlistwin_list)->prev_selected);
            if (!playback_get_playing())
                playback_initiate();
        }
        refresh = TRUE;
        break;
    case GDK_3:
        if (event->state & GDK_CONTROL_MASK)
            playlistwin_fileinfo();
        break;
    case GDK_Delete:
        if (event->state & GDK_CONTROL_MASK)
            playlist_delete(playlist, TRUE);
        else
            playlist_delete(playlist, FALSE);
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
        if (playlist_get_current_length(playlist) != -1)
            playback_seek(CLAMP
                              (playback_get_time() - 5000, 0,
                              playlist_get_current_length(playlist)) / 1000);
        break;
    case GDK_Right:
    case GDK_KP_Right:
    case GDK_KP_9:
        if (playlist_get_current_length(playlist) != -1)
            playback_seek(CLAMP
                              (playback_get_time() + 5000, 0,
                              playlist_get_current_length(playlist)) / 1000);
        break;
    case GDK_KP_4:
        playlist_prev(playlist);
        break;
    case GDK_KP_6:
        playlist_next(playlist);
        break;

    case GDK_Escape:
        mainwin_minimize_cb();
        break;
    case GDK_Tab:
        if (event->state & GDK_CONTROL_MASK)
            gtk_window_present(GTK_WINDOW(mainwin));
        break;
    default:
        return FALSE;
    }

    if (refresh) {
        // fixes keyboard scrolling gui freeze for me. -- mf0102
        //g_cond_signal(cond_scan);
        playlistwin_update_list(playlist_get_active());
    }

    return TRUE;
}

void
playlistwin_hide_timer(void)
{
    ui_skinned_textbox_set_text(playlistwin_time_min, "   ");
    ui_skinned_textbox_set_text(playlistwin_time_sec, "  ");
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
    ui_skinned_textbox_set_text(playlistwin_time_min, text);
    g_free(text);

    text = g_strdup_printf("%-2.2d", time % 60);
    ui_skinned_textbox_set_text(playlistwin_time_sec, text);
    g_free(text);
}

static void
playlistwin_drag_motion(GtkWidget * widget,
                        GdkDragContext * context,
                        gint x, gint y,
                        GtkSelectionData * selection_data,
                        guint info, guint time, gpointer user_data)
{
    UI_SKINNED_PLAYLIST(playlistwin_list)->drag_motion = TRUE;
    UI_SKINNED_PLAYLIST(playlistwin_list)->drag_motion_x = x;
    UI_SKINNED_PLAYLIST(playlistwin_list)->drag_motion_y = y;
    playlistwin_update_list(playlist_get_active());
    playlistwin_hint_flag = TRUE;
}

static void
playlistwin_drag_end(GtkWidget * widget,
                     GdkDragContext * context, gpointer user_data)
{
    UI_SKINNED_PLAYLIST(playlistwin_list)->drag_motion = FALSE;
    playlistwin_hint_flag = FALSE;
    playlistwin_update_list(playlist_get_active());
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
    Playlist *playlist = playlist_get_active();

    g_return_if_fail(selection_data);

    if (!selection_data->data) {
        g_message("Received no DND data!");
        return;
    }
    if (x < playlistwin_get_width() - 20 || y < cfg.playlist_height - 38) {
        pos = y / UI_SKINNED_PLAYLIST(playlistwin_list)->fheight + UI_SKINNED_PLAYLIST(playlistwin_list)->first;

        pos = MIN(pos, playlist_get_length(playlist));
        playlist_ins_url(playlist, (gchar *) selection_data->data, pos);
    }
    else
        playlist_add_url(playlist, (gchar *) selection_data->data);
}

static void
local_playlist_prev(void)
{
    playlist_prev(playlist_get_active());
}

static void
local_playlist_next(void)
{
    playlist_next(playlist_get_active());
}

static void
playlistwin_create_widgets(void)
{
    /* This function creates the custom widgets used by the playlist editor */

    /* text box for displaying song title in shaded mode */
    playlistwin_sinfo = ui_skinned_textbox_new(SKINNED_WINDOW(playlistwin)->fixed,
                                               4, 4, playlistwin_get_width() - 35, TRUE, SKIN_TEXT);

    playlistwin_set_sinfo_font(cfg.playlist_font);

    playlistwin_shade = ui_skinned_button_new();
    /* shade/unshade window push button */
    if (cfg.playlist_shaded)
        ui_skinned_push_button_setup(playlistwin_shade, SKINNED_WINDOW(playlistwin)->fixed,
                                     playlistwin_get_width() - 21, 3,
                                     9, 9, 128, 45, 150, 42, SKIN_PLEDIT);
    else
        ui_skinned_push_button_setup(playlistwin_shade, SKINNED_WINDOW(playlistwin)->fixed,
                                     playlistwin_get_width() - 21, 3,
                                     9, 9, 157, 3, 62, 42, SKIN_PLEDIT);

    g_signal_connect(playlistwin_shade, "clicked", playlistwin_shade_toggle, NULL );

    /* close window push button */
    playlistwin_close = ui_skinned_button_new();
    ui_skinned_push_button_setup(playlistwin_close, SKINNED_WINDOW(playlistwin)->fixed,
                                 playlistwin_get_width() - 11, 3, 9, 9,
                                 cfg.playlist_shaded ? 138 : 167,
                                 cfg.playlist_shaded ? 45 : 3, 52, 42, SKIN_PLEDIT);

    g_signal_connect(playlistwin_close, "clicked", playlistwin_hide, NULL );

    /* playlist list box */
    playlistwin_list = ui_skinned_playlist_new(SKINNED_WINDOW(playlistwin)->fixed, 12, 20,
                             playlistwin_get_width() - 31,
                             cfg.playlist_height - 58);
    ui_skinned_playlist_set_font(cfg.playlist_font);

    /* playlist list box slider */
    playlistwin_slider = ui_skinned_playlist_slider_new(SKINNED_WINDOW(playlistwin)->fixed, playlistwin_get_width() - 15,
                              20, cfg.playlist_height - 58);

    /* track time (minute) */
    playlistwin_time_min = ui_skinned_textbox_new(SKINNED_WINDOW(playlistwin)->fixed,
                       playlistwin_get_width() - 82,
                       cfg.playlist_height - 15, 15, FALSE, SKIN_TEXT);
    g_signal_connect(playlistwin_time_min, "button-press-event", G_CALLBACK(change_timer_mode_cb), NULL);

    /* track time (second) */
    playlistwin_time_sec = ui_skinned_textbox_new(SKINNED_WINDOW(playlistwin)->fixed,
                       playlistwin_get_width() - 64,
                       cfg.playlist_height - 15, 10, FALSE, SKIN_TEXT);
    g_signal_connect(playlistwin_time_sec, "button-press-event", G_CALLBACK(change_timer_mode_cb), NULL);

    /* playlist information (current track length / total track length) */
    playlistwin_info = ui_skinned_textbox_new(SKINNED_WINDOW(playlistwin)->fixed,
                       playlistwin_get_width() - 143,
                       cfg.playlist_height - 28, 90, FALSE, SKIN_TEXT);

    /* mini play control buttons at right bottom corner */

    /* rewind button */
    playlistwin_srew = ui_skinned_button_new();
    ui_skinned_small_button_setup(playlistwin_srew, SKINNED_WINDOW(playlistwin)->fixed,
                                  playlistwin_get_width() - 144,
                                  cfg.playlist_height - 16, 8, 7);
    g_signal_connect(playlistwin_srew, "clicked", local_playlist_prev, NULL);

    /* play button */
    playlistwin_splay = ui_skinned_button_new();
    ui_skinned_small_button_setup(playlistwin_splay, SKINNED_WINDOW(playlistwin)->fixed,
                                  playlistwin_get_width() - 138,
                                  cfg.playlist_height - 16, 10, 7);
    g_signal_connect(playlistwin_splay, "clicked", mainwin_play_pushed, NULL);

    /* pause button */
    playlistwin_spause = ui_skinned_button_new();
    ui_skinned_small_button_setup(playlistwin_spause, SKINNED_WINDOW(playlistwin)->fixed,
                                  playlistwin_get_width() - 128,
                                  cfg.playlist_height - 16, 10, 7);
    g_signal_connect(playlistwin_spause, "clicked", playback_pause, NULL);

    /* stop button */
    playlistwin_sstop = ui_skinned_button_new();
    ui_skinned_small_button_setup(playlistwin_sstop, SKINNED_WINDOW(playlistwin)->fixed,
                                  playlistwin_get_width() - 118,
                                  cfg.playlist_height - 16, 9, 7);
    g_signal_connect(playlistwin_sstop, "clicked", mainwin_stop_pushed, NULL);

    /* forward button */
    playlistwin_sfwd = ui_skinned_button_new();
    ui_skinned_small_button_setup(playlistwin_sfwd, SKINNED_WINDOW(playlistwin)->fixed,
                                  playlistwin_get_width() - 109,
                                  cfg.playlist_height - 16, 8, 7);
    g_signal_connect(playlistwin_sfwd, "clicked", local_playlist_next, NULL);

    /* eject button */
    playlistwin_seject = ui_skinned_button_new();
    ui_skinned_small_button_setup(playlistwin_seject, SKINNED_WINDOW(playlistwin)->fixed,
                                  playlistwin_get_width() - 100,
                                  cfg.playlist_height - 16, 9, 7);
    g_signal_connect(playlistwin_seject, "clicked", mainwin_eject_pushed, NULL);

    playlistwin_sscroll_up = ui_skinned_button_new();
    ui_skinned_small_button_setup(playlistwin_sscroll_up, SKINNED_WINDOW(playlistwin)->fixed,
                                  playlistwin_get_width() - 14,
                                  cfg.playlist_height - 35, 8, 5);
    g_signal_connect(playlistwin_sscroll_up, "clicked", playlistwin_scroll_up_pushed, NULL);

    playlistwin_sscroll_down = ui_skinned_button_new();
    ui_skinned_small_button_setup(playlistwin_sscroll_down, SKINNED_WINDOW(playlistwin)->fixed,
                                  playlistwin_get_width() - 14,
                                  cfg.playlist_height - 30, 8, 5);
    g_signal_connect(playlistwin_sscroll_down, "clicked", playlistwin_scroll_down_pushed, NULL);
}

static void
selection_received(GtkWidget * widget,
                   GtkSelectionData * selection_data, gpointer data)
{
    if (selection_data->type == GDK_SELECTION_TYPE_STRING &&
        selection_data->length > 0)
        playlist_add_url(playlist_get_active(), (gchar *) selection_data->data);
}

static void
playlistwin_create_window(void)
{
    GdkPixbuf *icon;

    playlistwin = ui_skinned_window_new("playlist");
    gtk_window_set_title(GTK_WINDOW(playlistwin), _("Audacious Playlist Editor"));
    gtk_window_set_role(GTK_WINDOW(playlistwin), "playlist");
    gtk_window_set_default_size(GTK_WINDOW(playlistwin),
                                playlistwin_get_width(),
                                playlistwin_get_height());
    gtk_window_set_resizable(GTK_WINDOW(playlistwin), TRUE);
    playlistwin_set_geometry_hints(cfg.playlist_shaded);

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
playlistwin_create(void)
{
    resize_mutex = g_mutex_new();
    playlistwin_create_window();

    playlistwin_create_widgets();
    playlistwin_update_info(playlist_get_active());

    gtk_window_add_accel_group(GTK_WINDOW(playlistwin), ui_manager_get_accel_group());
}


void
playlistwin_show(void)
{
    gtk_window_move(GTK_WINDOW(playlistwin), cfg.playlist_x, cfg.playlist_y);
    GtkAction *action = gtk_action_group_get_action(
      toggleaction_group_others , "show playlist editor" );
    gtk_toggle_action_set_active( GTK_TOGGLE_ACTION(action) , TRUE );

    cfg.playlist_visible = TRUE;
    UI_SKINNED_BUTTON(mainwin_pl)->inside = TRUE;
    gtk_widget_queue_draw(mainwin_pl);

    playlistwin_set_toprow(0);
    playlist_check_pos_current(playlist_get_active());

    gtk_widget_show_all(playlistwin);
    if (!cfg.playlist_shaded)
        gtk_widget_hide(playlistwin_sinfo);
    gtk_window_present(GTK_WINDOW(playlistwin));
}

void
playlistwin_hide(void)
{
    GtkAction *action = gtk_action_group_get_action(
      toggleaction_group_others , "show playlist editor" );
    gtk_toggle_action_set_active( GTK_TOGGLE_ACTION(action) , FALSE );

    gtk_widget_hide(playlistwin);
    cfg.playlist_visible = FALSE;
    UI_SKINNED_BUTTON(mainwin_pl)->inside = FALSE;
    gtk_widget_queue_draw(mainwin_pl);

    if ( cfg.player_visible )
    {
      gtk_window_present(GTK_WINDOW(mainwin));
      gtk_widget_grab_focus(mainwin);
    }
}

void action_playlist_track_info(void)
{
    playlistwin_fileinfo();
}

void action_queue_toggle(void)
{
    playlist_queue(playlist_get_active());
}

void action_playlist_sort_by_playlist_entry(void)
{
    Playlist *playlist = playlist_get_active();

    playlist_sort(playlist, PLAYLIST_SORT_PLAYLIST);
    playlistwin_update_list(playlist);
}

void action_playlist_sort_by_track_number(void)
{
    Playlist *playlist = playlist_get_active();

    playlist_sort(playlist, PLAYLIST_SORT_TRACK);
    playlistwin_update_list(playlist);
}

void action_playlist_sort_by_title(void)
{
    Playlist *playlist = playlist_get_active();

    playlist_sort(playlist, PLAYLIST_SORT_TITLE);
    playlistwin_update_list(playlist);
}

void action_playlist_sort_by_artist(void)
{
    Playlist *playlist = playlist_get_active();

    playlist_sort(playlist, PLAYLIST_SORT_ARTIST);
    playlistwin_update_list(playlist);
}

void action_playlist_sort_by_full_path(void)
{
    Playlist *playlist = playlist_get_active();

    playlist_sort(playlist, PLAYLIST_SORT_PATH);
    playlistwin_update_list(playlist);
}

void action_playlist_sort_by_date(void)
{
    Playlist *playlist = playlist_get_active();

    playlist_sort(playlist, PLAYLIST_SORT_DATE);
    playlistwin_update_list(playlist);
}

void action_playlist_sort_by_filename(void)
{
    Playlist *playlist = playlist_get_active();

    playlist_sort(playlist, PLAYLIST_SORT_FILENAME);
    playlistwin_update_list(playlist);
}

void action_playlist_sort_selected_by_playlist_entry(void)
{
    Playlist *playlist = playlist_get_active();

    playlist_sort_selected(playlist, PLAYLIST_SORT_PLAYLIST);
    playlistwin_update_list(playlist);
}

void action_playlist_sort_selected_by_track_number(void)
{
    Playlist *playlist = playlist_get_active();

    playlist_sort_selected(playlist, PLAYLIST_SORT_TRACK);
    playlistwin_update_list(playlist);
}

void action_playlist_sort_selected_by_title(void)
{
    Playlist *playlist = playlist_get_active();

    playlist_sort_selected(playlist, PLAYLIST_SORT_TITLE);
    playlistwin_update_list(playlist);
}

void action_playlist_sort_selected_by_artist(void)
{
    Playlist *playlist = playlist_get_active();

    playlist_sort_selected(playlist, PLAYLIST_SORT_ARTIST);
    playlistwin_update_list(playlist);
}

void action_playlist_sort_selected_by_full_path(void)
{
    Playlist *playlist = playlist_get_active();

    playlist_sort_selected(playlist, PLAYLIST_SORT_PATH);
    playlistwin_update_list(playlist);
}

void action_playlist_sort_selected_by_date(void)
{
    Playlist *playlist = playlist_get_active();

    playlist_sort_selected(playlist, PLAYLIST_SORT_DATE);
    playlistwin_update_list(playlist);
}

void action_playlist_sort_selected_by_filename(void)
{
    Playlist *playlist = playlist_get_active();

    playlist_sort_selected(playlist, PLAYLIST_SORT_FILENAME);
    playlistwin_update_list(playlist);
}

void action_playlist_randomize_list(void)
{
    Playlist *playlist = playlist_get_active();

    playlist_random(playlist);
    playlistwin_update_list(playlist);
}

void action_playlist_reverse_list(void)
{
    Playlist *playlist = playlist_get_active();

    playlist_reverse(playlist);
    playlistwin_update_list(playlist);
}

void
action_playlist_clear_queue(void)
{
    playlist_clear_queue(playlist_get_active());
}

void
action_playlist_remove_unavailable(void)
{
    playlist_remove_dead_files(playlist_get_active());
}

void
action_playlist_remove_dupes_by_title(void)
{
    playlist_remove_duplicates(playlist_get_active(), PLAYLIST_DUPS_TITLE);
}

void
action_playlist_remove_dupes_by_filename(void)
{
    playlist_remove_duplicates(playlist_get_active(), PLAYLIST_DUPS_FILENAME);
}

void
action_playlist_remove_dupes_by_full_path(void)
{
    playlist_remove_duplicates(playlist_get_active(), PLAYLIST_DUPS_PATH);
}

void
action_playlist_remove_all(void)
{
    playlist_clear(playlist_get_active());

    /* XXX -- should this really be coupled here? -nenolod */
    mainwin_clear_song_info();
}

void
action_playlist_remove_selected(void)
{
    playlist_delete(playlist_get_active(), FALSE);
}

void
action_playlist_remove_unselected(void)
{
    playlist_delete(playlist_get_active(), TRUE);
}

void
action_playlist_add_files(void)
{
    run_filebrowser(NO_PLAY_BUTTON);
}

void
action_playlist_add_url(void)
{
    mainwin_show_add_url_window();
}

void
action_playlist_new( void )
{
  Playlist *new_pl = playlist_new();
  playlist_add_playlist(new_pl);
  playlist_select_playlist(new_pl);
}

void
action_playlist_prev( void )
{
    playlist_select_prev();
}

void
action_playlist_next( void )
{
    playlist_select_next();
}

void
action_playlist_delete( void )
{
    playlist_remove_playlist( playlist_get_active() );
}

void
action_playlist_save_list(void)
{
    Playlist *playlist = playlist_get_active();

    playlistwin_select_playlist_to_save(playlist_get_current_name(playlist));
}

void
action_playlist_save_default_list(void)
{
    Playlist *playlist = playlist_get_active();

    playlist_save(playlist, bmp_paths[BMP_PATH_PLAYLIST_FILE]);
}

void
action_playlist_load_list(void)
{
    Playlist *playlist = playlist_get_active();

    playlistwin_select_playlist_to_load(playlist_get_current_name(playlist));
}

void
action_playlist_refresh_list(void)
{
    Playlist *playlist = playlist_get_active();

    playlist_read_info_selection(playlist);
    playlistwin_update_list(playlist);
}

void
action_open_list_manager(void)
{
    playlist_manager_ui_show();
}

void
action_playlist_search_and_select(void)
{
    playlistwin_select_search();
}

void
action_playlist_invert_selection(void)
{
    playlistwin_inverse_selection();
}

void
action_playlist_select_none(void)
{
    playlistwin_select_none();
}

void
action_playlist_select_all(void)
{
    playlistwin_select_all();
}



/* playlistwin_select_search callback functions
   placed here to avoid making the code messier :) */
void
playlistwin_select_search_cbt_cb(GtkWidget *called_cbt, gpointer other_cbt)
{
    if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(called_cbt)) == TRUE)
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(other_cbt), FALSE);
    return;
}

static gboolean
playlistwin_select_search_kp_cb(GtkWidget *entry, GdkEventKey *event,
                                gpointer searchdlg_win)
{
    switch (event->keyval)
    {
        case GDK_Return:
            gtk_dialog_response(GTK_DIALOG(searchdlg_win), GTK_RESPONSE_ACCEPT);
            return TRUE;
        default:
            return FALSE;
    }
}
