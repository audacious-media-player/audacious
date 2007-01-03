/*  Audacious - Cross-platform multimedia player
 *  Copyright (C) 2005-2007  Audacious development team.
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

#ifndef AUD_UIMANAGER_H
#define AUD_UIMANAGER_H


#include <glib.h>
#include <glib/gi18n.h>
#include <gtk/gtk.h>


GtkWidget *mainwin_songname_menu;
GtkWidget *mainwin_general_menu;
GtkWidget *mainwin_visualization_menu;
GtkWidget *mainwin_playback_menu;
GtkWidget *mainwin_playlist_menu;
GtkWidget *mainwin_view_menu;

GtkActionGroup *mainwin_toggleaction_group_others;
GtkActionGroup *mainwin_radioaction_group_anamode; /* Analyzer mode */
GtkActionGroup *mainwin_radioaction_group_anatype; /* Analyzer type */
GtkActionGroup *mainwin_radioaction_group_scomode; /* Scope mode */
GtkActionGroup *mainwin_radioaction_group_vprmode; /* Voiceprint mode */
GtkActionGroup *mainwin_radioaction_group_wshmode; /* WindowShade VU mode */
GtkActionGroup *mainwin_radioaction_group_refrate; /* Refresh rate */
GtkActionGroup *mainwin_radioaction_group_anafoff; /* Analyzer Falloff */
GtkActionGroup *mainwin_radioaction_group_peafoff; /* Peak Falloff */
GtkActionGroup *mainwin_radioaction_group_vismode; /* Visualization mode */
GtkActionGroup *mainwin_radioaction_group_viewtime; /* View time (remaining/elapsed) */
GtkActionGroup *mainwin_action_group_playback;
GtkActionGroup *mainwin_action_group_playlist;
GtkActionGroup *mainwin_action_group_visualization;
GtkActionGroup *mainwin_action_group_view;
GtkActionGroup *mainwin_action_group_others;


void uimanager_init ( void );
void uimanager_create_menus ( void );
GtkAccelGroup * ui_manager_get_accel_group ( void );
GtkWidget * ui_manager_get_popup_menu ( GtkUIManager * , const gchar * );
void ui_manager_popup_menu_show( GtkMenu * , gint , gint , guint , guint );
#define popup_menu_show(x1,x2,x3,x4,x5) ui_manager_popup_menu_show(x1,x2,x3,x4,x5)


#endif /* AUD_UIMANAGER_H */
