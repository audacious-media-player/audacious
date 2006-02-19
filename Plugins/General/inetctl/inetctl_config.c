/* inetctl_config.c - Configuration services */

#include "inetctl.h"

/* U/I defines */
GtkWidget *conf_dialog;
GtkObject *hour_w, *minute_w, *fadespeed_o;
GtkWidget *port_field;
gint hour, minute, fadespeed;
gchar *dummy;


void alert(gchar *message) {
   GtkWidget *dialog, *label, *okay_button;
   
   /* Create the widgets */
   dialog = gtk_dialog_new();
   label = gtk_label_new (message);
   okay_button = gtk_button_new_with_label("Okay");
   
   /* Ensure that the dialog box is destroyed when the user clicks ok. */
   gtk_signal_connect_object (GTK_OBJECT (okay_button), "clicked",
                              GTK_SIGNAL_FUNC (gtk_widget_destroy), GTK_OBJECT(dialog));
   gtk_container_add (GTK_CONTAINER (GTK_DIALOG(dialog)->action_area),
                      okay_button);
   
   /* Add the label, and show everything we've added to the dialog. */
   gtk_container_add (GTK_CONTAINER (GTK_DIALOG(dialog)->vbox), label);
   gtk_widget_show_all (dialog);
}

/* Write current configuration dialog values to config file */
void write_config() {
  ConfigDb *db = bmp_cfg_db_open();

  /* Extract current values from U/I */
  listenPort = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(port_field));

  /* Write values to config and close it */
  bmp_cfg_db_set_int(db, "inetctl", "port", listenPort);
  bmp_cfg_db_close(db);
}

/* Read values from config file and install.  If no config file */
/* then install sensible default values                         */
void read_config() {
  ConfigDb *db = bmp_cfg_db_open();
  bmp_cfg_db_get_int(db, "inetctl", "port", &listenPort);
  bmp_cfg_db_close(db);
}

void inetctl_config_ok (GtkWidget * wid, gpointer data) {
  write_config();
  gtk_widget_destroy (conf_dialog);
  conf_dialog = NULL;
  return;
}

void inetctl_config () {
  GtkWidget *ok_button, *apply_button, *cancel_button;
  GtkWidget *timebox, *port_w;
  GtkWidget *bigbox, *buttonbox;
  GtkWidget *timeframe;


  if (conf_dialog) return;

  conf_dialog = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  
  gtk_window_set_title (GTK_WINDOW (conf_dialog), ("iNetControl Configuration"));
  gtk_window_set_policy (GTK_WINDOW (conf_dialog), FALSE, FALSE, FALSE);
  gtk_window_set_position (GTK_WINDOW (conf_dialog), GTK_WIN_POS_MOUSE);
  
  gtk_container_set_border_width (GTK_CONTAINER (conf_dialog), 5);
  
  gtk_signal_connect (GTK_OBJECT (conf_dialog), "destroy", GTK_SIGNAL_FUNC (gtk_widget_destroyed),
		      &conf_dialog);
  
  bigbox = gtk_vbox_new (FALSE, 5);
  gtk_container_add (GTK_CONTAINER (GTK_WINDOW (conf_dialog)), bigbox);
  
  timeframe = gtk_frame_new ("Server options:");
  gtk_container_add (GTK_CONTAINER (bigbox), timeframe);
  
  timebox = gtk_hbox_new (FALSE, 5);
  gtk_container_set_border_width (GTK_CONTAINER (timebox), 5);
  gtk_container_add (GTK_CONTAINER (timeframe), timebox);
  
  port_w = (GtkWidget *) gtk_adjustment_new (listenPort, 1, 256 * 256, 1, 1, 1);
  port_field = gtk_spin_button_new (GTK_ADJUSTMENT (port_w), 1.0, 0);
  
  gtk_box_pack_start (GTK_BOX (timebox), gtk_label_new ("Port: "), FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (timebox), port_field, TRUE, TRUE, 0);
  
  buttonbox = gtk_hbutton_box_new ();
  gtk_button_box_set_layout (GTK_BUTTON_BOX (buttonbox), GTK_BUTTONBOX_END);
  gtk_button_box_set_spacing (GTK_BUTTON_BOX (buttonbox), 5);
  gtk_box_pack_start (GTK_BOX (bigbox), buttonbox, FALSE, FALSE, 0);
  
  ok_button = gtk_button_new_with_label ("Ok");
  apply_button = gtk_button_new_with_label ("Apply");
  cancel_button = gtk_button_new_with_label ("Cancel");
  
  gtk_signal_connect_object (GTK_OBJECT (cancel_button), "clicked", GTK_SIGNAL_FUNC (gtk_widget_destroy),
			     (gpointer) conf_dialog);
  
  gtk_signal_connect_object (GTK_OBJECT (apply_button), "clicked", GTK_SIGNAL_FUNC (write_config), NULL);
  gtk_signal_connect_object (GTK_OBJECT (ok_button), "clicked", GTK_SIGNAL_FUNC (inetctl_config_ok), NULL);
  
  GTK_WIDGET_SET_FLAGS (ok_button, GTK_CAN_DEFAULT);
  GTK_WIDGET_SET_FLAGS (cancel_button, GTK_CAN_DEFAULT);
  GTK_WIDGET_SET_FLAGS (apply_button, GTK_CAN_DEFAULT);
  
  gtk_box_pack_start (GTK_BOX (buttonbox), ok_button, TRUE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (buttonbox), cancel_button, TRUE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (buttonbox), apply_button, TRUE, TRUE, 0);
  
  gtk_widget_show_all (conf_dialog);
}

void inetctl_about () {
  GtkWindow *about_box = (GtkWindow *) gtk_window_new(GTK_WINDOW_TOPLEVEL);
  GtkVBox *container = (GtkVBox *) gtk_vbox_new(FALSE, 10);
  GtkLabel *copyright = (GtkLabel *) gtk_label_new("(c) 2001 Gerry Duprey");
  GtkLabel *email = (GtkLabel *) gtk_label_new("gerry@cdp1802.org");
  GtkLabel *url = (GtkLabel *) gtk_label_new("http://www.cdp1802.org");
  
  gtk_container_set_border_width(GTK_CONTAINER(container), 10);
  gtk_window_set_title(about_box, "iNetControl");
  
  gtk_container_add(GTK_CONTAINER(container), GTK_WIDGET(copyright));
  gtk_container_add(GTK_CONTAINER(container), GTK_WIDGET(email));
  gtk_container_add(GTK_CONTAINER(container), GTK_WIDGET(url));
  gtk_container_add(GTK_CONTAINER(about_box), GTK_WIDGET(container));
  
  gtk_widget_show(GTK_WIDGET(copyright));
  gtk_widget_show(GTK_WIDGET(container));
  gtk_widget_show(GTK_WIDGET(url));
  gtk_widget_show(GTK_WIDGET(email));
  gtk_widget_show(GTK_WIDGET(about_box));
}
