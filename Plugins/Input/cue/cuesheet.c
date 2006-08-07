/* Audacious: An advanced media player.
 * cuesheet.c: Support cuesheets as a media container.
 *
 * Copyright (C) 2006 William Pitcock <nenolod -at- nenolod.net>.
 *
 * This file was hacked out of of xmms-cueinfo,
 * Copyright (C) 2003  Oskar Liljeblad
 *
 * This software is copyrighted work licensed under the terms of the
 * GNU General Public License. Please consult the file "COPYING" for
 * details.
 */
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <audacious/plugin.h>
#include <audacious/output.h>
#include <audacious/playlist.h>

#define MAX_CUE_LINE_LENGTH 1000
#define MAX_CUE_TRACKS 1000

static void cache_cue_file(gchar *f);
static void free_cue_info(void);
static void fix_cue_argument(char *line);
static gboolean is_our_file(gchar *filespec);
static void play(gchar *uri);
static void play_cue_uri(gchar *uri);
static gint get_time(void);
static void seek(gint time);
static void stop(void);
static TitleInput *get_tuple(gchar *uri);
static TitleInput *get_tuple_uri(gchar *uri);

static gint watchdog_func(gpointer unused);

static gchar *cue_performer = NULL;
static gchar *cue_title = NULL;
static gchar *cue_file = NULL;
static gint last_cue_track = 0;
static gint cur_cue_track = 0;
static struct {
	gchar *performer;
	gchar *title;
	gint index;
} cue_tracks[MAX_CUE_TRACKS];
static gint timeout_tag = 0;
static gint finetune_seek = 0;

static InputPlugin *real_ip = NULL;

InputPlugin cue_ip =
{
	NULL,			/* handle */
	NULL,			/* filename */
	NULL,			/* description */
	NULL,	       	/* init */
	NULL,	       	/* about */
	NULL,	  	   	/* configure */
	is_our_file,
	NULL,		/* audio cd */
	play,
	stop,
	NULL,
	seek,
	NULL,		/* set eq */
	get_time,
	NULL,
	NULL,
	NULL,		/* cleanup */
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,		/* XXX get_song_info iface */
	NULL,
	NULL,
	get_tuple,
	NULL
};

static gboolean is_our_file(gchar *filename)
{
	gchar *ext;
	gboolean ret = FALSE;
	
	/* is it a cue:// URI? */
	if (!strncasecmp(filename, "cue://", 6))
		return TRUE;

	ext = strrchr(filename, '.');

	if(!ext)
		return FALSE;
	
	if (!strncasecmp(ext, ".cue", 4))
	{
		gint i;
		ret = -1;

		/* add the files, build cue urls, etc. */
		cache_cue_file(filename);

		for (i = 0; i < last_cue_track; i++)
		{
			gchar _buf[65535];

			g_snprintf(_buf, 65535, "cue://%s?%d", filename, i);
			playlist_add_url(_buf);
		}

		free_cue_info();
	}

	return ret;
}

static gint get_time(void)
{
	return get_output_time();
}

static void play(gchar *uri)
{
	/* this isn't a cue:// uri? */
	if (strncasecmp("cue://", uri, 6))
	{
		gchar *tmp = g_strdup_printf("cue://%s?0", uri);
		play_cue_uri(tmp);
		g_free(tmp);
		return;
	}

	play_cue_uri(uri);
}

static TitleInput *get_tuple(gchar *uri)
{
	TitleInput *ret;

	/* this isn't a cue:// uri? */
	if (strncasecmp("cue://", uri, 6))
	{
		gchar *tmp = g_strdup_printf("cue://%s?0", uri);
		ret = get_tuple_uri(tmp);
		g_free(tmp);
		return ret;
	}

	return get_tuple_uri(uri);
}

static TitleInput *get_tuple_uri(gchar *uri)
{
        gchar *path2 = g_strdup(uri + 6);
        gchar *_path = strchr(path2, '?');
	gint track = 0;
	InputPlugin *dec;
	TitleInput *phys_tuple, *out;

        if (_path != NULL && *_path == '?')
        {
                *_path = '\0';
                _path++;
                track = atoi(_path);
        }	

	cache_cue_file(path2);

	dec = input_check_file(cue_file, FALSE);

	if (dec == NULL)
		return NULL;

	phys_tuple = dec->get_song_tuple(cue_file);

	out = bmp_title_input_new();

	out->genre = g_strdup(phys_tuple->genre);	
	out->album_name = g_strdup(phys_tuple->album_name);
	out->file_path = g_strdup(phys_tuple->file_path);	
	out->file_name = g_strdup(phys_tuple->file_name);
	out->file_ext = g_strdup(phys_tuple->file_ext);
	out->length = phys_tuple->length;

	bmp_title_input_free(phys_tuple);

	out->track_name = g_strdup(cue_tracks[track].title);
	out->performer = g_strdup(cue_tracks[track].performer);

	return out;
}

static void seek(gint time)
{
	if (real_ip != NULL)
		real_ip->seek(time);
}

static void stop(void)
{
	if (real_ip != NULL)
		real_ip->stop();

	gtk_timeout_remove(timeout_tag);
	free_cue_info();

	real_ip->set_info = cue_ip.set_info;
	real_ip->output = NULL;
	real_ip = NULL;
}

static void set_info_override(gchar * unused, gint length, gint rate, gint freq, gint nch)
{
	gchar *title;

	/* annoying. */
	if (playlist_position->tuple == NULL)
	{
		gint pos = playlist_get_position();
		playlist_get_tuple(pos);
	}

	title = g_strdup(playlist_position->title);

	cue_ip.set_info(title, length, rate, freq, nch);
}

static void play_cue_uri(gchar *uri)
{
        gchar *path2 = g_strdup(uri + 6);
        gchar *_path = strchr(path2, '?');
	gint track = 0;

        if (_path != NULL && *_path == '?')
        {
                *_path = '\0';
                _path++;
                track = atoi(_path);
        }	

	cache_cue_file(path2);

	real_ip = input_check_file(cue_file, FALSE);

	if (real_ip != NULL)
	{
		real_ip->set_info = set_info_override;
		real_ip->output = cue_ip.output;
		real_ip->play_file(cue_file);
		real_ip->seek(finetune_seek ? finetune_seek / 1000 : cue_tracks[track].index / 1000);
	}

	finetune_seek = 0;

	cur_cue_track = track;

	timeout_tag = gtk_timeout_add(100, watchdog_func, NULL);
}

InputPlugin *get_iplugin_info(void)
{
	cue_ip.description = g_strdup_printf("Cuesheet Container Plugin");
	return &cue_ip;
}

/******************************************************* watchdog */

/*
 * This is fairly hard to explain.
 *
 * Basically we loop until we have reached the correct track.
 * Then we set a finetune adjustment to make sure we stay in the
 * right place.
 *
 * I used to recurse here (it was prettier), but that didn't work
 * as well as I was hoping.
 *
 * Anyhow, yeah. The logic here isn't great, but it works, so I'm
 * cool with it.
 *
 *     - nenolod
 */
static gint watchdog_func(gpointer unused)
{
	gint time = get_output_time();
	gboolean dir = FALSE;

	while (time < cue_tracks[cur_cue_track].index)
	{
		cur_cue_track--;
		if (!(time < cue_tracks[cur_cue_track].index))
			finetune_seek = time;
		playlist_prev();
		dir = TRUE;
		time = get_output_time();
		g_usleep(10000);
	}

	while (dir == FALSE && cur_cue_track != last_cue_track && (time > cue_tracks[cur_cue_track + 1].index))
	{
		cur_cue_track++;
		if (!(time > cue_tracks[cur_cue_track].index))
			finetune_seek = time;
		playlist_next();
		time = get_output_time();
		g_usleep(10000);
	}

	return TRUE;
}

/******************************************************** cuefile */

static void free_cue_info(void)
{
	g_free(cue_performer);
	cue_performer = NULL;
	g_free(cue_title);
	cue_title = NULL;
	g_free(cue_file);
	cue_file = NULL;
	for (; last_cue_track > 0; last_cue_track--) {
		g_free(cue_tracks[last_cue_track-1].performer);
		g_free(cue_tracks[last_cue_track-1].title);
	}
}

static void cache_cue_file(char *f)
{
	FILE *file = fopen(f, "rb");
	gchar line[MAX_CUE_LINE_LENGTH+1];

	if(!file)
		return;
	
	while (TRUE) {
		gint p;
		gint q;

		if (fgets(line, MAX_CUE_LINE_LENGTH+1, file) == NULL)
			return;

		for (p = 0; line[p] && isspace(line[p]); p++);
		if (!line[p])
			continue;
		for (q = p; line[q] && !isspace(line[q]); q++);
		if (!line[q])
			continue;
		line[q] = '\0';
		for (q++; line[q] && isspace(line[q]); q++);

		if (strcasecmp(line+p, "PERFORMER") == 0) {
			fix_cue_argument(line+q);
			if (last_cue_track == 0) {
				if (!g_utf8_validate(line + q, -1, NULL)) {
					cue_performer = g_locale_to_utf8 (line + q, -1, NULL, NULL, NULL);
				} else
					cue_performer = g_strdup(line+q);
			} else {
				if (!g_utf8_validate(line + q, -1, NULL)) {
					cue_tracks[last_cue_track-1].performer = g_locale_to_utf8 (line + q, -1, NULL, NULL, NULL);
				} else
					cue_tracks[last_cue_track-1].performer = g_strdup(line+q);
			}
		}
		else if (strcasecmp(line+p, "FILE") == 0) {
			gchar *tmp = g_path_get_dirname(f);
			fix_cue_argument(line+q);
			cue_file = g_strdup_printf("%s/%s", tmp, line+q);	/* XXX: yaz might need to UTF validate this?? -nenolod */
			g_free(tmp);
		}
		else if (strcasecmp(line+p, "TITLE") == 0) {
			fix_cue_argument(line+q);
			if (last_cue_track == 0) {
				if (!g_utf8_validate(line + q, -1, NULL)) {
					cue_title = g_locale_to_utf8 (line + q, -1, NULL, NULL, NULL);
				} else
					cue_title = g_strdup(line+q);
			} else {
				if (!g_utf8_validate(line + q, -1, NULL)) {
					cue_tracks[last_cue_track-1].title = g_locale_to_utf8 (line + q, -1, NULL, NULL, NULL);
				} else
					cue_tracks[last_cue_track-1].title = g_strdup(line+q);
			}
		}
		else if (strcasecmp(line+p, "TRACK") == 0) {
			gint track;

			fix_cue_argument(line+q);
			for (p = q; line[p] && isdigit(line[p]); p++);
			line[p] = '\0';
			for (; line[q] && line[q] == '0'; q++);
			if (!line[q])
				continue;
			track = atoi(line+q);
			if (track >= MAX_CUE_TRACKS)
				continue;
			last_cue_track = track;
			cue_tracks[last_cue_track-1].index = 0;
			cue_tracks[last_cue_track-1].performer = NULL;
			cue_tracks[last_cue_track-1].title = NULL;
		}
		else if (strcasecmp(line+p, "INDEX") == 0) {
			for (p = q; line[p] && !isspace(line[p]); p++);
			if (!line[p])
				continue;
			for (p++; line[p] && isspace(line[p]); p++);
			for (q = p; line[q] && !isspace(line[q]); q++);
			if (q-p >= 8 && line[p+2] == ':' && line[p+5] == ':') {
				cue_tracks[last_cue_track-1].index =
						((line[p+0]-'0')*10 + (line[p+1]-'0')) * 60000 +
						((line[p+3]-'0')*10 + (line[p+4]-'0')) * 1000 +
						((line[p+6]-'0')*10 + (line[p+7]-'0')) * 10;
			}
		}
	}

	fclose(file);
}

static void fix_cue_argument(char *line)
{
	if (line[0] == '"') {
		gchar *l2;
		for (l2 = line+1; *l2 && *l2 != '"'; l2++)
				*(l2-1) = *l2;
			*(l2-1) = *l2;
		for (; *line && *line != '"'; line++) {
			if (*line == '\\' && *(line+1)) {
				for (l2 = line+1; *l2 && *l2 != '"'; l2++)
					*(l2-1) = *l2;
				*(l2-1) = *l2;
			}
		}
		*line = '\0';
	}
	else {
		for (; *line && *line != '\r' && *line != '\n'; line++);
		*line = '\0';
	}
}
