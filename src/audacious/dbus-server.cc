/*
 * dbus-server.c
 * Copyright 2013-2020 John Lindgren
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

#include <assert.h>
#include <functional>
#include <future>

#include <libaudcore/audstrings.h>
#include <libaudcore/drct.h>
#include <libaudcore/equalizer.h>
#include <libaudcore/hook.h>
#include <libaudcore/interface.h>
#include <libaudcore/mainloop.h>
#include <libaudcore/playlist.h>
#include <libaudcore/plugins.h>
#include <libaudcore/runtime.h>
#include <libaudcore/threads.h>
#include <libaudcore/tuple.h>

#include "aud-dbus.h"
#include "main.h"

typedef ObjAudacious Obj;
typedef GDBusMethodInvocation Invoc;

#define FINISH(name) obj_audacious_complete_##name(obj, invoc)
#define FINISH2(name, ...)                                                     \
    obj_audacious_complete_##name(obj, invoc, __VA_ARGS__)

class MainThreadRunner
{
public:
    // Blocks current thread while waiting for a function to execute in
    // the main (UI) thread. Returns false if canceled.
    bool run(std::function<void()> func)
    {
        auto mh = m_mutex.take();
        if (m_canceled)
            return false;

        assert(!m_running);
        m_running = true;

        m_queued_func.queue([this, func]() {
            auto mh = m_mutex.take();
            if (m_canceled)
                return;

            func();
            m_running = false;
            m_cond.notify_all();
        });

        while (m_running && !m_canceled)
            m_cond.wait(mh);

        return !m_running;
    }

    void cancel()
    {
        auto mh = m_mutex.take();
        m_queued_func.stop();
        m_canceled = true;
        m_cond.notify_all();
    }

    void reset()
    {
        m_running = false;
        m_canceled = false;
    }

private:
    aud::mutex m_mutex;
    aud::condvar m_cond;
    QueuedFunc m_queued_func;
    bool m_running = false;
    bool m_canceled = false;
};

static MainThreadRunner main_runner;

#define ENTER_MAIN_THREAD(...)                                                 \
    if (!main_runner.run([__VA_ARGS__]() {
#define LEAVE_MAIN_THREAD()                                                    \
    })) return false;

static bool prefer_playing = true;

static Playlist current_playlist()
{
    Playlist list;

    if (prefer_playing)
        list = Playlist::playing_playlist();
    if (list == Playlist())
        list = Playlist::active_playlist();

    return list;
}

#define CURRENT current_playlist()

static Index<PlaylistAddItem> strv_to_index(const char * const * strv)
{
    Index<PlaylistAddItem> index;
    while (*strv)
        index.append(String(*strv++));

    return index;
}

static gboolean do_add(Obj * obj, Invoc * invoc, const char * file)
{
    ENTER_MAIN_THREAD(file)
    CURRENT.insert_entry(-1, file, Tuple(), false);
    LEAVE_MAIN_THREAD()
    FINISH(add);
    return true;
}

static gboolean do_add_list(Obj * obj, Invoc * invoc,
                            const char * const * filenames)
{
    ENTER_MAIN_THREAD(filenames)
    CURRENT.insert_items(-1, strv_to_index(filenames), false);
    LEAVE_MAIN_THREAD()
    FINISH(add_list);
    return true;
}

static gboolean do_add_url(Obj * obj, Invoc * invoc, const char * url)
{
    ENTER_MAIN_THREAD(url)
    CURRENT.insert_entry(-1, url, Tuple(), false);
    LEAVE_MAIN_THREAD()
    FINISH(add_url);
    return true;
}

static gboolean do_advance(Obj * obj, Invoc * invoc)
{
    ENTER_MAIN_THREAD()
    CURRENT.next_song(aud_get_bool("repeat"));
    LEAVE_MAIN_THREAD()
    FINISH(advance);
    return true;
}

static gboolean do_advance_album(Obj * obj, Invoc * invoc)
{
    ENTER_MAIN_THREAD()
    CURRENT.next_album(aud_get_bool("repeat"));
    LEAVE_MAIN_THREAD()
    FINISH(advance_album);
    return true;
}

static gboolean do_auto_advance(Obj * obj, Invoc * invoc)
{
    /* thread-safe */
    FINISH2(auto_advance, !aud_get_bool("no_playlist_advance"));
    return true;
}

static gboolean do_balance(Obj * obj, Invoc * invoc)
{
    /* thread-safe */
    FINISH2(balance, aud_drct_get_volume_balance());
    return true;
}

static gboolean do_clear(Obj * obj, Invoc * invoc)
{
    ENTER_MAIN_THREAD()
    CURRENT.remove_all_entries();
    LEAVE_MAIN_THREAD()
    FINISH(clear);
    return true;
}

static gboolean do_config_get(Obj * obj, Invoc * invoc, const char * section,
                              const char * name)
{
    /* thread-safe */
    String value = aud_get_str(section[0] ? section : nullptr, name);
    FINISH2(config_get, value);
    return true;
}

static gboolean do_config_set(Obj * obj, Invoc * invoc, const char * section,
                              const char * name, const char * value)
{
    /* thread-safe */
    aud_set_str(section[0] ? section : nullptr, name, value);
    FINISH(config_set);
    return true;
}

static gboolean do_delete(Obj * obj, Invoc * invoc, unsigned pos)
{
    ENTER_MAIN_THREAD(pos)
    CURRENT.remove_entry(pos);
    LEAVE_MAIN_THREAD()
    FINISH(delete);
    return true;
}

static gboolean do_delete_active_playlist(Obj * obj, Invoc * invoc)
{
    ENTER_MAIN_THREAD()
    CURRENT.remove_playlist();
    LEAVE_MAIN_THREAD()
    FINISH(delete_active_playlist);
    return true;
}

static gboolean do_eject(Obj * obj, Invoc * invoc)
{
    ENTER_MAIN_THREAD()
    if (!aud_get_headless_mode())
        aud_ui_show_filebrowser(true);

    LEAVE_MAIN_THREAD()
    FINISH(eject);
    return true;
}

static gboolean do_equalizer_activate(Obj * obj, Invoc * invoc, gboolean active)
{
    /* thread-safe */
    aud_set_bool("equalizer_active", active);
    FINISH(equalizer_activate);
    return true;
}

static gboolean do_get_active_playlist(Obj * obj, Invoc * invoc)
{
    int active_idx;
    ENTER_MAIN_THREAD(&active_idx)
    active_idx = CURRENT.index();
    LEAVE_MAIN_THREAD()
    FINISH2(get_active_playlist, active_idx);
    return true;
}

static gboolean do_get_active_playlist_name(Obj * obj, Invoc * invoc)
{
    String title;
    ENTER_MAIN_THREAD(&title)
    title = CURRENT.get_title();
    LEAVE_MAIN_THREAD()
    FINISH2(get_active_playlist_name, title ? title : "");
    return true;
}

static gboolean do_get_eq(Obj * obj, Invoc * invoc)
{
    /* thread-safe */
    double preamp = aud_get_double("equalizer_preamp");
    double bands[AUD_EQ_NBANDS];
    aud_eq_get_bands(bands);

    GVariant * var = g_variant_new_fixed_array(G_VARIANT_TYPE_DOUBLE, bands,
                                               AUD_EQ_NBANDS, sizeof(double));
    FINISH2(get_eq, preamp, var);
    return true;
}

static gboolean do_get_eq_band(Obj * obj, Invoc * invoc, int band)
{
    /* thread-safe */
    FINISH2(get_eq_band, aud_eq_get_band(band));
    return true;
}

static gboolean do_get_eq_preamp(Obj * obj, Invoc * invoc)
{
    /* thread-safe */
    FINISH2(get_eq_preamp, aud_get_double("equalizer_preamp"));
    return true;
}

static gboolean do_get_info(Obj * obj, Invoc * invoc)
{
    /* thread-safe */
    int bitrate, samplerate, channels;
    aud_drct_get_info(bitrate, samplerate, channels);
    FINISH2(get_info, bitrate, samplerate, channels);
    return true;
}

static gboolean do_get_playqueue_length(Obj * obj, Invoc * invoc)
{
    int n_queued;
    ENTER_MAIN_THREAD(&n_queued)
    n_queued = CURRENT.n_queued();
    LEAVE_MAIN_THREAD()
    FINISH2(get_playqueue_length, n_queued);
    return true;
}

static gboolean do_get_tuple_fields(Obj * obj, Invoc * invoc)
{
    /* thread-safe */
    const char * fields[Tuple::n_fields + 1];
    for (auto f : Tuple::all_fields())
        fields[f] = Tuple::field_get_name(f);

    fields[Tuple::n_fields] = nullptr;

    FINISH2(get_tuple_fields, fields);
    return true;
}

static gboolean do_info(Obj * obj, Invoc * invoc)
{
    /* thread-safe */
    int bitrate, samplerate, channels;
    aud_drct_get_info(bitrate, samplerate, channels);
    FINISH2(info, bitrate, samplerate, channels);
    return true;
}

static gboolean do_jump(Obj * obj, Invoc * invoc, unsigned pos)
{
    ENTER_MAIN_THREAD(pos)
    CURRENT.set_position(pos);
    LEAVE_MAIN_THREAD()
    FINISH(jump);
    return true;
}

static gboolean do_length(Obj * obj, Invoc * invoc)
{
    int n_entries;
    ENTER_MAIN_THREAD(&n_entries)
    n_entries = CURRENT.n_entries();
    LEAVE_MAIN_THREAD()
    FINISH2(length, n_entries);
    return true;
}

static gboolean do_main_win_visible(Obj * obj, Invoc * invoc)
{
    bool visible;
    ENTER_MAIN_THREAD(&visible)
    visible = !aud_get_headless_mode() && aud_ui_is_shown();
    LEAVE_MAIN_THREAD()
    FINISH2(main_win_visible, visible);
    return true;
}

static gboolean do_new_playlist(Obj * obj, Invoc * invoc)
{
    ENTER_MAIN_THREAD()
    Playlist::insert_playlist(CURRENT.index() + 1).activate();
    aud_drct_stop();
    LEAVE_MAIN_THREAD()
    FINISH(new_playlist);
    return true;
}

static gboolean do_number_of_playlists(Obj * obj, Invoc * invoc)
{
    /* thread-safe */
    FINISH2(number_of_playlists, Playlist::n_playlists());
    return true;
}

static gboolean do_open_list(Obj * obj, Invoc * invoc,
                             const char * const * filenames)
{
    ENTER_MAIN_THREAD(filenames)
    aud_drct_pl_open_list(strv_to_index(filenames));
    LEAVE_MAIN_THREAD()
    FINISH(open_list);
    return true;
}

static gboolean do_open_list_to_temp(Obj * obj, Invoc * invoc,
                                     const char * const * filenames)
{
    ENTER_MAIN_THREAD(filenames)
    aud_drct_pl_open_temp_list(strv_to_index(filenames));
    LEAVE_MAIN_THREAD()
    FINISH(open_list_to_temp);
    return true;
}

static gboolean do_pause(Obj * obj, Invoc * invoc)
{
    ENTER_MAIN_THREAD()
    aud_drct_pause();
    LEAVE_MAIN_THREAD()
    FINISH(pause);
    return true;
}

static gboolean do_paused(Obj * obj, Invoc * invoc)
{
    /* thread-safe */
    FINISH2(paused, aud_drct_get_paused());
    return true;
}

static gboolean do_play(Obj * obj, Invoc * invoc)
{
    ENTER_MAIN_THREAD()
    aud_drct_play();
    LEAVE_MAIN_THREAD()
    FINISH(play);
    return true;
}

static gboolean do_play_active_playlist(Obj * obj, Invoc * invoc)
{
    ENTER_MAIN_THREAD()
    CURRENT.start_playback();
    LEAVE_MAIN_THREAD()
    FINISH(play_active_playlist);
    return true;
}

static gboolean do_play_pause(Obj * obj, Invoc * invoc)
{
    ENTER_MAIN_THREAD()
    aud_drct_play_pause();
    LEAVE_MAIN_THREAD()
    FINISH(play_pause);
    return true;
}

static gboolean do_playing(Obj * obj, Invoc * invoc)
{
    /* thread-safe */
    FINISH2(playing, aud_drct_get_playing());
    return true;
}

static gboolean do_playlist_add(Obj * obj, Invoc * invoc, const char * list)
{
    ENTER_MAIN_THREAD(list)
    CURRENT.insert_entry(-1, list, Tuple(), false);
    LEAVE_MAIN_THREAD()
    FINISH(playlist_add);
    return true;
}

static gboolean do_playlist_enqueue_to_temp(Obj * obj, Invoc * invoc,
                                            const char * url)
{
    ENTER_MAIN_THREAD(url)
    aud_drct_pl_open_temp(url);
    LEAVE_MAIN_THREAD()
    FINISH(playlist_enqueue_to_temp);
    return true;
}

static gboolean do_playlist_ins_url_string(Obj * obj, Invoc * invoc,
                                           const char * url, int pos)
{
    ENTER_MAIN_THREAD(url, pos)
    CURRENT.insert_entry(pos, url, Tuple(), false);
    LEAVE_MAIN_THREAD()
    FINISH(playlist_ins_url_string);
    return true;
}

static gboolean do_playqueue_add(Obj * obj, Invoc * invoc, int pos)
{
    ENTER_MAIN_THREAD(pos)
    CURRENT.queue_insert(-1, pos);
    LEAVE_MAIN_THREAD()
    FINISH(playqueue_add);
    return true;
}

static gboolean do_playqueue_clear(Obj * obj, Invoc * invoc)
{
    ENTER_MAIN_THREAD()
    CURRENT.queue_remove_all();
    LEAVE_MAIN_THREAD()
    FINISH(playqueue_clear);
    return true;
}

static gboolean do_playqueue_is_queued(Obj * obj, Invoc * invoc, int pos)
{
    bool queued;
    ENTER_MAIN_THREAD(pos, &queued)
    queued = (CURRENT.queue_find_entry(pos) >= 0);
    LEAVE_MAIN_THREAD()
    FINISH2(playqueue_is_queued, queued);
    return true;
}

static gboolean do_playqueue_remove(Obj * obj, Invoc * invoc, int pos)
{
    ENTER_MAIN_THREAD(pos)
    auto playlist = CURRENT;
    int qpos = playlist.queue_find_entry(pos);

    if (qpos >= 0)
        playlist.queue_remove(qpos);

    LEAVE_MAIN_THREAD()
    FINISH(playqueue_remove);
    return true;
}

static gboolean do_plugin_enable(Obj * obj, Invoc * invoc, const char * name,
                                 gboolean enable)
{
    /* this part thread-safe */
    PluginHandle * plugin = aud_plugin_lookup_basename(name);
    if (!plugin)
    {
        AUDERR("No such plugin: %s\n", name);
        return false;
    }

    ENTER_MAIN_THREAD(plugin, enable)
    aud_plugin_enable(plugin, enable);
    LEAVE_MAIN_THREAD()
    FINISH(plugin_enable);
    return true;
}

static gboolean do_plugin_is_enabled(Obj * obj, Invoc * invoc,
                                     const char * name)
{
    /* thread-safe */
    PluginHandle * plugin = aud_plugin_lookup_basename(name);
    if (!plugin)
    {
        AUDERR("No such plugin: %s\n", name);
        return false;
    }

    FINISH2(plugin_is_enabled, aud_plugin_get_enabled(plugin));
    return true;
}

static gboolean do_position(Obj * obj, Invoc * invoc)
{
    int pos;
    ENTER_MAIN_THREAD(&pos)
    pos = CURRENT.get_position();
    LEAVE_MAIN_THREAD()
    FINISH2(position, pos);
    return true;
}

static gboolean do_queue_get_list_pos(Obj * obj, Invoc * invoc, unsigned qpos)
{
    int pos;
    ENTER_MAIN_THREAD(qpos, &pos)
    pos = CURRENT.queue_get_entry(qpos);
    LEAVE_MAIN_THREAD()
    FINISH2(queue_get_list_pos, pos);
    return true;
}

static gboolean do_queue_get_queue_pos(Obj * obj, Invoc * invoc, unsigned pos)
{
    int qpos;
    ENTER_MAIN_THREAD(pos, &qpos)
    qpos = CURRENT.queue_find_entry(pos);
    LEAVE_MAIN_THREAD()
    FINISH2(queue_get_queue_pos, qpos);
    return true;
}

static gboolean do_quit(Obj * obj, Invoc * invoc)
{
    /* thread-safe */
    event_queue("quit", nullptr);
    FINISH(quit);
    return true;
}

static gboolean do_record(Obj * obj, Invoc * invoc)
{
    ENTER_MAIN_THREAD()
    if (aud_drct_get_record_enabled())
        aud_set_bool("record", !aud_get_bool("record"));

    LEAVE_MAIN_THREAD()
    FINISH(record);
    return true;
}

static gboolean do_recording(Obj * obj, Invoc * invoc)
{
    bool recording = false;
    ENTER_MAIN_THREAD(&recording)
    if (aud_drct_get_record_enabled())
        recording = aud_get_bool("record");

    LEAVE_MAIN_THREAD()
    FINISH2(recording, recording);
    return true;
}

static gboolean do_repeat(Obj * obj, Invoc * invoc)
{
    /* thread-safe */
    FINISH2(repeat, aud_get_bool("repeat"));
    return true;
}

static gboolean do_reverse(Obj * obj, Invoc * invoc)
{
    ENTER_MAIN_THREAD()
    CURRENT.prev_song();
    LEAVE_MAIN_THREAD()
    FINISH(reverse);
    return true;
}

static gboolean do_reverse_album(Obj * obj, Invoc * invoc)
{
    ENTER_MAIN_THREAD()
    CURRENT.prev_album();
    LEAVE_MAIN_THREAD()
    FINISH(reverse_album);
    return true;
}

static gboolean do_seek(Obj * obj, Invoc * invoc, unsigned pos)
{
    ENTER_MAIN_THREAD(pos)
    aud_drct_seek(pos);
    LEAVE_MAIN_THREAD()
    FINISH(seek);
    return true;
}

static gboolean do_select_displayed_playlist(Obj * obj, Invoc * invoc)
{
    ENTER_MAIN_THREAD()
    prefer_playing = false;
    LEAVE_MAIN_THREAD()
    FINISH(select_displayed_playlist);
    return true;
}

static gboolean do_select_playing_playlist(Obj * obj, Invoc * invoc)
{
    ENTER_MAIN_THREAD()
    prefer_playing = true;
    LEAVE_MAIN_THREAD()
    FINISH(select_playing_playlist);
    return true;
}

static gboolean do_set_active_playlist(Obj * obj, Invoc * invoc, int index)
{
    ENTER_MAIN_THREAD(index)
    auto playlist = Playlist::by_index(index);

    playlist.activate();

    if (prefer_playing && aud_drct_get_playing())
        playlist.start_playback(aud_drct_get_paused());

    LEAVE_MAIN_THREAD()
    FINISH(set_active_playlist);
    return true;
}

static gboolean do_set_active_playlist_name(Obj * obj, Invoc * invoc,
                                            const char * title)
{
    ENTER_MAIN_THREAD(title)
    CURRENT.set_title(title);
    LEAVE_MAIN_THREAD()
    FINISH(set_active_playlist_name);
    return true;
}

static gboolean do_set_eq(Obj * obj, Invoc * invoc, double preamp,
                          GVariant * var)
{
    /* thread-safe */
    if (!g_variant_is_of_type(var, G_VARIANT_TYPE("ad")))
        return false;

    size_t nbands = 0;
    const double * bands =
        (double *)g_variant_get_fixed_array(var, &nbands, sizeof(double));

    if (nbands != AUD_EQ_NBANDS)
        return false;

    aud_set_double("equalizer_preamp", preamp);
    aud_eq_set_bands(bands);
    FINISH(set_eq);
    return true;
}

static gboolean do_set_eq_band(Obj * obj, Invoc * invoc, int band, double value)
{
    /* thread-safe (mostly; see comment in equalizer.cc) */
    aud_eq_set_band(band, value);
    FINISH(set_eq_band);
    return true;
}

static gboolean do_set_eq_preamp(Obj * obj, Invoc * invoc, double preamp)
{
    /* thread-safe */
    aud_set_double("equalizer_preamp", preamp);
    FINISH(set_eq_preamp);
    return true;
}

static gboolean do_set_volume(Obj * obj, Invoc * invoc, int vl, int vr)
{
    /* thread-safe */
    aud_drct_set_volume({vl, vr});
    FINISH(set_volume);
    return true;
}

static gboolean do_show_about_box(Obj * obj, Invoc * invoc, gboolean show)
{
    if (aud_get_headless_mode())
        return false;

    ENTER_MAIN_THREAD(show)

    if (show)
        aud_ui_show_about_window();
    else
        aud_ui_hide_about_window();

    LEAVE_MAIN_THREAD()
    FINISH(show_about_box);
    return true;
}

static gboolean do_show_filebrowser(Obj * obj, Invoc * invoc, gboolean show)
{
    if (aud_get_headless_mode())
        return false;

    ENTER_MAIN_THREAD(show)

    if (show)
        aud_ui_show_filebrowser(false);
    else
        aud_ui_hide_filebrowser();

    LEAVE_MAIN_THREAD()
    FINISH(show_filebrowser);
    return true;
}

static gboolean do_show_jtf_box(Obj * obj, Invoc * invoc, gboolean show)
{
    if (aud_get_headless_mode())
        return false;

    ENTER_MAIN_THREAD(show)

    if (show)
        aud_ui_show_jump_to_song();
    else
        aud_ui_hide_jump_to_song();

    LEAVE_MAIN_THREAD()
    FINISH(show_jtf_box);
    return true;
}

static gboolean do_show_main_win(Obj * obj, Invoc * invoc, gboolean show)
{
    if (aud_get_headless_mode())
        return false;

    ENTER_MAIN_THREAD(show)
    aud_ui_show(show);
    LEAVE_MAIN_THREAD()
    FINISH(show_main_win);
    return true;
}

static gboolean do_show_prefs_box(Obj * obj, Invoc * invoc, gboolean show)
{
    if (aud_get_headless_mode())
        return false;

    ENTER_MAIN_THREAD(show)

    if (show)
        aud_ui_show_prefs_window();
    else
        aud_ui_hide_prefs_window();

    LEAVE_MAIN_THREAD()
    FINISH(show_prefs_box);
    return true;
}

static gboolean do_shuffle(Obj * obj, Invoc * invoc)
{
    /* thread-safe */
    FINISH2(shuffle, aud_get_bool("shuffle"));
    return true;
}

static gboolean do_song_filename(Obj * obj, Invoc * invoc, unsigned pos)
{
    String filename;
    ENTER_MAIN_THREAD(pos, &filename);
    filename = CURRENT.entry_filename(pos);
    LEAVE_MAIN_THREAD()
    FINISH2(song_filename, filename ? filename : "");
    return true;
}

static gboolean do_song_frames(Obj * obj, Invoc * invoc, unsigned pos)
{
    Tuple tuple;
    ENTER_MAIN_THREAD(pos, &tuple);
    tuple = CURRENT.entry_tuple(pos);
    LEAVE_MAIN_THREAD()
    FINISH2(song_frames, aud::max(0, tuple.get_int(Tuple::Length)));
    return true;
}

static gboolean do_song_length(Obj * obj, Invoc * invoc, unsigned pos)
{
    Tuple tuple;
    ENTER_MAIN_THREAD(pos, &tuple)
    tuple = CURRENT.entry_tuple(pos);
    LEAVE_MAIN_THREAD()
    int length = aud::max(0, tuple.get_int(Tuple::Length));
    FINISH2(song_length, length / 1000);
    return true;
}

static gboolean do_song_title(Obj * obj, Invoc * invoc, unsigned pos)
{
    Tuple tuple;
    ENTER_MAIN_THREAD(pos, &tuple)
    tuple = CURRENT.entry_tuple(pos);
    LEAVE_MAIN_THREAD()
    String title = tuple.get_str(Tuple::FormattedTitle);
    FINISH2(song_title, title ? title : "");
    return true;
}

static gboolean do_song_tuple(Obj * obj, Invoc * invoc, unsigned pos,
                              const char * key)
{
    Tuple::Field field = Tuple::field_by_name(key);
    GVariant * var = nullptr;

    if (field >= 0)
    {
        Tuple tuple;
        ENTER_MAIN_THREAD(pos, &tuple)
        tuple = CURRENT.entry_tuple(pos);
        LEAVE_MAIN_THREAD()

        switch (tuple.get_value_type(field))
        {
        case Tuple::String:
            var = g_variant_new_string(tuple.get_str(field));
            break;

        case Tuple::Int:
            var = g_variant_new_int32(tuple.get_int(field));
            break;

        default:
            break;
        }
    }

    if (!var)
        var = g_variant_new_string("");

    FINISH2(song_tuple, g_variant_new_variant(var));
    return true;
}

static gboolean do_startup_notify(Obj * obj, Invoc * invoc, const char * id)
{
    ENTER_MAIN_THREAD(id)
    aud_ui_startup_notify(id);
    LEAVE_MAIN_THREAD()
    FINISH(startup_notify);
    return true;
}

static gboolean do_status(Obj * obj, Invoc * invoc)
{
    const char * status = "stopped";
    ENTER_MAIN_THREAD(&status)
    if (aud_drct_get_playing())
        status = aud_drct_get_paused() ? "paused" : "playing";

    LEAVE_MAIN_THREAD()
    FINISH2(status, status);
    return true;
}

static gboolean do_stop(Obj * obj, Invoc * invoc)
{
    ENTER_MAIN_THREAD()
    aud_drct_stop();
    LEAVE_MAIN_THREAD()
    FINISH(stop);
    return true;
}

static gboolean do_stop_after(Obj * obj, Invoc * invoc)
{
    /* thread-safe */
    FINISH2(stop_after, aud_get_bool("stop_after_current_song"));
    return true;
}

static gboolean do_stopped(Obj * obj, Invoc * invoc)
{
    /* thread-safe */
    FINISH2(stopped, !aud_drct_get_playing());
    return true;
}

static gboolean do_time(Obj * obj, Invoc * invoc)
{
    /* thread-safe */
    FINISH2(time, aud_drct_get_time());
    return true;
}

static gboolean do_toggle_auto_advance(Obj * obj, Invoc * invoc)
{
    /* thread-safe (*except for bug in aud_toggle_bool) */
    aud_toggle_bool("no_playlist_advance");
    FINISH(toggle_auto_advance);
    return true;
}

static gboolean do_toggle_repeat(Obj * obj, Invoc * invoc)
{
    /* thread-safe (*except for bug in aud_toggle_bool) */
    aud_toggle_bool("repeat");
    FINISH(toggle_repeat);
    return true;
}

static gboolean do_toggle_shuffle(Obj * obj, Invoc * invoc)
{
    /* thread-safe (*except for bug in aud_toggle_bool) */
    aud_toggle_bool("shuffle");
    FINISH(toggle_shuffle);
    return true;
}

static gboolean do_toggle_stop_after(Obj * obj, Invoc * invoc)
{
    /* thread-safe (*except for bug in aud_toggle_bool) */
    aud_toggle_bool("stop_after_current_song");
    FINISH(toggle_stop_after);
    return true;
}

static gboolean do_version(Obj * obj, Invoc * invoc)
{
    /* thread-safe */
    FINISH2(version, VERSION);
    return true;
}

static gboolean do_volume(Obj * obj, Invoc * invoc)
{
    /* thread-safe */
    StereoVolume volume = aud_drct_get_volume();
    FINISH2(volume, volume.left, volume.right);
    return true;
}

static const struct
{
    const char * signal;
    GCallback callback;
} handlers[] = {
    {"handle-add", (GCallback)do_add},
    {"handle-add-list", (GCallback)do_add_list},
    {"handle-add-url", (GCallback)do_add_url},
    {"handle-advance", (GCallback)do_advance},
    {"handle-advance-album", (GCallback)do_advance_album},
    {"handle-auto-advance", (GCallback)do_auto_advance},
    {"handle-balance", (GCallback)do_balance},
    {"handle-clear", (GCallback)do_clear},
    {"handle-config-get", (GCallback)do_config_get},
    {"handle-config-set", (GCallback)do_config_set},
    {"handle-delete", (GCallback)do_delete},
    {"handle-delete-active-playlist", (GCallback)do_delete_active_playlist},
    {"handle-eject", (GCallback)do_eject},
    {"handle-equalizer-activate", (GCallback)do_equalizer_activate},
    {"handle-get-active-playlist", (GCallback)do_get_active_playlist},
    {"handle-get-active-playlist-name", (GCallback)do_get_active_playlist_name},
    {"handle-get-eq", (GCallback)do_get_eq},
    {"handle-get-eq-band", (GCallback)do_get_eq_band},
    {"handle-get-eq-preamp", (GCallback)do_get_eq_preamp},
    {"handle-get-info", (GCallback)do_get_info},
    {"handle-get-playqueue-length", (GCallback)do_get_playqueue_length},
    {"handle-get-tuple-fields", (GCallback)do_get_tuple_fields},
    {"handle-info", (GCallback)do_info},
    {"handle-jump", (GCallback)do_jump},
    {"handle-length", (GCallback)do_length},
    {"handle-main-win-visible", (GCallback)do_main_win_visible},
    {"handle-new-playlist", (GCallback)do_new_playlist},
    {"handle-number-of-playlists", (GCallback)do_number_of_playlists},
    {"handle-open-list", (GCallback)do_open_list},
    {"handle-open-list-to-temp", (GCallback)do_open_list_to_temp},
    {"handle-pause", (GCallback)do_pause},
    {"handle-paused", (GCallback)do_paused},
    {"handle-play", (GCallback)do_play},
    {"handle-play-active-playlist", (GCallback)do_play_active_playlist},
    {"handle-play-pause", (GCallback)do_play_pause},
    {"handle-playing", (GCallback)do_playing},
    {"handle-playlist-add", (GCallback)do_playlist_add},
    {"handle-playlist-enqueue-to-temp", (GCallback)do_playlist_enqueue_to_temp},
    {"handle-playlist-ins-url-string", (GCallback)do_playlist_ins_url_string},
    {"handle-playqueue-add", (GCallback)do_playqueue_add},
    {"handle-playqueue-clear", (GCallback)do_playqueue_clear},
    {"handle-playqueue-is-queued", (GCallback)do_playqueue_is_queued},
    {"handle-playqueue-remove", (GCallback)do_playqueue_remove},
    {"handle-plugin-enable", (GCallback)do_plugin_enable},
    {"handle-plugin-is-enabled", (GCallback)do_plugin_is_enabled},
    {"handle-position", (GCallback)do_position},
    {"handle-queue-get-list-pos", (GCallback)do_queue_get_list_pos},
    {"handle-queue-get-queue-pos", (GCallback)do_queue_get_queue_pos},
    {"handle-quit", (GCallback)do_quit},
    {"handle-recording", (GCallback)do_recording},
    {"handle-record", (GCallback)do_record},
    {"handle-repeat", (GCallback)do_repeat},
    {"handle-reverse", (GCallback)do_reverse},
    {"handle-reverse-album", (GCallback)do_reverse_album},
    {"handle-seek", (GCallback)do_seek},
    {"handle-select-displayed-playlist",
     (GCallback)do_select_displayed_playlist},
    {"handle-select-playing-playlist", (GCallback)do_select_playing_playlist},
    {"handle-set-active-playlist", (GCallback)do_set_active_playlist},
    {"handle-set-active-playlist-name", (GCallback)do_set_active_playlist_name},
    {"handle-set-eq", (GCallback)do_set_eq},
    {"handle-set-eq-band", (GCallback)do_set_eq_band},
    {"handle-set-eq-preamp", (GCallback)do_set_eq_preamp},
    {"handle-set-volume", (GCallback)do_set_volume},
    {"handle-show-about-box", (GCallback)do_show_about_box},
    {"handle-show-filebrowser", (GCallback)do_show_filebrowser},
    {"handle-show-jtf-box", (GCallback)do_show_jtf_box},
    {"handle-show-main-win", (GCallback)do_show_main_win},
    {"handle-show-prefs-box", (GCallback)do_show_prefs_box},
    {"handle-shuffle", (GCallback)do_shuffle},
    {"handle-song-filename", (GCallback)do_song_filename},
    {"handle-song-frames", (GCallback)do_song_frames},
    {"handle-song-length", (GCallback)do_song_length},
    {"handle-song-title", (GCallback)do_song_title},
    {"handle-song-tuple", (GCallback)do_song_tuple},
    {"handle-startup-notify", (GCallback)do_startup_notify},
    {"handle-status", (GCallback)do_status},
    {"handle-stop", (GCallback)do_stop},
    {"handle-stop-after", (GCallback)do_stop_after},
    {"handle-stopped", (GCallback)do_stopped},
    {"handle-time", (GCallback)do_time},
    {"handle-toggle-auto-advance", (GCallback)do_toggle_auto_advance},
    {"handle-toggle-repeat", (GCallback)do_toggle_repeat},
    {"handle-toggle-shuffle", (GCallback)do_toggle_shuffle},
    {"handle-toggle-stop-after", (GCallback)do_toggle_stop_after},
    {"handle-version", (GCallback)do_version},
    {"handle-volume", (GCallback)do_volume}};

static GMainContext * dbus_context = nullptr;
static GMainLoop * dbus_mainloop = nullptr;
static std::thread dbus_thread;

static std::promise<bool> init_promise;
static bool init_promise_set;

static unsigned owner_id = 0;
static GDBusInterfaceSkeleton * skeleton = nullptr;

static void bus_acquired(GDBusConnection * bus, const char *, void *)
{
    skeleton = (GDBusInterfaceSkeleton *)obj_audacious_skeleton_new();

    for (auto & handler : handlers)
        g_signal_connect(skeleton, handler.signal, handler.callback, nullptr);

    GError * error = nullptr;
    if (!g_dbus_interface_skeleton_export(skeleton, bus,
                                          "/org/atheme/audacious", &error))
    {
        if (error)
        {
            AUDERR("D-Bus error: %s\n", error->message);
            g_error_free(error);
        }

        g_main_loop_quit(dbus_mainloop);
    }
}

static void name_acquired(GDBusConnection *, const char * name, void *)
{
    AUDINFO("Owned D-Bus name (%s) on session bus.\n", name);

    if (!init_promise_set)
    {
        init_promise.set_value(true);
        init_promise_set = true;
    }
}

static void name_lost(GDBusConnection *, const char * name, void *)
{
    AUDINFO("Owning D-Bus name (%s) failed, already taken?\n", name);

    g_main_loop_quit(dbus_mainloop);
}

static void dbus_server_run()
{
    g_main_context_push_thread_default(dbus_context);

    owner_id = g_bus_own_name(G_BUS_TYPE_SESSION, dbus_server_name(),
                              (GBusNameOwnerFlags)0, bus_acquired,
                              name_acquired, name_lost, nullptr, nullptr);

    g_main_loop_run(dbus_mainloop);

    if (!init_promise_set)
    {
        init_promise.set_value(false);
        init_promise_set = true;
    }

    g_main_context_pop_thread_default(dbus_context);
}

StringBuf dbus_server_name()
{
    int instance = aud_get_instance();
    return (instance == 1) ? str_copy("org.atheme.audacious")
                           : str_printf("org.atheme.audacious-%d", instance);
}

bool dbus_server_init()
{
    assert(!dbus_thread.joinable());

    init_promise = std::promise<bool>();
    init_promise_set = false;

    dbus_context = g_main_context_new();
    dbus_mainloop = g_main_loop_new(dbus_context, false);
    dbus_thread = std::thread(dbus_server_run);

    /* wait for thread to finish init */
    bool success = init_promise.get_future().get();
    if (!success)
        dbus_server_cleanup();

    return success;
}

void dbus_server_cleanup()
{
    if (!dbus_thread.joinable())
        return;

    g_main_loop_quit(dbus_mainloop);
    main_runner.cancel();
    dbus_thread.join();
    main_runner.reset();

    if (owner_id)
    {
        g_bus_unown_name(owner_id);
        owner_id = 0;
    }

    if (skeleton)
    {
        g_object_unref(skeleton);
        skeleton = nullptr;
    }

    g_main_loop_unref(dbus_mainloop);
    dbus_mainloop = nullptr;
    g_main_context_unref(dbus_context);
    dbus_context = nullptr;
}
