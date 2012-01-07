/*
 * jump-to-time.c
 * Copyright 1998-2003 XMMS development team
 * Copyright 2003-2004 BMP development team
 * Copyright 2011 John Lindgren
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; under version 2 or version 3 of the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses>.
 *
 * The Audacious team does not consider modular code linking to
 * Audacious or using our public API to be a derived work.
 */

#include <stdio.h>
#include <gtk/gtk.h>

#include <audacious/drct.h>
#include <audacious/gtk-compat.h>
#include <audacious/i18n.h>

#include "config.h"
#include "libaudgui.h"
#include "libaudgui-gtk.h"

static GtkWidget * window = NULL;

static void jump_to_time_cb (GtkWidget * widget, GtkWidget * entry)
{
    unsigned int min = 0, sec = 0;
    int params = sscanf (gtk_entry_get_text ((GtkEntry *) entry), "%u:%u", & min,
     & sec);

    int time;
    if (params == 2)
        time = 60 * min + sec;
    else if (params == 1)
        time = min;
    else
        return;

    if (aud_drct_get_playing ())
        aud_drct_seek (1000 * time);

    if (window)
        gtk_widget_destroy (window);
}

void audgui_jump_to_time (void)
{
    if (! aud_drct_get_playing ())
        return;

    if (window)
    {
        gtk_window_present ((GtkWindow *) window);
        return;
    }

    window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
    g_signal_connect (window, "destroy", (GCallback) gtk_widget_destroyed,
     & window);
    gtk_window_set_type_hint ((GtkWindow *) window, GDK_WINDOW_TYPE_HINT_DIALOG);
    gtk_window_set_title ((GtkWindow *) window, _("Jump to Time"));
    gtk_window_set_resizable ((GtkWindow *) window, FALSE);
    gtk_container_set_border_width ((GtkContainer *) window, 6);
    audgui_destroy_on_escape (window);

    GtkWidget * vbox = gtk_vbox_new (FALSE, 6);
    gtk_container_add ((GtkContainer *) window, vbox);

    GtkWidget * hbox_new = gtk_hbox_new (FALSE, 6);
    gtk_box_pack_start ((GtkBox *) vbox, hbox_new, FALSE, FALSE, 0);

    GtkWidget * time_entry = gtk_entry_new ();
    gtk_entry_set_activates_default ((GtkEntry *) time_entry, TRUE);
    gtk_box_pack_start ((GtkBox *) hbox_new, time_entry, FALSE, FALSE, 0);

    GtkWidget * label = gtk_label_new (_("mm:ss"));
    gtk_box_pack_start ((GtkBox *) hbox_new, label, FALSE, FALSE, 0);

    GtkWidget * bbox = gtk_hbutton_box_new ();
    gtk_box_pack_start ((GtkBox *) vbox, bbox, TRUE, TRUE, 0);
    gtk_button_box_set_layout ((GtkButtonBox *) bbox, GTK_BUTTONBOX_END);
    gtk_box_set_spacing ((GtkBox *) bbox, 6);

    GtkWidget * cancel = gtk_button_new_from_stock (GTK_STOCK_CANCEL);
    gtk_container_add ((GtkContainer *) bbox, cancel);
    g_signal_connect_swapped (cancel, "clicked", (GCallback) gtk_widget_destroy,
     window);

    GtkWidget * jump = gtk_button_new_from_stock (GTK_STOCK_JUMP_TO);
    gtk_widget_set_can_default (jump, TRUE);
    gtk_container_add ((GtkContainer *) bbox, jump);
    g_signal_connect (jump, "clicked", (GCallback) jump_to_time_cb, time_entry);

    unsigned int tindex = aud_drct_get_time () / 1000;
    char time_str[10];
    snprintf (time_str, sizeof time_str, "%u:%2.2u", tindex / 60, tindex % 60);
    gtk_entry_set_text ((GtkEntry *) time_entry, time_str);
    gtk_editable_select_region ((GtkEditable *) time_entry, 0, -1);

    gtk_widget_show_all (window);
    gtk_widget_grab_default (jump);
}
