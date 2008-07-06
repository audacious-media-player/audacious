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

#ifndef AUDACIOUS_UI_MANAGER_H
#define AUDACIOUS_UI_MANAGER_H

#include <glib.h>
#include <glib/gi18n.h>
#include <gtk/gtk.h>

G_BEGIN_DECLS

GtkWidget *mainwin_songname_menu;
GtkWidget *mainwin_general_menu;
GtkWidget *mainwin_visualization_menu;
GtkWidget *mainwin_playback_menu;
GtkWidget *mainwin_playlist_menu;
GtkWidget *mainwin_view_menu;

GtkWidget *playlistwin_pladd_menu;
GtkWidget *playlistwin_pldel_menu;
GtkWidget *playlistwin_plsel_menu;
GtkWidget *playlistwin_plsort_menu;
GtkWidget *playlistwin_pllist_menu;
GtkWidget *playlistwin_popup_menu;

GtkWidget *equalizerwin_presets_menu;

GtkActionGroup *toggleaction_group_others;
GtkActionGroup *radioaction_group_anamode; /* Analyzer mode */
GtkActionGroup *radioaction_group_anatype; /* Analyzer type */
GtkActionGroup *radioaction_group_scomode; /* Scope mode */
GtkActionGroup *radioaction_group_vprmode; /* Voiceprint mode */
GtkActionGroup *radioaction_group_wshmode; /* WindowShade VU mode */
GtkActionGroup *radioaction_group_refrate; /* Refresh rate */
GtkActionGroup *radioaction_group_anafoff; /* Analyzer Falloff */
GtkActionGroup *radioaction_group_peafoff; /* Peak Falloff */
GtkActionGroup *radioaction_group_vismode; /* Visualization mode */
GtkActionGroup *radioaction_group_viewtime; /* View time (remaining/elapsed) */
GtkActionGroup *action_group_playback;
GtkActionGroup *action_group_visualization;
GtkActionGroup *action_group_view;
GtkActionGroup *action_group_others;
GtkActionGroup *action_group_playlist;
GtkActionGroup *action_group_playlist_add;
GtkActionGroup *action_group_playlist_select;
GtkActionGroup *action_group_playlist_delete;
GtkActionGroup *action_group_playlist_sort;
GtkActionGroup *action_group_equalizer;


void ui_manager_init ( void );
void ui_manager_create_menus ( void );
GtkAccelGroup * ui_manager_get_accel_group ( void );
GtkWidget * ui_manager_get_popup_menu ( GtkUIManager * , const gchar * );
void ui_manager_popup_menu_show( GtkMenu * , gint , gint , guint , guint );
#define popup_menu_show(x1,x2,x3,x4,x5) ui_manager_popup_menu_show(x1,x2,x3,x4,x5)

G_END_DECLS

#endif /* AUDACIOUS_UI_MANAGER_H */
