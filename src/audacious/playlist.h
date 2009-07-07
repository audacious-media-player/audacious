/*  Audacious - Cross-platform multimedia player
 *  Copyright (C) 2005-2007  William Pitcock, Tony Vroon, George Averill,
 *                           Giacomo Lozito, Derek Pomery and Yoshiki Yazawa.
 *
 *  XMMS - Cross-platform multimedia player
 *  Copyright (C) 1998-2003  Peter Alm, Mikael Alm, Olle Hallnas,
 *                           Thomas Nilsson and 4Front Technologies
 *  Copyright (C) 1999-2003  Haavard Kvaalen
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; under version 3 of the License.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses>.
 *
 *  The Audacious team does not consider modular code linking to
 *  Audacious or using our public API to be a derived work.
 */
#ifndef AUDACIOUS_PLAYLIST_H
#define AUDACIOUS_PLAYLIST_H

/* XXX: Allow pre-0.2 libmowgli to build audacious. */
#ifdef TRUE
# undef TRUE
#endif

#ifdef FALSE
# undef FALSE
#endif

#include <mowgli.h>
#include <glib.h>
#include "libaudcore/tuple.h"
#include "libaudcore/tuple_formatter.h"

typedef struct _PlaylistEntry PlaylistEntry;
typedef struct _Playlist Playlist;

typedef enum {
    PLAYLIST_SORT_PATH,
    PLAYLIST_SORT_FILENAME,
    PLAYLIST_SORT_TITLE,
    PLAYLIST_SORT_ALBUM,
    PLAYLIST_SORT_ARTIST,
    PLAYLIST_SORT_DATE,
    PLAYLIST_SORT_TRACK,
    PLAYLIST_SORT_PLAYLIST
} PlaylistSortType;

typedef enum {
    PLAYLIST_DUPS_PATH,
    PLAYLIST_DUPS_FILENAME,
    PLAYLIST_DUPS_TITLE
} PlaylistDupsType;

#include "audacious/plugin.h"

G_BEGIN_DECLS

#define PLAYLIST_ENTRY(x)  ((PlaylistEntry*)(x))
struct _PlaylistEntry {
    gchar *filename;
    gchar *title;
    gint length;
    gboolean selected;
    InputPlugin *decoder;
    Tuple *tuple;		/* cached entry tuple, if available */
    gboolean failed;
};

#define PLAYLIST(x)  ((Playlist *)(x))

typedef enum {
    PLAYLIST_PLAIN = 0,
    PLAYLIST_STATIC = 1,
    PLAYLIST_USE_RELATIVE = 1 << 1
} PlaylistAttribute;

struct _Playlist {
    gchar         *title;
    gchar         *filename;
    gint           length;
    GList         *entries;
    GList         *queue;
    GList         *shuffle;
    PlaylistEntry *position;    /* bleah */
    gulong         pl_total_time;
    gulong         pl_selection_time;
    gboolean       pl_total_more;
    gboolean       pl_selection_more;
    GMutex        *mutex;       /* this is required for multiple playlist */
    gint           attribute; /* PlaylistAttribute */
    gulong         serial;     /* serial number */
    gpointer       ui_data;    /* pointer to UI data */
};

typedef enum {
    PLAYLIST_ASSOC_LINEAR,
    PLAYLIST_ASSOC_QUEUE,
    PLAYLIST_ASSOC_SHUFFLE
} PlaylistAssociation;

extern const guint n_titlestring_presets;

PlaylistEntry *playlist_entry_new(const gchar * filename,
                                  const gchar * title, const gint len,
				  InputPlugin * dec);
void playlist_entry_free(PlaylistEntry * entry);
void playlist_entry_get_info (PlaylistEntry * entry);

void playlist_entry_associate(Playlist * playlist, PlaylistEntry * entry,
                              PlaylistAssociation assoc);

void playlist_entry_associate_pos(Playlist * playlist, PlaylistEntry * entry,
                                  PlaylistAssociation assoc, gint pos);

void playlist_init (void);
void playlist_end (void);

void playlist_add_playlist(Playlist *);
void playlist_remove_playlist(Playlist *);
void playlist_select_playlist(Playlist *);
void playlist_select_next(void);
void playlist_select_prev(void);
GList * playlist_get_playlists(void);

void playlist_clear_only(Playlist *playlist);
void playlist_clear(Playlist *playlist);
void playlist_delete(Playlist *playlist, gboolean crop);

gboolean playlist_add(Playlist *playlist, const gchar * filename);
gboolean playlist_ins(Playlist *playlist, const gchar * filename, gint pos);
guint playlist_add_dir(Playlist *playlist, const gchar * dir);
guint playlist_ins_dir(Playlist *playlist, const gchar * dir, gint pos, gboolean background);
guint playlist_add_url(Playlist *playlist, const gchar * url);
guint playlist_ins_url(Playlist *playlist, const gchar * string, gint pos);

void playlist_set_info(Playlist *playlist, const gchar * title, gint length, gint rate,
                       gint freq, gint nch);
void playlist_set_info_old_abi(const gchar * title, gint length, gint rate,
                               gint freq, gint nch);
void playlist_check_pos_current(Playlist *playlist);
void playlist_next(Playlist *playlist);
void playlist_prev(Playlist *playlist);
void playlist_queue(Playlist *playlist);
void playlist_queue_position(Playlist *playlist, guint pos);
void playlist_queue_remove(Playlist *playlist, guint pos);
gint playlist_queue_get_length(Playlist *playlist);
gboolean playlist_is_position_queued(Playlist *playlist, guint pos);
void playlist_clear_queue(Playlist *playlist);
gint playlist_get_queue_position(Playlist *playlist, PlaylistEntry * entry);
gint playlist_get_queue_position_number(Playlist *playlist, guint pos);
gint playlist_get_queue_qposition_number(Playlist *playlist, guint pos);
void playlist_eof_reached(Playlist *playlist);
void playlist_set_position(Playlist *playlist, guint pos);
gint playlist_get_length(Playlist *playlist);
gint playlist_get_position(Playlist *playlist);
gint playlist_get_position_nolock(Playlist *playlist);
void playlist_generate_shuffle_list (Playlist * playlist);
gchar *playlist_get_info_text(Playlist *playlist);
gint playlist_get_current_length(Playlist *playlist);
void playlist_shift(Playlist *playlist, gint delta);

gboolean playlist_save(Playlist *playlist, const gchar * filename);
gboolean playlist_load(Playlist *playlist, const gchar * filename);

void playlist_update_all_titles(void);

void playlist_sort(Playlist *playlist, PlaylistSortType type);
void playlist_sort_selected(Playlist *playlist, PlaylistSortType type);

void playlist_reverse(Playlist *playlist);
void playlist_random(Playlist *playlist);
void playlist_remove_duplicates(Playlist *playlist, PlaylistDupsType);
void playlist_remove_dead_files(Playlist *playlist);

void playlist_delete_index(Playlist *playlist, guint pos);

PlaylistEntry *playlist_get_entry_to_play(Playlist *playlist);

/* XXX this is for reverse compatibility --nenolod */
const gchar *playlist_get_filename_to_play(Playlist *playlist);

gchar *playlist_get_filename(Playlist *playlist, guint pos);
gchar *playlist_get_songtitle(Playlist *playlist, guint pos);
Tuple *playlist_get_tuple(Playlist *playlist, guint pos);
gint playlist_get_songtime(Playlist *playlist, guint pos);

GList *playlist_get_selected(Playlist *playlist);
GList *playlist_get_selected_elems(Playlist *playlist);
int playlist_get_num_selected(Playlist *playlist);

void playlist_get_total_time(Playlist *playlist, gulong * total_time, gulong * selection_time,
                             gboolean * total_more,
                             gboolean * selection_more);

gint playlist_select_search(Playlist *playlist, Tuple *tuple, gint action);
void playlist_select_all(Playlist *playlist, gboolean set);
void playlist_select_range(Playlist *playlist, gint min, gint max, gboolean sel);
void playlist_select_invert_all(Playlist *playlist);
gboolean playlist_select_invert(Playlist *playlist, guint pos);

gboolean playlist_read_info_selection(Playlist *playlist);
void playlist_read_info(Playlist *playlist, guint pos);
void playlist_rescan (Playlist * playlist);

void playlist_set_shuffle(gboolean shuffle);

void playlist_clear_selected(Playlist *playlist);

GList *get_playlist_nth(Playlist *playlist, guint);

gboolean playlist_set_current_name(Playlist *playlist, const gchar * title);
const gchar *playlist_get_current_name(Playlist *playlist);

gboolean playlist_filename_set(Playlist *playlist, const gchar * filename);
gchar *playlist_filename_get(Playlist *playlist);

Playlist *playlist_new(void);
void playlist_free(Playlist *playlist);
Playlist *playlist_new_from_selected(void);

gboolean is_playlist_name(const gchar * filename);

#define PLAYLIST_LOCK(pl)    g_mutex_lock((pl)->mutex)
#define PLAYLIST_UNLOCK(pl)  g_mutex_unlock((pl)->mutex)
#define PLAYLIST_INCR_SERIAL(pl)    (pl)->serial++

G_LOCK_EXTERN(playlists);

extern void playlist_load_ins_file(Playlist *playlist, const gchar * filename,
                                   const gchar * playlist_name, gint pos,
                                   const gchar * title, gint len);

extern void playlist_load_ins_file_tuple(Playlist *playlist, const gchar * filename_p,
					 const gchar * playlist_name, gint pos,
					 Tuple *tuple);

Playlist *playlist_get_active(void);

gboolean playlist_playlists_equal(Playlist *p1, Playlist *p2);

extern const gchar *get_gentitle_format(void);

G_END_DECLS

#endif /* AUDACIOUS_PLAYLIST_H */
