#if GLIB_MAJOR_VERSION == 2 && GLIB_MINOR_VERSION < 14
    #define g_timeout_add_seconds(seconds, func, data) g_timeout_add \
     (1000 * (seconds), func, data)
#endif

#if ! GTK_CHECK_VERSION (2, 10, 0)
#define GDK_WINDOW_TYPE_HINT_TOOLTIP GDK_WINDOW_TYPE_HINT_MENU
#endif

#if ! GTK_CHECK_VERSION (2, 12, 0)
#define gtk_widget_set_tooltip_text(...)
#endif

#if ! GTK_CHECK_VERSION (2, 14, 0)
#define gtk_widget_get_window(w) ((w)->window)
#endif

#if ! GTK_CHECK_VERSION (2, 18, 0)
#define gtk_widget_set_can_default(w, t) do {if (t) GTK_WIDGET_SET_FLAGS ((w), GTK_CAN_DEFAULT); else GTK_WIDGET_UNSET_FLAGS ((w), GTK_CAN_DEFAULT);} while (0)
#define gtk_widget_set_can_focus(w, t) do {if (t) GTK_WIDGET_SET_FLAGS ((w), GTK_CAN_FOCUS); else GTK_WIDGET_UNSET_FLAGS ((w), GTK_CAN_FOCUS);} while (0)
#endif
