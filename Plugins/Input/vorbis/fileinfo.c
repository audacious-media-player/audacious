/*  BMP - Cross-platform multimedia player
 *  Copyright (C) 2003-2004  BMP development team.
 *
 *  Based on XMMS:
 *  Copyright (C) 1998-2003  XMMS development team.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301, USA.
 *
 */

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include <glib.h>
#include <glib/gi18n.h>
#include <gtk/gtk.h>
#include <ogg/ogg.h>
#include <vorbis/codec.h>
#include <vorbis/vorbisfile.h>

#include <math.h>
#include <stdlib.h>
#include <string.h>

#include <unistd.h>

#include "audacious/util.h"
#include <libaudacious/util.h>
#include <libaudacious/vfs.h>

#include "vorbis.h"
#include "vcedit.h"

#include "ogg.xpm"

static struct vte_struct {
    VFSFile *in;
    gchar *filename;
} vte;

static void fail(const gchar * error);
static void save_cb(GtkWidget * w, gpointer data);
static void remove_cb(GtkWidget * w, gpointer data);
static gint init_files(vcedit_state * state);
static gint close_files(vcedit_state * state);

extern GMutex *vf_mutex;
static GtkWidget *window = NULL;
static GList *genre_list = NULL;

static GtkWidget *title_entry, *album_entry, *performer_entry;
static GtkWidget *tracknumber_entry, *date_entry;
static GtkWidget *genre_combo, *user_comment_entry;
#ifdef ALL_VORBIS_TAGS
static GtkWidget *description_entry, *version_entry, *isrc_entry;
static GtkWidget *copyright_entry, *organization_entry, *location_entry;
#endif
static GtkWidget *rg_track_entry, *rg_album_entry, *rg_track_peak_entry,
    *rg_album_peak_entry;
static GtkWidget *rg_track_label, *rg_album_label, *rg_track_peak_label,
    *rg_album_peak_label;
static GtkWidget *rg_show_button;

GtkWidget *save_button, *remove_button;
GtkWidget *rg_frame, *rg_table;

/* From mpg123.c, as no standardized Ogg Vorbis genres exists. */
static const gchar *vorbis_genres[] = {
    N_("Blues"), N_("Classic Rock"), N_("Country"), N_("Dance"),
    N_("Disco"), N_("Funk"), N_("Grunge"), N_("Hip-Hop"),
    N_("Jazz"), N_("Metal"), N_("New Age"), N_("Oldies"),
    N_("Other"), N_("Pop"), N_("R&B"), N_("Rap"), N_("Reggae"),
    N_("Rock"), N_("Techno"), N_("Industrial"), N_("Alternative"),
    N_("Ska"), N_("Death Metal"), N_("Pranks"), N_("Soundtrack"),
    N_("Euro-Techno"), N_("Ambient"), N_("Trip-Hop"), N_("Vocal"),
    N_("Jazz+Funk"), N_("Fusion"), N_("Trance"), N_("Classical"),
    N_("Instrumental"), N_("Acid"), N_("House"), N_("Game"),
    N_("Sound Clip"), N_("Gospel"), N_("Noise"), N_("AlternRock"),
    N_("Bass"), N_("Soul"), N_("Punk"), N_("Space"),
    N_("Meditative"), N_("Instrumental Pop"),
    N_("Instrumental Rock"), N_("Ethnic"), N_("Gothic"),
    N_("Darkwave"), N_("Techno-Industrial"), N_("Electronic"),
    N_("Pop-Folk"), N_("Eurodance"), N_("Dream"),
    N_("Southern Rock"), N_("Comedy"), N_("Cult"),
    N_("Gangsta Rap"), N_("Top 40"), N_("Christian Rap"),
    N_("Pop/Funk"), N_("Jungle"), N_("Native American"),
    N_("Cabaret"), N_("New Wave"), N_("Psychedelic"), N_("Rave"),
    N_("Showtunes"), N_("Trailer"), N_("Lo-Fi"), N_("Tribal"),
    N_("Acid Punk"), N_("Acid Jazz"), N_("Polka"), N_("Retro"),
    N_("Musical"), N_("Rock & Roll"), N_("Hard Rock"), N_("Folk"),
    N_("Folk/Rock"), N_("National Folk"), N_("Swing"),
    N_("Fast-Fusion"), N_("Bebob"), N_("Latin"), N_("Revival"),
    N_("Celtic"), N_("Bluegrass"), N_("Avantgarde"),
    N_("Gothic Rock"), N_("Progressive Rock"),
    N_("Psychedelic Rock"), N_("Symphonic Rock"), N_("Slow Rock"),
    N_("Big Band"), N_("Chorus"), N_("Easy Listening"),
    N_("Acoustic"), N_("Humour"), N_("Speech"), N_("Chanson"),
    N_("Opera"), N_("Chamber Music"), N_("Sonata"), N_("Symphony"),
    N_("Booty Bass"), N_("Primus"), N_("Porn Groove"),
    N_("Satire"), N_("Slow Jam"), N_("Club"), N_("Tango"),
    N_("Samba"), N_("Folklore"), N_("Ballad"), N_("Power Ballad"),
    N_("Rhythmic Soul"), N_("Freestyle"), N_("Duet"),
    N_("Punk Rock"), N_("Drum Solo"), N_("A Cappella"),
    N_("Euro-House"), N_("Dance Hall"), N_("Goa"),
    N_("Drum & Bass"), N_("Club-House"), N_("Hardcore"),
    N_("Terror"), N_("Indie"), N_("BritPop"), N_("Negerpunk"),
    N_("Polsk Punk"), N_("Beat"), N_("Christian Gangsta Rap"),
    N_("Heavy Metal"), N_("Black Metal"), N_("Crossover"),
    N_("Contemporary Christian"), N_("Christian Rock"),
    N_("Merengue"), N_("Salsa"), N_("Thrash Metal"),
    N_("Anime"), N_("JPop"), N_("Synthpop")
};

static const gchar *
get_comment(vorbis_comment * vc, const gchar * tag)
{
    const gchar *value;

    g_return_val_if_fail(tag != NULL, "");

    if (!vc)
        return "";

    if ((value = vorbis_comment_query(vc, (gchar *) tag, 0)))
        return value;
    else
        return "";
}

static gboolean
str_equal_nocase(gconstpointer a,
                 gconstpointer b)
{
    return strcasecmp((const gchar *) a, (const gchar *) b) == 0;
}

static GHashTable *
hash_table_from_vorbis_comment(vorbis_comment * vc)
{
    GHashTable *table;
    gint i;

    table = g_hash_table_new_full(g_str_hash, str_equal_nocase,
                                  g_free, g_free);

    for (i = 0; i < vc->comments; i++) {
        gchar **frags;
#ifdef DEBUG
        g_message(vc->user_comments[i]);
#endif
        frags = g_strsplit(vc->user_comments[i], "=", 2);
      
        /* FIXME: need more rigorous checks to guard against
           borqued comments */

        /* No RHS? */
        if (!frags[1]) frags[1] = g_strdup("");

        g_hash_table_replace(table, frags[0], frags[1]);
        g_free(frags);
    }

    return table;
}

static void
comment_hash_add_tag(GHashTable * table,
                     const gchar * tag,
                     const gchar * value)
{
    g_hash_table_replace(table, g_strdup(tag), g_strdup(value));
}


static void
vorbis_comment_add_swapped(gchar * key,
                           gchar * value,
                           vorbis_comment * vc)
{
    vorbis_comment_add_tag(vc, key, value);
}

static void
hash_table_to_vorbis_comment(vorbis_comment * vc, GHashTable * table)
{
    vorbis_comment_clear(vc);
    g_hash_table_foreach(table, (GHFunc) vorbis_comment_add_swapped,
                         vc);
}


static void
fail(const gchar * error)
{
    gchar *errorstring;
    errorstring = g_strdup_printf(_("An error occured:\n%s"), error);

    xmms_show_message(_("Error!"), errorstring, _("Ok"), FALSE, NULL, NULL);

    g_free(errorstring);
    return;
}


static void
save_cb(GtkWidget * w, gpointer data)
{
    const gchar *track_name, *performer, *album_name, *date, *track_number;
    const gchar *genre, *user_comment;
#ifdef ALL_VORBIS_TAGS
    const gchar *description, *version, *isrc, *copyright, *organization;
    const gchar *location;
#endif
    const gchar *rg_track_gain, *rg_album_gain, *rg_track_peak, *rg_album_peak;

    GHashTable *comment_hash;

    vcedit_state *state;
    vorbis_comment *comment;

    if (!g_strncasecmp(vte.filename, "http://", 7))
        return;

    state = vcedit_new_state();

    g_mutex_lock(vf_mutex);
    if (init_files(state) < 0) {
        fail(_("Failed to modify tag (open)"));
        goto close;
    }

    comment = vcedit_comments(state);
    comment_hash = hash_table_from_vorbis_comment(comment);

    track_name = gtk_entry_get_text(GTK_ENTRY(title_entry));
    performer = gtk_entry_get_text(GTK_ENTRY(performer_entry));
    album_name = gtk_entry_get_text(GTK_ENTRY(album_entry));
    track_number = gtk_entry_get_text(GTK_ENTRY(tracknumber_entry));
    genre = gtk_entry_get_text(GTK_ENTRY(GTK_COMBO(genre_combo)->entry));
    date = gtk_entry_get_text(GTK_ENTRY(date_entry));
    user_comment = gtk_entry_get_text(GTK_ENTRY(user_comment_entry));
#ifdef ALL_VORBIS_TAGS
    location = gtk_entry_get_text(GTK_ENTRY(location_entry));
    description = gtk_entry_get_text(GTK_ENTRY(description_entry));
    version = gtk_entry_get_text(GTK_ENTRY(version_entry));
    isrc = gtk_entry_get_text(GTK_ENTRY(isrc_entry));
    organization = gtk_entry_get_text(GTK_ENTRY(organization_entry));
    copyright = gtk_entry_get_text(GTK_ENTRY(copyright_entry));
#endif
    rg_track_gain = gtk_entry_get_text(GTK_ENTRY(rg_track_entry));
    rg_album_gain = gtk_entry_get_text(GTK_ENTRY(rg_album_entry));
    rg_track_peak = gtk_entry_get_text(GTK_ENTRY(rg_track_peak_entry));
    rg_album_peak = gtk_entry_get_text(GTK_ENTRY(rg_album_peak_entry));

    comment_hash_add_tag(comment_hash, "title", track_name);
    comment_hash_add_tag(comment_hash, "artist", performer);
    comment_hash_add_tag(comment_hash, "album", album_name);
    comment_hash_add_tag(comment_hash, "tracknumber", track_number);
    comment_hash_add_tag(comment_hash, "genre", genre);
    comment_hash_add_tag(comment_hash, "date", date);
    comment_hash_add_tag(comment_hash, "comment", user_comment);

#ifdef ALL_VORBIS_TAGS
    comment_hash_add_tag(comment_hash, "location", location);
    comment_hash_add_tag(comment_hash, "description", description);
    comment_hash_add_tag(comment_hash, "version", version);
    comment_hash_add_tag(comment_hash, "isrc", isrc);
    comment_hash_add_tag(comment_hash, "organization", organization);
    comment_hash_add_tag(comment_hash, "copyright", copyright);
#endif

    comment_hash_add_tag(comment_hash, "replaygain_track_gain", rg_track_gain);
    comment_hash_add_tag(comment_hash, "replaygain_album_gain", rg_album_gain);
    comment_hash_add_tag(comment_hash, "replaygain_track_peak", rg_track_peak);
    comment_hash_add_tag(comment_hash, "replaygain_album_peak", rg_album_peak);

    hash_table_to_vorbis_comment(comment, comment_hash);
    g_hash_table_destroy(comment_hash);

    if (close_files(state) < 0)
        fail(_("Failed to modify tag (close)"));
    else {
        gtk_widget_set_sensitive(save_button, FALSE);
        gtk_widget_set_sensitive(remove_button, TRUE);
    }


  close:
    vcedit_clear(state);
    g_mutex_unlock(vf_mutex);
}

static void
remove_cb(GtkWidget * w, gpointer data)
{
    vcedit_state *state;
    vorbis_comment *comment;

    if (!g_strncasecmp(vte.filename, "http://", 7))
        return;

    state = vcedit_new_state();

    g_mutex_lock(vf_mutex);
    if (init_files(state) < 0) {
        fail(_("Failed to modify tag"));
        goto close;
    }

    comment = vcedit_comments(state);

    vorbis_comment_clear(comment);

    if (close_files(state) < 0) {
        fail(_("Failed to modify tag"));
    }
    else {
        gtk_entry_set_text(GTK_ENTRY(title_entry), "");
        gtk_entry_set_text(GTK_ENTRY(album_entry), "");
        gtk_entry_set_text(GTK_ENTRY(performer_entry), "");
        gtk_entry_set_text(GTK_ENTRY(tracknumber_entry), "");
        gtk_entry_set_text(GTK_ENTRY(date_entry), "");
        gtk_entry_set_text(GTK_ENTRY(genre_combo), "");
        gtk_entry_set_text(GTK_ENTRY(user_comment_entry), "");
    }

  close:
    vcedit_clear(state);
    g_mutex_unlock(vf_mutex);
/*     gtk_widget_destroy(window); */
}

static void
rg_show_cb(GtkWidget * w, gpointer data)
{
    if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(rg_show_button))) {
        gtk_widget_show(rg_frame);
    }
    else {
        gtk_widget_hide(rg_frame);
    }
}

static gint
init_files(vcedit_state * state)
{
    if ((vte.in = vfs_fopen(vte.filename, "rb")) == NULL) {
#ifdef DEBUG
        g_message("fileinfo.c: couldn't open file %s", vte.filename);
#endif
        return -1;
    }

    if (vcedit_open(state, vte.in) < 0) {
#ifdef DEBUG
        g_message("fileinfo.c: couldn't open file for editing %s",
                  vte.filename);
#endif
        vfs_fclose(vte.in);
        return -1;
    }

#ifdef DEBUG
    g_message("fileinfo.c: file successfully opened for editing %s",
              vte.filename);
#endif

    return 0;
}

static gint
close_files(vcedit_state * state)
{
    gint retval = 0, ofh;
    gchar *tmpfn;
    VFSFile *out;

    tmpfn = g_strdup_printf("%s.XXXXXX", vte.filename);

    if ((ofh = mkstemp(tmpfn)) < 0) {
        g_free(tmpfn);
        vfs_fclose(vte.in);
#ifdef DEBUG
        g_critical("fileinfo.c: couldn't create temp file");
#endif
        return -1;
    }
    else {
#ifdef DEBUG
        g_message("fileinfo.c: created temp file %s", tmpfn);
#endif
    }

    if ((out = vfs_fopen(tmpfn, "wb")) == NULL) {
        close(ofh);
        remove(tmpfn);
        g_free(tmpfn);
        vfs_fclose(vte.in);
#ifdef DEBUG
        g_critical("fileinfo.c: couldn't open temp file");
#endif
        return -1;
    }
    else {
#ifdef DEBUG
        g_message("fileinfo.c: opened temp file %s", tmpfn);
#endif
    }

    if (vcedit_write(state, out) < 0) {
#ifdef DEBUG
        g_warning("vcedit_write: %s", state->lasterror);
#endif
        retval = -1;
    }

    vfs_fclose(vte.in);

    if (vfs_fclose(out) != 0) {
#ifdef DEBUG
        g_critical("fileinfo.c: couldn't close out file");
#endif
        retval = -1;
    }
    else {
#ifdef DEBUG
        g_message("fileinfo.c: outfile closed");
#endif
    }

    if (retval < 0 || rename(tmpfn, vte.filename) < 0) {
        remove(tmpfn);
        retval = -1;
#ifdef DEBUG
        g_critical("fileinfo.c: couldn't rename file");
#endif
    }
    else {
#ifdef DEBUG
        g_message("fileinfo.c: file %s renamed successfully to %s", tmpfn,
                  vte.filename);
#endif
    }

    g_free(tmpfn);
    return retval;
}


static void
label_set_text(GtkLabel * label, const gchar * format, ...)
{
    va_list args;
    gchar *text;

    va_start(args, format);
    text = g_strdup_vprintf(format, args);
    va_end(args);

    gtk_label_set_text(label, text);
    g_free(text);
}

void
change_buttons(void)
{
    gtk_widget_set_sensitive(GTK_WIDGET(save_button), TRUE);
}


/***********************************************************************/

void
vorbis_file_info_box(gchar * filename)
{
    gchar *filename_utf8, *title;
    const gchar *rg_track_gain, *rg_track_peak;
    const gchar *rg_album_gain, *rg_album_peak;

    gint time, minutes, seconds, bitrate, rate, channels, filesize;
    gsize i;

    OggVorbis_File vf;
    vorbis_info *vi;
    vorbis_comment *comment = NULL;

    VFSFile *fh;

    gboolean clear_vf = FALSE;

    GtkWidget *pixmapwid;
    GdkPixbuf *pixbuf;
    PangoAttrList *attrs;
    PangoAttribute *attr;

    GtkWidget *boxx;
    GtkWidget *img;
    GtkWidget *test_table;

    static GtkWidget *info_frame, *info_box, *bitrate_label, *rate_label;
    static GtkWidget *bitrate_label_val, *rate_label_val;

    static GtkWidget *channel_label, *length_label, *filesize_label;
    static GtkWidget *channel_label_val, *length_label_val,
        *filesize_label_val;

    static GtkWidget *filename_entry, *tag_frame;

    g_free(vte.filename);
    vte.filename = g_strdup(filename);

    if (!window) {
        GtkWidget *hbox, *label, *filename_hbox, *vbox, *left_vbox;
        GtkWidget *table, *bbox, *cancel_button;

        window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
        gtk_window_set_type_hint(GTK_WINDOW(window),
                                 GDK_WINDOW_TYPE_HINT_DIALOG);
        gtk_window_set_resizable(GTK_WINDOW(window), FALSE);
        g_signal_connect(G_OBJECT(window), "destroy",
                         G_CALLBACK(gtk_widget_destroyed), &window);
        gtk_container_set_border_width(GTK_CONTAINER(window), 10);

        vbox = gtk_vbox_new(FALSE, 10);
        gtk_container_add(GTK_CONTAINER(window), vbox);

        filename_hbox = gtk_hbox_new(FALSE, 5);
        gtk_box_pack_start(GTK_BOX(vbox), filename_hbox, FALSE, TRUE, 0);

        pixbuf = gdk_pixbuf_new_from_xpm_data((const gchar **)
                                              gnome_mime_audio_ogg_xpm);
        pixmapwid = gtk_image_new_from_pixbuf(pixbuf);
        gtk_misc_set_alignment(GTK_MISC(pixmapwid), 0, 0);
        gtk_box_pack_start(GTK_BOX(filename_hbox), pixmapwid, FALSE, FALSE,
                           0);

        attrs = pango_attr_list_new();

        attr = pango_attr_weight_new(PANGO_WEIGHT_BOLD);
        attr->start_index = 0;
        attr->end_index = -1;
        pango_attr_list_insert(attrs, attr);

        label = gtk_label_new(_("Name:"));
        gtk_label_set_attributes(GTK_LABEL(label), attrs);
        gtk_box_pack_start(GTK_BOX(filename_hbox), label, FALSE, FALSE, 0);

        filename_entry = gtk_entry_new();
        gtk_editable_set_editable(GTK_EDITABLE(filename_entry), FALSE);
        gtk_box_pack_start(GTK_BOX(filename_hbox), filename_entry, TRUE,
                           TRUE, 0);

        hbox = gtk_hbox_new(FALSE, 10);
        gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, TRUE, 0);

        left_vbox = gtk_table_new(4, 7, FALSE);
        gtk_box_pack_start(GTK_BOX(hbox), left_vbox, FALSE, FALSE, 0);

        tag_frame = gtk_frame_new(_(" Ogg Vorbis Tag "));
        gtk_table_attach(GTK_TABLE(left_vbox), tag_frame, 2, 4, 0, 1,
                         GTK_FILL, GTK_FILL, 0, 4);

        table = gtk_table_new(16, 6, FALSE);
        gtk_container_set_border_width(GTK_CONTAINER(table), 5);
        gtk_container_add(GTK_CONTAINER(tag_frame), table);

        label = gtk_label_new(_("Title:"));
        gtk_label_set_attributes(GTK_LABEL(label), attrs);
        gtk_misc_set_alignment(GTK_MISC(label), 1, 0.5);
        gtk_table_attach(GTK_TABLE(table), label, 0, 1, 0, 1,
                         GTK_FILL, GTK_FILL, 5, 5);

        title_entry = gtk_entry_new();
        gtk_table_attach(GTK_TABLE(table), title_entry, 1, 4, 0, 1,
                         GTK_FILL | GTK_EXPAND | GTK_SHRINK,
                         GTK_FILL | GTK_EXPAND | GTK_SHRINK, 0, 5);

        label = gtk_label_new(_("Artist:"));
        gtk_label_set_attributes(GTK_LABEL(label), attrs);
        gtk_misc_set_alignment(GTK_MISC(label), 1, 0.5);
        gtk_table_attach(GTK_TABLE(table), label, 0, 1, 1, 2,
                         GTK_FILL, GTK_FILL, 5, 5);

        performer_entry = gtk_entry_new();
        gtk_table_attach(GTK_TABLE(table), performer_entry, 1, 4, 1, 2,
                         GTK_FILL | GTK_EXPAND | GTK_SHRINK,
                         GTK_FILL | GTK_EXPAND | GTK_SHRINK, 0, 5);

        label = gtk_label_new(_("Album:"));
        gtk_label_set_attributes(GTK_LABEL(label), attrs);
        gtk_misc_set_alignment(GTK_MISC(label), 1, 0.5);
        gtk_table_attach(GTK_TABLE(table), label, 0, 1, 2, 3,
                         GTK_FILL, GTK_FILL, 5, 5);

        album_entry = gtk_entry_new();
        gtk_table_attach(GTK_TABLE(table), album_entry, 1, 4, 2, 3,
                         GTK_FILL | GTK_EXPAND | GTK_SHRINK,
                         GTK_FILL | GTK_EXPAND | GTK_SHRINK, 0, 5);

        label = gtk_label_new(_("Comment:"));
        gtk_label_set_attributes(GTK_LABEL(label), attrs);
        gtk_misc_set_alignment(GTK_MISC(label), 1, 0.5);
        gtk_table_attach(GTK_TABLE(table), label, 0, 1, 3, 4,
                         GTK_FILL, GTK_FILL, 5, 5);

        user_comment_entry = gtk_entry_new();
        gtk_table_attach(GTK_TABLE(table), user_comment_entry, 1, 4, 3,
                         4, GTK_FILL | GTK_EXPAND | GTK_SHRINK,
                         GTK_FILL | GTK_EXPAND | GTK_SHRINK, 0, 5);

        label = gtk_label_new(_("Date:"));
        gtk_label_set_attributes(GTK_LABEL(label), attrs);
        gtk_misc_set_alignment(GTK_MISC(label), 1, 0.5);
        gtk_table_attach(GTK_TABLE(table), label, 0, 1, 4, 5,
                         GTK_FILL, GTK_FILL, 5, 5);

        date_entry = gtk_entry_new();
        gtk_widget_set_size_request(date_entry, 60, -1);
        gtk_table_attach(GTK_TABLE(table), date_entry, 1, 2, 4, 5,
                         GTK_FILL | GTK_EXPAND | GTK_SHRINK,
                         GTK_FILL | GTK_EXPAND | GTK_SHRINK, 0, 5);

        label = gtk_label_new(_("Track number:"));
        gtk_label_set_attributes(GTK_LABEL(label), attrs);
        gtk_misc_set_alignment(GTK_MISC(label), 1, 0.5);
        gtk_table_attach(GTK_TABLE(table), label, 2, 3, 4, 5,
                         GTK_FILL, GTK_FILL, 5, 5);

        tracknumber_entry = gtk_entry_new_with_max_length(4);
        gtk_widget_set_size_request(tracknumber_entry, 20, -1);
        gtk_table_attach(GTK_TABLE(table), tracknumber_entry, 3, 4, 4,
                         5, GTK_FILL | GTK_EXPAND | GTK_SHRINK,
                         GTK_FILL | GTK_EXPAND | GTK_SHRINK, 0, 5);

        label = gtk_label_new(_("Genre:"));
        gtk_label_set_attributes(GTK_LABEL(label), attrs);
        gtk_misc_set_alignment(GTK_MISC(label), 1, 0.5);
        gtk_table_attach(GTK_TABLE(table), label, 0, 1, 5, 6,
                         GTK_FILL, GTK_FILL, 5, 5);

        genre_combo = gtk_combo_new();
        if (!genre_list) {
            for (i = 0; i < G_N_ELEMENTS(vorbis_genres); i++)
                genre_list = g_list_prepend(genre_list, _(vorbis_genres[i]));
            genre_list = g_list_sort(genre_list, (GCompareFunc) g_strcasecmp);
        }
        gtk_combo_set_popdown_strings(GTK_COMBO(genre_combo), genre_list);
        gtk_table_attach(GTK_TABLE(table), genre_combo, 1, 4, 5, 6,
                         GTK_FILL | GTK_EXPAND | GTK_SHRINK,
                         GTK_FILL | GTK_EXPAND | GTK_SHRINK, 0, 5);

#ifdef ALL_VORBIS_TAGS
        label = gtk_label_new(_("Description:"));
        gtk_misc_set_alignment(GTK_MISC(label), 1, 0.5);
        gtk_table_attach(GTK_TABLE(table), label, 0, 1, 6, 7,
                         GTK_FILL, GTK_FILL, 5, 5);

        description_entry = gtk_entry_new();
        gtk_table_attach(GTK_TABLE(table), description_entry, 1, 4, 6,
                         7, GTK_FILL | GTK_EXPAND | GTK_SHRINK,
                         GTK_FILL | GTK_EXPAND | GTK_SHRINK, 0, 5);

        label = gtk_label_new(_("Location:"));
        gtk_misc_set_alignment(GTK_MISC(label), 1, 0.5);
        gtk_table_attach(GTK_TABLE(table), label, 0, 1, 7, 8,
                         GTK_FILL, GTK_FILL, 5, 5);

        location_entry = gtk_entry_new();
        gtk_table_attach(GTK_TABLE(table), location_entry, 1, 4, 7, 8,
                         GTK_FILL | GTK_EXPAND | GTK_SHRINK,
                         GTK_FILL | GTK_EXPAND | GTK_SHRINK, 0, 5);

        label = gtk_label_new(_("Version:"));
        gtk_misc_set_alignment(GTK_MISC(label), 1, 0.5);
        gtk_table_attach(GTK_TABLE(table), label, 0, 1, 8, 9,
                         GTK_FILL, GTK_FILL, 5, 5);

        version_entry = gtk_entry_new();
        gtk_widget_set_size_request(version_entry, 60, -1);
        gtk_table_attach(GTK_TABLE(table), version_entry, 1, 2, 8, 9,
                         GTK_FILL | GTK_EXPAND | GTK_SHRINK,
                         GTK_FILL | GTK_EXPAND | GTK_SHRINK, 0, 5);

        label = gtk_label_new(_("ISRC number:"));
        gtk_misc_set_alignment(GTK_MISC(label), 1, 0.5);
        gtk_table_attach(GTK_TABLE(table), label, 2, 3, 8, 9,
                         GTK_FILL, GTK_FILL, 5, 5);

        isrc_entry = gtk_entry_new();
        gtk_widget_set_size_request(isrc_entry, 20, -1);
        gtk_table_attach(GTK_TABLE(table), isrc_entry, 3, 4, 8, 9,
                         GTK_FILL | GTK_EXPAND | GTK_SHRINK,
                         GTK_FILL | GTK_EXPAND | GTK_SHRINK, 0, 5);

        label = gtk_label_new(_("Organization:"));
        gtk_misc_set_alignment(GTK_MISC(label), 1, 0.5);
        gtk_table_attach(GTK_TABLE(table), label, 0, 1, 9, 10,
                         GTK_FILL, GTK_FILL, 5, 5);

        organization_entry = gtk_entry_new();
        gtk_table_attach(GTK_TABLE(table), organization_entry, 1, 4, 9,
                         10, GTK_FILL | GTK_EXPAND | GTK_SHRINK,
                         GTK_FILL | GTK_EXPAND | GTK_SHRINK, 0, 5);

        label = gtk_label_new(_("Copyright:"));
        gtk_misc_set_alignment(GTK_MISC(label), 1, 0.5);
        gtk_table_attach(GTK_TABLE(table), label, 0, 1, 10, 11,
                         GTK_FILL, GTK_FILL, 5, 5);

        copyright_entry = gtk_entry_new();
        gtk_table_attach(GTK_TABLE(table), copyright_entry, 1, 4, 10,
                         11, GTK_FILL | GTK_EXPAND | GTK_SHRINK,
                         GTK_FILL | GTK_EXPAND | GTK_SHRINK, 0, 5);
#endif
        boxx = gtk_hbutton_box_new();
        gtk_hbutton_box_set_layout_default(GTK_BUTTONBOX_SPREAD);

        remove_button = gtk_button_new_from_stock(GTK_STOCK_DELETE);
        g_signal_connect_swapped(G_OBJECT(remove_button),
                                 "clicked", G_CALLBACK(remove_cb), NULL);
        gtk_container_add(GTK_CONTAINER(boxx), remove_button);

        save_button = gtk_button_new_from_stock(GTK_STOCK_SAVE);
        g_signal_connect(G_OBJECT(save_button), "clicked",
                         G_CALLBACK(save_cb), NULL);
        gtk_container_add(GTK_CONTAINER(boxx), save_button);

        gtk_table_attach(GTK_TABLE(table), boxx, 0, 5, 6, 7, GTK_FILL, 0,
                         0, 8);

        rg_show_button = gtk_toggle_button_new();
        img = gtk_image_new_from_stock(GTK_STOCK_GO_FORWARD,
                                       GTK_ICON_SIZE_MENU);
        gtk_container_add(GTK_CONTAINER(rg_show_button), img);
        g_signal_connect(G_OBJECT(rg_show_button), "toggled",
                         G_CALLBACK(rg_show_cb), NULL);

        gtk_table_attach(GTK_TABLE(left_vbox), rg_show_button, 4, 5, 0, 2,
                         GTK_FILL | GTK_EXPAND | GTK_SHRINK,
                         GTK_FILL | GTK_EXPAND | GTK_SHRINK, 5, 5);

        rg_frame = gtk_frame_new(_(" Ogg Vorbis ReplayGain "));
        gtk_table_attach(GTK_TABLE(left_vbox), rg_frame, 5, 6, 0, 4,
                         GTK_FILL, GTK_FILL, 0, 4);
        rg_table = gtk_table_new(16, 4, FALSE);
        gtk_container_add(GTK_CONTAINER(rg_frame), GTK_WIDGET(rg_table));

        rg_track_label = gtk_label_new(_("Track gain:"));
        gtk_misc_set_alignment(GTK_MISC(rg_track_label), 1, 0.5);
        gtk_table_attach(GTK_TABLE(rg_table), rg_track_label, 5, 6, 0, 1,
                         GTK_FILL, GTK_FILL, 5, 5);

        rg_track_entry = gtk_entry_new();
        gtk_table_attach(GTK_TABLE(rg_table), rg_track_entry, 6, 7, 0,
                         1, GTK_FILL | GTK_EXPAND | GTK_SHRINK,
                         GTK_FILL | GTK_EXPAND | GTK_SHRINK, 0, 5);

        rg_track_peak_label = gtk_label_new(_("Track peak:"));
        gtk_misc_set_alignment(GTK_MISC(rg_track_peak_label), 1, 0.5);
        gtk_table_attach(GTK_TABLE(rg_table), rg_track_peak_label, 5, 6, 1,
                         2, GTK_FILL, GTK_FILL, 5, 5);

        rg_track_peak_entry = gtk_entry_new();
        gtk_table_attach(GTK_TABLE(rg_table), rg_track_peak_entry, 6, 7, 1,
                         2, GTK_FILL | GTK_EXPAND | GTK_SHRINK,
                         GTK_FILL | GTK_EXPAND | GTK_SHRINK, 0, 5);


        rg_album_label = gtk_label_new(_("Album gain:"));
        gtk_misc_set_alignment(GTK_MISC(rg_album_label), 1, 0.5);
        gtk_table_attach(GTK_TABLE(rg_table), rg_album_label, 5, 6, 2, 3,
                         GTK_FILL, GTK_FILL, 5, 5);

        rg_album_entry = gtk_entry_new();
        gtk_table_attach(GTK_TABLE(rg_table), rg_album_entry, 6, 7, 2,
                         3, GTK_FILL | GTK_EXPAND | GTK_SHRINK,
                         GTK_FILL | GTK_EXPAND | GTK_SHRINK, 0, 5);

        rg_album_peak_label = gtk_label_new(_("Album peak:"));
        gtk_misc_set_alignment(GTK_MISC(rg_album_peak_label), 1, 0.5);
        gtk_table_attach(GTK_TABLE(rg_table), rg_album_peak_label, 5, 6, 3,
                         4, GTK_FILL, GTK_FILL, 5, 5);

        rg_album_peak_entry = gtk_entry_new();
        gtk_table_attach(GTK_TABLE(rg_table), rg_album_peak_entry, 6, 7, 3,
                         4, GTK_FILL | GTK_EXPAND | GTK_SHRINK,
                         GTK_FILL | GTK_EXPAND | GTK_SHRINK, 0, 5);

        bbox = gtk_hbutton_box_new();
        gtk_button_box_set_layout(GTK_BUTTON_BOX(bbox), GTK_BUTTONBOX_END);
        gtk_button_box_set_spacing(GTK_BUTTON_BOX(bbox), 5);
        gtk_table_attach(GTK_TABLE(left_vbox), bbox, 0, 4, 1, 2, GTK_FILL,
                         0, 0, 8);

        cancel_button = gtk_button_new_from_stock(GTK_STOCK_CLOSE);
        g_signal_connect_swapped(G_OBJECT(cancel_button),
                                 "clicked",
                                 G_CALLBACK(gtk_widget_destroy),
                                 G_OBJECT(window));
        GTK_WIDGET_SET_FLAGS(cancel_button, GTK_CAN_DEFAULT);
        gtk_box_pack_start(GTK_BOX(bbox), cancel_button, TRUE, TRUE, 0);
        gtk_widget_grab_default(cancel_button);


        gtk_table_set_col_spacing(GTK_TABLE(left_vbox), 1, 10);


        info_frame = gtk_frame_new(_(" Ogg Vorbis Info "));
        gtk_table_attach(GTK_TABLE(left_vbox), info_frame, 0, 2, 0, 1,
                         GTK_FILL, GTK_FILL, 0, 4);

        info_box = gtk_vbox_new(FALSE, 5);
        gtk_container_add(GTK_CONTAINER(info_frame), info_box);
        gtk_container_set_border_width(GTK_CONTAINER(info_box), 10);
        gtk_box_set_spacing(GTK_BOX(info_box), 0);

        /* FIXME: Obvious... */
        test_table = gtk_table_new(2, 10, FALSE);
        gtk_container_set_border_width(GTK_CONTAINER(test_table), 0);
        gtk_container_add(GTK_CONTAINER(info_box), test_table);


        bitrate_label = gtk_label_new(_("Bit rate:"));
        gtk_label_set_attributes(GTK_LABEL(bitrate_label), attrs);
        gtk_misc_set_alignment(GTK_MISC(bitrate_label), 1, 0.5);
        gtk_label_set_justify(GTK_LABEL(bitrate_label), GTK_JUSTIFY_RIGHT);
        gtk_table_attach(GTK_TABLE(test_table), bitrate_label, 0, 1, 0, 1,
                         GTK_FILL, GTK_FILL, 5, 2);

        bitrate_label_val = gtk_label_new("");
        gtk_misc_set_alignment(GTK_MISC(bitrate_label_val), 0, 0);
        gtk_label_set_justify(GTK_LABEL(bitrate_label_val), GTK_JUSTIFY_LEFT);
        gtk_table_attach(GTK_TABLE(test_table), bitrate_label_val, 1, 2, 0,
                         1, GTK_FILL, GTK_FILL, 10, 2);

        rate_label = gtk_label_new(_("Sample rate:"));
        gtk_label_set_attributes(GTK_LABEL(rate_label), attrs);
        gtk_misc_set_alignment(GTK_MISC(rate_label), 1, 0.5);
        gtk_label_set_justify(GTK_LABEL(rate_label), GTK_JUSTIFY_RIGHT);
        gtk_table_attach(GTK_TABLE(test_table), rate_label, 0, 1, 1, 2,
                         GTK_FILL, GTK_FILL, 5, 2);

        rate_label_val = gtk_label_new("");
        gtk_misc_set_alignment(GTK_MISC(rate_label_val), 0, 0);
        gtk_label_set_justify(GTK_LABEL(rate_label_val), GTK_JUSTIFY_LEFT);
        gtk_table_attach(GTK_TABLE(test_table), rate_label_val, 1, 2, 1, 2,
                         GTK_FILL, GTK_FILL, 10, 2);

        channel_label = gtk_label_new(_("Channels:"));
        gtk_label_set_attributes(GTK_LABEL(channel_label), attrs);
        gtk_misc_set_alignment(GTK_MISC(channel_label), 1, 0.5);
        gtk_label_set_justify(GTK_LABEL(channel_label), GTK_JUSTIFY_RIGHT);
        gtk_table_attach(GTK_TABLE(test_table), channel_label, 0, 1, 2, 3,
                         GTK_FILL, GTK_FILL, 5, 2);

        channel_label_val = gtk_label_new("");
        gtk_misc_set_alignment(GTK_MISC(channel_label_val), 0, 0);
        gtk_label_set_justify(GTK_LABEL(channel_label_val), GTK_JUSTIFY_LEFT);
        gtk_table_attach(GTK_TABLE(test_table), channel_label_val, 1, 2, 2,
                         3, GTK_FILL, GTK_FILL, 10, 2);

        length_label = gtk_label_new(_("Length:"));
        gtk_label_set_attributes(GTK_LABEL(length_label), attrs);
        gtk_misc_set_alignment(GTK_MISC(length_label), 1, 0.5);
        gtk_label_set_justify(GTK_LABEL(length_label), GTK_JUSTIFY_RIGHT);
        gtk_table_attach(GTK_TABLE(test_table), length_label, 0, 1, 3, 4,
                         GTK_FILL, GTK_FILL, 5, 2);

        length_label_val = gtk_label_new("");
        gtk_misc_set_alignment(GTK_MISC(length_label_val), 0, 0);
        gtk_label_set_justify(GTK_LABEL(length_label_val), GTK_JUSTIFY_LEFT);
        gtk_table_attach(GTK_TABLE(test_table), length_label_val, 1, 2, 3,
                         4, GTK_FILL, GTK_FILL, 10, 2);

        filesize_label = gtk_label_new(_("File size:"));
        gtk_label_set_attributes(GTK_LABEL(filesize_label), attrs);
        gtk_misc_set_alignment(GTK_MISC(filesize_label), 1, 0.5);
        gtk_label_set_justify(GTK_LABEL(filesize_label), GTK_JUSTIFY_RIGHT);
        gtk_table_attach(GTK_TABLE(test_table), filesize_label, 0, 1, 4, 5,
                         GTK_FILL, GTK_FILL, 5, 2);

        filesize_label_val = gtk_label_new("");
        gtk_misc_set_alignment(GTK_MISC(filesize_label_val), 0, 0);
        gtk_label_set_justify(GTK_LABEL(filesize_label_val),
                              GTK_JUSTIFY_LEFT);
        gtk_table_attach(GTK_TABLE(test_table), filesize_label_val, 1, 2,
                         4, 5, GTK_FILL, GTK_FILL, 10, 2);

        pango_attr_list_unref(attrs);
    }
    else
        gtk_window_present(GTK_WINDOW(window));

    if (!g_strncasecmp(vte.filename, "http://", 7))
        gtk_widget_set_sensitive(tag_frame, FALSE);
    else
        gtk_widget_set_sensitive(tag_frame, TRUE);

    gtk_label_set_text(GTK_LABEL(bitrate_label), _("Bit rate:"));
    gtk_label_set_text(GTK_LABEL(bitrate_label_val), _("N/A"));

    gtk_label_set_text(GTK_LABEL(rate_label), _("Sample rate:"));
    gtk_label_set_text(GTK_LABEL(rate_label_val), _("N/A"));

    gtk_label_set_text(GTK_LABEL(channel_label), _("Channels:"));
    gtk_label_set_text(GTK_LABEL(channel_label_val), _("N/A"));

    gtk_label_set_text(GTK_LABEL(length_label), _("Length:"));
    gtk_label_set_text(GTK_LABEL(length_label_val), _("N/A"));

    gtk_label_set_text(GTK_LABEL(filesize_label), _("File size:"));
    gtk_label_set_text(GTK_LABEL(filesize_label_val), _("N/A"));

    if ((fh = vfs_fopen(vte.filename, "r")) != NULL) {
        g_mutex_lock(vf_mutex);

        if (ov_open_callbacks(fh, &vf, NULL, 0, vorbis_callbacks) == 0) {
            comment = ov_comment(&vf, -1);
            if ((vi = ov_info(&vf, 0)) != NULL) {
                bitrate = vi->bitrate_nominal / 1000;
                rate = vi->rate;
                channels = vi->channels;
                clear_vf = TRUE;
                gtk_widget_set_sensitive(GTK_WIDGET(save_button), FALSE);
                gtk_widget_set_sensitive(GTK_WIDGET(remove_button), TRUE);
            }
            else {
                bitrate = 0;
                rate = 0;
                channels = 0;
                gtk_widget_set_sensitive(GTK_WIDGET(save_button), FALSE);
                gtk_widget_set_sensitive(GTK_WIDGET(remove_button), FALSE);
            }

            time = ov_time_total(&vf, -1);
            minutes = time / 60;
            seconds = time % 60;
            vfs_fseek(fh, 0, SEEK_END);
            filesize = vfs_ftell(fh);

            label_set_text(GTK_LABEL(bitrate_label_val),
                           _("%d KBit/s (nominal)"), bitrate);
            label_set_text(GTK_LABEL(rate_label_val), _("%d Hz"), rate);
            label_set_text(GTK_LABEL(channel_label_val), _("%d"), channels);
            label_set_text(GTK_LABEL(length_label_val),
                           _("%d:%.2d"), minutes, seconds);
            label_set_text(GTK_LABEL(filesize_label_val),
                           _("%d Bytes"), filesize);

        }
        else
            vfs_fclose(fh);
    }

    rg_track_gain = get_comment(comment, "replaygain_track_gain");
    if (*rg_track_gain == '\0')
        rg_track_gain = get_comment(comment, "rg_radio");   /* Old */

    rg_album_gain = get_comment(comment, "replaygain_album_gain");
    if (*rg_album_gain == '\0')
        rg_album_gain = get_comment(comment, "rg_audiophile");  /* Old */

    rg_track_peak = get_comment(comment, "replaygain_track_peak");
    if (*rg_track_peak == '\0')
        rg_track_peak = get_comment(comment, "rg_peak");    /* Old */

    rg_album_peak = get_comment(comment, "replaygain_album_peak");  /* Old had no album peak */

    /* Fill it all in .. */
    gtk_entry_set_text(GTK_ENTRY(title_entry),
                       get_comment(comment, "title"));
    gtk_entry_set_text(GTK_ENTRY(performer_entry),
                       get_comment(comment, "artist"));
    gtk_entry_set_text(GTK_ENTRY(album_entry),
                       get_comment(comment, "album"));
    gtk_entry_set_text(GTK_ENTRY(user_comment_entry),
                       get_comment(comment, "comment"));
    gtk_entry_set_text(GTK_ENTRY(GTK_COMBO(genre_combo)->entry), 
                       get_comment(comment, "genre"));
    gtk_entry_set_text(GTK_ENTRY(tracknumber_entry),
                       get_comment(comment, "tracknumber"));
    gtk_entry_set_text(GTK_ENTRY(date_entry),
                       get_comment(comment, "date"));
#ifdef ALL_VORBIS_TAGS
    gtk_entry_set_text(GTK_ENTRY(version_entry),
                       get_comment(comment, "version"));
    gtk_entry_set_text(GTK_ENTRY(description_entry),
                       get_comment(comment, "description"));
    gtk_entry_set_text(GTK_ENTRY(organization_entry),
                       get_comment(comment, "organization"));
    gtk_entry_set_text(GTK_ENTRY(copyright_entry),
                       get_comment(comment, "copyright"));
    gtk_entry_set_text(GTK_ENTRY(isrc_entry),
                       get_comment(comment, "isrc"));
    gtk_entry_set_text(GTK_ENTRY(location_entry),
                       get_comment(comment, "location"));
#endif
                       
    filename_utf8 = filename_to_utf8(vte.filename);

    title = g_strdup_printf(_("%s - Audacious"), g_basename(filename_utf8));
    gtk_window_set_title(GTK_WINDOW(window), title);
    g_free(title);

    gtk_entry_set_text(GTK_ENTRY(filename_entry), filename_utf8);
    gtk_editable_set_position(GTK_EDITABLE(filename_entry), -1);

    g_free(filename_utf8);

    gtk_entry_set_text(GTK_ENTRY(rg_track_entry), rg_track_gain);
    gtk_entry_set_text(GTK_ENTRY(rg_album_entry), rg_album_gain);
    gtk_entry_set_text(GTK_ENTRY(rg_track_peak_entry), rg_track_peak);
    gtk_editable_set_position(GTK_EDITABLE(rg_track_peak_entry), -1);
    gtk_entry_set_text(GTK_ENTRY(rg_album_peak_entry), rg_album_peak);
    gtk_editable_set_position(GTK_EDITABLE(rg_album_peak_entry), -1);

/*    if (*rg_track_gain == '\0' && *rg_album_gain == '\0' &&
        *rg_track_peak == '\0' && *rg_album_peak == '\0') {
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(rg_show_button),
                                     FALSE);
    }
    else
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(rg_show_button),
                                     TRUE);*/

    /* ov_clear closes the file */
    if (clear_vf) ov_clear(&vf);
    g_mutex_unlock(vf_mutex);


    gtk_widget_set_sensitive(tag_frame, vfs_is_writeable(vte.filename));

    g_signal_connect_swapped(title_entry, "changed", change_buttons,
                             save_button);
    g_signal_connect_swapped(performer_entry, "changed", change_buttons,
                             save_button);
    g_signal_connect_swapped(album_entry, "changed", change_buttons,
                             save_button);
    g_signal_connect_swapped(date_entry, "changed", change_buttons,
                             save_button);
    g_signal_connect_swapped(user_comment_entry, "changed", change_buttons,
                             save_button);
    g_signal_connect_swapped(tracknumber_entry, "changed", change_buttons,
                             save_button);
    g_signal_connect_swapped(GTK_COMBO(genre_combo)->entry, "changed",
                             G_CALLBACK(change_buttons), save_button);

    gtk_window_set_position(GTK_WINDOW(window), GTK_WIN_POS_CENTER);
    gtk_widget_show_all(window);
    gtk_widget_hide(rg_frame);

    gtk_widget_set_sensitive(save_button, FALSE);
    gtk_widget_set_sensitive(remove_button, FALSE);
}
