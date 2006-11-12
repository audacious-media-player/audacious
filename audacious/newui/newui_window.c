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
#include "audacious/ui_fileinfo.h"

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

static void
newui_entry_set_image(const char *text)
{
        GladeXML *xml = g_object_get_data(G_OBJECT(newui_win), "glade-xml");
        GtkWidget *widget = glade_xml_get_widget(xml, "newui_albumart_img");
        GdkPixbuf *pixbuf;
        int width, height;
        double aspect;

        if (xml == NULL || widget == NULL)
                return;

        pixbuf = gdk_pixbuf_new_from_file(text, NULL);

        if (pixbuf == NULL)
                return;

        width  = gdk_pixbuf_get_width(GDK_PIXBUF(pixbuf));
        height = gdk_pixbuf_get_height(GDK_PIXBUF(pixbuf));

        if(strcmp(DATA_DIR "/images/audio.png", text))
        {
                if(width == 0)
                        width = 1;
                aspect = (double)height / (double)width;
                if(aspect > 1.0) {
                        height = (int)(cfg.filepopup_pixelsize * aspect);
                        width = cfg.filepopup_pixelsize;
                } else {
                        height = cfg.filepopup_pixelsize;
                        width = (int)(cfg.filepopup_pixelsize / aspect);
                }
                GdkPixbuf *pixbuf2 = gdk_pixbuf_scale_simple(GDK_PIXBUF(pixbuf), width, height, GDK_INTERP_BILINEAR);
                g_object_unref(G_OBJECT(pixbuf));
                pixbuf = pixbuf2;
        }

        gtk_image_set_from_pixbuf(GTK_IMAGE(widget), GDK_PIXBUF(pixbuf));
        g_object_unref(G_OBJECT(pixbuf));
}

void
newui_update_nowplaying_from_entry(PlaylistEntry *entry)
{
	GladeXML *xml = g_object_get_data(G_OBJECT(newui_win), "glade-xml");
	GtkWidget *widget;
	gchar *tmp;
	static gchar *last_artwork = NULL;
	gchar *fullpath;

	widget = glade_xml_get_widget(xml, "newui_titlestring");
	tmp = g_strdup_printf("<span size='x-large'>%s</span>", entry->title);
	gtk_label_set_markup(GTK_LABEL(widget), tmp);
	g_free(tmp);

	if (entry->tuple == NULL)
		return;

	widget = glade_xml_get_widget(xml, "newui_label_title");
	gtk_label_set_text(GTK_LABEL(widget), entry->tuple->track_name);

	widget = glade_xml_get_widget(xml, "newui_label_artist");
	gtk_label_set_text(GTK_LABEL(widget), entry->tuple->performer);

	widget = glade_xml_get_widget(xml, "newui_label_album");
	gtk_label_set_text(GTK_LABEL(widget), entry->tuple->album_name);

	widget = glade_xml_get_widget(xml, "newui_label_genre");
	gtk_label_set_text(GTK_LABEL(widget), entry->tuple->genre);

	widget = glade_xml_get_widget(xml, "newui_label_year");
	tmp = g_strdup_printf("%d", entry->tuple->year);
	gtk_label_set_text(GTK_LABEL(widget), tmp);
	g_free(tmp);

	widget = glade_xml_get_widget(xml, "newui_label_tracknum");
	tmp = g_strdup_printf("%d", entry->tuple->track_number);
	gtk_label_set_text(GTK_LABEL(widget), tmp);
	g_free(tmp);

	widget = glade_xml_get_widget(xml, "newui_label_tracklen");
	tmp = g_strdup_printf("%d:%02d", entry->tuple->length / 60000, (entry->tuple->length / 1000) % 60);
	gtk_label_set_text(GTK_LABEL(widget), tmp);
	g_free(tmp);

        if (cfg.use_file_cover) {
                /* Use the file name */
                fullpath = g_strconcat(entry->tuple->file_path, "/", entry->tuple->file_name, NULL);
        } else {
                fullpath = g_strconcat(entry->tuple->file_path, "/", NULL);
        }

        if (!last_artwork || strcmp(last_artwork, fullpath))
	{
		if (last_artwork != NULL)
	                g_free(last_artwork);

                last_artwork = g_strdup(fullpath);

                tmp = fileinfo_recursive_get_image(entry->tuple->file_path, entry->tuple->file_name, 0);
                if (tmp)
                {
                        newui_entry_set_image(tmp);
                        g_free(tmp);
                } else {
                        newui_entry_set_image(DATA_DIR "/images/audio.png");
                }
        }

        g_free(fullpath);
}
