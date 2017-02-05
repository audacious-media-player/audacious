/*
 * config.c
 * Copyright 2011-2013 John Lindgren
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

#include "runtime.h"
#include "internal.h"

#include <assert.h>
#include <string.h>

#include "audstrings.h"
#include "hook.h"
#include "inifile.h"
#include "multihash.h"
#include "runtime.h"
#include "vfs.h"

#define DEFAULT_SECTION "audacious"

static const char * const core_defaults[] = {

 /* general */
 "advance_on_delete", "FALSE",
 "always_resume_paused", "TRUE",
 "clear_playlist", "TRUE",
 "open_to_temporary", "TRUE",
 "resume_playback_on_startup", "TRUE",
 "show_interface", "TRUE",

 /* equalizer */
 "eqpreset_default_file", "",
 "eqpreset_extension", "",
 "equalizer_active", "FALSE",
 "equalizer_autoload", "FALSE",
 "equalizer_bands", "0,0,0,0,0,0,0,0,0,0",
 "equalizer_preamp", "0",

 /* info popup / info window */
 "cover_name_exclude", "back",
 "cover_name_include", "album,cover,front,folder",
 "filepopup_delay", "5",
 "filepopup_showprogressbar", "FALSE",
 "recurse_for_cover", "FALSE",
 "recurse_for_cover_depth", "0",
 "show_filepopup_for_tuple", "TRUE",
 "use_file_cover", "FALSE",

 /* network */
 "net_buffer_kb", "128",
 "use_proxy", "FALSE",
 "use_proxy_auth", "FALSE",

 /* output */
 "default_gain", "0",
 "enable_replay_gain", "TRUE",
 "enable_clipping_prevention", "TRUE",
 "output_bit_depth", "-1",
 "output_buffer_size", "500",
 "record_stream", aud::numeric_string<(int) OutputStream::AfterReplayGain>::str,
 "replay_gain_mode", aud::numeric_string<(int) ReplayGainMode::Track>::str,
 "replay_gain_preamp", "0",
 "soft_clipping", "FALSE",
 "software_volume_control", "FALSE",
 "sw_volume_left", "100",
 "sw_volume_right", "100",

 /* playback */
 "album_shuffle", "FALSE",
 "no_playlist_advance", "FALSE",
 "repeat", "FALSE",
 "shuffle", "FALSE",
 "stop_after_current_song", "FALSE",

 /* playlist */
 "chardet_fallback", "ISO-8859-1",
#ifdef _WIN32
 "convert_backslash", "TRUE",
#else
 "convert_backslash", "FALSE",
#endif
 "generic_title_format", "${?artist:${artist} - }${?album:${album} - }${title}",
 "leading_zero", "FALSE",
 "show_hours", "TRUE",
 "metadata_fallbacks", "TRUE",
 "metadata_on_play", "FALSE",
 "show_numbers_in_pl", "FALSE",
 "slow_probe", "FALSE",

 nullptr};

enum OpType {
    OP_IS_DEFAULT,
    OP_GET,
    OP_SET,
    OP_SET_NO_FLAG,
    OP_CLEAR,
    OP_CLEAR_NO_FLAG
};

struct ConfigItem {
    String section;
    String key;
    String value;
};

struct ConfigNode;

// combined Data and Operation class
struct ConfigOp
{
    OpType type;
    const char * section;
    const char * key;
    String value;
    unsigned hash;
    bool result;

    ConfigNode * add (const ConfigOp *);
    bool found (ConfigNode * node);
};

struct ConfigNode : public MultiHash::Node, public ConfigItem
{
    bool match (const ConfigOp * op) const
        { return ! strcmp (section, op->section) && ! strcmp (key, op->key); }
};

typedef MultiHash_T<ConfigNode, ConfigOp> ConfigTable;

static ConfigTable s_defaults, s_config;
static volatile bool s_modified;

ConfigNode * ConfigOp::add (const ConfigOp *)
{
    switch (type)
    {
    case OP_IS_DEFAULT:
        result = ! value[0]; /* empty string is default */
        return nullptr;

    case OP_SET:
        result = true;
        s_modified = true;
        // fall-through

    case OP_SET_NO_FLAG:
    {
        ConfigNode * node = new ConfigNode;
        node->section = String (section);
        node->key = String (key);
        node->value = value;
        return node;
    }

    default:
        return nullptr;
    }
}

bool ConfigOp::found (ConfigNode * node)
{
    switch (type)
    {
    case OP_IS_DEFAULT:
        result = ! strcmp (node->value, value);
        return false;

    case OP_GET:
        value = node->value;
        return false;

    case OP_SET:
        result = !! strcmp (node->value, value);
        if (result)
            s_modified = true;
        // fall-through

    case OP_SET_NO_FLAG:
        node->value = value;
        return false;

    case OP_CLEAR:
        result = true;
        s_modified = true;
        // fall-through

    case OP_CLEAR_NO_FLAG:
        delete node;
        return true;

    default:
        return false;
    }
}

static bool config_op_run (ConfigOp & op, ConfigTable & table)
{
    if (! op.hash)
        op.hash = str_calc_hash (op.section) + str_calc_hash (op.key);

    op.result = false;
    table.lookup (& op, op.hash, op);
    return op.result;
}

class ConfigParser : public IniParser
{
private:
    String section;

    void handle_heading (const char * heading)
        { section = String (heading); }

    void handle_entry (const char * key, const char * value)
    {
        if (! section)
            return;

        ConfigOp op = {OP_SET_NO_FLAG, section, key, String (value)};
        config_op_run (op, s_config);
    }
};

void config_load ()
{
    StringBuf path = filename_to_uri (aud_get_path (AudPath::UserDir));
    path.insert (-1, "/config");

    if (VFSFile::test_file (path, VFS_EXISTS))
    {
        VFSFile file (path, "r");
        if (file)
            ConfigParser ().parse (file);
    }

    aud_config_set_defaults (nullptr, core_defaults);

    /* migrate from previous versions */
    if (aud_get_bool (0, "replay_gain_album"))
    {
        aud_set_str (0, "replay_gain_album", "");
        aud_set_int (0, "replay_gain_mode", (int) ReplayGainMode::Album);
    }
}

void config_save ()
{
    if (! s_modified)
        return;

    Index<ConfigItem> list;

    s_config.iterate ([&] (ConfigNode * node) {
        list.append (* node);

        s_modified = false;  // must be inside MultiHash lock
        return false;
    });

    list.sort ([] (const ConfigItem & a, const ConfigItem & b) {
        if (a.section == b.section)
            return strcmp (a.key, b.key);
        else
            return strcmp (a.section, b.section);
    });

    StringBuf path = filename_to_uri (aud_get_path (AudPath::UserDir));
    path.insert (-1, "/config");

    String current_heading;

    VFSFile file (path, "w");
    if (! file)
        goto FAILED;

    for (const ConfigItem & item : list)
    {
        if (item.section != current_heading)
        {
            if (! inifile_write_heading (file, item.section))
                goto FAILED;

            current_heading = item.section;
        }

        if (! inifile_write_entry (file, item.key, item.value))
            goto FAILED;
    }

    if (file.fflush () < 0)
        goto FAILED;

    return;

FAILED:
    AUDWARN ("Error saving configuration.\n");
}

EXPORT void aud_config_set_defaults (const char * section, const char * const * entries)
{
    if (! section)
        section = DEFAULT_SECTION;

    while (1)
    {
        const char * name = * entries ++;
        const char * value = * entries ++;
        if (! name || ! value)
            break;

        ConfigOp op = {OP_SET_NO_FLAG, section, name, String (value)};
        config_op_run (op, s_defaults);
    }
}

void config_cleanup ()
{
    s_config.clear ();
    s_defaults.clear ();
}

EXPORT void aud_set_str (const char * section, const char * name, const char * value)
{
    assert (name && value);

    ConfigOp op = {OP_IS_DEFAULT, section ? section : DEFAULT_SECTION, name, String (value)};
    bool is_default = config_op_run (op, s_defaults);

    op.type = is_default ? OP_CLEAR : OP_SET;
    bool changed = config_op_run (op, s_config);

    if (changed && ! section)
        event_queue (str_concat ({"set ", name}), nullptr);
}

EXPORT String aud_get_str (const char * section, const char * name)
{
    assert (name);

    ConfigOp op = {OP_GET, section ? section : DEFAULT_SECTION, name};
    config_op_run (op, s_config);

    if (! op.value)
        config_op_run (op, s_defaults);

    return op.value ? op.value : String ("");
}

EXPORT void aud_set_bool (const char * section, const char * name, bool value)
{
    aud_set_str (section, name, value ? "TRUE" : "FALSE");
}

EXPORT bool aud_get_bool (const char * section, const char * name)
{
    return ! strcmp (aud_get_str (section, name), "TRUE");
}

EXPORT void aud_toggle_bool (const char * section, const char * name)
{
    aud_set_bool (section, name, ! aud_get_bool (section, name));
}

EXPORT void aud_set_int (const char * section, const char * name, int value)
{
    aud_set_str (section, name, int_to_str (value));
}

EXPORT int aud_get_int (const char * section, const char * name)
{
    return str_to_int (aud_get_str (section, name));
}

EXPORT void aud_set_double (const char * section, const char * name, double value)
{
    aud_set_str (section, name, double_to_str (value));
}

EXPORT double aud_get_double (const char * section, const char * name)
{
    return str_to_double (aud_get_str (section, name));
}
