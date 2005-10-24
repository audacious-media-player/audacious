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
    /* FIXME: improper border spacing, for some reason vbox and
     * action_area not aligned, button_text totally ignored */

    GtkWidget *dialog, *box, *button;
    GtkWidget *scrolledwindow, *textview;
    GtkTextBuffer *textbuffer;

    dialog = gtk_dialog_new();
    gtk_window_set_modal(GTK_WINDOW(dialog), modal);
    gtk_window_set_title(GTK_WINDOW(dialog), title);
    gtk_window_set_default_size(GTK_WINDOW(dialog), 460, 400);

    box = GTK_DIALOG(dialog)->vbox;

    scrolledwindow = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolledwindow),
                                   GTK_POLICY_AUTOMATIC,
                                   GTK_POLICY_AUTOMATIC);
    gtk_box_pack_start(GTK_BOX(box), scrolledwindow, TRUE, TRUE, 0);

    textview = gtk_text_view_new();
    gtk_text_view_set_editable(GTK_TEXT_VIEW(textview), FALSE);
    gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(scrolledwindow),
                                          textview);

    textbuffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(textview));
    gtk_text_buffer_set_text(GTK_TEXT_BUFFER(textbuffer), text, -1);

    button = gtk_dialog_add_button(GTK_DIALOG(dialog), GTK_STOCK_CLOSE,
                                   GTK_RESPONSE_CLOSE);
    g_signal_connect_swapped(button, "clicked",
                             G_CALLBACK(gtk_widget_destroy), dialog);
    if (button_action)
        g_signal_connect(button, "clicked", button_action, action_data);

    GTK_WIDGET_SET_FLAGS(button, GTK_CAN_DEFAULT);
    gtk_widget_grab_default(button);

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
