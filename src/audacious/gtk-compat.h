/* Compatibility macros to make supporting multiple GTK versions easier.
 * Public domain. */

#ifndef AUD_GTK_COMPAT_H
#define AUD_GTK_COMPAT_H

#include <string.h>

#if defined GDK_KEY_Tab && ! defined GDK_Tab
#include <gdk/gdkkeysyms-compat.h>
#endif

#if ! GTK_CHECK_VERSION (2, 14, 0)
#define gtk_adjustment_get_page_size(a) ((a)->page_size)
#define gtk_adjustment_get_upper(a) ((a)->upper)
#define gtk_dialog_get_action_area(d) ((d)->action_area)
#define gtk_dialog_get_content_area(d) ((d)->vbox)
#define gtk_selection_data_get_data(s) ((s)->data)
#define gtk_selection_data_get_length(s) ((s)->length)
#define gtk_widget_get_window(w) ((w)->window)
#endif

#if ! GTK_CHECK_VERSION (2, 18, 0)

static inline void gtk_widget_set_can_default (GtkWidget * w, gboolean b)
{
    if (b)
	GTK_WIDGET_SET_FLAGS (w, GTK_CAN_DEFAULT);
    else
	GTK_WIDGET_UNSET_FLAGS (w, GTK_CAN_DEFAULT);
}

static inline void gtk_widget_set_can_focus (GtkWidget * w, gboolean b)
{
    if (b)
	GTK_WIDGET_SET_FLAGS (w, GTK_CAN_FOCUS);
    else
	GTK_WIDGET_UNSET_FLAGS (w, GTK_CAN_FOCUS);
}

#define gtk_widget_get_allocation(w, a) memcpy ((a), & (w)->allocation, sizeof (GtkAllocation))
#define gtk_widget_get_sensitive GTK_WIDGET_SENSITIVE
#define gtk_widget_get_visible GTK_WIDGET_VISIBLE
#define gtk_widget_is_toplevel GTK_WIDGET_TOPLEVEL
#endif

#if ! GTK_CHECK_VERSION (2, 20, 0)
#define gtk_widget_is_drawable GTK_WIDGET_DRAWABLE
#endif

#if ! GTK_CHECK_VERSION (3, 0, 0)

static inline void gdk_window_get_geometry_compat (GdkWindow * win, int * x,
 int * y, int * w, int * h)
{
    gdk_window_get_geometry (win, x, y, w, h, NULL);
}

#define GtkComboBoxText GtkComboBox
#define gdk_window_get_geometry gdk_window_get_geometry_compat
#define gtk_combo_box_text_new gtk_combo_box_new_text
#define gtk_combo_box_text_new_with_entry gtk_combo_box_entry_new_text
#define gtk_combo_box_text_append_text gtk_combo_box_append_text
#endif

#if GTK_CHECK_VERSION (3, 0, 0)
#define gtk_range_set_update_policy(...)
#endif

#endif /* AUD_GTK_COMPAT_H */
