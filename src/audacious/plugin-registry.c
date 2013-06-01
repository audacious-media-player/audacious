/*
 * plugin-registry.c
 * Copyright 2009-2011 John Lindgren
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions, and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions, and the following disclaimer in the documentation
 *    provided with the distribution.
 *
 * This software is provided "as is" and without any warranty, express or
 * implied. In no event shall the authors be liable for any damages arising from
 * the use of this software.
 */

/* While the registry is being built (during early startup) or destroyed (during
 * late shutdown), the registry_locked flag will be set.  Once this flag is
 * cleared, the registry will not be modified and can be read by concurrent
 * threads.  The one change that can happen during this time is that a plugin is
 * loaded; hence the mutex must be locked before checking that a plugin is
 * loaded and while loading it. */

#include <glib.h>
#include <pthread.h>
#include <stdio.h>
#include <string.h>

#include <libaudcore/audstrings.h>

#include "debug.h"
#include "i18n.h"
#include "interface.h"
#include "misc.h"
#include "plugin.h"
#include "plugins.h"

#define FILENAME "plugin-registry"
#define FORMAT 8

typedef struct {
    GList * schemes;
} TransportPluginData;

typedef struct {
    GList * exts;
} PlaylistPluginData;

typedef struct {
    GList * keys[INPUT_KEYS];
    bool_t has_images, has_subtunes, can_write_tuple, has_infowin;
} InputPluginData;

struct PluginHandle {
    char * path;
    bool_t confirmed, loaded;
    int timestamp, type;
    Plugin * header;
    char * name, * domain;
    int priority;
    bool_t has_about, has_configure, enabled;
    GList * watches;
    void * misc;

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

static const char * plugin_type_names[] = {
 [PLUGIN_TYPE_TRANSPORT] = "transport",
 [PLUGIN_TYPE_PLAYLIST] = "playlist",
 [PLUGIN_TYPE_INPUT] = "input",
 [PLUGIN_TYPE_EFFECT] = "effect",
 [PLUGIN_TYPE_OUTPUT] = "output",
 [PLUGIN_TYPE_VIS] = "vis",
 [PLUGIN_TYPE_GENERAL] = "general",
 [PLUGIN_TYPE_IFACE] = "iface"};

static const char * input_key_names[] = {
 [INPUT_KEY_SCHEME] = "scheme",
 [INPUT_KEY_EXTENSION] = "ext",
 [INPUT_KEY_MIME] = "mime"};

static GList * plugin_list = NULL;
static bool_t registry_locked = TRUE;
static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

static PluginHandle * plugin_new (char * path, bool_t confirmed, bool_t
 loaded, int timestamp, int type, Plugin * header)
{
    PluginHandle * plugin = g_malloc (sizeof (PluginHandle));

    plugin->path = path;
    plugin->confirmed = confirmed;
    plugin->loaded = loaded;
    plugin->timestamp = timestamp;
    plugin->type = type;
    plugin->header = header;
    plugin->name = NULL;
    plugin->domain = NULL;
    plugin->priority = 0;
    plugin->has_about = FALSE;
    plugin->has_configure = FALSE;
    plugin->enabled = FALSE;
    plugin->watches = NULL;
    plugin->misc = NULL;

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
    return plugin;
}

static void plugin_free (PluginHandle * plugin)
{
    plugin_list = g_list_remove (plugin_list, plugin);

    g_list_free_full (plugin->watches, g_free);

    if (plugin->type == PLUGIN_TYPE_TRANSPORT)
        g_list_free_full (plugin->u.t.schemes, g_free);
    else if (plugin->type == PLUGIN_TYPE_PLAYLIST)
        g_list_free_full (plugin->u.p.exts, g_free);
    else if (plugin->type == PLUGIN_TYPE_INPUT)
    {
        for (int key = 0; key < INPUT_KEYS; key ++)
            g_list_free_full (plugin->u.i.keys[key], g_free);
    }

    g_free (plugin->path);
    g_free (plugin->name);
    g_free (plugin->domain);
    g_free (plugin->misc);
    g_free (plugin);
}

static FILE * open_registry_file (const char * mode)
{
    char * path = g_strdup_printf ("%s/" FILENAME, get_path (AUD_PATH_USER_DIR));
    FILE * file = fopen (path, mode);
    g_free (path);
    return file;
}

static void transport_plugin_save (PluginHandle * plugin, FILE * handle)
{
    for (GList * node = plugin->u.t.schemes; node; node = node->next)
        fprintf (handle, "scheme %s\n", (const char *) node->data);
}

static void playlist_plugin_save (PluginHandle * plugin, FILE * handle)
{
    for (GList * node = plugin->u.p.exts; node; node = node->next)
        fprintf (handle, "ext %s\n", (const char *) node->data);
}

static void input_plugin_save (PluginHandle * plugin, FILE * handle)
{
    for (int key = 0; key < INPUT_KEYS; key ++)
    {
        for (GList * node = plugin->u.i.keys[key]; node; node = node->next)
            fprintf (handle, "%s %s\n", input_key_names[key], (const char *)
             node->data);
    }

    fprintf (handle, "images %d\n", plugin->u.i.has_images);
    fprintf (handle, "subtunes %d\n", plugin->u.i.has_subtunes);
    fprintf (handle, "writes %d\n", plugin->u.i.can_write_tuple);
    fprintf (handle, "infowin %d\n", plugin->u.i.has_infowin);
}

static void plugin_save (PluginHandle * plugin, FILE * handle)
{
    fprintf (handle, "%s %s\n", plugin_type_names[plugin->type], plugin->path);
    fprintf (handle, "stamp %d\n", plugin->timestamp);
    fprintf (handle, "name %s\n", plugin->name);

    if (plugin->domain)
        fprintf (handle, "domain %s\n", plugin->domain);

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

void plugin_registry_save (void)
{
    FILE * handle = open_registry_file ("w");
    g_return_if_fail (handle);

    fprintf (handle, "format %d\n", FORMAT);

    g_list_foreach (plugin_list, (GFunc) plugin_save, handle);
    fclose (handle);

    g_list_foreach (plugin_list, (GFunc) plugin_free, NULL);
    registry_locked = TRUE;
}

static char parse_key[512];
static char * parse_value;

static void parse_next (FILE * handle)
{
    parse_value = NULL;

    if (! fgets (parse_key, sizeof parse_key, handle))
        return;

    char * space = strchr (parse_key, ' ');
    if (! space)
        return;

    * space = 0;
    parse_value = space + 1;

    char * newline = strchr (parse_value, '\n');
    if (newline)
        * newline = 0;
}

static bool_t parse_integer (const char * key, int * value)
{
    return (parse_value && ! strcmp (parse_key, key) && sscanf (parse_value,
     "%d", value) == 1);
}

static char * parse_string (const char * key)
{
    return (parse_value && ! strcmp (parse_key, key)) ? g_strdup (parse_value) :
     NULL;
}

static void transport_plugin_parse (PluginHandle * plugin, FILE * handle)
{
    char * value;
    while ((value = parse_string ("scheme")))
    {
        plugin->u.t.schemes = g_list_prepend (plugin->u.t.schemes, value);
        parse_next (handle);
    }
}

static void playlist_plugin_parse (PluginHandle * plugin, FILE * handle)
{
    char * value;
    while ((value = parse_string ("ext")))
    {
        plugin->u.p.exts = g_list_prepend (plugin->u.p.exts, value);
        parse_next (handle);
    }
}

static void input_plugin_parse (PluginHandle * plugin, FILE * handle)
{
    for (int key = 0; key < INPUT_KEYS; key ++)
    {
        char * value;
        while ((value = parse_string (input_key_names[key])))
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

static bool_t plugin_parse (FILE * handle)
{
    char * path = NULL;

    int type;
    for (type = 0; type < PLUGIN_TYPES; type ++)
    {
        if ((path = parse_string (plugin_type_names[type])))
            goto FOUND;
    }

    return FALSE;

FOUND:
    parse_next (handle);

    int timestamp;
    if (! parse_integer ("stamp", & timestamp))
    {
        g_free (path);
        return FALSE;
    }

    PluginHandle * plugin = plugin_new (path, FALSE, FALSE, timestamp, type,
     NULL);
    parse_next (handle);

    if ((plugin->name = parse_string ("name")))
        parse_next (handle);
    if ((plugin->domain = parse_string ("domain")))
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

void plugin_registry_load (void)
{
    FILE * handle = open_registry_file ("r");
    if (! handle)
        goto UNLOCK;

    parse_next (handle);

    int format;
    if (! parse_integer ("format", & format) || format != FORMAT)
        goto ERR;

    parse_next (handle);

    while (plugin_parse (handle))
        ;

ERR:
    fclose (handle);
UNLOCK:
    registry_locked = FALSE;
}

static void plugin_prune (PluginHandle * plugin)
{
    if (plugin->confirmed)
        return;

    AUDDBG ("Plugin not found: %s\n", plugin->path);
    plugin_free (plugin);
}

int plugin_compare (PluginHandle * a, PluginHandle * b)
{
    if (a->type < b->type)
        return -1;
    if (a->type > b->type)
        return 1;
    if (a->priority < b->priority)
        return -1;
    if (a->priority > b->priority)
        return 1;

    int diff;
    if ((diff = string_compare (dgettext (a->domain, a->name), dgettext (b->domain, b->name))))
        return diff;

    return string_compare (a->path, b->path);
}

void plugin_registry_prune (void)
{
    g_list_foreach (plugin_list, (GFunc) plugin_prune, NULL);
    plugin_list = g_list_sort (plugin_list, (GCompareFunc) plugin_compare);
    registry_locked = TRUE;
}

static int plugin_lookup_cb (PluginHandle * plugin, const char * path)
{
    return strcmp (plugin->path, path);
}

PluginHandle * plugin_lookup (const char * path)
{
    GList * node = g_list_find_custom (plugin_list, path, (GCompareFunc)
     plugin_lookup_cb);
    return node ? node->data : NULL;
}

static int plugin_lookup_basename_cb (PluginHandle * plugin, const char * basename)
{
    char * test = g_path_get_basename (plugin->path);

    char * dot = strrchr (test, '.');
    if (dot)
        * dot = 0;

    int ret = strcmp (test, basename);

    g_free (test);
    return ret;
}

/* Note: If there are multiple plugins with the same basename, this returns only
 * one of them. So give different plugins different basenames. --jlindgren */
PluginHandle * plugin_lookup_basename (const char * basename)
{
    GList * node = g_list_find_custom (plugin_list, basename, (GCompareFunc)
     plugin_lookup_basename_cb);
    return node ? node->data : NULL;
}

static void plugin_get_info (PluginHandle * plugin, bool_t new)
{
    Plugin * header = plugin->header;

    g_free (plugin->name);
    g_free (plugin->domain);
    plugin->name = g_strdup (header->name);
    plugin->domain = PLUGIN_HAS_FUNC (header, domain) ? g_strdup (header->domain) : NULL;
    plugin->has_about = PLUGIN_HAS_FUNC (header, about) || PLUGIN_HAS_FUNC (header, about_text);
    plugin->has_configure = PLUGIN_HAS_FUNC (header, configure) || PLUGIN_HAS_FUNC (header, prefs);

    if (header->type == PLUGIN_TYPE_TRANSPORT)
    {
        TransportPlugin * tp = (TransportPlugin *) header;

        g_list_free_full (plugin->u.t.schemes, g_free);
        plugin->u.t.schemes = NULL;

        for (int i = 0; tp->schemes[i]; i ++)
            plugin->u.t.schemes = g_list_prepend (plugin->u.t.schemes, g_strdup
             (tp->schemes[i]));
    }
    else if (header->type == PLUGIN_TYPE_PLAYLIST)
    {
        PlaylistPlugin * pp = (PlaylistPlugin *) header;

        g_list_free_full (plugin->u.p.exts, g_free);
        plugin->u.p.exts = NULL;

        for (int i = 0; pp->extensions[i]; i ++)
            plugin->u.p.exts = g_list_prepend (plugin->u.p.exts, g_strdup
             (pp->extensions[i]));
    }
    else if (header->type == PLUGIN_TYPE_INPUT)
    {
        InputPlugin * ip = (InputPlugin *) header;
        plugin->priority = ip->priority;

        for (int key = 0; key < INPUT_KEYS; key ++)
        {
            g_list_free_full (plugin->u.i.keys[key], g_free);
            plugin->u.i.keys[key] = NULL;
        }

        if (PLUGIN_HAS_FUNC (ip, extensions))
        {
            for (int i = 0; ip->extensions[i]; i ++)
                plugin->u.i.keys[INPUT_KEY_EXTENSION] = g_list_prepend
                 (plugin->u.i.keys[INPUT_KEY_EXTENSION], g_strdup
                 (ip->extensions[i]));
        }

        if (PLUGIN_HAS_FUNC (ip, mimes))
        {
            for (int i = 0; ip->mimes[i]; i ++)
                plugin->u.i.keys[INPUT_KEY_MIME] = g_list_prepend
                 (plugin->u.i.keys[INPUT_KEY_MIME], g_strdup (ip->mimes[i]));
        }

        if (PLUGIN_HAS_FUNC (ip, schemes))
        {
            for (int i = 0; ip->schemes[i]; i ++)
                plugin->u.i.keys[INPUT_KEY_SCHEME] = g_list_prepend
                 (plugin->u.i.keys[INPUT_KEY_SCHEME], g_strdup (ip->schemes[i]));
        }

        plugin->u.i.has_images = PLUGIN_HAS_FUNC (ip, get_song_image);
        plugin->u.i.has_subtunes = ip->have_subtune;
        plugin->u.i.can_write_tuple = PLUGIN_HAS_FUNC (ip, update_song_tuple);
        plugin->u.i.has_infowin = PLUGIN_HAS_FUNC (ip, file_info_box);
    }
    else if (header->type == PLUGIN_TYPE_OUTPUT)
    {
        OutputPlugin * op = (OutputPlugin *) header;
        plugin->priority = 10 - op->probe_priority;
    }
    else if (header->type == PLUGIN_TYPE_EFFECT)
    {
        EffectPlugin * ep = (EffectPlugin *) header;
        plugin->priority = ep->order;
    }
    else if (header->type == PLUGIN_TYPE_GENERAL)
    {
        GeneralPlugin * gp = (GeneralPlugin *) header;
        if (new)
            plugin->enabled = gp->enabled_by_default;
    }
}

void plugin_register (const char * path, int timestamp)
{
    PluginHandle * plugin = plugin_lookup (path);

    if (plugin)
    {
        AUDDBG ("Register plugin: %s\n", path);
        plugin->confirmed = TRUE;

        if (plugin->timestamp != timestamp)
        {
            AUDDBG ("Rescan plugin: %s\n", path);
            Plugin * header = plugin_load (path);
            if (! header || header->type != plugin->type)
                return;

            plugin->loaded = TRUE;
            plugin->header = header;
            plugin->timestamp = timestamp;

            plugin_get_info (plugin, FALSE);
        }
    }
    else
    {
        AUDDBG ("New plugin: %s\n", path);
        Plugin * header = plugin_load (path);
        if (! header)
            return;

        plugin = plugin_new (g_strdup (path), TRUE, TRUE, timestamp,
         header->type, header);

        plugin_get_info (plugin, TRUE);
    }
}

int plugin_get_type (PluginHandle * plugin)
{
    return plugin->type;
}

const char * plugin_get_filename (PluginHandle * plugin)
{
    return plugin->path;
}

const void * plugin_get_header (PluginHandle * plugin)
{
    pthread_mutex_lock (& mutex);

    if (! plugin->loaded)
    {
        Plugin * header = plugin_load (plugin->path);
        if (! header || header->type != plugin->type)
            goto DONE;

        plugin->loaded = TRUE;
        plugin->header = header;
    }

DONE:
    pthread_mutex_unlock (& mutex);
    return plugin->header;
}

static int plugin_by_header_cb (PluginHandle * plugin, const void * header)
{
    return (plugin->header == header) ? 0 : -1;
}

PluginHandle * plugin_by_header (const void * header)
{
    GList * node = g_list_find_custom (plugin_list, header, (GCompareFunc)
     plugin_by_header_cb);
    return node ? node->data : NULL;
}

void plugin_for_each (int type, PluginForEachFunc func, void * data)
{
    for (GList * node = plugin_list; node; node = node->next)
    {
        if (((PluginHandle *) node->data)->type != type)
            continue;
        if (! func (node->data, data))
            break;
    }
}

const char * plugin_get_name (PluginHandle * plugin)
{
    return dgettext (plugin->domain, plugin->name);
}

bool_t plugin_has_about (PluginHandle * plugin)
{
    return plugin->has_about;
}

bool_t plugin_has_configure (PluginHandle * plugin)
{
    return plugin->has_configure;
}

bool_t plugin_get_enabled (PluginHandle * plugin)
{
    return plugin->enabled;
}

static void plugin_call_watches (PluginHandle * plugin)
{
    for (GList * node = plugin->watches; node; )
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

void plugin_set_enabled (PluginHandle * plugin, bool_t enabled)
{
    plugin->enabled = enabled;
    plugin_call_watches (plugin);
}

typedef struct {
    PluginForEachFunc func;
    void * data;
} PluginForEnabledState;

static bool_t plugin_for_enabled_cb (PluginHandle * plugin,
 PluginForEnabledState * state)
{
    if (! plugin->enabled)
        return TRUE;
    return state->func (plugin, state->data);
}

void plugin_for_enabled (int type, PluginForEachFunc func, void * data)
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
    for (GList * node = plugin->watches; node; )
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

void * plugin_get_misc_data (PluginHandle * plugin, int size)
{
    if (! plugin->misc)
        plugin->misc = g_malloc0 (size);

    return plugin->misc;
}

typedef struct {
    const char * scheme;
    PluginHandle * plugin;
} TransportPluginForSchemeState;

static bool_t transport_plugin_for_scheme_cb (PluginHandle * plugin,
 TransportPluginForSchemeState * state)
{
    if (! g_list_find_custom (plugin->u.t.schemes, state->scheme,
     (GCompareFunc) g_ascii_strcasecmp))
        return TRUE;

    state->plugin = plugin;
    return FALSE;
}

PluginHandle * transport_plugin_for_scheme (const char * scheme)
{
    TransportPluginForSchemeState state = {scheme, NULL};
    plugin_for_enabled (PLUGIN_TYPE_TRANSPORT, (PluginForEachFunc)
     transport_plugin_for_scheme_cb, & state);
    return state.plugin;
}

typedef struct {
    const char * ext;
    PluginHandle * plugin;
} PlaylistPluginForExtState;

static bool_t playlist_plugin_for_ext_cb (PluginHandle * plugin,
 PlaylistPluginForExtState * state)
{
    if (! g_list_find_custom (plugin->u.p.exts, state->ext,
     (GCompareFunc) g_ascii_strcasecmp))
        return TRUE;

    state->plugin = plugin;
    return FALSE;
}

PluginHandle * playlist_plugin_for_extension (const char * extension)
{
    PlaylistPluginForExtState state = {extension, NULL};
    plugin_for_enabled (PLUGIN_TYPE_PLAYLIST, (PluginForEachFunc)
     playlist_plugin_for_ext_cb, & state);
    return state.plugin;
}

typedef struct {
    int key;
    const char * value;
    PluginForEachFunc func;
    void * data;
} InputPluginForKeyState;

static bool_t input_plugin_for_key_cb (PluginHandle * plugin,
 InputPluginForKeyState * state)
{
    if (! g_list_find_custom (plugin->u.i.keys[state->key], state->value,
     (GCompareFunc) g_ascii_strcasecmp))
        return TRUE;

    return state->func (plugin, state->data);
}

void input_plugin_for_key (int key, const char * value, PluginForEachFunc
 func, void * data)
{
    InputPluginForKeyState state = {key, value, func, data};
    plugin_for_enabled (PLUGIN_TYPE_INPUT, (PluginForEachFunc)
     input_plugin_for_key_cb, & state);
}

bool_t input_plugin_has_images (PluginHandle * plugin)
{
    g_return_val_if_fail (plugin->type == PLUGIN_TYPE_INPUT, FALSE);
    return plugin->u.i.has_images;
}

bool_t input_plugin_has_subtunes (PluginHandle * plugin)
{
    g_return_val_if_fail (plugin->type == PLUGIN_TYPE_INPUT, FALSE);
    return plugin->u.i.has_subtunes;
}

bool_t input_plugin_can_write_tuple (PluginHandle * plugin)
{
    g_return_val_if_fail (plugin->type == PLUGIN_TYPE_INPUT, FALSE);
    return plugin->u.i.can_write_tuple;
}

bool_t input_plugin_has_infowin (PluginHandle * plugin)
{
    g_return_val_if_fail (plugin->type == PLUGIN_TYPE_INPUT, FALSE);
    return plugin->u.i.has_infowin;
}
