/*  Audacious - Cross-platform multimedia player
 *  Copyright (C) 2005-2009  Audacious development team
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

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#ifndef SHARED_SUFFIX
# define SHARED_SUFFIX G_MODULE_SUFFIX
#endif

#include <glib.h>
#include <glib/gi18n.h>
#include <glib/gprintf.h>
#include <gmodule.h>
#include <string.h>

#include "pluginenum.h"
#include "plugin-registry.h"

#include "audconfig.h"
#include "credits.h"
#include "effect.h"
#include "general.h"
#include "input.h"
#include "main.h"
#include "chardet.h"
#include "output.h"
#include "playback.h"
#include "playlist-new.h"
#include "playlist-utils.h"
#include "probe.h"
#include "audstrings.h"
#include "util.h"
#include "visualization.h"
#include "preferences.h"
#include "vfs_buffer.h"
#include "vfs_buffered_file.h"
#include "equalizer_preset.h"

#include "ui_albumart.h"
#include "ui_plugin_menu.h"
#include "ui_preferences.h"

#include <libaudgui/init.h>


const gchar *plugin_dir_list[] = {
    PLUGINSUBS,
    NULL
};

static void set_pvt_data(Plugin * plugin, gpointer data);
static gpointer get_pvt_data(void);

/*****************************************************************/

static struct _AudaciousFuncTableV1 _aud_papi_v1 = {
    .vfs_fopen = vfs_fopen,
    .vfs_dup = vfs_dup,
    .vfs_fclose = vfs_fclose,
    .vfs_fread = vfs_fread,
    .vfs_fwrite = vfs_fwrite,
    .vfs_getc = vfs_getc,
    .vfs_ungetc = vfs_ungetc,
    .vfs_fgets = vfs_fgets,
    .vfs_feof = vfs_feof,
    .vfs_fprintf = vfs_fprintf,
    .vfs_fseek = vfs_fseek,
    .vfs_rewind = vfs_rewind,
    .vfs_ftell = vfs_ftell,
    .vfs_fsize = vfs_fsize,
    .vfs_ftruncate = vfs_ftruncate,
    .vfs_fget_le16 = vfs_fget_le16,
    .vfs_fget_le32 = vfs_fget_le32,
    .vfs_fget_le64 = vfs_fget_le64,
    .vfs_fget_be16 = vfs_fget_be16,
    .vfs_fget_be32 = vfs_fget_be32,
    .vfs_fget_be64 = vfs_fget_be64,
    .vfs_fput_le16 = vfs_fput_le16,
    .vfs_fput_le32 = vfs_fput_le32,
    .vfs_fput_le64 = vfs_fput_le64,
    .vfs_fput_be16 = vfs_fput_be16,
    .vfs_fput_be32 = vfs_fput_be32,
    .vfs_fput_be64 = vfs_fput_be64,
    .vfs_is_streaming = vfs_is_streaming,
    .vfs_get_metadata = vfs_get_metadata,
    .vfs_file_test = vfs_file_test,
    .vfs_is_writeable = vfs_is_writeable,
    .vfs_is_remote = vfs_is_remote,
    .vfs_file_get_contents = vfs_file_get_contents,
    .vfs_register_transport = vfs_register_transport,

    .vfs_buffer_new = vfs_buffer_new,
    .vfs_buffer_new_from_string = vfs_buffer_new_from_string,

    .vfs_buffered_file_new_from_uri = vfs_buffered_file_new_from_uri,
    .vfs_buffered_file_release_live_fd = vfs_buffered_file_release_live_fd,

    .cfg_db_open = cfg_db_open,
    .cfg_db_close = cfg_db_close,

    .cfg_db_get_string = cfg_db_get_string,
    .cfg_db_get_int = cfg_db_get_int,
    .cfg_db_get_bool = cfg_db_get_bool,
    .cfg_db_get_float = cfg_db_get_float,
    .cfg_db_get_double = cfg_db_get_double,

    .cfg_db_set_string = cfg_db_set_string,
    .cfg_db_set_int = cfg_db_set_int,
    .cfg_db_set_bool = cfg_db_set_bool,
    .cfg_db_set_float = cfg_db_set_float,
    .cfg_db_set_double = cfg_db_set_double,

    .cfg_db_unset_key = cfg_db_unset_key,

    .uri_set_plugin = input_plugin_add_scheme_compat,
    .mime_set_plugin = input_plugin_add_mime_compat,

    .util_add_url_history_entry = util_add_url_history_entry,

    .str_to_utf8 = cd_str_to_utf8,
    .chardet_to_utf8 = cd_chardet_to_utf8,

    .playlist_container_register = playlist_container_register,
    .playlist_container_unregister = playlist_container_unregister,
    .playlist_container_read = playlist_container_read,
    .playlist_container_write = playlist_container_write,
    .playlist_container_find = playlist_container_find,

    .playlist_count = playlist_count,
    .playlist_insert = playlist_insert,
    .playlist_reorder = playlist_reorder,
    .playlist_delete = playlist_delete,

    .playlist_set_filename = playlist_set_filename,
    .playlist_get_filename = playlist_get_filename,
    .playlist_set_title = playlist_set_title,
    .playlist_get_title = playlist_get_title,

    .playlist_set_active = playlist_set_active,
    .playlist_get_active = playlist_get_active,
    .playlist_set_playing = playlist_set_playing,
    .playlist_get_playing = playlist_get_playing,

    .playlist_entry_count = playlist_entry_count,
    .playlist_entry_insert = playlist_entry_insert,
    .playlist_entry_insert_batch = playlist_entry_insert_batch,
    .playlist_entry_delete = playlist_entry_delete,

    .playlist_entry_get_filename = playlist_entry_get_filename,
    .playlist_entry_get_decoder = playlist_entry_get_decoder,
    .playlist_entry_set_tuple = playlist_entry_set_tuple,
    .playlist_entry_get_tuple = playlist_entry_get_tuple,
    .playlist_entry_get_title = playlist_entry_get_title,
    .playlist_entry_get_length = playlist_entry_get_length,

    .playlist_set_position = playlist_set_position,
    .playlist_get_position = playlist_get_position,

    .playlist_entry_set_selected = playlist_entry_set_selected,
    .playlist_entry_get_selected = playlist_entry_get_selected,
    .playlist_selected_count = playlist_selected_count,
    .playlist_select_all = playlist_select_all,

    .playlist_shift = playlist_shift,
    .playlist_shift_selected = playlist_shift_selected,
    .playlist_delete_selected = playlist_delete_selected,
    .playlist_reverse = playlist_reverse,
    .playlist_randomize = playlist_randomize,

    .playlist_sort_by_filename = playlist_sort_by_filename,
    .playlist_sort_by_tuple = playlist_sort_by_tuple,
    .playlist_sort_selected_by_filename = playlist_sort_selected_by_filename,
    .playlist_sort_selected_by_tuple = playlist_sort_selected_by_tuple,

    .playlist_rescan = playlist_rescan,

    .playlist_get_total_length = playlist_get_total_length,
    .playlist_get_selected_length = playlist_get_selected_length,

    .playlist_queue_count = playlist_queue_count,
    .playlist_queue_insert = playlist_queue_insert,
    .playlist_queue_insert_selected = playlist_queue_insert_selected,
    .playlist_queue_get_entry = playlist_queue_get_entry,
    .playlist_queue_find_entry = playlist_queue_find_entry,
    .playlist_queue_delete = playlist_queue_delete,

    .playlist_prev_song = playlist_prev_song,
    .playlist_next_song = playlist_next_song,

    .playlist_update_pending = playlist_update_pending,

    .get_gentitle_format = get_gentitle_format,

    .playlist_sort_by_scheme = playlist_sort_by_scheme,
    .playlist_sort_selected_by_scheme = playlist_sort_selected_by_scheme,
    .playlist_remove_duplicates_by_scheme = playlist_remove_duplicates_by_scheme,
    .playlist_remove_failed = playlist_remove_failed,
    .playlist_select_by_patterns = playlist_select_by_patterns,

    .filename_is_playlist = filename_is_playlist,

    .playlist_insert_playlist = playlist_insert_playlist,
    .playlist_save = playlist_save,
    .playlist_insert_folder = playlist_insert_folder,
    .playlist_insert_folder_v2 = playlist_insert_folder_v2,

    ._cfg = &cfg,

    .hook_associate = hook_associate,
    .hook_dissociate = hook_dissociate,
    .hook_dissociate_full = hook_dissociate_full,
    .hook_register = hook_register,
    .hook_call = hook_call,

    .open_ini_file = open_ini_file,
    .close_ini_file = close_ini_file,
    .read_ini_string = read_ini_string,
    .read_ini_array = read_ini_array,

    .menu_plugin_item_add = menu_plugin_item_add,
    .menu_plugin_item_remove = menu_plugin_item_remove,

    .drct_quit = drct_quit,
    .drct_eject = drct_eject,
    .drct_jtf_show = drct_jtf_show,
    .drct_main_win_is_visible = drct_main_win_is_visible,
    .drct_main_win_toggle = drct_main_win_toggle,
    .drct_eq_win_is_visible = drct_eq_win_is_visible,
    .drct_eq_win_toggle = drct_eq_win_toggle,
    .drct_pl_win_is_visible = drct_pl_win_is_visible,
    .drct_pl_win_toggle = drct_pl_win_toggle,
    .drct_activate = drct_activate,

    .drct_initiate = drct_initiate,
    .drct_play = drct_play,
    .drct_pause = drct_pause,
    .drct_stop = playback_stop,
    .drct_get_playing = drct_get_playing,
    .drct_get_paused = drct_get_paused,
    .drct_get_stopped = drct_get_stopped,
    .drct_get_info = playback_get_info,
    .drct_get_time = playback_get_time,
    .drct_get_length = playback_get_length,
    .drct_seek = playback_seek,
    .drct_get_volume = drct_get_volume,
    .drct_set_volume = drct_set_volume,
    .drct_get_volume_main = drct_get_volume_main,
    .drct_set_volume_main = drct_set_volume_main,
    .drct_get_volume_balance = drct_get_volume_balance,
    .drct_set_volume_balance = drct_set_volume_balance,

    .drct_pl_next = drct_pl_next,
    .drct_pl_prev = drct_pl_prev,
    .drct_pl_repeat_is_enabled = drct_pl_repeat_is_enabled,
    .drct_pl_repeat_toggle = drct_pl_repeat_toggle,
    .drct_pl_repeat_is_shuffled = drct_pl_repeat_is_shuffled,
    .drct_pl_shuffle_toggle = drct_pl_shuffle_toggle,
    .drct_pl_get_title = drct_pl_get_title,
    .drct_pl_get_time = drct_pl_get_time,
    .drct_pl_get_pos = drct_pl_get_pos,
    .drct_pl_get_file = drct_pl_get_file,
    .drct_pl_open = drct_pl_open,
    .drct_pl_open_list = drct_pl_open_list,
    .drct_pl_add = drct_pl_add,
    .drct_pl_clear = drct_pl_clear,
    .drct_pl_get_length = drct_pl_get_length,
    .drct_pl_delete = drct_pl_delete,
    .drct_pl_set_pos = drct_pl_set_pos,
    .drct_pl_ins_url_string = drct_pl_ins_url_string,
    .drct_pl_add_url_string = drct_pl_add_url_string,
    .drct_pl_enqueue_to_temp = drct_pl_enqueue_to_temp,

    .drct_pq_get_length = drct_pq_get_length,
    .drct_pq_add = drct_pq_add,
    .drct_pq_remove = drct_pq_remove,
    .drct_pq_clear = drct_pq_clear,
    .drct_pq_is_queued = drct_pq_is_queued,
    .drct_pq_get_position = drct_pq_get_position,
    .drct_pq_get_queue_position = drct_pq_get_queue_position,

    .util_get_localdir = util_get_localdir,

    .flow_new = flow_new,
    .flow_execute = flow_execute,
    .flow_link_element = flow_link_element,
    .flow_unlink_element = flow_unlink_element,
    .effect_flow = effect_flow,

    .get_output_list = get_output_list,
    .get_effect_list = get_effect_list,
    .enable_effect = effect_enable_plugin,

    .input_get_volume = input_get_volume,
    .construct_uri = construct_uri,
    .uri_to_display_basename = uri_to_display_basename,
    .uri_to_display_dirname = uri_to_display_dirname,

    .get_pvt_data = get_pvt_data,
    .set_pvt_data = set_pvt_data,

    .event_queue = event_queue,
    .event_queue_with_data_free = event_queue_with_data_free,

    .calc_mono_freq = calc_mono_freq,
    .calc_mono_pcm = calc_mono_pcm,
    .calc_stereo_pcm = calc_stereo_pcm,

    .create_widgets_with_domain = create_widgets_with_domain,

    .equalizer_read_presets = equalizer_read_presets,
    .equalizer_write_preset_file = equalizer_write_preset_file,
    .import_winamp_eqf = import_winamp_eqf,
    .save_preset_file = save_preset_file,
    .equalizer_read_aud_preset = equalizer_read_aud_preset,
    .load_preset_file = load_preset_file,

    .file_find_decoder = file_find_decoder,
    .file_read_tuple = file_read_tuple,
    .file_read_image = file_read_image,
    .file_can_write_tuple = file_can_write_tuple,
    .file_write_tuple = file_write_tuple,
    .custom_infowin = custom_infowin,

    .get_plugin_menu = get_plugin_menu,
    .playback_get_title = playback_get_title,
    .save_all_playlists = save_playlists,
    .get_associated_image_file = get_associated_image_file,

    .interface_get_current = interface_get_current,
    .interface_toggle_visibility = interface_toggle_visibility,
    .interface_show_error = interface_show_error_message,

    .get_audacious_credits = get_audacious_credits,

    /* playlist segmentation -- added in Audacious 2.2. */
    .playlist_entry_set_segmentation = playlist_entry_set_segmentation,
    .playlist_entry_is_segmented = playlist_entry_is_segmented,
    .playlist_entry_get_start_time = playlist_entry_get_start_time,
    .playlist_entry_get_end_time = playlist_entry_get_end_time,
};

/*****************************************************************/

extern GList *vfs_transports;

static GStaticPrivate cur_plugin_key = G_STATIC_PRIVATE_INIT;
static mowgli_dictionary_t *pvt_data_dict = NULL;

static mowgli_list_t *headers_list = NULL;

void plugin_set_current(Plugin * plugin)
{
    g_static_private_set(&cur_plugin_key, plugin, NULL);
}

static Plugin *plugin_get_current(void)
{
    return g_static_private_get(&cur_plugin_key);
}

static void set_pvt_data(Plugin * plugin, gpointer data)
{
    mowgli_dictionary_elem_t *elem;
    gchar *base_filename;

    base_filename = g_path_get_basename(plugin->filename);
    elem = mowgli_dictionary_find(pvt_data_dict, base_filename);
    if (elem == NULL)
        mowgli_dictionary_add(pvt_data_dict, base_filename, data);
    else
        elem->data = data;

    g_free(base_filename);
}

static gpointer get_pvt_data(void)
{
    Plugin *cur_p = plugin_get_current();
    gchar *base_filename;
    gpointer result;

    base_filename = g_path_get_basename(cur_p->filename);

    result = mowgli_dictionary_retrieve(pvt_data_dict, base_filename);
    g_free(base_filename);
    return result;
}

static gint effectlist_compare_func(gconstpointer a, gconstpointer b)
{
    const EffectPlugin *ap = a, *bp = b;
    if (ap->description && bp->description)
        return strcasecmp(ap->description, bp->description);
    else
        return 0;
}

static gint generallist_compare_func(gconstpointer a, gconstpointer b)
{
    const GeneralPlugin *ap = a, *bp = b;
    if (ap->description && bp->description)
        return strcasecmp(ap->description, bp->description);
    else
        return 0;
}

static gint vislist_compare_func(gconstpointer a, gconstpointer b)
{
    const VisPlugin *ap = a, *bp = b;
    if (ap->description && bp->description)
        return strcasecmp(ap->description, bp->description);
    else
        return 0;
}

static void input_plugin_init(Plugin * plugin)
{
    InputPlugin *p = INPUT_PLUGIN(plugin);

    input_plugin_set_priority (p, p->priority);

    /* build the extension hash table */
    gint i;
    if (p->vfs_extensions)
    {
        GList * extensions = NULL;

        for (i = 0; p->vfs_extensions[i] != NULL; i++)
            extensions = g_list_prepend (extensions, g_strdup
             (p->vfs_extensions[i]));

        input_plugin_add_keys (p, INPUT_KEY_EXTENSION, extensions);
    }

    if (p->init != NULL)
        p->init ();
}

static void effect_plugin_init(Plugin * plugin)
{
    EffectPlugin *p = EFFECT_PLUGIN(plugin);

    ep_data.effect_list = g_list_append(ep_data.effect_list, p);

    if (p->init != NULL)
        p->init ();
}

static void general_plugin_init(Plugin * plugin)
{
    GeneralPlugin *p = GENERAL_PLUGIN(plugin);

    gp_data.general_list = g_list_append(gp_data.general_list, p);
}

static void vis_plugin_init(Plugin * plugin)
{
    VisPlugin *p = VIS_PLUGIN(plugin);

    p->disable_plugin = vis_disable_plugin;
    vp_data.vis_list = g_list_append(vp_data.vis_list, p);
}

/*******************************************************************/

static void plugin2_dispose(GModule * module, const gchar * str, ...)
{
    gchar *buf;
    va_list va;

    va_start(va, str);
    buf = g_strdup_vprintf(str, va);
    va_end(va);

    g_message("*** %s\n", buf);
    g_free(buf);

    g_module_close(module);
}

void plugin2_process(PluginHeader * header, GModule * module, const gchar * filename)
{
    gint i, n;
    mowgli_node_t *hlist_node;

    if (header->magic != PLUGIN_MAGIC)
    {
        plugin2_dispose (module, "plugin <%s> discarded, invalid module magic",
         filename);
        return;
    }

    if (header->api_version != __AUDACIOUS_PLUGIN_API__)
    {
        plugin2_dispose (module, "plugin <%s> discarded, wanting API version "
         "%d, we implement API version %d", filename, header->api_version,
         __AUDACIOUS_PLUGIN_API__);
        return;
    }

    hlist_node = mowgli_node_create();
    mowgli_node_add(header, hlist_node, headers_list);

    if (header->init)
    {
        plugin_register (filename, PLUGIN_TYPE_BASIC, 0, NULL);
        header->init();
    }

    header->priv_assoc = g_new0(Plugin, 1);
    header->priv_assoc->handle = module;
    header->priv_assoc->filename = g_strdup(filename);

    n = 0;

    if (header->ip_list)
    {
        for (i = 0; (header->ip_list)[i] != NULL; i++, n++)
        {
            plugin_register (filename, PLUGIN_TYPE_INPUT, i, header->ip_list[i]);
            PLUGIN((header->ip_list)[i])->filename = g_strdup_printf("%s (#%d)", filename, n);
            input_plugin_init(PLUGIN((header->ip_list)[i]));
        }
    }

    if (header->op_list)
    {
        for (i = 0; (header->op_list)[i] != NULL; i++, n++)
        {
            OutputPlugin * plugin = header->op_list[i];

            plugin->filename = g_strdup_printf ("%s (#%d)", filename, n);
            plugin_register (filename, PLUGIN_TYPE_OUTPUT, i, plugin);
            output_plugin_set_priority (plugin, plugin->probe_priority);
        }
    }

    if (header->ep_list)
    {
        for (i = 0; (header->ep_list)[i] != NULL; i++, n++)
        {
            plugin_register (filename, PLUGIN_TYPE_EFFECT, i, header->ep_list[i]);
            PLUGIN((header->ep_list)[i])->filename = g_strdup_printf("%s (#%d)", filename, n);
            effect_plugin_init(PLUGIN((header->ep_list)[i]));
        }
    }


    if (header->gp_list)
    {
        for (i = 0; (header->gp_list)[i] != NULL; i++, n++)
        {
            plugin_register (filename, PLUGIN_TYPE_GENERAL, i, header->gp_list[i]);
            PLUGIN((header->gp_list)[i])->filename = g_strdup_printf("%s (#%d)", filename, n);
            general_plugin_init(PLUGIN((header->gp_list)[i]));
        }
    }

    if (header->vp_list)
    {
        for (i = 0; (header->vp_list)[i] != NULL; i++, n++)
        {
            plugin_register (filename, PLUGIN_TYPE_VIS, i, header->vp_list[i]);
            PLUGIN((header->vp_list)[i])->filename = g_strdup_printf("%s (#%d)", filename, n);
            vis_plugin_init(PLUGIN((header->vp_list)[i]));
        }
    }

    if (header->interface)
    {
        plugin_register (filename, PLUGIN_TYPE_IFACE, 0, header->interface);
        interface_register(header->interface);
    }
}

void plugin2_unload(PluginHeader * header, mowgli_node_t * hlist_node)
{
    GModule *module;
    gint i;

    g_return_if_fail(header->priv_assoc != NULL);

    if (header->ip_list != NULL)
    {
        for (i = 0; header->ip_list[i] != NULL; i ++)
        {
            if (header->ip_list[i]->cleanup != NULL)
                header->ip_list[i]->cleanup ();

            g_free (header->ip_list[i]->filename);
        }
    }

    if (header->op_list != NULL)
    {
        for (i = 0; header->op_list[i] != NULL; i ++)
            g_free (header->op_list[i]->filename);
    }

    if (header->interface)
        interface_deregister(header->interface);

    module = header->priv_assoc->handle;

    g_free(header->priv_assoc->filename);
    g_free(header->priv_assoc);

    if (header->fini)
        header->fini();

    mowgli_node_delete(hlist_node, headers_list);
    mowgli_node_free(hlist_node);

    g_module_close(module);
}

/******************************************************************/

void module_load (const gchar * filename)
{
    GModule *module;
    gpointer func;

    g_message("Loaded plugin (%s)", filename);

    if (!(module = g_module_open(filename, G_MODULE_BIND_LAZY | G_MODULE_BIND_LOCAL)))
    {
        printf("Failed to load plugin (%s): %s\n", filename, g_module_error());
        return;
    }

    /* v2 plugin loading */
    if (g_module_symbol(module, "get_plugin_info", &func))
    {
        PluginHeader *(*header_func_p) (struct _AudaciousFuncTableV1 *) = func;
        PluginHeader *header;

        /* this should never happen. */
        g_return_if_fail((header = header_func_p(&_aud_papi_v1)) != NULL);

        plugin2_process(header, module, filename);
        return;
    }

    printf("Invalid plugin (%s)\n", filename);
    g_module_close(module);
}

static gboolean scan_plugin_func(const gchar * path, const gchar * basename, gpointer data)
{
    if (!str_has_suffix_nocase(basename, SHARED_SUFFIX))
        return FALSE;

    if (!g_file_test(path, G_FILE_TEST_IS_REGULAR))
        return FALSE;

    module_register (path);

    return FALSE;
}

static void scan_plugins(const gchar * path)
{
    dir_foreach(path, scan_plugin_func, NULL, NULL);
}

static OutputPlugin * output_load_selected (void)
{
    OutputPlugin * plugin;

    if (cfg.output_path == NULL)
        return NULL;

    plugin = plugin_by_path (cfg.output_path, PLUGIN_TYPE_OUTPUT,
     cfg.output_number);

    if (plugin == NULL || plugin->init () != OUTPUT_PLUGIN_INIT_FOUND_DEVICES)
        return NULL;

    return plugin;
}

static gboolean output_probe_func (OutputPlugin * plugin, void * data)
{
    OutputPlugin * * result = data;

    if (plugin->init () != OUTPUT_PLUGIN_INIT_FOUND_DEVICES)
        return TRUE;

    * result = plugin;
    return FALSE;
}

static OutputPlugin * output_probe (void)
{
    OutputPlugin * plugin = NULL;

    output_plugin_by_priority (output_probe_func, & plugin);

    if (plugin == NULL)
        fprintf (stderr, "ALL OUTPUT PLUGINS FAILED TO INITIALIZE.\n");

    return plugin;
}

void plugin_system_init(void)
{
    gchar *dir;
    GtkWidget *dialog;
    gint dirsel = 0;

    /* give libaudgui its pointer to the API vector table */
    audgui_init (& _aud_papi_v1);

    if (!g_module_supported())
    {
        dialog = gtk_message_dialog_new(NULL, GTK_DIALOG_DESTROY_WITH_PARENT, GTK_MESSAGE_ERROR, GTK_BUTTONS_CLOSE, _("Module loading not supported! Plugins will not be loaded.\n"));
        gtk_dialog_run(GTK_DIALOG(dialog));
        gtk_widget_destroy(dialog);
        return;
    }

    plugin_registry_load ();

    pvt_data_dict = mowgli_dictionary_create(g_ascii_strcasecmp);

    headers_list = mowgli_list_create();

#ifndef DISABLE_USER_PLUGIN_DIR
    scan_plugins(aud_paths[BMP_PATH_USER_PLUGIN_DIR]);
    /*
     * This is in a separate loop so if the user puts them in the
     * wrong dir we'll still get them in the right order (home dir
     * first)                                                - Zinx
     */
    while (plugin_dir_list[dirsel])
    {
        dir = g_build_filename(aud_paths[BMP_PATH_USER_PLUGIN_DIR], plugin_dir_list[dirsel++], NULL);
        scan_plugins(dir);
        g_free(dir);
    }
    dirsel = 0;
#endif

    while (plugin_dir_list[dirsel])
    {
        dir = g_build_filename(PLUGIN_DIR, plugin_dir_list[dirsel++], NULL);
        scan_plugins(dir);
        g_free(dir);
    }

    plugin_registry_prune ();

    ep_data.effect_list = g_list_sort(ep_data.effect_list, effectlist_compare_func);
    ep_data.enabled_list = NULL;

    gp_data.general_list = g_list_sort(gp_data.general_list, generallist_compare_func);
    gp_data.enabled_list = NULL;

    vp_data.vis_list = g_list_sort(vp_data.vis_list, vislist_compare_func);
    vp_data.enabled_list = NULL;

    general_enable_from_stringified_list(cfg.enabled_gplugins);
    vis_enable_from_stringified_list(cfg.enabled_vplugins);
    effect_enable_from_stringified_list(cfg.enabled_eplugins);

    g_free(cfg.enabled_gplugins);
    cfg.enabled_gplugins = NULL;

    g_free(cfg.enabled_vplugins);
    cfg.enabled_vplugins = NULL;

    g_free(cfg.enabled_eplugins);
    cfg.enabled_eplugins = NULL;

    g_free(cfg.enabled_dplugins);
    cfg.enabled_dplugins = NULL;

    current_output_plugin = output_load_selected ();

    if (current_output_plugin == NULL)
        current_output_plugin = output_probe ();
}

void plugin_system_cleanup(void)
{
    EffectPlugin *ep;
    GeneralPlugin *gp;
    VisPlugin *vp;
    GList *node;
    mowgli_node_t *hlist_node;

    g_message("Shutting down plugin system");

    if (playback_get_playing ())
        playback_stop ();

    if (current_output_plugin != NULL)
    {
        if (current_output_plugin->cleanup != NULL)
            current_output_plugin->cleanup ();

        current_output_plugin = NULL;
    }

    plugin_registry_save ();

    for (node = get_effect_list(); node; node = g_list_next(node))
    {
        ep = EFFECT_PLUGIN(node->data);
        if (ep)
        {
            plugin_set_current((Plugin *) ep);

            if (ep->cleanup)
                ep->cleanup();

            g_free (ep->filename);
        }
    }

    if (ep_data.effect_list != NULL)
    {
        g_list_free(ep_data.effect_list);
        ep_data.effect_list = NULL;
    }

    for (node = get_general_enabled_list (); node; node = g_list_next (node))
    {
        gp = GENERAL_PLUGIN(node->data);
        if (gp)
        {
            plugin_set_current((Plugin *) gp);

            if (gp->cleanup)
                gp->cleanup();

            g_free (gp->filename);
        }
    }

    if (gp_data.general_list != NULL)
    {
        g_list_free(gp_data.general_list);
        gp_data.general_list = NULL;
    }

    for (node = get_vis_list(); node; node = g_list_next(node))
    {
        vp = VIS_PLUGIN(node->data);
        if (vp)
        {
            plugin_set_current((Plugin *) vp);

            if (vp->cleanup)
                vp->cleanup();

            g_free (vp->filename);
        }
    }

    if (vp_data.vis_list != NULL)
    {
        g_list_free(vp_data.vis_list);
        vp_data.vis_list = NULL;
    }

    /* XXX: vfs will crash otherwise. -nenolod */
    if (vfs_transports != NULL)
    {
        g_list_free(vfs_transports);
        vfs_transports = NULL;
    }

    MOWGLI_LIST_FOREACH(hlist_node, headers_list->head) plugin2_unload(hlist_node->data, hlist_node);

    mowgli_dictionary_destroy(pvt_data_dict, NULL, NULL);

    mowgli_list_free(headers_list);
}
