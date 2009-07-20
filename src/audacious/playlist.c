/*  Audacious
 *  Copyright (C) 2005-2009  Audacious team.
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
#ifdef HAVE_SYS_ERRNO_H
#  include <sys/errno.h>
#endif
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
#include "scanner.h"
#include "audstrings.h"
#include "util.h"
#include "vfs.h"
#include "debug.h"

typedef gint (*PlaylistCompareFunc) (PlaylistEntry * a, PlaylistEntry * b);
typedef void (*PlaylistSaveFunc) (FILE * file);

static GList *playlists = NULL;

extern GHashTable *ext_hash;

static gint path_compare(const gchar * a, const gchar * b);
static gint playlist_compare_path(PlaylistEntry * a, PlaylistEntry * b);
static gint playlist_compare_filename(PlaylistEntry * a, PlaylistEntry * b);
static gint playlist_compare_title(PlaylistEntry * a, PlaylistEntry * b);
static gint playlist_compare_album(PlaylistEntry * a, PlaylistEntry * b);
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
    playlist_compare_album,
    playlist_compare_artist,
    playlist_compare_date,
    playlist_compare_track,
    playlist_compare_playlist
};

static guint playlist_load_ins(Playlist * playlist, const gchar * filename, gint pos);

static void playlist_generate_shuffle_list_nolock(Playlist *);

static void playlist_recalc_total_time_nolock(Playlist *);
static void playlist_recalc_total_time(Playlist *);

const gchar *aud_titlestring_presets[] = {
    "${title}",
    "${?artist:${artist} - }${title}",
    "${?artist:${artist} - }${?album:${album} - }${title}",
    "${?artist:${artist} - }${?album:${album} - }${?track-number:${track-number}. }${title}",
    "${?artist:${artist} }${?album:[ ${album} ] }${?artist:- }${?track-number:${track-number}. }${title}",
    "${?album:${album} - }${title}"
};
const guint n_titlestring_presets = G_N_ELEMENTS(aud_titlestring_presets);

static mowgli_heap_t *playlist_entry_heap = NULL;

static InputPlugin *
find_decoder(const gchar * uri)
{
    InputPlugin *decoder = NULL;
    gchar *temp = filename_split_subtune(uri, NULL);
    gchar *temp2;
    GList **index;

    decoder = uri_get_plugin(temp);
    if (decoder != NULL)
        goto DONE;

    temp2 = strrchr(temp, '.');
    if (temp2 == NULL)
        goto DONE;

    temp2 = g_utf8_strdown(temp2 + 1, -1);
    g_free(temp);
    temp = temp2;

    index = g_hash_table_lookup(ext_hash, temp);

    if (index != NULL)
        decoder = (*index)->data;

DONE:
     g_free(temp);
     return decoder;
}

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
    entry->title = g_strdup ((title != NULL) ? title : filename);
    entry->length = length;
    entry->selected = FALSE;
    entry->decoder = dec;

    entry->tuple = NULL;
    entry->failed = FALSE;

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

static void playlist_entry_set_tuple (PlaylistEntry * entry, Tuple * tuple)
{
    const gchar * format = tuple_get_string (tuple, FIELD_FORMATTER, NULL);

    if (entry->tuple != NULL)
        tuple_free (entry->tuple);

    g_free (entry->title);

    entry->tuple = tuple;
    entry->failed = FALSE;
    entry->title = tuple_formatter_make_title_string (tuple, format ? format :
     get_gentitle_format ());
    entry->length = tuple_get_int (tuple, FIELD_LENGTH, NULL);
}

void playlist_entry_get_info (PlaylistEntry * entry)
{
    Tuple * tuple = NULL;

    if (entry->decoder == NULL)
        entry->decoder = find_decoder (entry->filename);

    if (tuple == NULL && entry->decoder != NULL &&
     entry->decoder->get_song_tuple != NULL)
        tuple = entry->decoder->get_song_tuple (entry->filename);

    if (tuple != NULL)
        playlist_entry_set_tuple (entry, tuple);
    else
        entry->failed = TRUE;
}

/* *********************** playlist selector code ************************* */

void playlist_init (void)
{
    Playlist * playlist;

    /* create a heap with 1024 playlist entry nodes preallocated. --nenolod */
    playlist_entry_heap = mowgli_heap_create(sizeof(PlaylistEntry), 1024,
                                             BH_NOW);

    playlist = playlist_new ();
    playlists = g_list_append (NULL, playlist);
    set_active_playlist (playlist);
}

void playlist_end (void)
{
    set_active_playlist (NULL);

    while (playlists != NULL)
    {
        playlist_clear (playlists->data);
        playlist_free (playlists->data);
        playlists = g_list_delete_link (playlists, playlists);
    }
}

void
playlist_add_playlist(Playlist *playlist)
{
    playlists = g_list_append(playlists, playlist);

    hook_call("playlist create", playlist);
    hook_call ("playlist update", 0);
}

void
playlist_remove_playlist(Playlist *playlist)
{
    gboolean active;
    GList * node;

    active = (playlist == get_active_playlist ());

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
        playlist_set_current_name(playlist, _("Untitled Playlist"));
        playlist_filename_set(playlist, NULL);
        return;
    }

    hook_call("playlist destroy", playlist);

    if (active)
    {
        node = g_list_find (playlists, playlist);
        set_active_playlist (node->next ? node->next->data : node->prev->data);
    }

    /* upon removal, a playlist should be cleared and freed */
    playlists = g_list_remove(playlists, playlist);
    playlist_clear_only (playlist);
    playlist_free(playlist);

    hook_call ("playlist update", 0);
}

GList *
playlist_get_playlists(void)
{
    return playlists;
}

void
playlist_select_next(void)
{
    GList * node = g_list_find (playlists, get_active_playlist ());

    if (! node->next)
        return;

    set_active_playlist (node->next->data);
    hook_call ("playlist update", 0);
}

void
playlist_select_prev(void)
{
    GList * node = g_list_find (playlists, get_active_playlist ());

    if (! node->prev)
        return;

    set_active_playlist (node->prev->data);
    hook_call ("playlist update", 0);
}

void
playlist_select_playlist(Playlist *playlist)
{
    set_active_playlist (playlist);
    hook_call ("playlist update", 0);
}

/* *********************** playlist code ********************** */

/* For compatibility with older code. -jlindgren */
Playlist * playlist_get_active (void)
{
    return get_active_playlist ();
}

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
    g_free (playlist->title);
    playlist->title = g_strdup (title);

    hook_call ("playlist update", playlist);
    return 1;
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
    playlist->attribute = PLAYLIST_PLAIN;
    playlist->serial = 0;

    PLAYLIST_UNLOCK(playlist);
}

void
playlist_shift(Playlist *playlist, gint delta)
{
    gint orig_delta;
    GList *n;
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

    if (orig_delta < 0)
    {
        for (delta = orig_delta; delta < 0; delta++)
        {
            MOWGLI_ITER_FOREACH(n, playlist->entries)
            {
                PlaylistEntry *entry = PLAYLIST_ENTRY(n->data);

                if (!entry->selected)
                    continue;

                glist_moveup(n);
            }
        }
    }
    else
    {
        for (delta = orig_delta; delta > 0; delta--)
        {
            MOWGLI_ITER_FOREACH_PREV (n, g_list_last (playlist->entries))
            {
                PlaylistEntry *entry = PLAYLIST_ENTRY(n->data);

                if (!entry->selected)
                    continue;

                glist_movedown(n);
            }
        }
    }

    /* do the remaining work. */
    playlist_generate_shuffle_list_nolock(playlist);
    PLAYLIST_INCR_SERIAL(playlist);
    PLAYLIST_UNLOCK(playlist);

    hook_call ("playlist update", playlist);
}

void
playlist_clear(Playlist *playlist)
{
    if (!playlist)
        return;

    playlist_clear_only(playlist);
    playlist_generate_shuffle_list(playlist);
    playlist_recalc_total_time(playlist);
    PLAYLIST_INCR_SERIAL(playlist);

    hook_call ("playlist update", playlist);
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

    if (restart_playing) {
        if (playlist->position)
            playback_initiate();
        else
            hook_call("playlist end reached", NULL);
    }

    hook_call ("playlist update", playlist);
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

    hook_call ("playlist update", playlist);
}

static void insert_file (Playlist * playlist, const gchar * uri, gint position,
 Tuple * tuple, const gchar * title, gint length, InputPlugin * decoder)
{
    PlaylistEntry * entry;

    if (tuple == NULL && decoder == NULL)
        decoder = find_decoder (uri);

    if (tuple == NULL && decoder != NULL && decoder->have_subtune &&
     decoder->get_song_tuple != NULL)
        tuple = decoder->get_song_tuple ((gchar *) uri);

    if (tuple != NULL && tuple->nsubtunes > 0)
    {
        gint subtune;

        for (subtune = 0; subtune < tuple->nsubtunes; subtune ++)
        {
            gchar * name = g_strdup_printf ("%s?%d", uri, (tuple->subtunes ==
             NULL) ? 1 + subtune : tuple->subtunes[subtune]);

            insert_file (playlist, name, position == -1 ? -1 : position +
             subtune, NULL, NULL, -1, decoder);
            g_free (name);
        }

        tuple_free (tuple);
        return;
    }

    entry = playlist_entry_new (uri, title, length, decoder);

    if (tuple != NULL)
        playlist_entry_set_tuple (entry, tuple);

    PLAYLIST_LOCK (playlist);

    if (position == -1)
        playlist->entries = g_list_append (playlist->entries, entry);
    else
        playlist->entries = g_list_insert (playlist->entries, entry, position);

    PLAYLIST_UNLOCK (playlist);

    if (tuple == NULL)
        scanner_reset ();
}

gboolean playlist_ins (Playlist * playlist, const gchar * filename, gint pos)
{
    if (is_playlist_name (filename))
    {
        playlist_load_ins (playlist, filename, pos);
        return TRUE;
    }

    insert_file (playlist, filename, pos, NULL, NULL, -1, NULL);
    playlist_generate_shuffle_list (playlist);

    hook_call ("playlist update", playlist);
    return TRUE;
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

        if (vfs_file_test(filename, G_FILE_TEST_IS_DIR)) { /* directory */
            GList *sub;
            gchar *dirfilename = g_filename_from_uri(filename, NULL, NULL);
            sub = playlist_dir_find_files(dirfilename, background, htab);
            g_free(dirfilename);
            g_free(filename);
            list = g_list_concat(list, sub);
        }
        else
            list = g_list_prepend (list, filename);

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

    hook_call ("playlist update", playlist);
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

    hook_call ("playlist update", playlist);
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
}

/* wrapper for playlist_set_info. this function is called by input plugins. */
void
playlist_set_info_old_abi(const gchar * title, gint length, gint rate,
                          gint freq, gint nch)
{
    playlist_set_info (get_active_playlist (), title, length, rate, freq, nch);
}

void
playlist_next(Playlist *playlist)
{
    GList *plist_pos_list;
    gboolean restart_playing = FALSE;

    if (!playlist_get_length(playlist))
        return;

    PLAYLIST_LOCK(playlist);

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

    hook_call ("playlist position", playlist);
}

void
playlist_prev(Playlist *playlist)
{
    GList *plist_pos_list;
    gboolean restart_playing = FALSE;

    if (!playlist_get_length(playlist))
        return;

    PLAYLIST_LOCK(playlist);

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

    hook_call ("playlist position", playlist);
}

void
playlist_queue(Playlist *playlist)
{
    GList *list = playlist_get_selected(playlist);
    GList *it = list;

    PLAYLIST_LOCK(playlist);

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

    hook_call ("playlist update", playlist);
}

void
playlist_queue_position(Playlist *playlist, guint pos)
{
    GList *tmp;
    PlaylistEntry *entry;

    PLAYLIST_LOCK(playlist);

    entry = g_list_nth_data(playlist->entries, pos);
    if ((tmp = g_list_find(playlist->queue, entry))) {
        playlist->queue = g_list_remove_link(playlist->queue, tmp);
        g_list_free_1(tmp);
    }
    else
        playlist->queue = g_list_append(playlist->queue, entry);
    PLAYLIST_UNLOCK(playlist);

    playlist_recalc_total_time(playlist);

    hook_call ("playlist update", playlist);
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

    hook_call ("playlist update", playlist);
}

void
playlist_queue_remove(Playlist *playlist, guint pos)
{
    void *entry;

    PLAYLIST_LOCK(playlist);
    entry = g_list_nth_data(playlist->entries, pos);
    playlist->queue = g_list_remove(playlist->queue, entry);
    PLAYLIST_UNLOCK(playlist);

    hook_call ("playlist update", playlist);
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

    playlist->position = node->data;
    playlist_generate_shuffle_list_nolock (playlist);
    PLAYLIST_UNLOCK(playlist);

    if (restart_playing)
        playback_initiate();

    hook_call ("playlist position", playlist);
}

void playlist_eof_reached (Playlist * playlist)
{
    gboolean play;

    PLAYLIST_LOCK (playlist);

    playlist->queue = g_list_remove (playlist->queue, playlist->position);

    if (playlist->queue != NULL)
    {
        playlist->position = playlist->queue->data;
        play = TRUE;
    }
    else if (cfg.no_playlist_advance)
        play = cfg.repeat;
    else
    {
        GList * node;

        play = ! cfg.stopaftersong;

        if (cfg.shuffle)
        {
            node = g_list_find (playlist->shuffle, playlist->position);

            if (node != NULL)
                node = node->next;

            if (node == NULL && cfg.repeat)
            {
                playlist_generate_shuffle_list_nolock (playlist);
                node = playlist->shuffle;
            }
        }
        else
        {
            node = g_list_find (playlist->entries, playlist->position);

            if (node != NULL)
                node = node->next;

            if (node == NULL && cfg.repeat)
                node = playlist->entries;
        }

        if (node != NULL)
            playlist->position = node->data;
        else
            play = FALSE;
    }

    PLAYLIST_UNLOCK(playlist);

    hook_call ("playlist position", playlist);

    if (play)
        playback_initiate ();
    else
        hook_call ("playback stop", NULL);
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

    if (cfg.show_numbers_in_pl)
       numbers = g_strdup_printf ("%d. ", 1 + g_list_index (playlist->entries,
        playlist->position));
    else
       numbers = g_strdup ("");

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
    gchar *ext;
    Playlist * last;

    g_return_val_if_fail(playlist != NULL, FALSE);
    g_return_val_if_fail(filename != NULL, FALSE);

    ext = strrchr(filename, '.') + 1;

    if ((plc = playlist_container_find(ext)) == NULL)
        return FALSE;

    if (plc->plc_write == NULL)
        return FALSE;

    /* Fix me: We shouldn't have to activate a playlist to save it. */
    last = get_active_playlist ();
    set_active_playlist (playlist);
    plc->plc_write (filename, 0);
    set_active_playlist (last);

    return TRUE;
}

gboolean
playlist_load(Playlist * playlist, const gchar * filename)
{
    if(!playlist_get_length(playlist)) {
        /* Loading new playlist */
        playlist_filename_set(playlist, filename);
    }

    return playlist_load_ins (playlist, filename, -1);
}

void playlist_load_ins_file (Playlist * playlist, const gchar * uri, const gchar
 * playlist_name, gint pos, const gchar * title, gint len)
{
    insert_file (playlist, uri, pos, NULL, title, len, NULL);
}

void playlist_load_ins_file_tuple (Playlist * playlist, const gchar * uri, const
 gchar * playlist_name, gint pos, Tuple * tuple)
{
    insert_file (playlist, uri, pos, tuple, NULL, -1, NULL);
}

static guint
playlist_load_ins(Playlist * playlist, const gchar * filename, gint pos)
{
    PlaylistContainer *plc;
    gchar *ext;
    gint old_len;
    Playlist * last;

    g_return_val_if_fail(playlist != NULL, 0);
    g_return_val_if_fail(filename != NULL, 0);

    ext = strrchr(filename, '.') + 1;
    plc = playlist_container_find(ext);

    g_return_val_if_fail(plc != NULL, 0);
    g_return_val_if_fail(plc->plc_read != NULL, 0);

    old_len = playlist_get_length(playlist);

    /* Fix me: We shouldn't have to activate a playlist to load into it. */
    last = get_active_playlist ();
    set_active_playlist (playlist);
    plc->plc_read (filename, pos);
    set_active_playlist (last);

    playlist_generate_shuffle_list(playlist);

    playlist_recalc_total_time(playlist); //tentative --yaz
    PLAYLIST_INCR_SERIAL(playlist);

    hook_call ("playlist load", playlist);
    hook_call ("playlist update", playlist);

    return playlist_get_length (playlist) - old_len;
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

    if (!playlist)
        return NULL;

    PLAYLIST_LOCK(playlist);

    if (!(node = g_list_nth(playlist->entries, pos))) {
        PLAYLIST_UNLOCK(playlist);
        return NULL;
    }

    entry = node->data;

    if (entry->tuple == NULL && !entry->failed)
        playlist_entry_get_info (entry);

    title = entry->title;

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
    GList *node;

    if (!playlist)
        return NULL;

    if (!(node = g_list_nth(playlist->entries, pos))) {
        return NULL;
    }

    entry = (PlaylistEntry *) node->data;

    if (entry->tuple == NULL && !entry->failed)
        playlist_entry_get_info (entry);

    return entry->tuple;
}

gint
playlist_get_songtime(Playlist *playlist, guint pos)
{
    PlaylistEntry *entry;
    GList *node;

    if (!(node = g_list_nth(playlist->entries, pos)))
        return -1;

    entry = node->data;

    if (entry->tuple == NULL && !entry->failed)
        playlist_entry_get_info (entry);

    return entry->length;
}

static gint
playlist_compare_track(PlaylistEntry * a, PlaylistEntry * b)
{
    gint tracknumber_a;
    gint tracknumber_b;

    g_return_val_if_fail(a != NULL, 0);
    g_return_val_if_fail(b != NULL, 0);

    if (a->tuple == NULL && !a->failed)
        playlist_entry_get_info (a);
    if (b->tuple == NULL && !b->failed)
        playlist_entry_get_info (b);

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

    if (a->tuple == NULL && !a->failed)
        playlist_entry_get_info (a);
    if (b->tuple == NULL && !b->failed)
        playlist_entry_get_info (b);

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

    if (a->tuple == NULL && !a->failed)
        playlist_entry_get_info (a);
    if (b->tuple == NULL && !b->failed)
        playlist_entry_get_info (b);

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
playlist_compare_album(PlaylistEntry * a,
                       PlaylistEntry * b)
{
    const gchar *a_album = NULL, *b_album = NULL;

    g_return_val_if_fail(a != NULL, 0);
    g_return_val_if_fail(b != NULL, 0);

    if (a->tuple == NULL && !a->failed)
        playlist_entry_get_info (a);
    if (b->tuple == NULL && !b->failed)
        playlist_entry_get_info (b);

    if (a->tuple != NULL) {
        a_album = tuple_get_string(a->tuple, FIELD_ALBUM, NULL);

        if (a_album == NULL)
            return 0;

        if (str_has_prefix_nocase(a_album, "the "))
            a_album += 4;
    }

    if (b->tuple != NULL) {
        b_album = tuple_get_string(b->tuple, FIELD_ALBUM, NULL);

        if (b_album == NULL)
            return 0;

        if (str_has_prefix_nocase(b_album, "the "))
            b_album += 4;
    }

    if (a_album != NULL && b_album != NULL)
        return strcasecmp(a_album, b_album);

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
  time_t a_time, b_time;
   a_time = playlist_get_mtime (a->filename);
   b_time = playlist_get_mtime (b->filename);
   if (a_time < b_time)
      return -1;
   if (a_time > b_time)
      return 1;
   return playlist_compare_filename (a, b);
}


void
playlist_sort(Playlist *playlist, PlaylistSortType type)
{
    playlist_remove_dead_files(playlist);
    PLAYLIST_LOCK(playlist);
    playlist->entries =
        g_list_sort(playlist->entries,
                    (GCompareFunc) playlist_compare_func_table[type]);
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
    PLAYLIST_INCR_SERIAL(playlist);
    PLAYLIST_UNLOCK(playlist);
}

void
playlist_reverse(Playlist *playlist)
{
    PLAYLIST_LOCK(playlist);
    playlist->entries = g_list_reverse(playlist->entries);
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

GList *
playlist_get_selected_elems(Playlist *playlist)
{
    GList *node, *list = NULL;
    gint i = 0;

    for (node = playlist->entries; node; node = g_list_next(node), i++) {
        PlaylistEntry *entry = node->data;
        if (entry->selected)
            list = g_list_prepend(list, node);
    }

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

    /* commenting out for now,
     * fix redraw playlist-widget in newui on every select - Michal */
    /* Don't break skins just because gtkui is broken! -jlindgren */
    hook_call ("playlist update", playlist);
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

void playlist_generate_shuffle_list (Playlist * playlist)
{
    PLAYLIST_LOCK(playlist);
    playlist_generate_shuffle_list_nolock(playlist);
    PLAYLIST_UNLOCK(playlist);
}

static void
playlist_generate_shuffle_list_nolock(Playlist *playlist)
{
    GList *node;

    if (!cfg.shuffle || !playlist)
        return;

    REQUIRE_LOCK(playlist->mutex);

    if (playlist->shuffle) {
        g_list_free(playlist->shuffle);
        playlist->shuffle = NULL;
    }

    playlist->shuffle = playlist_shuffle_list(playlist, g_list_copy(playlist->entries));

    if (playlist->position) {
        gint i = g_list_index(playlist->shuffle, playlist->position);
        node = g_list_nth(playlist->shuffle, i);
        playlist->shuffle = g_list_remove_link(playlist->shuffle, node);
        playlist->shuffle = g_list_prepend(playlist->shuffle, node->data);
    }
}

void
playlist_update_all_titles(void) /* update titles after format changing --asphyx */
{
    PlaylistEntry *entry;
    GList *node;
    Playlist * playlist = get_active_playlist ();
    const char * format;

    PLAYLIST_LOCK(playlist);
    for (node = playlist->entries; node; node = g_list_next(node)) {
        entry = node->data;

        if (! entry->tuple)
            continue;

        g_free (entry->title);
        format = tuple_get_string (entry->tuple, FIELD_FORMATTER, 0);
        entry->title = tuple_formatter_make_title_string (entry->tuple, format ?
         format : get_gentitle_format ());
    }
    PLAYLIST_UNLOCK(playlist);

    hook_call ("playlist update", playlist);
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
    playlist_recalc_total_time(playlist);
    PLAYLIST_INCR_SERIAL(playlist);

    hook_call ("playlist update", playlist);
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

    playlist_recalc_total_time(playlist);
    PLAYLIST_INCR_SERIAL(playlist);

    hook_call ("playlist update", playlist);
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

    hook_call ("playlist update", playlist);
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

    hook_call ("playlist update", playlist);
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

    hook_call ("playlist update", playlist);
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

    hook_call ("playlist update", playlist);
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

    hook_call ("playlist update", playlist);
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

        playlist_entry_get_info (entry);
    }

    PLAYLIST_UNLOCK(playlist);

    playlist_recalc_total_time(playlist);

    hook_call ("playlist update", playlist);
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

    playlist_recalc_total_time(playlist);
    PLAYLIST_INCR_SERIAL(playlist); //tentative --yaz

    hook_call ("playlist update", playlist);
}

void playlist_rescan (Playlist * playlist)
{
    GList * node;
    PlaylistEntry * entry;

    PLAYLIST_LOCK (playlist);

    for (node = playlist->entries; node; node = node->next)
    {
        entry = node->data;

        if (entry->tuple)
        {
            tuple_free (entry->tuple);
            entry->tuple = NULL;
            entry->failed = FALSE;
        }
    }

    PLAYLIST_UNLOCK (playlist);

    hook_call ("playlist update", playlist);
    scanner_reset ();
}

void
playlist_set_shuffle(gboolean shuffle)
{
    Playlist * playlist = get_active_playlist ();
    if (!playlist)
        return;

    PLAYLIST_LOCK(playlist);

    cfg.shuffle = shuffle;
    playlist_generate_shuffle_list_nolock(playlist);

    PLAYLIST_UNLOCK(playlist);
}

Playlist *
playlist_new(void)
{
    Playlist *playlist = g_new0(Playlist, 1);
    playlist->mutex = g_mutex_new();
    playlist->title = g_strdup(_("Untitled Playlist"));
    playlist->filename = NULL;
    playlist_clear(playlist);
    playlist->attribute = PLAYLIST_PLAIN;
    playlist->serial = 0;

    return playlist;
}

void
playlist_free(Playlist *playlist)
{
    g_mutex_free (playlist->mutex);
    g_free (playlist->title);
    g_free (playlist->filename);
    g_free (playlist);
}

Playlist *
playlist_new_from_selected(void)
{
    Playlist *newpl = playlist_new();
    Playlist * playlist = get_active_playlist ();
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

    hook_call ("playlist update", 0);
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
    char changed;

    if (!playlist)
        return NULL;

    PLAYLIST_LOCK(playlist);

    if (!playlist->position) {
        if (cfg.shuffle)
            playlist->position = playlist->shuffle->data;
        else
            playlist->position = playlist->entries->data;

        changed = 1;
    }
    else
        changed = 0;

    PLAYLIST_UNLOCK(playlist);

    if (changed)
        hook_call ("playlist position", playlist);

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

const gchar *
get_gentitle_format(void)
{
    guint titlestring_preset = cfg.titlestring_preset;

    if (titlestring_preset < n_titlestring_presets)
        return aud_titlestring_presets[titlestring_preset];

    return cfg.gentitle_format;
}
