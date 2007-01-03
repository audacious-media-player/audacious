/*  Audacious - Cross-platform multimedia player
 *  Copyright (C) 2005-2006  Audacious development team.
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

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include "ui_manager.h"
#include "actions-mainwin.h"

#include "mainwin.h"
#include "icons-stock.h"
#include "widgets/widgetcore.h"


static GtkUIManager *ui_manager;


/* toggle action entries */

static GtkToggleActionEntry mainwin_toggleaction_entries_others[] = {

	{ "autoscroll songname", NULL , N_("Autoscroll Songname"), NULL,
	  N_("Autoscroll Songname"), G_CALLBACK(action_autoscroll_songname) , FALSE },

	{ "stop after current song", NULL , N_("Stop after Current Song"), "<Ctrl>M",
	  N_("Stop after Current Song"), G_CALLBACK(action_stop_after_current_song) , FALSE },

	{ "anamode peaks", NULL , N_("Peaks"), NULL,
	  N_("Peaks"), G_CALLBACK(action_anamode_peaks) , FALSE },

	{ "playback repeat", NULL , N_("Repeat"), "R",
	  N_("Repeat"), G_CALLBACK(action_playback_repeat) , FALSE },

	{ "playback shuffle", NULL , N_("Shuffle"), "S",
	  N_("Shuffle"), G_CALLBACK(action_playback_shuffle) , FALSE },

	{ "playback no playlist advance", NULL , N_("No Playlist Advance"), "<Ctrl>N",
	  N_("No Playlist Advance"), G_CALLBACK(action_playback_noplaylistadvance) , FALSE },

	{ "show player", NULL , N_("Show Player"), "<Alt>M",
	  N_("Show Player"), G_CALLBACK(action_show_player) , FALSE },

	{ "show playlist editor", NULL , N_("Show Playlist Editor"), "<Alt>E",
	  N_("Show Playlist Editor"), G_CALLBACK(action_show_playlist_editor) , FALSE },

	{ "show equalizer", NULL , N_("Show Equalizer"), "<Alt>G",
	  N_("Show Equalizer"), G_CALLBACK(action_show_equalizer) , FALSE },

	{ "view always on top", NULL , N_("Always on Top"), "<Ctrl>O",
	  N_("Always on Top"), G_CALLBACK(action_view_always_on_top) , FALSE },

	{ "view put on all workspaces", NULL , N_("Put on All Workspaces"), "<Ctrl>S",
	  N_("Put on All Workspaces"), G_CALLBACK(action_view_on_all_workspaces) , FALSE },

	{ "roll up player", NULL , N_("Roll up Player"), "<Ctrl>W",
	  N_("Roll up Player"), G_CALLBACK(action_roll_up_player) , FALSE },

	{ "roll up playlist editor", NULL , N_("Roll up Playlist Editor"), "<Shift><Ctrl>W",
	  N_("Roll up Playlist Editor"), G_CALLBACK(action_roll_up_playlist_editor) , FALSE },

	{ "roll up equalizer", NULL , N_("Roll up Equalizer"), "<Ctrl><Alt>W",
	  N_("Roll up Equalizer"), G_CALLBACK(action_roll_up_equalizer) , FALSE },

	{ "view doublesize", NULL , N_("DoubleSize"), "<Ctrl>D",
	  N_("DoubleSize"), G_CALLBACK(action_view_doublesize) , FALSE },

	{ "view easy move", NULL , N_("Easy Move"), "<Ctrl>E",
	  N_("Easy Move"), G_CALLBACK(action_view_easymove) , FALSE }
};



/* radio action entries */

static GtkRadioActionEntry mainwin_radioaction_entries_vismode[] = {
	{ "vismode analyzer", NULL , N_("Analyzer"), NULL, N_("Analyzer"), VIS_ANALYZER },
	{ "vismode scope", NULL , N_("Scope"), NULL, N_("Scope"), VIS_SCOPE },
	{ "vismode voiceprint", NULL , N_("Voiceprint"), NULL, N_("Voiceprint"), VIS_VOICEPRINT },
	{ "vismode off", NULL , N_("Off"), NULL, N_("Off"), VIS_OFF }
};

static GtkRadioActionEntry mainwin_radioaction_entries_anamode[] = {
	{ "anamode normal", NULL , N_("Normal"), NULL, N_("Normal"), ANALYZER_NORMAL },
	{ "anamode fire", NULL , N_("Fire"), NULL, N_("Fire"), ANALYZER_FIRE },
	{ "anamode vertical lines", NULL , N_("Vertical Lines"), NULL, N_("Vertical Lines"), ANALYZER_VLINES }
};

static GtkRadioActionEntry mainwin_radioaction_entries_anatype[] = {
	{ "anatype lines", NULL , N_("Lines"), NULL, N_("Lines"), ANALYZER_LINES },
	{ "anatype bars", NULL , N_("Bars"), NULL, N_("Bars"), ANALYZER_BARS }
};

static GtkRadioActionEntry mainwin_radioaction_entries_scomode[] = {
	{ "scomode dot", NULL , N_("Dot Scope"), NULL, N_("Dot Scope"), SCOPE_DOT },
	{ "scomode line", NULL , N_("Line Scope"), NULL, N_("Line Scope"), SCOPE_LINE },
	{ "scomode solid", NULL , N_("Solid Scope"), NULL, N_("Solid Scope"), SCOPE_SOLID }
};

static GtkRadioActionEntry mainwin_radioaction_entries_vprmode[] = {
	{ "vprmode normal", NULL , N_("Normal"), NULL, N_("Normal"), VOICEPRINT_NORMAL },
	{ "vprmode fire", NULL , N_("Fire"), NULL, N_("Fire"), VOICEPRINT_FIRE },
	{ "vprmode ice", NULL , N_("Ice"), NULL, N_("Ice"), VOICEPRINT_ICE }
};

static GtkRadioActionEntry mainwin_radioaction_entries_wshmode[] = {
	{ "wshmode normal", NULL , N_("Normal"), NULL, N_("Normal"), VU_NORMAL },
	{ "wshmode smooth", NULL , N_("Smooth"), NULL, N_("Smooth"), VU_SMOOTH }
};

static GtkRadioActionEntry mainwin_radioaction_entries_refrate[] = {
	{ "refrate full", NULL , N_("Full (~50 fps)"), NULL, N_("Full (~50 fps)"), REFRESH_FULL },
	{ "refrate half", NULL , N_("Half (~25 fps)"), NULL, N_("Half (~25 fps)"), REFRESH_HALF },
	{ "refrate quarter", NULL , N_("Quarter (~13 fps)"), NULL, N_("Quarter (~13 fps)"), REFRESH_QUARTER },
	{ "refrate eighth", NULL , N_("Eighth (~6 fps)"), NULL, N_("Eighth (~6 fps)"), REFRESH_EIGTH }
};

static GtkRadioActionEntry mainwin_radioaction_entries_anafoff[] = {
	{ "anafoff slowest", NULL , N_("Slowest"), NULL, N_("Slowest"), FALLOFF_SLOWEST },
	{ "anafoff slow", NULL , N_("Slow"), NULL, N_("Slow"), FALLOFF_SLOW },
	{ "anafoff medium", NULL , N_("Medium"), NULL, N_("Medium"), FALLOFF_MEDIUM },
	{ "anafoff fast", NULL , N_("Fast"), NULL, N_("Fast"), FALLOFF_FAST },
	{ "anafoff fastest", NULL , N_("Fastest"), NULL, N_("Fastest"), FALLOFF_FASTEST }
};

static GtkRadioActionEntry mainwin_radioaction_entries_peafoff[] = {
	{ "peafoff slowest", NULL , N_("Slowest"), NULL, N_("Slowest"), FALLOFF_SLOWEST },
	{ "peafoff slow", NULL , N_("Slow"), NULL, N_("Slow"), FALLOFF_SLOW },
	{ "peafoff medium", NULL , N_("Medium"), NULL, N_("Medium"), FALLOFF_MEDIUM },
	{ "peafoff fast", NULL , N_("Fast"), NULL, N_("Fast"), FALLOFF_FAST },
	{ "peafoff fastest", NULL , N_("Fastest"), NULL, N_("Fastest"), FALLOFF_FASTEST }
};

static GtkRadioActionEntry mainwin_radioaction_entries_viewtime[] = {
	{ "view time elapsed", NULL , N_("Time Elapsed"), "<Ctrl>E", N_("Time Elapsed"), TIMER_ELAPSED },
	{ "view time remaining", NULL , N_("Time Remaining"), "<Ctrl>R", N_("Time Remaining"), TIMER_REMAINING }
};



/* normal actions */

static GtkActionEntry mainwin_action_entries_playback[] = {

	{ "playback", NULL, "Playback" },

	{ "playback play cd", GTK_STOCK_CDROM , N_("Play CD"), "<Alt>C",
	  N_("Play CD"), G_CALLBACK(action_playback_playcd) },

	{ "playback play", GTK_STOCK_MEDIA_PLAY , N_("Play"), "X",
	  N_("Play"), G_CALLBACK(action_playback_play) },

	{ "playback pause", GTK_STOCK_MEDIA_PAUSE , N_("Pause"), "C",
	  N_("Pause"), G_CALLBACK(action_playback_pause) },

	{ "playback stop", GTK_STOCK_MEDIA_STOP , N_("Stop"), "V",
	  N_("Stop"), G_CALLBACK(action_playback_stop) },

	{ "playback previous", GTK_STOCK_MEDIA_PREVIOUS , N_("Previous"), "Z",
	  N_("Previous"), G_CALLBACK(action_playback_previous) },

	{ "playback next", GTK_STOCK_MEDIA_NEXT , N_("Next"), "B",
	  N_("Next"), G_CALLBACK(action_playback_next) }
};


static GtkActionEntry mainwin_action_entries_visualization[] = {

	{ "visualization", NULL, "Visualization" },

	{ "vismode", NULL, "Visualization Mode" },

	{ "anamode", NULL, "Analyzer Mode" },

	{ "scomode", NULL, "Scope Mode" },

	{ "vprmode", NULL, "Voiceprint Mode" },

	{ "wshmode", NULL, "WindowShade VU Mode" },

	{ "refrate", NULL, "Refresh Rate" },

	{ "anafoff", NULL, "Analyzer Falloff" },

	{ "peafoff", NULL, "Peaks Falloff" }
};

static GtkActionEntry mainwin_action_entries_playlist[] = {

	{ "playlist", NULL, "Playlist" },

	{ "playlist new", NULL , N_("New Playlist"), "<Shift>N",
	  N_("New Playlist"), G_CALLBACK(action_playlist_new) },

	{ "playlist select next", NULL , N_("Select Next Playlist"), "<Shift>P",
	  N_("Select Next Playlist"), G_CALLBACK(action_playlist_next) },

	{ "playlist select previous", NULL , N_("Select Previous Playlist"), "<Shift><Ctrl>P",
	  N_("Select Previous Playlist"), G_CALLBACK(action_playlist_prev) }
};

static GtkActionEntry mainwin_action_entries_view[] = {

	{ "view", NULL, "View" }
};

static GtkActionEntry mainwin_action_entries_others[] = {

	{ "dummy", NULL, "dummy" },

	{ "track info", AUD_STOCK_INFO , N_("View Track Details"), NULL,
	  N_("View track details"), G_CALLBACK(action_track_info) },

	{ "about audacious", GTK_STOCK_DIALOG_INFO , N_("About Audacious"), NULL,
	  N_("About Audacious"), G_CALLBACK(action_about_audacious) },

	{ "play file", GTK_STOCK_OPEN , N_("Play File"), "L",
	  N_("Load and play a file"), G_CALLBACK(action_play_file) },

	{ "play location", GTK_STOCK_NETWORK , N_("Play Location"), "<Ctrl>L",
	  N_("Play media from the selected location"), G_CALLBACK(action_play_location) },

	{ "preferences", GTK_STOCK_PREFERENCES , N_("Preferences"), "<Ctrl>P",
	  N_("Open preferences window"), G_CALLBACK(action_preferences) },

	{ "quit", GTK_STOCK_QUIT , N_("_Quit"), NULL,
	  N_("Quit Audacious"), G_CALLBACK(action_quit) },

	{ "ab set", NULL , N_("Set A-B"), "A",
	  N_("Set A-B"), G_CALLBACK(action_ab_set) },

	{ "ab clear", NULL , N_("Clear A-B"), "<Ctrl>S",
	  N_("Clear A-B"), G_CALLBACK(action_ab_clear) },

	{ "jump to playlist start", GTK_STOCK_GOTO_TOP , N_("Jump to Playlist Start"), "<Ctrl>Z",
	  N_("Jump to Playlist Start"), G_CALLBACK(action_jump_to_playlist_start) },

	{ "jump to file", GTK_STOCK_JUMP_TO , N_("Jump to File"), "J",
	  N_("Jump to File"), G_CALLBACK(action_jump_to_file) },

	{ "jump to time", GTK_STOCK_JUMP_TO , N_("Jump to Time"), "<Ctrl>J",
	  N_("Jump to Time"), G_CALLBACK(action_jump_to_time) }
};



/* ***************************** */

void
ui_manager_init ( void )
{
  /* toggle actions */
  mainwin_toggleaction_group_others = gtk_action_group_new("mainwin_toggleaction_others");
  gtk_action_group_add_toggle_actions(
    mainwin_toggleaction_group_others , mainwin_toggleaction_entries_others ,
    G_N_ELEMENTS(mainwin_toggleaction_entries_others) , NULL );

  /* radio actions */
  mainwin_radioaction_group_anamode = gtk_action_group_new("mainwin_radioaction_anamode");
  gtk_action_group_add_radio_actions(
    mainwin_radioaction_group_anamode , mainwin_radioaction_entries_anamode ,
    G_N_ELEMENTS(mainwin_radioaction_entries_anamode) , 0 , G_CALLBACK(action_anamode) , NULL );

  mainwin_radioaction_group_anatype = gtk_action_group_new("mainwin_radioaction_anatype");
  gtk_action_group_add_radio_actions(
    mainwin_radioaction_group_anatype , mainwin_radioaction_entries_anatype ,
    G_N_ELEMENTS(mainwin_radioaction_entries_anatype) , 0 , G_CALLBACK(action_anatype) , NULL );

  mainwin_radioaction_group_scomode = gtk_action_group_new("mainwin_radioaction_scomode");
  gtk_action_group_add_radio_actions(
    mainwin_radioaction_group_scomode , mainwin_radioaction_entries_scomode ,
    G_N_ELEMENTS(mainwin_radioaction_entries_scomode) , 0 , G_CALLBACK(action_scomode) , NULL );

  mainwin_radioaction_group_vprmode = gtk_action_group_new("mainwin_radioaction_vprmode");
  gtk_action_group_add_radio_actions(
    mainwin_radioaction_group_vprmode , mainwin_radioaction_entries_vprmode ,
    G_N_ELEMENTS(mainwin_radioaction_entries_vprmode) , 0 , G_CALLBACK(action_vprmode) , NULL );

  mainwin_radioaction_group_wshmode = gtk_action_group_new("mainwin_radioaction_wshmode");
  gtk_action_group_add_radio_actions(
    mainwin_radioaction_group_wshmode , mainwin_radioaction_entries_wshmode ,
    G_N_ELEMENTS(mainwin_radioaction_entries_wshmode) , 0 , G_CALLBACK(action_wshmode) , NULL );

  mainwin_radioaction_group_refrate = gtk_action_group_new("mainwin_radioaction_refrate");
  gtk_action_group_add_radio_actions(
    mainwin_radioaction_group_refrate , mainwin_radioaction_entries_refrate ,
    G_N_ELEMENTS(mainwin_radioaction_entries_refrate) , 0 , G_CALLBACK(action_refrate) , NULL );

  mainwin_radioaction_group_anafoff = gtk_action_group_new("mainwin_radioaction_anafoff");
  gtk_action_group_add_radio_actions(
    mainwin_radioaction_group_anafoff , mainwin_radioaction_entries_anafoff ,
    G_N_ELEMENTS(mainwin_radioaction_entries_anafoff) , 0 , G_CALLBACK(action_anafoff) , NULL );

  mainwin_radioaction_group_peafoff = gtk_action_group_new("mainwin_radioaction_peafoff");
  gtk_action_group_add_radio_actions(
    mainwin_radioaction_group_peafoff , mainwin_radioaction_entries_peafoff ,
    G_N_ELEMENTS(mainwin_radioaction_entries_peafoff) , 0 , G_CALLBACK(action_peafoff) , NULL );

  mainwin_radioaction_group_vismode = gtk_action_group_new("mainwin_radioaction_vismode");
  gtk_action_group_add_radio_actions(
    mainwin_radioaction_group_vismode , mainwin_radioaction_entries_vismode ,
    G_N_ELEMENTS(mainwin_radioaction_entries_vismode) , 0 , G_CALLBACK(action_vismode) , NULL );

  mainwin_radioaction_group_viewtime = gtk_action_group_new("mainwin_radioaction_viewtime");
  gtk_action_group_add_radio_actions(
    mainwin_radioaction_group_viewtime , mainwin_radioaction_entries_viewtime ,
    G_N_ELEMENTS(mainwin_radioaction_entries_viewtime) , 0 , G_CALLBACK(action_viewtime) , NULL );

  /* normal actions */
  mainwin_action_group_playback = gtk_action_group_new("mainwin_action_playback");
    gtk_action_group_add_actions(
    mainwin_action_group_playback , mainwin_action_entries_playback ,
    G_N_ELEMENTS(mainwin_action_entries_playback) , NULL );

  mainwin_action_group_playlist = gtk_action_group_new("mainwin_action_playlist");
    gtk_action_group_add_actions(
    mainwin_action_group_playlist , mainwin_action_entries_playlist ,
    G_N_ELEMENTS(mainwin_action_entries_playlist) , NULL );

  mainwin_action_group_visualization = gtk_action_group_new("mainwin_action_visualization");
    gtk_action_group_add_actions(
    mainwin_action_group_visualization , mainwin_action_entries_visualization ,
    G_N_ELEMENTS(mainwin_action_entries_visualization) , NULL );

  mainwin_action_group_view = gtk_action_group_new("mainwin_action_view");
    gtk_action_group_add_actions(
    mainwin_action_group_view , mainwin_action_entries_view ,
    G_N_ELEMENTS(mainwin_action_entries_view) , NULL );

  mainwin_action_group_others = gtk_action_group_new("mainwin_action_others");
    gtk_action_group_add_actions(
    mainwin_action_group_others , mainwin_action_entries_others ,
    G_N_ELEMENTS(mainwin_action_entries_others) , NULL );

  /* ui */
  ui_manager = gtk_ui_manager_new();
  gtk_ui_manager_insert_action_group( ui_manager , mainwin_toggleaction_group_others , 0 );
  gtk_ui_manager_insert_action_group( ui_manager , mainwin_radioaction_group_anamode , 0 );
  gtk_ui_manager_insert_action_group( ui_manager , mainwin_radioaction_group_anatype , 0 );
  gtk_ui_manager_insert_action_group( ui_manager , mainwin_radioaction_group_scomode , 0 );
  gtk_ui_manager_insert_action_group( ui_manager , mainwin_radioaction_group_vprmode , 0 );
  gtk_ui_manager_insert_action_group( ui_manager , mainwin_radioaction_group_wshmode , 0 );
  gtk_ui_manager_insert_action_group( ui_manager , mainwin_radioaction_group_refrate , 0 );
  gtk_ui_manager_insert_action_group( ui_manager , mainwin_radioaction_group_anafoff , 0 );
  gtk_ui_manager_insert_action_group( ui_manager , mainwin_radioaction_group_peafoff , 0 );
  gtk_ui_manager_insert_action_group( ui_manager , mainwin_radioaction_group_vismode , 0 );
  gtk_ui_manager_insert_action_group( ui_manager , mainwin_radioaction_group_viewtime , 0 );
  gtk_ui_manager_insert_action_group( ui_manager , mainwin_action_group_playback , 0 );
  gtk_ui_manager_insert_action_group( ui_manager , mainwin_action_group_playlist , 0 );
  gtk_ui_manager_insert_action_group( ui_manager , mainwin_action_group_visualization , 0 );
  gtk_ui_manager_insert_action_group( ui_manager , mainwin_action_group_view , 0 );
  gtk_ui_manager_insert_action_group( ui_manager , mainwin_action_group_others , 0 );

  return;
}


void
ui_manager_create_menus ( void )
{
  GError *gerr = NULL;

  /* attach xml menu definitions */
  gtk_ui_manager_add_ui_from_file( ui_manager , DATA_DIR "/ui/mainwin.ui" , &gerr );

  if ( gerr != NULL )
  {
    g_critical( "Error creating UI<ui/mainwin.ui>: %s" , gerr->message );
    g_error_free( gerr );
    return;
  }

  /* create GtkMenu widgets using path from xml definitions */
  mainwin_songname_menu = ui_manager_get_popup_menu( ui_manager , "/mainwin-menus/songname-menu" );
  mainwin_visualization_menu = ui_manager_get_popup_menu( ui_manager , "/mainwin-menus/main-menu/visualization" );
  mainwin_playback_menu = ui_manager_get_popup_menu( ui_manager , "/mainwin-menus/main-menu/playback" );
  mainwin_playlist_menu = ui_manager_get_popup_menu( ui_manager , "/mainwin-menus/main-menu/playlist" );
  mainwin_view_menu = ui_manager_get_popup_menu( ui_manager , "/mainwin-menus/main-menu/view" );
  mainwin_general_menu = ui_manager_get_popup_menu( ui_manager , "/mainwin-menus/main-menu" );

  return;
}


GtkAccelGroup *
ui_manager_get_accel_group ( void )
{
  return gtk_ui_manager_get_accel_group( ui_manager );
}


GtkWidget *
ui_manager_get_popup_menu ( GtkUIManager * self , const gchar * path )
{
  GtkWidget *menu_item = gtk_ui_manager_get_widget( self , path );

  if (GTK_IS_MENU_ITEM(menu_item))
    return gtk_menu_item_get_submenu(GTK_MENU_ITEM(menu_item));
  else
    return NULL;
}


static void
menu_popup_pos_func (GtkMenu * menu , gint * x , gint * y , gboolean * push_in , gint * point )
{
  GtkRequisition requisition;
  gint screen_width;
  gint screen_height;

  gtk_widget_size_request(GTK_WIDGET(menu), &requisition);

  screen_width = gdk_screen_width();
  screen_height = gdk_screen_height();

  *x = CLAMP(point[0] - 2, 0, MAX(0, screen_width - requisition.width));
  *y = CLAMP(point[1] - 2, 0, MAX(0, screen_height - requisition.height));

  *push_in = FALSE;
}


void
ui_manager_popup_menu_show ( GtkMenu * menu , gint x , gint y , guint button , guint time )
{
  gint pos[2];
  pos[0] = x;
  pos[1] = y;

  gtk_menu_popup( menu , NULL , NULL ,
    (GtkMenuPositionFunc) menu_popup_pos_func , pos , button , time );
}
