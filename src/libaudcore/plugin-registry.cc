/*
 * plugin-registry.c
 * Copyright 2009-2013 John Lindgren
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

#include "plugins-internal.h"

#include <pthread.h>
#include <string.h>

#include <glib.h>
#include <glib/gstdio.h>

#include "audstrings.h"
#include "i18n.h"
#include "interface.h"
#include "plugin.h"
#include "runtime.h"

#define FILENAME "plugin-registry"
#define FORMAT 8

struct PluginWatch {
    PluginForEachFunc func;
    void * data;
};

struct PluginHandle
{
    String basename, path;
    bool loaded;
    int timestamp, type;
    Plugin * header;
    String name, domain;
    int priority;
    int has_about, has_configure, enabled;
    Index<PluginWatch> watches;

    /* for transport plugins */
    Index<String> schemes;

    /* for playlist plugins */
    Index<String> exts;

    /* for input plugins */
    Index<String> keys[INPUT_KEYS];
    int has_images, has_subtunes, can_write_tuple, has_infowin;

    PluginHandle (const char * basename, const char * path, bool loaded,
     int timestamp, int type, Plugin * header) :
        basename (basename),
        path (path),
        loaded (loaded),
        timestamp (timestamp),
        type (type),
        header (header),
        priority (0),
        has_about (false),
        has_configure (false),
        enabled (type == PLUGIN_TYPE_TRANSPORT ||
         type == PLUGIN_TYPE_PLAYLIST || type == PLUGIN_TYPE_INPUT),
        has_images (false),
        has_subtunes (false),
        can_write_tuple (false),
        has_infowin (false) {}

    ~PluginHandle ()
        { g_warn_if_fail (! watches.len ()); }
};

static const char * plugin_type_names[PLUGIN_TYPES] = {
    "transport",
    "playlist",
    "input",
    "effect",
    "output",
    "vis",
    "general",
    "iface"
};

static const char * input_key_names[INPUT_KEYS] = {
    "scheme",
    "ext",
    "mime"
};

typedef SmartPtr<PluginHandle> PluginPtr;
typedef Index<PluginPtr> PluginList;

static PluginList plugins[PLUGIN_TYPES];

static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

static StringBuf get_basename (const char * path)
{
    const char * slash = strrchr (path, G_DIR_SEPARATOR);
    const char * dot = slash ? strrchr (slash + 1, '.') : nullptr;

    return dot ? str_copy (slash + 1, dot - (slash + 1)) : StringBuf ();
}

static FILE * open_registry_file (const char * mode)
{
    StringBuf path = filename_build ({aud_get_path (AudPath::UserDir), FILENAME});
    return g_fopen (path, mode);
}

static void transport_plugin_save (PluginHandle * plugin, FILE * handle)
{
    for (const String & scheme : plugin->schemes)
        fprintf (handle, "scheme %s\n", (const char *) scheme);
}

static void playlist_plugin_save (PluginHandle * plugin, FILE * handle)
{
    for (const String & ext : plugin->exts)
        fprintf (handle, "ext %s\n", (const char *) ext);
}

static void input_plugin_save (PluginHandle * plugin, FILE * handle)
{
    for (int k = 0; k < INPUT_KEYS; k ++)
    {
        for (const String & key : plugin->keys[k])
            fprintf (handle, "%s %s\n", input_key_names[k], (const char *) key);
    }

    fprintf (handle, "images %d\n", plugin->has_images);
    fprintf (handle, "subtunes %d\n", plugin->has_subtunes);
    fprintf (handle, "writes %d\n", plugin->can_write_tuple);
    fprintf (handle, "infowin %d\n", plugin->has_infowin);
}

static void plugin_save (PluginHandle * plugin, FILE * handle)
{
    fprintf (handle, "%s %s\n", plugin_type_names[plugin->type], (const char *) plugin->path);
    fprintf (handle, "stamp %d\n", plugin->timestamp);
    fprintf (handle, "name %s\n", (const char *) plugin->name);

    if (plugin->domain)
        fprintf (handle, "domain %s\n", (const char *) plugin->domain);

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

    for (int t = 0; t < PLUGIN_TYPES; t ++)
    {
        for (PluginPtr & plugin : plugins[t])
            plugin_save (plugin.get (), handle);

        plugins[t].clear ();
    }

    fclose (handle);
}

static char parse_key[512];
static char * parse_value;

static void parse_next (FILE * handle)
{
    parse_value = nullptr;

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

static bool parse_integer (const char * key, int * value)
{
    return (parse_value && ! strcmp (parse_key, key) && sscanf (parse_value, "%d", value) == 1);
}

static String parse_string (const char * key)
{
    return (parse_value && ! strcmp (parse_key, key)) ? String (parse_value) : String ();
}

static void transport_plugin_parse (PluginHandle * plugin, FILE * handle)
{
    while (1)
    {
        String value = parse_string ("scheme");
        if (! value)
            break;

        plugin->schemes.append (std::move (value));
        parse_next (handle);
    }
}

static void playlist_plugin_parse (PluginHandle * plugin, FILE * handle)
{
    while (1)
    {
        String value = parse_string ("ext");
        if (! value)
            break;

        plugin->exts.append (std::move (value));
        parse_next (handle);
    }
}

static void input_plugin_parse (PluginHandle * plugin, FILE * handle)
{
    for (int key = 0; key < INPUT_KEYS; key ++)
    {
        while (1)
        {
            String value = parse_string (input_key_names[key]);
            if (! value)
                break;

            plugin->keys[key].append (std::move (value));
            parse_next (handle);
        }
    }

    if (parse_integer ("images", & plugin->has_images))
        parse_next (handle);
    if (parse_integer ("subtunes", & plugin->has_subtunes))
        parse_next (handle);
    if (parse_integer ("writes", & plugin->can_write_tuple))
        parse_next (handle);
    if (parse_integer ("infowin", & plugin->has_infowin))
        parse_next (handle);
}

static bool plugin_parse (FILE * handle)
{
    int type;
    String path;

    for (type = 0; type < PLUGIN_TYPES; type ++)
    {
        path = parse_string (plugin_type_names[type]);
        if (path)
            break;
    }

    if (! path)
        return false;

    StringBuf basename = get_basename (path);
    if (! basename)
        return false;

    parse_next (handle);

    int timestamp;
    if (! parse_integer ("stamp", & timestamp))
        return false;

    PluginHandle * plugin = new PluginHandle (basename, String (), false, timestamp, type, nullptr);
    plugins[type].append (PluginPtr (plugin));

    parse_next (handle);

    plugin->name = parse_string ("name");
    if (plugin->name)
        parse_next (handle);

    plugin->domain = parse_string ("domain");
    if (plugin->domain)
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

    return true;
}

void plugin_registry_load (void)
{
    FILE * handle = open_registry_file ("r");
    if (! handle)
        return;

    parse_next (handle);

    int format;
    if (! parse_integer ("format", & format) || format != FORMAT)
        goto ERR;

    parse_next (handle);

    while (plugin_parse (handle))
        ;

ERR:
    fclose (handle);
}

EXPORT int aud_plugin_compare (PluginHandle * a, PluginHandle * b)
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
    if ((diff = str_compare (dgettext (a->domain, a->name), dgettext (b->domain, b->name))))
        return diff;

    return str_compare (a->path, b->path);
}

void plugin_registry_prune (void)
{
    auto compare_cb = [] (const PluginPtr & a, const PluginPtr & b, void *) -> int
        { return aud_plugin_compare ((PluginHandle *) a.get (), (PluginHandle *) b.get ()); };

    for (int t = 0; t < PLUGIN_TYPES; t ++)
    {
        PluginList & list = plugins[t];

        for (int i = 0; i < list.len ();)
        {
            PluginPtr & plugin = list[i];

            if (plugin->path)
                i ++;
            else
            {
                AUDINFO ("Plugin not found: %s\n", (const char *) plugin->basename);
                list.remove (i, 1);
            }
        }

        list.sort (compare_cb, nullptr);
    }
}

/* Note: If there are multiple plugins with the same basename, this returns only
 * one of them.  Different plugins should be given different basenames. */
EXPORT PluginHandle * aud_plugin_lookup_basename (const char * basename)
{
    for (int t = 0; t < PLUGIN_TYPES; t ++)
    {
        for (PluginPtr & plugin : plugins[t])
        {
            if (! strcmp (plugin->basename, basename))
                return plugin.get ();
        }
    }

    return nullptr;
}

static void plugin_get_info (PluginHandle * plugin, bool is_new)
{
    Plugin * header = plugin->header;

    plugin->name = String (header->name);
    plugin->domain = PLUGIN_HAS_FUNC (header, domain) ? String (header->domain) : String ();
    plugin->has_about = PLUGIN_HAS_FUNC (header, about) || PLUGIN_HAS_FUNC (header, about_text);
    plugin->has_configure = PLUGIN_HAS_FUNC (header, configure) || PLUGIN_HAS_FUNC (header, prefs);

    if (header->type == PLUGIN_TYPE_TRANSPORT)
    {
        TransportPlugin * tp = (TransportPlugin *) header;

        plugin->schemes.clear ();
        for (int i = 0; tp->schemes[i]; i ++)
            plugin->schemes.append (String (tp->schemes[i]));
    }
    else if (header->type == PLUGIN_TYPE_PLAYLIST)
    {
        PlaylistPlugin * pp = (PlaylistPlugin *) header;

        plugin->exts.clear ();
        for (int i = 0; pp->extensions[i]; i ++)
            plugin->exts.append (String (pp->extensions[i]));
    }
    else if (header->type == PLUGIN_TYPE_INPUT)
    {
        InputPlugin * ip = (InputPlugin *) header;
        plugin->priority = ip->priority;

        for (int key = 0; key < INPUT_KEYS; key ++)
            plugin->keys[key].clear ();

        if (PLUGIN_HAS_FUNC (ip, extensions))
        {
            for (int i = 0; ip->extensions[i]; i ++)
                plugin->keys[INPUT_KEY_EXTENSION].append (String (ip->extensions[i]));
        }

        if (PLUGIN_HAS_FUNC (ip, mimes))
        {
            for (int i = 0; ip->mimes[i]; i ++)
                plugin->keys[INPUT_KEY_MIME].append (String (ip->mimes[i]));
        }

        if (PLUGIN_HAS_FUNC (ip, schemes))
        {
            for (int i = 0; ip->schemes[i]; i ++)
                plugin->keys[INPUT_KEY_SCHEME].append (String (ip->schemes[i]));
        }

        plugin->has_images = PLUGIN_HAS_FUNC (ip, get_song_image);
        plugin->has_subtunes = ip->have_subtune;
        plugin->can_write_tuple = PLUGIN_HAS_FUNC (ip, update_song_tuple);
        plugin->has_infowin = PLUGIN_HAS_FUNC (ip, file_info_box);
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
        if (is_new)
            plugin->enabled = gp->enabled_by_default;
    }
}

void plugin_register (const char * path, int timestamp)
{
    StringBuf basename = get_basename (path);
    if (! basename)
        return;

    PluginHandle * plugin = aud_plugin_lookup_basename (basename);

    if (plugin)
    {
        AUDINFO ("Register plugin: %s\n", path);
        plugin->path = String (path);

        if (plugin->timestamp != timestamp)
        {
            AUDINFO ("Rescan plugin: %s\n", path);
            Plugin * header = plugin_load (path);
            if (! header || header->type != plugin->type)
                return;

            plugin->loaded = true;
            plugin->header = header;
            plugin->timestamp = timestamp;

            plugin_get_info (plugin, false);
        }
    }
    else
    {
        AUDINFO ("New plugin: %s\n", path);
        Plugin * header = plugin_load (path);
        if (! header)
            return;

        plugin = new PluginHandle (basename, path, true, timestamp, header->type, header);
        plugins[plugin->type].append (PluginPtr (plugin));

        plugin_get_info (plugin, true);
    }
}

EXPORT int aud_plugin_get_type (PluginHandle * plugin)
{
    return plugin->type;
}

EXPORT const char * aud_plugin_get_basename (PluginHandle * plugin)
{
    return plugin->basename;
}

EXPORT const void * aud_plugin_get_header (PluginHandle * plugin)
{
    pthread_mutex_lock (& mutex);

    if (! plugin->loaded)
    {
        Plugin * header = plugin_load (plugin->path);
        if (! header || header->type != plugin->type)
            goto DONE;

        plugin->loaded = true;
        plugin->header = header;
    }

DONE:
    pthread_mutex_unlock (& mutex);
    return plugin->header;
}

EXPORT PluginHandle * aud_plugin_by_header (const void * header)
{
    for (int t = 0; t < PLUGIN_TYPES; t ++)
    {
        for (PluginPtr & plugin : plugins[t])
        {
            if (plugin->header == header)
                return plugin.get ();
        }
    }

    return nullptr;
}

EXPORT int aud_plugin_count (int type)
{
    g_return_val_if_fail (type >= 0 && type < PLUGIN_TYPES, 0);

    return plugins[type].len ();
}

EXPORT int aud_plugin_get_index (PluginHandle * plugin)
{
    PluginList & list = plugins[plugin->type];

    for (int i = 0; i < list.len (); i ++)
    {
        if (list[i].get () == plugin)
            return i;
    }

    g_return_val_if_reached (-1);
}

EXPORT PluginHandle * aud_plugin_by_index (int type, int index)
{
    g_return_val_if_fail (type >= 0 && type < PLUGIN_TYPES, nullptr);

    PluginList & list = plugins[type];
    g_return_val_if_fail (index >= 0 && index < list.len (), nullptr);

    return list[index].get ();
}

EXPORT void aud_plugin_for_each (int type, PluginForEachFunc func, void * data)
{
    g_return_if_fail (type >= 0 && type < PLUGIN_TYPES);

    for (PluginPtr & plugin : plugins[type])
    {
        if (! func (plugin.get (), data))
            return;
    }
}

EXPORT const char * aud_plugin_get_name (PluginHandle * plugin)
{
    return dgettext (plugin->domain, plugin->name);
}

EXPORT bool aud_plugin_has_about (PluginHandle * plugin)
{
    return plugin->has_about;
}

EXPORT bool aud_plugin_has_configure (PluginHandle * plugin)
{
    return plugin->has_configure;
}

EXPORT bool aud_plugin_get_enabled (PluginHandle * plugin)
{
    return plugin->enabled;
}

static void plugin_call_watches (PluginHandle * plugin)
{
    Index<PluginWatch> & watches = plugin->watches;

    for (int i = 0; i < watches.len ();)
    {
        PluginWatch & watch = watches[i];

        if (watch.func (plugin, watch.data))
            i ++;
        else
            watches.remove (i, 1);
    }
}

void plugin_set_enabled (PluginHandle * plugin, bool enabled)
{
    plugin->enabled = enabled;
    plugin_call_watches (plugin);
}

EXPORT void aud_plugin_for_enabled (int type, PluginForEachFunc func, void * data)
{
    g_return_if_fail (type >= 0 && type < PLUGIN_TYPES);

    for (PluginPtr & plugin : plugins[type])
    {
        if (plugin->enabled && ! func (plugin.get (), data))
            return;
    }
}

EXPORT void aud_plugin_add_watch (PluginHandle * plugin, PluginForEachFunc func, void * data)
{
    plugin->watches.append ({func, data});
}

EXPORT void aud_plugin_remove_watch (PluginHandle * plugin, PluginForEachFunc func, void * data)
{
    Index<PluginWatch> & watches = plugin->watches;

    for (int i = 0; i < watches.len ();)
    {
        PluginWatch & watch = watches[i];

        if (watch.func == func && watch.data == data)
            watches.remove (i, 1);
        else
            i ++;
    }
}

PluginHandle * transport_plugin_for_scheme (const char * scheme)
{
    for (PluginPtr & plugin : plugins[PLUGIN_TYPE_TRANSPORT])
    {
        if (! plugin->enabled)
            continue;

        for (String & s : plugin->schemes)
        {
            if (! strcmp (s, scheme))
                return plugin.get ();
        }
    }

    return nullptr;
}

void playlist_plugin_for_ext (const char * ext, PluginForEachFunc func, void * data)
{
    for (PluginPtr & plugin : plugins[PLUGIN_TYPE_PLAYLIST])
    {
        if (! plugin->enabled)
            continue;

        for (String & e : plugin->exts)
        {
            if (! g_ascii_strcasecmp (e, ext))
            {
                if (! func (plugin.get (), data))
                    return;

                break;
            }
        }
    }
}

void input_plugin_for_key (int key, const char * value, PluginForEachFunc func, void * data)
{
    g_return_if_fail (key >= 0 && key < INPUT_KEYS);

    for (PluginPtr & plugin : plugins[PLUGIN_TYPE_INPUT])
    {
        if (! plugin->enabled)
            continue;

        for (String & s : plugin->keys[key])
        {
            if (! g_ascii_strcasecmp (s, value))
            {
                if (! func (plugin.get (), data))
                    return;

                break;
            }
        }
    }
}

bool input_plugin_has_images (PluginHandle * plugin)
{
    g_return_val_if_fail (plugin->type == PLUGIN_TYPE_INPUT, false);
    return plugin->has_images;
}

bool input_plugin_has_subtunes (PluginHandle * plugin)
{
    g_return_val_if_fail (plugin->type == PLUGIN_TYPE_INPUT, false);
    return plugin->has_subtunes;
}

bool input_plugin_can_write_tuple (PluginHandle * plugin)
{
    g_return_val_if_fail (plugin->type == PLUGIN_TYPE_INPUT, false);
    return plugin->can_write_tuple;
}

bool input_plugin_has_infowin (PluginHandle * plugin)
{
    g_return_val_if_fail (plugin->type == PLUGIN_TYPE_INPUT, false);
    return plugin->has_infowin;
}
