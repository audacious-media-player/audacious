/*
 * Audacious2
 * Copyright (c) 2008 William Pitcock <nenolod@dereferenced.org>
 * Copyright (c) 2008-2009 Tomasz Moń <desowin@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; under version 3 of the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses>.
 *
 * The Audacious team does not consider modular code linking to
 * Audacious or using our public API to be a derived work.
 */

/*
 * This is the Interface API.
 *
 * Everything here is like totally subject to change.
 *     --nenolod
 */

#ifndef __AUDACIOUS2_INTERFACE_H__
#define __AUDACIOUS2_INTERFACE_H__

#include <gtk/gtk.h>
#include <mowgli.h>

typedef struct {
    GtkWidget** (*create_prefs_window)(void);
    void (*show_prefs_window)(void);
    void (*hide_prefs_window)(void);
    void (*destroy_prefs_window)(void);
    gint (*prefswin_page_new)(GtkWidget *container, gchar *name, gchar *imgurl);
} InterfaceOps;

typedef struct {
    void (*show_prefs_window)(gboolean show);
    void (*run_filebrowser)(gboolean play_button);
    void (*hide_filebrowser)(void);
    void (*toggle_visibility)(void);
    void (*show_error)(const gchar * markup);
    void (*show_jump_to_track)(void);
    void (*hide_jump_to_track)(void);
    void (*show_about_window)(void);
    void (*hide_about_window)(void);
    void (*toggle_shuffle)(void);
    void (*toggle_repeat)(void);
    GtkWidget *(*run_gtk_plugin)(GtkWidget *parent, const gchar *name);
    GtkWidget *(*stop_gtk_plugin)(GtkWidget *parent);
} InterfaceCbs;

typedef struct _Interface {
    gchar *id;                           /* simple ID like 'skinned' */
    gchar *desc;                         /* description like 'Skinned Interface' */
    gboolean (*init)(InterfaceCbs *cbs); /* init UI */
    gboolean (*fini)(void);              /* shutdown UI */

    InterfaceOps *ops;
} Interface;

void interface_register(Interface *i);
void interface_deregister(Interface *i);
void interface_run(Interface *i);
void interface_destroy(Interface *i);

Interface *interface_get(gchar *id);
const Interface *interface_get_current(void);
void interface_foreach(int (*foreach_cb)(mowgli_dictionary_elem_t *delem, void *privdata), void *privdata);

/* These functions have to be called from main thread
   Use event_queue if you need to call those from other threads */
void interface_show_prefs_window(gboolean show);
void interface_run_filebrowser(gboolean play_button);
void interface_hide_filebrowser(void);
void interface_toggle_visibility(void);
void interface_show_error_message(const gchar * markup);
void interface_show_jump_to_track(void);
void interface_run_gtk_plugin(GtkWidget *parent, const gchar *name);
void interface_stop_gtk_plugin(GtkWidget *parent);
void interface_toggle_shuffle(void);
void interface_toggle_repeat(void);

void register_interface_hooks(void);

#endif
