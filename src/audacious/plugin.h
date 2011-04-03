/*
 * plugin.h
 * Copyright 2005-2010 Audacious Development Team
 *
 * This file is part of Audacious.
 *
 * Audacious is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free Software
 * Foundation, version 2 or version 3 of the License.
 *
 * Audacious is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
 * A PARTICULAR PURPOSE. See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * Audacious. If not, see <http://www.gnu.org/licenses/>.
 *
 * The Audacious team does not consider modular code linking to Audacious or
 * using our public API to be a derived work.
 */

#ifndef AUDACIOUS_PLUGIN_H
#define AUDACIOUS_PLUGIN_H

#include <glib.h>
#include <gmodule.h>

#include <audacious/api.h>
#include <audacious/types.h>
#include <libaudcore/audio.h>
#include <libaudcore/tuple.h>
#include <libaudcore/vfs.h>

/* "Magic" bytes identifying an Audacious plugin header. */
#define _AUD_PLUGIN_MAGIC 0x8EAC8DE2

/* API version.  Plugins are marked with this number at compile time.
 *
 * _AUD_PLUGIN_VERSION is the current version; _AUD_PLUGIN_VERSION_MIN is
 * the oldest one we are backward compatible with.  Plugins marked older than
 * _AUD_PLUGIN_VERSION_MIN or newer than _AUD_PLUGIN_VERSION are not loaded.
 *
 * Before releases that add new pointers to the end of the API tables, increment
 * _AUD_PLUGIN_VERSION but leave _AUD_PLUGIN_VERSION_MIN the same.
 *
 * Before releases that break backward compatibility (e.g. remove pointers from
 * the API tables), increment _AUD_PLUGIN_VERSION *and* set
 * _AUD_PLUGIN_VERSION_MIN to the same value. */

#define _AUD_PLUGIN_VERSION_MIN 18 /* 2.5-alpha1 */
#define _AUD_PLUGIN_VERSION     20 /* 2.5-beta2 */

/**
 * The plugin module header. Each module can contain several plugins,
 * of any supported type.
 */
typedef const struct {
    gint magic; /* checked against _AUD_PLUGIN_MAGIC */
    gint version; /* checked against _AUD_PLUGIN_VERSION */
    const gchar * name;
    void (* init) (void);
    void (* fini) (void);

    /* These are arrays of pointers, ending with NULL: */
    InputPlugin * const * ip_list;
    OutputPlugin * const * op_list;
    EffectPlugin * const * ep_list;
    GeneralPlugin * const * gp_list;
    VisPlugin * const * vp_list;
    TransportPlugin * const * tp_list;
    PlaylistPlugin * const * pp_list;

    Iface * iface;
} PluginHeader;

#define DECLARE_PLUGIN(name, ...) \
 AudAPITable * _aud_api_table = NULL; \
 G_MODULE_EXPORT PluginHeader * get_plugin_info (AudAPITable * table) { \
    static PluginHeader h = {_AUD_PLUGIN_MAGIC, _AUD_PLUGIN_VERSION, #name, \
     __VA_ARGS__}; \
    _aud_api_table = table; \
    return & h; \
 }

#define SIMPLE_TRANSPORT_PLUGIN(name, t) DECLARE_PLUGIN(name, .tp_list = t)
#define SIMPLE_PLAYLIST_PLUGIN(name, p) DECLARE_PLUGIN(name, .pp_list = p)
#define SIMPLE_INPUT_PLUGIN(name, i) DECLARE_PLUGIN (name, .ip_list = i)
#define SIMPLE_EFFECT_PLUGIN(name, e) DECLARE_PLUGIN (name, .ep_list = e)
#define SIMPLE_OUTPUT_PLUGIN(name, o) DECLARE_PLUGIN (name, .op_list = o)
#define SIMPLE_VIS_PLUGIN(name, v) DECLARE_PLUGIN(name, .vp_list = v)
#define SIMPLE_GENERAL_PLUGIN(name, g) DECLARE_PLUGIN (name, .gp_list = g)
#define SIMPLE_IFACE_PLUGIN(name, i) DECLARE_PLUGIN(name, .iface = i)

#define SIMPLE_VISUAL_PLUGIN SIMPLE_VIS_PLUGIN /* deprecated */

#define PLUGIN_COMMON_FIELDS        \
    gchar *description;            \
    gboolean (* init) (void); \
    void (*cleanup) (void);        \
    void (*about) (void);        \
    void (*configure) (void);        \
    PluginPreferences *settings;

struct _Plugin {
    PLUGIN_COMMON_FIELDS
};

struct _TransportPlugin {
    PLUGIN_COMMON_FIELDS
    const gchar * const * schemes; /* array ending with NULL */
    const VFSConstructor * vtable;
};

struct _PlaylistPlugin {
    PLUGIN_COMMON_FIELDS
	const gchar * const * extensions; /* array ending with NULL */
	gboolean (* load) (const gchar * filename, gint list, gint at);
	gboolean (* save) (const gchar * filename, gint list);
};

struct _OutputPlugin {
    PLUGIN_COMMON_FIELDS

    /* During probing, plugins with higher priority (10 to 0) are tried first. */
    gint probe_priority;

    /* Returns current volume for left and right channels (0 to 100). */
    void (* get_volume) (gint * l, gint * r);

    /* Changes volume for left and right channels (0 to 100). */
    void (* set_volume) (gint l, gint r);

    /* Begins playback of a PCM stream.  <format> is one of the FMT_*
     * enumeration values defined in libaudcore/audio.h.  Returns nonzero on
     * success. */
    gboolean (* open_audio) (gint format, gint rate, gint chans);

    /* Ends playback.  Any buffered audio data is discarded. */
    void (* close_audio) (void);

    /* Returns how many bytes of data may be passed to a following write_audio()
     * call.  NULL if the plugin supports only blocking writes (not recommended). */
    gint (* buffer_free) (void);

    /* Waits until buffer_free() will return a size greater than zero.
     * output_time(), pause(), and flush() may be called meanwhile; if flush()
     * is called, period_wait() should return immediately.  NULL if the plugin
     * supports only blocking writes (not recommended). */
    void (* period_wait) (void);

    /* Buffers <size> bytes of data, in the format given to open_audio(). */
    void (* write_audio) (void * data, gint size);

    /* Waits until all buffered data has been heard by the user. */
    void (* drain) (void);

    /* Returns time count (in milliseconds) of how much data has been written. */
    gint (* written_time) (void);

    /* Returns time count (in milliseconds) of how much data has been heard by
     * the user. */
    gint (* output_time) (void);

    /* Pauses the stream if <p> is nonzero; otherwise unpauses it.
     * write_audio() will not be called while the stream is paused. */
    void (* pause) (gboolean p);

    /* Discards any buffered audio data and sets the time counter (in
     * milliseconds) of data written. */
    void (* flush) (gint time);

    /* Sets the time counter (in milliseconds) of data written without
     * discarding any buffered audio data.  If <time> is less than the amount of
     * buffered data, following calls to output_time() will return negative
     * values. */
    void (* set_written_time) (gint time);
};

struct _EffectPlugin {
    PLUGIN_COMMON_FIELDS

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

    /* If the effect does not change the number of channels or the sampling
     * rate, it can be enabled and disabled more smoothly. */
    gboolean preserves_format;
};

struct OutputAPI
{
    /* In a multi-thread plugin, only one of these functions may be called at
     * once (but see pause and abort_write for exceptions to this rule). */

    /* Prepare the output system for playback in the specified format.  Returns
     * nonzero on success.  If the call fails, no other output functions may be
     * called. */
    gint (* open_audio) (gint format, gint rate, gint channels);

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

typedef const struct _InputPlayback InputPlayback;

struct _InputPlayback {
    /* Pointer to the output API functions. */
    const struct OutputAPI * output;

    /* Allows the plugin to associate data with a playback instance. */
    void (* set_data) (InputPlayback * p, void * data);

    /* Returns the pointer passed to set_data. */
    void * (* get_data) (InputPlayback * p);

    /* Signifies that the plugin has started playback is ready to accept mseek,
     * pause, and stop calls. */
    void (* set_pb_ready) (InputPlayback * p);

    /* Updates attributes of the stream.  "bitrate" is in bits per second.
     * "samplerate" is in hertz. */
    void (* set_params) (InputPlayback * p, gint bitrate, gint samplerate,
     gint channels);

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
};

struct _InputPlugin {
    PLUGIN_COMMON_FIELDS

    /* Nonzero if the files handled by the plugin may contain more than one
     * song.  When reading the tuple for such a file, the plugin should set the
     * FIELD_SUBSONG_NUM field to the number of songs in the file.  For all
     * other files, the field should be left unset.
     *
     * Example:
     * 1. User adds a file named "somefile.xxx" to the playlist.  Having
     * determined that this plugin can handle the file, Audacious opens the file
     * and calls probe_for_tuple().  probe_for_tuple() sees that there are 3
     * songs in the file and sets FIELD_SUBSONG_NUM to 3.
     * 2. For each song in the file, Audacious opens the file and calls
     * probe_for_tuple() -- this time, however, a question mark and song number
     * are appended to the file name passed: "somefile.sid?2" refers to the
     * second song in the file "somefile.sid".
     * 3. When one of the songs is played, Audacious opens the file and calls
     * play() with a file name modified in this way.
     */
    gboolean have_subtune;

    /* Pointer to an array (terminated with NULL) of file extensions associated
     * with file types the plugin can handle. */
    const gchar * const * vfs_extensions;

    /* How quickly the plugin should be tried in searching for a plugin to
     * handle a file which could not be identified from its extension.  Plugins
     * with priority 0 are tried first, 10 last. */
    gint priority;

    /* Must return nonzero if the plugin can handle this file.  If the file
     * could not be opened, "file" will be NULL.  (This is normal in the case of
     * special URI schemes like cdda:// that do not represent actual files.) */
    gboolean (* is_our_file_from_vfs) (const gchar * filename, VFSFile * file);

    /* Must return a tuple containing metadata for this file, or NULL if no
     * metadata could be read.  If the file could not be opened, "file" will be
     * NULL.  Audacious takes over one reference to the tuple returned. */
    Tuple * (* probe_for_tuple) (const gchar * filename, VFSFile * file);

    /* Optional.  Must write metadata from a tuple to this file.  Must return
     * nonzero on success or zero on failure.  "file" will never be NULL. */
    /* Bug: This function does not support special URI schemes like cdda://,
     * since no file name is passed. */
    gboolean (* update_song_tuple) (const Tuple * tuple, VFSFile * file);

    /* Optional, and not recommended.  Must show a window with information about
     * this file.  If this function is provided, update_song_tuple should not be. */
    /* Bug: Implementing this function duplicates user interface code and code
     * to open the file in each and every plugin. */
    void (* file_info_box) (const gchar * filename);

    /* Optional.  Must try to read an "album art" image embedded in this file.
     * Must return nonzero on success or zero on failure.  If the file could not
     * be opened, "file" will be NULL.  On success, must fill "data" with a
     * pointer to a block of data allocated with g_malloc and "size" with the
     * size in bytes of that block.  The data may be in any format supported by
     * GTK.  Audacious will free the data when it is no longer needed. */
    gboolean (* get_song_image) (const gchar * filename, VFSFile * file,
     void * * data, gint * size);

    /* Must try to play this file.  "playback" is a structure containing output-
     * related functions which the plugin may make use of.  It also contains a
     * "data" pointer which the plugin may use to refer private data associated
     * with the playback state.  This pointer can then be used from pause,
     * mseek, and stop. If the file could not be opened, "file" will be NULL.
     * "start_time" is the position in milliseconds at which to start from, or
     * -1 to start from the beginning of the file.  "stop_time" is the position
     * in milliseconds at which to end playback, or -1 to play to the end of the
     * file.  "paused" specifies whether playback should immediately be paused.
     * Must return nonzero if some of the file was successfully played or zero
     * on failure. */
    gboolean (* play) (InputPlayback * playback, const gchar * filename,
     VFSFile * file, gint start_time, gint stop_time, gboolean pause);

    /* Must pause or unpause a file currently being played.  This function will
     * be called from a different thread than play, but it will not be called
     * before the plugin calls set_pb_ready or after stop is called. */
    void (* pause) (InputPlayback * playback, gboolean paused);

    /* Optional.  Must seek to the given position in milliseconds within a file
     * currently being played.  This function will be called from a different
     * thread than play, but it will not be called before the plugin calls
     * set_pb_ready or after stop is called. */
    void (* mseek) (InputPlayback * playback, gint time);

    /* Must signal a currently playing song to stop and cause play to return.
     * This function will be called from a different thread than play.  It will
     * only be called once. It should not join the thread from which play is
     * called. */
    void (* stop) (InputPlayback * playback);

    /* Advanced, for plugins that do not use Audacious's output system.  Use at
     * your own risk. */
    gint (* get_time) (InputPlayback * playback);
    gint (* get_volume) (gint * l, gint * r);
    gint (* set_volume) (gint l, gint r);
};

struct _GeneralPlugin {
    PLUGIN_COMMON_FIELDS

    /* GtkWidget * (* get_widget) (void); */
    void * (* get_widget) (void);
};

struct _VisPlugin {
    PLUGIN_COMMON_FIELDS

    gint num_pcm_chs_wanted;
    gint num_freq_chs_wanted;

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

#undef PLUGIN_COMMON_FIELDS

#endif /* AUDACIOUS_PLUGIN_H */
