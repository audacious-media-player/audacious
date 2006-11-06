/*  Audacious
 *  Copyright (C) 2005-2006  Audacious development team.
 *
 *  BMP - Cross-platform multimedia player
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
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include <glib.h>
#include <glib/gi18n.h>
#include <gtk/gtk.h>
#include <string.h>

#include "fft.h"
#include "input.h"
#include "main.h"
#include "mainwin.h"
#include "output.h"
#include "util.h"
#include "visualization.h"
#include "playback.h"
#include "widgets/widgetcore.h"
#include "pluginenum.h"
#include "titlestring.h"

#include "libaudacious/util.h"
#include "libaudacious/xentry.h"

G_LOCK_DEFINE_STATIC(vis_mutex);

struct _VisNode {
    gint time;
    gint nch;
    gint length;                /* number of samples per channel */
    gint16 data[2][512];
};

typedef struct _VisNode VisNode;


InputPluginData ip_data = {
    NULL,
    NULL,
    FALSE,
    FALSE,
    FALSE,
    FALSE,
    NULL
};

static GList *vis_list = NULL;

gchar *input_info_text = NULL;

InputPlugin *
get_current_input_plugin(void)
{
    return ip_data.current_input_plugin;
}

void
set_current_input_plugin(InputPlugin * ip)
{
    ip_data.current_input_plugin = ip;
}

GList *
get_input_list(void)
{
    return ip_data.input_list;
}


gboolean
input_is_enabled(const gchar * filename)
{
    gchar *basename = g_path_get_basename(filename);
    gint enabled;

    enabled = GPOINTER_TO_INT(g_hash_table_lookup(plugin_matrix, basename));
    g_free(basename);

    return enabled;
}

static void
disabled_iplugins_foreach_func(const gchar * name,
                               gboolean enabled,
                               GString * list)
{
    g_return_if_fail(list != NULL);

    if (enabled)
        return;

    if (list->len > 0)
        g_string_append(list, ":");

    g_string_append(list, name);
}

gchar *
input_stringify_disabled_list(void)
{
    GString *disabled_list;

    disabled_list = g_string_new("");
    g_hash_table_foreach(plugin_matrix,
                         (GHFunc) disabled_iplugins_foreach_func,
                         disabled_list);

    return g_string_free(disabled_list, FALSE);
}

void
free_vis_data(void)
{
    G_LOCK(vis_mutex);
    g_list_foreach(vis_list, (GFunc) g_free, NULL);
    g_list_free(vis_list);
    vis_list = NULL;
    G_UNLOCK(vis_mutex);
}

static void
convert_to_s16_ne(AFormat fmt, gpointer ptr, gint16 * left,
                  gint16 * right, gint nch, gint max)
{
    gint16 *ptr16;
    guint16 *ptru16;
    guint8 *ptru8;
    gint i;

    switch (fmt) {
    case FMT_U8:
        ptru8 = ptr;
        if (nch == 1)
            for (i = 0; i < max; i++)
                left[i] = ((*ptru8++) ^ 128) << 8;
        else
            for (i = 0; i < max; i++) {
                left[i] = ((*ptru8++) ^ 128) << 8;
                right[i] = ((*ptru8++) ^ 128) << 8;
            }
        break;
    case FMT_S8:
        ptru8 = ptr;
        if (nch == 1)
            for (i = 0; i < max; i++)
                left[i] = (*ptru8++) << 8;
        else
            for (i = 0; i < max; i++) {
                left[i] = (*ptru8++) << 8;
                right[i] = (*ptru8++) << 8;
            }
        break;
    case FMT_U16_LE:
        ptru16 = ptr;
        if (nch == 1)
            for (i = 0; i < max; i++, ptru16++)
                left[i] = GUINT16_FROM_LE(*ptru16) ^ 32768;
        else
            for (i = 0; i < max; i++) {
                left[i] = GUINT16_FROM_LE(*ptru16) ^ 32768;
                ptru16++;
                right[i] = GUINT16_FROM_LE(*ptru16) ^ 32768;
                ptru16++;
            }
        break;
    case FMT_U16_BE:
        ptru16 = ptr;
        if (nch == 1)
            for (i = 0; i < max; i++, ptru16++)
                left[i] = GUINT16_FROM_BE(*ptru16) ^ 32768;
        else
            for (i = 0; i < max; i++) {
                left[i] = GUINT16_FROM_BE(*ptru16) ^ 32768;
                ptru16++;
                right[i] = GUINT16_FROM_BE(*ptru16) ^ 32768;
                ptru16++;
            }
        break;
    case FMT_U16_NE:
        ptru16 = ptr;
        if (nch == 1)
            for (i = 0; i < max; i++)
                left[i] = (*ptru16++) ^ 32768;
        else
            for (i = 0; i < max; i++) {
                left[i] = (*ptru16++) ^ 32768;
                right[i] = (*ptru16++) ^ 32768;
            }
        break;
    case FMT_S16_LE:
        ptr16 = ptr;
        if (nch == 1)
            for (i = 0; i < max; i++, ptr16++)
                left[i] = GINT16_FROM_LE(*ptr16);
        else
            for (i = 0; i < max; i++) {
                left[i] = GINT16_FROM_LE(*ptr16);
                ptr16++;
                right[i] = GINT16_FROM_LE(*ptr16);
                ptr16++;
            }
        break;
    case FMT_S16_BE:
        ptr16 = ptr;
        if (nch == 1)
            for (i = 0; i < max; i++, ptr16++)
                left[i] = GINT16_FROM_BE(*ptr16);
        else
            for (i = 0; i < max; i++) {
                left[i] = GINT16_FROM_BE(*ptr16);
                ptr16++;
                right[i] = GINT16_FROM_BE(*ptr16);
                ptr16++;
            }
        break;
    case FMT_S16_NE:
        ptr16 = ptr;
        if (nch == 1)
            for (i = 0; i < max; i++)
                left[i] = (*ptr16++);
        else
            for (i = 0; i < max; i++) {
                left[i] = (*ptr16++);
                right[i] = (*ptr16++);
            }
        break;
    }
}

InputVisType
input_get_vis_type()
{
    return INPUT_VIS_OFF;
}

void
input_add_vis(gint time, guchar * s, InputVisType type)
{
    g_warning("plugin uses obsoleted input_add_vis()");
}

void
input_add_vis_pcm(gint time, AFormat fmt, gint nch, gint length, gpointer ptr)
{
    VisNode *vis_node;
    gint max;

    max = length / nch;
    if (fmt == FMT_U16_LE || fmt == FMT_U16_BE || fmt == FMT_U16_NE ||
        fmt == FMT_S16_LE || fmt == FMT_S16_BE || fmt == FMT_S16_NE)
        max /= 2;
    max = CLAMP(max, 0, 512);

    vis_node = g_new0(VisNode, 1);
    vis_node->time = time;
    vis_node->nch = nch;
    vis_node->length = max;
    convert_to_s16_ne(fmt, ptr, vis_node->data[0], vis_node->data[1], nch,
                      max);

    G_LOCK(vis_mutex);
    vis_list = g_list_append(vis_list, vis_node);
    G_UNLOCK(vis_mutex);
}

void
input_dont_show_warning(GtkObject * object, gpointer user_data)
{
    *((gboolean *) user_data) =
        !gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(object));
}


void
input_show_unplayable_files(const gchar * filename)
{
    static GtkWidget *dialog = NULL;
    static GtkListStore *store = NULL;

    const gchar *markup = 
        N_("<b><big>Unable to play files.</big></b>\n\n"
           "The following files could not be played. Please check that:\n"
           "1. they are accessible.\n"
           "2. you have enabled the media plugins required.");

    GtkTreeIter iter;

    gchar *filename_utf8;

    if (!dialog) {
        GtkWidget *vbox, *check;
        GtkWidget *expander;
        GtkWidget *scrolled, *treeview;
        GtkCellRenderer *renderer;

        dialog =
            gtk_message_dialog_new_with_markup(GTK_WINDOW(mainwin),
                                               GTK_DIALOG_DESTROY_WITH_PARENT,
                                               GTK_MESSAGE_ERROR,
                                               GTK_BUTTONS_OK,
                                               _(markup));

        vbox = gtk_vbox_new(FALSE, 6);

        check = gtk_check_button_new_with_label
                  (_("Don't show this warning anymore"));

        expander = gtk_expander_new_with_mnemonic(_("Show more _details"));

        scrolled = gtk_scrolled_window_new(NULL, NULL);
        gtk_container_add(GTK_CONTAINER(expander), scrolled);

        store = gtk_list_store_new(1, G_TYPE_STRING);

        treeview = gtk_tree_view_new_with_model(GTK_TREE_MODEL(store));
        gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(treeview), FALSE);
        gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(scrolled),
                                              treeview);
        
        renderer = gtk_cell_renderer_text_new();
        gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(treeview), 
                                                    -1, _("Filename"),
                                                    renderer,
                                                    "text", 0, 
                                                    NULL);

        vbox = GTK_DIALOG(dialog)->vbox;
        gtk_box_pack_start(GTK_BOX(vbox), check, FALSE, FALSE, 0);
        gtk_box_pack_start(GTK_BOX(vbox), expander, TRUE, TRUE, 0);

        g_signal_connect(dialog, "response",
                         G_CALLBACK(gtk_widget_destroy),
                         dialog);
        g_signal_connect(dialog, "destroy",
                         G_CALLBACK(gtk_widget_destroyed),
                         &dialog);
        g_signal_connect(check, "clicked",
                         G_CALLBACK(input_dont_show_warning),
                         &cfg.warn_about_unplayables);

        gtk_widget_show_all(dialog);
    }

    gtk_window_present(GTK_WINDOW(dialog));

    filename_utf8 = filename_to_utf8(filename);
    gtk_list_store_append(store, &iter);
    gtk_list_store_set(store, &iter, 0, filename_utf8, -1);
    g_free(filename_utf8);
}


void
input_file_not_playable(const gchar * filename)
{
    if (cfg.warn_about_unplayables)
        input_show_unplayable_files(filename);
}

/*
 * input_check_file()
 *
 * Inputs:
 *       filename to check recursively against input plugins
 *       whether or not to show an error
 *
 * Outputs:
 *       pointer to input plugin which can handle this file
 *       otherwise, NULL
 *
 *       (the previous code returned a boolean of whether or not we can
 *        play the file... even WORSE for performance)
 *
 * Side Effects:
 *       various input plugins open the file and probe it
 *       -- this can have very ugly effects performance wise on streams
 *
 * --nenolod, Dec 31 2005
 */
InputPlugin *
input_check_file(const gchar * filename, gboolean show_warning)
{
    VFSFile *fd;
    GList *node;
    InputPlugin *ip;
    gchar *filename_proxy;
    gint ret = 1;

    filename_proxy = g_strdup(filename);
    fd = vfs_fopen(filename, "rb");

    for (node = get_input_list(); node != NULL; node = g_list_next(node)) {
        ip = INPUT_PLUGIN(node->data);
        if (ip && input_is_enabled(ip->filename) &&
            (ip->is_our_file_from_vfs != NULL &&
             (ret = ip->is_our_file_from_vfs(filename_proxy, fd) > 0) || 
             (ip->is_our_file != NULL &&
              (ret = ip->is_our_file(filename_proxy)) > 0))) {
            g_free(filename_proxy);
            vfs_fclose(fd);
            return ip;
        }
	else if (ret <= -1)
	    break;
    }

    g_free(filename_proxy);

    if (show_warning && !(ret <= -1)) {
        input_file_not_playable(filename);
    }

    vfs_fclose(fd);

    return NULL;
}


void
input_set_eq(gint on, gfloat preamp, gfloat * bands)
{
    if (!ip_data.playing)
        return;

    if (!get_current_input_plugin())
        return;

    if (get_current_input_plugin()->set_eq)
        get_current_input_plugin()->set_eq(on, preamp, bands);
}

void
input_get_song_info(const gchar * filename, gchar ** title, gint * length)
{
    InputPlugin *ip = NULL;
    BmpTitleInput *input;
    GList *node;
    gchar *tmp = NULL, *ext;
    gchar *filename_proxy;

    g_return_if_fail(filename != NULL);
    g_return_if_fail(title != NULL);
    g_return_if_fail(length != NULL);

    filename_proxy = g_strdup(filename);

    for (node = get_input_list(); node != NULL; node = g_list_next(node)) {
        ip = INPUT_PLUGIN(node->data);
        if (input_is_enabled(ip->filename) && ip->is_our_file(filename_proxy))
            break;
    }

    if (ip && node && ip->get_song_info) {
        ip->get_song_info(filename_proxy, &tmp, length);
        *title = str_to_utf8(tmp);
        g_free(tmp);
    }
    else {
        input = bmp_title_input_new();

        tmp = g_strdup(filename);
        if ((ext = strrchr(tmp, '.')))
            *ext = '\0';

        input->file_name = g_path_get_basename(tmp);
        input->file_ext = ext ? ext + 1 : NULL;
        input->file_path = tmp;

        if ((tmp = xmms_get_titlestring(xmms_get_gentitle_format(), input))) {
            (*title) = str_to_utf8(tmp);
            g_free(tmp);
        }
        else {
            (*title) = filename_to_utf8(input->file_name);
        }

        (*length) = -1;

        bmp_title_input_free(input);
        input = NULL;
    }

    g_free(filename_proxy);
}

TitleInput *
input_get_song_tuple(const gchar * filename)
{
    InputPlugin *ip = NULL;
    TitleInput *input;
    GList *node;
    gchar *tmp = NULL, *ext;
    gchar *filename_proxy;

    if (filename == NULL)
	return NULL;

    filename_proxy = g_strdup(filename);

    for (node = get_input_list(); node != NULL; node = g_list_next(node)) {
        ip = INPUT_PLUGIN(node->data);
        if (input_is_enabled(ip->filename) && ip->is_our_file(filename_proxy))
            break;
    }

    if (ip && node && ip->get_song_tuple)
        input = ip->get_song_tuple(filename_proxy);
    else {
        input = bmp_title_input_new();

        tmp = g_strdup(filename);
        if ((ext = strrchr(tmp, '.')))
            *ext = '\0';

	input->track_name = NULL;
	input->length = -1;
	input_get_song_info(filename, &input->track_name, &input->length);
        input->file_name = g_path_get_basename(tmp);
        input->file_ext = ext ? ext + 1 : NULL;
        input->file_path = tmp;
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

    window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_resizable(GTK_WINDOW(window), FALSE);
    gtk_window_set_position(GTK_WINDOW(window), GTK_WIN_POS_CENTER);

    basename = g_path_get_basename(filename);
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

    filename_entry = xmms_entry_new();
    filename_utf8 = filename_to_utf8(filename);

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
                             GTK_SIGNAL_FUNC(gtk_widget_destroy),
                             GTK_OBJECT(window));
    GTK_WIDGET_SET_FLAGS(cancel, GTK_CAN_DEFAULT);
    gtk_box_pack_start(GTK_BOX(bbox), cancel, TRUE, TRUE, 0);

    gtk_widget_show_all(window);
}

void
input_file_info_box(const gchar * filename)
{
    GList *node;
    InputPlugin *ip;
    gchar *filename_proxy;

    filename_proxy = g_strdup(filename);

    for (node = get_input_list(); node != NULL; node = g_list_next(node)) {
        ip = INPUT_PLUGIN(node->data);
        if (input_is_enabled(ip->filename)
            && ip->is_our_file(filename_proxy)) {
            if (ip->file_info_box)
                ip->file_info_box(filename_proxy);
            else
                input_general_file_info_box(filename, ip);

            g_free(filename_proxy);
            return;
        }
    }

    input_general_file_info_box(filename, NULL);
    g_free(filename_proxy);
}

GList *
input_scan_dir(const gchar * path)
{
    GList *node, *result = NULL;
    InputPlugin *ip;
    gchar *path_proxy;

    g_return_val_if_fail(path != NULL, NULL);

    if (*path == '/')
        while (path[1] == '/')
            path++;

    path_proxy = g_strdup(path);

    for (node = get_input_list(); node; node = g_list_next(node)) {
        ip = INPUT_PLUGIN(node->data);

        if (!ip)
            continue;

        if (!ip->scan_dir)
            continue;

        if (!input_is_enabled(ip->filename))
            continue;

        if ((result = ip->scan_dir(path_proxy)))
            break;
    }

    g_free(path_proxy);

    return result;
}

void
input_get_volume(gint * l, gint * r)
{
    *l = -1;
    *r = -1;
    if (bmp_playback_get_playing()) {
        if (get_current_input_plugin() &&
            get_current_input_plugin()->get_volume) {
            get_current_input_plugin()->get_volume(l, r);
            return;
        }
    }
    output_get_volume(l, r);
}

void
input_set_volume(gint l, gint r)
{
    if (bmp_playback_get_playing()) {
        if (get_current_input_plugin() &&
            get_current_input_plugin()->set_volume) {
            get_current_input_plugin()->set_volume(l, r);
            return;
        }
    }
    output_set_volume(l, r);
}

void
input_update_vis(gint time)
{
    GList *node;
    VisNode *vis = NULL, *visnext = NULL;
    gboolean found = FALSE;

    G_LOCK(vis_mutex);
    node = vis_list;
    while (g_list_next(node) && !found) {
        visnext = g_list_next(node)->data;
        vis = node->data;

        if (vis->time >= time)
            break;

        vis_list = g_list_delete_link(vis_list, node);

        if (visnext->time >= time) {
            found = TRUE;
            break;
        }
        g_free(vis);
        node = vis_list;
    }
    G_UNLOCK(vis_mutex);

    if (found) {
        vis_send_data(vis->data, vis->nch, vis->length);
        g_free(vis);
    }
    else
        vis_send_data(NULL, 0, 0);
}


gchar *
input_get_info_text(void)
{
    return g_strdup(input_info_text);
}

void
input_set_info_text(const gchar * text)
{
    g_free(input_info_text);
    input_info_text = g_strdup(text);
    mainwin_set_info_text();
}

void
input_set_status_buffering(gboolean status)
{
    if (!bmp_playback_get_playing())
        return;

    if (!get_current_input_plugin())
        return;

    ip_data.buffering = status;

    g_return_if_fail(mainwin_playstatus != NULL);

    if (ip_data.buffering == TRUE && mainwin_playstatus != NULL && mainwin_playstatus->ps_status == STATUS_STOP)
        mainwin_playstatus->ps_status = STATUS_PLAY;

    playstatus_set_status_buffering(mainwin_playstatus, ip_data.buffering);
}

void
input_about(gint index)
{
    InputPlugin *ip;

    ip = g_list_nth(ip_data.input_list, index)->data;
    if (ip && ip->about)
        ip->about();
}

void
input_configure(gint index)
{
    InputPlugin *ip;

    ip = g_list_nth(ip_data.input_list, index)->data;
    if (ip && ip->configure)
        ip->configure();
}
