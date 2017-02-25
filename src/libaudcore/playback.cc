/*
 * playback.cc
 * Copyright 2009-2014 John Lindgren
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

/*
 * Since Audacious 3.6, the playback thread is completely asynchronous; that is,
 * the main thread never blocks waiting for the playback thread to process a
 * play(), seek(), or stop() command.  As a result, the playback thread can lag
 * behind the main/playlist thread, and the "current" song from the perspective
 * of the playback thread may not be the same as the "current" song from the
 * perspective of the main/playlist thread.  Therefore, some care is necessary
 * to ensure that information generated in the playback thread is not applied to
 * the wrong song.  To this end, separate serial numbers are maintained by each
 * thread and compared when information crosses thread boundaries; if the serial
 * numbers do not match, the information is generally discarded.
 *
 * Note that this file and playlist.cc each have their own mutex.  The one in
 * playlist.cc is conceptually the "outer" mutex and must be locked first (in
 * situations where both need to be locked) in order to avoid deadlock.
 */

#include "drct.h"
#include "internal.h"

#include <assert.h>
#include <pthread.h>

#include "audstrings.h"
#include "hook.h"
#include "i18n.h"
#include "interface.h"
#include "mainloop.h"
#include "output.h"
#include "playlist-internal.h"
#include "plugin.h"
#include "plugins.h"
#include "plugins-internal.h"
#include "runtime.h"

struct PlaybackState {
    bool playing = false;
    bool thread_running = false;
    int control_serial = 0;
    int playback_serial = 0;
};

struct PlaybackControl {
    bool paused = false;
    int seek = -1;
    int repeat_a = -1;
    int repeat_b = -1;
};

struct PlaybackInfo {
    // set by playback_set_info
    int entry = -1;
    Tuple tuple;
    String title;

    // set by playback thread
    String filename;
    int length = -1;
    int time_offset = 0;
    int stop_time = -1;

    ReplayGainInfo gain {};

    int bitrate = 0;
    int samplerate = 0;
    int channels = 0;

    bool ready = false;
    bool ended = false;
    bool error = false;
    String error_s;
};

static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t cond = PTHREAD_COND_INITIALIZER;

static PlaybackState pb_state;
static PlaybackControl pb_control;
static PlaybackInfo pb_info;

static QueuedFunc end_queue;
static bool song_finished = false;
static int failed_entries = 0;

static void lock ()
    { pthread_mutex_lock (& mutex); }
static void unlock ()
    { pthread_mutex_unlock (& mutex); }

static bool lock_if (bool (* test) ())
{
    lock ();
    if (test ())
        return true;

    unlock ();
    return false;
}

// check that the playback thread is not lagging
static bool in_sync ()
    { return pb_state.playing && pb_state.control_serial == pb_state.playback_serial; }

// check that the playback thread is not lagging and playback is "ready"
static bool is_ready ()
    { return in_sync () && pb_info.ready; }

// called by playback_entry_set_tuple() to ensure that the tuple still applies
// to the current song from the perspective of the main/playlist thread; the
// check is necessary because playback_entry_set_tuple() is itself called from
// the playback thread
bool playback_check_serial (int serial)
{
    lock ();
    bool okay = (pb_state.playing && pb_state.control_serial == serial);
    unlock ();
    return okay;
}

// called from the playlist to update the tuple for the current song
void playback_set_info (int entry, Tuple && tuple)
{
    // do nothing if the playback thread is lagging behind;
    // in that case, playback_set_info() will get called again anyway
    if (! lock_if (in_sync))
        return;

    if (tuple.valid () && tuple != pb_info.tuple)
    {
        pb_info.tuple = std::move (tuple);

        // don't call "tuple change" before "playback ready"
        if (is_ready ())
        {
            event_queue ("tuple change", nullptr);
            output_set_tuple (pb_info.tuple);
        }
    }

    String title = pb_info.tuple.get_str (Tuple::FormattedTitle);
    if (entry != pb_info.entry || title != pb_info.title)
    {
        pb_info.entry = entry;
        pb_info.title = title;

        // don't call "title change" before "playback ready"
        if (is_ready ())
            event_queue ("title change", nullptr);
    }

    unlock ();
}

// cleanup common to both playback_play() and playback_stop()
static void playback_cleanup_locked ()
{
    pb_state.playing = false;
    pb_control = PlaybackControl ();

    // discard audio buffer if the song did not end on its own
    if (! song_finished)
        output_flush (0);

    // miscellaneous cleanup
    end_queue.stop ();
    song_finished = false;

    event_queue_cancel ("playback ready");
    event_queue_cancel ("playback pause");
    event_queue_cancel ("playback unpause");
    event_queue_cancel ("playback seek");
    event_queue_cancel ("info change");
    event_queue_cancel ("title change");
    event_queue_cancel ("tuple change");

    aud_set_bool (nullptr, "stop_after_current_song", false);
}

// main thread: stops playback when no more songs are to be played
void playback_stop (bool exiting)
{
    if (! pb_state.playing && ! exiting)
        return;

    lock ();

    if (pb_state.playing)
        playback_cleanup_locked ();

    if (pb_state.thread_running)
    {
        // discard audio buffer if exiting
        if (exiting)
            output_flush (0, true);

        // signal playback thread to drain audio buffer
        pb_state.control_serial ++;
        pthread_cond_broadcast (& cond);

        // wait for playback thread to finish if exiting
        while (exiting && pb_state.thread_running)
            pthread_cond_wait (& cond, & mutex);
    }

    unlock ();

    // miscellaneous cleanup
    failed_entries = 0;
}

// called from top-level event loop after playback finishes
static void end_cb (void *)
{
    song_finished = true;
    hook_call ("playback end", nullptr);

    PlaylistEx playlist = Playlist::playing_playlist ();

    auto do_stop = [playlist] ()
    {
        aud_drct_stop ();
        playlist.set_position (playlist.get_position ());
    };

    auto do_next = [playlist] ()
    {
        if (! playlist.next_song (aud_get_bool (nullptr, "repeat")))
        {
            playlist.set_position (-1);
            hook_call ("playlist end reached", nullptr);
        }
    };

    if (aud_get_bool (nullptr, "no_playlist_advance"))
    {
        // we assume here that repeat is not enabled;
        // single-song repeats are handled in run_playback()
        do_stop ();
    }
    else if (aud_get_bool (nullptr, "stop_after_current_song"))
    {
        do_stop ();
        do_next ();
    }
    else
    {
        if (failed_entries < 10)
            do_next ();
        else
            do_stop ();
    }
}

// helper, can be called from either main or playback thread
static void request_seek_locked (int time)
{
    // set up "seek" command whether ready or not;
    // if not ready, it will take effect upon open_audio()
    pb_control.seek = aud::max (0, time);

    // trigger seek immediately if ready
    if (is_ready () && pb_info.length > 0)
    {
        output_flush (aud::clamp (time, 0, pb_info.length));
        event_queue ("playback seek", nullptr);
    }
}

// playback thread helper
static void run_playback ()
{
    // due to mutex ordering, we cannot call into the playlist while locked
    DecodeInfo dec = playback_entry_read (pb_state.playback_serial);

    if (! lock_if (in_sync))
        return;

    // for a cuesheet entry, determine the source filename
    pb_info.filename = pb_info.tuple.get_str (Tuple::AudioFile);
    if (! pb_info.filename)
        pb_info.filename = std::move (dec.filename);

    // check that we have all the necessary data
    if (! pb_info.filename || ! pb_info.tuple.valid () || ! dec.ip ||
     (! dec.ip->input_info.keys[InputKey::Scheme] && ! dec.file))
    {
        pb_info.error = true;
        pb_info.error_s = std::move (dec.error);
        unlock ();
        return;
    }

    // get various other bits of info from the tuple
    pb_info.length = pb_info.tuple.get_int (Tuple::Length);
    pb_info.time_offset = aud::max (0, pb_info.tuple.get_int (Tuple::StartTime));
    pb_info.stop_time = aud::max (-1, pb_info.tuple.get_int (Tuple::EndTime) - pb_info.time_offset);
    pb_info.gain = pb_info.tuple.get_replay_gain ();

    // force initial seek if we are playing a segmented track
    if (pb_info.time_offset > 0 && pb_control.seek < 0)
        pb_control.seek = 0;

    unlock ();

    while (1)
    {
        // hand off control to input plugin
        if (! dec.ip->play (pb_info.filename, dec.file))
            pb_info.error = true;

        // close audio (no-op if it wasn't opened)
        output_close_audio ();

        if (pb_info.error || pb_info.length <= 0)
            break;

        if (! lock_if (in_sync))
            break;

        // check whether we need to repeat
        pb_info.ended = (pb_control.repeat_a < 0 && ! (aud_get_bool (nullptr,
         "repeat") && aud_get_bool (nullptr, "no_playlist_advance")));

        if (! pb_info.ended)
            request_seek_locked (pb_control.repeat_a);

        unlock ();

        if (pb_info.ended)
            break;

        // rewind file pointer before repeating
        if (! open_input_file (pb_info.filename, "r", dec.ip, dec.file, & pb_info.error_s))
        {
            pb_info.error = true;
            break;
        }
    }
}

// playback thread helper
static void finish_playback_locked ()
{
    // record any playback error that occurred
    if (pb_info.error)
    {
        failed_entries ++;

        if (pb_info.error_s)
            aud_ui_show_error (str_printf (_("Error playing %s:\n%s"),
             (const char *) pb_info.filename, (const char *) pb_info.error_s));
        else
            AUDERR ("Playback finished with error.\n");
    }
    else
        failed_entries = 0;

    // queue up function to start next song (or perform cleanup)
    end_queue.queue (end_cb, nullptr);
}

// playback thread
static void * playback_thread (void *)
{
    lock ();

    while (1)
    {
        // wait for a command
        while (pb_state.control_serial == pb_state.playback_serial)
            pthread_cond_wait (& cond, & mutex);

        // fetch the command (either "play" or "drain")
        bool play = pb_state.playing;

        // update playback thread serial number
        pb_state.playback_serial = pb_state.control_serial;

        unlock ();

        if (play)
            run_playback ();
        else
            output_drain ();

        lock ();

        if (play)
        {
            // don't report errors or queue next song if another command is pending
            if (in_sync ())
                finish_playback_locked ();

            pb_info = PlaybackInfo ();
        }
        else
        {
            // quit if we did not receive a new command after draining
            if (pb_state.control_serial == pb_state.playback_serial)
                break;
        }
    }

    // signal the main thread that we are quitting
    pb_state.thread_running = false;
    pthread_cond_broadcast (& cond);
    unlock ();
    return nullptr;
}

// main thread: starts playback of a new song
void playback_play (int seek_time, bool pause)
{
    lock ();

    if (pb_state.playing)
        playback_cleanup_locked ();

    // set up "play" command
    pb_state.playing = true;
    pb_state.control_serial ++;
    pb_control.paused = pause;
    pb_control.seek = (seek_time > 0) ? seek_time : -1;

    // start playback thread (or signal it if it's already running)
    if (pb_state.thread_running)
        pthread_cond_broadcast (& cond);
    else
    {
        pthread_t thread;
        pthread_create (& thread, nullptr, playback_thread, nullptr);
        pthread_detach (thread);
        pb_state.thread_running = true;
    }

    unlock ();
}

// main thread
EXPORT void aud_drct_pause ()
{
    if (! pb_state.playing)
        return;

    lock ();

    // set up "pause" command whether ready or not;
    // if not ready, it will take effect upon open_audio()
    bool pause = ! pb_control.paused;
    pb_control.paused = pause;

    // apply pause immediately if ready
    if (is_ready ())
        output_pause (pause);

    event_queue (pause ? "playback pause" : "playback unpause", nullptr);

    unlock ();
}

// main thread
EXPORT void aud_drct_seek (int time)
{
    if (! pb_state.playing)
        return;

    lock ();
    request_seek_locked (time);
    unlock ();
}

EXPORT void InputPlugin::open_audio (int format, int rate, int channels)
{
    // don't open audio if playback thread is lagging
    if (! lock_if (in_sync))
        return;

    if (! output_open_audio (pb_info.filename, pb_info.tuple, format, rate,
     channels, aud::max (0, pb_control.seek)))
    {
        pb_info.error = true;
        pb_info.error_s = String (_("Invalid audio format"));
        unlock ();
        return;
    }

    output_set_replay_gain (pb_info.gain);
    if (pb_control.paused)
        output_pause (true);

    pb_info.samplerate = rate;
    pb_info.channels = channels;

    if (pb_info.ready)
        event_queue ("info change", nullptr);
    else
        event_queue ("playback ready", nullptr);

    pb_info.ready = true;

    unlock ();
}

EXPORT void InputPlugin::set_replay_gain (const ReplayGainInfo & gain)
{
    lock ();
    pb_info.gain = gain;

    if (is_ready ())
        output_set_replay_gain (gain);

    unlock ();
}

EXPORT void InputPlugin::write_audio (const void * data, int length)
{
    if (! lock_if (in_sync))
        return;

    // fetch A-B repeat settings
    int a = pb_control.repeat_a;
    int b = pb_control.repeat_b;

    unlock ();

    // it's okay to call output_write_audio() even if we are no longer in sync,
    // since it will return immediately if output_flush() has been called
    int stop_time = (b >= 0) ? b : pb_info.stop_time;
    if (output_write_audio (data, length, stop_time))
        return;

    if (! lock_if (in_sync))
        return;

    // if we are still in sync, then one of the following happened:
    // 1. output_flush() was called due to a seek request
    // 2. we've reached repeat point B
    // 3. we've reached the end of a segmented track
    if (pb_control.seek < 0)
    {
        if (b >= 0)
            request_seek_locked (a);
        else
            pb_info.ended = true;
    }

    unlock ();
}

EXPORT Tuple InputPlugin::get_playback_tuple ()
{
    lock ();
    Tuple tuple = pb_info.tuple.ref ();
    unlock ();

    // tuples passed to us from input plugins do not have fallback fields
    // generated; for consistency, tuples passed back should not either
    tuple.delete_fallbacks ();
    return tuple;
}

EXPORT void InputPlugin::set_playback_tuple (Tuple && tuple)
{
    // due to mutex ordering, we cannot call into the playlist while locked;
    // instead, playback_entry_set_tuple() calls back into first
    // playback_check_serial() and then eventually playback_set_info()
    playback_entry_set_tuple (pb_state.playback_serial, std::move (tuple));
}

EXPORT void InputPlugin::set_stream_bitrate (int bitrate)
{
    lock ();
    pb_info.bitrate = bitrate;

    if (is_ready ())
        event_queue ("info change", nullptr);

    unlock ();
}

EXPORT bool InputPlugin::check_stop ()
{
    lock ();
    bool stop = ! is_ready () || pb_info.ended || pb_info.error;
    unlock ();
    return stop;
}

EXPORT int InputPlugin::check_seek ()
{
    lock ();
    int seek = -1;

    if (is_ready () && pb_control.seek >= 0 && pb_info.length > 0)
    {
        seek = pb_info.time_offset + aud::min (pb_control.seek, pb_info.length);
        pb_control.seek = -1;
        output_resume ();
    }

    unlock ();
    return seek;
}

// thread-safe
EXPORT bool aud_drct_get_playing ()
{
    lock ();
    bool playing = pb_state.playing;
    unlock ();
    return playing;
}

// thread-safe
EXPORT bool aud_drct_get_ready ()
{
    lock ();
    bool ready = is_ready ();
    unlock ();
    return ready;
}

// thread-safe
EXPORT bool aud_drct_get_paused ()
{
    lock ();
    bool paused = pb_control.paused;
    unlock ();
    return paused;
}

// thread-safe
EXPORT String aud_drct_get_title ()
{
    if (! lock_if (is_ready))
        return String ();

    String title = pb_info.title;
    int entry = pb_info.entry;
    int length = pb_info.length;

    unlock ();

    StringBuf prefix = aud_get_bool (nullptr, "show_numbers_in_pl") ?
     str_printf ("%d. ", 1 + entry) : StringBuf (0);

    StringBuf time = (length > 0) ? str_format_time (length) : StringBuf ();
    StringBuf suffix = time ? str_concat ({" (", time, ")"}) : StringBuf (0);

    return String (str_concat ({prefix, title, suffix}));
}

// thread-safe
EXPORT Tuple aud_drct_get_tuple ()
{
    lock ();
    Tuple tuple = is_ready () ? pb_info.tuple.ref () : Tuple ();
    unlock ();
    return tuple;
}

// thread-safe
EXPORT void aud_drct_get_info (int & bitrate, int & samplerate, int & channels)
{
    lock ();

    bool ready = is_ready ();
    bitrate = ready ? pb_info.bitrate : 0;
    samplerate = ready ? pb_info.samplerate : 0;
    channels = ready ? pb_info.channels : 0;

    unlock ();
}

// thread-safe
EXPORT int aud_drct_get_time ()
{
    lock ();
    int time = is_ready () ? output_get_time () : 0;
    unlock ();
    return time;
}

// thread-safe
EXPORT int aud_drct_get_length ()
{
    lock ();
    int length = is_ready () ? pb_info.length : -1;
    unlock ();
    return length;
}

// main thread
EXPORT void aud_drct_set_ab_repeat (int a, int b)
{
    if (! pb_state.playing)
        return;

    lock ();

    pb_control.repeat_a = a;
    pb_control.repeat_b = b;

    if (b >= 0 && is_ready () && output_get_time () >= b)
        request_seek_locked (a);

    unlock ();
}

// thread-safe
EXPORT void aud_drct_get_ab_repeat (int & a, int & b)
{
    lock ();
    a = pb_control.repeat_a;
    b = pb_control.repeat_b;
    unlock ();
}
