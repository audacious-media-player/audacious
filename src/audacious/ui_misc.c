/*  Audacious - Cross-platform multimedia player
 *  Copyright © 2009 Audacious development team
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

#include <gtk/gtk.h>

#include "config.h"
#include "i18n.h"

GtkWidget *
ui_markup_label_new(const gchar *text)
{
    GtkWidget *label;

    g_return_val_if_fail(text != NULL, NULL);

    label = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(label), text);
    gtk_misc_set_alignment(GTK_MISC(label), 0, 0.5);

    return label;
}

void
ui_display_unsupported_version_warning(void)
{
    GtkWidget *window;
    GtkWidget *vbox;
    GtkWidget *label;
    GtkWidget *sep;
    GtkWidget *button;
    GtkWidget *bbox;

    window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(window), _("Audacious - Unsupported Version Warning"));
    gtk_window_set_resizable(GTK_WINDOW(window), FALSE);
    gtk_window_set_keep_above(GTK_WINDOW(window), TRUE);

    vbox = gtk_vbox_new(FALSE, 0);
    gtk_container_add(GTK_CONTAINER(window), vbox);

    label = ui_markup_label_new(_("<big><b>Audacious2 is not ready yet!</b></big>"));
    gtk_box_pack_start(GTK_BOX(vbox), label, FALSE, FALSE, 2);

    sep = gtk_hseparator_new();
    gtk_box_pack_start(GTK_BOX(vbox), sep, FALSE, FALSE, 2);

    label = ui_markup_label_new(_("<b>Please don't file bugs at this point unless they represent major problems.</b>"));
    gtk_box_pack_start(GTK_BOX(vbox), label, FALSE, FALSE, 4);

    label = ui_markup_label_new(_("This is an <i>alpha</i> build of Audacious2. It is still fairly incomplete and unpolished.\n"
                                  "We suspect that most minor bugs reported during this time will be resolved once more things\n"
                                  "stablize. However, if there is a major bug, such as a crash on playing a specific file, we\n"
                                  "need to know about it.\n"
                                  "\n"
                                  "However, we ask you to be aware that dealing with trivial bugs is distracting at this point,\n"
                                  "and to please refrain from doing so. Thanks in advance!"));
    gtk_misc_set_padding(GTK_MISC(label), 12, 0);
    gtk_box_pack_start(GTK_BOX(vbox), label, FALSE, FALSE, 4);

    label = ui_markup_label_new(_("<b>Blogging about bugs in it is also not helpful.</b>"));
    gtk_box_pack_start(GTK_BOX(vbox), label, FALSE, FALSE, 4);

    label = ui_markup_label_new(_("Likewise, we're developers. We don't read blogs on a regular basis. If there is a major bug,\n"
                                  "we ask you to bring it to our attention via our bugtracking instance at http://jira.atheme.org/,\n"
                                  "or via the forums that are on the website.\n"
                                  "\n"
                                  "This will help us find out more information about your concerns, which we would likely\n"
                                  "not know about otherwise."));
    gtk_misc_set_padding(GTK_MISC(label), 12, 0);
    gtk_box_pack_start(GTK_BOX(vbox), label, FALSE, FALSE, 4);

    sep = gtk_hseparator_new();
    gtk_box_pack_start(GTK_BOX(vbox), sep, FALSE, FALSE, 2);

    bbox = gtk_hbutton_box_new();
    gtk_button_box_set_layout(GTK_BUTTON_BOX(bbox), GTK_BUTTONBOX_END);
    gtk_box_pack_start(GTK_BOX(vbox), bbox, FALSE, FALSE, 2);

    button = gtk_button_new_from_stock(GTK_STOCK_CLOSE);
    g_signal_connect_swapped(G_OBJECT(button), "clicked", G_CALLBACK(gtk_widget_hide), window);
    gtk_container_add(GTK_CONTAINER(bbox), button);

    gtk_widget_show_all(window);
}
