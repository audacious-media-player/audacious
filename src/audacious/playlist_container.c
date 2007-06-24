/*
 * audacious: Cross-platform multimedia player.
 * playlist_container.c: Support for playlist containers.
 *
 * Copyright (c) 2005-2007 Audacious development team.
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

#include <glib.h>
#include <string.h>
#include "playlist_container.h"

/*
 * PlaylistContainer objects handle the import and export of Playlist
 * data. Basically, a PlaylistContainer acts as a filter for a PlaylistEntry.
 */

static GList *registered_plcs = NULL;

void playlist_container_register(PlaylistContainer *plc)
{
	registered_plcs = g_list_append(registered_plcs, plc);
}

void playlist_container_unregister(PlaylistContainer *plc)
{
	registered_plcs = g_list_remove(registered_plcs, plc);
}

PlaylistContainer *playlist_container_find(char *ext)
{
	GList *node;
	PlaylistContainer *plc;

	/* check ext neither is NULL nor 1 (in a consequence of optimization). */
	g_return_val_if_fail(ext != NULL && ext != (void *)1, NULL);

	for (node = registered_plcs; node != NULL; node = g_list_next(node)) {
		plc = PLAYLIST_CONTAINER(node->data);

		if (!g_strcasecmp(plc->ext, ext))
			return plc;
	}

	return NULL;
}

void playlist_container_read(char *filename, gint pos)
{
	char *ext = strrchr(filename, '.') + 1;		/* optimization: skip past the dot -nenolod */
	PlaylistContainer *plc = playlist_container_find(ext);

	if (plc->plc_read == NULL)
		return;

	plc->plc_read(filename, pos);
}

void playlist_container_write(char *filename, gint pos)
{
	char *ext = strrchr(filename, '.') + 1;		/* optimization: skip past the dot -nenolod */
	PlaylistContainer *plc = playlist_container_find(ext);

	if (plc->plc_write == NULL)
		return;

	plc->plc_write(filename, pos);
}

gboolean is_playlist_name(char *filename)
{
	char *ext = strrchr(filename, '.') + 1;		/* optimization: skip past the dot -nenolod */
	PlaylistContainer *plc = playlist_container_find(ext);

	if (plc != NULL)
		return TRUE;

	return FALSE;
}
