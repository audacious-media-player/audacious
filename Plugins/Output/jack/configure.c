/*      xmms - jack output plugin
 *    Copyright (C) 2004      Chris Morgan
 *
 *
 *  audacious port (2005) by Giacomo Lozito from develia.org
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
 *
 *  Contains code Copyright (C) 1998-2000 Mikael Alm, Olle Hallnas,
 *  Thomas Nillson, 4Front Technologies and Galex Yen
 */

#include "jack.h"

#include "libaudacious/configdb.h"

#include <gtk/gtk.h>

extern jackconfig jack_cfg;

static GtkWidget *configure_win = NULL, *vbox;

static GtkWidget *GTK_isTraceEnabled;
static GtkWidget *bbox, *ok, *cancel;

static GtkWidget *option_frame;
static GtkWidget *port_connection_mode_box;
static GtkWidget *port_connection_mode_combo;

#define GET_CHARS(edit) gtk_editable_get_chars(GTK_EDITABLE(edit), 0, -1)

static void configure_win_ok_cb(GtkWidget * w, gpointer data)
{
	ConfigDb *cfgfile;

	jack_cfg.isTraceEnabled = (gint) GTK_CHECK_BUTTON(GTK_isTraceEnabled)->toggle_button.active;
  jack_cfg.port_connection_mode = GET_CHARS(GTK_COMBO(port_connection_mode_combo)->entry);

  jack_set_port_connection_mode(); /* update the connection mode */

	cfgfile = bmp_cfg_db_open();

	bmp_cfg_db_set_bool(cfgfile, "jack", "isTraceEnabled", jack_cfg.isTraceEnabled);
	bmp_cfg_db_set_string(cfgfile, "jack", "port_connection_mode", jack_cfg.port_connection_mode);
	bmp_cfg_db_close(cfgfile);

	gtk_widget_destroy(configure_win);
}

static void get_port_connection_modes(GtkCombo *combo)
{
    GtkWidget *item;
    char *descr;

    descr = g_strdup("Connect to all available jack ports");
    item = gtk_list_item_new_with_label(descr);
    gtk_widget_show(item);
    g_free(descr);
    gtk_combo_set_item_string(combo, GTK_ITEM(item), "CONNECT_ALL");
    gtk_container_add(GTK_CONTAINER(combo->list), item);

    descr = g_strdup("Connect only the output ports");
    item = gtk_list_item_new_with_label(descr);
    gtk_widget_show(item);
    g_free(descr);
    gtk_combo_set_item_string(combo, GTK_ITEM(item), "CONNECT_OUTPUT");
    gtk_container_add(GTK_CONTAINER(combo->list), item);

    descr = g_strdup("Connect to no ports");
    item = gtk_list_item_new_with_label(descr);
    gtk_widget_show(item);
    g_free(descr);
    gtk_combo_set_item_string(combo, GTK_ITEM(item), "CONNECT_NONE");
    gtk_container_add(GTK_CONTAINER(combo->list), item);
}

void jack_configure(void)
{
	if (!configure_win)
	{	
		configure_win = gtk_window_new(GTK_WINDOW_TOPLEVEL);
		gtk_window_set_type_hint ( GTK_WINDOW(configure_win), GDK_WINDOW_TYPE_HINT_DIALOG);
		gtk_signal_connect(GTK_OBJECT(configure_win), "destroy", GTK_SIGNAL_FUNC(gtk_widget_destroyed), &configure_win);
		gtk_window_set_title(GTK_WINDOW(configure_win), ("jack Plugin configuration"));
		gtk_window_set_policy(GTK_WINDOW(configure_win), FALSE, FALSE, FALSE);
		gtk_window_set_position(GTK_WINDOW(configure_win), GTK_WIN_POS_MOUSE);
		gtk_container_border_width(GTK_CONTAINER(configure_win), 10);

    vbox = gtk_vbox_new(FALSE, 10);
    gtk_container_add(GTK_CONTAINER(configure_win), vbox);

    /* add a frame for other xmms-jack options */
    option_frame = gtk_frame_new("Options:");
    gtk_box_pack_start(GTK_BOX(vbox), option_frame, FALSE, FALSE, 0);

    /* add a hbox that will contain a label for a dropdown and the dropdown itself */
    port_connection_mode_box = gtk_hbox_new(FALSE, 5);
    gtk_container_set_border_width(GTK_CONTAINER(port_connection_mode_box), 5);
    gtk_container_add(GTK_CONTAINER(option_frame), port_connection_mode_box);

    /* add the label */
    gtk_box_pack_start(GTK_BOX(port_connection_mode_box), gtk_label_new("Connection mode:"),
                       FALSE, FALSE, 0);

    /* add the dropdown */
    port_connection_mode_combo = gtk_combo_new();
    get_port_connection_modes(GTK_COMBO(port_connection_mode_combo));
    gtk_entry_set_text(GTK_ENTRY(GTK_COMBO(port_connection_mode_combo)->entry),
                       jack_cfg.port_connection_mode);
    gtk_box_pack_start(GTK_BOX(port_connection_mode_box), port_connection_mode_combo,
                       TRUE, TRUE, 0);

    /* create a check_button for debug output */
    GTK_isTraceEnabled = gtk_check_button_new_with_label("Enable debug printing");
    gtk_box_pack_start(GTK_BOX(vbox), GTK_isTraceEnabled, FALSE, FALSE, 0);
    gtk_widget_show(GTK_isTraceEnabled);

    /* set the state of the check_button based upon the value of */
    /* isTracingEnabled */
    GTK_CHECK_BUTTON(GTK_isTraceEnabled)->toggle_button.active = jack_cfg.isTraceEnabled;

    /* add the box for the ok/canceled buttons at the bottom */
    bbox = gtk_hbox_new(FALSE, 10);
    gtk_box_pack_start(GTK_BOX(vbox), bbox, FALSE, FALSE, 0);

		ok = gtk_button_new_with_label(("Ok"));
		gtk_signal_connect(GTK_OBJECT(ok), "clicked", GTK_SIGNAL_FUNC(configure_win_ok_cb), NULL);
		GTK_WIDGET_SET_FLAGS(ok, GTK_CAN_DEFAULT);
		gtk_box_pack_start(GTK_BOX(bbox), ok, TRUE, TRUE, 0);
		gtk_widget_show(ok);
		gtk_widget_grab_default(ok);

		cancel = gtk_button_new_with_label(("Cancel"));
		gtk_signal_connect_object(GTK_OBJECT(cancel), "clicked", GTK_SIGNAL_FUNC(gtk_widget_destroy), GTK_OBJECT(configure_win));
		GTK_WIDGET_SET_FLAGS(cancel, GTK_CAN_DEFAULT);
		gtk_box_pack_start(GTK_BOX(bbox), cancel, TRUE, TRUE, 0);
		gtk_widget_show(cancel);

		gtk_widget_show_all(configure_win);
	}
	else
		gdk_window_raise(configure_win->window);
}
