/*
 * Audtool2
 * Copyright (c) 2007 Audacious development team
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <stdlib.h>
#include <string.h>
#include <glib.h>
#include <mowgli.h>
#include <locale.h>
#include "libaudclient/audctrl.h"
#include "audtool.h"

void playqueue_add(gint argc, gchar **argv)
{
	gint i;

	if (argc < 2)
	{
		audtool_whine("invalid parameters for %s.", argv[0]);
		audtool_whine("syntax: %s <position>", argv[0]);
		return;
	}

	i = atoi(argv[1]);

	if (i < 1 || i > audacious_remote_get_playlist_length(dbus_proxy))
	{
		audtool_whine("invalid playlist position %d", i);
		return;
	}

	if (!(audacious_remote_playqueue_is_queued(dbus_proxy, i - 1)))
		audacious_remote_playqueue_add(dbus_proxy, i - 1);
}

void playqueue_remove(gint argc, gchar **argv)
{
	gint i;

	if (argc < 2)
	{
		audtool_whine("invalid parameters for %s.", argv[0]);
		audtool_whine("syntax: %s <position>", argv[0]);
		return;
	}

	i = atoi(argv[1]);

	if (i < 1 || i > audacious_remote_get_playlist_length(dbus_proxy))
	{
		audtool_whine("invalid playlist position %d", i);
		return;
	}

	if (audacious_remote_playqueue_is_queued(dbus_proxy, i - 1))
		audacious_remote_playqueue_remove(dbus_proxy, i - 1);
}

void playqueue_is_queued(gint argc, gchar **argv)
{
	gint i;

	if (argc < 2)
	{
		audtool_whine("invalid parameters for playqueue-is-queued.", argv[0]);
		audtool_whine("syntax: %s playqueue-is-queued <position>", argv[0]);
		return;
	}

	i = atoi(argv[1]);

	if (i < 1 || i > audacious_remote_get_playlist_length(dbus_proxy))
	{
		audtool_whine("invalid playlist position %d", i);
		return;
	}
    if(audacious_remote_playqueue_is_queued(dbus_proxy, i - 1)) {
        audtool_report("OK");
        exit(0);
    }
    else
        exit(1);
}

void playqueue_get_queue_position(gint argc, gchar **argv)
{
	gint i, pos;

	if (argc < 2)
	{
		audtool_whine("invalid parameters for %s.", argv[0]);
		audtool_whine("syntax: %s <position>", argv[0]);
		return;
	}

	i = atoi(argv[1]);

	if (i < 1 || i > audacious_remote_get_playlist_length(dbus_proxy))
	{
		audtool_whine("invalid playlist position %d", i);
		return;
	}

	pos = audacious_remote_get_playqueue_queue_position(dbus_proxy, i - 1) + 1;

	if (pos < 1)
		return;

	audtool_report("%d", pos);
}

void playqueue_get_list_position(gint argc, gchar **argv)
{
	gint i, pos;

	if (argc < 2)
	{
		audtool_whine("invalid parameters for %s.", argv[0]);
		audtool_whine("syntax: %s <position>", argv[0]);
		return;
	}

	i = atoi(argv[1]);

	if (i < 1 || i > audacious_remote_get_playqueue_length(dbus_proxy))
	{
		audtool_whine("invalid playlist position %d", i);
		return;
	}

	pos = audacious_remote_get_playqueue_list_position(dbus_proxy, i - 1) + 1;

	if (pos < 1)
		return;

	audtool_report("%d", pos);
}

void playqueue_display(gint argc, gchar **argv)
{
	gint i, ii, position, frames, length, total;
	gchar *songname;
	gchar *fmt = NULL, *p;
	gint column;
	
	i = audacious_remote_get_playqueue_length(dbus_proxy);

	audtool_report("%d queued tracks.", i);

	total = 0;

	for (ii = 0; ii < i; ii++)
	{
		position = audacious_remote_get_playqueue_list_position(dbus_proxy, ii);
		songname = audacious_remote_get_playlist_title(dbus_proxy, position);
		frames = audacious_remote_get_playlist_time(dbus_proxy, position);
		length = frames / 1000;
		total += length;

		/* adjust width for multi byte characters */
		column = 60;
		if(songname) {
			p = songname;
			while(*p){
				gint stride;
				stride = g_utf8_next_char(p) - p;
				if(g_unichar_iswide(g_utf8_get_char(p))
#if ( (GLIB_MAJOR_VERSION == 2) && (GLIB_MINOR_VERSION >= 12) )
				   || g_unichar_iswide_cjk(g_utf8_get_char(p))
#endif
				){
					column += (stride - 2);
				}
				else {
					column += (stride - 1);
				}
				p = g_utf8_next_char(p);
			}
		}

		fmt = g_strdup_printf("%%4d | %%4d | %%-%ds | %%d:%%.2d", column);
		audtool_report(fmt, ii + 1, position + 1, songname, length / 60, length % 60);
		g_free(fmt);
	}

	audtool_report("Total length: %d:%.2d", total / 60, total % 60);
}

void playqueue_length(gint argc, gchar **argv)
{
	gint i;

	i = audacious_remote_get_playqueue_length(dbus_proxy);

	audtool_report("%d", i);
}

void playqueue_clear(gint argc, gchar **argv)
{
	audacious_remote_playqueue_clear(dbus_proxy);
}
