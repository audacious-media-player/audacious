/*  BMP - Cross-platform multimedia player
 *  Copyright (C) 2003-2004  BMP development team.
 *
 *  Based on XMMS:
 *  Copyright (C) 1998-2003  XMMS development team.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include "wav.h"

#include <glib.h>
#include <glib/gi18n.h>
#include <string.h>

#include <libaudacious/util.h>
#include <libaudacious/titlestring.h>
#include "audacious/output.h"


InputPlugin wav_ip = {
    NULL,
    NULL,
    NULL,                       /* Description */
    wav_init,
    NULL,
    NULL,
    is_our_file,
    NULL,
    play_file,
    stop,
    wav_pause,
    seek,
    NULL,
    get_time,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    get_song_info,
    NULL,                       /* file_info_box */
    NULL
};

WaveFile *wav_file = NULL;
static GThread *decode_thread;
static gboolean audio_error = FALSE;

InputPlugin *
get_iplugin_info(void)
{
    wav_ip.description = g_strdup_printf(_("WAV Audio Plugin"));
    return &wav_ip;
}

static void
wav_init(void)
{
    /* empty */
}

/* needed for is_our_file() */
static gint
read_n_bytes(VFSFile * file, guint8 * buf, gint n)
{
    if (vfs_fread(buf, 1, n, file) != n) {
        return FALSE;
    }
    return TRUE;
}

static guint32
convert_to_header(guint8 * buf)
{

    return (buf[0] << 24) + (buf[1] << 16) + (buf[2] << 8) + buf[3];
}

static guint32
convert_to_long(guint8 * buf)
{

    return (buf[3] << 24) + (buf[2] << 16) + (buf[1] << 8) + buf[0];
}

static guint16
read_wav_id(gchar * filename)
{
    VFSFile *file;
    guint16 wavid;
    guint8 buf[4];
    guint32 head;
    glong seek;

    if (!(file = vfs_fopen(filename, "rb"))) {  /* Could not open file */
        return 0;
    }
    if (!(read_n_bytes(file, buf, 4))) {
        vfs_fclose(file);
        return 0;
    }
    head = convert_to_header(buf);
    if (head == ('R' << 24) + ('I' << 16) + ('F' << 8) + 'F') { /* Found a riff -- maybe WAVE */
        if (vfs_fseek(file, 4, SEEK_CUR) != 0) {    /* some error occured */
            vfs_fclose(file);
            return 0;
        }
        if (!(read_n_bytes(file, buf, 4))) {
            vfs_fclose(file);
            return 0;
        }
        head = convert_to_header(buf);
        if (head == ('W' << 24) + ('A' << 16) + ('V' << 8) + 'E') { /* Found a WAVE */
            seek = 0;
            do {                /* we'll be looking for the fmt-chunk which comes before the data-chunk */
                /* A chunk consists of an header identifier (4 bytes), the length of the chunk
                   (4 bytes), and the chunkdata itself, padded to be an even number of bytes.
                   We'll skip all chunks until we find the "data"-one which could contain
                   mpeg-data */
                if (seek != 0) {
                    if (vfs_fseek(file, seek, SEEK_CUR) != 0) { /* some error occured */
                        vfs_fclose(file);
                        return 0;
                    }
                }
                if (!(read_n_bytes(file, buf, 4))) {
                    vfs_fclose(file);
                    return 0;
                }
                head = convert_to_header(buf);
                if (!(read_n_bytes(file, buf, 4))) {
                    vfs_fclose(file);
                    return 0;
                }
                seek = convert_to_long(buf);
                seek = seek + (seek % 2);   /* Has to be even (padding) */
                if (seek >= 2
                    && head == ('f' << 24) + ('m' << 16) + ('t' << 8) + ' ') {
                    if (!(read_n_bytes(file, buf, 2))) {
                        vfs_fclose(file);
                        return 0;
                    }
                    wavid = buf[0] + 256 * buf[1];
                    seek -= 2;
                    /* we could go on looking for other things, but all we wanted was the wavid */
                    vfs_fclose(file);
                    return wavid;
                }
            }
            while (head != ('d' << 24) + ('a' << 16) + ('t' << 8) + 'a');
            /* it's RIFF WAVE */
        }
        /* it's RIFF */
    }
    /* it's not even RIFF */
    vfs_fclose(file);
    return 0;
}

static const gchar *
get_extension(const gchar * filename)
{
    const gchar *ext = strrchr(filename, '.');
    return ext ? ext + 1 : NULL;
}

static gboolean
is_our_file(gchar * filename)
{
    gchar *ext;

    ext = strrchr(filename, '.');
    if (ext)
        if (!strcasecmp(ext, ".wav"))
            if (read_wav_id(filename) == WAVE_FORMAT_PCM)
                return TRUE;
    return FALSE;
}


static gchar *
get_title(const gchar * filename)
{
    TitleInput *input;
    gchar *title;

    input = bmp_title_input_new();

    input->file_name = g_path_get_basename(filename);
    input->file_ext = get_extension(filename);
    input->file_path = g_path_get_dirname(filename);

    if (!(title = xmms_get_titlestring(xmms_get_gentitle_format(), input)))
        title = g_strdup(input->file_name);

    g_free(input->file_path);
    g_free(input->file_name);
    g_free(input);

    return title;
}

static gint
read_le_long(VFSFile * file, glong * ret)
{
    guchar buf[4];

    if (vfs_fread(buf, 1, 4, file) != 4)
        return 0;

    *ret = (buf[3] << 24) | (buf[2] << 16) | (buf[1] << 8) | buf[0];
    return TRUE;
}

#define read_le_ulong(file,ret) read_le_long(file,(long*)ret)

static int
read_le_short(VFSFile * file, gshort * ret)
{
    guchar buf[2];

    if (vfs_fread(buf, 1, 2, file) != 2)
        return 0;

    *ret = (buf[1] << 8) | buf[0];
    return TRUE;
}

static gpointer
play_loop(gpointer arg)
{
    gchar data[2048 * 2];
    gint bytes, blk_size, rate;
    gint actual_read;

    blk_size = 512 * (wav_file->bits_per_sample / 8) * wav_file->channels;
    rate =
        wav_file->samples_per_sec * wav_file->channels *
        (wav_file->bits_per_sample / 8);
    while (wav_file->going) {
        if (!wav_file->eof) {
            bytes = blk_size;
            if (wav_file->length - wav_file->position < bytes)
                bytes = wav_file->length - wav_file->position;
            if (bytes > 0) {
                actual_read = vfs_fread(data, 1, bytes, wav_file->file);

                if (actual_read == 0) {
                    wav_file->eof = 1;
                    wav_ip.output->buffer_free();
                    wav_ip.output->buffer_free();
                }
                else {
                    if (wav_file->seek_to == -1)
                        produce_audio(wav_ip.output->written_time(),
                                      (wav_file->bits_per_sample ==
                                       16) ? FMT_S16_LE : FMT_U8,
                                      wav_file->channels, bytes, data,
                                      &wav_file->going);
                    wav_file->position += actual_read;
                }
            }
            else {
                wav_file->eof = TRUE;
                wav_ip.output->buffer_free();
                wav_ip.output->buffer_free();
                xmms_usleep(10000);
            }
        }
        else
            xmms_usleep(10000);
        if (wav_file->seek_to != -1) {
            wav_file->position = wav_file->seek_to * rate;
            vfs_fseek(wav_file->file,
                      wav_file->position + wav_file->data_offset, SEEK_SET);
            wav_ip.output->flush(wav_file->seek_to * 1000);
            wav_file->seek_to = -1;
        }

    }
    vfs_fclose(wav_file->file);

    g_thread_exit(NULL);
    return NULL;
}

static void
play_file(gchar * filename)
{
    gchar magic[4], *name;
    gulong len;
    gint rate;

    audio_error = FALSE;

    wav_file = g_new0(WaveFile, 1);
    if ((wav_file->file = vfs_fopen(filename, "rb"))) {
        vfs_fread(magic, 1, 4, wav_file->file);
        if (strncmp(magic, "RIFF", 4)) {
            vfs_fclose(wav_file->file);
            g_free(wav_file);
            wav_file = NULL;
            return;
        }
        read_le_ulong(wav_file->file, &len);
        vfs_fread(magic, 1, 4, wav_file->file);
        if (strncmp(magic, "WAVE", 4)) {
            vfs_fclose(wav_file->file);
            g_free(wav_file);
            wav_file = NULL;
            return;
        }
        for (;;) {
            vfs_fread(magic, 1, 4, wav_file->file);
            if (!read_le_ulong(wav_file->file, &len)) {
                vfs_fclose(wav_file->file);
                g_free(wav_file);
                wav_file = NULL;
                return;
            }
            if (!strncmp("fmt ", magic, 4))
                break;
            vfs_fseek(wav_file->file, len, SEEK_CUR);
        }
        if (len < 16) {
            vfs_fclose(wav_file->file);
            g_free(wav_file);
            wav_file = NULL;
            return;
        }
        read_le_short(wav_file->file, &wav_file->format_tag);
        switch (wav_file->format_tag) {
        case WAVE_FORMAT_UNKNOWN:
        case WAVE_FORMAT_ALAW:
        case WAVE_FORMAT_MULAW:
        case WAVE_FORMAT_ADPCM:
        case WAVE_FORMAT_OKI_ADPCM:
        case WAVE_FORMAT_DIGISTD:
        case WAVE_FORMAT_DIGIFIX:
        case IBM_FORMAT_MULAW:
        case IBM_FORMAT_ALAW:
        case IBM_FORMAT_ADPCM:
            vfs_fclose(wav_file->file);
            g_free(wav_file);
            wav_file = NULL;
            return;
        }
        read_le_short(wav_file->file, &wav_file->channels);
        read_le_long(wav_file->file, &wav_file->samples_per_sec);
        read_le_long(wav_file->file, &wav_file->avg_bytes_per_sec);
        read_le_short(wav_file->file, &wav_file->block_align);
        read_le_short(wav_file->file, &wav_file->bits_per_sample);
        if (wav_file->bits_per_sample != 8 && wav_file->bits_per_sample != 16) {
            vfs_fclose(wav_file->file);
            g_free(wav_file);
            wav_file = NULL;
            return;
        }
        len -= 16;
        if (len)
            vfs_fseek(wav_file->file, len, SEEK_CUR);

        for (;;) {
            vfs_fread(magic, 4, 1, wav_file->file);

            if (!read_le_ulong(wav_file->file, &len)) {
                vfs_fclose(wav_file->file);
                g_free(wav_file);
                wav_file = NULL;
                return;
            }
            if (!strncmp("data", magic, 4))
                break;
            vfs_fseek(wav_file->file, len, SEEK_CUR);
        }
        wav_file->data_offset = vfs_ftell(wav_file->file);
        wav_file->length = len;

        wav_file->position = 0;
        wav_file->going = 1;

        if (wav_ip.output->
            open_audio((wav_file->bits_per_sample ==
                        16) ? FMT_S16_LE : FMT_U8,
                       wav_file->samples_per_sec, wav_file->channels) == 0) {
            audio_error = TRUE;
            vfs_fclose(wav_file->file);
            g_free(wav_file);
            wav_file = NULL;
            return;
        }
        name = get_title(filename);
        rate =
            wav_file->samples_per_sec * wav_file->channels *
            (wav_file->bits_per_sample / 8);
        wav_ip.set_info(name, 1000 * (wav_file->length / rate), 8 * rate,
                        wav_file->samples_per_sec, wav_file->channels);
        g_free(name);
        wav_file->seek_to = -1;
        decode_thread = g_thread_create(play_loop, NULL, TRUE, NULL);
    }
}

static void
stop(void)
{
    if (wav_file && wav_file->going) {
        wav_file->going = 0;
        g_thread_join(decode_thread);
        wav_ip.output->close_audio();
        g_free(wav_file);
        wav_file = NULL;
    }
}

static void
wav_pause(gshort p)
{
    wav_ip.output->pause(p);
}

static void
seek(gint time)
{
    wav_file->seek_to = time;

    wav_file->eof = FALSE;

    while (wav_file->seek_to != -1)
        xmms_usleep(10000);
}

static int
get_time(void)
{
    if (audio_error)
        return -2;
    if (!wav_file)
        return -1;
    if (!wav_file->going
        || (wav_file->eof && !wav_ip.output->buffer_playing()))
        return -1;
    else {
        return wav_ip.output->output_time();
    }
}

static void
get_song_info(gchar * filename, gchar ** title, gint * length)
{
    gchar magic[4];
    gulong len;
    gint rate;
    WaveFile *wav_file;

    wav_file = g_malloc(sizeof(WaveFile));
    memset(wav_file, 0, sizeof(WaveFile));
    if (!(wav_file->file = vfs_fopen(filename, "rb")))
        return;

    vfs_fread(magic, 1, 4, wav_file->file);
    if (strncmp(magic, "RIFF", 4)) {
        vfs_fclose(wav_file->file);
        g_free(wav_file);
        wav_file = NULL;
        return;
    }
    read_le_ulong(wav_file->file, &len);
    vfs_fread(magic, 1, 4, wav_file->file);
    if (strncmp(magic, "WAVE", 4)) {
        vfs_fclose(wav_file->file);
        g_free(wav_file);
        wav_file = NULL;
        return;
    }
    for (;;) {
        vfs_fread(magic, 1, 4, wav_file->file);
        if (!read_le_ulong(wav_file->file, &len)) {
            vfs_fclose(wav_file->file);
            g_free(wav_file);
            wav_file = NULL;
            return;
        }
        if (!strncmp("fmt ", magic, 4))
            break;
        vfs_fseek(wav_file->file, len, SEEK_CUR);
    }
    if (len < 16) {
        vfs_fclose(wav_file->file);
        g_free(wav_file);
        wav_file = NULL;
        return;
    }
    read_le_short(wav_file->file, &wav_file->format_tag);
    switch (wav_file->format_tag) {
    case WAVE_FORMAT_UNKNOWN:
    case WAVE_FORMAT_ALAW:
    case WAVE_FORMAT_MULAW:
    case WAVE_FORMAT_ADPCM:
    case WAVE_FORMAT_OKI_ADPCM:
    case WAVE_FORMAT_DIGISTD:
    case WAVE_FORMAT_DIGIFIX:
    case IBM_FORMAT_MULAW:
    case IBM_FORMAT_ALAW:
    case IBM_FORMAT_ADPCM:
        vfs_fclose(wav_file->file);
        g_free(wav_file);
        wav_file = NULL;
        return;
    }
    read_le_short(wav_file->file, &wav_file->channels);
    read_le_long(wav_file->file, &wav_file->samples_per_sec);
    read_le_long(wav_file->file, &wav_file->avg_bytes_per_sec);
    read_le_short(wav_file->file, &wav_file->block_align);
    read_le_short(wav_file->file, &wav_file->bits_per_sample);
    if (wav_file->bits_per_sample != 8 && wav_file->bits_per_sample != 16) {
        vfs_fclose(wav_file->file);
        g_free(wav_file);
        wav_file = NULL;
        return;
    }
    len -= 16;
    if (len)
        vfs_fseek(wav_file->file, len, SEEK_CUR);

    for (;;) {
        vfs_fread(magic, 4, 1, wav_file->file);

        if (!read_le_ulong(wav_file->file, &len)) {
            vfs_fclose(wav_file->file);
            g_free(wav_file);
            wav_file = NULL;
            return;
        }
        if (!strncmp("data", magic, 4))
            break;
        vfs_fseek(wav_file->file, len, SEEK_CUR);
    }
    rate =
        wav_file->samples_per_sec * wav_file->channels *
        (wav_file->bits_per_sample / 8);
    (*length) = 1000 * (len / rate);
    (*title) = get_title(filename);

    vfs_fclose(wav_file->file);
    g_free(wav_file);
    wav_file = NULL;
}
