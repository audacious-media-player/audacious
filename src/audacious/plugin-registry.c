/*
 * plugin-registry.c
 * Copyright 2009-2010 John Lindgren
 *
 * This file is part of Audacious.
 *
 * Audacious is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free Software
 * Foundation, version 2 or version 3 of the License.
 *
 * Audacious is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
 * A PARTICULAR PURPOSE. See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * Audacious. If not, see <http://www.gnu.org/licenses/>.
 *
 * The Audacious team does not consider modular code linking to Audacious or
 * using our public API to be a derived work.
 */

#include <glib.h>
#include <limits.h>
#include <stdio.h>
#include <string.h>

#include <libaudcore/audstrings.h>

#include "debug.h"
#include "interface.h"
#include "misc.h"
#include "plugin.h"
#include "plugins.h"
#include "util.h"

#define FILENAME "plugin-registry"
#define FORMAT 5

typedef struct {
    gchar * path;
    gboolean confirmed;
    gint timestamp;
    gboolean loaded;
    GList * plugin_list;
} ModuleData;

typedef struct {
    GList * schemes;
} TransportPluginData;

typedef struct {
    GList * exts;
} PlaylistPluginData;

typedef struct {
    GList * keys[INPUT_KEYS];
    gboolean has_images, has_subtunes, can_write_tuple, has_infowin;
} InputPluginData;

struct PluginHandle {
    ModuleData * module;
    gint type, number;
    gboolean confirmed;
    const void * header;
    gchar * name;
    gint priority;
    gboolean has_about, has_configure, enabled;
    GList * watches;

    union {
        TransportPluginData t;
        PlaylistPluginData p;
        InputPluginData i;
    } u;
};

typedef struct {
    PluginForEachFunc func;
    void * data;
} PluginWatch;

static const gchar * plugin_type_names[] = {
 [PLUGIN_TYPE_LOWLEVEL] = NULL,
 [PLUGIN_TYPE_TRANSPORT] = "transport",
 [PLUGIN_TYPE_PLAYLIST] = "playlist",
 [PLUGIN_TYPE_INPUT] = "input",
 [PLUGIN_TYPE_EFFECT] = "effect",
 [PLUGIN_TYPE_OUTPUT] = "output",
 [PLUGIN_TYPE_VIS] = "vis",
 [PLUGIN_TYPE_GENERAL] = "general",
 [PLUGIN_TYPE_IFACE] = "iface"};
static const gchar * input_key_names[] = {
 [INPUT_KEY_SCHEME] = "scheme",
 [INPUT_KEY_EXTENSION] = "ext",
 [INPUT_KEY_MIME] = "mime"};
static GList * module_list = NULL;
static GList * plugin_list = NULL;
static gboolean registry_locked = TRUE;

static ModuleData * module_new (gchar * path, gboolean confirmed, gint
 timestamp, gboolean loaded)
{
    ModuleData * module = g_malloc (sizeof (ModuleData));

    module->path = path;
    module->confirmed = confirmed;
    module->timestamp = timestamp;
    module->loaded = loaded;
    module->plugin_list = NULL;

    module_list = g_list_prepend (module_list, module);

    return module;
}

static PluginHandle * plugin_new (ModuleData * module, gint type, gint number,
 gboolean confirmed, const void * header)
{
    PluginHandle * plugin = g_malloc (sizeof (PluginHandle));

    plugin->module = module;
    plugin->type = type;
    plugin->number = number;
    plugin->confirmed = confirmed;
    plugin->header = header;
    plugin->name = NULL;
    plugin->priority = 0;
    plugin->has_about = FALSE;
    plugin->has_configure = FALSE;
    plugin->enabled = FALSE;
    plugin->watches = NULL;

    if (type == PLUGIN_TYPE_TRANSPORT)
    {
        plugin->enabled = TRUE;
        plugin->u.t.schemes = NULL;
    }
    else if (type == PLUGIN_TYPE_PLAYLIST)
    {
        plugin->enabled = TRUE;
        plugin->u.p.exts = NULL;
    }
    else if (type == PLUGIN_TYPE_INPUT)
    {
        plugin->enabled = TRUE;
        memset (plugin->u.i.keys, 0, sizeof plugin->u.i.keys);
        plugin->u.i.has_images = FALSE;
        plugin->u.i.has_subtunes = FALSE;
        plugin->u.i.can_write_tuple = FALSE;
        plugin->u.i.has_infowin = FALSE;
    }

    plugin_list = g_list_prepend (plugin_list, plugin);
    module->plugin_list = g_list_prepend (module->plugin_list, plugin);

    return plugin;
}

static void plugin_free (PluginHandle * plugin, ModuleData * module)
{
    plugin_list = g_list_remove (plugin_list, plugin);
    module->plugin_list = g_list_remove (module->plugin_list, plugin);

    g_list_foreach (plugin->watches, (GFunc) g_free, NULL);
    g_list_free (plugin->watches);

    if (plugin->type == PLUGIN_TYPE_TRANSPORT)
    {
        g_list_foreach (plugin->u.t.schemes, (GFunc) g_free, NULL);
        g_list_free (plugin->u.t.schemes);
    }
    else if (plugin->type == PLUGIN_TYPE_PLAYLIST)
    {
        g_list_foreach (plugin->u.p.exts, (GFunc) g_free, NULL);
        g_list_free (plugin->u.p.exts);
    }
    else if (plugin->type == PLUGIN_TYPE_INPUT)
    {
        for (gint key = 0; key < INPUT_KEYS; key ++)
        {
            g_list_foreach (plugin->u.i.keys[key], (GFunc) g_free, NULL);
            g_list_free (plugin->u.i.keys[key]);
        }
    }

    g_free (plugin->name);
    g_free (plugin);
}

static void module_free (ModuleData * module)
{
    module_list = g_list_remove (module_list, module);

    g_list_foreach (module->plugin_list, (GFunc) plugin_free, module);

    g_free (module->path);
    g_free (module);
}

static FILE * open_registry_file (const gchar * mode)
{
    gchar path[PATH_MAX];
    snprintf (path, sizeof path, "%s/" FILENAME, get_path (AUD_PATH_USER_DIR));
    return fopen (path, mode);
}

static void transport_plugin_save (PluginHandle * plugin, FILE * handle)
{
    for (GList * node = plugin->u.t.schemes; node; node = node->next)
        fprintf (handle, "scheme %s\n", (const gchar *) node->data);
}

static void playlist_plugin_save (PluginHandle * plugin, FILE * handle)
{
    for (GList * node = plugin->u.p.exts; node; node = node->next)
        fprintf (handle, "ext %s\n", (const gchar *) node->data);
}

static void input_plugin_save (PluginHandle * plugin, FILE * handle)
{
    for (gint key = 0; key < INPUT_KEYS; key ++)
    {
        for (GList * node = plugin->u.i.keys[key]; node != NULL; node =
         node->next)
            fprintf (handle, "%s %s\n", input_key_names[key], (const gchar *)
             node->data);
    }

    fprintf (handle, "images %d\n", plugin->u.i.has_images);
    fprintf (handle, "subtunes %d\n", plugin->u.i.has_subtunes);
    fprintf (handle, "writes %d\n", plugin->u.i.can_write_tuple);
    fprintf (handle, "infowin %d\n", plugin->u.i.has_infowin);
}

static void plugin_save (PluginHandle * plugin, FILE * handle)
{
    fprintf (handle, "%s %d\n", plugin_type_names[plugin->type], plugin->number);
    fprintf (handle, "name %s\n", plugin->name);
    fprintf (handle, "priority %d\n", plugin->priority);
    fprintf (handle, "about %d\n", plugin->has_about);
    fprintf (handle, "config %d\n", plugin->has_configure);
    fprintf (handle, "enabled %d\n", plugin->enabled);

    if (plugin->type == PLUGIN_TYPE_TRANSPORT)
        transport_plugin_save (plugin, handle);
    else if (plugin->type == PLUGIN_TYPE_PLAYLIST)
        playlist_plugin_save (plugin, handle);
    else if (plugin->type == PLUGIN_TYPE_INPUT)
        input_plugin_save (plugin, handle);
}

static gint plugin_not_handled_cb (PluginHandle * plugin)
{
    return (plugin_type_names[plugin->type] == NULL) ? 0 : -1;
}

static void module_save (ModuleData * module, FILE * handle)
{
    /* If the module contains any plugins that we do not handle, we do not save
     * it, thereby forcing it to be loaded on next startup.  If the module
     * appears to contain no plugins at all, we can assume that it failed to
     * load, in which case we also want to try to load it again. */
    if (! module->plugin_list || g_list_find_custom (module->plugin_list, NULL,
     (GCompareFunc) plugin_not_handled_cb) != NULL)
        return;

    fprintf (handle, "module %s\n", module->path);
    fprintf (handle, "stamp %d\n", module->timestamp);

    g_list_foreach (module->plugin_list, (GFunc) plugin_save, handle);
}

void plugin_registry_save (void)
{
    FILE * handle = open_registry_file ("w");
    g_return_if_fail (handle != NULL);

    fprintf (handle, "format %d\n", FORMAT);

    g_list_foreach (module_list, (GFunc) module_save, handle);
    fclose (handle);

    g_list_foreach (module_list, (GFunc) module_free, NULL);
    registry_locked = TRUE;
}

static gchar parse_key[512];
static gchar * parse_value;

static void parse_next (FILE * handle)
{
    parse_value = NULL;

    if (fgets (parse_key, sizeof parse_key, handle) == NULL)
        return;

    gchar * space = strchr (parse_key, ' ');
    if (space == NULL)
        return;

    * space = 0;
    parse_value = space + 1;

    gchar * newline = strchr (parse_value, '\n');
    if (newline != NULL)
        * newline = 0;
}

static gboolean parse_integer (const gchar * key, gint * value)
{
    return (parse_value != NULL && ! strcmp (parse_key, key) && sscanf
     (parse_value, "%d", value) == 1);
}

static gchar * parse_string (const gchar * key)
{
    return (parse_value != NULL && ! strcmp (parse_key, key)) ? g_strdup
     (parse_value) : NULL;
}

static void transport_plugin_parse (PluginHandle * plugin, FILE * handle)
{
    gchar * value;
    while ((value = parse_string ("scheme")))
    {
        plugin->u.t.schemes = g_list_prepend (plugin->u.t.schemes, value);
        parse_next (handle);
    }
}

static void playlist_plugin_parse (PluginHandle * plugin, FILE * handle)
{
    gchar * value;
    while ((value = parse_string ("ext")))
    {
        plugin->u.p.exts = g_list_prepend (plugin->u.p.exts, value);
        parse_next (handle);
    }
}

static void input_plugin_parse (PluginHandle * plugin, FILE * handle)
{
    for (gint key = 0; key < INPUT_KEYS; key ++)
    {
        gchar * value;
        while ((value = parse_string (input_key_names[key])) != NULL)
        {
            plugin->u.i.keys[key] = g_list_prepend (plugin->u.i.keys[key],
             value);
            parse_next (handle);
        }
    }

    if (parse_integer ("images", & plugin->u.i.has_images))
        parse_next (handle);
    if (parse_integer ("subtunes", & plugin->u.i.has_subtunes))
        parse_next (handle);
    if (parse_integer ("writes", & plugin->u.i.can_write_tuple))
        parse_next (handle);
    if (parse_integer ("infowin", & plugin->u.i.has_infowin))
        parse_next (handle);
}

static gboolean plugin_parse (ModuleData * module, FILE * handle)
{
    gint type, number;
    for (type = 0; type < PLUGIN_TYPES; type ++)
    {
        if (plugin_type_names[type] != NULL && parse_integer
         (plugin_type_names[type], & number))
            goto FOUND;
    }

    return FALSE;

FOUND:;
    PluginHandle * plugin = plugin_new (module, type, number, FALSE, NULL);
    parse_next (handle);

    if ((plugin->name = parse_string ("name")) != NULL)
        parse_next (handle);
    if (parse_integer ("priority", & plugin->priority))
        parse_next (handle);
    if (parse_integer ("about", & plugin->has_about))
        parse_next (handle);
    if (parse_integer ("config", & plugin->has_configure))
        parse_next (handle);
    if (parse_integer ("enabled", & plugin->enabled))
        parse_next (handle);

    if (type == PLUGIN_TYPE_TRANSPORT)
        transport_plugin_parse (plugin, handle);
    else if (type == PLUGIN_TYPE_PLAYLIST)
        playlist_plugin_parse (plugin, handle);
    else if (type == PLUGIN_TYPE_INPUT)
        input_plugin_parse (plugin, handle);

    return TRUE;
}

static gboolean module_parse (FILE * handle)
{
    gchar * path = parse_string ("module");
    if (path == NULL)
        return FALSE;

    parse_next (handle);

    gint timestamp;
    if (! parse_integer ("stamp", & timestamp))
    {
        g_free (path);
        return FALSE;
    }

    ModuleData * module = module_new (path, FALSE, timestamp, FALSE);
    parse_next (handle);

    while (plugin_parse (module, handle))
        ;

    return TRUE;
}

void plugin_registry_load (void)
{
    FILE * handle = open_registry_file ("r");
    if (handle == NULL)
        goto UNLOCK;

    parse_next (handle);

    gint format;
    if (! parse_integer ("format", & format) || format != FORMAT)
        goto ERR;

    parse_next (handle);

    while (module_parse (handle))
        ;

ERR:
    fclose (handle);
UNLOCK:
    registry_locked = FALSE;
}

static void plugin_prune (PluginHandle * plugin, ModuleData * module)
{
    if (plugin->confirmed)
        return;

    AUDDBG ("Plugin not found: %s %d:%d\n", plugin->module->path, plugin->type,
     plugin->number);
    plugin_free (plugin, module);
}

static void module_prune (ModuleData * module)
{
    if (! module->confirmed)
    {
        AUDDBG ("Module not found: %s\n", module->path);
        module_free (module);
        return;
    }

    if (module->loaded)
        g_list_foreach (module->plugin_list, (GFunc) plugin_prune, module);
}

gint plugin_compare (PluginHandle * a, PluginHandle * b)
{
    if (a->type < b->type)
        return -1;
    if (a->type > b->type)
        return 1;
    if (a->priority < b->priority)
        return -1;
    if (a->priority > b->priority)
        return 1;

    gint diff;
    if ((diff = string_compare (a->name, b->name)))
        return diff;
    if ((diff = string_compare (a->module->path, b->module->path)))
        return diff;

    if (a->number < b->number)
        return -1;
    if (a->number > b->number)
        return 1;

    return 0;
}

void plugin_registry_prune (void)
{
    g_list_foreach (module_list, (GFunc) module_prune, NULL);
    plugin_list = g_list_sort (plugin_list, (GCompareFunc) plugin_compare);
    registry_locked = TRUE;
}

static gint module_lookup_cb (ModuleData * module, const gchar * path)
{
    return strcmp (module->path, path);
}

static ModuleData * module_lookup (const gchar * path)
{
    GList * node = g_list_find_custom (module_list, path, (GCompareFunc)
     module_lookup_cb);
    return (node != NULL) ? node->data : NULL;
}

void module_register (const gchar * path)
{
    gint timestamp = file_get_mtime (path);
    g_return_if_fail (timestamp >= 0);

    ModuleData * module = module_lookup (path);
    if (module == NULL)
    {
        AUDDBG ("New module: %s\n", path);
        g_return_if_fail (! registry_locked);
        module = module_new (g_strdup (path), TRUE, timestamp, TRUE);
        module_load (path);
        module->loaded = TRUE;
        return;
    }

    AUDDBG ("Register module: %s\n", path);
    module->confirmed = TRUE;
    if (module->timestamp == timestamp)
        return;

    AUDDBG ("Rescan module: %s\n", path);
    module->timestamp = timestamp;
    module_load (path);
    module->loaded = TRUE;
}

typedef struct {
    gint type, number;
} PluginLookupState;

static gint plugin_lookup_cb (PluginHandle * plugin, PluginLookupState * state)
{
    return (plugin->type == state->type && plugin->number == state->number) ? 0
     : -1;
}

static PluginHandle * plugin_lookup_real (ModuleData * module, gint type, gint
 number)
{
    PluginLookupState state = {type, number};
    GList * node = g_list_find_custom (module->plugin_list, & state,
     (GCompareFunc) plugin_lookup_cb);
    return (node != NULL) ? node->data : NULL;
}

void plugin_register (gint type, const gchar * path, gint number, const void *
 header)
{
    ModuleData * module = module_lookup (path);
    g_return_if_fail (module != NULL);

    PluginHandle * plugin = plugin_lookup_real (module, type, number);
    if (plugin == NULL)
    {
        AUDDBG ("New plugin: %s %d:%d\n", path, type, number);
        g_return_if_fail (! registry_locked);
        plugin = plugin_new (module, type, number, TRUE, header);
    }

    AUDDBG ("Register plugin: %s %d:%d\n", path, type, number);
    plugin->confirmed = TRUE;
    plugin->header = header;

    if (type != PLUGIN_TYPE_LOWLEVEL && type != PLUGIN_TYPE_IFACE)
    {
        Plugin * gp = header;
        g_free (plugin->name);
        plugin->name = g_strdup (gp->description);
        plugin->has_about = (gp->about != NULL);
        plugin->has_configure = (gp->configure != NULL || gp->settings != NULL);
    }

    if (type == PLUGIN_TYPE_TRANSPORT)
    {
        TransportPlugin * tp = header;
        for (gint i = 0; tp->schemes[i]; i ++)
            plugin->u.t.schemes = g_list_prepend (plugin->u.t.schemes, g_strdup
             (tp->schemes[i]));
    }
    else if (type == PLUGIN_TYPE_PLAYLIST)
    {
        PlaylistPlugin * pp = header;
        for (gint i = 0; pp->extensions[i]; i ++)
            plugin->u.p.exts = g_list_prepend (plugin->u.p.exts, g_strdup
             (pp->extensions[i]));
    }
    else if (type == PLUGIN_TYPE_INPUT)
    {
        InputPlugin * ip = header;
        plugin->priority = ip->priority;

        for (gint key = 0; key < INPUT_KEYS; key ++)
        {
            g_list_foreach (plugin->u.i.keys[key], (GFunc) g_free, NULL);
            g_list_free (plugin->u.i.keys[key]);
            plugin->u.i.keys[key] = NULL;
        }

        if (ip->vfs_extensions != NULL)
        {
            for (gint i = 0; ip->vfs_extensions[i] != NULL; i ++)
                plugin->u.i.keys[INPUT_KEY_EXTENSION] = g_list_prepend
                 (plugin->u.i.keys[INPUT_KEY_EXTENSION], g_strdup
                 (ip->vfs_extensions[i]));
        }

        plugin->u.i.has_images = (ip->get_song_image != NULL);
        plugin->u.i.has_subtunes = ip->have_subtune;
        plugin->u.i.can_write_tuple = (ip->update_song_tuple != NULL);
        plugin->u.i.has_infowin = (ip->file_info_box != NULL);
    }
    else if (type == PLUGIN_TYPE_OUTPUT)
    {
        OutputPlugin * op = header;
        plugin->priority = 10 - op->probe_priority;
    }
    else if (type == PLUGIN_TYPE_EFFECT)
    {
        EffectPlugin * ep = header;
        plugin->priority = ep->order;
    }
    else if (type == PLUGIN_TYPE_IFACE)
    {
        Iface * i = (void *) header;
        g_free (plugin->name);
        plugin->name = g_strdup (i->desc);
    }
}

gint plugin_get_type (PluginHandle * plugin)
{
    return plugin->type;
}

const gchar * plugin_get_filename (PluginHandle * plugin)
{
    return plugin->module->path;
}

gint plugin_get_number (PluginHandle * plugin)
{
    return plugin->number;
}

PluginHandle * plugin_lookup (gint type, const gchar * path, gint number)
{
    ModuleData * module = module_lookup (path);
    if (module == NULL)
        return NULL;

    return plugin_lookup_real (module, type, number);
}

const void * plugin_get_header (PluginHandle * plugin)
{
    if (! plugin->module->loaded)
    {
        module_load (plugin->module->path);
        plugin->module->loaded = TRUE;
    }

    return plugin->header;
}

static gint plugin_by_header_cb (PluginHandle * plugin, const void * header)
{
    return (plugin->header == header) ? 0 : -1;
}

PluginHandle * plugin_by_header (const void * header)
{
    GList * node = g_list_find_custom (plugin_list, header, (GCompareFunc)
     plugin_by_header_cb);
    return (node != NULL) ? node->data : NULL;
}

void plugin_for_each (gint type, PluginForEachFunc func, void * data)
{
    for (GList * node = plugin_list; node != NULL; node = node->next)
    {
        if (((PluginHandle *) node->data)->type != type)
            continue;
        if (! func (node->data, data))
            break;
    }
}

const gchar * plugin_get_name (PluginHandle * plugin)
{
    return plugin->name;
}

gboolean plugin_has_about (PluginHandle * plugin)
{
    return plugin->has_about;
}

gboolean plugin_has_configure (PluginHandle * plugin)
{
    return plugin->has_configure;
}

gboolean plugin_get_enabled (PluginHandle * plugin)
{
    return plugin->enabled;
}

static void plugin_call_watches (PluginHandle * plugin)
{
    for (GList * node = plugin->watches; node != NULL; )
    {
        GList * next = node->next;
        PluginWatch * watch = node->data;

        if (! watch->func (plugin, watch->data))
        {
            g_free (watch);
            plugin->watches = g_list_delete_link (plugin->watches, node);
        }

        node = next;
    }
}

void plugin_set_enabled (PluginHandle * plugin, gboolean enabled)
{
    plugin->enabled = enabled;
    plugin_call_watches (plugin);
}

typedef struct {
    PluginForEachFunc func;
    void * data;
} PluginForEnabledState;

static gboolean plugin_for_enabled_cb (PluginHandle * plugin,
 PluginForEnabledState * state)
{
    if (! plugin->enabled)
        return TRUE;
    return state->func (plugin, state->data);
}

void plugin_for_enabled (gint type, PluginForEachFunc func, void * data)
{
    PluginForEnabledState state = {func, data};
    plugin_for_each (type, (PluginForEachFunc) plugin_for_enabled_cb, & state);
}

void plugin_add_watch (PluginHandle * plugin, PluginForEachFunc func, void *
 data)
{
    PluginWatch * watch = g_malloc (sizeof (PluginWatch));
    watch->func = func;
    watch->data = data;
    plugin->watches = g_list_prepend (plugin->watches, watch);
}

void plugin_remove_watch (PluginHandle * plugin, PluginForEachFunc func, void *
 data)
{
    for (GList * node = plugin->watches; node != NULL; )
    {
        GList * next = node->next;
        PluginWatch * watch = node->data;

        if (watch->func == func && watch->data == data)
        {
            g_free (watch);
            plugin->watches = g_list_delete_link (plugin->watches, node);
        }

        node = next;
    }
}


typedef struct {
    const gchar * scheme;
    PluginHandle * plugin;
} TransportPluginForSchemeState;

static gboolean transport_plugin_for_scheme_cb (PluginHandle * plugin,
 TransportPluginForSchemeState * state)
{
    if (! g_list_find_custom (plugin->u.t.schemes, state->scheme, (GCompareFunc)
     strcasecmp))
        return TRUE;

    state->plugin = plugin;
    return FALSE;
}

PluginHandle * transport_plugin_for_scheme (const gchar * scheme)
{
    TransportPluginForSchemeState state = {scheme, NULL};
    plugin_for_enabled (PLUGIN_TYPE_TRANSPORT, (PluginForEachFunc)
     transport_plugin_for_scheme_cb, & state);
    return state.plugin;
}

typedef struct {
    const gchar * ext;
    PluginHandle * plugin;
} PlaylistPluginForExtState;

static gboolean playlist_plugin_for_ext_cb (PluginHandle * plugin,
 PlaylistPluginForExtState * state)
{
    if (! g_list_find_custom (plugin->u.p.exts, state->ext, (GCompareFunc)
     strcasecmp))
        return TRUE;

    state->plugin = plugin;
    return FALSE;
}

PluginHandle * playlist_plugin_for_extension (const gchar * extension)
{
    PlaylistPluginForExtState state = {extension, NULL};
    plugin_for_enabled (PLUGIN_TYPE_PLAYLIST, (PluginForEachFunc)
     playlist_plugin_for_ext_cb, & state);
    return state.plugin;
}

typedef struct {
    gint key;
    const gchar * value;
    PluginForEachFunc func;
    void * data;
} InputPluginForKeyState;

static gboolean input_plugin_for_key_cb (PluginHandle * plugin,
 InputPluginForKeyState * state)
{
    if (g_list_find_custom (plugin->u.i.keys[state->key], state->value,
     (GCompareFunc) strcasecmp) == NULL)
        return TRUE;

    return state->func (plugin, state->data);
}

void input_plugin_for_key (gint key, const gchar * value, PluginForEachFunc
 func, void * data)
{
    InputPluginForKeyState state = {key, value, func, data};
    plugin_for_enabled (PLUGIN_TYPE_INPUT, (PluginForEachFunc)
     input_plugin_for_key_cb, & state);
}

static void input_plugin_add_key (InputPlugin * header, gint key, const gchar *
 value)
{
    PluginHandle * plugin = plugin_by_header (header);
    g_return_if_fail (plugin != NULL);
    plugin->u.i.keys[key] = g_list_prepend (plugin->u.i.keys[key], g_strdup
     (value));
}

void uri_set_plugin (const gchar * scheme, InputPlugin * header)
{
    input_plugin_add_key (header, INPUT_KEY_SCHEME, scheme);
}

void mime_set_plugin (const gchar * mime, InputPlugin * header)
{
    input_plugin_add_key (header, INPUT_KEY_MIME, mime);
}

gboolean input_plugin_has_images (PluginHandle * plugin)
{
    g_return_val_if_fail (plugin->type == PLUGIN_TYPE_INPUT, FALSE);
    return plugin->u.i.has_images;
}

gboolean input_plugin_has_subtunes (PluginHandle * plugin)
{
    g_return_val_if_fail (plugin->type == PLUGIN_TYPE_INPUT, FALSE);
    return plugin->u.i.has_subtunes;
}

gboolean input_plugin_can_write_tuple (PluginHandle * plugin)
{
    g_return_val_if_fail (plugin->type == PLUGIN_TYPE_INPUT, FALSE);
    return plugin->u.i.can_write_tuple;
}

gboolean input_plugin_has_infowin (PluginHandle * plugin)
{
    g_return_val_if_fail (plugin->type == PLUGIN_TYPE_INPUT, FALSE);
    return plugin->u.i.has_infowin;
}
