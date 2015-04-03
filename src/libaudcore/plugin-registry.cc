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

#include <errno.h>
#include <pthread.h>
#include <string.h>

#include <glib/gstdio.h>

#include "audstrings.h"
#include "i18n.h"
#include "interface.h"
#include "plugin.h"
#include "runtime.h"

#define FILENAME "plugin-registry"

/* Increment this when the format of the plugin-registry file changes.
 * Add 10 if the format changes in a way that will break parse_plugins_fallback(). */
#define FORMAT 9

/* Oldest file format supported by parse_plugins_fallback() */
#define MIN_FORMAT 2  // "enabled" flag was added in Audacious 2.4

struct PluginWatch {
    PluginWatchFunc func;
    void * data;
};

class PluginHandle
{
public:
    String basename, path;
    bool loaded;
    int timestamp;
    PluginType type;
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
    aud::array<InputKey, Index<String>> keys;
    int has_subtunes, writes_tag;

    PluginHandle (const char * basename, const char * path, bool loaded,
     int timestamp, PluginType type, Plugin * header) :
        basename (basename),
        path (path),
        loaded (loaded),
        timestamp (timestamp),
        type (type),
        header (header),
        priority (0),
        has_about (false),
        has_configure (false),
        enabled (type == PluginType::Transport ||
         type == PluginType::Playlist || type == PluginType::Input),
        has_subtunes (false),
        writes_tag (false) {}

    ~PluginHandle ()
    {
        if (watches.len ())
            AUDWARN ("Plugin watch count not zero at exit!\n");
    }
};

static constexpr aud::array<PluginType, const char *> plugin_type_names = {
    "transport",
    "playlist",
    "input",
    "effect",
    "output",
    "vis",
    "general",
    "iface"
};

static constexpr aud::array<InputKey, const char *> input_key_names = {
    "scheme",
    "ext",
    "mime"
};

static aud::array<PluginType, Index<PluginHandle *>> plugins;
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
    FILE * handle = g_fopen (path, mode);

    if (! handle && errno != ENOENT)
        AUDWARN ("%s: %s\n", (const char *) path, strerror (errno));

    return handle;
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
    for (auto k : aud::range<InputKey> ())
    {
        for (const String & key : plugin->keys[k])
            fprintf (handle, "%s %s\n", input_key_names[k], (const char *) key);
    }

    fprintf (handle, "subtunes %d\n", plugin->has_subtunes);
    fprintf (handle, "writes %d\n", plugin->writes_tag);
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

    if (plugin->type == PluginType::Transport)
        transport_plugin_save (plugin, handle);
    else if (plugin->type == PluginType::Playlist)
        playlist_plugin_save (plugin, handle);
    else if (plugin->type == PluginType::Input)
        input_plugin_save (plugin, handle);
}

void plugin_registry_save ()
{
    FILE * handle = open_registry_file ("w");
    if (! handle)
        return;

    fprintf (handle, "format %d\n", FORMAT);

    for (auto & list : plugins)
    {
        for (PluginHandle * plugin : list)
        {
            plugin_save (plugin, handle);
            delete plugin;
        }

        list.clear ();
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
    for (auto key : aud::range<InputKey> ())
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

    if (parse_integer ("subtunes", & plugin->has_subtunes))
        parse_next (handle);
    if (parse_integer ("writes", & plugin->writes_tag))
        parse_next (handle);
}

static bool plugin_parse (FILE * handle)
{
    PluginType type;
    String path;

    for (auto type2 : aud::range<PluginType> ())
    {
        type = type2;
        if ((path = parse_string (plugin_type_names[type2])))
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

    auto plugin = new PluginHandle (basename, String (), false, timestamp, type, nullptr);
    plugins[type].append (plugin);

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

    if (type == PluginType::Transport)
        transport_plugin_parse (plugin, handle);
    else if (type == PluginType::Playlist)
        playlist_plugin_parse (plugin, handle);
    else if (type == PluginType::Input)
        input_plugin_parse (plugin, handle);

    return true;
}

/* try to migrate enabled status from another version */
static void parse_plugins_fallback (FILE * handle)
{
    for (; parse_value; parse_next (handle))
    {
        PluginType type;
        String path;
        int enabled;

        for (auto type2 : aud::range<PluginType> ())
        {
            type = type2;
            if ((path = parse_string (plugin_type_names[type2])))
                break;
        }

        if (! path)
            continue;

        StringBuf basename = get_basename (path);
        if (! basename)
            continue;

        parse_next (handle);

        for (; parse_value; parse_next (handle))
        {
            if (parse_integer ("enabled", & enabled))
                break;
        }

        if (! parse_value)
            return;

        // setting timestamp to zero forces a rescan
        auto plugin = new PluginHandle (basename, String (), false, 0, type, nullptr);
        plugins[type].append (plugin);
        plugin->enabled = enabled;
    }
}

void plugin_registry_load ()
{
    FILE * handle = open_registry_file ("r");
    if (! handle)
        return;

    parse_next (handle);

    int format;
    if (! parse_integer ("format", & format))
        goto ERR;

    parse_next (handle);

    if (format == FORMAT)
    {
        while (plugin_parse (handle))
            continue;
    }
    else if (format >= MIN_FORMAT && format < FORMAT + 10)
        parse_plugins_fallback (handle);

ERR:
    fclose (handle);
}

static int plugin_compare (PluginHandle * const & a, PluginHandle * const & b, void *)
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

void plugin_registry_prune ()
{
    auto check_not_found = [] (PluginHandle * plugin)
    {
        if (plugin->path)
            return false;

        AUDINFO ("Plugin not found: %s\n", (const char *) plugin->basename);
        delete plugin;
        return true;
    };

    for (auto & list : plugins)
    {
        list.remove_if (check_not_found);
        list.sort (plugin_compare, nullptr);
    }
}

/* Note: If there are multiple plugins with the same basename, this returns only
 * one of them.  Different plugins should be given different basenames. */
EXPORT PluginHandle * aud_plugin_lookup_basename (const char * basename)
{
    for (auto & list : plugins)
    {
        for (PluginHandle * plugin : list)
        {
            if (! strcmp (plugin->basename, basename))
                return plugin;
        }
    }

    return nullptr;
}

static void plugin_get_info (PluginHandle * plugin, bool is_new)
{
    Plugin * header = plugin->header;

    plugin->name = String (header->info.name);
    plugin->domain = String (header->info.domain);
    plugin->has_about = (bool) header->info.about;
    plugin->has_configure = (bool) header->info.prefs;

    if (header->type == PluginType::Transport)
    {
        TransportPlugin * tp = (TransportPlugin *) header;

        plugin->schemes.clear ();
        for (const char * scheme : tp->schemes)
            plugin->schemes.append (String (scheme));
    }
    else if (header->type == PluginType::Playlist)
    {
        PlaylistPlugin * pp = (PlaylistPlugin *) header;

        plugin->exts.clear ();
        for (const char * ext : pp->extensions)
            plugin->exts.append (String (ext));
    }
    else if (header->type == PluginType::Input)
    {
        InputPlugin * ip = (InputPlugin *) header;
        plugin->priority = ip->input_info.priority;

        for (auto k : aud::range<InputKey> ())
        {
            plugin->keys[k].clear ();
            for (auto key = ip->input_info.keys[k]; key && * key; key ++)
                plugin->keys[k].append (String (* key));
        }

        plugin->has_subtunes = (ip->input_info.flags & InputPlugin::FlagSubtunes);
        plugin->writes_tag = (ip->input_info.flags & InputPlugin::FlagWritesTag);
    }
    else if (header->type == PluginType::Output)
    {
        OutputPlugin * op = (OutputPlugin *) header;
        plugin->priority = 10 - op->priority;
    }
    else if (header->type == PluginType::Effect)
    {
        EffectPlugin * ep = (EffectPlugin *) header;
        plugin->priority = ep->order;
    }
    else if (header->type == PluginType::General)
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
        plugins[plugin->type].append (plugin);

        plugin_get_info (plugin, true);
    }
}

EXPORT PluginType aud_plugin_get_type (PluginHandle * plugin)
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
    for (auto & list : plugins)
    {
        for (PluginHandle * plugin : list)
        {
            if (plugin->header == header)
                return plugin;
        }
    }

    return nullptr;
}

EXPORT const Index<PluginHandle *> & aud_plugin_list (PluginType type)
{
    return plugins[type];
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
    auto call_and_check_remove = [=] (const PluginWatch & watch)
        { return ! watch.func (plugin, watch.data); };

    plugin->watches.remove_if (call_and_check_remove);
}

void plugin_set_enabled (PluginHandle * plugin, bool enabled)
{
    plugin->enabled = enabled;
    plugin_call_watches (plugin);
}

void plugin_set_failed (PluginHandle * plugin)
{
    plugin->header = nullptr;
}

EXPORT void aud_plugin_add_watch (PluginHandle * plugin, PluginWatchFunc func, void * data)
{
    plugin->watches.append (func, data);
}

EXPORT void aud_plugin_remove_watch (PluginHandle * plugin, PluginWatchFunc func, void * data)
{
    auto is_match = [=] (const PluginWatch & watch)
        { return watch.func == func && watch.data == data; };

    plugin->watches.remove_if (is_match);
}

bool transport_plugin_has_scheme (PluginHandle * plugin, const char * scheme)
{
    g_return_val_if_fail (plugin->type == PluginType::Transport, false);

    for (String & s : plugin->schemes)
    {
        if (! strcmp (s, scheme))
            return true;
    }

    return false;
}

bool playlist_plugin_has_ext (PluginHandle * plugin, const char * ext)
{
    g_return_val_if_fail (plugin->type == PluginType::Playlist, false);

    for (String & e : plugin->exts)
    {
        if (! strcmp_nocase (e, ext))
            return true;
    }

    return false;
}

bool input_plugin_has_key (PluginHandle * plugin, InputKey key, const char * value)
{
    g_return_val_if_fail (plugin->type == PluginType::Input, false);

    for (String & s : plugin->keys[key])
    {
        if (! strcmp_nocase (s, value))
            return true;
    }

    return false;
}

bool input_plugin_has_subtunes (PluginHandle * plugin)
{
    g_return_val_if_fail (plugin->type == PluginType::Input, false);
    return plugin->has_subtunes;
}

bool input_plugin_can_write_tuple (PluginHandle * plugin)
{
    g_return_val_if_fail (plugin->type == PluginType::Input, false);
    return plugin->writes_tag;
}
