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
#include <libaudacious/beepctrl.h>

#define MAX_CUE_LINE_LENGTH 1000
#define MAX_CUE_TRACKS 1000

#define EMPTIZE(x) ((x)==NULL ? "":(x))

static void init(void);
static void cache_cue_file(FILE *file);
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

static gchar *cue_performer = NULL;
static gchar *cue_title = NULL;
static gchar *cue_file = NULL;
static gint last_cue_track = 0;
static gint cur_cue_track = 0;
static gint entry_lock = 0;
static struct {
	gchar *performer;
	gchar *title;
	gint index;
} cue_tracks[MAX_CUE_TRACKS];
static gint previous_song = -1;
static gint previous_length = -2;
static gint timeout_tag = 0;

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

	if (!strncasecmp(ext, ".cue", 4) && ext + 5 == '\0')
	{
		gint i;
		FILE *f = fopen(filename, "rb");
		ret = TRUE;

		/* add the files, build cue urls, etc. */
		cache_cue_file(f);

		for (i = 1; i < last_cue_track; i++)
		{
			gchar _buf[65535];

			g_snprintf(_buf, 65535, "cue://%s?%d", filename, i);
			playlist_add_url(_buf);
		}

		fclose(f);
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
	FILE *f;
	InputPlugin *dec;
	TitleInput *phys_tuple, *out;

        if (_path != NULL && *_path == '?')
        {
                *_path = '\0';
                _path++;
                track = atoi(_path);
        }	

	f = fopen(path2, "rb");
	cache_cue_file(f);
	fclose(f);

	dec = input_check_file(cue_file, FALSE);

	if (dec == NULL)
		return NULL;

	phys_tuple = dec->get_song_tuple(path2);

	out = bmp_title_input_new();

	out->genre = g_strdup(phys_tuple->genre);	
	out->album_name = g_strdup(phys_tuple->album_name);
	out->length = phys_tuple->length;

	bmp_title_input_free(phys_tuple);

	out->track_name = cue_tracks[track].title;
	out->performer = cue_tracks[track].performer;

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

	free_cue_info();
}

static void play_cue_uri(gchar *uri)
{
        gchar *path2 = g_strdup(uri + 6);
        gchar *_path = strchr(path2, '?');
	gint track = 0;
	FILE *f;

        if (_path != NULL && *_path == '?')
        {
                *_path = '\0';
                _path++;
                track = atoi(_path);
        }	

	f = fopen(path2, "rb");
	cache_cue_file(f);
	fclose(f);

	real_ip = input_check_file(cue_file, FALSE);

	if (real_ip != NULL)
	{
		real_ip->output = cue_ip.output;
		real_ip->play_file(cue_file);
		real_ip->seek(cue_tracks[track].index / 1000);	/* XXX: seek doesn't use frames? strange... -nenolod */
	}

	cur_cue_track = track;
}

InputPlugin *get_iplugin_info(void)
{
	cue_ip.description = g_strdup_printf("Cuesheet Container Plugin");
	return &cue_ip;
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

static void cache_cue_file(FILE *file)
{
	gchar line[MAX_CUE_LINE_LENGTH+1];

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
			fix_cue_argument(line+q);
			cue_file = g_strdup(line+q);		/* XXX: yaz might need to UTF validate this?? -nenolod */
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
