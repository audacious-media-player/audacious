/*
 * Audacious: Cross platform multimedia player
 * Copyright (c) 2005  Audacious Team
 *
 * Driver for Game_Music_Emu library. See details at:
 * http://www.slack.net/~ant/libs/
 */

#include "Audacious_Driver.h"

extern "C" {

#include <glib.h>
#include <glib/gi18n.h>
#include <gtk/gtk.h>
#include "libaudacious/configfile.h"
#include "libaudacious/util.h"
#include "libaudacious/titlestring.h"
#include "audacious/input.h"
#include "audacious/output.h"
#include "audacious/playback.h"

};

struct AudaciousConsoleConfig audcfg = { 180, FALSE, 32000 };

#include <cstring>

#ifdef WORDS_BIGENDIAN
# define MY_FMT FMT_S16_BE
#else
# define MY_FMT FMT_S16_LE
#endif

static Spc_Emu *spc = NULL;
static GThread *decode_thread;

static void *play_loop_spc(gpointer arg);
static void console_init(void);
extern "C" void console_aboutbox(void);
static void console_stop(void);
static void console_pause(gshort p);
static int get_time(void);
static gboolean console_ip_is_going;
extern InputPlugin console_ip;
static int playing_type;

static int is_our_file(gchar *filename)
{
	gchar *ext;

	ext = strrchr(filename, '.');

	if (ext)
	{
		if (!strcasecmp(ext, ".spc"))
			return 1;
	}

	return 0;
}

static gchar *get_title_spc(gchar *filename)
{
	gchar *title;
	Emu_Std_Reader reader;
	Spc_Emu::header_t header;

	reader.open(filename);
	reader.read(&header, sizeof(header));

	if (header.song)
	{
		TitleInput *tinput;

		tinput = bmp_title_input_new();

		tinput->performer = g_strdup(header.game);
		tinput->album_name = g_strdup(header.game);
		tinput->track_name = g_strdup(header.song);		

		tinput->file_name = g_path_get_basename(filename);
		tinput->file_path = g_path_get_dirname(filename);

		title = xmms_get_titlestring(xmms_get_gentitle_format(),
				tinput);

		g_free(tinput);
	}
	else
		title = g_strdup(filename);

	return title;
}

static gchar *get_title(gchar *filename)
{
	gchar *ext;

	ext = strrchr(filename, '.');

	if (ext)
	{
		if (!strcasecmp(ext, ".spc"))
			return get_title_spc(filename);
	}

	return NULL;
}

static void get_song_info(char *filename, char **title, int *length)
{
	(*title) = get_title(filename);

	if (audcfg.loop_length)
		(*length) = audcfg.loop_length;
	else
		(*length) = -1;
}

static void play_file_spc(char *filename)
{
	gchar *name;
	Emu_Std_Reader reader;
	Spc_Emu::header_t header;
	gint samplerate;

	if (audcfg.resample == TRUE)
		samplerate = audcfg.resample_rate;
	else
		samplerate = Spc_Emu::native_sample_rate;

	reader.open(filename);
	reader.read(&header, sizeof(header));

	spc = new Spc_Emu;
	spc->init(samplerate);
	spc->load(header, reader);
	spc->start_track(0);

	console_ip_is_going = TRUE;

	name = get_title(filename);

	if (audcfg.loop_length)
		console_ip.set_info(name, audcfg.loop_length * 1000, 
			spc->voice_count() * 1000, samplerate, 2);
	else
		console_ip.set_info(name, -1, spc->voice_count() * 1000,
			samplerate, 2);

	g_free(name);

        if (!console_ip.output->open_audio(MY_FMT, samplerate, 2))
                 return;

	playing_type = PLAY_TYPE_SPC;

	decode_thread = g_thread_create(play_loop_spc, spc, TRUE, NULL);
}

static void play_file(char *filename)
{
	gchar *ext;

	ext = strrchr(filename, '.');

	if (ext)
	{
		if (!strcasecmp(ext, ".spc"))
			play_file_spc(filename);
	}
}

static void seek(gint time)
{
	// XXX: Not yet implemented
}

InputPlugin console_ip = {
	NULL,
	NULL,
	NULL,
	console_init,
	console_aboutbox,
	NULL,
	is_our_file,
	NULL,
	play_file,
	console_stop,
	console_pause,
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
	NULL,
	NULL
};

static void console_stop(void)
{
	console_ip_is_going = FALSE;
        g_thread_join(decode_thread);
        console_ip.output->close_audio();
}

extern "C" InputPlugin *get_iplugin_info(void)
{
        console_ip.description = g_strdup_printf(_("SPC, GYM, NSF, VGM and GBS module decoder"));
        return &console_ip;
}

static void console_pause(gshort p)
{
        console_ip.output->pause(p);
}

static void *play_loop_spc(gpointer arg)
{
	Spc_Emu *my_spc = (Spc_Emu *) arg;
        Music_Emu::sample_t buf[1024];

	for (;;)
	{
		if (!console_ip_is_going)
			break;

		my_spc->play(1024, buf);

		if ((console_ip.output->output_time() / 1000) > 
			audcfg.loop_length && audcfg.loop_length != 0)
			break;
		console_ip.add_vis_pcm(console_ip.output->written_time(),
			MY_FMT, 1, 2048, buf);
	        while(console_ip.output->buffer_free() < 2048)
			xmms_usleep(10000);
		console_ip.output->write_audio(buf, 2048);
	}

        delete spc;
        console_ip.output->close_audio();
	console_ip_is_going = FALSE;
	playing_type = PLAY_TYPE_NONE;
        g_thread_exit(NULL);

        return NULL;
}

static int get_time(void)
{
	if (console_ip_is_going == TRUE)
		return console_ip.output->output_time();
	else
		return -1;
}

static void console_init(void)
{

}

extern "C" void console_aboutbox(void)
{
	xmms_show_message(_("About the Console Music Decoder"),
			_("Console music decoder engine based on Game_Music_Emu 0.2.4.\n"
			  "Audacious implementation by: William Pitcock <nenolod@nenolod.net>"),
			_("Ok"),
			FALSE, NULL, NULL);
}
