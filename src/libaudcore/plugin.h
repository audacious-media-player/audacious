/*
 * plugin.h
 * Copyright 2005-2013 William Pitcock, Yoshiki Yazawa, Eugene Zagidullin, and
 *                     John Lindgren
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

#ifndef LIBAUDCORE_PLUGIN_H
#define LIBAUDCORE_PLUGIN_H

#include <libaudcore/audio.h>
#include <libaudcore/export.h>
#include <libaudcore/plugins.h>
#include <libaudcore/tuple.h>
#include <libaudcore/visualizer.h>
#include <libaudcore/vfs.h>

enum class AudMenuID;
struct PluginPreferences;

/* "Magic" bytes identifying an Audacious plugin header. */
#define _AUD_PLUGIN_MAGIC ((int) 0x8EAC8DE2)

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

#define _AUD_PLUGIN_VERSION_MIN 48 /* 3.8-devel */
#define _AUD_PLUGIN_VERSION     48 /* 3.8-devel */

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
 * Plugins that do not need to receive messages can set take_message() to nullptr.
 *
 * Each message includes a code indicating the type of message, a pointer to
 * some data, and a value indicating the size of that data. What the message
 * data contains is entirely up to the two plugins involved. For this reason, it
 * is crucial that both plugins agree on the meaning of the message codes used.
 *
 * Once the message is sent, an integer error code is returned. If the receiving
 * plugin does not provide the take_message() method, -1 is returned. If
 * take_message() does not recognize the message code, it should ignore the
 * message and return -1. An error code of zero represents success. Other error
 * codes may be used with more specific meanings.
 *
 * For the time being, aud_plugin_send_message() should only be called from the
 * program's main thread. */

/* plugin flags */
enum {
    PluginGLibOnly = 0x1, // plugin requires GLib main loop
    PluginQtOnly   = 0x2  // plugin requires Qt main loop
};

struct PluginInfo
{
    const char * name;
    const char * domain; // for gettext
    const char * about;
    const PluginPreferences * prefs;
    int flags;
};

class LIBAUDCORE_PUBLIC Plugin
{
public:
    constexpr Plugin (PluginType type, PluginInfo info) :
        type (type),
        info (info) {}

    const int magic = _AUD_PLUGIN_MAGIC;
    const int version = _AUD_PLUGIN_VERSION;

    const PluginType type;
    const PluginInfo info;

    virtual bool init () { return true; }
    virtual void cleanup () {}

    virtual int take_message (const char * code, const void * data, int size) { return -1; }
};

class LIBAUDCORE_PUBLIC TransportPlugin : public Plugin
{
public:
    constexpr TransportPlugin (const PluginInfo info,
     const ArrayRef<const char *> schemes) :
        Plugin (PluginType::Transport, info),
        schemes (schemes) {}

    /* supported URI schemes (without "://") */
    const ArrayRef<const char *> schemes;

    virtual VFSImpl * fopen (const char * filename, const char * mode, String & error) = 0;

    virtual VFSFileTest test_file (const char * filename, VFSFileTest test, String & error)
        { return VFSFileTest (0); }

    virtual Index<String> read_folder (const char * filename, String & error)
        { return Index<String> (); }
};

class LIBAUDCORE_PUBLIC PlaylistPlugin : public Plugin
{
public:
    constexpr PlaylistPlugin (const PluginInfo info,
     const ArrayRef<const char *> extensions, bool can_save) :
        Plugin (PluginType::Playlist, info),
        extensions (extensions),
        can_save (can_save) {}

    /* supported file extensions (without periods) */
    const ArrayRef<const char *> extensions;

    /* true if the plugin can save playlists */
    const bool can_save;

    /* path: URI of playlist file (in)
     * file: VFS handle of playlist file (in, read-only file, not seekable)
     * title: title of playlist (out)
     * items: playlist entries (out) */
    virtual bool load (const char * path, VFSFile & file, String & title,
     Index<PlaylistAddItem> & items) = 0;

    /* path: URI of playlist file (in)
     * file: VFS handle of playlist file (in, write-only file, not seekable)
     * title: title of playlist (in)
     * items: playlist entries (in) */
    virtual bool save (const char * path, VFSFile & file, const char * title,
     const Index<PlaylistAddItem> & items) { return false; }
};

class LIBAUDCORE_PUBLIC OutputPlugin : public Plugin
{
public:
    constexpr OutputPlugin (const PluginInfo info, int priority, bool force_reopen = false) :
        Plugin (PluginType::Output, info),
        priority (priority),
        force_reopen (force_reopen) {}

    /* During probing, plugins with higher priority (10 to 0) are tried first. */
    const int priority;

    /* Whether close_audio() and open_audio() must always be called between
     * songs, even if the audio format is the same.  Note that this defeats
     * gapless playback. */
    const bool force_reopen;

    /* Returns current volume for left and right channels (0 to 100). */
    virtual StereoVolume get_volume () = 0;

    /* Changes volume for left and right channels (0 to 100). */
    virtual void set_volume (StereoVolume volume) = 0;

    /* Sets information about the song being played.  This function will be
     * called before open_audio(). */
    virtual void set_info (const char * filename, const Tuple & tuple) {}

    /* Begins playback of a PCM stream.  <format> is one of the FMT_*
     * enumeration values defined in libaudcore/audio.h.  Returns true on
     * success. */
    virtual bool open_audio (int format, int rate, int chans, String & error) = 0;

    /* Ends playback.  Any buffered audio data is discarded. */
    virtual void close_audio () = 0;

    /* Waits until write_audio() will return a size greater than zero.
     * get_delay(), pause(), and flush() may be called meanwhile; if flush()
     * is called, period_wait() should return immediately. */
    virtual void period_wait () = 0;

    /* Writes up to <size> bytes of data, in the format given to open_audio().
     * If there is not enough buffer space for all <size> bytes, writes only as
     * many bytes as can be written immediately without blocking.  Returns the
     * number of bytes actually written. */
    virtual int write_audio (const void * data, int size) = 0;

    /* Waits until all buffered data has been heard by the user. */
    virtual void drain () = 0;

    /* Returns an estimate of how many milliseconds will pass before all the
     * data passed to write_audio() has been heard by the user. */
    virtual int get_delay () = 0;

    /* Pauses the stream if <p> is nonzero; otherwise unpauses it.
     * write_audio() will not be called while the stream is paused. */
    virtual void pause (bool pause) = 0;

    /* Discards any buffered audio data. */
    virtual void flush () = 0;
};

class LIBAUDCORE_PUBLIC EffectPlugin : public Plugin
{
public:
    constexpr EffectPlugin (const PluginInfo info, int order, bool preserves_format) :
        Plugin (PluginType::Effect, info),
        order (order),
        preserves_format (preserves_format) {}

    /* Effects with lowest order (0 to 9) are applied first. */
    const int order;

    /* If the effect does not change the number of channels or the sampling
     * rate, it can be enabled and disabled more smoothly. */
    const bool preserves_format;

    /* All processing is done in floating point.  If the effect plugin wants to
     * change the channel count or sample rate, it can change the parameters
     * passed to start().  They cannot be changed in the middle of a song. */
    virtual void start (int & channels, int & rate) = 0;

    /* Performs effect processing.  process() may modify the audio samples in
     * place and return a reference to the same buffer, or it may return a
     * reference to an internal working buffer.  The number of output samples
     * need not be the same as the number of input samples. */
    virtual Index<float> & process (Index<float> & data) = 0;

    /* Optional.  A seek is taking place; any buffers should be discarded.
     * Unless the "force" flag is set, the plugin may choose to override the
     * normal flush behavior and handle the flush itself (for example, to
     * perform crossfading).  The flush() function should return false in this
     * case to prevent flush() from being called in downstream effect plugins. */
    virtual bool flush (bool force)
        { return true; }

    /* Exactly like process() except that any buffers should be drained (i.e.
     * the data processed and returned).  finish() will be called a second time
     * at the end of the last song in the playlist. */
    virtual Index<float> & finish (Index<float> & data, bool end_of_playlist)
        { return process (data); }

    /* Required only for plugins that change the time domain (e.g. a time
     * stretch) or use read-ahead buffering.  translate_delay() must do two
     * things: first, translate <delay> (which is in milliseconds) from the
     * output time domain back to the input time domain; second, increase
     * <delay> by the size of the read-ahead buffer.  It should return the
     * adjusted delay. */
    virtual int adjust_delay (int delay)
        { return delay; }
};

enum class InputKey {
    Ext,
    MIME,
    Scheme,
    count
};

class LIBAUDCORE_PUBLIC InputPlugin : public Plugin
{
public:
    enum {
        /* Indicates that the plugin can write file tags */
        FlagWritesTag = (1 << 0),

        /* Indicates that files handled by the plugin may contain more than one
         * song.  When reading the tuple for such a file, the plugin should set
         * the FIELD_SUBSONG_NUM field to the number of songs in the file.  For
         * all other files, the field should be left unset.
         *
         * Example:
         * 1. User adds a file named "somefile.xxx" to the playlist.  Having
         * determined that this plugin can handle the file, Audacious opens the
         * file and calls probe_for_tuple().  probe_for_tuple() sees that there
         * are 3 songs in the file and sets FIELD_SUBSONG_NUM to 3.
         * 2. For each song in the file, Audacious opens the file and calls
         * probe_for_tuple(); this time, however, a question mark and song
         * number are appended to the file name passed: "somefile.sid?2" refers
         * to the second song in the file "somefile.sid".
         * 3. When one of the songs is played, Audacious opens the file and
         * calls play() with a file name modified in this way. */
        FlagSubtunes = (1 << 1)
    };

    struct InputInfo
    {
        typedef const char * const * List;

        int flags, priority;
        aud::array<InputKey, List> keys;

        constexpr InputInfo (int flags = 0) :
            flags (flags), priority (0), keys {} {}

        /* Associates file extensions with the plugin. */
        constexpr InputInfo with_exts (List exts) const
            { return InputInfo (flags, priority,
              exts, keys[InputKey::MIME], keys[InputKey::Scheme]); }

        /* Associates MIME types with the plugin. */
        constexpr InputInfo with_mimes (List mimes) const
            { return InputInfo (flags, priority,
              keys[InputKey::Ext], mimes, keys[InputKey::Scheme]); }

        /* Associates custom URI schemes with the plugin.  Plugins using custom
         * URI schemes are expected to handle their own I/O.  Hence, any VFSFile
         * passed to play(), read_tuple(), etc. will be null. */
        constexpr InputInfo with_schemes (List schemes) const
            { return InputInfo (flags, priority,
              keys[InputKey::Ext], keys[InputKey::MIME], schemes); }

        /* Sets how quickly the plugin should be tried in searching for a plugin
         * to handle a file which could not be identified from its extension.
         * Plugins with priority 0 are tried first, 10 last. */
        constexpr InputInfo with_priority (int priority) const
            { return InputInfo (flags, priority,
              keys[InputKey::Ext], keys[InputKey::MIME], keys[InputKey::Scheme]); }

    private:
        constexpr InputInfo (int flags, int priority, List exts, List mimes, List schemes) :
            flags (flags), priority (priority), keys {exts, mimes, schemes} {}
    };

    constexpr InputPlugin (PluginInfo info, InputInfo input_info) :
        Plugin (PluginType::Input, info),
        input_info (input_info) {}

    const InputInfo input_info;

    /* Returns true if the plugin can handle the file. */
    virtual bool is_our_file (const char * filename, VFSFile & file) = 0;

    /* Reads metadata and album art (if requested and available) from the file.
     * The filename fields of the tuple are already set before the function is
     * called.  If album art is not needed, <image> will be nullptr.  The return
     * value should be true if <tuple> was successfully read, regardless of
     * whether album art was read. */
    virtual bool read_tag (const char * filename, VFSFile & file, Tuple & tuple,
     Index<char> * image) = 0;

    /* Plays the file.  Returns false on error.  Also see input-api.h. */
    virtual bool play (const char * filename, VFSFile & file) = 0;

    /* Optional.  Writes metadata to the file, returning false on error. */
    virtual bool write_tuple (const char * filename, VFSFile & file, const Tuple & tuple)
        { return false; }

    /* Optional.  Displays a window showing info about the file.  In general,
     * this function should be avoided since Audacious already provides a file
     * info window. */
    virtual bool file_info_box (const char * filename, VFSFile & file)
        { return false; }

protected:
    /* Prepares the output system for playback in the specified format.  Also
     * triggers the "playback ready" hook.  Hence, if you call set_replay_gain,
     * set_playback_tuple, or set_stream_bitrate, consider doing so before
     * calling open_audio.  There is no return value.  If the requested audio
     * format is not supported, write_audio() will do nothing and check_stop()
     * will immediately return true. */
    static void open_audio (int format, int rate, int channels);

    /* Informs the output system of replay gain values for the current song so
     * that volume levels can be adjusted accordingly, if the user so desires.
     * This may be called at any time during playback should the values change. */
    static void set_replay_gain (const ReplayGainInfo & gain);

    /* Passes audio data to the output system for playback.  The data must be in
     * the format passed to open_audio(), and the length (in bytes) must be an
     * integral number of frames.  This function blocks until all the data has
     * been written (though it may not yet be heard by the user). */
    static void write_audio (const void * data, int length);

    /* Returns the current tuple for the stream. */
    static Tuple get_playback_tuple ();

    /* Updates the tuple for the stream. */
    static void set_playback_tuple (Tuple && tuple);

    /* Updates the displayed bitrate, in bits per second. */
    static void set_stream_bitrate (int bitrate);

    /* Checks whether playback is to be stopped.  The play() function should
     * poll check_stop() periodically and return as soon as check_stop() returns
     * true. */
    static bool check_stop ();

    /* Checks whether a seek has been requested.  If so, returns the position to
     * seek to, in milliseconds.  Otherwise, returns -1. */
    static int check_seek ();
};

class LIBAUDCORE_PUBLIC DockablePlugin : public Plugin
{
public:
    constexpr DockablePlugin (PluginType type, PluginInfo info) :
        Plugin (type, info) {}

    /* GtkWidget * get_gtk_widget () */
    virtual void * get_gtk_widget () { return nullptr; }

    /* QWidget * get_qt_widget () */
    virtual void * get_qt_widget () { return nullptr; }
};

class LIBAUDCORE_PUBLIC GeneralPlugin : public DockablePlugin
{
public:
    constexpr GeneralPlugin (PluginInfo info, bool enabled_by_default) :
        DockablePlugin (PluginType::General, info),
        enabled_by_default (enabled_by_default) {}

    const bool enabled_by_default;
};

class LIBAUDCORE_PUBLIC VisPlugin : public DockablePlugin, public Visualizer
{
public:
    constexpr VisPlugin (PluginInfo info, int type_mask) :
        DockablePlugin (PluginType::Vis, info),
        Visualizer (type_mask) {}
};

class LIBAUDCORE_PUBLIC IfacePlugin : public Plugin
{
public:
    constexpr IfacePlugin (PluginInfo info) :
        Plugin (PluginType::Iface, info) {}

    virtual void show (bool show) = 0;
    virtual void run () = 0;
    virtual void quit () = 0;

    virtual void show_about_window () = 0;
    virtual void hide_about_window () = 0;
    virtual void show_filebrowser (bool open) = 0;
    virtual void hide_filebrowser () = 0;
    virtual void show_jump_to_song () = 0;
    virtual void hide_jump_to_song () = 0;
    virtual void show_prefs_window () = 0;
    virtual void hide_prefs_window () = 0;
    virtual void plugin_menu_add (AudMenuID id, void func (), const char * name, const char * icon) = 0;
    virtual void plugin_menu_remove (AudMenuID id, void func ()) = 0;

    virtual void startup_notify (const char * id) {}
};

#endif
