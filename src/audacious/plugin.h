/*  Audacious
 *  Copyright (C) 2005-2010  Audacious team.
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
#include <gmodule.h>

#include <libaudcore/audio.h>
#include <libaudcore/hook.h>
#include <libaudcore/tuple.h>

#include <audacious/api.h>
#include <audacious/audconfig.h>
#include <audacious/playlist_container.h>
#include <audacious/preferences.h>
#include <audacious/interface.h>
#include <audacious/equalizer_preset.h>

//@{
/** Plugin type cast macros */
#define PLUGIN(x)               ((Plugin *)(x))
#define INPUT_PLUGIN(x)         ((InputPlugin *)(x))
#define OUTPUT_PLUGIN(x)        ((OutputPlugin *)(x))
#define EFFECT_PLUGIN(x)        ((EffectPlugin *)(x))
#define GENERAL_PLUGIN(x)       ((GeneralPlugin *)(x))
#define VIS_PLUGIN(x)           ((VisPlugin *)(x))
//@}

//@{
/** Preprocessor defines for different API features */
#define __AUDACIOUS_NEWVFS__                /**< @deprecated define for availability of VFS API. */
#define __AUDACIOUS_PLUGIN_API__ 16         /**< Current generic plugin API/ABI version, exact match is required for plugin to be loaded. */
#define __AUDACIOUS_INPUT_PLUGIN_API__ 8    /**< Input plugin API version. */
//@}

typedef enum {
    INPUT_VIS_ANALYZER,
    INPUT_VIS_SCOPE,
    INPUT_VIS_VU,
    INPUT_VIS_OFF
} InputVisType;

typedef enum {
    PLUGIN_MESSAGE_ERROR = 0,
    PLUGIN_MESSAGE_OK = 1,
    PLUGIN_MESSAGE_DEFERRED = 2
} PluginMessageResponse;

#define EQUALIZER_MAX_GAIN 12

typedef struct _Plugin        Plugin;
typedef struct _InputPlugin   InputPlugin;
typedef struct _OutputPlugin  OutputPlugin;
typedef struct _EffectPlugin  EffectPlugin;
typedef struct _GeneralPlugin GeneralPlugin;
typedef struct _VisPlugin     VisPlugin;

typedef struct _InputPlayback InputPlayback;

/** ReplayGain information structure */
typedef struct {
    gfloat track_gain;  /**< Track gain in decibels (dB) */
    gfloat track_peak;
    gfloat album_gain;
    gfloat album_peak;
} ReplayGainInfo;

#include "audacious/input.h"
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
    void (* init) (void);
    void (* fini) (void);
    Plugin *priv_assoc;
    InputPlugin **ip_list;  /**< List of InputPlugin(s) in this module */
    OutputPlugin **op_list;
    EffectPlugin **ep_list;
    GeneralPlugin **gp_list;
    VisPlugin **vp_list;
    Interface *interface;
} PluginHeader;

#define PLUGIN_MAGIC 0x8EAC8DE2

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
    /* Plugin registry */
    void (* uri_set_plugin) (const gchar * uri, InputPlugin * ip);
    void (* mime_set_plugin) (const gchar * mimetype, InputPlugin * ip);

    /* Util funcs */
    gchar *(*util_get_localdir)(void);
    void (*util_add_url_history_entry)(const gchar * url);

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

    const gchar * (* get_gentitle_format) (void);

    /* state vars */
    AudConfig *_cfg;

    /* PluginMenu API */
    /* gint (* menu_plugin_item_add) (gint menu, GtkWidget * item); */
    gint (* menu_plugin_item_add) (gint menu, void * item);
    /* gint (* menu_plugin_item_remove) (gint, GtkWidget * item); */
    gint (* menu_plugin_item_remove) (gint, void * item);

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
    void (* enable_general) (GeneralPlugin *plugin, gboolean enable);

    gchar *(*construct_uri)(gchar *string, const gchar *playlist_name);
    gchar *(*uri_to_display_basename)(const gchar * uri);
    gchar *(*uri_to_display_dirname)(const gchar * uri);

    void (*calc_mono_freq)(gint16 dest[2][256], gint16 src[2][512], gint nch);
    void (*calc_mono_pcm)(gint16 dest[2][512], gint16 src[2][512], gint nch);
    void (*calc_stereo_pcm)(gint16 dest[2][512], gint16 src[2][512], gint nch);

    /* void (* create_widgets_with_domain) (GtkBox * box, PreferencesWidget *
     widgets, gint amt, const gchar * domain); */
    void (* create_widgets_with_domain) (void * box, PreferencesWidget *
     widgets, gint amt, const gchar * domain);

    GList *(*equalizer_read_presets)(const gchar * basename);
    void (*equalizer_write_preset_file)(GList * list, const gchar * basename);
    GList *(*import_winamp_eqf)(VFSFile * file);
    void (*save_preset_file)(EqualizerPreset *preset, const gchar * filename);
    EqualizerPreset *(*equalizer_read_aud_preset)(const gchar * filename);
    EqualizerPreset *(*load_preset_file)(const gchar *filename);

    /* File probing API */
    InputPlugin * (* file_find_decoder) (const gchar * filename, gboolean fast);
    Tuple * (* file_read_tuple) (const gchar * filename, InputPlugin * decoder);
    gboolean (* file_read_image) (const gchar * filename, InputPlugin * decoder,
     void * * data, gint * size);
    gboolean (* file_can_write_tuple) (const gchar * filename, InputPlugin *
     decoder);
    gboolean (* file_write_tuple) (const gchar * filename, InputPlugin *
     decoder, Tuple * tuple);
    gboolean (* custom_infowin) (const gchar * filename, InputPlugin * decoder);

    /* Miscellaneous */
    /* GtkWidget * (* get_plugin_menu) (gint id); */
    void * (* get_plugin_menu) (gint id);
    gchar * (* playback_get_title) (void);
    void (* save_all_playlists) (void);
    gchar * (* get_associated_image_file) (const gchar * filename);

    /* Interface API */
    const Interface * (* interface_get_current) (void);
    void (* interface_toggle_visibility) (void);
    void (* interface_show_error) (const gchar * markup);

    void (* get_audacious_credits)(const gchar **brief, const gchar *** credits, const gchar *** translators);

    void (*vis_runner_add_hook)(HookFunction func, void * user_data);
    void (*vis_runner_remove_hook)(HookFunction func);
};

/* Convenience macros for accessing the public API. */
#define _audvt _aud_api_table->vt

/*      public name                     vtable mapping */
#define aud_mime_set_plugin             _audvt->mime_set_plugin
#define aud_uri_set_plugin              _audvt->uri_set_plugin

#define aud_util_get_localdir           _audvt->util_get_localdir
#define aud_util_add_url_history_entry  _audvt->util_add_url_history_entry

#define aud_str_to_utf8                 _audvt->str_to_utf8
#define aud_chardet_to_utf8             _audvt->chardet_to_utf8

#define aud_playlist_container_register     _audvt->playlist_container_register
#define aud_playlist_container_unregister   _audvt->playlist_container_unregister
#define aud_playlist_container_read         _audvt->playlist_container_read
#define aud_playlist_container_write        _audvt->playlist_container_write
#define aud_playlist_container_find         _audvt->playlist_container_find

#define aud_get_gentitle_format         _audvt->get_gentitle_format

#define aud_cfg                         _audvt->_cfg

#define aud_menu_plugin_item_add        _audvt->menu_plugin_item_add
#define aud_menu_plugin_item_remove     _audvt->menu_plugin_item_remove

#define aud_flow_execute                _audvt->flow_execute
#define aud_flow_new                    _audvt->flow_new
#define aud_flow_link_element           _audvt->flow_link_element
#define aud_flow_unlink_element         _audvt->flow_unlink_element
#define aud_effect_flow                 _audvt->effect_flow
#define aud_flow_destroy                mowgli_object_unref

#define aud_get_output_list             _audvt->get_output_list
#define aud_get_effect_list             _audvt->get_effect_list
#define aud_enable_effect               _audvt->enable_effect
#define aud_enable_general              _audvt->enable_general

#define aud_construct_uri               _audvt->construct_uri
#define aud_uri_to_display_basename     _audvt->uri_to_display_basename
#define aud_uri_to_display_dirname      _audvt->uri_to_display_dirname

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

#define aud_file_find_decoder           _audvt->file_find_decoder
#define aud_file_read_tuple             _audvt->file_read_tuple
#define aud_file_read_image             _audvt->file_read_image
#define aud_file_can_write_tuple        _audvt->file_can_write_tuple
#define aud_file_write_tuple            _audvt->file_write_tuple
#define aud_custom_infowin              _audvt->custom_infowin

#define aud_get_plugin_menu             _audvt->get_plugin_menu
#define aud_playback_get_title          _audvt->playback_get_title
#define aud_save_all_playlists          _audvt->save_all_playlists
#define aud_get_associated_image_file   _audvt->get_associated_image_file

#define aud_interface_get_current       _audvt->interface_get_current
#define aud_interface_toggle_visibility _audvt->interface_toggle_visibility
#define aud_interface_show_error        _audvt->interface_show_error

#define aud_get_audacious_credits       _audvt->get_audacious_credits

#define aud_vis_runner_add_hook		_audvt->vis_runner_add_hook
#define aud_vis_runner_remove_hook	_audvt->vis_runner_remove_hook


#define DECLARE_PLUGIN(name, init, fini, ...) \
    G_BEGIN_DECLS \
    static PluginHeader _pluginInfo = { PLUGIN_MAGIC, __AUDACIOUS_PLUGIN_API__, \
        (gchar *)#name, init, fini, NULL, __VA_ARGS__ }; \
    AudAPITable * _aud_api_table = NULL; \
    G_MODULE_EXPORT PluginHeader * get_plugin_info (AudAPITable * table) { \
        _aud_api_table = table; \
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
    PluginPreferences *settings;     \
    PluginMessageResponse (*sendmsg)(gint msgtype, gpointer msgdata);

/* Sadly, this is the most we can generalize out of the disparate
   plugin structs usable with typecasts - descender */
struct _Plugin {
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
    gint (* buffer_free) (void);
    gint (*buffer_playing) (void); /* obsolete */
    gint (*output_time) (void);
    gint (*written_time) (void);

    void (*tell_audio) (AFormat * fmt, gint * rate, gint * nch); /* obsolete */
    void (* drain) (void);
    void (* period_wait) (void);
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
    void (* flush) (void);

    /* Exactly like process() except that any buffers should be drained (i.e.
     * the data processed and returned).  finish() will be called a second time
     * at the end of the last song in the playlist. */
    void (* finish) (gfloat * * data, gint * samples);

    /* For effects that change the length of the song, these functions allow the
     * correct time to be displayed. */
    gint (* decoder_to_output_time) (gint time);
    gint (* output_to_decoder_time) (gint time);

    /* Effects with lowest order (0 to 9) are applied first. */
    gint order;
};

struct OutputAPI
{
    /* In a multi-thread plugin, only one of these functions may be called at
     * once (but see pause and abort_write for exceptions to this rule). */

    /* Prepare the output system for playback in the specified format.  Returns
     * nonzero on success.  If the call fails, no other output functions may be
     * called. */
    gint (* open_audio) (AFormat format, gint rate, gint channels);

    /* Informs the output system of replay gain values for the current song so
     * that volume levels can be adjusted accordingly, if the user so desires.
     * This may be called at any time during playback should the values change. */
    void (* set_replaygain_info) (ReplayGainInfo * info);

    /* Pass audio data to the output system for playback.  The data must be in
     * the format passed to open_audio, and the length (in bytes) must be an
     * integral number of frames.  This function blocks until all the data has
     * been written (though it may not yet be heard by the user); if the output
     * system is paused; this may be indefinitely.  See abort_write for a way to
     * interrupt a blocked call. */
    void (* write_audio) (void * data, gint length);

    /* End playback.  Any audio data currently buffered by the output system
     * will be discarded.  After the call, no other output functions, except
     * open_audio, may be called. */
    void (* close_audio) (void);

    /* Pause or unpause playback.  This function may be called during a call to
     * write_audio, in which write_audio will block until playback is unpaused
     * (but see abort_write to prevent the call from blocking). */
    void (* pause) (gboolean pause);

    /* Discard any audio data currently buffered by the output system, and set
     * the time counter to a new value.  This function is intended to be used
     * for seeking. */
    void (* flush) (gint time);

    /* Returns the time counter.  Note that this represents the amount of audio
     * data passed to the output system, not the amount actually heard by the
     * user.  This function is useful for handling a changed audio format:
     * First, save the time counter using this function.  Second, call
     * close_audio and then open_audio with the new format (note that the call
     * may fail).  Finally, restore the time counter using flush. */
    gint (* written_time) (void);

    /* Returns TRUE if there is data remaining in the output buffer; FALSE if
     * all data written to the output system has been heard by the user.  This
     * function should be polled (1/50 second is a reasonable delay between
     * calls) at the end of a song before calling close_audio.  Once it returns
     * FALSE, close_audio can be called without cutting off any of the end of
     * the song. */
    gboolean (* buffer_playing) (void);

    /* Interrupt a call to write_audio so that it returns immediately.  This
     * works even when the call is blocked by pause.  Buffered audio data is
     * discarded as in flush.  Until flush is called or the output system is
     * reset, further calls to write_audio will have no effect and return
     * immediately.  This function is intended to be used in seeking or
     * stopping in a multi-thread plugin.  To seek, the handler function (called
     * in the main thread) should first set a flag for the decoding thread and
     * then call abort_write.  When the decoding thread notices the flag, it
     * should do the actual seek, call flush, and finally clear the flag.  Once
     * the flag is cleared, the handler function may return. */
    void (* abort_write) (void);
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
     * Sets / updates playback entry #Tuple.
     * @attention Caller gives up ownership of one reference to the tuple.
     * @since Added in Audacious 2.2.
     */
    void (*set_tuple) (InputPlayback * playback, Tuple * tuple);

    /* If replay gain settings are stored in the tuple associated with the
     * current song, this function can be called (after opening audio) to apply
     * those settings.  If the settings are changed in a call to set_tuple, this
     * function must be called again to apply the updated settings. */
    void (* set_gain_from_playlist) (InputPlayback * playback);

    /* deprecated */
    void (*pass_audio) (InputPlayback *, AFormat, gint, gint, gpointer, gint *);
    void (*set_replaygain_info) (InputPlayback *, ReplayGainInfo *);
};

/**
 * Input plugin structure.
 */
struct _InputPlugin {
    PLUGIN_COMMON_FIELDS

    gboolean have_subtune;      /**< Plugin supports/uses subtunes. */
    gchar **vfs_extensions;     /**< Filename extension to be associated to this plugin. */
    gint priority; /* 0 = first, 10 = last */

    gint (*is_our_file_from_vfs) (const gchar *filename, VFSFile *fd);
    Tuple *(*get_song_tuple) (const gchar * filename);
    Tuple *(*probe_for_tuple) (const gchar *uri, VFSFile *fd);

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
    void (*file_info_box) (const gchar * filename);

    /* Warning: Check for file == NULL. */
    gboolean (* get_song_image) (const gchar * filename, VFSFile * file,
     void * * data, gint * size);

    /* Warning: Check for file == NULL. */
    gboolean (* play) (InputPlayback * playback, const gchar * filename,
     VFSFile * file, gint start_time, gint stop_time, gboolean pause);

    void (*pause) (InputPlayback * playback, gshort paused);
    void (*mseek) (InputPlayback * playback, gulong millisecond);
    void (*stop) (InputPlayback * playback);

    /* advanced: for plugins that do not use Audacious's output system */
    gint (*get_time) (InputPlayback * playback);
    gint (*get_volume) (gint * l, gint * r);
    gint (*set_volume) (gint l, gint r);

    /* deprecated */
    gint (*is_our_file) (const gchar * filename);
    void (*play_file) (InputPlayback * playback);
    void (*seek) (InputPlayback * playback, gint time);
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

    /* GtkWidget * (* get_widget) (void); */
    void * (* get_widget) (void);
};

/* undefine the macro -- struct Plugin should be used instead. */
#undef PLUGIN_COMMON_FIELDS

#endif /* AUDACIOUS_PLUGIN_H */
