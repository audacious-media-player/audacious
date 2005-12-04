#include <gtk/gtk.h>

#include "config.h"

#include "about.h"

#if 0
static GtkWidget *about_window = NULL;

static void on_button_close_clicked (GtkButton *button, gpointer user_data);
static GtkWidget* create_window_about (void);


void lv_xmms_about_show ()
{
	if (about_window != NULL) {
		gtk_widget_show (about_window);
		return;
	}

	about_window = create_window_about ();
	gtk_widget_show (about_window);
}


/*
 *
 * Private methods
 * 
 */

static void on_button_close_clicked (GtkButton *button, gpointer user_data)
{
	gtk_widget_hide (about_window);
}

GtkWidget* create_window_about (void)
{
  GtkWidget *window_about;
  GtkWidget *vbox1;
  GtkWidget *notebook_about;
  GtkWidget *scrolledwindow1;
  GtkWidget *text_about;
  GtkWidget *label_credits;
  GtkWidget *scrolledwindow2;
  GtkWidget *text_about_translators;
  GtkWidget *label_about_translators;
  GtkWidget *hseparator1;
  GtkWidget *hbox_buttons;
  GtkWidget *button_close;

  window_about = gtk_window_new (GTK_WINDOW_DIALOG);
  gtk_object_set_data (GTK_OBJECT (window_about), "window_about", window_about);
  gtk_window_set_title (GTK_WINDOW (window_about), _("About Libvisual Audacious Plugin"));
  gtk_window_set_position (GTK_WINDOW (window_about), GTK_WIN_POS_CENTER);
  gtk_window_set_default_size (GTK_WINDOW (window_about), 457, 230);

  vbox1 = gtk_vbox_new (FALSE, 0);
  gtk_widget_ref (vbox1);
  gtk_object_set_data_full (GTK_OBJECT (window_about), "vbox1", vbox1,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (vbox1);
  gtk_container_add (GTK_CONTAINER (window_about), vbox1);
  gtk_container_set_border_width (GTK_CONTAINER (vbox1), 6);

  notebook_about = gtk_notebook_new ();
  gtk_widget_ref (notebook_about);
  gtk_object_set_data_full (GTK_OBJECT (window_about), "notebook_about", notebook_about,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (notebook_about);
  gtk_box_pack_start (GTK_BOX (vbox1), notebook_about, TRUE, TRUE, 0);

  scrolledwindow1 = gtk_scrolled_window_new (NULL, NULL);
  gtk_widget_ref (scrolledwindow1);
  gtk_object_set_data_full (GTK_OBJECT (window_about), "scrolledwindow1", scrolledwindow1,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (scrolledwindow1);
  gtk_container_add (GTK_CONTAINER (notebook_about), scrolledwindow1);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolledwindow1), GTK_POLICY_NEVER, GTK_POLICY_ALWAYS);

  text_about = gtk_text_new (NULL, NULL);
  gtk_widget_ref (text_about);
  gtk_object_set_data_full (GTK_OBJECT (window_about), "text_about", text_about,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (text_about);
  gtk_container_add (GTK_CONTAINER (scrolledwindow1), text_about);
  gtk_text_insert (GTK_TEXT (text_about), NULL, NULL, NULL,
                   _("Libvisual Audacious Plugin\n\nCopyright (C) 2004, Duilio Protti <dprotti@users.sourceforge.net>\nDennis Smit <ds@nerds-incorporated.org>\n\nThe Libvisual Audacious Plugin, more information about Libvisual can be found at\nhttp://libvisual.sf.net\n"), -1);

  label_credits = gtk_label_new (_("Credits"));
  gtk_widget_ref (label_credits);
  gtk_object_set_data_full (GTK_OBJECT (window_about), "label_credits", label_credits,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (label_credits);
  gtk_notebook_set_tab_label (GTK_NOTEBOOK (notebook_about), gtk_notebook_get_nth_page (GTK_NOTEBOOK (notebook_about), 0), label_credits);

  scrolledwindow2 = gtk_scrolled_window_new (NULL, NULL);
  gtk_widget_ref (scrolledwindow2);
  gtk_object_set_data_full (GTK_OBJECT (window_about), "scrolledwindow2", scrolledwindow2,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (scrolledwindow2);
  gtk_container_add (GTK_CONTAINER (notebook_about), scrolledwindow2);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolledwindow2), GTK_POLICY_NEVER, GTK_POLICY_ALWAYS);

  text_about_translators = gtk_text_new (NULL, NULL);
  gtk_widget_ref (text_about_translators);
  gtk_object_set_data_full (GTK_OBJECT (window_about), "text_about_translators", text_about_translators,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (text_about_translators);
  gtk_container_add (GTK_CONTAINER (scrolledwindow2), text_about_translators);
  gtk_text_insert (GTK_TEXT (text_about_translators), NULL, NULL, NULL,
                   _("Brazilian Portuguese: Gustavo Sverzut Barbieri\nDutch: Dennis Smit\nFrench: Jean-Christophe Hoelt\nSpanish: Duilio Protti\n"), -1);

  label_about_translators = gtk_label_new (_("Translators"));
  gtk_widget_ref (label_about_translators);
  gtk_object_set_data_full (GTK_OBJECT (window_about), "label_about_translators", label_about_translators,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (label_about_translators);
  gtk_notebook_set_tab_label (GTK_NOTEBOOK (notebook_about), gtk_notebook_get_nth_page (GTK_NOTEBOOK (notebook_about), 1), label_about_translators);

  hseparator1 = gtk_hseparator_new ();
  gtk_widget_ref (hseparator1);
  gtk_object_set_data_full (GTK_OBJECT (window_about), "hseparator1", hseparator1,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (hseparator1);
  gtk_box_pack_start (GTK_BOX (vbox1), hseparator1, FALSE, FALSE, 6);

  hbox_buttons = gtk_hbox_new (FALSE, 0);
  gtk_widget_ref (hbox_buttons);
  gtk_object_set_data_full (GTK_OBJECT (window_about), "hbox_buttons", hbox_buttons,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (hbox_buttons);
  gtk_box_pack_start (GTK_BOX (vbox1), hbox_buttons, FALSE, FALSE, 0);

  button_close = gtk_button_new_with_label (_("Close"));
  gtk_widget_ref (button_close);
  gtk_object_set_data_full (GTK_OBJECT (window_about), "button_close", button_close,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (button_close);
  gtk_box_pack_start (GTK_BOX (hbox_buttons), button_close, FALSE, FALSE, 0);
  GTK_WIDGET_SET_FLAGS (button_close, GTK_CAN_DEFAULT);

  gtk_signal_connect (GTK_OBJECT (button_close), "clicked",
                      GTK_SIGNAL_FUNC (on_button_close_clicked),
                      NULL);

  gtk_widget_grab_default (button_close);
  return window_about;
}
#endif
