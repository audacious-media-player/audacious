/*
 * Audacious: A cross-platform multimedia player
 * Copyright (c) 2006 William Pitcock, Tony Vroon, George Averill,
 *                    Giacomo Lozito, Derek Pomery and Yoshiki Yazawa.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; under version 2 of the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301, USA.
 */

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include <glib.h>
#include <glib/gi18n.h>
#include <gtk/gtk.h>
#include <string.h>

#include "titlestring.h"
#include "ui_fileinfopopup.h"
#include "main.h"
#include "ui_main.h"
#include "playlist.h"
#include "playback.h"


static void
filepopup_entry_set_text(GtkWidget *filepopup_win, const char *entry_name, const char *text)
{
  GtkWidget *widget = g_object_get_data(G_OBJECT(filepopup_win), entry_name);

  if (widget == NULL)
    return;

  gtk_label_set_text( GTK_LABEL(widget) , text );
  return;
}

static void
filepopup_entry_set_image(GtkWidget *filepopup_win, const char *entry_name, const char *text)
{
	GtkWidget *widget = g_object_get_data(G_OBJECT(filepopup_win), entry_name);
	GdkPixbuf *pixbuf;
	int width, height;
	double aspect;
	GdkPixbuf *pixbuf2;

	if (widget == NULL)
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
		pixbuf2 = gdk_pixbuf_scale_simple(GDK_PIXBUF(pixbuf), width, height, GDK_INTERP_BILINEAR);
		g_object_unref(G_OBJECT(pixbuf));
		pixbuf = pixbuf2;
	}

	gtk_image_set_from_pixbuf(GTK_IMAGE(widget), GDK_PIXBUF(pixbuf));
	g_object_unref(G_OBJECT(pixbuf));
}

static void
filepopup_entry_set_text_free(GtkWidget *filepopup_win, const char *entry_name, char *text)
{
	GtkWidget *widget = g_object_get_data(G_OBJECT(filepopup_win), entry_name);

	if (widget == NULL)
		return;

	gtk_label_set_text(GTK_LABEL(widget), text);

	g_free(text);
}

static gboolean
has_front_cover_extension(const gchar *name)
{
	char *ext;

	ext = strrchr(name, '.');
	if (!ext) {
		/* No file extension */
		return FALSE;
	}

	return g_strcasecmp(ext, ".jpg") == 0 ||
	       g_strcasecmp(ext, ".jpeg") == 0 ||
	       g_strcasecmp(ext, ".png") == 0;
}

static gboolean
cover_name_filter(const gchar *name, const gchar *filter, const gboolean ret_on_empty)
{
	gboolean result = FALSE;
	gchar **splitted;
	gchar *current;
	gchar *lname;
	gint i;

	if (!filter || strlen(filter) == 0) {
		return ret_on_empty;
	}

	splitted = g_strsplit(filter, ",", 0);

	lname = g_strdup(name);
	g_strdown(lname);

	for (i = 0; !result && (current = splitted[i]); i++) {
		gchar *stripped = g_strstrip(g_strdup(current));
		g_strdown(stripped);

		result = result || strstr(lname, stripped);

		g_free(stripped);
	}

	g_free(lname);
	g_strfreev(splitted);

	return result;
}

/* Check wether it's an image we want */
static gboolean
is_front_cover_image(const gchar *imgfile)
{
	return cover_name_filter(imgfile, cfg.cover_name_include, TRUE) &&
	       !cover_name_filter(imgfile, cfg.cover_name_exclude, FALSE);
}

static gboolean
is_file_image(const gchar *imgfile, const gchar *file_name)
{
	char *imgfile_ext, *file_name_ext;
	size_t imgfile_len, file_name_len;

	imgfile_ext = strrchr(imgfile, '.');
	if (!imgfile_ext) {
		/* No file extension */
		return FALSE;
	}

	file_name_ext = strrchr(file_name, '.');
	if (!file_name_ext) {
		/* No file extension */
		return FALSE;
	}

	imgfile_len = (imgfile_ext - imgfile);
	file_name_len = (file_name_ext - file_name);

	if (imgfile_len == file_name_len) {
		return (g_ascii_strncasecmp(imgfile, file_name, imgfile_len) == 0);
	} else {
		return FALSE;
	}
}

static gchar*
fileinfo_recursive_get_image(const gchar* path,
	const gchar* file_name, gint depth)
{
	GDir *d;

	if (cfg.recurse_for_cover && depth > cfg.recurse_for_cover_depth)
		return NULL;
	
	d = g_dir_open(path, 0, NULL);

	if (d) {
		const gchar *f;

		if (cfg.use_file_cover && file_name) {
			/* Look for images matching file name */
			while((f = g_dir_read_name(d))) { 
				gchar *newpath = g_strconcat(path, "/", f, NULL);

				if (!g_file_test(newpath, G_FILE_TEST_IS_DIR) &&
				    has_front_cover_extension(f) &&
				    is_file_image(f, file_name)) {
					g_dir_close(d);
					return newpath;
				}

				g_free(newpath);
			}
			g_dir_rewind(d);
		}
		
		/* Search for files using filter */
		while ((f = g_dir_read_name(d))) {
			gchar *newpath = g_strconcat(path, "/", f, NULL);

			if (!g_file_test(newpath, G_FILE_TEST_IS_DIR) &&
			    has_front_cover_extension(f) &&
			    is_front_cover_image(f)) {
				g_dir_close(d);
				return newpath;
			}

			g_free(newpath);
		}
		g_dir_rewind(d);

		/* checks whether recursive or not. */
		if (!cfg.recurse_for_cover) {
			g_dir_close(d);
			return NULL;
		}

		/* Descend into directories recursively. */
		while ((f = g_dir_read_name(d))) {
			gchar *newpath = g_strconcat(path, "/", f, NULL);
			
			if(g_file_test(newpath, G_FILE_TEST_IS_DIR)) {
				gchar *tmp = fileinfo_recursive_get_image(newpath,
					NULL, depth + 1);
				if(tmp) {
					g_free(newpath);
					g_dir_close(d);
					return tmp;
				}
			}

			g_free(newpath);
		}

		g_dir_close(d);
	}

	return NULL;
}


static gboolean
audacious_fileinfopopup_progress_cb ( gpointer filepopup_win )
{
  GtkWidget *progressbar = g_object_get_data( G_OBJECT(filepopup_win) , "progressbar" );
  gchar *tooltip_file = g_object_get_data( G_OBJECT(filepopup_win) , "file" );
  gchar *current_file;
  Playlist *pl;
  gint length = GPOINTER_TO_INT(g_object_get_data( G_OBJECT(filepopup_win) , "length" ));
  gint pos;
  gint time;

  pl = playlist_get_active();
  pos = playlist_get_position(pl);

  current_file = playlist_get_filename( pl , pos );
  time = playback_get_time();

  if ( ( time != -1 ) &&
       ( length != -1 ) &&
       ( ( current_file != NULL ) && ( !strcmp(tooltip_file,current_file) ) ) )
  {
    gchar *progress_time = g_strdup_printf("%d:%02d", time / 60000, (time / 1000) % 60);
    gtk_progress_bar_set_fraction( GTK_PROGRESS_BAR(progressbar) , (gdouble)time / (gdouble)length );
    gtk_progress_bar_set_text( GTK_PROGRESS_BAR(progressbar) , progress_time );
    g_free( progress_time );
    if ( GTK_WIDGET_VISIBLE(progressbar) != TRUE )
      gtk_widget_show(progressbar);
  }
  else
  {
    /* tooltip opened, but song is not the same,
       or playback is stopped, or length is not applicabile */
    if ( GTK_WIDGET_VISIBLE(progressbar) == TRUE )
      gtk_widget_hide(progressbar);
  }

  return TRUE;
}

static gboolean
audacious_fileinfopopup_progress_check_active ( GtkWidget * filepopup_win )
{
  if ( GPOINTER_TO_INT(g_object_get_data(G_OBJECT(filepopup_win),"progress_sid")) == 0 )
    return FALSE;
  else
    return TRUE;
}

static void
audacious_fileinfopopup_progress_init ( GtkWidget * filepopup_win )
{
  g_object_set_data( G_OBJECT(filepopup_win) , "progress_sid" , GINT_TO_POINTER(0) );
}

static void
audacious_fileinfopopup_progress_start ( GtkWidget * filepopup_win )
{
  gint sid = g_timeout_add( 500 , (GSourceFunc)audacious_fileinfopopup_progress_cb , filepopup_win );
  g_object_set_data( G_OBJECT(filepopup_win) , "progress_sid" , GINT_TO_POINTER(sid) );
}

static void
audacious_fileinfopopup_progress_stop ( GtkWidget * filepopup_win )
{
  gint sid = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(filepopup_win),"progress_sid"));
  if ( sid != 0 )
  {
    g_source_remove( sid );
    g_object_set_data(G_OBJECT(filepopup_win),"progress_sid",GINT_TO_POINTER(0));
  }
}



GtkWidget *
audacious_fileinfopopup_create(void)
{
  GtkWidget *filepopup_win;
  GtkWidget *filepopup_hbox;
  GtkWidget *filepopup_data_image;
  GtkWidget *filepopup_data_table;
  GtkWidget *filepopup_data_info_header[7];
  GtkWidget *filepopup_data_info_label[7];
  GtkWidget *filepopup_progress;
  gchar *markup;

  filepopup_win = gtk_window_new( GTK_WINDOW_POPUP );
  gtk_window_set_decorated( GTK_WINDOW(filepopup_win), FALSE );
  gtk_container_set_border_width( GTK_CONTAINER(filepopup_win) , 6 );
  gtk_window_set_transient_for( GTK_WINDOW(filepopup_win) , GTK_WINDOW(mainwin) );

  filepopup_hbox = gtk_hbox_new( FALSE , 0 );
  gtk_container_add( GTK_CONTAINER(filepopup_win) , filepopup_hbox );

  filepopup_data_image = gtk_image_new();
  gtk_misc_set_alignment( GTK_MISC(filepopup_data_image) , 0.5 , 0 );
  gtk_image_set_from_file( GTK_IMAGE(filepopup_data_image) , DATA_DIR "/images/audio.png" );
  g_object_set_data( G_OBJECT(filepopup_win) , "image_artwork" , filepopup_data_image );
  g_object_set_data( G_OBJECT(filepopup_win) , "last_artwork" , NULL );
  gtk_box_pack_start( GTK_BOX(filepopup_hbox) , filepopup_data_image , FALSE , FALSE , 0 );

  gtk_box_pack_start( GTK_BOX(filepopup_hbox) , gtk_vseparator_new() , FALSE , FALSE , 6 );

  filepopup_data_table = gtk_table_new( 8 , 2 , FALSE );
  gtk_table_set_row_spacings( GTK_TABLE(filepopup_data_table) , 0 );
  gtk_table_set_col_spacings( GTK_TABLE(filepopup_data_table) , 6 );
  gtk_box_pack_start( GTK_BOX(filepopup_hbox) , filepopup_data_table , TRUE , TRUE , 0 );

  /* title */
  filepopup_data_info_header[0] = gtk_label_new("");
  filepopup_data_info_label[0] = gtk_label_new("");
  gtk_misc_set_alignment( GTK_MISC(filepopup_data_info_header[0]) , 0 , 0.5 );
  gtk_misc_set_alignment( GTK_MISC(filepopup_data_info_label[0]) , 0 , 0.5 );
  gtk_misc_set_padding( GTK_MISC(filepopup_data_info_header[0]) , 0 , 3 );
  gtk_misc_set_padding( GTK_MISC(filepopup_data_info_label[0]) , 0 , 3 );
  markup = g_markup_printf_escaped( "<span style=\"italic\">%s</span>" , _("Title") );
  gtk_label_set_markup( GTK_LABEL(filepopup_data_info_header[0]) , markup );
  g_free( markup );
  g_object_set_data( G_OBJECT(filepopup_win) , "header_title" , filepopup_data_info_header[0] );
  g_object_set_data( G_OBJECT(filepopup_win) , "label_title" , filepopup_data_info_label[0] );
  gtk_table_attach( GTK_TABLE(filepopup_data_table) , filepopup_data_info_header[0] ,
    0 , 1 , 0 , 1 , GTK_FILL , 0 , 0 , 0 );
  gtk_table_attach( GTK_TABLE(filepopup_data_table) , filepopup_data_info_label[0] ,
    1 , 2 , 0 , 1 , GTK_FILL , 0 , 0 , 0 );

  /* artist */
  filepopup_data_info_header[1] = gtk_label_new("");
  filepopup_data_info_label[1] = gtk_label_new("");
  gtk_misc_set_alignment( GTK_MISC(filepopup_data_info_header[1]) , 0 , 0.5 );
  gtk_misc_set_alignment( GTK_MISC(filepopup_data_info_label[1]) , 0 , 0.5 );
  gtk_misc_set_padding( GTK_MISC(filepopup_data_info_header[1]) , 0 , 3 );
  gtk_misc_set_padding( GTK_MISC(filepopup_data_info_label[1]) , 0 , 3 );
  markup = g_markup_printf_escaped( "<span style=\"italic\">%s</span>" , _("Artist") );
  gtk_label_set_markup( GTK_LABEL(filepopup_data_info_header[1]) , markup );
  g_free( markup );
  g_object_set_data( G_OBJECT(filepopup_win) , "header_artist" , filepopup_data_info_header[1] );
  g_object_set_data( G_OBJECT(filepopup_win) , "label_artist" , filepopup_data_info_label[1] );
  gtk_table_attach( GTK_TABLE(filepopup_data_table) , filepopup_data_info_header[1] ,
    0 , 1 , 1 , 2 , GTK_FILL , 0 , 0 , 0 );
  gtk_table_attach( GTK_TABLE(filepopup_data_table) , filepopup_data_info_label[1] ,
    1 , 2 , 1 , 2 , GTK_FILL , 0 , 0 , 0 );

  /* album */
  filepopup_data_info_header[2] = gtk_label_new("");
  filepopup_data_info_label[2] = gtk_label_new("");
  gtk_misc_set_alignment( GTK_MISC(filepopup_data_info_header[2]) , 0 , 0.5 );
  gtk_misc_set_alignment( GTK_MISC(filepopup_data_info_label[2]) , 0 , 0.5 );
  gtk_misc_set_padding( GTK_MISC(filepopup_data_info_header[2]) , 0 , 3 );
  gtk_misc_set_padding( GTK_MISC(filepopup_data_info_label[2]) , 0 , 3 );
  markup = g_markup_printf_escaped( "<span style=\"italic\">%s</span>" , _("Album") );
  gtk_label_set_markup( GTK_LABEL(filepopup_data_info_header[2]) , markup );
  g_free( markup );
  g_object_set_data( G_OBJECT(filepopup_win) , "header_album" , filepopup_data_info_header[2] );
  g_object_set_data( G_OBJECT(filepopup_win) , "label_album" , filepopup_data_info_label[2] );
  gtk_table_attach( GTK_TABLE(filepopup_data_table) , filepopup_data_info_header[2] ,
    0 , 1 , 2 , 3 , GTK_FILL , 0 , 0 , 0 );
  gtk_table_attach( GTK_TABLE(filepopup_data_table) , filepopup_data_info_label[2] ,
    1 , 2 , 2 , 3 , GTK_FILL , 0 , 0 , 0 );

  /* genre */
  filepopup_data_info_header[3] = gtk_label_new("");
  filepopup_data_info_label[3] = gtk_label_new("");
  gtk_misc_set_alignment( GTK_MISC(filepopup_data_info_header[3]) , 0 , 0.5 );
  gtk_misc_set_alignment( GTK_MISC(filepopup_data_info_label[3]) , 0 , 0.5 );
  gtk_misc_set_padding( GTK_MISC(filepopup_data_info_header[3]) , 0 , 3 );
  gtk_misc_set_padding( GTK_MISC(filepopup_data_info_label[3]) , 0 , 3 );
  markup = g_markup_printf_escaped( "<span style=\"italic\">%s</span>" , _("Genre") );
  gtk_label_set_markup( GTK_LABEL(filepopup_data_info_header[3]) , markup );
  g_free( markup );
  g_object_set_data( G_OBJECT(filepopup_win) , "header_genre" , filepopup_data_info_header[3] );
  g_object_set_data( G_OBJECT(filepopup_win) , "label_genre" , filepopup_data_info_label[3] );
  gtk_table_attach( GTK_TABLE(filepopup_data_table) , filepopup_data_info_header[3] ,
    0 , 1 , 3 , 4 , GTK_FILL , 0 , 0 , 0 );
  gtk_table_attach( GTK_TABLE(filepopup_data_table) , filepopup_data_info_label[3] ,
    1 , 2 , 3 , 4 , GTK_FILL , 0 , 0 , 0 );

  /* year */
  filepopup_data_info_header[4] = gtk_label_new("");
  filepopup_data_info_label[4] = gtk_label_new("");
  gtk_misc_set_alignment( GTK_MISC(filepopup_data_info_header[4]) , 0 , 0.5 );
  gtk_misc_set_alignment( GTK_MISC(filepopup_data_info_label[4]) , 0 , 0.5 );
  gtk_misc_set_padding( GTK_MISC(filepopup_data_info_header[4]) , 0 , 3 );
  gtk_misc_set_padding( GTK_MISC(filepopup_data_info_label[4]) , 0 , 3 );
  markup = g_markup_printf_escaped( "<span style=\"italic\">%s</span>" , _("Year") );
  gtk_label_set_markup( GTK_LABEL(filepopup_data_info_header[4]) , markup );
  g_free( markup );
  g_object_set_data( G_OBJECT(filepopup_win) , "header_year" , filepopup_data_info_header[4] );
  g_object_set_data( G_OBJECT(filepopup_win) , "label_year" , filepopup_data_info_label[4] );
  gtk_table_attach( GTK_TABLE(filepopup_data_table) , filepopup_data_info_header[4] ,
    0 , 1 , 4 , 5 , GTK_FILL , 0 , 0 , 0 );
  gtk_table_attach( GTK_TABLE(filepopup_data_table) , filepopup_data_info_label[4] ,
    1 , 2 , 4 , 5 , GTK_FILL , 0 , 0 , 0 );

  /* Track Number */
  filepopup_data_info_header[5] = gtk_label_new("");
  filepopup_data_info_label[5] = gtk_label_new("");
  gtk_misc_set_alignment( GTK_MISC(filepopup_data_info_header[5]) , 0 , 0.5 );
  gtk_misc_set_alignment( GTK_MISC(filepopup_data_info_label[5]) , 0 , 0.5 );
  gtk_misc_set_padding( GTK_MISC(filepopup_data_info_header[5]) , 0 , 3 );
  gtk_misc_set_padding( GTK_MISC(filepopup_data_info_label[5]) , 0 , 3 );
  markup = g_markup_printf_escaped( "<span style=\"italic\">%s</span>" , _("Track Number") );
  gtk_label_set_markup( GTK_LABEL(filepopup_data_info_header[5]) , markup );
  g_free( markup );
  g_object_set_data( G_OBJECT(filepopup_win) , "header_tracknum" , filepopup_data_info_header[5] );
  g_object_set_data( G_OBJECT(filepopup_win) , "label_tracknum" , filepopup_data_info_label[5] );
  gtk_table_attach( GTK_TABLE(filepopup_data_table) , filepopup_data_info_header[5] ,
    0 , 1 , 5 , 6 , GTK_FILL , 0 , 0 , 0 );
  gtk_table_attach( GTK_TABLE(filepopup_data_table) , filepopup_data_info_label[5] ,
    1 , 2 , 5 , 6 , GTK_FILL , 0 , 0 , 0 );

  /* Track Length */
  filepopup_data_info_header[6] = gtk_label_new("");
  filepopup_data_info_label[6] = gtk_label_new("");
  gtk_misc_set_alignment( GTK_MISC(filepopup_data_info_header[6]) , 0 , 0.5 );
  gtk_misc_set_alignment( GTK_MISC(filepopup_data_info_label[6]) , 0 , 0.5 );
  gtk_misc_set_padding( GTK_MISC(filepopup_data_info_header[6]) , 0 , 3 );
  gtk_misc_set_padding( GTK_MISC(filepopup_data_info_label[6]) , 0 , 3 );
  markup = g_markup_printf_escaped( "<span style=\"italic\">%s</span>" , _("Track Length") );
  gtk_label_set_markup( GTK_LABEL(filepopup_data_info_header[6]) , markup );
  g_free( markup );
  g_object_set_data( G_OBJECT(filepopup_win) , "header_tracklen" , filepopup_data_info_header[6] );
  g_object_set_data( G_OBJECT(filepopup_win) , "label_tracklen" , filepopup_data_info_label[6] );
  g_object_set_data( G_OBJECT(filepopup_win), "length" , GINT_TO_POINTER(-1) );
  gtk_table_attach( GTK_TABLE(filepopup_data_table) , filepopup_data_info_header[6] ,
    0 , 1 , 6 , 7 , GTK_FILL , 0 , 0 , 0 );
  gtk_table_attach( GTK_TABLE(filepopup_data_table) , filepopup_data_info_label[6] ,
    1 , 2 , 6 , 7 , GTK_FILL , 0 , 0 , 0 );
  gtk_table_set_row_spacing( GTK_TABLE(filepopup_data_table) , 6 , 6 );

  /* Track Progress */
  filepopup_progress = gtk_progress_bar_new();
  gtk_progress_bar_set_orientation( GTK_PROGRESS_BAR(filepopup_progress) , GTK_PROGRESS_LEFT_TO_RIGHT );
  gtk_progress_bar_set_text( GTK_PROGRESS_BAR(filepopup_progress) , "" );
  gtk_table_attach( GTK_TABLE(filepopup_data_table) , filepopup_progress ,
    0 , 2 , 7 , 8 , GTK_FILL , 0 , 0 , 0 );
  g_object_set_data( G_OBJECT(filepopup_win) , "file" , NULL );
  g_object_set_data( G_OBJECT(filepopup_win) , "progressbar" , filepopup_progress );
  audacious_fileinfopopup_progress_init( filepopup_win );

  /* this will realize all widgets contained in filepopup_hbox */
  gtk_widget_show_all(filepopup_hbox);

  /* do not show the track progress */
  gtk_widget_hide( filepopup_progress );

  return filepopup_win;
}

void
audacious_fileinfopopup_destroy(GtkWidget *filepopup_win)
{
  audacious_fileinfopopup_progress_stop( filepopup_win );

  gchar *last_artwork = g_object_get_data( G_OBJECT(filepopup_win) , "last_artwork" );
  if ( last_artwork != NULL ) g_free(last_artwork);
  gtk_widget_destroy( filepopup_win );
  return;
}

void
audacious_fileinfopopup_show_from_tuple(GtkWidget *filepopup_win, TitleInput *tuple)
{
	gchar *tmp = NULL;
	gint x, y, x_off = 3, y_off = 3, h, w;

	gchar *last_artwork = g_object_get_data( G_OBJECT(filepopup_win) , "last_artwork" );
	const static char default_artwork[] = DATA_DIR "/images/audio.png";

	if (tuple == NULL)
		return;

	if ( g_object_get_data( G_OBJECT(filepopup_win) , "file" ) != NULL )
	  g_free( g_object_get_data( G_OBJECT(filepopup_win) , "file" ) );
	if ( ( tuple->file_path ) && ( tuple->file_name ) )
	{
		g_object_set_data( G_OBJECT(filepopup_win) , "file" ,
		  g_build_filename( tuple->file_path , tuple->file_name , NULL ) );
	}

	gtk_widget_realize(filepopup_win);

	if ( tuple->track_name != NULL )
	{
	  gchar *markup = g_markup_printf_escaped( "<span style=\"italic\">%s</span>" , _("Title") );
	  gtk_label_set_markup( GTK_LABEL(g_object_get_data(G_OBJECT(filepopup_win),"header_title")), markup );
	  g_free( markup );
        filepopup_entry_set_text(filepopup_win, "label_title", tuple->track_name);
	}
	else
	{
        /* display the filename if track_name is not available */
	  gchar *markup = g_markup_printf_escaped( "<span style=\"italic\">%s</span>" , _("Filename") );
	  gtk_label_set_markup( GTK_LABEL(g_object_get_data(G_OBJECT(filepopup_win),"header_title")), markup );
	  g_free( markup );
        filepopup_entry_set_text(filepopup_win, "label_title", tuple->file_name);
	}

	if ( tuple->performer != NULL )
	{
	  filepopup_entry_set_text(filepopup_win, "label_artist", tuple->performer);
	  gtk_widget_show(GTK_WIDGET(g_object_get_data(G_OBJECT(filepopup_win),"header_artist")));
	  gtk_widget_show(GTK_WIDGET(g_object_get_data(G_OBJECT(filepopup_win),"label_artist")));
	}
	else
	{
	  gtk_widget_hide(GTK_WIDGET(g_object_get_data(G_OBJECT(filepopup_win),"header_artist")));
	  gtk_widget_hide(GTK_WIDGET(g_object_get_data(G_OBJECT(filepopup_win),"label_artist")));
	}

	if ( tuple->album_name != NULL )
	{
	  filepopup_entry_set_text(filepopup_win, "label_album", tuple->album_name);
	  gtk_widget_show(GTK_WIDGET(g_object_get_data(G_OBJECT(filepopup_win),"header_album")));
	  gtk_widget_show(GTK_WIDGET(g_object_get_data(G_OBJECT(filepopup_win),"label_album")));
	}
	else
	{
	  gtk_widget_hide(GTK_WIDGET(g_object_get_data(G_OBJECT(filepopup_win),"header_album")));
	  gtk_widget_hide(GTK_WIDGET(g_object_get_data(G_OBJECT(filepopup_win),"label_album")));
	}

	if ( tuple->genre != NULL )
	{
	  filepopup_entry_set_text(filepopup_win, "label_genre", tuple->genre);
	  gtk_widget_show(GTK_WIDGET(g_object_get_data(G_OBJECT(filepopup_win),"header_genre")));
	  gtk_widget_show(GTK_WIDGET(g_object_get_data(G_OBJECT(filepopup_win),"label_genre")));
	}
	else
	{
	  gtk_widget_hide(GTK_WIDGET(g_object_get_data(G_OBJECT(filepopup_win),"header_genre")));
	  gtk_widget_hide(GTK_WIDGET(g_object_get_data(G_OBJECT(filepopup_win),"label_genre")));
	}

	if (tuple->length > 0)
	{
		filepopup_entry_set_text_free(filepopup_win, "label_tracklen", g_strdup_printf("%d:%02d", tuple->length / 60000, (tuple->length / 1000) % 60));
		g_object_set_data( G_OBJECT(filepopup_win), "length" , GINT_TO_POINTER(tuple->length) );
		gtk_widget_show(GTK_WIDGET(g_object_get_data(G_OBJECT(filepopup_win),"header_tracklen")));
		gtk_widget_show(GTK_WIDGET(g_object_get_data(G_OBJECT(filepopup_win),"label_tracklen")));
	}
	else
	{
		filepopup_entry_set_text(filepopup_win, "label_tracklen", "");
		g_object_set_data( G_OBJECT(filepopup_win), "length" , GINT_TO_POINTER(-1) );
		gtk_widget_hide(GTK_WIDGET(g_object_get_data(G_OBJECT(filepopup_win),"header_tracklen")));
		gtk_widget_hide(GTK_WIDGET(g_object_get_data(G_OBJECT(filepopup_win),"label_tracklen")));
	}

	if (tuple->year != 0)
	{
		filepopup_entry_set_text_free(filepopup_win, "label_year", g_strdup_printf("%d", tuple->year));
		gtk_widget_show(GTK_WIDGET(g_object_get_data(G_OBJECT(filepopup_win),"header_year")));
		gtk_widget_show(GTK_WIDGET(g_object_get_data(G_OBJECT(filepopup_win),"label_year")));
	}
	else
	{
		gtk_widget_hide(GTK_WIDGET(g_object_get_data(G_OBJECT(filepopup_win),"header_year")));
		gtk_widget_hide(GTK_WIDGET(g_object_get_data(G_OBJECT(filepopup_win),"label_year")));
	}

	if (tuple->track_number != 0)
	{
		filepopup_entry_set_text_free(filepopup_win, "label_tracknum", g_strdup_printf("%d", tuple->track_number));
		gtk_widget_show(GTK_WIDGET(g_object_get_data(G_OBJECT(filepopup_win),"header_tracknum")));
		gtk_widget_show(GTK_WIDGET(g_object_get_data(G_OBJECT(filepopup_win),"label_tracknum")));
	}
	else
	{
		filepopup_entry_set_text(filepopup_win, "label_tracknum", "");
		gtk_widget_hide(GTK_WIDGET(g_object_get_data(G_OBJECT(filepopup_win),"header_tracknum")));
		gtk_widget_hide(GTK_WIDGET(g_object_get_data(G_OBJECT(filepopup_win),"label_tracknum")));
	}

	if ( ( tuple->file_path ) && ( tuple->file_name ) )
	{
		tmp = fileinfo_recursive_get_image(tuple->file_path, tuple->file_name, 0);
		if (tmp) { // picture found
			if (!last_artwork || strcmp(last_artwork, tmp)) { // new picture
				filepopup_entry_set_image(filepopup_win, "image_artwork", tmp);
				if (last_artwork) g_free(last_artwork);
				last_artwork = tmp;
				g_object_set_data( G_OBJECT(filepopup_win) , "last_artwork" , last_artwork );
			}
			else { // same picture
			}
		}
		else { // no picture found
			if (!last_artwork || strcmp(last_artwork, default_artwork)) {
				filepopup_entry_set_image(filepopup_win, "image_artwork", default_artwork);
				if (last_artwork) g_free(last_artwork);
				last_artwork = g_strdup(default_artwork);
				g_object_set_data( G_OBJECT(filepopup_win) , "last_artwork" , last_artwork );
			}
			else {
			}
		}
	}

	/* start a timer that updates a progress bar if the tooltip
	   is shown for the song that is being currently played */
	if ( audacious_fileinfopopup_progress_check_active( filepopup_win ) == FALSE )
	{
		audacious_fileinfopopup_progress_start( filepopup_win );
		/* also run immediately the callback once, to update progressbar status  */
		audacious_fileinfopopup_progress_cb( filepopup_win );
	}

	gdk_window_get_pointer(NULL, &x, &y, NULL);
	gtk_window_get_size(GTK_WINDOW(filepopup_win), &w, &h);
	if (gdk_screen_width()-(w+3) < x) x_off = (w*-1)-3;
	if (gdk_screen_height()-(h+3) < y) y_off = (h*-1)-3;
	gtk_window_move(GTK_WINDOW(filepopup_win), x + x_off, y + y_off);

	gtk_widget_show(filepopup_win);
}

void
audacious_fileinfopopup_show_from_title(GtkWidget *filepopup_win, gchar *title)
{
	TitleInput * tuple = bmp_title_input_new();
	tuple->track_name = g_strdup(title);
	audacious_fileinfopopup_show_from_tuple( filepopup_win , tuple );
	bmp_title_input_free( tuple );
	return;
}

void
audacious_fileinfopopup_hide(GtkWidget *filepopup_win, gpointer unused)
{
	if ( GTK_WIDGET_VISIBLE(filepopup_win) == TRUE )
	{
		audacious_fileinfopopup_progress_stop( filepopup_win );

		gtk_widget_hide(filepopup_win);

		filepopup_entry_set_text(GTK_WIDGET(filepopup_win), "label_title", "");
		filepopup_entry_set_text(GTK_WIDGET(filepopup_win), "label_artist", "");
		filepopup_entry_set_text(GTK_WIDGET(filepopup_win), "label_album", "");
		filepopup_entry_set_text(GTK_WIDGET(filepopup_win), "label_genre", "");
		filepopup_entry_set_text(GTK_WIDGET(filepopup_win), "label_track", "");
		filepopup_entry_set_text(GTK_WIDGET(filepopup_win), "label_year", "");
		filepopup_entry_set_text(GTK_WIDGET(filepopup_win), "label_length", "");

		gtk_window_resize(GTK_WINDOW(filepopup_win), 1, 1);
	}
}
