/*  BMP - Cross-platform multimedia player
 *  Copyright (C) 2003-2004  BMP development team.
 *
 *  Based on XMMS:
 *  Copyright (C) 1998-2003  XMMS development team.
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
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include <glib.h>
#include <glib/gi18n.h>
#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

#include "audacious/util.h"
#include <libaudacious/util.h>
#include <libaudacious/vfs.h>
#include <libaudacious/xentry.h>

#include "tagging.h"

#include "m4a.xpm"

static GtkWidget *window = NULL;
static GtkWidget *filename_entry, *id3_frame;
static GtkWidget *title_entry, *artist_entry, *album_entry, *year_entry,
    *tracknum_entry, *comment_entry;
static GtkWidget *genre_combo;

GtkWidget *vbox, *hbox, *left_vbox, *table;
GtkWidget *mpeg_frame, *mpeg_box;
GtkWidget *label, *filename_vbox;
GtkWidget *bbox;
GtkWidget *remove_id3, *cancel, *save;
GtkWidget *boxx;

const gchar *emphasis[4];
const gchar *bool_label[2];

static GList *genre_list = NULL;
static gchar *current_filename = NULL;

#define MAX_STR_LEN 100

#if 0
static guint
audmp4_strip_spaces(char *src, size_t n)
{   
    gchar *space = NULL,        /* last space in src */
        *start = src;

    while (n--)
        switch (*src++) {
        case '\0':
            n = 0;              /* breaks out of while loop */

            src--;
            break;
        case ' ':
            if (space == NULL)
                space = src - 1;
            break;
        default:
            space = NULL;       /* don't terminate intermediate spaces */

            break;
        }
    if (space != NULL) {
        src = space;
        *src = '\0';
    }
    return src - start;
}

static void
set_entry_tag(GtkEntry * entry, gchar * tag, gint length)
{
    gint stripped_len;
    gchar *text, *text_utf8;

    stripped_len = audmp4_strip_spaces(tag, length);
    text = g_strdup_printf(tag);

    if ((text_utf8 = str_to_utf8(text))) {
        gtk_entry_set_text(entry, text_utf8);
        g_free(text_utf8);
    }
    else {
        gtk_entry_set_text(entry, "");
    }

    g_free(text);
}

static void
get_entry_tag(GtkEntry * entry, gchar * tag, gint length)
{
    gchar *text = str_to_utf8(gtk_entry_get_text(entry));
    memset(tag, ' ', length);
    memcpy(tag, text, strlen(text) > length ? length : strlen(text));
}

static void
press_save(GtkWidget * w, gpointer data)
{
    gtk_button_clicked(GTK_BUTTON(save));
}
#endif

static gint
genre_comp_func(gconstpointer a, gconstpointer b)
{
    return strcasecmp(a, b);
}

static gboolean
fileinfo_keypress_cb(GtkWidget * widget,
                     GdkEventKey * event,
                     gpointer data)
{
    if (!event)
        return FALSE;

    switch (event->keyval) {
    case GDK_Escape:
        gtk_widget_destroy(window);
        break;
    default:
        return FALSE;
    }

    return TRUE;
}

#if 0
static void
label_set_text(GtkWidget * label, gchar * str, ...)
{
    va_list args;
    gchar tempstr[MAX_STR_LEN];

    va_start(args, str);
    g_vsnprintf(tempstr, MAX_STR_LEN, str, args);
    va_end(args);

    gtk_label_set_text(GTK_LABEL(label), tempstr);
}
#endif

static void
change_buttons(GtkObject * object)
{
    gtk_widget_set_sensitive(GTK_WIDGET(object), TRUE);
}

void
audmp4_file_info_box(gchar * filename)
{
    gint i;
    gchar *title, *filename_utf8;
    MP4FileHandle mp4file;

    emphasis[0] = _("None");
    emphasis[1] = _("50/15 ms");
    emphasis[2] = "";
    emphasis[3] = _("CCIT J.17");
    bool_label[0] = _("No");
    bool_label[1] = _("Yes");

    if (!window) {
        GtkWidget *pixmapwid;
        GdkPixbuf *pixbuf;
        PangoAttrList *attrs;
        PangoAttribute *attr;

        window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
        gtk_window_set_type_hint(GTK_WINDOW(window),
                                 GDK_WINDOW_TYPE_HINT_DIALOG);
        gtk_window_set_resizable(GTK_WINDOW(window), TRUE);
        gtk_window_set_position(GTK_WINDOW(window), GTK_WIN_POS_CENTER);
        g_signal_connect(G_OBJECT(window), "destroy",
                         G_CALLBACK(gtk_widget_destroyed), &window);
        gtk_container_set_border_width(GTK_CONTAINER(window), 10);

        vbox = gtk_vbox_new(FALSE, 10);
        gtk_container_add(GTK_CONTAINER(window), vbox);


        filename_vbox = gtk_hbox_new(FALSE, 5);
        gtk_box_pack_start(GTK_BOX(vbox), filename_vbox, FALSE, TRUE, 0);

        pixbuf = gdk_pixbuf_new_from_xpm_data((const gchar **)
                                              m4a_xpm);
        pixmapwid = gtk_image_new_from_pixbuf(pixbuf);
        g_object_unref(pixbuf);
        gtk_misc_set_alignment(GTK_MISC(pixmapwid), 0, 0);
        gtk_box_pack_start(GTK_BOX(filename_vbox), pixmapwid, FALSE, FALSE,
                           0);

        label = gtk_label_new(NULL);

        attrs = pango_attr_list_new();

        attr = pango_attr_weight_new(PANGO_WEIGHT_BOLD);
        attr->start_index = 0;
        attr->end_index = -1;
        pango_attr_list_insert(attrs, attr);

        gtk_label_set_attributes(GTK_LABEL(label), attrs);
        gtk_label_set_text(GTK_LABEL(label), _("Name:"));
        gtk_box_pack_start(GTK_BOX(filename_vbox), label, FALSE, FALSE, 0);

        filename_entry = gtk_entry_new();
        gtk_editable_set_editable(GTK_EDITABLE(filename_entry), FALSE);
        gtk_box_pack_start(GTK_BOX(filename_vbox), filename_entry, TRUE,
                           TRUE, 0);

        hbox = gtk_hbox_new(FALSE, 0);
        gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, TRUE, 0);

        /* tagging information */
        id3_frame = gtk_frame_new(_("Song Metadata"));
        gtk_box_pack_start(GTK_BOX(vbox), id3_frame, FALSE, TRUE, 0);

        table = gtk_table_new(7, 5, FALSE);
        gtk_container_set_border_width(GTK_CONTAINER(table), 5);
        gtk_container_add(GTK_CONTAINER(id3_frame), table);

        label = gtk_label_new(_("Title:"));
        gtk_misc_set_alignment(GTK_MISC(label), 1, 0.5);
        gtk_label_set_attributes(GTK_LABEL(label), attrs);
        gtk_table_attach(GTK_TABLE(table), label, 0, 1, 0, 1, GTK_FILL,
                         GTK_FILL, 5, 5);

        title_entry = gtk_entry_new();
        gtk_table_attach(GTK_TABLE(table), title_entry, 1, 6, 0, 1,
                         GTK_FILL | GTK_EXPAND | GTK_SHRINK,
                         GTK_FILL | GTK_EXPAND | GTK_SHRINK, 0, 5);

        label = gtk_label_new(_("Artist:"));
        gtk_misc_set_alignment(GTK_MISC(label), 1, 0.5);
        gtk_label_set_attributes(GTK_LABEL(label), attrs);
        gtk_table_attach(GTK_TABLE(table), label, 0, 1, 1, 2, GTK_FILL,
                         GTK_FILL, 5, 5);

        artist_entry = gtk_entry_new();
        gtk_table_attach(GTK_TABLE(table), artist_entry, 1, 6, 1, 2,
                         GTK_FILL | GTK_EXPAND | GTK_SHRINK,
                         GTK_FILL | GTK_EXPAND | GTK_SHRINK, 0, 5);

        label = gtk_label_new(_("Album:"));
        gtk_misc_set_alignment(GTK_MISC(label), 1, 0.5);
        gtk_label_set_attributes(GTK_LABEL(label), attrs);
        gtk_table_attach(GTK_TABLE(table), label, 0, 1, 2, 3, GTK_FILL,
                         GTK_FILL, 5, 5);

        album_entry = gtk_entry_new();
        gtk_table_attach(GTK_TABLE(table), album_entry, 1, 6, 2, 3,
                         GTK_FILL | GTK_EXPAND | GTK_SHRINK,
                         GTK_FILL | GTK_EXPAND | GTK_SHRINK, 0, 5);

        label = gtk_label_new(_("Comment:"));
        gtk_misc_set_alignment(GTK_MISC(label), 1, 0.5);
        gtk_label_set_attributes(GTK_LABEL(label), attrs);
        gtk_table_attach(GTK_TABLE(table), label, 0, 1, 3, 4, GTK_FILL,
                         GTK_FILL, 5, 5);

        comment_entry = gtk_entry_new();
        gtk_table_attach(GTK_TABLE(table), comment_entry, 1, 6, 3, 4,
                         GTK_FILL | GTK_EXPAND | GTK_SHRINK,
                         GTK_FILL | GTK_EXPAND | GTK_SHRINK, 0, 5);

        label = gtk_label_new(_("Year:"));
        gtk_misc_set_alignment(GTK_MISC(label), 1, 0.5);
        gtk_label_set_attributes(GTK_LABEL(label), attrs);
        gtk_table_attach(GTK_TABLE(table), label, 0, 1, 4, 5, GTK_FILL,
                         GTK_FILL, 5, 5);

        year_entry = gtk_entry_new();
        gtk_entry_set_width_chars(GTK_ENTRY(year_entry),4);
        gtk_table_attach(GTK_TABLE(table), year_entry, 1, 2, 4, 5,
                         GTK_FILL | GTK_EXPAND | GTK_SHRINK,
                         GTK_FILL | GTK_EXPAND | GTK_SHRINK, 0, 5);

        label = gtk_label_new(_("Track number:"));
        gtk_misc_set_alignment(GTK_MISC(label), 1, 0.5);
        gtk_label_set_attributes(GTK_LABEL(label), attrs);
        gtk_table_attach(GTK_TABLE(table), label, 2, 3, 4, 5, GTK_FILL,
                         GTK_FILL, 5, 5);

        tracknum_entry = gtk_entry_new();
        gtk_widget_set_usize(tracknum_entry, 40, -1);
        gtk_table_attach(GTK_TABLE(table), tracknum_entry, 3, 4, 4, 5,
                         GTK_FILL | GTK_EXPAND | GTK_SHRINK,
                         GTK_FILL | GTK_EXPAND | GTK_SHRINK, 0, 5);

        pango_attr_list_unref(attrs);

        label = gtk_label_new(_("Genre:"));
        gtk_misc_set_alignment(GTK_MISC(label), 1, 0.5);
        gtk_label_set_attributes(GTK_LABEL(label), attrs);
        gtk_table_attach(GTK_TABLE(table), label, 0, 1, 5, 6, GTK_FILL,
                         GTK_FILL, 5, 5);

        genre_combo = gtk_combo_new();
        gtk_entry_set_editable(GTK_ENTRY(GTK_COMBO(genre_combo)->entry),
                               FALSE);
        if (!genre_list) {
            for (i = 0; i < GENRE_MAX; i++)
                genre_list =
                    g_list_prepend(genre_list,
                                   (gchar *) audmp4_id3_genres[i]);
            genre_list = g_list_prepend(genre_list, "");
            genre_list = g_list_sort(genre_list, genre_comp_func);
        }
        gtk_combo_set_popdown_strings(GTK_COMBO(genre_combo), genre_list);

        gtk_table_attach(GTK_TABLE(table), genre_combo, 1, 6, 5, 6,
                         GTK_FILL | GTK_EXPAND | GTK_SHRINK,
                         GTK_FILL | GTK_EXPAND | GTK_SHRINK, 0, 5);

        boxx = gtk_hbutton_box_new();
        gtk_hbutton_box_set_layout_default(GTK_BUTTONBOX_SPREAD);

        bbox = gtk_hbutton_box_new();
        gtk_button_box_set_layout(GTK_BUTTON_BOX(bbox), GTK_BUTTONBOX_END);
        gtk_button_box_set_spacing(GTK_BUTTON_BOX(bbox), 5);
        gtk_box_pack_start(GTK_BOX(vbox), bbox, FALSE, TRUE, 0);

        cancel = gtk_button_new_from_stock(GTK_STOCK_CLOSE);
        g_signal_connect_swapped(G_OBJECT(cancel), "clicked",
                                 G_CALLBACK(gtk_widget_destroy),
                                 G_OBJECT(window));
        GTK_WIDGET_SET_FLAGS(cancel, GTK_CAN_DEFAULT);
        gtk_box_pack_start(GTK_BOX(bbox), cancel, FALSE, FALSE, 0);
        gtk_widget_grab_default(cancel);


        gtk_table_set_col_spacing(GTK_TABLE(left_vbox), 1, 10);


        g_signal_connect_swapped(G_OBJECT(title_entry), "changed",
                                 G_CALLBACK(change_buttons), save);
        g_signal_connect_swapped(G_OBJECT(artist_entry), "changed",
                                 G_CALLBACK(change_buttons), save);
        g_signal_connect_swapped(G_OBJECT(album_entry), "changed",
                                 G_CALLBACK(change_buttons), save);
        g_signal_connect_swapped(G_OBJECT(year_entry), "changed",
                                 G_CALLBACK(change_buttons), save);
        g_signal_connect_swapped(G_OBJECT(comment_entry), "changed",
                                 G_CALLBACK(change_buttons), save);
        g_signal_connect_swapped(G_OBJECT(tracknum_entry), "changed",
                                 G_CALLBACK(change_buttons), save);
        g_signal_connect_swapped(G_OBJECT(GTK_COMBO(genre_combo)->entry), "changed",
                                 G_CALLBACK(change_buttons), save);
        g_signal_connect(G_OBJECT(window), "key_press_event",
                         G_CALLBACK(fileinfo_keypress_cb), NULL);
    }

    g_free(current_filename);
    current_filename = g_strdup(filename);

    filename_utf8 = filename_to_utf8(filename);

    title = g_strdup_printf(_("%s - Audacious"), g_basename(filename_utf8));
    gtk_window_set_title(GTK_WINDOW(window), title);
    g_free(title);

    gtk_entry_set_text(GTK_ENTRY(filename_entry), filename_utf8);
    g_free(filename_utf8);

    gtk_editable_set_position(GTK_EDITABLE(filename_entry), -1);

    gtk_entry_set_text(GTK_ENTRY(artist_entry), "");
    gtk_entry_set_text(GTK_ENTRY(album_entry), "");
    gtk_entry_set_text(GTK_ENTRY(year_entry), "");
    gtk_entry_set_text(GTK_ENTRY(tracknum_entry), "");
    gtk_entry_set_text(GTK_ENTRY(comment_entry), "");

    gtk_list_select_item(GTK_LIST(GTK_COMBO(genre_combo)->list),
                         g_list_index(genre_list, ""));

    gtk_widget_set_sensitive(id3_frame,
                             vfs_is_writeable(filename));

    gtk_widget_set_sensitive(GTK_WIDGET(save), FALSE);

    /* Ok! Lets set the information now. */

    if ((mp4file = MP4Read(filename, 0)) != NULL)
    {
        gtk_entry_set_text(GTK_ENTRY(artist_entry), audmp4_get_artist(mp4file));
        gtk_entry_set_text(GTK_ENTRY(title_entry), audmp4_get_title(mp4file));
        gtk_entry_set_text(GTK_ENTRY(year_entry), g_strdup_printf("%d", audmp4_get_year(mp4file)));
        gtk_entry_set_text(GTK_ENTRY(album_entry), audmp4_get_album(mp4file));
        MP4Close(mp4file);
    }

    gtk_widget_show_all(window);
}
