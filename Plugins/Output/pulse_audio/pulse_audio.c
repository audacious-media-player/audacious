/***
  This file is part of xmms-pulse.
 
  xmms-pulse is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
 
  xmms-pulse is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
  General Public License for more details.
 
  You should have received a copy of the GNU General Public License
  along with xmms-pulse; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
  USA.
***/

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <assert.h>
/* #include <pthread.h> */
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>

#include <gtk/gtk.h>
#include "audacious/plugin.h"
#include "libaudacious/beepctrl.h"
#include "libaudacious/util.h"

#include <pulse/pulseaudio.h>

static pa_context *context = NULL;
static pa_stream *stream = NULL;
static pa_threaded_mainloop *mainloop = NULL;

static pa_cvolume volume;
static int volume_valid = 0;

static int do_trigger = 0;
static uint64_t written = 0;
static int time_offset_msec = 0;
static int just_flushed = 0;

static int connected = 0;

static pa_time_event *volume_time_event = NULL;

#define CHECK_DEAD_GOTO(label, warn) do { \
if (!mainloop || \
    !context || pa_context_get_state(context) != PA_CONTEXT_READY || \
    !stream || pa_stream_get_state(stream) != PA_STREAM_READY) { \
        if (warn) \
            g_warning("Connection died: %s", context ? pa_strerror(pa_context_errno(context)) : "NULL"); \
        goto label; \
    }  \
} while(0);

#define CHECK_CONNECTED(retval) \
do { \
    if (!connected) return retval; \
} while (0);

/* This function is from xmms' core */
gint ctrlsocket_get_session_id(void);

static const char* get_song_name(void) {
    static char t[256];
    gint session, pos;
    char *str, *u;

    session = ctrlsocket_get_session_id();
    pos = xmms_remote_get_playlist_pos(session);
    if (!(str = xmms_remote_get_playlist_title(session, pos)))
        return "Playback Stream";

    snprintf(t, sizeof(t), "%s", u = pa_locale_to_utf8(str));
    pa_xfree(u);
    
    return t;
}

static void info_cb(struct pa_context *c, const struct pa_sink_input_info *i, int is_last, void *userdata) {
    assert(c);

    if (!i)
        return;

    volume = i->volume;
    volume_valid = 1;
}

static void subscribe_cb(struct pa_context *c, enum pa_subscription_event_type t, uint32_t index, void *userdata) {
    pa_operation *o;
    
    assert(c);

    if (!stream ||
        index != pa_stream_get_index(stream) ||
        (t != (PA_SUBSCRIPTION_EVENT_SINK_INPUT|PA_SUBSCRIPTION_EVENT_CHANGE) &&
         t != (PA_SUBSCRIPTION_EVENT_SINK_INPUT|PA_SUBSCRIPTION_EVENT_NEW)))
        return;

    if (!(o = pa_context_get_sink_input_info(c, index, info_cb, NULL))) {
        g_warning("pa_context_get_sink_input_info() failed: %s", pa_strerror(pa_context_errno(c)));
        return;
    }
    
    pa_operation_unref(o);
}

static void context_state_cb(pa_context *c, void *userdata) {
    assert(c);

    switch (pa_context_get_state(c)) {
        case PA_CONTEXT_READY:
        case PA_CONTEXT_TERMINATED:
        case PA_CONTEXT_FAILED:
            pa_threaded_mainloop_signal(mainloop, 0);
            break;

        case PA_CONTEXT_UNCONNECTED:
        case PA_CONTEXT_CONNECTING:
        case PA_CONTEXT_AUTHORIZING:
        case PA_CONTEXT_SETTING_NAME:
            break;
    }
}

static void stream_state_cb(pa_stream *s, void * userdata) {
    assert(s);

    switch (pa_stream_get_state(s)) {

        case PA_STREAM_READY:
        case PA_STREAM_FAILED:
        case PA_STREAM_TERMINATED:
            pa_threaded_mainloop_signal(mainloop, 0);
            break;

        case PA_STREAM_UNCONNECTED:
        case PA_STREAM_CREATING:
            break;
    }
}

static void stream_success_cb(pa_stream *s, int success, void *userdata) {
    assert(s);

    if (userdata)
        *(int*) userdata = success;
    
    pa_threaded_mainloop_signal(mainloop, 0);
}

static void context_success_cb(pa_context *c, int success, void *userdata) {
    assert(c);

    if (userdata)
        *(int*) userdata = success;
    
    pa_threaded_mainloop_signal(mainloop, 0);
}

static void stream_request_cb(pa_stream *s, size_t length, void *userdata) {
    assert(s);

    pa_threaded_mainloop_signal(mainloop, 0);
}

static void stream_latency_update_cb(pa_stream *s, void *userdata) {
    assert(s);

    pa_threaded_mainloop_signal(mainloop, 0);
}

static void pulse_get_volume(int *l, int *r) {
    pa_cvolume v;
    int b = 0;

/*     g_message("get_volume"); */

    *l = *r = 100;

    if (connected) {
        pa_threaded_mainloop_lock(mainloop);
        CHECK_DEAD_GOTO(fail, 1);
        
        v = volume;
        b = volume_valid;

    fail:
        pa_threaded_mainloop_unlock(mainloop);
    } else {
        v = volume;
        b = volume_valid;
    }
    
    if (b) {
        if (v.channels == 2) {
            *l = (int) ((v.values[0]*100)/PA_VOLUME_NORM);
            *r = (int) ((v.values[1]*100)/PA_VOLUME_NORM);
        } else
            *l = *r = (int) ((pa_cvolume_avg(&v)*100)/PA_VOLUME_NORM);
    }
}

static void volume_time_cb(pa_mainloop_api *api, pa_time_event *e, const struct timeval *tv, void *userdata) {
    pa_operation *o;
    
    if (!(o = pa_context_set_sink_input_volume(context, pa_stream_get_index(stream), &volume, NULL, NULL))) 
        g_warning("pa_context_set_sink_input_volume() failed: %s", pa_strerror(pa_context_errno(context)));
    else
        pa_operation_unref(o);

    /* We don't wait for completion of this command */

    api->time_free(volume_time_event);
    volume_time_event = NULL;
}

static void pulse_set_volume(int l, int r) {

/*     g_message("set_volume"); */

    if (connected) {
        pa_threaded_mainloop_lock(mainloop);
        CHECK_DEAD_GOTO(fail, 1);
    }

    if (!volume_valid || volume.channels !=  1) {
        volume.values[0] = ((pa_volume_t) l * PA_VOLUME_NORM)/100;
        volume.values[1] = ((pa_volume_t) r * PA_VOLUME_NORM)/100;
        volume.channels = 2;
    } else {
        volume.values[0] = ((pa_volume_t) l * PA_VOLUME_NORM)/100;
        volume.channels = 1;
    }

    volume_valid = 1;

    if (connected && !volume_time_event) {
        struct timeval tv;
        pa_mainloop_api *api = pa_threaded_mainloop_get_api(mainloop);
        volume_time_event = api->time_new(api, pa_timeval_add(pa_gettimeofday(&tv), 100000), volume_time_cb, NULL);
    }

fail:
    if (connected)
        pa_threaded_mainloop_unlock(mainloop);
}

static void pulse_pause(short b) {
    pa_operation *o = NULL;
    int success = 0;

/*     g_message("pause"); */

    CHECK_CONNECTED();

    pa_threaded_mainloop_lock(mainloop);
    CHECK_DEAD_GOTO(fail, 1);

    if (!(o = pa_stream_cork(stream, b, stream_success_cb, &success))) {
        g_warning("pa_stream_cork() failed: %s", pa_strerror(pa_context_errno(context)));
        goto fail;
    }
    
    while (pa_operation_get_state(o) != PA_OPERATION_DONE) {
        CHECK_DEAD_GOTO(fail, 1);
        pa_threaded_mainloop_wait(mainloop);
    }

    if (!success)
        g_warning("pa_stream_cork() failed: %s", pa_strerror(pa_context_errno(context)));
    
fail:

    if (o)
        pa_operation_unref(o);
    
    pa_threaded_mainloop_unlock(mainloop);
}

static int pulse_free(void) {
    size_t l = 0;
    pa_operation *o = NULL;

/*     g_message("free"); */

    CHECK_CONNECTED(0);

    pa_threaded_mainloop_lock(mainloop);
    CHECK_DEAD_GOTO(fail, 1);

    if ((l = pa_stream_writable_size(stream)) == (size_t) -1) {
        g_warning("pa_stream_writable_size() failed: %s", pa_strerror(pa_context_errno(context)));
        l = 0;
        goto fail;
    }

    /* If this function is called twice with no pulse_write() call in
     * between this means we should trigger the playback */
    if (do_trigger) {
        int success = 0;
        
        if (!(o = pa_stream_trigger(stream, stream_success_cb, &success))) {
            g_warning("pa_stream_trigger() failed: %s", pa_strerror(pa_context_errno(context)));
            goto fail;
        }
        
        while (pa_operation_get_state(o) != PA_OPERATION_DONE) {
            CHECK_DEAD_GOTO(fail, 1);
            pa_threaded_mainloop_wait(mainloop);
        }
        
        if (!success)
            g_warning("pa_stream_trigger() failed: %s", pa_strerror(pa_context_errno(context)));
    }
    
fail:
    if (o)
        pa_operation_unref(o);
    
    pa_threaded_mainloop_unlock(mainloop);

    do_trigger = !!l;
    return (int) l;
}

static int pulse_get_written_time(void) {
    int r = 0;
    
/*     g_message("get_written_time"); */

    CHECK_CONNECTED(0);

    pa_threaded_mainloop_lock(mainloop);
    CHECK_DEAD_GOTO(fail, 1);
    
    r = (int) (((double) written*1000) / pa_bytes_per_second(pa_stream_get_sample_spec(stream)));

/*     g_message("written_time = %i", r); */

fail:
    pa_threaded_mainloop_unlock(mainloop);

    return r;
}

static int pulse_get_output_time(void) {
    int r = 0;
    pa_usec_t t;
    
/*     g_message("get_output_time"); */

    CHECK_CONNECTED(0);

    pa_threaded_mainloop_lock(mainloop);

    for (;;) {
        CHECK_DEAD_GOTO(fail, 1);
        
        if (pa_stream_get_time(stream, &t) >= 0)
            break;

        if (pa_context_errno(context) != PA_ERR_NODATA) {
            g_warning("pa_stream_get_time() failed: %s", pa_strerror(pa_context_errno(context)));
            goto fail;
        }

        pa_threaded_mainloop_wait(mainloop);
    }

    r = (int) (t / 1000);

    if (just_flushed) {
        time_offset_msec -= r;
        just_flushed = 0;
    } 

    r += time_offset_msec;

/*     g_message("output_time = %i", r); */
    
fail:
    pa_threaded_mainloop_unlock(mainloop);
    
    return r;
}

static int pulse_playing(void) {
    int r = 0;
    const pa_timing_info *i;

    CHECK_CONNECTED(0);
    
/*     g_message("playing"); */

    pa_threaded_mainloop_lock(mainloop);

    for (;;) {
        CHECK_DEAD_GOTO(fail, 1);

        if ((i = pa_stream_get_timing_info(stream)))
            break;
        
        if (pa_context_errno(context) != PA_ERR_NODATA) {
            g_warning("pa_stream_get_timing_info() failed: %s", pa_strerror(pa_context_errno(context)));
            goto fail;
        }

        pa_threaded_mainloop_wait(mainloop);
    }

    r = i->playing;

fail:
    pa_threaded_mainloop_unlock(mainloop);

    return r;
}

static void pulse_flush(int time) {
    pa_operation *o = NULL;
    int success = 0;

/*     g_message("flush"); */

    CHECK_CONNECTED();

    pa_threaded_mainloop_lock(mainloop);
    CHECK_DEAD_GOTO(fail, 1);

    if (!(o = pa_stream_flush(stream, stream_success_cb, &success))) {
        g_warning("pa_stream_flush() failed: %s", pa_strerror(pa_context_errno(context)));
        goto fail;
    }
    
    while (pa_operation_get_state(o) != PA_OPERATION_DONE) {
        CHECK_DEAD_GOTO(fail, 1);
        pa_threaded_mainloop_wait(mainloop);
    }

    if (!success)
        g_warning("pa_stream_flush() failed: %s", pa_strerror(pa_context_errno(context)));
    
    written = (uint64_t) (((double) time * pa_bytes_per_second(pa_stream_get_sample_spec(stream))) / 1000);
    just_flushed = 1;
    time_offset_msec = time;
    
fail:
    if (o)
        pa_operation_unref(o);
    
    pa_threaded_mainloop_unlock(mainloop);
}

static void pulse_write(void* ptr, int length) {

/*     g_message("write"); */
    
    CHECK_CONNECTED();

    pa_threaded_mainloop_lock(mainloop);
    CHECK_DEAD_GOTO(fail, 1);

    if (pa_stream_write(stream, ptr, length, NULL, PA_SEEK_RELATIVE, 0) < 0) {
        g_warning("pa_stream_write() failed: %s", pa_strerror(pa_context_errno(context)));
        goto fail;
    }
    
    do_trigger = 0;
    written += length;

fail:
    
    pa_threaded_mainloop_unlock(mainloop);
}

static void drain(void) {
    pa_operation *o = NULL;
    int success = 0;

    CHECK_CONNECTED();

    pa_threaded_mainloop_lock(mainloop);
    CHECK_DEAD_GOTO(fail, 0);

    if (!(o = pa_stream_drain(stream, stream_success_cb, &success))) {
        g_warning("pa_stream_drain() failed: %s", pa_strerror(pa_context_errno(context)));
        goto fail;
    }
    
    while (pa_operation_get_state(o) != PA_OPERATION_DONE) {
        CHECK_DEAD_GOTO(fail, 1);
        pa_threaded_mainloop_wait(mainloop);
    }

    if (!success)
        g_warning("pa_stream_drain() failed: %s", pa_strerror(pa_context_errno(context)));
    
fail:
    if (o)
        pa_operation_unref(o);
    
    pa_threaded_mainloop_unlock(mainloop);
}

static void pulse_close(void) {

/*     g_message("close"); */
    
    drain();

    connected = 0;

    if (mainloop)
        pa_threaded_mainloop_stop(mainloop);

    if (stream) {
        pa_stream_disconnect(stream);
        pa_stream_unref(stream);
        stream = NULL;
    }

    if (context) {
        pa_context_disconnect(context);
        pa_context_unref(context);
        context = NULL;
    }
    
    if (mainloop) {
        pa_threaded_mainloop_free(mainloop);
        mainloop = NULL;
    }

    volume_time_event = NULL;
}

static int pulse_open(AFormat fmt, int rate, int nch) {
    pa_sample_spec ss;
    pa_operation *o = NULL;
    int success;

/*     g_message("open"); */

    g_assert(!mainloop);
    g_assert(!context);
    g_assert(!stream);
    g_assert(!connected);
    
    if (fmt == FMT_U8)
        ss.format = PA_SAMPLE_U8;
    else if (fmt == FMT_S16_LE)
        ss.format = PA_SAMPLE_S16LE;
    else if (fmt == FMT_S16_BE)
        ss.format = PA_SAMPLE_S16BE;
    else if (fmt == FMT_S16_NE)
        ss.format = PA_SAMPLE_S16NE;
    else
        return FALSE;

    ss.rate = rate;
    ss.channels = nch;

    if (!pa_sample_spec_valid(&ss))
        return FALSE;

    if (!volume_valid) {
        pa_cvolume_reset(&volume, ss.channels);
        volume_valid = 1;
    } else if (volume.channels != ss.channels)
        pa_cvolume_set(&volume, ss.channels, pa_cvolume_avg(&volume));

    if (!(mainloop = pa_threaded_mainloop_new())) {
        g_warning("Failed to allocate main loop");
        goto fail;
    }

    pa_threaded_mainloop_lock(mainloop);
    
    if (!(context = pa_context_new(pa_threaded_mainloop_get_api(mainloop), "XMMS"))) {
        g_warning("Failed to allocate context");
        goto unlock_and_fail;
    }

    pa_context_set_state_callback(context, context_state_cb, NULL);
    pa_context_set_subscribe_callback(context, subscribe_cb, NULL);

    if (pa_context_connect(context, NULL, 0, NULL) < 0) {
        g_warning("Failed to connect to server: %s", pa_strerror(pa_context_errno(context)));
        goto unlock_and_fail;
    }

    if (pa_threaded_mainloop_start(mainloop) < 0) {
        g_warning("Failed to start main loop");
        goto unlock_and_fail;
    }

    /* Wait until the context is ready */
    pa_threaded_mainloop_wait(mainloop);

    if (pa_context_get_state(context) != PA_CONTEXT_READY) {
        g_warning("Failed to connect to server: %s", pa_strerror(pa_context_errno(context)));
        goto unlock_and_fail;
    }

    if (!(stream = pa_stream_new(context, get_song_name(), &ss, NULL))) {
        g_warning("Failed to create stream: %s", pa_strerror(pa_context_errno(context)));
        goto unlock_and_fail;
    }

    pa_stream_set_state_callback(stream, stream_state_cb, NULL);
    pa_stream_set_write_callback(stream, stream_request_cb, NULL);
    pa_stream_set_latency_update_callback(stream, stream_latency_update_cb, NULL);

    if (pa_stream_connect_playback(stream, NULL, NULL, PA_STREAM_INTERPOLATE_TIMING|PA_STREAM_AUTO_TIMING_UPDATE, &volume, NULL) < 0) {
        g_warning("Failed to connect stream: %s", pa_strerror(pa_context_errno(context)));
        goto unlock_and_fail;
    }

    /* Wait until the stream is ready */
    pa_threaded_mainloop_wait(mainloop);

    if (pa_stream_get_state(stream) != PA_STREAM_READY) {
        g_warning("Failed to connect stream: %s", pa_strerror(pa_context_errno(context)));
        goto unlock_and_fail;
    }

    /* Now subscribe to events */
    if (!(o = pa_context_subscribe(context, PA_SUBSCRIPTION_MASK_SINK_INPUT, context_success_cb, &success))) {
        g_warning("pa_context_subscribe() failed: %s", pa_strerror(pa_context_errno(context)));
        goto unlock_and_fail;
    }
    
    success = 0;
    while (pa_operation_get_state(o) != PA_OPERATION_DONE) {
        CHECK_DEAD_GOTO(fail, 1);
        pa_threaded_mainloop_wait(mainloop);
    }

    if (!success) {
        g_warning("pa_context_subscribe() failed: %s", pa_strerror(pa_context_errno(context)));
        goto unlock_and_fail;
    }

    pa_operation_unref(o);

    /* Now request the initial stream info */
    if (!(o = pa_context_get_sink_input_info(context, pa_stream_get_index(stream), info_cb, NULL))) {
        g_warning("pa_context_get_sink_input_info() failed: %s", pa_strerror(pa_context_errno(context)));
        goto unlock_and_fail;
    }
    
    while (pa_operation_get_state(o) != PA_OPERATION_DONE) {
        CHECK_DEAD_GOTO(fail, 1);
        pa_threaded_mainloop_wait(mainloop);
    }

    if (!volume_valid) {
        g_warning("pa_context_get_sink_input_info() failed: %s", pa_strerror(pa_context_errno(context)));
        goto unlock_and_fail;
    }

    do_trigger = 0;
    written = 0;
    time_offset_msec = 0;
    just_flushed = 0;
    connected = 1;
    volume_time_event = NULL;
    
    pa_threaded_mainloop_unlock(mainloop);
    
    return TRUE;

unlock_and_fail:

    if (o)
        pa_operation_unref(o);
    
    pa_threaded_mainloop_unlock(mainloop);
    
fail:

    pulse_close();
    
    return FALSE;
}

static void pulse_init(void) {
}

static void pulse_about(void) {
    static GtkWidget *dialog;
    
    if (dialog != NULL)
        return;
    
    dialog = xmms_show_message(
            "About XMMS PulseAudio Output Plugin",
            "XMMS PulseAudio Output Plugin\n\n "
            "This program is free software; you can redistribute it and/or modify\n"
            "it under the terms of the GNU General Public License as published by\n"
            "the Free Software Foundation; either version 2 of the License, or\n"
            "(at your option) any later version.\n"
            "\n"
            "This program is distributed in the hope that it will be useful,\n"
            "but WITHOUT ANY WARRANTY; without even the implied warranty of\n"
            "MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the\n"
            "GNU General Public License for more details.\n"
            "\n"
            "You should have received a copy of the GNU General Public License\n"
            "along with this program; if not, write to the Free Software\n"
            "Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307,\n"
            "USA.",
            "OK",
            FALSE,
            NULL,
            NULL);
    
    gtk_signal_connect(
            GTK_OBJECT(dialog),
            "destroy",
            GTK_SIGNAL_FUNC(gtk_widget_destroyed),
            &dialog);
}


OutputPlugin *get_oplugin_info(void) {
    static OutputPlugin pulse_plugin = {
        NULL,
        NULL,
        "PulseAudio Output Plugin",
        pulse_init,
	NULL,                        
        pulse_about,
        NULL,                       /* pulse_configure, */
        pulse_get_volume,
        pulse_set_volume,
        pulse_open,                         
        pulse_write,                        
        pulse_close,                        
        pulse_flush,                        
        pulse_pause,                        
        pulse_free,                         
        pulse_playing,                      
        pulse_get_output_time,              
        pulse_get_written_time,
	NULL,             
    };

    return &pulse_plugin;
}
