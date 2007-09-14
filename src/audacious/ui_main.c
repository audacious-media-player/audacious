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

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif


#include <glib.h>
#include <glib/gi18n.h>
#include <glib/gprintf.h>
#include <gtk/gtk.h>
#include <gtk/gtkmessagedialog.h>

/* GDK including */
#include "platform/smartinclude.h"

#include <math.h>
#include <stdlib.h>
#include <string.h>

#include <sys/types.h>

#if defined(USE_REGEX_ONIGURUMA)
  #include <onigposix.h>
#elif defined(USE_REGEX_PCRE)
  #include <pcreposix.h>
#else
  #include <regex.h>
#endif

#include "ui_main.h"
#include "icons-stock.h"

#include "actions-mainwin.h"

#include "main.h"
#include "configdb.h"
#include "dnd.h"
#include "dock.h"
#include "hints.h"
#include "input.h"
#include "playback.h"
#include "playlist.h"
#include "pluginenum.h"
#include "ui_credits.h"
#include "ui_equalizer.h"
#include "ui_fileopener.h"
#include "ui_manager.h"
#include "ui_playlist.h"
#include "ui_preferences.h"
#include "ui_skinselector.h"
#include "ui_urlopener.h"
#include "ui_lastfm.h"
#include "strings.h"
#include "util.h"
#include "visualization.h"

#include "skin.h"
#include "ui_skinned_window.h"
#include "ui_skinned_button.h"
#include "ui_skinned_textbox.h"
#include "ui_skinned_number.h"
#include "ui_skinned_horizontal_slider.h"
#include "ui_skinned_menurow.h"
#include "ui_skinned_playstatus.h"
#include "ui_skinned_monostereo.h"
#include "ui_skinned_playlist.h"
#include "ui_jumptotrack.h"

#include "ui_main_evlisteners.h"

static GTimeVal cb_time; /* click delay for tristate is defined by TRISTATE_THRESHOLD */

#define ITEM_SEPARATOR {"/-", NULL, NULL, 0, "<Separator>"}
#define TRISTATE_THRESHOLD 200

#define VOLSET_DISP_TIMES 5

enum {
    MAINWIN_SEEK_REV = -1,
    MAINWIN_SEEK_NIL,
    MAINWIN_SEEK_FWD
};

typedef struct _PlaybackInfo PlaybackInfo;

struct _PlaybackInfo {
    gchar *title;
    gint bitrate;
    gint frequency;
    gint n_channels;
};

GtkWidget *mainwin = NULL;
GtkWidget *err = NULL; /* an error dialog for miscellaneous error messages */

static GdkBitmap *nullmask;
static gint balance;

static GtkWidget *mainwin_jtt = NULL;

gint seek_state = MAINWIN_SEEK_NIL;
gint seek_initial_pos = 0;

static GtkWidget *mainwin_menubtn;
static GtkWidget *mainwin_minimize, *mainwin_shade, *mainwin_close;

static GtkWidget *mainwin_rew, *mainwin_fwd;
static GtkWidget *mainwin_eject;
static GtkWidget *mainwin_play, *mainwin_pause, *mainwin_stop;

static GtkWidget *mainwin_shuffle, *mainwin_repeat;
GtkWidget *mainwin_eq, *mainwin_pl;

GtkWidget *mainwin_info;
GtkWidget *mainwin_stime_min, *mainwin_stime_sec;

static GtkWidget *mainwin_rate_text, *mainwin_freq_text,
    *mainwin_othertext;

GtkWidget *mainwin_playstatus;

GtkWidget *mainwin_minus_num, *mainwin_10min_num, *mainwin_min_num;
GtkWidget *mainwin_10sec_num, *mainwin_sec_num;

static gboolean setting_volume = FALSE;

GtkWidget *mainwin_vis;
GtkWidget *mainwin_svis;

GtkWidget *mainwin_sposition = NULL;

static GtkWidget *mainwin_menurow;
static GtkWidget *mainwin_volume, *mainwin_balance;
GtkWidget *mainwin_position;

static GtkWidget *mainwin_monostereo;
static GtkWidget *mainwin_srew, *mainwin_splay, *mainwin_spause;
static GtkWidget *mainwin_sstop, *mainwin_sfwd, *mainwin_seject, *mainwin_about;

static gint mainwin_timeout_id;

static gboolean mainwin_info_text_locked = FALSE;

static int ab_position_a = -1;
static int ab_position_b = -1;

static PlaybackInfo playback_info = { NULL, 0, 0, 0 };

static void mainwin_refresh_visible(void);
static gint mainwin_idle_func(gpointer data);

static void set_timer_mode_menu_cb(TimerMode mode);
static void set_timer_mode(TimerMode mode);
static void change_timer_mode(void);

void mainwin_position_motion_cb(GtkWidget *widget, gint pos);
void mainwin_position_release_cb(GtkWidget *widget, gint pos);

void set_doublesize(gboolean doublesize);
void mainwin_eq_pushed(gboolean toggled);
void mainwin_pl_pushed(gboolean toggled);


/* FIXME: placed here for now */
void
playback_get_sample_params(gint * bitrate,
                           gint * frequency,
                           gint * n_channels)
{
    if (bitrate)
        *bitrate = playback_info.bitrate;

    if (frequency)
        *frequency = playback_info.frequency;

    if (n_channels)
        *n_channels = playback_info.n_channels;
}

static void
playback_set_sample_params(gint bitrate,
                           gint frequency,
                           gint n_channels)
{
    if (bitrate >= 0)
        playback_info.bitrate = bitrate;

    if (frequency >= 0)
        playback_info.frequency = frequency;

    if (n_channels >= 0)
        playback_info.n_channels = n_channels;
}

static void
mainwin_set_title_scroll(gboolean scroll)
{
    cfg.autoscroll = scroll;
    ui_skinned_textbox_set_scroll(mainwin_info, cfg.autoscroll);
}


void
mainwin_set_always_on_top(gboolean always)
{
    GtkAction *action = gtk_action_group_get_action(
      toggleaction_group_others , "view always on top" );
    gtk_toggle_action_set_active( GTK_TOGGLE_ACTION(action) , always );
}

static void
mainwin_set_shape_mask(void)
{
    if (!cfg.player_visible)
        return;

    if (cfg.doublesize == FALSE)
        gtk_widget_shape_combine_mask(mainwin,
                                  skin_get_mask(bmp_active_skin,
                                                SKIN_MASK_MAIN), 0, 0);
    else
        gtk_widget_shape_combine_mask(mainwin, NULL, 0, 0);
}

static void
mainwin_set_shade(gboolean shaded)
{
    GtkAction *action = gtk_action_group_get_action(
      toggleaction_group_others , "roll up player" );
    gtk_toggle_action_set_active( GTK_TOGGLE_ACTION(action) , shaded );
}

static void
mainwin_set_shade_menu_cb(gboolean shaded)
{
    cfg.player_shaded = shaded;

    mainwin_set_shape_mask();
    if (shaded) {
        dock_shade(dock_window_list, GTK_WINDOW(mainwin),
                   MAINWIN_SHADED_HEIGHT * (cfg.doublesize + 1));
    } else {
        gint height = !bmp_active_skin->properties.mainwin_height ? MAINWIN_HEIGHT :
                       bmp_active_skin->properties.mainwin_height;

        dock_shade(dock_window_list, GTK_WINDOW(mainwin), height * (cfg.doublesize + 1));
    }

    mainwin_refresh_visible();
    ui_skinned_set_push_button_data(mainwin_shade, 0, cfg.player_shaded ? 27 : 18, 9, cfg.player_shaded ? 27 : 18);
}

static void
mainwin_vis_set_refresh(RefreshRate rate)
{
    cfg.vis_refresh = rate;
}

static void
mainwin_vis_set_afalloff(FalloffSpeed speed)
{
    cfg.analyzer_falloff = speed;
}

static void
mainwin_vis_set_pfalloff(FalloffSpeed speed)
{
    cfg.peaks_falloff = speed;
}

static void
mainwin_vis_set_analyzer_mode(AnalyzerMode mode)
{
    cfg.analyzer_mode = mode;
}

static void
mainwin_vis_set_analyzer_type(AnalyzerType mode)
{
    cfg.analyzer_type = mode;
}

void
mainwin_vis_set_type(VisType mode)
{
    GtkAction *action;

    switch ( mode )
    {
        case VIS_ANALYZER:
            action = gtk_action_group_get_action(
              radioaction_group_vismode , "vismode analyzer" );
            break;
        case VIS_SCOPE:
            action = gtk_action_group_get_action(
              radioaction_group_vismode , "vismode scope" );
            break;
        case VIS_VOICEPRINT:
            action = gtk_action_group_get_action(
              radioaction_group_vismode , "vismode voiceprint" );
            break;
        case VIS_OFF:
        default:
            action = gtk_action_group_get_action(
              radioaction_group_vismode , "vismode off" );
            break;
    }

    gtk_toggle_action_set_active( GTK_TOGGLE_ACTION(action) , TRUE );
}

static void
mainwin_vis_set_type_menu_cb(VisType mode)
{
    cfg.vis_type = mode;

    if (mode == VIS_OFF) {
        if (cfg.player_shaded) {
            ui_svis_set_visible(mainwin_svis, FALSE);
            ui_vis_set_visible(mainwin_vis, TRUE);
        } else {
            ui_svis_set_visible(mainwin_svis, TRUE);
            ui_vis_set_visible(mainwin_vis, FALSE);
        }
    }
    if (mode == VIS_ANALYZER || mode == VIS_SCOPE || mode == VIS_VOICEPRINT) {
        if (cfg.player_shaded) {
            ui_svis_clear_data(mainwin_svis);
            ui_svis_set_visible(mainwin_svis, TRUE);
            ui_vis_clear_data(mainwin_vis);
            ui_vis_set_visible(mainwin_vis, FALSE);
        } else {
            ui_svis_clear_data(mainwin_svis);
            ui_svis_set_visible(mainwin_svis, FALSE);
            ui_vis_clear_data(mainwin_vis);
            ui_vis_set_visible(mainwin_vis, TRUE);
        }
    }
}

static void
mainwin_menubtn_cb(void)
{
    gint x, y;
    gtk_window_get_position(GTK_WINDOW(mainwin), &x, &y);
    ui_manager_popup_menu_show(GTK_MENU(mainwin_general_menu),
                            x + 6 * (1 + cfg.doublesize),
                            y + MAINWIN_SHADED_HEIGHT * (1 + cfg.doublesize),
                            1, GDK_CURRENT_TIME);
}

static void
mainwin_about_cb(void)
{
    gint x, y;
    gdk_window_get_pointer(NULL, &x, &y, NULL);
    ui_manager_popup_menu_show(GTK_MENU(mainwin_general_menu),
                            x + 6 * (1 + cfg.doublesize),
                            y + MAINWIN_SHADED_HEIGHT * (1 + cfg.doublesize),
                            1, GDK_CURRENT_TIME);
}

void
mainwin_minimize_cb(void)
{
    if (!mainwin)
        return;

    gtk_window_iconify(GTK_WINDOW(mainwin));
}

static void
mainwin_shade_toggle(void)
{
    mainwin_set_shade(!cfg.player_shaded);
}

void
mainwin_quit_cb(void)
{
    GList *playlists = NULL, *playlists_top = NULL;

    playlist_stop_get_info_thread();

    gtk_widget_hide(equalizerwin);
    gtk_widget_hide(playlistwin);
    gtk_widget_hide(mainwin);

    g_source_remove(mainwin_timeout_id);

    bmp_config_save();
    gtk_accel_map_save(bmp_paths[BMP_PATH_ACCEL_FILE]);

    plugin_system_cleanup();


    /* free and clear each playlist */
    playlists = playlist_get_playlists();
    playlists_top = playlists;
    while ( playlists != NULL )
    {
        playlist_clear((Playlist*)playlists->data);
        playlist_free((Playlist*)playlists->data);
        playlists = g_list_next(playlists);
    }
    g_list_free( playlists_top );

    gtk_main_quit();

    exit(EXIT_SUCCESS);
}

gboolean
mainwin_vis_cb(GtkWidget *widget, GdkEventButton *event)
{
    if (event->button == 1) {
        cfg.vis_type++;

        if (cfg.vis_type > VIS_OFF)
            cfg.vis_type = VIS_ANALYZER;

        mainwin_vis_set_type(cfg.vis_type);
    } else if (event->button == 3) {
        gint x, y;
        gdk_window_get_pointer(NULL, &x, &y, NULL);
        ui_manager_popup_menu_show(GTK_MENU(mainwin_visualization_menu),
                                   x, y, 3, event->time);
    }
    return TRUE;
}

static void
mainwin_destroy(GtkWidget * widget, gpointer data)
{
    mainwin_quit_cb();
}

static gchar *mainwin_tb_old_text = NULL;

void
mainwin_lock_info_text(const gchar * text)
{
    if (mainwin_info_text_locked != TRUE)
        mainwin_tb_old_text = g_strdup(bmp_active_skin->properties.mainwin_othertext_is_status ?
        UI_SKINNED_TEXTBOX(mainwin_othertext)->text : UI_SKINNED_TEXTBOX(mainwin_info)->text);

    mainwin_info_text_locked = TRUE;
    if (bmp_active_skin->properties.mainwin_othertext_is_status)
        ui_skinned_textbox_set_text(mainwin_othertext, text);
    else
        ui_skinned_textbox_set_text(mainwin_info, text);
}

void
mainwin_release_info_text(void)
{
    mainwin_info_text_locked = FALSE;

    if (mainwin_tb_old_text != NULL)
    {
        if (bmp_active_skin->properties.mainwin_othertext_is_status)
            ui_skinned_textbox_set_text(mainwin_othertext, mainwin_tb_old_text);
        else
            ui_skinned_textbox_set_text(mainwin_info, mainwin_tb_old_text);
        g_free(mainwin_tb_old_text);
        mainwin_tb_old_text = NULL;
    }
}


static gchar *
make_mainwin_title(const gchar * title)
{
    if (title)
        return g_strdup_printf(_("%s - Audacious"), title);
    else
        return g_strdup(_("Audacious"));
}

void
mainwin_set_song_title(const gchar * title)
{
    gchar *mainwin_title_text = make_mainwin_title(title);
    gtk_window_set_title(GTK_WINDOW(mainwin), mainwin_title_text);
}

static void
mainwin_refresh_visible(void)
{
    if (!bmp_active_skin || !cfg.player_visible)
        return;

    gtk_widget_show_all(mainwin);

    if (!bmp_active_skin->properties.mainwin_text_visible)
        gtk_widget_hide(mainwin_info);

    if (!bmp_active_skin->properties.mainwin_vis_visible)
        gtk_widget_hide(mainwin_vis);

    if (!bmp_active_skin->properties.mainwin_menurow_visible)
        gtk_widget_hide(mainwin_menurow);

    if (bmp_active_skin->properties.mainwin_othertext) {
        gtk_widget_hide(mainwin_rate_text);
        gtk_widget_hide(mainwin_freq_text);
        gtk_widget_hide(mainwin_monostereo);

        if (!bmp_active_skin->properties.mainwin_othertext_visible)
            gtk_widget_hide(mainwin_othertext);
    } else {
        gtk_widget_hide(mainwin_othertext);
    }

    if (!bmp_active_skin->properties.mainwin_vis_visible)
        gtk_widget_hide(mainwin_vis);

    if (!playback_get_playing()) {
        gtk_widget_hide(mainwin_minus_num);
        gtk_widget_hide(mainwin_10min_num);
        gtk_widget_hide(mainwin_min_num);
        gtk_widget_hide(mainwin_10sec_num);
        gtk_widget_hide(mainwin_sec_num);

        gtk_widget_hide(mainwin_stime_min);
        gtk_widget_hide(mainwin_stime_sec);

        gtk_widget_hide(mainwin_position);
        gtk_widget_hide(mainwin_sposition);
    }

    if (cfg.player_shaded) {
        ui_svis_clear_data(mainwin_svis);
        if (cfg.vis_type != VIS_OFF)
            ui_svis_set_visible(mainwin_svis, TRUE);
        else
            ui_svis_set_visible(mainwin_svis, FALSE);

        ui_skinned_textbox_set_scroll(mainwin_info, FALSE);
        if (!playback_get_playing()) {
            gtk_widget_hide(mainwin_sposition);
            gtk_widget_hide(mainwin_stime_min);
            gtk_widget_hide(mainwin_stime_sec);
        }
    } else {
        gtk_widget_hide(mainwin_srew);
        gtk_widget_hide(mainwin_splay);
        gtk_widget_hide(mainwin_spause);
        gtk_widget_hide(mainwin_sstop);
        gtk_widget_hide(mainwin_sfwd);
        gtk_widget_hide(mainwin_seject);
        gtk_widget_hide(mainwin_stime_min);
        gtk_widget_hide(mainwin_stime_sec);
        gtk_widget_hide(mainwin_svis);
        gtk_widget_hide(mainwin_sposition);
        ui_vis_clear_data(mainwin_vis);
        if (cfg.vis_type != VIS_OFF)
            ui_vis_set_visible(mainwin_vis, TRUE);
        else
            ui_vis_set_visible(mainwin_vis, FALSE);

        ui_skinned_textbox_set_scroll(mainwin_info, cfg.autoscroll);
    }
}

void
mainwin_refresh_hints(void)
{
    /* positioning and size attributes */
    if (bmp_active_skin->properties.mainwin_vis_x && bmp_active_skin->properties.mainwin_vis_y)
    gtk_fixed_move(GTK_FIXED(SKINNED_WINDOW(mainwin)->fixed), GTK_WIDGET(mainwin_vis), bmp_active_skin->properties.mainwin_vis_x,
        bmp_active_skin->properties.mainwin_vis_y);

    if (bmp_active_skin->properties.mainwin_vis_width)
    gtk_widget_set_size_request(mainwin_vis, bmp_active_skin->properties.mainwin_vis_width*(1+cfg.doublesize),
        UI_VIS(mainwin_vis)->height*(1+cfg.doublesize));

    if (bmp_active_skin->properties.mainwin_text_x && bmp_active_skin->properties.mainwin_text_y)
    gtk_fixed_move(GTK_FIXED(SKINNED_WINDOW(mainwin)->fixed), GTK_WIDGET(mainwin_info), bmp_active_skin->properties.mainwin_text_x,
        bmp_active_skin->properties.mainwin_text_y);

    if (bmp_active_skin->properties.mainwin_text_width)
    gtk_widget_set_size_request(mainwin_info, bmp_active_skin->properties.mainwin_text_width*(1+cfg.doublesize),
        UI_SKINNED_TEXTBOX(mainwin_info)->height*(1+cfg.doublesize));

    if (bmp_active_skin->properties.mainwin_infobar_x && bmp_active_skin->properties.mainwin_infobar_y)
    gtk_fixed_move(GTK_FIXED(SKINNED_WINDOW(mainwin)->fixed), GTK_WIDGET(mainwin_othertext), bmp_active_skin->properties.mainwin_infobar_x,
        bmp_active_skin->properties.mainwin_infobar_y);

    if (bmp_active_skin->properties.mainwin_number_0_x && bmp_active_skin->properties.mainwin_number_0_y)
    gtk_fixed_move(GTK_FIXED(SKINNED_WINDOW(mainwin)->fixed), GTK_WIDGET(mainwin_minus_num), bmp_active_skin->properties.mainwin_number_0_x,
        bmp_active_skin->properties.mainwin_number_0_y);

    if (bmp_active_skin->properties.mainwin_number_1_x && bmp_active_skin->properties.mainwin_number_1_y)
    gtk_fixed_move(GTK_FIXED(SKINNED_WINDOW(mainwin)->fixed), GTK_WIDGET(mainwin_10min_num), bmp_active_skin->properties.mainwin_number_1_x,
        bmp_active_skin->properties.mainwin_number_1_y);

    if (bmp_active_skin->properties.mainwin_number_2_x && bmp_active_skin->properties.mainwin_number_2_y)
    gtk_fixed_move(GTK_FIXED(SKINNED_WINDOW(mainwin)->fixed), GTK_WIDGET(mainwin_min_num), bmp_active_skin->properties.mainwin_number_2_x,
        bmp_active_skin->properties.mainwin_number_2_y);

    if (bmp_active_skin->properties.mainwin_number_3_x && bmp_active_skin->properties.mainwin_number_3_y)
    gtk_fixed_move(GTK_FIXED(SKINNED_WINDOW(mainwin)->fixed), GTK_WIDGET(mainwin_10sec_num), bmp_active_skin->properties.mainwin_number_3_x,
        bmp_active_skin->properties.mainwin_number_3_y);

    if (bmp_active_skin->properties.mainwin_number_4_x && bmp_active_skin->properties.mainwin_number_4_y)
    gtk_fixed_move(GTK_FIXED(SKINNED_WINDOW(mainwin)->fixed), GTK_WIDGET(mainwin_sec_num), bmp_active_skin->properties.mainwin_number_4_x,
        bmp_active_skin->properties.mainwin_number_4_y);

    if (bmp_active_skin->properties.mainwin_playstatus_x && bmp_active_skin->properties.mainwin_playstatus_y)
    gtk_fixed_move(GTK_FIXED(SKINNED_WINDOW(mainwin)->fixed), mainwin_playstatus, bmp_active_skin->properties.mainwin_playstatus_x,
        bmp_active_skin->properties.mainwin_playstatus_y);

    if (bmp_active_skin->properties.mainwin_volume_x && bmp_active_skin->properties.mainwin_volume_y)
    gtk_fixed_move(GTK_FIXED(SKINNED_WINDOW(mainwin)->fixed), GTK_WIDGET(mainwin_volume), bmp_active_skin->properties.mainwin_volume_x,
        bmp_active_skin->properties.mainwin_volume_y);

    if (bmp_active_skin->properties.mainwin_balance_x && bmp_active_skin->properties.mainwin_balance_y)
    gtk_fixed_move(GTK_FIXED(SKINNED_WINDOW(mainwin)->fixed), GTK_WIDGET(mainwin_balance), bmp_active_skin->properties.mainwin_balance_x,
        bmp_active_skin->properties.mainwin_balance_y);

    if (bmp_active_skin->properties.mainwin_position_x && bmp_active_skin->properties.mainwin_position_y)
    gtk_fixed_move(GTK_FIXED(SKINNED_WINDOW(mainwin)->fixed), GTK_WIDGET(mainwin_position), bmp_active_skin->properties.mainwin_position_x,
        bmp_active_skin->properties.mainwin_position_y);

    if (bmp_active_skin->properties.mainwin_previous_x && bmp_active_skin->properties.mainwin_previous_y)
    gtk_fixed_move(GTK_FIXED(SKINNED_WINDOW(mainwin)->fixed), mainwin_rew, bmp_active_skin->properties.mainwin_previous_x,
        bmp_active_skin->properties.mainwin_previous_y);

    if (bmp_active_skin->properties.mainwin_play_x && bmp_active_skin->properties.mainwin_play_y)
    gtk_fixed_move(GTK_FIXED(SKINNED_WINDOW(mainwin)->fixed), GTK_WIDGET(mainwin_play), bmp_active_skin->properties.mainwin_play_x,
        bmp_active_skin->properties.mainwin_play_y);

    if (bmp_active_skin->properties.mainwin_pause_x && bmp_active_skin->properties.mainwin_pause_y)
    gtk_fixed_move(GTK_FIXED(SKINNED_WINDOW(mainwin)->fixed), GTK_WIDGET(mainwin_pause), bmp_active_skin->properties.mainwin_pause_x,
        bmp_active_skin->properties.mainwin_pause_y);

    if (bmp_active_skin->properties.mainwin_stop_x && bmp_active_skin->properties.mainwin_stop_y)
    gtk_fixed_move(GTK_FIXED(SKINNED_WINDOW(mainwin)->fixed), GTK_WIDGET(mainwin_stop), bmp_active_skin->properties.mainwin_stop_x,
        bmp_active_skin->properties.mainwin_stop_y);

    if (bmp_active_skin->properties.mainwin_next_x && bmp_active_skin->properties.mainwin_next_y)
    gtk_fixed_move(GTK_FIXED(SKINNED_WINDOW(mainwin)->fixed), GTK_WIDGET(mainwin_fwd), bmp_active_skin->properties.mainwin_next_x,
        bmp_active_skin->properties.mainwin_next_y);

    if (bmp_active_skin->properties.mainwin_eject_x && bmp_active_skin->properties.mainwin_eject_y)
    gtk_fixed_move(GTK_FIXED(SKINNED_WINDOW(mainwin)->fixed), GTK_WIDGET(mainwin_eject), bmp_active_skin->properties.mainwin_eject_x,
        bmp_active_skin->properties.mainwin_eject_y);

    if (bmp_active_skin->properties.mainwin_eqbutton_x && bmp_active_skin->properties.mainwin_eqbutton_y)
    gtk_fixed_move(GTK_FIXED(SKINNED_WINDOW(mainwin)->fixed), GTK_WIDGET(mainwin_eq), bmp_active_skin->properties.mainwin_eqbutton_x,
        bmp_active_skin->properties.mainwin_eqbutton_y);

    if (bmp_active_skin->properties.mainwin_plbutton_x && bmp_active_skin->properties.mainwin_plbutton_y)
    gtk_fixed_move(GTK_FIXED(SKINNED_WINDOW(mainwin)->fixed), GTK_WIDGET(mainwin_pl), bmp_active_skin->properties.mainwin_plbutton_x,
        bmp_active_skin->properties.mainwin_plbutton_y);

    if (bmp_active_skin->properties.mainwin_shuffle_x && bmp_active_skin->properties.mainwin_shuffle_y)
    gtk_fixed_move(GTK_FIXED(SKINNED_WINDOW(mainwin)->fixed), GTK_WIDGET(mainwin_shuffle), bmp_active_skin->properties.mainwin_shuffle_x,
        bmp_active_skin->properties.mainwin_shuffle_y);

    if (bmp_active_skin->properties.mainwin_repeat_x && bmp_active_skin->properties.mainwin_repeat_y)
    gtk_fixed_move(GTK_FIXED(SKINNED_WINDOW(mainwin)->fixed), GTK_WIDGET(mainwin_repeat), bmp_active_skin->properties.mainwin_repeat_x,
        bmp_active_skin->properties.mainwin_repeat_y);

    if (bmp_active_skin->properties.mainwin_about_x && bmp_active_skin->properties.mainwin_about_y)
    gtk_fixed_move(GTK_FIXED(SKINNED_WINDOW(mainwin)->fixed), GTK_WIDGET(mainwin_about), bmp_active_skin->properties.mainwin_about_x,
        bmp_active_skin->properties.mainwin_about_y);

    if (bmp_active_skin->properties.mainwin_minimize_x && bmp_active_skin->properties.mainwin_minimize_y)
    gtk_fixed_move(GTK_FIXED(SKINNED_WINDOW(mainwin)->fixed), GTK_WIDGET(mainwin_minimize), cfg.player_shaded ? 244 : bmp_active_skin->properties.mainwin_minimize_x,
        cfg.player_shaded ? 3 : bmp_active_skin->properties.mainwin_minimize_y);

    if (bmp_active_skin->properties.mainwin_shade_x && bmp_active_skin->properties.mainwin_shade_y)
    gtk_fixed_move(GTK_FIXED(SKINNED_WINDOW(mainwin)->fixed), GTK_WIDGET(mainwin_shade), cfg.player_shaded ? 254 : bmp_active_skin->properties.mainwin_shade_x,
        cfg.player_shaded ? 3 : bmp_active_skin->properties.mainwin_shade_y);

    if (bmp_active_skin->properties.mainwin_close_x && bmp_active_skin->properties.mainwin_close_y)
    gtk_fixed_move(GTK_FIXED(SKINNED_WINDOW(mainwin)->fixed), GTK_WIDGET(mainwin_close), cfg.player_shaded ? 264 : bmp_active_skin->properties.mainwin_close_x,
        cfg.player_shaded ? 3 : bmp_active_skin->properties.mainwin_close_y);

    mainwin_refresh_visible();
    /* window size, mainwinWidth && mainwinHeight properties */
    if (bmp_active_skin->properties.mainwin_height && bmp_active_skin->properties.mainwin_width)
    {
    gint width, height;

    gdk_window_get_size(mainwin->window, &width, &height);

        if (width == bmp_active_skin->properties.mainwin_width * (cfg.doublesize + 1) &&
        height == bmp_active_skin->properties.mainwin_height * (cfg.doublesize + 1))
            return;

        dock_window_resize(GTK_WINDOW(mainwin), cfg.player_shaded ? MAINWIN_SHADED_WIDTH * (cfg.doublesize + 1) : bmp_active_skin->properties.mainwin_width * (cfg.doublesize + 1),
        cfg.player_shaded ? MAINWIN_SHADED_HEIGHT * (cfg.doublesize + 1) : bmp_active_skin->properties.mainwin_height * (cfg.doublesize + 1),
        bmp_active_skin->properties.mainwin_width * (cfg.doublesize + 1),
        bmp_active_skin->properties.mainwin_height * (cfg.doublesize + 1));

    gdk_flush();
    }
}

void
mainwin_set_song_info(gint bitrate,
                      gint frequency,
                      gint n_channels)
{
    gchar *text;
    gchar *title;
    Playlist *playlist = playlist_get_active();

    playback_set_sample_params(bitrate, frequency, n_channels);

    GDK_THREADS_ENTER();
    if (bitrate != -1) {
        bitrate /= 1000;

        if (bitrate < 1000) {
            /* Show bitrate in 1000s */
            text = g_strdup_printf("%3d", bitrate);
            ui_skinned_textbox_set_text(mainwin_rate_text, text);
        }
        else {
            /* Show bitrate in 100,000s */
            text = g_strdup_printf("%2dH", bitrate / 100);
            ui_skinned_textbox_set_text(mainwin_rate_text, text);
        }
    }
    else
        ui_skinned_textbox_set_text(mainwin_rate_text, _("VBR"));

    /* Show sampling frequency in kHz */
    text = g_strdup_printf("%2d", frequency / 1000);
    ui_skinned_textbox_set_text(mainwin_freq_text, text);

    ui_skinned_monostereo_set_num_channels(mainwin_monostereo, n_channels);

    if (!playback_get_paused() && mainwin_playstatus != NULL)
        ui_skinned_playstatus_set_status(mainwin_playstatus, STATUS_PLAY);

    if (bmp_active_skin && bmp_active_skin->properties.mainwin_othertext)
    {
        if (bitrate != -1)
            text = g_strdup_printf("%d kbps, %0.1f kHz, %s",
            bitrate,
            (gfloat) frequency / 1000,
            (n_channels > 1) ? _("stereo") : _("mono"));
        else
            text = g_strdup_printf("VBR, %0.1f kHz, %s",
            (gfloat) frequency / 1000,
            (n_channels > 1) ? _("stereo") : _("mono"));

        ui_skinned_textbox_set_text(mainwin_othertext, text);
    }

    title = playlist_get_info_text(playlist);
    mainwin_set_song_title(title);
    g_free(title);
    GDK_THREADS_LEAVE();
}

void
mainwin_clear_song_info(void)
{
    if (!mainwin)
        return;

    /* clear title */
    mainwin_set_song_title(NULL);

    /* clear sampling parameters */
    playback_set_sample_params(0, 0, 0);

    UI_SKINNED_HORIZONTAL_SLIDER(mainwin_position)->pressed = FALSE;
    UI_SKINNED_HORIZONTAL_SLIDER(mainwin_sposition)->pressed = FALSE;

    /* clear sampling parameter displays */
    ui_skinned_textbox_set_text(mainwin_rate_text, "   ");
    ui_skinned_textbox_set_text(mainwin_freq_text, "  ");
    ui_skinned_monostereo_set_num_channels(mainwin_monostereo, 0);

    if (mainwin_playstatus != NULL)
        ui_skinned_playstatus_set_status(mainwin_playstatus, STATUS_STOP);

    mainwin_refresh_visible();

    playlistwin_hide_timer();
    ui_vis_clear_data(mainwin_vis);
    ui_svis_clear_data(mainwin_svis);
}

void
mainwin_disable_seekbar(void)
{
    if (!mainwin)
        return;

    gtk_widget_hide(mainwin_position);
    gtk_widget_hide(mainwin_sposition);
}

static gboolean
mainwin_mouse_button_release(GtkWidget * widget,
                             GdkEventButton * event,
                             gpointer callback_data)
{
    if (dock_is_moving(GTK_WINDOW(mainwin))) {
        dock_move_release(GTK_WINDOW(mainwin));
    }

    return FALSE;
}

void
mainwin_scrolled(GtkWidget *widget, GdkEventScroll *event,
    gpointer callback_data)
{
    Playlist *playlist = playlist_get_active();

    switch (event->direction) {
    case GDK_SCROLL_UP:
        mainwin_set_volume_diff(cfg.mouse_change);
        break;
    case GDK_SCROLL_DOWN:
        mainwin_set_volume_diff(-cfg.mouse_change);
        break;
    case GDK_SCROLL_LEFT:
        if (playlist_get_current_length(playlist) != -1)
            playback_seek(CLAMP(playback_get_time() - 1000,
                0, playlist_get_current_length(playlist)) / 1000);
        break;
    case GDK_SCROLL_RIGHT:
        if (playlist_get_current_length(playlist) != -1)
            playback_seek(CLAMP(playback_get_time() + 1000,
                0, playlist_get_current_length(playlist)) / 1000);
        break;
    }
}

static gboolean
mainwin_mouse_button_press(GtkWidget * widget,
                           GdkEventButton * event,
                           gpointer callback_data)
{
    if (cfg.doublesize) {
        /*
         * A hack to make doublesize transparent to callbacks.
         * We should make a copy of this data instead of
         * tampering with the data we get from gtk+
         */
        event->x /= 2;
        event->y /= 2;
    }

    if (event->button == 1 && event->type == GDK_BUTTON_PRESS &&
        (cfg.easy_move || event->y < 14)) {
        gtk_window_present(GTK_WINDOW(mainwin));
        dock_move_press(dock_window_list, GTK_WINDOW(mainwin), event,
                        TRUE);
    }
    else if (event->button == 1 && event->type == GDK_2BUTTON_PRESS && event->y < 14) {
        mainwin_set_shade(!cfg.player_shaded);
        if (dock_is_moving(GTK_WINDOW(mainwin)))
            dock_move_release(GTK_WINDOW(mainwin));
    }

    if (event->button == 3) {
        if ( (event->y > 70) && (event->x < 128) )
        {

            ui_manager_popup_menu_show(GTK_MENU(mainwin_playback_menu),
                                    event->x_root,
                                    event->y_root, 3, event->time);
        } else {
            /*
             * Pop up the main menu a few pixels down.
             * This will avoid that anything is selected
             * if one right-clicks to focus the window
             * without raising it.
             *
             ***MD I think the above is stupid, people don't expect this
             *
             */
            ui_manager_popup_menu_show(GTK_MENU(mainwin_general_menu),
                                    event->x_root,
                                    event->y_root, 3, event->time);
        }
    }

    return FALSE;
}

static gboolean
mainwin_keypress(GtkWidget * grab_widget,
                 GdkEventKey * event,
                 gpointer data)
{
    Playlist *playlist = playlist_get_active();

    switch (event->keyval) {

    case GDK_Up:
    case GDK_KP_Up:
    case GDK_KP_8:
        mainwin_set_volume_diff(2);
        break;
    case GDK_Down:
    case GDK_KP_Down:
    case GDK_KP_2:
        mainwin_set_volume_diff(-2);
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
    case GDK_KP_Insert:
        ui_jump_to_track();
        break;
    case GDK_Return:
    case GDK_KP_Enter:
    case GDK_KP_5:
        mainwin_play_pushed();
        break;
    case GDK_space:
        playback_pause();
        break;
    case GDK_Escape:
        mainwin_minimize_cb();
        break;
    case GDK_Tab:
        if (event->state & GDK_CONTROL_MASK)
            gtk_window_present(GTK_WINDOW(equalizerwin));
        break;
    default:
        return FALSE;
    }

    return TRUE;
}

static void
mainwin_jump_to_time_cb(GtkWidget * widget,
                        GtkWidget * entry)
{
    guint min = 0, sec = 0, params;
    gint time;
    Playlist *playlist = playlist_get_active();

    params = sscanf(gtk_entry_get_text(GTK_ENTRY(entry)), "%u:%u",
                    &min, &sec);
    if (params == 2)
        time = (min * 60) + sec;
    else if (params == 1)
        time = min;
    else
        return;

    if (playlist_get_current_length(playlist) > -1 &&
        time <= (playlist_get_current_length(playlist) / 1000))
    {
        playback_seek(time);
        gtk_widget_destroy(mainwin_jtt);
    }
}


void
mainwin_jump_to_time(void)
{
    GtkWidget *vbox, *hbox_new, *hbox_total;
    GtkWidget *time_entry, *label, *bbox, *jump, *cancel;
    guint tindex;
    gchar time_str[10];

    if (!playback_get_playing()) {
        report_error("JIT can't be launched when no track is being played.\n");
        return;
    }

    if (mainwin_jtt) {
        gtk_window_present(GTK_WINDOW(mainwin_jtt));
        return;
    }

    mainwin_jtt = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_type_hint(GTK_WINDOW(mainwin_jtt),
                             GDK_WINDOW_TYPE_HINT_DIALOG);

    gtk_window_set_title(GTK_WINDOW(mainwin_jtt), _("Jump to Time"));
    gtk_window_set_position(GTK_WINDOW(mainwin_jtt), GTK_WIN_POS_CENTER);
    gtk_window_set_transient_for(GTK_WINDOW(mainwin_jtt),
                                 GTK_WINDOW(mainwin));

    g_signal_connect(mainwin_jtt, "destroy",
                     G_CALLBACK(gtk_widget_destroyed), &mainwin_jtt);
    gtk_container_border_width(GTK_CONTAINER(mainwin_jtt), 10);

    vbox = gtk_vbox_new(FALSE, 5);
    gtk_container_add(GTK_CONTAINER(mainwin_jtt), vbox);

    hbox_new = gtk_hbox_new(FALSE, 0);
    gtk_box_pack_start(GTK_BOX(vbox), hbox_new, TRUE, TRUE, 5);

    time_entry = gtk_entry_new();
    gtk_box_pack_start(GTK_BOX(hbox_new), time_entry, FALSE, FALSE, 5);
    g_signal_connect(time_entry, "activate",
                     G_CALLBACK(mainwin_jump_to_time_cb), time_entry);

    gtk_widget_set_size_request(time_entry, 70, -1);
    label = gtk_label_new(_("minutes:seconds"));
    gtk_box_pack_start(GTK_BOX(hbox_new), label, FALSE, FALSE, 5);

    hbox_total = gtk_hbox_new(FALSE, 0);
    gtk_box_pack_start(GTK_BOX(vbox), hbox_total, TRUE, TRUE, 5);
    gtk_widget_show(hbox_total);

    /* FIXME: Disable display of current track length. It's not
       updated when track changes */
#if 0
    label = gtk_label_new(_("Track length:"));
    gtk_box_pack_start(GTK_BOX(hbox_total), label, FALSE, FALSE, 5);

    len = playlist_get_current_length() / 1000;
    g_snprintf(time_str, sizeof(time_str), "%u:%2.2u", len / 60, len % 60);
    label = gtk_label_new(time_str);

    gtk_box_pack_start(GTK_BOX(hbox_total), label, FALSE, FALSE, 10);
#endif

    bbox = gtk_hbutton_box_new();
    gtk_box_pack_start(GTK_BOX(vbox), bbox, TRUE, TRUE, 0);
    gtk_button_box_set_layout(GTK_BUTTON_BOX(bbox), GTK_BUTTONBOX_END);
    gtk_button_box_set_spacing(GTK_BUTTON_BOX(bbox), 5);

    cancel = gtk_button_new_from_stock(GTK_STOCK_CANCEL);
    GTK_WIDGET_SET_FLAGS(cancel, GTK_CAN_DEFAULT);
    gtk_container_add(GTK_CONTAINER(bbox), cancel);
    g_signal_connect_swapped(cancel, "clicked",
                             G_CALLBACK(gtk_widget_destroy), mainwin_jtt);

    jump = gtk_button_new_from_stock(GTK_STOCK_JUMP_TO);
    GTK_WIDGET_SET_FLAGS(jump, GTK_CAN_DEFAULT);
    gtk_container_add(GTK_CONTAINER(bbox), jump);
    g_signal_connect(jump, "clicked",
                     G_CALLBACK(mainwin_jump_to_time_cb), time_entry);

    tindex = playback_get_time() / 1000;
    g_snprintf(time_str, sizeof(time_str), "%u:%2.2u", tindex / 60,
               tindex % 60);
    gtk_entry_set_text(GTK_ENTRY(time_entry), time_str);

    gtk_entry_select_region(GTK_ENTRY(time_entry), 0, strlen(time_str));

    gtk_widget_show_all(mainwin_jtt);

    gtk_widget_grab_focus(time_entry);
    gtk_widget_grab_default(jump);
}

/*
 * Rewritten 09/13/06:
 *
 * Remove all of this flaky iter/sourcelist/strsplit stuff.
 * All we care about is the filepath.
 *
 * We can figure this out and easily pass it to g_filename_from_uri().
 *   - nenolod
 */
void
mainwin_drag_data_received(GtkWidget * widget,
                           GdkDragContext * context,
                           gint x,
                           gint y,
                           GtkSelectionData * selection_data,
                           guint info,
                           guint time,
                           gpointer user_data)
{
    Playlist *playlist = playlist_get_active();

    g_return_if_fail(selection_data != NULL);
    g_return_if_fail(selection_data->data != NULL);

    if (str_has_prefix_nocase((gchar *) selection_data->data, "fonts:///"))
    {
        gchar *path = (gchar *) selection_data->data;
        gchar *decoded = g_filename_from_uri(path, NULL, NULL);

        if (decoded == NULL)
            return;

        cfg.playlist_font = g_strconcat(decoded, strrchr(cfg.playlist_font, ' '), NULL);
        ui_skinned_playlist_set_font(cfg.playlist_font);
        playlistwin_update_list(playlist);

        g_free(decoded);

        return;
    }

    playlist_clear(playlist);
    playlist_add_url(playlist, (gchar *) selection_data->data);
    playback_initiate();
}

static void
on_add_url_add_clicked(GtkWidget * widget,
                       GtkWidget * entry)
{
    const gchar *text = gtk_entry_get_text(GTK_ENTRY(entry));
    if (text && *text)
        playlist_add_url(playlist_get_active(), text);
}

static void
on_add_url_ok_clicked(GtkWidget * widget,
                      GtkWidget * entry)
{
    Playlist *playlist = playlist_get_active();

    const gchar *text = gtk_entry_get_text(GTK_ENTRY(entry));
    if (text && *text)
    {
        playlist_clear(playlist);
        playlist_add_url(playlist, text);
        playback_initiate();
    }
}

static void
on_visibility_warning_toggle(GtkToggleButton *tbt, gpointer unused)
{
    cfg.warn_about_win_visibility = !gtk_toggle_button_get_active(tbt);
}

static void
on_visibility_warning_response(GtkDialog *dlg, gint r_id, gpointer unused)
{
    switch (r_id)
    {
        case GTK_RESPONSE_OK:
            mainwin_show(TRUE);
            break;
        case GTK_RESPONSE_CANCEL:
        default:
            break;
    }
    gtk_widget_destroy(GTK_WIDGET(dlg));
}

void
mainwin_show_visibility_warning(void)
{
    if (cfg.warn_about_win_visibility)
    {
        GtkWidget *label, *checkbt, *vbox;
        GtkWidget *warning_dlg = gtk_dialog_new_with_buttons( _("Audacious - visibility warning") ,
            GTK_WINDOW(mainwin) , GTK_DIALOG_DESTROY_WITH_PARENT ,
            _("Show main player window") , GTK_RESPONSE_OK ,
            _("Ignore") , GTK_RESPONSE_CANCEL , NULL );
        vbox = gtk_vbox_new( FALSE , 4 );
        gtk_container_set_border_width( GTK_CONTAINER(vbox) , 4 );
        gtk_box_pack_start( GTK_BOX(GTK_DIALOG(warning_dlg)->vbox) , vbox , TRUE , TRUE , 0 );
        label = gtk_label_new( _("Audacious has been started with all of its windows hidden.\n"
                                 "You may want to show the player window again to control Audacious; "
                                 "otherwise, you'll have to control it remotely via audtool or "
                                 "enabled plugins (such as the statusicon plugin).") );
        gtk_label_set_line_wrap( GTK_LABEL(label) , TRUE );
        gtk_misc_set_alignment( GTK_MISC(label) , 0.0 , 0.0 );
        checkbt = gtk_check_button_new_with_label( _("Always ignore, show/hide is controlled remotely") );
        gtk_box_pack_start( GTK_BOX(vbox) , label , TRUE , TRUE , 0 );
        gtk_box_pack_start( GTK_BOX(vbox) , checkbt , TRUE , TRUE , 0 );
        g_signal_connect( G_OBJECT(checkbt) , "toggled" ,
            G_CALLBACK(on_visibility_warning_toggle) , NULL );
        g_signal_connect( G_OBJECT(warning_dlg) , "response" ,
            G_CALLBACK(on_visibility_warning_response) , NULL );
        gtk_widget_show_all(warning_dlg);
    }
}

void
mainwin_show_add_url_window(void)
{
    static GtkWidget *url_window = NULL;

    if (!url_window) {
        url_window =
            util_add_url_dialog_new(_("Enter location to play:"),
                    G_CALLBACK(on_add_url_ok_clicked),
                                    G_CALLBACK(on_add_url_add_clicked));
        gtk_window_set_transient_for(GTK_WINDOW(url_window),
                                     GTK_WINDOW(mainwin));
        g_signal_connect(url_window, "destroy",
                         G_CALLBACK(gtk_widget_destroyed),
                         &url_window);
    }

    gtk_window_present(GTK_WINDOW(url_window));
}

static void
check_set( GtkActionGroup * action_group ,
           const gchar * action_name ,
           gboolean is_on )
{
    /* check_set noew uses gtkaction */
    GtkAction *action = gtk_action_group_get_action( action_group , action_name );
    if ( action != NULL )
        gtk_toggle_action_set_active( GTK_TOGGLE_ACTION(action) , is_on );
    return;
}

void
mainwin_eject_pushed(void)
{
    run_filebrowser(PLAY_BUTTON);
}

void
mainwin_rev_pushed(void)
{
    g_get_current_time(&cb_time);

    seek_initial_pos = ui_skinned_horizontal_slider_get_position(mainwin_position);
    seek_state = MAINWIN_SEEK_REV;
}

void
mainwin_rev_release(void)
{
    GTimeVal now_time;
    GTimeVal delta_time;
    gulong now_dur;

    g_get_current_time(&now_time);

    delta_time.tv_usec = now_time.tv_usec - cb_time.tv_usec;
    delta_time.tv_sec = now_time.tv_sec - cb_time.tv_sec;

    now_dur = labs((delta_time.tv_sec * 1000) + (glong) (delta_time.tv_usec / 1000));

    if ( now_dur <= TRISTATE_THRESHOLD )
    {
        /* interpret as 'skip to previous song' */
        playlist_prev(playlist_get_active());
    }
    else
    {
        /* interpret as 'seek' */
        mainwin_position_release_cb( mainwin_position, ui_skinned_horizontal_slider_get_position(mainwin_position) );
    }

    seek_state = MAINWIN_SEEK_NIL;
}

void
mainwin_fwd_pushed(void)
{
    g_get_current_time(&cb_time);
    seek_initial_pos = ui_skinned_horizontal_slider_get_position(mainwin_position);
    seek_state = MAINWIN_SEEK_FWD;
}

void
mainwin_fwd_release(void)
{
    GTimeVal now_time;
    GTimeVal delta_time;
    gulong now_dur;

    g_get_current_time(&now_time);

    delta_time.tv_usec = now_time.tv_usec - cb_time.tv_usec;
    delta_time.tv_sec = now_time.tv_sec - cb_time.tv_sec;

    now_dur = labs((delta_time.tv_sec * 1000) + (glong) (delta_time.tv_usec / 1000));

    if ( now_dur <= TRISTATE_THRESHOLD )
    {
        /* interpret as 'skip to next song' */
        playlist_next(playlist_get_active());
    }
    else
    {
        /* interpret as 'seek' */
        mainwin_position_release_cb( mainwin_position, ui_skinned_horizontal_slider_get_position(mainwin_position) );
    }

    seek_state = MAINWIN_SEEK_NIL;
}

void
mainwin_play_pushed(void)
{
    if (ab_position_a != -1)
        playback_seek(ab_position_a / 1000);
    if (playback_get_paused()) {
        playback_pause();
        return;
    }

    if (playlist_get_length(playlist_get_active()))
        playback_initiate();
    else
        mainwin_eject_pushed();
}

void
mainwin_stop_pushed(void)
{
    ip_data.stop = TRUE;
    playback_stop();
    mainwin_clear_song_info();
    ab_position_a = ab_position_b = -1;
    ip_data.stop = FALSE;
}

void
mainwin_shuffle_pushed(gboolean toggled)
{
    check_set( toggleaction_group_others , "playback shuffle" , toggled );
}

void mainwin_shuffle_pushed_cb(void) {
    mainwin_shuffle_pushed(UI_SKINNED_BUTTON(mainwin_shuffle)->inside);
}

void
mainwin_repeat_pushed(gboolean toggled)
{
    check_set( toggleaction_group_others , "playback repeat" , toggled );
}

void mainwin_repeat_pushed_cb(void) {
    mainwin_repeat_pushed(UI_SKINNED_BUTTON(mainwin_repeat)->inside);
}

void mainwin_equalizer_pushed_cb(void) {
    mainwin_eq_pushed(UI_SKINNED_BUTTON(mainwin_eq)->inside);
}

void mainwin_playlist_pushed_cb(void) {
    mainwin_pl_pushed(UI_SKINNED_BUTTON(mainwin_pl)->inside);
}

void
mainwin_eq_pushed(gboolean toggled)
{
        equalizerwin_show(toggled);
}

void
mainwin_pl_pushed(gboolean toggled)
{
    if (toggled)
        playlistwin_show();
    else
        playlistwin_hide();
}

gint
mainwin_spos_frame_cb(gint pos)
{
    if (mainwin_sposition) {
        gint x = 0;
        if (pos < 6)
            x = 17;
        else if (pos < 9)
            x = 20;
        else
            x = 23;

        UI_SKINNED_HORIZONTAL_SLIDER(mainwin_sposition)->knob_nx = x;
        UI_SKINNED_HORIZONTAL_SLIDER(mainwin_sposition)->knob_px = x;
    }
    return 1;
}

void
mainwin_spos_motion_cb(GtkWidget *widget, gint pos)
{
    gint time;
    gchar *time_msg;
    Playlist *playlist = playlist_get_active();

    pos--;

    time = ((playlist_get_current_length(playlist) / 1000) * pos) / 12;

    if (cfg.timer_mode == TIMER_REMAINING) {
        time = (playlist_get_current_length(playlist) / 1000) - time;
        time_msg = g_strdup_printf("-%2.2d", time / 60);
        ui_skinned_textbox_set_text(mainwin_stime_min, time_msg);
        g_free(time_msg);
    }
    else {
        time_msg = g_strdup_printf(" %2.2d", time / 60);
        ui_skinned_textbox_set_text(mainwin_stime_min, time_msg);
        g_free(time_msg);
    }

    time_msg = g_strdup_printf("%2.2d", time % 60);
    ui_skinned_textbox_set_text(mainwin_stime_sec, time_msg);
    g_free(time_msg);
}

void
mainwin_spos_release_cb(GtkWidget *widget, gint pos)
{
    playback_seek(((playlist_get_current_length(playlist_get_active()) / 1000) *
                       (pos - 1)) / 12);
}

void
mainwin_position_motion_cb(GtkWidget *widget, gint pos)
{
    gint length, time;
    gchar *seek_msg;

    length = playlist_get_current_length(playlist_get_active()) / 1000;
    time = (length * pos) / 219;
    seek_msg = g_strdup_printf(_("Seek to: %d:%-2.2d/%d:%-2.2d (%d%%)"),
                               time / 60, time % 60,
                               length / 60, length % 60,
                               (length != 0) ? (time * 100) / length : 0);
    mainwin_lock_info_text(seek_msg);
    g_free(seek_msg);
}

void
mainwin_position_release_cb(GtkWidget *widget, gint pos)
{
    gint length, time;

    length = playlist_get_current_length(playlist_get_active()) / 1000;
    time = (length * pos) / 219;
    playback_seek(time);
    mainwin_release_info_text();
}

gint
mainwin_volume_frame_cb(gint pos)
{
    return (gint) rint((pos / 52.0) * 28);
}

void
mainwin_adjust_volume_motion(gint v)
{
    gchar *volume_msg;

    setting_volume = TRUE;

    volume_msg = g_strdup_printf(_("Volume: %d%%"), v);
    mainwin_lock_info_text(volume_msg);
    g_free(volume_msg);

    if (balance < 0)
        input_set_volume(v, (v * (100 - abs(balance))) / 100);
    else if (balance > 0)
        input_set_volume((v * (100 - abs(balance))) / 100, v);
    else
        input_set_volume(v, v);
}

void
mainwin_adjust_volume_release(void)
{
    mainwin_release_info_text();
    setting_volume = FALSE;
    read_volume(VOLUME_ADJUSTED);
}

void
mainwin_adjust_balance_motion(gint b)
{
    gchar *balance_msg;
    gint v, pvl, pvr;

    setting_volume = TRUE;
    balance = b;
    input_get_volume(&pvl, &pvr);
    v = MAX(pvl, pvr);
    if (b < 0) {
        balance_msg = g_strdup_printf(_("Balance: %d%% left"), -b);
        input_set_volume(v, (gint) rint(((100 + b) / 100.0) * v));
    }
    else if (b == 0) {
        balance_msg = g_strdup_printf(_("Balance: center"));
        input_set_volume(v, v);
    }
    else {                      /* b > 0 */
        balance_msg = g_strdup_printf(_("Balance: %d%% right"), b);
        input_set_volume((gint) rint(((100 - b) / 100.0) * v), v);
    }
    mainwin_lock_info_text(balance_msg);
    g_free(balance_msg);
}

void
mainwin_adjust_balance_release(void)
{
    mainwin_release_info_text();
    setting_volume = FALSE;
    read_volume(VOLUME_ADJUSTED);
}

void
mainwin_set_volume_slider(gint percent)
{
    ui_skinned_horizontal_slider_set_position(mainwin_volume, (gint) rint((percent * 51) / 100.0));
}

void
mainwin_set_balance_slider(gint percent)
{
    ui_skinned_horizontal_slider_set_position(mainwin_balance, (gint) rint(((percent * 12) / 100.0) + 12));
}

void
mainwin_volume_motion_cb(GtkWidget *widget, gint pos)
{
    gint vol = (pos * 100) / 51;
    mainwin_adjust_volume_motion(vol);
    equalizerwin_set_volume_slider(vol);
}

void
mainwin_volume_release_cb(GtkWidget *widget, gint pos)
{
    mainwin_adjust_volume_release();
}

gint
mainwin_balance_frame_cb(gint pos)
{
    return ((abs(pos - 12) * 28) / 13);
}

void
mainwin_balance_motion_cb(GtkWidget *widget, gint pos)
{
    gint bal = ((pos - 12) * 100) / 12;
    mainwin_adjust_balance_motion(bal);
    equalizerwin_set_balance_slider(bal);
}

void
mainwin_balance_release_cb(GtkWidget *widget, gint pos)
{
    mainwin_adjust_volume_release();
}

void
mainwin_set_volume_diff(gint diff)
{
    gint vl, vr, vol;

    input_get_volume(&vl, &vr);
    vol = MAX(vl, vr);
    vol = CLAMP(vol + diff, 0, 100);

    mainwin_adjust_volume_motion(vol);
    setting_volume = FALSE;
    mainwin_set_volume_slider(vol);
    equalizerwin_set_volume_slider(vol);
    read_volume(VOLUME_SET);
}

void
mainwin_set_balance_diff(gint diff)
{
    gint b;
    b = CLAMP(balance + diff, -100, 100);
    mainwin_adjust_balance_motion(b);
    setting_volume = FALSE;
    mainwin_set_balance_slider(b);
    equalizerwin_set_balance_slider(b);
    read_volume(VOLUME_SET);
}

void
mainwin_show(gboolean show)
{
    if (show)
        mainwin_real_show();
    else
        mainwin_real_hide();
}

void
mainwin_real_show(void)
{
    cfg.player_visible = TRUE;

    check_set( toggleaction_group_others , "show player" , TRUE );

    if (cfg.player_shaded)
        ui_vis_clear_data(mainwin_vis);

    mainwin_set_shape_mask();

    if (cfg.show_wm_decorations) {
        if (cfg.player_x != -1 && cfg.save_window_position)
            gtk_window_move(GTK_WINDOW(mainwin), cfg.player_x, cfg.player_y);

        gtk_widget_show(mainwin);
        return;
    }

    if (nullmask)
    {
      g_object_unref(nullmask);
      nullmask = NULL;
    }

    gtk_window_resize(GTK_WINDOW(mainwin),
                         !bmp_active_skin->properties.mainwin_width ? PLAYER_WIDTH :
                bmp_active_skin->properties.mainwin_width,
                         !bmp_active_skin->properties.mainwin_height ? PLAYER_HEIGHT :
                bmp_active_skin->properties.mainwin_height);

    if (cfg.player_x != -1 && cfg.save_window_position)
        gtk_window_move(GTK_WINDOW(mainwin), cfg.player_x, cfg.player_y);

    gtk_window_present(GTK_WINDOW(mainwin));
    mainwin_refresh_hints();
}

void
mainwin_real_hide(void)
{
    GdkGC *gc;
    GdkColor pattern;

    check_set( toggleaction_group_others , "show player", FALSE);

    if (cfg.player_shaded)
        ui_svis_clear_data(mainwin_svis);

    if (!cfg.show_wm_decorations) {
        nullmask = gdk_pixmap_new(mainwin->window, 20, 20, 1);
        gc = gdk_gc_new(nullmask);
        pattern.pixel = 0;
        gdk_gc_set_foreground(gc, &pattern);
        gdk_draw_rectangle(nullmask, gc, TRUE, 0, 0, 20, 20);
        g_object_unref(gc);
        gtk_widget_shape_combine_mask(mainwin, nullmask, 0, 0);
    }

    gtk_widget_hide(mainwin);

    cfg.player_visible = FALSE;
}


void
mainwin_set_stopaftersong(gboolean stop)
{
    cfg.stopaftersong = stop;
    check_set(toggleaction_group_others, "stop after current song", cfg.stopaftersong);
}

void
mainwin_set_noplaylistadvance(gboolean no_advance)
{
    cfg.no_playlist_advance = no_advance;
    check_set(toggleaction_group_others, "playback no playlist advance", cfg.no_playlist_advance);
}

static void
mainwin_set_doublesize(gboolean doublesize)
{
    gint height;

    if (cfg.player_shaded)
        height = MAINWIN_SHADED_HEIGHT;
    else
        height = bmp_active_skin->properties.mainwin_height;

    mainwin_set_shape_mask();

    dock_window_resize(GTK_WINDOW(mainwin), cfg.player_shaded ? MAINWIN_SHADED_WIDTH : bmp_active_skin->properties.mainwin_width,
        cfg.player_shaded ? MAINWIN_SHADED_HEIGHT : bmp_active_skin->properties.mainwin_height,
        bmp_active_skin->properties.mainwin_width * 2, bmp_active_skin->properties.mainwin_height * 2);

    GList *iter;
    for (iter = GTK_FIXED (SKINNED_WINDOW(mainwin)->fixed)->children; iter; iter = g_list_next (iter)) {
        GtkFixedChild *child_data = (GtkFixedChild *) iter->data;
        GtkWidget *child = child_data->widget;
        g_signal_emit_by_name(child, "toggle-double-size");
    }

    mainwin_refresh_hints();
}

void
set_doublesize(gboolean doublesize)
{
    cfg.doublesize = doublesize;

    mainwin_set_doublesize(doublesize);

    if (cfg.eq_doublesize_linked)
        equalizerwin_set_doublesize(doublesize);
}



void
mainwin_general_menu_callback(gpointer data,
                              guint action,
                              GtkWidget * item)
{
    Playlist *playlist = playlist_get_active();

    switch (action) {
    case MAINWIN_GENERAL_PREFS:
        show_prefs_window();
        break;
    case MAINWIN_GENERAL_ABOUT:
        show_about_window();
        break;
    case MAINWIN_GENERAL_PLAYFILE:
        run_filebrowser(NO_PLAY_BUTTON);
        break;
    case MAINWIN_GENERAL_PLAYLOCATION:
        mainwin_show_add_url_window();
        break;
    case MAINWIN_GENERAL_LASTFM:
        action_lastfm();
        break;
    case MAINWIN_GENERAL_FILEINFO:
        playlist_fileinfo_current(playlist);
        break;
    case MAINWIN_GENERAL_FOCUSPLWIN:
        gtk_window_present(GTK_WINDOW(playlistwin));
        break;
    case MAINWIN_GENERAL_SHOWMWIN:
        mainwin_show(GTK_CHECK_MENU_ITEM(item)->active);
        break;
    case MAINWIN_GENERAL_SHOWPLWIN:
        if (GTK_CHECK_MENU_ITEM(item)->active)
            playlistwin_show();
        else
            playlistwin_hide();
        break;
    case MAINWIN_GENERAL_SHOWEQWIN:
        if (GTK_CHECK_MENU_ITEM(item)->active)
            equalizerwin_real_show();
        else
            equalizerwin_real_hide();
        break;
    case MAINWIN_GENERAL_PREV:
        playlist_prev(playlist);
        break;
    case MAINWIN_GENERAL_PLAY:
        mainwin_play_pushed();
        break;
    case MAINWIN_GENERAL_PAUSE:
        playback_pause();
        break;
    case MAINWIN_GENERAL_STOP:
        mainwin_stop_pushed();
        break;
    case MAINWIN_GENERAL_NEXT:
        playlist_next(playlist);
        break;
    case MAINWIN_GENERAL_BACK5SEC:
        if (playback_get_playing()
            && playlist_get_current_length(playlist) != -1)
            playback_seek_relative(-5);
        break;
    case MAINWIN_GENERAL_FWD5SEC:
        if (playback_get_playing()
            && playlist_get_current_length(playlist) != -1)
            playback_seek_relative(5);
        break;
    case MAINWIN_GENERAL_START:
        playlist_set_position(playlist, 0);
        break;
    case MAINWIN_GENERAL_JTT:
        mainwin_jump_to_time();
        break;
    case MAINWIN_GENERAL_JTF:
        ui_jump_to_track();
        break;
    case MAINWIN_GENERAL_EXIT:
        mainwin_quit_cb();
        break;
    case MAINWIN_GENERAL_SETAB:
        if (playlist_get_current_length(playlist) != -1) {
            if (ab_position_a == -1) {
                ab_position_a = playback_get_time();
                ab_position_b = -1;
        mainwin_lock_info_text("'Loop-Point A Position' set.");
            } else if (ab_position_b == -1) {
                int time = playback_get_time();
                if (time > ab_position_a)
                    ab_position_b = time;
        mainwin_release_info_text();
            } else {
                ab_position_a = playback_get_time();
                ab_position_b = -1;
        mainwin_lock_info_text("'Loop-Point A Position' reset.");
            }
        }
        break;
    case MAINWIN_GENERAL_CLEARAB:
        if (playlist_get_current_length(playlist) != -1) {
            ab_position_a = ab_position_b = -1;
        mainwin_release_info_text();
        }
        break;
    case MAINWIN_GENERAL_NEW_PL:
        {
            Playlist *new_pl = playlist_new();
            playlist_add_playlist(new_pl);
            playlist_select_playlist(new_pl);
        }
        break;
    case MAINWIN_GENERAL_PREV_PL:
        playlist_select_prev();
        break;
    case MAINWIN_GENERAL_NEXT_PL:
        playlist_select_next();
        break;
    }
}

static void
mainwin_mr_change(GtkWidget *widget, MenuRowItem i)
{
    switch (i) {
    case MENUROW_OPTIONS:
        mainwin_lock_info_text(_("Options Menu"));
        break;
    case MENUROW_ALWAYS:
        if (UI_SKINNED_MENUROW(mainwin_menurow)->always_selected)
            mainwin_lock_info_text(_("Disable 'Always On Top'"));
        else
            mainwin_lock_info_text(_("Enable 'Always On Top'"));
        break;
    case MENUROW_FILEINFOBOX:
        mainwin_lock_info_text(_("File Info Box"));
        break;
    case MENUROW_DOUBLESIZE:
        if (UI_SKINNED_MENUROW(mainwin_menurow)->doublesize_selected)
            mainwin_lock_info_text(_("Disable 'Doublesize'"));
        else
            mainwin_lock_info_text(_("Enable 'Doublesize'"));
        break;
    case MENUROW_VISUALIZATION:
        mainwin_lock_info_text(_("Visualization Menu"));
        break;
    case MENUROW_NONE:
        break;
    }
}

static void
mainwin_mr_release(GtkWidget *widget, MenuRowItem i)
{
    GdkModifierType modmask;
    gint x, y;

    switch (i) {
    case MENUROW_OPTIONS:
        gdk_window_get_pointer(NULL, &x, &y, &modmask);
        ui_manager_popup_menu_show(GTK_MENU(mainwin_view_menu), x, y, 1,
                                GDK_CURRENT_TIME);
        break;
    case MENUROW_ALWAYS:
        gtk_toggle_action_set_active(
          GTK_TOGGLE_ACTION(gtk_action_group_get_action(
          toggleaction_group_others , "view always on top" )) ,
          UI_SKINNED_MENUROW(mainwin_menurow)->always_selected );
        break;
    case MENUROW_FILEINFOBOX:
        playlist_fileinfo_current(playlist_get_active());
        break;
    case MENUROW_DOUBLESIZE:
        gtk_toggle_action_set_active(
          GTK_TOGGLE_ACTION(gtk_action_group_get_action(
          toggleaction_group_others , "view doublesize" )) ,
          UI_SKINNED_MENUROW(mainwin_menurow)->doublesize_selected );
        break;
    case MENUROW_VISUALIZATION:
        gdk_window_get_pointer(NULL, &x, &y, &modmask);
        ui_manager_popup_menu_show(GTK_MENU(mainwin_visualization_menu), x, y, 1, GDK_CURRENT_TIME);
        break;
    case MENUROW_NONE:
        break;
    }
    mainwin_release_info_text();
}

void
run_no_output_device_dialog(gpointer hook_data, gpointer user_data)
{
    const gchar *markup =
        N_("<b><big>Couldn't open audio.</big></b>\n\n"
           "Please check that:\n"
           "1. You have the correct output plugin selected.\n"
           "2. No other programs is blocking the soundcard.\n"
           "3. Your soundcard is configured properly.\n");

    GDK_THREADS_ENTER();
    GtkWidget *dialog =
        gtk_message_dialog_new_with_markup(GTK_WINDOW(mainwin),
                                           GTK_DIALOG_DESTROY_WITH_PARENT,
                                           GTK_MESSAGE_ERROR,
                                           GTK_BUTTONS_OK,
                                           _(markup));
    gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);
    GDK_THREADS_LEAVE();
}

void
read_volume(gint when)
{
    static gint pvl = 0, pvr = 0;
    static gint times = VOLSET_DISP_TIMES;
    static gboolean changing = FALSE;

    gint vl, vr, b, v;

    input_get_volume(&vl, &vr);

    switch (when) {
    case VOLSET_STARTUP:
        vl = CLAMP(vl, 0, 100);
        vr = CLAMP(vr, 0, 100);
        pvl = vl;
        pvr = vr;
        v = MAX(vl, vr);
        if (vl > vr)
            b = (gint) rint(((gdouble) vr / vl) * 100) - 100;
        else if (vl < vr)
            b = 100 - (gint) rint(((gdouble) vl / vr) * 100);
        else
            b = 0;

        balance = b;
        mainwin_set_volume_slider(v);
        equalizerwin_set_volume_slider(v);
        mainwin_set_balance_slider(b);
        equalizerwin_set_balance_slider(b);
        return;

    case VOLSET_UPDATE:
        if (vl == -1 || vr == -1)
            return;

        if (setting_volume) {
            pvl = vl;
            pvr = vr;
            return;
        }

        if (pvr == vr && pvl == vl && changing) {
            if (times < VOLSET_DISP_TIMES)
                times++;
            else {
                mainwin_release_info_text();
                changing = FALSE;
            }
        }
        else if (pvr != vr || pvl != vl) {
            gchar *tmp;

            v = MAX(vl, vr);
            if (vl > vr)
                b = (gint) rint(((gdouble) vr / vl) * 100) - 100;
            else if (vl < vr)
                b = 100 - (gint) rint(((gdouble) vl / vr) * 100);
            else
                b = 0;

            if (MAX(vl, vr) != MAX(pvl, pvr))
                tmp = g_strdup_printf(_("VOLUME: %d%%"), v);
            else {
                if (vl > vr) {
                    tmp = g_strdup_printf(_("BALANCE: %d%% LEFT"), -b);
                }
                else if (vr == vl)
                    tmp = g_strdup_printf(_("BALANCE: CENTER"));
                else {          /* (vl < vr) */
                    tmp = g_strdup_printf(_("BALANCE: %d%% RIGHT"), b);
                }
            }
            mainwin_lock_info_text(tmp);
            g_free(tmp);

            pvr = vr;
            pvl = vl;
            times = 0;
            changing = TRUE;
            mainwin_set_volume_slider(v);
            equalizerwin_set_volume_slider(v);

            /* Don't change the balance slider if the volume has been
             * set to zero.  The balance can be anything, and our best
             * guess is what is was before. */
            if (v > 0) {
                balance = b;
                mainwin_set_balance_slider(b);
                equalizerwin_set_balance_slider(b);
            }
        }
        break;

    case VOLUME_ADJUSTED:
        pvl = vl;
        pvr = vr;
        break;

    case VOLUME_SET:
        times = 0;
        changing = TRUE;
        pvl = vl;
        pvr = vr;
        break;
    }
}


/* TODO: HAL! */
gboolean
can_play_cd(void)
{
    GList *ilist;

    for (ilist = get_input_list(); ilist; ilist = g_list_next(ilist)) {
        InputPlugin *ip = INPUT_PLUGIN(ilist->data);

        if (!g_ascii_strcasecmp(g_basename(ip->filename),
                                PLUGIN_FILENAME("cdaudio"))) {
            return TRUE;
        }
    }

    return FALSE;
}


static void
set_timer_mode(TimerMode mode)
{
    if (mode == TIMER_ELAPSED)
        check_set(radioaction_group_viewtime, "view time elapsed", TRUE);
    else
        check_set(radioaction_group_viewtime, "view time remaining", TRUE);
}

static void
set_timer_mode_menu_cb(TimerMode mode)
{
    cfg.timer_mode = mode;
}

gboolean
change_timer_mode_cb(GtkWidget *widget, GdkEventButton *event)
{
    if (event->button == 1) {
        change_timer_mode();
    } else if (event->button == 3)
        return FALSE;

    return TRUE;
}

void change_timer_mode(void) {
    if (cfg.timer_mode == TIMER_ELAPSED)
        set_timer_mode(TIMER_REMAINING);
    else
        set_timer_mode(TIMER_ELAPSED);
    if (playback_get_playing())
        mainwin_update_song_info();
}

static void
mainwin_playlist_prev(void)
{
    playlist_prev(playlist_get_active());
}

static void
mainwin_playlist_next(void)
{
    playlist_next(playlist_get_active());
}

void
mainwin_setup_menus(void)
{
    set_timer_mode(cfg.timer_mode);

    /* View menu */

    check_set(toggleaction_group_others, "view always on top", cfg.always_on_top);
    check_set(toggleaction_group_others, "view put on all workspaces", cfg.sticky);
    check_set(toggleaction_group_others, "roll up player", cfg.player_shaded);
    check_set(toggleaction_group_others, "roll up playlist editor", cfg.playlist_shaded);
    check_set(toggleaction_group_others, "roll up equalizer", cfg.equalizer_shaded);
    check_set(toggleaction_group_others, "view easy move", cfg.easy_move);
    check_set(toggleaction_group_others, "view doublesize", cfg.doublesize);

    /* Songname menu */

    check_set(toggleaction_group_others, "autoscroll songname", cfg.autoscroll);
    check_set(toggleaction_group_others, "stop after current song", cfg.stopaftersong);

    /* Playback menu */

    check_set(toggleaction_group_others, "playback repeat", cfg.repeat);
    check_set(toggleaction_group_others, "playback shuffle", cfg.shuffle);
    check_set(toggleaction_group_others, "playback no playlist advance", cfg.no_playlist_advance);

    /* Visualization menu */

    switch ( cfg.vis_type )
    {
      case VIS_ANALYZER:
        check_set(radioaction_group_vismode, "vismode analyzer", TRUE);
        break;
      case VIS_SCOPE:
        check_set(radioaction_group_vismode, "vismode scope", TRUE);
        break;
      case VIS_VOICEPRINT:
        check_set(radioaction_group_vismode, "vismode voiceprint", TRUE);
        break;
      case VIS_OFF:
      default:
        check_set(radioaction_group_vismode, "vismode off", TRUE);
        break;
    }

    switch ( cfg.analyzer_mode )
    {
      case ANALYZER_FIRE:
        check_set(radioaction_group_anamode, "anamode fire", TRUE);
        break;
      case ANALYZER_VLINES:
        check_set(radioaction_group_anamode, "anamode vertical lines", TRUE);
        break;
      case ANALYZER_NORMAL:
      default:
        check_set(radioaction_group_anamode, "anamode normal", TRUE);
        break;
    }

    switch ( cfg.analyzer_type )
    {
      case ANALYZER_BARS:
        check_set(radioaction_group_anatype, "anatype bars", TRUE);
        break;
      case ANALYZER_LINES:
      default:
        check_set(radioaction_group_anatype, "anatype lines", TRUE);
        break;
    }

    check_set(toggleaction_group_others, "anamode peaks", cfg.analyzer_peaks );

    switch ( cfg.scope_mode )
    {
      case SCOPE_LINE:
        check_set(radioaction_group_scomode, "scomode line", TRUE);
        break;
      case SCOPE_SOLID:
        check_set(radioaction_group_scomode, "scomode solid", TRUE);
        break;
      case SCOPE_DOT:
      default:
        check_set(radioaction_group_scomode, "scomode dot", TRUE);
        break;
    }

    switch ( cfg.voiceprint_mode )
    {
      case VOICEPRINT_FIRE:
        check_set(radioaction_group_vprmode, "vprmode fire", TRUE);
        break;
      case VOICEPRINT_ICE:
        check_set(radioaction_group_vprmode, "vprmode ice", TRUE);
        break;
      case VOICEPRINT_NORMAL:
      default:
        check_set(radioaction_group_vprmode, "vprmode normal", TRUE);
        break;
    }

    switch ( cfg.vu_mode )
    {
      case VU_SMOOTH:
        check_set(radioaction_group_wshmode, "wshmode smooth", TRUE);
        break;
      case VU_NORMAL:
      default:
        check_set(radioaction_group_wshmode, "wshmode normal", TRUE);
        break;
    }

    switch ( cfg.vis_refresh )
    {
      case REFRESH_HALF:
        check_set(radioaction_group_refrate, "refrate half", TRUE);
        break;
      case REFRESH_QUARTER:
        check_set(radioaction_group_refrate, "refrate quarter", TRUE);
        break;
      case REFRESH_EIGTH:
        check_set(radioaction_group_refrate, "refrate eighth", TRUE);
        break;
      case REFRESH_FULL:
      default:
        check_set(radioaction_group_refrate, "refrate full", TRUE);
        break;
    }

    switch ( cfg.analyzer_falloff )
    {
      case FALLOFF_SLOW:
        check_set(radioaction_group_anafoff, "anafoff slow", TRUE);
        break;
      case FALLOFF_MEDIUM:
        check_set(radioaction_group_anafoff, "anafoff medium", TRUE);
        break;
      case FALLOFF_FAST:
        check_set(radioaction_group_anafoff, "anafoff fast", TRUE);
        break;
      case FALLOFF_FASTEST:
        check_set(radioaction_group_anafoff, "anafoff fastest", TRUE);
        break;
      case FALLOFF_SLOWEST:
      default:
        check_set(radioaction_group_anafoff, "anafoff slowest", TRUE);
        break;
    }

    switch ( cfg.peaks_falloff )
    {
      case FALLOFF_SLOW:
        check_set(radioaction_group_peafoff, "peafoff slow", TRUE);
        break;
      case FALLOFF_MEDIUM:
        check_set(radioaction_group_peafoff, "peafoff medium", TRUE);
        break;
      case FALLOFF_FAST:
        check_set(radioaction_group_peafoff, "peafoff fast", TRUE);
        break;
      case FALLOFF_FASTEST:
        check_set(radioaction_group_peafoff, "peafoff fastest", TRUE);
        break;
      case FALLOFF_SLOWEST:
      default:
        check_set(radioaction_group_peafoff, "peafoff slowest", TRUE);
        break;
    }

}

static void mainwin_info_double_clicked_cb(void) {
     playlist_fileinfo_current(playlist_get_active());
}

static void mainwin_info_right_clicked_cb(void) {
     gint x, y;
     gdk_window_get_pointer(NULL, &x, &y, NULL);
     ui_manager_popup_menu_show(GTK_MENU(mainwin_songname_menu), x, y, 3, GDK_CURRENT_TIME);
}

static void
mainwin_create_widgets(void)
{
    mainwin_menubtn = ui_skinned_button_new();
    ui_skinned_push_button_setup(mainwin_menubtn, SKINNED_WINDOW(mainwin)->fixed,
                                 6, 3, 9, 9, 0, 0, 0, 9, SKIN_TITLEBAR);
    g_signal_connect(mainwin_menubtn, "clicked", mainwin_menubtn_cb, NULL );

    mainwin_minimize = ui_skinned_button_new();
    ui_skinned_push_button_setup(mainwin_minimize, SKINNED_WINDOW(mainwin)->fixed,
                                 244, 3, 9, 9, 9, 0, 9, 9, SKIN_TITLEBAR);
    g_signal_connect(mainwin_minimize, "clicked", mainwin_minimize_cb, NULL );

    mainwin_shade = ui_skinned_button_new();
    ui_skinned_push_button_setup(mainwin_shade, SKINNED_WINDOW(mainwin)->fixed,
                                 254, 3, 9, 9, 0,
                                 cfg.player_shaded ? 27 : 18, 9, cfg.player_shaded ? 27 : 18, SKIN_TITLEBAR);
    g_signal_connect(mainwin_shade, "clicked", mainwin_shade_toggle, NULL );

    mainwin_close = ui_skinned_button_new();
    ui_skinned_push_button_setup(mainwin_close, SKINNED_WINDOW(mainwin)->fixed,
                                 264, 3, 9, 9, 18, 0, 18, 9, SKIN_TITLEBAR);
    g_signal_connect(mainwin_close, "clicked", mainwin_quit_cb, NULL );

    mainwin_rew = ui_skinned_button_new();
    ui_skinned_push_button_setup(mainwin_rew, SKINNED_WINDOW(mainwin)->fixed,
                                 16, 88, 23, 18, 0, 0, 0, 18, SKIN_CBUTTONS);
    g_signal_connect(mainwin_rew, "pressed", mainwin_rev_pushed, NULL);
    g_signal_connect(mainwin_rew, "released", mainwin_rev_release, NULL);

    mainwin_fwd = ui_skinned_button_new();
    ui_skinned_push_button_setup(mainwin_fwd, SKINNED_WINDOW(mainwin)->fixed,
                                 108, 88, 22, 18, 92, 0, 92, 18, SKIN_CBUTTONS);
    g_signal_connect(mainwin_fwd, "pressed", mainwin_fwd_pushed, NULL);
    g_signal_connect(mainwin_fwd, "released", mainwin_fwd_release, NULL);

    mainwin_play = ui_skinned_button_new();
    ui_skinned_push_button_setup(mainwin_play, SKINNED_WINDOW(mainwin)->fixed,
                                 39, 88, 23, 18, 23, 0, 23, 18, SKIN_CBUTTONS);
    g_signal_connect(mainwin_play, "clicked", mainwin_play_pushed, NULL );

    mainwin_pause = ui_skinned_button_new();
    ui_skinned_push_button_setup(mainwin_pause, SKINNED_WINDOW(mainwin)->fixed,
                                 62, 88, 23, 18, 46, 0, 46, 18, SKIN_CBUTTONS);
    g_signal_connect(mainwin_pause, "clicked", playback_pause, NULL );

    mainwin_stop = ui_skinned_button_new();
    ui_skinned_push_button_setup(mainwin_stop, SKINNED_WINDOW(mainwin)->fixed,
                                 85, 88, 23, 18, 69, 0, 69, 18, SKIN_CBUTTONS);
    g_signal_connect(mainwin_stop, "clicked", mainwin_stop_pushed, NULL );

    mainwin_eject = ui_skinned_button_new();
    ui_skinned_push_button_setup(mainwin_eject, SKINNED_WINDOW(mainwin)->fixed,
                                 136, 89, 22, 16, 114, 0, 114, 16, SKIN_CBUTTONS);
    g_signal_connect(mainwin_eject, "clicked", mainwin_eject_pushed, NULL);

    mainwin_srew = ui_skinned_button_new();
    ui_skinned_small_button_setup(mainwin_srew, SKINNED_WINDOW(mainwin)->fixed, 169, 4, 8, 7);
    g_signal_connect(mainwin_srew, "clicked", mainwin_playlist_prev, NULL);

    mainwin_splay = ui_skinned_button_new();
    ui_skinned_small_button_setup(mainwin_splay, SKINNED_WINDOW(mainwin)->fixed, 177, 4, 10, 7);
    g_signal_connect(mainwin_splay, "clicked", mainwin_play_pushed, NULL);

    mainwin_spause = ui_skinned_button_new();
    ui_skinned_small_button_setup(mainwin_spause, SKINNED_WINDOW(mainwin)->fixed, 187, 4, 10, 7);
    g_signal_connect(mainwin_spause, "clicked", playback_pause, NULL);

    mainwin_sstop = ui_skinned_button_new();
    ui_skinned_small_button_setup(mainwin_sstop, SKINNED_WINDOW(mainwin)->fixed, 197, 4, 9, 7);
    g_signal_connect(mainwin_sstop, "clicked", mainwin_stop_pushed, NULL);

    mainwin_sfwd = ui_skinned_button_new();
    ui_skinned_small_button_setup(mainwin_sfwd, SKINNED_WINDOW(mainwin)->fixed, 206, 4, 8, 7);
    g_signal_connect(mainwin_sfwd, "clicked", mainwin_playlist_next, NULL);

    mainwin_seject = ui_skinned_button_new();
    ui_skinned_small_button_setup(mainwin_seject, SKINNED_WINDOW(mainwin)->fixed, 216, 4, 9, 7);
    g_signal_connect(mainwin_seject, "clicked", mainwin_eject_pushed, NULL);

    mainwin_shuffle = ui_skinned_button_new();
    ui_skinned_toggle_button_setup(mainwin_shuffle, SKINNED_WINDOW(mainwin)->fixed,
                                   164, 89, 46, 15, 28, 0, 28, 15, 28, 30, 28, 45, SKIN_SHUFREP);
    g_signal_connect(mainwin_shuffle, "clicked", mainwin_shuffle_pushed_cb, NULL);

    mainwin_repeat = ui_skinned_button_new();
    ui_skinned_toggle_button_setup(mainwin_repeat, SKINNED_WINDOW(mainwin)->fixed,
                                   210, 89, 28, 15, 0, 0, 0, 15, 0, 30, 0, 45, SKIN_SHUFREP);
    g_signal_connect(mainwin_repeat, "clicked", mainwin_repeat_pushed_cb, NULL);

    mainwin_eq = ui_skinned_button_new();
    ui_skinned_toggle_button_setup(mainwin_eq, SKINNED_WINDOW(mainwin)->fixed,
                                   219, 58, 23, 12, 0, 61, 46, 61, 0, 73, 46, 73, SKIN_SHUFREP);
    g_signal_connect(mainwin_eq, "clicked", mainwin_equalizer_pushed_cb, NULL);
    UI_SKINNED_BUTTON(mainwin_eq)->inside = cfg.equalizer_visible;

    mainwin_pl = ui_skinned_button_new();
    ui_skinned_toggle_button_setup(mainwin_pl, SKINNED_WINDOW(mainwin)->fixed,
                                   242, 58, 23, 12, 23, 61, 69, 61, 23, 73, 69, 73, SKIN_SHUFREP);
    g_signal_connect(mainwin_pl, "clicked", mainwin_playlist_pushed_cb, NULL);
    UI_SKINNED_BUTTON(mainwin_pl)->inside = cfg.playlist_visible;

    mainwin_info = ui_skinned_textbox_new(SKINNED_WINDOW(mainwin)->fixed, 112, 27, 153, 1, SKIN_TEXT);
    ui_skinned_textbox_set_scroll(mainwin_info, cfg.autoscroll);
    ui_skinned_textbox_set_xfont(mainwin_info, cfg.mainwin_use_xfont, cfg.mainwin_font);
    g_signal_connect(mainwin_info, "double-clicked", mainwin_info_double_clicked_cb, NULL);
    g_signal_connect(mainwin_info, "right-clicked", mainwin_info_right_clicked_cb, NULL);

    mainwin_othertext = ui_skinned_textbox_new(SKINNED_WINDOW(mainwin)->fixed, 112, 43, 153, 1, SKIN_TEXT);

    mainwin_rate_text = ui_skinned_textbox_new(SKINNED_WINDOW(mainwin)->fixed, 111, 43, 15, 0, SKIN_TEXT);

    mainwin_freq_text = ui_skinned_textbox_new(SKINNED_WINDOW(mainwin)->fixed, 156, 43, 10, 0, SKIN_TEXT);

    mainwin_menurow = ui_skinned_menurow_new(SKINNED_WINDOW(mainwin)->fixed, 10, 22, 304, 0, 304, 44,  SKIN_TITLEBAR);
    g_signal_connect(mainwin_menurow, "change", G_CALLBACK(mainwin_mr_change), NULL);
    g_signal_connect(mainwin_menurow, "release", G_CALLBACK(mainwin_mr_release), NULL);

    mainwin_volume = ui_skinned_horizontal_slider_new(SKINNED_WINDOW(mainwin)->fixed, 107, 57, 68,
                                                      13, 15, 422, 0, 422, 14, 11, 15, 0, 0, 51,
                                                      mainwin_volume_frame_cb, SKIN_VOLUME);
    g_signal_connect(mainwin_volume, "motion", G_CALLBACK(mainwin_volume_motion_cb), NULL);
    g_signal_connect(mainwin_volume, "release", G_CALLBACK(mainwin_volume_release_cb), NULL);

    mainwin_balance = ui_skinned_horizontal_slider_new(SKINNED_WINDOW(mainwin)->fixed, 177, 57, 38,
                                                       13, 15, 422, 0, 422, 14, 11, 15, 9, 0, 24,
                                                       mainwin_balance_frame_cb, SKIN_BALANCE);
    g_signal_connect(mainwin_balance, "motion", G_CALLBACK(mainwin_balance_motion_cb), NULL);
    g_signal_connect(mainwin_balance, "release", G_CALLBACK(mainwin_balance_release_cb), NULL);

    mainwin_monostereo = ui_skinned_monostereo_new(SKINNED_WINDOW(mainwin)->fixed, 212, 41, SKIN_MONOSTEREO);

    mainwin_playstatus = ui_skinned_playstatus_new(SKINNED_WINDOW(mainwin)->fixed, 24, 28);

    mainwin_minus_num = ui_skinned_number_new(SKINNED_WINDOW(mainwin)->fixed, 36, 26, SKIN_NUMBERS);
    g_signal_connect(mainwin_minus_num, "button-press-event", G_CALLBACK(change_timer_mode_cb), NULL);

    mainwin_10min_num = ui_skinned_number_new(SKINNED_WINDOW(mainwin)->fixed, 48, 26, SKIN_NUMBERS);
    g_signal_connect(mainwin_10min_num, "button-press-event", G_CALLBACK(change_timer_mode_cb), NULL);

    mainwin_min_num = ui_skinned_number_new(SKINNED_WINDOW(mainwin)->fixed, 60, 26, SKIN_NUMBERS);
    g_signal_connect(mainwin_min_num, "button-press-event", G_CALLBACK(change_timer_mode_cb), NULL);

    mainwin_10sec_num = ui_skinned_number_new(SKINNED_WINDOW(mainwin)->fixed, 78, 26, SKIN_NUMBERS);
    g_signal_connect(mainwin_10sec_num, "button-press-event", G_CALLBACK(change_timer_mode_cb), NULL);

    mainwin_sec_num = ui_skinned_number_new(SKINNED_WINDOW(mainwin)->fixed, 90, 26, SKIN_NUMBERS);
    g_signal_connect(mainwin_sec_num, "button-press-event", G_CALLBACK(change_timer_mode_cb), NULL);

    mainwin_about = ui_skinned_button_new();
    ui_skinned_small_button_setup(mainwin_about, SKINNED_WINDOW(mainwin)->fixed, 247, 83, 20, 25);
    g_signal_connect(mainwin_about, "clicked", show_about_window, NULL);
    g_signal_connect(mainwin_about, "right-clicked", mainwin_about_cb, NULL );

    mainwin_vis = ui_vis_new(SKINNED_WINDOW(mainwin)->fixed, 24, 43, 76);
    g_signal_connect(mainwin_vis, "button-press-event", G_CALLBACK(mainwin_vis_cb), NULL);
    mainwin_svis = ui_svis_new(SKINNED_WINDOW(mainwin)->fixed, 79, 5);
    g_signal_connect(mainwin_svis, "button-press-event", G_CALLBACK(mainwin_vis_cb), NULL);

    mainwin_position = ui_skinned_horizontal_slider_new(SKINNED_WINDOW(mainwin)->fixed, 16, 72, 248,
                                                        10, 248, 0, 278, 0, 29, 10, 10, 0, 0, 219,
                                                        NULL, SKIN_POSBAR);
    g_signal_connect(mainwin_position, "motion", G_CALLBACK(mainwin_position_motion_cb), NULL);
    g_signal_connect(mainwin_position, "release", G_CALLBACK(mainwin_position_release_cb), NULL);

    mainwin_sposition = ui_skinned_horizontal_slider_new(SKINNED_WINDOW(mainwin)->fixed, 226, 4, 17,
                                                         7, 17, 36, 17, 36, 3, 7, 36, 0, 1, 13,
                                                         mainwin_spos_frame_cb, SKIN_TITLEBAR);
    g_signal_connect(mainwin_sposition, "motion", G_CALLBACK(mainwin_spos_motion_cb), NULL);
    g_signal_connect(mainwin_sposition, "release", G_CALLBACK(mainwin_spos_release_cb), NULL);

    mainwin_stime_min = ui_skinned_textbox_new(SKINNED_WINDOW(mainwin)->fixed, 130, 4, 15, FALSE, SKIN_TEXT);
    g_signal_connect(mainwin_stime_min, "button-press-event", G_CALLBACK(change_timer_mode_cb), NULL);

    mainwin_stime_sec = ui_skinned_textbox_new(SKINNED_WINDOW(mainwin)->fixed, 147, 4, 10, FALSE, SKIN_TEXT);
    g_signal_connect(mainwin_stime_sec, "button-press-event", G_CALLBACK(change_timer_mode_cb), NULL);

    err = gtk_message_dialog_new(GTK_WINDOW(mainwin), GTK_DIALOG_DESTROY_WITH_PARENT|GTK_DIALOG_MODAL,
                                 GTK_MESSAGE_ERROR, GTK_BUTTONS_CLOSE, _("Error in Audacious."));


    gtk_window_set_position(GTK_WINDOW(err), GTK_WIN_POS_CENTER);
    /* Dang well better set an error message or you'll see this */
    gtk_message_dialog_format_secondary_text(GTK_MESSAGE_DIALOG(err),
                                             "Boo! Bad stuff! Booga Booga!");

    hook_associate("playback audio error", (void *) mainwin_stop_pushed, NULL);
    hook_associate("playback audio error", (void *) run_no_output_device_dialog, NULL);
}

static void
mainwin_create_window(void)
{
    gint width, height;

    mainwin = ui_skinned_window_new("player");
    gtk_window_set_title(GTK_WINDOW(mainwin), _("Audacious"));
    gtk_window_set_role(GTK_WINDOW(mainwin), "player");
    gtk_window_set_resizable(GTK_WINDOW(mainwin), FALSE);

    width = cfg.player_shaded ? MAINWIN_SHADED_WIDTH : bmp_active_skin->properties.mainwin_width;
    height = cfg.player_shaded ? MAINWIN_SHADED_HEIGHT : bmp_active_skin->properties.mainwin_height;

    if (cfg.doublesize) {
        width *= 2;
        height *= 2;
    }

    gtk_widget_set_size_request(mainwin, width, height);

    if (cfg.player_x != -1 && cfg.save_window_position)
        gtk_window_move(GTK_WINDOW(mainwin), cfg.player_x, cfg.player_y);

    g_signal_connect(mainwin, "destroy", G_CALLBACK(mainwin_destroy), NULL);
    g_signal_connect(mainwin, "button_press_event",
                     G_CALLBACK(mainwin_mouse_button_press), NULL);
    g_signal_connect(mainwin, "scroll_event",
                     G_CALLBACK(mainwin_scrolled), NULL);
    g_signal_connect(mainwin, "button_release_event",
                     G_CALLBACK(mainwin_mouse_button_release), NULL);

    bmp_drag_dest_set(mainwin);

    g_signal_connect(mainwin, "key_press_event",
                     G_CALLBACK(mainwin_keypress), NULL);

    ui_main_evlistener_init();
}

void
mainwin_create(void)
{
    mainwin_create_window();

    gtk_window_add_accel_group( GTK_WINDOW(mainwin) , ui_manager_get_accel_group() );

    mainwin_create_widgets();
}

void
mainwin_attach_idle_func(void)
{
    mainwin_timeout_id = g_timeout_add(MAINWIN_UPDATE_INTERVAL,
                                       mainwin_idle_func, NULL);
}

gboolean
mainwin_update_song_info(void)
{
    gint time = playback_get_time();
    gint length, t;
    gchar stime_prefix;

    if (!playback_get_playing())
        return FALSE;

    if (ab_position_a != -1 && ab_position_b != -1 && time > ab_position_b)
        playback_seek(ab_position_a/1000);

    length = playlist_get_current_length(playlist_get_active());
    playlistwin_set_time(time, length, cfg.timer_mode);

    if (cfg.timer_mode == TIMER_REMAINING) {
        if (length != -1) {
            ui_skinned_number_set_number(mainwin_minus_num, 11);
            t = length - time;
            stime_prefix = '-';
        }
        else {
            ui_skinned_number_set_number(mainwin_minus_num, 10);
            t = time;
            stime_prefix = ' ';
        }
    }
    else {
        ui_skinned_number_set_number(mainwin_minus_num, 10);
        t = time;
        stime_prefix = ' ';
    }
    t /= 1000;

    /* Show the time in the format HH:MM when we have more than 100
     * minutes. */
    if (t >= 100 * 60)
        t /= 60;
    ui_skinned_number_set_number(mainwin_10min_num, t / 600);
    ui_skinned_number_set_number(mainwin_min_num, (t / 60) % 10);
    ui_skinned_number_set_number(mainwin_10sec_num, (t / 10) % 6);
    ui_skinned_number_set_number(mainwin_sec_num, t % 10);

    if (!UI_SKINNED_HORIZONTAL_SLIDER(mainwin_sposition)->pressed) {
        gchar *time_str;

        time_str = g_strdup_printf("%c%2.2d", stime_prefix, t / 60);
        ui_skinned_textbox_set_text(mainwin_stime_min, time_str);
        g_free(time_str);

        time_str = g_strdup_printf("%2.2d", t % 60);
        ui_skinned_textbox_set_text(mainwin_stime_sec, time_str);
        g_free(time_str);
    }

    time /= 1000;
    length /= 1000;
    if (length > 0) {
        if (time > length) {
            ui_skinned_horizontal_slider_set_position(mainwin_position, 219);
            ui_skinned_horizontal_slider_set_position(mainwin_sposition, 13);
        }
        /* update the slider position ONLY if there is not a seek in progress */
        else if (seek_state == MAINWIN_SEEK_NIL)  {
            ui_skinned_horizontal_slider_set_position(mainwin_position, (time * 219) / length);
            ui_skinned_horizontal_slider_set_position(mainwin_sposition,
                                 ((time * 12) / length) + 1);
        }
    }
    else {
        ui_skinned_horizontal_slider_set_position(mainwin_position, 0);
        ui_skinned_horizontal_slider_set_position(mainwin_sposition, 1);
    }

    return TRUE;
}

static gboolean
mainwin_idle_func(gpointer data)
{
    static gint count = 0;

    if (playback_get_playing())  
        input_update_vis(playback_get_time());    

    GDK_THREADS_ENTER();

    if (!count) {
        read_volume(VOLSET_UPDATE);
        count = 10;
    }
    else
        count--;

    /* tristate buttons seek */
    if ( seek_state != MAINWIN_SEEK_NIL )
    {
      GTimeVal now_time;
      GTimeVal delta_time;
      gulong now_dur;
      g_get_current_time(&now_time);

      delta_time.tv_usec = now_time.tv_usec - cb_time.tv_usec;
      delta_time.tv_sec = now_time.tv_sec - cb_time.tv_sec;

      now_dur = labs((delta_time.tv_sec * 1000) + (glong) (delta_time.tv_usec / 1000));

      if ( now_dur > TRISTATE_THRESHOLD )
      {
        gint np;
        if (seek_state == MAINWIN_SEEK_REV)
          np = seek_initial_pos - labs((gulong)(now_dur/100)); /* seek back */
        else
          np = seek_initial_pos + labs((gulong)(now_dur/100)); /* seek forward */

        /* boundaries check */
        if (np < 0 )
          np = 0;
        else if ( np > 219 )
          np = 219;

        ui_skinned_horizontal_slider_set_position( mainwin_position , np );
        mainwin_position_motion_cb( mainwin_position, np );
      }
    }

    GDK_THREADS_LEAVE();

    /*
    if (seek_state == MAINWIN_SEEK_REV)
        playback_seek(CLAMP(playback_get_time() - 1000, 0,
                                playlist_get_current_length()) / 1000);
    else if (seek_state == MAINWIN_SEEK_FWD)
        playback_seek(CLAMP(playback_get_time() + 1000, 0,
                                playlist_get_current_length()) / 1000);
    */

    return TRUE;
}


void
audacious_menu_main_show( gint x , gint y , guint button , guint time )
{
  /* convenience function that shows the main popup menu wherever requested */
  ui_manager_popup_menu_show( GTK_MENU(mainwin_general_menu),
                              x , y , button , time );
  return;
}


/* toggleactionentries actions */

void
action_anamode_peaks( GtkToggleAction * action )
{
  cfg.analyzer_peaks = gtk_toggle_action_get_active( action );
}

void
action_autoscroll_songname( GtkToggleAction * action )
{
  mainwin_set_title_scroll(gtk_toggle_action_get_active(action));
  playlistwin_set_sinfo_scroll(cfg.autoscroll); /* propagate scroll setting to playlistwin_sinfo */
}

void
action_playback_noplaylistadvance( GtkToggleAction * action )
{
  cfg.no_playlist_advance = gtk_toggle_action_get_active( action );
}

void
action_playback_repeat( GtkToggleAction * action )
{
  cfg.repeat = gtk_toggle_action_get_active( action );
  UI_SKINNED_BUTTON(mainwin_repeat)->inside = cfg.repeat;
  gtk_widget_queue_draw(mainwin_repeat);
}

void
action_playback_shuffle( GtkToggleAction * action )
{
  cfg.shuffle = gtk_toggle_action_get_active( action );
  playlist_set_shuffle(cfg.shuffle);
  UI_SKINNED_BUTTON(mainwin_shuffle)->inside = cfg.shuffle;
  gtk_widget_queue_draw(mainwin_shuffle);
}

void
action_stop_after_current_song( GtkToggleAction * action )
{
  cfg.stopaftersong = gtk_toggle_action_get_active( action );
}

void
action_view_always_on_top( GtkToggleAction * action )
{
  UI_SKINNED_MENUROW(mainwin_menurow)->always_selected = gtk_toggle_action_get_active( action );
  cfg.always_on_top = UI_SKINNED_MENUROW(mainwin_menurow)->always_selected;
  gtk_widget_queue_draw(mainwin_menurow);
  hint_set_always(cfg.always_on_top);
}

void
action_view_doublesize( GtkToggleAction * action )
{
  UI_SKINNED_MENUROW(mainwin_menurow)->doublesize_selected = gtk_toggle_action_get_active( action );
  gtk_widget_queue_draw(mainwin_menurow);
  set_doublesize(UI_SKINNED_MENUROW(mainwin_menurow)->doublesize_selected);
  gdk_flush();
}

void
action_view_easymove( GtkToggleAction * action )
{
  cfg.easy_move = gtk_toggle_action_get_active( action );
}

void
action_view_on_all_workspaces( GtkToggleAction * action )
{
  cfg.sticky = gtk_toggle_action_get_active( action );
  hint_set_sticky(cfg.sticky);
}

void
action_roll_up_equalizer( GtkToggleAction * action )
{
  equalizerwin_set_shade_menu_cb(gtk_toggle_action_get_active(action));
}

void
action_roll_up_player( GtkToggleAction * action )
{
  mainwin_set_shade_menu_cb(gtk_toggle_action_get_active(action));
}

void
action_roll_up_playlist_editor( GtkToggleAction * action )
{
  playlistwin_set_shade(gtk_toggle_action_get_active(action));
}

void
action_show_equalizer( GtkToggleAction * action )
{
  if (gtk_toggle_action_get_active(action))
    equalizerwin_real_show();
  else
    equalizerwin_real_hide();
}

void
action_show_playlist_editor( GtkToggleAction * action )
{
  if (gtk_toggle_action_get_active(action))
    playlistwin_show();
  else
    playlistwin_hide();
}

void
action_show_player( GtkToggleAction * action )
{
  mainwin_show(gtk_toggle_action_get_active(action));
}


/* radioactionentries actions (one callback for each radio group) */

void
action_anafoff( GtkAction *action, GtkRadioAction *current )
{
  mainwin_vis_set_afalloff(gtk_radio_action_get_current_value(current));
}

void
action_anamode( GtkAction *action, GtkRadioAction *current )
{
  mainwin_vis_set_analyzer_mode(gtk_radio_action_get_current_value(current));
}

void
action_anatype( GtkAction *action, GtkRadioAction *current )
{
  mainwin_vis_set_analyzer_type(gtk_radio_action_get_current_value(current));
}

void
action_peafoff( GtkAction *action, GtkRadioAction *current )
{
  mainwin_vis_set_pfalloff(gtk_radio_action_get_current_value(current));
}

void
action_refrate( GtkAction *action, GtkRadioAction *current )
{
  mainwin_vis_set_refresh(gtk_radio_action_get_current_value(current));
}

void
action_scomode( GtkAction *action, GtkRadioAction *current )
{
  cfg.scope_mode = gtk_radio_action_get_current_value(current);
}

void
action_vismode( GtkAction *action, GtkRadioAction *current )
{
  mainwin_vis_set_type_menu_cb(gtk_radio_action_get_current_value(current));
}

void
action_vprmode( GtkAction *action, GtkRadioAction *current )
{
  cfg.voiceprint_mode = gtk_radio_action_get_current_value(current);
}

void
action_wshmode( GtkAction *action, GtkRadioAction *current )
{
  cfg.vu_mode = gtk_radio_action_get_current_value(current);
}

void
action_viewtime( GtkAction *action, GtkRadioAction *current )
{
  set_timer_mode_menu_cb(gtk_radio_action_get_current_value(current));
}


/* actionentries actions */

void
action_about_audacious( void )
{
    show_about_window();
}

void
action_play_file( void )
{
    run_filebrowser(PLAY_BUTTON);
}

void
action_play_location( void )
{
    mainwin_show_add_url_window();
}

void
action_ab_set( void )
{
    Playlist *playlist = playlist_get_active();
    if (playlist_get_current_length(playlist) != -1)
    {
        if (ab_position_a == -1)
        {
            ab_position_a = playback_get_time();
            ab_position_b = -1;
            mainwin_lock_info_text("LOOP-POINT A POSITION SET.");
        }
        else if (ab_position_b == -1)
        {
            int time = playback_get_time();
            if (time > ab_position_a)
                ab_position_b = time;
            mainwin_release_info_text();
        }
        else
        {
            ab_position_a = playback_get_time();
            ab_position_b = -1;
            mainwin_lock_info_text("LOOP-POINT A POSITION RESET.");
        }
    }
}

void
action_ab_clear( void )
{
    Playlist *playlist = playlist_get_active();
    if (playlist_get_current_length(playlist) != -1)
    {
        ab_position_a = ab_position_b = -1;
        mainwin_release_info_text();
    }
}

void
action_current_track_info( void )
{
    playlist_fileinfo_current(playlist_get_active());
}

void
action_jump_to_file( void )
{
    ui_jump_to_track();
}

void
action_lastfm( void )
{
    ui_lastfm();
}

void
action_jump_to_playlist_start( void )
{
    Playlist *playlist = playlist_get_active();
    playlist_set_position(playlist, 0);
}

void
action_jump_to_time( void )
{
    mainwin_jump_to_time();
}

void
action_playback_next( void )
{
    Playlist *playlist = playlist_get_active();
    playlist_next(playlist);
}

void
action_playback_previous( void )
{
    Playlist *playlist = playlist_get_active();
    playlist_prev(playlist);
}

void
action_playback_play( void )
{
    mainwin_play_pushed();
}

void
action_playback_pause( void )
{
    playback_pause();
}

void
action_playback_stop( void )
{
    mainwin_stop_pushed();
}

void
action_preferences( void )
{
    show_prefs_window();
}

void
action_quit( void )
{
    mainwin_quit_cb();
}
