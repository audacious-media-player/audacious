/*
*
* Author: Giacomo Lozito <james@develia.org>, (C) 2005-2006
*
* This program is free software; you can redistribute it and/or modify it
* under the terms of the GNU General Public License as published by the
* Free Software Foundation; either version 2 of the License, or (at your
* option) any later version.
*
* This program is distributed in the hope that it will be useful, but
* WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
* General Public License for more details.
*
* You should have received a copy of the GNU General Public License along
* with this program; if not, write to the Free Software Foundation, Inc.,
* 59 Temple Place, Suite 330, Boston, MA 02111-1307  USA
*
*/


#include "i_utils.h"
#include "amidi-plug.logo.xpm"

static amidiplug_gui_about_t amidiplug_gui_about = { NULL };

void i_about_ev_destroy( void );
void i_about_ev_bok( void );


/* count the number of occurrencies of a specific character 'c'
   in the string 'string' (it must be a null-terminated string) */
gint i_util_str_count( gchar * string , gchar c )
{
  gint i = 0 , count = 0;
  while ( string[i] != '\0' )
  {
    if ( string[i] == c )
      ++count;
    ++i;
  }
  return count;
}


void i_about_gui( void )
{
  GtkWidget *logoandinfo_vbox , *aboutwin_vbox;
  GtkWidget *logo_image , *logo_frame;
  GtkWidget *info_frame , *info_scrolledwin , *info_textview;
  GtkWidget *hseparator , *hbuttonbox , *button_ok;
  GtkTextBuffer *info_textbuffer;
  GdkPixbuf *logo_pixbuf;

  if ( amidiplug_gui_about.about_win )
    return;

  amidiplug_gui_about.about_win = gtk_window_new( GTK_WINDOW_TOPLEVEL );
  gtk_window_set_type_hint( GTK_WINDOW(amidiplug_gui_about.about_win), GDK_WINDOW_TYPE_HINT_DIALOG );
  gtk_window_set_title( GTK_WINDOW(amidiplug_gui_about.about_win), "AMIDI-Plug - about" );
  gtk_window_set_resizable( GTK_WINDOW(amidiplug_gui_about.about_win) , FALSE );
  gtk_container_set_border_width( GTK_CONTAINER(amidiplug_gui_about.about_win), 10 );
  g_signal_connect( G_OBJECT(amidiplug_gui_about.about_win) ,
                    "destroy" , G_CALLBACK(i_about_ev_destroy) , NULL );

  aboutwin_vbox = gtk_vbox_new( FALSE , 0 );

  logoandinfo_vbox = gtk_vbox_new( TRUE , 2 );
  gtk_container_add( GTK_CONTAINER(amidiplug_gui_about.about_win) , aboutwin_vbox );

  logo_pixbuf = gdk_pixbuf_new_from_xpm_data( (const gchar **)amidiplug_xpm_logo );
  logo_image = gtk_image_new_from_pixbuf( logo_pixbuf );
  g_object_unref( logo_pixbuf );

  logo_frame = gtk_frame_new( NULL );
  gtk_container_add( GTK_CONTAINER(logo_frame) , logo_image );
  gtk_box_pack_start( GTK_BOX(logoandinfo_vbox) , logo_frame , TRUE , TRUE , 0 );

  info_textview = gtk_text_view_new();
  info_textbuffer = gtk_text_view_get_buffer( GTK_TEXT_VIEW(info_textview) );
  gtk_text_view_set_editable( GTK_TEXT_VIEW(info_textview) , FALSE );
  gtk_text_view_set_cursor_visible( GTK_TEXT_VIEW(info_textview) , FALSE );
  gtk_text_view_set_justification( GTK_TEXT_VIEW(info_textview) , GTK_JUSTIFY_LEFT );
  gtk_text_view_set_left_margin( GTK_TEXT_VIEW(info_textview) , 10 );

  gtk_text_buffer_set_text( info_textbuffer ,
                            "\nAMIDI-Plug " AMIDIPLUG_VERSION
                            "\nplay MIDI music through the ALSA sequencer\n"
                            "http://www.develia.org/projects.php?p=amidiplug\n\n"
                            "written by Giacomo Lozito\n"
                            "< james@develia.org >\n\n\n"
                            "special thanks to...\n\n"
                            "Clemens Ladisch and Jaroslav Kysela\n"
                            "for their cool programs aplaymidi and amixer; those\n"
                            "were really useful, along with alsa-lib docs, in order\n"
                            "to learn more about the ALSA API\n\n"
                            "Alfredo Spadafina\n"
                            "for the nice midi keyboard logo\n\n"
                            "Tony Vroon\n"
                            "for the good help with alpha testing\n\n"
                            , -1 );

  info_scrolledwin = gtk_scrolled_window_new( NULL , NULL );
  gtk_scrolled_window_set_policy( GTK_SCROLLED_WINDOW(info_scrolledwin) ,
                                  GTK_POLICY_NEVER , GTK_POLICY_ALWAYS );
  gtk_container_add( GTK_CONTAINER(info_scrolledwin) , info_textview );
  info_frame = gtk_frame_new( NULL );
  gtk_container_add( GTK_CONTAINER(info_frame) , info_scrolledwin );

  gtk_box_pack_start( GTK_BOX(logoandinfo_vbox) , info_frame , TRUE , TRUE , 0 );

  gtk_box_pack_start( GTK_BOX(aboutwin_vbox) , logoandinfo_vbox , TRUE , TRUE , 0 );

  /* horizontal separator and buttons */
  hseparator = gtk_hseparator_new();
  gtk_box_pack_start( GTK_BOX(aboutwin_vbox) , hseparator , FALSE , FALSE , 4 );
  hbuttonbox = gtk_hbutton_box_new();
  gtk_button_box_set_layout( GTK_BUTTON_BOX(hbuttonbox) , GTK_BUTTONBOX_END );
  button_ok = gtk_button_new_from_stock( GTK_STOCK_OK );
  g_signal_connect( G_OBJECT(button_ok) , "clicked" , G_CALLBACK(i_about_ev_bok) , NULL );
  gtk_container_add( GTK_CONTAINER(hbuttonbox) , button_ok );
  gtk_box_pack_start( GTK_BOX(aboutwin_vbox) , hbuttonbox , FALSE , FALSE , 0 );

  gtk_widget_show_all( amidiplug_gui_about.about_win );
}


void i_about_ev_destroy( void )
{
  amidiplug_gui_about.about_win = NULL;
}


void i_about_ev_bok( void )
{
  gtk_widget_destroy(amidiplug_gui_about.about_win);
}
