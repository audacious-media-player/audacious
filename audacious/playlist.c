/*  BMP (C) GPL 2003 $top_src_dir/AUTHORS
 *
 *  based on:
 *
 *  XMMS - Cross-platform multimedia player
 *  Copyright (C) 1998-2003  Peter Alm, Mikael Alm, Olle Hallnas,
 *                           Thomas Nilsson and 4Front Technologies
 *  Copyright (C) 1999-2003  Haavard Kvaalen
 *
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Tmple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include "playlist.h"

#include <glib.h>
#include <glib/gprintf.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/errno.h>

#include "input.h"
#include "main.h"
#include "mainwin.h"
#include "libaudacious/util.h"
#include "libaudacious/configdb.h"
#include "libaudacious/vfs.h"
#include "equalizer.h"
#include "playback.h"
#include "playlist.h"
#include "ui_playlist.h"
#include "playlist_list.h"
#include "skin.h"
#include "urldecode.h"
#include "util.h"

#include "debug.h"

typedef gint (*PlaylistCompareFunc) (const PlaylistEntry * a, const PlaylistEntry * b);
typedef void (*PlaylistSaveFunc) (FILE * file);

PlaylistEntry *playlist_position;
G_LOCK_DEFINE(playlist);

/* NOTE: match the order listed in PlaylistFormat enum */
static const gchar *playlist_format_suffixes[] = { 
    ".m3u", ".pls", NULL 
};

static GList *playlist = NULL;
static GList *shuffle_list = NULL;
static GList *queued_list = NULL;

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
static gboolean loading_playlist = FALSE;

G_LOCK_DEFINE(playlist_get_info_going);

static gchar *playlist_current_name = NULL;

static gboolean playlist_get_info_scan_active = FALSE;
static gboolean playlist_get_info_going = FALSE;
static GThread *playlist_get_info_thread;


static gint path_compare(const gchar * a, const gchar * b);
static gint playlist_compare_path(const PlaylistEntry * a, const PlaylistEntry * b);
static gint playlist_compare_filename(const PlaylistEntry * a, const PlaylistEntry * b);
static gint playlist_compare_title(const PlaylistEntry * a, const PlaylistEntry * b);
static gint playlist_compare_date(const PlaylistEntry * a, const PlaylistEntry * b);

static PlaylistCompareFunc playlist_compare_func_table[] = {
    playlist_compare_path,
    playlist_compare_filename,
    playlist_compare_title,
    playlist_compare_date
};

static void playlist_save_m3u(FILE * file);
static void playlist_save_pls(FILE * file);

static PlaylistSaveFunc playlist_save_func_table[] = {
    playlist_save_m3u,
    playlist_save_pls
};


static guint playlist_load_ins(const gchar * filename, gint pos);

static void playlist_load_ins_file(const gchar * filename,
                                   const gchar * playlist_name, gint pos,
                                   const gchar * title, gint len);

static void playlist_generate_shuffle_list(void);
static void playlist_generate_shuffle_list_nolock(void);

static void playlist_recalc_total_time_nolock(void);
static void playlist_recalc_total_time(void);


PlaylistEntry *
playlist_entry_new(const gchar * filename,
                   const gchar * title,
                   const gint length,
		   InputPlugin * dec)
{
    PlaylistEntry *entry;

    entry = g_new0(PlaylistEntry, 1);
    entry->filename = g_strdup(filename);
    entry->title = str_to_utf8(title);
    entry->length = length;
    entry->selected = FALSE;
    entry->decoder = dec;

    return entry;
}

void
playlist_entry_free(PlaylistEntry * entry)
{
    if (!entry)
        return;

    g_free(entry->filename);
    g_free(entry->title);
    g_free(entry);
}

static gboolean
playlist_entry_get_info(PlaylistEntry * entry)
{
    gchar *title = NULL;
    gint length = -1;

    g_return_val_if_fail(entry != NULL, FALSE);

    if (entry->decoder == NULL)
        input_get_song_info(entry->filename, &title, &length);
    else if (entry->decoder->get_song_info != NULL)
        entry->decoder->get_song_info(entry->filename, &title, &length);

    if (!title && length == -1)
        return FALSE;

    /* entry is still around */
    entry->title = title;
    entry->length = length;

    return TRUE;
}


const gchar *
playlist_get_current_name(void)
{
    return playlist_current_name;
}

gboolean
playlist_set_current_name(const gchar * filename)
{
    g_free(playlist_current_name);

    if (!filename) {
        playlist_current_name = NULL;
        return FALSE;
    }

    playlist_current_name = g_strdup(filename);
    return TRUE;
}

static GList *
find_playlist_position_list(void)
{
    REQUIRE_STATIC_LOCK(playlist);

    if (!playlist_position) {
        if (cfg.shuffle)
            return shuffle_list;
        else
            return playlist;
    }

    if (cfg.shuffle)
        return g_list_find(shuffle_list, playlist_position);
    else
        return g_list_find(playlist, playlist_position);
}

static void
play_queued(void)
{
    GList *tmp = queued_list;

    REQUIRE_STATIC_LOCK(playlist);

    playlist_position = queued_list->data;
    queued_list = g_list_remove_link(queued_list, queued_list);
    g_list_free_1(tmp);
}

void
playlist_clear(void)
{
    if (bmp_playback_get_playing())
        bmp_playback_stop();

    PLAYLIST_LOCK();

    if (playlist) {
        g_list_foreach(playlist, (GFunc) playlist_entry_free, NULL);
        g_list_free(playlist);

        playlist = NULL;
        playlist_position = NULL;
    }

    PLAYLIST_UNLOCK();

    playlist_generate_shuffle_list();
    playlistwin_update_list();
    playlist_recalc_total_time();
}

void
playlist_delete_node(GList * node, gboolean * set_info_text,
                     gboolean * restart_playing)
{
    PlaylistEntry *entry;
    GList *playing_song = NULL;

    REQUIRE_STATIC_LOCK(playlist);

    /* We call g_list_find manually here because we don't want an item
     * in the shuffle_list */

    if (playlist_position)
        playing_song = g_list_find(playlist, playlist_position);

    entry = PLAYLIST_ENTRY(node->data);

    if (playing_song == node) {
        *set_info_text = TRUE;

        if (bmp_playback_get_playing()) {
            PLAYLIST_UNLOCK();
            bmp_playback_stop();
            PLAYLIST_LOCK();
            *restart_playing = TRUE;
        }

        playing_song = find_playlist_position_list();

        if (g_list_next(playing_song))
            playlist_position = g_list_next(playing_song)->data;
        else if (g_list_previous(playing_song))
            playlist_position = g_list_previous(playing_song)->data;
        else
            playlist_position = NULL;

        /* Make sure the entry did not disappear under us */
        if (g_list_index(playlist_get(), entry) == -1)
            return;

    }
    else if (g_list_position(playlist, playing_song) >
             g_list_position(playlist, node)) {
        *set_info_text = TRUE;
    }

    shuffle_list = g_list_remove(shuffle_list, entry);
    playlist = g_list_remove_link(playlist, node);
    playlist_entry_free(entry);
    g_list_free_1(node);

    playlist_recalc_total_time_nolock();
}

void
playlist_delete_index(guint pos)
{
    gboolean restart_playing = FALSE, set_info_text = FALSE;
    GList *node;

    PLAYLIST_LOCK();

    if (!playlist) {
        PLAYLIST_UNLOCK();
        return;
    }

    node = g_list_nth(playlist, pos);

    if (!node) {
        PLAYLIST_UNLOCK();
        return;
    }

    playlist_delete_node(node, &set_info_text, &restart_playing);

    PLAYLIST_UNLOCK();

    playlist_recalc_total_time();

    playlistwin_update_list();
    if (restart_playing) {
        if (playlist_position) {
            bmp_playback_initiate();
        }
        else {
            mainwin_clear_song_info();
        }
    }
    else if (set_info_text) {
        mainwin_set_info_text();
    }
}

void
playlist_delete_filenames(GList * filenames)
{
    GList *node, *fnode;
    gboolean set_info_text = FALSE, restart_playing = FALSE;

    PLAYLIST_LOCK();

    for (fnode = filenames; fnode; fnode = g_list_next(fnode)) {
        node = playlist;

        while (node) {
            GList *next = g_list_next(node);
            PlaylistEntry *entry = node->data;

            if (!strcmp(entry->filename, fnode->data))
                playlist_delete_node(node, &set_info_text, &restart_playing);

            node = next;
        }
    }

    playlist_recalc_total_time();
    PLAYLIST_UNLOCK();

    playlistwin_update_list();

    if (restart_playing) {
        if (playlist_position) {
            bmp_playback_initiate();
        }
        else {
            mainwin_clear_song_info();
        }
    }
    else if (set_info_text) {
        mainwin_set_info_text();
    }

}

void
playlist_delete(gboolean crop)
{
    gboolean restart_playing = FALSE, set_info_text = FALSE;
    GList *node, *next_node;
    PlaylistEntry *entry;

    PLAYLIST_LOCK();

    node = playlist;

    while (node) {
        entry = PLAYLIST_ENTRY(node->data);

        next_node = g_list_next(node);

        if ((entry->selected && !crop) || (!entry->selected && crop)) {
            playlist_delete_node(node, &set_info_text, &restart_playing);
        }

        node = next_node;
    }

    PLAYLIST_UNLOCK();

    playlist_recalc_total_time();

    if (set_info_text) {
        mainwin_set_info_text();
    }

    if (restart_playing) {
        if (playlist_position) {
            bmp_playback_initiate();
        }
        else {
            mainwin_clear_song_info();
        }
    }

    playlistwin_update_list();
}

static void
__playlist_ins_with_info(const gchar * filename,
                         gint pos,
                         const gchar * title,
                         gint len,
			 InputPlugin * dec)
{
    g_return_if_fail(filename != NULL);

    PLAYLIST_LOCK();
    playlist = g_list_insert(playlist,
                             playlist_entry_new(filename, title, len, dec),
                             pos);
    PLAYLIST_UNLOCK();

    playlist_get_info_scan_active = TRUE;
}

static void
__playlist_ins(const gchar * filename, gint pos, InputPlugin *dec)
{
    __playlist_ins_with_info(filename, pos, NULL, -1, dec);
    playlist_recalc_total_time();
}


PlaylistFormat
playlist_format_get_from_name(const gchar * filename)
{
    int i;

    for (i = 0; i < PLAYLIST_FORMAT_COUNT; i++)
    {
        if (str_has_suffix_nocase(filename, playlist_format_suffixes[i]))
            return i;
    }

    return PLAYLIST_FORMAT_UNKNOWN;
}

gboolean
is_playlist_name(const gchar * filename)
{
    g_return_val_if_fail(filename != NULL, FALSE);
    return playlist_format_get_from_name(filename) != PLAYLIST_FORMAT_UNKNOWN;
}

gboolean
playlist_ins(const gchar * filename, gint pos)
{
    gchar buf[64], *p;
    gint r;
    VFSFile *file;
    InputPlugin *dec;

    if (is_playlist_name(filename)) {
        playlist_load_ins(filename, pos);
        return TRUE;
    }

    if (loading_playlist == TRUE)
	dec = NULL;
    else
	dec = input_check_file(filename, TRUE);

    if (loading_playlist == TRUE || (loading_playlist == FALSE && dec != NULL))
    {
	__playlist_ins(filename, pos, dec);
	playlist_generate_shuffle_list();
	playlistwin_update_list();
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
        playlist_load_ins(filename, pos);
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
    // FIXME: remove the const cast
    g_return_val_if_fail(filename != NULL, FALSE);
    return (g_basename((gchar *) filename)[0] == '.');
}

static GList *
playlist_dir_find_files(const gchar * path,
                        gboolean background,
                        GHashTable * htab)
{
    GDir *dir;
    GList *list = NULL, *ilist;
    const gchar *dir_entry;

    struct stat statbuf;
    DeviceInode *devino;

    if (!g_file_test(path, G_FILE_TEST_IS_DIR))
        return NULL;

    stat(path, &statbuf);
    devino = devino_new(statbuf.st_dev, statbuf.st_ino);

    if (g_hash_table_lookup(htab, devino)) {
        g_free(devino);
        return NULL;
    }

    g_hash_table_insert(htab, devino, GINT_TO_POINTER(1));

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

    if (!(dir = g_dir_open(path, 0, NULL)))
        return NULL;

    while ((dir_entry = g_dir_read_name(dir))) {
        gchar *filename;

        if (file_is_hidden(dir_entry))
            continue;

        filename = g_build_filename(path, dir_entry, NULL);

        if (g_file_test(filename, G_FILE_TEST_IS_DIR)) {
            GList *sub;
            sub = playlist_dir_find_files(filename, background, htab);
            g_free(filename);
            list = g_list_concat(list, sub);
        }
        else if (input_check_file(filename, TRUE))
            list = g_list_prepend(list, filename);
        else
            g_free(filename);

        while (background && gtk_events_pending())
            gtk_main_iteration();
    }
    g_dir_close(dir);

    return list;
}

gboolean
playlist_add(const gchar * filename)
{
    return playlist_ins(filename, -1);
}

guint 
playlist_add_dir(const gchar * directory)
{
    return playlist_ins_dir(directory, -1, TRUE);
}

guint
playlist_add_url(const gchar * url)
{
    return playlist_ins_url(url, -1);
}

guint
playlist_ins_dir(const gchar * path,
                    gint pos,
                    gboolean background)
{
    guint entries = 0;
    GList *list, *node;
    GHashTable *htab;

    htab = g_hash_table_new(devino_hash, devino_compare);

    list = playlist_dir_find_files(path, background, htab);
    list = g_list_sort(list, (GCompareFunc) path_compare);

    g_hash_table_foreach_remove(htab, devino_destroy, NULL);

    for (node = list; node; node = g_list_next(node)) {
        __playlist_ins(node->data, pos, NULL);
        g_free(node->data);
        entries++;
        if (pos >= 0)
            pos++;
    }

    g_list_free(list);

    playlist_recalc_total_time();
    playlist_generate_shuffle_list();
    playlistwin_update_list();
    return entries;
}

guint
playlist_ins_url(const gchar * string,
                    gint pos)
{
    gchar *tmp;
    gint i = 1, entries = 0;
    gboolean first = TRUE;
    guint firstpos = 0;
    gboolean success = FALSE;
    gchar *decoded = NULL;

    g_return_val_if_fail(string != NULL, 0);

    playlistwin_update_list();

    while (*string) {
        GList *node;
        tmp = strchr(string, '\n');
        if (tmp) {
            if (*(tmp - 1) == '\r')
                *(tmp - 1) = '\0';
            *tmp = '\0';
        }

        if (!(decoded = xmms_urldecode_path(string)))
            decoded = g_strdup(string);

        if (g_file_test(decoded, G_FILE_TEST_IS_DIR)) {
            i = playlist_ins_dir(decoded, pos, FALSE);
        }
        else {
            if (is_playlist_name(decoded)) {
                i = playlist_load_ins(decoded, pos);
            }
            else {
                success = playlist_ins(decoded, pos);
                i = 1;
            }
        }

        g_free(decoded);

        PLAYLIST_LOCK();
        node = g_list_nth(playlist_get(), pos);
        PLAYLIST_UNLOCK();

        entries += i;

        if (first) {
            first = FALSE;
            firstpos = pos;
        }

        if (pos >= 0)
            pos += i;
        if (!tmp)
            break;

        string = tmp + 1;
    }

    playlist_recalc_total_time();
    playlist_generate_shuffle_list();
    playlistwin_update_list();

    return entries;
}

void
playlist_set_info(const gchar * title, gint length, gint rate,
                  gint freq, gint nch)
{
    PLAYLIST_LOCK();

    if (playlist_position) {
        g_free(playlist_position->title);
        playlist_position->title = g_strdup(title);
        playlist_position->length = length;
    }

    PLAYLIST_UNLOCK();

    playlist_recalc_total_time();

    mainwin_set_song_info(rate, freq, nch);
}

void
playlist_check_pos_current(void)
{
    gint pos, row, bottom;

    PLAYLIST_LOCK();
    if (!playlist || !playlist_position || !playlistwin_list) {
        PLAYLIST_UNLOCK();
        return;
    }

    pos = g_list_index(playlist, playlist_position);

    if (playlistwin_item_visible(pos)) {
        PLAYLIST_UNLOCK();
        return;
    }

    bottom = MAX(0, playlist_get_length_nolock() -
                 playlistwin_list->pl_num_visible);
    row = CLAMP(pos - playlistwin_list->pl_num_visible / 2, 0, bottom);
    PLAYLIST_UNLOCK();
    playlistwin_set_toprow(row);
}

void
playlist_next(void)
{
    GList *plist_pos_list;
    gboolean restart_playing = FALSE;

    PLAYLIST_LOCK();
    if (!playlist) {
        PLAYLIST_UNLOCK();
        return;
    }

    plist_pos_list = find_playlist_position_list();

    if (!cfg.repeat && !g_list_next(plist_pos_list)) {
        PLAYLIST_UNLOCK();
        return;
    }

    if (bmp_playback_get_playing()) {
        /* We need to stop before changing playlist_position */
        PLAYLIST_UNLOCK();
        bmp_playback_stop();
        PLAYLIST_LOCK();
        restart_playing = TRUE;
    }

    plist_pos_list = find_playlist_position_list();
    if (queued_list)
        play_queued();
    else if (g_list_next(plist_pos_list))
        playlist_position = g_list_next(plist_pos_list)->data;
    else if (cfg.repeat) {
        playlist_position = NULL;
        playlist_generate_shuffle_list_nolock();
        if (cfg.shuffle)
            playlist_position = shuffle_list->data;
        else
            playlist_position = playlist->data;
    }
    PLAYLIST_UNLOCK();
    playlist_check_pos_current();

    if (restart_playing)
        bmp_playback_initiate();
    else {
        mainwin_set_info_text();
        playlistwin_update_list();
    }
}

void
playlist_prev(void)
{
    GList *plist_pos_list;
    gboolean restart_playing = FALSE;

    PLAYLIST_LOCK();
    if (!playlist) {
        PLAYLIST_UNLOCK();
        return;
    }

    plist_pos_list = find_playlist_position_list();

    if (!cfg.repeat && !g_list_previous(plist_pos_list)) {
        PLAYLIST_UNLOCK();
        return;
    }

    if (bmp_playback_get_playing()) {
        /* We need to stop before changing playlist_position */
        PLAYLIST_UNLOCK();
        bmp_playback_stop();
        PLAYLIST_LOCK();
        restart_playing = TRUE;
    }

    plist_pos_list = find_playlist_position_list();
    if (g_list_previous(plist_pos_list)) {
        playlist_position = g_list_previous(plist_pos_list)->data;
    }
    else if (cfg.repeat) {
        GList *node;
        playlist_position = NULL;
        playlist_generate_shuffle_list_nolock();
        if (cfg.shuffle)
            node = g_list_last(shuffle_list);
        else
            node = g_list_last(playlist);
        if (node)
            playlist_position = node->data;
    }

    PLAYLIST_UNLOCK();

    playlist_check_pos_current();

    if (restart_playing)
        bmp_playback_initiate();
    else {
        mainwin_set_info_text();
        playlistwin_update_list();
    }
}

void
playlist_queue(void)
{
    GList *list = playlist_get_selected();
    GList *it = list;

    PLAYLIST_LOCK();

    while (it) {
        GList *next = g_list_next(it);
        GList *tmp;

        it->data = g_list_nth_data(playlist, GPOINTER_TO_INT(it->data));
        if ((tmp = g_list_find(queued_list, it->data))) {
            queued_list = g_list_remove_link(queued_list, tmp);
            g_list_free_1(tmp);
            list = g_list_remove_link(list, it);
            g_list_free_1(it);
        }

        it = next;
    }

    queued_list = g_list_concat(queued_list, list);

    PLAYLIST_UNLOCK();

    playlist_recalc_total_time();
    playlistwin_update_list();
}

void
playlist_queue_position(guint pos)
{
    GList *tmp;
    PlaylistEntry *entry;

    PLAYLIST_LOCK();
    entry = g_list_nth_data(playlist, pos);
    if ((tmp = g_list_find(queued_list, entry))) {
        queued_list = g_list_remove_link(queued_list, tmp);
        g_list_free_1(tmp);
    }
    else
        queued_list = g_list_append(queued_list, entry);
    PLAYLIST_UNLOCK();

    playlist_recalc_total_time();
    playlistwin_update_list();
}

gboolean
playlist_is_position_queued(guint pos)
{
    PlaylistEntry *entry;
    GList *tmp;

    PLAYLIST_LOCK();
    entry = g_list_nth_data(playlist, pos);
    tmp = g_list_find(queued_list, entry);
    PLAYLIST_UNLOCK();

    return tmp != NULL;
}

void
playlist_clear_queue(void)
{
    PLAYLIST_LOCK();
    g_list_free(queued_list);
    queued_list = NULL;
    PLAYLIST_UNLOCK();

    playlist_recalc_total_time();
    playlistwin_update_list();
}

void
playlist_queue_remove(guint pos)
{
    void *entry;

    PLAYLIST_LOCK();
    entry = g_list_nth_data(playlist, pos);
    queued_list = g_list_remove(queued_list, entry);
    PLAYLIST_UNLOCK();

    playlistwin_update_list();
}

gint
playlist_get_queue_position(PlaylistEntry * entry)
{
    return g_list_index(queued_list, entry);
}

void
playlist_set_position(guint pos)
{
    GList *node;
    gboolean restart_playing = FALSE;

    PLAYLIST_LOCK();
    if (!playlist) {
        PLAYLIST_UNLOCK();
        return;
    }

    node = g_list_nth(playlist, pos);
    if (!node) {
        PLAYLIST_UNLOCK();
        return;
    }

    if (bmp_playback_get_playing()) {
        /* We need to stop before changing playlist_position */
        PLAYLIST_UNLOCK();
        bmp_playback_stop();
        PLAYLIST_LOCK();
        restart_playing = TRUE;
    }

    playlist_position = node->data;
    PLAYLIST_UNLOCK();
    playlist_check_pos_current();

    if (restart_playing)
        bmp_playback_initiate();
    else {
        mainwin_set_info_text();
        playlistwin_update_list();
    }

    /*
     * Regenerate the shuffle list when the user set a position
     * manually
     */
    playlist_generate_shuffle_list();
    playlist_recalc_total_time();
}

void
playlist_eof_reached(void)
{
    GList *plist_pos_list;

    bmp_playback_stop();

    PLAYLIST_LOCK();
    plist_pos_list = find_playlist_position_list();

    if (cfg.no_playlist_advance) {
        PLAYLIST_UNLOCK();
        mainwin_clear_song_info();
        if (cfg.repeat)
            bmp_playback_initiate();
        return;
    }

    if (queued_list) {
        play_queued();
    }
    else if (!g_list_next(plist_pos_list)) {
        if (cfg.shuffle) {
            playlist_position = NULL;
            playlist_generate_shuffle_list_nolock();
        }
        else
            playlist_position = playlist->data;

        if (!cfg.repeat) {
            PLAYLIST_UNLOCK();
            mainwin_clear_song_info();
            mainwin_set_info_text();
            return;
        }
    }
    else
        playlist_position = g_list_next(plist_pos_list)->data;

    PLAYLIST_UNLOCK();

    playlist_check_pos_current();
    bmp_playback_initiate();
    mainwin_set_info_text();
    playlistwin_update_list();
}

gint
playlist_get_length(void)
{
    gint retval;

    PLAYLIST_LOCK();
    retval = playlist_get_length_nolock();
    PLAYLIST_UNLOCK();

    return retval;
}

gint
playlist_queue_get_length(void)
{
    gint length;

    PLAYLIST_LOCK();
    length = g_list_length(queued_list);
    PLAYLIST_UNLOCK();

    return length;
}

gint
playlist_get_length_nolock(void)
{
    REQUIRE_STATIC_LOCK(playlist);
    return g_list_length(playlist);
}

gchar *
playlist_get_info_text(void)
{
    gchar *text, *title, *numbers, *length;

    PLAYLIST_LOCK();
    if (!playlist_position) {
        PLAYLIST_UNLOCK();
        return NULL;
    }

    /* FIXME: there should not be a need to do additional conversion,
     * if playlist is properly maintained */
    if (playlist_position->title) {
        title = str_to_utf8(playlist_position->title);
    }
    else {
        gchar *basename = g_path_get_basename(playlist_position->filename);
        title = filename_to_utf8(basename);
        g_free(basename);
    }

    /*
     * If the user don't want numbers in the playlist, don't
     * display them in other parts of XMMS
     */

    if (cfg.show_numbers_in_pl)
        numbers = g_strdup_printf("%d. ", playlist_get_position_nolock() + 1);
    else
        numbers = g_strdup("");

    if (playlist_position->length != -1)
        length = g_strdup_printf(" (%d:%-2.2d)",
                                 playlist_position->length / 60000,
                                 (playlist_position->length / 1000) % 60);
    else
        length = g_strdup("");

    PLAYLIST_UNLOCK();

    text = convert_title_text(g_strconcat(numbers, title, length, NULL));

    g_free(numbers);
    g_free(title);
    g_free(length);

    return text;
}

gint
playlist_get_current_length(void)
{
    gint len = 0;

    PLAYLIST_LOCK();
    if (playlist && playlist_position)
        len = playlist_position->length;
    PLAYLIST_UNLOCK();

    return len;
}

static void
playlist_save_m3u(FILE * file)
{
    GList *node;

    g_return_if_fail(file != NULL);

    if (cfg.use_pl_metadata)
        g_fprintf(file, "#EXTM3U\n");

    PLAYLIST_LOCK();

    for (node = playlist; node; node = g_list_next(node)) {
        PlaylistEntry *entry = PLAYLIST_ENTRY(node->data);

        if (entry->title && cfg.use_pl_metadata) {
            gint seconds;

            if (entry->length > 0)
                seconds = (entry->length) / 1000;
            else
                seconds = -1;

            g_fprintf(file, "#EXTINF:%d,%s\n", seconds, entry->title);
        }

        g_fprintf(file, "%s\n", entry->filename);
    }

    PLAYLIST_UNLOCK();
}

static void
playlist_save_pls(FILE * file)
{
    GList *node;

    g_return_if_fail(file != NULL);

    g_fprintf(file, "[playlist]\n");
    g_fprintf(file, "NumberOfEntries=%d\n", playlist_get_length());

    PLAYLIST_LOCK();
    
    for (node = playlist; node; node = g_list_next(node)) {
        PlaylistEntry *entry = PLAYLIST_ENTRY(node->data);
        
        g_fprintf(file, "File%d=%s\n", g_list_position(playlist, node) + 1,
                  entry->filename);
    }
    
    PLAYLIST_UNLOCK();
}

gboolean
playlist_save(const gchar * filename,
              PlaylistFormat format)
{
    FILE *file;

    g_return_val_if_fail(filename != NULL, FALSE);

    playlist_set_current_name(filename);

    if ((file = fopen(filename, "w")) == NULL)
        return FALSE;

    playlist_save_func_table[format](file);
    
    return (fclose(file) == 0);
}

gboolean
playlist_load(const gchar * filename)
{
    gboolean ret = FALSE;

    loading_playlist = TRUE;
    ret = playlist_load_ins(filename, -1);
    loading_playlist = FALSE;

    return ret;
}


static void
playlist_load_ins_file(const gchar * filename_p,
                       const gchar * playlist_name, gint pos,
                       const gchar * title, gint len)
{
    gchar *filename;
    gchar *tmp, *path;
    InputPlugin *dec;		/* for decoder cache */

    g_return_if_fail(filename_p != NULL);
    g_return_if_fail(playlist_name != NULL);

    filename = g_strstrip(g_strdup(filename_p));

    if (cfg.use_backslash_as_dir_delimiter) {
        while ((tmp = strchr(filename, '\\')) != NULL)
            *tmp = '/';
    }

    if (filename[0] != '/' && !strstr(filename, "://")) {
        path = g_strdup(playlist_name);
        if ((tmp = strrchr(path, '/')))
            *tmp = '\0';
        else {
	    if (loading_playlist != TRUE)
	        dec = input_check_file(filename, FALSE);
	    else
		dec = NULL;

            __playlist_ins_with_info(filename, pos, title, len, dec);
            return;
        }
        tmp = g_build_filename(path, filename, NULL);

	if (loading_playlist != TRUE)
	    dec = input_check_file(tmp, FALSE);
	else
	    dec = NULL;

        __playlist_ins_with_info(tmp, pos, title, len, dec);
        g_free(tmp);
        g_free(path);
    }
    else
    {
	if (loading_playlist != TRUE)
	    dec = input_check_file(filename, FALSE);
	else
	    dec = NULL;

        __playlist_ins_with_info(filename, pos, title, len, dec);
    }

    g_free(filename);
}

static void
parse_extm3u_info(const gchar * info, gchar ** title, gint * length)
{
    gchar *str;

    g_return_if_fail(info != NULL);
    g_return_if_fail(title != NULL);
    g_return_if_fail(length != NULL);

    *title = NULL;
    *length = -1;

    if (!str_has_prefix_nocase(info, "#EXTINF:")) {
        g_message("Invalid m3u metadata (%s)", info);
        return;
    }

    info += 8;

    *length = atoi(info);
    if (*length <= 0)
        *length = -1;
    else
        *length *= 1000;

    if ((str = strchr(info, ','))) {
        *title = g_strstrip(g_strdup(str + 1));
        if (strlen(*title) < 1) {
            g_free(*title);
            *title = NULL;
        }
    }
}

static guint
playlist_load_pls(const gchar * filename, gint pos)
{
    guint i, count, added_count = 0;
    gchar key[10];
    gchar *line;

    g_return_val_if_fail(filename != NULL, 0);

    if (!str_has_suffix_nocase(filename, ".pls"))
        return 0;

    if (!(line = read_ini_string(filename, "playlist", "NumberOfEntries")))
        return 0;

    count = atoi(line);
    g_free(line);

    for (i = 1; i <= count; i++) {
        g_snprintf(key, sizeof(key), "File%d", i);
        if ((line = read_ini_string(filename, "playlist", key))) {
            playlist_load_ins_file(line, filename, pos, NULL, -1);
            added_count++;

            if (pos >= 0)
                pos++;

            g_free(line);
        }
    }

    playlist_generate_shuffle_list();
    playlistwin_update_list();

    return added_count;
}

static guint
playlist_load_m3u(const gchar * filename, gint pos)
{
    FILE *file;
    gchar *line;
    gchar *ext_info = NULL, *ext_title = NULL;
    gsize line_len = 1024;
    gint ext_len = -1;
    gboolean is_extm3u = FALSE;
    guint added_count = 0;

    if (!(file = fopen(filename, "r")))
        return 0;

    line = g_malloc(line_len);
    while (fgets(line, line_len, file)) {
        while (strlen(line) == line_len - 1 && line[strlen(line) - 1] != '\n') {
            line_len += 1024;
            line = g_realloc(line, line_len);
            fgets(&line[strlen(line)], 1024, file);
        }

        while (line[strlen(line) - 1] == '\r' ||
               line[strlen(line) - 1] == '\n')
            line[strlen(line) - 1] = '\0';

        if (str_has_prefix_nocase(line, "#EXTM3U")) {
            is_extm3u = TRUE;
            continue;
        }

        if (is_extm3u && str_has_prefix_nocase(line, "#EXTINF:")) {
            str_replace_in(&ext_info, g_strdup(line));
            continue;
        }

        if (line[0] == '#' || strlen(line) == 0) { 
            if (ext_info) {
                g_free(ext_info);
                ext_info = NULL;
            }
            continue;
        }

        if (is_extm3u) {
            if (cfg.use_pl_metadata && ext_info)
                parse_extm3u_info(ext_info, &ext_title, &ext_len);
            g_free(ext_info);
            ext_info = NULL;
        }

        playlist_load_ins_file(line, filename, pos, ext_title, ext_len);

        str_replace_in(&ext_title, NULL);
        ext_len = -1;

        added_count++;
        if (pos >= 0)
            pos++;
    }

    fclose(file);
    g_free(line);

    playlist_generate_shuffle_list();
    playlistwin_update_list();

    if (g_ascii_strcasecmp(filename, BMP_PLAYLIST_BASENAME))
        playlist_set_current_name(NULL);
    else
        playlist_set_current_name(filename);

    return added_count;
}

static guint
playlist_load_ins(const gchar * filename, gint pos)
{
    guint added_count;

    g_return_val_if_fail(filename != NULL, 0);

    /* .pls ? */
    if ((added_count = playlist_load_pls(filename, pos)) > 0)
        return added_count;

    /* Assume .m3u */
    return playlist_load_m3u(filename, pos);
}

GList *
get_playlist_nth(guint nth)
{
    REQUIRE_STATIC_LOCK(playlist);
    return g_list_nth(playlist, nth);
}


GList *
playlist_get(void)
{
    REQUIRE_STATIC_LOCK(playlist);
    return playlist;
}

gint
playlist_get_position_nolock(void)
{
    REQUIRE_STATIC_LOCK(playlist);

    if (playlist && playlist_position)
        return g_list_index(playlist, playlist_position);
    return 0;
}

gint
playlist_get_position(void)
{
    gint pos;

    PLAYLIST_LOCK();
    pos = playlist_get_position_nolock();
    PLAYLIST_UNLOCK();

    return pos;
}

gchar *
playlist_get_filename(guint pos)
{
    gchar *filename;
    PlaylistEntry *entry;
    GList *node;

    PLAYLIST_LOCK();
    if (!playlist) {
        PLAYLIST_UNLOCK();
        return NULL;
    }
    node = g_list_nth(playlist, pos);
    if (!node) {
        PLAYLIST_UNLOCK();
        return NULL;
    }
    entry = node->data;

    filename = g_strdup(entry->filename);
    PLAYLIST_UNLOCK();

    return filename;
}

gchar *
playlist_get_songtitle(guint pos)
{
    gchar *title = NULL;
    PlaylistEntry *entry;
    GList *node;

    PLAYLIST_LOCK();

    if (!playlist) {
        PLAYLIST_UNLOCK();
        return NULL;
    }

    if (!(node = g_list_nth(playlist, pos))) {
        PLAYLIST_UNLOCK();
        return NULL;
    }

    entry = node->data;

    /* FIXME: simplify this logic */
    if (!entry->title && entry->length == -1) {
        if (playlist_entry_get_info(entry))
            title = entry->title;
    }
    else {
        title = entry->title;
    }

    PLAYLIST_UNLOCK();

    if (!title) {
        title = g_path_get_basename(entry->filename);
        return str_replace(title, filename_to_utf8(title));
    }

    return str_to_utf8(title);
}

gint
playlist_get_songtime(guint pos)
{
    gint song_time = -1;
    PlaylistEntry *entry;
    GList *node;

    PLAYLIST_LOCK();

    if (!playlist) {
        PLAYLIST_UNLOCK();
        return -1;
    }

    if (!(node = g_list_nth(playlist, pos))) {
        PLAYLIST_UNLOCK();
        return -1;
    }

    entry = node->data;

    if (!entry->title && entry->length == -1) {
        if (playlist_entry_get_info(entry))
            song_time = entry->length;

        PLAYLIST_UNLOCK();
    }
    else {
        song_time = entry->length;
        PLAYLIST_UNLOCK();
    }

    return song_time;
}

static gint
playlist_compare_title(const PlaylistEntry * a,
                       const PlaylistEntry * b)
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

    return strcasecmp(a_title, b_title);
}

static gint
playlist_compare_filename(const PlaylistEntry * a,
                          const PlaylistEntry * b)
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
playlist_compare_path(const PlaylistEntry * a,
                      const PlaylistEntry * b)
{
    return path_compare(a->filename, b->filename);
}

static gint
playlist_compare_date(const PlaylistEntry * a,
                      const PlaylistEntry * b)
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
playlist_sort(PlaylistSortType type)
{
    playlist_remove_dead_files();
    PLAYLIST_LOCK();
    playlist =
        g_list_sort(playlist,
                    (GCompareFunc) playlist_compare_func_table[type]);
    PLAYLIST_UNLOCK();
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
playlist_sort_selected(PlaylistSortType type)
{
    PLAYLIST_LOCK();
    playlist = playlist_sort_selected_generic(playlist, (GCompareFunc)
                                              playlist_compare_func_table
                                              [type]);
    PLAYLIST_UNLOCK();
}

void
playlist_reverse(void)
{
    PLAYLIST_LOCK();
    playlist = g_list_reverse(playlist);
    PLAYLIST_UNLOCK();
}

static GList *
playlist_shuffle_list(GList * list)
{
    /*
     * Note that this doesn't make a copy of the original list.
     * The pointer to the original list is not valid after this
     * fuction is run.
     */
    gint len = g_list_length(list);
    gint i, j;
    GList *node, **ptrs;

    REQUIRE_STATIC_LOCK(playlist);

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
playlist_random(void)
{
    PLAYLIST_LOCK();
    playlist = playlist_shuffle_list(playlist);
    PLAYLIST_UNLOCK();
}

GList *
playlist_get_selected(void)
{
    GList *node, *list = NULL;
    gint i = 0;

    PLAYLIST_LOCK();
    for (node = playlist_get(); node; node = g_list_next(node), i++) {
        PlaylistEntry *entry = node->data;
        if (entry->selected)
            list = g_list_prepend(list, GINT_TO_POINTER(i));
    }
    PLAYLIST_UNLOCK();
    return g_list_reverse(list);
}

void
playlist_clear_selected(void)
{
    GList *node = NULL;
    gint i = 0;

    PLAYLIST_LOCK();
    for (node = playlist_get(); node; node = g_list_next(node), i++) {
        PLAYLIST_ENTRY(node->data)->selected = FALSE;
    }
    PLAYLIST_UNLOCK();
    playlist_recalc_total_time();
}

gint
playlist_get_num_selected(void)
{
    GList *node;
    gint num = 0;

    PLAYLIST_LOCK();
    for (node = playlist_get(); node; node = g_list_next(node)) {
        PlaylistEntry *entry = node->data;
        if (entry->selected)
            num++;
    }
    PLAYLIST_UNLOCK();
    return num;
}


static void
playlist_generate_shuffle_list(void)
{
    PLAYLIST_LOCK();
    playlist_generate_shuffle_list_nolock();
    PLAYLIST_UNLOCK();
}

static void
playlist_generate_shuffle_list_nolock(void)
{
    GList *node;
    gint numsongs;

    REQUIRE_STATIC_LOCK(playlist);

    if (shuffle_list) {
        g_list_free(shuffle_list);
        shuffle_list = NULL;
    }

    if (!cfg.shuffle || !playlist)
        return;

    shuffle_list = playlist_shuffle_list(g_list_copy(playlist));
    numsongs = g_list_length(shuffle_list);

    if (playlist_position) {
        gint i = g_list_index(shuffle_list, playlist_position);
        node = g_list_nth(shuffle_list, i);
        shuffle_list = g_list_remove_link(shuffle_list, node);
        shuffle_list = g_list_prepend(shuffle_list, node->data);
    }
}

void
playlist_fileinfo(guint pos)
{
    gchar *path = NULL;
    GList *node;

    PLAYLIST_LOCK();
    if ((node = g_list_nth(playlist_get(), pos))) {
        PlaylistEntry *entry = node->data;
        path = g_strdup(entry->filename);
    }
    PLAYLIST_UNLOCK();
    if (path) {
        input_file_info_box(path);
        g_free(path);
    }
}

void
playlist_fileinfo_current(void)
{
    gchar *path = NULL;

    PLAYLIST_LOCK();
    if (playlist_get() && playlist_position)
        path = g_strdup(playlist_position->filename);
    PLAYLIST_UNLOCK();

    if (path) {
        input_file_info_box(path);
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

static gpointer
playlist_get_info_func(gpointer arg)
{
    GList *node;
    gboolean update_playlistwin = FALSE;
    gboolean update_mainwin = FALSE;

    while (playlist_get_info_is_going()) {
        PlaylistEntry *entry;

        if (cfg.use_pl_metadata &&
            cfg.get_info_on_load &&
            playlist_get_info_scan_active) {

            PLAYLIST_LOCK();
            for (node = playlist_get(); node; node = g_list_next(node)) {
                entry = node->data;

                if (entry->title || entry->length != -1)
                    continue;

                if (!playlist_entry_get_info(entry)) {
                    if (g_list_index(playlist_get(), entry) == -1)
                        /* Entry disappeared while we looked it up.
                           Restart. */
                        node = playlist_get();
                }
                else if (entry->title || entry->length != -1) {
                    update_playlistwin = TRUE;
                    if (entry == playlist_position)
                        update_mainwin = TRUE;
                    break;
                }
            }
            PLAYLIST_UNLOCK();

            if (!node)
                playlist_get_info_scan_active = FALSE;
        }
        else if (!cfg.get_info_on_load &&
                 cfg.get_info_on_demand &&
                 cfg.playlist_visible &&
                 !cfg.playlist_shaded &&
                 cfg.use_pl_metadata) {

            gboolean found = FALSE;

            PLAYLIST_LOCK();

            if (!playlist_get()) {
                PLAYLIST_UNLOCK();
                g_usleep(1000000);
                continue;
            }

            for (node =
                 g_list_nth(playlist_get(), playlistwin_get_toprow());
                 node
                 &&
                 playlistwin_item_visible(g_list_position
                                          (playlist_get(), node));
                 node = g_list_next(node)) {
                entry = node->data;
                if (entry->title || entry->length != -1)
                    continue;

                if (!playlist_entry_get_info(entry)) {
                    if (g_list_index(playlist_get(), entry) == -1)
                        /* Entry disapeared while we
                           looked it up.  Restart. */
                        node =
                            g_list_nth(playlist_get(),
                                       playlistwin_get_toprow());
                }
                else if (entry->title || entry->length != -1) {
                    update_playlistwin = TRUE;
                    if (entry == playlist_position)
                        update_mainwin = TRUE;
                    found = TRUE;
                    break;
                }
            }

            PLAYLIST_UNLOCK();

            if (!found) {
                g_usleep(500000);
                continue;
            }
        }
        else
            g_usleep(500000);

        if (update_playlistwin) {
            playlistwin_update_list();
            update_playlistwin = FALSE;
        }

        if (update_mainwin) {
            mainwin_set_info_text();
            update_mainwin = FALSE;
        }
    }

    g_thread_exit(NULL);
    return NULL;
}

void
playlist_start_get_info_thread(void)
{
    playlist_get_info_going = TRUE;
    playlist_get_info_thread = g_thread_create(playlist_get_info_func,
                                               NULL, TRUE, NULL);
}

void
playlist_stop_get_info_thread(void)
{
    G_LOCK(playlist_get_info_going);
    playlist_get_info_going = FALSE;
    G_UNLOCK(playlist_get_info_going);
    g_thread_join(playlist_get_info_thread);
}

void
playlist_start_get_info_scan(void)
{
    playlist_get_info_scan_active = TRUE;
}

void
playlist_remove_dead_files(void)
{
    GList *node, *next_node;

    PLAYLIST_LOCK();

    for (node = playlist; node; node = next_node) {
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

        if (entry == playlist_position) {
            /* Don't remove the currently playing song */
            if (bmp_playback_get_playing())
                continue;

            if (next_node)
                playlist_position = PLAYLIST_ENTRY(next_node->data);
            else
                playlist_position = NULL;
        }

        playlist_entry_free(entry);
        playlist = g_list_delete_link(playlist, node);
    }
   
    PLAYLIST_UNLOCK();
    
    playlist_generate_shuffle_list();
    playlistwin_update_list();
    playlist_recalc_total_time();
}

static gulong pl_total_time = 0, pl_selection_time = 0;
static gboolean pl_total_more = FALSE, pl_selection_more = FALSE;

void
playlist_get_total_time(gulong * total_time,
                        gulong * selection_time,
                        gboolean * total_more,
                        gboolean * selection_more)
{
    PLAYLIST_LOCK();
    *total_time = pl_total_time;
    *selection_time = pl_selection_time;
    *total_more = pl_total_more;
    *selection_more = pl_selection_more;
    PLAYLIST_UNLOCK();
}


static void
playlist_recalc_total_time_nolock(void)
{
    GList *list;
    PlaylistEntry *entry;

    REQUIRE_STATIC_LOCK(playlist);

    pl_total_time = 0;
    pl_selection_time = 0;
    pl_total_more = FALSE;
    pl_selection_more = FALSE;

    for (list = playlist_get(); list; list = g_list_next(list)) {
        entry = list->data;

        if (entry->length != -1)
            pl_total_time += entry->length / 1000;
        else
            pl_total_more = TRUE;

        if (entry->selected) {
            if (entry->length != -1)
                pl_selection_time += entry->length / 1000;
            else
                pl_selection_more = TRUE;
        }
    }
}

static void
playlist_recalc_total_time(void)
{
    PLAYLIST_LOCK();
    playlist_recalc_total_time_nolock();
    PLAYLIST_UNLOCK();
}


void
playlist_select_all(gboolean set)
{
    GList *list;

    PLAYLIST_LOCK();

    for (list = playlist_get(); list; list = g_list_next(list)) {
        PlaylistEntry *entry = list->data;
        entry->selected = set;
    }

    PLAYLIST_UNLOCK();
    playlist_recalc_total_time();
}

void
playlist_select_invert_all(void)
{
    GList *list;

    PLAYLIST_LOCK();

    for (list = playlist_get(); list; list = g_list_next(list)) {
        PlaylistEntry *entry = list->data;
        entry->selected = !entry->selected;
    }

    PLAYLIST_UNLOCK();
    playlist_recalc_total_time();
}

gboolean
playlist_select_invert(guint pos)
{
    GList *list;
    gboolean invert_ok = FALSE;

    PLAYLIST_LOCK();

    if ((list = g_list_nth(playlist_get(), pos))) {
        PlaylistEntry *entry = list->data;
        entry->selected = !entry->selected;
        invert_ok = TRUE;
    }

    PLAYLIST_UNLOCK();
    playlist_recalc_total_time();

    return invert_ok;
}


void
playlist_select_range(gint min_pos, gint max_pos, gboolean select)
{
    GList *list;
    gint i;

    if (min_pos > max_pos)
        SWAP(min_pos, max_pos);

    PLAYLIST_LOCK();

    list = g_list_nth(playlist_get(), min_pos);
    for (i = min_pos; i <= max_pos && list; i++) {
        PlaylistEntry *entry = list->data;
        entry->selected = select;
        list = g_list_next(list);
    }

    PLAYLIST_UNLOCK();

    playlist_recalc_total_time();
}

gboolean
playlist_read_info_selection(void)
{
    GList *node;
    gboolean retval = FALSE;

    PLAYLIST_LOCK();

    for (node = playlist_get(); node; node = g_list_next(node)) {
        PlaylistEntry *entry = node->data;
        if (!entry->selected)
            continue;

        retval = TRUE;

        str_replace_in(&entry->title, NULL);
        entry->length = -1;

        if (!playlist_entry_get_info(entry)) {
            if (g_list_index(playlist_get(), entry) == -1)
                /* Entry disappeared while we looked it up. Restart. */
                node = playlist_get();
        }
    }

    PLAYLIST_UNLOCK();

    playlistwin_update_list();
    playlist_recalc_total_time();

    return retval;
}

void
playlist_read_info(guint pos)
{
    GList *node;

    PLAYLIST_LOCK();

    if ((node = g_list_nth(playlist_get(), pos))) {
        PlaylistEntry *entry = node->data;
        str_replace_in(&entry->title, NULL);
        entry->length = -1;
        playlist_entry_get_info(entry);
    }

    PLAYLIST_UNLOCK();

    playlistwin_update_list();
    playlist_recalc_total_time();
}

void
playlist_set_shuffle(gboolean shuffle)
{
    PLAYLIST_LOCK();

    cfg.shuffle = shuffle;
    playlist_generate_shuffle_list_nolock();

    PLAYLIST_UNLOCK();
}

void
playlist_new(void) 
{
    playlist_set_current_name(NULL);
    playlist_clear();
    mainwin_clear_song_info();
    mainwin_set_info_text();
}


const gchar *
playlist_get_filename_to_play(void)
{
    const gchar *filename = NULL;
    
    PLAYLIST_LOCK();

    if (playlist) {
        if (!playlist_position) {
            if (cfg.shuffle)
                playlist_position = shuffle_list->data;
            else
                playlist_position = playlist->data;
        }

        filename = playlist_position->filename;
    }

    PLAYLIST_UNLOCK();

    return filename;
}

PlaylistEntry *
playlist_get_entry_to_play(void)
{
    PLAYLIST_LOCK();

    if (playlist) {
        if (!playlist_position) {
            if (cfg.shuffle)
                playlist_position = shuffle_list->data;
            else
                playlist_position = playlist->data;
        }
    }

    PLAYLIST_UNLOCK();

    return playlist_position;
}
