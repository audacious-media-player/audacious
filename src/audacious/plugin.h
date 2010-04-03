/*  Audacious
 *  Copyright (C) 2005-2009  Audacious team.
 *
 *  BMP - Cross-platform multimedia player
 *  Copyright (C) 2003-2004  BMP development team.
 *
 *  Based on XMMS:
 *  Copyright (C) 1998-2000  Peter Alm, Mikael Alm, Olle Hallnas, Thomas Nilsson and 4Front Technologies
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */
/**
 * @file plugin.h
 * @brief Main Audacious plugin API header file.
 *
 */
#ifndef AUDACIOUS_PLUGIN_H
#define AUDACIOUS_PLUGIN_H

#include <glib.h>
#include <gtk/gtk.h>

#include "libaudcore/audio.h"
#include "libaudcore/audstrings.h"
#include "libaudcore/index.h"
#include "libaudcore/vfs.h"
#include "libaudcore/tuple.h"
#include "libaudcore/tuple_formatter.h"
#include "audacious/eventqueue.h"
#include "audacious/configdb.h"
#include "audacious/playlist_container.h"
#include "audacious/main.h"
#include "audacious/preferences.h"
#include "audacious/interface.h"
#include "audacious/equalizer_preset.h"
#include "libaudtag/audtag.h"

//@{
/** Plugin type cast macros */
#define PLUGIN(x)               ((Plugin *)(x))
#define INPUT_PLUGIN(x)         ((InputPlugin *)(x))
#define OUTPUT_PLUGIN(x)        ((OutputPlugin *)(x))
#define EFFECT_PLUGIN(x)        ((EffectPlugin *)(x))
#define GENERAL_PLUGIN(x)       ((GeneralPlugin *)(x))
#define VIS_PLUGIN(x)           ((VisPlugin *)(x))
#define LOWLEVEL_PLUGIN(x)      ((LowlevelPlugin *)(x))
//@}

//@{
/** Preprocessor defines for different API features */
#define __AUDACIOUS_NEWVFS__                /**< @deprecated define for availability of VFS API. */
#define __AUDACIOUS_PLUGIN_API__ 13         /**< Current generic plugin API/ABI version, exact match is required for plugin to be loaded. */
#define __AUDACIOUS_INPUT_PLUGIN_API__ 8    /**< Input plugin API version. */
//@}

typedef enum {
    INPUT_VIS_ANALYZER,
    INPUT_VIS_SCOPE,
    INPUT_VIS_VU,
    INPUT_VIS_OFF
} InputVisType;

/** Playlist update signal types */
enum
{
    PLAYLIST_UPDATE_SELECTION = 1,  /**< */
    PLAYLIST_UPDATE_METADATA,       /**< */
    PLAYLIST_UPDATE_STRUCTURE,      /**< */
};

enum {
    PLAYLIST_SORT_PATH,
    PLAYLIST_SORT_FILENAME,
    PLAYLIST_SORT_TITLE,
    PLAYLIST_SORT_ALBUM,
    PLAYLIST_SORT_ARTIST,
    PLAYLIST_SORT_DATE,
    PLAYLIST_SORT_TRACK,
    PLAYLIST_SORT_SCHEMES
};

typedef struct _Plugin        Plugin;
typedef struct _InputPlugin   InputPlugin;
typedef struct _OutputPlugin  OutputPlugin;
typedef struct _EffectPlugin  EffectPlugin;
typedef struct _GeneralPlugin GeneralPlugin;
typedef struct _VisPlugin     VisPlugin;
typedef struct _LowlevelPlugin LowlevelPlugin;

typedef struct _InputPlayback InputPlayback;

/** ReplayGain information structure */
typedef struct {
    gfloat track_gain;  /**< Track gain in decibels (dB) */
    gfloat track_peak;
    gfloat album_gain;
    gfloat album_peak;
} ReplayGainInfo;

typedef GHashTable INIFile;

#include "audacious/input.h"
#include "audacious/hook.h"
#include "audacious/flow.h"

/**
 * The plugin module header. Each module can contain several plugins,
 * of any supported type.
 */
typedef struct {
    gint magic;             /**< Audacious plugin module magic ID */
    gint api_version;       /**< API version plugin has been compiled for,
                                 this is checked against __AUDACIOUS_PLUGIN_API__ */
    gchar *name;            /**< Module name */
    GCallback init;
    GCallback fini;
    Plugin *priv_assoc;
    InputPlugin **ip_list;  /**< List of InputPlugin(s) in this module */
    OutputPlugin **op_list;
    EffectPlugin **ep_list;
    GeneralPlugin **gp_list;
    VisPlugin **vp_list;
    Interface *interface;
} PluginHeader;

#define PLUGIN_MAGIC 0x8EAC8DE2

/* macro for debug print */
#ifdef DEBUG
#  define AUDDBG(...) do { g_print("%s:%d %s(): ", __FILE__, (int)__LINE__, __FUNCTION__); g_print(__VA_ARGS__); } while (0)
#else
#  define AUDDBG(...) do { } while (0)
#endif

/**
 * Audacious plugin API vtable.
 * This table defines the functions available for plugins through
 * Audacious API. Any Audacious functions NOT defined here will not
 * be exported to plugins, and are considered "internal".
 *
 * @attention Only add new functions to the bottom of this list
 * unless API/ABI breakage is planned!
 */
struct _AudaciousFuncTableV1 {

    /* VFS */
    VFSFile *(*vfs_fopen)(const gchar *uri, const gchar *mode);
    gint (*vfs_fclose)(VFSFile *fd);
    VFSFile *(*vfs_dup)(VFSFile *in);
    gsize (*vfs_fread)(gpointer ptr,
                 gsize size,
                 gsize nmemb,
                 VFSFile * file);
    gsize (*vfs_fwrite)(gconstpointer ptr,
                  gsize size,
                  gsize nmemb,
                  VFSFile *file);

    gint (*vfs_getc)(VFSFile *stream);
    gint (*vfs_ungetc)(gint c,
                       VFSFile *stream);
    gchar *(*vfs_fgets)(gchar *s,
                        gint n,
                        VFSFile *stream);

    gint (*vfs_fseek)(VFSFile * file,
                      glong offset,
                      gint whence);
    void (*vfs_rewind)(VFSFile * file);
    glong (*vfs_ftell)(VFSFile * file);
    gboolean (*vfs_feof)(VFSFile * file);

    gboolean (*vfs_file_test)(const gchar * path,
                              GFileTest test);

    gboolean (*vfs_is_writeable)(const gchar * path);
    gboolean (*vfs_truncate)(VFSFile * file, glong length);
    off_t (*vfs_fsize)(VFSFile * file);
    gchar *(*vfs_get_metadata)(VFSFile * file, const gchar * field);

    int (*vfs_fprintf)(VFSFile *stream, gchar const *format, ...)
        __attribute__ ((__format__ (__printf__, 2, 3)));

    gboolean (*vfs_register_transport)(VFSConstructor *vtable);
    void (*vfs_file_get_contents)(const gchar *filename, gchar **buf, gsize *size);
    gboolean (*vfs_is_remote)(const gchar * path);
    gboolean (*vfs_is_streaming)(VFSFile *file);

    /* VFS Buffer */
    VFSFile *(*vfs_buffer_new)(gpointer data, gsize size);
    VFSFile *(*vfs_buffer_new_from_string)(gchar *str);

    /* VFS Buffered File */
    VFSFile *(*vfs_buffered_file_new_from_uri)(const gchar *uri);
    VFSFile *(*vfs_buffered_file_release_live_fd)(VFSFile *fd);

    /* VFS endianess helper functions */
    gboolean (*vfs_fget_le16)(guint16 *value, VFSFile *stream);
    gboolean (*vfs_fget_le32)(guint32 *value, VFSFile *stream);
    gboolean (*vfs_fget_le64)(guint64 *value, VFSFile *stream);
    gboolean (*vfs_fget_be16)(guint16 *value, VFSFile *stream);
    gboolean (*vfs_fget_be32)(guint32 *value, VFSFile *stream);
    gboolean (*vfs_fget_be64)(guint64 *value, VFSFile *stream);

    gboolean (*vfs_fput_le16)(guint16 value, VFSFile *stream);
    gboolean (*vfs_fput_le32)(guint32 value, VFSFile *stream);
    gboolean (*vfs_fput_le64)(guint64 value, VFSFile *stream);
    gboolean (*vfs_fput_be16)(guint16 value, VFSFile *stream);
    gboolean (*vfs_fput_be32)(guint32 value, VFSFile *stream);
    gboolean (*vfs_fput_be64)(guint64 value, VFSFile *stream);

    /* ConfigDb */
    mcs_handle_t *(*cfg_db_open)(void);
    void (*cfg_db_close)(mcs_handle_t *db);

    gboolean (*cfg_db_get_string)(mcs_handle_t *db,
                                  const gchar *section,
                                  const gchar *key,
                                  gchar **value);
    gboolean (*cfg_db_get_int)(mcs_handle_t *db,
                               const gchar *section,
                               const gchar *key,
                               gint *value);
    gboolean (*cfg_db_get_bool)(mcs_handle_t *db,
                                const gchar *section,
                                const gchar *key,
                                gboolean *value);
    gboolean (*cfg_db_get_float)(mcs_handle_t *db,
                                 const gchar *section,
                                 const gchar *key,
                                 gfloat *value);
    gboolean (*cfg_db_get_double)(mcs_handle_t *db,
                                  const gchar *section,
                                  const gchar *key,
                                  gdouble *value);

    void (*cfg_db_set_string)(mcs_handle_t *db,
                              const gchar *section,
                              const gchar *key,
                              const gchar *value);
    void (*cfg_db_set_int)(mcs_handle_t *db,
                           const gchar *section,
                           const gchar *key,
                           gint value);
    void (*cfg_db_set_bool)(mcs_handle_t *db,
                            const gchar *section,
                            const gchar *key,
                            gboolean value);
    void (*cfg_db_set_float)(mcs_handle_t *db,
                             const gchar *section,
                             const gchar *key,
                             gfloat value);
    void (*cfg_db_set_double)(mcs_handle_t *db,
                              const gchar *section,
                              const gchar *key,
                              gdouble value);

    void (*cfg_db_unset_key)(mcs_handle_t *db,
                             const gchar *section,
                             const gchar *key);

    /* Plugin registry */
    void (* uri_set_plugin) (const gchar * uri, InputPlugin * ip);
    void (* mime_set_plugin) (const gchar * mimetype, InputPlugin * ip);

    /* Util funcs */
    GtkWidget *(*util_info_dialog)(const gchar * title, const gchar * text,
                                   const gchar * button_text, gboolean modal,
                                   GCallback button_action,
                                   gpointer action_data);
    gchar *(*util_get_localdir)(void);
    void (*util_menu_main_show)(gint x, gint y, guint button, guint time);

    gpointer (*smart_realloc)(gpointer ptr, gsize *size);
    void (*util_add_url_history_entry)(const gchar * url);

    /* INI funcs */
    INIFile *(*open_ini_file)(const gchar *filename);
    void (*close_ini_file)(INIFile *key_file);
    gchar *(*read_ini_string)(INIFile *key_file, const gchar *section,
                                           const gchar *key);
    GArray *(*read_ini_array)(INIFile *key_file, const gchar *section,
                       const gchar *key);


    /* strings API */
    gchar *(*str_to_utf8)(const gchar * str);
    gchar *(*chardet_to_utf8)(const gchar *str, gssize len,
                       gsize *arg_bytes_read, gsize *arg_bytes_write,
                       GError **arg_error);

    /* PlaylistContainer API. */
    void (*playlist_container_register)(PlaylistContainer *plc);
    void (*playlist_container_unregister)(PlaylistContainer *plc);
    void (*playlist_container_read)(gchar *filename, gint pos);
    void (*playlist_container_write)(gchar *filename, gint pos);
    PlaylistContainer *(*playlist_container_find)(gchar *ext);

    /* Playlist API II (core) */
    gint (* playlist_count) (void);
    void (* playlist_insert) (gint at);
    void (* playlist_delete) (gint playlist);

    void (* playlist_set_filename) (gint playlist, const gchar * filename);
    const gchar * (* playlist_get_filename) (gint playlist);
    void (* playlist_set_title) (gint playlist, const gchar * title);
    const gchar * (* playlist_get_title) (gint playlist);

    void (* playlist_set_active) (gint playlist);
    gint (* playlist_get_active) (void);
    void (* playlist_set_playing) (gint playlist);
    gint (* playlist_get_playing) (void);

    gint (* playlist_entry_count) (gint playlist);
    void (* playlist_entry_insert) (gint playlist, gint at, gchar * filename,
     Tuple * tuple);
    void (* playlist_entry_insert_batch) (gint playlist, gint at, struct index *
     filenames, struct index * tuples);
    void (* playlist_entry_delete) (gint playlist, gint at, gint number);

    const gchar * (* playlist_entry_get_filename) (gint playlist, gint entry);
    InputPlugin * (* playlist_entry_get_decoder) (gint playlist, gint entry);
    void (* playlist_entry_set_tuple) (gint playlist, gint entry, Tuple * tuple);
    const Tuple * (* playlist_entry_get_tuple) (gint playlist, gint entry);
    const gchar * (* playlist_entry_get_title) (gint playlist, gint entry);
    gint (* playlist_entry_get_length) (gint playlist, gint entry);

    void (* playlist_set_position) (gint playlist, gint position);
    gint (* playlist_get_position) (gint playlist);

    void (* playlist_entry_set_selected) (gint playlist, gint entry, gboolean
     selected);
    gboolean (* playlist_entry_get_selected) (gint playlist, gint entry);
    gint (* playlist_selected_count) (gint playlist);
    void (* playlist_select_all) (gint playlist, gboolean selected);

    gint (* playlist_shift) (gint playlist, gint position, gint distance);
    gint (* playlist_shift_selected) (gint playlist, gint distance);
    void (* playlist_delete_selected) (gint playlist);
    void (* playlist_reverse) (gint playlist);
    void (* playlist_randomize) (gint playlist);

    void (* playlist_sort_by_filename) (gint playlist, gint (* compare) (const
     gchar * a, const gchar * b));
    void (* playlist_sort_by_tuple) (gint playlist, gint (* compare) (const
     Tuple * a, const Tuple * b));
    void (* playlist_sort_selected_by_filename) (gint playlist, gint (* compare)
     (const gchar * a, const gchar * b));
    void (* playlist_sort_selected_by_tuple) (gint playlist, gint (* compare)
     (const Tuple * a, const Tuple * b));

    void (* playlist_rescan) (gint playlist);

    gint64 (* playlist_get_total_length) (gint playlist);
    gint64 (* playlist_get_selected_length) (gint playlist);

    void (* playlist_set_shuffle) (gboolean shuffle);

    gint (* playlist_queue_count) (gint playlist);
    void (* playlist_queue_insert) (gint playlist, gint at, gint entry);
    void (* playlist_queue_insert_selected) (gint playlist, gint at);
    gint (* playlist_queue_get_entry) (gint playlist, gint at);
    gint (* playlist_queue_find_entry) (gint playlist, gint entry);
    void (* playlist_queue_delete) (gint playlist, gint at, gint number);

    gboolean (* playlist_prev_song) (gint playlist);
    gboolean (* playlist_next_song) (gint playlist, gboolean repeat);

    gboolean (* playlist_update_pending) (void);

    /* Playlist API II (extra) */
    const gchar * (* get_gentitle_format) (void);

    void (* playlist_sort_by_scheme) (gint playlist, gint scheme);
    void (* playlist_sort_selected_by_scheme) (gint playlist, gint scheme);
    void (* playlist_remove_duplicates_by_scheme) (gint playlist, gint scheme);
    void (* playlist_remove_failed) (gint playlist);
    void (* playlist_select_by_patterns) (gint playlist, const Tuple * patterns);

    gboolean (* filename_is_playlist) (const gchar * filename);

    gboolean (* playlist_insert_playlist) (gint playlist, gint at, const gchar *
     filename);
    gboolean (* playlist_save) (gint playlist, const gchar * filename);
    void (* playlist_insert_folder) (gint playlist, gint at, const gchar *
     folder);
    void (* save_all_playlists) (void);

    /* state vars */
    AudConfig *_cfg;

    /* hook API */
    void (*hook_register)(const gchar *name);
    gint (*hook_associate)(const gchar *name, HookFunction func, gpointer user_data);
    gint (*hook_dissociate)(const gchar *name, HookFunction func);
    gint (*hook_dissociate_full)(const gchar *name, HookFunction func, gpointer user_data);
    void (*hook_call)(const gchar *name, gpointer hook_data);

    /* PluginMenu API */
    gint (*menu_plugin_item_add)(gint, GtkWidget *);
    gint (*menu_plugin_item_remove)(gint, GtkWidget *);

    /* DRCT API. */
    void (*drct_quit) ( void );
    void (*drct_eject) ( void );
    void (*drct_jtf_show) ( void );
    gboolean (*drct_main_win_is_visible)( void );
    void (*drct_main_win_toggle) ( gboolean );
    gboolean (*drct_eq_win_is_visible)( void );
    void (*drct_eq_win_toggle) ( gboolean );
    gboolean (*drct_pl_win_is_visible)( void );
    void (*drct_pl_win_toggle) ( gboolean );
    void (*drct_set_skin)(gchar *skinfile);
    void (*drct_activate)(void);

    /* DRCT API: playback */
    void (*drct_initiate) ( void );
    void (*drct_play) ( void );
    void (*drct_pause) ( void );
    void (*drct_stop) ( void );
    gboolean (*drct_get_playing)( void );
    gboolean (*drct_get_paused)( void );
    gboolean (*drct_get_stopped)( void );
    void (*drct_get_info)( gint *rate, gint *freq, gint *nch);
    gint (*drct_get_time )( void );
    gint (*drct_get_length )( void );
    void (* drct_seek) (gint pos);
    void (*drct_get_volume)( gint *vl, gint *vr );
    void (*drct_set_volume)( gint vl, gint vr );
    void (*drct_get_volume_main)( gint *v );
    void (*drct_set_volume_main)( gint v );
    void (*drct_get_volume_balance)( gint *b );
    void (*drct_set_volume_balance)( gint b );

    /* DRCT API: playlist */
    void (*drct_pl_next)( void );
    void (*drct_pl_prev)( void );
    gboolean (*drct_pl_repeat_is_enabled)( void );
    void (*drct_pl_repeat_toggle) ( void );
    gboolean (*drct_pl_repeat_is_shuffled)( void );
    void (*drct_pl_shuffle_toggle) ( void );
    gchar *(*drct_pl_get_title)( gint pos );
    gint (*drct_pl_get_time)( gint pos );
    gint (*drct_pl_get_pos)( void );
    gchar *(*drct_pl_get_file)( gint pos );
    void (* drct_pl_open) (const gchar * filename);
    void (* drct_pl_open_list) (GList * list);
    void (*drct_pl_add) ( GList * list );
    void (*drct_pl_clear) ( void );
    gint (*drct_pl_get_length)( void );
    void (*drct_pl_delete) ( gint pos );
    void (*drct_pl_set_pos)( gint pos );
    void (* drct_pl_ins_url_string) (const gchar * string, gint pos);
    void (* drct_pl_add_url_string) (const gchar * string);
    void (* drct_pl_enqueue_to_temp) (const gchar * filename);

    /* DRCT API: playqueue */
    gint (*drct_pq_get_length)( void );
    void (*drct_pq_add)( gint pos );
    void (*drct_pq_remove)( gint pos );
    void (*drct_pq_clear)( void );
    gboolean (*drct_pq_is_queued)( gint pos );
    gint (*drct_pq_get_position)( gint pos );
    gint (*drct_pq_get_queue_position)( gint pos );

    /* FileInfoPopup API */
    GtkWidget *(*fileinfopopup_create)(void);
    void (*fileinfopopup_destroy)(GtkWidget* fileinfopopup_win);
    void (*fileinfopopup_show_from_tuple)(GtkWidget *fileinfopopup_win, Tuple *tuple);
    void (*fileinfopopup_show_from_title)(GtkWidget *fileinfopopup_win, gchar *title);
    void (*fileinfopopup_hide)(GtkWidget *filepopup_win, gpointer unused);

    /* InputPlayback */
    InputPlayback *(*playback_new)(void);
    void (*playback_free)(InputPlayback *);
    void (*playback_run)(InputPlayback *);

    /* Flows */
    gsize (*flow_execute)(Flow *flow, gint time, gpointer *data, gsize len, AFormat fmt,
                          gint srate, gint channels);
    Flow *(*flow_new)(void);
    void (*flow_link_element)(Flow *flow, FlowFunction func);
    void (*flow_unlink_element)(Flow *flow, FlowFunction func);
    void (*effect_flow)(FlowContext *context);

    GList *(*get_output_list)(void);
    GList * (* get_effect_list) (void);
    void (* enable_effect) (EffectPlugin * effect, gboolean enable);

    void (*input_get_volume)(gint * l, gint * r);

    gchar *(*construct_uri)(gchar *string, const gchar *playlist_name);
    gchar *(*uri_to_display_basename)(const gchar * uri);
    gchar *(*uri_to_display_dirname)(const gchar * uri);

    void (*set_pvt_data)(Plugin * plugin, gpointer data);
    gpointer (*get_pvt_data)(void);

    void (*event_queue)(const gchar *name, gpointer user_data);
    void (*event_queue_with_data_free)(const gchar *name, gpointer user_data);

    void (*calc_mono_freq)(gint16 dest[2][256], gint16 src[2][512], gint nch);
    void (*calc_mono_pcm)(gint16 dest[2][512], gint16 src[2][512], gint nch);
    void (*calc_stereo_pcm)(gint16 dest[2][512], gint16 src[2][512], gint nch);

    void (* create_widgets_with_domain) (GtkBox * box, PreferencesWidget *
     widgets, gint amt, const gchar * domain);

    GList *(*equalizer_read_presets)(const gchar * basename);
    void (*equalizer_write_preset_file)(GList * list, const gchar * basename);
    GList *(*import_winamp_eqf)(VFSFile * file);
    void (*save_preset_file)(EqualizerPreset *preset, const gchar * filename);
    EqualizerPreset *(*equalizer_read_aud_preset)(const gchar * filename);
    EqualizerPreset *(*load_preset_file)(const gchar *filename);

//    /* Audtag lib functions */
//    Tuple *(*tag_tuple_read)(Tuple* tuple);
//    gint (*tag_tuple_write_to_file)(Tuple *tuple);

    /* Miscellaneous */
    GtkWidget * (* get_plugin_menu) (gint id);
    gchar * (* playback_get_title) (void);
    void (* fileinfo_show) (gint playlist, gint entry);
    void (* fileinfo_show_current) (void);

    /* Interface API */
    const Interface * (* interface_get_current) (void);
    void (* interface_toggle_visibility) (void);
    void (* interface_show_error) (const gchar * markup);

    void (* get_audacious_credits)(const gchar **brief, const gchar *** credits, const gchar *** translators);

    void (* playlist_entry_set_segmentation)(gint playlist_num, gint entry_num, gint start, gint end);
    gboolean (* playlist_entry_is_segmented)(gint playlist_num, gint entry_num);
    gint (* playlist_entry_get_start_time)(gint playlist_num, gint entry_num);
    gint (* playlist_entry_get_end_time)(gint playlist_num, gint entry_num);

    /* Move to proper place when API can be broken */
    void (* playlist_insert_folder_v2) (gint playlist, gint at, const gchar *
     folder, gboolean play);
};


/* Convenience macros for accessing the public API. */
/*    public name            vtable mapping      */
#define aud_vfs_fopen                   _audvt->vfs_fopen
#define aud_vfs_fclose                  _audvt->vfs_fclose
#define aud_vfs_dup                     _audvt->vfs_dup
#define aud_vfs_fread                   _audvt->vfs_fread
#define aud_vfs_fwrite                  _audvt->vfs_fwrite
#define aud_vfs_getc                    _audvt->vfs_getc
#define aud_vfs_ungetc                  _audvt->vfs_ungetc
#define aud_vfs_fgets                   _audvt->vfs_fgets
#define aud_vfs_fseek                   _audvt->vfs_fseek
#define aud_vfs_rewind                  _audvt->vfs_rewind
#define aud_vfs_ftell                   _audvt->vfs_ftell
#define aud_vfs_feof                    _audvt->vfs_feof
#define aud_vfs_file_test               _audvt->vfs_file_test
#define aud_vfs_is_writeable            _audvt->vfs_is_writeable
#define aud_vfs_truncate                _audvt->vfs_truncate
#define aud_vfs_fsize                   _audvt->vfs_fsize
#define aud_vfs_get_metadata            _audvt->vfs_get_metadata
#define aud_vfs_fprintf                 _audvt->vfs_fprintf
#define aud_vfs_register_transport      _audvt->vfs_register_transport
#define aud_vfs_file_get_contents       _audvt->vfs_file_get_contents
#define aud_vfs_is_remote               _audvt->vfs_is_remote
#define aud_vfs_is_streaming            _audvt->vfs_is_streaming

#define aud_vfs_buffer_new              _audvt->vfs_buffer_new
#define aud_vfs_buffer_new_from_string  _audvt->vfs_buffer_new_from_string

#define aud_vfs_buffered_file_new_from_uri      _audvt->vfs_buffered_file_new_from_uri
#define aud_vfs_buffered_file_release_live_fd   _audvt->vfs_buffered_file_release_live_fd

#define aud_vfs_fget_le16               _audvt->vfs_fget_le16
#define aud_vfs_fget_le32               _audvt->vfs_fget_le32
#define aud_vfs_fget_le64               _audvt->vfs_fget_le64
#define aud_vfs_fget_be16               _audvt->vfs_fget_be16
#define aud_vfs_fget_be32               _audvt->vfs_fget_be32
#define aud_vfs_fget_be64               _audvt->vfs_fget_be64

#define aud_vfs_fput_le16               _audvt->vfs_fput_le16
#define aud_vfs_fput_le32               _audvt->vfs_fput_le32
#define aud_vfs_fput_le64               _audvt->vfs_fput_le64
#define aud_vfs_fput_be16               _audvt->vfs_fput_be16
#define aud_vfs_fput_be32               _audvt->vfs_fput_be32
#define aud_vfs_fput_be64               _audvt->vfs_fput_be64

/* XXX: deprecation warnings */
#define ConfigDb mcs_handle_t        /* Alias for compatibility -- ccr */
#define aud_cfg_db_open                 _audvt->cfg_db_open
#define aud_cfg_db_close                _audvt->cfg_db_close
#define aud_cfg_db_set_string           _audvt->cfg_db_set_string
#define aud_cfg_db_set_int              _audvt->cfg_db_set_int
#define aud_cfg_db_set_bool             _audvt->cfg_db_set_bool
#define aud_cfg_db_set_float            _audvt->cfg_db_set_float
#define aud_cfg_db_set_double           _audvt->cfg_db_set_double
#define aud_cfg_db_get_string           _audvt->cfg_db_get_string
#define aud_cfg_db_get_int              _audvt->cfg_db_get_int
#define aud_cfg_db_get_bool             _audvt->cfg_db_get_bool
#define aud_cfg_db_get_float            _audvt->cfg_db_get_float
#define aud_cfg_db_get_double           _audvt->cfg_db_get_double
#define aud_cfg_db_unset_key            _audvt->cfg_db_unset_key

/* These functions are in libaudcore; macros here for compatibility. */
#define aud_tuple_new                   tuple_new
#define aud_tuple_new_from_filename     tuple_new_from_filename
#define aud_tuple_associate_string      tuple_associate_string
#define aud_tuple_associate_string_rel  tuple_associate_string_rel
#define aud_tuple_associate_int         tuple_associate_int
#define aud_tuple_disassociate          tuple_disassociate
#define aud_tuple_disassociate_now      tuple_disassociate_now
#define aud_tuple_get_value_type        tuple_get_value_type
#define aud_tuple_get_string            tuple_get_string
#define aud_tuple_get_int               tuple_get_int
#define aud_tuple_free                  mowgli_object_unref

/* These functions are in libaudcore; macros here for compatibility. */
#define aud_tuple_formatter_process_string       tuple_formatter_process_string
#define aud_tuple_formatter_make_title_string    tuple_formatter_make_title_string
#define aud_tuple_formatter_register_expression  tuple_formatter_register_expression
#define aud_tuple_formatter_register_function    tuple_formatter_register_function
#define aud_tuple_formatter_process_expr         tuple_formatter_process_expr
#define aud_tuple_formatter_process_function     tuple_formatter_process_function
#define aud_tuple_formatter_process_construct    tuple_formatter_process_construct

#define aud_mime_set_plugin             _audvt->mime_set_plugin
#define aud_uri_set_plugin              _audvt->uri_set_plugin

#define aud_info_dialog                 _audvt->util_info_dialog
#define audacious_info_dialog           _audvt->util_info_dialog
#define aud_smart_realloc               _audvt->smart_realloc
#define aud_util_add_url_history_entry  _audvt->util_add_url_history_entry

#define aud_str_to_utf8                 _audvt->str_to_utf8
#define aud_chardet_to_utf8             _audvt->chardet_to_utf8

/* These functions are in libaudcore; macros here for compatibility. */
#define aud_escape_shell_chars          escape_shell_chars
#define aud_str_append                  str_append
#define aud_str_replace                 str_replace
#define aud_str_replace_in              str_replace_in
#define aud_str_has_prefix_nocase       str_has_prefix_nocase
#define aud_str_has_suffix_nocase       str_has_suffix_nocase
#define aud_str_has_suffixes_nocase     str_has_suffixes_nocase
#define aud_str_to_utf8_fallback        str_to_utf8_fallback
#define aud_filename_to_utf8            filename_to_utf8
#define aud_str_skip_chars              str_skip_chars
#define aud_filename_split_subtune      filename_split_subtune

#define aud_playlist_container_register     _audvt->playlist_container_register
#define aud_playlist_container_unregister   _audvt->playlist_container_unregister
#define aud_playlist_container_read         _audvt->playlist_container_read
#define aud_playlist_container_write        _audvt->playlist_container_write
#define aud_playlist_container_find         _audvt->playlist_container_find

#define aud_playlist_count              _audvt->playlist_count
#define aud_playlist_insert             _audvt->playlist_insert
#define aud_playlist_delete             _audvt->playlist_delete

#define aud_playlist_set_filename       _audvt->playlist_set_filename
#define aud_playlist_get_filename       _audvt->playlist_get_filename
#define aud_playlist_set_title          _audvt->playlist_set_title
#define aud_playlist_get_title          _audvt->playlist_get_title

#define aud_playlist_set_active         _audvt->playlist_set_active
#define aud_playlist_get_active         _audvt->playlist_get_active
#define aud_playlist_set_playing        _audvt->playlist_set_playing
#define aud_playlist_get_playing        _audvt->playlist_get_playing

#define aud_playlist_entry_count        _audvt->playlist_entry_count
#define aud_playlist_entry_insert       _audvt->playlist_entry_insert
#define aud_playlist_entry_insert_batch _audvt->playlist_entry_insert_batch
#define aud_playlist_entry_delete       _audvt->playlist_entry_delete

#define aud_playlist_entry_get_filename _audvt->playlist_entry_get_filename
#define aud_playlist_entry_get_decoder  _audvt->playlist_entry_get_decoder
#define aud_playlist_entry_set_tuple    _audvt->playlist_entry_set_tuple
#define aud_playlist_entry_get_tuple    _audvt->playlist_entry_get_tuple
#define aud_playlist_entry_get_title    _audvt->playlist_entry_get_title
#define aud_playlist_entry_get_length   _audvt->playlist_entry_get_length

#define aud_playlist_set_position       _audvt->playlist_set_position
#define aud_playlist_get_position       _audvt->playlist_get_position

#define aud_playlist_entry_set_selected _audvt->playlist_entry_set_selected
#define aud_playlist_entry_get_selected _audvt->playlist_entry_get_selected
#define aud_playlist_selected_count     _audvt->playlist_selected_count
#define aud_playlist_select_all         _audvt->playlist_select_all

#define aud_playlist_shift              _audvt->playlist_shift
#define aud_playlist_shift_selected     _audvt->playlist_shift_selected
#define aud_playlist_delete_selected    _audvt->playlist_delete_selected
#define aud_playlist_reverse            _audvt->playlist_reverse
#define aud_playlist_randomize          _audvt->playlist_randomize

#define aud_playlist_sort_by_filename   _audvt->playlist_sort_by_filename
#define aud_playlist_sort_by_tuple      _audvt->playlist_sort_by_tuple
#define aud_playlist_sort_selected_by_filename \
    _audvt->playlist_sort_selected_by_filename
#define aud_playlist_sort_selected_by_tuple \
    _audvt->playlist_sort_selected_by_tuple

#define aud_playlist_rescan             _audvt->playlist_rescan
#define aud_playlist_get_total_length   _audvt->playlist_get_total_length
#define aud_playlist_get_selected_length _audvt->playlist_get_selected_length
#define aud_playlist_set_shuffle        _audvt->playlist_set_shuffle

#define aud_playlist_queue_count        _audvt->playlist_queue_count
#define aud_playlist_queue_insert       _audvt->playlist_queue_insert
#define aud_playlist_queue_insert_selected \
    _audvt->playlist_queue_insert_selected
#define aud_playlist_queue_get_entry    _audvt->playlist_queue_get_entry
#define aud_playlist_queue_find_entry   _audvt->playlist_queue_find_entry
#define aud_playlist_queue_delete       _audvt->playlist_queue_delete

#define aud_playlist_prev_song          _audvt->playlist_prev_song
#define aud_playlist_next_song          _audvt->playlist_next_song

#define aud_playlist_update_pending     _audvt->playlist_update_pending

#define aud_get_gentitle_format         _audvt->get_gentitle_format

#define aud_playlist_sort_by_scheme     _audvt->playlist_sort_by_scheme
#define aud_playlist_sort_selected_by_scheme \
    _audvt->playlist_sort_selected_by_scheme
#define aud_playlist_remove_duplicates_by_scheme \
 _audvt->playlist_remove_duplicates_by_scheme
#define aud_playlist_remove_failed      _audvt->playlist_remove_failed
#define aud_playlist_select_by_patterns _audvt->playlist_select_by_patterns

#define aud_filename_is_playlist        _audvt->filename_is_playlist

#define aud_playlist_insert_playlist    _audvt->playlist_insert_playlist
#define aud_playlist_save               _audvt->playlist_save
#define aud_playlist_insert_folder      _audvt->playlist_insert_folder
#define aud_playlist_insert_folder_v2   _audvt->playlist_insert_folder_v2

#define aud_cfg                         _audvt->_cfg

#define aud_hook_associate              _audvt->hook_associate
#define aud_hook_dissociate             _audvt->hook_dissociate
#define aud_hook_dissociate_full        _audvt->hook_dissociate_full
#define aud_hook_register               _audvt->hook_register
#define aud_hook_call                   _audvt->hook_call

#define aud_open_ini_file               _audvt->open_ini_file
#define aud_close_ini_file              _audvt->close_ini_file
#define aud_read_ini_string             _audvt->read_ini_string
#define aud_read_ini_array              _audvt->read_ini_array

#define audacious_menu_plugin_item_add  _audvt->menu_plugin_item_add
#define audacious_menu_plugin_item_remove _audvt->menu_plugin_item_remove
#define aud_menu_plugin_item_add        _audvt->menu_plugin_item_add
#define aud_menu_plugin_item_remove     _audvt->menu_plugin_item_remove

#define audacious_drct_quit                 _audvt->drct_quit
#define audacious_drct_eject                _audvt->drct_eject
#define audacious_drct_jtf_show             _audvt->drct_jtf_show
#define audacious_drct_main_win_is_visible  _audvt->drct_main_win_is_visible
#define audacious_drct_main_win_toggle      _audvt->drct_main_win_toggle
#define audacious_drct_eq_win_is_visible    _audvt->drct_eq_win_is_visible
#define audacious_drct_eq_win_toggle        _audvt->drct_eq_win_toggle
#define audacious_drct_pl_win_is_visible    _audvt->drct_pl_win_is_visible
#define audacious_drct_pl_win_toggle        _audvt->drct_pl_win_toggle
#define audacious_drct_set_skin             _audvt->drct_set_skin
#define audacious_drct_activate             _audvt->drct_activate
#define audacious_drct_initiate             _audvt->drct_initiate
#define audacious_drct_play                 _audvt->drct_play
#define audacious_drct_pause                _audvt->drct_pause
#define audacious_drct_stop                 _audvt->drct_stop
#define audacious_drct_get_playing          _audvt->drct_get_playing
#define audacious_drct_get_paused           _audvt->drct_get_paused
#define audacious_drct_get_stopped          _audvt->drct_get_stopped
#define audacious_drct_get_info             _audvt->drct_get_info
#define audacious_drct_get_time             _audvt->drct_get_time
#define audacious_drct_get_length           _audvt->drct_get_length
#define audacious_drct_seek                 _audvt->drct_seek
#define audacious_drct_get_volume           _audvt->drct_get_volume
#define audacious_drct_set_volume           _audvt->drct_set_volume
#define audacious_drct_get_volume_main      _audvt->drct_get_volume_main
#define audacious_drct_set_volume_main      _audvt->drct_set_volume_main
#define audacious_drct_get_volume_balance   _audvt->drct_get_volume_balance
#define audacious_drct_set_volume_balance   _audvt->drct_set_volume_balance
#define audacious_drct_pl_next              _audvt->drct_pl_next
#define audacious_drct_pl_prev              _audvt->drct_pl_prev
#define audacious_drct_pl_repeat_is_enabled _audvt->drct_pl_repeat_is_enabled
#define audacious_drct_pl_repeat_toggle     _audvt->drct_pl_repeat_toggle
#define audacious_drct_pl_repeat_is_shuffled _audvt->drct_pl_repeat_is_shuffled
#define audacious_drct_pl_shuffle_toggle    _audvt->drct_pl_shuffle_toggle
#define audacious_drct_pl_get_title         _audvt->drct_pl_get_title
#define audacious_drct_pl_get_time          _audvt->drct_pl_get_time
#define audacious_drct_pl_get_pos           _audvt->drct_pl_get_pos
#define audacious_drct_pl_get_file          _audvt->drct_pl_get_file
#define audacious_drct_pl_open              _audvt->drct_pl_open
#define audacious_drct_pl_open_list         _audvt->drct_pl_open_list
#define audacious_drct_pl_add               _audvt->drct_pl_add
#define audacious_drct_pl_clear             _audvt->drct_pl_clear
#define audacious_drct_pl_get_length        _audvt->drct_pl_get_length
#define audacious_drct_pl_delete            _audvt->drct_pl_delete
#define audacious_drct_pl_set_pos           _audvt->drct_pl_set_pos
#define audacious_drct_pl_ins_url_string    _audvt->drct_pl_ins_url_string
#define audacious_drct_pl_add_url_string    _audvt->drct_pl_add_url_string
#define audacious_drct_pl_enqueue_to_temp   _audvt->drct_pl_enqueue_to_temp

#define audacious_drct_pq_get_length        _audvt->drct_pq_get_length
#define audacious_drct_pq_add               _audvt->drct_pq_add
#define audacious_drct_pq_remove            _audvt->drct_pq_remove
#define audacious_drct_pq_clear             _audvt->drct_pq_clear
#define audacious_drct_pq_is_queued         _audvt->drct_pq_is_queued
#define audacious_drct_pq_get_position      _audvt->drct_pq_get_position
#define audacious_drct_pq_get_queue_position _audvt->drct_pq_get_queue_position

#define audacious_fileinfopopup_create          _audvt->fileinfopopup_create
#define audacious_fileinfopopup_destroy         _audvt->fileinfopopup_destroy
#define audacious_fileinfopopup_show_from_tuple _audvt->fileinfopopup_show_from_tuple
#define audacious_fileinfopopup_show_from_title _audvt->fileinfopopup_show_from_title
#define audacious_fileinfopopup_hide            _audvt->fileinfopopup_hide

#define audacious_get_localdir          _audvt->util_get_localdir

#define aud_playback_new                _audvt->playback_new
#define aud_playback_run                _audvt->playback_run
#define aud_playback_free(x)            _audvt->playback_free

#define aud_flow_execute                _audvt->flow_execute
#define aud_flow_new                    _audvt->flow_new
#define aud_flow_link_element           _audvt->flow_link_element
#define aud_flow_unlink_element         _audvt->flow_unlink_element
#define aud_effect_flow                 _audvt->effect_flow
#define aud_flow_destroy(flow)          mowgli_object_unref(flow)

#define audacious_menu_main_show        _audvt->util_menu_main_show

#define aud_get_dock_window_list        _audvt->get_dock_window_list
#define aud_dock_add_window             _audvt->dock_add_window
#define aud_dock_remove_window          _audvt->dock_remove_window
#define aud_dock_move_press             _audvt->dock_move_press
#define aud_dock_move_motion            _audvt->dock_move_motion
#define aud_dock_move_release           _audvt->dock_move_release
#define aud_dock_is_moving              _audvt->dock_is_moving

#define aud_get_output_list             _audvt->get_output_list
#define aud_get_effect_list             _audvt->get_effect_list
#define aud_enable_effect               _audvt->enable_effect

#define aud_input_get_volume            _audvt->input_get_volume

#define aud_construct_uri               _audvt->construct_uri
#define aud_uri_to_display_basename     _audvt->uri_to_display_basename
#define aud_uri_to_display_dirname      _audvt->uri_to_display_dirname

#define aud_set_pvt_data                _audvt->set_pvt_data
#define aud_get_pvt_data                _audvt->get_pvt_data

#define aud_event_queue                 _audvt->event_queue
#define aud_event_queue_with_data_free  _audvt->event_queue_with_data_free

#define aud_calc_mono_freq              _audvt->calc_mono_freq
#define aud_calc_mono_pcm               _audvt->calc_mono_pcm
#define aud_calc_stereo_pcm             _audvt->calc_stereo_pcm

#define aud_create_widgets(b, w, a) \
 _audvt->create_widgets_with_domain (b, w, a, PACKAGE);

#define aud_equalizer_read_presets      _audvt->equalizer_read_presets
#define aud_equalizer_write_preset_file _audvt->equalizer_write_preset_file
#define aud_import_winamp_eqf           _audvt->import_winamp_eqf
#define aud_save_preset_file            _audvt->save_preset_file
#define aud_equalizer_read_aud_preset   _audvt->equalizer_read_aud_preset
#define aud_load_preset_file            _audvt->load_preset_file
#define aud_output_plugin_cleanup       _audvt->output_plugin_cleanup
#define aud_output_plugin_reinit        _audvt->output_plugin_reinit

#define aud_get_plugin_menu             _audvt->get_plugin_menu
#define aud_playback_get_title          _audvt->playback_get_title
#define aud_fileinfo_show               _audvt->fileinfo_show
#define aud_fileinfo_show_current       _audvt->fileinfo_show_current
#define aud_save_all_playlists          _audvt->save_all_playlists

//#define aud_tag_tuple_read                  _audvt->tag_tuple_read
//#define aud_tag_tuple_write_to_file         _audvt->tag_tuple_write

#define aud_interface_get_current       _audvt->interface_get_current
#define aud_interface_toggle_visibility _audvt->interface_toggle_visibility
#define aud_interface_show_error        _audvt->interface_show_error

#define aud_get_audacious_credits       _audvt->get_audacious_credits

#define aud_playlist_entry_set_segmentation		_audvt->playlist_entry_set_segmentation
#define aud_playlist_entry_is_segmented			_audvt->playlist_entry_is_segmented
#define aud_playlist_entry_get_start_time		_audvt->playlist_entry_get_start_time
#define aud_playlist_entry_get_end_time			_audvt->playlist_entry_get_end_time

#include "audacious/auddrct.h"

/* for multi-file plugins :( */
G_BEGIN_DECLS
extern struct _AudaciousFuncTableV1 *_audvt;
G_END_DECLS

#define DECLARE_PLUGIN(name, init, fini, ...) \
    G_BEGIN_DECLS \
    static PluginHeader _pluginInfo = { PLUGIN_MAGIC, __AUDACIOUS_PLUGIN_API__, \
        (gchar *)#name, init, fini, NULL, __VA_ARGS__ }; \
    struct _AudaciousFuncTableV1 *_audvt = NULL; \
    G_MODULE_EXPORT PluginHeader *get_plugin_info(struct _AudaciousFuncTableV1 *_vt) { \
        _audvt = _vt; \
        return &_pluginInfo; \
    } \
    G_END_DECLS

#define SIMPLE_INPUT_PLUGIN(name, ip_list) \
    DECLARE_PLUGIN(name, NULL, NULL, ip_list)

#define SIMPLE_OUTPUT_PLUGIN(name, op_list) \
    DECLARE_PLUGIN(name, NULL, NULL, NULL, op_list)

#define SIMPLE_EFFECT_PLUGIN(name, ep_list) \
    DECLARE_PLUGIN(name, NULL, NULL, NULL, NULL, ep_list)

#define SIMPLE_GENERAL_PLUGIN(name, gp_list) \
    DECLARE_PLUGIN(name, NULL, NULL, NULL, NULL, NULL, gp_list)

#define SIMPLE_VISUAL_PLUGIN(name, vp_list) \
    DECLARE_PLUGIN(name, NULL, NULL, NULL, NULL, NULL, NULL, vp_list)

#define SIMPLE_INTERFACE_PLUGIN(name, interface) \
    DECLARE_PLUGIN(name, NULL, NULL, NULL, NULL, NULL, NULL, NULL, interface)


#define PLUGIN_COMMON_FIELDS        \
    gpointer handle;            \
    gchar *filename;            \
    gchar *description;            \
    void (*init) (void);        \
    void (*cleanup) (void);        \
    void (*about) (void);        \
    void (*configure) (void);        \
    PluginPreferences *settings;

/* Sadly, this is the most we can generalize out of the disparate
   plugin structs usable with typecasts - descender */
struct _Plugin {
    PLUGIN_COMMON_FIELDS
};

/*
 * LowlevelPlugin is used for lowlevel system services, such as PlaylistContainers,
 * VFSContainers and the like.
 *
 * They are not GUI visible at this time.
 *
 * XXX: Is this still in use in 1.4? --nenolod
 */
struct _LowlevelPlugin {
    PLUGIN_COMMON_FIELDS
};

typedef enum {
    OUTPUT_PLUGIN_INIT_FAIL,
    OUTPUT_PLUGIN_INIT_NO_DEVICES,
    OUTPUT_PLUGIN_INIT_FOUND_DEVICES,
} OutputPluginInitStatus;

struct _OutputPlugin {
    gpointer handle;
    gchar *filename;
    gchar *description;

    OutputPluginInitStatus (*init) (void);
    void (*cleanup) (void);
    void (*about) (void);
    void (*configure) (void);

    gboolean enabled;
    gint probe_priority;

    void (*get_volume) (gint * l, gint * r);
    void (*set_volume) (gint l, gint r);

    gint (*open_audio) (AFormat fmt, gint rate, gint nch);
    void (*write_audio) (gpointer ptr, gint length);
    void (*close_audio) (void);

    void (* set_written_time) (gint time);
    void (*flush) (gint time);
    void (*pause) (gshort paused);
    gint (*buffer_free) (void); /* obsolete */
    gint (*buffer_playing) (void); /* obsolete */
    gint (*output_time) (void);
    gint (*written_time) (void);

    void (*tell_audio) (AFormat * fmt, gint * rate, gint * nch); /* obsolete */
    void (* drain) (void);
};

struct _EffectPlugin {
    PLUGIN_COMMON_FIELDS

    gboolean enabled;

    /* old API */
    gint (*mod_samples) (gpointer * data, gint length, AFormat fmt, gint srate, gint nch);
    void (*query_format) (AFormat * fmt, gint * rate, gint * nch);

    /* new API */

    /* All processing is done in floating point.  If the effect plugin wants to
     * change the channel count or sample rate, it can change the parameters
     * passed to start().  They cannot be changed in the middle of a song. */
    void (* start) (gint * channels, gint * rate);

    /* process() has two options: modify the samples in place and leave the data
     * pointer unchanged or copy them into a buffer of its own.  If it sets the
     * pointer to dynamically allocated memory, it is the plugin's job to free
     * that memory.  process() may return different lengths of audio than it is
     * passed, even a zero length. */
    void (* process) (gfloat * * data, gint * samples);

    /* A seek is taking place; any buffers should be discarded. */
    void (* flush) ();

    /* Exactly like process() except that any buffers should be drained (i.e.
     * the data processed and returned). */
    void (* finish) (gfloat * * data, gint * samples);

    /* For effects that change the length of the song, these functions allow the
     * correct time to be displayed. */
    gint (* decoder_to_output_time) (gint time);
    gint (* output_to_decoder_time) (gint time);
};

struct OutputAPI
{
    gint (* open_audio) (AFormat fmt, gint rate, gint nch);
    void (* set_replaygain_info) (ReplayGainInfo * info);
    void (* write_audio) (gpointer ptr, gint length);
    void (* close_audio) (void);

    void (* pause) (gboolean pause);
    void (* flush) (gint time);
    gint (* written_time) (void);
    gboolean (* buffer_playing) (void);
};

struct _InputPlayback {
    gchar *filename;    /**< Filename URI */
    void *data;

    gint playing;       /**< 1 = Playing, 0 = Stopped. */
    gboolean error;     /**< TRUE if there has been an error. */
    gboolean eof;       /**< TRUE if end of file has been reached- */

    InputPlugin *plugin;
    struct OutputAPI * output;
    GThread *thread;

    GMutex *pb_ready_mutex;
    GCond *pb_ready_cond;
    gint pb_ready_val;
    gint (*set_pb_ready) (InputPlayback*);

    GMutex *pb_change_mutex;
    GCond *pb_change_cond;
    void (*set_pb_change) (InputPlayback *self);

    gint nch;           /**< */
    gint rate;          /**< */
    gint freq;          /**< */
    gint length;
    gchar *title;

    /**
     * Set playback parameters. Title should be NULL and length should be 0.
     * @deprecated Use of this function to set title or length is deprecated,
     * please use #set_tuple() for information other than bitrate/samplerate/channels.
     */
    void (*set_params) (InputPlayback * playback, const gchar * title, gint
     length, gint bitrate, gint samplerate, gint channels);

    /**
     * Set playback entry title.
     * @deprecated This function is deprecated, use #set_tuple() instead.
     */
    void (*set_title) (InputPlayback * playback, const gchar * title);

    void (*pass_audio) (InputPlayback *, AFormat, gint, gint, gpointer, gint *);

    /* called by input plugin when RG info available --asphyx */
    void (*set_replaygain_info) (InputPlayback *, ReplayGainInfo *);

    /**
     * Sets / updates playback entry #Tuple.
     * @attention Caller gives up ownership of one reference to the tuple.
     * @since Added in Audacious 2.2.
     */
    void (*set_tuple) (InputPlayback * playback, Tuple * tuple);

    gboolean segmented;
    gint start;
    gint end;
    gint end_timeout;
};

/**
 * Input plugin structure.
 */
struct _InputPlugin {
    PLUGIN_COMMON_FIELDS

    gboolean have_subtune;      /**< Plugin supports/uses subtunes. */
    gchar **vfs_extensions;     /**< Filename extension to be associated to this plugin. */

    GList *(*scan_dir) (gchar * dirname);

    /**
     * Check if this plugin can handle given file/filename.
     * @deprecated Use 3is_our_file_from_vfs() or #probe_for_tuple().
     */
    gint (*is_our_file) (const gchar * filename);
    gint (*is_our_file_from_vfs) (const gchar *filename, VFSFile *fd);
    Tuple *(*probe_for_tuple) (const gchar *uri, VFSFile *fd);

    void (*play_file) (InputPlayback * playback);
    void (*stop) (InputPlayback * playback);
    void (*pause) (InputPlayback * playback, gshort paused);
    void (*seek) (InputPlayback * playback, gint time);
    void (*mseek) (InputPlayback * playback, gulong millisecond);

    gint (*get_time) (InputPlayback * playback);

    gint (*get_volume) (gint * l, gint * r);
    gint (*set_volume) (gint l, gint r);

    void (*file_info_box) (const gchar * filename);

    Tuple *(*get_song_tuple) (const gchar * filename);

    /**
     * Plugin can provide this function for file metadata (aka tag)
     * writing functionality when there is no reason to provide its
     * own custom file info dialog.
     *
     * - In current Audacious version, if plugin provides file_info_box(), the latter will be used in any case.
     * - Each field in tuple means operation on one and only one tag field:
     *   - Set this field to appropriate value, if non-empty string or positive number provided.
     *   - Set this field to blank (or just delete, at plugins`s discretion), if empty string or negative number provided.
     *
     * @param[in] tuple Tuple with the desired metadata.
     * @param[in] fd VFS file descriptor pointing to file to modify.
     */
    gboolean (*update_song_tuple)(Tuple *tuple, VFSFile *fd);

    gint priority; /* 0 = first, 10 = last */
};

struct _GeneralPlugin {
    PLUGIN_COMMON_FIELDS

    gboolean enabled;
};

struct _VisPlugin {
    PLUGIN_COMMON_FIELDS

    gboolean enabled;
    gint num_pcm_chs_wanted;
    gint num_freq_chs_wanted;

    void (*disable_plugin) (struct _VisPlugin *);
    void (*playback_start) (void);
    void (*playback_stop) (void);
    void (*render_pcm) (gint16 pcm_data[2][512]);

    /* The range of intensities is 0 - 32767 (though theoretically it is
     * possible for the FFT to result in bigger values, making the final
     * intensity negative due to overflowing the 16bit signed integer.)
     *
     * If output is mono, only freq_data[0] is filled.
     */
    void (*render_freq) (gint16 freq_data[2][256]);

    GtkWidget *(*get_widget) (void);
};

/* undefine the macro -- struct Plugin should be used instead. */
#undef PLUGIN_COMMON_FIELDS

#endif /* AUDACIOUS_PLUGIN_H */
