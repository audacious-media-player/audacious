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
 *  the Free Software Foundation; under version 2 of the License.
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

GtkWidget *playlistwin;

PlayList_List *playlistwin_list = NULL;
PButton *playlistwin_shade, *playlistwin_close;

static gboolean playlistwin_resizing = FALSE;

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

static void playlistwin_select_search_cbt_cb( GtkWidget *called_cbt ,
                                              gpointer other_cbt );
static gboolean playlistwin_select_search_kp_cb( GtkWidget *entry , GdkEventKey *event ,
                                                 gpointer searchdlg_win );

static void action_new_list(void);
static void action_load_list(void);
static void action_save_list(void);
static void action_save_default_list(void);
static void action_refresh_list(void);

static void action_search_and_select(void);
static void action_invert_selection(void);
static void action_select_all(void);
static void action_select_none(void);

static void action_clear_queue(void);
static void action_remove_unavailable(void);
static void action_remove_dupes_by_title(void);
static void action_remove_dupes_by_filename(void);
static void action_remove_dupes_by_full_path(void);
static void action_remove_all(void);
static void action_remove_selected(void);
static void action_remove_unselected(void);

static void action_add_cd(void);
static void action_add_url(void);
static void action_add_files(void);

static void action_randomize_list(void);
static void action_reverse_list(void);

static void action_sort_by_title(void);
static void action_sort_by_artist(void);
static void action_sort_by_filename(void);
static void action_sort_by_full_path(void);
static void action_sort_by_date(void);
static void action_sort_by_track_number(void);
static void action_sort_by_playlist_entry(void);

static void action_sort_selected_by_title(void);
static void action_sort_selected_by_artist(void);
static void action_sort_selected_by_filename(void);
static void action_sort_selected_by_full_path(void);
static void action_sort_selected_by_date(void);
static void action_sort_selected_by_track_number(void);
static void action_sort_selected_by_playlist_entry(void);

static void action_view_track_info(void);
static void action_queue_toggle(void);

static GtkActionEntry playlistwin_actions[] = {
	{ "dummy", NULL, "dummy" },

	/* **************** playlist menu **************** */

	{ "new list", GTK_STOCK_NEW, N_("New List"), "<Shift>N",
	  N_("Creates a new playlist."), G_CALLBACK(action_new_list) },

	{ "load list", GTK_STOCK_OPEN, N_("Load List"), "O",
	  N_("Loads a playlist file into the selected playlist."), G_CALLBACK(action_load_list) },

	{ "save list", GTK_STOCK_SAVE, N_("Save List"), "<Shift>S",
	  N_("Saves the selected playlist."), G_CALLBACK(action_save_list) },

	{ "save default list", GTK_STOCK_SAVE, N_("Save Default List"), "<Alt>S",
	  N_("Saves the selected playlist to the default location."),
	  G_CALLBACK(action_save_default_list) },

	{ "refresh list", GTK_STOCK_REFRESH, N_("Refresh List"), "F5",
	  N_("Refreshes metadata associated with a playlist entry."),
	  G_CALLBACK(action_refresh_list) },

	/* **************** select menu **************** */

	{ "search and select", GTK_STOCK_FIND, N_("Search and Select"), "<Ctrl>F",
	  N_("Searches the playlist and selects playlist entries based on specific criteria."),
	  G_CALLBACK(action_search_and_select) },

	{ "invert selection", NULL, N_("Invert Selection"), NULL,
	  N_("Inverts the selected and unselected entries."),
	  G_CALLBACK(action_invert_selection) },

	{ "select all", NULL, N_("Select All"), "<Ctrl>A",
	  N_("Selects all of the playlist entries."),
	  G_CALLBACK(action_select_all) },

	{ "select none", NULL, N_("Select None"), "<Shift><Ctrl>A",
	  N_("Deselects all of the playlist entries."),
	  G_CALLBACK(action_select_none) },

	/* **************** delete menu **************** */

	{ "clear queue", GTK_STOCK_REMOVE, N_("Clear Queue"), "<Shift>Q",
	  N_("Clears the queue associated with this playlist."),
	  G_CALLBACK(action_clear_queue) },

	{ "remove unavailable", NULL, N_("Remove Unavailable Files"), NULL,
	  N_("Removes unavailable files from the playlist."),
	  G_CALLBACK(action_remove_unavailable) },

	{ "remove duplicates menu", NULL, N_("Remove Duplicates") },

	{ "remove by title", NULL, N_("By Title"), NULL,
	  N_("Removes duplicate entries from the playlist by title."),
	  G_CALLBACK(action_remove_dupes_by_title) },

	{ "remove by filename", NULL, N_("By Filename"), NULL, 
	  N_("Removes duplicate entries from the playlist by filename."),
	  G_CALLBACK(action_remove_dupes_by_filename) },

	{ "remove by full path", NULL, N_("By Path + Filename"), NULL, 
	  N_("Removes duplicate entries from the playlist by their full path."),
	  G_CALLBACK(action_remove_dupes_by_full_path) },

	{ "remove all", GTK_STOCK_CLEAR, N_("Remove All"), NULL, 
	  N_("Removes all entries from the playlist."),
	  G_CALLBACK(action_remove_all) },

	{ "remove unselected", GTK_STOCK_REMOVE, N_("Remove Unselected"), NULL,
	  N_("Remove unselected entries from the playlist."),
	  G_CALLBACK(action_remove_unselected) },

	{ "remove selected", GTK_STOCK_REMOVE, N_("Remove Selected"), "Delete", 
	  N_("Remove selected entries from the playlist."),
	  G_CALLBACK(action_remove_selected) },

	/* **************** add menu **************** */

	{ "add cd", GTK_STOCK_CDROM, N_("Add CD..."), "<Shift>C",
	  N_("Adds a CD to the playlist."),
	  G_CALLBACK(action_add_cd) },

	{ "add url", GTK_STOCK_NETWORK, N_("Add Internet Address..."), "<Ctrl>H",
	  N_("Adds a remote track to the playlist."),
	  G_CALLBACK(action_add_url) },

	{ "add files", GTK_STOCK_ADD, N_("Add Files..."), "F",
	  N_("Adds files to the playlist."),
	  G_CALLBACK(action_add_files) },

	/* **************** sort menu **************** */

	{ "randomize list", NULL, N_("Randomize List"), "<Ctrl><Shift>R",
	  N_("Randomizes the playlist."),
	  G_CALLBACK(action_randomize_list) },

	{ "reverse list", NULL, N_("Reverse List"), NULL,
	  N_("Reverses the playlist."),
	  G_CALLBACK(action_reverse_list) },

	{ "sort menu", NULL, N_("Sort List") },

	{ "sort by title", NULL, N_("By Title"), NULL,
	  N_("Sorts the list by title."),
	  G_CALLBACK(action_sort_by_title) },

	{ "sort by artist", NULL, N_("By Artist"), NULL,
	  N_("Sorts the list by artist."),
	  G_CALLBACK(action_sort_by_artist) },

	{ "sort by filename", NULL, N_("By Filename"), NULL,
	  N_("Sorts the list by filename."),
	  G_CALLBACK(action_sort_by_filename) },

	{ "sort by full path", NULL, N_("By Path + Filename"), NULL,
	  N_("Sorts the list by full pathname."),
	  G_CALLBACK(action_sort_by_full_path) },

	{ "sort by date", NULL, N_("By Date"), NULL,
	  N_("Sorts the list by modification time."),
	  G_CALLBACK(action_sort_by_date) },

	{ "sort by track number", NULL, N_("By Track Number"), NULL,
	  N_("Sorts the list by track number."),
	  G_CALLBACK(action_sort_by_track_number) },

	{ "sort by playlist entry", NULL, N_("By Playlist Entry"), NULL,
	  N_("Sorts the list by playlist entry."),
	  G_CALLBACK(action_sort_by_playlist_entry) },

	{ "sort selected menu", NULL, N_("Sort Selected") },

	{ "sort selected by title", NULL, N_("By Title"), NULL,
	  N_("Sorts the list by title."),
	  G_CALLBACK(action_sort_selected_by_title) },

	{ "sort selected by artist", NULL, N_("By Artist"), NULL,
	  N_("Sorts the list by artist."),
	  G_CALLBACK(action_sort_selected_by_artist) },

	{ "sort selected by filename", NULL, N_("By Filename"), NULL,
	  N_("Sorts the list by filename."),
	  G_CALLBACK(action_sort_selected_by_filename) },

	{ "sort selected by full path", NULL, N_("By Path + Filename"), NULL,
	  N_("Sorts the list by full pathname."),
	  G_CALLBACK(action_sort_selected_by_full_path) },

	{ "sort selected by date", NULL, N_("By Date"), NULL,
	  N_("Sorts the list by modification time."),
	  G_CALLBACK(action_sort_selected_by_date) },

	{ "sort selected by track number", NULL, N_("By Track Number"), NULL,
	  N_("Sorts the list by track number."),
	  G_CALLBACK(action_sort_selected_by_track_number) },

	{ "sort selected by playlist entry", NULL, N_("By Playlist Entry"), NULL,
	  N_("Sorts the list by playlist entry."),
	  G_CALLBACK(action_sort_selected_by_playlist_entry) },

	/* stuff used in rightclick menu, but not yet covered */

	{ "view track info", NULL, N_("View Track Details"), "<Alt>I",
	  N_("Displays track information."),
	  G_CALLBACK(action_view_track_info) },

	{ "queue toggle", NULL, N_("Queue Toggle"), "Q", 
	  N_("Enables/disables the entry in the playlist's queue."),
	  G_CALLBACK(action_queue_toggle) },
};

static GtkWidget *pladd_menu, *pldel_menu, *plsel_menu, *plsort_menu, *pllist_menu;
static GtkWidget *playlistwin_popup_menu;

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
    textbox_set_text(playlistwin_info, text ? text : "");
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
        textbox_set_text(playlistwin_sinfo, "");
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
    playlistwin_update_list(playlist_get_active());
}

void
playlistwin_update_list(Playlist *playlist)
{
    g_return_if_fail(playlistwin_list != NULL);

    widget_draw(WIDGET(playlistwin_list));
    widget_draw(WIDGET(playlistwin_slider));
    playlistwin_update_info(playlist);
    playlistwin_update_sinfo(playlist);
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

    gdk_pointer_ungrab(GDK_CURRENT_TIME);
    playlistwin_resizing = FALSE;
    gdk_flush();
 
    if (dock_is_moving(GTK_WINDOW(playlistwin)))
    {
       dock_move_release(GTK_WINDOW(playlistwin));

       if (cfg.playlist_transparent)
           playlistwin_update_list(playlist_get_active());
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
    playlistwin_list->pl_prev_selected = 0;
    playlistwin_list->pl_prev_min = 0;
    playlistwin_list->pl_prev_max = playlist_get_length(playlist) - 1;
    playlistwin_update_list(playlist);
}

static void
playlistwin_select_none(void)
{
    playlist_select_all(playlist_get_active(), FALSE);
    playlistwin_list->pl_prev_selected = -1;
    playlistwin_list->pl_prev_min = -1;
    playlistwin_update_list(playlist_get_active());
}

static void
playlistwin_select_search(void)
{
    Playlist *playlist = playlist_get_active();
    GtkWidget *searchdlg_win, *searchdlg_table;
    GtkWidget *searchdlg_hbox, *searchdlg_logo, *searchdlg_helptext;
    GtkWidget *searchdlg_entry_track_name, *searchdlg_label_track_name;
    GtkWidget *searchdlg_entry_album_name, *searchdlg_label_album_name;
    GtkWidget *searchdlg_entry_file_name, *searchdlg_label_file_name;
    GtkWidget *searchdlg_entry_performer, *searchdlg_label_performer;
    GtkWidget *searchdlg_checkbt_clearprevsel;
    GtkWidget *searchdlg_checkbt_newplaylist;
    GtkWidget *searchdlg_checkbt_autoenqueue;
    gint result;

    /* create dialog */
    searchdlg_win = gtk_dialog_new_with_buttons(
      "Search entries in active playlist" , GTK_WINDOW(mainwin) ,
      GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT ,
      GTK_STOCK_CANCEL , GTK_RESPONSE_REJECT , GTK_STOCK_OK , GTK_RESPONSE_ACCEPT , NULL );
    /* help text and logo */
    searchdlg_hbox = gtk_hbox_new( FALSE , 4 );
    searchdlg_logo = gtk_image_new_from_stock( GTK_STOCK_FIND , GTK_ICON_SIZE_DIALOG );
    searchdlg_helptext = gtk_label_new( _("Select entries in playlist by filling one or more "
      "fields. Fields use regular expressions syntax, case-insensitive. If you don't know how "
      "regular expressions work, simply insert a literal portion of what you're searching for.") );
    gtk_label_set_line_wrap( GTK_LABEL(searchdlg_helptext) , TRUE );
    gtk_box_pack_start( GTK_BOX(searchdlg_hbox) , searchdlg_logo , FALSE , FALSE , 0 );
    gtk_box_pack_start( GTK_BOX(searchdlg_hbox) , searchdlg_helptext , FALSE , FALSE , 0 );
    /* track name */
    searchdlg_label_track_name = gtk_label_new( _("Track name: ") );
    searchdlg_entry_track_name = gtk_entry_new();
    gtk_misc_set_alignment( GTK_MISC(searchdlg_label_track_name) , 0 , 0.5 );
    g_signal_connect( G_OBJECT(searchdlg_entry_track_name) , "key-press-event" ,
      G_CALLBACK(playlistwin_select_search_kp_cb) , searchdlg_win );
    /* album name */
    searchdlg_label_album_name = gtk_label_new( _("Album name: ") );
    searchdlg_entry_album_name = gtk_entry_new();
    gtk_misc_set_alignment( GTK_MISC(searchdlg_label_album_name) , 0 , 0.5 );
    g_signal_connect( G_OBJECT(searchdlg_entry_album_name) , "key-press-event" ,
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
    gtk_table_attach( GTK_TABLE(searchdlg_table) , searchdlg_label_track_name ,
      0 , 1 , 1 , 2 , GTK_FILL , GTK_FILL | GTK_EXPAND , 0 , 2 );
    gtk_table_attach( GTK_TABLE(searchdlg_table) , searchdlg_entry_track_name ,
      1 , 2 , 1 , 2 , GTK_FILL | GTK_EXPAND , GTK_FILL | GTK_EXPAND , 0 , 2 );
    gtk_table_attach( GTK_TABLE(searchdlg_table) , searchdlg_label_album_name ,
      0 , 1 , 2 , 3 , GTK_FILL , GTK_FILL | GTK_EXPAND , 0 , 2 );
    gtk_table_attach( GTK_TABLE(searchdlg_table) , searchdlg_entry_album_name ,
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
         TitleInput *tuple = g_malloc(sizeof(TitleInput));
         gchar *searchdata = NULL;
         searchdata = (gchar*)gtk_entry_get_text( GTK_ENTRY(searchdlg_entry_track_name) );
         tuple->track_name = ( strcmp(searchdata,"") ) ? g_strdup(searchdata) : NULL;
         searchdata = (gchar*)gtk_entry_get_text( GTK_ENTRY(searchdlg_entry_album_name) );
         tuple->album_name = ( strcmp(searchdata,"") ) ? g_strdup(searchdata) : NULL;
         searchdata = (gchar*)gtk_entry_get_text( GTK_ENTRY(searchdlg_entry_performer) );
         tuple->performer = ( strcmp(searchdata,"") ) ? g_strdup(searchdata) : NULL;
         searchdata = (gchar*)gtk_entry_get_text( GTK_ENTRY(searchdlg_entry_file_name) );
         tuple->file_name = ( strcmp(searchdata,"") ) ? g_strdup(searchdata) : NULL;
         /* check if previous selection should be cleared before searching */
         if ( gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(searchdlg_checkbt_clearprevsel)) == TRUE )
             playlistwin_select_none();
         /* now send this tuple to the real search function */
         matched_entries_num = playlist_select_search( playlist , tuple , 0 );
         /* we do not need the tuple and its data anymore */
         if ( tuple->track_name != NULL ) g_free( tuple->track_name );
         if ( tuple->album_name != NULL )  g_free( tuple->album_name );
         if ( tuple->performer != NULL ) g_free( tuple->performer );
         if ( tuple->file_name != NULL ) g_free( tuple->file_name );
         g_free( tuple );
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
    playlistwin_list->pl_prev_selected = -1;
    playlistwin_list->pl_prev_min = -1;
    playlistwin_update_list(playlist_get_active());
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
    playlistwin_update_sinfo(playlist_get_active());

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
menu_set_item_sensitive(GtkItemFactory * item_factory,
                        const gchar * path,
                        gboolean sensitive)
{
    GtkWidget *item = gtk_item_factory_get_widget(item_factory, path);
    gtk_widget_set_sensitive(item, sensitive);
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

    if (!playlist_save(playlist_get_active(), filename))
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
    Playlist *playlist = playlist_get_active();

    g_return_if_fail(filename != NULL);

    str_replace_in(&cfg.playlist_path, g_strdup(filename));

    playlist_clear(playlist);
    mainwin_clear_song_info();
    mainwin_set_info_text();

    playlist_load(playlist, filename);
    playlist_set_current_name(playlist, filename);
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

    g_cond_signal(cond_scan);

}

static void
menu_popup_pos_func(GtkMenu * menu,
                    gint * x,
                    gint * y,
                    gboolean * push_in,
                    gint * point)
{
    *x = point[0];
    *y = point[1];
    *push_in = FALSE;
}

static void
menu_popup(GtkMenu * menu,
           gint x,
           gint y,
           guint button,
           guint time)
{
    gint pos[2];

    pos[0] = x;
    pos[1] = y;

    gtk_menu_popup(menu, NULL, NULL,
                   (GtkMenuPositionFunc) menu_popup_pos_func, pos,
                   button, time);
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
        gtk_widget_size_request(pladd_menu, &req);
        menu_popup(GTK_MENU(pladd_menu),
                   xpos + 12,
                   (ypos + playlistwin_get_height()) - 8 - req.height,
                   event->button,
                   event->time);
        grab = FALSE;
    }
    else if (event->button == 1 && REGION_L(41, 66, 29, 11)) {
        /* SUB button menu */
        gtk_widget_size_request(pldel_menu, &req);
        menu_popup(GTK_MENU(pldel_menu),
                   xpos + 40,
                   (ypos + playlistwin_get_height()) - 8 - req.height,
                   event->button,
                   event->time);
        grab = FALSE;
    }
    else if (event->button == 1 && REGION_L(70, 95, 29, 11)) {
        /* SEL button menu */
        gtk_widget_size_request(plsel_menu, &req);
        menu_popup(GTK_MENU(plsel_menu),
                   xpos + 68,
                   (ypos + playlistwin_get_height()) - 8 - req.height,
                   event->button,
                   event->time);
        grab = FALSE;
    }
    else if (event->button == 1 && REGION_L(99, 124, 29, 11)) {
        /* MISC button menu */
        gtk_widget_size_request(plsort_menu, &req);
        menu_popup(GTK_MENU(plsort_menu),
                   xpos + 100,
                   (ypos + playlistwin_get_height()) - 8 - req.height,
                   event->button,
                   event->time);
        grab = FALSE;
    }
    else if (event->button == 1 && REGION_R(46, 23, 29, 11)) {
        /* LIST button menu */
        gtk_widget_size_request(pllist_menu, &req);
        menu_popup(GTK_MENU(pllist_menu),
                   xpos + playlistwin_get_width() - req.width - 12,
                   (ypos + playlistwin_get_height()) - 8 - req.height,
                   event->button,
                   event->time);
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
             !inside_sensitive_widgets(event->x, event->y) && (cfg.easy_move || event->y < 14))
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
        menu_popup(GTK_MENU(playlistwin_popup_menu),
                               event->x_root, event->y_root + 5,
                               event->button, event->time);
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
    Playlist *playlist = playlist_get_active();

    if ((state & GDK_MOD1_MASK) && (state & GDK_SHIFT_MASK))
        return;
    if (!(state & GDK_MOD1_MASK))
        playlist_select_all(playlist, FALSE);

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
            CLAMP(pl->pl_prev_max, 0, playlist_get_length(playlist) - 1);

        pl->pl_first = MIN(pl->pl_first, pl->pl_prev_max);
        pl->pl_first = MAX(pl->pl_first, pl->pl_prev_max -
                           pl->pl_num_visible + 1);
        playlist_select_range(playlist, pl->pl_prev_min, pl->pl_prev_max, TRUE);
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
        CLAMP(pl->pl_prev_selected, 0, playlist_get_length(playlist) - 1);

    if (pl->pl_prev_selected < pl->pl_first)
        pl->pl_first--;
    else if (pl->pl_prev_selected >= (pl->pl_first + pl->pl_num_visible))
        pl->pl_first++;

    playlist_select_range(playlist, pl->pl_prev_selected, pl->pl_prev_selected, TRUE);
    pl->pl_prev_min = -1;
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
            playlist_get_length(playlist) - playlistwin_list->pl_num_visible;
        refresh = TRUE;
        break;
    case GDK_Return:
        if (playlistwin_list->pl_prev_selected > -1
            && playlistwin_item_visible(playlistwin_list->pl_prev_selected)) {
            playlist_set_position(playlist, playlistwin_list->pl_prev_selected);
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
            bmp_playback_seek(CLAMP
                              (bmp_playback_get_time() - 5000, 0,
                              playlist_get_current_length(playlist)) / 1000);
        break;
    case GDK_Right:
    case GDK_KP_Right:
    case GDK_KP_9:
        if (playlist_get_current_length(playlist) != -1)
            bmp_playback_seek(CLAMP
                              (bmp_playback_get_time() + 5000, 0,
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
    default:
        return FALSE;
    }

    if (refresh) {
        g_cond_signal(cond_scan);
        playlistwin_update_list(playlist_get_active());
    }

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
    playlistwin_update_list(playlist_get_active());
    playlistwin_hint_flag = TRUE;
}

static void
playlistwin_drag_end(GtkWidget * widget,
                     GdkDragContext * context, gpointer user_data)
{
    playlistwin_list->pl_drag_motion = FALSE;
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

    g_return_if_fail(selection_data != NULL);

    if (!selection_data->data) {
        g_message("Received no DND data!");
        return;
    }

    if (widget_contains(WIDGET(playlistwin_list), x, y)) {
        pos = (y - WIDGET(playlistwin_list)->y) /
            playlistwin_list->pl_fheight + playlistwin_list->pl_first;

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
                       cfg.playlist_height - 16, 8, 7, local_playlist_prev);

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
                       cfg.playlist_height - 16, 8, 7, local_playlist_next);

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
        playlist_add_url(playlist_get_active(), (gchar *) selection_data->data);
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

static GtkWidget *
ui_manager_get_popup(GtkUIManager * self, const gchar * path)
{
    GtkWidget *menu_item;

    menu_item = gtk_ui_manager_get_widget(self, path);

    if (GTK_IS_MENU_ITEM(menu_item))
        return gtk_menu_item_get_submenu(GTK_MENU_ITEM(menu_item));
    else
        return NULL;
}

void
playlistwin_create_popup_menus(void)
{
    GtkUIManager *ui_manager;
    GtkActionGroup *action_group;
    GError *error = NULL;

    action_group = gtk_action_group_new("playlistwin");
    gtk_action_group_add_actions(action_group, playlistwin_actions,
				 G_N_ELEMENTS(playlistwin_actions), NULL);

    ui_manager = gtk_ui_manager_new();
    gtk_ui_manager_add_ui_from_file(ui_manager, DATA_DIR "/ui/playlist.ui", &error);

    if (error)
    {
        g_critical("Error creating UI<ui/playlist.ui>: %s", error->message);
	return;
    }

    gtk_ui_manager_insert_action_group(ui_manager, action_group, 0);

    /* playlist window popup menu */
    playlistwin_popup_menu = ui_manager_get_popup(ui_manager, "/playlist-menus/playlist-rightclick-menu");

    pladd_menu  = ui_manager_get_popup(ui_manager, "/playlist-menus/add-menu");
    pldel_menu  = ui_manager_get_popup(ui_manager, "/playlist-menus/del-menu");
    plsel_menu  = ui_manager_get_popup(ui_manager, "/playlist-menus/select-menu");
    plsort_menu = ui_manager_get_popup(ui_manager, "/playlist-menus/misc-menu");
    pllist_menu = ui_manager_get_popup(ui_manager, "/playlist-menus/playlist-menu");

    gtk_window_add_accel_group(GTK_WINDOW(playlistwin),
			       gtk_ui_manager_get_accel_group(ui_manager));
}

void
playlistwin_create(void)
{
    Playlist *playlist;
    playlistwin_create_window();
    playlistwin_create_popup_menus();

    /* create GC and back pixmap for custom widget to draw on */
    playlistwin_gc = gdk_gc_new(playlistwin->window);
    playlistwin_bg = gdk_pixmap_new(playlistwin->window,
                                    playlistwin_get_width(),
                                    playlistwin_get_height_unshaded(), -1);
    gdk_window_set_back_pixmap(playlistwin->window, playlistwin_bg, 0);

    playlistwin_create_widgets();
    playlistwin_update_info(playlist_get_active());

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
    playlist_check_pos_current(playlist_get_active());

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

static void action_view_track_info(void)
{
    playlistwin_fileinfo();
}

static void action_queue_toggle(void)
{
    playlist_queue(playlist_get_active());
}

static void action_sort_by_playlist_entry(void)
{
    Playlist *playlist = playlist_get_active();

    playlist_sort(playlist, PLAYLIST_SORT_PLAYLIST);
    playlistwin_update_list(playlist);
}

static void action_sort_by_track_number(void)
{
    Playlist *playlist = playlist_get_active();

    playlist_sort(playlist, PLAYLIST_SORT_TRACK);
    playlistwin_update_list(playlist);
}

static void action_sort_by_title(void)
{
    Playlist *playlist = playlist_get_active();

    playlist_sort(playlist, PLAYLIST_SORT_TITLE);
    playlistwin_update_list(playlist);
}

static void action_sort_by_artist(void)
{
    Playlist *playlist = playlist_get_active();

    playlist_sort(playlist, PLAYLIST_SORT_ARTIST);
    playlistwin_update_list(playlist);
}

static void action_sort_by_full_path(void)
{
    Playlist *playlist = playlist_get_active();

    playlist_sort(playlist, PLAYLIST_SORT_PATH);
    playlistwin_update_list(playlist);
}

static void action_sort_by_date(void)
{
    Playlist *playlist = playlist_get_active();

    playlist_sort(playlist, PLAYLIST_SORT_DATE);
    playlistwin_update_list(playlist);
}

static void action_sort_by_filename(void)
{
    Playlist *playlist = playlist_get_active();

    playlist_sort(playlist, PLAYLIST_SORT_FILENAME);
    playlistwin_update_list(playlist);
}

static void action_sort_selected_by_playlist_entry(void)
{
    Playlist *playlist = playlist_get_active();

    playlist_sort_selected(playlist, PLAYLIST_SORT_PLAYLIST);
    playlistwin_update_list(playlist);
}

static void action_sort_selected_by_track_number(void)
{
    Playlist *playlist = playlist_get_active();

    playlist_sort_selected(playlist, PLAYLIST_SORT_TRACK);
    playlistwin_update_list(playlist);
}

static void action_sort_selected_by_title(void)
{
    Playlist *playlist = playlist_get_active();

    playlist_sort_selected(playlist, PLAYLIST_SORT_TITLE);
    playlistwin_update_list(playlist);
}

static void action_sort_selected_by_artist(void)
{
    Playlist *playlist = playlist_get_active();

    playlist_sort_selected(playlist, PLAYLIST_SORT_ARTIST);
    playlistwin_update_list(playlist);
}

static void action_sort_selected_by_full_path(void)
{
    Playlist *playlist = playlist_get_active();

    playlist_sort_selected(playlist, PLAYLIST_SORT_PATH);
    playlistwin_update_list(playlist);
}

static void action_sort_selected_by_date(void)
{
    Playlist *playlist = playlist_get_active();

    playlist_sort_selected(playlist, PLAYLIST_SORT_DATE);
    playlistwin_update_list(playlist);
}

static void action_sort_selected_by_filename(void)
{
    Playlist *playlist = playlist_get_active();

    playlist_sort_selected(playlist, PLAYLIST_SORT_FILENAME);
    playlistwin_update_list(playlist);
}

static void action_randomize_list(void)
{
    Playlist *playlist = playlist_get_active();

    playlist_random(playlist);
    playlistwin_update_list(playlist);
}

static void action_reverse_list(void)
{
    Playlist *playlist = playlist_get_active();

    playlist_reverse(playlist);
    playlistwin_update_list(playlist);
}

static void
action_clear_queue(void)
{
    playlist_clear_queue(playlist_get_active());
}

static void
action_remove_unavailable(void)
{
    playlist_remove_dead_files(playlist_get_active());
}

static void
action_remove_dupes_by_title(void)
{
    playlist_remove_duplicates(playlist_get_active(), PLAYLIST_DUPS_TITLE);
}

static void
action_remove_dupes_by_filename(void)
{
    playlist_remove_duplicates(playlist_get_active(), PLAYLIST_DUPS_FILENAME);
}

static void
action_remove_dupes_by_full_path(void)
{
    playlist_remove_duplicates(playlist_get_active(), PLAYLIST_DUPS_PATH);
}

static void
action_remove_all(void)
{
    playlist_clear(playlist_get_active());

    /* XXX -- should this really be coupled here? -nenolod */
    mainwin_clear_song_info();
    mainwin_set_info_text();
}

static void
action_remove_selected(void)
{
    playlist_delete(playlist_get_active(), FALSE);
}

static void
action_remove_unselected(void)
{
    playlist_delete(playlist_get_active(), TRUE);
}

static void
action_add_files(void)
{
    util_run_filebrowser(NO_PLAY_BUTTON);
}

void add_medium(void); /* XXX */

static void
action_add_cd(void)
{
    add_medium();
}

static void
action_add_url(void)
{
    mainwin_show_add_url_window();
}

static void
action_new_list(void)
{
    Playlist *playlist = playlist_get_active();

    playlist_set_current_name(playlist, NULL);
    playlist_clear(playlist);
    mainwin_clear_song_info();
    mainwin_set_info_text();
}

static void
action_save_list(void)
{
    Playlist *playlist = playlist_get_active();

    playlistwin_select_playlist_to_save(playlist_get_current_name(playlist));
}

static void
action_save_default_list(void)
{
    Playlist *playlist = playlist_get_active();

    playlist_save(playlist, bmp_paths[BMP_PATH_PLAYLIST_FILE]);
}

static void
action_load_list(void)
{
    Playlist *playlist = playlist_get_active();

    playlistwin_select_playlist_to_load(playlist_get_current_name(playlist));
}

static void
action_refresh_list(void)
{
    Playlist *playlist = playlist_get_active();

    playlist_read_info_selection(playlist);
    playlistwin_update_list(playlist);
}

static void
action_search_and_select(void)
{
    playlistwin_select_search();
}

static void
action_invert_selection(void)
{
    playlistwin_inverse_selection();
}

static void
action_select_none(void)
{
    playlistwin_select_none();
}

static void
action_select_all(void)
{
    playlistwin_select_all();
}

/* playlistwin_select_search callback functions
   placed here to avoid making the code messier :) */
static void
playlistwin_select_search_cbt_cb( GtkWidget *called_cbt , gpointer other_cbt )
{
    if ( gtk_toggle_button_get_active( GTK_TOGGLE_BUTTON(called_cbt) ) == TRUE )
        gtk_toggle_button_set_active( GTK_TOGGLE_BUTTON(other_cbt) , FALSE );
    return;
}

static gboolean
playlistwin_select_search_kp_cb( GtkWidget *entry , GdkEventKey *event ,
                                 gpointer searchdlg_win )
{
    switch (event->keyval)
    {
        case GDK_Return:
            gtk_dialog_response( GTK_DIALOG(searchdlg_win) , GTK_RESPONSE_ACCEPT );
            return TRUE;
        default:
            return FALSE;
    }
}
