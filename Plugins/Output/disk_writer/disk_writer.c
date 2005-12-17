/*  XMMS - Cross-platform multimedia player
 *  Copyright (C) 1998-2004  Peter Alm, Mikael Alm, Olle Hallnas,
 *                           Thomas Nilsson and 4Front Technologies
 *  Copyright (C) 1999-2004  Haavard Kvaalen
 *
 *  File name suffix option added by Heikki Orsila 2003
 *  <heikki.orsila@iki.fi> (no copyrights claimed)
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
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include "config.h"

#include <gtk/gtk.h>
#include <glib/gi18n.h>
#include <stdio.h>
#include <string.h>

#include "audacious/plugin.h"
#include "libaudacious/beepctrl.h"
#include "libaudacious/dirbrowser.h"
#include "libaudacious/configfile.h"
#include "libaudacious/util.h"

struct wavhead
{
	guint32 main_chunk;
	guint32 length;
	guint32 chunk_type;
	guint32 sub_chunk;
	guint32 sc_len;
	guint16 format;
	guint16 modus;
	guint32 sample_fq;
	guint32 byte_p_sec;
	guint16 byte_p_spl;
	guint16 bit_p_spl;
	guint32 data_chunk;
	guint32 data_length;
};

static GtkWidget *configure_win = NULL, *configure_vbox;
static GtkWidget *path_hbox, *path_label, *path_entry, *path_browse, *path_dirbrowser = NULL;
static GtkWidget *configure_separator;
static GtkWidget *configure_bbox, *configure_ok, *configure_cancel;

static GtkWidget *use_suffix_toggle = NULL;
static gboolean use_suffix = FALSE;

static gchar *file_path = NULL;
static FILE *output_file = NULL;
static struct wavhead header;
static guint64 written = 0;
static AFormat afmt;
gint ctrlsocket_get_session_id(void);		/* FIXME */

static void disk_init(void);
static gint disk_open(AFormat fmt, gint rate, gint nch);
static void disk_write(void *ptr, gint length);
static void disk_close(void);
static void disk_flush(gint time);
static void disk_pause(short p);
static gint disk_free(void);
static gint disk_playing(void);
static gint disk_get_written_time(void);
static gint disk_get_output_time(void);
static void disk_configure(void);

OutputPlugin disk_op =
{
	NULL,
	NULL,
	NULL,			/* Description */
	disk_init,
	NULL,
	NULL,			/* about */
	disk_configure,		/* configure */
	NULL,			/* get_volume */
	NULL,			/* set_volume */
	disk_open,
	disk_write,
	disk_close,
	disk_flush,
	disk_pause,
	disk_free,
	disk_playing,
	disk_get_output_time,
	disk_get_written_time,
};

OutputPlugin *get_oplugin_info(void)
{
	disk_op.description = g_strdup_printf(_("Disk Writer Plugin %s"), VERSION);
	return &disk_op;
}

static void disk_init(void)
{
	ConfigFile *cfgfile;

	cfgfile = xmms_cfg_open_default_file();
	if (cfgfile)
	{
		xmms_cfg_read_string(cfgfile, "disk_writer", "file_path", &file_path);
		xmms_cfg_read_boolean(cfgfile, "disk_writer", "use_suffix", &use_suffix);
		xmms_cfg_free(cfgfile);
	}

	if (!file_path)
		file_path = g_strdup(g_get_home_dir());
}

static gint disk_open(AFormat fmt, gint rate, gint nch)
{
	gchar *filename, *title, *temp;
	gint pos;

	written = 0;
	afmt = fmt;

	if (xmms_check_realtime_priority())
	{
		xmms_show_message(_("Error"),
				  _("You cannot use the Disk Writer plugin\n"
				    "when you're running in realtime mode."),
				  _("OK"), FALSE, NULL, NULL);
		return 0;
	}

	pos = xmms_remote_get_playlist_pos(ctrlsocket_get_session_id());
	title = xmms_remote_get_playlist_file(ctrlsocket_get_session_id(), pos);
	if (!use_suffix) {
		if (title != NULL && (temp = strrchr(title, '.')) != NULL) {
			*temp = '\0';
		}
	}
	if (title == NULL || strlen(g_basename(title)) == 0)
	{
		g_free(title);
		/* No filename, lets try title instead */
		title = xmms_remote_get_playlist_title(ctrlsocket_get_session_id(), pos);
		while (title != NULL && (temp = strchr(title, '/')) != NULL)
			*temp = '-';

		if (title == NULL || strlen(g_basename(title)) == 0)
		{
			g_free(title);
			/* No title either.  Just set it to something. */
			title = g_strdup_printf("xmms-%d", pos);
		}
	}
	filename = g_strdup_printf("%s/%s.wav", file_path, g_basename(title));
	g_free(title);

	output_file = fopen(filename, "wb");
	g_free(filename);

	if (!output_file)
		return 0;

	memcpy(&header.main_chunk, "RIFF", 4);
	header.length = GUINT32_TO_LE(0);
	memcpy(&header.chunk_type, "WAVE", 4);
	memcpy(&header.sub_chunk, "fmt ", 4);
	header.sc_len = GUINT32_TO_LE(16);
	header.format = GUINT16_TO_LE(1);
	header.modus = GUINT16_TO_LE(nch);
	header.sample_fq = GUINT32_TO_LE(rate);
	if (fmt == FMT_U8 || fmt == FMT_S8)
		header.bit_p_spl = GUINT16_TO_LE(8);
	else
		header.bit_p_spl = GUINT16_TO_LE(16);
	header.byte_p_sec = GUINT32_TO_LE(rate * header.modus * (GUINT16_FROM_LE(header.bit_p_spl) / 8));
	header.byte_p_spl = GUINT16_TO_LE((GUINT16_FROM_LE(header.bit_p_spl) / (8 / nch)));
	memcpy(&header.data_chunk, "data", 4);
	header.data_length = GUINT32_TO_LE(0);
	fwrite(&header, sizeof (struct wavhead), 1, output_file);

	return 1;
}

static void convert_buffer(gpointer buffer, gint length)
{
	gint i;

	if (afmt == FMT_S8)
	{
		guint8 *ptr1 = buffer;
		gint8 *ptr2 = buffer;

		for (i = 0; i < length; i++)
			*(ptr1++) = *(ptr2++) ^ 128;
	}
	if (afmt == FMT_S16_BE)
	{
		gint16 *ptr = buffer;

		for (i = 0; i < length >> 1; i++, ptr++)
			*ptr = GUINT16_SWAP_LE_BE(*ptr);
	}
	if (afmt == FMT_S16_NE)
	{
		gint16 *ptr = buffer;

		for (i = 0; i < length >> 1; i++, ptr++)
			*ptr = GINT16_TO_LE(*ptr);
	}
	if (afmt == FMT_U16_BE)
	{
		gint16 *ptr1 = buffer;
		guint16 *ptr2 = buffer;

		for (i = 0; i < length >> 1; i++, ptr2++)
			*(ptr1++) = GINT16_TO_LE(GUINT16_FROM_BE(*ptr2) ^ 32768);
	}
	if (afmt == FMT_U16_LE)
	{
		gint16 *ptr1 = buffer;
		guint16 *ptr2 = buffer;

		for (i = 0; i < length >> 1; i++, ptr2++)
			*(ptr1++) = GINT16_TO_LE(GUINT16_FROM_LE(*ptr2) ^ 32768);
	}
	if (afmt == FMT_U16_NE)
	{
		gint16 *ptr1 = buffer;
		guint16 *ptr2 = buffer;

		for (i = 0; i < length >> 1; i++, ptr2++)
			*(ptr1++) = GINT16_TO_LE((*ptr2) ^ 32768);
	}
}

static void disk_write(void *ptr, gint length)
{
	if (afmt == FMT_S8 || afmt == FMT_S16_BE ||
	    afmt == FMT_U16_LE || afmt == FMT_U16_BE || afmt == FMT_U16_NE)
		convert_buffer(ptr, length);
#ifdef WORDS_BIGENDIAN
	if (afmt == FMT_S16_NE)
		convert_buffer(ptr, length);
#endif
	written += fwrite(ptr, 1, length, output_file);
}

static void disk_close(void)
{
	if (output_file)
	{
		header.length = GUINT32_TO_LE(written + sizeof (struct wavhead) - 8);

		header.data_length = GUINT32_TO_LE(written);
		fseek(output_file, 0, SEEK_SET);
		fwrite(&header, sizeof (struct wavhead), 1, output_file);

		fclose(output_file);
		written = 0;
	}
	output_file = NULL;
}

static void disk_flush(gint time)
{
}

static void disk_pause(short p)
{
}

static gint disk_free(void)
{
	return 1000000;
}

static gint disk_playing(void)
{
	return 0;
}

static gint disk_get_written_time(void)
{
	if(header.byte_p_sec != 0)
		return (gint) ((written * 1000) / header.byte_p_sec);
	return 0;
}

static gint disk_get_output_time(void)
{
	return disk_get_written_time();
}

static void path_dirbrowser_cb(gchar * dir)
{
	gtk_entry_set_text(GTK_ENTRY(path_entry), dir);
}

static void path_browse_cb(GtkWidget * w, gpointer data)
{
	if (!path_dirbrowser)
	{
		path_dirbrowser = xmms_create_dir_browser(_("Select the directory where you want to store the output files:"), file_path, GTK_SELECTION_SINGLE, path_dirbrowser_cb);
		gtk_signal_connect(GTK_OBJECT(path_dirbrowser), "destroy", GTK_SIGNAL_FUNC(gtk_widget_destroyed), &path_dirbrowser);
		gtk_window_set_transient_for(GTK_WINDOW(path_dirbrowser), GTK_WINDOW(configure_win));
		gtk_widget_show(path_dirbrowser);
	}
}

static void configure_ok_cb(gpointer data)
{
	ConfigFile *cfgfile;

	if (file_path)
		g_free(file_path);
	file_path = g_strdup(gtk_entry_get_text(GTK_ENTRY(path_entry)));

	use_suffix =
		gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(use_suffix_toggle));

	cfgfile = xmms_cfg_open_default_file();
	if (!cfgfile)
		cfgfile = xmms_cfg_new();

	xmms_cfg_write_string(cfgfile, "disk_writer", "file_path", file_path);
	xmms_cfg_write_boolean(cfgfile, "disk_writer", "use_suffix", use_suffix);
	xmms_cfg_write_default_file(cfgfile);
	xmms_cfg_free(cfgfile);

	gtk_widget_destroy(configure_win);
	if (path_dirbrowser)
		gtk_widget_destroy(path_dirbrowser);
}

static void configure_destroy(void)
{
	if (path_dirbrowser)
		gtk_widget_destroy(path_dirbrowser);
}

static void disk_configure(void)
{
	GtkTooltips *use_suffix_tooltips;

	if (!configure_win)
	{
		configure_win = gtk_window_new(GTK_WINDOW_TOPLEVEL);

		gtk_signal_connect(GTK_OBJECT(configure_win), "destroy", GTK_SIGNAL_FUNC(configure_destroy), NULL);
		gtk_signal_connect(GTK_OBJECT(configure_win), "destroy", GTK_SIGNAL_FUNC(gtk_widget_destroyed), &configure_win);
		gtk_window_set_title(GTK_WINDOW(configure_win), _("Disk Writer Configuration"));
		gtk_window_set_position(GTK_WINDOW(configure_win), GTK_WIN_POS_MOUSE);

		gtk_container_set_border_width(GTK_CONTAINER(configure_win), 10);

		configure_vbox = gtk_vbox_new(FALSE, 10);
		gtk_container_add(GTK_CONTAINER(configure_win), configure_vbox);

		path_hbox = gtk_hbox_new(FALSE, 5);
		gtk_box_pack_start(GTK_BOX(configure_vbox), path_hbox, FALSE, FALSE, 0);

		path_label = gtk_label_new(_("Path:"));
		gtk_box_pack_start(GTK_BOX(path_hbox), path_label, FALSE, FALSE, 0);
		gtk_widget_show(path_label);

		path_entry = gtk_entry_new();
		if (file_path)
			gtk_entry_set_text(GTK_ENTRY(path_entry), file_path);
		gtk_widget_set_usize(path_entry, 200, -1);
		gtk_box_pack_start(GTK_BOX(path_hbox), path_entry, TRUE, TRUE, 0);
		gtk_widget_show(path_entry);

		path_browse = gtk_button_new_with_label(_("Browse"));
		gtk_signal_connect(GTK_OBJECT(path_browse), "clicked", GTK_SIGNAL_FUNC(path_browse_cb), NULL);
		gtk_box_pack_start(GTK_BOX(path_hbox), path_browse, FALSE, FALSE, 0);
		gtk_widget_show(path_browse);

		gtk_widget_show(path_hbox);

		use_suffix_toggle = gtk_check_button_new_with_label(_("Don't strip file name extension"));
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(use_suffix_toggle), use_suffix);
		gtk_box_pack_start(GTK_BOX(configure_vbox), use_suffix_toggle, FALSE, FALSE, 0);
		use_suffix_tooltips = gtk_tooltips_new();
		gtk_tooltips_set_tip(use_suffix_tooltips, use_suffix_toggle, "If enabled, the extension from the original filename will not be stripped before adding the .wav extension to the end.", NULL);
		gtk_tooltips_enable(use_suffix_tooltips);
		gtk_widget_show(use_suffix_toggle);

		configure_separator = gtk_hseparator_new();
		gtk_box_pack_start(GTK_BOX(configure_vbox), configure_separator, FALSE, FALSE, 0);
		gtk_widget_show(configure_separator);

		configure_bbox = gtk_hbutton_box_new();
		gtk_button_box_set_layout(GTK_BUTTON_BOX(configure_bbox), GTK_BUTTONBOX_END);
		gtk_button_box_set_spacing(GTK_BUTTON_BOX(configure_bbox), 5);
		gtk_box_pack_start(GTK_BOX(configure_vbox), configure_bbox, FALSE, FALSE, 0);

		configure_ok = gtk_button_new_with_label(_("OK"));
		gtk_signal_connect(GTK_OBJECT(configure_ok), "clicked", GTK_SIGNAL_FUNC(configure_ok_cb), NULL);
		GTK_WIDGET_SET_FLAGS(configure_ok, GTK_CAN_DEFAULT);
		gtk_box_pack_start(GTK_BOX(configure_bbox), configure_ok, TRUE, TRUE, 0);
		gtk_widget_show(configure_ok);
		gtk_widget_grab_default(configure_ok);

		configure_cancel = gtk_button_new_with_label(_("Cancel"));
		gtk_signal_connect_object(GTK_OBJECT(configure_cancel), "clicked", GTK_SIGNAL_FUNC(gtk_widget_destroy), GTK_OBJECT(configure_win));
		GTK_WIDGET_SET_FLAGS(configure_cancel, GTK_CAN_DEFAULT);
		gtk_box_pack_start(GTK_BOX(configure_bbox), configure_cancel, TRUE, TRUE, 0);
		gtk_widget_show(configure_cancel);
		gtk_widget_show(configure_bbox);
		gtk_widget_show(configure_vbox);
		gtk_widget_show(configure_win);
	}
}
