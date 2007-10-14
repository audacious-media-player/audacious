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

#include "audacious/mime.h"
#include "audacious/custom_uri.h"

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
