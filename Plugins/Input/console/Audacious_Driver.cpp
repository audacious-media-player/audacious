/*
 * Audacious: Cross platform multimedia player
 * Copyright (c) 2005  Audacious Team
 *
 * Driver for Game_Music_Emu library. See details at:
 * http://www.slack.net/~ant/libs/
 */

#include "Audacious_Driver.h"

#include <glib/gi18n.h>
#include "libaudacious/configfile.h"
#include "libaudacious/util.h"
#include "libaudacious/titlestring.h"
#include "audacious/output.h"
#include <gtk/gtk.h>

#include <cstring>

static Spc_Emu *spc = NULL;
static GThread *decode_thread;

static void *play_loop(gpointer arg);
static void console_stop(void);
static void console_pause(gshort p);
static int get_time(void);

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

static gchar *get_title(gchar *filename)
{
	gchar *title;
	Emu_Std_Reader reader;
	Spc_Emu::header_t header;

	reader.open(filename);
	reader.read(&header, sizeof(header));

	title = g_strdup(header.song);

	return title;
}

static void get_song_info(char *filename, char **title, int *length)
{
	(*title) = get_title(filename);
	(*length) = -1;
}

static void play_file(char *filename)
{
	Emu_Std_Reader reader;
	Spc_Emu::header_t header;

	reader.open(filename);
	reader.read(&header, sizeof(header));

	spc = new Spc_Emu;
	spc->init(44100);
	spc->load(header, reader);
	spc->start_track(0);

	decode_thread = g_thread_create(play_loop, NULL, FALSE, NULL);
}

static void seek(gint time)
{
	// XXX: Not yet implemented
}

static void console_init(void)
{
	// nothing to do here
}

InputPlugin console_ip = {
	NULL,
	NULL,
	NULL,
	console_init,
	NULL,
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
	NULL,
	NULL,
	NULL
};

static void console_stop(void)
{
        g_thread_join(decode_thread);
        console_ip.output->close_audio();
}

InputPlugin *get_iplugin_info(void)
{
        console_ip.description = g_strdup_printf(_("Console music plugin"));
        return &console_ip;
}

static void console_pause(gshort p)
{
        console_ip.output->pause(p);
}

static void *play_loop(gpointer arg)
{
        Music_Emu::sample_t buf[1024];

        while (spc->play(1024, buf))
        {
                produce_audio(console_ip.output->written_time(),
                                FMT_S16_LE, 2, 1024, (char *) buf,
                                NULL);
                xmms_usleep(100000);
        }

        delete spc;
        g_thread_exit(NULL);

        return NULL;
}

static int get_time(void)
{
        return console_ip.output->output_time();
}

