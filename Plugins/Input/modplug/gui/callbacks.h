#include <gtk/gtk.h>


gboolean
hide_window                            (GtkWidget       *widget,
                                        GdkEvent        *event,
                                        gpointer         user_data);

void
on_about_close_clicked                 (GtkButton       *button,
                                        gpointer         user_data);

void
on_config_ok_clicked                   (GtkButton       *button,
                                        gpointer         user_data);

void
on_config_apply_clicked                (GtkButton       *button,
                                        gpointer         user_data);

void
on_config_cancel_clicked               (GtkButton       *button,
                                        gpointer         user_data);

void
on_info_close_clicked                  (GtkButton       *button,
                                        gpointer         user_data);
