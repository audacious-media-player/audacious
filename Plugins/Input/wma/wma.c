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
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */
#include <stdlib.h>
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

static GtkWidget *dialog;
static GtkWidget *dialog1, *button1, *label1;

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
static void wma_file_info_box(char *filename); 
static char *wsong_title;
static int wsong_time;

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
    wma_file_info_box,  // Bring up an info window for the filename passed in
    NULL            	// Handle to the current output plugin. Filled in by xmms
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
    gtk_signal_connect(GTK_OBJECT(dialog1), "destroy",
                        GTK_SIGNAL_FUNC(gtk_widget_destroyed), &dialog1);
    gtk_window_set_title(GTK_WINDOW(dialog1), title);
    gtk_window_set_policy(GTK_WINDOW(dialog1), FALSE, FALSE, FALSE);
    gtk_container_border_width(GTK_CONTAINER(dialog1), 5);
    label1 = gtk_label_new(message);
    gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog1)->vbox), label1, TRUE, TRUE, 0);
    gtk_widget_show(label1);

    button1 = gtk_button_new_with_label(_(" Close "));
    gtk_signal_connect_object(GTK_OBJECT(button1), "clicked",
	                        GTK_SIGNAL_FUNC(gtk_widget_destroy),
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
    if(str && strlen(str) > 0) return str;
    return NULL;
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
    AVFormatContext *in = NULL;
    
    (*len_real) = -1;
    (*title_real) = NULL;

    if (av_open_input_file(&in, str_twenty_to_space(filename), NULL, 0, NULL) < 0)
	return;
	
    av_find_stream_info(in);
    (*len_real) = get_song_time(in);
    (*title_real) = get_song_title(in, filename);
    av_close_input_file(in);
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

    wma_s_outbuf = g_malloc0(wma_st_buff);
    wma_outbuf = g_malloc0(AVCODEC_MAX_AUDIO_FRAME_SIZE);
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

static void wma_file_info_box (char *filename) 
{
    GtkWidget *dialog_vbox1;
    GtkWidget *vbox1;
    GtkWidget *hbox1;
    GtkWidget *label_name;
    GtkWidget *entry_filename;
    GtkWidget *hbox2;
    GtkWidget *frame_wma_info;
    GtkWidget *alignment1;
    GtkWidget *table1;
    GtkWidget *label_album;
    GtkWidget *label_year;
    GtkWidget *label_track;
    GtkWidget *label_genre;
    GtkWidget *label_comments;
    GtkWidget *label_wma_version;
    GtkWidget *label_bitrate;
    GtkWidget *label_rate;
    GtkWidget *label_chans;
    GtkWidget *label_play_time;
    GtkWidget *label_filesize;
    GtkWidget *label_wma_vers_val;
    GtkWidget *label_bitrate_val;
    GtkWidget *label_rate_val;
    GtkWidget *label_chans_val;
    GtkWidget *label_playtime_val;
    GtkWidget *label_filesize_val;
    GtkWidget *label4;
    GtkWidget *frame_tags;
    GtkWidget *alignment2;
    GtkWidget *table2;
    GtkWidget *label_artist;
    GtkWidget *label_title;
    GtkWidget *entry_artist;
    GtkWidget *entry_album;
    GtkWidget *entry_year;
    GtkWidget *entry_title;
    GtkWidget *entry_track;
    GtkWidget *entry_genre;
    GtkWidget *entry_comments;
    GtkWidget *label5;
    GtkWidget *dialog_action_area1;
    GtkWidget *okbutton;

    AVFormatContext *in = NULL;
    AVCodecContext *s = NULL;
    AVCodec *codec;
    gint tns, thh, tmm, tss;
    gint i;
    gchar *title,
          *channels,
          *bitrate,
          *playtime,
          *samplerate,
          *filesize;
    VFSFile *f;
    if (dialog) {
        (void)printf(_("Info dialog is already opened!\n"));
        return;
    }

    if(av_open_input_file(&in, filename, NULL, 0, NULL) < 0)
        return;

    for(i = 0; i < in->nb_streams; i++) {
        s = &in->streams[i]->codec;
        if(s->codec_type == CODEC_TYPE_AUDIO)
            break;
    }

    av_find_stream_info(in);
    codec = avcodec_find_decoder(s->codec_id);

    /* window title */
    title = g_strdup_printf(_("File Info - %s"), g_basename(filename));
    /* channels */
    if (s->channels == 1)
        channels = g_strdup("MONO");
    else
        channels = g_strdup("STEREO");

    /* bitrate */
    bitrate = g_strdup_printf(_("%d Kb/s"), (s->bit_rate / 1000));

    /* playtime */
    if (in->duration != 0) {
        tns = in->duration/1000000LL;
        thh = tns/3600;
        tmm = (tns%3600)/60;
        tss = (tns%60);
        playtime = g_strdup_printf(_("%02d:%02d:%02d"), thh, tmm, tss);
    } else
        playtime = g_strdup("N/A");

    /* samplerate */
    samplerate = g_strdup_printf(_("%d Hz"), s->sample_rate);

    /* filesize */
    f = vfs_fopen(filename, "rb");

    if (f == NULL)
        return;

    vfs_fseek(f, 0, SEEK_END);
    filesize = g_strdup_printf(_("%lu Bytes"), vfs_ftell(f));
    vfs_fclose(f);

    dialog = gtk_dialog_new();

    gtk_signal_connect(GTK_OBJECT(dialog), "destroy",
        GTK_SIGNAL_FUNC(gtk_widget_destroyed), &dialog);

    gtk_window_set_title(GTK_WINDOW(dialog), title);
    gtk_window_set_type_hint(GTK_WINDOW(dialog), GDK_WINDOW_TYPE_HINT_DIALOG);
    gtk_window_set_policy(GTK_WINDOW(dialog), FALSE, FALSE, FALSE);

    dialog_vbox1 = GTK_DIALOG(dialog)->vbox;
    gtk_widget_show(dialog_vbox1);

    vbox1 = gtk_vbox_new(FALSE, 0);
    gtk_widget_show(vbox1);
    gtk_box_pack_start(GTK_BOX(dialog_vbox1), vbox1, TRUE, TRUE, 0);

    hbox1 = gtk_hbox_new (FALSE, 0);
    gtk_widget_show (hbox1);
    gtk_box_pack_start(GTK_BOX(vbox1), hbox1, FALSE, FALSE, 0);

    label_name = gtk_label_new(_("<b>Name:</b>"));
    gtk_widget_show(label_name);
    gtk_box_pack_start(GTK_BOX (hbox1), label_name, FALSE, FALSE, 0);
    gtk_misc_set_alignment(GTK_MISC (label_name), 0.48, 0.51);
    gtk_misc_set_padding(GTK_MISC (label_name), 10, 10);
    gtk_label_set_use_markup(GTK_LABEL(label_name), TRUE);

    entry_filename = gtk_entry_new();
    gtk_widget_show(entry_filename);
    gtk_box_pack_start(GTK_BOX(hbox1), entry_filename, TRUE, TRUE, 4);
    gtk_editable_set_editable(GTK_EDITABLE(entry_filename), FALSE);
    gtk_entry_set_text(GTK_ENTRY(entry_filename), filename);

    hbox2 = gtk_hbox_new(FALSE, 0);
    gtk_widget_show(hbox2);
    gtk_box_pack_start(GTK_BOX(vbox1), hbox2, TRUE, TRUE, 0);

    frame_wma_info = gtk_frame_new(NULL);
    gtk_widget_show(frame_wma_info);
    gtk_box_pack_start(GTK_BOX(hbox2), frame_wma_info, TRUE, TRUE, 0);
    gtk_frame_set_shadow_type(GTK_FRAME (frame_wma_info), GTK_SHADOW_ETCHED_IN);
    gtk_container_set_border_width (GTK_CONTAINER(frame_wma_info), 10);

    alignment1 = gtk_alignment_new(0.5, 0.5, 1, 1);
    gtk_widget_show(alignment1);
    gtk_container_add(GTK_CONTAINER(frame_wma_info), alignment1);
    gtk_alignment_set_padding(GTK_ALIGNMENT(alignment1), 0, 0, 0, 0);
    gtk_container_set_border_width(GTK_CONTAINER(alignment1), 2);

    table1 = gtk_table_new(6, 2, FALSE);
    gtk_widget_show(table1);
    gtk_container_add(GTK_CONTAINER(alignment1), table1);
    gtk_container_set_border_width(GTK_CONTAINER(table1), 6);
    gtk_table_set_row_spacings(GTK_TABLE(table1), 3);
    gtk_table_set_col_spacings(GTK_TABLE(table1), 8);
    /* WMA Version label */
    label_wma_version = gtk_label_new(_("<b>WMA Version:</b>"));
    gtk_widget_show(label_wma_version);
    gtk_table_attach(GTK_TABLE(table1), label_wma_version, 0, 1, 0, 1,
            (GtkAttachOptions) (GTK_FILL),
            (GtkAttachOptions) (0), 0, 0);
    gtk_misc_set_alignment(GTK_MISC(label_wma_version), 0, 0.5);
    gtk_label_set_use_markup(GTK_LABEL(label_wma_version), TRUE);

    /* Bitrate */
    label_bitrate = gtk_label_new(_("<b>Bitrate:</b>"));
    gtk_widget_show(label_bitrate);
    gtk_table_attach(GTK_TABLE(table1), label_bitrate, 0, 1, 1, 2,
            (GtkAttachOptions) (GTK_FILL),
            (GtkAttachOptions) (0), 0, 0);
    gtk_misc_set_alignment(GTK_MISC(label_bitrate), 0, 0.5);
    gtk_label_set_use_markup(GTK_LABEL(label_bitrate), TRUE);

       /* Samplerate */
    label_rate = gtk_label_new(_("<b>Samplerate:</b>"));
    gtk_widget_show(label_rate);
    gtk_table_attach(GTK_TABLE(table1), label_rate, 0, 1, 2, 3,
            (GtkAttachOptions) (GTK_FILL),
            (GtkAttachOptions) (0), 0, 0);
    gtk_misc_set_alignment(GTK_MISC(label_rate), 0, 0.5);
    gtk_label_set_use_markup(GTK_LABEL(label_rate), TRUE);

    /* Channels */
    label_chans = gtk_label_new(_("<b>Channels:</b>"));
    gtk_widget_show(label_chans);
    gtk_table_attach(GTK_TABLE (table1), label_chans, 0, 1, 3, 4,
            (GtkAttachOptions) (GTK_FILL),
            (GtkAttachOptions) (0), 0, 0);
    gtk_misc_set_alignment(GTK_MISC(label_chans), 0, 0.5);
    gtk_label_set_use_markup(GTK_LABEL(label_chans), TRUE);

    /* Play time */
    label_play_time = gtk_label_new(_("<b>Play time:</b>"));
    gtk_widget_show(label_play_time);
    gtk_table_attach(GTK_TABLE (table1), label_play_time, 0, 1, 4, 5,
            (GtkAttachOptions) (GTK_FILL),
            (GtkAttachOptions) (0), 0, 0);
    gtk_misc_set_alignment(GTK_MISC(label_play_time), 0, 0.5);
    gtk_label_set_use_markup(GTK_LABEL(label_play_time), TRUE);

       /* Filesize */
    label_filesize = gtk_label_new(_("<b>Filesize:</b>"));
    gtk_widget_show(label_filesize);
    gtk_table_attach(GTK_TABLE(table1), label_filesize, 0, 1, 5, 6,
            (GtkAttachOptions) (GTK_FILL),
            (GtkAttachOptions) (0), 0, 0);
    gtk_misc_set_alignment(GTK_MISC(label_filesize), 0, 0.5);
    gtk_label_set_use_markup(GTK_LABEL(label_filesize), TRUE);


    label_wma_vers_val = gtk_label_new(codec->name);
    gtk_widget_show(label_wma_vers_val);
    gtk_table_attach(GTK_TABLE(table1), label_wma_vers_val, 1, 2, 0, 1,
            (GtkAttachOptions)(GTK_FILL),
            (GtkAttachOptions) (0), 0, 0);
    gtk_misc_set_alignment(GTK_MISC(label_wma_vers_val), 0, 0.5);

    label_bitrate_val = gtk_label_new(bitrate);
    gtk_widget_show(label_bitrate_val);
    gtk_table_attach(GTK_TABLE(table1), label_bitrate_val, 1, 2, 1, 2,
            (GtkAttachOptions)(GTK_FILL),
            (GtkAttachOptions) (0), 0, 0);
    gtk_misc_set_alignment(GTK_MISC(label_bitrate_val), 0, 0.5);

    label_rate_val = gtk_label_new(samplerate);
    gtk_widget_show(label_rate_val);
    gtk_table_attach(GTK_TABLE(table1), label_rate_val, 1, 2, 2, 3,
            (GtkAttachOptions)(GTK_FILL),
            (GtkAttachOptions) (0), 0, 0);
    gtk_misc_set_alignment(GTK_MISC(label_rate_val), 0, 0.5);

    label_chans_val = gtk_label_new(channels);
    gtk_widget_show(label_chans_val);
    gtk_table_attach(GTK_TABLE(table1), label_chans_val, 1, 2, 3, 4,
            (GtkAttachOptions)(GTK_FILL),
            (GtkAttachOptions) (0), 0, 0);
    gtk_misc_set_alignment(GTK_MISC (label_chans_val), 0, 0.5);

    label_playtime_val = gtk_label_new(playtime);
    gtk_widget_show(label_playtime_val);
    gtk_table_attach(GTK_TABLE(table1), label_playtime_val, 1, 2, 4, 5,
            (GtkAttachOptions)(GTK_FILL),
            (GtkAttachOptions) (0), 0, 0);
    gtk_misc_set_alignment(GTK_MISC(label_playtime_val), 0, 0.5);

    label_filesize_val = gtk_label_new(filesize);
    gtk_widget_show(label_filesize_val);
    gtk_table_attach(GTK_TABLE (table1), label_filesize_val, 1, 2, 5, 6,
            (GtkAttachOptions)(GTK_FILL),
            (GtkAttachOptions) (0), 0, 0);
    gtk_misc_set_alignment(GTK_MISC(label_filesize_val), 0, 0.5);

    label4 = gtk_label_new (_("WMA Info"));
    gtk_widget_show(label4);
    gtk_frame_set_label_widget(GTK_FRAME(frame_wma_info), label4);
    frame_tags = gtk_frame_new (NULL);
    gtk_widget_show (frame_tags);
    gtk_box_pack_start (GTK_BOX (hbox2), frame_tags, TRUE, TRUE, 0);
    gtk_frame_set_shadow_type (GTK_FRAME (frame_tags), GTK_SHADOW_ETCHED_IN);
    gtk_container_set_border_width (GTK_CONTAINER (frame_tags), 10);


    alignment2 = gtk_alignment_new (0.5, 0.5, 1, 1);
    gtk_widget_show (alignment2);
    gtk_container_add (GTK_CONTAINER (frame_tags), alignment2);
    gtk_alignment_set_padding (GTK_ALIGNMENT (alignment2), 0, 0, 12, 0);
    gtk_container_set_border_width (GTK_CONTAINER (alignment2), 2);


    table2 = gtk_table_new(8, 2, FALSE);
    gtk_widget_show(table2);
    gtk_container_add(GTK_CONTAINER(alignment2), table2);
    gtk_container_set_border_width(GTK_CONTAINER(table2), 6);
    gtk_table_set_row_spacings(GTK_TABLE(table2), 3);
    gtk_table_set_col_spacings(GTK_TABLE(table2), 8);

    /* Artist */
    label_artist = gtk_label_new(_("<b>Artist:</b>"));
    gtk_widget_show(label_artist);
    gtk_table_attach(GTK_TABLE (table2), label_artist, 0, 1, 0, 1,
            (GtkAttachOptions) (GTK_FILL),
            (GtkAttachOptions) (0), 0, 0);
    gtk_misc_set_alignment(GTK_MISC(label_artist), 0, 0.5);
    gtk_label_set_use_markup(GTK_LABEL(label_artist), TRUE);

    /* Title */
    label_title = gtk_label_new(_("<b>Title:</b>"));
    gtk_widget_show(label_title);
    gtk_table_attach(GTK_TABLE (table2), label_title, 0, 1, 1, 2,
            (GtkAttachOptions) (GTK_FILL),
            (GtkAttachOptions) (0), 0, 0);
    gtk_misc_set_alignment(GTK_MISC(label_title), 0, 0.5);
    gtk_label_set_use_markup(GTK_LABEL(label_title), TRUE);

    /* Album */
    label_album = gtk_label_new(_("<b>Album:</b>"));
    gtk_widget_show(label_album);
    gtk_table_attach(GTK_TABLE (table2), label_album, 0, 1, 2, 3,
            (GtkAttachOptions) (GTK_FILL),
            (GtkAttachOptions) (0), 0, 0);
    gtk_misc_set_alignment(GTK_MISC(label_album), 0, 0.5);
    gtk_label_set_use_markup(GTK_LABEL(label_album), TRUE);

    /* Comments */
    label_comments = gtk_label_new(_("<b>Comments:</b>"));
    gtk_widget_show(label_comments);
    gtk_table_attach(GTK_TABLE(table2), label_comments, 0, 1, 3, 4,
            (GtkAttachOptions) (GTK_FILL),
            (GtkAttachOptions) (0), 0, 0);
    gtk_misc_set_alignment(GTK_MISC(label_comments), 0, 0.5);
    gtk_label_set_use_markup(GTK_LABEL(label_comments), TRUE);

    /* Year */
    label_year = gtk_label_new(_("<b>Year:</b>"));
    gtk_widget_show(label_year);
    gtk_table_attach(GTK_TABLE (table2), label_year, 0, 1, 4, 5,
            (GtkAttachOptions) (GTK_FILL),
            (GtkAttachOptions) (0), 0, 0);
    gtk_misc_set_alignment(GTK_MISC(label_year), 0, 0.5);
    gtk_label_set_use_markup(GTK_LABEL(label_year), TRUE);

    /* Track */
    label_track = gtk_label_new(_("<b>Track:</b>"));
    gtk_widget_show(label_track);
    gtk_table_attach(GTK_TABLE (table2), label_track, 0, 1, 5, 6,
            (GtkAttachOptions) (GTK_FILL),
            (GtkAttachOptions) (0), 0, 0);
    gtk_misc_set_alignment(GTK_MISC(label_track), 0, 0.5);
    gtk_label_set_use_markup(GTK_LABEL(label_track), TRUE);

    /* Genre */
    label_genre = gtk_label_new(_("<b>Genre:</b>"));
    gtk_widget_show(label_genre);
    gtk_table_attach(GTK_TABLE (table2), label_genre, 0, 1, 6, 7,
            (GtkAttachOptions) (GTK_FILL),
            (GtkAttachOptions) (0), 0, 0);
    gtk_misc_set_alignment(GTK_MISC (label_genre), 0, 0.5);
    gtk_label_set_use_markup(GTK_LABEL(label_genre), TRUE);


    entry_artist = gtk_entry_new();
    gtk_widget_show (entry_artist);
    gtk_table_attach (GTK_TABLE (table2), entry_artist, 1, 2, 0, 1,
            (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
            (GtkAttachOptions) (0), 0, 0);
    gtk_editable_set_editable (GTK_EDITABLE (entry_artist), FALSE);
    gtk_entry_set_text(GTK_ENTRY(entry_artist), in->author);

    entry_title = gtk_entry_new();
    gtk_widget_show(entry_title);
    gtk_table_attach (GTK_TABLE (table2), entry_title, 1, 2, 1, 2,
            (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
            (GtkAttachOptions) (0), 0, 0);
    gtk_editable_set_editable(GTK_EDITABLE (entry_title), FALSE);
    gtk_entry_set_text(GTK_ENTRY(entry_title), in->title);

    entry_album = gtk_entry_new();
    gtk_widget_show(entry_album);
    gtk_table_attach(GTK_TABLE (table2), entry_album, 1, 2, 2, 3,
            (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
            (GtkAttachOptions) (0), 0, 0);
    gtk_editable_set_editable(GTK_EDITABLE (entry_album), FALSE);
    gtk_entry_set_text(GTK_ENTRY(entry_album), in->album);

    entry_comments = gtk_entry_new();
    gtk_widget_show(entry_comments);
    gtk_table_attach(GTK_TABLE (table2), entry_comments, 1, 2, 3, 4,
            (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
            (GtkAttachOptions) (0), 0, 0);
    gtk_editable_set_editable(GTK_EDITABLE (entry_comments), FALSE);
    gtk_entry_set_text(GTK_ENTRY(entry_comments), in->comment);

    entry_year = gtk_entry_new();
    gtk_widget_show(entry_year);
    gtk_table_attach(GTK_TABLE (table2), entry_year, 1, 2, 4, 5,
            (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
            (GtkAttachOptions) (0), 0, 0);
    gtk_editable_set_editable(GTK_EDITABLE (entry_year), FALSE);
    gtk_entry_set_text(GTK_ENTRY(entry_year), g_strdup_printf("%d", in->year));

    entry_track = gtk_entry_new();
    gtk_widget_show(entry_track);
    gtk_table_attach(GTK_TABLE (table2), entry_track, 1, 2, 5, 6,
            (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
            (GtkAttachOptions) (0), 0, 0);
    gtk_editable_set_editable(GTK_EDITABLE (entry_track), FALSE);
    gtk_entry_set_text(GTK_ENTRY(entry_track), g_strdup_printf("%d", in->track));

    entry_genre = gtk_entry_new();
    gtk_widget_show(entry_genre);
    gtk_table_attach(GTK_TABLE (table2), entry_genre, 1, 2, 6, 7,
            (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
            (GtkAttachOptions) (0), 0, 0);
    gtk_editable_set_editable(GTK_EDITABLE (entry_genre), FALSE);
    gtk_entry_set_text(GTK_ENTRY(entry_genre), in->genre);


    label5 = gtk_label_new(_("Tags"));
    gtk_widget_show(label5);
    gtk_frame_set_label_widget(GTK_FRAME(frame_tags), label5);


    dialog_action_area1 = GTK_DIALOG(dialog)->action_area;
    gtk_widget_show(dialog_action_area1);
    gtk_button_box_set_layout(GTK_BUTTON_BOX(dialog_action_area1), GTK_BUTTONBOX_END);

    okbutton = gtk_button_new_from_stock("gtk-ok");
    gtk_widget_show(okbutton);
    gtk_dialog_add_action_widget(GTK_DIALOG(dialog), okbutton, GTK_RESPONSE_OK);
    GTK_WIDGET_SET_FLAGS(okbutton, GTK_CAN_DEFAULT);

    gtk_signal_connect_object(GTK_OBJECT(okbutton), "clicked",
            GTK_SIGNAL_FUNC(gtk_widget_destroy), GTK_OBJECT(dialog));

    gtk_widget_show(dialog);

    g_free(title);
    g_free(channels);
    g_free(bitrate);
    g_free(playtime);
    g_free(samplerate);
    g_free(filesize);
}
