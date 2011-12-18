/*  Audacious - Cross-platform multimedia player
 *  Copyright (C) 2005-2011  Audacious development team
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
#include <pthread.h>

#include <libaudcore/audstrings.h>
#include <libaudgui/init.h>

#include "config.h"

#ifndef SHARED_SUFFIX
# define SHARED_SUFFIX G_MODULE_SUFFIX
#endif

#include "debug.h"
#include "plugin.h"
#include "ui_preferences.h"
#include "util.h"

#define AUD_API_DECLARE
#include "drct.h"
#include "misc.h"
#include "playlist.h"
#include "plugins.h"
#undef AUD_API_DECLARE

static const char * plugin_dir_list[] = {PLUGINSUBS, NULL};

char verbose = 0;

AudAPITable api_table = {
 .drct_api = & drct_api,
 .misc_api = & misc_api,
 .playlist_api = & playlist_api,
 .plugins_api = & plugins_api,
 .verbose = & verbose};

typedef struct {
    Plugin * header;
    GModule * module;
} LoadedModule;

static GList * loaded_modules = NULL;
static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

static void plugin2_process (Plugin * header, GModule * module, const char * filename)
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

    switch (header->type)
    {
    case PLUGIN_TYPE_TRANSPORT:
    case PLUGIN_TYPE_PLAYLIST:
    case PLUGIN_TYPE_INPUT:
    case PLUGIN_TYPE_EFFECT:
        if (PLUGIN_HAS_FUNC (header, init) && ! header->init ())
        {
            fprintf (stderr, " *** ERROR: %s failed to initialize.\n", filename);
            g_module_close (module);
            return;
        }
        break;
    }

    pthread_mutex_lock (& mutex);
    LoadedModule * loaded = g_slice_new (LoadedModule);
    loaded->header = header;
    loaded->module = module;
    loaded_modules = g_list_prepend (loaded_modules, loaded);
    pthread_mutex_unlock (& mutex);

    plugin_register_loaded (filename, header);
}

static void plugin2_unload (LoadedModule * loaded)
{
    Plugin * header = loaded->header;

    switch (header->type)
    {
    case PLUGIN_TYPE_TRANSPORT:
    case PLUGIN_TYPE_PLAYLIST:
    case PLUGIN_TYPE_INPUT:
    case PLUGIN_TYPE_EFFECT:
        if (PLUGIN_HAS_FUNC (header, settings))
            plugin_preferences_cleanup (header->settings);
        if (PLUGIN_HAS_FUNC (header, cleanup))
            header->cleanup ();
        break;
    }

    pthread_mutex_lock (& mutex);
    g_module_close (loaded->module);
    g_slice_free (LoadedModule, loaded);
    pthread_mutex_unlock (& mutex);
}

/******************************************************************/

void plugin_load (const char * filename)
{
    GModule *module;
    Plugin * (* func) (AudAPITable * table);

    AUDDBG ("Loading plugin: %s.\n", filename);

    if (!(module = g_module_open(filename, G_MODULE_BIND_LAZY | G_MODULE_BIND_LOCAL)))
    {
        printf("Failed to load plugin (%s): %s\n", filename, g_module_error());
        return;
    }

    /* v2 plugin loading */
    if (g_module_symbol (module, "get_plugin_info", (void *) & func))
    {
        Plugin * header = func (& api_table);
        g_return_if_fail (header != NULL);
        plugin2_process(header, module, filename);
        return;
    }

    printf("Invalid plugin (%s)\n", filename);
    g_module_close(module);
}

static bool_t scan_plugin_func(const char * path, const char * basename, gpointer data)
{
    if (!str_has_suffix_nocase(basename, SHARED_SUFFIX))
        return FALSE;

    if (!g_file_test(path, G_FILE_TEST_IS_REGULAR))
        return FALSE;

    plugin_register (path);

    return FALSE;
}

static void scan_plugins(const char * path)
{
    dir_foreach (path, scan_plugin_func, NULL);
}

void plugin_system_init(void)
{
    assert (g_module_supported ());

    char *dir;
    int dirsel = 0;

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

    audgui_cleanup ();
}
