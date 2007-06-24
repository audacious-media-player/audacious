/*
 * audacious: Cross-platform multimedia player.
 * playlist_container.h: Support for playlist containers.            
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

#ifndef _PLAYLIST_CONTAINER_H_
#define _PLAYLIST_CONTAINER_H_

G_BEGIN_DECLS

struct _PlaylistContainer {
	char *name;					/* human-readable name */
	char *ext;					/* extension */
	void (*plc_read)(const gchar *filename, gint pos);	/* plc_load */
	void (*plc_write)(const gchar *filename, gint pos);	/* plc_write */
};

typedef struct _PlaylistContainer PlaylistContainer;

#define PLAYLIST_CONTAINER(x)		((PlaylistContainer *)(x))

extern void playlist_container_register(PlaylistContainer *plc);
extern void playlist_container_unregister(PlaylistContainer *plc);
extern void playlist_container_read(char *filename, gint pos);
extern void playlist_container_write(char *filename, gint pos);
extern PlaylistContainer *playlist_container_find(char *ext);

G_END_DECLS

#endif
