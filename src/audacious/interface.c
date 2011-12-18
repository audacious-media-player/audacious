/*
 * Audacious2
 * Copyright (c) 2008 William Pitcock <nenolod@dereferenced.org>
 * Copyright (c) 2008-2009 Tomasz Moń <desowin@gmail.com>
 * Copyright (c) 2010-2011 John Lindgren <john.lindgren@tds.net>
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

#include <gtk/gtk.h>
#include <pthread.h>

#include <libaudcore/hook.h>

#include "debug.h"
#include "general.h"
#include "interface.h"
#include "main.h"
#include "misc.h"
#include "plugin.h"
#include "plugins.h"
#include "visualization.h"

static IfacePlugin *current_interface = NULL;

static pthread_mutex_t error_mutex = PTHREAD_MUTEX_INITIALIZER;
static GQueue error_queue = G_QUEUE_INIT;
static int error_source;

bool_t interface_load (PluginHandle * plugin)
{
    IfacePlugin * i = plugin_get_header (plugin);
    g_return_val_if_fail (i, FALSE);

    if (PLUGIN_HAS_FUNC (i, init) && ! i->init ())
        return FALSE;

    current_interface = i;
    return TRUE;
}

void interface_unload (void)
{
    g_return_if_fail (current_interface);

    if (PLUGIN_HAS_FUNC (current_interface, cleanup))
        current_interface->cleanup ();

    current_interface = NULL;
}

void interface_show (bool_t show)
{
    g_return_if_fail (current_interface);

    if (PLUGIN_HAS_FUNC (current_interface, show))
        current_interface->show (show);
}

bool_t interface_is_shown (void)
{
    g_return_val_if_fail (current_interface, FALSE);

    if (PLUGIN_HAS_FUNC (current_interface, is_shown))
        return current_interface->is_shown ();
    return TRUE;
}

bool_t interface_is_focused (void)
{
    g_return_val_if_fail (current_interface, FALSE);

    if (PLUGIN_HAS_FUNC (current_interface, is_focused))
        return current_interface->is_focused ();
    return TRUE;
}

static bool_t error_idle_func (void * unused)
{
    pthread_mutex_lock (& error_mutex);

    char * message;
    while ((message = g_queue_pop_head (& error_queue)))
    {
        pthread_mutex_unlock (& error_mutex);

        if (current_interface && PLUGIN_HAS_FUNC (current_interface, show_error))
            current_interface->show_error (message);
        else
            fprintf (stderr, "ERROR: %s\n", message);

        g_free (message);

        pthread_mutex_lock (& error_mutex);
    }

    error_source = 0;

    pthread_mutex_unlock (& error_mutex);
    return FALSE;
}

void interface_show_error (const char * message)
{
    pthread_mutex_lock (& error_mutex);

    g_queue_push_tail (& error_queue, g_strdup (message));

    if (! error_source)
        error_source = g_idle_add (error_idle_func, NULL);

    pthread_mutex_unlock (& error_mutex);
}

/*
 * bool_t play_button
 *       TRUE  - open files
 *       FALSE - add files
 */
void interface_show_filebrowser (bool_t play_button)
{
    g_return_if_fail (current_interface);

    if (PLUGIN_HAS_FUNC (current_interface, show_filebrowser))
        current_interface->show_filebrowser (play_button);
}

void interface_show_jump_to_track (void)
{
    g_return_if_fail (current_interface);

    if (PLUGIN_HAS_FUNC (current_interface, show_jump_to_track))
        current_interface->show_jump_to_track ();
}

static bool_t delete_cb (GtkWidget * window, GdkEvent * event, PluginHandle *
 plugin)
{
    plugin_enable (plugin, FALSE);
    return TRUE;
}

void interface_add_plugin_widget (PluginHandle * plugin, GtkWidget * widget)
{
    g_return_if_fail (current_interface);

    if (PLUGIN_HAS_FUNC (current_interface, run_gtk_plugin))
        current_interface->run_gtk_plugin (widget, plugin_get_name (plugin));
    else
    {
        GtkWidget * window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
        gtk_window_set_title ((GtkWindow *) window, plugin_get_name (plugin));
        gtk_window_set_default_size ((GtkWindow *) window, 300, 200);
        gtk_container_add ((GtkContainer *) window, widget);
        g_signal_connect (window, "delete-event", (GCallback) delete_cb, plugin);
        gtk_widget_show_all (window);
    }
}

void interface_remove_plugin_widget (PluginHandle * plugin, GtkWidget * widget)
{
    g_return_if_fail (current_interface);

    if (PLUGIN_HAS_FUNC (current_interface, stop_gtk_plugin))
        current_interface->stop_gtk_plugin (widget);
    else
        gtk_widget_destroy (gtk_widget_get_parent (widget));
}

void interface_install_toolbar (void * widget)
{
    g_return_if_fail (current_interface);

    if (PLUGIN_HAS_FUNC (current_interface, install_toolbar))
        current_interface->install_toolbar (widget);
    else
        g_object_ref (widget);
}

void interface_uninstall_toolbar (void * widget)
{
    g_return_if_fail (current_interface);

    if (PLUGIN_HAS_FUNC (current_interface, uninstall_toolbar))
        current_interface->uninstall_toolbar (widget);
    else
        g_object_unref (widget);
}

static bool_t probe_cb (PluginHandle * p, PluginHandle * * pp)
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

bool_t iface_plugin_set_current (PluginHandle * plugin)
{
    hook_call ("config save", NULL); /* tell interface to save layout */

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
