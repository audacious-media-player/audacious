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

#include <audacious/api.h>
#include <audacious/types.h>
#include <libaudcore/audio.h>
#include <libaudcore/index.h>
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

#define _AUD_PLUGIN_VERSION_MIN 38 /* 3.2-alpha2 */
#define _AUD_PLUGIN_VERSION     38

/* A NOTE ON THREADS
 *
 * How thread-safe a plugin must be depends on the type of plugin.  Note that
 * some parts of the Audacious API are *not* thread-safe and therefore cannot be
 * used in some parts of some plugins; for example, input plugins cannot use
 * GUI-related calls or access the playlist except in about() and configure().
 *
 * Thread-safe plugins: transport, playlist, input, effect, and output.  These
 * must be mostly thread-safe.  init() and cleanup() may be called from
 * secondary threads; however, no other functions provided by the plugin will be
 * called at the same time.  about() and configure() will be called only from
 * the main thread.  All other functions provided by the plugin may be called
 * from any thread and from multiple threads simultaneously.
 *
 * Exceptions:
 * - Because many existing input plugins are not coded to handle simultaneous
 *   calls to play(), play() will only be called from one thread at a time.  New
 *   plugins should not rely on this exception, though.
 * - Some combinations of calls, especially for output and effect plugins, make
 *   no sense; for example, flush() in an output plugin will only be called
 *   after open_audio() and before close_audio().
 *
 * Single-thread plugins: visualization, general, and interface.  Functions
 * provided by these plugins will only be called from the main thread. */

/* CROSS-PLUGIN MESSAGES
 *
 * Since 3.2, Audacious implements a basic messaging system between plugins.
 * Messages are sent using aud_plugin_send_message() and received through the
 * take_message() method specified in the header of the receiving plugin.
 * Plugins that do not need to receive messages can set take_message() to NULL.
 *
 * Each message includes a code indicating the type of message, a pointer to
 * some data, and a value indicating the size of that data. What the message
 * data contains is entirely up to the two plugins involved. For this reason, it
 * is crucial that both plugins agree on the meaning of the message codes used.
 *
 * Once the message is sent, an integer error code is returned. If the receiving
 * plugin does not provide the take_message() method, ENOSYS is returned. If
 * take_message() does not recognize the message code, it should ignore the
 * message and return EINVAL. An error code of zero represents success. Other
 * error codes may be used with more specific meanings.
 *
 * For the time being, aud_plugin_send_message() should only be called from the
 * program's main thread. */

#define PLUGIN_COMMON_FIELDS \
    int magic; /* checked against _AUD_PLUGIN_MAGIC */ \
    int version; /* checked against _AUD_PLUGIN_VERSION */ \
    int type; /* PLUGIN_TYPE_XXX */ \
    int size; /* size in bytes of the struct */ \
    const char * name; \
    bool_t (* init) (void); \
    void (* cleanup) (void); \
    int (* take_message) (const char * code, const void * data, int size); \
    void (* about) (void); \
    void (* configure) (void); \
    PluginPreferences * settings;

struct _Plugin
{
    PLUGIN_COMMON_FIELDS
};

struct _TransportPlugin
{
    PLUGIN_COMMON_FIELDS
    const char * const * schemes; /* array ending with NULL */
    const VFSConstructor * vtable;
};

struct _PlaylistPlugin
{
    PLUGIN_COMMON_FIELDS
	const char * const * extensions; /* array ending with NULL */
	bool_t (* load) (const char * path, VFSFile * file,
     char * * title, /* pooled */
     Index * filenames, /* of (char *), pooled */
     Index * tuples); /* of (Tuple *) */
	bool_t (* save) (const char * path, VFSFile * file, const char * title,
     Index * filenames, /* of (char *) */
     Index * tuples); /* of (Tuple *) */
};

struct _OutputPlugin
{
    PLUGIN_COMMON_FIELDS

    /* During probing, plugins with higher priority (10 to 0) are tried first. */
    int probe_priority;

    /* Returns current volume for left and right channels (0 to 100). */
    void (* get_volume) (int * l, int * r);

    /* Changes volume for left and right channels (0 to 100). */
    void (* set_volume) (int l, int r);

    /* Begins playback of a PCM stream.  <format> is one of the FMT_*
     * enumeration values defined in libaudcore/audio.h.  Returns nonzero on
     * success. */
    bool_t (* open_audio) (int format, int rate, int chans);

    /* Ends playback.  Any buffered audio data is discarded. */
    void (* close_audio) (void);

    /* Returns how many bytes of data may be passed to a following write_audio()
     * call.  NULL if the plugin supports only blocking writes (not recommended). */
    int (* buffer_free) (void);

    /* Waits until buffer_free() will return a size greater than zero.
     * output_time(), pause(), and flush() may be called meanwhile; if flush()
     * is called, period_wait() should return immediately.  NULL if the plugin
     * supports only blocking writes (not recommended). */
    void (* period_wait) (void);

    /* Buffers <size> bytes of data, in the format given to open_audio(). */
    void (* write_audio) (void * data, int size);

    /* Waits until all buffered data has been heard by the user. */
    void (* drain) (void);

    /* Returns time count (in milliseconds) of how much data has been written. */
    int (* written_time) (void);

    /* Returns time count (in milliseconds) of how much data has been heard by
     * the user. */
    int (* output_time) (void);

    /* Pauses the stream if <p> is nonzero; otherwise unpauses it.
     * write_audio() will not be called while the stream is paused. */
    void (* pause) (bool_t p);

    /* Discards any buffered audio data and sets the time counter (in
     * milliseconds) of data written. */
    void (* flush) (int time);

    /* Sets the time counter (in milliseconds) of data written without
     * discarding any buffered audio data.  If <time> is less than the amount of
     * buffered data, following calls to output_time() will return negative
     * values. */
    void (* set_written_time) (int time);
};

struct _EffectPlugin
{
    PLUGIN_COMMON_FIELDS

    /* All processing is done in floating point.  If the effect plugin wants to
     * change the channel count or sample rate, it can change the parameters
     * passed to start().  They cannot be changed in the middle of a song. */
    void (* start) (int * channels, int * rate);

    /* process() has two options: modify the samples in place and leave the data
     * pointer unchanged or copy them into a buffer of its own.  If it sets the
     * pointer to dynamically allocated memory, it is the plugin's job to free
     * that memory.  process() may return different lengths of audio than it is
     * passed, even a zero length. */
    void (* process) (float * * data, int * samples);

    /* A seek is taking place; any buffers should be discarded. */
    void (* flush) (void);

    /* Exactly like process() except that any buffers should be drained (i.e.
     * the data processed and returned).  finish() will be called a second time
     * at the end of the last song in the playlist. */
    void (* finish) (float * * data, int * samples);

    /* Optional.  For effects that change the length of the song, these
     * functions allow the correct time to be displayed. */
    int (* decoder_to_output_time) (int time);
    int (* output_to_decoder_time) (int time);

    /* Effects with lowest order (0 to 9) are applied first. */
    int order;

    /* If the effect does not change the number of channels or the sampling
     * rate, it can be enabled and disabled more smoothly. */
    bool_t preserves_format;
};

struct OutputAPI
{
    /* In a multi-thread plugin, only one of these functions may be called at
     * once (but see pause and abort_write for exceptions to this rule). */

    /* Prepare the output system for playback in the specified format.  Returns
     * nonzero on success.  If the call fails, no other output functions may be
     * called. */
    int (* open_audio) (int format, int rate, int channels);

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
    void (* write_audio) (void * data, int length);

    /* End playback.  Any audio data currently buffered by the output system
     * will be discarded.  After the call, no other output functions, except
     * open_audio, may be called. */
    void (* close_audio) (void);

    /* Pause or unpause playback.  This function may be called during a call to
     * write_audio, in which write_audio will block until playback is unpaused
     * (but see abort_write to prevent the call from blocking). */
    void (* pause) (bool_t pause);

    /* Discard any audio data currently buffered by the output system, and set
     * the time counter to a new value.  This function is intended to be used
     * for seeking. */
    void (* flush) (int time);

    /* Returns the time counter.  Note that this represents the amount of audio
     * data passed to the output system, not the amount actually heard by the
     * user.  This function is useful for handling a changed audio format:
     * First, save the time counter using this function.  Second, call
     * close_audio and then open_audio with the new format (note that the call
     * may fail).  Finally, restore the time counter using flush. */
    int (* written_time) (void);

    /* Returns TRUE if there is data remaining in the output buffer; FALSE if
     * all data written to the output system has been heard by the user.  This
     * function should be polled (1/50 second is a reasonable delay between
     * calls) at the end of a song before calling close_audio.  Once it returns
     * FALSE, close_audio can be called without cutting off any of the end of
     * the song. */
    bool_t (* buffer_playing) (void);

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

struct _InputPlayback
{
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
    void (* set_params) (InputPlayback * p, int bitrate, int samplerate,
     int channels);

    /* Updates metadata for the stream.  Caller gives up ownership of one
     * reference to the tuple. */
    void (* set_tuple) (InputPlayback * playback, Tuple * tuple);

    /* If replay gain settings are stored in the tuple associated with the
     * current song, this function can be called (after opening audio) to apply
     * those settings.  If the settings are changed in a call to set_tuple, this
     * function must be called again to apply the updated settings. */
    void (* set_gain_from_playlist) (InputPlayback * playback);
};

struct _InputPlugin
{
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
    bool_t have_subtune;

    /* Pointer to an array (terminated with NULL) of file extensions associated
     * with file types the plugin can handle. */
    const char * const * extensions;
    /* Pointer to an array (terminated with NULL) of MIME types the plugin can
     * handle. */
    const char * const * mimes;
    /* Pointer to an array (terminated with NULL) of custom URI schemes the
     * plugin can handle. */
    const char * const * schemes;

    /* How quickly the plugin should be tried in searching for a plugin to
     * handle a file which could not be identified from its extension.  Plugins
     * with priority 0 are tried first, 10 last. */
    int priority;

    /* Must return nonzero if the plugin can handle this file.  If the file
     * could not be opened, "file" will be NULL.  (This is normal in the case of
     * special URI schemes like cdda:// that do not represent actual files.) */
    bool_t (* is_our_file_from_vfs) (const char * filename, VFSFile * file);

    /* Must return a tuple containing metadata for this file, or NULL if no
     * metadata could be read.  If the file could not be opened, "file" will be
     * NULL.  Audacious takes over one reference to the tuple returned. */
    Tuple * (* probe_for_tuple) (const char * filename, VFSFile * file);

    /* Optional.  Must write metadata from a tuple to this file.  Must return
     * nonzero on success or zero on failure.  "file" will never be NULL. */
    /* Bug: This function does not support special URI schemes like cdda://,
     * since no file name is passed. */
    bool_t (* update_song_tuple) (const Tuple * tuple, VFSFile * file);

    /* Optional, and not recommended.  Must show a window with information about
     * this file.  If this function is provided, update_song_tuple should not be. */
    /* Bug: Implementing this function duplicates user interface code and code
     * to open the file in each and every plugin. */
    void (* file_info_box) (const char * filename);

    /* Optional.  Must try to read an "album art" image embedded in this file.
     * Must return nonzero on success or zero on failure.  If the file could not
     * be opened, "file" will be NULL.  On success, must fill "data" with a
     * pointer to a block of data allocated with g_malloc and "size" with the
     * size in bytes of that block.  The data may be in any format supported by
     * GTK.  Audacious will free the data when it is no longer needed. */
    bool_t (* get_song_image) (const char * filename, VFSFile * file,
     void * * data, int64_t * size);

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
    bool_t (* play) (InputPlayback * playback, const char * filename,
     VFSFile * file, int start_time, int stop_time, bool_t pause);

    /* Must pause or unpause a file currently being played.  This function will
     * be called from a different thread than play, but it will not be called
     * before the plugin calls set_pb_ready or after stop is called. */
    void (* pause) (InputPlayback * playback, bool_t paused);

    /* Optional.  Must seek to the given position in milliseconds within a file
     * currently being played.  This function will be called from a different
     * thread than play, but it will not be called before the plugin calls
     * set_pb_ready or after stop is called. */
    void (* mseek) (InputPlayback * playback, int time);

    /* Must signal a currently playing song to stop and cause play to return.
     * This function will be called from a different thread than play.  It will
     * only be called once. It should not join the thread from which play is
     * called. */
    void (* stop) (InputPlayback * playback);

    /* Advanced, for plugins that do not use Audacious's output system.  Use at
     * your own risk. */
    int (* get_time) (InputPlayback * playback);
    int (* get_volume) (int * l, int * r);
    int (* set_volume) (int l, int r);
};

struct _GeneralPlugin
{
    PLUGIN_COMMON_FIELDS

    bool_t enabled_by_default;

    /* GtkWidget * (* get_widget) (void); */
    void * (* get_widget) (void);
};

struct _VisPlugin
{
    PLUGIN_COMMON_FIELDS

    /* reset internal state and clear display */
    void (* clear) (void);

    /* 512 frames of a single-channel PCM signal */
    void (* render_mono_pcm) (const float * pcm);

    /* 512 frames of an interleaved multi-channel PCM signal */
    void (* render_multi_pcm) (const float * pcm, int channels);

    /* intensity of frequencies 1/512, 2/512, ..., 256/512 of sample rate */
    void (* render_freq) (const float * freq);

    /* GtkWidget * (* get_widget) (void); */
    void * (* get_widget) (void);
};

struct _IfacePlugin
{
    PLUGIN_COMMON_FIELDS

    /* is_shown() may return nonzero even if the interface is not actually
     * visible; for example, if it is obscured by other windows or minimized.
     * is_focused() only returns nonzero if the interface is actually visible;
     * in X11, this should be determined by whether the interface has the
     * toplevel focus.  show() should show and raise the interface, so that both
     * is_shown() and is_focused() will return nonzero. */
    void (* show) (bool_t show);
    bool_t (* is_shown) (void);

    void (* show_error) (const char * markup);
    void (* show_filebrowser) (bool_t play_button);
    void (* show_jump_to_track) (void);

    void (* run_gtk_plugin) (void /* GtkWidget */ * widget, const char * name);
    void (* stop_gtk_plugin) (void /* GtkWidget */ * widget);

    void (* install_toolbar) (void /* GtkWidget */ * button);
    void (* uninstall_toolbar) (void /* GtkWidget */ * button);

    /* added after 3.0-alpha1 */
    bool_t (* is_focused) (void);
};

#undef PLUGIN_COMMON_FIELDS

#define AUD_PLUGIN(stype, itype, ...) \
AudAPITable * _aud_api_table = NULL; \
stype _aud_plugin_self = { \
 .magic = _AUD_PLUGIN_MAGIC, \
 .version = _AUD_PLUGIN_VERSION, \
 .type = itype, \
 .size = sizeof (stype), \
 __VA_ARGS__}; \
stype * get_plugin_info (AudAPITable * table) { \
    _aud_api_table = table; \
    return & _aud_plugin_self; \
}

#define AUD_TRANSPORT_PLUGIN(...) AUD_PLUGIN (TransportPlugin, PLUGIN_TYPE_TRANSPORT, __VA_ARGS__)
#define AUD_PLAYLIST_PLUGIN(...) AUD_PLUGIN (PlaylistPlugin, PLUGIN_TYPE_PLAYLIST, __VA_ARGS__)
#define AUD_INPUT_PLUGIN(...) AUD_PLUGIN (InputPlugin, PLUGIN_TYPE_INPUT, __VA_ARGS__)
#define AUD_EFFECT_PLUGIN(...) AUD_PLUGIN (EffectPlugin, PLUGIN_TYPE_EFFECT, __VA_ARGS__)
#define AUD_OUTPUT_PLUGIN(...) AUD_PLUGIN (OutputPlugin, PLUGIN_TYPE_OUTPUT, __VA_ARGS__)
#define AUD_VIS_PLUGIN(...) AUD_PLUGIN (VisPlugin, PLUGIN_TYPE_VIS, __VA_ARGS__)
#define AUD_GENERAL_PLUGIN(...) AUD_PLUGIN (GeneralPlugin, PLUGIN_TYPE_GENERAL, __VA_ARGS__)
#define AUD_IFACE_PLUGIN(...) AUD_PLUGIN (IfacePlugin, PLUGIN_TYPE_IFACE, __VA_ARGS__)

#define PLUGIN_HAS_FUNC(p, func) \
 ((p)->size > (char *) & (p)->func - (char *) (p) && (p)->func)

#endif /* AUDACIOUS_PLUGIN_H */
