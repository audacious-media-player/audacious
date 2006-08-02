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
* 51 Franklin Street, Fifth Floor, Boston, MA 02111-1307  USA
*
*/


#include "i_utils.h"
#include <gtk/gtk.h>
#include "amidi-plug.logo.xpm"


void i_about_gui( void )
{
  static GtkWidget * aboutwin = NULL;
  GtkWidget *logoandinfo_vbox , *aboutwin_vbox;
  GtkWidget *logo_image , *logo_frame;
  GtkWidget *info_frame , *info_scrolledwin , *info_textview;
  GtkWidget *hseparator , *hbuttonbox , *button_ok;
  GtkTextBuffer *info_textbuffer;
  GdkPixbuf *logo_pixbuf;

  if ( aboutwin != NULL )
    return;

  aboutwin = gtk_window_new( GTK_WINDOW_TOPLEVEL );
  gtk_window_set_type_hint( GTK_WINDOW(aboutwin), GDK_WINDOW_TYPE_HINT_DIALOG );
  gtk_window_set_title( GTK_WINDOW(aboutwin), "AMIDI-Plug - about" );
  gtk_window_set_resizable( GTK_WINDOW(aboutwin) , FALSE );
  gtk_container_set_border_width( GTK_CONTAINER(aboutwin), 10 );
  g_signal_connect( G_OBJECT(aboutwin) , "destroy" , G_CALLBACK(gtk_widget_destroyed) , &aboutwin );

  aboutwin_vbox = gtk_vbox_new( FALSE , 0 );

  logoandinfo_vbox = gtk_vbox_new( TRUE , 2 );
  gtk_container_add( GTK_CONTAINER(aboutwin) , aboutwin_vbox );

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
                            "\nmodular MIDI music player\n"
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
  g_signal_connect_swapped( G_OBJECT(button_ok) , "clicked" , G_CALLBACK(gtk_widget_destroy) , aboutwin );
  gtk_container_add( GTK_CONTAINER(hbuttonbox) , button_ok );
  gtk_box_pack_start( GTK_BOX(aboutwin_vbox) , hbuttonbox , FALSE , FALSE , 0 );

  gtk_widget_show_all( aboutwin );
}


gpointer i_message_gui( gchar * title , gchar * message ,
                        gint type , gpointer parent_win )
{
  GtkWidget *win;
  GtkMessageType mtype = GTK_MESSAGE_INFO;

  switch ( type )
  {
    case AMIDIPLUG_MESSAGE_INFO:
      mtype = GTK_MESSAGE_INFO; break;
    case AMIDIPLUG_MESSAGE_WARN:
      mtype = GTK_MESSAGE_WARNING; break;
    case AMIDIPLUG_MESSAGE_ERR:
      mtype = GTK_MESSAGE_ERROR; break;
  }

  if ( parent_win != NULL )
    win = gtk_message_dialog_new( GTK_WINDOW(parent_win) , GTK_DIALOG_DESTROY_WITH_PARENT ,
                                  mtype , GTK_BUTTONS_OK , message );
  else
    win = gtk_message_dialog_new( NULL , 0 , mtype , GTK_BUTTONS_OK , message );

  gtk_window_set_title( GTK_WINDOW(win) , title );
  g_signal_connect_swapped( G_OBJECT(win) , "response" , G_CALLBACK(gtk_widget_destroy) , win );

  return win;
}
