/*
 * audacious: Cross-platform multimedia player.
 * ui_manager.h: Code for working with GtkUIManager objects.
 *
 * Copyright (c) 2005-2007 Audacious development team.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
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


#endif /* AUD_UIMANAGER_H */
