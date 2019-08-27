/*
 * output.c
 * Copyright 2009-2015 John Lindgren
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

#include "output.h"

#include <assert.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

#include "equalizer.h"
#include "hook.h"
#include "i18n.h"
#include "interface.h"
#include "internal.h"
#include "plugin.h"
#include "plugins.h"
#include "runtime.h"
#include "threads.h"

/* With Audacious 3.7, there is some support for secondary output plugins.
 * Notes and limitations:
 *  - Only one secondary output can be in use at a time.
 *  - A reduced API is used, consisting of only open_audio(), close_audio(), and
 *    write_audio().
 *  - The primary and secondary outputs are run from the same thread, with
 *    timing controlled by the primary's period_wait().  To avoid dropouts in
 *    the primary output, the secondary's write_audio() must be able to process
 *    audio faster than realtime.
 *  - The secondary's write_audio() is called in a tight loop until it has
 *    caught up to the primary, and should never return a zero byte count. */

/* Locking in this module is complicated by the fact that some of the
 * output plugin functions (specifically period_wait() and drain()) are
 * blocking calls.  Various other functions are designed to be called
 * in parallel with period_wait() and drain().
 *
 * A two-mutex scheme is used to ensure that safe operations are allowed
 * while a blocking call is in progress, and unsafe operations are
 * prevented.  The "major" mutex is locked before an unsafe operations;
 * only the "minor" mutex is locked before a safe one.  The "minor" mutex
 * protects all changes to state data.
 *
 * The thread performing a blocking call (i.e. the input thread) must
 * perform the following sequence:
 *
 *   1. Lock the major mutex
 *   2. Lock the minor mutex
 *   3. Check whether state data is correct for the call
 *   4. Unlock the minor mutex
 *   5. Call the blocking function
 *   6. Unlock the major mutex
 *
 * The following classes attempt to enforce some of the rules regarding
 * locking and state data. */

/* locks held for a "safe" operation */
struct SafeLock {
    aud::mutex::holder minor;
};

/* locks held for an "unsafe" operation */
struct UnsafeLock : SafeLock {
    aud::mutex::holder major;
};

class OutputState
{
public:
    bool input () const { return (m_flags & INPUT); }
    bool output () const { return (m_flags & OUTPUT); }
    bool secondary () const { return (m_flags & SECONDARY); }
    bool paused () const { return (m_flags & PAUSED); }
    bool flushed () const { return (m_flags & FLUSHED); }
    bool resetting () const { return (m_flags & RESETTING); }

    SafeLock lock_safe ()
        { return {mutex_minor.take ()}; }

    UnsafeLock lock_unsafe () {
        UnsafeLock lock;
        lock.major = mutex_major.take ();
        lock.minor = mutex_minor.take ();
        return lock;
    }

    /* safe state changes */
    void set_secondary (SafeLock &, bool on) { set_flag (SECONDARY, on); }
    void set_paused (SafeLock &, bool on) { set_flag (PAUSED, on); }
    void set_flushed (SafeLock &, bool on) { set_flag (FLUSHED, on); }
    void set_resetting (SafeLock &, bool on) { set_flag (RESETTING, on); }

    /* unsafe state changes */
    void set_input (UnsafeLock &, bool on) { set_flag (INPUT, on); }
    void set_output (UnsafeLock &, bool on) { set_flag (OUTPUT, on); }

    void await_change (SafeLock & lock)
        { cond.wait (lock.minor); }

private:
    static constexpr int INPUT     = (1 << 0);  /* input plugin connected */
    static constexpr int OUTPUT    = (1 << 1);  /* primary output plugin connected */
    static constexpr int SECONDARY = (1 << 2);  /* secondary output plugin connected */
    static constexpr int PAUSED    = (1 << 3);  /* paused */
    static constexpr int FLUSHED   = (1 << 4);  /* flushed, writes ignored until resume */
    static constexpr int RESETTING = (1 << 5);  /* resetting output system */

    int m_flags = 0;

    aud::mutex mutex_major;
    aud::mutex mutex_minor;

    aud::condvar cond;

    void set_flag (int flag, bool on)
    {
        m_flags = on ? (m_flags | flag) : (m_flags & ~flag);
        /* wake any thread waiting for a state change */
        cond.notify_all ();
    }
};

static OutputState state;

static OutputPlugin * cop; /* current (primary) output plugin */
static OutputPlugin * sop; /* secondary output plugin */

static OutputStream record_stream;

static int seek_time;
static String in_filename;
static Tuple in_tuple;
static int in_format, in_channels, in_rate;
static int effect_channels, effect_rate;
static int sec_channels, sec_rate;
static int out_format, out_channels, out_rate;
static int out_bytes_per_sec, out_bytes_held;
static int64_t in_frames, out_bytes_written;
static ReplayGainInfo gain_info;
static bool gain_info_valid;

static Index<float> buffer1;
static Index<char> buffer2;

static inline int get_format (bool & automatic)
{
    automatic = false;

    switch (aud_get_int ("output_bit_depth"))
    {
        case 16: return FMT_S16_NE;
        case 24: return FMT_S24_3NE;
        case 32: return FMT_S32_NE;

        // return FMT_FLOAT for "auto" as well
        case -1: automatic = true;
        default: return FMT_FLOAT;
    }
}

static void setup_effects (SafeLock &)
{
    assert (state.input ());

    effect_channels = in_channels;
    effect_rate = in_rate;

    effect_start (effect_channels, effect_rate);
    eq_set_format (effect_channels, effect_rate);
}

static void cleanup_output (UnsafeLock & lock)
{
    if (! state.output ())
        return;

    // avoid locking up if the input thread reaches close_audio() while
    // paused (unlikely but possible with perfect timing)
    if (out_bytes_written && ! state.paused ())
    {
        lock.minor.unlock ();
        cop->drain ();
        lock.minor.lock ();
    }

    state.set_output (lock, false);

    buffer1.clear ();
    buffer2.clear ();

    cop->close_audio ();
    vis_runner_start_stop (false, false);
}

static void cleanup_secondary (SafeLock & lock)
{
    if (! state.secondary ())
        return;

    state.set_secondary (lock, false);
    sop->close_audio ();
}

static void apply_pause (SafeLock & lock, bool pause, bool new_output = false)
{
    if (state.output ())
    {
        // assume output plugin is unpaused after open_audio()
        if (pause != (new_output ? false : state.paused ()))
            cop->pause (pause);

        vis_runner_start_stop (true, pause);
    }

    state.set_paused (lock, pause);
}

static bool open_audio_with_info (OutputPlugin * op, const char * filename,
 const Tuple & tuple, int format, int rate, int chans, String & error)
{
    op->set_info (filename, tuple);
    return op->open_audio (format, rate, chans, error);
}

static void setup_output (UnsafeLock & lock, bool new_input, bool pause)
{
    assert (state.input ());

    if (! cop)
        return;

    bool automatic;
    int format = get_format (automatic);

    if (state.output () && effect_channels == out_channels &&
     effect_rate == out_rate && ! (new_input && cop->force_reopen))
    {
        AUDINFO ("Reuse output, %d channels, %d Hz.\n", effect_channels, effect_rate);
        apply_pause (lock, pause);
        return;
    }

    AUDINFO ("Setup output, format %d, %d channels, %d Hz.\n", format, effect_channels, effect_rate);

    cleanup_output (lock);

    String error;
    while (! open_audio_with_info (cop, in_filename, in_tuple, format,
     effect_rate, effect_channels, error))
    {
        if (automatic && format == FMT_FLOAT)
            format = FMT_S32_NE;
        else if (automatic && format == FMT_S32_NE)
            format = FMT_S16_NE;
        else if (format == FMT_S24_3NE)
            format = FMT_S24_NE; /* some output plugins support only padded 24-bit */
        else
        {
            aud_ui_show_error (error ? (const char *) error : _("Error opening output stream"));
            return;
        }

        AUDINFO ("Falling back to format %d.\n", format);
    }

    state.set_output (lock, true);

    out_format = format;
    out_channels = effect_channels;
    out_rate = effect_rate;

    out_bytes_per_sec = FMT_SIZEOF (format) * out_channels * out_rate;
    out_bytes_held = 0;
    out_bytes_written = 0;

    apply_pause (lock, pause, true);
}

static void setup_secondary (SafeLock & lock, bool new_input)
{
    assert (state.input ());

    if (! sop)
        return;

    int rate, channels;
    record_stream = (OutputStream) aud_get_int ("record_stream");

    if (record_stream < OutputStream::AfterEffects)
    {
        rate = in_rate;
        channels = in_channels;
    }
    else
    {
        rate = effect_rate;
        channels = effect_channels;
    }

    if (state.secondary () && channels == sec_channels && rate == sec_rate &&
     ! (new_input && sop->force_reopen))
        return;

    cleanup_secondary (lock);

    String error;
    if (! open_audio_with_info (sop, in_filename, in_tuple, FMT_FLOAT, rate, channels, error))
    {
        aud_ui_show_error (error ? (const char *) error : _("Error recording output stream"));
        return;
    }

    state.set_secondary (lock, true);

    sec_channels = channels;
    sec_rate = rate;
}

static void flush_output (SafeLock &)
{
    assert (state.output ());

    out_bytes_held = 0;
    out_bytes_written = 0;

    cop->flush ();
    vis_runner_flush ();
}

static void apply_replay_gain (SafeLock &, Index<float> & data)
{
    if (! aud_get_bool ("enable_replay_gain"))
        return;

    float factor = powf (10, aud_get_double ("replay_gain_preamp") / 20);

    if (gain_info_valid)
    {
        float peak;

        auto mode = (ReplayGainMode) aud_get_int ("replay_gain_mode");
        if ((mode == ReplayGainMode::Album) ||
            (mode == ReplayGainMode::Automatic &&
             (! aud_get_bool ("shuffle") || aud_get_bool ("album_shuffle"))))
        {
            factor *= powf (10, gain_info.album_gain / 20);
            peak = gain_info.album_peak;
        }
        else
        {
            factor *= powf (10, gain_info.track_gain / 20);
            peak = gain_info.track_peak;
        }

        if (aud_get_bool ("enable_clipping_prevention") && peak * factor > 1)
            factor = 1 / peak;
    }
    else
        factor *= powf (10, aud_get_double ("default_gain") / 20);

    if (factor < 0.99 || factor > 1.01)
        audio_amplify (data.begin (), 1, data.len (), & factor);
}

static void write_secondary (SafeLock &, const Index<float> & data)
{
    assert (state.secondary ());

    auto begin = (const char *) data.begin ();
    auto end = (const char *) data.end ();

    while (begin < end)
        begin += sop->write_audio (begin, end - begin);
}

static void write_output (UnsafeLock & lock, Index<float> & data)
{
    assert (state.output ());

    if (! data.len ())
        return;

    if (state.secondary () && record_stream == OutputStream::AfterEffects)
        write_secondary (lock, data);

    int out_time = aud::rescale<int64_t> (out_bytes_written, out_bytes_per_sec, 1000);
    vis_runner_pass_audio (out_time, data, out_channels, out_rate);

    eq_filter (data.begin (), data.len ());

    if (state.secondary () && record_stream == OutputStream::AfterEqualizer)
        write_secondary (lock, data);

    if (aud_get_bool ("software_volume_control"))
    {
        StereoVolume v = {aud_get_int ("sw_volume_left"), aud_get_int ("sw_volume_right")};
        audio_amplify (data.begin (), out_channels, data.len () / out_channels, v);
    }

    if (aud_get_bool ("soft_clipping"))
        audio_soft_clip (data.begin (), data.len ());

    const void * out_data = data.begin ();

    if (out_format != FMT_FLOAT)
    {
        buffer2.resize (FMT_SIZEOF (out_format) * data.len ());
        audio_to_int (data.begin (), buffer2.begin (), out_format, data.len ());
        out_data = buffer2.begin ();
    }

    out_bytes_held = FMT_SIZEOF (out_format) * data.len ();

    while (out_bytes_held && ! state.resetting ())
    {
        if (state.paused ())
        {
            // avoid locking up if the input thread reaches close_audio() while
            // paused (unlikely but possible with perfect timing)
            if (! state.input ())
                break;

            state.await_change (lock);
            continue;
        }

        int written = cop->write_audio (out_data, out_bytes_held);

        out_data = (const char *) out_data + written;
        out_bytes_held -= written;
        out_bytes_written += written;

        if (! out_bytes_held)
            break;

        lock.minor.unlock ();
        cop->period_wait ();
        lock.minor.lock ();
    }
}

static bool process_audio (UnsafeLock & lock, const void * data, int size, int stop_time)
{
    assert (state.input () && state.output ());

    int samples = size / FMT_SIZEOF (in_format);
    bool stopped = false;

    if (stop_time != -1)
    {
        int64_t frames_left = aud::rescale<int64_t> (stop_time - seek_time, 1000, in_rate) - in_frames;
        int64_t samples_left = in_channels * aud::max ((int64_t) 0, frames_left);

        if (samples >= samples_left)
        {
            samples = samples_left;
            stopped = true;
        }
    }

    in_frames += samples / in_channels;

    buffer1.resize (samples);

    if (in_format == FMT_FLOAT)
        memcpy (buffer1.begin (), data, sizeof (float) * samples);
    else
        audio_from_int (data, in_format, buffer1.begin (), samples);

    if (state.secondary () && record_stream == OutputStream::AsDecoded)
        write_secondary (lock, buffer1);

    apply_replay_gain (lock, buffer1);

    if (state.secondary () && record_stream == OutputStream::AfterReplayGain)
        write_secondary (lock, buffer1);

    write_output (lock, effect_process (buffer1));

    return ! stopped;
}

static void finish_effects (UnsafeLock & lock, bool end_of_playlist)
{
    assert (state.output ());

    buffer1.resize (0);
    write_output (lock, effect_finish (buffer1, end_of_playlist));
}

bool output_open_audio (const String & filename, const Tuple & tuple,
 int format, int rate, int channels, int start_time, bool pause)
{
    /* prevent division by zero */
    if (rate < 1 || channels < 1 || channels > AUD_MAX_CHANNELS)
        return false;

    auto lock = state.lock_unsafe ();

    state.set_input (lock, true);
    state.set_flushed (lock, false);

    seek_time = start_time;
    gain_info_valid = false;

    in_filename = filename;
    in_tuple = tuple.ref ();
    in_format = format;
    in_channels = channels;
    in_rate = rate;
    in_frames = 0;

    setup_effects (lock);
    setup_output (lock, true, pause);

    if (aud_get_bool ("record"))
        setup_secondary (lock, true);

    return true;
}

void output_set_tuple (const Tuple & tuple)
{
    auto lock = state.lock_safe ();

    if (state.input ())
        in_tuple = tuple.ref ();
}

void output_set_replay_gain (const ReplayGainInfo & info)
{
    auto lock = state.lock_safe ();

    if (state.input ())
    {
        gain_info = info;
        gain_info_valid = true;

        AUDINFO ("Replay Gain info:\n");
        AUDINFO (" album gain: %f dB\n", info.album_gain);
        AUDINFO (" album peak: %f\n", info.album_peak);
        AUDINFO (" track gain: %f dB\n", info.track_gain);
        AUDINFO (" track peak: %f\n", info.track_peak);
    }
}

/* returns false if stop_time is reached */
bool output_write_audio (const void * data, int size, int stop_time)
{
    while (1)
    {
        auto lock = state.lock_unsafe ();
        if (! state.input () || state.flushed ())
            return false;

        if (state.output () && ! state.resetting ())
            return process_audio (lock, data, size, stop_time);

        lock.major.unlock ();
        state.await_change (lock);
    }
}

void output_flush (int time, bool force)
{
    auto lock = state.lock_safe ();

    if (state.input () || state.output ())
    {
        // allow effect plugins to prevent the flush, but
        // always flush if paused to prevent locking up
        bool flush = effect_flush (state.paused () || force);
        if (flush && state.output ())
            flush_output (lock);
    }

    if (state.input ())
    {
        state.set_flushed (lock, true);
        seek_time = time;
        in_frames = 0;
    }
}

void output_resume ()
{
    auto lock = state.lock_safe ();

    if (state.input ())
        state.set_flushed (lock, false);
}

void output_pause (bool pause)
{
    auto lock = state.lock_safe ();

    if (state.input ())
        apply_pause (lock, pause);
}

int output_get_time ()
{
    auto lock = state.lock_safe ();
    int time = 0, delay = 0;

    if (state.input ())
    {
        if (state.output ())
        {
            delay = cop->get_delay ();
            delay += aud::rescale<int64_t> (out_bytes_held, out_bytes_per_sec, 1000);
        }

        delay = effect_adjust_delay (delay);
        time = aud::rescale<int64_t> (in_frames, in_rate, 1000);
        time = seek_time + aud::max (time - delay, 0);
    }

    return time;
}

int output_get_raw_time ()
{
    auto lock = state.lock_safe ();
    int time = 0;

    if (state.output ())
    {
        time = aud::rescale<int64_t> (out_bytes_written, out_bytes_per_sec, 1000);
        time = aud::max (time - cop->get_delay (), 0);
    }

    return time;
}

void output_close_audio ()
{
    auto lock = state.lock_unsafe ();

    if (state.input ())
    {
        state.set_input (lock, false);
        in_filename = String ();
        in_tuple = Tuple ();

        if (state.output ())
            finish_effects (lock, false); /* first time for end of song */
    }
}

void output_drain ()
{
    auto lock = state.lock_unsafe ();

    if (! state.input ())
    {
        if (state.output ())
            finish_effects (lock, true); /* second time for end of playlist */

        cleanup_output (lock);
        cleanup_secondary (lock);
    }
}

static void output_reset (OutputReset type, OutputPlugin * op)
{
    auto lock1 = state.lock_safe ();

    state.set_resetting (lock1, true);

    if (state.output ())
        flush_output (lock1);

    lock1.minor.unlock ();
    auto lock2 = state.lock_unsafe ();

    if (type != OutputReset::EffectsOnly)
        cleanup_output (lock2);

    /* this does not reset the secondary plugin */
    if (type == OutputReset::ResetPlugin)
    {
        if (cop)
            cop->cleanup ();

        if (op)
        {
            /* secondary plugin may become primary */
            if (op == sop)
            {
                cleanup_secondary (lock2);
                sop = nullptr;
            }
            else if (! op->init ())
                op = nullptr;
        }

        cop = op;
    }

    if (state.input ())
    {
        if (type == OutputReset::EffectsOnly)
            setup_effects (lock2);

        setup_output (lock2, false, state.paused ());

        if (aud_get_bool ("record"))
            setup_secondary (lock2, false);
    }

    state.set_resetting (lock2, false);
}

EXPORT void aud_output_reset (OutputReset type)
{
    output_reset (type, cop);
}

EXPORT StereoVolume aud_drct_get_volume ()
{
    auto lock = state.lock_safe ();
    StereoVolume volume = {0, 0};

    if (aud_get_bool ("software_volume_control"))
        volume = {aud_get_int ("sw_volume_left"), aud_get_int ("sw_volume_right")};
    else if (cop)
        volume = cop->get_volume ();

    return volume;
}

EXPORT void aud_drct_set_volume (StereoVolume volume)
{
    auto lock = state.lock_safe ();

    volume.left = aud::clamp (volume.left, 0, 100);
    volume.right = aud::clamp (volume.right, 0, 100);

    if (aud_get_bool ("software_volume_control"))
    {
        aud_set_int ("sw_volume_left", volume.left);
        aud_set_int ("sw_volume_right", volume.right);
    }
    else if (cop)
        cop->set_volume (volume);
}

PluginHandle * output_plugin_get_current ()
{
    return cop ? aud_plugin_by_header (cop) : nullptr;
}

PluginHandle * output_plugin_get_secondary ()
{
    return sop ? aud_plugin_by_header (sop) : nullptr;
}

bool output_plugin_set_current (PluginHandle * plugin)
{
    output_reset (OutputReset::ResetPlugin, plugin ?
     (OutputPlugin *) aud_plugin_get_header (plugin) : nullptr);
    return (! plugin || cop);
}

bool output_plugin_set_secondary (PluginHandle * plugin)
{
    auto lock = state.lock_safe ();

    cleanup_secondary (lock);
    if (sop)
        sop->cleanup ();

    sop = plugin ? (OutputPlugin *) aud_plugin_get_header (plugin) : nullptr;
    if (sop && ! sop->init ())
        sop = nullptr;

    if (state.input () && aud_get_bool ("record"))
        setup_secondary (lock, false);

    return (! plugin || sop);
}

static void record_settings_changed (void *, void *)
{
    auto lock = state.lock_safe ();

    if (state.input () && aud_get_bool ("record"))
        setup_secondary (lock, false);
    else
        cleanup_secondary (lock);
}

void output_init ()
{
    hook_associate ("set record", record_settings_changed, nullptr);
    hook_associate ("set record_stream", record_settings_changed, nullptr);
}

void output_cleanup ()
{
    hook_dissociate ("set record", record_settings_changed);
    hook_dissociate ("set record_stream", record_settings_changed);
}
