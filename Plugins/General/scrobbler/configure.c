#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>

#include <glib.h>
#include <glib/gi18n.h>

#include <gdk/gdkkeysyms.h>
#include <gtk/gtk.h>

#include "configure.h"

#define GLADE_HOOKUP_OBJECT(component,widget,name) \
  g_object_set_data_full (G_OBJECT (component), name, \
    gtk_widget_ref (widget), (GDestroyNotify) gtk_widget_unref)

#define GLADE_HOOKUP_OBJECT_NO_REF(component,widget,name) \
  g_object_set_data (G_OBJECT (component), name, widget)

GtkWidget*
create_window1 (void)
{
  GtkWidget *window1;
  GtkWidget *vbox2;
  GtkWidget *table1;
  GtkWidget *entry2;
  GtkWidget *label3;
  GtkWidget *hseparator2;
  GtkWidget *hseparator3;
  GtkWidget *label1;
  GtkWidget *label2;
  GtkWidget *entry1;
  GtkWidget *hseparator1;
  GtkWidget *hbuttonbox1;
  GtkWidget *button5;
  GtkWidget *button6;

  window1 = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  gtk_container_set_border_width (GTK_CONTAINER (window1), 12);
  gtk_window_set_title (GTK_WINDOW (window1), _("window1"));

  vbox2 = gtk_vbox_new (FALSE, 0);
  gtk_widget_show (vbox2);
  gtk_container_add (GTK_CONTAINER (window1), vbox2);

  table1 = gtk_table_new (4, 2, FALSE);
  gtk_widget_show (table1);
  gtk_box_pack_start (GTK_BOX (vbox2), table1, FALSE, FALSE, 0);

  entry2 = gtk_entry_new ();
  gtk_widget_show (entry2);
  gtk_table_attach (GTK_TABLE (table1), entry2, 1, 2, 3, 4,
                    (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);

  label3 = gtk_label_new (_("Password:"));
  gtk_widget_show (label3);
  gtk_table_attach (GTK_TABLE (table1), label3, 0, 1, 3, 4,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);
  gtk_label_set_justify (GTK_LABEL (label3), GTK_JUSTIFY_RIGHT);
  gtk_misc_set_alignment (GTK_MISC (label3), 1, 0.5);

  hseparator2 = gtk_hseparator_new ();
  gtk_widget_show (hseparator2);
  gtk_table_attach (GTK_TABLE (table1), hseparator2, 0, 1, 1, 2,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);

  hseparator3 = gtk_hseparator_new ();
  gtk_widget_show (hseparator3);
  gtk_table_attach (GTK_TABLE (table1), hseparator3, 1, 2, 1, 2,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);

  label1 = gtk_label_new (_("<b>Scrobbler Preferences</b>"));
  gtk_widget_show (label1);
  gtk_table_attach (GTK_TABLE (table1), label1, 0, 1, 0, 1,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);
  gtk_label_set_use_markup (GTK_LABEL (label1), TRUE);
  gtk_misc_set_alignment (GTK_MISC (label1), 0, 0.5);

  label2 = gtk_label_new (_("Username:"));
  gtk_widget_show (label2);
  gtk_table_attach (GTK_TABLE (table1), label2, 0, 1, 2, 3,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);
  gtk_label_set_justify (GTK_LABEL (label2), GTK_JUSTIFY_RIGHT);
  gtk_misc_set_alignment (GTK_MISC (label2), 1, 0.5);

  entry1 = gtk_entry_new ();
  gtk_widget_show (entry1);
  gtk_table_attach (GTK_TABLE (table1), entry1, 1, 2, 2, 3,
                    (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);

  hseparator1 = gtk_hseparator_new ();
  gtk_widget_show (hseparator1);
  gtk_box_pack_start (GTK_BOX (vbox2), hseparator1, FALSE, FALSE, 0);

  hbuttonbox1 = gtk_hbutton_box_new ();
  gtk_widget_show (hbuttonbox1);
  gtk_box_pack_start (GTK_BOX (vbox2), hbuttonbox1, FALSE, FALSE, 0);
  gtk_button_box_set_layout (GTK_BUTTON_BOX (hbuttonbox1), GTK_BUTTONBOX_END);
  gtk_box_set_spacing (GTK_BOX (hbuttonbox1), 5);

  button5 = gtk_button_new_from_stock ("gtk-ok");
  gtk_widget_show (button5);
  gtk_container_add (GTK_CONTAINER (hbuttonbox1), button5);
  GTK_WIDGET_SET_FLAGS (button5, GTK_CAN_DEFAULT);

  button6 = gtk_button_new_from_stock ("gtk-close");
  gtk_widget_show (button6);
  gtk_container_add (GTK_CONTAINER (hbuttonbox1), button6);
  GTK_WIDGET_SET_FLAGS (button6, GTK_CAN_DEFAULT);

  /* Store pointers to all widgets, for use by lookup_widget(). */
  GLADE_HOOKUP_OBJECT_NO_REF (window1, window1, "window1");
  GLADE_HOOKUP_OBJECT (window1, vbox2, "vbox2");
  GLADE_HOOKUP_OBJECT (window1, table1, "table1");
  GLADE_HOOKUP_OBJECT (window1, entry2, "entry2");
  GLADE_HOOKUP_OBJECT (window1, label3, "label3");
  GLADE_HOOKUP_OBJECT (window1, hseparator2, "hseparator2");
  GLADE_HOOKUP_OBJECT (window1, hseparator3, "hseparator3");
  GLADE_HOOKUP_OBJECT (window1, label1, "label1");
  GLADE_HOOKUP_OBJECT (window1, label2, "label2");
  GLADE_HOOKUP_OBJECT (window1, entry1, "entry1");
  GLADE_HOOKUP_OBJECT (window1, hseparator1, "hseparator1");
  GLADE_HOOKUP_OBJECT (window1, hbuttonbox1, "hbuttonbox1");
  GLADE_HOOKUP_OBJECT (window1, button5, "button5");
  GLADE_HOOKUP_OBJECT (window1, button6, "button6");

  return window1;
}

