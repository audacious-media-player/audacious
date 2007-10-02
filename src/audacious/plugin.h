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

#define DECLARE_PLUGIN(name, init, fini, ...) \
	G_BEGIN_DECLS \
	static PluginHeader _pluginInfo = { PLUGIN_MAGIC, __AUDACIOUS_PLUGIN_API__, \
		(gchar *)#name, init, fini, NULL, __VA_ARGS__ }; \
	static struct _AudaciousFuncVTable1 *_audvt = NULL; \
	G_MODULE_EXPORT PluginHeader *get_plugin_info(struct _AudaciousFuncVTable1 *_vt) { \
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

G_BEGIN_DECLS

/* So that input plugins can get the title formatting information */
G_CONST_RETURN gchar * get_gentitle_format(void);

G_END_DECLS

#include "audacious/mime.h"
#include "audacious/custom_uri.h"

#endif
