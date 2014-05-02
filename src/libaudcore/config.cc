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

#include <glib.h>
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
 "clear_playlist", "TRUE",
 "open_to_temporary", "TRUE",
 "resume_playback_on_startup", "FALSE",
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
 "filepopup_showprogressbar", "TRUE",
 "recurse_for_cover", "FALSE",
 "recurse_for_cover_depth", "0",
 "show_filepopup_for_tuple", "TRUE",
 "use_file_cover", "FALSE",

 /* network */
 "use_proxy", "FALSE",
 "use_proxy_auth", "FALSE",

 /* output */
 "default_gain", "0",
 "enable_replay_gain", "TRUE",
 "enable_clipping_prevention", "TRUE",
 "output_bit_depth", "16",
 "output_buffer_size", "500",
 "replay_gain_album", "FALSE",
 "replay_gain_preamp", "0",
 "soft_clipping", "FALSE",
 "software_volume_control", "FALSE",
 "sw_volume_left", "100",
 "sw_volume_right", "100",

 /* playback */
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
 "metadata_on_play", "FALSE",
 "show_numbers_in_pl", "FALSE",

 NULL};

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

struct ConfigNode {
    MultihashNode node;
    ConfigItem item;
};

struct ConfigOp {
    OpType type;
    const char * section;
    const char * key;
    String value;
    unsigned hash;
    bool_t result;
};

struct LoadState {
    String section;
};

struct SaveState {
    Index<ConfigItem> list;
};

static int item_compare (const ConfigItem & a, const ConfigItem & b, void *)
{
    if (str_equal (a.section, b.section))
        return strcmp (a.key, b.key);
    else
        return strcmp (a.section, b.section);
}

static unsigned config_node_hash (const MultihashNode * node0)
{
    const ConfigNode * node = (const ConfigNode *) node0;

    return g_str_hash (node->item.section) + g_str_hash (node->item.key);
}

static bool_t config_node_match (const MultihashNode * node0, const void * data, unsigned hash)
{
    const ConfigNode * node = (const ConfigNode *) node0;
    const ConfigOp * op = (const ConfigOp *) data;

    return ! strcmp (node->item.section, op->section) && ! strcmp (node->item.key, op->key);
}

static MultihashTable defaults = {
    .hash_func = config_node_hash,
    .match_func = config_node_match
};

static MultihashTable config = {
    .hash_func = config_node_hash,
    .match_func = config_node_match
};

static volatile bool_t modified;

static MultihashNode * add_cb (const void * data, unsigned hash, void * state)
{
    ConfigOp * op = (ConfigOp *) state;

    switch (op->type)
    {
    case OP_IS_DEFAULT:
        op->result = ! op->value[0]; /* empty string is default */
        return NULL;

    case OP_SET:
        op->result = TRUE;
        modified = TRUE;

    case OP_SET_NO_FLAG:
    {
        ConfigNode * node = new ConfigNode;
        node->item.section = String (op->section);
        node->item.key = String (op->key);
        node->item.value = op->value;
        return (MultihashNode *) node;
    }

    default:
        return NULL;
    }
}

static bool_t action_cb (MultihashNode * node0, void * state)
{
    ConfigNode * node = (ConfigNode *) node0;
    ConfigOp * op = (ConfigOp *) state;

    switch (op->type)
    {
    case OP_IS_DEFAULT:
        op->result = ! strcmp (node->item.value, op->value);
        return FALSE;

    case OP_GET:
        op->value = node->item.value;
        return FALSE;

    case OP_SET:
        op->result = !! strcmp (node->item.value, op->value);
        if (op->result)
            modified = TRUE;

    case OP_SET_NO_FLAG:
        node->item.value = op->value;
        return FALSE;

    case OP_CLEAR:
        op->result = TRUE;
        modified = TRUE;

    case OP_CLEAR_NO_FLAG:
        delete node;
        return TRUE;

    default:
        return FALSE;
    }
}

static bool_t config_op_run (ConfigOp * op, MultihashTable * table)
{
    if (! op->hash)
        op->hash = g_str_hash (op->section) + g_str_hash (op->key);

    op->result = FALSE;
    multihash_lookup (table, op, op->hash, add_cb, action_cb, op);
    return op->result;
}

static void load_heading (const char * section, void * data)
{
    LoadState * state = (LoadState *) data;

    state->section = String (section);
}

static void load_entry (const char * key, const char * value, void * data)
{
    LoadState * state = (LoadState *) data;
    g_return_if_fail (state->section);

    ConfigOp op = {OP_SET_NO_FLAG, state->section, key, String (value)};
    config_op_run (& op, & config);
}

void config_load (void)
{
    String folder = filename_to_uri (aud_get_path (AUD_PATH_USER_DIR));
    SCONCAT2 (path, folder, "/config");

    if (vfs_file_test (path, VFS_EXISTS))
    {
        VFSFile * file = vfs_fopen (path, "r");

        if (file)
        {
            LoadState state = LoadState ();

            inifile_parse (file, load_heading, load_entry, & state);

            vfs_fclose (file);
        }
    }

    aud_config_set_defaults (NULL, core_defaults);
}

static bool_t add_to_save_list (MultihashNode * node0, void * state0)
{
    ConfigNode * node = (ConfigNode *) node0;
    SaveState * state = (SaveState *) state0;

    ConfigItem & item = state->list.append ();

    item.section = node->item.section;
    item.key = node->item.key;
    item.value = node->item.value;

    modified = FALSE;

    return FALSE;
}

void config_save (void)
{
    if (! modified)
        return;

    SaveState state = SaveState ();

    multihash_iterate (& config, add_to_save_list, & state);
    state.list.sort (item_compare, nullptr);

    String folder = filename_to_uri (aud_get_path (AUD_PATH_USER_DIR));
    SCONCAT2 (path, folder, "/config");

    VFSFile * file = vfs_fopen (path, "w");

    if (file)
    {
        const char * current_heading = NULL;

        for (const ConfigItem & item : state.list)
        {
            if (! str_equal (item.section, current_heading))
            {
                inifile_write_heading (file, item.section);
                current_heading = item.section;
            }

            inifile_write_entry (file, item.key, item.value);
        }

        vfs_fclose (file);
    }
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
        config_op_run (& op, & defaults);
    }
}

void config_cleanup (void)
{
    ConfigOp op = {.type = OP_CLEAR_NO_FLAG};
    multihash_iterate (& config, action_cb, & op);
    multihash_iterate (& defaults, action_cb, & op);
}

EXPORT void aud_set_str (const char * section, const char * name, const char * value)
{
    g_return_if_fail (name && value);

    ConfigOp op = {OP_IS_DEFAULT, section ? section : DEFAULT_SECTION, name, String (value)};
    bool_t is_default = config_op_run (& op, & defaults);

    op.type = is_default ? OP_CLEAR : OP_SET;
    bool_t changed = config_op_run (& op, & config);

    if (changed && ! section)
    {
        SCONCAT2 (event, "set ", name);
        event_queue (event, NULL);
    }
}

EXPORT String aud_get_str (const char * section, const char * name)
{
    g_return_val_if_fail (name, String ());

    ConfigOp op = {OP_GET, section ? section : DEFAULT_SECTION, name};
    config_op_run (& op, & config);

    if (! op.value)
        config_op_run (& op, & defaults);

    return op.value ? op.value : String ("");
}

EXPORT void aud_set_bool (const char * section, const char * name, bool_t value)
{
    aud_set_str (section, name, value ? "TRUE" : "FALSE");
}

EXPORT bool_t aud_get_bool (const char * section, const char * name)
{
    return ! strcmp (aud_get_str (section, name), "TRUE");
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
