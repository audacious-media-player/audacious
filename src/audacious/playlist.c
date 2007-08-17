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

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include "playlist.h"

#include <mowgli.h>
#include <glib.h>
#include <glib/gprintf.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/errno.h>

#if defined(USE_REGEX_ONIGURUMA)
  #include <onigposix.h>
#elif defined(USE_REGEX_PCRE)
  #include <pcreposix.h>
#else
  #include <regex.h>
#endif

#include "input.h"
#include "main.h"
#include "ui_main.h"
#include "util.h"
#include "configdb.h"
#include "vfs.h"
#include "ui_equalizer.h"
#include "playback.h"
#include "playlist.h"
#include "playlist_container.h"
#include "ui_playlist_manager.h"
#include "ui_playlist.h"
#include "strings.h"
#include "ui_fileinfo.h"

#include "debug.h"

#include "hook.h"

#include "playlist_evmessages.h"
#include "playlist_evlisteners.h"
#include "ui_skinned_playlist.h"

typedef gint (*PlaylistCompareFunc) (PlaylistEntry * a, PlaylistEntry * b);
typedef void (*PlaylistSaveFunc) (FILE * file);

/* If we manually change the song, p_p_b_j will show us where to go back to */
PlaylistEntry *playlist_position_before_jump = NULL;

static GList *playlists = NULL;
static GList *playlists_iter;

/* If this is set to TRUE, we do not probe upon playlist add.
 *
 * Under Audacious 0.1.x, this was not a big deal because we used 
 * file extension introspection instead of looking for file format magic
 * strings.
 *
 * Because we use file magic strings, we have to fstat a file being added
 * to a playlist up to 1 * <number of input plugins installed> times.
 *
 * This can get really slow now that we're looking for files to add to a
 * playlist. (Up to 5 minutes for 5000 songs, etcetera.)
 *
 * So, we obviously don't want to probe while opening a large playlist 
 * up. Hince the boolean below.
 *
 * January 7, 2006, William Pitcock <nenolod@nenolod.net>
 */

G_LOCK_DEFINE(playlist_get_info_going);

//static gchar *playlist_current_name = NULL;

static gboolean playlist_get_info_scan_active = FALSE;
static gboolean playlist_get_info_going = FALSE;
static GThread *playlist_get_info_thread;

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
    entry->title = str_to_utf8(title);
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

    if (entry->filename != NULL)
        g_free(entry->filename);

    if (entry->title != NULL)
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

    if (entry->tuple == NULL || tuple_get_int(entry->tuple, "mtime") > 0 || tuple_get_int(entry->tuple, "mtime") == -1)
        modtime = playlist_get_mtime(entry->filename);
    else
        modtime = 0;  /* URI -nenolod */

    if (str_has_prefix_nocase(entry->filename, "http:") &&
        g_thread_self() != playlist_get_info_thread)
        return FALSE;

    if (entry->decoder == NULL)
    {
        pr = input_check_file(entry->filename, FALSE);
        if (pr)
           entry->decoder = pr->ip;
    }

    /* renew tuple if file mtime is newer than tuple mtime. */
    if(entry->tuple){
        if(tuple_get_int(entry->tuple, "mtime") == modtime)
            return TRUE;
        else {
            mowgli_object_unref(entry->tuple);
            entry->tuple = NULL;
        }
    }

    if (pr != NULL && pr->tuple != NULL)
        tuple = pr->tuple;
    else if (entry->decoder != NULL && entry->decoder->get_song_tuple != NULL)
        tuple = entry->decoder->get_song_tuple(entry->filename);

    if (tuple == NULL)
        return FALSE;

    /* attach mtime */
    tuple_associate_int(tuple, "mtime", modtime);

    /* entry is still around */
    formatter = tuple_get_string(tuple, "formatter");
    entry->title = tuple_formatter_make_title_string(tuple, formatter ?
                                                  formatter : get_gentitle_format());
    entry->length = tuple_get_int(tuple, "length");
    entry->tuple = tuple;

    g_free(pr);

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

    playlist_evlistener_init();
}

void
playlist_add_playlist(Playlist *playlist)
{
    playlists = g_list_append(playlists, playlist);

    if (playlists_iter == NULL)
        playlists_iter = playlists;

    playlist_manager_update();
}

void
playlist_remove_playlist(Playlist *playlist)
{
    /* users suppose playback will be stopped on removing playlist */
    if (playback_get_playing()) {
        ip_data.stop = TRUE;
        playback_stop();
        ip_data.stop = FALSE;
        mainwin_clear_song_info();
    }

    /* trying to free the last playlist simply clears and resets it */
    if (g_list_length(playlists) < 2) {
        playlist_clear(playlist);
        playlist_set_current_name(playlist, NULL);
        return;
    }

    if (playlist == playlist_get_active())
        playlist_select_next();

    /* upon removal, a playlist should be cleared and freed */
    playlists = g_list_remove(playlists, playlist);
    playlist_clear(playlist);
    playlist_free(playlist);

    if (playlists_iter == NULL)
        playlists_iter = playlists;

    playlist_manager_update();
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

    playlistwin_update_list(playlist_get_active());
}

void
playlist_select_prev(void)
{
    if (playlists_iter == NULL)
        playlists_iter = playlists;

    playlists_iter = g_list_previous(playlists_iter);

    if (playlists_iter == NULL)
        playlists_iter = playlists;

    playlistwin_update_list(playlist_get_active());
}

void
playlist_select_playlist(Playlist *playlist)
{
    if (playlists_iter == NULL)
        playlists_iter = playlists;

    playlists_iter = g_list_find(playlists, playlist);

    if (playlists_iter == NULL)
        playlists_iter = playlists;

    playlistwin_update_list(playlist);
}

/* *********************** playlist code ********************** */

const gchar *
playlist_get_current_name(Playlist *playlist)
{
    return playlist->title;
}

/* filename is real filename here. --yaz */
gboolean
playlist_set_current_name(Playlist *playlist, const gchar * filename)
{
    if (playlist->title)
        g_free(playlist->title);

    if (!filename) {
        playlist->title = NULL;
        return FALSE;
    }

    playlist->title = filename_to_utf8(filename);
    return TRUE;
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

    REQUIRE_LOCK( playlist->mutex );

    playlist->position = playlist->queue->data;
    playlist->queue = g_list_remove_link(playlist->queue, playlist->queue);
    g_list_free_1(tmp);
}

void
playlist_clear(Playlist *playlist)
{
    if (!playlist)
        return;

    PLAYLIST_LOCK( playlist->mutex );

    g_list_foreach(playlist->entries, (GFunc) playlist_entry_free, NULL);
    g_list_free(playlist->entries);
    playlist->position = NULL;
    playlist->entries = NULL;
    playlist->tail = NULL;
    playlist->attribute = PLAYLIST_PLAIN;

    PLAYLIST_UNLOCK( playlist->mutex );

    playlist_generate_shuffle_list(playlist);
    playlistwin_update_list(playlist);
    playlist_recalc_total_time(playlist);
    playlist_manager_update();
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
            PLAYLIST_UNLOCK(playlist->mutex);
            ip_data.stop = TRUE;
            playback_stop();
            ip_data.stop = FALSE;
            PLAYLIST_LOCK(playlist->mutex);
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
}

void
playlist_delete_index(Playlist *playlist, guint pos)
{
    gboolean restart_playing = FALSE, set_info_text = FALSE;
    GList *node;

    if (!playlist)
        return;

    PLAYLIST_LOCK(playlist->mutex);

    node = g_list_nth(playlist->entries, pos);

    if (!node) {
        PLAYLIST_UNLOCK(playlist->mutex);
        return;
    }

    playlist_delete_node(playlist, node, &set_info_text, &restart_playing);

    PLAYLIST_UNLOCK(playlist->mutex);

    playlist_recalc_total_time(playlist);

    playlistwin_update_list(playlist);
    if (restart_playing) {
        if (playlist->position)
            playback_initiate();
        else {
            mainwin_clear_song_info();
        }
    }

    playlist_manager_update();
}

void
playlist_delete_filenames(Playlist * playlist, GList * filenames)
{
    GList *node, *fnode;
    gboolean set_info_text = FALSE, restart_playing = FALSE;

    PLAYLIST_LOCK(playlist->mutex);

    for (fnode = filenames; fnode; fnode = g_list_next(fnode)) {
        node = playlist->entries;

        while (node) {
            GList *next = g_list_next(node);
            PlaylistEntry *entry = node->data;

            if (!strcmp(entry->filename, fnode->data))
                playlist_delete_node(playlist, node, &set_info_text, &restart_playing);

            node = next;
        }
    }

    PLAYLIST_UNLOCK(playlist->mutex);

    playlist_recalc_total_time(playlist);
    playlistwin_update_list(playlist);

    if (restart_playing) {
        if (playlist->position)
            playback_initiate();
        else {
            mainwin_clear_song_info();
        }
    }

    playlist_manager_update();
}

void
playlist_delete(Playlist * playlist, gboolean crop)
{
    gboolean restart_playing = FALSE, set_info_text = FALSE;
    GList *node, *next_node;
    PlaylistEntry *entry;

    g_return_if_fail(playlist != NULL);

    PLAYLIST_LOCK(playlist->mutex);

    node = playlist->entries;

    while (node) {
        entry = PLAYLIST_ENTRY(node->data);

        next_node = g_list_next(node);

        if ((entry->selected && !crop) || (!entry->selected && crop)) {
            playlist_delete_node(playlist, node, &set_info_text, &restart_playing);
        }

        node = next_node;
    }

    PLAYLIST_UNLOCK(playlist->mutex);

    playlist_recalc_total_time(playlist);

    if (restart_playing) {
        if (playlist->position)
            playback_initiate();
        else {
            mainwin_clear_song_info();
        }
    }

    playlistwin_update_list(playlist);
    playlist_manager_update();
}

static void
__playlist_ins_with_info(Playlist * playlist,
                         const gchar * filename,
                         gint pos,
                         const gchar * title,
                         gint len,
                         InputPlugin * dec)
{
    g_return_if_fail(filename != NULL);

    PLAYLIST_LOCK( playlist->mutex );
    playlist->entries = g_list_insert(playlist->entries,
                             playlist_entry_new(filename, title, len, dec),
                             pos);
    PLAYLIST_UNLOCK( playlist->mutex );

    g_mutex_lock(mutex_scan);
    playlist_get_info_scan_active = TRUE;
    g_mutex_unlock(mutex_scan);
    g_cond_signal(cond_scan);
}

static void
__playlist_ins_with_info_tuple(Playlist * playlist,
			       const gchar * filename,
			       gint pos,
			       Tuple *tuple,
			       InputPlugin * dec)
{
    PlaylistEntry *entry;

    g_return_if_fail(playlist != NULL);
    g_return_if_fail(filename != NULL);

    entry = playlist_entry_new(filename, tuple ? tuple_get_string(tuple, "title") : NULL, tuple ? tuple_get_int(tuple, "length") : -1, dec);
    if(!playlist->tail)
        playlist->tail = g_list_last(playlist->entries);

    PLAYLIST_LOCK( playlist->mutex );
    if(pos == -1) { // the common case
        GList *element;
        element = g_list_alloc();
        element->data = entry;
        element->prev = playlist->tail; // NULL is allowed here.
        element->next = NULL;

        if(!playlist->entries) { // this is the first element
            playlist->entries = element;
            playlist->tail = element;
        }
        else { // the rests
            g_return_if_fail(playlist->tail != NULL);
            playlist->tail->next = element;
            playlist->tail = element;
        }
    }
    else {
        playlist->entries = g_list_insert(playlist->entries, entry, pos);
    }

    PLAYLIST_UNLOCK( playlist->mutex );
    if (tuple != NULL) {
        const gchar *formatter = tuple_get_string(tuple, "formatter");
        entry->title = tuple_formatter_make_title_string(tuple, formatter ?
                                                      formatter : get_gentitle_format());
        entry->length = tuple_get_int(tuple, "length");
        entry->tuple = tuple;
    }

    if(tuple != NULL && tuple_get_int(tuple, "mtime") == -1) { // kick the scanner thread only if mtime = -1 (uninitialized).
        g_mutex_lock(mutex_scan);
        playlist_get_info_scan_active = TRUE;
        g_mutex_unlock(mutex_scan);
        g_cond_signal(cond_scan);
    }
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

    g_return_val_if_fail(playlist != NULL, FALSE);
    g_return_val_if_fail(filename != NULL, FALSE);

    if (is_playlist_name(filename)) {
        playlist->loading_playlist = TRUE;
        playlist_load_ins(playlist, filename, pos);
        playlist->loading_playlist = FALSE;
        return TRUE;
    }

    if (playlist->loading_playlist == TRUE || cfg.playlist_detect == TRUE)
	dec = NULL;
    else if (!str_has_prefix_nocase(filename, "http://") && 
	     !str_has_prefix_nocase(filename, "https://"))
    {
        pr = input_check_file(filename, TRUE);

        if (pr)
        {
            dec = pr->ip;
            tuple = pr->tuple;
        }

        g_free(pr);
    }

    if (cfg.playlist_detect == TRUE || playlist->loading_playlist == TRUE || (playlist->loading_playlist == FALSE && dec != NULL) || (playlist->loading_playlist == FALSE && !is_playlist_name(filename) && str_has_prefix_nocase(filename, "http")))
    {
        __playlist_ins_with_info_tuple(playlist, filename, pos, tuple, dec);
        playlist_generate_shuffle_list(playlist);
        playlistwin_update_list(playlist);
        return TRUE;
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

    if (!(file = vfs_fopen(filename, "rb")))
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

    if (devino)
    {
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
    return (g_basename(filename)[0] == '.');
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

        if (file_is_hidden(dir_entry))
            continue;

        tmp = g_build_filename(path, dir_entry, NULL);
        filename = g_filename_to_uri(tmp, NULL, NULL);
        g_free(tmp);

        if (vfs_file_test(filename, G_FILE_TEST_IS_DIR)) {
            GList *sub;
            gchar *dirfilename = g_filename_from_uri(filename, NULL, NULL);
            sub = playlist_dir_find_files(dirfilename, background, htab);
            g_free(dirfilename);
            g_free(filename);
            list = g_list_concat(list, sub);
        }
        else if (cfg.playlist_detect == TRUE)
            list = g_list_prepend(list, filename);
        else if ((pr = input_check_file(filename, TRUE)) != NULL)
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
//    printf("playlist_add_url: entries = %d\n", entries);
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
    playlistwin_update_list(playlist);
    playlist_manager_update();
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

//    playlistwin_update_list(playlist); // is this necessary? --yaz

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

        if (vfs_file_test(decoded, G_FILE_TEST_IS_DIR)) {
            i = playlist_ins_dir(playlist, decoded, pos, FALSE);
        }
        else {
            if (is_playlist_name(decoded)) {
                i = playlist_load_ins(playlist, decoded, pos);
            }
            else if (playlist_ins(playlist, decoded, pos)) {
                i = 1;
            }
        }

        g_free(decoded);

        PLAYLIST_LOCK(playlist->mutex);
        node = g_list_nth(playlist->entries, pos);
        PLAYLIST_UNLOCK(playlist->mutex);

        entries += i;

        if (pos >= 0)
            pos += i;
        if (!tmp)
            break;

        string = tmp + 1;
    }

    playlist_recalc_total_time(playlist);
    playlist_generate_shuffle_list(playlist);
    playlistwin_update_list(playlist);

    playlist_manager_update();

    return entries;
}

void
playlist_set_info_old_abi(const gchar * title, gint length, gint rate,
                          gint freq, gint nch)
{
    Playlist *playlist = playlist_get_active();
    PlaylistEventInfoChange *msg;
    gchar *text;

    if(length == -1) {
        event_queue("hide seekbar", (gpointer)0xdeadbeef); // event_queue hates NULL --yaz
    }

    g_return_if_fail(playlist != NULL);

    if (playlist->position) {
        g_free(playlist->position->title);
        playlist->position->title = g_strdup(title);
        playlist->position->length = length;

        // overwrite tuple::title, mainly for streaming. it may incur side effects. --yaz
        if(playlist->position->tuple && tuple_get_int(playlist->position->tuple, "length") == -1){
            tuple_disassociate(playlist->position->tuple, "title");
            tuple_associate_string(playlist->position->tuple, "title", title);
        }
    }

    playlist_recalc_total_time(playlist);

    /* broadcast a PlaylistEventInfoChange message. */
    msg = g_new0(PlaylistEventInfoChange, 1);
    msg->bitrate = rate;
    msg->samplerate = freq;
    msg->channels = nch;

    event_queue("playlist info change", msg);

    text = playlist_get_info_text(playlist);
    event_queue("title change", text);

    if ( playlist->position )
        hook_call( "playlist set info" , playlist->position );
}

void
playlist_set_info(Playlist * playlist, const gchar * title, gint length, gint rate,
                  gint freq, gint nch)
{
    g_return_if_fail(playlist != NULL);

    if (playlist->position) {
        g_free(playlist->position->title);
        playlist->position->title = g_strdup(title);
        playlist->position->length = length;
    }

    playlist_recalc_total_time(playlist);

    mainwin_set_song_info(rate, freq, nch);

    if ( playlist->position )
        hook_call( "playlist set info" , playlist->position );
}

void
playlist_check_pos_current(Playlist *playlist)
{
    gint pos, row, bottom;

    if (!playlist)
        return;

    PLAYLIST_LOCK(playlist->mutex);
    if (!playlist->position || !playlistwin_list) {
        PLAYLIST_UNLOCK(playlist->mutex);
        return;
    }

    pos = g_list_index(playlist->entries, playlist->position);

    if (playlistwin_item_visible(pos)) {
        PLAYLIST_UNLOCK(playlist->mutex);
        return;
    }

    bottom = MAX(0, playlist_get_length(playlist) -
                 UI_SKINNED_PLAYLIST(playlistwin_list)->num_visible);
    row = CLAMP(pos - UI_SKINNED_PLAYLIST(playlistwin_list)->num_visible / 2, 0, bottom);
    PLAYLIST_UNLOCK(playlist->mutex);
    playlistwin_set_toprow(row);
    g_cond_signal(cond_scan);
}

void
playlist_next(Playlist *playlist)
{
    GList *plist_pos_list;
    gboolean restart_playing = FALSE;
    if (!playlist_get_length(playlist))
        return;

    PLAYLIST_LOCK(playlist->mutex);

    if ((playlist_position_before_jump != NULL) && playlist->queue == NULL)
    {
        playlist->position = playlist_position_before_jump;
        playlist_position_before_jump = NULL;
    }

    plist_pos_list = find_playlist_position_list(playlist);

    if (!cfg.repeat && !g_list_next(plist_pos_list) && playlist->queue == NULL) {
        PLAYLIST_UNLOCK(playlist->mutex);
        return;
    }

    if (playback_get_playing()) {
        /* We need to stop before changing playlist_position */
        PLAYLIST_UNLOCK(playlist->mutex);
        ip_data.stop = TRUE;
        playback_stop();
        ip_data.stop = FALSE;
        PLAYLIST_LOCK(playlist->mutex);
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
    PLAYLIST_UNLOCK(playlist->mutex);
    playlist_check_pos_current(playlist);

    if (restart_playing)
        playback_initiate();

    playlistwin_update_list(playlist);
}

void
playlist_prev(Playlist *playlist)
{
    GList *plist_pos_list;
    gboolean restart_playing = FALSE;

    if (!playlist_get_length(playlist))
        return;

    PLAYLIST_LOCK(playlist->mutex);

    if ((playlist_position_before_jump != NULL) && playlist->queue == NULL)
    {
        playlist->position = playlist_position_before_jump;
        playlist_position_before_jump = NULL;
    }

    plist_pos_list = find_playlist_position_list(playlist);

    if (!cfg.repeat && !g_list_previous(plist_pos_list)) {
        PLAYLIST_UNLOCK(playlist->mutex);
        return;
    }

    if (playback_get_playing()) {
        /* We need to stop before changing playlist_position */
        PLAYLIST_UNLOCK(playlist->mutex);
        ip_data.stop = TRUE;
        playback_stop();
        ip_data.stop = FALSE;
        PLAYLIST_LOCK(playlist->mutex);
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

    PLAYLIST_UNLOCK(playlist->mutex);

    playlist_check_pos_current(playlist);

    if (restart_playing)
        playback_initiate();
    else
        playlistwin_update_list(playlist);
}

void
playlist_queue(Playlist *playlist)
{
    GList *list = playlist_get_selected(playlist);
    GList *it = list;

    PLAYLIST_LOCK(playlist->mutex);

    if ((cfg.shuffle) && (playlist_position_before_jump == NULL))
    {
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

    PLAYLIST_UNLOCK(playlist->mutex);

    playlist_recalc_total_time(playlist);
    playlistwin_update_list(playlist);
}

void
playlist_queue_position(Playlist *playlist, guint pos)
{
    GList *tmp;
    PlaylistEntry *entry;

    PLAYLIST_LOCK(playlist->mutex);

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
    PLAYLIST_UNLOCK(playlist->mutex);

    playlist_recalc_total_time(playlist);
    playlistwin_update_list(playlist);
}

gboolean
playlist_is_position_queued(Playlist *playlist, guint pos)
{
    PlaylistEntry *entry;
    GList *tmp;

    PLAYLIST_LOCK(playlist->mutex);
    entry = g_list_nth_data(playlist->entries, pos);
    tmp = g_list_find(playlist->queue, entry);
    PLAYLIST_UNLOCK(playlist->mutex);

    return tmp != NULL;
}

gint
playlist_get_queue_position_number(Playlist *playlist, guint pos)
{
    PlaylistEntry *entry;
    gint tmp;

    PLAYLIST_LOCK(playlist->mutex);
    entry = g_list_nth_data(playlist->entries, pos);
    tmp = g_list_index(playlist->queue, entry);
    PLAYLIST_UNLOCK(playlist->mutex);

    return tmp;
}

gint
playlist_get_queue_qposition_number(Playlist *playlist, guint pos)
{
    PlaylistEntry *entry;
    gint tmp;

    PLAYLIST_LOCK(playlist->mutex);
    entry = g_list_nth_data(playlist->queue, pos);
    tmp = g_list_index(playlist->entries, entry);
    PLAYLIST_UNLOCK(playlist->mutex);

    return tmp;
}

void
playlist_clear_queue(Playlist *playlist)
{
    PLAYLIST_LOCK(playlist->mutex);
    g_list_free(playlist->queue);
    playlist->queue = NULL;
    PLAYLIST_UNLOCK(playlist->mutex);

    playlist_recalc_total_time(playlist);
    playlistwin_update_list(playlist);
}

void
playlist_queue_remove(Playlist *playlist, guint pos)
{
    void *entry;

    PLAYLIST_LOCK(playlist->mutex);
    entry = g_list_nth_data(playlist->entries, pos);
    playlist->queue = g_list_remove(playlist->queue, entry);
    PLAYLIST_UNLOCK(playlist->mutex);

    playlistwin_update_list(playlist);
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

    PLAYLIST_LOCK(playlist->mutex);

    node = g_list_nth(playlist->entries, pos);
    if (!node) {
        PLAYLIST_UNLOCK(playlist->mutex);
        return;
    }

    if (playback_get_playing()) {
        /* We need to stop before changing playlist_position */
        PLAYLIST_UNLOCK(playlist->mutex);
        ip_data.stop = TRUE;
        playback_stop();
        ip_data.stop = FALSE;
        PLAYLIST_LOCK(playlist->mutex);
        restart_playing = TRUE;
    }

    if ((cfg.shuffle) && (playlist_position_before_jump == NULL))
    {
        /* Shuffling and this is our first manual jump. */
        playlist_position_before_jump = playlist->position;
    }

    playlist->position = node->data;
    PLAYLIST_UNLOCK(playlist->mutex);
    playlist_check_pos_current(playlist);

    if (restart_playing)
        playback_initiate();
    else
        playlistwin_update_list(playlist);
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

    PLAYLIST_LOCK(playlist->mutex);
    
    if ((playlist_position_before_jump != NULL) && playlist->queue == NULL)
    {
        playlist->position = playlist_position_before_jump;
        playlist_position_before_jump = NULL;
    }

    plist_pos_list = find_playlist_position_list(playlist);

    if (cfg.no_playlist_advance) {
        PLAYLIST_UNLOCK(playlist->mutex);
        mainwin_clear_song_info();
        if (cfg.repeat)
            playback_initiate();
        return;
    }

    if (cfg.stopaftersong) {
        PLAYLIST_UNLOCK(playlist->mutex);
        mainwin_clear_song_info();
        mainwin_set_stopaftersong(FALSE);
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
            PLAYLIST_UNLOCK(playlist->mutex);
	    hook_call("playlist end reached", playlist->position);
            mainwin_clear_song_info();
            return;
        }
    }
    else
        playlist->position = g_list_next(plist_pos_list)->data;

    PLAYLIST_UNLOCK(playlist->mutex);

    playlist_check_pos_current(playlist);
    playback_initiate();
    playlistwin_update_list(playlist);
}

gint
playlist_queue_get_length(Playlist *playlist)
{
    gint length;

    PLAYLIST_LOCK(playlist->mutex);
    length = g_list_length(playlist->queue);
    PLAYLIST_UNLOCK(playlist->mutex);

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

    PLAYLIST_LOCK(playlist->mutex);
    if (!playlist->position) {
        PLAYLIST_UNLOCK(playlist->mutex);
        return NULL;
    }

    /* FIXME: there should not be a need to do additional conversion,
     * if playlist is properly maintained */
    if (playlist->position->title) {
        title = str_to_utf8(playlist->position->title);
    }
    else {
        gchar *realfn = NULL;
        gchar *basename = NULL;
        realfn = g_filename_from_uri(playlist->position->filename, NULL, NULL);
        basename = g_path_get_basename(realfn ? realfn : playlist->position->filename);
        title = filename_to_utf8(basename);
        g_free(realfn); realfn = NULL;
        g_free(basename); basename = NULL;
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

    PLAYLIST_UNLOCK(playlist->mutex);

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
    gchar *ext;

    g_return_val_if_fail(playlist != NULL, FALSE);
    g_return_val_if_fail(filename != NULL, FALSE);

    ext = strrchr(filename, '.') + 1;

    playlist_set_current_name(playlist, filename);

    if ((plc = playlist_container_find(ext)) == NULL)
        return FALSE;

    if (plc->plc_write == NULL)
        return FALSE;

    plc->plc_write(filename, 0);

    return TRUE;
}

gboolean
playlist_load(Playlist * playlist, const gchar * filename)
{
    guint ret = 0;
    g_return_val_if_fail(playlist != NULL, FALSE);

    playlist->loading_playlist = TRUE;
    ret = playlist_load_ins(playlist, filename, -1);
    playlist->loading_playlist = FALSE;

    return ret ? TRUE : FALSE;
}

void
playlist_load_ins_file(Playlist *playlist,
		       const gchar * filename_p,
                       const gchar * playlist_name, gint pos,
                       const gchar * title, gint len)
{
    gchar *filename;
    gchar *tmp, *path;
    ProbeResult *pr = NULL;

    g_return_if_fail(filename_p != NULL);
    g_return_if_fail(playlist != NULL);
    g_return_if_fail(playlist_name != NULL);

    filename = g_strchug(g_strdup(filename_p));

    if(cfg.convert_slash)
        while ((tmp = strchr(filename, '\\')) != NULL)
            *tmp = '/';

    if (filename[0] != '/' && !strstr(filename, "://")) {
        path = g_strdup(playlist_name);
        if ((tmp = strrchr(path, '/')))
            *tmp = '\0';
        else {
	    if ((playlist->loading_playlist == TRUE ||
		cfg.playlist_detect == TRUE))
                pr = NULL;
	    else if (!str_has_prefix_nocase(filename, "http://") && 
	        !str_has_prefix_nocase(filename, "https://"))
		pr = input_check_file(filename, FALSE);

            __playlist_ins_with_info(playlist, filename, pos, title, len, pr ? pr->ip : NULL);

            g_free(pr);
            return;
        }
        tmp = g_build_filename(path, filename, NULL);

	if (playlist->loading_playlist == TRUE && cfg.playlist_detect == TRUE)
	    pr = NULL;
        else if (!str_has_prefix_nocase(tmp, "http://") && 
	    !str_has_prefix_nocase(tmp, "https://"))
	    pr = input_check_file(tmp, FALSE);

        __playlist_ins_with_info(playlist, tmp, pos, title, len, pr ? pr->ip : NULL);
        g_free(tmp);
        g_free(path);
        g_free(pr);
    }
    else
    {
        if ((playlist->loading_playlist == TRUE ||
  	    cfg.playlist_detect == TRUE))
            pr = NULL;
	else if (!str_has_prefix_nocase(filename, "http://") && 
	    !str_has_prefix_nocase(filename, "https://"))
	    pr = input_check_file(filename, FALSE);

        __playlist_ins_with_info(playlist, filename, pos, title, len, pr ? pr->ip : NULL);

        g_free(pr);
    }

    g_free(filename);
}

void
playlist_load_ins_file_tuple(Playlist * playlist,
			     const gchar * filename_p,
			     const gchar * playlist_name, 
			     gint pos,
			     Tuple *tuple)
{
    gchar *filename;
    gchar *tmp, *path;
    ProbeResult *pr = NULL;		/* for decoder cache */

    g_return_if_fail(filename_p != NULL);
    g_return_if_fail(playlist_name != NULL);
    g_return_if_fail(playlist != NULL);

    filename = g_strchug(g_strdup(filename_p));

    if(cfg.convert_slash)
        while ((tmp = strchr(filename, '\\')) != NULL)
            *tmp = '/';

    if (filename[0] != '/' && !strstr(filename, "://")) {
        path = g_strdup(playlist_name);
        if ((tmp = strrchr(path, '/')))
            *tmp = '\0';
        else {
            if ((playlist->loading_playlist == TRUE ||
    	        cfg.playlist_detect == TRUE))
                pr = NULL;
	    else if (!str_has_prefix_nocase(filename, "http://") && 
	        !str_has_prefix_nocase(filename, "https://"))
	        pr = input_check_file(filename, FALSE);

            __playlist_ins_with_info_tuple(playlist, filename, pos, tuple, pr ? pr->ip : NULL);

            g_free(pr);
            return;
        }
        tmp = g_build_filename(path, filename, NULL);

        if ((playlist->loading_playlist == TRUE ||
            cfg.playlist_detect == TRUE))
            pr = NULL;
        else if (!str_has_prefix_nocase(filename, "http://") && 
            !str_has_prefix_nocase(filename, "https://"))
            pr = input_check_file(filename, FALSE);

        __playlist_ins_with_info_tuple(playlist, tmp, pos, tuple, pr ? pr->ip : NULL);
        g_free(tmp);
        g_free(path);
        g_free(pr);
    }
    else
    {
        if ((playlist->loading_playlist == TRUE ||
            cfg.playlist_detect == TRUE))
            pr = NULL;
        else if (!str_has_prefix_nocase(filename, "http://") && 
            !str_has_prefix_nocase(filename, "https://"))
            pr = input_check_file(filename, FALSE);

        __playlist_ins_with_info_tuple(playlist, filename, pos, tuple, pr ? pr->ip : NULL);
        g_free(pr);
    }

    g_free(filename);
}

static guint
playlist_load_ins(Playlist * playlist, const gchar * filename, gint pos)
{
    PlaylistContainer *plc;
    gchar *ext;
    gint old_len, new_len;
    
    g_return_val_if_fail(playlist != NULL, 0);
    g_return_val_if_fail(filename != NULL, 0);

    ext = strrchr(filename, '.') + 1;
    plc = playlist_container_find(ext);

    g_return_val_if_fail(plc != NULL, 0);
    g_return_val_if_fail(plc->plc_read != NULL, 0);

    old_len = playlist_get_length(playlist);
    plc->plc_read(filename, pos);
    new_len = playlist_get_length(playlist);

    playlist_generate_shuffle_list(playlist);
    playlistwin_update_list(playlist);

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

    PLAYLIST_LOCK(playlist->mutex);
    pos = playlist_get_position_nolock(playlist);
    PLAYLIST_UNLOCK(playlist->mutex);

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

    PLAYLIST_LOCK(playlist->mutex);
    node = g_list_nth(playlist->entries, pos);
    if (!node) {
        PLAYLIST_UNLOCK(playlist->mutex);
        return NULL;
    }
    entry = node->data;

    filename = g_strdup(entry->filename);
    PLAYLIST_UNLOCK(playlist->mutex);

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

    PLAYLIST_LOCK(playlist->mutex);

    if (!(node = g_list_nth(playlist->entries, pos))) {
        PLAYLIST_UNLOCK(playlist->mutex);
        return NULL;
    }

    entry = node->data;

    if (entry->tuple)
        mtime = tuple_get_int(entry->tuple, "mtime");
    else
        mtime = 0;

    /* FIXME: simplify this logic */
    if ((entry->title == NULL && entry->length == -1) ||
        (entry->tuple && mtime != 0 && (mtime == -1 || mtime != playlist_get_mtime(entry->filename))))
    {
        if (playlist_entry_get_info(entry))
            title = entry->title;
    }
    else {
        title = entry->title;
    }

    PLAYLIST_UNLOCK(playlist->mutex);

    if (!title) {
        gchar *realfn = NULL;
        realfn = g_filename_from_uri(entry->filename, NULL, NULL);
        title = g_path_get_basename(realfn ? realfn : entry->filename);
        g_free(realfn); realfn = NULL;
        return str_replace(title, filename_to_utf8(title));
    }

    return str_to_utf8(title);
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
        mtime = tuple_get_int(tuple, "mtime");
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

    if (!(node = g_list_nth(playlist->entries, pos))) {
        return -1;
    }

    entry = node->data;

    if (entry->tuple)
        mtime = tuple_get_int(entry->tuple, "mtime");
    else
        mtime = 0;

    if (entry->tuple == NULL ||
        (mtime != 0 && (mtime == -1 || mtime != playlist_get_mtime(entry->filename)))) {

        if (playlist_entry_get_info(entry))
            song_time = entry->length;
    }
    else {
        song_time = entry->length;
    }

    return song_time;
}

static gint
playlist_compare_track(PlaylistEntry * a,
		       PlaylistEntry * b)
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

    tracknumber_a = tuple_get_int(a->tuple, "track-number");
    tracknumber_b = tuple_get_int(b->tuple, "track-number");

    return (tracknumber_a && tracknumber_b ?
	    tracknumber_a - tracknumber_b : 0);
}

static gint
playlist_compare_playlist(PlaylistEntry * a,
		          PlaylistEntry * b)
{
    const gchar *a_title = NULL, *b_title = NULL;

    g_return_val_if_fail(a != NULL, 0);
    g_return_val_if_fail(b != NULL, 0);

    if (a->title != NULL)
        a_title = a->title;
    else {
        if (strrchr(a->filename, '/'))
            a_title = strrchr(a->filename, '/') + 1;
        else
            a_title = a->filename;
    }

    if (b->title != NULL)
        b_title = b->title;
    else {
        if (strrchr(a->filename, '/'))
            b_title = strrchr(b->filename, '/') + 1;
        else
            b_title = b->filename;
    }

    return strcasecmp(a_title, b_title);
}

static gint
playlist_compare_title(PlaylistEntry * a,
                       PlaylistEntry * b)
{
    const gchar *a_title = NULL, *b_title = NULL;

    g_return_val_if_fail(a != NULL, 0);
    g_return_val_if_fail(b != NULL, 0);

    if(!a->tuple)
        playlist_entry_get_info(a);
    if(!b->tuple)
        playlist_entry_get_info(b);

    if (a->tuple != NULL)
        a_title = tuple_get_string(a->tuple, "title");
    if (b->tuple != NULL)
        b_title = tuple_get_string(b->tuple, "title");

    if (a_title != NULL && b_title != NULL)
        return strcasecmp(a_title, b_title);

    if (a->title != NULL)
        a_title = a->title;
    else {
        if (strrchr(a->filename, '/'))
            a_title = strrchr(a->filename, '/') + 1;
        else
            a_title = a->filename;
    }

    if (b->title != NULL)
        b_title = b->title;
    else {
        if (strrchr(a->filename, '/'))
            b_title = strrchr(b->filename, '/') + 1;
        else
            b_title = b->filename;
    }

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
        a_artist = tuple_get_string(a->tuple, "artist");
        if (str_has_prefix_nocase(a_artist, "the "))
            a_artist += 4;
    }

    if (b->tuple != NULL) {
        b_artist = tuple_get_string(b->tuple, "artist");
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
    PLAYLIST_LOCK(playlist->mutex);
    playlist->entries =
        g_list_sort(playlist->entries,
                    (GCompareFunc) playlist_compare_func_table[type]);
    PLAYLIST_UNLOCK(playlist->mutex);
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
    PLAYLIST_LOCK(playlist->mutex);
    playlist->entries = playlist_sort_selected_generic(playlist->entries, (GCompareFunc)
                                                       playlist_compare_func_table
                                                       [type]);
    PLAYLIST_UNLOCK(playlist->mutex);
}

void
playlist_reverse(Playlist *playlist)
{
    PLAYLIST_LOCK(playlist->mutex);
    playlist->entries = g_list_reverse(playlist->entries);
    PLAYLIST_UNLOCK(playlist->mutex);
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
    PLAYLIST_LOCK(playlist->mutex);
    playlist->entries = playlist_shuffle_list(playlist, playlist->entries);
    PLAYLIST_UNLOCK(playlist->mutex);
}

GList *
playlist_get_selected(Playlist *playlist)
{
    GList *node, *list = NULL;
    gint i = 0;

    PLAYLIST_LOCK(playlist->mutex);
    for (node = playlist->entries; node; node = g_list_next(node), i++) {
        PlaylistEntry *entry = node->data;
        if (entry->selected)
            list = g_list_prepend(list, GINT_TO_POINTER(i));
    }
    PLAYLIST_UNLOCK(playlist->mutex);
    return g_list_reverse(list);
}

void
playlist_clear_selected(Playlist *playlist)
{
    GList *node = NULL;
    gint i = 0;

    PLAYLIST_LOCK(playlist->mutex);
    for (node = playlist->entries; node; node = g_list_next(node), i++) {
        PLAYLIST_ENTRY(node->data)->selected = FALSE;
    }
    PLAYLIST_UNLOCK(playlist->mutex);
    playlist_recalc_total_time(playlist);
    playlist_manager_update();
}

gint
playlist_get_num_selected(Playlist *playlist)
{
    GList *node;
    gint num = 0;

    PLAYLIST_LOCK(playlist->mutex);
    for (node = playlist->entries; node; node = g_list_next(node)) {
        PlaylistEntry *entry = node->data;
        if (entry->selected)
            num++;
    }
    PLAYLIST_UNLOCK(playlist->mutex);
    return num;
}


static void
playlist_generate_shuffle_list(Playlist *playlist)
{
    PLAYLIST_LOCK(playlist->mutex);
    playlist_generate_shuffle_list_nolock(playlist);
    PLAYLIST_UNLOCK(playlist->mutex);
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

void
playlist_fileinfo(Playlist *playlist, guint pos)
{
    gchar *path = NULL;
    GList *node;
    PlaylistEntry *entry = NULL;
    Tuple *tuple = NULL;
    ProbeResult *pr = NULL;
    time_t mtime;

    PLAYLIST_LOCK(playlist->mutex);

    if ((node = g_list_nth(playlist->entries, pos)))
    {
        entry = node->data;
        tuple = entry->tuple;
        path = g_strdup(entry->filename);
    }

    PLAYLIST_UNLOCK(playlist->mutex);

    if (entry->tuple)
        mtime = tuple_get_int(entry->tuple, "mtime");
    else
        mtime = 0;

    /* No tuple? Try to set this entry up properly. --nenolod */
    if (entry->tuple == NULL || mtime == -1 ||
        mtime == 0 || mtime != playlist_get_mtime(entry->filename))
    {
        playlist_entry_get_info(entry);
        tuple = entry->tuple;
    }

    if (tuple != NULL)
    {
        if (entry->decoder == NULL)
        {
            pr = input_check_file(entry->filename, FALSE); /* try to find a decoder */
            entry->decoder = pr ? pr->ip : NULL;

            g_free(pr);
        }

        if (entry->decoder != NULL && entry->decoder->file_info_box == NULL)
            fileinfo_show_for_tuple(tuple);
        else if (entry->decoder != NULL && entry->decoder->file_info_box != NULL)
            entry->decoder->file_info_box(path);
        else
            fileinfo_show_for_path(path);
        g_free(path);
    }
    else if (path != NULL)
    {
        if (entry != NULL && entry->decoder != NULL && entry->decoder->file_info_box != NULL)
            entry->decoder->file_info_box(path);
        else
            fileinfo_show_for_path(path);
        g_free(path);
    }
}

void
playlist_fileinfo_current(Playlist *playlist)
{
    gchar *path = NULL;
    Tuple *tuple = NULL;

    PLAYLIST_LOCK(playlist->mutex);

    if (playlist->entries && playlist->position)
    {
        path = g_strdup(playlist->position->filename);
        if (( playlist->position->tuple == NULL ) || ( playlist->position->decoder == NULL ))
          playlist_entry_get_info(playlist->position);
        tuple = playlist->position->tuple;
    }

    PLAYLIST_UNLOCK(playlist->mutex);

    if (tuple != NULL)
    {
        if (playlist->position->decoder != NULL && playlist->position->decoder->file_info_box == NULL)
            fileinfo_show_for_tuple(tuple);
        else if (playlist->position->decoder != NULL && playlist->position->decoder->file_info_box != NULL)
            playlist->position->decoder->file_info_box(path);
        else
            fileinfo_show_for_path(path);
        g_free(path);
    }
    else if (path != NULL)
    {
        if (playlist->position != NULL && playlist->position->decoder != NULL && playlist->position->decoder->file_info_box != NULL)
            playlist->position->decoder->file_info_box(path);
        else
            fileinfo_show_for_path(path);
        g_free(path);
    }
}


static gboolean
playlist_get_info_is_going(void)
{
    gboolean result;

    G_LOCK(playlist_get_info_going);
    result = playlist_get_info_going;
    G_UNLOCK(playlist_get_info_going);

    return result;
}

static gboolean
playlist_request_win_update(gpointer unused)
{
    Playlist *playlist = playlist_get_active();
    playlistwin_update_list(playlist);
    return FALSE; /* to be called only once */
}

static gpointer
playlist_get_info_func(gpointer arg)
{
    GList *node;
    gboolean update_playlistwin = FALSE;

    while (playlist_get_info_is_going()) {
        PlaylistEntry *entry;
        Playlist *playlist = playlist_get_active();

        // on_load
        if (cfg.use_pl_metadata &&
            cfg.get_info_on_load &&
            playlist_get_info_scan_active) {

            for (node = playlist->entries; node; node = g_list_next(node)) {
                entry = node->data;

                if(playlist->attribute & PLAYLIST_STATIC ||
                   (entry->tuple && tuple_get_int(entry->tuple, "length") > -1 && tuple_get_int(entry->tuple, "mtime") != -1)) {
                    update_playlistwin = TRUE;
                    continue;
                }

                if (!playlist_entry_get_info(entry)) {
                    if (g_list_index(playlist->entries, entry) == -1)
                        /* Entry disappeared while we looked it up.
                           Restart. */
                        node = playlist->entries;
                }
                else if ((entry->tuple != NULL || entry->title != NULL) && entry->length != -1) {
                    update_playlistwin = TRUE;
                    break;
                }
            }
            
            if (!node) {
                g_mutex_lock(mutex_scan);
                playlist_get_info_scan_active = FALSE;
                g_mutex_unlock(mutex_scan);
            }
        } // on_load

        // on_demand
        else if (!cfg.get_info_on_load &&
                 cfg.get_info_on_demand &&
                 cfg.playlist_visible &&
                 !cfg.playlist_shaded &&
                 cfg.use_pl_metadata) {

            g_mutex_lock(mutex_scan);
            playlist_get_info_scan_active = FALSE;
            g_mutex_unlock(mutex_scan);

            for (node = g_list_nth(playlist->entries, playlistwin_get_toprow());
                 node && playlistwin_item_visible(g_list_position(playlist->entries, node));
                 node = g_list_next(node)) {

                 entry = node->data;

                 if(playlist->attribute & PLAYLIST_STATIC ||
                   (entry->tuple && tuple_get_int(entry->tuple, "length") > -1 && tuple_get_int(entry->tuple, "mtime") != -1)) {
                    update_playlistwin = TRUE;
                    continue;
                 }

                 if (!playlist_entry_get_info(entry)) { 
                     if (g_list_index(playlist->entries, entry) == -1)
                        /* Entry disapeared while we
                           looked it up.  Restart. */
                        node = g_list_nth(playlist->entries,
                                          playlistwin_get_toprow());
                 }
                 else if ((entry->tuple != NULL || entry->title != NULL) && entry->length != -1) {
                     update_playlistwin = TRUE;
                        // no need for break here since this iteration is very short.
                }
            }
        } // on_demand
        else if (cfg.get_info_on_demand && 
			(!cfg.playlist_visible || cfg.playlist_shaded
                	 || !cfg.use_pl_metadata))
        {
            g_mutex_lock(mutex_scan);
            playlist_get_info_scan_active = FALSE;
            g_mutex_unlock(mutex_scan);
        }
        else /* not on_demand and not on_load...
                NOTE: this shouldn't happen anymore, sanity check in bmp_config_load now */
        {
            g_mutex_lock(mutex_scan);
            playlist_get_info_scan_active = FALSE;
            g_mutex_unlock(mutex_scan);
        }

        if (update_playlistwin) {
            /* we are in a different thread, so we can't do UI updates directly;
               instead, schedule a playlist update in the main loop --giacomo */
            g_idle_add_full(G_PRIORITY_HIGH_IDLE, playlist_request_win_update, NULL, NULL);
            update_playlistwin = FALSE;
        }

        if (playlist_get_info_scan_active) {
            continue;
        }

        g_mutex_lock(mutex_scan);
        g_cond_wait(cond_scan, mutex_scan);
        g_mutex_unlock(mutex_scan);

    } // while

    g_thread_exit(NULL);
    return NULL;
}

void
playlist_start_get_info_thread(void)
{
    G_LOCK(playlist_get_info_going);
    playlist_get_info_going = TRUE;
    G_UNLOCK(playlist_get_info_going);

    playlist_get_info_thread = g_thread_create(playlist_get_info_func,
                                               NULL, TRUE, NULL);
}

void
playlist_stop_get_info_thread(void)
{
    G_LOCK(playlist_get_info_going);
    playlist_get_info_going = FALSE;
    G_UNLOCK(playlist_get_info_going);

    g_cond_broadcast(cond_scan);
    g_thread_join(playlist_get_info_thread);
}

void
playlist_start_get_info_scan(void)
{
    g_mutex_lock(mutex_scan);
    playlist_get_info_scan_active = TRUE;
    g_mutex_unlock(mutex_scan);
    g_cond_signal(cond_scan);
}

void
playlist_remove_dead_files(Playlist *playlist)
{
    GList *node, *next_node;

    PLAYLIST_LOCK(playlist->mutex);

    for (node = playlist->entries; node; node = next_node) {
        PlaylistEntry *entry = PLAYLIST_ENTRY(node->data);
        next_node = g_list_next(node);

        if (!entry || !entry->filename) {
            g_message(G_STRLOC ": Playlist entry is invalid!");
            continue;
        }

        /* FIXME: What about 'file:///'? */
        /* Don't kill URLs */
        if (strstr(entry->filename, "://"))
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
   
    PLAYLIST_UNLOCK(playlist->mutex);
    
    playlist_generate_shuffle_list(playlist);
    playlistwin_update_list(playlist);
    playlist_recalc_total_time(playlist);
    playlist_manager_update();
}


static gint
playlist_dupscmp_title(PlaylistEntry * a,
                       PlaylistEntry * b)
{
    const gchar *a_title, *b_title;

    g_return_val_if_fail(a != NULL, 0);
    g_return_val_if_fail(b != NULL, 0);

    if (a->title)
        a_title = a->title;
    else {
        if (strrchr(a->filename, '/'))
            a_title = strrchr(a->filename, '/') + 1;
        else
            a_title = a->filename;
    }

    if (b->title)
        b_title = b->title;
    else {
        if (strrchr(a->filename, '/'))
            b_title = strrchr(b->filename, '/') + 1;
        else
            b_title = b->filename;
    }

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

    PLAYLIST_LOCK(playlist->mutex);

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

    PLAYLIST_UNLOCK(playlist->mutex);

    playlistwin_update_list(playlist);
    playlist_recalc_total_time(playlist);

    playlist_manager_update();
}

void
playlist_get_total_time(Playlist * playlist,
			gulong * total_time,
                        gulong * selection_time,
                        gboolean * total_more,
                        gboolean * selection_more)
{
    PLAYLIST_LOCK(playlist->mutex);
    *total_time = playlist->pl_total_time;
    *selection_time = playlist->pl_selection_time;
    *total_more = playlist->pl_total_more;
    *selection_more = playlist->pl_selection_more;
    PLAYLIST_UNLOCK(playlist->mutex);
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
    PLAYLIST_LOCK(playlist->mutex);
    playlist_recalc_total_time_nolock(playlist);
    PLAYLIST_UNLOCK(playlist->mutex);
}

gint
playlist_select_search( Playlist *playlist , Tuple *tuple , gint action )
{
    GList *entry_list = NULL, *found_list = NULL, *sel_list = NULL;
    gboolean is_first_search = TRUE;
    gint num_of_entries_found = 0;

    #if defined(USE_REGEX_ONIGURUMA)
    /* set encoding for Oniguruma regex to UTF-8 */
    reg_set_encoding( REG_POSIX_ENCODING_UTF8 );
    onig_set_default_syntax( ONIG_SYNTAX_POSIX_BASIC );
    #endif

    PLAYLIST_LOCK(playlist->mutex);

    if ( tuple_get_string(tuple, "title") != NULL )
    {
        /* match by track_name */
        const gchar *regex_pattern = tuple_get_string(tuple, "title");
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
                const gchar *track_name = tuple_get_string( tuple, "title" );
                PlaylistEntry *entry = entry_list->data;
                if ( ( entry->tuple != NULL ) && ( track_name != NULL ) &&
                   ( regexec( &regex , track_name , 0 , NULL , 0 ) == 0 ) )
                {
                    tfound_list = g_list_append( tfound_list , entry );
                }
            }
            g_list_free( found_list ); /* wipe old found_list */
            found_list = tfound_list; /* move tfound_list in found_list */
            regfree( &regex );
        }
        is_first_search = FALSE;
    }

    if ( tuple_get_string(tuple, "album") != NULL )
    {
        /* match by album_name */
        const gchar *regex_pattern = tuple_get_string(tuple, "album");
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
                const gchar *album_name = tuple_get_string( tuple, "album" );
                PlaylistEntry *entry = entry_list->data;
                if ( ( entry->tuple != NULL ) && ( album_name != NULL ) &&
                   ( regexec( &regex , album_name , 0 , NULL , 0 ) == 0 ) )
                {
                    tfound_list = g_list_append( tfound_list , entry );
                }
            }
            g_list_free( found_list ); /* wipe old found_list */
            found_list = tfound_list; /* move tfound_list in found_list */
            regfree( &regex );
        }
        is_first_search = FALSE;
    }

    if ( tuple_get_string(tuple, "artist") != NULL )
    {
        /* match by performer */
        const gchar *regex_pattern = tuple_get_string(tuple, "artist");
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
                const gchar *performer = tuple_get_string( tuple, "performer" );
                PlaylistEntry *entry = entry_list->data;
                if ( ( entry->tuple != NULL ) && ( performer != NULL ) &&
                   ( regexec( &regex , performer , 0 , NULL , 0 ) == 0 ) )
                {
                    tfound_list = g_list_append( tfound_list , entry );
                }
            }
            g_list_free( found_list ); /* wipe old found_list */
            found_list = tfound_list; /* move tfound_list in found_list */
            regfree( &regex );
        }
        is_first_search = FALSE;
    }

    if ( tuple_get_string(tuple, "file-name") != NULL )
    {
        /* match by file_name */
        const gchar *regex_pattern = tuple_get_string(tuple, "file-name");
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
                const gchar *file_name = tuple_get_string( tuple, "file-name" );
                PlaylistEntry *entry = entry_list->data;
                if ( ( entry->tuple != NULL ) && ( file_name != NULL ) &&
                   ( regexec( &regex , file_name , 0 , NULL , 0 ) == 0 ) )
                {
                    tfound_list = g_list_append( tfound_list , entry );
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

    PLAYLIST_UNLOCK(playlist->mutex);
    playlist_recalc_total_time(playlist);

    return num_of_entries_found;
}

void
playlist_select_all(Playlist *playlist, gboolean set)
{
    GList *list;

    PLAYLIST_LOCK(playlist->mutex);

    for (list = playlist->entries; list; list = g_list_next(list)) {
        PlaylistEntry *entry = list->data;
        entry->selected = set;
    }

    PLAYLIST_UNLOCK(playlist->mutex);
    playlist_recalc_total_time(playlist);
}

void
playlist_select_invert_all(Playlist *playlist)
{
    GList *list;

    PLAYLIST_LOCK(playlist->mutex);

    for (list = playlist->entries; list; list = g_list_next(list)) {
        PlaylistEntry *entry = list->data;
        entry->selected = !entry->selected;
    }

    PLAYLIST_UNLOCK(playlist->mutex);
    playlist_recalc_total_time(playlist);
}

gboolean
playlist_select_invert(Playlist *playlist, guint pos)
{
    GList *list;
    gboolean invert_ok = FALSE;

    PLAYLIST_LOCK(playlist->mutex);

    if ((list = g_list_nth(playlist->entries, pos))) {
        PlaylistEntry *entry = list->data;
        entry->selected = !entry->selected;
        invert_ok = TRUE;
    }

    PLAYLIST_UNLOCK(playlist->mutex);
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

    PLAYLIST_LOCK(playlist->mutex);

    list = g_list_nth(playlist->entries, min_pos);
    for (i = min_pos; i <= max_pos && list; i++) {
        PlaylistEntry *entry = list->data;
        entry->selected = select;
        list = g_list_next(list);
    }

    PLAYLIST_UNLOCK(playlist->mutex);

    playlist_recalc_total_time(playlist);
}

gboolean
playlist_read_info_selection(Playlist *playlist)
{
    GList *node;
    gboolean retval = FALSE;

    PLAYLIST_LOCK(playlist->mutex);

    for (node = playlist->entries; node; node = g_list_next(node)) {
        PlaylistEntry *entry = node->data;
        if (!entry->selected)
            continue;

        retval = TRUE;

        str_replace_in(&entry->title, NULL);
        entry->length = -1;

	/* invalidate mtime to reread */
	if (entry->tuple != NULL)
            tuple_associate_int(entry->tuple, "mtime", -1); /* -1 denotes "non-initialized". now 0 is for stream etc. yaz */

        if (!playlist_entry_get_info(entry)) {
            if (g_list_index(playlist->entries, entry) == -1)
                /* Entry disappeared while we looked it up. Restart. */
                node = playlist->entries;
        }
    }

    PLAYLIST_UNLOCK(playlist->mutex);

    playlistwin_update_list(playlist);
    playlist_recalc_total_time(playlist);

    return retval;
}

void
playlist_read_info(Playlist *playlist, guint pos)
{
    GList *node;

    PLAYLIST_LOCK(playlist->mutex);

    if ((node = g_list_nth(playlist->entries, pos))) {
        PlaylistEntry *entry = node->data;
        str_replace_in(&entry->title, NULL);
        entry->length = -1;
        playlist_entry_get_info(entry);
    }

    PLAYLIST_UNLOCK(playlist->mutex);

    playlistwin_update_list(playlist);
    playlist_recalc_total_time(playlist);
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

    PLAYLIST_LOCK(playlist->mutex);

    playlist_position_before_jump = NULL;

    cfg.shuffle = shuffle;
    playlist_generate_shuffle_list_nolock(playlist);

    PLAYLIST_UNLOCK(playlist->mutex);
}

Playlist *
playlist_new(void)
{
    Playlist *playlist = g_new0(Playlist, 1);
    playlist->mutex = g_mutex_new();
    playlist->loading_playlist = FALSE;

    playlist_set_current_name(playlist, NULL);
    playlist_clear(playlist);

    return playlist;
}

void
playlist_free(Playlist *playlist)
{
    g_mutex_free( playlist->mutex );
    g_free( playlist );
    return;
}

Playlist *
playlist_new_from_selected(void)
{
    Playlist *newpl = playlist_new();
    Playlist *playlist = playlist_get_active();
    GList *list = playlist_get_selected(playlist);

    playlist_add_playlist( newpl );

    PLAYLIST_LOCK(playlist->mutex);

    while ( list != NULL )
    {
        PlaylistEntry *entry = g_list_nth_data(playlist->entries, GPOINTER_TO_INT(list->data));
        if ( entry->filename != NULL ) /* paranoid? oh well... */
            playlist_add( newpl , entry->filename );
        list = g_list_next(list);
    }

    PLAYLIST_UNLOCK(playlist->mutex);

    playlist_recalc_total_time(newpl);
    playlistwin_update_list(playlist);

    return newpl;
}

const gchar *
playlist_get_filename_to_play(Playlist *playlist)
{
    const gchar *filename = NULL;

    if (!playlist)
        return NULL;

    PLAYLIST_LOCK(playlist->mutex);

    if (!playlist->position) {
        if (cfg.shuffle)
            playlist->position = playlist->shuffle->data;
        else
            playlist->position = playlist->entries->data;
    }

    filename = playlist->position->filename;

    PLAYLIST_UNLOCK(playlist->mutex);

    return filename;
}

PlaylistEntry *
playlist_get_entry_to_play(Playlist *playlist)
{
    if (!playlist)
        return NULL;

    PLAYLIST_LOCK(playlist->mutex);

    if (!playlist->position) {
        if (cfg.shuffle)
            playlist->position = playlist->shuffle->data;
        else
            playlist->position = playlist->entries->data;
    }

    PLAYLIST_UNLOCK(playlist->mutex);

    return playlist->position;
}
