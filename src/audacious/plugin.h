/*  Audacious
 *  Copyright (C) 2005-2007  Audacious team.
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

#ifndef BMP_PLUGIN_H
#define BMP_PLUGIN_H

#include <glib.h>
#include <gtk/gtk.h>
#include "audacious/vfs.h"
#include "audacious/tuple.h"
#include "audacious/tuple_formatter.h"
#include "audacious/eventqueue.h"
#include "audacious/configdb.h"
#include "audacious/playlist_container.h"
#include "audacious/main.h"

#define PLUGIN(x)         ((Plugin *)(x))
#define INPUT_PLUGIN(x)   ((InputPlugin *)(x))
#define OUTPUT_PLUGIN(x)  ((OutputPlugin *)(x))
#define EFFECT_PLUGIN(x)  ((EffectPlugin *)(x))
#define GENERAL_PLUGIN(x) ((GeneralPlugin *)(x))
#define VIS_PLUGIN(x)     ((VisPlugin *)(x))
#define DISCOVERY_PLUGIN(x)     ((DiscoveryPlugin *)(x))

#define LOWLEVEL_PLUGIN(x) ((LowlevelPlugin *)(x))

#define __AUDACIOUS_NEWVFS__
#define __AUDACIOUS_PLUGIN_API__ 6
#define __AUDACIOUS_INPUT_PLUGIN_API__ 6

typedef enum {
    FMT_U8,
    FMT_S8,
    FMT_U16_LE,
    FMT_U16_BE,
    FMT_U16_NE,
    FMT_S16_LE,
    FMT_S16_BE,
    FMT_S16_NE
} AFormat;

typedef enum {
    INPUT_VIS_ANALYZER,
    INPUT_VIS_SCOPE,
    INPUT_VIS_VU,
    INPUT_VIS_OFF
} InputVisType;


typedef struct _Plugin        Plugin;
typedef struct _InputPlugin   InputPlugin;
typedef struct _OutputPlugin  OutputPlugin;
typedef struct _EffectPlugin  EffectPlugin;
typedef struct _GeneralPlugin GeneralPlugin;
typedef struct _VisPlugin     VisPlugin;
typedef struct _DiscoveryPlugin DiscoveryPlugin;
typedef struct _LowlevelPlugin LowlevelPlugin;

typedef struct _InputPlayback InputPlayback;

typedef GHashTable INIFile;

#include "audacious/playlist.h"
#include "audacious/input.h"
#include "audacious/mime.h"
#include "audacious/custom_uri.h"
#include "audacious/hook.h"
#include "audacious/xconvert.h"
#include "audacious/ui_plugin_menu.h"
#include "audacious/formatter.h"

#define PLUGIN_COMMON_FIELDS		\
    gpointer handle;			\
    gchar *filename;			\
    gchar *description;			\
    void (*init) (void);		\
    void (*cleanup) (void);		\
    void (*about) (void);		\
    void (*configure) (void);		\
    gboolean enabled;
	

/*
 * The v2 Module header.
 *
 * _list fields contain a null-terminated list of "plugins" to register.
 * A single library can provide multiple plugins.
 *     --nenolod
 */
typedef struct {
    gint magic;
    gint api_version;
    gchar *name;
    GCallback init;
    GCallback fini;
    Plugin *priv_assoc;
    InputPlugin **ip_list;
    OutputPlugin **op_list;
    EffectPlugin **ep_list;
    GeneralPlugin **gp_list;
    VisPlugin **vp_list;
    DiscoveryPlugin **dp_list;
} PluginHeader;

#define PLUGIN_MAGIC 0x8EAC8DE2

/* define the public API here */
/* add new functions to the bottom of this list!!!! --nenolod */
struct _AudaciousFuncTableV1 {

    /* VFS */
    VFSFile *(*vfs_fopen)(const gchar *uri, const gchar *mode);
    gint (*vfs_fclose)(VFSFile *fd);
    VFSFile *(*vfs_dup)(VFSFile *in);
    size_t (*vfs_fread)(gpointer ptr,
                 size_t size,
                 size_t nmemb,
                 VFSFile * file);
    size_t (*vfs_fwrite)(gconstpointer ptr,
                  size_t size,
                  size_t nmemb,
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

    /* ConfigDb */
    ConfigDb *(*cfg_db_open)(void);
    void (*cfg_db_close)(ConfigDb *db);

    gboolean (*cfg_db_get_string)(ConfigDb *db,
                                  const gchar *section,
                                  const gchar *key,
                                  gchar **value);
    gboolean (*cfg_db_get_int)(ConfigDb *db,
                               const gchar *section,
                               const gchar *key,
                               gint *value);
    gboolean (*cfg_db_get_bool)(ConfigDb *db,
                                const gchar *section,
                                const gchar *key,
                                gboolean *value);
    gboolean (*cfg_db_get_float)(ConfigDb *db,
                                 const gchar *section,
                                 const gchar *key,
                                 gfloat *value);
    gboolean (*cfg_db_get_double)(ConfigDb *db,
                                  const gchar *section,
                                  const gchar *key,
                                  gdouble *value);

    void (*cfg_db_set_string)(ConfigDb *db,
                              const gchar *section,
                              const gchar *key,
                              const gchar *value);
    void (*cfg_db_set_int)(ConfigDb *db,
                           const gchar *section,
                           const gchar *key,
                           gint value);
    void (*cfg_db_set_bool)(ConfigDb *db,
                            const gchar *section,
                            const gchar *key,
                            gboolean value);
    void (*cfg_db_set_float)(ConfigDb *db,
                             const gchar *section,
                             const gchar *key,
                             gfloat value);
    void (*cfg_db_set_double)(ConfigDb *db,
                              const gchar *section,
                              const gchar *key,
                              gdouble value);

    void (*cfg_db_unset_key)(ConfigDb *db,
                             const gchar *section,
                             const gchar *key);

    /* Tuple manipulation API */
    Tuple *(*tuple_new)(void);
    Tuple *(*tuple_new_from_filename)(const gchar *filename);

    gboolean (*tuple_associate_string)(Tuple *tuple,
                                       const gint nfield,
                                       const gchar *field,
                                       const gchar *string);
    gboolean (*tuple_associate_int)(Tuple *tuple,
                                    const gint nfield,
                                    const gchar *field,
                                    gint integer);

    void (*tuple_disassociate)(Tuple *tuple,
                               const gint nfield,
                               const gchar *field);

    TupleValueType (*tuple_get_value_type)(Tuple *tuple,
                                           const gint nfield,
                                           const gchar *field);

    const gchar *(*tuple_get_string)(Tuple *tuple,
                                     const gint nfield,
                                     const gchar *field);
    gint (*tuple_get_int)(Tuple *tuple,
                       const gint nfield,
                       const gchar *field);

    /* tuple formatter API */
    gchar *(*tuple_formatter_process_string)(Tuple *tuple, const gchar *string);
    gchar *(*tuple_formatter_make_title_string)(Tuple *tuple, const gchar *string);
    void (*tuple_formatter_register_expression)(const gchar *keyword,
           gboolean (*func)(Tuple *tuple, const gchar *argument));
    void (*tuple_formatter_register_function)(const gchar *keyword,
           gchar *(*func)(Tuple *tuple, gchar **argument));
    gchar *(*tuple_formatter_process_expr)(Tuple *tuple, const gchar *expression,
           const gchar *argument);
    gchar *(*tuple_formatter_process_function)(Tuple *tuple, const gchar *expression,
           const gchar *argument);
    gchar *(*tuple_formatter_process_construct)(Tuple *tuple, const gchar *string);

    /* MIME types */
    InputPlugin *(*mime_get_plugin)(const gchar *mimetype);
    void (*mime_set_plugin)(const gchar *mimetype, InputPlugin *ip);

    /* Custom URI registry */
    InputPlugin *(*uri_get_plugin)(const gchar *filename);
    void (*uri_set_plugin)(const gchar *uri, InputPlugin *ip);

    /* Util funcs */
    GtkWidget *(*util_info_dialog)(const gchar * title, const gchar * text,
                                   const gchar * button_text, gboolean modal,
                                   GCallback button_action,
                                   gpointer action_data);
    const gchar *(*get_gentitle_format)(void);

    /* INI funcs */
    INIFile *(*open_ini_file)(const gchar *filename);
    void (*close_ini_file)(INIFile *key_file);
    gchar *(*read_ini_string)(INIFile *key_file, const gchar *section,
                                           const gchar *key);
    GArray *(*read_ini_array)(INIFile *key_file, const gchar *section,
                       const gchar *key);


    /* strings API */
    gchar *(*escape_shell_chars)(const gchar * string);

    gchar *(*str_append)(gchar * str, const gchar * add_str);
    gchar *(*str_replace)(gchar * str, gchar * new_str);
    void (*str_replace_in)(gchar ** str, gchar * new_str);

    gboolean (*str_has_prefix_nocase)(const gchar * str, const gchar * prefix);
    gboolean (*str_has_suffix_nocase)(const gchar * str, const gchar * suffix);
    gboolean (*str_has_suffixes_nocase)(const gchar * str, gchar * const *suffixes);

    gchar *(*str_to_utf8_fallback)(const gchar * str);
    gchar *(*filename_to_utf8)(const gchar * filename);
    gchar *(*str_to_utf8)(const gchar * str);

    const gchar *(*str_skip_chars)(const gchar * str, const gchar * chars);

    gchar *(*convert_title_text)(gchar * text);

    gchar *(*chardet_to_utf8)(const gchar *str, gssize len,
                       gsize *arg_bytes_read, gsize *arg_bytes_write,
                       GError **arg_error);

    /* PlaylistContainer API. */
    void (*playlist_container_register)(PlaylistContainer *plc);
    void (*playlist_container_unregister)(PlaylistContainer *plc);
    void (*playlist_container_read)(gchar *filename, gint pos);
    void (*playlist_container_write)(gchar *filename, gint pos);
    PlaylistContainer *(*playlist_container_find)(gchar *ext);

    /* Playlist API */
    PlaylistEntry *(*playlist_entry_new)(const gchar * filename,
                                  const gchar * title, const gint len,
                                  InputPlugin * dec);
    void (*playlist_entry_free)(PlaylistEntry * entry);

    void (*playlist_add_playlist)(Playlist *);
    void (*playlist_remove_playlist)(Playlist *);
    void (*playlist_select_playlist)(Playlist *);
    void (*playlist_select_next)(void);
    void (*playlist_select_prev)(void);
    GList *(*playlist_get_playlists)(void);

    void (*playlist_clear_only)(Playlist *playlist);
    void (*playlist_clear)(Playlist *playlist);
    void (*playlist_delete)(Playlist *playlist, gboolean crop);

    gboolean (*playlist_add)(Playlist *playlist, const gchar * filename);
    gboolean (*playlist_ins)(Playlist *playlist, const gchar * filename, gint pos);
    guint (*playlist_add_dir)(Playlist *playlist, const gchar * dir);
    guint (*playlist_ins_dir)(Playlist *playlist, const gchar * dir, gint pos, gboolean background);
    guint (*playlist_add_url)(Playlist *playlist, const gchar * url);
    guint (*playlist_ins_url)(Playlist *playlist, const gchar * string, gint pos);

    void (*playlist_check_pos_current)(Playlist *playlist);
    void (*playlist_next)(Playlist *playlist);
    void (*playlist_prev)(Playlist *playlist);
    void (*playlist_queue)(Playlist *playlist);
    void (*playlist_queue_position)(Playlist *playlist, guint pos);
    void (*playlist_queue_remove)(Playlist *playlist, guint pos);
    gint (*playlist_queue_get_length)(Playlist *playlist);
    gboolean (*playlist_is_position_queued)(Playlist *playlist, guint pos);
    void (*playlist_clear_queue)(Playlist *playlist);
    gint (*playlist_get_queue_position)(Playlist *playlist, PlaylistEntry * entry);
    gint (*playlist_get_queue_position_number)(Playlist *playlist, guint pos);
    gint (*playlist_get_queue_qposition_number)(Playlist *playlist, guint pos);
    void (*playlist_eof_reached)(Playlist *playlist);
    void (*playlist_set_position)(Playlist *playlist, guint pos);
    gint (*playlist_get_length)(Playlist *playlist);
    gint (*playlist_get_position)(Playlist *playlist);
    gint (*playlist_get_position_nolock)(Playlist *playlist);
    gchar *(*playlist_get_info_text)(Playlist *playlist);
    gint (*playlist_get_current_length)(Playlist *playlist);

    gboolean (*playlist_save)(Playlist *playlist, const gchar * filename);
    gboolean (*playlist_load)(Playlist *playlist, const gchar * filename);

    void (*playlist_sort)(Playlist *playlist, PlaylistSortType type);
    void (*playlist_sort_selected)(Playlist *playlist, PlaylistSortType type);

    void (*playlist_reverse)(Playlist *playlist);
    void (*playlist_random)(Playlist *playlist);
    void (*playlist_remove_duplicates)(Playlist *playlist, PlaylistDupsType);
    void (*playlist_remove_dead_files)(Playlist *playlist);

    void (*playlist_fileinfo_current)(Playlist *playlist);
    void (*playlist_fileinfo)(Playlist *playlist, guint pos);

    void (*playlist_delete_index)(Playlist *playlist, guint pos);
    void (*playlist_delete_filenames)(Playlist *playlist, GList * filenames);

    PlaylistEntry *(*playlist_get_entry_to_play)(Playlist *playlist);

    gchar *(*playlist_get_filename)(Playlist *playlist, guint pos);
    gchar *(*playlist_get_songtitle)(Playlist *playlist, guint pos);
    Tuple *(*playlist_get_tuple)(Playlist *playlist, guint pos);
    gint (*playlist_get_songtime)(Playlist *playlist, guint pos);

    GList *(*playlist_get_selected)(Playlist *playlist);
    int (*playlist_get_num_selected)(Playlist *playlist);

    void (*playlist_get_total_time)(Playlist *playlist, gulong * total_time, gulong * selection_time,
                             gboolean * total_more,
                             gboolean * selection_more);

    gint (*playlist_select_search)(Playlist *playlist, Tuple *tuple, gint action);
    void (*playlist_select_all)(Playlist *playlist, gboolean set);
    void (*playlist_select_range)(Playlist *playlist, gint min, gint max, gboolean sel);
    void (*playlist_select_invert_all)(Playlist *playlist);
    gboolean (*playlist_select_invert)(Playlist *playlist, guint pos);

    gboolean (*playlist_read_info_selection)(Playlist *playlist);
    void (*playlist_read_info)(Playlist *playlist, guint pos);

    void (*playlist_set_shuffle)(gboolean shuffle);

    void (*playlist_clear_selected)(Playlist *playlist);

    GList *(*get_playlist_nth)(Playlist *playlist, guint);

    gboolean (*playlist_set_current_name)(Playlist *playlist, const gchar * title);
    const gchar *(*playlist_get_current_name)(Playlist *playlist);

    gboolean (*playlist_filename_set)(Playlist *playlist, const gchar * filename);

    gchar *(*playlist_filename_get)(Playlist *playlist);

    Playlist *(*playlist_new)(void);
    void (*playlist_free)(Playlist *playlist);
    Playlist *(*playlist_new_from_selected)(void);

    gboolean (*is_playlist_name)(const gchar * filename);

    void (*playlist_load_ins_file)(Playlist *playlist, const gchar * filename,
                                   const gchar * playlist_name, gint pos,
                                   const gchar * title, gint len);

    void (*playlist_load_ins_file_tuple)(Playlist *playlist, const gchar * filename_p,
                                         const gchar * playlist_name, gint pos,
                                         Tuple *tuple);

    Playlist *(*playlist_get_active)(void);

    gboolean (*playlist_playlists_equal)(Playlist *p1, Playlist *p2);

    /* state vars */
    InputPluginData *ip_state;
    BmpConfig *_cfg;

    /* hook API */
    void (*hook_register)(const gchar *name);
    gint (*hook_associate)(const gchar *name, HookFunction func, gpointer user_data);
    gint (*hook_dissociate)(const gchar *name, HookFunction func);
    void (*hook_call)(const gchar *name, gpointer hook_data);

    /* xconvert API */
    struct xmms_convert_buffers *(*xmms_convert_buffers_new)(void);
    void (*xmms_convert_buffers_free)(struct xmms_convert_buffers *buf);
    void (*xmms_convert_buffers_destroy)(struct xmms_convert_buffers *buf);
    convert_func_t (*xmms_convert_get_func)(AFormat output, AFormat input);
    convert_channel_func_t (*xmms_convert_get_channel_func)(AFormat fmt,
                                                     int output,
                                                     int input);
    convert_freq_func_t (*xmms_convert_get_frequency_func)(AFormat fmt,
                                                    int channels);

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
    void (*drct_play) ( void );
    void (*drct_pause) ( void );
    void (*drct_stop) ( void );
    gboolean (*drct_get_playing)( void );
    gboolean (*drct_get_paused)( void );
    gboolean (*drct_get_stopped)( void );
    void (*drct_get_info)( gint *rate, gint *freq, gint *nch);
    gint (*drct_get_time )( void );
    void (*drct_seek) ( guint pos );
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
    void (*drct_pl_add) ( GList * list );
    void (*drct_pl_clear) ( void );
    gint (*drct_pl_get_length)( void );
    void (*drct_pl_delete) ( gint pos );
    void (*drct_pl_set_pos)( gint pos );
    void (*drct_pl_ins_url_string)( gchar * string, gint pos );
    void (*drct_pl_add_url_string)( gchar * string );
    void (*drct_pl_enqueue_to_temp)( gchar * string );

    /* DRCT API: playqueue */
    gint (*drct_pq_get_length)( void );
    void (*drct_pq_add)( gint pos );
    void (*drct_pq_remove)( gint pos );
    void (*drct_pq_clear)( void );
    gboolean (*drct_pq_is_queued)( gint pos );
    gint (*drct_pq_get_position)( gint pos );
    gint (*drct_pq_get_queue_position)( gint pos );

    /* Formatter API */
    Formatter *(*formatter_new)(void);
    void (*formatter_destroy)(Formatter * formatter);
    void (*formatter_associate)(Formatter * formatter, guchar id,
                                gchar * value);
    void (*formatter_dissociate)(Formatter * formatter, guchar id);
    gchar *(*formatter_format)(Formatter * formatter, gchar * format);

    gint (*prefswin_page_new)(GtkWidget *container, gchar *name, gchar *imgurl);
    void (*prefswin_page_destroy)(GtkWidget *container);

    /* FileInfoPopup API */
    GtkWidget *(*fileinfopopup_create)(void);
    void (*fileinfopopup_destroy)(GtkWidget* fileinfopopup_win);
    void (*fileinfopopup_show_from_tuple)(GtkWidget *fileinfopopup_win, Tuple *tuple);
    void (*fileinfopopup_show_from_title)(GtkWidget *fileinfopopup_win, gchar *title);
    void (*fileinfopopup_hide)(GtkWidget *filepopup_win, gpointer unused);
};

/* Convenience macros for accessing the public API. */
/*	public name			vtable mapping      */
#define aud_vfs_fopen			_audvt->vfs_fopen
#define aud_vfs_fclose			_audvt->vfs_fclose
#define aud_vfs_dup			_audvt->vfs_dup
#define aud_vfs_fread			_audvt->vfs_fread
#define aud_vfs_fwrite			_audvt->vfs_fwrite
#define aud_vfs_getc			_audvt->vfs_getc
#define aud_vfs_ungetc			_audvt->vfs_ungetc
#define aud_vfs_fgets			_audvt->vfs_fgets
#define aud_vfs_fseek			_audvt->vfs_fseek
#define aud_vfs_rewind			_audvt->vfs_rewind
#define aud_vfs_ftell			_audvt->vfs_ftell
#define aud_vfs_feof			_audvt->vfs_feof
#define aud_vfs_file_test		_audvt->vfs_file_test
#define aud_vfs_is_writeable		_audvt->vfs_is_writeable
#define aud_vfs_truncate		_audvt->vfs_truncate
#define aud_vfs_fsize			_audvt->vfs_fsize
#define aud_vfs_get_metadata		_audvt->vfs_get_metadata
#define aud_vfs_fprintf			_audvt->vfs_fprintf
#define aud_vfs_register_transport	_audvt->vfs_register_transport
#define aud_vfs_file_get_contents	_audvt->vfs_file_get_contents
#define aud_vfs_is_remote		_audvt->vfs_is_remote
#define aud_vfs_is_streaming		_audvt->vfs_is_streaming

#define aud_vfs_buffer_new		_audvt->vfs_buffer_new
#define aud_vfs_buffer_new_from_string	_audvt->vfs_buffer_new_from_string

#define aud_vfs_buffered_file_new_from_uri	_audvt->vfs_buffered_file_new_from_uri
#define aud_vfs_buffered_file_release_live_fd	_audvt->vfs_buffered_file_release_live_fd

/* XXX: deprecation warnings */
#define bmp_cfg_db_open			_audvt->cfg_db_open
#define bmp_cfg_db_close		_audvt->cfg_db_close
#define bmp_cfg_db_set_string		_audvt->cfg_db_set_string
#define bmp_cfg_db_set_int		_audvt->cfg_db_set_int
#define bmp_cfg_db_set_bool		_audvt->cfg_db_set_bool
#define bmp_cfg_db_set_float		_audvt->cfg_db_set_float
#define bmp_cfg_db_set_double		_audvt->cfg_db_set_double
#define bmp_cfg_db_get_string		_audvt->cfg_db_get_string
#define bmp_cfg_db_get_int		_audvt->cfg_db_get_int
#define bmp_cfg_db_get_bool		_audvt->cfg_db_get_bool
#define bmp_cfg_db_get_float		_audvt->cfg_db_get_float
#define bmp_cfg_db_get_double		_audvt->cfg_db_get_double
#define bmp_cfg_db_unset_key		_audvt->cfg_db_unset_key

#define aud_cfg_db_open			_audvt->cfg_db_open
#define aud_cfg_db_close		_audvt->cfg_db_close
#define aud_cfg_db_set_string		_audvt->cfg_db_set_string
#define aud_cfg_db_set_int		_audvt->cfg_db_set_int
#define aud_cfg_db_set_bool		_audvt->cfg_db_set_bool
#define aud_cfg_db_set_float		_audvt->cfg_db_set_float
#define aud_cfg_db_set_double		_audvt->cfg_db_set_double
#define aud_cfg_db_get_string		_audvt->cfg_db_get_string
#define aud_cfg_db_get_int		_audvt->cfg_db_get_int
#define aud_cfg_db_get_bool		_audvt->cfg_db_get_bool
#define aud_cfg_db_get_float		_audvt->cfg_db_get_float
#define aud_cfg_db_get_double		_audvt->cfg_db_get_double
#define aud_cfg_db_unset_key		_audvt->cfg_db_unset_key

#define aud_tuple_new			_audvt->tuple_new
#define aud_tuple_new_from_filename	_audvt->tuple_new_from_filename
#define aud_tuple_associate_string	_audvt->tuple_associate_string
#define aud_tuple_associate_int		_audvt->tuple_associate_int
#define aud_tuple_disassociate		_audvt->tuple_disassociate
#define aud_tuple_disassociate_now	_audvt->tuple_disassociate_now
#define aud_tuple_get_value_type	_audvt->tuple_get_value_type
#define aud_tuple_get_string		_audvt->tuple_get_string
#define aud_tuple_get_int		_audvt->tuple_get_int
#define aud_tuple_free			mowgli_object_unref

#define aud_tuple_formatter_process_string		_audvt->tuple_formatter_process_string
#define aud_tuple_formatter_make_title_string		_audvt->tuple_formatter_make_title_string
#define aud_tuple_formatter_register_expression		_audvt->tuple_formatter_register_expression
#define aud_tuple_formatter_register_function		_audvt->tuple_formatter_register_function
#define aud_tuple_formatter_process_expr		_audvt->tuple_formatter_process_expr
#define aud_tuple_formatter_process_function		_audvt->tuple_formatter_process_function
#define aud_tuple_formatter_process_construct		_audvt->tuple_formatter_process_construct

#define aud_mime_get_plugin		_audvt->mime_get_plugin
#define aud_mime_set_plugin		_audvt->mime_set_plugin

#define aud_uri_get_plugin		_audvt->uri_get_plugin
#define aud_uri_set_plugin		_audvt->uri_set_plugin

#define aud_info_dialog			_audvt->util_info_dialog
#define audacious_info_dialog		_audvt->util_info_dialog
#define aud_get_gentitle_format		_audvt->get_gentitle_format

#define aud_escape_shell_chars		_audvt->escape_shell_chars
#define aud_str_append			_audvt->str_append
#define aud_str_replace			_audvt->str_replace
#define aud_str_replace_in		_audvt->str_replace_in
#define aud_str_has_prefix_nocase	_audvt->str_has_prefix_nocase
#define aud_str_has_suffix_nocase	_audvt->str_has_suffix_nocase
#define aud_str_has_suffixes_nocase	_audvt->str_has_suffixes_nocase
#define aud_str_to_utf8_fallback	_audvt->str_to_utf8_fallback
#define aud_filename_to_utf8		_audvt->filename_to_utf8
#define aud_str_to_utf8			_audvt->str_to_utf8
#define aud_str_skip_chars		_audvt->str_skip_chars
#define aud_convert_title_text		_audvt->convert_title_text
#define aud_chardet_to_utf8		_audvt->chardet_to_utf8

#define aud_playlist_container_register		_audvt->playlist_container_register
#define aud_playlist_container_unregister	_audvt->playlist_container_unregister
#define aud_playlist_container_read		_audvt->playlist_container_read
#define aud_playlist_container_write		_audvt->playlist_container_write
#define aud_playlist_container_find		_audvt->playlist_container_find

#define aud_playlist_entry_new			_audvt->playlist_entry_new
#define aud_playlist_entry_free			_audvt->playlist_entry_free

#define aud_playlist_add_playlist		_audvt->playlist_add_playlist
#define aud_playlist_remove_playlist		_audvt->playlist_remove_playlist
#define aud_playlist_select_playlist		_audvt->playlist_select_playlist
#define aud_playlist_select_next		_audvt->playlist_select_next
#define aud_playlist_select_prev		_audvt->playlist_select_prev
#define aud_playlist_get_playlists		_audvt->playlist_get_playlists

#define aud_playlist_clear_only			_audvt->playlist_clear_only
#define aud_playlist_clear			_audvt->playlist_clear
#define aud_playlist_delete			_audvt->playlist_delete

#define aud_playlist_add			_audvt->playlist_add
#define aud_playlist_ins			_audvt->playlist_ins
#define aud_playlist_add_dir			_audvt->playlist_add_dir
#define aud_playlist_ins_dir			_audvt->playlist_ins_dir
#define aud_playlist_add_url			_audvt->playlist_add_url
#define aud_playlist_ins_url			_audvt->playlist_ins_url

#define aud_playlist_check_pos_current		_audvt->playlist_check_pos_current
#define aud_playlist_next			_audvt->playlist_next
#define aud_playlist_prev			_audvt->playlist_prev

#define aud_playlist_queue			_audvt->playlist_queue
#define aud_playlist_queue_position		_audvt->playlist_queue_position
#define aud_playlist_queue_remove		_audvt->playlist_queue_remove
#define aud_playlist_queue_get_length		_audvt->playlist_queue_get_length
#define aud_playlist_is_position_queued		_audvt->playlist_is_position_queued
#define aud_playlist_clear_queue		_audvt->playlist_clear_queue
#define aud_playlist_get_queue_position		_audvt->playlist_get_queue_position
#define aud_playlist_get_queue_position_number	_audvt->playlist_get_queue_position_number
#define aud_playlist_get_queue_qposition_number	_audvt->playlist_get_queue_qposition_number
#define aud_playlist_eof_reached		_audvt->playlist_eof_reached
#define aud_playlist_set_position		_audvt->playlist_set_position
#define aud_playlist_get_length			_audvt->playlist_get_length
#define aud_playlist_get_position		_audvt->playlist_get_position
#define aud_playlist_get_position_nolock	_audvt->playlist_get_position_nolock
#define aud_playlist_get_info_text		_audvt->playlist_get_info_text
#define aud_playlist_get_current_length		_audvt->playlist_get_current_length

#define aud_playlist_save			_audvt->playlist_save
#define aud_playlist_load			_audvt->playlist_load

#define aud_playlist_sort			_audvt->playlist_sort
#define aud_playlist_sort_selected		_audvt->playlist_sort_selected

#define aud_playlist_reverse			_audvt->playlist_reverse
#define aud_playlist_random			_audvt->playlist_random
#define aud_playlist_remove_duplicates		_audvt->playlist_remove_duplicates
#define aud_playlist_remove_dead_files		_audvt->playlist_remove_dead_files

#define aud_playlist_fileinfo_current		_audvt->playlist_fileinfo_current
#define aud_playlist_fileinfo			_audvt->playlist_fileinfo

#define aud_playlist_delete_index		_audvt->playlist_delete_index
#define aud_playlist_delete_filenames		_audvt->playlist_delete_filenames

#define aud_playlist_get_entry_to_play		_audvt->playlist_get_entry_to_play

#define aud_playlist_get_filename		_audvt->playlist_get_filename
#define aud_playlist_get_songtitle		_audvt->playlist_get_songtitle
#define aud_playlist_get_tuple			_audvt->playlist_get_tuple
#define aud_playlist_get_songtime		_audvt->playlist_get_songtime

#define aud_playlist_get_selected		_audvt->playlist_get_selected
#define aud_playlist_get_num_selected		_audvt->playlist_get_num_selected

#define aud_playlist_get_total_time		_audvt->playlist_get_total_time

#define aud_playlist_select_search		_audvt->playlist_select_search
#define aud_playlist_select_all			_audvt->playlist_select_all
#define aud_playlist_select_range		_audvt->playlist_select_range
#define aud_playlist_select_invert_all		_audvt->playlist_select_invert_all
#define aud_playlist_select_invert		_audvt->playlist_select_invert

#define aud_playlist_read_info_selection	_audvt->playlist_read_info_selection
#define aud_playlist_read_info			_audvt->playlist_read_info

#define aud_playlist_set_shuffle		_audvt->playlist_set_shuffle

#define aud_playlist_clear_selected		_audvt->playlist_clear_selected

#define aud_get_playlist_nth			_audvt->get_playlist_nth

#define aud_playlist_set_current_name		_audvt->playlist_set_current_name
#define aud_playlist_get_current_name		_audvt->playlist_get_current_name

#define aud_playlist_filename_set		_audvt->playlist_filename_set
#define aud_playlist_filename_get		_audvt->playlist_filename_get

#define aud_playlist_new			_audvt->playlist_new
#define aud_playlist_free			_audvt->playlist_free
#define aud_playlist_new_from_selected		_audvt->playlist_new_from_selected

#define aud_is_playlist_name			_audvt->is_playlist_name

#define aud_playlist_load_ins_file		_audvt->playlist_load_ins_file
#define aud_playlist_load_ins_file_tuple	_audvt->playlist_load_ins_file_tuple

#define aud_playlist_get_active			_audvt->playlist_get_active
#define aud_playlist_playlists_equal		_audvt->playlist_playlists_equal

#define aud_ip_state				_audvt->ip_state
#define aud_cfg					_audvt->_cfg

#define aud_hook_associate			_audvt->hook_associate
#define aud_hook_dissociate			_audvt->hook_dissociate
#define aud_hook_register			_audvt->hook_register
#define aud_hook_call				_audvt->hook_call

#define aud_open_ini_file			_audvt->open_ini_file
#define aud_close_ini_file			_audvt->close_ini_file
#define aud_read_ini_string			_audvt->read_ini_string
#define aud_read_ini_array			_audvt->read_ini_array

#define aud_convert_buffers_new			_audvt->xmms_convert_buffers_new
#define aud_convert_buffers_free		_audvt->xmms_convert_buffers_free
#define aud_convert_buffers_destroy		_audvt->xmms_convert_buffers_destroy
#define aud_convert_get_func			_audvt->xmms_convert_get_func
#define aud_convert_get_channel_func		_audvt->xmms_convert_get_channel_func
#define aud_convert_get_frequency_func		_audvt->xmms_convert_get_frequency_func

#define audacious_menu_plugin_item_add		_audvt->menu_plugin_item_add
#define audacious_menu_plugin_item_remove	_audvt->menu_plugin_item_remove
#define aud_menu_plugin_item_add		_audvt->menu_plugin_item_add
#define aud_menu_plugin_item_remove		_audvt->menu_plugin_item_remove

#define audacious_drct_quit			_audvt->drct_quit
#define audacious_drct_eject			_audvt->drct_eject
#define audacious_drct_jtf_show			_audvt->drct_jtf_show
#define audacious_drct_main_win_is_visible	_audvt->drct_main_win_is_visible
#define audacious_drct_main_win_toggle		_audvt->drct_main_win_toggle
#define audacious_drct_eq_win_is_visible	_audvt->drct_eq_win_is_visible
#define audacious_drct_eq_win_toggle		_audvt->drct_eq_win_toggle
#define audacious_drct_pl_win_is_visible	_audvt->drct_pl_win_is_visible
#define audacious_drct_pl_win_toggle		_audvt->drct_pl_win_toggle
#define audacious_drct_set_skin			_audvt->drct_set_skin
#define audacious_drct_activate			_audvt->drct_activate

#define audacious_drct_play			_audvt->drct_play
#define audacious_drct_pause			_audvt->drct_pause
#define audacious_drct_stop			_audvt->drct_stop
#define audacious_drct_get_playing		_audvt->drct_get_playing
#define audacious_drct_get_paused		_audvt->drct_get_paused
#define audacious_drct_get_stopped		_audvt->drct_get_stopped
#define audacious_drct_get_info			_audvt->drct_get_info
#define audacious_drct_get_time			_audvt->drct_get_time
#define audacious_drct_seek			_audvt->drct_seek
#define audacious_drct_get_volume		_audvt->drct_get_volume
#define audacious_drct_set_volume		_audvt->drct_set_volume
#define audacious_drct_get_volume_main		_audvt->drct_get_volume_main
#define audacious_drct_set_volume_main		_audvt->drct_set_volume_main
#define audacious_drct_get_volume_balance	_audvt->drct_get_volume_balance
#define audacious_drct_set_volume_balance	_audvt->drct_set_volume_balance

#define audacious_drct_pl_next			_audvt->drct_pl_next
#define audacious_drct_pl_prev			_audvt->drct_pl_prev
#define audacious_drct_pl_repeat_is_enabled	_audvt->drct_pl_repeat_is_enabled
#define audacious_drct_pl_repeat_toggle		_audvt->drct_pl_repeat_toggle
#define audacious_drct_pl_repeat_is_shuffled	_audvt->drct_pl_repeat_is_shuffled
#define audacious_drct_pl_shuffle_toggle	_audvt->drct_pl_shuffle_toggle
#define audacious_drct_pl_get_title		_audvt->drct_pl_get_title
#define audacious_drct_pl_get_time		_audvt->drct_pl_get_time
#define audacious_drct_pl_get_pos		_audvt->drct_pl_get_pos
#define audacious_drct_pl_get_file		_audvt->drct_pl_get_file
#define audacious_drct_pl_add			_audvt->drct_pl_add
#define audacious_drct_pl_clear			_audvt->drct_pl_clear
#define audacious_drct_pl_get_length		_audvt->drct_pl_get_length
#define audacious_drct_pl_delete		_audvt->drct_pl_delete
#define audacious_drct_pl_set_pos		_audvt->drct_pl_set_pos
#define audacious_drct_pl_ins_url_string	_audvt->drct_pl_ins_url_string
#define audacious_drct_pl_add_url_string	_audvt->drct_pl_add_url_string
#define audacious_drct_pl_enqueue_to_temp	_audvt->drct_pl_enqueue_to_temp

#define audacious_drct_pq_get_length		_audvt->drct_pq_get_length
#define audacious_drct_pq_add			_audvt->drct_pq_add
#define audacious_drct_pq_remove		_audvt->drct_pq_remove
#define audacious_drct_pq_clear			_audvt->drct_pq_clear
#define audacious_drct_pq_is_queued		_audvt->drct_pq_is_queued
#define audacious_drct_pq_get_position		_audvt->drct_pq_get_position
#define audacious_drct_pq_get_queue_position	_audvt->drct_pq_get_queue_position

#define aud_formatter_new			_audvt->formatter_new
#define aud_formatter_destroy			_audvt->formatter_destroy
#define aud_formatter_associate			_audvt->formatter_associate
#define aud_formatter_dissociate		_audvt->formatter_dissociate
#define aud_formatter_format			_audvt->formatter_format

#define aud_prefswin_page_new			_audvt->prefswin_page_new
#define aud_prefswin_page_destroy		_audvt->prefswin_page_destroy

#define audacious_fileinfopopup_create			_audvt->fileinfopopup_create
#define audacious_fileinfopopup_destroy			_audvt->fileinfopopup_destroy
#define audacious_fileinfopopup_show_from_tuple		_audvt->fileinfopopup_show_from_tuple
#define audacious_fileinfopopup_show_from_title		_audvt->fileinfopopup_show_from_title
#define audacious_fileinfopopup_hide			_audvt->fileinfopopup_hide

#include "audacious/auddrct.h"

/* for multi-file plugins :( */
extern struct _AudaciousFuncTableV1 *_audvt;

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

#define SIMPLE_DISCOVER_PLUGIN(name, dp_list) \
    DECLARE_PLUGIN(name, NULL, NULL, NULL, NULL, NULL, NULL, NULL, dp_list)

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

struct _OutputPlugin {
    PLUGIN_COMMON_FIELDS

    void (*get_volume) (gint * l, gint * r);
    void (*set_volume) (gint l, gint r);

    gint (*open_audio) (AFormat fmt, gint rate, gint nch);
    void (*write_audio) (gpointer ptr, gint length);
    void (*close_audio) (void);

    void (*flush) (gint time);
    void (*pause) (gshort paused);
    gint (*buffer_free) (void);
    gint (*buffer_playing) (void);
    gint (*output_time) (void);
    gint (*written_time) (void);

    void (*tell_audio) (AFormat * fmt, gint * rate, gint * nch);
};

struct _EffectPlugin {
    PLUGIN_COMMON_FIELDS

    gint (*mod_samples) (gpointer * data, gint length, AFormat fmt, gint srate, gint nch);
    void (*query_format) (AFormat * fmt, gint * rate, gint * nch);
};

struct _InputPlayback {
    gchar *filename;
    InputPlugin *plugin;
    void *data;
    OutputPlugin *output;

    int playing;
    gboolean error;
    gboolean eof;

    GThread *thread;
    
    GMutex *pb_ready_mutex;
    GCond *pb_ready_cond;
    gint pb_ready_val;    
    gint (*set_pb_ready) (InputPlayback*);

    GMutex *pb_change_mutex;
    GCond *pb_change_cond;
    void (*set_pb_change)(InputPlayback *self);

    gint nch;
    gint rate;
    gint freq;
    gint length;
    gchar *title;
    
    void (*set_params) (InputPlayback *, gchar * title, gint length, gint rate, gint freq, gint nch);
    void (*set_title) (InputPlayback *, gchar * text);

    void (*pass_audio) (InputPlayback *, AFormat, gint, gint, gpointer, gint *);
};

struct _InputPlugin {
    PLUGIN_COMMON_FIELDS

    gint (*is_our_file) (gchar * filename);
    GList *(*scan_dir) (gchar * dirname);

    void (*play_file) (InputPlayback * playback);
    void (*stop) (InputPlayback * playback);
    void (*pause) (InputPlayback * playback, gshort paused);
    void (*seek) (InputPlayback * playback, gint time);

    void (*set_eq) (gint on, gfloat preamp, gfloat * bands);

    gint (*get_time) (InputPlayback * playback);

    gint (*get_volume) (gint * l, gint * r);
    gint (*set_volume) (gint l, gint r);

    InputVisType (*get_vis_type) (void);
    void (*add_vis_pcm) (gint time, AFormat fmt, gint nch, gint length, gpointer ptr);

    void (*set_info) (gchar * title, gint length, gint rate, gint freq, gint nch);
    void (*set_info_text) (gchar * text);
    void (*get_song_info) (gchar * filename, gchar ** title, gint * length);
    void (*file_info_box) (gchar * filename);

    OutputPlugin *output; /* deprecated */

    /* Added in Audacious 1.1.0 */
    Tuple *(*get_song_tuple) (gchar * filename);
    void (*set_song_tuple) (Tuple * tuple);
    void (*set_status_buffering) (gboolean status);

    /* Added in Audacious 1.3.0 */
    gint (*is_our_file_from_vfs) (gchar *filename, VFSFile *fd);
    gchar **vfs_extensions;

    /* Added in Audacious 1.4.0 */
    void (*mseek) (InputPlayback * playback, gulong millisecond);
    Tuple *(*probe_for_tuple)(gchar *uri, VFSFile *fd);
};

struct _GeneralPlugin {
    PLUGIN_COMMON_FIELDS
};

struct _VisPlugin {
    PLUGIN_COMMON_FIELDS

    gint num_pcm_chs_wanted;
    gint num_freq_chs_wanted;

    void (*disable_plugin) (struct _VisPlugin *);
    void (*playback_start) (void);
    void (*playback_stop) (void);
    void (*render_pcm) (gint16 pcm_data[2][512]);
    void (*render_freq) (gint16 freq_data[2][256]);
};

struct _DiscoveryPlugin {
    PLUGIN_COMMON_FIELDS

    GList *(*get_devices);  
};

/* undefine the macro -- struct Plugin should be used instead. */
#undef PLUGIN_COMMON_FIELDS

#endif
