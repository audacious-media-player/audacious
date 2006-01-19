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
#include "libaudacious/configdb.h"
#include "libaudacious/util.h"
#include "libaudacious/titlestring.h"
#include "libaudacious/vfs.h"
#include "audacious/input.h"
#include "audacious/output.h"
#include "libaudcore/playback.h"

};

struct AudaciousConsoleConfig audcfg = { 180, FALSE, 32000 };

#include <cstring>
#include <stdio.h>

static Spc_Emu *spc = NULL;
static Nsf_Emu *nsf = NULL;
static Gbs_Emu *gbs = NULL;
static Gym_Emu *gym = NULL;
static Vgm_Emu *vgm = NULL;
static GThread *decode_thread;
GStaticMutex playback_mutex = G_STATIC_MUTEX_INIT;

static void *play_loop_spc(gpointer arg);
static void *play_loop_nsf(gpointer arg);
static void *play_loop_gbs(gpointer arg);
static void *play_loop_gym(gpointer arg);
static void *play_loop_vgm(gpointer arg);
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
	VFSFile *file;
	gchar magic[4];
	if (file = vfs_fopen(filename, "rb")) {
        	vfs_fread(magic, 1, 4, file);
		if (!strncmp(magic, "SNES", 4)) {
			vfs_fclose(file);
			return PLAY_TYPE_SPC;
		}
		if (!strncmp(magic, "NESM", 4)) {
			vfs_fclose(file);
			return PLAY_TYPE_NSF;
		}
		if (!strncmp(magic, "GYMX", 4)) {
			vfs_fclose(file);
			return PLAY_TYPE_GYM;
		}
		if (!strncmp(magic, "GBS", 3)) {
			vfs_fclose(file);
			return PLAY_TYPE_GBS;
		}
		if (!strncmp(magic, "Vgm", 3)) {
			vfs_fclose(file);
			return PLAY_TYPE_VGM;
		}
		vfs_fclose(file);
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

	if (header.version > 10)
	{
		TitleInput *tinput;

		tinput = bmp_title_input_new();

		tinput->performer = g_strdup(header.author);
		tinput->album_name = g_strdup(header.game);
		tinput->track_name = g_strdup(header.song);		

		tinput->file_name = g_path_get_basename(filename);
		tinput->file_path = g_path_get_dirname(filename);

		title = xmms_get_titlestring(xmms_get_gentitle_format(),
				tinput);

		g_free(tinput);
	}
	else
		title = g_path_get_basename(filename);

	return title;
}

static gchar *get_title_nsf(gchar *filename)
{
	gchar *title;
	title = g_path_get_basename(filename);
	return title;
}

static gchar *get_title_gbs(gchar *filename)
{
	gchar *title;
	Emu_Std_Reader reader;
	Gbs_Emu::header_t header;

	reader.open(filename);
	reader.read(&header, sizeof(header));

	if (header.game)
	{
		TitleInput *tinput;

		tinput = bmp_title_input_new();

		tinput->performer = g_strdup(header.author);
		tinput->album_name = g_strdup(header.copyright);
		tinput->track_name = g_strdup(header.game);		

		tinput->file_name = g_path_get_basename(filename);
		tinput->file_path = g_path_get_dirname(filename);

		title = xmms_get_titlestring(xmms_get_gentitle_format(),
				tinput);

		g_free(tinput);
	}
	else
		title = g_path_get_basename(filename);

	return title;
}

static gchar *get_title_gym(gchar *filename)
{
	gchar *title;
	Emu_Std_Reader reader;
	Gym_Emu::header_t header;

	reader.open(filename);
	reader.read(&header, sizeof(header));

	if (header.song)
	{
		TitleInput *tinput;

		tinput = bmp_title_input_new();

		tinput->performer = g_strdup(header.game);
		tinput->album_name = g_strdup(header.copyright);
		tinput->track_name = g_strdup(header.song);		

		tinput->file_name = g_path_get_basename(filename);
		tinput->file_path = g_path_get_dirname(filename);

		title = xmms_get_titlestring(xmms_get_gentitle_format(),
				tinput);

		g_free(tinput);
	}
	else
		title = g_path_get_basename(filename);

	return title;
}

static gchar *get_title_vgm(gchar *filename)
{
	gchar *title;
	title = g_path_get_basename(filename);
	return title;
}

static gchar *get_title(gchar *filename)
{
	switch (is_our_file(filename))
	{
		case PLAY_TYPE_SPC:
			return get_title_spc(filename);
			break;
		case PLAY_TYPE_NSF:
			return get_title_nsf(filename);
			break;
		case PLAY_TYPE_GBS:
			return get_title_gbs(filename);
			break;
		case PLAY_TYPE_GYM:
			return get_title_gym(filename);
			break;
		case PLAY_TYPE_VGM:
			return get_title_vgm(filename);
			break;
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

        if (!console_ip.output->open_audio(FMT_S16_NE, samplerate, 2))
                 return;

	playing_type = PLAY_TYPE_SPC;

	decode_thread = g_thread_create(play_loop_spc, spc, TRUE, NULL);
}

static void play_file_nsf(char *filename)
{
	gchar *name;
	Emu_Std_Reader reader;
	Nsf_Emu::header_t header;
	gint samplerate;

	if (audcfg.resample == TRUE)
		samplerate = audcfg.resample_rate;
	else
		samplerate = 44100;

	reader.open(filename);
	reader.read(&header, sizeof(header));

	nsf = new Nsf_Emu;
	nsf->init(samplerate);
	nsf->load(header, reader);
	nsf->start_track(0);

	console_ip_is_going = TRUE;

	name = get_title(filename);

	if (audcfg.loop_length)
		console_ip.set_info(name, audcfg.loop_length * 1000, 
			nsf->voice_count() * 1000, samplerate, 2);
	else
		console_ip.set_info(name, -1, nsf->voice_count() * 1000,
			samplerate, 2);

	g_free(name);

        if (!console_ip.output->open_audio(FMT_S16_NE, samplerate, 2))
                 return;

	playing_type = PLAY_TYPE_NSF;

	decode_thread = g_thread_create(play_loop_nsf, nsf, TRUE, NULL);
}

static void play_file_gbs(char *filename)
{
	gchar *name;
	Emu_Std_Reader reader;
	Gbs_Emu::header_t header;
	gint samplerate;

	if (audcfg.resample == TRUE)
		samplerate = audcfg.resample_rate;
	else
		samplerate = 44100;

	reader.open(filename);
	reader.read(&header, sizeof(header));

	gbs = new Gbs_Emu;
	gbs->init(samplerate);
	gbs->load(header, reader);
	gbs->start_track(0);

	console_ip_is_going = TRUE;

	name = get_title(filename);

	if (audcfg.loop_length)
		console_ip.set_info(name, audcfg.loop_length * 1000, 
			gbs->voice_count() * 1000, samplerate, 2);
	else
		console_ip.set_info(name, -1, gbs->voice_count() * 1000,
			samplerate, 2);

	g_free(name);

        if (!console_ip.output->open_audio(FMT_S16_NE, samplerate, 2))
                 return;

	playing_type = PLAY_TYPE_GBS;

	decode_thread = g_thread_create(play_loop_gbs, gbs, TRUE, NULL);
}

static void play_file_gym(char *filename)
{
	gchar *name;
	Emu_Std_Reader reader;
	Gym_Emu::header_t header;
	gint samplerate;

	if (audcfg.resample == TRUE)
		samplerate = audcfg.resample_rate;
	else
		samplerate = 44100;

	reader.open(filename);
	reader.read(&header, sizeof(header));

	gym = new Gym_Emu;
	gym->init(samplerate);
	gym->load(header, reader);
	gym->start_track(0);

	console_ip_is_going = TRUE;

	name = get_title(filename);

	if (gym->track_length() > 0)
		console_ip.set_info(name, gym->track_length() * 1000, 
			gym->voice_count() * 1000, samplerate, 2);
	else if (audcfg.loop_length)
		console_ip.set_info(name, audcfg.loop_length * 1000, 
			gym->voice_count() * 1000, samplerate, 2);
	else
		console_ip.set_info(name, -1, gym->voice_count() * 1000,
			samplerate, 2);

	g_free(name);

        if (!console_ip.output->open_audio(FMT_S16_NE, samplerate, 2))
                 return;

	playing_type = PLAY_TYPE_GYM;

	decode_thread = g_thread_create(play_loop_gym, gym, TRUE, NULL);
}

static void play_file_vgm(char *filename)
{
	gchar *name;
	Emu_Std_Reader reader;
	Vgm_Emu::header_t header;
	gint samplerate;

	if (audcfg.resample == TRUE)
		samplerate = audcfg.resample_rate;
	else
		samplerate = 44100;

	reader.open(filename);
	reader.read(&header, sizeof(header));

	vgm = new Vgm_Emu;
	vgm->init(samplerate);
	vgm->load(header, reader);
	vgm->start_track(0);

	console_ip_is_going = TRUE;

	name = get_title(filename);

	if (vgm->track_length() > 0)
		console_ip.set_info(name, vgm->track_length() * 1000, 
			vgm->voice_count() * 1000, samplerate, 2);
	else if (audcfg.loop_length)
		console_ip.set_info(name, audcfg.loop_length * 1000, 
			vgm->voice_count() * 1000, samplerate, 2);
	else
		console_ip.set_info(name, -1, vgm->voice_count() * 1000,
			samplerate, 2);

	g_free(name);

        if (!console_ip.output->open_audio(FMT_S16_NE, samplerate, 2))
                 return;

	playing_type = PLAY_TYPE_VGM;

	decode_thread = g_thread_create(play_loop_vgm, vgm, TRUE, NULL);
}

static void play_file(char *filename)
{
	switch (is_our_file(filename))
	{
		case PLAY_TYPE_SPC:
			play_file_spc(filename);
			break;
		case PLAY_TYPE_NSF:
			play_file_nsf(filename);
			break;
		case PLAY_TYPE_GBS:
			play_file_gbs(filename);
			break;
		case PLAY_TYPE_GYM:
			play_file_gym(filename);
			break;
		case PLAY_TYPE_VGM:
			play_file_vgm(filename);
			break;
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
	g_static_mutex_lock(&playback_mutex);
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
		produce_audio(console_ip.output->written_time(),
			FMT_S16_NE, 1, 2048, buf, NULL);
	        while(console_ip.output->buffer_free() < 2048)
			xmms_usleep(10000);
	}

        delete spc;
        console_ip.output->close_audio();
	console_ip_is_going = FALSE;
	playing_type = PLAY_TYPE_NONE;
	g_static_mutex_unlock(&playback_mutex);
        g_thread_exit(NULL);

        return NULL;
}

static void *play_loop_nsf(gpointer arg)
{
	g_static_mutex_lock(&playback_mutex);
	Nsf_Emu *my_nsf = (Nsf_Emu *) arg;
        Music_Emu::sample_t buf[1024];

	for (;;)
	{
		if (!console_ip_is_going)
			break;

		my_nsf->play(1024, buf);

		if ((console_ip.output->output_time() / 1000) > 
			audcfg.loop_length && audcfg.loop_length != 0)
			break;
		produce_audio(console_ip.output->written_time(),
			FMT_S16_NE, 1, 2048, buf, NULL);
	        while(console_ip.output->buffer_free() < 2048)
			xmms_usleep(10000);
	}

        delete nsf;
        console_ip.output->close_audio();
	console_ip_is_going = FALSE;
	playing_type = PLAY_TYPE_NONE;
	g_static_mutex_unlock(&playback_mutex);
        g_thread_exit(NULL);

        return NULL;
}

static void *play_loop_gbs(gpointer arg)
{
	g_static_mutex_lock(&playback_mutex);
	Gbs_Emu *my_gbs = (Gbs_Emu *) arg;
        Music_Emu::sample_t buf[1024];

	for (;;)
	{
		if (!console_ip_is_going)
			break;

		my_gbs->play(1024, buf);

		if ((console_ip.output->output_time() / 1000) > 
			audcfg.loop_length && audcfg.loop_length != 0)
			break;
		produce_audio(console_ip.output->written_time(),
			FMT_S16_NE, 1, 2048, buf, NULL);
	        while(console_ip.output->buffer_free() < 2048)
			xmms_usleep(10000);
	}

        delete gbs;
        console_ip.output->close_audio();
	console_ip_is_going = FALSE;
	playing_type = PLAY_TYPE_NONE;
	g_static_mutex_unlock(&playback_mutex);
        g_thread_exit(NULL);

        return NULL;
}

static void *play_loop_gym(gpointer arg)
{
	g_static_mutex_lock(&playback_mutex);
	Gym_Emu *my_gym = (Gym_Emu *) arg;
        Music_Emu::sample_t buf[1024];

	for (;;)
	{
		if (!console_ip_is_going)
			break;

		my_gym->play(1024, buf);

		if ((console_ip.output->output_time() / 1000) > 
			gym->track_length() && gym->track_length() != 0)
			break;
		if ((console_ip.output->output_time() / 1000) > 
			audcfg.loop_length && audcfg.loop_length != 0)
			break;
		produce_audio(console_ip.output->written_time(),
			FMT_S16_NE, 1, 2048, buf, NULL);
	        while(console_ip.output->buffer_free() < 2048)
			xmms_usleep(10000);
	}

        delete gym;
        console_ip.output->close_audio();
	console_ip_is_going = FALSE;
	playing_type = PLAY_TYPE_NONE;
	g_static_mutex_unlock(&playback_mutex);
        g_thread_exit(NULL);

        return NULL;
}

static void *play_loop_vgm(gpointer arg)
{
	g_static_mutex_lock(&playback_mutex);
	Vgm_Emu *my_vgm = (Vgm_Emu *) arg;
        Music_Emu::sample_t buf[1024];

	for (;;)
	{
		if (!console_ip_is_going)
			break;

		my_vgm->play(1024, buf);

		if ((console_ip.output->output_time() / 1000) > 
			vgm->track_length() && vgm->track_length() != 0)
			break;
		if ((console_ip.output->output_time() / 1000) > 
			audcfg.loop_length && audcfg.loop_length != 0)
			break;
		produce_audio(console_ip.output->written_time(),
			FMT_S16_NE, 1, 2048, buf, NULL);
	        while(console_ip.output->buffer_free() < 2048)
			xmms_usleep(10000);
	}

        delete vgm;
        console_ip.output->close_audio();
	console_ip_is_going = FALSE;
	playing_type = PLAY_TYPE_NONE;
	g_static_mutex_unlock(&playback_mutex);
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
	ConfigDb *db;

	db = bmp_cfg_db_open();

	bmp_cfg_db_get_int(db, "console", "loop_length", &audcfg.loop_length);
	bmp_cfg_db_get_bool(db, "console", "resample", &audcfg.resample);
	bmp_cfg_db_get_int(db, "console", "resample_rate", &audcfg.resample_rate);

	bmp_cfg_db_close(db);
}

extern "C" void console_aboutbox(void)
{
	xmms_show_message(_("About the Console Music Decoder"),
			_("Console music decoder engine based on Game_Music_Emu 0.2.4.\n"
			  "Audacious implementation by: William Pitcock <nenolod@nenolod.net>"),
			_("Ok"),
			FALSE, NULL, NULL);
}
