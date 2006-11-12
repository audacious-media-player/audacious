/*  Audacious
 *  Copyright (C) 2005-2006  Audacious team
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
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 
 *  02110-1301, USA.
 */

#include "audacious/glade.h"

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include <glib.h>
#include <glib/gi18n.h>
#include <gtk/gtk.h>
#include <glade/glade.h>
#include <string.h>
#include <stddef.h>
#include <stdio.h>
#include <sys/types.h>
#include <dirent.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "audacious/plugin.h"
#include "audacious/pluginenum.h"
#include "audacious/input.h"
#include "audacious/effect.h"
#include "audacious/general.h"
#include "audacious/output.h"
#include "audacious/visualization.h"

#include "audacious/main.h"
#include "audacious/urldecode.h"
#include "audacious/util.h"
#include "audacious/dnd.h"
#include "audacious/titlestring.h"

#include "libaudacious/configdb.h"

#include "audacious/playlist.h"

#include "audacious/mainwin.h"

GtkWidget *newui_win;

void
create_newui_window(void)
{
	const char *glade_file = DATA_DIR "/glade/newui.glade";
	GladeXML *xml;
	GtkWidget *widget;

	xml = glade_xml_new_or_die(_("Stock GTK2 UI"), glade_file, NULL, NULL);

	glade_xml_signal_autoconnect(xml);

	newui_win = glade_xml_get_widget(xml, "newui_window");
	g_object_set_data(G_OBJECT(newui_win), "glade-xml", xml);

	widget = glade_xml_get_widget(xml, "newui_albumart_img");
	gtk_image_set_from_file(GTK_IMAGE(widget), DATA_DIR "/images/audio.png");

	/* build menu and toolbars */
}

void
show_newui_window(void)
{
	gtk_widget_show(newui_win);
}

void
newui_update_nowplaying_from_entry(PlaylistEntry *entry)
{
	GladeXML *xml = g_object_get_data(G_OBJECT(newui_win), "glade-xml");
	GtkWidget *widget;
	gchar *tmp;

	widget = glade_xml_get_widget(xml, "newui_titlestring");
	tmp = g_strdup_printf("<span size='x-large'>%s</span>", entry->title);
	gtk_label_set_markup(GTK_LABEL(widget), tmp);
	g_free(tmp);
}
