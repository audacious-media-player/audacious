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


#include "i_fileinfo.h"
/* this is needed to retrieve information */
#include "i_midi.h"
/* icon from gnome-mime-audio-midi.png of the GNOME ICON SET */
#include "amidi-plug.midiicon.xpm"

static amidiplug_gui_fileinfo_t amidiplug_gui_fileinfo = { NULL };


void i_fileinfo_ev_destroy( GtkWidget * win , gpointer mf )
{
  i_midi_free( (midifile_t *)mf );
  g_free( mf );
  amidiplug_gui_fileinfo.fileinfo_win = NULL;
}


void i_fileinfo_ev_close( void )
{
  gtk_widget_destroy( amidiplug_gui_fileinfo.fileinfo_win );
}


void i_fileinfo_table_add_entry( gchar * field_text , gchar * value_text ,
                                 GtkWidget * table , guint line , PangoAttrList * attrlist )
{
  GtkWidget *field, *value;
  field = gtk_label_new( field_text );
  gtk_label_set_attributes( GTK_LABEL(field) , attrlist );
  gtk_misc_set_alignment( GTK_MISC(field) , 0 , 0 );
  gtk_label_set_justify( GTK_LABEL(field) , GTK_JUSTIFY_LEFT );
  gtk_table_attach( GTK_TABLE(table) , field , 0 , 1 , line , (line + 1) ,
                    GTK_FILL , GTK_FILL , 5 , 2 );
  value = gtk_label_new( value_text );
  gtk_misc_set_alignment( GTK_MISC(value) , 0 , 0 );
  gtk_label_set_justify( GTK_LABEL(value) , GTK_JUSTIFY_LEFT );
  gtk_table_attach( GTK_TABLE(table) , value , 1 , 2 , line , (line + 1) ,
                    GTK_FILL , GTK_FILL , 5 , 2 );
  return;
}


void i_fileinfo_gui( gchar * filename )
{
  GtkWidget *fileinfowin_vbox;
  GtkWidget *title_hbox , *title_icon_image , *title_name_f_label , *title_name_v_entry;
  GtkWidget *info_frame , *info_table;
  GtkWidget *footer_hbbox, *footer_bclose;
  GdkPixbuf *title_icon_pixbuf;
  PangoAttrList *pangoattrlist;
  PangoAttribute *pangoattr;
  GString *value_gstring;
  gchar *title , *filename_utf8;
  gint bpm = 0, wavg_bpm = 0;
  midifile_t * mf = g_malloc(sizeof(midifile_t));

  if ( amidiplug_gui_fileinfo.fileinfo_win )
    return;

  /****************** midifile parser ******************/
  if ( !i_midi_parse_from_filename( filename , mf ) )
    return;
  /* midifile is filled with information at this point,
     bpm information is needed too */
  i_midi_get_bpm( mf , &bpm , &wavg_bpm );
  /*****************************************************/

  amidiplug_gui_fileinfo.fileinfo_win = gtk_window_new( GTK_WINDOW_TOPLEVEL );
  gtk_window_set_type_hint( GTK_WINDOW(amidiplug_gui_fileinfo.fileinfo_win), GDK_WINDOW_TYPE_HINT_DIALOG );
  gtk_window_set_resizable( GTK_WINDOW(amidiplug_gui_fileinfo.fileinfo_win) , FALSE );
  gtk_window_set_position( GTK_WINDOW(amidiplug_gui_fileinfo.fileinfo_win) , GTK_WIN_POS_CENTER );
  g_signal_connect( G_OBJECT(amidiplug_gui_fileinfo.fileinfo_win) ,
                    "destroy" , G_CALLBACK(i_fileinfo_ev_destroy) , mf );
  gtk_container_set_border_width( GTK_CONTAINER(amidiplug_gui_fileinfo.fileinfo_win), 10 );

  fileinfowin_vbox = gtk_vbox_new( FALSE , 10 );
  gtk_container_add( GTK_CONTAINER(amidiplug_gui_fileinfo.fileinfo_win) , fileinfowin_vbox );

  /* pango attributes */
  pangoattrlist = pango_attr_list_new();
  pangoattr = pango_attr_weight_new( PANGO_WEIGHT_BOLD );
  pangoattr->start_index = 0;
  pangoattr->end_index = G_MAXINT;
  pango_attr_list_insert( pangoattrlist , pangoattr );

  /******************
   *** TITLE LINE ***/
  title_hbox = gtk_hbox_new( FALSE , 5 );
  gtk_box_pack_start( GTK_BOX(fileinfowin_vbox) , title_hbox , FALSE , FALSE , 0 );

  title_icon_pixbuf = gdk_pixbuf_new_from_xpm_data( (const gchar **)amidiplug_xpm_midiicon );
  title_icon_image = gtk_image_new_from_pixbuf( title_icon_pixbuf );
  g_object_unref( title_icon_pixbuf );
  gtk_misc_set_alignment( GTK_MISC(title_icon_image) , 0 , 0 );
  gtk_box_pack_start( GTK_BOX(title_hbox) , title_icon_image , FALSE , FALSE , 0 );

  title_name_f_label = gtk_label_new( _("Name:") );
  gtk_label_set_attributes( GTK_LABEL(title_name_f_label) , pangoattrlist );
  gtk_box_pack_start( GTK_BOX(title_hbox) , title_name_f_label , FALSE , FALSE , 0 );

  title_name_v_entry = gtk_entry_new();
  gtk_editable_set_editable( GTK_EDITABLE(title_name_v_entry) , FALSE );
  gtk_widget_set_size_request( GTK_WIDGET(title_name_v_entry) , 200 , -1 );
  gtk_box_pack_start(GTK_BOX(title_hbox) , title_name_v_entry , TRUE , TRUE , 0 );

  /*********************
   *** MIDI INFO BOX ***/
  info_frame = gtk_frame_new( _(" MIDI Info ") );
  gtk_box_pack_start( GTK_BOX(fileinfowin_vbox) , info_frame , TRUE , TRUE , 0 );
  info_table = gtk_table_new( 6 , 2 , FALSE );
  gtk_container_set_border_width( GTK_CONTAINER(info_table) , 5 );
  gtk_container_add( GTK_CONTAINER(info_frame) , info_table );
  value_gstring = g_string_new( "" );

  /* midi format */
  G_STRING_PRINTF( value_gstring , "type %i" , mf->format );
  i_fileinfo_table_add_entry( _("Format:") , value_gstring->str , info_table , 0 , pangoattrlist );
  /* midi length */
  G_STRING_PRINTF( value_gstring , "%i" , (gint)(mf->length / 1000) );
  i_fileinfo_table_add_entry( _("Length (msec):") , value_gstring->str , info_table , 1 , pangoattrlist );
  /* midi num of tracks */
  G_STRING_PRINTF( value_gstring , "%i" , mf->num_tracks );
  i_fileinfo_table_add_entry( _("Num of Tracks:") , value_gstring->str , info_table , 2 , pangoattrlist );
  /* midi bpm */
  if ( bpm > 0 )
    G_STRING_PRINTF( value_gstring , "%i" , bpm ); /* fixed bpm */
  else
    G_STRING_PRINTF( value_gstring , _("variable") ); /* variable bpm */
  i_fileinfo_table_add_entry( _("BPM:") , value_gstring->str , info_table , 3 , pangoattrlist );
  /* midi weighted average bpm */
  if ( bpm > 0 )
    G_STRING_PRINTF( value_gstring , "/" ); /* fixed bpm, don't care about wavg_bpm */
  else
    G_STRING_PRINTF( value_gstring , "%i" , wavg_bpm ); /* variable bpm, display wavg_bpm */
  i_fileinfo_table_add_entry( _("BPM (wavg):") , value_gstring->str , info_table , 4 , pangoattrlist );
  /* midi time division */
  G_STRING_PRINTF( value_gstring , "%i" , mf->time_division );
  i_fileinfo_table_add_entry( _("Time Div:") , value_gstring->str , info_table , 5 , pangoattrlist );

  g_string_free( value_gstring , TRUE );

  /**************
   *** FOOTER ***/
  footer_hbbox = gtk_hbutton_box_new();
  gtk_button_box_set_layout( GTK_BUTTON_BOX(footer_hbbox) , GTK_BUTTONBOX_END );
  footer_bclose = gtk_button_new_from_stock( GTK_STOCK_CLOSE );
  g_signal_connect( G_OBJECT(footer_bclose) , "clicked" , G_CALLBACK(i_fileinfo_ev_close) , NULL );
  gtk_container_add( GTK_CONTAINER(footer_hbbox) , footer_bclose );
  gtk_box_pack_start( GTK_BOX(fileinfowin_vbox) , footer_hbbox , FALSE , FALSE , 0 );


  /* utf8-ize filename and set window title */
  filename_utf8 = g_strdup(g_filename_to_utf8( filename , -1 , NULL , NULL , NULL ));
  if ( !filename_utf8 )
  {
    /* utf8 fallback */
    gchar *chr , *convert_str = g_strdup( filename );
    for ( chr = convert_str ; *chr ; chr++ )
    {
      if ( *chr & 0x80 )
        *chr = '?';
    }
    filename_utf8 = g_strconcat( convert_str , _("  (invalid UTF-8)") , NULL );
    g_free(convert_str);
  }
  title = g_strdup_printf( "%s - " PLAYER_NAME , g_basename(filename_utf8));
  gtk_window_set_title( GTK_WINDOW(amidiplug_gui_fileinfo.fileinfo_win) , title);
  g_free(title);
  /* set the text for the filename header too */
  gtk_entry_set_text( GTK_ENTRY(title_name_v_entry) , filename_utf8 );
  gtk_editable_set_position( GTK_EDITABLE(title_name_v_entry) , -1 );
  g_free(filename_utf8);

  gtk_widget_grab_focus( GTK_WIDGET(footer_bclose) );
  gtk_widget_show_all( amidiplug_gui_fileinfo.fileinfo_win );
}
