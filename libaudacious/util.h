#ifndef XMMS_UTIL_H
#define XMMS_UTIL_H


#include <glib.h>
#include <gtk/gtk.h>


/* XMMS names */

#define bmp_info_dialog(title, text, button_text, model, button_action, action_data) \
  xmms_show_message(title, text, button_text, model, button_action, action_data)

#define bmp_usleep(usec) \
  xmms_usleep(usec)

#define bmp_check_realtime_priority() \
  xmms_check_realtime_priority()


G_BEGIN_DECLS

GtkWidget *xmms_show_message(const gchar * title, const gchar * text,
                             const gchar * button_text, gboolean modal,
                             GtkSignalFunc button_action,
                             gpointer action_data);
gboolean xmms_check_realtime_priority(void);
void xmms_usleep(gint usec);

G_END_DECLS

#endif
