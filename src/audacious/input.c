/*  Audacious - Cross-platform multimedia player
 *  Copyright (C) 2005-2009  Audacious development team
 *
 *  Based on BMP:
 *  Copyright (C) 2003-2004  BMP development team
 *
 *  Based on XMMS:
 *  Copyright (C) 1998-2003  XMMS development team
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

#include <glib.h>
#include <glib/gi18n.h>
#include <gtk/gtk.h>
#include <string.h>

#include <mowgli.h>

#include "input.h"
#include "plugin-registry.h"
#include "probe.h"

PlaybackData ip_data =
{
    NULL,
    FALSE,
    FALSE,
    FALSE,
    NULL
};

InputPlayback *
get_current_input_playback(void)
{
    return ip_data.current_input_playback;
}

void
set_current_input_playback(InputPlayback * ip)
{
    ip_data.current_input_playback = ip;
}

static gboolean plugin_list_func (void * plugin, void * data)
{
    GList * * list = data;

    * list = g_list_prepend (* list, plugin);
    return TRUE;
}

GList * get_input_list (void)
{
    static GList * list = NULL;

    if (list == NULL)
        plugin_for_each (PLUGIN_TYPE_INPUT, plugin_list_func, & list);

    return list;
}

Tuple *
input_get_song_tuple(const gchar * filename)
{
    InputPlugin *ip = NULL;
    Tuple *input = NULL;
    gchar *ext = NULL;

    if (filename == NULL)
        return NULL;

    ip = file_probe (filename, FALSE);

    if (ip && ip->get_song_tuple)
        input = ip->get_song_tuple (filename);
    else
    {
        gchar *scratch;

        scratch = uri_to_display_basename(filename);
        tuple_associate_string(input, FIELD_FILE_NAME, NULL, scratch);
        g_free(scratch);

        scratch = uri_to_display_dirname(filename);
        tuple_associate_string(input, FIELD_FILE_PATH, NULL, scratch);
        g_free(scratch);

        ext = strrchr(filename, '.');
        if (ext != NULL) {
            ++ext;
            tuple_associate_string(input, FIELD_FILE_EXT, NULL, ext);
        }

        tuple_associate_int(input, FIELD_LENGTH, NULL, -1);
    }

    return input;
}

static void
input_general_file_info_box(const gchar * filename, InputPlugin * ip)
{
    GtkWidget *window, *vbox;
    GtkWidget *label, *filename_hbox, *filename_entry;
    GtkWidget *bbox, *cancel;

    gchar *title, *fileinfo, *basename, *iplugin;
    gchar *filename_utf8;
    gchar *realfn = NULL;

    window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_resizable(GTK_WINDOW(window), FALSE);
    gtk_window_set_position(GTK_WINDOW(window), GTK_WIN_POS_CENTER);

    realfn = g_filename_from_uri(filename, NULL, NULL);
    basename = g_path_get_basename(realfn ? realfn : filename);
    fileinfo = filename_to_utf8(basename);
    title = g_strdup_printf(_("audacious: %s"), fileinfo);

    gtk_window_set_title(GTK_WINDOW(window), title);

    g_free(title);
    g_free(fileinfo);
    g_free(basename);

    gtk_container_set_border_width(GTK_CONTAINER(window), 10);

    vbox = gtk_vbox_new(FALSE, 10);
    gtk_container_add(GTK_CONTAINER(window), vbox);

    filename_hbox = gtk_hbox_new(FALSE, 5);
    gtk_box_pack_start(GTK_BOX(vbox), filename_hbox, FALSE, TRUE, 0);

    label = gtk_label_new(_("Filename:"));
    gtk_box_pack_start(GTK_BOX(filename_hbox), label, FALSE, TRUE, 0);

    filename_entry = gtk_entry_new();
    filename_utf8 = filename_to_utf8(realfn ? realfn : filename);
    g_free(realfn); realfn = NULL;

    gtk_entry_set_text(GTK_ENTRY(filename_entry), filename_utf8);
    gtk_editable_set_editable(GTK_EDITABLE(filename_entry), FALSE);
    gtk_box_pack_start(GTK_BOX(filename_hbox), filename_entry, TRUE, TRUE, 0);

    g_free(filename_utf8);

    if (ip)
        if (ip->description)
            iplugin = ip->description;
        else
            iplugin = ip->filename;
    else
        iplugin = _("No input plugin recognized this file");

    title = g_strdup_printf(_("Input plugin: %s"), iplugin);

    label = gtk_label_new(title);
    gtk_misc_set_alignment(GTK_MISC(label), 0, 0.5);
    gtk_label_set_justify(GTK_LABEL(label), GTK_JUSTIFY_LEFT);
    g_free(title);
    gtk_box_pack_start(GTK_BOX(vbox), label, FALSE, TRUE, 0);

    bbox = gtk_hbutton_box_new();
    gtk_button_box_set_layout(GTK_BUTTON_BOX(bbox), GTK_BUTTONBOX_END);
    gtk_box_pack_start(GTK_BOX(vbox), bbox, FALSE, FALSE, 0);

    cancel = gtk_button_new_from_stock(GTK_STOCK_CLOSE);
    g_signal_connect_swapped(G_OBJECT(cancel), "clicked",
                             G_CALLBACK(gtk_widget_destroy),
                             window);
    GTK_WIDGET_SET_FLAGS(cancel, GTK_CAN_DEFAULT);
    gtk_box_pack_start(GTK_BOX(bbox), cancel, TRUE, TRUE, 0);

    gtk_widget_show_all(window);
}

void input_file_info_box (const gchar * filename)
{
    InputPlugin * decoder;

    decoder = file_probe (filename, FALSE);

    if (decoder != NULL && decoder->file_info_box != NULL)
        decoder->file_info_box (filename);
    else
        input_general_file_info_box (filename, decoder);
}

void
input_get_volume(gint * l, gint * r)
{
    if (ip_data.playing && ip_data.current_input_playback &&
     ip_data.current_input_playback->plugin->get_volume &&
     ip_data.current_input_playback->plugin->get_volume (l, r))
        return;

    output_get_volume (l, r);
}

void
input_set_volume(gint l, gint r)
{
    gint h_vol[2];

    h_vol[0] = l;
    h_vol[1] = r;
    hook_call("volume set", h_vol);

    if (ip_data.playing && ip_data.current_input_playback &&
     ip_data.current_input_playback->plugin->set_volume &&
     ip_data.current_input_playback->plugin->set_volume (l, r))
        return;

    output_set_volume (l, r);
}
