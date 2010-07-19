/*  Audacious - Cross-platform multimedia player
 *  Copyright (C) 2005-2009  Audacious development team
 *
 *  Based on BMP:
 *  Copyright (C) 2003-2004  BMP development team
 *
 *  Based on XMMS:
 *  Copyright (C) 1998-2003  XMMS development team
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

#ifndef SHARED_SUFFIX
# define SHARED_SUFFIX G_MODULE_SUFFIX
#endif

#include <glib.h>
#include <gmodule.h>
#include <gtk/gtk.h>
#include <string.h>

#include <libaudcore/audstrings.h>
#include <libaudgui/init.h>

#include "pluginenum.h"
#include "plugins.h"

#include "audconfig.h"
#include "effect.h"
#include "general.h"
#include "i18n.h"
#include "interface.h"
#include "main.h"
#include "output.h"
#include "playback.h"
#include "util.h"
#include "visualization.h"

#define AUD_API_DECLARE
#include "configdb.h"
#include "drct.h"
#include "misc.h"
#include "playlist.h"
#include "plugins.h"
#undef AUD_API_DECLARE

const gchar *plugin_dir_list[] = {
    PLUGINSUBS,
    NULL
};

static AudAPITable api_table = {
 .configdb_api = & configdb_api,
 .drct_api = & drct_api,
 .misc_api = & misc_api,
 .playlist_api = & playlist_api,
 .plugins_api = & plugins_api,
 .cfg = & cfg};

extern GList *vfs_transports;
static mowgli_list_t *headers_list = NULL;

static void input_plugin_init(Plugin * plugin)
{
    InputPlugin *p = INPUT_PLUGIN(plugin);

    if (p->init != NULL)
        p->init ();
}

static void effect_plugin_init(Plugin * plugin)
{
    EffectPlugin *p = EFFECT_PLUGIN(plugin);

    if (p->init != NULL)
        p->init ();
}

static void vis_plugin_disable_by_header (VisPlugin * header)
{
    vis_plugin_enable (plugin_by_header (header), FALSE);
}

static void vis_plugin_init(Plugin * plugin)
{
    ((VisPlugin *) plugin)->disable_plugin = vis_plugin_disable_by_header;
}

/*******************************************************************/

static void plugin2_dispose(GModule * module, const gchar * str, ...)
{
    gchar *buf;
    va_list va;

    va_start(va, str);
    buf = g_strdup_vprintf(str, va);
    va_end(va);

    g_message("*** %s\n", buf);
    g_free(buf);

    g_module_close(module);
}

void plugin2_process(PluginHeader * header, GModule * module, const gchar * filename)
{
    gint i, n;
    mowgli_node_t *hlist_node;

    if (header->magic != PLUGIN_MAGIC)
    {
        plugin2_dispose (module, "plugin <%s> discarded, invalid module magic",
         filename);
        return;
    }

    if (header->api_version != __AUDACIOUS_PLUGIN_API__)
    {
        plugin2_dispose (module, "plugin <%s> discarded, wanting API version "
         "%d, we implement API version %d", filename, header->api_version,
         __AUDACIOUS_PLUGIN_API__);
        return;
    }

    hlist_node = mowgli_node_create();
    mowgli_node_add(header, hlist_node, headers_list);

    if (header->init)
    {
        plugin_register (filename, PLUGIN_TYPE_BASIC, 0, NULL);
        header->init();
    }

    header->priv_assoc = g_new0(Plugin, 1);
    header->priv_assoc->handle = module;
    header->priv_assoc->filename = g_strdup(filename);

    n = 0;

    if (header->ip_list)
    {
        for (i = 0; (header->ip_list)[i] != NULL; i++, n++)
        {
            plugin_register (filename, PLUGIN_TYPE_INPUT, i, header->ip_list[i]);
            PLUGIN((header->ip_list)[i])->filename = g_strdup_printf("%s (#%d)", filename, n);
            input_plugin_init(PLUGIN((header->ip_list)[i]));
        }
    }

    if (header->op_list)
    {
        for (i = 0; (header->op_list)[i] != NULL; i++, n++)
        {
            OutputPlugin * plugin = header->op_list[i];

            plugin->filename = g_strdup_printf ("%s (#%d)", filename, n);
            plugin_register (filename, PLUGIN_TYPE_OUTPUT, i, plugin);
        }
    }

    if (header->ep_list)
    {
        for (i = 0; (header->ep_list)[i] != NULL; i++, n++)
        {
            plugin_register (filename, PLUGIN_TYPE_EFFECT, i, header->ep_list[i]);
            PLUGIN((header->ep_list)[i])->filename = g_strdup_printf("%s (#%d)", filename, n);
            effect_plugin_init(PLUGIN((header->ep_list)[i]));
        }
    }


    if (header->gp_list)
    {
        for (i = 0; (header->gp_list)[i] != NULL; i++, n++)
        {
            plugin_register (filename, PLUGIN_TYPE_GENERAL, i, header->gp_list[i]);
            PLUGIN((header->gp_list)[i])->filename = g_strdup_printf("%s (#%d)", filename, n);
        }
    }

    if (header->vp_list)
    {
        for (i = 0; (header->vp_list)[i] != NULL; i++, n++)
        {
            plugin_register (filename, PLUGIN_TYPE_VIS, i, header->vp_list[i]);
            PLUGIN((header->vp_list)[i])->filename = g_strdup_printf("%s (#%d)", filename, n);
            vis_plugin_init(PLUGIN((header->vp_list)[i]));
        }
    }

    if (header->interface)
        plugin_register (filename, PLUGIN_TYPE_IFACE, 0, header->interface);
}

void plugin2_unload(PluginHeader * header, mowgli_node_t * hlist_node)
{
    GModule *module;

    g_return_if_fail(header->priv_assoc != NULL);

    if (header->ip_list != NULL)
    {
        for (gint i = 0; header->ip_list[i] != NULL; i ++)
        {
            if (header->ip_list[i]->cleanup != NULL)
                header->ip_list[i]->cleanup ();

            g_free (header->ip_list[i]->filename);
        }
    }

    if (header->op_list != NULL)
    {
        for (gint i = 0; header->op_list[i] != NULL; i ++)
            g_free (header->op_list[i]->filename);
    }

    if (header->ep_list != NULL)
    {
        for (gint i = 0; header->ep_list[i] != NULL; i ++)
        {
            if (header->ep_list[i]->cleanup != NULL)
                header->ep_list[i]->cleanup ();

            g_free (header->ep_list[i]->filename);
        }
    }

    if (header->vp_list != NULL)
    {
        for (gint i = 0; header->vp_list[i] != NULL; i ++)
            g_free (header->vp_list[i]->filename);
    }

    if (header->gp_list != NULL)
    {
        for (gint i = 0; header->gp_list[i] != NULL; i ++)
            g_free (header->gp_list[i]->filename);
    }

    module = header->priv_assoc->handle;

    g_free(header->priv_assoc->filename);
    g_free(header->priv_assoc);

    if (header->fini)
        header->fini();

    mowgli_node_delete(hlist_node, headers_list);
    mowgli_node_free(hlist_node);

    g_module_close(module);
}

/******************************************************************/

void module_load (const gchar * filename)
{
    GModule *module;
    PluginHeader * (* func) (AudAPITable * table);

    g_message("Loaded plugin (%s)", filename);

    if (!(module = g_module_open(filename, G_MODULE_BIND_LAZY | G_MODULE_BIND_LOCAL)))
    {
        printf("Failed to load plugin (%s): %s\n", filename, g_module_error());
        return;
    }

    /* v2 plugin loading */
    if (g_module_symbol (module, "get_plugin_info", (void *) & func))
    {
        PluginHeader * header = func (& api_table);
        g_return_if_fail (header != NULL);
        plugin2_process(header, module, filename);
        return;
    }

    printf("Invalid plugin (%s)\n", filename);
    g_module_close(module);
}

static gboolean scan_plugin_func(const gchar * path, const gchar * basename, gpointer data)
{
    if (!str_has_suffix_nocase(basename, SHARED_SUFFIX))
        return FALSE;

    if (!g_file_test(path, G_FILE_TEST_IS_REGULAR))
        return FALSE;

    module_register (path);

    return FALSE;
}

static void scan_plugins(const gchar * path)
{
    dir_foreach(path, scan_plugin_func, NULL, NULL);
}

static OutputPlugin * output_load_selected (void)
{
    if (cfg.output_path == NULL)
        return NULL;

    PluginHandle * handle = plugin_by_path (cfg.output_path, PLUGIN_TYPE_OUTPUT,
     cfg.output_number);
    if (handle == NULL)
        return NULL;

    OutputPlugin * plugin = plugin_get_header (handle);
    if (plugin == NULL || plugin->init () != OUTPUT_PLUGIN_INIT_FOUND_DEVICES)
        return NULL;

    return plugin;
}

static gboolean output_probe_func (PluginHandle * handle, OutputPlugin * * result)
{
    g_message ("Probing output plugin %s", plugin_get_name (handle));
    OutputPlugin * plugin = plugin_get_header (handle);

    if (plugin == NULL || plugin->init == NULL || plugin->init () !=
     OUTPUT_PLUGIN_INIT_FOUND_DEVICES)
        return TRUE;

    * result = plugin;
    return FALSE;
}

static OutputPlugin * output_probe (void)
{
    OutputPlugin * plugin = NULL;
    plugin_for_each (PLUGIN_TYPE_OUTPUT, (PluginForEachFunc) output_probe_func,
     & plugin);

    if (plugin == NULL)
        fprintf (stderr, "ALL OUTPUT PLUGINS FAILED TO INITIALIZE.\n");

    return plugin;
}

void plugin_system_init(void)
{
    gchar *dir;
    GtkWidget *dialog;
    gint dirsel = 0;

    audgui_init (& api_table);

    if (!g_module_supported())
    {
        dialog = gtk_message_dialog_new(NULL, GTK_DIALOG_DESTROY_WITH_PARENT, GTK_MESSAGE_ERROR, GTK_BUTTONS_CLOSE, _("Module loading not supported! Plugins will not be loaded.\n"));
        gtk_dialog_run(GTK_DIALOG(dialog));
        gtk_widget_destroy(dialog);
        return;
    }

    plugin_registry_load ();

    headers_list = mowgli_list_create();

#ifndef DISABLE_USER_PLUGIN_DIR
    scan_plugins(aud_paths[BMP_PATH_USER_PLUGIN_DIR]);
    /*
     * This is in a separate loop so if the user puts them in the
     * wrong dir we'll still get them in the right order (home dir
     * first)                                                - Zinx
     */
    while (plugin_dir_list[dirsel])
    {
        dir = g_build_filename(aud_paths[BMP_PATH_USER_PLUGIN_DIR], plugin_dir_list[dirsel++], NULL);
        scan_plugins(dir);
        g_free(dir);
    }
    dirsel = 0;
#endif

    while (plugin_dir_list[dirsel])
    {
        dir = g_build_filename(PLUGIN_DIR, plugin_dir_list[dirsel++], NULL);
        scan_plugins(dir);
        g_free(dir);
    }

    plugin_registry_prune ();

    current_output_plugin = output_load_selected ();

    if (current_output_plugin == NULL)
        current_output_plugin = output_probe ();

    general_init ();
}

void plugin_system_cleanup(void)
{
    mowgli_node_t *hlist_node;

    g_message("Shutting down plugin system");

    if (current_output_plugin != NULL)
    {
        if (current_output_plugin->cleanup != NULL)
            current_output_plugin->cleanup ();

        current_output_plugin = NULL;
    }

    general_cleanup ();

    plugin_registry_save ();

    /* XXX: vfs will crash otherwise. -nenolod */
    if (vfs_transports != NULL)
    {
        g_list_free(vfs_transports);
        vfs_transports = NULL;
    }

    MOWGLI_LIST_FOREACH(hlist_node, headers_list->head) plugin2_unload(hlist_node->data, hlist_node);

    mowgli_list_free(headers_list);
}
