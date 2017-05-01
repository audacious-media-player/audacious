/*
 * plugin-load.cc
 * Copyright 2007-2013 William Pitcock and John Lindgren
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

#include <assert.h>
#include <errno.h>
#include <pthread.h>
#include <string.h>
#include <sys/stat.h>

#include <glib/gstdio.h>
#include <gmodule.h>

#include "audstrings.h"
#include "internal.h"
#include "plugin.h"
#include "runtime.h"

static const char * plugin_dir_list[] = {
    "Transport",
    "Container",
    "Input",
    "Output",
    "Effect",
    "General",
    "Visualization"
};

struct LoadedModule {
    Plugin * header;
    GModule * module;
};

static Index<LoadedModule> loaded_modules;

bool plugin_check_flags (int flags)
{
    switch (aud_get_mainloop_type ())
    {
        case MainloopType::GLib: flags &= ~PluginGLibOnly; break;
        case MainloopType::Qt: flags &= ~PluginQtOnly; break;
    }

    return ! flags;
}

Plugin * plugin_load (const char * filename)
{
    AUDINFO ("Loading plugin: %s.\n", filename);

    GModule * module = g_module_open (filename, G_MODULE_BIND_LOCAL);

    if (! module)
    {
        AUDERR ("%s could not be loaded: %s\n", filename, g_module_error ());
        return nullptr;
    }

    Plugin * header;
    if (! g_module_symbol (module, "aud_plugin_instance", (void * *) & header))
        header = nullptr;

    if (! header || header->magic != _AUD_PLUGIN_MAGIC)
    {
        AUDERR ("%s is not a valid Audacious plugin.\n", filename);
        g_module_close (module);
        return nullptr;
    }

    if (header->version < _AUD_PLUGIN_VERSION_MIN ||
        header->version > _AUD_PLUGIN_VERSION)
    {
        AUDERR ("%s is not compatible with this version of Audacious.\n", filename);
        g_module_close (module);
        return nullptr;
    }

    if (plugin_check_flags (header->info.flags) &&
        (header->type == PluginType::Transport ||
         header->type == PluginType::Playlist ||
         header->type == PluginType::Input ||
         header->type == PluginType::Effect))
    {
        if (! header->init ())
        {
            AUDERR ("%s failed to initialize.\n", filename);
            g_module_close (module);
            return nullptr;
        }
    }

    loaded_modules.append (header, module);

    return header;
}

static void plugin_unload (LoadedModule & loaded)
{
    if (plugin_check_flags (loaded.header->info.flags) &&
        (loaded.header->type == PluginType::Transport ||
         loaded.header->type == PluginType::Playlist ||
         loaded.header->type == PluginType::Input ||
         loaded.header->type == PluginType::Effect))
    {
        loaded.header->cleanup ();
    }

#ifndef VALGRIND_FRIENDLY
    g_module_close (loaded.module);
#endif
}

/******************************************************************/

static bool scan_plugin_func (const char * path, const char * basename, void * data)
{
    if (! str_has_suffix_nocase (basename, PLUGIN_SUFFIX))
        return false;

    GStatBuf st;
    if (g_stat (path, & st) < 0)
    {
        AUDERR ("Unable to stat %s: %s\n", path, strerror (errno));
        return false;
    }

    if (S_ISREG (st.st_mode))
        plugin_register (path, st.st_mtime);

    return false;
}

static void scan_plugins (const char * path)
{
    dir_foreach (path, scan_plugin_func, nullptr);
}

void plugin_system_init ()
{
    assert (g_module_supported ());

    plugin_registry_load ();

    const char * path = aud_get_path (AudPath::PluginDir);
    for (const char * dir : plugin_dir_list)
        scan_plugins (filename_build ({path, dir}));

    plugin_registry_prune ();
}

void plugin_system_cleanup ()
{
    plugin_registry_save ();
    plugin_registry_cleanup ();

    for (LoadedModule & loaded : loaded_modules)
        plugin_unload (loaded);

    loaded_modules.clear ();
}
