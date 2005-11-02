/*
 * Audacious: Cross platform multimedia player
 * Copyright (c) 2005  Audacious Team
 *
 * Driver for Game_Music_Emu library. See details at:
 * http://www.slack.net/~ant/libs/
 */

#include "Audacious_Driver.h"

#include "libaudacious/configfile.h"
#include "libaudacious/util.h"
#include "libaudacious/titlestring.h"
#include <gtk/gtk.h>

#include <cstring>

static Spc_Emu *spc = NULL;
static GThread decode_thread;

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
	TitleInput *input;
	gchar *title;
	Emu_Std_Reader reader;
	Spc_Emu::header_t header;

	reader.open(filename);
	reader.read(&header, sizeof(header));

	title = g_strdup(header.song);

	return title;
}
