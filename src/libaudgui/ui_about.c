/*
 *  Audacious - Cross-platform multimedia player
 *  Copyright (C) 2005-2007  Audacious development team
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

#include <limits.h>

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include <glib.h>
#include <gtk/gtk.h>

#include <gdk/gdk.h>
#include <gdk/gdkkeysyms.h>

#include <audacious/gtk-compat.h>
#include <audacious/i18n.h>
#include <audacious/misc.h>

#include "libaudgui-gtk.h"

/* ui_credits.c */
GtkWidget * audgui_get_credits_widget (void);

static GtkWidget *about_window = NULL;

void
audgui_show_about_window(void)
{
    GtkWidget *brief_label;
    gchar *text;
    static const gchar *audacious_brief;

    if (about_window != NULL)
    {
        gtk_window_present(GTK_WINDOW(about_window));
        return;
    }

    aud_get_audacious_credits(&audacious_brief, NULL, NULL);

    about_window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_container_set_border_width ((GtkContainer *) about_window, 3);

    g_signal_connect(about_window, "destroy",
                     G_CALLBACK(gtk_widget_destroyed), &about_window);

    gtk_window_set_title(GTK_WINDOW(about_window), _("About Audacious"));
    gtk_window_set_resizable(GTK_WINDOW(about_window), FALSE);
    audgui_destroy_on_escape (about_window);

    GtkWidget * vbox = gtk_vbox_new (FALSE, 6);
    gtk_container_add ((GtkContainer *) about_window, vbox);

    gchar name[PATH_MAX];
    snprintf (name, sizeof name, "%s/images/about-logo.png", aud_get_path
     (AUD_PATH_DATA_DIR));
    GtkWidget * image = gtk_image_new_from_file (name);
    gtk_box_pack_start ((GtkBox *) vbox, image, FALSE, FALSE, 0);

    brief_label = gtk_label_new(NULL);
    text = g_strdup_printf(_(audacious_brief), VERSION);

    gtk_label_set_markup(GTK_LABEL(brief_label), text);
    gtk_label_set_justify(GTK_LABEL(brief_label), GTK_JUSTIFY_CENTER);
    g_free(text);
    
    gtk_box_pack_start ((GtkBox *) vbox, brief_label, FALSE, FALSE, 0);

    GtkWidget * exp = gtk_expander_new (_("Credits"));
    gtk_container_add ((GtkContainer *) exp, audgui_get_credits_widget ());
    gtk_box_pack_start ((GtkBox *) vbox, exp, TRUE, TRUE, 0);

    gtk_widget_show_all(about_window);
}

void
audgui_hide_about_window(void)
{
    g_return_if_fail(about_window);
    gtk_widget_hide(GTK_WIDGET(about_window));
}
