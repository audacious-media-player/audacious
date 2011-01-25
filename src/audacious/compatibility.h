#if ! GLIB_CHECK_VERSION (2, 14, 0)
#define G_QUEUE_INIT {NULL, NULL, 0}
#define g_queue_clear(q) do {g_list_free ((q)->head); (q)->head = NULL; (q)->tail = NULL; (q)->length = 0;} while (0)
#define g_timeout_add_seconds(s, f, d) g_timeout_add (1000 * (s), (f), (d))
#endif

#ifdef GTK_CHECK_VERSION /* GTK headers included? */

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

#endif
