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

#include <glib.h>
#include <gtk/gtk.h>

#include <libaudcore/hook.h>

#include "audconfig.h"
#include "config.h"
#include "debug.h"
#include "i18n.h"
#include "interface.h"
#include "ui_preferences.h"

static Interface *current_interface = NULL;

static InterfaceOps interface_ops = {
    .create_prefs_window = create_prefs_window,
    .show_prefs_window = show_prefs_window,
    .hide_prefs_window = hide_prefs_window,
    .destroy_prefs_window = destroy_prefs_window,
    .prefswin_page_new = prefswin_page_new,
};

static InterfaceCbs interface_cbs = { NULL };

static gboolean interface_search_cb (PluginHandle * plugin, PluginHandle * *
 pluginp)
{
    * pluginp = plugin;
    return FALSE;
}

PluginHandle * interface_get_default (void)
{
    PluginHandle * plugin;

    if (cfg.iface_path == NULL || (plugin = plugin_by_path (cfg.iface_path,
     PLUGIN_TYPE_IFACE, cfg.iface_number)) == NULL || ! plugin_get_enabled
     (plugin))
    {
        AUDDBG ("Searching for an interface.\n");
        plugin_for_enabled (PLUGIN_TYPE_IFACE, (PluginForEachFunc)
         interface_search_cb, & plugin);
        if (plugin == NULL)
            return NULL;

        interface_set_default (plugin);
    }

    return plugin;
}

void interface_set_default (PluginHandle * plugin)
{
    const gchar * path;
    gint type;

    g_free (cfg.iface_path);
    plugin_get_path (plugin, & path, & type, & cfg.iface_number);
    cfg.iface_path = g_strdup (path);
}

gboolean interface_load (PluginHandle * plugin)
{
    Interface * i = plugin_get_header (plugin);
    g_return_val_if_fail (i != NULL, FALSE);

    current_interface = i;
    i->ops = & interface_ops;
    return i->init (& interface_cbs);
}

void interface_unload (void)
{
    g_return_if_fail (current_interface != NULL);

    if (current_interface->fini != NULL)
        current_interface->fini ();

    current_interface = NULL;
    memset (& interface_cbs, 0, sizeof interface_cbs);
}

void
interface_show_prefs_window(gboolean show)
{
    if (interface_cbs.show_prefs_window != NULL)
        interface_cbs.show_prefs_window(show);
    else
        g_message("Interface didn't register show_prefs_window function");
}

/*
 * gboolean play_button
 *       TRUE  - open files
 *       FALSE - add files
 */
void
interface_run_filebrowser(gboolean play_button)
{
    if (interface_cbs.run_filebrowser != NULL)
        interface_cbs.run_filebrowser(play_button);
    else
        g_message("Interface didn't register run_filebrowser function");
}

void
interface_hide_filebrowser(void)
{
    if (interface_cbs.hide_filebrowser != NULL)
        interface_cbs.hide_filebrowser();
    else
        g_message("Interface didn't register hide_filebrowser function");
}

void
interface_toggle_visibility(void)
{
    if (interface_cbs.toggle_visibility != NULL)
        interface_cbs.toggle_visibility();
    else
        g_message("Interface didn't register toggle_visibility function");
}

void
interface_show_error_message(const gchar * markup)
{
    if (interface_cbs.show_error != NULL)
        interface_cbs.show_error(markup);
    else
        g_message("Interface didn't register show_error function");
}

void
interface_show_jump_to_track(void)
{
    if (interface_cbs.show_jump_to_track != NULL)
        interface_cbs.show_jump_to_track();
    else
        g_message("Interface didn't register show_jump_to_track function");
}

void
interface_hide_jump_to_track(void)
{
    if (interface_cbs.hide_jump_to_track != NULL)
        interface_cbs.hide_jump_to_track();
    else
        g_message("Interface didn't register hide_jump_to_track function");
}

void
interface_show_about_window(gboolean show)
{
    if (show == FALSE) {
        if (interface_cbs.hide_about_window != NULL)
            interface_cbs.hide_about_window();
        else
            g_message("Interface didn't register hide_about_window function");
    } else {
        if (interface_cbs.show_about_window != NULL)
            interface_cbs.show_about_window();
        else
            g_message("Interface didn't register show_about_window function");
    }
}

/* void interface_run_gtk_plugin (GtkWidget * parent, const gchar * name) */
void interface_run_gtk_plugin (void * parent, const gchar * name)
{
    if (interface_cbs.run_gtk_plugin != NULL)
        interface_cbs.run_gtk_plugin(parent, name);
    else {
        GtkWidget *win;

        g_return_if_fail(parent != NULL);
        g_return_if_fail(name != NULL);

        win = gtk_window_new(GTK_WINDOW_TOPLEVEL);
        gtk_window_set_title(GTK_WINDOW(win), _(name));
        gtk_container_add(GTK_CONTAINER(win), parent);
        gtk_widget_show_all(win);

        g_object_set_data(G_OBJECT(parent), "parentwin", win);
    }
}

/* void interface_stop_gtk_plugin (GtkWidget * parent) */
void interface_stop_gtk_plugin (void * parent)
{
    if (interface_cbs.stop_gtk_plugin != NULL)
        interface_cbs.stop_gtk_plugin(parent);
    else {
        GtkWidget *win;

        g_return_if_fail(parent != NULL);

        win = g_object_get_data(G_OBJECT(parent), "parentwin");
        gtk_widget_destroy(win);
    }
}

void
interface_toggle_shuffle(void)
{
    if (interface_cbs.toggle_shuffle != NULL)
        interface_cbs.toggle_shuffle();
    else
        g_message("Interface didn't register toggle_shuffle function");
}

void
interface_toggle_repeat(void)
{
    if (interface_cbs.toggle_repeat != NULL)
        interface_cbs.toggle_repeat();
    else
        g_message("Interface didn't register toggle_repeat function");
}

typedef enum {
    HOOK_PREFSWIN_SHOW,
    HOOK_FILEBROWSER_SHOW,
    HOOK_FILEBROWSER_HIDE,
    HOOK_TOGGLE_VISIBILITY,
    HOOK_SHOW_ERROR,
    HOOK_JUMPTOTRACK_SHOW,
    HOOK_JUMPTOTRACK_HIDE,
    HOOK_ABOUTWIN_SHOW,
    HOOK_TOGGLE_SHUFFLE,
    HOOK_TOGGLE_REPEAT,
} InterfaceHookID;

void
interface_hook_handler(gpointer hook_data, gpointer user_data)
{
    switch (GPOINTER_TO_INT(user_data)) {
        case HOOK_PREFSWIN_SHOW:
            interface_show_prefs_window(GPOINTER_TO_INT(hook_data));
            break;
        case HOOK_FILEBROWSER_SHOW:
            interface_run_filebrowser(GPOINTER_TO_INT(hook_data));
            break;
        case HOOK_FILEBROWSER_HIDE:
            interface_hide_filebrowser();
            break;
        case HOOK_TOGGLE_VISIBILITY:
            interface_toggle_visibility();
            break;
        case HOOK_SHOW_ERROR:
            interface_show_error_message((const gchar *) hook_data);
            break;
        case HOOK_JUMPTOTRACK_SHOW:
            interface_show_jump_to_track();
            break;
        case HOOK_JUMPTOTRACK_HIDE:
            interface_hide_jump_to_track();
            break;
        case HOOK_ABOUTWIN_SHOW:
            interface_show_about_window(GPOINTER_TO_INT(hook_data));
            break;
	case HOOK_TOGGLE_SHUFFLE:
            interface_toggle_shuffle();
            break;
	case HOOK_TOGGLE_REPEAT:
            interface_toggle_repeat();
            break;
        default:
            break;
    }
}

typedef struct {
    const gchar *name;
    InterfaceHookID id;
} InterfaceHooks;

static InterfaceHooks hooks[] = {
    {"prefswin show", HOOK_PREFSWIN_SHOW},
    {"filebrowser show", HOOK_FILEBROWSER_SHOW},
    {"filebrowser hide", HOOK_FILEBROWSER_HIDE},
    {"interface toggle visibility", HOOK_TOGGLE_VISIBILITY},
    {"interface show error", HOOK_SHOW_ERROR},
    {"interface show jump to track", HOOK_JUMPTOTRACK_SHOW},
    {"interface hide jump to track", HOOK_JUMPTOTRACK_HIDE},
    {"aboutwin show", HOOK_ABOUTWIN_SHOW},
    {"toggle shuffle", HOOK_TOGGLE_SHUFFLE},
    {"toggle repeat", HOOK_TOGGLE_REPEAT},
};

void
register_interface_hooks(void)
{
    gint i;
    for (i=0; i<G_N_ELEMENTS(hooks); i++)
        hook_associate(hooks[i].name,
                       (HookFunction) interface_hook_handler,
                       GINT_TO_POINTER(hooks[i].id));

}

