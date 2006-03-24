/*
 * Copyright (C) Tony Arcieri <bascule@inferno.tusculum.edu>
 * Copyright (C) 2001-2002  Haavard Kvaalen <havardk@xmms.org>
 *
 * ReplayGain processing Copyright (C) 2002 Gian-Carlo Pascutto <gcp@sjeng.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
 * 02111-1307, USA.
 *
 */

/*
 * 2002-01-11 ReplayGain processing added by Gian-Carlo Pascutto <gcp@sjeng.org>
 */

/*
 * Note that this uses vorbisfile, which is not (currently)
 * thread-safe.
 */

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include <glib.h>
#include <glib/gi18n.h>
#include <gtk/gtk.h>

#include <stdlib.h>
#include <math.h>
#include <string.h>

#include <fcntl.h>

#include <ogg/ogg.h>
#include <vorbis/codec.h>
#include <vorbis/vorbisfile.h>

#include "audacious/plugin.h"
#include "audacious/output.h"
#include "libaudacious/util.h"
#include "libaudacious/configdb.h"
#include "libaudacious/titlestring.h"

#include "vorbis.h"
#include "http.h"

extern vorbis_config_t vorbis_cfg;

static int vorbis_check_file(char *filename);
static void vorbis_play(char *filename);
static void vorbis_stop(void);
static void vorbis_pause(short p);
static void vorbis_seek(int time);
static int vorbis_time(void);
static void vorbis_get_song_info(char *filename, char **title, int *length);
static gchar *vorbis_generate_title(OggVorbis_File * vorbisfile,
                                    const gchar * fn);
static void vorbis_aboutbox(void);
static void vorbis_init(void);
static void vorbis_cleanup(void);
static long vorbis_process_replaygain(float **pcm, int samples, int ch,
                                      char *pcmout, float rg_scale);
static gboolean vorbis_update_replaygain(float *scale);

static size_t ovcb_read(void *ptr, size_t size, size_t nmemb,
                        void *datasource);
static int ovcb_seek(void *datasource, int64_t offset, int whence);
static int ovcb_close(void *datasource);
static long ovcb_tell(void *datasource);

ov_callbacks vorbis_callbacks = {
    ovcb_read,
    ovcb_seek,
    ovcb_close,
    ovcb_tell
};

InputPlugin vorbis_ip = {
    NULL,
    NULL,
    NULL,                       /* description */
    vorbis_init,                /* init */
    vorbis_aboutbox,            /* aboutbox */
    vorbis_configure,           /* configure */
    vorbis_check_file,          /* is_our_file */
    NULL,
    vorbis_play,
    vorbis_stop,
    vorbis_pause,
    vorbis_seek,
    NULL,                       /* set eq */
    vorbis_time,
    NULL,
    NULL,
    vorbis_cleanup,
    NULL,
    NULL,
    NULL,
    NULL,
    vorbis_get_song_info,
    vorbis_file_info_box,       /* file info box, tag editing */
    NULL,
};

static OggVorbis_File vf;

static GThread *thread;
int vorbis_playing = 0;
static int vorbis_eos = 0;
static int vorbis_is_streaming = 0;
static int vorbis_bytes_streamed = 0;
static volatile int seekneeded = -1;
static int samplerate, channels;
GMutex *vf_mutex;
static gboolean output_error;

gchar **vorbis_tag_encoding_list = NULL;

InputPlugin *
get_iplugin_info(void)
{
    vorbis_ip.description = g_strdup_printf(_("Ogg Vorbis Audio Plugin"));
    return &vorbis_ip;
}

static int
vorbis_check_file(char *filename)
{
    VFSFile *stream;
    OggVorbis_File vfile;       /* avoid thread interaction */
    char *ext;
    gint result;

    /* is this our http resource? */
    if (strncasecmp(filename, "http://", 7) == 0) {
        ext = strrchr(filename, '.');
        if (ext) {
            if (!strncasecmp(ext, ".ogg", 4)) {
                return TRUE;
            }
        }
        return FALSE;
    }

    if (!(stream = vfs_fopen(filename, "r"))) {
        return FALSE;
    }
    /*
     * The open function performs full stream detection and machine
     * initialization.  If it returns zero, the stream *is* Vorbis and
     * we're fully ready to decode.
     */

    /* libvorbisfile isn't thread safe... */
    memset(&vfile, 0, sizeof(vfile));
    g_mutex_lock(vf_mutex);

    result = ov_test_callbacks(stream, &vfile, NULL, 0, vorbis_callbacks);

    switch (result) {
    case OV_EREAD:
#ifdef DEBUG
        g_message("** vorbis.c: Media read error: %s", filename);
#endif
        g_mutex_unlock(vf_mutex);
        vfs_fclose(stream);
        return FALSE;
        break;
    case OV_ENOTVORBIS:
#ifdef DEBUG
        g_message("** vorbis.c: Not Vorbis data: %s", filename);
#endif
        g_mutex_unlock(vf_mutex);
        vfs_fclose(stream);
        return FALSE;
        break;
    case OV_EVERSION:
#ifdef DEBUG
        g_message("** vorbis.c: Version mismatch: %s", filename);
#endif
        g_mutex_unlock(vf_mutex);
        vfs_fclose(stream);
        return FALSE;
        break;
    case OV_EBADHEADER:
#ifdef DEBUG
        g_message("** vorbis.c: Invalid Vorbis bistream header: %s",
                  filename);
#endif
        g_mutex_unlock(vf_mutex);
        vfs_fclose(stream);
        return FALSE;
        break;
    case OV_EFAULT:
#ifdef DEBUG
        g_message("** vorbis.c: Internal logic fault while reading %s",
                  filename);
#endif
        g_mutex_unlock(vf_mutex);
        vfs_fclose(stream);
        return FALSE;
        break;
    case 0:
        break;
    default:
        break;
    }


    ov_clear(&vfile);           /* once the ov_open succeeds, the stream belongs to
                                   vorbisfile.a.  ov_clear will fclose it */
    g_mutex_unlock(vf_mutex);
    return TRUE;
}

static void
vorbis_jump_to_time(long time)
{
    g_mutex_lock(vf_mutex);

    /*
     * We need to guard against seeking to the end, or things
     * don't work right.  Instead, just seek to one second prior
     * to this
     */
    if (time == ov_time_total(&vf, -1))
        time--;

    vorbis_ip.output->flush(time * 1000);
    ov_time_seek(&vf, time);

    g_mutex_unlock(vf_mutex);
}

static void
do_seek(void)
{
    if (seekneeded != -1 && !vorbis_is_streaming) {
        vorbis_jump_to_time(seekneeded);
        seekneeded = -1;
        vorbis_eos = FALSE;
    }
}

static int
vorbis_process_data(int last_section, gboolean use_rg, float rg_scale)
{
    char pcmout[4096];
    int bytes;
    float **pcm;

    /*
     * A vorbis physical bitstream may consist of many logical
     * sections (information for each of which may be fetched from
     * the vf structure).  This value is filled in by ov_read to
     * alert us what section we're currently decoding in case we
     * need to change playback settings at a section boundary
     */
    int current_section;

    g_mutex_lock(vf_mutex);
    if (use_rg) {
        bytes =
            ov_read_float(&vf, &pcm, sizeof(pcmout) / 2 / channels,
                          &current_section);
        if (bytes > 0)
            bytes = vorbis_process_replaygain(pcm, bytes, channels,
                                              pcmout, rg_scale);
    }
    else {
        bytes =
            ov_read(&vf, pcmout, sizeof(pcmout),
                    (int) (G_BYTE_ORDER == G_BIG_ENDIAN),
                    2, 1, &current_section);
    }

    switch (bytes) {
    case 0:
        /* EOF */
        g_mutex_unlock(vf_mutex);
        vorbis_ip.output->buffer_free();
        vorbis_ip.output->buffer_free();
        vorbis_eos = TRUE;
        return last_section;

    case OV_HOLE:
    case OV_EBADLINK:
        /*
         * error in the stream.  Not a problem, just
         * reporting it in case we (the app) cares.
         * In this case, we don't.
         */
        g_mutex_unlock(vf_mutex);
        return last_section;
    }

    if (current_section != last_section) {
        /*
         * The info struct is different in each section.  vf
         * holds them all for the given bitstream.  This
         * requests the current one
         */
        vorbis_info *vi = ov_info(&vf, -1);

        if (vi->channels > 2) {
            vorbis_eos = TRUE;
            g_mutex_unlock(vf_mutex);
            return current_section;
        }


        if (vi->rate != samplerate || vi->channels != channels) {
            samplerate = vi->rate;
            channels = vi->channels;
            vorbis_ip.output->buffer_free();
            vorbis_ip.output->buffer_free();
            vorbis_ip.output->close_audio();
            if (!vorbis_ip.output->
                open_audio(FMT_S16_NE, vi->rate, vi->channels)) {
                output_error = TRUE;
                vorbis_eos = TRUE;
                g_mutex_unlock(vf_mutex);
                return current_section;
            }
            vorbis_ip.output->flush(ov_time_tell(&vf) * 1000);
        }
    }

    g_mutex_unlock(vf_mutex);

    if (!vorbis_playing)
        return current_section;

    if (seekneeded != -1)
        do_seek();

    produce_audio(vorbis_ip.output->written_time(),
                  FMT_S16_NE, channels, bytes, pcmout, &vorbis_playing);

    return current_section;
}

static gpointer
vorbis_play_loop(gpointer arg)
{
    char *filename = (char *) arg;
    gchar *title = NULL;
    double time;
    long timercount = 0;
    vorbis_info *vi;

    int last_section = -1;

    VFSFile *stream = NULL;
    void *datasource = NULL;

    gboolean use_rg;
    float rg_scale = 1.0;

    memset(&vf, 0, sizeof(vf));

    if (strncasecmp("http://", filename, 7) != 0) {
        /* file is a real file */
        if ((stream = vfs_fopen(filename, "r")) == NULL) {
            vorbis_eos = TRUE;
            goto play_cleanup;
        }
        datasource = (void *) stream;
    }
    else {
        /* file is a stream */
        vorbis_is_streaming = 1;
        vorbis_http_open(filename);
        datasource = "NULL";
    }

    /*
     * The open function performs full stream detection and
     * machine initialization.  None of the rest of ov_xx() works
     * without it
     */

    g_mutex_lock(vf_mutex);
    if (ov_open_callbacks(datasource, &vf, NULL, 0, vorbis_callbacks) < 0) {
        vorbis_callbacks.close_func(datasource);
        g_mutex_unlock(vf_mutex);
        vorbis_eos = TRUE;
        goto play_cleanup;
    }
    vi = ov_info(&vf, -1);

    if (vorbis_is_streaming)
        time = -1;
    else
        time = ov_time_total(&vf, -1) * 1000;

    if (vi->channels > 2) {
        vorbis_eos = TRUE;
        g_mutex_unlock(vf_mutex);
        goto play_cleanup;
    }

    samplerate = vi->rate;
    channels = vi->channels;

    title = vorbis_generate_title(&vf, filename);
    use_rg = vorbis_update_replaygain(&rg_scale);

    vorbis_ip.set_info(title, time, ov_bitrate(&vf, -1), samplerate,
                       channels);
    if (!vorbis_ip.output->open_audio(FMT_S16_NE, vi->rate, vi->channels)) {
        output_error = TRUE;
        g_mutex_unlock(vf_mutex);
        goto play_cleanup;
    }
    g_mutex_unlock(vf_mutex);

    seekneeded = -1;

    /*
     * Note that chaining changes things here; A vorbis file may
     * be a mix of different channels, bitrates and sample rates.
     * You can fetch the information for any section of the file
     * using the ov_ interface.
     */

    while (vorbis_playing) {
        int current_section;

        if (seekneeded != -1)
            do_seek();

        if (vorbis_eos) {
            xmms_usleep(20000);
            continue;
        }

        current_section = vorbis_process_data(last_section, use_rg, rg_scale);

        if (current_section != last_section) {
            /*
             * set total play time, bitrate, rate, and channels of
             * current section
             */
            if (title)
                g_free(title);
            g_mutex_lock(vf_mutex);
            title = vorbis_generate_title(&vf, filename);
            use_rg = vorbis_update_replaygain(&rg_scale);

            if (vorbis_is_streaming)
                time = -1;
            else
                time = ov_time_total(&vf, -1) * 1000;

            vorbis_ip.set_info(title, time,
                               ov_bitrate(&vf, current_section),
                               samplerate, channels);
            g_mutex_unlock(vf_mutex);
            timercount = vorbis_ip.output->output_time();

            last_section = current_section;
        }

        if (!(vi->bitrate_upper == vi->bitrate_lower && vi->bitrate_upper == vi->bitrate_nominal)
            && (vorbis_ip.output->output_time() > timercount + 1000
                || vorbis_ip.output->output_time() < timercount)) {
            /*
             * simple hack to avoid updating too
             * often
             */
            long br;

            g_mutex_lock(vf_mutex);
            br = ov_bitrate_instant(&vf);
            g_mutex_unlock(vf_mutex);
            if (br > 0)
                vorbis_ip.set_info(title, time, br, samplerate, channels);
            timercount = vorbis_ip.output->output_time();
        }
    }
    if (!output_error)
        vorbis_ip.output->close_audio();
    /* fall through intentional */

  play_cleanup:
    g_free(title);
    g_free(filename);

    /*
     * ov_clear closes the stream if its open.  Safe to call on an
     * uninitialized structure as long as we've zeroed it
     */
    g_mutex_lock(vf_mutex);
    ov_clear(&vf);
    g_mutex_unlock(vf_mutex);
    vorbis_is_streaming = 0;
    return NULL;
}

static void
vorbis_play(char *filename)
{
    vorbis_playing = 1;
    vorbis_bytes_streamed = 0;
    vorbis_eos = 0;
    output_error = FALSE;

    thread = g_thread_create(vorbis_play_loop, g_strdup(filename), TRUE,
                             NULL);
}

static void
vorbis_stop(void)
{
    if (vorbis_playing) {
        vorbis_playing = 0;
        g_thread_join(thread);
    }
}

static void
vorbis_pause(short p)
{
    vorbis_ip.output->pause(p);
}

static int
vorbis_time(void)
{
    if (output_error)
        return -2;
    if (vorbis_eos && !vorbis_ip.output->buffer_playing())
        return -1;
    return vorbis_ip.output->output_time();
}

static gchar *
convert_tag_title(gchar * title)
{
    gchar **encoding = vorbis_tag_encoding_list;
    gchar *new_title = NULL;

    if (g_utf8_validate(title, -1, NULL))
        return title;

    while (*encoding && !new_title) {
        new_title = g_convert(title, strlen(title), "UTF-8", *encoding++,
                              NULL, NULL, NULL);
    }

    if (new_title) {
        g_free(title);
        return new_title;
    }

    /* FIXME: We're relying on BMP core to provide fallback
     * conversion */
    return title;
}

static void
vorbis_seek(int time)
{
    if (vorbis_is_streaming)
        return;

    seekneeded = time;

    while (seekneeded != -1)
        xmms_usleep(20000);
}

static void
vorbis_get_song_info(char *filename, char **title, int *length)
{
    VFSFile *stream;
    OggVorbis_File vf;          /* avoid thread interaction */

    if (strncasecmp(filename, "http://", 7)) {
        if ((stream = vfs_fopen(filename, "r")) == NULL)
            return;

        /*
         * The open function performs full stream detection and
         * machine initialization.  If it returns zero, the stream
         * *is* Vorbis and we're fully ready to decode.
         */
        g_mutex_lock(vf_mutex);
        if (ov_open_callbacks(stream, &vf, NULL, 0, vorbis_callbacks) < 0) {
            g_mutex_unlock(vf_mutex);
            vfs_fclose(stream);
            return;
        }

        /* Retrieve the length */
        *length = ov_time_total(&vf, -1) * 1000;

        *title = NULL;
        *title = vorbis_generate_title(&vf, filename);
        /*
         * once the ov_open succeeds, the stream belongs to
         * vorbisfile.a.  ov_clear will fclose it
         */
        ov_clear(&vf);
        g_mutex_unlock(vf_mutex);
    }
    else {
        /* streaming song info */
        *length = -1;
        *title = (char *) vorbis_http_get_title(filename);
/* Encoding patch */
        if (vorbis_cfg.title_encoding_enabled)
            *title = convert_tag_title(*title);
/* Encoding patch */
    }
}

static const gchar *
get_extension(const gchar * filename)
{
    const gchar *ext;
    if ((ext = strrchr(filename, '.')))
        ++ext;
    return ext;
}

/* Make sure you've locked vf_mutex */
static gboolean
vorbis_update_replaygain(float *scale)
{
    vorbis_comment *comment;
    char *rg_gain = NULL, *rg_peak_str = NULL;
    float rg_peak;

    if (!vorbis_cfg.use_replaygain && !vorbis_cfg.use_anticlip)
        return FALSE;
    if ((comment = ov_comment(&vf, -1)) == NULL)
        return FALSE;

    *scale = 1.0;

    if (vorbis_cfg.use_replaygain) {
        if (vorbis_cfg.replaygain_mode == REPLAYGAIN_MODE_ALBUM) {
            rg_gain =
                vorbis_comment_query(comment, "replaygain_album_gain", 0);
            if (!rg_gain)
                rg_gain = vorbis_comment_query(comment, "rg_audiophile", 0);    /* Old */
        }

        if (!rg_gain)
            rg_gain =
                vorbis_comment_query(comment, "replaygain_track_gain", 0);
        if (!rg_gain)
            rg_gain = vorbis_comment_query(comment, "rg_radio", 0); /* Old */

        /* FIXME: Make sure this string is the correct format first? */
        if (rg_gain)
            *scale = pow(10., atof(rg_gain) / 20);
    }

    if (vorbis_cfg.use_anticlip) {
        if (vorbis_cfg.replaygain_mode == REPLAYGAIN_MODE_ALBUM)
            rg_peak_str =
                vorbis_comment_query(comment, "replaygain_album_peak", 0);

        if (!rg_peak_str)
            rg_peak_str =
                vorbis_comment_query(comment, "replaygain_track_peak", 0);
        if (!rg_peak_str)
            rg_peak_str = vorbis_comment_query(comment, "rg_peak", 0);  /* Old */

        if (rg_peak_str)
            rg_peak = atof(rg_peak_str);
        else
            rg_peak = 1;

        if (*scale * rg_peak > 1.0)
            *scale = 1.0 / rg_peak;
    }

    if (*scale != 1.0 || vorbis_cfg.use_booster) {
        /* safety */
        if (*scale > 15.0)
            *scale = 15.0;

        return TRUE;
    }

    return FALSE;
}

#if (G_BYTE_ORDER == G_BIG_ENDIAN)
#  define GET_BYTE1(val) ((val) >> 8)
#  define GET_BYTE2(val) ((val) & 0xff)
#else
#  define GET_BYTE1(val) ((val) & 0xff)
#  define GET_BYTE2(val) ((val) >> 8)
#endif

static long
vorbis_process_replaygain(float **pcm, int samples, int ch,
                          char *pcmout, float rg_scale)
{
    int i, j;
    /* ReplayGain processing */
    for (i = 0; i < samples; i++)
        for (j = 0; j < ch; j++) {
            float sample = pcm[j][i] * rg_scale;
            int value;

            if (vorbis_cfg.use_booster) {
                sample *= 2;

                /* hard 6dB limiting */
                if (sample < -0.5)
                    sample = tanh((sample + 0.5) / 0.5) * 0.5 - 0.5;
                else if (sample > 0.5)
                    sample = tanh((sample - 0.5) / 0.5) * 0.5 + 0.5;
            }

            value = sample * 32767;
            if (value > 32767)
                value = 32767;
            else if (value < -32767)
                value = -32767;

            *pcmout++ = GET_BYTE1(value);
            *pcmout++ = GET_BYTE2(value);
        }

    return 2 * ch * samples;
}


static gchar *
vorbis_generate_title(OggVorbis_File * vorbisfile, const gchar * filename)
{
    /* Caller should hold vf_mutex */
    gchar *displaytitle = NULL;
    vorbis_comment *comment;
    TitleInput *input;

    input = bmp_title_input_new();

    input->file_name = g_path_get_basename(filename);
    input->file_ext = get_extension(filename);
    input->file_path = g_path_get_dirname(filename);

    if ((comment = ov_comment(vorbisfile, -1))) {
        input->track_name =
            g_strdup(vorbis_comment_query(comment, "title", 0));
        input->performer =
            g_strdup(vorbis_comment_query(comment, "artist", 0));
        input->album_name =
            g_strdup(vorbis_comment_query(comment, "album", 0));

        if (vorbis_comment_query(comment, "tracknumber", 0))
            input->track_number =
                atoi(vorbis_comment_query(comment, "tracknumber", 0));

        input->date = g_strdup(vorbis_comment_query(comment, "date", 0));
        input->genre = g_strdup(vorbis_comment_query(comment, "genre", 0));
        input->comment =
            g_strdup(vorbis_comment_query(comment, "comment", 0));
    }

    if (!(displaytitle = xmms_get_titlestring(vorbis_cfg.tag_override ?
                                              vorbis_cfg.tag_format :
                                              xmms_get_gentitle_format(),
                                              input))) {
        if (!vorbis_is_streaming)
            displaytitle = g_strdup(input->file_name);
        else
            displaytitle = vorbis_http_get_title(filename);
    }

    g_free(input->file_name);
    g_free(input->file_path);
    g_free(input->track_name);
    g_free(input->performer);
    g_free(input->album_name);
    g_free(input->date);
    g_free(input->genre);
    g_free(input->comment);
    g_free(input);
/* Encoding patch */
    if (vorbis_cfg.title_encoding_enabled)
        displaytitle = convert_tag_title(displaytitle);
/* Encoding patch */    

    return displaytitle;
}

static void
vorbis_aboutbox()
{
    static GtkWidget *about_window;

    if (about_window)
        gdk_window_raise(about_window->window);
    else
    {
      about_window = xmms_show_message(_("About Ogg Vorbis Audio Plugin"),
                                       /*
                                        * I18N: UTF-8 Translation: "Haavard Kvaalen" ->
                                        * "H\303\245vard Kv\303\245len"
                                        */
                                       _
                                       ("Ogg Vorbis Plugin by the Xiph.org Foundation\n\n"
                                        "Original code by\n"
                                        "Tony Arcieri <bascule@inferno.tusculum.edu>\n"
                                        "Contributions from\n"
                                        "Chris Montgomery <monty@xiph.org>\n"
                                        "Peter Alm <peter@xmms.org>\n"
                                        "Michael Smith <msmith@labyrinth.edu.au>\n"
                                        "Jack Moffitt <jack@icecast.org>\n"
                                        "Jorn Baayen <jorn@nl.linux.org>\n"
                                        "Haavard Kvaalen <havardk@xmms.org>\n"
                                        "Gian-Carlo Pascutto <gcp@sjeng.org>\n\n"
                                        "Visit the Xiph.org Foundation at http://www.xiph.org/\n"),
                                       _("Ok"), FALSE, NULL, NULL);
      g_signal_connect(G_OBJECT(about_window), "destroy",
                       G_CALLBACK(gtk_widget_destroyed), &about_window);
    }
}


static void
vorbis_init(void)
{
    ConfigDb *db;

    memset(&vorbis_cfg, 0, sizeof(vorbis_config_t));
    vorbis_cfg.http_buffer_size = 128;
    vorbis_cfg.http_prebuffer = 25;
    vorbis_cfg.proxy_port = 8080;
    vorbis_cfg.proxy_use_auth = FALSE;
    vorbis_cfg.proxy_user = NULL;
    vorbis_cfg.proxy_pass = NULL;
    vorbis_cfg.tag_override = FALSE;
    vorbis_cfg.tag_format = NULL;
    vorbis_cfg.use_anticlip = FALSE;
    vorbis_cfg.use_replaygain = FALSE;
    vorbis_cfg.replaygain_mode = REPLAYGAIN_MODE_TRACK;
    vorbis_cfg.use_booster = FALSE;
/* Encoding patch */
    vorbis_cfg.title_encoding_enabled = FALSE;
    vorbis_cfg.title_encoding = NULL;
/* Encoding patch */

    db = bmp_cfg_db_open();
    bmp_cfg_db_get_int(db, "vorbis", "http_buffer_size",
                       &vorbis_cfg.http_buffer_size);
    bmp_cfg_db_get_int(db, "vorbis", "http_prebuffer",
                       &vorbis_cfg.http_prebuffer);
    bmp_cfg_db_get_bool(db, "vorbis", "save_http_stream",
                        &vorbis_cfg.save_http_stream);
    if (!bmp_cfg_db_get_string(db, "vorbis", "save_http_path",
                               &vorbis_cfg.save_http_path))
        vorbis_cfg.save_http_path = g_strdup(g_get_home_dir());

    bmp_cfg_db_get_bool(db, "vorbis", "use_proxy", &vorbis_cfg.use_proxy);
    if (!bmp_cfg_db_get_string
        (db, "vorbis", "proxy_host", &vorbis_cfg.proxy_host))
        vorbis_cfg.proxy_host = g_strdup("localhost");
    bmp_cfg_db_get_int(db, "vorbis", "proxy_port", &vorbis_cfg.proxy_port);
    bmp_cfg_db_get_bool(db, "vorbis", "proxy_use_auth",
                        &vorbis_cfg.proxy_use_auth);
    bmp_cfg_db_get_string(db, "vorbis", "proxy_user", &vorbis_cfg.proxy_user);
    bmp_cfg_db_get_string(db, "vorbis", "proxy_pass", &vorbis_cfg.proxy_pass);
    bmp_cfg_db_get_bool(db, "vorbis", "tag_override",
                        &vorbis_cfg.tag_override);
    if (!bmp_cfg_db_get_string(db, "vorbis", "tag_format",
                               &vorbis_cfg.tag_format))
        vorbis_cfg.tag_format = g_strdup("%p - %t");
    bmp_cfg_db_get_bool(db, "vorbis", "use_anticlip",
                        &vorbis_cfg.use_anticlip);
    bmp_cfg_db_get_bool(db, "vorbis", "use_replaygain",
                        &vorbis_cfg.use_replaygain);
    bmp_cfg_db_get_int(db, "vorbis", "replaygain_mode",
                       &vorbis_cfg.replaygain_mode);
    bmp_cfg_db_get_bool(db, "vorbis", "use_booster", &vorbis_cfg.use_booster);
    /* Encoding patch */
    bmp_cfg_db_get_bool(db, "vorbis", "title_encoding_enabled", &vorbis_cfg.title_encoding_enabled);
    bmp_cfg_db_get_string(db, "vorbis", "title_encoding", &vorbis_cfg.title_encoding);
    if (vorbis_cfg.title_encoding_enabled)
        vorbis_tag_encoding_list = g_strsplit_set(vorbis_cfg.title_encoding, ENCODING_SEPARATOR, 0);
    
    /* Encoding patch */
    bmp_cfg_db_close(db);

    vf_mutex = g_mutex_new();
}

static void
vorbis_cleanup(void)
{
    g_free(vorbis_ip.description);
    vorbis_ip.description = NULL;

    if (vorbis_cfg.save_http_path) {
        free(vorbis_cfg.save_http_path);
        vorbis_cfg.save_http_path = NULL;
    }

    if (vorbis_cfg.proxy_host) {
        free(vorbis_cfg.proxy_host);
        vorbis_cfg.proxy_host = NULL;
    }

    if (vorbis_cfg.proxy_user) {
        free(vorbis_cfg.proxy_user);
        vorbis_cfg.proxy_user = NULL;
    }

    if (vorbis_cfg.proxy_pass) {
        free(vorbis_cfg.proxy_pass);
        vorbis_cfg.proxy_pass = NULL;
    }

    if (vorbis_cfg.tag_format) {
        free(vorbis_cfg.tag_format);
        vorbis_cfg.tag_format = NULL;
    }

    if (vorbis_cfg.title_encoding) {
        free(vorbis_cfg.title_encoding);
        vorbis_cfg.title_encoding = NULL;
    }

    g_strfreev(vorbis_tag_encoding_list);
    g_mutex_free(vf_mutex);
}

static size_t
ovcb_read(void *ptr, size_t size, size_t nmemb, void *datasource)
{
    size_t tmp;


    if (vorbis_is_streaming) {
        /* this is a stream */
        tmp = vorbis_http_read(ptr, size * nmemb);
        vorbis_bytes_streamed += tmp;
        return tmp;
    }

    return vfs_fread(ptr, size, nmemb, (VFSFile *) datasource);
}

static int
ovcb_seek(void *datasource, int64_t offset, int whence)
{
    if (vorbis_is_streaming) {
        /* this is a stream */
        /* streams aren't seekable */
        return -1;
    }

    return vfs_fseek((VFSFile *) datasource, offset, whence);
}

static int
ovcb_close(void *datasource)
{
    if (vorbis_is_streaming) {
        /* this is a stream */
        vorbis_http_close();
        return 0;
    }

    return vfs_fclose((VFSFile *) datasource);
}

static long
ovcb_tell(void *datasource)
{
    if (vorbis_is_streaming) {
        /* this is a stream */
        /* return bytes read */
        return vorbis_bytes_streamed;
    }

    return vfs_ftell((VFSFile *) datasource);
}
