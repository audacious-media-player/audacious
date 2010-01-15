#if GLIB_MAJOR_VERSION == 2 && GLIB_MINOR_VERSION < 14
    #define g_timeout_add_seconds(seconds, func, data) g_timeout_add \
     (1000 * (seconds), func, data)
#endif

#if GTK_MAJOR_VERSION == 2 && GTK_MINOR_VERSION < 10
    #define GDK_WINDOW_TYPE_HINT_TOOLTIP GDK_WINDOW_TYPE_HINT_MENU
#endif

#if GTK_MAJOR_VERSION == 2 && GTK_MINOR_VERSION < 12
    #define gtk_widget_set_tooltip_text(...)
#endif

