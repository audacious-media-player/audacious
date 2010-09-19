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

#include <assert.h>

#include <glib.h>
#include <gmodule.h>

#include <libaudcore/audstrings.h>
#include <libaudgui/init.h>

#include "config.h"

#ifndef SHARED_SUFFIX
# define SHARED_SUFFIX G_MODULE_SUFFIX
#endif

#include "audconfig.h"
#include "debug.h"
#include "main.h"
#include "plugin.h"
#include "ui_preferences.h"
#include "util.h"

#define AUD_API_DECLARE
#include "configdb.h"
#include "drct.h"
#include "misc.h"
#include "playlist.h"
#include "plugins.h"
#undef AUD_API_DECLARE

static const gchar * plugin_dir_list[] = {PLUGINSUBS, NULL};

static AudAPITable api_table = {
 .configdb_api = & configdb_api,
 .drct_api = & drct_api,
 .misc_api = & misc_api,
 .playlist_api = & playlist_api,
 .plugins_api = & plugins_api,
 .cfg = & cfg};

extern GList *vfs_transports;
static mowgli_list_t *headers_list = NULL;

/*******************************************************************/

static void plugin2_dispose(GModule * module, const gchar * str, ...)
{
    gchar *buf;
    va_list va;

    va_start(va, str);
    buf = g_strdup_vprintf(str, va);
    va_end(va);

    AUDDBG ("*** %s\n", buf);
    g_free(buf);

    g_module_close(module);
}

static void vis_plugin_disable_by_header (VisPlugin * header)
{
    plugin_enable (plugin_by_header (header), FALSE);
}

void plugin2_process(PluginHeader * header, GModule * module, const gchar * filename)
{
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

    header->priv = module;

    if (header->init != NULL)
    {
        plugin_register (PLUGIN_TYPE_LOWLEVEL, filename, 0, NULL);
        header->init ();
    }

    if (header->ip_list != NULL)
    {
        InputPlugin * ip;
        for (gint i = 0; (ip = header->ip_list[i]) != NULL; i ++)
        {
            plugin_register (PLUGIN_TYPE_INPUT, filename, i, ip);

            if (ip->init != NULL)
                ip->init ();
        }
    }

    if (header->ep_list != NULL)
    {
        EffectPlugin * ep;
        for (gint i = 0; (ep = header->ep_list[i]) != NULL; i ++)
        {
            plugin_register (PLUGIN_TYPE_EFFECT, filename, i, ep);

            if (ep->init != NULL)
                ep->init ();
        }
    }

    if (header->op_list != NULL)
    {
        OutputPlugin * op;
        for (gint i = 0; (op = header->op_list[i]) != NULL; i ++)
            plugin_register (PLUGIN_TYPE_OUTPUT, filename, i, op);
    }

    if (header->vp_list != NULL)
    {
        VisPlugin * vp;
        for (gint i = 0; (vp = header->vp_list[i]) != NULL; i ++)
        {
            vp->disable_plugin = vis_plugin_disable_by_header;
            plugin_register (PLUGIN_TYPE_VIS, filename, i, vp);
        }
    }

    if (header->gp_list != NULL)
    {
        GeneralPlugin * gp;
        for (gint i = 0; (gp = header->gp_list[i]) != NULL; i ++)
            plugin_register (PLUGIN_TYPE_GENERAL, filename, i, gp);
    }

    if (header->interface != NULL)
        plugin_register (PLUGIN_TYPE_IFACE, filename, 0, header->interface);
}

void plugin2_unload(PluginHeader * header, mowgli_node_t * hlist_node)
{
    if (header->ip_list != NULL)
    {
        InputPlugin * ip;
        for (gint i = 0; (ip = header->ip_list[i]) != NULL; i ++)
        {
            if (ip->settings != NULL)
                plugin_preferences_cleanup (ip->settings);
            if (ip->cleanup != NULL)
                ip->cleanup ();
        }
    }

    if (header->ep_list != NULL)
    {
        EffectPlugin * ep;
        for (gint i = 0; (ep = header->ep_list[i]) != NULL; i ++)
        {
            if (ep->settings != NULL)
                plugin_preferences_cleanup (ep->settings);
            if (ep->cleanup != NULL)
                ep->cleanup ();
        }
    }

    if (header->fini != NULL)
        header->fini ();

    mowgli_node_delete(hlist_node, headers_list);
    mowgli_node_free(hlist_node);

    g_module_close (header->priv);
}

/******************************************************************/

void module_load (const gchar * filename)
{
    GModule *module;
    PluginHeader * (* func) (AudAPITable * table);

    AUDDBG ("Loading plugin: %s.\n", filename);

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

void plugin_system_init(void)
{
    assert (g_module_supported ());

    gchar *dir;
    gint dirsel = 0;

    audgui_init (& api_table);

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
}

void plugin_system_cleanup(void)
{
    mowgli_node_t *hlist_node;

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
