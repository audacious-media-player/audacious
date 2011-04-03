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

typedef struct {
    PluginHeader * header;
    GModule * module;
} LoadedModule;

static GList * loaded_modules = NULL;

static void plugin2_process (PluginHeader * header, GModule * module, const gchar * filename)
{
    if (header->magic != _AUD_PLUGIN_MAGIC)
    {
        fprintf (stderr, " *** ERROR: %s is not a valid Audacious plugin.\n", filename);
        g_module_close (module);
        return;
    }

    if (header->version < _AUD_PLUGIN_VERSION_MIN || header->version > _AUD_PLUGIN_VERSION)
    {
        fprintf (stderr, " *** ERROR: %s is not compatible with this version of Audacious.\n", filename);
        g_module_close (module);
        return;
    }

    LoadedModule * loaded = g_slice_new (LoadedModule);
    loaded->header = header;
    loaded->module = module;
    loaded_modules = g_list_prepend (loaded_modules, loaded);

    if (header->init != NULL)
    {
        plugin_register (PLUGIN_TYPE_LOWLEVEL, filename, 0, NULL);
        header->init ();
    }

    if (header->tp_list)
    {
        TransportPlugin * tp;
        for (gint i = 0; (tp = header->tp_list[i]); i ++)
        {
            plugin_register (PLUGIN_TYPE_TRANSPORT, filename, i, tp);
            if (tp->init != NULL)
                tp->init (); /* FIXME: Pay attention to the return value. */
        }
    }

    if (header->pp_list)
    {
        PlaylistPlugin * pp;
        for (gint i = 0; (pp = header->pp_list[i]); i ++)
        {
            plugin_register (PLUGIN_TYPE_PLAYLIST, filename, i, pp);
            if (pp->init != NULL)
                pp->init (); /* FIXME: Pay attention to the return value. */
        }
    }

    if (header->ip_list != NULL)
    {
        InputPlugin * ip;
        for (gint i = 0; (ip = header->ip_list[i]) != NULL; i ++)
        {
            plugin_register (PLUGIN_TYPE_INPUT, filename, i, ip);
            if (ip->init != NULL)
                ip->init (); /* FIXME: Pay attention to the return value. */
        }
    }

    if (header->ep_list != NULL)
    {
        EffectPlugin * ep;
        for (gint i = 0; (ep = header->ep_list[i]) != NULL; i ++)
        {
            plugin_register (PLUGIN_TYPE_EFFECT, filename, i, ep);
            if (ep->init != NULL)
                ep->init (); /* FIXME: Pay attention to the return value. */
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
            plugin_register (PLUGIN_TYPE_VIS, filename, i, vp);
    }

    if (header->gp_list != NULL)
    {
        GeneralPlugin * gp;
        for (gint i = 0; (gp = header->gp_list[i]) != NULL; i ++)
            plugin_register (PLUGIN_TYPE_GENERAL, filename, i, gp);
    }

    if (header->iface != NULL)
        plugin_register (PLUGIN_TYPE_IFACE, filename, 0, header->iface);
}

static void plugin2_unload (LoadedModule * loaded)
{
    PluginHeader * header = loaded->header;

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

    if (header->pp_list != NULL)
    {
        PlaylistPlugin * pp;
        for (gint i = 0; (pp = header->pp_list[i]) != NULL; i ++)
        {
            if (pp->settings != NULL)
                plugin_preferences_cleanup (pp->settings);
            if (pp->cleanup != NULL)
                pp->cleanup ();
        }
    }

    if (header->tp_list != NULL)
    {
        TransportPlugin * tp;
        for (gint i = 0; (tp = header->tp_list[i]) != NULL; i ++)
        {
            if (tp->settings != NULL)
                plugin_preferences_cleanup (tp->settings);
            if (tp->cleanup != NULL)
                tp->cleanup ();
        }
    }

    if (header->fini != NULL)
        header->fini ();

    g_module_close (loaded->module);
    g_slice_free (LoadedModule, loaded);
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

#ifndef DISABLE_USER_PLUGIN_DIR
    scan_plugins (get_path (AUD_PATH_USER_PLUGIN_DIR));
    /*
     * This is in a separate loop so if the user puts them in the
     * wrong dir we'll still get them in the right order (home dir
     * first)                                                - Zinx
     */
    while (plugin_dir_list[dirsel])
    {
        dir = g_build_filename (get_path (AUD_PATH_USER_PLUGIN_DIR),
         plugin_dir_list[dirsel ++], NULL);
        scan_plugins(dir);
        g_free(dir);
    }
    dirsel = 0;
#endif

    while (plugin_dir_list[dirsel])
    {
        dir = g_build_filename (get_path (AUD_PATH_PLUGIN_DIR),
         plugin_dir_list[dirsel ++], NULL);
        scan_plugins(dir);
        g_free(dir);
    }

    plugin_registry_prune ();
}

void plugin_system_cleanup(void)
{
    plugin_registry_save ();

    for (GList * node = loaded_modules; node != NULL; node = node->next)
        plugin2_unload (node->data);

    g_list_free (loaded_modules);
    loaded_modules = NULL;
}
