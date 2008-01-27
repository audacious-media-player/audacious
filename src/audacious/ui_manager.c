/*  Audacious - Cross-platform multimedia player
 *  Copyright (C) 2005-2007  Audacious development team.
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

#include "ui_manager.h"
#include "actions-mainwin.h"
#include "actions-playlist.h"
#include "actions-equalizer.h"

/* this header contains prototypes for plugin-available menu functions */
#include "ui_plugin_menu.h"

/* TODO ui_main.h is only included because ui_manager.c needs the values of
   TimerMode enum; move that enum elsewhere so we can get rid of this include */
#include "ui_main.h"

#include "icons-stock.h"

#include "sync-menu.h"

static GtkUIManager *ui_manager = NULL;
static gboolean menu_created = FALSE;


/* toggle action entries */

static GtkToggleActionEntry toggleaction_entries_others[] = {

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

	{ "view scale", NULL , N_("Scale"), "<Ctrl>D",
	  N_("DoubleSize"), G_CALLBACK(action_view_scale) , FALSE },

	{ "view easy move", NULL , N_("Easy Move"), "<Ctrl>E",
	  N_("Easy Move"), G_CALLBACK(action_view_easymove) , FALSE }
};



/* radio action entries */

static GtkRadioActionEntry radioaction_entries_vismode[] = {
	{ "vismode analyzer", NULL , N_("Analyzer"), NULL, N_("Analyzer"), VIS_ANALYZER },
	{ "vismode scope", NULL , N_("Scope"), NULL, N_("Scope"), VIS_SCOPE },
	{ "vismode voiceprint", NULL , N_("Voiceprint"), NULL, N_("Voiceprint"), VIS_VOICEPRINT },
	{ "vismode off", NULL , N_("Off"), NULL, N_("Off"), VIS_OFF }
};

static GtkRadioActionEntry radioaction_entries_anamode[] = {
	{ "anamode normal", NULL , N_("Normal"), NULL, N_("Normal"), ANALYZER_NORMAL },
	{ "anamode fire", NULL , N_("Fire"), NULL, N_("Fire"), ANALYZER_FIRE },
	{ "anamode vertical lines", NULL , N_("Vertical Lines"), NULL, N_("Vertical Lines"), ANALYZER_VLINES }
};

static GtkRadioActionEntry radioaction_entries_anatype[] = {
	{ "anatype lines", NULL , N_("Lines"), NULL, N_("Lines"), ANALYZER_LINES },
	{ "anatype bars", NULL , N_("Bars"), NULL, N_("Bars"), ANALYZER_BARS }
};

static GtkRadioActionEntry radioaction_entries_scomode[] = {
	{ "scomode dot", NULL , N_("Dot Scope"), NULL, N_("Dot Scope"), SCOPE_DOT },
	{ "scomode line", NULL , N_("Line Scope"), NULL, N_("Line Scope"), SCOPE_LINE },
	{ "scomode solid", NULL , N_("Solid Scope"), NULL, N_("Solid Scope"), SCOPE_SOLID }
};

static GtkRadioActionEntry radioaction_entries_vprmode[] = {
	{ "vprmode normal", NULL , N_("Normal"), NULL, N_("Normal"), VOICEPRINT_NORMAL },
	{ "vprmode fire", NULL , N_("Fire"), NULL, N_("Fire"), VOICEPRINT_FIRE },
	{ "vprmode ice", NULL , N_("Ice"), NULL, N_("Ice"), VOICEPRINT_ICE }
};

static GtkRadioActionEntry radioaction_entries_wshmode[] = {
	{ "wshmode normal", NULL , N_("Normal"), NULL, N_("Normal"), VU_NORMAL },
	{ "wshmode smooth", NULL , N_("Smooth"), NULL, N_("Smooth"), VU_SMOOTH }
};

static GtkRadioActionEntry radioaction_entries_refrate[] = {
	{ "refrate full", NULL , N_("Full (~50 fps)"), NULL, N_("Full (~50 fps)"), REFRESH_FULL },
	{ "refrate half", NULL , N_("Half (~25 fps)"), NULL, N_("Half (~25 fps)"), REFRESH_HALF },
	{ "refrate quarter", NULL , N_("Quarter (~13 fps)"), NULL, N_("Quarter (~13 fps)"), REFRESH_QUARTER },
	{ "refrate eighth", NULL , N_("Eighth (~6 fps)"), NULL, N_("Eighth (~6 fps)"), REFRESH_EIGTH }
};

static GtkRadioActionEntry radioaction_entries_anafoff[] = {
	{ "anafoff slowest", NULL , N_("Slowest"), NULL, N_("Slowest"), FALLOFF_SLOWEST },
	{ "anafoff slow", NULL , N_("Slow"), NULL, N_("Slow"), FALLOFF_SLOW },
	{ "anafoff medium", NULL , N_("Medium"), NULL, N_("Medium"), FALLOFF_MEDIUM },
	{ "anafoff fast", NULL , N_("Fast"), NULL, N_("Fast"), FALLOFF_FAST },
	{ "anafoff fastest", NULL , N_("Fastest"), NULL, N_("Fastest"), FALLOFF_FASTEST }
};

static GtkRadioActionEntry radioaction_entries_peafoff[] = {
	{ "peafoff slowest", NULL , N_("Slowest"), NULL, N_("Slowest"), FALLOFF_SLOWEST },
	{ "peafoff slow", NULL , N_("Slow"), NULL, N_("Slow"), FALLOFF_SLOW },
	{ "peafoff medium", NULL , N_("Medium"), NULL, N_("Medium"), FALLOFF_MEDIUM },
	{ "peafoff fast", NULL , N_("Fast"), NULL, N_("Fast"), FALLOFF_FAST },
	{ "peafoff fastest", NULL , N_("Fastest"), NULL, N_("Fastest"), FALLOFF_FASTEST }
};

static GtkRadioActionEntry radioaction_entries_viewtime[] = {
	{ "view time elapsed", NULL , N_("Time Elapsed"), "<Ctrl>E", N_("Time Elapsed"), TIMER_ELAPSED },
	{ "view time remaining", NULL , N_("Time Remaining"), "<Ctrl>R", N_("Time Remaining"), TIMER_REMAINING }
};



/* normal actions */

static GtkActionEntry action_entries_playback[] = {

	{ "playback", NULL, N_("Playback") },
	
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


static GtkActionEntry action_entries_visualization[] = {
	{ "visualization", NULL, N_("Visualization") },
	{ "vismode", NULL, N_("Visualization Mode") },
	{ "anamode", NULL, N_("Analyzer Mode") },
	{ "scomode", NULL, N_("Scope Mode") },
	{ "vprmode", NULL, N_("Voiceprint Mode") },
	{ "wshmode", NULL, N_("WindowShade VU Mode") },
	{ "refrate", NULL, N_("Refresh Rate") },
	{ "anafoff", NULL, N_("Analyzer Falloff") },
	{ "peafoff", NULL, N_("Peaks Falloff") }
};

static GtkActionEntry action_entries_playlist[] = {

	{ "playlist", NULL, N_("Playlist") },

	{ "playlist new", GTK_STOCK_NEW , N_("New Playlist"), "<Shift>N",
	  N_("New Playlist"), G_CALLBACK(action_playlist_new) },

	{ "playlist select next", GTK_STOCK_MEDIA_NEXT, N_("Select Next Playlist"), "<Shift>P",
	  N_("Select Next Playlist"), G_CALLBACK(action_playlist_next) },

	{ "playlist select previous", GTK_STOCK_MEDIA_PREVIOUS, N_("Select Previous Playlist"), "<Shift><Ctrl>P",
	  N_("Select Previous Playlist"), G_CALLBACK(action_playlist_prev) },

	{ "playlist delete", GTK_STOCK_DELETE , N_("Delete Playlist"), "<Shift>D",
	  N_("Delete Playlist"), G_CALLBACK(action_playlist_delete) },

        { "playlist load", GTK_STOCK_OPEN, N_("Load List"), "O",
          N_("Loads a playlist file into the selected playlist."), G_CALLBACK(action_playlist_load_list) },

        { "playlist save", GTK_STOCK_SAVE, N_("Save List"), "<Shift>S",
          N_("Saves the selected playlist."), G_CALLBACK(action_playlist_save_list) },

        { "playlist save default", GTK_STOCK_SAVE, N_("Save Default List"), "<Alt>S",
          N_("Saves the selected playlist to the default location."),
          G_CALLBACK(action_playlist_save_default_list) },

        { "playlist refresh", GTK_STOCK_REFRESH, N_("Refresh List"), "F5",
          N_("Refreshes metadata associated with a playlist entry."),
          G_CALLBACK(action_playlist_refresh_list) },

        { "playlist manager", AUD_STOCK_PLAYLIST , N_("List Manager"), "P",
          N_("Opens the playlist manager."),
          G_CALLBACK(action_open_list_manager) }
};

static GtkActionEntry action_entries_view[] = {

	{ "view", NULL, N_("View") }
};

static GtkActionEntry action_entries_playlist_add[] = {
        { "playlist add url", GTK_STOCK_NETWORK, N_("Add Internet Address..."), "<Ctrl>H",
          N_("Adds a remote track to the playlist."),
          G_CALLBACK(action_playlist_add_url) },

        { "playlist add files", GTK_STOCK_ADD, N_("Add Files..."), "F",
          N_("Adds files to the playlist."),
          G_CALLBACK(action_playlist_add_files) },
};

static GtkActionEntry action_entries_playlist_select[] = {
        { "playlist search and select", GTK_STOCK_FIND, N_("Search and Select"), "<Ctrl>F",
          N_("Searches the playlist and selects playlist entries based on specific criteria."),
          G_CALLBACK(action_playlist_search_and_select) },

        { "playlist invert selection", NULL , N_("Invert Selection"), NULL,
          N_("Inverts the selected and unselected entries."),
          G_CALLBACK(action_playlist_invert_selection) },

        { "playlist select all", NULL , N_("Select All"), "<Ctrl>A",
          N_("Selects all of the playlist entries."),
          G_CALLBACK(action_playlist_select_all) },

        { "playlist select none", NULL , N_("Select None"), "<Shift><Ctrl>A",
          N_("Deselects all of the playlist entries."),
          G_CALLBACK(action_playlist_select_none) },
};

static GtkActionEntry action_entries_playlist_delete[] = {
	{ "playlist remove all", GTK_STOCK_CLEAR, N_("Remove All"), NULL, 
	  N_("Removes all entries from the playlist."),
	  G_CALLBACK(action_playlist_remove_all) },

	{ "playlist clear queue", GTK_STOCK_CLEAR, N_("Clear Queue"), "<Shift>Q",
	  N_("Clears the queue associated with this playlist."),
	  G_CALLBACK(action_playlist_clear_queue) },

	{ "playlist remove unavailable", GTK_STOCK_DIALOG_ERROR , N_("Remove Unavailable Files"), NULL,
	  N_("Removes unavailable files from the playlist."),
	  G_CALLBACK(action_playlist_remove_unavailable) },

	{ "playlist remove dups menu", NULL , N_("Remove Duplicates") },

	{ "playlist remove dups by title", NULL , N_("By Title"), NULL,
	  N_("Removes duplicate entries from the playlist by title."),
	  G_CALLBACK(action_playlist_remove_dupes_by_title) },

	{ "playlist remove dups by filename", NULL , N_("By Filename"), NULL, 
	  N_("Removes duplicate entries from the playlist by filename."),
	  G_CALLBACK(action_playlist_remove_dupes_by_filename) },

	{ "playlist remove dups by full path", NULL , N_("By Path + Filename"), NULL, 
	  N_("Removes duplicate entries from the playlist by their full path."),
	  G_CALLBACK(action_playlist_remove_dupes_by_full_path) },

	{ "playlist remove unselected", GTK_STOCK_REMOVE, N_("Remove Unselected"), NULL,
	  N_("Remove unselected entries from the playlist."),
	  G_CALLBACK(action_playlist_remove_unselected) },

	{ "playlist remove selected", GTK_STOCK_REMOVE, N_("Remove Selected"), "Delete", 
	  N_("Remove selected entries from the playlist."),
	  G_CALLBACK(action_playlist_remove_selected) },
};

static GtkActionEntry action_entries_playlist_sort[] = {
	{ "playlist randomize list", AUD_STOCK_RANDOMIZEPL , N_("Randomize List"), "<Ctrl><Shift>R",
	  N_("Randomizes the playlist."),
	  G_CALLBACK(action_playlist_randomize_list) },

	{ "playlist reverse list", GTK_STOCK_GO_UP , N_("Reverse List"), NULL,
	  N_("Reverses the playlist."),
	  G_CALLBACK(action_playlist_reverse_list) },

	{ "playlist sort menu", GTK_STOCK_GO_DOWN , N_("Sort List") },

	{ "playlist sort by title", NULL , N_("By Title"), NULL,
	  N_("Sorts the list by title."),
	  G_CALLBACK(action_playlist_sort_by_title) },

	{ "playlist sort by artist", NULL , N_("By Artist"), NULL,
	  N_("Sorts the list by artist."),
	  G_CALLBACK(action_playlist_sort_by_artist) },

	{ "playlist sort by filename", NULL , N_("By Filename"), NULL,
	  N_("Sorts the list by filename."),
	  G_CALLBACK(action_playlist_sort_by_filename) },

	{ "playlist sort by full path", NULL , N_("By Path + Filename"), NULL,
	  N_("Sorts the list by full pathname."),
	  G_CALLBACK(action_playlist_sort_by_full_path) },

	{ "playlist sort by date", NULL , N_("By Date"), NULL,
	  N_("Sorts the list by modification time."),
	  G_CALLBACK(action_playlist_sort_by_date) },

	{ "playlist sort by track number", NULL , N_("By Track Number"), NULL,
	  N_("Sorts the list by track number."),
	  G_CALLBACK(action_playlist_sort_by_track_number) },

	{ "playlist sort by playlist entry", NULL , N_("By Playlist Entry"), NULL,
	  N_("Sorts the list by playlist entry."),
	  G_CALLBACK(action_playlist_sort_by_playlist_entry) },

	{ "playlist sort selected menu", GTK_STOCK_GO_DOWN , N_("Sort Selected") },

	{ "playlist sort selected by title", NULL , N_("By Title"), NULL,
	  N_("Sorts the list by title."),
	  G_CALLBACK(action_playlist_sort_selected_by_title) },

	{ "playlist sort selected by artist", NULL, N_("By Artist"), NULL,
	  N_("Sorts the list by artist."),
	  G_CALLBACK(action_playlist_sort_selected_by_artist) },

	{ "playlist sort selected by filename", NULL , N_("By Filename"), NULL,
	  N_("Sorts the list by filename."),
	  G_CALLBACK(action_playlist_sort_selected_by_filename) },

	{ "playlist sort selected by full path", NULL , N_("By Path + Filename"), NULL,
	  N_("Sorts the list by full pathname."),
	  G_CALLBACK(action_playlist_sort_selected_by_full_path) },

	{ "playlist sort selected by date", NULL , N_("By Date"), NULL,
	  N_("Sorts the list by modification time."),
	  G_CALLBACK(action_playlist_sort_selected_by_date) },

	{ "playlist sort selected by track number", NULL , N_("By Track Number"), NULL,
	  N_("Sorts the list by track number."),
	  G_CALLBACK(action_playlist_sort_selected_by_track_number) },

	{ "playlist sort selected by playlist entry", NULL, N_("By Playlist Entry"), NULL,
	  N_("Sorts the list by playlist entry."),
	  G_CALLBACK(action_playlist_sort_selected_by_playlist_entry) },
};

static GtkActionEntry action_entries_others[] = {

	{ "dummy", NULL, "dummy" },

        /* XXX Carbon support */
        { "file", NULL, N_("File") },
        { "help", NULL, N_("Help") },

	{ "plugins-menu", AUD_STOCK_PLUGIN, N_("Plugin Services") },

	{ "current track info", GTK_STOCK_INFO , N_("View Track Details"), "I",
	  N_("View track details"), G_CALLBACK(action_current_track_info) },

	{ "playlist track info", GTK_STOCK_INFO , N_("View Track Details"), "<Alt>I",
	  N_("View track details"), G_CALLBACK(action_playlist_track_info) },

	{ "about audacious", GTK_STOCK_DIALOG_INFO , N_("About Audacious"), NULL,
	  N_("About Audacious"), G_CALLBACK(action_about_audacious) },

	{ "play file", GTK_STOCK_OPEN , N_("Play File"), "L",
	  N_("Load and play a file"), G_CALLBACK(action_play_file) },

	{ "play location", GTK_STOCK_NETWORK , N_("Play Location"), "<Ctrl>L",
	  N_("Play media from the selected location"), G_CALLBACK(action_play_location) },

    { "plugins", NULL , N_("Plugin services") },

	{ "preferences", GTK_STOCK_PREFERENCES , N_("Preferences"), "<Ctrl>P",
	  N_("Open preferences window"), G_CALLBACK(action_preferences) },

	{ "quit", GTK_STOCK_QUIT , N_("_Quit"), NULL,
	  N_("Quit Audacious"), G_CALLBACK(action_quit) },

	{ "ab set", NULL , N_("Set A-B"), "A",
	  N_("Set A-B"), G_CALLBACK(action_ab_set) },

	{ "ab clear", NULL , N_("Clear A-B"), "<Shift>A",
	  N_("Clear A-B"), G_CALLBACK(action_ab_clear) },

	{ "jump to playlist start", GTK_STOCK_GOTO_TOP , N_("Jump to Playlist Start"), "<Ctrl>Z",
	  N_("Jump to Playlist Start"), G_CALLBACK(action_jump_to_playlist_start) },

	{ "jump to file", GTK_STOCK_JUMP_TO , N_("Jump to File"), "J",
	  N_("Jump to File"), G_CALLBACK(action_jump_to_file) },

	{ "jump to time", GTK_STOCK_JUMP_TO , N_("Jump to Time"), "<Ctrl>J",
	  N_("Jump to Time"), G_CALLBACK(action_jump_to_time) },

	{ "queue toggle", AUD_STOCK_QUEUETOGGLE , N_("Queue Toggle"), "Q", 
	  N_("Enables/disables the entry in the playlist's queue."),
	  G_CALLBACK(action_queue_toggle) },
};


static GtkActionEntry action_entries_equalizer[] = {

    { "equ preset load menu", NULL, N_("Load") },
    { "equ preset import menu", NULL, N_("Import") },
    { "equ preset save menu", NULL, N_("Save") },
    { "equ preset delete menu", NULL, N_("Delete") },

    { "equ load preset", NULL, N_("Preset"), NULL,
      N_("Load preset"), G_CALLBACK(action_equ_load_preset) },

    { "equ load auto preset", NULL, N_("Auto-load preset"), NULL,
      N_("Load auto-load preset"), G_CALLBACK(action_equ_load_auto_preset) },

    { "equ load default preset", NULL, N_("Default"), NULL,
      N_("Load default preset into equalizer"), G_CALLBACK(action_equ_load_default_preset) },

    { "equ zero preset", NULL, N_("Zero"), NULL,
      N_("Set equalizer preset levels to zero"), G_CALLBACK(action_equ_zero_preset) },

    { "equ load preset file", NULL, N_("From file"), NULL,
      N_("Load preset from file"), G_CALLBACK(action_equ_load_preset_file) },

    { "equ load preset eqf", NULL, N_("From WinAMP EQF file"), NULL,
      N_("Load preset from WinAMP EQF file"), G_CALLBACK(action_equ_load_preset_eqf) },

    { "equ import winamp presets", NULL, N_("WinAMP Presets"), NULL,
      N_("Import WinAMP presets"), G_CALLBACK(action_equ_import_winamp_presets) },

    { "equ save preset", NULL, N_("Preset"), NULL,
      N_("Save preset"), G_CALLBACK(action_equ_save_preset) },

    { "equ save auto preset", NULL, N_("Auto-load preset"), NULL,
      N_("Save auto-load preset"), G_CALLBACK(action_equ_save_auto_preset) },

    { "equ save default preset", NULL, N_("Default"), NULL,
      N_("Save default preset"), G_CALLBACK(action_equ_save_default_preset) },

    { "equ save preset file", NULL, N_("To file"), NULL,
      N_("Save preset to file"), G_CALLBACK(action_equ_save_preset_file) },

    { "equ save preset eqf", NULL, N_("To WinAMP EQF file"), NULL,
      N_("Save preset to WinAMP EQF file"), G_CALLBACK(action_equ_save_preset_eqf) },

    { "equ delete preset", NULL, N_("Preset"), NULL,
      N_("Delete preset"), G_CALLBACK(action_equ_delete_preset) },

    { "equ delete auto preset", NULL, N_("Auto-load preset"), NULL,
      N_("Delete auto-load preset"), G_CALLBACK(action_equ_delete_auto_preset) }
};



/* ***************************** */


static GtkActionGroup *
ui_manager_new_action_group( const gchar * group_name )
{
  GtkActionGroup *group = gtk_action_group_new( group_name );
  gtk_action_group_set_translation_domain( group , PACKAGE_NAME );
  return group;
}

void
ui_manager_init ( void )
{
  /* toggle actions */
  toggleaction_group_others = ui_manager_new_action_group("toggleaction_others");
  gtk_action_group_add_toggle_actions(
    toggleaction_group_others , toggleaction_entries_others ,
    G_N_ELEMENTS(toggleaction_entries_others) , NULL );

  /* radio actions */
  radioaction_group_anamode = ui_manager_new_action_group("radioaction_anamode");
  gtk_action_group_add_radio_actions(
    radioaction_group_anamode , radioaction_entries_anamode ,
    G_N_ELEMENTS(radioaction_entries_anamode) , 0 , G_CALLBACK(action_anamode) , NULL );

  radioaction_group_anatype = ui_manager_new_action_group("radioaction_anatype");
  gtk_action_group_add_radio_actions(
    radioaction_group_anatype , radioaction_entries_anatype ,
    G_N_ELEMENTS(radioaction_entries_anatype) , 0 , G_CALLBACK(action_anatype) , NULL );

  radioaction_group_scomode = ui_manager_new_action_group("radioaction_scomode");
  gtk_action_group_add_radio_actions(
    radioaction_group_scomode , radioaction_entries_scomode ,
    G_N_ELEMENTS(radioaction_entries_scomode) , 0 , G_CALLBACK(action_scomode) , NULL );

  radioaction_group_vprmode = ui_manager_new_action_group("radioaction_vprmode");
  gtk_action_group_add_radio_actions(
    radioaction_group_vprmode , radioaction_entries_vprmode ,
    G_N_ELEMENTS(radioaction_entries_vprmode) , 0 , G_CALLBACK(action_vprmode) , NULL );

  radioaction_group_wshmode = ui_manager_new_action_group("radioaction_wshmode");
  gtk_action_group_add_radio_actions(
    radioaction_group_wshmode , radioaction_entries_wshmode ,
    G_N_ELEMENTS(radioaction_entries_wshmode) , 0 , G_CALLBACK(action_wshmode) , NULL );

  radioaction_group_refrate = ui_manager_new_action_group("radioaction_refrate");
  gtk_action_group_add_radio_actions(
    radioaction_group_refrate , radioaction_entries_refrate ,
    G_N_ELEMENTS(radioaction_entries_refrate) , 0 , G_CALLBACK(action_refrate) , NULL );

  radioaction_group_anafoff = ui_manager_new_action_group("radioaction_anafoff");
  gtk_action_group_add_radio_actions(
    radioaction_group_anafoff , radioaction_entries_anafoff ,
    G_N_ELEMENTS(radioaction_entries_anafoff) , 0 , G_CALLBACK(action_anafoff) , NULL );

  radioaction_group_peafoff = ui_manager_new_action_group("radioaction_peafoff");
  gtk_action_group_add_radio_actions(
    radioaction_group_peafoff , radioaction_entries_peafoff ,
    G_N_ELEMENTS(radioaction_entries_peafoff) , 0 , G_CALLBACK(action_peafoff) , NULL );

  radioaction_group_vismode = ui_manager_new_action_group("radioaction_vismode");
  gtk_action_group_add_radio_actions(
    radioaction_group_vismode , radioaction_entries_vismode ,
    G_N_ELEMENTS(radioaction_entries_vismode) , 0 , G_CALLBACK(action_vismode) , NULL );

  radioaction_group_viewtime = ui_manager_new_action_group("radioaction_viewtime");
  gtk_action_group_add_radio_actions(
    radioaction_group_viewtime , radioaction_entries_viewtime ,
    G_N_ELEMENTS(radioaction_entries_viewtime) , 0 , G_CALLBACK(action_viewtime) , NULL );

  /* normal actions */
  action_group_playback = ui_manager_new_action_group("action_playback");
    gtk_action_group_add_actions(
    action_group_playback , action_entries_playback ,
    G_N_ELEMENTS(action_entries_playback) , NULL );

  action_group_playlist = ui_manager_new_action_group("action_playlist");
    gtk_action_group_add_actions(
    action_group_playlist , action_entries_playlist ,
    G_N_ELEMENTS(action_entries_playlist) , NULL );

  action_group_visualization = ui_manager_new_action_group("action_visualization");
    gtk_action_group_add_actions(
    action_group_visualization , action_entries_visualization ,
    G_N_ELEMENTS(action_entries_visualization) , NULL );

  action_group_view = ui_manager_new_action_group("action_view");
    gtk_action_group_add_actions(
    action_group_view , action_entries_view ,
    G_N_ELEMENTS(action_entries_view) , NULL );

  action_group_others = ui_manager_new_action_group("action_others");
    gtk_action_group_add_actions(
    action_group_others , action_entries_others ,
    G_N_ELEMENTS(action_entries_others) , NULL );

  action_group_playlist_add = ui_manager_new_action_group("action_playlist_add");
  gtk_action_group_add_actions(
    action_group_playlist_add, action_entries_playlist_add,
    G_N_ELEMENTS(action_entries_playlist_add), NULL );

  action_group_playlist_select = ui_manager_new_action_group("action_playlist_select");
  gtk_action_group_add_actions(
    action_group_playlist_select, action_entries_playlist_select,
    G_N_ELEMENTS(action_entries_playlist_select), NULL );

  action_group_playlist_delete = ui_manager_new_action_group("action_playlist_delete");
  gtk_action_group_add_actions(
    action_group_playlist_delete, action_entries_playlist_delete,
    G_N_ELEMENTS(action_entries_playlist_delete), NULL );

  action_group_playlist_sort = ui_manager_new_action_group("action_playlist_sort");
  gtk_action_group_add_actions(
    action_group_playlist_sort, action_entries_playlist_sort,
    G_N_ELEMENTS(action_entries_playlist_sort), NULL );

  action_group_equalizer = ui_manager_new_action_group("action_equalizer");
  gtk_action_group_add_actions(
    action_group_equalizer, action_entries_equalizer,
    G_N_ELEMENTS(action_entries_equalizer), NULL);

  /* ui */
  ui_manager = gtk_ui_manager_new();
  gtk_ui_manager_insert_action_group( ui_manager , toggleaction_group_others , 0 );
  gtk_ui_manager_insert_action_group( ui_manager , radioaction_group_anamode , 0 );
  gtk_ui_manager_insert_action_group( ui_manager , radioaction_group_anatype , 0 );
  gtk_ui_manager_insert_action_group( ui_manager , radioaction_group_scomode , 0 );
  gtk_ui_manager_insert_action_group( ui_manager , radioaction_group_vprmode , 0 );
  gtk_ui_manager_insert_action_group( ui_manager , radioaction_group_wshmode , 0 );
  gtk_ui_manager_insert_action_group( ui_manager , radioaction_group_refrate , 0 );
  gtk_ui_manager_insert_action_group( ui_manager , radioaction_group_anafoff , 0 );
  gtk_ui_manager_insert_action_group( ui_manager , radioaction_group_peafoff , 0 );
  gtk_ui_manager_insert_action_group( ui_manager , radioaction_group_vismode , 0 );
  gtk_ui_manager_insert_action_group( ui_manager , radioaction_group_viewtime , 0 );
  gtk_ui_manager_insert_action_group( ui_manager , action_group_playback , 0 );
  gtk_ui_manager_insert_action_group( ui_manager , action_group_playlist , 0 );
  gtk_ui_manager_insert_action_group( ui_manager , action_group_visualization , 0 );
  gtk_ui_manager_insert_action_group( ui_manager , action_group_view , 0 );
  gtk_ui_manager_insert_action_group( ui_manager , action_group_others , 0 );
  gtk_ui_manager_insert_action_group( ui_manager , action_group_playlist_add , 0 );
  gtk_ui_manager_insert_action_group( ui_manager , action_group_playlist_select , 0 );
  gtk_ui_manager_insert_action_group( ui_manager , action_group_playlist_delete , 0 );
  gtk_ui_manager_insert_action_group( ui_manager , action_group_playlist_sort , 0 );
  gtk_ui_manager_insert_action_group( ui_manager , action_group_equalizer , 0 );

  return;
}

#ifdef GDK_WINDOWING_QUARTZ
static GtkWidget *carbon_menubar;
#endif

static void
ui_manager_create_menus_init_pmenu( gchar * path )
{
  GtkWidget *plugins_menu_item = gtk_ui_manager_get_widget( ui_manager , path );
  if ( plugins_menu_item )
  {
    /* initially set count of items under plugins_menu_item to 0 */
    g_object_set_data( G_OBJECT(plugins_menu_item) , "ic" , GINT_TO_POINTER(0) );
    /* and since it's 0, hide the plugins_menu_item */
    gtk_widget_hide( plugins_menu_item );
  }
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

  /* initialize plugins-menu for mainwin-menus */
  ui_manager_create_menus_init_pmenu( "/mainwin-menus/main-menu/plugins-menu" );

#ifdef GDK_WINDOWING_QUARTZ
  gtk_ui_manager_add_ui_from_file( ui_manager , DATA_DIR "/ui/carbon-menubar.ui" , &gerr );

  if ( gerr != NULL )
  {
    g_critical( "Error creating UI<ui/carbon-menubar.ui>: %s" , gerr->message );
    g_error_free( gerr );
    return;
  }

  carbon_menubar = ui_manager_get_popup_menu( ui_manager , "/carbon-menubar/main-menu" );
  sync_menu_takeover_menu(GTK_MENU_SHELL(carbon_menubar));
#endif

  gtk_ui_manager_add_ui_from_file( ui_manager , DATA_DIR "/ui/playlist.ui" , &gerr );

  if ( gerr != NULL )
  {
    g_critical( "Error creating UI<ui/playlist.ui>: %s" , gerr->message );
    g_error_free( gerr );
    return;
  }

  playlistwin_popup_menu = ui_manager_get_popup_menu(ui_manager, "/playlist-menus/playlist-rightclick-menu");

  playlistwin_pladd_menu  = ui_manager_get_popup_menu(ui_manager, "/playlist-menus/add-menu");
  playlistwin_pldel_menu  = ui_manager_get_popup_menu(ui_manager, "/playlist-menus/del-menu");
  playlistwin_plsel_menu  = ui_manager_get_popup_menu(ui_manager, "/playlist-menus/select-menu");
  playlistwin_plsort_menu = ui_manager_get_popup_menu(ui_manager, "/playlist-menus/misc-menu");
  playlistwin_pllist_menu = ui_manager_get_popup_menu(ui_manager, "/playlist-menus/playlist-menu");

  /* initialize plugins-menu for playlist-menus */
  ui_manager_create_menus_init_pmenu( "/playlist-menus/playlist-menu/plugins-menu" );
  ui_manager_create_menus_init_pmenu( "/playlist-menus/add-menu/plugins-menu" );
  ui_manager_create_menus_init_pmenu( "/playlist-menus/del-menu/plugins-menu" );
  ui_manager_create_menus_init_pmenu( "/playlist-menus/select-menu/plugins-menu" );
  ui_manager_create_menus_init_pmenu( "/playlist-menus/misc-menu/plugins-menu" );
  ui_manager_create_menus_init_pmenu( "/playlist-menus/playlist-rightclick-menu/plugins-menu" );

  gtk_ui_manager_add_ui_from_file( ui_manager , DATA_DIR "/ui/equalizer.ui" , &gerr );

  if ( gerr != NULL )
  {
    g_critical( "Error creating UI<ui/equalizer.ui>: %s" , gerr->message );
    g_error_free( gerr );
    return;
  }

  equalizerwin_presets_menu = ui_manager_get_popup_menu(ui_manager, "/equalizer-menus/preset-menu");

  menu_created = TRUE;

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



/******************************/
/* plugin-available functions */

#define _MP_GWID(y) gtk_ui_manager_get_widget( ui_manager , y )

static GtkWidget*
audacious_menu_plugin_menuwid( menu_id )
{
  switch (menu_id)
  {
    case AUDACIOUS_MENU_MAIN:
      return _MP_GWID("/mainwin-menus/main-menu/plugins-menu");
    case AUDACIOUS_MENU_PLAYLIST:
      return _MP_GWID("/playlist-menus/playlist-menu/plugins-menu");
    case AUDACIOUS_MENU_PLAYLIST_RCLICK:
      return _MP_GWID("/playlist-menus/playlist-rightclick-menu/plugins-menu");
    case AUDACIOUS_MENU_PLAYLIST_ADD:
      return _MP_GWID("/playlist-menus/add-menu/plugins-menu");
    case AUDACIOUS_MENU_PLAYLIST_REMOVE:
      return _MP_GWID("/playlist-menus/del-menu/plugins-menu");
    case AUDACIOUS_MENU_PLAYLIST_SELECT:
      return _MP_GWID("/playlist-menus/select-menu/plugins-menu");
    case AUDACIOUS_MENU_PLAYLIST_MISC:
      return _MP_GWID("/playlist-menus/misc-menu/plugins-menu");
    default:
      return NULL;
  }
}


gint
menu_plugin_item_add( gint menu_id , GtkWidget * item )
{
  if ( menu_created )
  {
    GtkWidget *plugins_menu = NULL;
    GtkWidget *plugins_menu_item = audacious_menu_plugin_menuwid( menu_id );
    if ( plugins_menu_item )
    {
      gint ic = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(plugins_menu_item),"ic"));
      if ( ic == 0 ) /* no items under plugins_menu_item, create the submenu */
      {
        plugins_menu = gtk_menu_new();
        gtk_menu_item_set_submenu( GTK_MENU_ITEM(plugins_menu_item), plugins_menu );
      }
      else /* items available under plugins_menu_item, pick the existing submenu */
      {
        plugins_menu = gtk_menu_item_get_submenu( GTK_MENU_ITEM(plugins_menu_item) );
        if ( !plugins_menu ) return -1;
      }
      gtk_menu_shell_append( GTK_MENU_SHELL(plugins_menu) , item );
      gtk_widget_show( plugins_menu_item );
      g_object_set_data( G_OBJECT(plugins_menu_item) , "ic" , GINT_TO_POINTER(++ic) );
      return 0; /* success */
    }
  }
  return -1; /* failure */
}


gint
menu_plugin_item_remove( gint menu_id , GtkWidget * item )
{
  if ( menu_created )
  {
    GtkWidget *plugins_menu = NULL;
    GtkWidget *plugins_menu_item = audacious_menu_plugin_menuwid( menu_id );
    if ( plugins_menu_item )
    {
      gint ic = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(plugins_menu_item),"ic"));
      if ( ic > 0 )
      {
        plugins_menu = gtk_menu_item_get_submenu( GTK_MENU_ITEM(plugins_menu_item) );
        if ( plugins_menu )
        {
          /* remove the plugin-added entry */
          gtk_container_remove( GTK_CONTAINER(plugins_menu) , item );
          g_object_set_data( G_OBJECT(plugins_menu_item) , "ic" , GINT_TO_POINTER(--ic) );
          if ( ic == 0 ) /* if the menu is empty now, destroy it */
          {
            gtk_menu_item_remove_submenu( GTK_MENU_ITEM(plugins_menu_item) );
            gtk_widget_destroy( plugins_menu );
            gtk_widget_hide( plugins_menu_item );
          }
          return 0; /* success */
        }
      }
    }
  }
  return -1; /* failure */
}
