/*
 *  Audacious WMA input plugin
 *  (C) 2005 Audacious development team
 *
 *  Based on:
 *  xmms-wma - WMA player for BMP
 *  Copyright (C) 2004,2005 McMCC <mcmcc@mail.ru>
 *  bmp-wma - WMA player for BMP
 *  Copyright (C) 2004 Roman Bogorodskiy <bogorodskiy@inbox.ru>
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
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02111-1307, USA.
 */
#include <stdlib.h>
#include <malloc.h>
#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <glib.h>
#include <glib/gi18n.h>

#include "audacious/plugin.h"
#include "audacious/output.h"
#include "libaudacious/util.h"
#include "libaudacious/titlestring.h"
#include "libaudacious/vfs.h"

#include "avcodec.h"
#include "avformat.h"

#define ABOUT_TXT "Adapted for use in audacious by Tony Vroon (chainsaw@gentoo.org) from\n \
the BEEP-WMA plugin which is Copyright (C) 2004,2005 Mokrushin I.V. aka McMCC (mcmcc@mail.ru)\n \
and the BMP-WMA plugin which is Copyright (C) 2004 Roman Bogorodskiy <bogorodskiy@inbox.ru>.\n \
This plugin based on source code " LIBAVCODEC_IDENT "\nby Fabrice Bellard from \
http://ffmpeg.sourceforge.net.\n\n \
This program is free software; you can redistribute it and/or modify \n \
it under the terms of the GNU General Public License as published by \n \
the Free Software Foundation; either version 2 of the License, or \n \
(at your option) any later version. \n\n \
This program is distributed in the hope that it will be useful, \n \
but WITHOUT ANY WARRANTY; without even the implied warranty of \n \
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. \n \
See the GNU General Public License for more details.\n"
#define PLUGIN_NAME "Audacious-WMA"
#define PLUGIN_VERSION "v.1.0.5"
#define ST_BUFF 1024

static int wma_decode = 0;
static gboolean wma_pause = 0;
static int wma_seekpos = -1;
static int wma_st_buff, wma_idx, wma_idx2;
static GThread *wma_decode_thread;
GStaticMutex wma_mutex = G_STATIC_MUTEX_INIT;
static AVCodecContext *c = NULL;
static AVFormatContext *ic = NULL;
static AVCodecContext *c2 = NULL;
static AVFormatContext *ic2 = NULL;
static uint8_t *wma_outbuf, *wma_s_outbuf;

char description[64];
static void wma_about(void);
static void wma_init(void);
static int wma_is_our_file(char *filename);
static void wma_play_file(char *filename);
static void wma_stop(void);
static void wma_seek(int time);
static void wma_do_pause(short p);
static int wma_get_time(void);
static void wma_get_song_info(char *filename, char **title, int *length);
static TitleInput *wma_get_song_tuple(char *filename);
static char *wsong_title;
static int wsong_time;

static GtkWidget *dialog1, *button1, *label1;

InputPlugin *get_iplugin_info(void);

InputPlugin wma_ip =
{
    NULL,           	// Filled in by xmms
    NULL,           	// Filled in by xmms
    description,    	// The description that is shown in the preferences box
    wma_init,           // Called when the plugin is loaded
    wma_about,          // Show the about box
    NULL,  	    	// Show the configure box
    wma_is_our_file,    // Return 1 if the plugin can handle the file
    NULL,           	// Scan dir
    wma_play_file,      // Play file
    wma_stop,           // Stop
    wma_do_pause,       // Pause
    wma_seek,           // Seek
    NULL,               // Set the equalizer, most plugins won't be able to do this
    wma_get_time,       // Get the time, usually returns the output plugins output time
    NULL,           	// Get volume
    NULL,           	// Set volume
    NULL,           	// OBSOLETE!
    NULL,           	// OBSOLETE!
    NULL,           	// Send data to the visualization plugins
    NULL,           	// Fill in the stuff that is shown in the player window
    NULL,           	// Show some text in the song title box. Filled in by xmms
    wma_get_song_info,  // Function to grab the title string
    NULL,               // Bring up an info window for the filename passed in
    NULL,           	// Handle to the current output plugin. Filled in by xmms
    wma_get_song_tuple, // Tuple builder
};

InputPlugin *get_iplugin_info(void)
{
    memset(description, 0, 64);
    wma_ip.description = g_strdup_printf(_("WMA Player %s"), PACKAGE_VERSION);
    return &wma_ip;
}

static gchar *str_twenty_to_space(gchar * str)
{
    gchar *match, *match_end;

    g_return_val_if_fail(str != NULL, NULL);

    while ((match = strstr(str, "%20"))) {
        match_end = match + 3;
        *match++ = ' ';
        while (*match_end)
            *match++ = *match_end++;
        *match = 0;
    }

    return str;
}

static void wma_about(void) 
{
    char *title;
    char *message;

    if (dialog1) return;
    
    title = (char *)g_malloc(80);
    message = (char *)g_malloc(1000);
    memset(title, 0, 80);
    memset(message, 0, 1000);

    sprintf(title, _("About %s"), PLUGIN_NAME);
    sprintf(message, "%s %s\n\n%s", PLUGIN_NAME, PLUGIN_VERSION, ABOUT_TXT);

    dialog1 = gtk_dialog_new();
    g_signal_connect(G_OBJECT(dialog1), "destroy",
                        G_CALLBACK(gtk_widget_destroyed), &dialog1);
    gtk_window_set_title(GTK_WINDOW(dialog1), title);
    gtk_window_set_policy(GTK_WINDOW(dialog1), FALSE, FALSE, FALSE);
    gtk_container_border_width(GTK_CONTAINER(dialog1), 5);
    label1 = gtk_label_new(message);
    gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog1)->vbox), label1, TRUE, TRUE, 0);
    gtk_widget_show(label1);

    button1 = gtk_button_new_with_label(_(" Close "));
    g_signal_connect_swapped(G_OBJECT(button1), "clicked",
	                        G_CALLBACK(gtk_widget_destroy),
    	                        GTK_OBJECT(dialog1));
    gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog1)->action_area), button1,
                     FALSE, FALSE, 0);

    gtk_widget_show(button1);
    gtk_widget_show(dialog1);
    gtk_widget_grab_focus(button1);
    g_free(title);
    g_free(message);
}

static void wma_init(void)
{
    avcodec_init();
    avcodec_register_all();
    av_register_all();
}

static int wma_is_our_file(char *filename)
{
    AVCodec *codec2;

    if(av_open_input_file(&ic2, str_twenty_to_space(filename), NULL, 0, NULL) < 0) return 0;

    for(wma_idx2 = 0; wma_idx2 < ic2->nb_streams; wma_idx2++) {
        c2 = &ic2->streams[wma_idx2]->codec;
        if(c2->codec_type == CODEC_TYPE_AUDIO) break;
    }

    av_find_stream_info(ic2);

    codec2 = avcodec_find_decoder(c2->codec_id);

    if(!codec2) {
        av_close_input_file(ic2);
	return 0;
    }
	
    av_close_input_file(ic2);
    return 1;
}

static void wma_do_pause(short p)
{
    wma_pause = p;
    wma_ip.output->pause(wma_pause);
}

static void wma_seek(int time) 
{
    wma_seekpos = time;
    if(wma_pause) wma_ip.output->pause(0);
    while(wma_decode && wma_seekpos!=-1) xmms_usleep(10000);
    if(wma_pause) wma_ip.output->pause(1);
}

static int wma_get_time(void)
{
    wma_ip.output->buffer_free();
    if(wma_decode) return wma_ip.output->output_time();
    return -1;
}

static gchar *extname(const char *filename)
{
    gchar *ext = strrchr(filename, '.');
    if(ext != NULL) ++ext;
    return ext;
}

static char* w_getstr(char* str)
{
    if(str && strlen(str) > 0) return g_strdup(str);
    return NULL;
}

static TitleInput *wma_get_song_tuple(gchar * filename)
{
    TitleInput *tuple = NULL;
    AVFormatContext *in = NULL;
    gchar *filename_proxy = g_strdup(filename);

    if (av_open_input_file(&in, str_twenty_to_space(filename), NULL, 0, NULL) < 0)
	return NULL;

    tuple = bmp_title_input_new();

    tuple->file_name = g_path_get_basename(filename_proxy);
    tuple->file_path = g_path_get_dirname(filename_proxy);
    tuple->file_ext = extname(filename_proxy);
	
    av_find_stream_info(in);

    if((in->title[0] != '\0') || (in->author[0] != '\0') || (in->album[0] != '\0') ||
       (in->comment[0] != '\0') || (in->genre[0] != '\0') || (in->year != 0) || (in->track != 0))
    {	
	tuple->performer = w_getstr(in->author);
	tuple->album_name = w_getstr(in->album);
	tuple->track_name = w_getstr(in->title);
	tuple->year = in->year;
	tuple->track_number = in->track;
	tuple->genre = w_getstr(in->genre);
	tuple->comment = w_getstr(in->comment);
    }

    if (in->duration)
        tuple->length = in->duration / 1000;

    av_close_input_file(in);

    return tuple;
}

static gchar *get_song_title(AVFormatContext *in, gchar * filename)
{
    gchar *ret = NULL;
    TitleInput *input;

    input = bmp_title_input_new();
    
    if((in->title[0] != '\0') || (in->author[0] != '\0') || (in->album[0] != '\0') ||
       (in->comment[0] != '\0') || (in->genre[0] != '\0') || (in->year != 0) || (in->track != 0))
    {	
	input->performer = w_getstr(in->author);
	input->album_name = w_getstr(in->album);
	input->track_name = w_getstr(in->title);
	input->year = in->year;
	input->track_number = in->track;
	input->genre = w_getstr(in->genre);
	input->comment = w_getstr(in->comment);
    }
    input->file_name = g_path_get_basename(filename);
    input->file_path = g_path_get_dirname(filename);
    input->file_ext = extname(filename);
    ret = xmms_get_titlestring(xmms_get_gentitle_format(), input);
    if(input) g_free(input);

    if(!ret)
    {
	    ret = g_strdup(input->file_name);
            if (extname(ret) != NULL)
                    *(extname(ret) - 1) = '\0';
    }
    return ret;
}

static guint get_song_time(AVFormatContext *in)
{
    if(in->duration)
	return in->duration/1000;
    else
	return 0;
}

static void wma_get_song_info(char *filename, char **title_real, int *len_real)
{
    TitleInput *tuple = wma_get_song_tuple(filename);

    if (tuple == NULL)
        return;

    (*len_real) = tuple->length;
    (*title_real) = xmms_get_titlestring(xmms_get_gentitle_format(), tuple);
}

static void wma_playbuff(int out_size)
{
    FifoBuffer f;
    int sst_buff;
    
    fifo_init(&f, out_size*2);
    fifo_write(&f, wma_outbuf, out_size, &f.wptr);
    while(!fifo_read(&f, wma_s_outbuf, wma_st_buff, &f.rptr) && wma_decode)
    {
        sst_buff = wma_st_buff;
	if(wma_pause) memset(wma_s_outbuf, 0, sst_buff);	
    	while(wma_ip.output->buffer_free() < wma_st_buff) xmms_usleep(20000);
	produce_audio(wma_ip.output->written_time(), FMT_S16_NE,
    			    c->channels, sst_buff, (short *)wma_s_outbuf, NULL);
	memset(wma_s_outbuf, 0, sst_buff);
    }
    fifo_free(&f);
    return;
}

static void *wma_play_loop(void *arg)
{
    uint8_t *inbuf_ptr;
    int out_size, size, len;
    AVPacket pkt;
    
    g_static_mutex_lock(&wma_mutex);
    while(wma_decode){

	if(wma_seekpos != -1)
	{
	    av_seek_frame(ic, wma_idx, wma_seekpos * 1000000LL);
	    wma_ip.output->flush(wma_seekpos * 1000);
	    wma_seekpos = -1;
	}

        if(av_read_frame(ic, &pkt) < 0) break;

        size = pkt.size;
        inbuf_ptr = pkt.data;
	
        if(size == 0) break;
	
        while(size > 0){
            len = avcodec_decode_audio(c, (short *)wma_outbuf, &out_size,
                                       inbuf_ptr, size);
	    if(len < 0) break;
	    
            if(out_size <= 0) continue;

	    wma_playbuff(out_size);

            size -= len;
            inbuf_ptr += len;
            if(pkt.data) av_free_packet(&pkt);
        }
    }
    while(wma_decode && wma_ip.output->buffer_playing()) xmms_usleep(30000);
    wma_decode = 0;
    if(wma_s_outbuf) g_free(wma_s_outbuf);
    if(wma_outbuf) g_free(wma_outbuf);
    if(pkt.data) av_free_packet(&pkt);
    if(c) avcodec_close(c);
    if(ic) av_close_input_file(ic);
    g_static_mutex_unlock(&wma_mutex);
    g_thread_exit(NULL);
    return(NULL);
}

static void wma_play_file(char *filename) 
{
    AVCodec *codec;
    
    if(av_open_input_file(&ic, str_twenty_to_space(filename), NULL, 0, NULL) < 0) return;

    for(wma_idx = 0; wma_idx < ic->nb_streams; wma_idx++) {
        c = &ic->streams[wma_idx]->codec;
        if(c->codec_type == CODEC_TYPE_AUDIO) break;
    }

    av_find_stream_info(ic);

    codec = avcodec_find_decoder(c->codec_id);

    if(!codec) return;
	
    if(avcodec_open(c, codec) < 0) return;
	    	    
    wsong_title = get_song_title(ic, filename);
    wsong_time = get_song_time(ic);

    if(wma_ip.output->open_audio( FMT_S16_NE, c->sample_rate, c->channels) <= 0) return;

    wma_st_buff  = ST_BUFF;
	
    wma_ip.set_info(wsong_title, wsong_time, c->bit_rate, c->sample_rate, c->channels);

    wma_s_outbuf = memalign(16, wma_st_buff);
    wma_outbuf = memalign(16, AVCODEC_MAX_AUDIO_FRAME_SIZE);
    wma_seekpos = -1;
    wma_decode = 1;
    wma_decode_thread = g_thread_create((GThreadFunc)wma_play_loop, NULL, TRUE, NULL);
}

static void wma_stop(void) 
{
    wma_decode = 0;
    if(wma_pause) wma_do_pause(0);
    g_thread_join(wma_decode_thread);
    wma_ip.output->close_audio();
}	
