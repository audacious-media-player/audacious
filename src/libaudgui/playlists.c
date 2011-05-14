/*
 * Audacious
 * Copyright (C) 2005-2011 Audacious Team
 *
 * Based on BMP:
 * Copyright (C) 2003-2004 BMP Development Team
 *
 * Based on XMMS:
 * Copyright (C) 1998-2003 XMMS Development Team
 *
 * Audacious is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free Software
 * Foundation, version 2 or version 3 of the License.
 *
 * Audacious is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
 * A PARTICULAR PURPOSE. See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * Audacious. If not, see <http://www.gnu.org/licenses/>.
 *
 * The Audacious team does not consider modular code linking to Audacious or
 * using our public API to be a derived work.
 */

#include <gtk/gtk.h>

#include <audacious/audconfig.h>
#include <audacious/i18n.h>
#include <audacious/playlist.h>
#include <libaudcore/vfs.h>

#include "config.h"
#include "libaudgui.h"

static gchar * select_file (gboolean save, const gchar * default_filename)
{
    GtkWidget * dialog = gtk_file_chooser_dialog_new (save ?
     _("Export Playlist") : _("Import Playlist"), NULL, save ?
     GTK_FILE_CHOOSER_ACTION_SAVE : GTK_FILE_CHOOSER_ACTION_OPEN, NULL, NULL);

    if (default_filename)
        gtk_file_chooser_set_uri ((GtkFileChooser *) dialog, default_filename);

    gtk_dialog_add_button ((GtkDialog *) dialog, GTK_STOCK_CANCEL,
     GTK_RESPONSE_REJECT);
    gtk_dialog_add_button ((GtkDialog *) dialog, save ? GTK_STOCK_SAVE :
     GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT);

    gtk_dialog_set_default_response ((GtkDialog *) dialog, GTK_RESPONSE_ACCEPT);

    if (aud_cfg->playlist_path)
        gtk_file_chooser_set_current_folder_uri ((GtkFileChooser *) dialog,
         aud_cfg->playlist_path);

    gchar * filename = NULL;
    if (gtk_dialog_run ((GtkDialog *) dialog) == GTK_RESPONSE_ACCEPT)
        filename = gtk_file_chooser_get_uri ((GtkFileChooser *) dialog);

    g_free (aud_cfg->playlist_path);
    aud_cfg->playlist_path = gtk_file_chooser_get_current_folder_uri
     ((GtkFileChooser *) dialog);

    gtk_widget_destroy (dialog);
    return filename;
}

static gboolean confirm_overwrite (const gchar * filename)
{
    GtkWidget * dialog = gtk_message_dialog_new (NULL, 0, GTK_MESSAGE_QUESTION,
     GTK_BUTTONS_YES_NO, _("Overwrite %s?"), filename);

    gint result = gtk_dialog_run ((GtkDialog *) dialog);
    gtk_widget_destroy (dialog);
    return (result == GTK_RESPONSE_YES);
}

void audgui_import_playlist (void)
{
    gint list = aud_playlist_get_active ();
    gint id = aud_playlist_get_unique_id (list);

    gchar * filename = select_file (FALSE, NULL);
    if (! filename)
        return;

    if ((list = aud_playlist_by_unique_id (id)) < 0)
        return;

    aud_playlist_entry_delete (list, 0, aud_playlist_entry_count (list));
    aud_playlist_entry_insert (list, 0, filename, NULL, FALSE);
    aud_playlist_set_filename (list, filename);
}

void audgui_export_playlist (void)
{
    gint list = aud_playlist_get_active ();
    gint id = aud_playlist_get_unique_id (list);

    gchar * def = aud_playlist_get_filename (list);
    gchar * filename = select_file (TRUE, def);
    g_free (def);

    if (! filename || (vfs_file_test (filename, G_FILE_TEST_EXISTS) &&
     ! confirm_overwrite (filename)))
        return;

    if ((list = aud_playlist_by_unique_id (id)) < 0)
        return;

    aud_playlist_save (list, filename);
    g_free (filename);
}
