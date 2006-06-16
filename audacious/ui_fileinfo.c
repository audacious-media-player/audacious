/*
 * Audacious: A cross-platform multimedia player
 * Copyright (c) 2006 William Pitcock, Tony Vroon, George Averill,
 *                    Giacomo Lozito, Derek Pomery and Yoshiki Yazawa.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

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

#include "glade.h"

#include "plugin.h"
#include "pluginenum.h"
#include "input.h"
#include "effect.h"
#include "general.h"
#include "output.h"
#include "visualization.h"

#include "main.h"
#include "skin.h"
#include "urldecode.h"
#include "util.h"
#include "dnd.h"
#include "libaudacious/configdb.h"
#include "libaudacious/titlestring.h"

#include "mainwin.h"
#include "ui_playlist.h"
#include "skinwin.h"
#include "playlist_list.h"
#include "build_stamp.h"
#include "ui_fileinfo.h"

GtkWidget *fileinfo_win;

static void
fileinfo_entry_set_text(const char *entry, const char *text)
{
	GladeXML *xml = g_object_get_data(G_OBJECT(fileinfo_win), "glade-xml");
	GtkWidget *widget = glade_xml_get_widget(xml, entry);

	if (xml == NULL || widget == NULL)
		return;

	gtk_entry_set_text(GTK_ENTRY(widget), text);
}

static void
fileinfo_entry_set_text_free(const char *entry, char *text)
{
	GladeXML *xml = g_object_get_data(G_OBJECT(fileinfo_win), "glade-xml");
	GtkWidget *widget = glade_xml_get_widget(xml, entry);

	if (xml == NULL || widget == NULL)
		return;

	gtk_entry_set_text(GTK_ENTRY(widget), text);

	g_free(text);
}

void fileinfo_hide(gpointer unused)
{
	gtk_widget_hide(fileinfo_win);

	/* Clear it out. */
	fileinfo_entry_set_text("entry_title", "");
	fileinfo_entry_set_text("entry_artist", "");
	fileinfo_entry_set_text("entry_album", "");
	fileinfo_entry_set_text("entry_comment", "");
	fileinfo_entry_set_text("entry_genre", "");
	fileinfo_entry_set_text("entry_year", "");
	fileinfo_entry_set_text("entry_track", "");
	fileinfo_entry_set_text("entry_location", "");
}

void
create_fileinfo_window(void)
{
	const gchar *glade_file = DATA_DIR "/glade/fileinfo.glade";
	GladeXML *xml;
	GtkWidget *widget;

	xml = glade_xml_new_or_die(_("Track Information Window"), glade_file, NULL, NULL);

	glade_xml_signal_autoconnect(xml);

	fileinfo_win = glade_xml_get_widget(xml, "fileinfo_win");
	g_object_set_data(G_OBJECT(fileinfo_win), "glade-xml", xml);
	gtk_window_set_transient_for(GTK_WINDOW(fileinfo_win), GTK_WINDOW(mainwin));

	widget = glade_xml_get_widget(xml, "image_artwork");
	gtk_image_set_from_file(GTK_IMAGE(widget), DATA_DIR "/images/audio.png");

	widget = glade_xml_get_widget(xml, "btn_close");
	g_signal_connect(G_OBJECT(widget), "clicked", (GCallback) fileinfo_hide, NULL);
}

void
fileinfo_show_for_tuple(TitleInput *tuple)
{
	if (tuple == NULL)
		return;

	gtk_widget_realize(fileinfo_win);

	fileinfo_entry_set_text("entry_title", tuple->track_name);
	fileinfo_entry_set_text("entry_artist", tuple->performer);
	fileinfo_entry_set_text("entry_album", tuple->album_name);
	fileinfo_entry_set_text("entry_comment", tuple->comment);
	fileinfo_entry_set_text("entry_genre", tuple->genre);
	fileinfo_entry_set_text("entry_location", tuple->file_path);

	if (tuple->year != 0)
		fileinfo_entry_set_text_free("entry_year", g_strdup_printf("%d", tuple->year));

	if (tuple->track_number != 0)
		fileinfo_entry_set_text_free("entry_track", g_strdup_printf("%d", tuple->track_number));

	gtk_widget_show(fileinfo_win);
}

void
fileinfo_show_for_filepath(const gchar *path)
{
	TitleInput *tuple = input_get_song_tuple(path);

	if (tuple == NULL)
		return input_file_info_box(path);

	fileinfo_show_for_tuple(tuple);

	bmp_title_input_free(tuple);
}
