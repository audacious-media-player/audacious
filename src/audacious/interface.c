/*
 * Audacious2
 * Copyright (c) 2008 William Pitcock <nenolod@dereferenced.org>
 * Copyright (c) 2008-2009 Tomasz Moń <desowin@gmail.com>
 * Copyright (c) 2010 John Lindgren <john.lindgren@tds.net>
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

#include <string.h>
#include <gtk/gtk.h>

#include <libaudcore/hook.h>

#include "audconfig.h"
#include "config.h"
#include "debug.h"
#include "general.h"
#include "i18n.h"
#include "interface.h"
#include "misc.h"
#include "plugins.h"
#include "ui_preferences.h"
#include "visualization.h"

static Iface *current_interface = NULL;

static IfaceOps interface_ops = {
    .create_prefs_window = create_prefs_window,
    .show_prefs_window = show_prefs_window,
    .hide_prefs_window = hide_prefs_window,
    .destroy_prefs_window = destroy_prefs_window,
    .prefswin_page_new = prefswin_page_new,
};

static IfaceCbs interface_cbs = { NULL };

gboolean interface_load (PluginHandle * plugin)
{
    Iface * i = (Iface *) plugin_get_header (plugin);
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
        AUDDBG ("Interface didn't register show_prefs_window function.\n");
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
        AUDDBG ("Interface didn't register run_filebrowser function.\n");
}

void
interface_hide_filebrowser(void)
{
    if (interface_cbs.hide_filebrowser != NULL)
        interface_cbs.hide_filebrowser();
    else
        AUDDBG ("Interface didn't register hide_filebrowser function.\n");
}

void
interface_toggle_visibility(void)
{
    if (interface_cbs.toggle_visibility != NULL)
        interface_cbs.toggle_visibility();
    else
        AUDDBG ("Interface didn't register toggle_visibility function.\n");
}

void
interface_show_error_message(const gchar * markup)
{
    if (interface_cbs.show_error != NULL)
        interface_cbs.show_error(markup);
    else
        AUDDBG ("Interface didn't register show_error function.\n");
}

void
interface_show_jump_to_track(void)
{
    if (interface_cbs.show_jump_to_track != NULL)
        interface_cbs.show_jump_to_track();
    else
        AUDDBG ("Interface didn't register show_jump_to_track function.\n");
}

void
interface_hide_jump_to_track(void)
{
    if (interface_cbs.hide_jump_to_track != NULL)
        interface_cbs.hide_jump_to_track();
    else
        AUDDBG ("Interface didn't register hide_jump_to_track function.\n");
}

void
interface_show_about_window(gboolean show)
{
    if (show == FALSE) {
        if (interface_cbs.hide_about_window != NULL)
            interface_cbs.hide_about_window();
        else
            AUDDBG ("Interface didn't register hide_about_window function.\n");
    } else {
        if (interface_cbs.show_about_window != NULL)
            interface_cbs.show_about_window();
        else
            AUDDBG ("Interface didn't register show_about_window function.\n");
    }
}

static gboolean delete_cb (GtkWidget * window, GdkEvent * event, PluginHandle *
 plugin)
{
    plugin_enable (plugin, FALSE);
    return TRUE;
}

void interface_add_plugin_widget (PluginHandle * plugin, GtkWidget * widget)
{
    if (interface_cbs.run_gtk_plugin != NULL)
        interface_cbs.run_gtk_plugin (widget, plugin_get_name (plugin));
    else
    {
        GtkWidget * window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
        gtk_window_set_title ((GtkWindow *) window, plugin_get_name (plugin));
        gtk_container_add ((GtkContainer *) window, widget);
        g_signal_connect (window, "delete-event", (GCallback) delete_cb, plugin);
        gtk_widget_show_all (window);
    }
}

void interface_remove_plugin_widget (PluginHandle * plugin, GtkWidget * widget)
{
    if (interface_cbs.stop_gtk_plugin != NULL)
        interface_cbs.stop_gtk_plugin (widget);
    else
        gtk_widget_destroy (gtk_widget_get_parent (widget));
}

void
interface_install_toolbar(void *widget)
{
    if (interface_cbs.install_toolbar != NULL)
        interface_cbs.install_toolbar(widget);
    else
        AUDDBG ("Interface didn't register install_toolbar function.\n");
}

void
interface_uninstall_toolbar(void *widget)
{
    if (interface_cbs.uninstall_toolbar != NULL)
        interface_cbs.uninstall_toolbar(widget);
    else
        AUDDBG ("Interface didn't register uninstall_toolbar function.\n");
}

void
interface_toggle_shuffle(void)
{
    if (interface_cbs.toggle_shuffle != NULL)
        interface_cbs.toggle_shuffle();
    else
        AUDDBG ("Interface didn't register toggle_shuffle function.\n");
}

void
interface_toggle_repeat(void)
{
    if (interface_cbs.toggle_repeat != NULL)
        interface_cbs.toggle_repeat();
    else
        AUDDBG ("Interface didn't register toggle_repeat function.\n");
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
} IfaceHookID;

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
    IfaceHookID id;
} IfaceHooks;

static IfaceHooks hooks[] = {
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

static gboolean probe_cb (PluginHandle * p, PluginHandle * * pp)
{
    * pp = p;
    return FALSE;
}

PluginHandle * iface_plugin_probe (void)
{
    PluginHandle * p = NULL;
    plugin_for_each (PLUGIN_TYPE_IFACE, (PluginForEachFunc) probe_cb, & p);
    return p;
}

static PluginHandle * current_plugin = NULL;

PluginHandle * iface_plugin_get_current (void)
{
    return current_plugin;
}

gboolean iface_plugin_set_current (PluginHandle * plugin)
{
    if (current_plugin != NULL)
    {
        AUDDBG ("Unloading plugin widgets.\n");
        general_cleanup ();

        AUDDBG ("Unloading visualizers.\n");
        vis_cleanup ();

        AUDDBG ("Unloading %s.\n", plugin_get_name (current_plugin));
        interface_unload ();

        current_plugin = NULL;
    }

    if (plugin != NULL)
    {
        AUDDBG ("Loading %s.\n", plugin_get_name (plugin));

        if (! interface_load (plugin))
            return FALSE;

        current_plugin = plugin;

        AUDDBG ("Loading visualizers.\n");
        vis_init ();

        AUDDBG ("Loading plugin widgets.\n");
        general_init ();
    }

    return TRUE;
}
