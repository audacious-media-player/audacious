/*
 * probe.c
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

#include "probe.h"

#include <string.h>

#include "audstrings.h"
#include "i18n.h"
#include "internal.h"
#include "playlist.h"
#include "plugin.h"
#include "plugins-internal.h"
#include "runtime.h"

static bool open_file (const char * filename, InputPlugin * ip,
 const char * mode, VFSFile & file, String * error = nullptr)
{
    /* no need to open a handle for custom URI schemes */
    if (ip && ip->input_info.keys[InputKey::Scheme])
        return true;

    /* already open? */
    if (file && file.fseek (0, VFS_SEEK_SET) == 0)
        return true;

    file = VFSFile (filename, mode);
    if (! file && error)
        * error = String (file.error ());

    return (bool) file;
}

static PluginHandle * do_find_decoder (const char * filename, bool fast,
 VFSFile & file, String * error)
{
    AUDINFO ("%s %s.\n", fast ? "Fast-probing" : "Probing", filename);

    auto & list = aud_plugin_list (PluginType::Input);

    StringBuf scheme = uri_get_scheme (filename);
    StringBuf ext = uri_get_extension (filename);
    Index<PluginHandle *> ext_matches;

    for (PluginHandle * plugin : list)
    {
        if (! aud_plugin_get_enabled (plugin))
            continue;

        if (scheme && input_plugin_has_key (plugin, InputKey::Scheme, scheme))
        {
            AUDINFO ("Matched %s by URI scheme.\n", aud_plugin_get_name (plugin));
            return plugin;
        }

        if (ext && input_plugin_has_key (plugin, InputKey::Ext, ext))
            ext_matches.append (plugin);
    }

    if (ext_matches.len () == 1)
    {
        AUDINFO ("Matched %s by extension.\n", aud_plugin_get_name (ext_matches[0]));
        return ext_matches[0];
    }

    AUDDBG ("Matched %d plugins by extension.\n", ext_matches.len ());

    if (fast && ! ext_matches.len ())
        return nullptr;

    AUDDBG ("Opening %s.\n", filename);

    if (! open_file (filename, nullptr, "r", file, error))
    {
        AUDINFO ("Open failed.\n");
        return nullptr;
    }

    String mime = file.get_metadata ("content-type");

    if (mime)
    {
        for (PluginHandle * plugin : (ext_matches.len () ? ext_matches : list))
        {
            if (! aud_plugin_get_enabled (plugin))
                continue;

            if (input_plugin_has_key (plugin, InputKey::MIME, mime))
            {
                AUDINFO ("Matched %s by MIME type %s.\n",
                 aud_plugin_get_name (plugin), (const char *) mime);
                return plugin;
            }
        }
    }

    file.set_limit_to_buffer (true);

    for (PluginHandle * plugin : (ext_matches.len () ? ext_matches : list))
    {
        if (! aud_plugin_get_enabled (plugin))
            continue;

        AUDINFO ("Trying %s.\n", aud_plugin_get_name (plugin));

        auto ip = (InputPlugin *) aud_plugin_get_header (plugin);
        if (! ip)
            continue;

        if (ip->is_our_file (filename, file))
        {
            AUDINFO ("Matched %s by content.\n", aud_plugin_get_name (plugin));
            return plugin;
        }

        if (file.fseek (0, VFS_SEEK_SET) != 0)
        {
            if (error)
                * error = String (_("Seek error"));

            AUDINFO ("Seek failed.\n");
            return nullptr;
        }
    }

    if (error)
        * error = String (_("File format not recognized"));

    AUDINFO ("No plugins matched.\n");
    return nullptr;
}

EXPORT PluginHandle * aud_file_find_decoder (const char * filename, bool fast, String * error)
{
    VFSFile file;
    return do_find_decoder (filename, fast, file, error);
}

EXPORT PluginHandle * aud_file_find_decoder (const char * filename, bool fast,
 VFSFile & file, String * error)
{
    auto plugin = do_find_decoder (filename, fast, file, error);

    /* reset VFSFile state for later reuse */
    if (file)
        file.set_limit_to_buffer (false);

    return plugin;
}

EXPORT Tuple aud_file_read_tuple (const char * filename, PluginHandle * decoder,
 VFSFile & file, String * error)
{
    auto ip = (InputPlugin *) aud_plugin_get_header (decoder);
    if (! ip && error)
        * error = String (_("Error loading plugin"));
    if (! ip)
        return Tuple ();

    if (! open_file (filename, ip, "r", file, error))
        return Tuple ();

    Tuple tuple = ip->read_tuple (filename, file);
    if (! tuple && error)
        * error = String (_("Error reading metadata"));

    return tuple;
}

EXPORT Tuple aud_file_read_tuple (const char * filename, PluginHandle * decoder, String * error)
{
    VFSFile file;
    return aud_file_read_tuple (filename, decoder, file, error);
}

EXPORT Index<char> aud_file_read_image (const char * filename, PluginHandle * decoder, VFSFile & file)
{
    auto ip = (InputPlugin *) aud_plugin_get_header (decoder);
    if (! ip)
        return Index<char> ();

    if (! open_file (filename, ip, "r", file))
        return Index<char> ();

    return ip->read_image (filename, file);
}

EXPORT Index<char> aud_file_read_image (const char * filename, PluginHandle * decoder)
{
    VFSFile file;
    return aud_file_read_image (filename, decoder, file);
}

EXPORT bool aud_file_can_write_tuple (const char * filename, PluginHandle * decoder)
{
    return input_plugin_can_write_tuple (decoder);
}

EXPORT bool aud_file_write_tuple (const char * filename,
 PluginHandle * decoder, const Tuple & tuple)
{
    auto ip = (InputPlugin *) aud_plugin_get_header (decoder);
    if (! ip)
        return false;

    VFSFile file;
    if (! open_file (filename, ip, "r+", file))
        return false;

    bool success = ip->write_tuple (filename, file, tuple);

    if (success && file && file.fflush () != 0)
        success = false;

    if (success)
        aud_playlist_rescan_file (filename);

    return success;
}

EXPORT bool aud_custom_infowin (const char * filename, PluginHandle * decoder)
{
    auto ip = (InputPlugin *) aud_plugin_get_header (decoder);
    if (! ip)
        return false;

    VFSFile file;
    if (! open_file (filename, ip, "r", file))
        return false;

    return ip->file_info_box (filename, file);
}
