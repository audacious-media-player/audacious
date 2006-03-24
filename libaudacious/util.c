#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include <glib.h>
#include <gtk/gtk.h>


GtkWidget *
xmms_show_message(const gchar * title, const gchar * text,
                  const gchar * button_text, gboolean modal,
                  GtkSignalFunc button_action, gpointer action_data)
{
  GtkWidget *dialog;
  GtkWidget *dialog_vbox, *dialog_hbox, *dialog_bbox;
  GtkWidget *dialog_bbox_b1;
  GtkWidget *dialog_textlabel;
  GtkWidget *dialog_icon;

  dialog = gtk_window_new(GTK_WINDOW_TOPLEVEL);
  gtk_window_set_type_hint( GTK_WINDOW(dialog) , GDK_WINDOW_TYPE_HINT_DIALOG );
  gtk_window_set_modal( GTK_WINDOW(dialog) , modal );
  gtk_window_set_title( GTK_WINDOW(dialog) , title );
  gtk_container_set_border_width( GTK_CONTAINER(dialog) , 10 );

  dialog_vbox = gtk_vbox_new( FALSE , 0 );
  dialog_hbox = gtk_hbox_new( FALSE , 0 );

  /* icon */
  dialog_icon = gtk_image_new_from_stock( GTK_STOCK_DIALOG_INFO , GTK_ICON_SIZE_DIALOG );
  gtk_box_pack_start( GTK_BOX(dialog_hbox) , dialog_icon , FALSE , FALSE , 2 );

  /* label */
  dialog_textlabel = gtk_label_new( text );
  /* gtk_label_set_selectable( GTK_LABEL(dialog_textlabel) , TRUE ); */
  gtk_box_pack_start( GTK_BOX(dialog_hbox) , dialog_textlabel , TRUE , TRUE , 2 );

  gtk_box_pack_start( GTK_BOX(dialog_vbox) , dialog_hbox , FALSE , FALSE , 2 );
  gtk_box_pack_start( GTK_BOX(dialog_vbox) , gtk_hseparator_new() , FALSE , FALSE , 4 );

  dialog_bbox = gtk_hbutton_box_new();
  gtk_button_box_set_layout( GTK_BUTTON_BOX(dialog_bbox) , GTK_BUTTONBOX_END );
  dialog_bbox_b1 = gtk_button_new_with_label( button_text );
  g_signal_connect_swapped( G_OBJECT(dialog_bbox_b1) , "clicked" ,
                            G_CALLBACK(gtk_widget_destroy) , dialog );
  if ( button_action )
    g_signal_connect( G_OBJECT(dialog_bbox_b1) , "clicked" ,
                      button_action , action_data );
  GTK_WIDGET_SET_FLAGS( dialog_bbox_b1 , GTK_CAN_DEFAULT);
  gtk_widget_grab_default( dialog_bbox_b1 );

  gtk_container_add( GTK_CONTAINER(dialog_bbox) , dialog_bbox_b1 );
  gtk_box_pack_start( GTK_BOX(dialog_vbox) , dialog_bbox , FALSE , FALSE , 0 );

  gtk_container_add( GTK_CONTAINER(dialog) , dialog_vbox );
  gtk_widget_show_all(dialog);

  return dialog;
}

gboolean
xmms_check_realtime_priority(void)
{
    return FALSE;
}

void
xmms_usleep(gint usec)
{
    g_usleep(usec);
}
