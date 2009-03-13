/*  Audacious
 *  Copyright (C) 2005-2007  Audacious team.
 *
 *  BMP (C) GPL 2003 $top_src_dir/AUTHORS
 *
 *  based on:
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

/* #define AUD_DEBUG 1 */

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include "playlist.h"

#include <glib.h>
#include <glib/gprintf.h>
#include <glib/gi18n.h>
#include <mowgli.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/errno.h>
#include <time.h>
#include <unistd.h>

#if defined(USE_REGEX_ONIGURUMA)
#  include <onigposix.h>
#elif defined(USE_REGEX_PCRE)
#  include <pcreposix.h>
#else
#  include <regex.h>
#endif

#include "configdb.h"
#include "hook.h"
#include "input.h"
#include "playback.h"
#include "playlist_container.h"
#include "playlist_evmessages.h"
#include "pluginenum.h"
#include "strings.h"
#include "util.h"
#include "vfs.h"
#include "debug.h"

typedef gint (*PlaylistCompareFunc) (PlaylistEntry * a, PlaylistEntry * b);
typedef void (*PlaylistSaveFunc) (FILE * file);

/* If we manually change the song, p_p_b_j will show us where to go back to */
static PlaylistEntry *playlist_position_before_jump = NULL;

static GList *playlists = NULL;
static GList *playlists_iter;

static gboolean playlist_get_info_scan_active = FALSE;
static GStaticRWLock playlist_get_info_rwlock = G_STATIC_RW_LOCK_INIT;
static gboolean playlist_get_info_going = FALSE;
static GThread *playlist_get_info_thread;

extern GHashTable *ext_hash;

static gint path_compare(const gchar * a, const gchar * b);
static gint playlist_compare_path(PlaylistEntry * a, PlaylistEntry * b);
static gint playlist_compare_filename(PlaylistEntry * a, PlaylistEntry * b);
static gint playlist_compare_title(PlaylistEntry * a, PlaylistEntry * b);
static gint playlist_compare_artist(PlaylistEntry * a, PlaylistEntry * b);
static time_t playlist_get_mtime(const gchar *filename);
static gint playlist_compare_date(PlaylistEntry * a, PlaylistEntry * b);
static gint playlist_compare_track(PlaylistEntry * a, PlaylistEntry * b);
static gint playlist_compare_playlist(PlaylistEntry * a, PlaylistEntry * b);

static gint playlist_dupscmp_path(PlaylistEntry * a, PlaylistEntry * b);
static gint playlist_dupscmp_filename(PlaylistEntry * a, PlaylistEntry * b);
static gint playlist_dupscmp_title(PlaylistEntry * a, PlaylistEntry * b);

static PlaylistCompareFunc playlist_compare_func_table[] = {
    playlist_compare_path,
    playlist_compare_filename,
    playlist_compare_title,
    playlist_compare_artist,
    playlist_compare_date,
    playlist_compare_track,
    playlist_compare_playlist
};

static guint playlist_load_ins(Playlist * playlist, const gchar * filename, gint pos);

static void playlist_generate_shuffle_list(Playlist *);
static void playlist_generate_shuffle_list_nolock(Playlist *);

static void playlist_recalc_total_time_nolock(Playlist *);
static void playlist_recalc_total_time(Playlist *);
static gboolean playlist_entry_get_info(PlaylistEntry * entry);


#define EXT_TRUE    1
#define EXT_FALSE   0
#define EXT_HAVE_SUBTUNE    2
#define EXT_CUSTOM  3

const gchar *aud_titlestring_presets[] = {
    "${title}",
    "${?artist:${artist} - }${title}",
    "${?artist:${artist} - }${?album:${album} - }${title}",
    "${?artist:${artist} - }${?album:${album} - }${?track-number:${track-number}. }${title}",
    "${?artist:${artist} }${?album:[ ${album} ] }${?artist:- }${?track-number:${track-number}. }${title}",
    "${?album:${album} - }${title}"
};
const guint n_titlestring_presets = G_N_ELEMENTS(aud_titlestring_presets);


static gint filter_by_extension(const gchar *filename);
static gboolean is_http(const gchar *filename);
static gboolean do_precheck(Playlist *playlist, const gchar *uri, ProbeResult **pr);

static mowgli_heap_t *playlist_entry_heap = NULL;

/* *********************** playlist entry code ********************** */

PlaylistEntry *
playlist_entry_new(const gchar * filename,
                   const gchar * title,
                   const gint length,
                   InputPlugin * dec)
{
    PlaylistEntry *entry;

    entry = mowgli_heap_alloc(playlist_entry_heap);
    entry->filename = g_strdup(filename);
    entry->title = str_assert_utf8(title);
    entry->length = length;
    entry->selected = FALSE;
    entry->decoder = dec;

    /* only do this if we have a decoder, otherwise it just takes too long */
    if (entry->decoder)
        playlist_entry_get_info(entry);

    return entry;
}

void
playlist_entry_free(PlaylistEntry * entry)
{
    if (!entry)
        return;

    if (entry->tuple != NULL) {
        mowgli_object_unref(entry->tuple);
        entry->tuple = NULL;
    }

    g_free(entry->filename);
    g_free(entry->title);
    mowgli_heap_free(playlist_entry_heap, entry);
}

static gboolean
playlist_entry_get_info(PlaylistEntry * entry)
{
    Tuple *tuple = NULL;
    ProbeResult *pr = NULL;
    time_t modtime;
    const gchar *formatter;

    g_return_val_if_fail(entry != NULL, FALSE);

    if (entry->tuple == NULL || tuple_get_int(entry->tuple, FIELD_MTIME, NULL) > 0 ||
        tuple_get_int(entry->tuple, FIELD_MTIME, NULL) == -1)
        modtime = playlist_get_mtime(entry->filename);
    else
        modtime = 0;  /* URI -nenolod */

    if (str_has_prefix_nocase(entry->filename, "http:") &&
        g_thread_self() != playlist_get_info_thread)
        return FALSE;

    if (entry->decoder == NULL) {
        pr = input_check_file(entry->filename, FALSE);
        if (pr != NULL)
            entry->decoder = pr->ip;
    }

    /* renew tuple if file mtime is newer than tuple mtime. */
    if (entry->tuple){
        if (tuple_get_int(entry->tuple, FIELD_MTIME, NULL) == modtime) {
            if (pr != NULL) g_free(pr);

            if (entry->title_is_valid == FALSE) { /* update title even tuple is present and up to date --asphyx */
                AUDDBG("updating title from actual tuple\n");
                formatter = tuple_get_string(entry->tuple, FIELD_FORMATTER, NULL);
                if (entry->title != NULL) g_free(entry->title);
                entry->title = tuple_formatter_make_title_string(entry->tuple, formatter ?
                                                                 formatter : get_gentitle_format());
                entry->title_is_valid = TRUE;
                AUDDBG("new title: \"%s\"\n", entry->title);
            }

            return TRUE;
        } else {
            mowgli_object_unref(entry->tuple);
            entry->tuple = NULL;
            return TRUE;
        }
    }

    if (pr != NULL && pr->tuple != NULL)
        tuple = pr->tuple;
    else if (entry->decoder != NULL && entry->decoder->get_song_tuple != NULL)
    {
        plugin_set_current((Plugin *)(entry->decoder));
        tuple = entry->decoder->get_song_tuple(entry->filename);
    }

    if (tuple == NULL) {
        if (pr != NULL) g_free(pr);
        return FALSE;
    }

    /* attach mtime */
    tuple_associate_int(tuple, FIELD_MTIME, NULL, modtime);

    /* entry is still around */
    formatter = tuple_get_string(tuple, FIELD_FORMATTER, NULL);
    entry->title = tuple_formatter_make_title_string(tuple, formatter ?
                                                     formatter : get_gentitle_format());
    entry->title_is_valid = TRUE;
    entry->length = tuple_get_int(tuple, FIELD_LENGTH, NULL);
    entry->tuple = tuple;

    if (pr != NULL) g_free(pr);

    return TRUE;
}

/* *********************** playlist selector code ************************* */

void
playlist_init(void)
{
    Playlist *initial_pl;

    /* create a heap with 1024 playlist entry nodes preallocated. --nenolod */
    playlist_entry_heap = mowgli_heap_create(sizeof(PlaylistEntry), 1024,
                                             BH_NOW);

    /* FIXME: is this really necessary? REQUIRE_STATIC_LOCK(playlists); */

    initial_pl = playlist_new();

    playlist_add_playlist(initial_pl);
}

void
playlist_add_playlist(Playlist *playlist)
{
    playlists = g_list_append(playlists, playlist);

    if (playlists_iter == NULL)
        playlists_iter = playlists;

    hook_call("playlist create", playlist);
    event_queue("playlist update", NULL);
}

void
playlist_remove_playlist(Playlist *playlist)
{
    gboolean active;
    active = (playlist && playlist == playlist_get_active());
    /* users suppose playback will be stopped on removing playlist */
    if (active && playback_get_playing()) {
        ip_data.stop = TRUE;
        playback_stop();
        ip_data.stop = FALSE;
        hook_call("playlist end reached", NULL);
    }

    /* trying to free the last playlist simply clears and resets it */
    if (g_list_length(playlists) < 2) {
        playlist_clear(playlist);
        playlist_set_current_name(playlist, NULL);
        playlist_filename_set(playlist, NULL);
        return;
    }

    hook_call("playlist destroy", playlist);

    if (active) playlist_select_next();

    /* upon removal, a playlist should be cleared and freed */
    playlists = g_list_remove(playlists, playlist);
    playlist_clear(playlist);
    playlist_free(playlist);

    if (playlists_iter == NULL)
        playlists_iter = playlists;

    event_queue("playlist update", NULL);
}

GList *
playlist_get_playlists(void)
{
    return playlists;
}

void
playlist_select_next(void)
{
    if (playlists_iter == NULL)
        playlists_iter = playlists;

    playlists_iter = g_list_next(playlists_iter);

    if (playlists_iter == NULL)
        playlists_iter = playlists;

    event_queue("playlist update", playlist_get_active());
}

void
playlist_select_prev(void)
{
    if (playlists_iter == NULL)
        playlists_iter = playlists;

    playlists_iter = g_list_previous(playlists_iter);

    if (playlists_iter == NULL)
        playlists_iter = playlists;

    event_queue("playlist update", playlist_get_active());
}

void
playlist_select_playlist(Playlist *playlist)
{
    playlists_iter = g_list_find(playlists, playlist);

    if (playlists_iter == NULL)
        playlists_iter = playlists;

    event_queue("playlist update", playlist);
}

/* *********************** playlist code ********************** */

const gchar *
playlist_get_current_name(Playlist *playlist)
{
    return playlist->title;
}

/* This function now sets the playlist title, not the playlist filename.
 * See playlist_filename_set */
gboolean
playlist_set_current_name(Playlist *playlist, const gchar * title)
{
    gchar *oldtitle = playlist->title;

    if (!title) {
        playlist->title = NULL;
        g_free(oldtitle);
        event_queue("playlist update", NULL);
        return FALSE;
    }

    playlist->title = str_assert_utf8(title);
    g_free(oldtitle);
    event_queue("playlist update", NULL);
    return TRUE;
}

/* Setting the filename allows the original playlist to be modified later */
gboolean
playlist_filename_set(Playlist *playlist, const gchar * filename)
{
    gchar *old;
    old = playlist->filename;

    if (!filename) {
        playlist->filename = NULL;
        g_free(old);
        return FALSE;
    }

    playlist->filename = filename_to_utf8(filename);
    g_free(old);
    return TRUE;
}

gchar *
playlist_filename_get(Playlist *playlist)
{
    if (!playlist->filename) return NULL;
    return g_filename_from_utf8(playlist->filename, -1, NULL, NULL, NULL);
}

static GList *
find_playlist_position_list(Playlist *playlist)
{
    REQUIRE_LOCK(playlist->mutex);

    if (!playlist->position) {
        if (cfg.shuffle)
            return playlist->shuffle;
        else
            return playlist->entries;
    }

    if (cfg.shuffle)
        return g_list_find(playlist->shuffle, playlist->position);
    else
        return g_list_find(playlist->entries, playlist->position);
}

static void
play_queued(Playlist *playlist)
{
    GList *tmp = playlist->queue;

    REQUIRE_LOCK(playlist->mutex);

    playlist->position = playlist->queue->data;
    playlist->queue = g_list_remove_link(playlist->queue, playlist->queue);
    g_list_free_1(tmp);
}

void
playlist_clear_only(Playlist *playlist)
{
    PLAYLIST_LOCK(playlist);

    g_list_foreach(playlist->entries, (GFunc) playlist_entry_free, NULL);
    g_list_free(playlist->entries);
    playlist->position = NULL;
    playlist->entries = NULL;
    playlist->tail = NULL;
    playlist->attribute = PLAYLIST_PLAIN;
    playlist->serial = 0;

    PLAYLIST_UNLOCK(playlist);
}

void
playlist_shift(Playlist *playlist, gint delta)
{
    gint orig_delta;
    GList *n, *tn;
    g_return_if_fail(playlist != NULL);

    if (delta == 0)
        return;

    PLAYLIST_LOCK(playlist);

    /* copy the delta over. */
    orig_delta = delta;

    /* even though it is unlikely we would ever be calling playlist_shift()
       on an empty playlist... we should probably check for this. --nenolod */
    if ((n = playlist->entries) == NULL)
    {
        PLAYLIST_UNLOCK(playlist);
        return;
    }

    MOWGLI_ITER_FOREACH_SAFE(n, tn, playlist->entries)
    {
        PlaylistEntry *entry = PLAYLIST_ENTRY(n->data);

        if (!entry->selected)
            continue;

        if (orig_delta > 0)
            for (delta = orig_delta; delta > 0; delta--)
                glist_movedown(n);
        else (orig_delta < 0)
            for (delta = orig_delta; delta > 0; delta--)
                glist_moveup(n);
    }

    /* do the remaining work. */
    playlist_generate_shuffle_list(playlist);
    event_queue("playlist update", playlist);
    PLAYLIST_INCR_SERIAL(playlist);

    PLAYLIST_UNLOCK(playlist);
}

void
playlist_clear(Playlist *playlist)
{
    if (!playlist)
        return;

    playlist_clear_only(playlist);
    playlist_generate_shuffle_list(playlist);
    event_queue("playlist update", playlist);
    playlist_recalc_total_time(playlist);
    PLAYLIST_INCR_SERIAL(playlist);
}

static void
playlist_delete_node(Playlist * playlist, GList * node, gboolean * set_info_text,
                     gboolean * restart_playing)
{
    PlaylistEntry *entry;
    GList *playing_song = NULL;

    REQUIRE_LOCK(playlist->mutex);

    /* We call g_list_find manually here because we don't want an item
     * in the shuffle_list */

    if (playlist->position)
        playing_song = g_list_find(playlist->entries, playlist->position);

    entry = PLAYLIST_ENTRY(node->data);

    if (playing_song == node) {
        *set_info_text = TRUE;

        if (playback_get_playing()) {
            PLAYLIST_UNLOCK(playlist);
            ip_data.stop = TRUE;
            playback_stop();
            ip_data.stop = FALSE;
            PLAYLIST_LOCK(playlist);
            *restart_playing = TRUE;
        }

        playing_song = find_playlist_position_list(playlist);

        if (g_list_next(playing_song))
            playlist->position = g_list_next(playing_song)->data;
        else if (g_list_previous(playing_song))
            playlist->position = g_list_previous(playing_song)->data;
        else
            playlist->position = NULL;

        /* Make sure the entry did not disappear under us */
        if (g_list_index(playlist->entries, entry) == -1)
            return;

    }
    else if (g_list_position(playlist->entries, playing_song) >
             g_list_position(playlist->entries, node)) {
        *set_info_text = TRUE;
    }

    playlist->shuffle = g_list_remove(playlist->shuffle, entry);
    playlist->queue = g_list_remove(playlist->queue, entry);
    playlist->entries = g_list_remove_link(playlist->entries, node);
    playlist->tail = g_list_last(playlist->entries);
    playlist_entry_free(entry);
    g_list_free_1(node);

    playlist_recalc_total_time_nolock(playlist);
    PLAYLIST_INCR_SERIAL(playlist);
}

void
playlist_delete_index(Playlist *playlist, guint pos)
{
    gboolean restart_playing = FALSE, set_info_text = FALSE;
    GList *node;

    if (!playlist)
        return;

    PLAYLIST_LOCK(playlist);

    node = g_list_nth(playlist->entries, pos);

    if (!node) {
        PLAYLIST_UNLOCK(playlist);
        return;
    }

    playlist_delete_node(playlist, node, &set_info_text, &restart_playing);

    PLAYLIST_UNLOCK(playlist);

    playlist_recalc_total_time(playlist);
    PLAYLIST_INCR_SERIAL(playlist);

    event_queue("playlist update", playlist);
    if (restart_playing) {
        if (playlist->position)
            playback_initiate();
        else
            hook_call("playlist end reached", NULL);
    }
}

void
playlist_delete(Playlist * playlist, gboolean crop)
{
    gboolean restart_playing = FALSE, set_info_text = FALSE;
    GList *node, *next_node;
    PlaylistEntry *entry;

    g_return_if_fail(playlist != NULL);

    PLAYLIST_LOCK(playlist);

    node = playlist->entries;

    while (node) {
        entry = PLAYLIST_ENTRY(node->data);

        next_node = g_list_next(node);

        if ((entry->selected && !crop) || (!entry->selected && crop)) {
            playlist_delete_node(playlist, node, &set_info_text, &restart_playing);
        }

        node = next_node;
    }

    PLAYLIST_UNLOCK(playlist);

    playlist_recalc_total_time(playlist);
    PLAYLIST_INCR_SERIAL(playlist);

    if (restart_playing) {
        if (playlist->position)
            playback_initiate();
        else
            hook_call("playlist end reached", NULL);
    }

    event_queue("playlist update", playlist);
}

static void
__playlist_ins_file(Playlist * playlist,
                    const gchar * filename,
                    gint pos,
                    Tuple *tuple,
                    const gchar *title, // may NULL
                    gint len,
                    InputPlugin * dec)
{
    PlaylistEntry *entry;
    Tuple *parent_tuple = NULL;
    gint nsubtunes = 0, subtune = 0;
    gboolean add_flag = TRUE;

    g_return_if_fail(playlist != NULL);
    g_return_if_fail(filename != NULL);

    if (tuple != NULL) {
        nsubtunes = tuple->nsubtunes;
        if (nsubtunes > 0) {
            parent_tuple = tuple;
            subtune = 1;
        }
    }

    for (; add_flag && subtune <= nsubtunes; subtune++) {
        gchar *filename_entry;

        if (nsubtunes > 0) {
            filename_entry = g_strdup_printf("%s?%d", filename,
                                             parent_tuple->subtunes ? parent_tuple->subtunes[subtune - 1] : subtune);

            /* We're dealing with subtune, let's ask again tuple information
             * to plugin, by passing the ?subtune suffix; this way we get
             * specific subtune information in the tuple, if available.
             */
            plugin_set_current((Plugin *)dec);
            tuple = dec->get_song_tuple(filename_entry);
        } else
            filename_entry = g_strdup(filename);

        if (tuple) {
            entry = playlist_entry_new(filename_entry,
                                       tuple_get_string(tuple, FIELD_TITLE, NULL),
                                       tuple_get_int(tuple, FIELD_LENGTH, NULL), dec);
        }
        else {
            entry = playlist_entry_new(filename_entry, title, len, dec);
        }

        g_free(filename_entry);


        PLAYLIST_LOCK(playlist);

        if (!playlist->tail)
            playlist->tail = g_list_last(playlist->entries);

        if (pos == -1) { // the common case
            GList *element;
            element = g_list_alloc();
            element->data = entry;
            element->prev = playlist->tail; // NULL is allowed here.
            element->next = NULL;

            if(!playlist->entries) { // this is the first element
                playlist->entries = element;
                playlist->tail = element;
            } else { // the rests
                if (playlist->tail != NULL) {
                    playlist->tail->next = element;
                    playlist->tail = element;
                } else
                    add_flag = FALSE;
            }
        } else {
            playlist->entries = g_list_insert(playlist->entries, entry, pos++);
            playlist->tail = g_list_last(playlist->entries);
        }

        if (tuple != NULL) {
            const gchar *formatter = tuple_get_string(tuple, FIELD_FORMATTER, NULL);
            g_free(entry->title);
            entry->title = tuple_formatter_make_title_string(tuple,
                                                             formatter ? formatter : get_gentitle_format());
            entry->title_is_valid = TRUE;
            entry->length = tuple_get_int(tuple, FIELD_LENGTH, NULL);
            entry->tuple = tuple;
        }

        PLAYLIST_UNLOCK(playlist);
    }

    if (parent_tuple)
        tuple_free(parent_tuple);

    if (!tuple || (tuple && tuple_get_int(tuple, FIELD_MTIME, NULL) == -1)) {
        // kick the scanner thread when tuple == NULL or mtime = -1 (uninitialized)
        g_mutex_lock(mutex_scan);
        playlist_get_info_scan_active = TRUE;
        g_mutex_unlock(mutex_scan);
        g_cond_signal(cond_scan);
    }
    PLAYLIST_INCR_SERIAL(playlist);
}

gboolean
playlist_ins(Playlist * playlist, const gchar * filename, gint pos)
{
    gchar buf[64], *p;
    gint r;
    VFSFile *file;
    ProbeResult *pr = NULL;
    InputPlugin *dec = NULL;
    Tuple *tuple = NULL;
    gboolean http_flag = is_http(filename);

    g_return_val_if_fail(playlist != NULL, FALSE);
    g_return_val_if_fail(filename != NULL, FALSE);

    PLAYLIST_INCR_SERIAL(playlist);

    /* load playlist */
    if (is_playlist_name(filename)) {
        playlist->loading_playlist = TRUE;
        playlist_load_ins(playlist, filename, pos);
        playlist->loading_playlist = FALSE;
        return TRUE;
    }

    if (do_precheck(playlist, filename, &pr)) {
        if (pr) {
            dec = pr->ip;
            tuple = pr->tuple;
        }
        /* add filename to playlist */
        if (cfg.playlist_detect == TRUE ||
            playlist->loading_playlist == TRUE ||
            (playlist->loading_playlist == FALSE && dec != NULL) ||
            (playlist->loading_playlist == FALSE && !is_playlist_name(filename)
             && http_flag))
        {
            __playlist_ins_file(playlist, filename, pos, tuple, NULL, -1, dec);

            g_free(pr);
            playlist_generate_shuffle_list(playlist);
            event_queue("playlist update", playlist);
            return TRUE;
        }
    }

    /* Some files (typically produced by some cgi-scripts) don't have
     * the correct extension.  Try to recognize these files by looking
     * at their content.  We only check for http entries since it does
     * not make sense to have file entries in a playlist fetched from
     * the net. */

    /* Some strange people put fifo's with the .mp3 extension, so we
     * need to make sure it's a real file (otherwise fread() may block
     * and stall the entire program) */

    /* FIXME: bah, FIFOs actually pass this regular file test */
    if (!vfs_file_test(filename, G_FILE_TEST_IS_REGULAR))
        return FALSE;

    if ((file = vfs_fopen(filename, "rb")) == NULL)
        return FALSE;

    r = vfs_fread(buf, 1, sizeof(buf), file);
    vfs_fclose(file);

    for (p = buf; r-- > 0 && (*p == '\r' || *p == '\n'); p++);

    if (r > 5 && str_has_prefix_nocase(p, "http:")) {
        playlist_load_ins(playlist, filename, pos);
        return TRUE;
    }

    if (r > 6 && str_has_prefix_nocase(p, "https:")) {
        playlist_load_ins(playlist, filename, pos);
        return TRUE;
    }

    return FALSE;
}

/* FIXME: The next few functions are specific to Unix
 * filesystems. Either abstract it away, or don't even bother checking
 * at such low level */

typedef struct {
    dev_t dev;
    ino_t ino;
} DeviceInode;

static DeviceInode *
devino_new(dev_t device,
           ino_t inode)
{
    DeviceInode *devino = g_new0(DeviceInode, 1);

    if (devino) {
        devino->dev = device;
        devino->ino = inode;
    }

    return devino;
}

static guint
devino_hash(gconstpointer key)
{
    const DeviceInode *d = key;
    return d->ino;
}

static gint
devino_compare(gconstpointer a,
               gconstpointer b)
{
    const DeviceInode *da = a, *db = b;
    return (da->dev == db->dev && da->ino == db->ino);
}

static gboolean
devino_destroy(gpointer key, 
               gpointer value,
               gpointer data)
{
    g_free(key);
    return TRUE;
}

static gboolean
file_is_hidden(const gchar * filename)
{
    g_return_val_if_fail(filename != NULL, FALSE);
    return (g_path_get_basename(filename)[0] == '.');
}

static GList *
playlist_dir_find_files(const gchar * path,
                        gboolean background,
                        GHashTable * htab)
{
    GDir *dir;
    GList *list = NULL, *ilist;
    const gchar *dir_entry;
    ProbeResult *pr = NULL;

    struct stat statbuf;
    DeviceInode *devino;

    if (!path)
        return NULL;

    if (!vfs_file_test(path, G_FILE_TEST_IS_DIR))
        return NULL;

    stat(path, &statbuf);
    devino = devino_new(statbuf.st_dev, statbuf.st_ino);

    if (g_hash_table_lookup(htab, devino)) {
        g_free(devino);
        return NULL;
    }

    g_hash_table_insert(htab, devino, GINT_TO_POINTER(1));

    /* XXX: what the hell is this for? --nenolod */
    if ((ilist = input_scan_dir(path))) {
        GList *node;
        for (node = ilist; node; node = g_list_next(node)) {
            gchar *name = g_build_filename(path, node->data, NULL);
            list = g_list_prepend(list, name);
            g_free(node->data);
        }
        g_list_free(ilist);
        return list;
    }

    /* g_dir_open does not handle URI, so path should come here not-urified. --giacomo */
    if (!(dir = g_dir_open(path, 0, NULL)))
        return NULL;

    while ((dir_entry = g_dir_read_name(dir))) {
        gchar *filename, *tmp;
        gint ext_flag;
        gboolean http_flag;

        if (file_is_hidden(dir_entry))
            continue;

        tmp = g_build_filename(path, dir_entry, NULL);
        filename = g_filename_to_uri(tmp, NULL, NULL);
        g_free(tmp);

        ext_flag = filter_by_extension(filename);
        http_flag = is_http(filename);

        if (vfs_file_test(filename, G_FILE_TEST_IS_DIR)) { /* directory */
            GList *sub;
            gchar *dirfilename = g_filename_from_uri(filename, NULL, NULL);
            sub = playlist_dir_find_files(dirfilename, background, htab);
            g_free(dirfilename);
            g_free(filename);
            list = g_list_concat(list, sub);
        }
        else if (cfg.playlist_detect && ext_flag != EXT_HAVE_SUBTUNE && ext_flag != EXT_CUSTOM) { /* local file, no probing, no subtune */
            if(cfg.use_extension_probing) {
                if(ext_flag == EXT_TRUE)
                    list = g_list_prepend(list, filename);
                else // ext_flag == EXT_FALSE => extension isn't known
                    g_free(filename);
            }
            else
                list = g_list_prepend(list, filename);
        }
        else if ((pr = input_check_file(filename, TRUE)) != NULL) /* local file, probing or have subtune */
        {
            list = g_list_prepend(list, filename);

            g_free(pr);
            pr = NULL;
        }
        else
            g_free(filename);

        while (background && gtk_events_pending())
            gtk_main_iteration();
    }
    g_dir_close(dir);

    return list;
}

gboolean
playlist_add(Playlist * playlist, const gchar * filename)
{
    return playlist_ins(playlist, filename, -1);
}

guint 
playlist_add_dir(Playlist * playlist, const gchar * directory)
{
    return playlist_ins_dir(playlist, directory, -1, TRUE);
}

guint
playlist_add_url(Playlist * playlist, const gchar * url)
{
    guint entries;
    entries = playlist_ins_url(playlist, url, -1);
    return entries;
}

guint
playlist_ins_dir(Playlist * playlist, const gchar * path,
                 gint pos,
                 gboolean background)
{
    guint entries = 0;
    GList *list, *node;
    GHashTable *htab;
    gchar *path2 = g_filename_from_uri(path, NULL, NULL);

    if (path2 == NULL)
        path2 = g_strdup(path);

    htab = g_hash_table_new(devino_hash, devino_compare);

    list = playlist_dir_find_files(path2, background, htab);
    list = g_list_sort(list, (GCompareFunc) path_compare);

    g_hash_table_foreach_remove(htab, devino_destroy, NULL);

    for (node = list; node; node = g_list_next(node)) {
        playlist_ins(playlist, node->data, pos);
        g_free(node->data);
        entries++;
        if (pos >= 0)
            pos++;
    }

    g_list_free(list);
    g_free(path2);

    playlist_recalc_total_time(playlist);
    playlist_generate_shuffle_list(playlist);
    event_queue("playlist update", playlist);
    return entries;
}

guint
playlist_ins_url(Playlist * playlist, const gchar * string,
                 gint pos)
{
    gchar *tmp;
    gint entries = 0;
    gchar *decoded = NULL;

    g_return_val_if_fail(playlist != NULL, 0);
    g_return_val_if_fail(string != NULL, 0);

    while (*string) {
        GList *node;
        guint i = 0;
        tmp = strchr(string, '\n');
        if (tmp) {
            if (*(tmp - 1) == '\r')
                *(tmp - 1) = '\0';
            *tmp = '\0';
        }

        decoded = g_strdup(string);

        if (vfs_file_test(decoded, G_FILE_TEST_IS_DIR))
            i = playlist_ins_dir(playlist, decoded, pos, FALSE);
        else if (is_playlist_name(decoded))
            i = playlist_load_ins(playlist, decoded, pos);
        else if (playlist_ins(playlist, decoded, pos))
            i = 1;

        g_free(decoded);

        PLAYLIST_LOCK(playlist);
        node = g_list_nth(playlist->entries, pos);
        PLAYLIST_UNLOCK(playlist);

        entries += i;

        if (pos >= 0)
            pos += i;
        if (!tmp)
            break;

        string = tmp + 1;
    }

    playlist_recalc_total_time(playlist);
    PLAYLIST_INCR_SERIAL(playlist);
    playlist_generate_shuffle_list(playlist);
    event_queue("playlist update", playlist);

    return entries;
}

/* set info for current song. */
void
playlist_set_info(Playlist * playlist, const gchar * title,
                  gint length, gint rate, gint freq, gint nch)
{
    PlaylistEventInfoChange *msg;
    gchar *text;

    g_return_if_fail(playlist != NULL);

    if (length == -1) {
        // event_queue hates NULL --yaz
        event_queue("hide seekbar", (gpointer)0xdeadbeef);
    }

    if (playlist->position) {
        g_free(playlist->position->title);
        playlist->position->title = g_strdup(title);
        playlist->position->length = length;

        // overwrite tuple::title, mainly for streaming. it may incur side effects. --yaz
        if (playlist->position->tuple && tuple_get_int(playlist->position->tuple, FIELD_LENGTH, NULL) == -1){
            tuple_disassociate(playlist->position->tuple, FIELD_TITLE, NULL);
            tuple_associate_string(playlist->position->tuple, FIELD_TITLE, NULL, title);
        }
    }

    playlist_recalc_total_time(playlist);

    /* broadcast a PlaylistEventInfoChange message. */
    msg = g_new0(PlaylistEventInfoChange, 1);
    msg->bitrate = rate;
    msg->samplerate = freq;
    msg->channels = nch;

    playback_set_sample_params(rate, freq, nch);
    event_queue_with_data_free("playlist info change", msg);

    text = playlist_get_info_text(playlist);
    event_queue_with_data_free("title change", text);

    if ( playlist->position )
        event_queue( "playlist set info" , playlist->position );
}

/* wrapper for playlist_set_info. this function is called by input plugins. */
void
playlist_set_info_old_abi(const gchar * title, gint length, gint rate,
                          gint freq, gint nch)
{
    Playlist *playlist = playlist_get_active();
    playlist_set_info(playlist, title, length, rate, freq, nch);
}

void
playlist_next(Playlist *playlist)
{
    GList *plist_pos_list;
    gboolean restart_playing = FALSE;

    if (!playlist_get_length(playlist))
        return;

    PLAYLIST_LOCK(playlist);

    if ((playlist_position_before_jump != NULL) && playlist->queue == NULL) {
        playlist->position = playlist_position_before_jump;
        playlist_position_before_jump = NULL;
    }

    plist_pos_list = find_playlist_position_list(playlist);

    if (!cfg.repeat && !g_list_next(plist_pos_list) && playlist->queue == NULL)
    {
        PLAYLIST_UNLOCK(playlist);
        return;
    }

    if (playback_get_playing()) {
        /* We need to stop before changing playlist_position */
        PLAYLIST_UNLOCK(playlist);
        ip_data.stop = TRUE;
        playback_stop();
        ip_data.stop = FALSE;
        PLAYLIST_LOCK(playlist);
        restart_playing = TRUE;
    }

    plist_pos_list = find_playlist_position_list(playlist);
    if (playlist->queue != NULL)
        play_queued(playlist);
    else if (g_list_next(plist_pos_list))
        playlist->position = g_list_next(plist_pos_list)->data;
    else if (cfg.repeat) {
        playlist->position = NULL;
        playlist_generate_shuffle_list_nolock(playlist);
        if (cfg.shuffle)
            playlist->position = playlist->shuffle->data;
        else
            playlist->position = playlist->entries->data;
    }

    PLAYLIST_UNLOCK(playlist);

    if (restart_playing)
        playback_initiate();
    else
        event_queue("playlist update", playlist);
}

void
playlist_prev(Playlist *playlist)
{
    GList *plist_pos_list;
    gboolean restart_playing = FALSE;

    if (!playlist_get_length(playlist))
        return;

    PLAYLIST_LOCK(playlist);

    if ((playlist_position_before_jump != NULL) && playlist->queue == NULL) {
        playlist->position = playlist_position_before_jump;
        playlist_position_before_jump = NULL;
    }

    plist_pos_list = find_playlist_position_list(playlist);

    if (!cfg.repeat && !g_list_previous(plist_pos_list)) {
        PLAYLIST_UNLOCK(playlist);
        return;
    }

    if (playback_get_playing()) {
        /* We need to stop before changing playlist_position */
        PLAYLIST_UNLOCK(playlist);
        ip_data.stop = TRUE;
        playback_stop();
        ip_data.stop = FALSE;
        PLAYLIST_LOCK(playlist);
        restart_playing = TRUE;
    }

    plist_pos_list = find_playlist_position_list(playlist);
    if (g_list_previous(plist_pos_list)) {
        playlist->position = g_list_previous(plist_pos_list)->data;
    }
    else if (cfg.repeat) {
        GList *node;
        playlist->position = NULL;
        playlist_generate_shuffle_list_nolock(playlist);
        if (cfg.shuffle)
            node = g_list_last(playlist->shuffle);
        else
            node = g_list_last(playlist->entries);
        if (node)
            playlist->position = node->data;
    }

    PLAYLIST_UNLOCK(playlist);

    if (restart_playing)
        playback_initiate();
    else
        event_queue("playlist update", playlist);
}

void
playlist_queue(Playlist *playlist)
{
    GList *list = playlist_get_selected(playlist);
    GList *it = list;

    PLAYLIST_LOCK(playlist);

    if ((cfg.shuffle) && (playlist_position_before_jump == NULL)) {
        /* Shuffling and this is our first manual jump. */
        playlist_position_before_jump = playlist->position;
    }

    while (it) {
        GList *next = g_list_next(it);
        GList *tmp;

        /* XXX: WTF? --nenolod */
        it->data = g_list_nth_data(playlist->entries, GPOINTER_TO_INT(it->data));
        if ((tmp = g_list_find(playlist->queue, it->data))) {
            playlist->queue = g_list_remove_link(playlist->queue, tmp);
            g_list_free_1(tmp);
            list = g_list_remove_link(list, it);
            g_list_free_1(it);
        }

        it = next;
    }

    playlist->queue = g_list_concat(playlist->queue, list);

    PLAYLIST_UNLOCK(playlist);

    playlist_recalc_total_time(playlist);
    event_queue("playlist update", playlist);
}

void
playlist_queue_position(Playlist *playlist, guint pos)
{
    GList *tmp;
    PlaylistEntry *entry;

    PLAYLIST_LOCK(playlist);

    if ((cfg.shuffle) && (playlist_position_before_jump == NULL))
    {
        /* Shuffling and this is our first manual jump. */
        playlist_position_before_jump = playlist->position;
    }

    entry = g_list_nth_data(playlist->entries, pos);
    if ((tmp = g_list_find(playlist->queue, entry))) {
        playlist->queue = g_list_remove_link(playlist->queue, tmp);
        g_list_free_1(tmp);
    }
    else
        playlist->queue = g_list_append(playlist->queue, entry);
    PLAYLIST_UNLOCK(playlist);

    playlist_recalc_total_time(playlist);
    event_queue("playlist update", playlist);
}

gboolean
playlist_is_position_queued(Playlist *playlist, guint pos)
{
    PlaylistEntry *entry;
    GList *tmp = NULL;

    PLAYLIST_LOCK(playlist);
    entry = g_list_nth_data(playlist->entries, pos);
    tmp = g_list_find(playlist->queue, entry);
    PLAYLIST_UNLOCK(playlist);

    return tmp != NULL;
}

gint
playlist_get_queue_position_number(Playlist *playlist, guint pos)
{
    PlaylistEntry *entry;
    gint tmp;

    PLAYLIST_LOCK(playlist);
    entry = g_list_nth_data(playlist->entries, pos);
    tmp = g_list_index(playlist->queue, entry);
    PLAYLIST_UNLOCK(playlist);

    return tmp;
}

gint
playlist_get_queue_qposition_number(Playlist *playlist, guint pos)
{
    PlaylistEntry *entry;
    gint tmp;

    PLAYLIST_LOCK(playlist);
    entry = g_list_nth_data(playlist->queue, pos);
    tmp = g_list_index(playlist->entries, entry);
    PLAYLIST_UNLOCK(playlist);

    return tmp;
}

void
playlist_clear_queue(Playlist *playlist)
{
    PLAYLIST_LOCK(playlist);
    g_list_free(playlist->queue);
    playlist->queue = NULL;
    PLAYLIST_UNLOCK(playlist);

    playlist_recalc_total_time(playlist);
    event_queue("playlist update", playlist);
}

void
playlist_queue_remove(Playlist *playlist, guint pos)
{
    void *entry;

    PLAYLIST_LOCK(playlist);
    entry = g_list_nth_data(playlist->entries, pos);
    playlist->queue = g_list_remove(playlist->queue, entry);
    PLAYLIST_UNLOCK(playlist);

    event_queue("playlist update", playlist);
}

gint
playlist_get_queue_position(Playlist *playlist, PlaylistEntry * entry)
{
    return g_list_index(playlist->queue, entry);
}

void
playlist_set_position(Playlist *playlist, guint pos)
{
    GList *node;
    gboolean restart_playing = FALSE;

    if (!playlist)
        return;

    PLAYLIST_LOCK(playlist);

    node = g_list_nth(playlist->entries, pos);
    if (!node) {
        PLAYLIST_UNLOCK(playlist);
        return;
    }

    if (playback_get_playing()) {
        /* We need to stop before changing playlist_position */
        PLAYLIST_UNLOCK(playlist);
        ip_data.stop = TRUE;
        playback_stop();
        ip_data.stop = FALSE;
        PLAYLIST_LOCK(playlist);
        restart_playing = TRUE;
    }

    if ((cfg.shuffle) && (playlist_position_before_jump == NULL))
    {
        /* Shuffling and this is our first manual jump. */
        playlist_position_before_jump = playlist->position;
    }

    playlist->position = node->data;
    PLAYLIST_UNLOCK(playlist);

    if (restart_playing)
        playback_initiate();
    else
        event_queue("playlist update", playlist);
}

void
playlist_eof_reached(Playlist *playlist)
{
    GList *plist_pos_list;

    if ((cfg.no_playlist_advance && !cfg.repeat) || cfg.stopaftersong)
        ip_data.stop = TRUE;
    playback_stop();
    if ((cfg.no_playlist_advance && !cfg.repeat) || cfg.stopaftersong)  
        ip_data.stop = FALSE;

    hook_call("playback end", playlist->position);

    PLAYLIST_LOCK(playlist);

    if ((playlist_position_before_jump != NULL) && playlist->queue == NULL)
    {
        playlist->position = playlist_position_before_jump;
        playlist_position_before_jump = NULL;
    }

    plist_pos_list = find_playlist_position_list(playlist);

    if (cfg.no_playlist_advance) {
        PLAYLIST_UNLOCK(playlist);
        if (cfg.repeat)
            playback_initiate();
        else
            hook_call("playlist end reached", NULL);
        return;
    }

    if (cfg.stopaftersong) {
        PLAYLIST_UNLOCK(playlist);
        hook_call("playlist end reached", NULL);
        return;
    }

    if (playlist->queue != NULL) {
        play_queued(playlist);
    }
    else if (!g_list_next(plist_pos_list)) {
        if (cfg.shuffle) {
            playlist->position = NULL;
            playlist_generate_shuffle_list_nolock(playlist);
        }
        else if (playlist->entries != NULL)
            playlist->position = playlist->entries->data;

        if (!cfg.repeat) {
            PLAYLIST_UNLOCK(playlist);
            hook_call("playlist end reached", NULL);
            return;
        }
    }
    else
        playlist->position = g_list_next(plist_pos_list)->data;

    PLAYLIST_UNLOCK(playlist);

    playback_initiate();
    event_queue("playlist update", playlist);
}

gint
playlist_queue_get_length(Playlist *playlist)
{
    gint length;

    PLAYLIST_LOCK(playlist);
    length = g_list_length(playlist->queue);
    PLAYLIST_UNLOCK(playlist);

    return length;
}

gint
playlist_get_length(Playlist *playlist)
{
    return g_list_length(playlist->entries);
}

gchar *
playlist_get_info_text(Playlist *playlist)
{
    gchar *text, *title, *numbers, *length;

    g_return_val_if_fail(playlist != NULL, NULL);

    PLAYLIST_LOCK(playlist);
    if (!playlist->position) {
        PLAYLIST_UNLOCK(playlist);
        return NULL;
    }

    /* FIXME: there should not be a need to do additional conversion,
     * if playlist is properly maintained */
    if (playlist->position->title) {
        title = str_assert_utf8(playlist->position->title);
    }
    else {
        gchar *realfn = g_filename_from_uri(playlist->position->filename, NULL, NULL);
        gchar *basename = g_path_get_basename(realfn ? realfn : playlist->position->filename);
        title = filename_to_utf8(basename);
        g_free(realfn);
        g_free(basename);
    }

    /*
     * If the user don't want numbers in the playlist, don't
     * display them in other parts of XMMS
     */

    if (cfg.show_numbers_in_pl)
        numbers = g_strdup_printf("%d. ", playlist_get_position_nolock(playlist) + 1);
    else
        numbers = g_strdup("");

    if (playlist->position->length != -1)
        length = g_strdup_printf(" (%d:%-2.2d)",
                                 playlist->position->length / 60000,
                                 (playlist->position->length / 1000) % 60);
    else
        length = g_strdup("");

    PLAYLIST_UNLOCK(playlist);

    text = convert_title_text(g_strconcat(numbers, title, length, NULL));

    g_free(numbers);
    g_free(title);
    g_free(length);

    return text;
}

gint
playlist_get_current_length(Playlist * playlist)
{
    gint len = 0;

    if (!playlist)
        return 0;

    if (playlist->position)
        len = playlist->position->length;

    return len;
}

gboolean
playlist_save(Playlist * playlist, const gchar * filename)
{
    PlaylistContainer *plc = NULL;
    GList *old_iter;
    gchar *ext;

    g_return_val_if_fail(playlist != NULL, FALSE);
    g_return_val_if_fail(filename != NULL, FALSE);

    ext = strrchr(filename, '.') + 1;

    if ((plc = playlist_container_find(ext)) == NULL)
        return FALSE;

    if (plc->plc_write == NULL)
        return FALSE;

    /* Save the right playlist to disk */
    if (playlist != playlist_get_active()) {
        old_iter = playlists_iter;
        playlists_iter = g_list_find(playlists, playlist);
        if(!playlists_iter) playlists_iter = old_iter;
        plc->plc_write(filename, 0);
        playlists_iter = old_iter;
    } else {
        plc->plc_write(filename, 0);
    }

    return TRUE;
}

gboolean
playlist_load(Playlist * playlist, const gchar * filename)
{
    guint ret = 0;
    g_return_val_if_fail(playlist != NULL, FALSE);

    playlist->loading_playlist = TRUE;
    if(!playlist_get_length(playlist)) {
        /* Loading new playlist */
        playlist_filename_set(playlist, filename);
    }
    ret = playlist_load_ins(playlist, filename, -1);
    playlist->loading_playlist = FALSE;

    return ret ? TRUE : FALSE;
}

void
playlist_load_ins_file(Playlist *playlist,
                       const gchar * uri,
                       const gchar * playlist_name, gint pos,
                       const gchar * title, gint len)
{
    ProbeResult *pr = NULL;

    g_return_if_fail(uri != NULL);
    g_return_if_fail(playlist_name != NULL);
    g_return_if_fail(playlist != NULL);

    if(do_precheck(playlist, uri, &pr)) {
        __playlist_ins_file(playlist, uri, pos, NULL, title, len, pr ? pr->ip : NULL);
    }
    g_free(pr);
}

void
playlist_load_ins_file_tuple(Playlist * playlist,
                             const gchar * uri,
                             const gchar * playlist_name,   //path of playlist file itself
                             gint pos,
                             Tuple *tuple)
{
    ProbeResult *pr = NULL;		/* for decoder cache */

    g_return_if_fail(uri != NULL);
    g_return_if_fail(playlist_name != NULL);
    g_return_if_fail(playlist != NULL);

    if(do_precheck(playlist, uri, &pr)) {
        __playlist_ins_file(playlist, uri, pos, tuple, NULL, -1, pr ? pr->ip : NULL);
    }
    g_free(pr);

}

static gboolean
do_precheck(Playlist *playlist, const gchar *uri, ProbeResult **pr)
{
    gint ext_flag = filter_by_extension(uri);
    gboolean http_flag = is_http(uri);
    gboolean rv = FALSE;

    /* playlist file or remote uri */
    if ((playlist->loading_playlist == TRUE && ext_flag != EXT_HAVE_SUBTUNE ) || http_flag == TRUE) {
        pr = NULL;
        rv = TRUE;
    }
    /* local file and on-demand probing is on */
    else if (cfg.playlist_detect == TRUE && ext_flag != EXT_HAVE_SUBTUNE && ext_flag != EXT_CUSTOM) {
        if(cfg.use_extension_probing && ext_flag == EXT_FALSE) {
            AUDDBG("reject %s\n", uri);
            rv = FALSE;
        }
        else {
            pr = NULL;
            rv = TRUE;
        }
    }
    /* find decorder for local file */
    else {
        *pr = input_check_file(uri, TRUE);
        if(*pr) {
            AUDDBG("got pr\n");
            rv = TRUE;
        }
    }

    return rv;
}

static guint
playlist_load_ins(Playlist * playlist, const gchar * filename, gint pos)
{
    PlaylistContainer *plc;
    GList *old_iter;
    gchar *ext;
    gint old_len, new_len;

    g_return_val_if_fail(playlist != NULL, 0);
    g_return_val_if_fail(filename != NULL, 0);

    ext = strrchr(filename, '.') + 1;
    plc = playlist_container_find(ext);

    g_return_val_if_fail(plc != NULL, 0);
    g_return_val_if_fail(plc->plc_read != NULL, 0);

    old_len = playlist_get_length(playlist);
    /* make sure it adds files to the right playlist */
    if (playlist != playlist_get_active()) {
        old_iter = playlists_iter;
        playlists_iter = g_list_find(playlists, playlist);
        if (!playlists_iter) playlists_iter = old_iter;
        plc->plc_read(filename, pos);
        playlists_iter = old_iter;
    } else {
        plc->plc_read(filename, pos);
    }
    new_len = playlist_get_length(playlist);

    playlist_generate_shuffle_list(playlist);
    event_queue("playlist update", playlist);

    playlist_recalc_total_time(playlist); //tentative --yaz
    PLAYLIST_INCR_SERIAL(playlist);

    return new_len - old_len;
}

GList *
get_playlist_nth(Playlist *playlist, guint nth)
{
    g_warning("deprecated function get_playlist_nth() was called");
    REQUIRE_LOCK(playlist->mutex);
    return g_list_nth(playlist->entries, nth);
}

gint
playlist_get_position_nolock(Playlist *playlist)
{
    if (playlist && playlist->position)
        return g_list_index(playlist->entries, playlist->position);
    return 0;
}

gint
playlist_get_position(Playlist *playlist)
{
    gint pos;

    PLAYLIST_LOCK(playlist);
    pos = playlist_get_position_nolock(playlist);
    PLAYLIST_UNLOCK(playlist);

    return pos;
}

gchar *
playlist_get_filename(Playlist *playlist, guint pos)
{
    gchar *filename;
    PlaylistEntry *entry;
    GList *node;

    if (!playlist)
        return NULL;

    PLAYLIST_LOCK(playlist);
    node = g_list_nth(playlist->entries, pos);
    if (!node) {
        PLAYLIST_UNLOCK(playlist);
        return NULL;
    }
    entry = node->data;

    filename = g_strdup(entry->filename);
    PLAYLIST_UNLOCK(playlist);

    return filename;
}

gchar *
playlist_get_songtitle(Playlist *playlist, guint pos)
{
    gchar *title = NULL;
    PlaylistEntry *entry;
    GList *node;
    time_t mtime;

    if (!playlist)
        return NULL;

    PLAYLIST_LOCK(playlist);

    if (!(node = g_list_nth(playlist->entries, pos))) {
        PLAYLIST_UNLOCK(playlist);
        return NULL;
    }

    entry = node->data;

    if (entry->tuple)
        mtime = tuple_get_int(entry->tuple, FIELD_MTIME, NULL);
    else
        mtime = 0;

    /* FIXME: simplify this logic */
    if ((entry->title == NULL && entry->length == -1) ||
        (entry->tuple && mtime != 0 &&
         (mtime == -1 || mtime != playlist_get_mtime(entry->filename))))
    {
        if (playlist_entry_get_info(entry))
            title = entry->title;
    }
    else {
        title = entry->title;
    }

    PLAYLIST_UNLOCK(playlist);

    if (!title) {
        gchar *realfn = NULL;
        realfn = g_filename_from_uri(entry->filename, NULL, NULL);
        title = g_path_get_basename(realfn ? realfn : entry->filename);
        g_free(realfn); realfn = NULL;
        return str_replace(title, filename_to_utf8(title));
    }

    return str_assert_utf8(title);
}

Tuple *
playlist_get_tuple(Playlist *playlist, guint pos)
{
    PlaylistEntry *entry;
    Tuple *tuple = NULL;
    GList *node;
    time_t mtime;

    if (!playlist)
        return NULL;

    if (!(node = g_list_nth(playlist->entries, pos))) {
        return NULL;
    }

    entry = (PlaylistEntry *) node->data;

    tuple = entry->tuple;

    if (tuple)
        mtime = tuple_get_int(tuple, FIELD_MTIME, NULL);
    else
        mtime = 0;

    // if no tuple or tuple with old mtime, get new one.
    if (tuple == NULL || 
        (entry->tuple && mtime != 0 && (mtime == -1 || mtime != playlist_get_mtime(entry->filename))))
    {
        playlist_entry_get_info(entry);
        tuple = entry->tuple;
    }

    return tuple;
}

gint
playlist_get_songtime(Playlist *playlist, guint pos)
{
    gint song_time = -1;
    PlaylistEntry *entry;
    GList *node;
    time_t mtime;

    if (!playlist)
        return -1;

    if (!(node = g_list_nth(playlist->entries, pos)))
        return -1;

    entry = node->data;

    if (entry->tuple)
        mtime = tuple_get_int(entry->tuple, FIELD_MTIME, NULL);
    else
        mtime = 0;

    if (entry->tuple == NULL ||
        (mtime != 0 && (mtime == -1 || mtime != playlist_get_mtime(entry->filename)))) {

        if (playlist_entry_get_info(entry))
            song_time = entry->length;
    } else
        song_time = entry->length;

    return song_time;
}

static gint
playlist_compare_track(PlaylistEntry * a, PlaylistEntry * b)
{
    gint tracknumber_a;
    gint tracknumber_b;

    g_return_val_if_fail(a != NULL, 0);
    g_return_val_if_fail(b != NULL, 0);

    if(!a->tuple)
        playlist_entry_get_info(a);
    if(!b->tuple)
        playlist_entry_get_info(b);

    if (a->tuple == NULL)
        return 0;
    if (b->tuple == NULL)
        return 0;

    tracknumber_a = tuple_get_int(a->tuple, FIELD_TRACK_NUMBER, NULL);
    tracknumber_b = tuple_get_int(b->tuple, FIELD_TRACK_NUMBER, NULL);

    return (tracknumber_a && tracknumber_b ?
            tracknumber_a - tracknumber_b : 0);
}

static void
playlist_get_entry_title(PlaylistEntry * e, const gchar ** title)
{
    if (e->title)
        *title = e->title;
    else {
        if (strrchr(e->filename, '/'))
            *title = strrchr(e->filename, '/') + 1;
        else
            *title = e->filename;
    }
}

static gint
playlist_compare_playlist(PlaylistEntry * a, PlaylistEntry * b)
{
    const gchar *a_title = NULL, *b_title = NULL;

    g_return_val_if_fail(a != NULL, 0);
    g_return_val_if_fail(b != NULL, 0);

    playlist_get_entry_title(a, &a_title);
    playlist_get_entry_title(b, &b_title);

    return strcasecmp(a_title, b_title);
}

static gint
playlist_compare_title(PlaylistEntry * a,
                       PlaylistEntry * b)
{
    const gchar *a_title = NULL, *b_title = NULL;

    g_return_val_if_fail(a != NULL, 0);
    g_return_val_if_fail(b != NULL, 0);

    if (a->tuple == NULL)
        playlist_entry_get_info(a);
    if (b->tuple == NULL)
        playlist_entry_get_info(b);

    if (a->tuple != NULL)
        a_title = tuple_get_string(a->tuple, FIELD_TITLE, NULL);

    if (b->tuple != NULL)
        b_title = tuple_get_string(b->tuple, FIELD_TITLE, NULL);

    if (a_title != NULL && b_title != NULL)
        return strcasecmp(a_title, b_title);

    playlist_get_entry_title(a, &a_title);
    playlist_get_entry_title(b, &b_title);

    return strcasecmp(a_title, b_title);
}

static gint
playlist_compare_artist(PlaylistEntry * a,
                        PlaylistEntry * b)
{
    const gchar *a_artist = NULL, *b_artist = NULL;

    g_return_val_if_fail(a != NULL, 0);
    g_return_val_if_fail(b != NULL, 0);

    if (a->tuple != NULL)
        playlist_entry_get_info(a);

    if (b->tuple != NULL)
        playlist_entry_get_info(b);

    if (a->tuple != NULL) {
        a_artist = tuple_get_string(a->tuple, FIELD_ARTIST, NULL);

        if (a_artist == NULL)
            return 0;

        if (str_has_prefix_nocase(a_artist, "the "))
            a_artist += 4;
    }

    if (b->tuple != NULL) {
        b_artist = tuple_get_string(b->tuple, FIELD_ARTIST, NULL);

        if (b_artist == NULL)
            return 0;

        if (str_has_prefix_nocase(b_artist, "the "))
            b_artist += 4;
    }

    if (a_artist != NULL && b_artist != NULL)
        return strcasecmp(a_artist, b_artist);

    return 0;
}

static gint
playlist_compare_filename(PlaylistEntry * a,
                          PlaylistEntry * b)
{
    gchar *a_filename, *b_filename;

    g_return_val_if_fail(a != NULL, 0);
    g_return_val_if_fail(b != NULL, 0);

    if (strrchr(a->filename, '/'))
        a_filename = strrchr(a->filename, '/') + 1;
    else
        a_filename = a->filename;

    if (strrchr(b->filename, '/'))
        b_filename = strrchr(b->filename, '/') + 1;
    else
        b_filename = b->filename;


    return strcasecmp(a_filename, b_filename);
}

static gint
path_compare(const gchar * a, const gchar * b)
{
    gchar *posa, *posb;
    gint len, ret;

    posa = strrchr(a, '/');
    posb = strrchr(b, '/');

    /*
     * Sort directories before files
     */
    if (posa && posb && (posa - a != posb - b)) {
        if (posa - a > posb - b) {
            len = posb - b;
            ret = -1;
        }
        else {
            len = posa - a;
            ret = 1;
        }
        if (!strncasecmp(a, b, len))
            return ret;
    }
    return strcasecmp(a, b);
}

static gint
playlist_compare_path(PlaylistEntry * a,
                      PlaylistEntry * b)
{
    return path_compare(a->filename, b->filename);
}


static time_t
playlist_get_mtime(const gchar *filename)
{
    struct stat buf;
    gint rv;
    gchar *realfn = NULL;

    /* stat() does not accept file:// --yaz */
    realfn = g_filename_from_uri(filename, NULL, NULL);
    rv = stat(realfn ? realfn : filename, &buf);
    g_free(realfn); realfn = NULL;

    if (rv == 0) {
        return buf.st_mtime;
    } else {
        return 0; //error
    }
}


static gint
playlist_compare_date(PlaylistEntry * a,
                      PlaylistEntry * b)
{
    struct stat buf;
    time_t modtime;

    gint rv;


    rv = stat(a->filename, &buf);

    if (rv == 0) {
        modtime = buf.st_mtime;
        rv = stat(b->filename, &buf);

        if (stat(b->filename, &buf) == 0) {
            if (buf.st_mtime == modtime)
                return 0;
            else
                return (buf.st_mtime - modtime) > 0 ? -1 : 1;
        }
        else
            return -1;
    }
    else if (!lstat(b->filename, &buf))
        return 1;
    else
        return playlist_compare_filename(a, b);
}


void
playlist_sort(Playlist *playlist, PlaylistSortType type)
{
    playlist_remove_dead_files(playlist);
    PLAYLIST_LOCK(playlist);
    playlist->entries =
        g_list_sort(playlist->entries,
                    (GCompareFunc) playlist_compare_func_table[type]);
    playlist->tail = g_list_last(playlist->entries);
    PLAYLIST_INCR_SERIAL(playlist);
    PLAYLIST_UNLOCK(playlist);
}

static GList *
playlist_sort_selected_generic(GList * list, GCompareFunc cmpfunc)
{
    GList *list1, *list2;
    GList *tmp_list = NULL;
    GList *index_list = NULL;

    /*
     * We take all the selected entries out of the playlist,
     * sorts them, and then put them back in again.
     */

    list1 = g_list_last(list);

    while (list1) {
        list2 = g_list_previous(list1);
        if (PLAYLIST_ENTRY(list1->data)->selected) {
            gpointer idx;
            idx = GINT_TO_POINTER(g_list_position(list, list1));
            index_list = g_list_prepend(index_list, idx);
            list = g_list_remove_link(list, list1);
            tmp_list = g_list_concat(list1, tmp_list);
        }
        list1 = list2;
    }

    tmp_list = g_list_sort(tmp_list, cmpfunc);
    list1 = tmp_list;
    list2 = index_list;

    while (list2) {
        if (!list1) {
            g_critical(G_STRLOC ": Error during list sorting. "
                       "Possibly dropped some playlist-entries.");
            break;
        }

        list = g_list_insert(list, list1->data, GPOINTER_TO_INT(list2->data));

        list2 = g_list_next(list2);
        list1 = g_list_next(list1);
    }

    g_list_free(index_list);
    g_list_free(tmp_list);

    return list;
}

void
playlist_sort_selected(Playlist *playlist, PlaylistSortType type)
{
    PLAYLIST_LOCK(playlist);
    playlist->entries = playlist_sort_selected_generic(playlist->entries, (GCompareFunc)
                                                       playlist_compare_func_table
                                                       [type]);
    playlist->tail = g_list_last(playlist->entries);
    PLAYLIST_INCR_SERIAL(playlist);
    PLAYLIST_UNLOCK(playlist);
}

void
playlist_reverse(Playlist *playlist)
{
    PLAYLIST_LOCK(playlist);
    playlist->entries = g_list_reverse(playlist->entries);
    playlist->tail = g_list_last(playlist->entries);
    PLAYLIST_INCR_SERIAL(playlist);
    PLAYLIST_UNLOCK(playlist);
}

static GList *
playlist_shuffle_list(Playlist *playlist, GList * list)
{
    /*
     * Note that this doesn't make a copy of the original list.
     * The pointer to the original list is not valid after this
     * fuction is run.
     */
    gint len = g_list_length(list);
    gint i, j;
    GList *node, **ptrs;

    if (!playlist)
        return NULL;

    REQUIRE_LOCK(playlist->mutex);

    if (!len)
        return NULL;

    ptrs = g_new(GList *, len);

    for (node = list, i = 0; i < len; node = g_list_next(node), i++)
        ptrs[i] = node;

    j = g_random_int_range(0, len);
    list = ptrs[j];
    ptrs[j]->next = NULL;
    ptrs[j] = ptrs[0];

    for (i = 1; i < len; i++) {
        j = g_random_int_range(0, len - i);
        list->prev = ptrs[i + j];
        ptrs[i + j]->next = list;
        list = ptrs[i + j];
        ptrs[i + j] = ptrs[i];
    }
    list->prev = NULL;

    g_free(ptrs);

    return list;
}

void
playlist_random(Playlist *playlist)
{
    PLAYLIST_LOCK(playlist);
    playlist->entries = playlist_shuffle_list(playlist, playlist->entries);
    playlist->tail = g_list_last(playlist->entries);
    PLAYLIST_INCR_SERIAL(playlist);
    PLAYLIST_UNLOCK(playlist);
}

GList *
playlist_get_selected(Playlist *playlist)
{
    GList *node, *list = NULL;
    gint i = 0;

    PLAYLIST_LOCK(playlist);
    for (node = playlist->entries; node; node = g_list_next(node), i++) {
        PlaylistEntry *entry = node->data;
        if (entry->selected)
            list = g_list_prepend(list, GINT_TO_POINTER(i));
    }
    PLAYLIST_UNLOCK(playlist);
    return g_list_reverse(list);
}

void
playlist_clear_selected(Playlist *playlist)
{
    GList *node = NULL;
    gint i = 0;

    PLAYLIST_LOCK(playlist);
    for (node = playlist->entries; node; node = g_list_next(node), i++) {
        PLAYLIST_ENTRY(node->data)->selected = FALSE;
    }
    PLAYLIST_INCR_SERIAL(playlist);
    PLAYLIST_UNLOCK(playlist);
    playlist_recalc_total_time(playlist);
    event_queue("playlist update", playlist);
}

gint
playlist_get_num_selected(Playlist *playlist)
{
    GList *node;
    gint num = 0;

    PLAYLIST_LOCK(playlist);
    for (node = playlist->entries; node; node = g_list_next(node)) {
        PlaylistEntry *entry = node->data;
        if (entry->selected)
            num++;
    }
    PLAYLIST_UNLOCK(playlist);
    return num;
}


static void
playlist_generate_shuffle_list(Playlist *playlist)
{
    PLAYLIST_LOCK(playlist);
    playlist_generate_shuffle_list_nolock(playlist);
    PLAYLIST_UNLOCK(playlist);
}

static void
playlist_generate_shuffle_list_nolock(Playlist *playlist)
{
    GList *node;
    gint numsongs;

    if (!cfg.shuffle || !playlist)
        return;

    REQUIRE_LOCK(playlist->mutex);

    if (playlist->shuffle) {
        g_list_free(playlist->shuffle);
        playlist->shuffle = NULL;
    }

    playlist->shuffle = playlist_shuffle_list(playlist, g_list_copy(playlist->entries));
    numsongs = g_list_length(playlist->shuffle);

    if (playlist->position) {
        gint i = g_list_index(playlist->shuffle, playlist->position);
        node = g_list_nth(playlist->shuffle, i);
        playlist->shuffle = g_list_remove_link(playlist->shuffle, node);
        playlist->shuffle = g_list_prepend(playlist->shuffle, node->data);
    }
}


static gboolean
playlist_get_info_is_going(void)
{
    gboolean result;

    g_static_rw_lock_reader_lock(&playlist_get_info_rwlock);
    result = playlist_get_info_going;
    g_static_rw_lock_reader_unlock(&playlist_get_info_rwlock);

    return result;
}


static gpointer
playlist_get_info_func(gpointer arg)
{
    GList *node;
    gboolean update_playlistwin = FALSE;

    while (playlist_get_info_is_going()) {
        PlaylistEntry *entry;
        Playlist *playlist = playlist_get_active();

        if (cfg.use_pl_metadata &&
            playlist_get_info_scan_active) {

            for (node = playlist->entries; node; node = g_list_next(node)) {
                entry = node->data;

                if (playlist->attribute & PLAYLIST_STATIC || // live lock fix
                    (entry->tuple &&
                     tuple_get_int(entry->tuple, FIELD_LENGTH, NULL) > -1 &&
                     tuple_get_int(entry->tuple, FIELD_MTIME, NULL) != -1 &&
                     entry->title_is_valid))
                {
                    update_playlistwin = TRUE;
                    continue;
                }

                if (!playlist_entry_get_info(entry) && 
                    g_list_index(playlist->entries, entry) == -1)
                    /* Entry disappeared while we looked it up.
                       Restart. */
                    node = playlist->entries;
                else if ((entry->tuple != NULL) ||
			 (entry->title != NULL && 
                         tuple_get_int(entry->tuple, FIELD_LENGTH, NULL) > -1 &&
                         tuple_get_int(entry->tuple, FIELD_MTIME, NULL) != -1))
                {
                    update_playlistwin = FALSE;
                    playlist_get_info_scan_active = FALSE;
                    break; /* hmmm... --asphyx */
                }
            }

            if (!node) {
                g_mutex_lock(mutex_scan);
                playlist_get_info_scan_active = FALSE;
                g_mutex_unlock(mutex_scan);
            }
        } // on_load

        if (update_playlistwin) {
            Playlist *playlist = playlist_get_active();
            event_queue("playlist update", playlist);
            PLAYLIST_INCR_SERIAL(playlist);
            update_playlistwin = FALSE;
        }

        if (playlist_get_info_scan_active)
            continue;

        g_mutex_lock(mutex_scan);
        g_cond_wait(cond_scan, mutex_scan);
        g_mutex_unlock(mutex_scan);

        // AUDDBG("scanner invoked\n");

    } // while

    g_thread_exit(NULL);
    return NULL;
}

void
playlist_start_get_info_thread(void)
{
    g_static_rw_lock_writer_lock(&playlist_get_info_rwlock);
    playlist_get_info_going = TRUE;
    g_static_rw_lock_writer_unlock(&playlist_get_info_rwlock);

    playlist_get_info_thread = g_thread_create(playlist_get_info_func,
                                               NULL, TRUE, NULL);
}

void
playlist_stop_get_info_thread(void)
{
    g_static_rw_lock_writer_lock(&playlist_get_info_rwlock);
    if (!playlist_get_info_going) {
        g_static_rw_lock_writer_unlock(&playlist_get_info_rwlock);
        return;
    }
    
    playlist_get_info_going = FALSE;
    g_static_rw_lock_writer_unlock(&playlist_get_info_rwlock);

    g_cond_broadcast(cond_scan);
    g_thread_join(playlist_get_info_thread);
}

void
playlist_start_get_info_scan(void)
{
    AUDDBG("waking up scan thread\n");
    g_mutex_lock(mutex_scan);
    playlist_get_info_scan_active = TRUE;
    g_mutex_unlock(mutex_scan);

    g_cond_signal(cond_scan);
}

void
playlist_update_all_titles(void) /* update titles after format changing --asphyx */
{
    PlaylistEntry *entry;
    GList *node;
    Playlist *playlist = playlist_get_active();

    AUDDBG("invalidating titles\n");
    PLAYLIST_LOCK(playlist);
    for (node = playlist->entries; node; node = g_list_next(node)) {
        entry = node->data;
        entry->title_is_valid = FALSE;
    }
    PLAYLIST_UNLOCK(playlist);
    playlist_start_get_info_scan();
}

void
playlist_remove_dead_files(Playlist *playlist)
{
    GList *node, *next_node;

    PLAYLIST_LOCK(playlist);

    for (node = playlist->entries; node; node = next_node) {
        PlaylistEntry *entry = PLAYLIST_ENTRY(node->data);
        next_node = g_list_next(node);

        if (!entry || !entry->filename) {
            g_message(G_STRLOC ": Playlist entry is invalid!");
            continue;
        }

        if (!g_str_has_prefix(entry->filename, "file://"))
            continue;

        /* FIXME: Should test for readability */
        if (vfs_file_test(entry->filename, G_FILE_TEST_EXISTS))  
            continue;

        if (entry == playlist->position) {
            /* Don't remove the currently playing song */
            if (playback_get_playing())
                continue;

            if (next_node)
                playlist->position = PLAYLIST_ENTRY(next_node->data);
            else
                playlist->position = NULL;
        }

        playlist_entry_free(entry);
        playlist->entries = g_list_delete_link(playlist->entries, node);
    }

    PLAYLIST_UNLOCK(playlist);

    playlist_generate_shuffle_list(playlist);
    event_queue("playlist update", playlist);
    playlist_recalc_total_time(playlist);
    PLAYLIST_INCR_SERIAL(playlist);
}


static gint
playlist_dupscmp_title(PlaylistEntry * a,
                       PlaylistEntry * b)
{
    const gchar *a_title, *b_title;

    g_return_val_if_fail(a != NULL, 0);
    g_return_val_if_fail(b != NULL, 0);

    playlist_get_entry_title(a, &a_title);
    playlist_get_entry_title(b, &b_title);

    return strcmp(a_title, b_title);
}

static gint
playlist_dupscmp_filename(PlaylistEntry * a,
                          PlaylistEntry * b )
{
    gchar *a_filename, *b_filename;

    g_return_val_if_fail(a != NULL, 0);
    g_return_val_if_fail(b != NULL, 0);

    if (strrchr(a->filename, '/'))
        a_filename = strrchr(a->filename, '/') + 1;
    else
        a_filename = a->filename;

    if (strrchr(b->filename, '/'))
        b_filename = strrchr(b->filename, '/') + 1;
    else
        b_filename = b->filename;

    return strcmp(a_filename, b_filename);
}

static gint
playlist_dupscmp_path(PlaylistEntry * a,
                      PlaylistEntry * b)
{
    /* simply compare the entire filename string */
    return strcmp(a->filename, b->filename);
}

void
playlist_remove_duplicates(Playlist *playlist, PlaylistDupsType type)
{
    GList *node, *next_node;
    GList *node_cmp, *next_node_cmp;
    gint (*dups_compare_func)(PlaylistEntry * , PlaylistEntry *);

    switch ( type )
    {
        case PLAYLIST_DUPS_TITLE:
            dups_compare_func = playlist_dupscmp_title;
            break;
        case PLAYLIST_DUPS_PATH:
            dups_compare_func = playlist_dupscmp_path;
            break;
        case PLAYLIST_DUPS_FILENAME:
        default:
            dups_compare_func = playlist_dupscmp_filename;
            break;
    }

    PLAYLIST_LOCK(playlist);

    for (node = playlist->entries; node; node = next_node) {
        PlaylistEntry *entry = PLAYLIST_ENTRY(node->data);
        next_node = g_list_next(node);

        if (!entry || !entry->filename) {
            g_message(G_STRLOC ": Playlist entry is invalid!");
            continue;
        }

        for (node_cmp = next_node; node_cmp; node_cmp = next_node_cmp) {
            PlaylistEntry *entry_cmp = PLAYLIST_ENTRY(node_cmp->data);
            next_node_cmp = g_list_next(node_cmp);

            if (!entry_cmp || !entry_cmp->filename) {
                g_message(G_STRLOC ": Playlist entry is invalid!");
                continue;
            }

            /* compare using the chosen dups_compare_func */
            if ( !dups_compare_func( entry , entry_cmp ) ) {

                if (entry_cmp == playlist->position) {
                    /* Don't remove the currently playing song */
                    if (playback_get_playing())
                        continue;

                    if (next_node_cmp)
                        playlist->position = PLAYLIST_ENTRY(next_node_cmp->data);
                    else
                        playlist->position = NULL;
                }

                /* check if this was the next item of the external
                   loop; if true, replace it with the next of the next*/
                if ( node_cmp == next_node )
                    next_node = g_list_next(next_node);

                playlist_entry_free(entry_cmp);
                playlist->entries = g_list_delete_link(playlist->entries, node_cmp);
            }
        }
    }

    PLAYLIST_UNLOCK(playlist);

    event_queue("playlist update", playlist);
    playlist_recalc_total_time(playlist);
    PLAYLIST_INCR_SERIAL(playlist);
}

void
playlist_get_total_time(Playlist * playlist,
                        gulong * total_time,
                        gulong * selection_time,
                        gboolean * total_more,
                        gboolean * selection_more)
{
    PLAYLIST_LOCK(playlist);
    *total_time = playlist->pl_total_time;
    *selection_time = playlist->pl_selection_time;
    *total_more = playlist->pl_total_more;
    *selection_more = playlist->pl_selection_more;
    PLAYLIST_UNLOCK(playlist);
}

static void
playlist_recalc_total_time_nolock(Playlist *playlist)
{
    GList *list;
    PlaylistEntry *entry;

    REQUIRE_LOCK(playlist->mutex);

    playlist->pl_total_time = 0;
    playlist->pl_selection_time = 0;
    playlist->pl_total_more = FALSE;
    playlist->pl_selection_more = FALSE;

    for (list = playlist->entries; list; list = g_list_next(list)) {
        entry = list->data;

        if (entry->length != -1)
            playlist->pl_total_time += entry->length / 1000;
        else
            playlist->pl_total_more = TRUE;

        if (entry->selected) {
            if (entry->length != -1)
                playlist->pl_selection_time += entry->length / 1000;
            else
                playlist->pl_selection_more = TRUE;
        }
    }
}

static void
playlist_recalc_total_time(Playlist *playlist)
{
    PLAYLIST_LOCK(playlist);
    playlist_recalc_total_time_nolock(playlist);
    PLAYLIST_UNLOCK(playlist);
}

gint
playlist_select_search( Playlist *playlist , Tuple *tuple , gint action )
{
    GList *entry_list = NULL, *found_list = NULL, *sel_list = NULL;
    gboolean is_first_search = TRUE;
    gint num_of_entries_found = 0;
    const gchar *regex_pattern;
    const gchar *track_name;
    const gchar *album_name;
    const gchar *performer;
    const gchar *file_name;

#if defined(USE_REGEX_ONIGURUMA)
    /* set encoding for Oniguruma regex to UTF-8 */
    reg_set_encoding( REG_POSIX_ENCODING_UTF8 );
    onig_set_default_syntax( ONIG_SYNTAX_POSIX_BASIC );
#endif

    PLAYLIST_LOCK(playlist);

    if ( (regex_pattern = tuple_get_string(tuple, FIELD_TITLE, NULL)) != NULL &&
         (*regex_pattern != '\0') )
    {
        /* match by track_name */
        regex_t regex;
#if defined(USE_REGEX_PCRE)
        if ( regcomp( &regex , regex_pattern , REG_NOSUB | REG_ICASE | REG_UTF8 ) == 0 )
#else
            if ( regcomp( &regex , regex_pattern , REG_NOSUB | REG_ICASE ) == 0 )
#endif
            {
                GList *tfound_list = NULL;
                if ( is_first_search == TRUE ) entry_list = playlist->entries;
                else entry_list = found_list; /* use found_list */
                for ( ; entry_list ; entry_list = g_list_next(entry_list) )
                {
                    PlaylistEntry *entry = entry_list->data;
                    if ( entry->tuple != NULL )
                    {
                        track_name = tuple_get_string( entry->tuple, FIELD_TITLE, NULL );
                        if (( track_name != NULL ) &&
                            ( regexec( &regex , track_name , 0 , NULL , 0 ) == 0 ) )
                        {
                            tfound_list = g_list_append( tfound_list , entry );
                        }
                    }
                }
                g_list_free( found_list ); /* wipe old found_list */
                found_list = tfound_list; /* move tfound_list in found_list */
                regfree( &regex );
            }
        is_first_search = FALSE;
    }

    if ( (regex_pattern = tuple_get_string(tuple, FIELD_ALBUM, NULL)) != NULL &&
         (*regex_pattern != '\0') )
    {
        /* match by album_name */
        regex_t regex;
#if defined(USE_REGEX_PCRE)
        if ( regcomp( &regex , regex_pattern , REG_NOSUB | REG_ICASE | REG_UTF8 ) == 0 )
#else
            if ( regcomp( &regex , regex_pattern , REG_NOSUB | REG_ICASE ) == 0 )
#endif
            {
                GList *tfound_list = NULL;
                if ( is_first_search == TRUE ) entry_list = playlist->entries;
                else entry_list = found_list; /* use found_list */
                for ( ; entry_list ; entry_list = g_list_next(entry_list) )
                {
                    PlaylistEntry *entry = entry_list->data;
                    if ( entry->tuple != NULL )
                    {
                        album_name = tuple_get_string( entry->tuple, FIELD_ALBUM, NULL );
                        if ( ( album_name != NULL ) &&
                             ( regexec( &regex , album_name , 0 , NULL , 0 ) == 0 ) )
                        {
                            tfound_list = g_list_append( tfound_list , entry );
                        }
                    }
                }
                g_list_free( found_list ); /* wipe old found_list */
                found_list = tfound_list; /* move tfound_list in found_list */
                regfree( &regex );
            }
        is_first_search = FALSE;
    }

    if ( (regex_pattern = tuple_get_string(tuple, FIELD_ARTIST, NULL)) != NULL &&
         (*regex_pattern != '\0') )
    {
        /* match by performer */
        regex_t regex;
#if defined(USE_REGEX_PCRE)
        if ( regcomp( &regex , regex_pattern , REG_NOSUB | REG_ICASE | REG_UTF8 ) == 0 )
#else
            if ( regcomp( &regex , regex_pattern , REG_NOSUB | REG_ICASE ) == 0 )
#endif
            {
                GList *tfound_list = NULL;
                if ( is_first_search == TRUE ) entry_list = playlist->entries;
                else entry_list = found_list; /* use found_list */
                for ( ; entry_list ; entry_list = g_list_next(entry_list) )
                {
                    PlaylistEntry *entry = entry_list->data;
                    if ( entry->tuple != NULL )
                    {
                        performer = tuple_get_string( entry->tuple, FIELD_ARTIST, NULL );
                        if ( ( entry->tuple != NULL ) && ( performer != NULL ) &&
                             ( regexec( &regex , performer , 0 , NULL , 0 ) == 0 ) )
                        {
                            tfound_list = g_list_append( tfound_list , entry );
                        }
                    }
                }
                g_list_free( found_list ); /* wipe old found_list */
                found_list = tfound_list; /* move tfound_list in found_list */
                regfree( &regex );
            }
        is_first_search = FALSE;
    }

    if ( (regex_pattern = tuple_get_string(tuple, FIELD_FILE_NAME, NULL)) != NULL &&
         (*regex_pattern != '\0') )
    {
        /* match by file_name */
        regex_t regex;
#if defined(USE_REGEX_PCRE)
        if ( regcomp( &regex , regex_pattern , REG_NOSUB | REG_ICASE | REG_UTF8 ) == 0 )
#else
            if ( regcomp( &regex , regex_pattern , REG_NOSUB | REG_ICASE ) == 0 )
#endif
            {
                GList *tfound_list = NULL;
                if ( is_first_search == TRUE ) entry_list = playlist->entries;
                else entry_list = found_list; /* use found_list */
                for ( ; entry_list ; entry_list = g_list_next(entry_list) )
                {
                    PlaylistEntry *entry = entry_list->data;
                    if ( entry->tuple != NULL )
                    {
                        file_name = tuple_get_string( entry->tuple, FIELD_FILE_NAME, NULL );
                        if ( ( file_name != NULL ) &&
                             ( regexec( &regex , file_name , 0 , NULL , 0 ) == 0 ) )
                        {
                            tfound_list = g_list_append( tfound_list , entry );
                        }
                    }
                }
                g_list_free( found_list ); /* wipe old found_list */
                found_list = tfound_list; /* move tfound_list in found_list */
                regfree( &regex );
            }
        is_first_search = FALSE;
    }

    /* NOTE: action = 0 -> default behaviour, select all matching entries */
    /* if some entries are still in found_list, those
       are what the user is searching for; select them */
    for ( sel_list = found_list ; sel_list ; sel_list = g_list_next(sel_list) )
    {
        PlaylistEntry *entry = sel_list->data;
        entry->selected = TRUE;
        num_of_entries_found++;
    }

    g_list_free( found_list );

    PLAYLIST_UNLOCK(playlist);
    playlist_recalc_total_time(playlist);
    //    PLAYLIST_INCR_SERIAL(playlist); //unnecessary? --yaz

    return num_of_entries_found;
}

void
playlist_select_all(Playlist *playlist, gboolean set)
{
    GList *list;

    PLAYLIST_LOCK(playlist);

    for (list = playlist->entries; list; list = g_list_next(list)) {
        PlaylistEntry *entry = list->data;
        entry->selected = set;
    }

    PLAYLIST_UNLOCK(playlist);
    playlist_recalc_total_time(playlist);
}

void
playlist_select_invert_all(Playlist *playlist)
{
    GList *list;

    PLAYLIST_LOCK(playlist);

    for (list = playlist->entries; list; list = g_list_next(list)) {
        PlaylistEntry *entry = list->data;
        entry->selected = !entry->selected;
    }

    PLAYLIST_UNLOCK(playlist);
    playlist_recalc_total_time(playlist);
}

gboolean
playlist_select_invert(Playlist *playlist, guint pos)
{
    GList *list;
    gboolean invert_ok = FALSE;

    PLAYLIST_LOCK(playlist);

    if ((list = g_list_nth(playlist->entries, pos))) {
        PlaylistEntry *entry = list->data;
        entry->selected = !entry->selected;
        invert_ok = TRUE;
    }

    PLAYLIST_UNLOCK(playlist);
    playlist_recalc_total_time(playlist);

    return invert_ok;
}


void
playlist_select_range(Playlist *playlist, gint min_pos, gint max_pos, gboolean select)
{
    GList *list;
    gint i;

    if (min_pos > max_pos)
        SWAP(min_pos, max_pos);

    PLAYLIST_LOCK(playlist);

    list = g_list_nth(playlist->entries, min_pos);
    for (i = min_pos; i <= max_pos && list; i++) {
        PlaylistEntry *entry = list->data;
        entry->selected = select;
        list = g_list_next(list);
    }

    PLAYLIST_UNLOCK(playlist);

    playlist_recalc_total_time(playlist);
}

gboolean
playlist_read_info_selection(Playlist *playlist)
{
    GList *node;
    gboolean retval = FALSE;

    PLAYLIST_LOCK(playlist);

    for (node = playlist->entries; node; node = g_list_next(node)) {
        PlaylistEntry *entry = node->data;
        if (!entry->selected)
            continue;

        retval = TRUE;

        str_replace_in(&entry->title, NULL);
        entry->length = -1;

        /* invalidate mtime to reread */
        if (entry->tuple != NULL)
            tuple_associate_int(entry->tuple, FIELD_MTIME, NULL, -1); /* -1 denotes "non-initialized". now 0 is for stream etc. yaz */

        if (!playlist_entry_get_info(entry)) {
            if (g_list_index(playlist->entries, entry) == -1)
                /* Entry disappeared while we looked it up. Restart. */
                node = playlist->entries;
        }
    }

    PLAYLIST_UNLOCK(playlist);

    event_queue("playlist update", playlist);
    playlist_recalc_total_time(playlist);
    PLAYLIST_INCR_SERIAL(playlist); //tentative --yaz

    return retval;
}

void
playlist_read_info(Playlist *playlist, guint pos)
{
    GList *node;

    PLAYLIST_LOCK(playlist);

    if ((node = g_list_nth(playlist->entries, pos))) {
        PlaylistEntry *entry = node->data;
        str_replace_in(&entry->title, NULL);
        entry->length = -1;
        playlist_entry_get_info(entry);
    }

    PLAYLIST_UNLOCK(playlist);

    event_queue("playlist update", playlist);
    playlist_recalc_total_time(playlist);
    PLAYLIST_INCR_SERIAL(playlist); //tentative --yaz
}

Playlist *
playlist_get_active(void)
{
    if (playlists_iter != NULL)
        return (Playlist *) playlists_iter->data;

    if (playlists)
        return (Playlist *) playlists->data;

    return NULL;
}

void
playlist_set_shuffle(gboolean shuffle)
{
    Playlist *playlist = playlist_get_active();
    if (!playlist)
        return;

    PLAYLIST_LOCK(playlist);

    playlist_position_before_jump = NULL;

    cfg.shuffle = shuffle;
    playlist_generate_shuffle_list_nolock(playlist);

    PLAYLIST_UNLOCK(playlist);
}

Playlist *
playlist_new(void)
{
    Playlist *playlist = g_new0(Playlist, 1);
    playlist->mutex = g_mutex_new();
    playlist->loading_playlist = FALSE;
    playlist->title = g_strdup(_("Untitled Playlist"));
    playlist->filename = NULL;
    playlist_clear(playlist);
    playlist->tail = NULL;
    playlist->attribute = PLAYLIST_PLAIN;
    playlist->serial = 0;

    return playlist;
}

void
playlist_free(Playlist *playlist)
{
    if (!playlist)
        return;

    if (playlist->filename)
        g_free( playlist->filename );
    g_mutex_free( playlist->mutex );
    g_free( playlist );
    playlist = NULL; //XXX lead to crash? --yaz
}

Playlist *
playlist_new_from_selected(void)
{
    Playlist *newpl = playlist_new();
    Playlist *playlist = playlist_get_active();
    GList *list = playlist_get_selected(playlist);

    playlist_add_playlist( newpl );

    PLAYLIST_LOCK(playlist);

    while ( list != NULL )
    {
        PlaylistEntry *entry = g_list_nth_data(playlist->entries, GPOINTER_TO_INT(list->data));
        if ( entry->filename != NULL ) /* paranoid? oh well... */
            playlist_add( newpl , entry->filename );
        list = g_list_next(list);
    }

    PLAYLIST_UNLOCK(playlist);

    playlist_recalc_total_time(newpl);
    event_queue("playlist update", playlist);

    return newpl;
}

const gchar *
playlist_get_filename_to_play(Playlist *playlist)
{
    const gchar *filename = NULL;

    if (!playlist)
        return NULL;

    PLAYLIST_LOCK(playlist);

    if (!playlist->position) {
        if (cfg.shuffle)
            playlist->position = playlist->shuffle->data;
        else
            playlist->position = playlist->entries->data;
    }

    filename = playlist->position->filename;

    PLAYLIST_UNLOCK(playlist);

    return filename;
}

PlaylistEntry *
playlist_get_entry_to_play(Playlist *playlist)
{
    if (!playlist)
        return NULL;

    PLAYLIST_LOCK(playlist);

    if (!playlist->position) {
        if (cfg.shuffle)
            playlist->position = playlist->shuffle->data;
        else
            playlist->position = playlist->entries->data;
    }

    PLAYLIST_UNLOCK(playlist);

    return playlist->position;
}

gboolean
playlist_playlists_equal(Playlist *p1, Playlist *p2)
{
    GList *l1, *l2;
    PlaylistEntry *e1, *e2;
    if (!p1 || !p2) return FALSE;
    l1 = p1->entries;
    l2 = p2->entries;
    do {
        if (!l1 && !l2) break;
        if (!l1 || !l2) return FALSE; /* different length */
        e1 = (PlaylistEntry *) l1->data;
        e2 = (PlaylistEntry *) l2->data;
        if (strcmp(e1->filename, e2->filename) != 0) return FALSE;
        l1 = l1->next;
        l2 = l2->next;
    } while(1);
    return TRUE;
}

static gint
filter_by_extension(const gchar *uri)
{
    gchar *base, *ext, *lext, *filename, *tmp_uri;
    gchar *tmp;
    gint rv;
    GList **lhandle, *node;
    InputPlugin *ip;

    g_return_val_if_fail(uri != NULL, EXT_FALSE);

    /* Some URIs will end in ?<subsong> to determine the subsong requested. */
    tmp_uri = g_strdup(uri);
    tmp = strrchr(tmp_uri, '?');

    if (tmp != NULL && g_ascii_isdigit(*(tmp + 1)))
        *tmp = '\0';

    /* Check for plugins with custom URI:// strings */
    /* cue:// cdda:// tone:// tact:// */
    if ((ip = uri_get_plugin(tmp_uri)) != NULL && ip->enabled) {
        g_free(tmp_uri);
        return EXT_CUSTOM;
    }

    tmp = g_filename_from_uri(tmp_uri, NULL, NULL);
    filename = g_strdup(tmp ? tmp : tmp_uri);
    g_free(tmp); tmp = NULL;
    g_free(tmp_uri); tmp_uri = NULL;


    base = g_path_get_basename(filename);
    g_free(filename);
    ext = strrchr(base, '.');

    if(!ext) {
        g_free(base);
        return EXT_FALSE;
    }

    lext = g_ascii_strdown(ext+1, -1);
    g_free(base);

    lhandle = g_hash_table_lookup(ext_hash, lext);
    g_free(lext);

    if(!lhandle) {
        return EXT_FALSE;
    }

    for(node = *lhandle; node; node = g_list_next(node)) {
        ip = (InputPlugin *)node->data;

        if(ip->have_subtune == TRUE) {
            return EXT_HAVE_SUBTUNE;
        }
        else
            rv = EXT_TRUE;
    }

    return rv;
}

static gboolean
is_http(const gchar *uri)
{
    gboolean rv = FALSE;

    if(str_has_prefix_nocase(uri, "http://") ||
       str_has_prefix_nocase(uri, "https://")) {
        rv = TRUE;
    }

    return rv;
}

const gchar *
get_gentitle_format(void)
{
    guint titlestring_preset = cfg.titlestring_preset;

    if (titlestring_preset < n_titlestring_presets)
        return aud_titlestring_presets[titlestring_preset];

    return cfg.gentitle_format;
}
