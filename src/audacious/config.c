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

#include <glib.h>
#include <string.h>

#include <libaudcore/audstrings.h>
#include <libaudcore/hook.h>
#include <libaudcore/inifile.h>
#include <libaudcore/multihash.h>

#include "main.h"
#include "misc.h"

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

typedef enum {
    OP_IS_DEFAULT,
    OP_GET,
    OP_SET,
    OP_SET_NO_FLAG,
    OP_CLEAR,
    OP_CLEAR_NO_FLAG
} OpType;

typedef struct {
    const char * section;
    const char * key;
    const char * value;
} ConfigItem;

typedef struct {
    MultihashNode node;
    ConfigItem item;
} ConfigNode;

typedef struct {
    OpType type;
    ConfigItem item;
    unsigned hash;
    bool_t result;
} ConfigOp;

typedef struct {
    char * section;
} LoadState;

typedef struct {
    GArray * list;
} SaveState;

static unsigned item_hash (const ConfigItem * item)
{
    return g_str_hash (item->section) + g_str_hash (item->key);
}

/* assumes pooled strings */
static int item_compare (const ConfigItem * a, const ConfigItem * b)
{
    if (str_equal (a->section, b->section))
        return strcmp (a->key, b->key);
    else
        return strcmp (a->section, b->section);
}

/* assumes pooled strings */
static void item_clear (ConfigItem * item)
{
    str_unref ((char *) item->section);
    str_unref ((char *) item->key);
    str_unref ((char *) item->value);
}

static unsigned config_node_hash (const MultihashNode * node0)
{
    const ConfigNode * node = (const ConfigNode *) node0;

    return item_hash (& node->item);
}

static bool_t config_node_match (const MultihashNode * node0, const void * data, unsigned hash)
{
    const ConfigNode * node = (const ConfigNode *) node0;
    const ConfigItem * item = data;

    return ! strcmp (node->item.section, item->section) && ! strcmp (node->item.key, item->key);
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
    ConfigOp * op = state;

    switch (op->type)
    {
    case OP_IS_DEFAULT:
        op->result = ! op->item.value[0]; /* empty string is default */
        return NULL;

    case OP_SET:
        op->result = TRUE;
        modified = TRUE;

    case OP_SET_NO_FLAG:;
        ConfigNode * node = g_slice_new (ConfigNode);
        node->item.section = str_get (op->item.section);
        node->item.key = str_get (op->item.key);
        node->item.value = str_get (op->item.value);
        return (MultihashNode *) node;

    default:
        return NULL;
    }
}

static bool_t action_cb (MultihashNode * node0, void * state)
{
    ConfigNode * node = (ConfigNode *) node0;
    ConfigOp * op = state;

    switch (op->type)
    {
    case OP_IS_DEFAULT:
        op->result = ! strcmp (node->item.value, op->item.value);
        return FALSE;

    case OP_GET:
        op->item.value = str_ref (node->item.value);
        return FALSE;

    case OP_SET:
        op->result = !! strcmp (node->item.value, op->item.value);
        if (op->result)
            modified = TRUE;

    case OP_SET_NO_FLAG:
        str_unref ((char *) node->item.value);
        node->item.value = str_get (op->item.value);
        return FALSE;

    case OP_CLEAR:
        op->result = TRUE;
        modified = TRUE;

    case OP_CLEAR_NO_FLAG:
        item_clear (& node->item);
        g_slice_free (ConfigNode, node);
        return TRUE;

    default:
        return FALSE;
    }
}

static bool_t config_op_run (ConfigOp * op, OpType type, MultihashTable * table)
{
    if (! op->hash)
        op->hash = item_hash (& op->item);

    op->type = type;
    op->result = FALSE;
    multihash_lookup (table, & op->item, op->hash, add_cb, action_cb, op);
    return op->result;
}

static void load_heading (const char * section, void * data)
{
    LoadState * state = data;

    str_unref (state->section);
    state->section = str_get (section);
}

static void load_entry (const char * key, const char * value, void * data)
{
    LoadState * state = data;
    g_return_if_fail (state->section);

    ConfigOp op = {.item = {state->section, key, value}};
    config_op_run (& op, OP_SET_NO_FLAG, & config);
}

void config_load (void)
{
    char * folder = filename_to_uri (get_path (AUD_PATH_USER_DIR));
    SCONCAT2 (path, folder, "/config");
    str_unref (folder);

    if (vfs_file_test (path, VFS_EXISTS))
    {
        VFSFile * file = vfs_fopen (path, "r");

        if (file)
        {
            LoadState state = {0};

            inifile_parse (file, load_heading, load_entry, & state);

            str_unref (state.section);
            vfs_fclose (file);
        }
    }

    config_set_defaults (NULL, core_defaults);
}

static bool_t add_to_save_list (MultihashNode * node0, void * state0)
{
    ConfigNode * node = (ConfigNode *) node0;
    SaveState * state = state0;

    int pos = state->list->len;
    g_array_set_size (state->list, pos + 1);

    ConfigItem * copy = & g_array_index (state->list, ConfigItem, pos);
    copy->section = str_ref (node->item.section);
    copy->key = str_ref (node->item.key);
    copy->value = str_ref (node->item.value);

    modified = FALSE;

    return FALSE;
}

void config_save (void)
{
    if (! modified)
        return;

    SaveState state = {.list = g_array_new (FALSE, FALSE, sizeof (ConfigItem))};

    multihash_iterate (& config, add_to_save_list, & state);
    g_array_sort (state.list, (GCompareFunc) item_compare);

    char * folder = filename_to_uri (get_path (AUD_PATH_USER_DIR));
    SCONCAT2 (path, folder, "/config");
    str_unref (folder);

    VFSFile * file = vfs_fopen (path, "w");

    if (file)
    {
        const char * current_heading = NULL;

        for (int i = 0; i < state.list->len; i ++)
        {
            ConfigItem * item = & g_array_index (state.list, ConfigItem, i);

            if (! str_equal (item->section, current_heading))
            {
                inifile_write_heading (file, item->section);
                current_heading = item->section;
            }

            inifile_write_entry (file, item->key, item->value);
        }

        vfs_fclose (file);
    }

    g_array_set_clear_func (state.list, (GDestroyNotify) item_clear);
    g_array_free (state.list, TRUE);
}

void config_set_defaults (const char * section, const char * const * entries)
{
    if (! section)
        section = DEFAULT_SECTION;

    while (1)
    {
        const char * name = * entries ++;
        const char * value = * entries ++;
        if (! name || ! value)
            break;

        ConfigOp op = {.item = {section, name, value}};
        config_op_run (& op, OP_SET_NO_FLAG, & defaults);
    }
}

void config_cleanup (void)
{
    ConfigOp op = {.type = OP_CLEAR_NO_FLAG};
    multihash_iterate (& config, action_cb, & op);
    multihash_iterate (& defaults, action_cb, & op);
}

void set_str (const char * section, const char * name, const char * value)
{
    g_return_if_fail (name && value);

    ConfigOp op = {.item = {section ? section : DEFAULT_SECTION, name, value}};

    bool_t is_default = config_op_run (& op, OP_IS_DEFAULT, & defaults);
    bool_t changed = config_op_run (& op, is_default ? OP_CLEAR : OP_SET, & config);

    if (changed && ! section)
    {
        SCONCAT2 (event, "set ", name);
        event_queue (event, NULL);
    }
}

char * get_str (const char * section, const char * name)
{
    g_return_val_if_fail (name, NULL);

    ConfigOp op = {.item = {section ? section : DEFAULT_SECTION, name, NULL}};

    config_op_run (& op, OP_GET, & config);

    if (! op.item.value)
        config_op_run (& op, OP_GET, & defaults);

    return op.item.value ? (char *) op.item.value : str_get ("");
}

void set_bool (const char * section, const char * name, bool_t value)
{
    set_str (section, name, value ? "TRUE" : "FALSE");
}

bool_t get_bool (const char * section, const char * name)
{
    char * string = get_str (section, name);
    bool_t value = ! strcmp (string, "TRUE");
    str_unref (string);
    return value;
}

void set_int (const char * section, const char * name, int value)
{
    char * string = int_to_str (value);
    g_return_if_fail (string);
    set_str (section, name, string);
    str_unref (string);
}

int get_int (const char * section, const char * name)
{
    char * string = get_str (section, name);
    int value = str_to_int (string);
    str_unref (string);
    return value;
}

void set_double (const char * section, const char * name, double value)
{
    char * string = double_to_str (value);
    g_return_if_fail (string);
    set_str (section, name, string);
    str_unref (string);
}

double get_double (const char * section, const char * name)
{
    char * string = get_str (section, name);
    double value = str_to_double (string);
    str_unref (string);
    return value;
}
